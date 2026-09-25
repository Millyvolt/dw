# DWM1004C SS-TWR
# 01 Oct 2020
# Release 1

This is an umbrella project supporting three different toolchains:
Keil MDK-ARM Lite 5.30, ARM Compiler 5.06 update 6 (build 750)
System Workbench for STM32 using gcc compiler version 7.3.1 20180622 (7-2018-q2-update)
Makefile Project using gcc compiler version 7.3.1 20180622 (7-2018-q2-update)

This is a  project delivered on DWM1004C hardware V3.1, running on STM32L041G6U6S MCU

This code implements a Single Sided Two Way Ranging between 2 x DWM1004C devices, one configured as Initiator, the other as Responder 
The same binary code supports both modes : Initiator and Responder
The Console Shell mode as initially implemented for TDOA tag code has been kept in this project : 
The same set of commands is supported, which allows the testing of various RF settings, such as channel, PRF, preamble length, datarate, etc...
For any settings change to be taken into account, the new configuration shall be saved into EEPROM memory by issuing a 'SAVE' command, then a 'RESET'
Make sure to reflect symetrically the same commands to the peer device thru another Console interface.

==== Building the code ====
Unpack the source code to the “DWM1004_ss_twr” folder. 

1. System Workbench for STM32
    In the SW IDE, choose File->Import…  and import it as General/Existing project into Workspace

    Connect the board to the PC. You may require to install J-Link drivers, which could be found on the Segger web site:
    https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack
    Install J-Link Software and Documentation pack, which includes drivers and J-Flash Lite tool needed for reprogramming new FW binary into tag.
    If it is the first run create a new GDB Segger J-Link Debugging configuration in SW IDE.
2. Keil MDK-ARM Lite
    Open DWM1004C_MDK.uvprojx in MDK_ARM folder: Project->Open project
    Press F7 to build
    Press F8 to load into DWM1004C module, Ctrl+F5 to start dubugging
3. Makefile
    Open shell and go to the project directory
    Run "make all"

==== New commands ====
Note : The console UART baudrate should be set to 38400 (just like the DWM1004 TDOA code)
4 new commands were added :
'INIT' : configure the device in Initiator mode (TWR_ROLE = 1 as reported by 'STAT' command)
'RESP' : configure the device in Responder mode (TWR_ROLE = 0 as reported by 'STAT' command)
'NLOS <0/1>' : to configure the LDE with LOS-optimized settings (0) or NLOS optimized settings (1) - adjust the NTM/PMULT register accordingly
'RESET' : force a software reset, typically used after changing a setting and issuing 'SAVE' commands
Note 1: the TWR role (initiator or responder), and the NLOS setting are both persistent as saved into EEPROM upon 'SAVE' command
Note 2: The user button can also be used to switch between INIT and RESP role.

=== BLINKFAST command behaviour ====
This command is now used to set the period in ms of the TWR initiation

=== STAT command ====
the 'STAT' command will display all settings plus the TWR role and the NLOS settings :

JS008F{"Info":{
"Device":"DWM1004C SS-TWR",
"Version":"5.0.0",
"Build":"Oct 10 2019 18:22:14",
"Driver":"DW1000 Device Driver Version 05.01.01"}}
JS0102{"UWB PARAM":{
"CHAN":2,
"PRF":64,
"PLEN":128,
"DATARATE":850,
"TXCODE":9,
"PAC":8,
"NSSFD":0,
"PHRMODE":0,
"SMARTPOWER":1,
"BLINKFAST":2000,
"BLINKSLOW":5000,
"RANDOMNESS":10,
"TAGIDSET":0,
"TAGID":0x0000000000000000,
"TWR_ROLE":1
"NLOS":1}}
ok

==== Example of configuration update and save ======
On device 1 : (to behave as TWR Initiator)
DATARATE 850
>ok
PLEN 1024
>ok
NSSFD 1
>ok
BLINKFAST 500
>ok
INIT
>ok
SAVE
>ok
RESET
>ok

On device 2 : (to behave as TWR Responder)
DATARATE 850
>ok
PLEN 1024
>ok
NSSFD 1
>ok
BLINKFAST 500
>ok
RESP
>ok
SAVE
>ok
RESET
>ok

From then on, the Initiator should log on the console TWR results.

==== TWR log output ==== 
The device configured as Initiator will print out TWR results on the console, following a format as below : 
Dist_cm:  411 RX:  -92.95 ClkPPM -13.80 SeqN:  65
Dist_cm:  415 RX:  -92.47 ClkPPM -13.97 SeqN:  66
Dist_cm:  406 RX:  -92.70 ClkPPM -13.73 SeqN:  67
Dist_cm:  410 RX:  -93.15 ClkPPM -13.88 SeqN:  68
Dist_cm:  359 RX:  -92.70 ClkPPM -13.89 SeqN:  69
Dist_cm:  411 RX:  -92.47 ClkPPM -13.75 SeqN:  70
Dist_cm:  401 RX:  -93.21 ClkPPM -13.50 SeqN:  71
Dist_cm:  405 RX:  -92.95 ClkPPM -13.62 SeqN:  72

Dist_cm: distance in cm
RX     : the first path level estimate in dBm (see DW1000 User Manual section 4.7)
ClkPPM : the clock offset in ppm as computed from the carrier integrator
SeqN   : Sequence number, range from 0 to 255, increments every time the initiator sends the frame. Can be used to check if some transactions are missing due to bad receprion or range too high


==== Building the code ====
Unpack the source code to the “DWM1004_ss_twr” folder. 

1. System Workbench for STM32
    In the SW IDE, choose File->Import…  and import it as General/Existing project into Workspace

    Connect the board to the PC. You may require to install J-Link drivers, which could be found on the Segger web site:
    https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack
    Install J-Link Software and Documentation pack, which includes drivers and J-Flash Lite tool needed for reprogramming new FW binary into tag.
    If it is the first run create a new GDB Segger J-Link Debugging configuration in SW IDE.
2. Keil MDK-ARM Lite
    Open DWM1004C_MDK.uvprojx in MDK_ARM folder: Project->Open project
    Press F7 to build
    Press F8 to load into DWM1004C module, Ctrl+F5 to start dubugging
3. Makefile
    Open shell and go to the project directory
    Run "make all"
