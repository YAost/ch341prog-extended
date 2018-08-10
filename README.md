# ch341prog-extended
Extended command line tool based on ch341prog project. Allows to program SPI flash memory on Linux-based systems.
## Description
This tool is based on ch341prog project by setarcos (https://github.com/setarcos/ch341prog). Additional features are:
1) Ability to use specific USB device by its address
2) Using multiple devices in one system (multiple windows)
3) Setting specific size of erasable memory area
4) Writing data without pre-erasing (automatic erase)
5) Sending custom commands to SPI-device
6) Cyclic erasing of memory sectors
## Usage
These commands are available via included sample program. See
```
./ch341example --help
````
to get full description. List of additional functions:
### Specific USB device
Find out bus address of USB device via
```
lsusb
```
Set specific USB bus number using --bus and --dev
```
./ch341example --bus 3 --dev 4
```
### Smart write
Use --file to set file reading, set --blocksize to set size of erasable block. Set memory address via --from and --to:
```
./ch341example --smartwrite --file data.hex --blocksize 4096 --from 0 --to 4096
```
### Custom SPI commands
To send data from file use --file parameter. To enter bytes from terminal, use --bytes parameter. Set size of whole packet using --packet. Use --output to print result in file;
Example of reading 6 bytes of flash memory:
```
./ch341example --spi --bytes 03000000 --packet 10
```
### Cyclic erasing
Used for producing "bad" bits. Use -f to save log into file, set -g to continue previous log.
```
./ch341example -D -f file.log -g
