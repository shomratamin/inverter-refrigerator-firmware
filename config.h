/*
 * config.h
 *
 *  Created on: Nov 16, 2015
 *      Author: a0132403
 */

#ifndef CONFIG_H_
#define CONFIG_H_




#endif /* CONFIG_H_ */

#include "msp430f5132.h"

//USER DEFINED PARAMETERS

#define PWM_PERIOD 				1251; //((25MHz/PWM Frequency)+1)
#define BEMF_THRESHOLD 			500; //Back EMF threshold
#define MAX_DUTYCYCLE 			350 ; //relative to PWM_PERIOD
#define MIN_DUTYCYCLE 			300 ; //relative to PWM_PERIOD
#define MAX_OPEN_LOOP_FREQ 		7; // Not implemented in this code
#define START_UP_DUTY 			4; //in percentage
#define START_UP_MAX_DUTY 		7; //in percentage

#define ACCEL_CONSTANT          1000; 	// Reduce this parameter to increase the acceleration of the motor.
										//Minimum value should be 1000. default value is 5000.


//................................................................PROTOTYPES OF THE FUNCTIONS USED ........................................................................
void Init_InstaSpin (void);
void High_Impedance (void);
void Process_Integration(void);

//..................................................................PROTOTYPES OF THE FUNCTIONS END HERE...................................................................

void A_PWM();
void A_LOW();
void A_Z();

void B_PWM();
void B_LOW();
void B_Z();

void C_PWM();
void C_LOW();
void C_Z();

void Init_Clocks (void);
void SetVcoreUp (unsigned int level);
void init_IO (void);
void init_WDT (void);
void init_ADC (void);
void init_TimerD (void);
void Commutate(void);
void init_COMPB (void);
void init_TEC (void);
void Init_UART(void);
