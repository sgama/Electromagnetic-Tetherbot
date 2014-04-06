/*
	PROJECT 2 TRANSMITTER CODE
*/
#include <stdio.h>
#include <at89lp51rd2.h>

// ~C51~ 
#define CLEAR_SCREEN "\x1b[2J"
#define CLK 22118400L
#define BAUD 115200L
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))

//We want timer 0 to interrupt every 100 microseconds ((1/10000Hz)=100 us)
#define FREQ 10000L
#define TIMER0_RELOAD_VALUE (65536L-(CLK/(12L*FREQ)))


#define FOLLOW 4200
#define CLOSER 4400
#define FURTHER 4600
#define TURN 4800
#define PARK 5000

//These variables are used in the ISR

int getFreq();
int getRawFreq();
int parseCmd();
void tx_byte( char val );
void wait_bit_time();

volatile unsigned char transmit_toggle;

unsigned char _c51_external_startup(void)
{
	// Configure ports as a bidirectional with internal pull-ups.
	P0M0=0;	P0M1=0;
	P1M0=0;	P1M1=0;
	P2M0=1;	P2M1=1;
	P3M0=0;	P3M1=0;
	AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O
	P4M0=0;	P4M1=0;
    
    // Initialize the serial port and baud rate generator
    PCON|=0x80;
	SCON = 0x52;
    BDRCON=0;
    BRL=BRG_VAL;
    BDRCON=BRR|TBCK|RBCK|SPD;
	
	EA=1;  // Enable global interrupts
    
    return 0;
}


void wait10ms (void)
{
_asm
;For a 22.1184MHz crystal one machine cycle 
;takes 12/22.1184MHz=0.5425347us
mov R2, #1
L3: mov R1, #250
L2: mov R0, #184
L1: djnz R0, L1 ; 2 machine cycles-> 2*0.5425347us*184=200us
djnz R1, L2 ; 200us*250=0.05s
djnz R2, L3 ; 0.05s*20=1s
ret
_endasm;
}

void wait1s (void)
{
_asm
;For a 22.1184MHz crystal one machine cycle 
;takes 12/22.1184MHz=0.5425347us
mov R2, #20
L3_1: mov R1, #250
L2_1: mov R0, #184
L1_1: djnz R0, L1_1 ; 2 machine cycles-> 2*0.5425347us*184=200us
djnz R1, L2_1 ; 200us*250=0.05s
djnz R2, L3_1 ; 0.05s*20=1s
ret
_endasm;
}

void main (void)
{ 
	TR0=0; // Disable timer/counter 0
	TMOD=0B_00000101; // Set timer/counter 0 as 16-bit counter
	
	while(1)
	{	
		unsigned int command; 
		command = parseCmd();
		if (command==FOLLOW){
			printf( CLEAR_SCREEN );
			printf( "START/STOP FOLLOWING \r");
			tx_byte(1);
			wait1s();
		}
		else if (command==CLOSER){
			printf( CLEAR_SCREEN );
			printf( "MOVE CLOSER \r");
			tx_byte(2);
			wait1s();
		}
		else if (command==FURTHER){
			printf( CLEAR_SCREEN );
			printf( "MOVE AWAY \r");
			tx_byte(3);
			wait1s();
		}
		else if (command==TURN){
			printf( CLEAR_SCREEN );
			printf( "TURN AROUND \r");
			tx_byte(4);
			wait1s();
		}
		else if (command==PARK){
			printf( CLEAR_SCREEN );
			printf( "WHAT, YOU WANT ME TO PARK? \r");
			tx_byte(5);
			wait1s();
		}
		else {
			printf( CLEAR_SCREEN );
			printf( "AWAITING COMMAND, COMMANDER \r");
			P2_2=1;
		}
	}
}

int parseCmd(){
	unsigned int newfreq;
	unsigned int frequency = 0;
	newfreq = getFreq();
	while(newfreq>2000){
		frequency = newfreq;
		newfreq = getFreq();
	}
	if (frequency > FOLLOW-50 && frequency < FOLLOW+50) return FOLLOW;
	else if(frequency > CLOSER-50 && frequency < CLOSER+50) return CLOSER;
	else if(frequency > FURTHER-50 && frequency < FURTHER+50) return FURTHER;
	else if(frequency > TURN-50 && frequency < TURN+50) return TURN;
	else if(frequency > PARK-50 && frequency < PARK+50) return PARK;
	else return 0;
}

//WILL HANG ON FUCKUPS
int getFreq(){
	unsigned int frequency;
	frequency=getRawFreq();
	if (frequency>12000)return 0;
	return frequency;
}

int getRawFreq(){

	unsigned int frequency1;
	unsigned int frequency2;
	unsigned int frequency3;
		// Reset the counter
		TL0=0;
		TH0=0;
		// Start counting
		TR0=1;
		// Wait one second
		wait10ms();
		// Stop counter 0, TH0-TL0 has the frequency!
		TR0=0;
		frequency1=(TH0*256+TL0)*20;
		
		// Reset the counter
		TL0=0;
		TH0=0;
		// Start counting
		TR0=1;
		// Wait one second
		wait10ms();
		// Stop counter 0, TH0-TL0 has the frequency!
		TR0=0;
		frequency2=(TH0*256+TL0)*20;
		
		if (frequency2<(frequency1-50) || frequency2>(frequency1+50)) return 0;
		
		// Reset the counter
		TL0=0;
		TH0=0;
		// Start counting
		TR0=1;
		// Wait one second
		wait10ms();
		// Stop counter 0, TH0-TL0 has the frequency!
		TR0=0;
		frequency3=(TH0*256+TL0)*20;
		
		if (frequency2<(frequency1-50) || frequency2>(frequency1+50)) return 0;
		
		return frequency1;
}

void tx_byte( char val )
{
	char j;
	//Send the start bit
	P2_2=0;
	wait_bit_time();
	for (j=0; j<8; j++)
	{
		P2_2 = val&(0x01<<j)?1:0;
		wait_bit_time();
	}
	P2_2=1;
	//Send the stop bits
	wait_bit_time();
	wait_bit_time();
}

void wait_bit_time (void)
{
	_asm
		;For a 22.1184MHz crystal one machine cycle 
		;takes 12/22.1184MHz=0.5425347us
		mov R2, #10
	L6:	mov R1, #8
	L5:	mov R0, #184
	L4:	djnz R0, L4 ; 2 machine cycles-> 2*0.5425347us*184=200us
	    djnz R1, L5 ; 200us*4=800us
	    djnz R2, L6 ; 800*10 = 8 ms
	    ret
	_endasm;
}