
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "include/ch341a_ext.h"

/*********global variables*********/
uint8_t terminateFlag = 0;                      // exit flag after locating bad bits; also used for CTRL + C purposes
char opChar = 0;                                // variable is used to prevent input of more than one activity command

/********* prototypes ************/

void sig_handler(int signo);                    // to react at user's CTRL+C combination
uint32_t countZeroes(uint8_t byte);
uint32_t countOnes(uint8_t byte);
/*********** main *************/

int main(int argc, char* argv[])
{
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        printf("Can't catch SIGINT\n");         // setting callback function for CTRL+C combination

    time_t rawTime;                             //
    time ( &rawTime);                           // time structures for logging purposes
    struct tm *tm = localtime( & rawTime);      //

    int32_t retCode = 0;                        // return code for all return-functions

    uint8_t *dataBuffer;                        // main data buffer, contents opened file, or filled with data from module
    uint8_t *outputBuffer = NULL;               // used to output data from SPI

    FILE *file;
    char *fileNameBasic;                        // name of file using basic read and write commands
    char *fileNameAdditional = NULL;            // name of file to use in Smart Write and SPI commands
    char *fileNameSpiOutput = NULL;             // name of file to output data after using SPI command

    uint32_t autoCapacity = 0;                  // capacity of memory module
    uint32_t manualCapacity = 0;                // variable to set manual capacity
    uint32_t minimalEraseBlockSize = 0x10000;    // to use in erase block functions and smart write

    uint32_t packetSize = 0;                    // is used to send SPI commands
    uint8_t *packetBytes = NULL;                // packet bytes to be sent on SPI device, converted from raw parameter string
    uint32_t numberOfBytes = 0;                 // size of packetBytes massive

    uint32_t from = 0;                          // variables for Smart Write function
    uint32_t to = 0;                            // which define reading sector
    uint32_t blockAddr = 0;                     // address of erasable block

    uint8_t continueFlag = 0;                   // continue logging flag for bit-destruction mode
    uint8_t exitFlag = 0;                       // After locating bad bits - to exit
    uint32_t itterNumber = 0;                   // number of erase itterations
    uint32_t badBits = 0;



    int bus = 0;
    uint8_t dev = 0;

    const char usage[]  =
    "\nUsage:\n"\
    " -h    --help                          Display help message\n"\
    " -i    --info                          Display current module information as capacity etc\n"\
    " -r    --read          <filename>      Read data from memory module in file\n"\
    " -w    --write         <filename>      Write data on memory module using data from file\n"\
    " -E    --erase                         Erase full memory module using single command\n"\
    " -b    --bus           <bus number>    Set specific USB bus number, use with 'dev' parameter\n"\
    " -d    --dev           <dev number>    Set specific USB device address, use with 'bus' parameter\n"\
    " -c    --capacity      <bytes>         Set specific size of memory module\n"\
    " -s    --blocksize     <bytes>         Set specific size of minimal erasable memory block, default is 65536\n"\
    " -e    --eraseblock    <address>       Erase specific data block. Set size of erasable block\n"\
    "                                           using --blocksize parameter. \n"\
    " -W    --smartwrite                    Writing data using automatic data erase. To set data use --file parameter\n"\
    "                                           Set size of erasable block see --blocksize parameter\n"\
    "                                           Set address using --from and --to commands\n"\
    " -f    --file          <filename>      Write data from file, use only with --smartwrite command and --spi\n"\
    " -o    --output        <filename>      Output result data from using --spi command into file\n"\
    " -B    --bytes         <byte string>   Fills byte string, use with --spi. No spaces between bytes are needed\n"\
    " -p    --packet        <bytes>         Set size of SPI packet to send\n"\
    " -S    --spi                           Send manual commands to SPI device. To send data from file use --file parameter.\n"\
    "                                            To manually type bytes use --bytes, set size of packet using --packet\n"\
    "                                            Set --output parameter to print output into file\n"\
    " -F    --from          <address>       Set specific address, combine with --to parameter Used in Smart Write function\n"\
    " -T    --to            <address>       See --from parameter\n"\
    " -D    --destr         <address>       Begin cyclic erasing of 64K sector to produce bad bits. Use -s to set other sector size\n"\
    "                                       Use -f to select file for logging. Use -g to continue older logging (select old file)\n"\
    " -g    --goon                          Use result of producing bad bits (number of itterrations and bits) from selected file\n\n";


    const struct option options[] =
    {

            {"bus",         required_argument,    0, 'b'},
            {"bytes",       required_argument,    0, 'B'},
            {"capacity",    required_argument,    0, 'c'},
            {"dev",         required_argument,    0, 'd'},
            {"destr",       required_argument,    0, 'D'},
            {"eraseblock",  required_argument,    0, 'e'},
            {"erasechip",   no_argument,          0, 'E'},
            {"file",        required_argument,    0, 'f'},
            {"from",        required_argument,    0, 'F'},
            {"goon",        no_argument,          0, 'g'},
            {"help",        no_argument,          0, 'h'},
            {"info",        no_argument,          0, 'i'},
            {"output",      required_argument,    0, 'o'},
            {"packet",      required_argument,    0, 'p'},
            {"read",        required_argument,    0, 'r'},
            {"blocksize",   required_argument,    0, 's'},
            {"spi",         no_argument,          0, 'S'},
            {"to",          required_argument,    0, 'T'},
            {"write",       required_argument,    0, 'w'},
            {"smartwrite",  no_argument,          0, 'W'},
            {0, 0, 0, 0}
    };
    int optionIndex = 0;         //  stores option index using getopt_long
    int result;                  //  needs to detect size of string

    while ((result = getopt_long (argc, argv, "b:B:c:d:D:e:Ef:F:ghio:p:r:s:ST:w:W", options, &optionIndex)) != - 1)  {
        switch  (result) {
            case 'b':   {
                bus = atoi(optarg);
                break;
            };

            case 'B':   {
                if ((strlen(optarg) % 2) == 0)  {
                    numberOfBytes = strlen(optarg);
                    packetBytes = (char*)malloc(numberOfBytes);
                    int j = 0;
                    for (int i = 0; i < strlen(optarg); i+=2)    {
                        char byte[2];
                        byte[0] = optarg[i];
                        byte[1] = optarg[i+1];
                        packetBytes[j] = strtol(byte, NULL, 16);
                        j++;
                    }
                }
                break;
            };

            case 'c':   {
                manualCapacity = atoi(optarg);
                break;
            };

            case 'd':   {
                dev = atoi(optarg);
                break;
            };

            case 'D':   {
                blockAddr = atoi(optarg);
                if (!opChar)    opChar = result;
                else            opChar = 'x';
                break;
            };

            case 'e':   {
                blockAddr = atoi(optarg);
                if (!opChar)    opChar = result;
                else            opChar = 'x';
                break;
            };

            case 'E':   {
                if (!opChar)    opChar = result;
                else            opChar = 'x';
                break;
            };

            case 'f':   {
                fileNameAdditional = (char*)malloc(strlen(optarg)+1);
                strcpy(fileNameAdditional, optarg);
                break;
            };

            case 'F':   {
                from = atoi(optarg);
                break;
            };

            case 'g':   {
                continueFlag = 1;
                break;
            }

            case 'h':   {
                printf(usage);
                break;
            };

            case 'i':   {
                if (!opChar)    opChar = result;
                else            opChar = 'x';
                break;
            };

            case 'o':   {
                fileNameSpiOutput = (char*)malloc(strlen(optarg)+1);
                strcpy(fileNameSpiOutput,optarg);
                break;
            };

            case 'p':   {
                packetSize = atoi(optarg);
                break;
            };

            case 'r':   {
                if (!opChar)    {
                    opChar = result;
                    fileNameBasic = (char*)malloc(strlen(optarg)+1);
                    strcpy(fileNameBasic,optarg);
                }
                else    opChar = 'x';
                break;
            };

            case 's':   {
                minimalEraseBlockSize = atoi(optarg);
                break;
            };

            case 'S':   {
                if (!opChar)    opChar = result;
                else            opChar = 'x';
                break;
            };

            case 'T':   {
                to = atoi(optarg);
                break;
            };

            case 'w':   {
                if (!opChar)    {
                    opChar = result;
                    fileNameBasic = (char*)malloc(strlen(optarg)+1);
                    strcpy(fileNameBasic,optarg);
                }
                else    opChar = 'x';
                break;
            };

            case 'W':   {
                if (!opChar)    opChar = result;
                else            opChar = 'x';
                break;
            };

            case '?': default: {
                opChar = '?';
                break;
            };
        }
    }


    if (opChar == 0)    {
        printf("No operating command was set, see --help\n");
    }

    if (opChar == 'x')  {
        printf("Only one operating command can be used\n");
        return 0;
    }

    if (opChar == '?')  {
        printf("Non-existing symbol has been used or parameter lost, get some --help to find out.\n");
        return 0;
    }

    if (opChar == 'S')  {
        if (packetBytes != NULL && fileNameAdditional != NULL)  {
            printf("SPI command mode error: using --file and --bytes at the same time is forbidden\n");
            return 0;
        }
        if (packetBytes == NULL && fileNameAdditional == NULL && packetSize == 0)    {
            printf("SPI command mode error: no parameters detected, see --help\n");
            return 0;
        }
        if (packetBytes != NULL && packetSize == 0)  {
            printf("SPI command mode error: set packet size using --packet\n");
            return 0;
        }
        if (packetBytes == NULL && packetSize != 0)  {
            printf("SPI command mode error: set packet bytes using --bytes\n");
            return 0;
        }

    }

    if (opChar == 'D')  {
        if (continueFlag && fileNameAdditional == NULL) {
            printf("Cycling erasing error: set file using -f to continue older erasing\n");
            return 0;
        }
        if (!continueFlag && fileNameAdditional == NULL) {
            printf("Cycling erasing error: set file using -f for logging purposes\n");
            return 0;
        }

    }

    if (opChar) {
        if(SetDeviceByBus(bus, dev) < 0) return -1;
        if (ch341SetStream(CH341A_STM_I2C_750K) < 0) return -1;
    }


    if (opChar == 'r' || opChar == 'w' || opChar == 'W' || opChar == 'E' || opChar == 'i')    {
        retCode = ch341SpiCapacity();
        if (retCode >= 0)   {
            autoCapacity = 1 << retCode;
            printf("Chip capacity is %d\n",autoCapacity);
            if (autoCapacity <= 0) {
                printf("Error settting automatic capacity, set manually using --capacity\n");
                goto out;
            }
        }
    }



    if (retCode < 0) goto out;

    switch (opChar) {

        case 'D':   {
            file = fopen(fileNameAdditional, "a+");                      // open file at the end of it or create new one if it doesn't exists
            if (file)   {
                /****************STARTING LINE*****************/
                if (continueFlag)   {
                    char buff[11];
                    fseek(file, -10, SEEK_END);
                    fgets(buff, 11, file);
                    badBits = strtoul(buff, NULL, 10);
                    fseek(file, -37, SEEK_END);
                    fgets(buff, 11, file);
                    itterNumber = strtoul(buff, NULL, 10);
                    fprintf(file, "\n\n");
                }

                fprintf(file, "%02d/%02d/%04d %02d:%02d:%02d ",                                              // time and date print at the begining of cycle
                        tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);

                if (continueFlag)   fprintf(file, "CONTINUE ");
                else                fprintf(file, "START ");

                fprintf(file, "sector size %dK address 0x%06X ", minimalEraseBlockSize/1024, blockAddr);       // also chosen sector size

                fprintf(file, "itteration number %010u bad bits number %010u\n", itterNumber, badBits);

                if (ferror(file))    {
                    fprintf(stderr, "Error writing file '%s'\n", fileNameAdditional);
                }

                /**************MAIN ERASING CYCLE*****************/

                dataBuffer = (uint8_t*)malloc(minimalEraseBlockSize);                // buffer for data reading (bad bits analysis)
                memset(dataBuffer, 0xFF, minimalEraseBlockSize);
                uint8_t subIterCounter = 0;                                          // erase 100 times then check for bad

                while (!exitFlag && !terminateFlag)   {

                    uint32_t cycleBadZeroes = 0;
                    uint32_t cycleBadOnes = 0;

                    if (itterNumber > 100000 && subIterCounter == 100)  {
                        retCode = SpiEraseSector(blockAddr, minimalEraseBlockSize);
                        if (retCode >= 0)   {
                            uint8_t timeout = 0;
                            do {
                                retCode = ch341ReadStatus();
                                if (retCode < 0) break;
                                if (timeout == 100) break;
                            } while(retCode != 0);
                            if (retCode >= 0 && timeout != 100) {
                                itterNumber++;
                            }
                            else {
                                printf("Error occured while erasing block\n");
                                exitFlag = 1;
                            }
                        }
                        else {
                            printf("Error occured while erasing block\n");
                            exitFlag = 1;
                        }
                        retCode = ch341SpiRead(dataBuffer, blockAddr, minimalEraseBlockSize);       // read data after erasing
                        if (retCode < 0) {
                            printf("Error occured while reading\n");
                            exitFlag = 1;
                        }
                        for (int i = 0; i < minimalEraseBlockSize && !exitFlag; i++) {              // counting bad zeroes after performing erasing (0 -> 1 - erase)
                            if (dataBuffer[i] != 0xFF)
                                cycleBadZeroes += countZeroes(dataBuffer[i]);
                        }
                        memset(dataBuffer, 0x00, minimalEraseBlockSize);
                        retCode = ch341SpiWrite(dataBuffer, blockAddr, minimalEraseBlockSize);      // write zeroes to memory
                        if (retCode < 0) {
                            printf("Error occured while writing\n");
                            exitFlag = 1;
                        }
                        retCode = ch341SpiRead(dataBuffer, blockAddr, minimalEraseBlockSize);       // read data after writing zeroes
                        if (retCode < 0) {
                            printf("Error occured while reading\n");
                            exitFlag = 1;
                        }
                        for (int i = 0; i < minimalEraseBlockSize && !exitFlag; i++) {              // counting bad ones after filling with 0 (1 -> 0 - writing)
                            if (dataBuffer[i] != 0x00)
                                cycleBadOnes += countOnes(dataBuffer[i]);
                        }
                        if (cycleBadOnes+cycleBadZeroes > badBits)  {
                            badBits = cycleBadOnes + cycleBadZeroes;
                            time ( &rawTime);
                            tm = localtime( & rawTime);
                            fprintf(file, "%02d/%02d/%04d %02d:%02d:%02d ",                                              // time and date print at the begining of cycle
                                tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
                            fprintf(file, "BAD BITS DETECT zeroes %05u ones %05u itteration number %010u ", cycleBadZeroes, cycleBadOnes, itterNumber);
                            printf("Stop? 1 - Yes / 0 - No ");
                            scanf("%d", &exitFlag);
                        }
                        subIterCounter = 0;
                    }
                    else    {
                        retCode = SpiEraseSector(blockAddr, minimalEraseBlockSize);
                        if (retCode >= 0)   {
                            uint8_t timeout = 0;
                            do {
                                retCode = ch341ReadStatus();
                                if (retCode < 0) break;
                                if (timeout == 100) break;
                            } while(retCode != 0);
                            if (retCode >= 0 && timeout != 100) {
                                itterNumber++;
                            }
                            else {
                                printf("Error occured while erasing block\n");
                                exitFlag = 1;
                            }
                        }
                        else {
                            printf("Error occured while erasing block\n");
                            exitFlag = 1;
                        }
                        memset(dataBuffer, 0x00, minimalEraseBlockSize);
                        retCode = ch341SpiWrite(dataBuffer, blockAddr, minimalEraseBlockSize);      // write zeroes to memory
                        if (retCode < 0) {
                            printf("Error occured while writing\n");
                            exitFlag = 1;
                        }
                        if (!exitFlag && itterNumber >= 100000) subIterCounter++;

                    }

                }
                /**************ENDING LINE*************************/
                time ( &rawTime);
                tm = localtime( & rawTime);
                fprintf(file, "%02d/%02d/%04d %02d:%02d:%02d ",
                        tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
                if (terminateFlag)
                    fprintf(file, "TERMINATED ");
                else
                    fprintf(file, "END ");
                fprintf(file, "itteration number %010u bad bits number %010u", itterNumber, badBits);
                if (file) fclose(file);

            }
            else
                fprintf(stderr, "Error creating file '%s'\n", fileNameAdditional);

            break;
        };

        case 'e':   {
            retCode = SpiEraseSector(blockAddr, minimalEraseBlockSize);
            if (retCode >= 0)   {
                uint8_t timeout = 0;
                printf("Erasing");
                do {
                    sleep(1);
                    retCode = ch341ReadStatus();
                    if (retCode < 0) break;
                    printf(".");
                    fflush(stdout);
                    timeout++;
                    if (timeout == 100) break;
                } while(retCode != 0);
                printf("\n");
                if (retCode >= 0) printf("Block erasing completed\n");
                else printf("Error occured while erasing block\n");
            }
            else printf("Error occured while erasing block\n");
            break;
        };

        case 'E':   {
            retCode = ch341EraseChip();
            if (retCode >= 0)   {
                uint8_t timeout = 0;
                printf("Erasing");
                do {
                    sleep(1);
                    retCode = ch341ReadStatus();
                    if (retCode < 0) break;
                    printf(".");
                    fflush(stdout);
                    timeout++;
                    if (timeout == 100) break;
                } while(retCode != 0);
                printf("\n");
                if (retCode >= 0) printf("Chip erasing completed\n");
                else printf("Error occured while erasing chip\n");
            }
            break;
        };

        case 'r':    {
            uint32_t cap;
            if (manualCapacity != 0) cap = manualCapacity;
            else                     cap = autoCapacity;
            dataBuffer = (uint8_t*)malloc(cap);
            if (!dataBuffer) {
                printf("Error allocating memory\n");
                break;
            }
            printf("Begin reading memory\n");
            retCode = ch341SpiRead(dataBuffer, 0, cap);
            if (retCode >= 0)   {
                file = fopen(fileNameBasic, "wb");
                if (file)   {
                    fwrite(dataBuffer, 1, cap, file);
                    if (ferror(file))   {
                        fprintf(stderr, "Error writing file '%s'\n", fileNameBasic);
                    }
                    else printf("Memory reading completed\n");
                    if (file) fclose(file);
                }
                else
                    fprintf(stderr, "Error creating file '%s'\n", fileNameBasic);
            }
            break;
        };

        case 'S':   {
            if (packetBytes != NULL)  {
                dataBuffer = (uint8_t*)malloc(packetSize);
                outputBuffer = (uint8_t*)malloc(packetSize);
                memset(dataBuffer, 0xFF, packetSize);
                memset(outputBuffer, 0x00, packetSize);
                for (int i = 0; i < numberOfBytes; i++) {
                    dataBuffer[i] = packetBytes[i];
                }
                retCode = ch341SpiStream(dataBuffer, outputBuffer, packetSize);
                if (retCode < 0) {
                    printf("Error occured while sending SPI command\n");
                    break;
                }
            }

            if (fileNameAdditional != NULL)  {
                file = fopen(fileNameAdditional, "rb");
                if (file)   {
                    fseek(file, 0, SEEK_END);
                    packetSize = ftell(file);
                    fseek(file, 0, SEEK_SET);
                    dataBuffer = (uint8_t*)malloc(packetSize);
                    outputBuffer = (uint8_t*)malloc(packetSize);
                    retCode = fread(dataBuffer, 1, packetSize, file);
                    if (ferror(file))   {
                        fprintf(stderr, "Error reading file '%s'.", fileNameAdditional);
                    }
                    retCode = ch341SpiStream(dataBuffer, outputBuffer, packetSize);
                    if (retCode < 0) printf("Error occured while sending SPI command\n");
                    if (file)   fclose(file);
                }
                else fprintf(stderr, "Couldn't open file '%s'.", fileNameAdditional);
            }

            if (fileNameSpiOutput != NULL)   {
                file = fopen(fileNameSpiOutput, "wb");
                if (file)   {
                    fwrite(outputBuffer, 1, packetSize, file);
                    if (ferror(file))   {
                        fprintf(stderr, "Error writing file '%s'\n", fileNameSpiOutput);
                    }
                    else printf("Sending SPI command completed\n");
                    if (file) fclose(file);
                }
                else
                    fprintf(stderr, "Error creating file '%s'\n", fileNameSpiOutput);

            }
            else    {
                for (int i = 0; i < packetSize; i++)    {
                    if (i == 0 || (i % 16 == 0)) printf("\n");
                    printf("%02X ", outputBuffer[i]);
                }
                printf("\n\n");
            }
            break;
        };

        case 'w':   {
            uint32_t cap;
            if (manualCapacity != 0) cap = manualCapacity;
            else                     cap = autoCapacity;
            dataBuffer = (uint8_t*)malloc(cap);
            if (!dataBuffer) {
                printf("Error allocating memory\n");
                break;
            }

            file = fopen(fileNameBasic, "rb");
            if (file)   {
                retCode = fread(dataBuffer, 1, cap, file);
                if (ferror(file))   {
                    fprintf(stderr, "Error reading file '%s'.", fileNameAdditional);
                }
                printf("Begin writing memory\n");
                retCode = ch341SpiWrite(dataBuffer, 0, cap);
                if (retCode < 0) printf("Error occured while writing data\n");
                else printf("Writing completed\n");
                if (file)   fclose(file);
            }
            else fprintf(stderr, "Couldn't open file '%s'.", fileNameAdditional);
            break;
        };

        case 'W':   {
            if (fileNameAdditional != NULL) {
                if ((from < to) && ((to <= manualCapacity) || (to <= autoCapacity)))  {
                    file = fopen(fileNameAdditional, "rb");
                    if (file)    {
                        fseek(file, 0, SEEK_END);
                        uint32_t fileSize = ftell(file);
                        fseek(file, 0, SEEK_SET);
                        if (fileSize == to - from)  {
                            dataBuffer = (uint8_t*)malloc(fileSize);
                            retCode = fread(dataBuffer, 1, fileSize, file);
                            if (ferror(file))   {
                                fprintf(stderr, "Error reading file '%s'.", fileNameAdditional);
                            }
                            retCode = SpiWriteSegment(dataBuffer, from, to, minimalEraseBlockSize);
                            if (retCode < 0) printf("Error occured while perfoming Smart Write\n");
                        }
                        if (file)   fclose(file);
                    }
                    else    {
                        fprintf(stderr, "Couldn't open file '%s'.", fileNameAdditional);
                    }

                }
                else
                    printf("Address is not valid, check --from and --to parameters\n");
            }
            else
                printf("Filename is not set, use --file parameter\n");

            break;
        };

    }

out:
    ch341Release();
    return 0;

}

/****************** functions *************************/

void sig_handler(int signo)  {
    if (signo == SIGINT)    {
        if (opChar == 'D')
            terminateFlag = 1;
        else
            exit(0);
    }
}

uint32_t countZeroes(uint8_t byte)    {
    uint8_t tempByte = 0xFF ^ byte;
    uint32_t cnt = 0;
    for (int i = 0; i < 8; i++) {
        if ((tempByte & 0b00000001) == 1) cnt++;
        tempByte = tempByte >> 1;
    }
    return cnt;
}

uint32_t countOnes(uint8_t byte)    {
    uint8_t tempByte = 0x00 ^ byte;
    uint32_t cnt = 0;
    for (int i = 0; i < 8; i++) {
        if ((tempByte & 0b00000001) == 1) cnt++;
        tempByte = tempByte >> 1;
    }
    return cnt;
}
