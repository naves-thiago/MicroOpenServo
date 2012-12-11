/*
    Copyright (c) 2006 Michael P. Thompson <mpthompson@gmail.com>

    Permission is hereby granted, free of charge, to any person 
    obtaining a copy of this software and associated documentation 
    files (the "Software"), to deal in the Software without 
    restriction, including without limitation the rights to use, copy, 
    modify, merge, publish, distribute, sublicense, and/or sell copies 
    of the Software, and to permit persons to whom the Software is 
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be 
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.

    $Id$
*/

#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "openservo.h"
#include "config.h"
#include "pwm.h"
#include "registers.h"

#if 0
#if !defined(__AVR_ATmega168__)
#  error "This module is not being compiled for the AVR ATmega168 MCU."
#endif
#endif

//
// ATmega168
// =========
//
// PWM output to the servo motor utilizes Timer/Counter1 in 8-bit mode.  
// Output to the motor is assigned as follows:
//
//  OC1A (PB1) - Servo PWM output direction A
//  OC1B (PB2) - Servo PWM output direction B
//

// Determine the top value for timer/counter1 from the frequency divider.
#define PWM_TOP_VALUE(div)      ((uint16_t) div << 4) - 1;

// Determines the compare value associated with the duty cycle for timer/counter1.
#define PWM_OCRN_VALUE(div,pwm) (uint16_t) (((uint32_t) pwm * (((uint32_t) div << 4) - 1)) / 255)

// Flags that indicate PWM output in A and B direction.
static uint8_t pwm_a;
static uint8_t pwm_b;

// Pwm frequency divider value.
static uint16_t pwm_div;

//
// The delay_loop function is used to provide a delay. The purpose of the delay is to
// allow changes asserted at the AVRs I/O pins to take effect in the H-bridge (for
// example turning on or off one of the MOSFETs). This is to prevent improper states
// from occurring in the H-bridge and leading to brownouts or "crowbaring" of the
// supply.
//
#define DELAYLOOP 8
inline static void delay_loop(int n)
{
    uint8_t i;
    for(i=0; i<n; i++)
    {
        asm("nop");
    }
}

//
//
//
void pwm_dir_a(uint8_t pwm_duty)
// Send PWM signal for rotation with the indicated pwm ratio (0 - 255).
// This function is meant to be called only by pwm_update.
{
    // Determine the duty cycle value for the timer.
    //uint16_t duty_cycle = PWM_OCRN_VALUE(pwm_div, pwm_duty);

    // Disable interrupts.
    cli();

    // Only 1 pwm should be on at any time
    // Disable PWM_B (PA7/OC0B) and high side FET b
	TCCR0A &= ~((1<<COM0B1) | (1<<COM0B0));
	OCR0B = 0;
	PORTA |= (1<<PA1);
	PORTA &= ~(1<<PA7);
    
    // Setting OCR0B to 0 won't give us 0 duty cycle, so disable the PWM
    if (pwm_duty == 0)
    {
	    TCCR0A &= ~(1<<COM0A1);
	    PORTB &= ~(1<<PB2);
		PORTA |= (1<<PA2);
	    OCR0A = 0;
	    
	    // Give time to the H-Bridge
	    delay_loop(DELAYLOOP);
    }
    else
    {
		// Update the PWM duty cycle.
	    //OCR0A = duty_cycle;
	    OCR0A = pwm_duty;
		
		// Enable PWM
		TCCR0A |= (1<<COM0A1);
		
		// Enable High Side FET
		PORTA &= ~(1<<PA2);
    }

    // Set the B direction flag.
    pwm_a = pwm_duty;

    // Restore interrupts.
    sei();

    // Save the pwm A and B duty values.
    registers_write_byte(REG_PWM_DIRA, pwm_a);
    registers_write_byte(REG_PWM_DIRB, pwm_b);
}


