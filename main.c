// System Configuration
// Device Part Number	: MSP430F5132
// compiler             : Code Composer Studio 5.5.0.00077
// Oscillator Frequency	: 25MHz,  using internal DCO
// PWM generation		: TIMER D1.0, 25MHz, OUTMOD = Up mode-set/reset,PWM frequency set for 20 kHz using
//						  constant "PWM_PERIOD".
// PWM Mode 			: Asymmetrical with no dead band

// Position Feedback    : sensor-less

// ADC					: ADC -> A0 –> DC-bus voltage sense
//						: ADC -> A2 –> Temperature sense
//						: ADC -> A3, A4, A5 –> Motor Winding voltage sensing (applicable for sensor- less control)
//						: ADC -> A8 –> Potentiometer voltage sensing for changing speed
//						: ADC -> A7 –> Current sense amplifier output

//Other Peripherals     : COMP_B –> Comparator input for Current limit Protection (channel CB14)
//						: TEC0FLT1 –> Over-current limit Fault input
// 						: UART RX -> P1.6 is used utilizing port mapping
// 						: UART TX -> P1.1 is used, default UART TX pin

//Indications 			: P2.0 -> OutputPin - LED toggle in every commutation
//					   	: P3.0 -> OutputPin - ENABLE signal to the gate driver. Not used in this ptrogram
//						: P2.1, P2.2 , P2.3 -> Three bottom side switch PWM output
//						: P1.7, P2.5, PJ.6 -> Three top side switch PWM output


/*****************************************************************************************************************/
/*****************************************************************************************************************/

#include "msp430f5132.h"
#include "stdint.h"
#include "config.h"
#include "BLDC_lib.h"
#include "commands.h"

#define CALTDH0CTL1_256        *((unsigned int *)0x1A36)


void user_defined_parameters (void);
void Init_Clocks (void);
void SetVcoreUp (unsigned int level);
void init_COMPB (void);
void init_TEC (void);
void Init_UART(void);
void Init_TimerA(void);
void Stop_Motor(void);
void Start_Motor(void);
void Adjust_RPM(void);
void PrintNumber(int n, char base);
void write_flash(void);
void read_flash(void);
void Adjust_Step(void);


volatile unsigned int Motor_E_Frequency = 0;
int time_elapsed_minute = 0;
int minute_counter = 0;
int second_counter = 0;
int MOTOR_SPIN_STATUS = 1;
int motor_cont_running_time_minute = 0;
int running_condition_m_counter = 0;
volatile unsigned int prevcycle = 0;
volatile unsigned int cycle = 0;
char A0_Timer_Tick = 0;
unsigned char tx_string_index = 0;
char tx_string[7] = "shomrat";
char uart_tx_interrupt = 0;
char uart_rx_interrupt = 0;
int safe_to_start_motor = 1;

unsigned int speed_step = 1;
unsigned int speed_pwm_step = 20;
int motor_target_electrical_frequency = 108;

unsigned char *flash_motor_running_condition = (unsigned char *)0x1800;
unsigned char *flash_running_minutes = (unsigned char *)0x1801;
unsigned char *flash_off_minutes = (unsigned char *)0x1802;

unsigned char motor_running_condition = 98;
unsigned char motor_running_condition_cur_cycle = 98;
unsigned int running_minutes = 25;
unsigned int off_minutes = 25;

static const int speed_step_table[9] = {96,108,120,132,144,160,200,240,260};


