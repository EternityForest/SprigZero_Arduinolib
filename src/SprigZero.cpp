#include <stdlib.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
#include "SprigZero.h"
#include "Arduino.h"

void startTimer2RTCExt( void )
{
	/* Using 8bit Timer2 to generate the tick.
	 * A 32.768 KHz crystal must be attached to the appropriate pins.
	 * We then adjust the scale factor and counter to roll over at the top
	 * so we can get EXACT seconds for the Real Time clock.
	 *
	 * This code is correct for an ATmega328p (Arduino Uno) but will fail to function
	 * because the pins are used for an external crystal.
	 */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
    	TIMSK2 &= ~( _BV(OCIE2B)|_BV(OCIE2A)|_BV(TOIE2) );  // disable all Timer2 interrupts
    	TIFR2 |=  _BV(OCF2B)|_BV(OCF2A)|_BV(TOV2);		    // clear all pending interrupts
        ASSR = _BV(AS2);              				        // set Timer/Counter2 to be asynchronous from the CPU clock
                               					            // with a second external clock (32,768kHz) driving it.
        TCNT2  = 0x00;				 		                // zero out the counter
        TCCR2A = 0x00;						                // Normal mode
        TCCR2B = _BV(CS22) | _BV(CS20);				        // divide timer clock by 128 so counter will roll over at MAX
    
        while( ASSR & (_BV(TCN2UB)|_BV(OCR2AUB)|_BV(TCR2AUB))); // Wait until Timer2 update complete
    
        /* Enable the interrupt - this is okay as interrupts are currently globally disabled. */
        TIMSK2 |= _BV(TOIE2);					            // When the TOIE2 bit is written to one, the interrupt is enabled
    }
}

static uint32_t unixTime =0;

uint16_t readVCC()
{
    
    //XCas derivation of this algorithm for 1.225v ref:
    //solve(adc = (1.225/Vcc)*1023, Vcc)
    
    pinMode(REF_PIN,INPUT_PULLUP);
    uint16_t x = analogRead(REF_PIN);
    uint32_t y = 1253175L;
    //Compute VCC in millivolts
    y = y/x;
    return y;
    //Turn off to save power
    pinMode(REF_PIN,INPUT);
}



uint16_t readmV(uint8_t pin)
{

    uint32_t vcc = readVCC();
    //Take a reading and discard, because the pin may be a high impedence thing.
    analogRead(pin);
    uint32_t meas = analogRead(pin);
    vcc+= readVCC();
    
    //Average the Vcc, we have to assume it is constanr
    vcc = vcc>>2;
    
    //Get microvolts per ADC step
    vcc *= 1000000L;
    vcc /= 1023;

    //Multiply by the number of ADC steps to get uV
    meas *= vcc;
    
    //Divide by 1000 to get millivolts.
    return (meas/1000);
}

bool detectExternalPower()
{
    
}

uint16_t doPowerManagement(uint8_t seconds)
{
    
   start:
        pinMode(LDO_CONTROL,INPUT);
        delay(3);
        uint16_t vcc = readVCC();
        
        //If we have available input power, Vcc will be high when we enable the LDO.
        //Otherwise, we should just sleep
        if(vcc> 2650)
        {
            //Measuring battery voltage requires turning off the charger,
            //Because the charger is on our end of a limiting resistor.
            //It also means we need to wait for the voltage to equalize, because we use a large capacitor.
            pinMode(LDO_CONTROL, OUTPUT);
            digitalWrite(LDO_CONTROL,LOW);
            vcc = readVCC();
            
            for(uint8_t x =0;x<10;x++)
            {
                //Too low!  Stop even trying to measure anything
                if (vcc< 2250)
                {
                    //PWM it, because otherwise
                    //We 
                    analogWrite(LDO_CONTROL,25);
                    break;
                }
                vcc=readVCC();
                delay(1);
            }
            
            //Compensate for voltage drop in the 10-ohm resistor.
            vcc += 14;
            
            //Use PWM for charging, we can't actually fully sleep.
            sleepMode(SLEEP_IDLE);

            //At this point x should be the true battery voltage.
            
            //Less than the float charge voltage, leave it off.
            //Less that 2.4v, do a very slow charge. the LDO can't take more than 0.
            if(vcc<2400)
            {
                analogWrite(LDO_CONTROL,45);
            }
            
            //Batt voltage not deeply discharged. Assuming 21v maximum input,
            //We need to limit current to 15mA I think.
            else if(vcc<2575)
            {
                analogWrite(LDO_CONTROL,140);
            }
            else
            {
                //If we are fully charged, we have no reason
                //to waste power with the PWM stuff.
                sleepMode(SLEEP_POWER_SAVE);
            }
        }
        //No input power, turn off the reg 
        else
        {
            pinMode(LDO_CONTROL,OUTPUT);
            //Turn off the reg, so it does not 
            digitalWrite(LDO_CONTROL, LOW);
             //Use real deep sleep
            sleepMode(SLEEP_POWER_SAVE);
        }
    
    //Use the selected sleep mode
    if(seconds)
    {
        unsigned long m = millis();
        while((millis()-m)>1000)
        {
            sleep();
        }
        seconds --;
        goto start;
    }
    return vcc;
}


void setUnixTime(uint32_t t)
{
    noInterrupts();
    unixTime=t;
    interrupts();
}

uint32_t getUnixTime()
{
    uint32_t x = 0;
    noInterrupts();
    x=unixTime;
    interrupts();
    return x;
}

/*
 * Tick ISR for the RTC.  All this does is increment the RTC tick count, once per second.
 * Use ISR_NOBLOCK where there is an important timer running, that should preempt the RTC.
 * As long as it completes within one second, then there is no issue.
 */
ISR(TIMER2_OVF_vect, ISR_NAKED ISR_NOBLOCK ) __attribute__ ((hot, flatten));
ISR(TIMER2_OVF_vect)
{
	unixTime +=1;
	reti();
}
