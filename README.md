# RC-AUTO-Car

###**A remote controlled and self driving car**

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
* Bluetooth Serial Transmitter App (I used "ArduinoRC") 

####**INSTRUCTIONS**:
1. Use the schematic provided under [/documents/**Schematic.jpg**](./documents/Schematic.jpg) to build the remote controlled/auto driving car.
2. Download a bluetooth serial transmitter application on your android phone, and configure the keys to values you wish to send.(I reconmmend using "ArduinoRC" from the Android Play Store)
3. In the source code for microcontroller 1, modify the expected values to the same values you have configured on your Android app.
4. Compile the source code for microcontroller 1 and microcontroller 2, and then program them accordingly.
5. Select the mode you want the car to be in right now, and drive away! If you wish to change modes, reset the car by holding down both buttons and then re-select the desired mode you want. 

Further details and instructions can be found under [/documents/**Report.docx**](./documents/Report.docx). 


