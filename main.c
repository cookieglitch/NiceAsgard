//***************************************************************************************
//  Ultime Road Test - Element 14
//	Home Automation System Base Station
//
//  John Tiernan
//  April 2012
//  Built with Code Composer Studio v5
//***************************************************************************************

#include <msp430.h>				
#include "light.h"
#include "thermostat.h"
#include "wlan.h"
#include "evnt_handler.h"    // callback function declaration
#include "nvmem.h"
#include "socket.h"
#include "CC3000_common.h"
#include "netapp.h"
#include "version.h"
#include <string.h>
//#include "msp430fr5739.h"

#include "remotedefs.h"
#include "states.h"

//System Settings
#define SYSTEMID 0x00

#define ONBOARD_GUI_EN	0
#define CAP_UI_EN		0

int SYSTEM_STATE = DAY0;

//Output settings
settingsMAX.blindPos = 1024;
settingsMAX.lightLev = 1024;
settingsMIN.blindPos = 0;
settingsMIN.lightLev = 0;

/**
 * State		Settings
 * dayNorm		blindPos = 1024, lightLev = 0
 * nightNorm	blindPos = 0, lightLev = 1024
 * film			blindPos = 0, lightLev = 0 / lightLev = 250
 * open			blindPos = 1024, lightLev = 0
 * closed		blindPos = 0, lightLev = 0
 */

dayNorm.blindPos = 1024;
dayNorm.lightLev = 0;
nightNorm.blindPos = 0;
nightNorm.lightLev = 1024;
open.blindPos = 1024;
open.lightLev = 0;
closed.blindPos = 0;
closed.lightLev = 0;

film.blindPos = 0
film.lightLev = 250;

//Copypasta from demo app
//
// Simple Config Prefix
//
char aucCC3000_prefix[] = { 'P', 'D', 'E' };
#define CC3000_RX_BUFFER_SIZE						(128)

//
// Reception from the air, buffer - the max data length  + headers
//
#if defined(__IAR_SYSTEMS_ICC__)
__no_init unsigned char pucCC3000_Rx_Buffer[CC3000_RX_BUFFER_SIZE + 100];
#else
unsigned char pucCC3000_Rx_Buffer[CC3000_RX_BUFFER_SIZE + 100];
#endif

static unsigned char CC3000ConectionState;
//
// These are the CC3000 connection status states
//
#define CC3000_INIT_STATE             1
#define CC3000_NOT_CONNECTED_STATE    2
#define CC3000_CONNECTED_STATE        3
#define CC3000_CONNECTED_PORTS_OPEN_STATE 4

//
// If no connection is made to the AP then this sets the period in which
// the connection will be attempted again
// At 100ms intervals 600 is 1 minute
//
#define NUM_OF_SKIP_ATTEMPTS          600

unsigned long g_ulSocketUdpLightBulbs, g_ulSocketUdpThermostatsRcv,
		g_ulSocketUdpThermostatsSend;
unsigned long g_ulSmartConfigFinished;
unsigned char g_ucHaveIpAddress = 0;

#define MCLK        8000000
//
// uncomment this if you would like to use DHCP
// Maybe controlled in preprocessor's defined symbols in IAR
//
//#define DHCP_EN                 1
//
// Other Wise, you can specify the IP address here
//
unsigned char g_ThisIpAddress[4] = {192, 168, 55, 3};
unsigned char g_ThisMulticastAddress[4] = {224, 0, 0, 1};


//Message buffers
int message = 0;

//Received messages
unsigned char stateBuffer[100];