void main (void)
{
    WDTCTL = WDTPW + WDTHOLD; // Stop WDT
    _delay_cycles(2000000);
    SetVcoreUp (0x01);
    Init_Clocks ();							  				// Initialize clocks for 25 MHz
    __bis_SR_register(GIE);        							// Enable Interrupts
    user_defined_parameters ();
    init_IO ();
    init_ADC ();

    init_TimerD ();
    TD0CCR0 = PWM_PERIOD;
    init_WDT ();// set the ADC sampling interval to 1.3ms
    init_COMPB ();
    init_TEC ();
    Init_UART();
    Init_InstaSpin();
    Init_TimerA();

    static const char uart_motor_electric_frequecy = 'f';
    static const char uart_motor_speed_up = 'u';
    static const char uart_motor_speed_down = 'd';
    static const char uart_start_motor = 's';
    static const char uart_stop_motor = 'x';
    static const char uart_motor_running_condition = 'r';
    static const char uart_motor_ruuning_minutes = 'O';
    static const char uart_motor_off_minutes = 'o';
    static const char uart_current_duty_cycle = 'c';
    static const char uart_current_speed_step = 'a';
    static const char uart_motor_dc_voltage = 'v';
    static const char uart_system_up_minites = 't';
    static const char uart_igbt_temperature = 'i';
    static const char uart_firmware_reset = 'R';

    unsigned long dividend = 0;
    unsigned long divisor = 0;
    unsigned long divisor_counter = 0;
    char start_running_cond_division_flag = 0;
    unsigned char temp_r_condition = 0;

    read_flash();

    while(1)
    {
        if(MOTOR_SPIN_STATUS == 1)
        {

            Process_Integration ();
            Commutate();
            dc_bus_compensation ();

        }

        if(start_running_cond_division_flag == 1)
        {
            divisor_counter += divisor;

            if(divisor_counter >= dividend || temp_r_condition > 98)
            {
                motor_running_condition_cur_cycle = temp_r_condition;
                start_running_cond_division_flag = 0;
                dividend = 0;
                divisor = 0;
                divisor_counter = 0;
                temp_r_condition = 0;
            }
            else
            {
                temp_r_condition += 1;

            }
        }


        if(A0_Timer_Tick == 1)
        {
            A0_Timer_Tick = 0;

            if(running_condition_m_counter == 10)
            {
                running_condition_m_counter = 0;
                dividend = running_minutes * 100;
                divisor = running_minutes + off_minutes;
                start_running_cond_division_flag = 1;
                if(running_minutes > 6350 || off_minutes > 6350)
                {
                    running_minutes = 1;
                    off_minutes = 1;
                }
            }

            minute_counter += 1;
            second_counter += 1;
            if(minute_counter == 3000)
            {
                time_elapsed_minute += 1;
                minute_counter = 0;
                running_condition_m_counter += 1;
                if(Motor_E_Frequency > 0)
                {
                    running_minutes++;
                    motor_cont_running_time_minute += 1;
                }
                else
                {
                    off_minutes += 1;
                    motor_cont_running_time_minute = 0;
                }

                Adjust_Step();
            }

            if(second_counter == 50)
            {
                second_counter = 0;
                cycle = Cycle_Count;
                Motor_E_Frequency = cycle - prevcycle;
                prevcycle = cycle;

                if(State > 2 && Motor_E_Frequency < 1)
                {
                    if(MOTOR_SPIN_STATUS == 1)
                    {
                        Stop_Motor();
                        UCA0TXBUF = 'S';
                        write_flash(); //on production
                    }
                    else if(DC_Bus_Voltage_present > 430 && safe_to_start_motor == 1 && DC_Bus_Voltage_present < 960)
                    {
                        Start_Motor();
                    }
                }

                if(MOTOR_SPIN_STATUS == 1)
                {
                    Adjust_RPM();
                }
            }
        }


        if(uart_tx_interrupt == 1)
        {
            if(tx_string[tx_string_index] != '\0')
            {
                if((UCA0IFG & UCTXIFG))
                {
                    UCA0TXBUF = tx_string[tx_string_index];//*tx_string;
                    tx_string_index++;
                    if(tx_string_index > 6)
                    {
                        tx_string_index = 0;
                        uart_tx_interrupt = 0;
                    }
                }
            }
            else
            {
                uart_tx_interrupt = 0;
                tx_string_index = 0;
            }
        }

        if(uart_rx_interrupt == 1)
        {
            uart_rx_interrupt = 0;
            char rx_val = UCA0RXBUF;
            if(rx_val == uart_motor_electric_frequecy)
            {
                PrintNumber(Motor_E_Frequency, 10);
            }
            if(rx_val == uart_motor_speed_up)
            {
                if(speed_step < 8)
                    speed_step += 1;
                motor_target_electrical_frequency = speed_step_table[speed_step];
            }
            if(rx_val == uart_motor_speed_down)
            {
                if(speed_step >= 1)
                    speed_step -= 1;
                motor_target_electrical_frequency = speed_step_table[speed_step];
            }
            if(rx_val == uart_start_motor)
            {
                Start_Motor();
            }
            if(rx_val == uart_stop_motor)
            {
                Stop_Motor();
            }
            else if(rx_val == uart_current_duty_cycle)
            {
                PrintNumber(MAX_DUTY, 10);
            }
            else if(rx_val == uart_system_up_minites)
            {
                PrintNumber(time_elapsed_minute, 10);
            }
            else if(rx_val == uart_current_speed_step)
            {
                PrintNumber(speed_step, 10);
            }
            else if(rx_val == uart_motor_dc_voltage)
            {
                PrintNumber(DC_Bus_Voltage_present, 10);
            }
            else if(rx_val == uart_igbt_temperature)
            {
                PrintNumber(Temperature_feedback, 10);
            }
            else if(rx_val == uart_motor_ruuning_minutes)
            {
                PrintNumber(running_minutes, 10);
            }
            else if(rx_val == uart_motor_off_minutes)
            {
                PrintNumber(off_minutes, 10);
            }
            else if(rx_val == uart_motor_running_condition)
            {
                read_flash();
                PrintNumber(motor_running_condition, 10);
            }
            else if(rx_val == uart_firmware_reset)
            {
                Stop_Motor();
                motor_running_condition = 98;
                motor_running_condition_cur_cycle = 98;
                running_minutes = 25;
                off_minutes = 25;
                write_flash(); //on production
            }
        }

    }										//end of while
} 	//end main


