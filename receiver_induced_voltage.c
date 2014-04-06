#include <stdio.h>
#include <at89lp51rd2.h>
// ~C51~
#define CLK 22118400L
#define BAUD 115200L
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))
#define MAX_TIMER 0xFFFF
#define CH_0 0
#define CH_1 1
#define OneOverRootTwo 0.70710678
unsigned char _c51_external_startup(void)
{
// Configure ports as a bidirectional with internal pull-ups.
P0M0=0; P0M1=0;
P1M0=0; P1M1=0;
P2M0=0; P2M1=0;
P3M0=0; P3M1=0;
AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O
P4M0=0; P4M1=0;
// The AT89LP51RB2 has a baud rate generator:
PCON|=0x80;
SCON = 0x52;
BDRCON=0;
BRL=BRG_VAL;
BDRCON=BRR|TBCK|RBCK|SPD;
return 0;
}


void initTimer0(void)
{
	TR0=0; // Disable timer/counter 0
	TMOD=0B_00000001; // Set timer/counter 0 as 16-bit timer
	//ET0=1; // enable timer 0 interrupt
	//P3_2=1; // Pin used as input
}

void SPIWrite( unsigned char value)
{
	SPSTA&=(~SPIF); // Clear the SPIF flag in SPSTA
	SPDAT=value;
	while((SPSTA & SPIF)!=SPIF); //Wait for transmission to end
}

unsigned int get_voltage(unsigned int channel)
{
	unsigned int adc;
	// initialize the SPI port to read the MCP3004 ADC attached to it.
	SPCON&=(~SPEN); // Disable SPI
	SPCON=MSTR|CPOL|CPHA|SPR1|SPR0|SSDIS;
	SPCON|=SPEN; // Enable SPI
	P1_4=0; // Activate the MCP3004 ADC.
	SPIWrite(channel|0x18); // Send start bit, single/diff* bit, D2, D1, and D0 bits.
	for(adc=0; adc<10; adc++); // Wait for S/H to setup
	SPIWrite(0x55); // Read bits 9 down to 4
	adc=((SPDAT&0x3f)*0x100);
	SPIWrite(0x55); // Read bits 3 down to 0
	P1_4=1; // Deactivate the MCP3004 ADC.
	adc+=(SPDAT&0xf0); // SPDR contains the low part of the result.
	adc>>=4;
	return adc;
}

void wait1s (void)
{
	_asm
			;For a 22.1184MHz crystal one machine cycle
			;takes 12/22.1184MHz=0.5425347us
			push AR0
			push AR1
			push AR2
			mov R2, #20
		L3: mov R1, #250
		L2: mov R0, #184
		L1: djnz R0, L1 ; 2 machine cycles-> 2*0.5425347us*184=200us
			djnz R1, L2 ; 200us*250=0.05s
			djnz R2, L3 ; 0.05s*20=1s
			pop AR2
			pop AR1
			pop AR0
			ret
	_endasm;
}

void main(void)
{
//	initTimer0();
	
	unsigned int ch_0_voltage;
	unsigned int ch_1_voltage;
	
	while(1)
	{
		TR0=0; // Stop timer 0
		TH0=0; TL0=0; // Reset the timer
		
		ch_0_voltage = get_voltage(CH_0);
		ch_1_voltage = get_voltage(CH_1);		
		
	    printf("v1=%u  v2=%u    \r", ch_0_voltage, ch_1_voltage);
	    //wait1s();
	}	
}