void sys_init()
{
	//More copypasta for system basics (Switches etc)
	// Startup clock system in max. DCO setting ~8MHz
	// This value is closer to 10MHz on untrimmed parts
	CSCTL0_H = 0xA5;                          // Unlock register
	CSCTL1 |= DCOFSEL0 + DCOFSEL1;            // Set max. DCO setting
	CSCTL2 = SELA__VLOCLK + SELS__DCOCLK + SELM__DCOCLK;
	CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;        // set all dividers
	CSCTL0_H = 0x01;

	// Turn off temp.
	REFCTL0 |= REFTCOFF;
	REFCTL0 &= ~REFON;

    // Enable switches
    // (S1)P4.0 is configured as a switch
    // (S2)P4.1  is reserved for the CC3000
    P4OUT |= S1_BIT;                     // Configure pullup resistor
    P4DIR &= ~(S1_BIT);                  // Direction = input
    P4REN |= S1_BIT;                     // Enable pullup resistor

	// P4.1 - WLAN enable full DS
	P4OUT &= ~BIT1;
	P4DIR |= BIT1;
	P4SEL1 &= ~BIT1;
	P4SEL0 &= ~BIT1;

	// Enable LEDs
	P3OUT &= ~(BIT6 + BIT7 + BIT5 + BIT4);
	P3DIR |= BIT6 + BIT7 + BIT5 + BIT4;
	PJOUT &= ~(BIT0 + BIT1 + BIT2 + BIT3);
	PJDIR |= BIT0 + BIT1 + BIT2 + BIT3;

	// Terminate Unused GPIOs
	// P1.0 - P1.6 is unused
	P1OUT &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT5 + BIT6 + BIT7);
	P1DIR &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT5 + BIT6 + BIT7);
	P1REN |= (BIT0 + BIT1 + BIT2 + BIT3 + BIT5 + BIT6 + BIT7);

	// P1.4 is used as input from NTC voltage divider
	// Set it to output low
	P1OUT &= ~BIT4;
	P1DIR |= BIT4;

	// Configure the SPI CS to be on P1.3
	P1OUT |= BIT3;
	P1DIR |= BIT3;
	P1SEL1 &= ~BIT3;
	P1SEL0 &= ~BIT3;

	// P2.2 - P2.6 is unused
	P2OUT &= ~(BIT2 + BIT4 + BIT5 + BIT6);
	P2DIR &= ~(BIT2 + BIT4 + BIT5 + BIT6);
	P2REN |= (BIT2 + BIT4 + BIT5 + BIT6);

	// Configure SPI IRQ line on P2.3
	P2DIR &= (~BIT3);
	P2SEL1 &= ~BIT3;
	P2SEL0 &= ~BIT3;
	// Clear Flag
	P2IFG &= ~BIT3;

	// Configure UART pins P2.0 & P2.1
	P2SEL1 |= BIT0 + BIT1;
	P2SEL0 &= ~(BIT0 + BIT1);

	// P2.7 is used to power the voltage divider for the NTC thermistor
	P2OUT &= ~BIT7;
	P2DIR |= BIT7;

	// P3.0,P3.1 and P3.2 are accelerometer inputs
	P3OUT &= ~(BIT0 + BIT1 + BIT2);
	P3DIR &= ~(BIT0 + BIT1 + BIT2);
	P3REN |= BIT0 + BIT1 + BIT2;

	// PJ.0,1,2,3 are used as LEDs
	// crystal pins for XT1 are unused
	PJOUT &= ~(BIT4 + BIT5);
	PJDIR &= ~(BIT4 + BIT5);
	PJREN |= BIT4 + BIT5;
	
	// TODO Enable a few GPIO as inputs for UI
	// May change depending on UI interface (Capacitive?)
	#if CAP_UI_EN == 1
	//Setup for Cap UI
	
	#else
	//Setup for switch UI
	
	#endif
}

/**
 * Copypasta ALL the basic functions from HomeAutomationExample!
 */