void Adjust_RPM(void)
{
    if(Motor_E_Frequency < motor_target_electrical_frequency)
    {
        int _diff = motor_target_electrical_frequency - Motor_E_Frequency;
        if(_diff > 80)
        {
            speed_pwm_step = 30;
        }
        else if(_diff > 40)
        {
            speed_pwm_step = 20;
        }
        else if(_diff > 20)
        {
            speed_pwm_step = 15;
        }
        else if(_diff > 10)
        {
            speed_pwm_step = 10;
        }
        else if(_diff > 2)
        {
            speed_pwm_step = 5;
        }
        else
        {
            speed_pwm_step = 1;
        }

        MAX_DUTY += speed_pwm_step;

        if(MAX_DUTY > 1220)
            MAX_DUTY = 1220;
    }
    else if(Motor_E_Frequency > motor_target_electrical_frequency)
    {
        int _diff = Motor_E_Frequency - motor_target_electrical_frequency;
        if(_diff > 80)
        {
            speed_pwm_step = 15;
        }
        else if(_diff > 40)
        {
            speed_pwm_step = 20;
        }
        else if(_diff > 20)
        {
            speed_pwm_step = 15;
        }
        else if(_diff > 10)
        {
            speed_pwm_step = 10;
        }
        else if(_diff > 2)
        {
            speed_pwm_step = 5;
        }
        else
        {
            speed_pwm_step = 1;
        }

        MAX_DUTY -= speed_pwm_step;

        if(MAX_DUTY < 300)
            MAX_DUTY = 300;
    }

}

