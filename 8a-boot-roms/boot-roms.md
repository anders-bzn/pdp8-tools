# PDP-8A Boot ROM images
This is a collection of boot ROM images that has been found in different M8317 boards. The OS8 SerialDisk ROM's has been produced with the help of the "create-bootrom" program.

The contents of the ROM's are the boot code itself and commands that tells where in memory it should be stored and commands to start it. When the SW omnibus signal is asserted the board will act as a console front panel and "toggle" in the boot program in the correct place in memory and start it.

* [158A2 & 159A2 with parsed content](15xA2.zip)
* [465A2 & 469A2 with parsed content](46xA2.zip)
* [Unknown content from ROMs that sat on the M8317 that I bought with parsed content](1287xx.zip)
* [OS8 SerialDisk boot proms](km8-a-serialdisk-boot-prom.zip)
