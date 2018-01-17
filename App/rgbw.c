/*
 * rgbw.c
 *
 *  Created on: Jun 14, 2017
 *
 * 	@author Artur Balanuta <Artur [dot] Balanuta [at] Gmail [dot] com>
 */

#include "rgbw.h"
#include "io.h"

Task_Struct PWMTaskStruct;
Char TaskPWMStack[PWMTASKSTACKSIZE];


struct ColorHandlers_s ColorHandlers;
Mailbox_Handle mbox;

void PWM_Task_Fxn_Loop();





void PWM_fadeTo(PWMDevice *pwm, uint8_t duty, uint16_t time){

    int32_t range;
    int32_t increment;
    uint16_t divisions = (time < UPDATE_INTERVAL) ? 1 : time / UPDATE_INTERVAL; // mimimum delay is UPDATE_INTERVAL milliseconds
    int32_t duty_now = pwm->duty;

    if(pwm->inverted){
        range = duty - (255 - pwm->duty);
        increment = -(range/divisions);
        duty = 255 - duty;  // inverts the value (duty, is the final value)
    }else{
        range = duty - pwm->duty;
        increment = (range/divisions);
    }

    if (range == 0)
        return;

    for (uint16_t i = 0; i < time; i += UPDATE_INTERVAL){

        duty_now += increment;

        if( duty_now >= 0 && duty_now <= 255){
            PWM_setDuty(pwm->handler, duty_now/(float)255 * PWM_DUTY_FRACTION_MAX);
            pwm->duty = duty_now;
        }else{
            // adjust for any fraction errors
            PWM_setDuty(pwm->handler, duty/(float)255 * PWM_DUTY_FRACTION_MAX);
            pwm->duty = duty;
        }
        DELAY_MS(UPDATE_INTERVAL);
    }

}

void PWM_fadeRainbow(uint32_t delay){

	PWM_fadeTo(&ColorHandlers.LedGreen, 255, delay);
	PWM_fadeTo(&ColorHandlers.LedRed, 0, delay);
	PWM_fadeTo(&ColorHandlers.LedBlue, 255, delay);
	PWM_fadeTo(&ColorHandlers.LedGreen, 0, delay);
	PWM_fadeTo(&ColorHandlers.LedRed, 255, delay);
	PWM_fadeTo(&ColorHandlers.LedBlue, 0, delay);
}

void PWM_Color_Set(Color_t color)
{
    PWM_fadeTo(&ColorHandlers.LedRed,   ((color & COLOR_RED) >> 2) * 255, UPDATE_INTERVAL);
    PWM_fadeTo(&ColorHandlers.LedGreen, ((color & COLOR_GREEN) >> 1) * 255, UPDATE_INTERVAL);
    PWM_fadeTo(&ColorHandlers.LedBlue,  ((color & COLOR_BLUE)) * 255, UPDATE_INTERVAL);
}

