# LoRaBug_RGBWStripController
The Firmware for the [LoRaBUG RGBW Strip Controller](https://github.com/OpenChirp/LoRaBug/tree/master/Modules/LoRaBUG_RGBW_DaughterBoard) is configured to run as a class C LoRaWAN device.

Add LoRaWAN device ID, application and Network key in App/Commisioning.h

Compile using:
    
    TI v5.2.9 compiler
    XDCTools v3.32.0.06_core 
    TI-RTOS for CC13XX and CC26XX v2.21.0.06

Usage:

    // 1st byte: Mode
    // 2nd byte: Color (0-7) or Delay/50 ms (optional)

    // Modes
    // 0x00 - Fixed Color Mode (0-7)
    // 0x01 - Rainbow Mode
    // 0x02 - Pulsating Color Mode
    // 0x03 - Change to random Color (1-7)

    // 0x0A - Set Color (0-7)
    // 0x0B - Set Delay
    // 0x0C - Next Color
    // 0x0D - Previous Color
    // 0x0E - More Delay
    // 0x0F - Less Delay
    // 0x10 - Next Mode (0-3) (keeps Color and Delay)
    // 0x11 - Prev Mode (0-3) (keeps Color and Delay)