//*****************************************************************************
//
//! Delay100ms
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Performs a long delay of 100ms based on MCLK
//!				
//
//*****************************************************************************
void Delay100ms() {
	__delay_cycles(MCLK / 10);
	__no_operation();
}
void SaveAndResetSystem(void) {
	// Nothing critical required to save to FRAM.
	// Rest CPU by writing to WD with illegal password.
	WDTCTL = (0xFF00u) + WDTHOLD;
}
//*****************************************************************************
//
//! ReadMessages
//!
//! \param  none
//!
//! \return true if received a message, false if no message returned
//!
//! \brief  The function checks to see if the CC3000 has a message to read.
//!         if so, then reads and parses.
//!				
//
//*****************************************************************************
unsigned char ReadMessages(void) {
	unsigned char i;
	unsigned char ucMsgStatus = 0;
	signed short sRxMsgLen;
	fd_set socks;
	struct timeval timeout;
	int iReadSocks;
	sockaddr tSocketAddr;
	socklen_t tRxPacketLength;
	socklen_t optlen;
	unsigned long uloptval;
	signed char cRetVal;
	
	//
	// Timeout set for 1 ms
	//
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	
	//
	// First use select to determine if there are any messages ready
	// in the CC3000 for the thermostat
	//
	FD_ZERO(&socks);
	FD_SET(g_ulSocketUdpThermostatsRcv, &socks);
	FD_SET(g_ulSocketUdpLightBulbs, &socks);
	
	// Test Light Socket to see if it is still open
	cRetVal = getsockopt(g_ulSocketUdpLightBulbs, SOL_SOCKET,
						 SOCKOPT_RECV_TIMEOUT, &uloptval, &optlen);
	if (cRetVal == -1) {
		// We could try and reopen the port if it was closed
		// Instead We will just do a CPU reset and start over
		SaveAndResetSystem();
	}
	// Test Thermostat Socket to see if it is still open
	cRetVal = getsockopt(g_ulSocketUdpThermostatsRcv, SOL_SOCKET,
						 SOCKOPT_RECV_TIMEOUT, &uloptval, &optlen);
	if (cRetVal == -1) {
		// We could try and reopen the port if it was closed
		// Instead We will just do a CPU reset and start over
		SaveAndResetSystem();
	}
	//
	// First argument is Number of sockets +
	// We are binded to 2 sockets so we pass a 3
	//
	iReadSocks = select(3, &socks, 0, 0, &timeout);
	if (iReadSocks > 0) {
		LedOn(LED_RX);
		// 2 sockets so we cycle twice
		for (i = 0; i < 2; i++) {
			// Check if socket has data
			if (FD_ISSET(i, &socks)) {
				if (i == g_ulSocketUdpThermostatsRcv) {
					sRxMsgLen = recvfrom(g_ulSocketUdpThermostatsRcv,
										 pucCC3000_Rx_Buffer, CC3000_RX_BUFFER_SIZE, 0,
										 &tSocketAddr, &tRxPacketLength);
					//
					// double check that we have data.  Should return > 0
					//
					if (sRxMsgLen > 0) {
						ThermostatServicePacket(pucCC3000_Rx_Buffer, sRxMsgLen);
						ucMsgStatus = 1;
						stateBuffer = pucCC3000_Rx_Buffer;
					}
				} else if (i == g_ulSocketUdpLightBulbs) {
					sRxMsgLen = recvfrom(g_ulSocketUdpLightBulbs,
										 pucCC3000_Rx_Buffer, CC3000_RX_BUFFER_SIZE, 0,
										 &tSocketAddr, &tRxPacketLength);
					//
					// double check that we have data.  Should return > 0
					//
					if (sRxMsgLen > 0) {
						LightServicePacket(pucCC3000_Rx_Buffer, sRxMsgLen);
						ucMsgStatus = 1;
						stateBuffer = pucCC3000_Rx_Buffer;
					}
				}
			}
		}
		LedOff(LED_RX);
	}
	return (ucMsgStatus);
}

