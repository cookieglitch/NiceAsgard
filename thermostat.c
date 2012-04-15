/*
 * thermostat.c - HomeAutomation_prj.
 * 
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the project's author nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "msp430fr5739.h"
#include "thermostat.h"

volatile unsigned short usADCResult = 0;
unsigned short usThermostatTermperature;

// Create new Thermostat
tThermostat Thermostat;

//*****************************************************************************
//
// Prototypes for the static functions
//
//*****************************************************************************
static void ThermistorSetup(void);
static void TakeADCMeas(void);
//*****************************************************************************
//
//! ThermistorSetup
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Initializes Thermistor
//!				
//
//*****************************************************************************
void ThermistorSetup(void)
{
  // ~16KHz sampling
  //Turn on Power
  P2DIR |= BIT7;
  P2OUT |= BIT7;
  
  // Configure ADC
  P1SEL1 |= BIT4;  
  P1SEL0 |= BIT4; 
  
  // Configure ADC
  ADC10CTL0 &= ~ADC10ENC; 
  // ADC10ON, S&H=192 ADC clks
  ADC10CTL0 = ADC10SHT_7 + ADC10ON;        
  // ADCCLK = MODOSC = 5MHz
  ADC10CTL1 = ADC10SHS_0 + ADC10SHP + ADC10SSEL_0; 
  // 10-bit conversion results
  ADC10CTL2 = ADC10RES;                  
  // A4 ADC input select; Vref=AVCC  
  ADC10MCTL0 = ADC10INCH_4;             
  // Enable ADC conv complete interrupt
  ADC10IE = ADC10IE0;                      
}

//*****************************************************************************
//
//! TakeADCMeas
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Take ADC measurement by starting ADC and stopping the CPU until 
//!         complete
//!				
//
//*****************************************************************************
void TakeADCMeas(void)
{  
  while (ADC10CTL1 & BUSY); 
  // Start conversion 
  ADC10CTL0 |= ADC10ENC | ADC10SC ;   
  // LPM0, ADC10_ISR will force exit
  __bis_SR_register(CPUOFF + GIE); 
  // For debug only
  __no_operation();                       
}

//*****************************************************************************
//
//! ThermostatInit
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Inititializes the thermostat ADC
//!				
//
//*****************************************************************************
void ThermostatInit(void)
{
  unsigned char i;
  ThermistorSetup();
  // Take 8 measurements
  for(i = 8; i; i--)
  {
    TakeADCMeas();
    Thermostat.Temperature += usADCResult;
  }
  // Then average it and then Update the Temperature
  Thermostat.Temperature = Thermostat.Temperature>>3;
  
  // Set the defaults.  We don't have a connection yet to the
  // Host so lets just use our current temperature for the set point
  // and disable the alarm points.
  
  // Set the Setpoint to the current temperature
  Thermostat.SetPoint = Thermostat.Temperature;
  // Set the Limits to Min and Max 
  Thermostat.AlarmHighPoint = 0;
  Thermostat.AlarmLowPoint = 1024;
  Thermostat.State = THERMOSTAT_STATE_IDLE;
}


//*****************************************************************************
//
//! ThermostatServicePacket
//!
//!  \param  *DataInBuff -- Pointer to data packet
//!           DataLen --    Length of data packet
//!
//!  \return none
//!
//!  \brief  This function parses the CC3000 packet received from the 
//!          PC application.  It then updates the Thermostat setpoint and limits 
//!          with the new data
//
//*****************************************************************************
void ThermostatServicePacket(unsigned char *DataInBuff, unsigned short DataLen)
{
  unsigned char ucOpCode;
  // There could be multiple packets in one read so we need to parse.
  while(DataLen)
  {
    //
    // First find start and enf of pkt
    //
    if( (*(DataInBuff+0) == 'S') && (*(DataInBuff+1) == '1') &&
        (*(DataInBuff+9) == 'S') && (*(DataInBuff+10) == '2') )
    {
      // Found data, move to data portion
      DataInBuff+=2;
      // Get Instruction.
      // It is either a new set point or a Twitter control
      // message
      ucOpCode = *DataInBuff;
      DataInBuff++;
      if(ucOpCode == 0)
      {
        // New Setpoint        
        Thermostat.SetPoint = *DataInBuff;
        DataInBuff++;
        Thermostat.SetPoint = (((unsigned short)*DataInBuff)<<8) |
                                 Thermostat.SetPoint;
        DataInBuff++;
        // Low Limit
        Thermostat.AlarmLowPoint = *DataInBuff;
        DataInBuff++;
        Thermostat.AlarmLowPoint = (((unsigned short)*DataInBuff)<<8) |
                                      Thermostat.AlarmLowPoint;
        DataInBuff++;
        // High Limit
        Thermostat.AlarmHighPoint = *DataInBuff;
        DataInBuff++;
        Thermostat.AlarmHighPoint = (((unsigned short)*DataInBuff)<<8) |
                                       Thermostat.AlarmHighPoint;
        DataInBuff++;
        
        DataLen-=11;
        DataInBuff++;
        DataInBuff++;
      }
    }
    else
    {
      DataInBuff++;
      DataLen--;
    }
  }
}


//*****************************************************************************
//
//! ThermostatUpdateTemperature
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Takes a Temperature Measurement and stores the temperature.
//!				
//
//*****************************************************************************
void ThermostatUpdateTemperature(void)
{
  TakeADCMeas();
  // Update the Temperature
  Thermostat.Temperature = usADCResult;
}

//*****************************************************************************
//
//! ThermostatUpdateStatus
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Uses the Temperature to update it's state based on levels.
//!         States: THERMOSTAT_STATE_IDLE, THERMOSTAT_STATE_HEATING, 
//!                 THERMOSTAT_STATE_COOLING, THERMOSTAT_STATE_ALARM
//!				
//
//*****************************************************************************
void ThermostatUpdateStatus(void)
{
  #define STATE_CHANGE_THREASHOLD     20
  static unsigned char ucStateChangeAlarmFilterCount = 0;

  // The Alarm Points and setpoint are in ADC counts.
  // Thus as the temperature gets hotter the Thermistor
  // Resistance gets smaller and the ADC counts decrease.
  // Therefore, the evaluation is the opposite to what would 
  // be intuitive.
  if( (Thermostat.Temperature < Thermostat.AlarmHighPoint) ||
      (Thermostat.Temperature > Thermostat.AlarmLowPoint) )
  {
    ucStateChangeAlarmFilterCount++;
    if(ucStateChangeAlarmFilterCount >= STATE_CHANGE_THREASHOLD)
    {
      Thermostat.State = THERMOSTAT_STATE_ALARM;
    }
  }
  else
  {
    ucStateChangeAlarmFilterCount = 0;
    Thermostat.State = THERMOSTAT_STATE_IDLE;
  }
  
}
//*****************************************************************************
//
//! ThermostatGetState
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Returns the thermostat's current state.
//!				
//
//*****************************************************************************
unsigned char ThermostatGetState(void)
{
  return (Thermostat.State);
}
//*****************************************************************************
//
//! ThermostatGetTemperature
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Returns the thermostat's current Temperature.
//!				
//
//*****************************************************************************
unsigned short ThermostatGetTemperature(void)
{
  return(Thermostat.Temperature);
}

/**********************************************************************//**
 * @brief  ADC10 ISR for MODE3 and MODE4
 * 
 * @param  none 
 *  
 * @return none
 *************************************************************************/
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  switch(__even_in_range(ADC10IV,ADC10IV_ADC10IFG))
  {
    case ADC10IV_NONE: break;               // No interrupt
    case ADC10IV_ADC10OVIFG: break;         // conversion result overflow
    case ADC10IV_ADC10TOVIFG: break;        // conversion time overflow
    case ADC10IV_ADC10HIIFG: break;         // ADC10HI
    case ADC10IV_ADC10LOIFG: break;         // ADC10LO
    case ADC10IV_ADC10INIFG: break;         // ADC10IN
    case ADC10IV_ADC10IFG: 
             usADCResult = ADC10MEM0;             
             __bic_SR_register_on_exit(CPUOFF);                                              
             break;                          // Clear CPUOFF bit from 0(SR)                         
    default: break; 
  }  
}
