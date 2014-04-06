#include <stdio.h>
#include <at89lp51rd2.h>

// ~C51~ 
#define CLEAR_SCREEN "\x1b[2J"
#define CLK 22118400L
#define BAUD 115200L
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))
#define CH_0 0
#define CH_1 1 

//We want timer 0 to interrupt every 100 microseconds ((1/10000Hz)=100 us)
#define FREQ 10000L
#define TIMER0_RELOAD_VALUE (65536L-(CLK/(12L*FREQ)))

//These variables are used in the ISR
#define ADC_MIN 2

volatile unsigned char pwmcount;
volatile unsigned char direction;
volatile int pwm;
void turn_left();
void turn_right();
void forward();
void reverse();
void stop();
unsigned int get_voltage(unsigned int channel);
void SPIWrite( unsigned char value);
void wait_one_and_half_bit_time(void);
void wait_bit_time(void);
char rx_byte (void);
void follow(unsigned int d1, unsigned int d2);
void follow_move(float v1);
float adjust_d2(unsigned int d2);
char compare(float a, float b);
void execute_command(char command);
void togglefollow(void);
void wait2ms (void);
char reached_required_distance(float v1);
void wait1s (void);

volatile char distance;
volatile char follow_flag;


unsigned char _c51_external_startup(void)
{
	// Configure ports as a bidirectional with internal pull-ups.
	P0M0=0;	P0M1=0;
	P1M0=0;	P1M1=0;
	P2M0=0;	P2M1=0;
	P3M0=0;	P3M1=0;
	AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O
	P4M0=0;	P4M1=0;
    
    // Initialize the serial port and baud rate generator
    PCON|=0x80;
	SCON = 0x52;
    BDRCON=0;
    BRL=BRG_VAL;
    BDRCON=BRR|TBCK|RBCK|SPD;
	
	// Initialize timer 0 for ISR 'pwmcounter()' below
	TR0=0; // Stop timer 0
	TMOD=0x01; // 16-bit timer
	// Use the autoreload feature available in the AT89LP51RB2
	// WARNING: There was an error in at89lp51rd2.h that prevents the
	// autoreload feature to work.  Please download a newer at89lp51rd2.h
	// file and copy it to the crosside\call51\include folder.
	TH0=RH0=TIMER0_RELOAD_VALUE/0x100;
	TL0=RL0=TIMER0_RELOAD_VALUE%0x100;
	TR0=1; // Start timer 0 (bit 4 in TCON)
	ET0=0; // Enable timer 0 interrupt
	EA=1;  // Enable global interrupts
	
	pwmcount = 0;
    
    return 0;
}

void stop(){
		P3_6 = 0;
		P3_7 = 0;
		P2_1 = 0;
		P2_0 = 0;
}

void turn_right (){
		P3_6 = 1;
		P3_7 = 0;
		P2_1 = 1;
		P2_0 = 0;	
}

void turn_left (){
		P3_6 = 0;
		P3_7 = 1;
		P2_1 = 0;
		P2_0 = 1;
}

void forward (){
		P3_6 = 0;
		P3_7 = 1; //right motor (m1) 
		P2_0 = 0;
		P2_1 = 1; //left motor	(m2)
}