//*****************************************************************************
//
//! This function: init_spi
//!
//!  \param  none
//!
//!  \return none
//!
//!  \brief  initializes an SPI interface
//
//*****************************************************************************
int init_spi(void) {
	// Select the SPI lines: MISO/MOSI on P1.6,7 CLK on P2.2
	P1SEL1 |= (BIT6 + BIT7);
	P1SEL0 &= (~(BIT6 + BIT7));
	
	P2SEL1 |= (BIT2);
	P2SEL0 &= ~BIT2;
	
	UCB0CTLW0 |= UCSWRST; // **Put state machine in reset**
	UCB0CTLW0 |= (UCMST + UCSYNC + UCMSB); // 3-pin, 8-bit SPI master
	// Clock polarity high, MSB
	UCB0CTLW0 |= UCSSEL__SMCLK; // UCSSEL__SMCLK
	UCB0BR0 = 2; // /8 of SMCLK = 24Mhz (3mhz)
	UCB0BR1 = 0; //
	
	UCB0CTLW0 &= ~UCSWRST; // **Initialize USCI state machine**
	
	return (ESUCCESS);
}

//*****************************************************************************
//
//! sendDriverPatch
//!
//! \param  pointer to the length
//!
//! \return none
//!
//! \brief  The function returns a pointer to the driver patch
//
//*****************************************************************************
char *sendDriverPatch(unsigned long *Length) {
	*Length = 0;
	return NULL;
}

//*****************************************************************************
//
//! sendBootLoaderPatch
//!
//! \param  pointer to the length
//!
//! \return none
//!
//! \brief  The function returns a pointer to the boot loader patch: since there is no patch yet -
//!				it returns 0
//
//*****************************************************************************
char *sendBootLoaderPatch(unsigned long *Length) {
	*Length = 0;
	return NULL;
}

//*****************************************************************************
//
//! sendWLFWPatch
//!
//! \param  pointer to the length
//!
//! \return none
//!
//! \brief  The function returns a pointer to the FW patch
//
//*****************************************************************************

char *sendWLFWPatch(unsigned long *Length) {
	*Length = 0;
	return NULL;
}

//*****************************************************************************
//
//! ReadWlanInterruptPin
//!
//! \param  none
//!
//! \return none
//!
//! \brief  return wlan interrup pin
//
//*****************************************************************************

long ReadWlanInterruptPin(void) {
	return (P2IN & BIT3);
}

//*****************************************************************************
//
//! Enable waln IrQ pin
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Nonr
//
//*****************************************************************************

void WlanInterruptEnable() {
	P2IES |= BIT3;
	P2IE |= BIT3;
}

//*****************************************************************************
//
//! Disable waln IrQ pin
//!
//! \param  none
//!
//! \return none
//!
//! \brief  Nonr
//
//*****************************************************************************

void WlanInterruptDisable() {
	P2IE &= ~BIT3;
}

//*****************************************************************************
//
//! WriteWlanPin
//!
//! \param  new val
//!
//! \return none
//!
//! \brief  void
//
//*****************************************************************************

void WriteWlanPin(unsigned char val) {
	if (val) {
		P4OUT |= BIT1;
	} else {
		P4OUT &= ~BIT1;
	}
	
}

//*****************************************************************************
//
//! CC3000_UsynchCallback
//!
//! \param  Event type
//!
//! \return none
//!
//! \brief  The function handles asynchronous events that come from CC3000  
//!		  device and operates a LED4 to have an on-board indication
//
//*****************************************************************************