void PWM_Pulse(Color_t color, uint16_t delay){

	switch(color){

	case COLOR_BLACK:
	    PWM_fadeTo(&ColorHandlers.LedRed, 0, delay);
	    PWM_fadeTo(&ColorHandlers.LedGreen, 0, delay);
	    PWM_fadeTo(&ColorHandlers.LedBlue, 0, delay);
	    break;

	case COLOR_RED:
		PWM_fadeTo(&ColorHandlers.LedRed, 255, delay);
		PWM_fadeTo(&ColorHandlers.LedRed, 0, delay);
		break;

	case COLOR_GREEN:
		PWM_fadeTo(&ColorHandlers.LedGreen, 255, delay);
		PWM_fadeTo(&ColorHandlers.LedGreen, 0, delay);
		break;

	case COLOR_BLUE:
		PWM_fadeTo(&ColorHandlers.LedBlue, 255, delay);
		PWM_fadeTo(&ColorHandlers.LedBlue, 0, delay);
		break;

	case COLOR_YELLOW:
		PWM_fadeTo(&ColorHandlers.LedRed, 255, delay/2);
		PWM_fadeTo(&ColorHandlers.LedGreen, 255, delay/2);
		PWM_fadeTo(&ColorHandlers.LedGreen, 0, delay/2);
		PWM_fadeTo(&ColorHandlers.LedRed, 0, delay/2);
		break;

	case COLOR_CYAN:
		PWM_fadeTo(&ColorHandlers.LedGreen, 255, delay/2);
		PWM_fadeTo(&ColorHandlers.LedBlue, 255, delay/2);
		PWM_fadeTo(&ColorHandlers.LedBlue, 0, delay/2);
		PWM_fadeTo(&ColorHandlers.LedGreen, 0, delay/2);
		break;

	case COLOR_PURPLE:
		PWM_fadeTo(&ColorHandlers.LedRed, 255, delay/2);
		PWM_fadeTo(&ColorHandlers.LedBlue, 255, delay/2);
		PWM_fadeTo(&ColorHandlers.LedBlue, 0, delay/2);
		PWM_fadeTo(&ColorHandlers.LedRed, 0, delay/2);
		break;

	case COLOR_WHITE:
		PWM_fadeTo(&ColorHandlers.LedRed, 255, delay/3);
		PWM_fadeTo(&ColorHandlers.LedGreen, 255, delay/3);
		PWM_fadeTo(&ColorHandlers.LedBlue, 255, delay/3);
		PWM_fadeTo(&ColorHandlers.LedBlue, 0, delay/3);
		PWM_fadeTo(&ColorHandlers.LedGreen, 0, delay/3);
		PWM_fadeTo(&ColorHandlers.LedRed, 0, delay/3);
		break;

	default:
		delay > UPDATE_INTERVAL ? DELAY_MS(delay) : DELAY_MS(UPDATE_INTERVAL);
	}
}

void PWM_Task_Fxn_Loop(){

    struct Command_s command;
    uint8_t prev_mode = 0;           // start in static color mode
    uint8_t color = COLOR_BLUE; // default color
    uint16_t delay = 500;       // default delay in ms

	// Initializes PWM devices
	PWM_Init_Devices();

    /* Demo PWM */
	PWM_fadeRainbow(500);

	//uartputs("# Starting Command Loop...\n");

	while(1){

	    if(!Mailbox_pend(mbox, &command, BIOS_WAIT_FOREVER)) {
	    	    System_abort("Failed to pend mailbox\n");
	    }

	    switch (command.buff[0])	// Select the operation mode
	    {


	    case 0:	// set Fixed Color
	    prev_mode = 0;
	    color = command.buff[1];
	    PWM_Color_Set((Color_t)color);
	    	break;


	    case 1:	// Random Color Mode
	    prev_mode = 1;
        // Set one of the 8 colors randomly
        PWM_Color_Set((Color_t)randr(0, 8));
	    	break;


	    case 2: // Rainbow Transition Mode (byte 1 contains delay/50)
	    prev_mode = 2;
	    	delay = 1000; //command.buff[2] * 50;
	    	while(Mailbox_getNumPendingMsgs(mbox) == 0){
	    		PWM_fadeRainbow(delay);
	    	}
	    	break;


	    case 3:	//Pulsating Color Mode (byte 1 contains the color, byte 2 contains delay/50)
	    prev_mode = command.buff[0];
	    	color = command.buff[1];
	    	delay = 750; //command.buff[2] * 50;
	    	while(Mailbox_getNumPendingMsgs(mbox) == 0){
	    		PWM_Pulse((Color_t)color, delay);
	    	}
	    	break;



	    	// SPECIAL COMMANDS

	    case 0xa:  // Next Color
	        command.buff[0] = prev_mode;
	        command.buff[1] = (color + 1) % 8;
	        Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	    break;

	    case 0xb:  // Previous Color
	        command.buff[0] = prev_mode;
	        command.buff[1] = (color - 1) % 8;
	        Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	    break;

	    case 0xc:  // Change mode
	        command.buff[0] = (prev_mode + 1) % 4; // change to the next mode
	        Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	    break;


	    default: // Colors Off
	        PWM_Pulse(COLOR_BLACK, 0);
	    	break;
	    }
	}

}




