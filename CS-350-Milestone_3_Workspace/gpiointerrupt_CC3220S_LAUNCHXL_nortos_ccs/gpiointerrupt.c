/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>

/* Driver configuration */
#include "ti_drivers_config.h"

#include <ti/drivers/Timer.h>

//Morse Code sequences for SOS and OK
const char* SOS = "... --- ..."; // SOS in Morse code
const char* OK = "--- -.-";       // OK in Morse code

//State machine states and variables
enum State {
    SENDING_SOS,
    SENDING_OK
};
//Setting currentState to SENDING_SOS
enum State currentState = SENDING_SOS;


int symbolIndex = 0;
int dashPause = 1500000;
int charPause = 500000;
int dotPause = 500000;
int wordPause = 3500000;
int queue = 0;

void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    switch (currentState)
        {
        //State to send SOS message
        case SENDING_SOS:
            if (SOS[symbolIndex] == '.') {
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
                usleep(dotPause);
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            } else if (SOS[symbolIndex] == '-') {
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
                usleep(dashPause);
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            }
            symbolIndex++;
            usleep(charPause);
            break;

        //State to send OK message
        case SENDING_OK:
            if (OK[symbolIndex] == '.') {
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
                usleep(dotPause);
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);

            } else if (OK[symbolIndex] == '-') {
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
                usleep(dashPause);
                GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);

            }
            symbolIndex++;
            usleep(charPause);
            break;

        default:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            break;
        }
    //If currently sending SOS, then check if complete, if so pause for next word and restet symbolIndex.
    if (currentState == SENDING_SOS && symbolIndex == 11) {
        // SOS message is complete
        usleep(wordPause);
        symbolIndex = 0;
        //then check status of queue to see if a button has queued up a change request, if so, change states.
        if (queue == 1){
            currentState = SENDING_OK;
        }else{
            currentState = SENDING_SOS;
        }
        //If sending OK message, check if message is completed.  If so pause for next word and restet symbolIndex.
    } else if (currentState == SENDING_OK && symbolIndex == 7) {
        // OK message is complete
        usleep(wordPause);
        symbolIndex = 0;
        //check status of queue to see if a button has queued up a change request, if so, change states.
        if (queue == 0){
            currentState = SENDING_SOS;
        }else{
            currentState = SENDING_OK;
        }


    }
}
void initTimer(void)
{

 Timer_Handle timer0;
 Timer_Params params;
 Timer_init();
Timer_Params_init(&params);
params.period = 500000;
params.periodUnits = Timer_PERIOD_US;
params.timerMode = Timer_CONTINUOUS_CALLBACK;
params.timerCallback = timerCallback;



 timer0 = Timer_open(CONFIG_TIMER_0, &params);
 if (timer0 == NULL) {
 /* Failed to initialized timer */
 while (1) {}
 }
 if (Timer_start(timer0) == Timer_STATUS_ERROR) {
 /* Failed to start timer */
 while (1) {}
 }
}
/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */

//Creating function to change the state of the state machine, so that it can be easily implemented on all buttons
void changeState(){
    if (queue == 1){
        queue = 0;
        }else{
            queue = 1;
        }
}
void gpioButtonFxn0(uint_least8_t index)
{
// Toggle between SOS and OK messages
    changeState();
}


/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  This may not be used for all boards.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn1(uint_least8_t index)
{
    //change state of state machine
    changeState();

}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    symbolIndex = 0;

    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn on user LED */
    //GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);



    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on CONFIG_GPIO_BUTTON1.
     */
    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1) {
        /* Configure BUTTON1 pin */
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        /* Install Button callback */
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }
    initTimer();
    return (NULL);
}