void CC3000_UsynchCallback(long lEventType, char * data, unsigned char length) {
	if (lEventType == HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE)
	{
		g_ulSmartConfigFinished = 1;
	}
	
	if (lEventType == HCI_EVNT_WLAN_UNSOL_CONNECT)
	{
		CC3000ConectionState = CC3000_CONNECTED_STATE;
		
		//
		// Turn on the LED1
		//
		LedOn(LED_CONNECTION);
#ifndef DHCP_EN
		// we are using a static IP address so we don't have to wait for an IP
		g_ucHaveIpAddress = 1;
#endif
	}
	
	if (lEventType == HCI_EVNT_WLAN_UNSOL_DISCONNECT)
	{
		//
		// Turn off the LED1
		//
		LedOff(LED_CONNECTION);
        
        // 
        // If we already have an open connection to the AP and we are informed 
        // that we have disconnected ( This could happen if get out of range or 
        // the AP looses power) then reset and reestablish a connection.
        //
        if(CC3000ConectionState == CC3000_CONNECTED_PORTS_OPEN_STATE)
        {
            // Connection to AP unintentionally closed. Reset
            SaveAndResetSystem();
        }
        
        CC3000ConectionState = CC3000_NOT_CONNECTED_STATE;
	}
	
	if (lEventType == HCI_EVNT_WLAN_UNSOL_DHCP)
	{
		//
		// Should have an IP address now. Can send data.
		//
		g_ucHaveIpAddress = 1;
		//
		// Get our assigned address from the passed data
		//
		g_ThisIpAddress[0] = data[3];
		g_ThisIpAddress[1] = data[2];
		g_ThisIpAddress[2] = data[1];
		g_ThisIpAddress[3] = data[0];
	}
}

//*****************************************************************************
//
//! initCC3000Driver
//!
//!  \param  None
//!
//!  \return none
//!
//!  \brief  The function initializes a CC3000 device and triggers it to 
//!          start operation
//
//*****************************************************************************
int initCC3000Driver(void) {
	//
	//init all layers
	//
	init_spi();
	
	// WLAN On API Implementation
	//
	wlan_init(CC3000_UsynchCallback, sendWLFWPatch, sendDriverPatch,
			  sendBootLoaderPatch, ReadWlanInterruptPin, WlanInterruptEnable,
			  WlanInterruptDisable, WriteWlanPin);
	
	//
	// add just over a 1 second delay to ensure that the slow clock is stable
	//
	__delay_cycles(2410000);
	
	//
	// Trigger a WLAN device. Set true because there are patches available
	//
	wlan_start(false);
	
	//
	// Mask out all non-required events from CC3000
	//
#ifdef DHCP_EN
    //
    // Setup DHCP.  Although this should be unnecessary as DHCP is the default
    //
    unsigned char pucSubnetMask[4], pucIP_Addr[4], pucIP_DefaultGWAddr[4],
	pucDNS[4];
    memset(pucSubnetMask,       0, sizeof(pucSubnetMask));
    memset(pucIP_Addr,          0, sizeof(pucIP_Addr));
    memset(pucIP_DefaultGWAddr, 0, sizeof(pucIP_DefaultGWAddr));
    memset(pucDNS,              0, sizeof(pucDNS));
    
    netapp_dhcp((unsigned long *) pucIP_Addr, (unsigned long *) pucSubnetMask,
				(unsigned long *) pucIP_DefaultGWAddr, (unsigned long *) pucDNS);
    wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE |
                        HCI_EVNT_WLAN_UNSOL_INIT |
                        HCI_EVNT_WLAN_ASYNC_PING_REPORT);
	
#else
    //
    // Setup DHCP
    //
	unsigned char pucSubnetMask[4], pucIP_Addr[4], pucIP_DefaultGWAddr[4],
	pucDNS[4];
    memset(pucSubnetMask,       0xFF, sizeof(pucSubnetMask));
    memcpy(pucIP_Addr, g_ThisIpAddress, sizeof(g_ThisIpAddress));
	
    pucIP_DefaultGWAddr[0] = g_ThisIpAddress[0];
    pucIP_DefaultGWAddr[1] = g_ThisIpAddress[1];
    pucIP_DefaultGWAddr[2] = g_ThisIpAddress[2];
    pucIP_DefaultGWAddr[3] = 1;
	
    memset(pucDNS,              0, sizeof(pucDNS));
    netapp_dhcp((unsigned long *) pucIP_Addr, (unsigned long *) pucSubnetMask,
				(unsigned long *) pucIP_DefaultGWAddr, (unsigned long *) pucDNS);
    wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE  |
                        HCI_EVNT_WLAN_UNSOL_INIT |
                        HCI_EVNT_WLAN_UNSOL_DHCP |
                        HCI_EVNT_WLAN_ASYNC_PING_REPORT);
