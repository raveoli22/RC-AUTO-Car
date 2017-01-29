# RC-AUTO-Car

A remote controlled and self driving car

####**HARDWARE REQUIREMENTS**: 
* 2x ATMEGA1284 Microcontrollers
* 1x Ultrasonic Transmitter
* 1x Bluetooth Module
* 1x LCD Screen
* 4x DC Motors
* 2x Motor Drivers
* 1x Android Cell Phone

####**SOFTWARE REQUIREMENTS**:
* Atmel Studio
* Bluetooth Serial Transmitter App (I used ArduinoRC) 

1. Using the schematic provided under [/documents/**Schematic.jpg**](./documents/Schematic.jpg), build the remote controlled car.
2. Download a bluetooth serial transmitter application on your android phone, and configure the keys to values you wish to send.
3. In the source code for microcontroller 1, modify the values to ones you expect to receive from your cell phone.
4. Compile the source code for microcontroller 1 and microcontroller 2, and then program them accordingly.
5. Select the mode you want the car to be in right now, and drive away! If you wish to change modes, reset the car by holding down both buttons and then re-select the desired mode you want. 

Further details and instructions can be found under [/documents/**Report.docx**](./documents/Report.docx). 


