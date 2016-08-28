/*
 * buzzer.h
 *
 *  Created on: Aug 15, 2016
 *      Author: constantin.chabirand@gmail.com
 */

/*      ALL CREDITS GOES TO @eserra from instructables.com
 *
 *      http://www.instructables.com/id/How-to-easily-play-music-with-buzzer-on-arduino-Th/
 *
 */

#ifndef APPLICATION_USER_BUZZER_H_
#define APPLICATION_USER_BUZZER_H_

#include "stm32l1xx_hal.h"
#include "stm32l1xx_hal_tim.h"

/**
 * Play the beginning of the imperial match form start wars series
 *
 * @pre TIM4 CHANNEL_3 must be initialized to generate PWM signals
 * @pre TIM4, CHANNEL_3 must be started before calling this function
 */
void Buzzer_playImperialMarch();

#endif /* APPLICATION_USER_BUZZER_H_ */