#endif
	return (0);
}
//*****************************************************************************
//
//! StartSmartConfig
//!
//!  \param  None
//!
//!  \return none
//!
//!  \brief  The function triggers a smart configuration process on CC3000.
//!			it exists upon completion of the process
//
//*****************************************************************************
void StartSmartConfig(void) {
    g_ulSmartConfigFinished = 0;
    //
    // Reset all the previous configuration
    //
    wlan_ioctl_set_connection_policy(0, 0, 0);
    
    //
    // Trigger the Smart Config process
    //
    
    // Start blinking LED2 during Smart Configuration process
    LedOff(LED_SIMPLE_CONFIG);
    
    wlan_first_time_config_set_prefix(aucCC3000_prefix);
    
    //
    // Start the Smart Config process
    //
    wlan_first_time_config_start();
    
    //
    // Wait for simple config finished
    // Set a 1 minute timeout to be safe 
    //
    unsigned char ucTimeOut = 60;
    while (g_ulSmartConfigFinished == 0 &&
           ucTimeOut) 
    {
		__delay_cycles(6000000);
		
		LedOn(LED_SIMPLE_CONFIG);
		
		__delay_cycles(6000000);
		
		LedOff(LED_SIMPLE_CONFIG);
		
		ucTimeOut--;
    }
    
    LedOff(LED_SIMPLE_CONFIG);
    
    //
    // Configure to connect automatically to the AP retrieved in the 
    // simple config process
    //
    wlan_ioctl_set_connection_policy(0, 0, 1);
    
    //
    // reset the CC3000
    // 
    wlan_stop();
    
    Delay100ms();
    
    wlan_start(false);
	
    //
    // Mask out all non-required events
    //
#ifdef DHCP_EN
    wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE  |
						HCI_EVNT_WLAN_UNSOL_INIT |
						HCI_EVNT_WLAN_ASYNC_PING_REPORT);
#else    
    wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE  |
                        HCI_EVNT_WLAN_UNSOL_INIT |
                        HCI_EVNT_WLAN_UNSOL_DHCP |
                        HCI_EVNT_WLAN_ASYNC_PING_REPORT);
#endif
}

