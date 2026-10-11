# DWM1004S TDoA Tag
# 23 Dec 2019
# Release 1

This is the project delivered on DWM1004C anchor hardware, running on STM32L041G6U6S MCU

Building the code.
Unpack the source code to the “dwm1004c_tdoa_tag” folder. 
In the SW IDE, choose File->Import…  and import it as General/Existing project into Workspace

Connect the board to the PC. You may require to install J-Link drivers, which could be found on the Segger web site:
https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack
Install J-Link Software and Documentation pack, which includes drivers and J-Flash Lite tool needed for reprogramming new FW binary into tag.
If it is the first run create a new GDB Segger J-Link Debugging configuration in SW IDE.

The project was compiled with the gcc compiler version 7.3.1 (7-2018-q2-update 20180622).

For further information read DWM1004C_TDoA_Tag_Software_Guide