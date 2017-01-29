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


enum LCDState {Init,Menu,cap_button1,Menu2,cap_button2,select1_cap,select2_cap} LCD_state; //remote control state machine
unsigned char data; 

unsigned char customPointer[8] = {
	0b00000,
	0b01000,
	0b01100,
	0b01110,
	0b01100,
	0b01000,
	0b00000,
	0b00000
};

unsigned char customClear[8] = {
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000
};


unsigned int mode = 0;          // 0: none
                            // 1: manual mode
							// 2: automatic mode
							
unsigned int main_menu = 0; // 0: display main menu
                            // 1: do not display main menu, in another mode

void Main_LCD_Tick(){	
	unsigned char ButtonChange = ~PINC & 0x01;
	unsigned char ButtonSelect = ~PINC & 0x02;
	

	//transitions
	switch(LCD_state){
		case Init: 
			//PORTC = 0xFF;
			if (main_menu == 0){
				LCD_state = Menu;
			}
			else {
				LCD_state = Init;
			}
			break;
		case Menu: 
			if (ButtonChange){
				//LCD_ClearScreen();
				LCD_state = cap_button1;
			}
			else if (ButtonSelect){
				USART_Send(0x01,0);
				main_menu = 1;
				LCD_state = select1_cap;
			}
			else if (main_menu == 0){
				LCD_state = Menu; 
			}
			break;
		case cap_button1:
			if (!ButtonChange){
				LCD_ClearScreen();
				LCD_state = Menu2;
			}
			else {
				LCD_state = cap_button1;
			}
			break;
		case select1_cap: 
			if (!ButtonSelect){
				mode = 1;
				LCD_state = Init;
			}
			else {
				LCD_state = select1_cap;
			}
			break;
		case Menu2: 
			if (ButtonChange){
				//LCD_ClearScreen();
				LCD_state = cap_button2;
			}
			else if (ButtonSelect){
				USART_Send(0x11,0);
				main_menu = 1;
				LCD_state = select2_cap;
			}
			else if (main_menu == 0){
				LCD_state = Menu2; 
			}
			break;
		case cap_button2:
			if (!ButtonChange){
				LCD_ClearScreen();
				LCD_state = Menu;
			}
			else {
				LCD_state = cap_button2;
			}
			break;
		case select2_cap:
			if (!ButtonSelect){
				mode = 2;
				LCD_state = Init;
			}
			else {
				LCD_state = select2_cap;
			}
			break;
			break;
		default: 
			LCD_state = Init;
			break;
	}
	//actions
	switch(LCD_state){
		case Init:
			break;
		case Menu: 
			//LCD_ClearScreen();
			LCD_build(0,customPointer);
			LCD_Cursor(1);
			LCD_WriteData(0);
			LCD_DisplayString(3,"MANUAL DRIVE");
			LCD_DisplayString(19,"AUTO   DRIVE");
			break;
		case cap_button1:
			break;
		case Menu2:
			//LCD_ClearScreen();
			LCD_build(0,customPointer);
			LCD_Cursor(17);
			LCD_WriteData(0);
			LCD_DisplayString(3,"MANUAL DRIVE");
			LCD_DisplayString(19,"AUTO   DRIVE");
			break;
		case cap_button2:
			break;
		case select1_cap:
			LCD_ClearScreen();
			LCD_DisplayString(2,"LOADING.....");
			LCD_DisplayString(19,"PLEASE WAIT");
			break;
		case select2_cap:
			LCD_ClearScreen();
			LCD_DisplayString(2,"LOADING.....");
			LCD_DisplayString(19,"PLEASE WAIT");
			break;
		default: 
			break;
	}

}

enum AUTO_states {init,display,cap_reset}auto_state;
void LCD_AUTO(){
	unsigned char Buttonreset = ~PINC & 0x80;
	//transitions
	switch(auto_state){
		case init:
			if (mode == 2){
				LCD_ClearScreen();
				auto_state = display;
			}
			else {
				auto_state = init;
			}
			break;
		case display: 
			if (mode == 2){
				if(Buttonreset){
					auto_state = cap_reset;
				}
				else {
					auto_state = display;
				}
			}
			else {
				auto_state = init;
			}
			break;
		case cap_reset:
			if (!Buttonreset){
				mode = 0;
				LCD_ClearScreen();
				main_menu = 0;
				USART_Send(0xFF,0);
				auto_state = init;
				
			}
			else {
				auto_state = cap_reset;
			}
			break;
		default: 
			auto_state = init; 
			break;
	}
	//actions
	switch(auto_state){
		case init: 
			break;
		case display: 
			LCD_DisplayString(2, "AUTOMATIC MODE");
			break;
		case cap_reset:
			LCD_ClearScreen();
			LCD_DisplayString(2,"RESETING....");
			LCD_DisplayString(19,"PLEASE WAIT");
		default: 
			break;
	}
}

enum MAN_states {init2,display2,cap_reset2}man_state;
void Man_Tick(){
	unsigned char Buttonreset = ~PINC & 0x80;
	//transitions
	switch(man_state){
		case init2: 
			if (mode == 1){
				LCD_ClearScreen();
				man_state = display2;
			}
			else {
				man_state = init2;
			}
			break;
		case display2: 
			if (mode == 1){
				if (Buttonreset){
					man_state = cap_reset2; 
				}
				else {
					man_state = display2;
				}
				
			}
			else {
				man_state = init2;
			}
			break;
		case cap_reset2: 
			if (!Buttonreset){
				mode = 0;
				LCD_ClearScreen();
				USART_Send(0xFF,0);
				main_menu = 0;
				man_state = init2;
			}
			else {
				man_state = cap_reset2;
			}
			
			break;
		default: 
			man_state = init2;
			break;
	}
	//actions
	switch(man_state){
		case init2: 
			break;
		case display2: 
			LCD_DisplayString(2, "MANUAL MODE");
			LCD_DisplayString(16, "  HAVE FUN!!! ");
			
			break;
		case cap_reset2: 
			LCD_ClearScreen();
			LCD_DisplayString(2,"RESETING....");
			LCD_DisplayString(19,"PLEASE WAIT");
			break;
		default: 
			break;
	}
}

void MainLCDTick()
{
   LCD_state = Init;
   while(1)
   { 	
	Main_LCD_Tick();
	vTaskDelay(50); 
   } 
}

void AutoTick()
{
	auto_state = init;
	while(1)
	{
		LCD_AUTO();
		vTaskDelay(100);
	}
}

void ManTick()
{
	man_state = init2;
	while(1)
	{
		Man_Tick();
		vTaskDelay(100);
	}
}

void Start_LCD_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(MainLCDTick, (signed portCHAR *)"MainLCDTick", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void Start_AUTO_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(AutoTick, (signed portCHAR *)"AutoTick", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void Start_MAN_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(ManTick, (signed portCHAR *)"ManTick", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

int main(void) 
{ 
	//inputs 
	DDRC = 0x00; PORTC = 0xFF;
	DDRD = 0xFF; PORTD = 0x00;
	
	
	//outputs 
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xFF; PORTB = 0x00; 
	
   
   	LCD_init();
	LCD_ClearScreen();
	initUSART(0);
	
	//initUSART(1);
   
   //Start Tasks
   Start_LCD_Pulse(1);
   Start_AUTO_Pulse(1);
   Start_MAN_Pulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}