//*****************************************************************************
//
//! ServiceCC300StateMachine
//!
//!  \param  None
//!
//!  \return none
//!
//!  \brief  This function is monitors the CC3000 connection status and establishes 
//!			a connection when not connected.
//
//*****************************************************************************
void ServiceCC300StateMachine(void) {
	unsigned char i;
	static unsigned short ucNumberOfSkippedAttempts = 0;
	static unsigned char ucPortsOpen = 0;
	sockaddr tSocketAddr;
	signed char cRetVal;
	
	switch (CC3000ConectionState) {
		case CC3000_INIT_STATE:
			CC3000ConectionState = CC3000_NOT_CONNECTED_STATE;
			//
			//  Start the CC3000
			//
			initCC3000Driver();
			//
			// If button is pressed then the user wants to delete the policy 
			// so they can connect to a different AP most likely.
			//
			if(S1_PRESSED)
			{
				//
				// Reset all the previous configuration
				//
				wlan_ioctl_set_connection_policy(0,0,0);	
				wlan_ioctl_del_profile(0);
				wlan_ioctl_del_profile(1);
				wlan_ioctl_del_profile(2);
				
				//
				// reset the CC3000
				// 
				wlan_stop();      
				initCC3000Driver();
			}
			else
			{  
				//
				// Configure to connect automatically to the AP retrieved in the 
				// simple config process
				//
				wlan_ioctl_set_connection_policy(0, 0, 1);
			}
			
			//
			// Wait 3 seconds to see if we connect
			//
			for (i = 0;
				 (i < 15) && (CC3000ConectionState == CC3000_NOT_CONNECTED_STATE);
				 i++) {
				Delay100ms();
				LedOn(LED_TRYING_CONNECT);
				Delay100ms();
				LedOff(LED_TRYING_CONNECT);
			}
			
			if (CC3000ConectionState == CC3000_NOT_CONNECTED_STATE)
			{
				//
				// Try Connect to the AP using smart config
				//
#ifdef SMART_CONFIG_EN
#if (defined(SECURITY_WEP) || defined(SECURITY_WPA) || defined(SECURITY_WPA2))
#error "Error: Smart Config cannot be built with wlan_connect method.  Please Choose only one method"
#endif
				StartSmartConfig();
#else      
				// or, direct connect
#if   defined(SECURITY_WEP)
				wlan_connect(1, "TP-LINK", 0x07,
							 NULL, "password", sizeof("password"));
#elif  defined(SECURITY_WPA)
				wlan_connect(2, "TP-LINK", 0x07,
							 NULL, "password", sizeof("password"));
#elif  defined(SECURITY_WPA2)
				wlan_connect(3, "TP-LINK", 0x07,
							 NULL, "password", sizeof("password"));
#else // no security
				wlan_connect(0, "TP-LINK", 0x07, NULL, NULL, 0);
#endif            
				
#endif      
				//
				// Wait 3 seconds to see if we connect this time
				//
				for (i = 0;
					 (i < 15)
					 && (CC3000ConectionState
						 == CC3000_NOT_CONNECTED_STATE); i++) {
						 Delay100ms();
						 LedOn(LED_TRYING_CONNECT);
						 Delay100ms();
						 LedOff(LED_TRYING_CONNECT);
					 }
				
			}
			
			break;
		case CC3000_NOT_CONNECTED_STATE:
			ucNumberOfSkippedAttempts++;
			if (ucNumberOfSkippedAttempts >= NUM_OF_SKIP_ATTEMPTS)
			{
				//
				// Turn off the LED1
				//
				LedOff(LED_CONNECTION);
				
				// No Connection to AP try again by  Resetint the processor
				SaveAndResetSystem();
			}
			break;
		case CC3000_CONNECTED_STATE:
			if ((ucPortsOpen == 0) && (g_ucHaveIpAddress == 1)) {
				//
				// Open UDP sockets
				//
				g_ulSocketUdpLightBulbs = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				tSocketAddr.sa_family = AF_INET;
				
				//
				// Light UDP Port
				//
				tSocketAddr.sa_data[0] = (unsigned char) (LIGHT_UDP_PORT >> 8);
				tSocketAddr.sa_data[1] = (unsigned char) LIGHT_UDP_PORT;
				
				memcpy(&tSocketAddr.sa_data[2], g_ThisMulticastAddress, 4);
				
				cRetVal = bind(g_ulSocketUdpLightBulbs, &tSocketAddr,
							   sizeof(sockaddr));
				if (cRetVal == -1) {
					// Socket unintentionally closed. Reset
					SaveAndResetSystem();
				}
				
				//
				// THERMOSTAT UDP PORTs
				//
				tSocketAddr.sa_data[0] = (unsigned char) (THERMOSTAT_UDP_PORT_RX
														  >> 8);
				tSocketAddr.sa_data[1] = (unsigned char) THERMOSTAT_UDP_PORT_RX;
				
				memcpy(&tSocketAddr.sa_data[2], g_ThisMulticastAddress, 4);
				
				g_ulSocketUdpThermostatsRcv = socket(AF_INET, SOCK_DGRAM,
													 IPPROTO_UDP);
				
				cRetVal = bind(g_ulSocketUdpThermostatsRcv, &tSocketAddr,
							   sizeof(sockaddr));
				if (cRetVal == -1) {
					// Socket unintentionally closed. Reset
					SaveAndResetSystem();
				}
				
				tSocketAddr.sa_data[0] = (unsigned char) (THERMOSTAT_UDP_PORT_TX
														  >> 8);
				tSocketAddr.sa_data[1] = (unsigned char) THERMOSTAT_UDP_PORT_TX;
				
				g_ulSocketUdpThermostatsSend = socket(AF_INET, SOCK_DGRAM,
													  IPPROTO_UDP);
				
				ucPortsOpen = 1;
				CC3000ConectionState = CC3000_CONNECTED_PORTS_OPEN_STATE;
			}
			break;
		case CC3000_CONNECTED_PORTS_OPEN_STATE:
			break;
		default:
			break;
	}
}



