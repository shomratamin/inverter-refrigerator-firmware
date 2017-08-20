/*
 * BLDC_lib.h
 *
 *  Created on: Nov 16, 2015
 *      Author: a0132403
 */

#ifndef BLDC_LIB_H_
#define BLDC_LIB_H_






void user_defined_parameters (void);
void dc_bus_compensation (void);

void Init_InstaSpin (void);
void High_Impedance (void);
void Process_Integration(void);


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

extern unsigned int 	INTEGRATION_CONSTANT;   	//1200
extern	unsigned char 	START_UP_DUTYCYCLE;							//%
extern unsigned char 	START_UP_MAX_DUTYCYCLE;					//%
extern unsigned char 	INCREMENTAL_DUTY;						//%
extern unsigned char 	INCREMENTAL_TIME;						//mS
extern	unsigned int 	WAIT_TIME;							//mS
extern unsigned char 	SPEED_RAMP_CYCLES;
extern unsigned char 	SPEED_DUTY_MAX;			//%
extern unsigned char 	SPEED_DUTY_MIN;			//%
extern unsigned int    MAX_DUTY ;
extern unsigned int    Velocity_Ramp;
extern unsigned int    MIN_DUTY;
extern unsigned char State;
extern int Temperature_feedback;
extern int DC_Bus_Voltage_present;
extern volatile unsigned int Cycle_Count;

#endif /* BLDC_LIB_H_ */