//static void pwm_dir_b(uint8_t pwm_duty)
void pwm_dir_b(uint8_t pwm_duty)
// Send PWM signal for rotation with the indicated pwm ratio (0 - 255).
// This function is meant to be called only by pwm_update.
{
    // Determine the duty cycle value for the timer.
    //uint16_t duty_cycle = PWM_OCRN_VALUE(pwm_div, pwm_duty);

    // Disable interrupts.
    cli();

    // Only 1 pwm should be on at any time
    // Disable PWM_A (PB2/OC0A) and high side FET a
    TCCR0A &= ~((1<<COM0A1) | (1<<COM0A0));
	PORTA |= (1<<PA2);
    PORTB &= ~(1<<PB2);
    OCR0A = 0;
    
    
    // Setting OCR0B to 0 won't give us 0 duty cycle, so disable the PWM
    if (pwm_duty == 0)
    {
		TCCR0A &= ~(1<<COM0B1);
		OCR0B = 0;
		PORTA |= (1<<PA1);
		PORTA &= ~(1<<PA7);

	    // Give time to the H-Bridge
	    delay_loop(DELAYLOOP);
    }
    else
    {
	    // Update the PWM duty cycle.
	    //OCR0B = duty_cycle;
	    OCR0B = pwm_duty;
	    
	    // Enable PWM
	    TCCR0A |= (1<<COM0B1);
	    
	    // Enable High Side FET
	    PORTA &= ~(1<<PA1);
    }

    // Set the B direction flag.
    pwm_a = pwm_duty;

    // Restore interrupts.
    sei();

    // Save the pwm A and B duty values.
    registers_write_byte(REG_PWM_DIRA, pwm_a);
    registers_write_byte(REG_PWM_DIRB, pwm_b);
}


void pwm_registers_defaults(void)
// Initialize the PWM algorithm related register values.  This is done 
// here to keep the PWM related code in a single file.  
{
    // PWM divider is a value between 1 and 1024.  This divides the fundamental
    // PWM frequency (500 kHz for 8MHz clock, 1250 kHz for 20MHz clock) by a 
    // constant value to produce a PWM frequency suitable to drive a motor.  A 
    // small motor with low inductance and impedance such as those found in an 
    // RC servo will my typically use a divider value between 16 and 64.  A larger 
    // motor with higher inductance and impedance may require a greater divider.
    registers_write_word(REG_PWM_FREQ_DIVIDER_HI, REG_PWM_FREQ_DIVIDER_LO, DEFAULT_PWM_FREQ_DIVIDER);
}


void pwm_init(void)
// Initialize the PWM module for controlling a DC motor.
{
    // Initialize the pwm frequency divider value.
    pwm_div = registers_read_word(REG_PWM_FREQ_DIVIDER_HI, REG_PWM_FREQ_DIVIDER_LO);

    TCCR0A = 0;
        asm("nop");
        asm("nop");
        asm("nop");

    // Set PB2/OC0A and PA7/OC0B to low.
    PORTB &= ~(1<<PB2);
    PORTA &= ~(1<<PA7);

	// Set high side FETs off
	PORTA |= (1<<PA1) | (1<<PA2);

    // Enable PB2/OC0A and PA7/OC0B and high side FETs as outputs.
    DDRB |= (1<<PB2);
    DDRA |= (1<<PA7) | (1<<PA1) | (1<<PA2);

    // Set the PWM duty cycle to zero.
    OCR0A = 0;
    OCR0B = 0;
	
	//WGM is set to 3 which is fast PWM mode, PWMs start off
	TCCR0A |= ((1 << WGM01) | (1 << WGM00));

	//This turns the the 8 bit counter 0 on and sets the pre-scaler to 8
	TCCR0B |= (1 << CS00);
	
	
    // Update the pwm values.
    registers_write_byte(REG_PWM_DIRA, 0);
    registers_write_byte(REG_PWM_DIRB, 0);
}