// TODO Write drivers

/**
 * changeBlindPos
 * \param New blind position (int)
 * \return None
 * \bried Changes blind position to given value
 */
void changeBlindPos(int pos)
{
	if (pos < settingsMAX.blindPos && pos > settingsMIN.blindPos)
	{
		//Change pos
	}
	else
	{
		//Do nothing
	}
	
}

/**
 * changeLightLev
 * \param New light level (int)
 * \return None
 * \brief Changes light level to given value
 */
void changeLightLev(int lev)
{
	if (lev < settingsMAX.lightLev && lev > settingsMIN.lightLev)
	{
		//Change level
	}
	else
	{
		//Do nothing
	}
	
}

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer
	P1DIR |= 0x01;                        // Set P1.0 to output direction
	
	//System init
	sys_init();
	
	//WLAN Connect
	//
	// First Time try to connect
	//
	CC3000ConectionState = CC3000_INIT_STATE;
	ServiceCC300StateMachine();
	
	
	for (;;)
	{
		/**
		 * Main program
		 */
		
		//Handle incoming wifi
		if(CC3000ConectionState == CC3000_CONNECTED_PORTS_OPEN_STATE)
		{
			if(ReadMessages())
			{
				//Get messages from WLAN from buffer
				//Light packet length=7
				if(stateBuffer[6] == 2 || stateBuffer[6] == '2')
				{
					//If buffer has contents, set state based on values based on range
					int roomID = stateBuffer[4];
					int state = stateBuffer[2];
					
					//clear buffer for next use
					for(int f = 0; f < 100; f++)
					{
						stateBuffer[f] = 0;
					}
					
					/**
					 * State		Settings
					 * dayNorm		blindPos = 1024, lightLev = 0
					 * nightNorm	blindPos = 0, lightLev = 1024
					 * film			blindPos = 0, lightLev = 0 / lightLev = 250
					 * open			blindPos = 1024, lightLev = 0
					 * closed		blindPos = 0, lightLev = 0
					 */
					
					//Handle message
					if(roomID == SYSTEMID)
					{
						if (SYSTEMSTATE != state)
						{
							SYSTEMSTATE = state;
							
							if (state == CLOSE0)
							{
								changeBlindPos(closed.blindPos);
								changeLightLev(closed.lightLev);
							}
							else if(state == OPEN0)
							{
								changeBlindPos(open.blindPos);
								changeLightLev(open.lightLev);
							}
							else if(state == FILM0)
							{
								changeBlindPos(film.blindPos);
								changeLightLev(film.lightLev);
							}
							else if(state == DAY0)
							{
								changeBlindPos(dayNorm.blindPos);
								changeLightLev(dayNorm.lightLev);
							}
							else if(state == NIGHT0)
							{
								changeBlindPos(nightNorm.blindPos);
								changeLightLev(nightNorm.lightLev);
							}
							else
							{
								//Leave system as is and continue
							}
						}
						else
						{
							//Do nothing
						}

					}
					else
					{
						//Forward on instruction
						
					}

					
				}

				
				//Blink LED for fun
				volatile unsigned int i;            // volatile to prevent optimization

				P1OUT ^= 0x01;                      // Toggle P1.0 using exclusive-OR

				i = 10000;                          // SW Delay
				do i--;
				while (i != 0);
			}
		}
		
		//Handle on board GUI if enabled
		#if ONBOARD_GUI_EN == 1
			#if CAP_UI_EN == 1
					//Handle Cap UI
					
			#else
					//Handle switch UI
					
			#endif
		#endif
  }
}