/*
 * Function that initiates the PWM devices, the LEDs are powered at LOW
 * this means their PWM value should be the inverted of the active.
 * i.e. 255 duty cycle means 0, the Mosfets are not inverted.
 */
void PWM_Init_Devices(){

	PWM_Params params;
	PWM_Params_init(&params);

	// Same as default
	params.idleLevel = PWM_IDLE_LOW;              		// Output low when PWM is not running
	params.periodUnits = PWM_PERIOD_HZ;					// Period is in Hz
	params.periodValue = 1e6;                      		// 1MHz
	params.dutyUnits = PWM_DUTY_FRACTION;   			// Duty is fraction of period
	params.dutyValue = 0 * PWM_DUTY_FRACTION_MAX; 		// % duty cycle

	ColorHandlers.LedRed.handler = 		PWM_open(Board_PWM1, &params);
    ColorHandlers.LedGreen.handler = 	PWM_open(Board_PWM2, &params);
    ColorHandlers.LedBlue.handler =  	PWM_open(Board_PWM3, &params);

    ColorHandlers.MosfetRed.handler = 	PWM_open(Board_PWM4, &params);
    ColorHandlers.MosfetGreen.handler = PWM_open(Board_PWM5, &params);
    ColorHandlers.MosfetBlue.handler = 	PWM_open(Board_PWM6, &params);
    ColorHandlers.MosfetWhite.handler = PWM_open(Board_PWM7, &params);

    if (ColorHandlers.LedRed.handler == NULL || ColorHandlers.LedGreen.handler == NULL ||
    		ColorHandlers.LedBlue.handler == NULL || ColorHandlers.MosfetRed.handler == NULL ||
			ColorHandlers.MosfetGreen.handler == NULL || ColorHandlers.MosfetBlue.handler == NULL ||
			ColorHandlers.MosfetWhite.handler == NULL){
    	System_abort("Failed to Init PWMs");
    }

    PWM_start(ColorHandlers.LedRed.handler);
    PWM_start(ColorHandlers.LedGreen.handler);
    PWM_start(ColorHandlers.LedBlue.handler);
    PWM_start(ColorHandlers.MosfetRed.handler);
    PWM_start(ColorHandlers.MosfetGreen.handler);
    PWM_start(ColorHandlers.MosfetBlue.handler);
    PWM_start(ColorHandlers.MosfetWhite.handler);


    // RGB LEDs LOW is at duty cycle 100 % (255)
    ColorHandlers.LedRed.inverted = true;
    ColorHandlers.LedRed.duty = 255;
	ColorHandlers.LedGreen.inverted = true;
	ColorHandlers.LedGreen.duty = 255;
	ColorHandlers.LedBlue.inverted = true;
	ColorHandlers.LedBlue.duty = 255;

    // Turn RGB LEDs off buy increasing duty cycle to 100%
    PWM_setDuty(ColorHandlers.LedRed.handler, 1 * PWM_DUTY_FRACTION_MAX);
    PWM_setDuty(ColorHandlers.LedGreen.handler, 1 * PWM_DUTY_FRACTION_MAX);
    PWM_setDuty(ColorHandlers.LedBlue.handler, 1 * PWM_DUTY_FRACTION_MAX);

}

void PWM_CreateTask(int priority){

    /* Create pwm Task */
    Task_Params TaskParams;
    Task_Params_init(&TaskParams);

    TaskParams.stackSize = PWMTASKSTACKSIZE;
    TaskParams.stack = &TaskPWMStack;
    TaskParams.priority = priority;
    TaskParams.instance->name = "pwm_Task";
    Task_construct(&PWMTaskStruct, (Task_FuncPtr) PWM_Task_Fxn_Loop, &TaskParams,NULL);

    // Create Mailbox before BIOS_Start
    mbox = Mailbox_create(sizeof(Command_t), MAILBOX_SIZE, NULL, NULL);
    if (mbox == NULL)
    {
        System_abort("Failed to create mailbox");
    }

}
