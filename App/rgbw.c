/*
 * rgbw.c
 *
 *  Created on: Jun 14, 2017
 *
 * 	@author Artur Balanuta <Artur [dot] Balanuta [at] Gmail [dot] com>
 */

#include "rgbw.h"
#include "io.h"

#define DELAY_MS(i)    Task_sleep(((i) * 1000) / Clock_tickPeriod)
#define TIME_MS(i) i*(1000/Clock_tickPeriod)
#define PWM_Color_Set(x) PWM_Pulse(x, 0)

#define MAX_DELAY 10000 //ms
#define MIN_DELAY 75    //ms
#define DELAY_CHANGE_RATIO 2

Task_Struct PWMTaskStruct;
Char TaskPWMStack[PWMTASKSTACKSIZE];


struct ColorHandlers_s c_handler;
Mailbox_Handle mbox;

void PWM_Task_Fxn_Loop();

void PWM_fadeTo(uint8_t rgb_target[3], uint32_t time){

    uint32_t update_cycles = (time < UPDATE_INTERVAL) ? 1 : time / UPDATE_INTERVAL; // mimimum delay is UPDATE_INTERVAL milliseconds

    uint8_t duty_red = (&c_handler.LedRed)->duty;
    uint8_t duty_green = (&c_handler.LedGreen)->duty;
    uint8_t duty_blue = (&c_handler.LedBlue)->duty;

    int32_t red_inc = ((int32_t)rgb_target[0] - (int32_t)duty_red)/(int32_t)update_cycles;
    int32_t green_inc = ((int32_t)rgb_target[1] - (int32_t)duty_green)/(int32_t)update_cycles;
    int32_t blue_inc = ((int32_t)rgb_target[2] - (int32_t)duty_blue)/(int32_t)update_cycles;

    for (uint32_t i = 0; i < update_cycles; i++){
        duty_red += red_inc;
        duty_green += green_inc;
        duty_blue += blue_inc;

        PWM_setDuty((&c_handler.LedRed)->handler,
                    (255-duty_red)/(float)255 * PWM_DUTY_FRACTION_MAX);
        PWM_setDuty((&c_handler.MosfetRed)->handler,
                    duty_red/(float)255 * PWM_DUTY_FRACTION_MAX);

        PWM_setDuty((&c_handler.LedGreen)->handler,
                    (255-duty_green)/(float)255 * PWM_DUTY_FRACTION_MAX);
        PWM_setDuty((&c_handler.MosfetGreen)->handler,
                    duty_green/(float)255 * PWM_DUTY_FRACTION_MAX);

        PWM_setDuty((&c_handler.LedBlue)->handler,
                    (255-duty_blue)/(float)255 * PWM_DUTY_FRACTION_MAX);
        PWM_setDuty((&c_handler.MosfetBlue)->handler,
                    duty_blue/(float)255 * PWM_DUTY_FRACTION_MAX);

        DELAY_MS(UPDATE_INTERVAL);
    }

    // Fix fractional imprecisions
    PWM_setDuty((&c_handler.LedRed)->handler,
                (255-rgb_target[0])/(float)255 * PWM_DUTY_FRACTION_MAX);
    PWM_setDuty((&c_handler.MosfetRed)->handler,
                rgb_target[0]/(float)255 * PWM_DUTY_FRACTION_MAX);

    PWM_setDuty((&c_handler.LedGreen)->handler,
                (255-rgb_target[1])/(float)255 * PWM_DUTY_FRACTION_MAX);
    PWM_setDuty((&c_handler.MosfetGreen)->handler,
                    rgb_target[1]/(float)255 * PWM_DUTY_FRACTION_MAX);

    PWM_setDuty((&c_handler.LedBlue)->handler,
                (255-rgb_target[2])/(float)255 * PWM_DUTY_FRACTION_MAX);
    PWM_setDuty((&c_handler.MosfetBlue)->handler,
                    rgb_target[2]/(float)255 * PWM_DUTY_FRACTION_MAX);

    (&c_handler.LedRed)->duty = rgb_target[0];
    (&c_handler.MosfetRed)->duty = rgb_target[0];
    (&c_handler.LedGreen)->duty = rgb_target[1];
    (&c_handler.MosfetGreen)->duty = rgb_target[1];
    (&c_handler.LedBlue)->duty = rgb_target[2];
    (&c_handler.MosfetBlue)->duty = rgb_target[2];
}




void PWM_Pulse(Color_t color, uint32_t delay){

    uint8_t target[] = {((color & COLOR_RED) >> 2) * 255,
                         ((color & COLOR_GREEN) >> 1) * 255,
                         (color & COLOR_BLUE) * 255};

    PWM_fadeTo(target, delay);
}

void PWM_fadeRainbow(uint32_t delay){

    for (uint8_t i = 1; i < 8; i++ ){
        PWM_Pulse((Color_t)i, delay);
    }

}

