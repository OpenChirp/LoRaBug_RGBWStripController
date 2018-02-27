/*
 *  ======= buttons ========
 *
 *  Created on: Dec 29, 2017
 *  Author:     Artur Balanuta <Artur [dot] Balanuta [at] Gmail [dot] com>
 */

#include <stdlib.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/drivers/PIN.h>

#include "buttons.h"
#include "rgbw.h"
#include "io.h"
#include <Board_LoRaBUG.h>


#define BUTTONS_STACK_SIZE 350

#define DELAY_MS(i)    Task_sleep(((i) * 1000) / Clock_tickPeriod)
#define TIME_MS(i) i*(1000/Clock_tickPeriod)

Task_Struct buttonsTaskStruct;
char buttonsTaskStack[BUTTONS_STACK_SIZE];

Mailbox_Handle mbox;

static PIN_State  navigationPinState;
static PIN_Handle navigationPinHandle;

volatile uint32_t lastPress = 0;
volatile uint8_t state = 0;

PIN_Config navigationPinTable[] = {
    Board_BTN_NEXT   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    Board_BTN_PREV   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    Board_BTN_SELECT | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};

// State 0: Changing Modes
// State 1: Changing Colors
// State 2: Change Delay

void navigationIntCallback(PIN_Handle handle, PIN_Id pinId){
    PIN_setInterrupt(navigationPinHandle, pinId|PIN_IRQ_DIS); //disable interupts on this pin

    uint32_t now = (uint32_t) Clock_getCompletedTicks();

    if (abs(now - lastPress) > TIME_MS(300)){
        lastPress = (uint32_t) Clock_getCompletedTicks();

        struct Command_s cmd;
        cmd.buff[0] = 0xFF;

        switch (state){

        case 0: // Changing Modes
            switch(pinId){
            case Board_BTN_NEXT:
                cmd.buff[0] = 0x10;
            break;
            case Board_BTN_PREV:
                cmd.buff[0] = 0x11;
            break;
            case Board_BTN_SELECT:
                state = (state + 1) % 3; //Next State
            break;
            }
        break;

        case 1: // Changing Colors
            switch(pinId){
            case Board_BTN_NEXT:
                cmd.buff[0] = 0x0C;
            break;
            case Board_BTN_PREV:
                cmd.buff[0] = 0x0D;
            break;
            case Board_BTN_SELECT:
                state = (state + 1) % 3; //Next State
            break;
            }
        break;

        case 2: // Changing Delay
            switch(pinId){
            case Board_BTN_NEXT:
                cmd.buff[0] = 0x0E;
            break;
            case Board_BTN_PREV:
                cmd.buff[0] = 0x0F;
            break;
            case Board_BTN_SELECT:
                state = (state + 1) % 3; //Next State
            break;
            }
        break;
        }
        if (cmd.buff[0] != 0xff){
            if(!Mailbox_post(mbox, &cmd, BIOS_NO_WAIT)) {
                System_printf("Failed to post on mailbox\n");
            }
        }

    }

    PIN_setInterrupt(navigationPinHandle, pinId|PIN_IRQ_NEGEDGE);
    return;
}

void setupNavpins()
{
    navigationPinHandle = PIN_open(&navigationPinState, navigationPinTable);
    if (navigationPinHandle == NULL)
    {
        System_abort("Failed to open board BTN pin\n");
    }
    if (PIN_registerIntCb(navigationPinHandle, navigationIntCallback) != PIN_SUCCESS)
    {
        System_abort("Failed to register btn int callback\n");
    }
}

void Task_Loop(){

    setupNavpins();

    while(1){
        DELAY_MS(1000);
    }

}


void BUTTONS_CreateTask(int priority){

    /* Create pwm Task */
    Task_Params TaskParams;
    Task_Params_init(&TaskParams);

    TaskParams.stackSize = BUTTONS_STACK_SIZE;
    TaskParams.stack = &buttonsTaskStack;
    TaskParams.priority = priority;
    TaskParams.instance->name = "buttons_Task";
    Task_construct(&buttonsTaskStruct, (Task_FuncPtr) Task_Loop, &TaskParams,NULL);

    // Create Mailbox before BIOS_Start
    mbox = Mailbox_create(sizeof(Command_t), MAILBOX_SIZE, NULL, NULL);
    if (mbox == NULL)
    {
        System_abort("Failed to create mailbox");
    }

}
