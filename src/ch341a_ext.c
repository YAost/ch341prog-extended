
#include "../include/ch341a_ext.h"


int32_t SpiReadSegment(uint8_t *buffer, uint32_t from, uint32_t to)
{
    int32_t ret = 0;
    ret = ch341SpiRead(buffer, from, to - from + 1);
    if (ret < 0) return ret;
    return 0;
}

int32_t SpiWriteSegment(uint8_t *buffer, uint32_t from, uint32_t to, uint32_t sectSize)
{
    int j = 0;
    int32_t ret;
    uint32_t sectFrom, sectTo;  // border of physical sector
    uint32_t sectNum = 1;   // number of physical sectors involved, 1 by default
    uint8_t *sectBuf;   // temporary buffer to keep needed physical sectors

    sectFrom = ( from / sectSize ) * sectSize; // calculate the start of first physical sector involved    0x90000
    sectTo = ( to / sectSize ) * sectSize;     // calculate the start of last physical sector involved   0xB0000
    if (sectTo == sectFrom)   // check if it's only one physical sector
        sectTo += sectSize - 1; // last element of needed sector
    else
    {
        sectNum = (( sectTo + sectSize - sectFrom ) / sectSize); // number of requested physical sections
        sectTo = sectTo + (sectSize - 1);   // setting the address of last byte
    }
    printf("Desired segment is [%06X, %06X], physical one is [%06X, %06X]\n", from, to, sectFrom, sectTo);
    printf("Total number of minimal physical sectors involved is %X\n", sectNum);
    sectBuf = (uint8_t *)malloc(sectTo - sectFrom + 1); //temporary buffer to keep the whole sector
    printf("Allocated %X bytes of memory for buf\n", sectTo - sectFrom);
    ret = SpiReadSegment(sectBuf, sectFrom, sectTo);
    if (ret < 0) return ret;
    printf("Start erasing\n");
    uint8_t timeout = 0;
    for (int i = 0; i < sectNum; i++)               //erasing number of sections
    {
        //printf("Erasing sector %06X\n", sectFrom + (sectSize*i));

        ret = SpiEraseSector(sectFrom + ( sectSize * i ), sectSize);
        if (ret < 0) goto out;
        do {
            sleep(1);
            ret = ch341ReadStatus();
            if (ret < 0) goto out;
            printf(".");
            fflush(stdout);
            timeout++;
            if (timeout == 100) break;
        } while(ret != 0);
        if (timeout == 100) {
            fprintf(stderr, "Chip erase timeout.\n");
            break;
        }
        //else
           // printf("Chip erase done!\n");
    }

    if (ret < 0) return ret;
    printf("Absolute borders are [%06X,%06X]\n", from-sectFrom, to-sectFrom+1);
    printf("First element of buffer is %X\n", sectBuf[0]);
    printf("First element of data buffer is %X\n", buffer[0]);
    for (int i = (from-sectFrom); i < to - sectFrom + 1 ; i++)
    {
        sectBuf[i] = buffer[j];
        j++;
    }
    ret = SpiWriteSector(sectBuf, sectFrom, sectTo);
    if (ret < 0) return ret;
    else printf("Writing completed!\n");
out:    return 0;
}

int32_t SpiWriteSector(uint8_t *buffer, uint32_t from, uint32_t to)
{
    int32_t ret;
    ret = ch341SpiWrite(buffer, from, to - from + 1);
    if (ret < 0) return ret;
    return 0;
}

int32_t SpiEraseSector(uint32_t addr, uint32_t sectSize)
{
    uint8_t outComm[1], inComm[1];
    uint8_t outAddr[4], inAddr[4];
    int32_t ret;
    if (retDevHandle() == NULL) return -1;
    outComm[0] = 0x06;  //Write enable
    outAddr[0] = 0xD8;
    ret = ch341SpiStream(outComm, inComm, 1);
    if (ret < 0) return ret;    //sector erase packet: SE command + address (3 bytes)
    if (sectSize == 0x10000) outAddr[0] = 0xD8;         //64k
    if (sectSize == 0x8000) outAddr[0] = 0x52;          //32k
    if (sectSize == 0x2000) outAddr[0] = 0x40;          //8k
    if (sectSize == 0x1000) outAddr[0] = 0x20;          //4k
    outAddr[1] = addr >> 16;
    outAddr[2]= (addr & 0xFF00) >> 8;
    outAddr[3]= addr & 0xFF;
    ret = ch341SpiStream(outAddr, inAddr, 4);
    if (ret < 0) return ret;
    outComm[0] = 0x04;  //Write disable
    ret = ch341SpiStream(outComm, inComm, 1);
    if (ret < 0) return ret;
    return 0;
}

int32_t SetDeviceByBus(uint8_t bus, uint8_t dev)
{
    struct libusb_device_handle *devHandle = retDevHandle();
    struct libusb_context *cntxt = NULL;
    struct libusb_device *device = NULL;
    int32_t ret;

    uint8_t  desc[0x12];

    if (devHandle != NULL) {
        fprintf(stderr, "Call ch341Release before re-configure\n");
        return -1;
    }
    ret = libusb_init(NULL);
    if(ret < 0) {
        fprintf(stderr, "Couldn't initialise libusb\n");
        return -1;
    }

    libusb_set_debug(NULL, 3);

    struct libusb_device **list;
    ssize_t cnt = libusb_get_device_list(cntxt, &list);
    if (cnt < 0) {
        printf("Error occured getting USB device list'\n");
        return -1;
    }
    for (ssize_t i = 0; i < cnt; i++)   {
        struct libusb_device *found = list[i];
        struct libusb_device_descriptor iDesc = { 0 };
        struct libusb_device_handle *interDev = NULL;
        ret = libusb_get_device_descriptor(found, &iDesc);
        if ( ret == 0 )   {
            //printf("Bus %03i Device %03i | VID:PID %04x:%04x\n", libusb_get_bus_number(found), libusb_get_device_address(found), iDesc.idVendor, iDesc.idProduct);

            if (

            (bus == 0 || dev == 0) ?
            (iDesc.idVendor == 0x1A86 && iDesc.idProduct == 0x5512)
            :
            ((libusb_get_bus_number(found) == bus) && (libusb_get_device_address(found) == dev))

                )
                {
                ret = libusb_open(found, &interDev);
                if (ret == 0)   {
                    if (found = libusb_get_device(interDev))  {
                        if(libusb_kernel_driver_active(interDev, 0))    {
                            ret = libusb_detach_kernel_driver(interDev, 0);
                            if (ret)    {
                                fprintf(stderr, "Failed to detach kernel driver: '%s'\n", strerror(-ret));

                            }
                        }
                        else
                            ret = 0;
                        if (ret == 0)   {
                            ret = libusb_claim_interface(interDev, 0);
                            if (ret == 0)   {
                                device = found;
                                printf("Device with bus number %d and address %d is handled\n", libusb_get_bus_number(found), libusb_get_device_address(found));
                                libusb_release_interface(interDev, 0);
                                libusb_close(interDev);
                                break;
                            }
                            else    {
                                //printf("^ This device is busy, looking for another\n");
                            }
                        }

                    }
                    else
                        fprintf(stderr, "Couldn't get bus number and address.\n");
                    libusb_close(interDev);
                }
                else
                    fprintf(stderr, "Couldn't open device");
            }
        }
        else {
            fprintf(stderr, "Failed to get device descriptor: '%s'\n", strerror(-ret));
        }
    }
    libusb_free_device_list(list, 1);
    ret = setDeviceHandler(device);
    if (ret < 0 )return -1;
    else return 0;

}
