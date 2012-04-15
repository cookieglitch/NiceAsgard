/*
 * light.c - HomeAutomation_prj.
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
#include "light.h"
#include <string.h>

#define NUM_OF_LIGHTS       2
tLight Light[NUM_OF_LIGHTS];
unsigned short LightTargetVal[NUM_OF_LIGHTS];

//*****************************************************************************
//
// Prototypes for the static functions
//
//*****************************************************************************
static void LightSetLightValue(unsigned short Value, unsigned char idx);
//*****************************************************************************
//
//! LightInit
//!
//!  \param  None
//!
//!  \return none
//!
//!  \brief  This function Initializes the Lights and sets up the Dimmer control
//
//*****************************************************************************
void LightInit(void)
{
  //
  // Set port
  //
  Light[0].pReg = (unsigned short*)&TB2CCR1;
  Light[1].pReg = (unsigned short*)&TB2CCR2;
  
  //
  // Turn all lights off
  //
  Light[0].usDimmerValue = 0;
  Light[1].usDimmerValue = 0;  
  
  LightSetLightValue(0, 0);
  LightSetLightValue(0, 1);

  //
  // Setup PWM. 3.6,7 are Outputs.
  //  
  P3DIR |= BIT6+BIT7;        
  // Select there output options for Timer control
  P3SEL0 |= BIT6+BIT7;  
  
  // SMCLK/8, enable int
  TB2CTL = TBSSEL_2 + ID_3 + TBIE;        
  // PWM Period is 1ms
  TB2CCR0 = 1000-1;             
  // CCR1 reset/set
  TB2CCTL1 = OUTMOD_7;    
  // CCR1 PWM duty cycle
  *(Light[0].pReg) = Light[0].usDimmerValue;
  // CCR2 reset/set
  TB2CCTL2 = OUTMOD_7;                    
  // CCR2 PWM duty cycle
  *(Light[1].pReg) = Light[1].usDimmerValue;        
  // SMCLK, up mode, clear TAR
  TB2CTL = TBSSEL_2 + MC_1 + ID_3 + TBIE + TBCLR; 
}

//*****************************************************************************
//
//! LightSetLightValue
//!
//!  \param  usDimmerValue - Value from 0 to 1000 where 0 is off and 1000 is 100%
//!          ucIdx - Index of the LED array.
//!
//!  \return none
//!
//!  \brief  This function updates the Dimmer Value of a specific light
//
//*****************************************************************************
void LightSetLightValue(unsigned short usDimmerValue, unsigned char ucIdx)
{
  // if value is very low then turn LED off
  if(usDimmerValue < 9)
  {
    usDimmerValue = 0;
  }
  LightTargetVal[ucIdx] = usDimmerValue;
}

//*****************************************************************************
//
//! LightServicePacket
//!
//!  \param  *DataInBuff -- Pointer to data packet
//!           DataLen --    Length of data packet
//!
//!  \return none
//!
//!  \brief  This function parses the CC3000 packet received from the 
//!          PC application.  It then updates the light with the new data
//
//*****************************************************************************
void LightServicePacket(unsigned char *DataInBuff, unsigned short DataLen)
{
  unsigned short usDimmerValue;
  unsigned char ucIdx;
  // There could be multiple packets in one read so we need to parse.
  // Since the data in the middle will be effectively erralevant we 
  // can skip it and just use the first pkt and the last.
  if(DataLen > 14)
  {
    memcpy(&DataInBuff[7], &DataInBuff[DataLen-7], 7);
    DataLen = 14;
  }
  while(DataLen)
  {
    //
    // First find start and enf of pkt
    //
    if( (*(DataInBuff+0) == 'S') && (*(DataInBuff+1) == '1') &&
        (*(DataInBuff+5) == 'S') && (*(DataInBuff+6) == '2') )
    {
      // Found data, move to data portion
      DataInBuff+=2;
      // Get Dimmer Val
      usDimmerValue = *DataInBuff;
      DataInBuff++;
      usDimmerValue = (((unsigned short)*DataInBuff)<<8) | usDimmerValue;
      // Find which Light fixture
      DataInBuff++;
      ucIdx = *DataInBuff;
      DataInBuff++;
      DataLen-=7;
      LightSetLightValue(usDimmerValue, ucIdx);
      DataInBuff++;
      DataInBuff++;
    }
    else
    {
      DataInBuff++;
      DataLen--;
    }
  }
}

/**********************************************************************//**
 * @brief  Timer2 B ISR, used as the Lights PWM and used to add filtering
 *         Timer period is 1ms.  Dimmervalues are only changed by +/- 1 per 
 *         timer period to smooth the transistion.
 * 
 * @param  none 
 *  
 * @return none
 *************************************************************************/
#pragma vector = TIMER2_B1_VECTOR
__interrupt void Timer2_B (void)
{  
  unsigned char i;
  switch(__even_in_range(TB2IV, TB2IV_TB2IFG))
  {
    case TB2IV_TB2IFG:
    {            
      for(i = 0; i < NUM_OF_LIGHTS; i++)
      {
        if( Light[i].usDimmerValue < LightTargetVal[i] )
        {
          Light[i].usDimmerValue++;
          //
          // Update PWM
          //
          *(Light[i].pReg) = Light[i].usDimmerValue;
        }
        else if( Light[i].usDimmerValue > LightTargetVal[i] )
        {
          Light[i].usDimmerValue--;
          //
          // Update PWM
          //
          *(Light[i].pReg) = Light[i].usDimmerValue;
        }
      }
      break;
    }
  }
}
