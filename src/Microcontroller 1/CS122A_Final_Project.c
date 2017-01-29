/* Partner(s) Name & E-mail: Yuxuan(Leo) Li & yli066@ucr.edu
 * Lab Section: 022
 * Assignment: Final Project
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 */

#define F_CPU 8000000UL // Assume uC operates at 8MHz
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/portpins.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "usart_ATmega1284.h"



unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}

unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}

volatile unsigned int DISTANCE = 0;

volatile unsigned int edge_flag = 0; // 0: rising edge
									 // 1: falling edge
unsigned long ticks = 0;
unsigned int send = 0;
void Send_signal(){
	if (send == 0){
		_delay_ms(10);
		PORTB &= ~(1 << PINB0); //clear trigger
		_delay_us(1);
		PORTB |= (1 << PINB0); //Send 15us second pulse
		_delay_us(15);
		PORTB &= ~(1 << PINB0); //clear trigger
		send = 1;
	}
}
void Capture_init(){
	EIMSK = (1 << INT2);
	EICRA = (1 << ISC20); 
}
void initial_timer(){
	TCCR3B |= (1 << CS00);
}
void stop_timer(){
	TCCR3B |= 0x00;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------
//AUTO MODE TASK
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------

unsigned char mode = 0; //0: no mode
						//1: manual mode
						//2: automatic mode
unsigned char control; //data received from control LCD

unsigned char TURNNOW = 0;

enum TIMERstate {initial,trigger_receive} timer_state;


void TIMER_TICK(){
	//transitions
	switch(timer_state){
		case initial:
			
			if (mode == 2){
				Capture_init();
				initial_timer();
				timer_state = trigger_receive;
			}
			else {
				timer_state = initial;
			}
			break;
		case trigger_receive: 
			if (mode == 2){
				timer_state = trigger_receive;
			}
			else {
				timer_state = initial;
			}
			break;
		default: 
			timer_state = initial;
			break;
	}
	
	//actions
	switch(timer_state){
		case initial:
			break;
		case trigger_receive:
			Send_signal();
			DISTANCE = ticks/58;
			PORTA = DISTANCE;
			if (DISTANCE < 25){
				TURNNOW = 1;
			}
			else {
				TURNNOW = 0;
			}
			break;
		default:
			break;
	}
}

unsigned int cnt = 0; 
unsigned int cnt2 = 0;
enum automaticdriving_state {wait,Driving,turn1,turn2} A_D_State;

void Going(){
	//transitions
	switch(A_D_State){
		case wait:	
			if (mode == 2){
				A_D_State = Driving;
			}
			else {
				A_D_State = wait;
			}
			break;
		case Driving: 
			if (TURNNOW){
				A_D_State = turn1;
			}
			else if (mode == 0){
				A_D_State = wait;
			}
			else {
				A_D_State = Driving;
			}
			break;
/*
		case Driving2:
			if (TURNNOW){
				A_D_State = turn1;
			}
			else if (mode == 0){
				A_D_State = wait;
			}
			else {
				A_D_State = Driving;
			}
			break;
*/
		case turn1: 
			if (mode == 0){
				A_D_State = wait;
			}
			else {
				if (cnt >= 3){
					cnt = 0;
					A_D_State = turn2;
				}
				else {
					A_D_State = turn1;
				}
			}
			break;
		case turn2:
			if (mode == 0){
				A_D_State = wait;
			}
			else {
				if (cnt2 >= 3){
					cnt2 = 0;
					A_D_State = Driving;
				}
				else {
					A_D_State = turn2;
				}
			}
			break;
		default: 
			A_D_State = wait;
			break;
	}
	
	//actions
	switch(A_D_State){
		case wait: 
			break;
		case Driving: 
			PORTC = 0x55;
			break;
		case turn1: 
			cnt++;
			PORTC = 0xAA;
			break;
		case turn2: 
			cnt2++;
			PORTC = 0x66;
			break;
		default: 
			break;
	}

}
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------
//CONTROL TASK
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------



enum CONTROLstate {Init,C_BUFFER} control_state;

void CONTROL_TICK(){
	//transitions
	switch(control_state){
		case Init: 
			control_state = C_BUFFER;
			break;
		case C_BUFFER: 
			//PORTA = mode;
			if (USART_HasReceived(1)){
				control = USART_Receive(1);
				USART_Flush(1);
				if (control == 0x01){
					mode = 1;
				}
				else if (control == 0x11){
					mode = 2;
				}
				else if (control == 0xFF){
					mode = 0;
					PORTC = 0x00;
				}
				control_state = C_BUFFER;
			}
			else {
				control_state = C_BUFFER;
			}
			break;
		default:
			control_state = Init;
			break;
	}
	
	//actions
	switch(control_state){
		case Init:
			break;
		case C_BUFFER:
			break;
		default:
			break;
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------
//MANUAL DRIVING TASKS
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------

enum RCState {BUFFER,F,B,R,L} RC_state; //remote control state machine
unsigned char data; 
unsigned char dir = 0;
unsigned int not_r_cnt = 0;

void RC_Tick(){	
	//Transitions
	switch(RC_state){
		case BUFFER: 
		
		if (mode == 1){
			if (USART_HasReceived(0)){
					
					data = USART_Receive(0);
					PORTA = data;
					USART_Flush(0);
					if (data == 0xF7){
						//dir = 1;
						RC_state = F;
					}
					else if (data == 0xF3){
						//dir = 2;
						RC_state = B;
					}
					else if (data == 0xC4){
						//dir = 3;
						RC_state = R;
					}
					else if (data == 0xC1){
						//dir = 4;
						RC_state = L;
					}
				}
				else {
					USART_Flush(0);
					not_r_cnt++;
					PORTA = 0x00;
					PORTC = 0x00;
				}
			}
			else {
				//dir = 0;
				RC_state = BUFFER;
			}
			break;
		case F: 
			if (USART_HasReceived(0)){
				data = USART_Receive(0);
				if (data == 0xF7){
					RC_state = F;
				}
			}
			else {
				
				RC_state = BUFFER; 
			}
			break;
		case B: 
			if (USART_HasReceived(0)){
				data = USART_Receive(0);
				if (data == 0xF3){
					RC_state = B;
				}
			}
			else {
				
				RC_state = BUFFER;
			}
			break;
		case R:
			if (USART_HasReceived(0)){
				data = USART_Receive(0);
				if (data == 0xC4){
					RC_state = R;
				}
			}
			else {
				
				RC_state = BUFFER;
			}
			break;
		case L: 
			if (USART_HasReceived(0)){
				data = USART_Receive(0);
				if (data == 0xC1){
					RC_state = L;
				}
			}
			else {
				
				RC_state = BUFFER;
			}
			break;
		default: 
			RC_state = BUFFER;
			break;
	}	
	
	//Actions
	switch(RC_state){
		case BUFFER: 
			break;
		case F:
			PORTC = 0x55;
			break;
		case B:
			PORTC = 0xAA;
			break;
		case R:
			PORTC = 0x66;
			break;
		case L:
			PORTC = 0x99;
			break;
		default:
			break;
	}
}

/*
enum DirectionState {STOP,FORWARD,BACKWARD,RIGHT,LEFT} direction_state;
void DIRECTION_TICK(){
	//transitions
	switch(direction_state){
		case STOP:
			if (dir == 1){
				direction_state = FORWARD;
			}
			else if (dir == 2){
				direction_state = BACKWARD;
			}
			else if (dir == 3){
				direction_state = RIGHT;
			}
			else if (dir == 4){
				direction_state = LEFT;
			}
			else {
				direction_state = STOP;
			}
			break;
		case FORWARD: 
			if (dir != 1){
				direction_state = STOP;
			}
			else {
				direction_state = FORWARD;
			}
			break;
		case BACKWARD:
			if (dir != 2){
				direction_state = STOP;
			}
			else {
				direction_state = BACKWARD;
			}
			break;
		case RIGHT:
			if (dir != 3){
				direction_state = STOP;
			}
			else {
				direction_state = RIGHT;
			}
			break;
		case LEFT: 
			if (dir != 4){
				direction_state = STOP;
			}
			else {
				direction_state = LEFT;
			}
			break;
		default: 
			break;
	}
	//actions
	switch(direction_state){
		case STOP:
			PORTC = 0x00;
			break;
		case FORWARD:
			PORTC = 0x55;
			break;
		case BACKWARD:
			PORTC = 0xAA;
			break;
		case RIGHT:
			PORTC = 0x66;
			break;
		case LEFT:
			PORTC = 0x99;
			break;
		default:
			break;
	}
	
}


enum testing_state {test} testings;
	
void test1(){
	switch(testings){
		case test: 
			PORTC = 0x55;
			break;
		default: 
			testings = test; 
	}
}

*/

//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------
void RCTick()
{
   RC_state = BUFFER;
   while(1)
   { 	
	RC_Tick();
	vTaskDelay(10); 
   } 
}

void CONTROLTick()
{
	control_state = Init;
	while(1)
	{
		CONTROL_TICK();
		vTaskDelay(50);
	}
}

/*
void DIRECTIONTick()
{
	direction_state = STOP;
	while(1)
	{
		DIRECTION_TICK();
		vTaskDelay(15);
	}
}
*/

void TIMERTick()
{
	timer_state = initial;
	while(1)
	{
		TIMER_TICK();
		vTaskDelay(25);
	}
}
/*
void testTick()
{
	testings = test;
	while(1)
	{
		test1();
		vTaskDelay(100);
	}
}
*/
void AUTOTick()
{
	A_D_State = wait;
	while(1)
	{
		Going();
		vTaskDelay(10);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


void Start_M_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(RCTick, (signed portCHAR *)"RCTick", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void Start_C_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(CONTROLTick, (signed portCHAR *)"CONTROLTick", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
/*
void Start_D_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(DIRECTIONTick, (signed portCHAR *)"DIRECTIONTick", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
*/

void Start_TIMER_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(TIMERTick, (signed portCHAR *)"TIMERTick", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
/*
void Start_test_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(testTick, (signed portCHAR *)"testTick", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
*/
void Start_AUTO_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(AUTOTick, (signed portCHAR *)"AUTOTick", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------

int main(void) 
{ 
	//inputs 
	DDRB = 0x01; PORTB = 0x00;
	
	DDRD = 0x00; PORTD = 0xFF;
	
	//outputs 
	DDRA = 0xFF; PORTA=0x00; //trigger pin
	DDRC = 0xFF; PORTC=0x00; //Motor out

   //Start Usart
   initUSART(0);
   initUSART(1);
   
   //start external interrupts 

   //Start Tasks
   Start_M_Pulse(1); //motor pulse
   Start_AUTO_Pulse(2); //auto control pulse
   Start_C_Pulse(4); //control pulse
   //Start_D_Pulse(1); //direction pulse
   Start_TIMER_Pulse(1); //auto pulse
   //Start_test_Pulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
   return 0; 
	
}

ISR (INT2_vect){
	if(edge_flag == 1){ //check if echo turned low
		
		ticks = TCNT3;
		edge_flag = 0;
		send = 0;
	}
	
	else if (edge_flag == 0){ //Check if echo turned high
		edge_flag = 1;
		TCNT3 = 0;
	}
}