void pwm_update(uint16_t position, int16_t pwm)
// Update the PWM signal being sent to the motor.  The PWM value should be
// a signed integer in the range of -255 to -1 for clockwise movement,
// 1 to 255 for counter-clockwise movement or zero to stop all movement.
// This function provides a sanity check against the servo position and
// will prevent the servo from being driven past a minimum and maximum
// position.
{
    uint8_t pwm_width;
    uint16_t min_position;
    uint16_t max_position;

    // Quick check to see if the frequency divider changed.  If so we need to 
    // configure a new top value for timer/counter0.  This value should only 
    // change infrequently so we aren't too elegant in how we handle updating
    // the value.  However, we need to be careful that we don't configure the
    // top to a value lower than the counter and compare values.
	
	/*
    if (registers_read_word(REG_PWM_FREQ_DIVIDER_HI, REG_PWM_FREQ_DIVIDER_LO) != pwm_div)
    {
        // Disable OC0A and OC0B outputs.
        TCCR0A &= ~((1<<COM1A1) | (1<<COM1A0));
        TCCR0A &= ~((1<<COM1B1) | (1<<COM1B0));

        // Set PB2/OC0A and PA7/OC0B to low.
        PORTB &= ~(1<<PB2);
        PORTA &= ~(1<<PA7);

        delay_loop(DELAYLOOP);

        // Reset the A and B direction flags.
        pwm_a = 0;
        pwm_b = 0;

        // Update the pwm frequency divider value.
        pwm_div = registers_read_word(REG_PWM_FREQ_DIVIDER_HI, REG_PWM_FREQ_DIVIDER_LO);

        // Update the timer top value.
        //ICR1 = PWM_TOP_VALUE(pwm_div);

        // Reset the counter and compare values to prevent problems with the new top value.
        TCNT0 = 0;
        OCR0A = 0;
        OCR0B = 0;
    }
	*/
	
	
    // Are we reversing the seek sense?
    if (registers_read_byte(REG_REVERSE_SEEK) != 0)
    {
        // Yes. Swap the minimum and maximum position.

        // Get the minimum and maximum seek position.
        min_position = registers_read_word(REG_MAX_SEEK_HI, REG_MAX_SEEK_LO);
        max_position = registers_read_word(REG_MIN_SEEK_HI, REG_MIN_SEEK_LO);

        // Make sure these values are sane 10-bit values.
        if (min_position > 0x3ff) min_position = 0x3ff;
        if (max_position > 0x3ff) max_position = 0x3ff;

        // Adjust the values because of the reverse sense.
        min_position = 0x3ff - min_position;
        max_position = 0x3ff - max_position;
    }
    else
    {
        // No. Use the minimum and maximum position as is.

        // Get the minimum and maximum seek position.
        min_position = registers_read_word(REG_MIN_SEEK_HI, REG_MIN_SEEK_LO);
        max_position = registers_read_word(REG_MAX_SEEK_HI, REG_MAX_SEEK_LO);

        // Make sure these values are sane 10-bit values.
        if (min_position > 0x3ff) min_position = 0x3ff;
        if (max_position > 0x3ff) max_position = 0x3ff;
    }

    // Disable clockwise movements when position is below the minimum position.
    if ((position < min_position) && (pwm < 0)) pwm = 0;

    // Disable counter-clockwise movements when position is above the maximum position.
    if ((position > max_position) && (pwm > 0)) pwm = 0;

    // Determine if PWM is disabled in the registers.
    if (!(registers_read_byte(REG_FLAGS_LO) & (1<<FLAGS_LO_PWM_ENABLED))) pwm = 0;

    // Determine direction of servo movement or stop.
    if (pwm < 0)
    {
        // Less than zero. Turn clockwise.

        // Get the PWM width from the PWM value.
        pwm_width = (uint8_t) -pwm;

        // Turn clockwise.
#if SWAP_PWM_DIRECTION_ENABLED
        pwm_dir_a(pwm_width);
#else
        pwm_dir_b(pwm_width);
#endif
    }
    else if (pwm > 0)
    {
        // More than zero. Turn counter-clockwise.

        // Get the PWM width from the PWM value.
        pwm_width = (uint8_t) pwm;

        // Turn counter-clockwise.
#if SWAP_PWM_DIRECTION_ENABLED
        pwm_dir_b(pwm_width);
#else
        pwm_dir_a(pwm_width);
#endif

    }
    else
    {
        // Stop all PWM activity to the motor.
        pwm_stop();
    }
}


void pwm_stop(void)
// Stop all PWM signals to the motor.
{
    // Disable interrupts.
    cli();

    // Are we moving in the A or B direction?
    if (pwm_a || pwm_b)
    {
        // Disable OC1A and OC1B outputs.
        TCCR0A &= ~((1<<COM1A1) | (1<<COM1A0));
        TCCR0A &= ~((1<<COM1B1) | (1<<COM1B0));

        // Set PB2/OC0A and PA7/OC0B to low.
        PORTB &= ~(1<<PB2);
        PORTA &= ~(1<<PA7);

        delay_loop(DELAYLOOP);

        // Reset the A and B direction flags.
        pwm_a = 0;
        pwm_b = 0;
    }

    // Set the PWM duty cycle to zero.
    OCR0A = 0;
    OCR0B = 0;

    // Restore interrupts.
    sei();

    // Save the pwm A and B duty values.
    registers_write_byte(REG_PWM_DIRA, pwm_a);
    registers_write_byte(REG_PWM_DIRB, pwm_b);
}

