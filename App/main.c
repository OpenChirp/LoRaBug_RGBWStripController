/**
 * Created on: Dec 26, 2017
 * @author Artur Balanuta <Artur [dot] Balanuta [at] Gmail [dot] com>
 */


#include <xdc/std.h>
#include <string.h> // strlen in uartputs and LoRaWan code
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Header files */
// #include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/UART.h>

#include "board.h"
#include "rgbw.h"
#include "lora.h"
#include "buttons.h"
/*
 *  ======== main ========
 */
int main(void)


{

    /* Call board init functions */
    Board_initGeneral();
    // Board_initI2C();s
    Board_initSPI();
    Board_initUART();
    Board_initPWM();
    // Board_initWatchdog();


    /* Construct lora Task  thread  (Priority 2 (Higher) )*/
    LORA_CreateTask(2);

    /* Create task that handles PWM Devices (Priority 1)*/
    PWM_CreateTask(1);

    /* Create task that handles Buttons press (Priority 1)*/
    BUTTONS_CreateTask(1);

    // Initialize UART
    setupuart();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
