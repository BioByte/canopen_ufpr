/*
This file is part of CanFestival, a library implementing CanOpen Stack.

Copyright (C): Edouard TISSERANT and Francis DUPIN
AVR Port: Andreas GLAUSER and Peter CHRISTEN

See COPYING file for copyrights details.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// AVR implementation of the  CANopen timer driver, uses Timer 3 (16 bit)

// Includes for the Canfestival driver
#include "canfestival.h"
#include "timer.h"
#include "DueTimer.h"

#define TimerCounter    TC2->TC_CV
#define TimerAlarm      TC2->TC_RC

void callTimeDispatch(void)

/************************** Modul variables **********************************/
// Store the last timer value to calculate the elapsed time
static TIMEVAL last_time_set = TIMEVAL_MAX;     

void initTimer(void)
/******************************************************************************
Initializes the timer, turn on the interrupt and put the interrupt time to zero
INPUT	void
OUTPUT	void
******************************************************************************/
{
  unsigned int dummy;

  // First, enable the clock of the TIMER
  AT91F_PMC_EnablePeriphClock (AT91C_BASE_PMC, 1 << AT91C_ID_TC);
  // Disable the clock and the interrupts
  TC2->TC_CCR = TC_CCR_CLKDIS ; //ESSE ESTÁ PRONTO E FINALIZADO
  TC2->TC_IDR = 0xFFFFFFFF ; //DEIXA IGUAL MAS NAO SABEMOS O QUE É
  // Clear status bit
  dummy = TC2->TC_SR; //FEITO
  // Suppress warning variable "dummy" was set but never used
  dummy = dummy;

  // Set the Mode of the Timer Counter (MCK / 128)
  TC2->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK4; //FEITO

  // Enable the clock
  TC2->TC_CCR = TC_CCR_CLKEN; //FEITO

  // Open Timer interrupt
  AT91F_AIC_ConfigureIt (AT91C_BASE_AIC, AT91C_ID_TC, TIMER_INTERRUPT_LEVEL,
			 AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE, timer_can_irq_handler);

  TC2->TC_IER = TC_SR_CPCS;  //  IRQ enable CPC FEITO

  

  //AT91F_AIC_EnableIt (AT91C_BASE_AIC, AT91C_ID_TC2);

  // Start Timer
  TC2->TC_CCR = TC_CCR_SWTRG ; //FEITO



  NVIC_ClearPendingIRQ(Timers[timer].irq);
	NVIC_EnableIRQ(Timers[timer].irq);
  TC_Start(Timers[timer].tc, Timers[timer].channel);    
}

void setTimer(TIMEVAL value)
/******************************************************************************
Set the timer for the next alarm.
INPUT	value TIMEVAL (unsigned long)
OUTPUT	void
******************************************************************************/
{
  TimerAlarm += (int)value;	// Add the desired time to timer interrupt time
}

TIMEVAL getElapsedTime(void)
/******************************************************************************
Return the elapsed time to tell the Stack how much time is spent since last call.
INPUT	void
OUTPUT	value TIMEVAL (unsigned long) the elapsed time
******************************************************************************/
{
  unsigned int timer = TimerCounter;            // Copy the value of the running timer
  if (timer > last_time_set)                    // In case the timer value is higher than the last time.
    return (timer - last_time_set);             // Calculate the time difference
  else if (timer < last_time_set)
    return (last_time_set - timer);             // Calculate the time difference
  else
    return TIMEVAL_MAX;
}

void callTimeDispatch(void)
{
  last_time_set = TimerCounter;
  TimeDispatch();                               // Call the time handler of the stack to adapt the elapsed time
}



