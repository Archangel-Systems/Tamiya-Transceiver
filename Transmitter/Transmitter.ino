/*
 Arduino RF Transmitter.
 */

#include <SPI.h>
#include "RF24.h"

/****************** Pin Locations ***************************/
#define PC_Link      2  // Rx/TX Board Link Indicator LED.
#define RF_Link      A5 // RF Communication Status LED.
#define GND          A7 // Common Ground.

/****************** FunDuino Shield ***************************/
#define Stick_Analog_X A0 //Funduino Board Joystick [X].
#define Stick_Analog_Y A1 //Funduino Board Joystick [Y].
const int X_THRESHOLD_LOW = 342; //Funduino Board Joystick [X] Sensetivity.
const int X_THRESHOLD_HIGH = 345; //Funduino Board Joystick [X] Sensetivity.    
const int Y_THRESHOLD_LOW = 342; //Funduino Board Joystick [Y] Sensetivity.
const int Y_THRESHOLD_HIGH = 345; //Funduino Board Joystick [Y] Sensetivity.

/****************** RF Communication ***************************/
byte addresses[][6] = {"1Node","2Node"}; //Create "Nodes" [ Devices ]
bool radioNumber = 1; // Set this radio as radio number 0 for Rx or 1 for Tx.
//RF24 radio(7,8); //Set up nRF24L01 radio on SPI bus. pins 7&8 for regular use OR pins 9&10 for FUNDUINO Shield.
RF24 radio(9,10); //Set up nRF24L01 radio on SPI bus. pins 7&8 for regular use OR pins 9&10 for FUNDUINO Shield.
struct dataStruct{bool Direction; int Speed; int Angle;}myData; //Create a data structure for transmitting and receiving Data.
struct dataStruct1{bool Direction; int Speed; int Angle;}myData2; //Create a data structure for transmitting and receiving Data.
int devicetype;
bool RF_Connection = 0;
unsigned long StartTime; //Initializes Counter.
unsigned long TimeOutTime= 1000000; //1000ms TimeOut.

/****************** PC Communication ***************************/
String serialcmd; //Receiving PC DATA.
String serialrsnd; //Sended DATA TO PC.
boolean FlowData = false; //Control "Data Flow" [Continuously Send Sensor Data To PC].
bool Serial_Connection = false; //Detect Serial Port Connection Status.
int Motor_Speed = 0;
int Servo_Angle = 0;

/**************************************************************/
void setup()
{
  Serial.begin(115200); // initialize the serial communication:
  pinMode(GND,INPUT); // Define Common Ground.
  pinMode(PC_Link,OUTPUT); // SerialPort Activity LED.
  pinMode(RF_Link,OUTPUT); // RF Communication LED.
  pinMode(Stick_Analog_X,INPUT); //Analog stick x.
  pinMode(Stick_Analog_Y,INPUT); //analog stick y.
  radio.begin(); //Initiate RF24 Device.
  radio.setPALevel(RF24_PA_HIGH); // Set Power Amplifier (PA) level to one of four levels [RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm]
  /****************** Open a writing and reading pipe on each radio, with opposite addresses ***************************/
  if(radioNumber){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  }else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }
  radio.startListening();  // Start the radio listening for data
}

void loop() {
  Read_Stick();
  SearchRF();
  RF_SendData();
  RF_GetData(); 
  Serial_Tx();
}

/****************** Serial Port Communication (PC) [No Call Required] ***************************/ 
void serialEvent(){
  while (Serial.available() > 0) {
    // read data from Serial Port To A String
    serialcmd = Serial.readStringUntil('\n');
    if (serialcmd == "Connect"){Serial.println("Connected"); Serial_Connection = true;}
    else if (serialcmd == "Stop"){Serial.println("Stopped");FlowData = false;}
    else if (serialcmd == "Check"){if(RF_Connection == 0){Serial.println("HW Fail");}else{Serial.println("HW OK");}}
    else if (serialcmd == "Clean"){Serial.println("Kwh Cleaned");}
    else if (serialcmd == "Disconnect"){Serial.println("Disconnected");Serial_Connection = false;}
    else if (serialcmd == "Start"){Serial.println("Starting Data Flow...");FlowData = true;}
    else { Analyze_SerialData(); }
    Serial_Tx();
  }
}