void Adjust_Step(void)
{
    if(motor_cont_running_time_minute == 120)
    {
        speed_step += 3;
        if(speed_step > 8)
            speed_step = 8;
        motor_target_electrical_frequency = speed_step_table[speed_step];
    }
    else if(motor_cont_running_time_minute == 100)
    {
        speed_step += 3;
        if(speed_step > 8)
            speed_step = 8;
        motor_target_electrical_frequency = speed_step_table[speed_step];
    }
    else if(motor_cont_running_time_minute == 80)
    {
        speed_step += 2;
        if(speed_step > 8)
            speed_step = 8;
        motor_target_electrical_frequency = speed_step_table[speed_step];
    }
    else if(motor_cont_running_time_minute == 30)
    {
        if(motor_running_condition > 75)
        {
            speed_step = 8;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
        else if(motor_running_condition > 63)
        {
            speed_step = 6;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
        else if(motor_running_condition > 57)
        {
            speed_step = 4;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
        else if(motor_running_condition > 51)
        {
            speed_step = 2;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
        else
        {
            speed_step = 0;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
    }
    else if(motor_cont_running_time_minute == 1)
    {
        if(motor_running_condition > 75)
        {
            speed_step = 8;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
        else if(motor_running_condition > 63)
        {
            speed_step = 7;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
        else if(motor_running_condition > 57)
        {
            speed_step = 6;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
        else if(motor_running_condition > 51)
        {
            speed_step = 5;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
        else
        {
            speed_step = 4;
            motor_target_electrical_frequency = speed_step_table[speed_step];
        }
    }
}

void Stop_Motor(void)
{
    MOTOR_SPIN_STATUS = 0;
    A_Z();
    B_Z();
    C_Z();
    //    P2OUT &= ~(BIT1 + BIT2 + BIT3 + BIT5);
    //    PJOUT &= ~BIT6;
    //    P1OUT &= ~BIT7;
    P2OUT |= BIT0;

}

void write_flash(void)
{
    unsigned char r_m = (unsigned char)(running_minutes/25);
    if(r_m < 1)
        r_m = 1;
    unsigned char o_m = (unsigned char)(off_minutes/25);
    if(o_m < 1)
        o_m = 1;

    FCTL1 = FWKEY + ERASE;                    // Set Erase bit
    FCTL3 = FWKEY;
    *flash_motor_running_condition = 0;
    while(!(FCTL3 & WAIT));
    FCTL1 = FWKEY + WRT; // Set WRT bit for write operation
    *flash_motor_running_condition = motor_running_condition_cur_cycle;//motor_running_condition;
    while(!(FCTL3 & WAIT));
    *flash_running_minutes = r_m;//motor_running_condition;
    while(!(FCTL3 & WAIT));
    *flash_off_minutes = o_m;//motor_running_condition;
    while(!(FCTL3 & WAIT));
    FCTL1 = FWKEY; // Clear WRT bit
    FCTL3 = FWKEY + LOCK; // Set LOCK bit
}

void read_flash(void)
{
    motor_running_condition = *flash_motor_running_condition;
    motor_running_condition_cur_cycle = motor_running_condition;
    running_minutes = (unsigned int)(*flash_running_minutes * 25);
    off_minutes = (unsigned int)(*flash_off_minutes * 25);
}

void Start_Motor(void)
{
    write_flash(); //on production
    PMMCTL0 = PMMPW | PMMSWBOR;
}

void Init_TimerA(void)
{
    TA0CCTL0 = CCIE;
    TA0CTL = TASSEL_2 + MC_1 + ID_3;
    TA0CCR0 = 62500;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0(void)
{
    A0_Timer_Tick = 1;
}


void user_defined_parameters (void)
{
    INTEGRATION_CONSTANT =	BEMF_THRESHOLD;   			//1200
    START_UP_DUTYCYCLE = START_UP_DUTY;					//%
    START_UP_MAX_DUTYCYCLE	= START_UP_MAX_DUTY;		//%
    MAX_DUTY  =  MAX_DUTYCYCLE ;
    MIN_DUTY  =  MIN_DUTYCYCLE ;
    //  more parameters
    Velocity_Ramp = ACCEL_CONSTANT;
}

// Clocks And Vcore
void Init_Clocks (void)
{
    SetVcoreUp (0x01);
    SetVcoreUp (0x02);
    SetVcoreUp (0x03);

    // Configure DCO = 25Mhz
    UCSCTL3 = SELREF_2;                       // Set DCO FLL reference = REFO
    UCSCTL4 |= SELA_2;                        // Set ACLK = REFO
    __bis_SR_register(SCG0);                  // Disable the FLL control loop
    UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_7;                      // Select DCO range 50MHz operation
    UCSCTL2 = FLLD_1 + 762;                   // Set DCO Multiplier for 25MHz
    // (N + 1) * FLLRef = Fdco
    // (762 + 1) * 32768 = 25MHz
    // Set FLL Div = fDCOCLK/2
    __bic_SR_register(SCG0);                  // Enable the FLL control loop

    // Worst-case settling time for the DCO when the DCO range bits have been
    // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
    // UG for optimization.
    // 32 x 32 x 25 MHz / 32,768 Hz ~ 780k MCLK cycles for DCO to settle
    __delay_cycles(782000);

    // Loop until Xt1 & DCO stabilizes - In this case only DCO has to stabilize
    do
    {
        UCSCTL7 &= ~(XT1LFOFFG + XT1HFOFFG + DCOFFG);
        //Clear XT1,DCO fault flags
        SFRIFG1 &= ~OFIFG;                      //Clear fault flags
    }while (SFRIFG1&OFIFG);                   //Test oscillator fault flag
}

void SetVcoreUp (unsigned int level)
{
    // Open PMM registers for write
    PMMCTL0_H = PMMPW_H;
    // Set SVS/SVM high side new level
    SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
    // Set SVM low side to new level
    SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;
    // Wait till SVM is settled
    while ((PMMIFG & SVSMLDLYIFG) == 0);
    // Clear already set flags
    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
    // Set VCore to new level
    PMMCTL0_L = PMMCOREV0 * level;
    // Wait till new level reached
    if ((PMMIFG & SVMLIFG))
        while ((PMMIFG & SVMLVLRIFG) == 0);
    // Set SVS/SVM low side to new level
    SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
    // Lock PMM registers for write access
    PMMCTL0_H = 0x00;
}

void init_COMPB (void)
{
    CBCTL0 |= CBIPEN + CBIPSEL_14; // Enable V+, input channel CB1
    CBCTL1 |= CBPWRMD_1;          // normal power mode
    CBCTL2 |= CBRSEL;             // VREF is applied to -terminal
    CBCTL2 |= CBRS_3 + CBREFL_2;    // R-ladder off; Bandgap voltage amplifier ON and generates 2.0V reference.
    //CBREFL_1--> Comp. B Reference voltage level 1 : 1.5V
    //CBREFL_2--> Comp. B Reference voltage level 1 : 2.0V
    //CBREFL_3--> Comp. B Reference voltage level 1 : 2.5V

    CBCTL3 |= BITC;               // Input Buffer Disable @P1.1/CB1
    CBCTL1 |= CBON;               // Turn On ComparatorB

    __delay_cycles(75);           // delay for the reference to settle
    __no_operation();                         // For debug
}

void init_TEC (void)
{

    TEC0XCTL1 = TECXFLTLVS1 + TECXFLTPOL1;     // level edge trigger
    TEC0XCTL0 = TECXFLTEN1;                   // EXTFLT1 enable for controlling TD0.1

    __no_operation();                         // For debugger
}

void Init_UART(void)
{
    P1SEL |= BIT1+BIT6;                       // P1.1,2 = USCI_A0 TXD/RXD

    PMAPKEYID = 0x02D52;
    PMAPPWD = 0x02D52;
    PMAPCTL = PMAPRECFG;

    P1MAP1 = PM_UCA0TXD;
    P1MAP6 = PM_UCA0RXD;

    UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 217; //25  //217                // 25MHz 115200 (see User's Guide)
    UCA0BR1 = 0;                              // 25MHz 115200
    UCA0MCTL |= UCBRS_0 + UCBRF_0;            // Modulation UCBRSx=1, UCBRFx=0
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    UCA0IE |= UCRXIE;
}

#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch(__even_in_range(UCA0IV,4))
    {
    case 0:break;                             // Vector 0 - no interrupt
    case 2:                                   // Vector 2 - RXIFG
        UCA0IFG &= ~UCRXIFG;
        uart_rx_interrupt = 1;               // TX -> RXed character
        break;
    case 4:
        break;                             // Vector 4 - TXIFG
    default: break;
    }
}

void PrintNumber(int n, char base)
{
    char buf[8 * sizeof(int) + 2]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[sizeof(buf) -1]; //&buf[sizeof(buf) - 1];

    *str = '\0';
    *--str = 'e';

    // prevent crash if called with base == 1
    if (base < 2) base = 10;

    do {
        unsigned int m = n;
        n /= base;
        char c = m - base * n;
        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while(n);

    //    if(*tx_string != '\0')
    //    {
    //        UCA0TXBUF = *tx_string;
    //        *tx_string++;
    //    }

    unsigned char i = 0;
    for (i = 0; *str != '\0'; *++str)
    {
        tx_string[i] = *str;
        i++;
        if(i > 6)
        {
            i = 6;
            tx_string[i] = '\0';
            break;
        }
    }
    tx_string[i] = '\0';
    uart_tx_interrupt = 1;

    //    for (; *tx_string != '\0'; *str++)
    //    {
    //        while (!(UCA0IFG & UCTXIFG));
    //        UCA0TXBUF = *str;
    //    }
    //
    //    while (!(UCA0IFG & UCTXIFG));
    //    UCA0TXBUF = 'e';
}