void PWM_Task_Fxn_Loop(){

    struct Command_s command;
    uint8_t mode, prev_mode = 2;    // start in Pulsating Mode
    uint8_t color = COLOR_RED;
    uint32_t delay = 1500;          // default delay in ms

	// Initializes PWM devices
	PWM_Init_Devices();

    /* Demo PWM */
	//PWM_fadeRainbow(500);
	PWM_Color_Set(COLOR_BLACK);
	//command.buff[0] = prev_mode;
	//Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	//uartputs("# Starting Command Loop...\n");

	while(1){

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

	    if(!Mailbox_pend(mbox, &command, BIOS_WAIT_FOREVER)) {
	    	    System_abort("Failed to pend mailbox\n");
	    }

	    mode = command.buff[0];
	    switch (mode)	// Select the operation mode
	    {

	    case 0:	// set Fixed Color
            prev_mode = mode;
            PWM_Color_Set((Color_t)color);
	    	break;

	    case 1: // Rainbow Transition Mode
            prev_mode = mode;
            while(Mailbox_getNumPendingMsgs(mbox) == 0){
                PWM_fadeRainbow(delay);
            }
	    	break;

	    case 2:	//Pulsating Color Mode
	        prev_mode = mode;
            while(Mailbox_getNumPendingMsgs(mbox) == 0){
                PWM_Pulse((Color_t)color, delay/2);
                PWM_Pulse(COLOR_BLACK, delay/2);
            }
	    	break;

	    case 3: // Change to random Color (1-7)
            color = (Color_t)randr(1, 8);
	        command.buff[0] = prev_mode;
	        Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	    break;

	    case 0xA:  // Set Color
	        if (command.buff[1] >= 0 && command.buff[1] <= 7){
	            color = command.buff[1];
	        }
	        command.buff[0] = prev_mode;
	        Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	    break;

	    case 0xB:  // Set Delay
	        if (((uint8_t)command.buff[1]) > 0){
	            delay = ((uint8_t)command.buff[1])*50;
	        }
	        command.buff[0] = prev_mode;
	        Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	    break;

	    case 0xC: // Next Color
	        color = (color + 1) % 8;
	        command.buff[0] = prev_mode;
	        Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	    break;

	    case 0xD: // Prev Color
            color = (color - 1) % 8;
            command.buff[0] = prev_mode;
            Mailbox_post(mbox, &command, BIOS_NO_WAIT);
         break;

	    case 0xE: // More Delay
	        if (delay * DELAY_CHANGE_RATIO > MAX_DELAY){
	            delay = MAX_DELAY;
	        }else{
	            delay = delay * DELAY_CHANGE_RATIO;
	        }
            command.buff[0] = prev_mode;
            Mailbox_post(mbox, &command, BIOS_NO_WAIT);
         break;

	    case 0xF: //Less Delay
            if (delay / DELAY_CHANGE_RATIO < MIN_DELAY){
                delay = MIN_DELAY;
            }else{
                delay = delay / DELAY_CHANGE_RATIO;
            }
            command.buff[0] = prev_mode;
            Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	        break;

	    case 0x10: // Next Mode (0-3) (keeps Color and Delay)
	        command.buff[0] = (prev_mode + 1) % 4; // change to the next mode
	        Mailbox_post(mbox, &command, BIOS_NO_WAIT);
	    break;

	    case 0x11: // Prev Mode (0-3) (keeps Color and Delay)
            command.buff[0] = (prev_mode - 1) % 4; // change to the next mode
            Mailbox_post(mbox, &command, BIOS_NO_WAIT);
        break;

	    default: // Colors Off
	        PWM_Color_Set(COLOR_BLACK);
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

	c_handler.LedRed.handler = 		PWM_open(Board_PWM1, &params);
    c_handler.LedGreen.handler = 	PWM_open(Board_PWM2, &params);
    c_handler.LedBlue.handler =  	PWM_open(Board_PWM3, &params);

    c_handler.MosfetRed.handler = 	PWM_open(Board_PWM4, &params);
    c_handler.MosfetGreen.handler = PWM_open(Board_PWM5, &params);
    c_handler.MosfetBlue.handler = 	PWM_open(Board_PWM6, &params);
    c_handler.MosfetWhite.handler = PWM_open(Board_PWM7, &params);

    if (c_handler.LedRed.handler == NULL || c_handler.LedGreen.handler == NULL ||
    		c_handler.LedBlue.handler == NULL || c_handler.MosfetRed.handler == NULL ||
			c_handler.MosfetGreen.handler == NULL || c_handler.MosfetBlue.handler == NULL ||
			c_handler.MosfetWhite.handler == NULL){
    	System_abort("Failed to Init PWMs");
    }

    PWM_start(c_handler.LedRed.handler);
    PWM_start(c_handler.LedGreen.handler);
    PWM_start(c_handler.LedBlue.handler);
    PWM_start(c_handler.MosfetRed.handler);
    PWM_start(c_handler.MosfetGreen.handler);
    PWM_start(c_handler.MosfetBlue.handler);
    PWM_start(c_handler.MosfetWhite.handler);


    // RGB LEDs LOW is at duty cycle 100 % (255)
    c_handler.LedRed.inverted = true;
    c_handler.LedRed.duty = 255;
	c_handler.LedGreen.inverted = true;
	c_handler.LedGreen.duty = 255;
	c_handler.LedBlue.inverted = true;
	c_handler.LedBlue.duty = 255;

    // Turn RGB LEDs off buy increasing duty cycle to 100%
    PWM_setDuty(c_handler.LedRed.handler, 1 * PWM_DUTY_FRACTION_MAX);
    PWM_setDuty(c_handler.LedGreen.handler, 1 * PWM_DUTY_FRACTION_MAX);
    PWM_setDuty(c_handler.LedBlue.handler, 1 * PWM_DUTY_FRACTION_MAX);

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