void reverse (){
		P3_7 = 0;
		P3_6 = 1;
		P2_1 = 0;
		P2_0 = 1;	
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

void SPIWrite( unsigned char value)
{
	SPSTA&=(~SPIF); // Clear the SPIF flag in SPSTA
	SPDAT=value;
	while((SPSTA & SPIF)!=SPIF); //Wait for transmission to end
}

char rx_byte (void )
{
	char j, val;
	int v;
	//Skip the start bit
	val=0;
	wait_one_and_half_bit_time();
	for(j=0; j<8; j++)
	{
		v=get_voltage(0);
		val|=(v>ADC_MIN)?(0x01<<j):0x00;
		wait_bit_time();
	}
	//Wait for stop bits
	wait_one_and_half_bit_time();
	return val;
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
	    djnz R2, L6 ; 800*10 = 16 ms
	    ret
	_endasm;
}

void wait2ms (void)
{
	_asm
		;For a 22.1184MHz crystal one machine cycle 
		;takes 12/22.1184MHz=0.5425347us
		mov R2, #10
	L3:	mov R1, #1
	L2:	mov R0, #184
	L1:	djnz R0, L1 ; 2 machine cycles-> 2*0.5425347us*184=200us
	    djnz R1, L2 ; 200us*1=200us
	    djnz R2, L3 ; 200*10 = 4 ms
	    ret
	_endasm;
}
void wait1s (void)
{
	_asm
		push AR2
		push AR1
		push AR0
			
		;For a 22.1184MHz crystal one machine cycle 
		;takes 12/22.1184MHz=0.5425347us
	    mov R2, #1
	L12:	mov R1, #240
	L11:	mov R0, #110
	L10:	djnz R0, L10 ; 2 machine cycles-> 2*0.5425347us*92=100us
	    djnz R1, L11 ; 100us*250=0.025s
	    djnz R2, L12 ; 0.025s*1=25ms
	    
	    pop AR0
	    pop AR1
	    pop AR2
	    
	    ret
    _endasm;
}
void wait_one_and_half_bit_time(void)
{
	_asm
		;For a 22.1184MHz crystal one machine cycle 
		;takes 12/22.1184MHz=0.5425347us
		mov R2, #10
	L9:	mov R1, #12
	L8:	mov R0, #184
	L7:	djnz R0, L7 ; 2 machine cycles-> 2*0.5425347us*184=200us
	    djnz R1, L8 ; 200us*12=2400us
	    djnz R2, L9 ; 2400*5 = 24 ms
	    ret
	_endasm;
}

void main (void) 
{ 
	unsigned int d1;
	unsigned int d2;
	char command=0;
	char step=1;
	follow_flag=1;
	distance=3; /// intial distance
	while(1)
	{
		if(get_voltage(0)<ADC_MIN)
		{
			command=rx_byte();
			if(command>0 && command<6){
			stop();
			execute_command(command);
			}
		}
		if(follow_flag)
		{
			d1=get_voltage(CH_0);
			d2=get_voltage(CH_1);
			
			follow(d1,d2);
			
		}
	}
}

float adjust_d2(unsigned int d2)
{
	return d2*1.0;
}

void follow(unsigned int d1, unsigned int d2)
{
	float v1,v2;
	char v1_state, v2_state;
	v1=(float)d1;
	v2=adjust_d2(d2);
	v1_state=reached_required_distance(v1);
	v2_state=reached_required_distance(v2);
	
	if (v1_state==-1){
		//move m1 forward
		P3_6 = 0;
		P3_7 = 1;
	}
	else if (v1_state==1){
		//move m1 backward
		P3_6 = 1;
		P3_7 = 0;
	}
	else {
		P3_6 = 0;
		P3_7 = 0;
		
	}
	
	if (v2_state==-1){
		//move m2 forward
		P2_0 = 0;
		P2_1 = 1;
	}
	else if (v2_state==1){
		//move m2 backward
		P2_0 = 1;
		P2_1 = 0;
	}
	else {
		//stop m2
		P2_0 = 0;
		P2_1 = 0;
	}


}

/**
*Returns 0 if a==b
*Returns 1 if a>b
*Returns -1 if a<b
**/
char compare(float a, float b)
{
	float tolerance_a, tolerance_b;
	if(a==b)
	{
		return 0;
	}
	else if(a>b)
	{
		tolerance_a=0.06*a;
		tolerance_b=0.06*b;
		if((a-tolerance_a)<(b+tolerance_b))
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else if(a<b)
	{
		tolerance_a=0.06*a;
		tolerance_b=0.06*b;
		if((a+tolerance_a)>(b-tolerance_b))
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

void follow_move(float v1)
{
	char tooFarVStooClose;
	tooFarVStooClose=reached_required_distance(v1);
	if(tooFarVStooClose != 0)
	{
		if(tooFarVStooClose == 1)
		{
			reverse();
			wait1s();
			stop();
		}
		if(tooFarVStooClose == -1)
		{
			forward();
			wait1s();
			stop();
		}
	}
}

/**
*Returns 0 if reached destination
*Returns -1 if it is too far away
*Returns 1 if it is too close
**/
char reached_required_distance(float v1)
{
	if(distance == 0)
	{
		if((v1>=100)&&(v1<=125))
		{
			return 0;
		}
		if(v1<100)
		{
			return -1;
		}
		if(v1>125)
		{
			return 1;
		}
		return 0;
	}
	else if(distance == 1)
	{
		if((v1>=35)&&(v1<=45))
		{
			return 0;
		}
		if(v1<35)
		{
			return -1;
		}
		if(v1>45)
		{
			return 1;
		}
		return 0;
	}
	else if(distance == 2)
	{
		if((v1>=10)&&(v1<=12))
		{
			return 0;
		}
		if(v1<10)
		{
			return -1;
		}
		if(v1>12)
		{
			return 1;
		}
		return 0;
	}
	else if(distance == 3)
	{
		if((v1>=2)&&(v1<=3))
		{
			return 0;
		}
		if(v1<2)
		{
			return -1;
		}
		if(v1>3)
		{
			return 1;
		}
		return 0;
	}
	return 0;
}

void execute_command(char command){
	if (command==1){
		togglefollow();
	}
	else if (command==2){
		if(distance>0){
		distance--;
		}
	}
	else if (command==3){
		if(distance<3){
		distance++;
		}
	}
	else if (command==4){
		turn_around();
	}
	else if (command==5){
		if( follow_flag == 1 )
		{
				togglefollow();
		}
		turn_left();
		wait_timed45();
		reverse();
		wait_halfsecond();
		turn_right();
		wait_timed45();
		stop();
	}
}

void turn_around(){
	follow_flag=0;
	turn_left();
	wait_timed180();
	stop();
}

void togglefollow(){
	follow_flag=!follow_flag;
}

//Half second
void wait_halfsecond (void)
{
_asm
;For a 22.1184MHz crystal one machine cycle 
;takes 12/22.1184MHz=0.5425347us
mov R2, #11
L3_3: mov R1, #250
L2_3: mov R0, #184
L1_3: djnz R0, L1_3 ; 2 machine cycles-> 2*0.5425347us*184=200us
djnz R1, L2_3 ; 200us*250=0.05s
djnz R2, L3_3 ; 0.05s*20=1s
ret
_endasm;
}

//(~1.2 / 4) seconds
void wait_timed45 (void)
{
_asm
;For a 22.1184MHz crystal one machine cycle 
;takes 12/22.1184MHz=0.5425347us
mov R2, #6
L3_2: mov R1, #250
L2_2: mov R0, #184
L1_2: djnz R0, L1_2 ; 2 machine cycles-> 2*0.5425347us*184=200us
djnz R1, L2_2 ; 200us*250=0.05s
djnz R2, L3_2 ; 0.05s*20=1s
ret
_endasm;
}

//1.2 seconds
void wait_timed180 (void)
{
_asm
;For a 22.1184MHz crystal one machine cycle 
;takes 12/22.1184MHz=0.5425347us
mov R2, #22
L3_1: mov R1, #250
L2_1: mov R0, #184
L1_1: djnz R0, L1_1 ; 2 machine cycles-> 2*0.5425347us*184=200us
djnz R1, L2_1 ; 200us*250=0.05s
djnz R2, L3_1 ; 0.05s*20=1s
ret
_endasm;
}