/****************** Analyze Incoming Serial Data. ***************************/ 
void  Analyze_SerialData(){
  if (serialcmd.charAt(0) == 'F')
  {
    Motor_Speed = (serialcmd.substring(1)).toInt();
    myData.Speed = Motor_Speed;
    myData.Direction = 1;
  }
  else if (serialcmd.charAt(0) == 'R')
  {
    Motor_Speed = (serialcmd.substring(1)).toInt();
    Motor_Speed = -Motor_Speed;
    myData.Speed = Motor_Speed;
    myData.Direction = 0;
  //  Serial.print("Data is: ");Serial.println(Motor_Speed);
  }
  if (serialcmd.charAt(0) == 'S')
  {
     myData.Angle = (serialcmd.substring(1)).toInt();
  }
}

/****************** Read Stick Values from Shield ***************************/
void  Read_Stick(){
 Motor_Speed = analogRead(Stick_Analog_Y);
 Servo_Angle = analogRead(Stick_Analog_X);
 if (Motor_Speed <= Y_THRESHOLD_LOW)
 {
  myData.Direction = 0;
  myData.Speed = map(Motor_Speed, 0, X_THRESHOLD_LOW, 0, 1023);
 }
 else if (Motor_Speed >= Y_THRESHOLD_HIGH)
 {
  myData.Direction = 1;
  myData.Speed = map(Motor_Speed, X_THRESHOLD_HIGH, 683, 0, 1023);
 }
 else{   myData.Direction = 1; myData.Speed=0; }
 if (Servo_Angle <= X_THRESHOLD_LOW)
 {
  myData.Angle = map(Servo_Angle, 0, X_THRESHOLD_LOW, 0, 90);
 }
 else if (Servo_Angle >= X_THRESHOLD_HIGH)
 {
  myData.Angle = map(Servo_Angle, X_THRESHOLD_HIGH, 683, 90, 180);
 }
 else { myData.Angle = 90; }
}

/****************** Detect Serial Port Connection ***************************/ 
void Serial_Tx(){
if (Serial_Connection == true) {
 digitalWrite( PC_Link,HIGH);
   Serial.print("Stick X: ");      Serial.println(myData.Angle);
   Serial.print("Stick Y: ");      Serial.println(Motor_Speed);
  Serial.print("Car Direction: "); Serial.println(myData.Direction);
  Serial.print("RF Connection: "); Serial.println(RF_Connection);
  delay(50);
 }
else {
  digitalWrite( PC_Link,LOW);
  digitalWrite( RF_Link,LOW);
}
}

/****************** Search For RF Connection ***************************/
void SearchRF(){
//if (Serial_Connection) {
 if (RF_Connection == 0) {
  radio.startListening();
  if (radio.available()) {
     radio.read( &devicetype, sizeof(int));
     if (devicetype == 1)
     {
      digitalWrite(RF_Link,HIGH);
      Serial.println("RF Link Found");
      Serial.print("DeviceType:");
      Serial.println(devicetype);
      RF_Connection = 1;}
    }
}
}

/****************** Get RF Data From Device ***************************/
void RF_GetData(){
if (RF_Connection){
radio.startListening();
 if( radio.available()){
      while (radio.available()) {
        radio.read( &myData2, sizeof(myData2) );
      }
  }
  else 
  {
  StartTime = micros();
  while (!radio.available()){
    if (micros() - StartTime > TimeOutTime ){ //If No RF Data Was Received for more then TimeOutTime.
      digitalWrite(RF_Link,LOW);
      Serial.println("RF Link Lost");
      RF_Connection = 0;
      break; //Get Out from the loop to communicate with pc.
    }
  }
  }
}
else {
 // Serial.println("RF Fail");
 // delay(TimeOutTime/1000);
  }
}


/****************** Send Data To RF Device ***************************/
void RF_SendData(){
  if (RF_Connection){
  radio.stopListening();
  if (!radio.write( &myData, sizeof(myData))){}
}
}




