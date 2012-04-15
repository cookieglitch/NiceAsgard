/*******************************************************************************
 *
 * thermostat.c
 * HomeAutomation_prj
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

#define THERMOSTAT_STATE_IDLE       1
#define THERMOSTAT_STATE_HEATING    2
#define THERMOSTAT_STATE_COOLING    3
#define THERMOSTAT_STATE_ALARM      4

//*****************************************************************************
//
//!
//!  This structure is used to define a thermostat's object and contains
//!	 only members and no methods
//
//*****************************************************************************
typedef struct _thermostat_t
{
    unsigned short Temperature;                
    unsigned short SetPoint;
    unsigned short AlarmHighPoint;
    unsigned short AlarmLowPoint;
    unsigned char  State;    
} tThermostat;

// Function Declaration
extern void ThermostatInit(void);
extern void ThermostatServicePacket(unsigned char *DataInBuff, unsigned short DataLen);
extern void ThermostatUpdateTemperature(void);
extern unsigned short ThermostatGetTemperature(void);
extern void ThermostatUpdateStatus(void);
extern unsigned char ThermostatGetState(void);
