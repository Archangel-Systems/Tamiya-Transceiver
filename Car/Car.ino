/*
 Advanced Arduino Data Transmitter
 */
//#include <NewPing.h>
#include <SPI.h>
#include <Servo.h>
#include "RF24.h"

/****************** Pin Locations ***************************/
#define PC_Link      2  // Rx/TX Board Link Indicator LED.
#define RF_Link      4 // RF Communication Status LED.
#define GND          A7 // Common Ground.

#define Steering_Srv 3 // Set Steering Servo on pin 3.
#define MotorA_IA    5 // Motor A Direction Set to 0 OR 255.
#define MotorA_IB    6 // Motor A Speed.

/****************** RF Communication ***************************/
byte addresses[][6] = {"1Node","2Node"}; //Create "Nodes" [ Devices ]
bool radioNumber = 0; // Set this radio as radio number 0 for Rx or 1 for Tx.
RF24 radio(7,8); //Set up nRF24L01 radio on SPI bus. pins 7&8 for regular use OR pins 9&10 for FUNDUINO Shield.
//RF24 radio(9,10); //Set up nRF24L01 radio on SPI bus. pins 7&8 for regular use OR pins 9&10 for FUNDUINO Shield.
struct dataStruct{bool Direction; int Speed; int Angle;}myData; //Create a data structure for transmitting and receiving Data.
int devicetype = 1; //Set Up Device as: '0'-Host, '1'-Car....
bool RF_Connection = 0;
unsigned long StartTime; //Initializes Counter.
unsigned long TimeOutTime= 250000; //2.5s TimeOut.

/****************** PC Communication ***************************/
String serialcmd; //Receiving PC DATA.
String serialrsnd; //Sended DATA TO PC.
boolean FlowData = false; //Control "Data Flow" [Continuously Send Sensor Data To PC].
bool Serial_Connection = false; //Detect Serial Port Connection Status.
int Motor_Speed = 0;
int Servo_Angle = 0;

/****************** Motors And Lights ***************************/
Servo Steering;
Servo Engine;
/**************************************************************/
void setup()
{
  Steering.attach(Steering_Srv); //Attaches The SteeringWheel Servo To Pin 6.
  Serial.begin(115200); // initialize the serial communication.
  pinMode(MotorA_IA,OUTPUT); // Motor A+
  pinMode(MotorA_IB,OUTPUT); // Motor A-
  digitalWrite(MotorA_IA,LOW); // Stop Motor A.
  digitalWrite(MotorA_IB,LOW); // Stop Motor A.
  pinMode(GND,INPUT); //Define Common Ground.
  radioNumber = 0; //Set Radio Channel as either Rx['0'] Or Tx ['1'].
  pinMode(PC_Link,OUTPUT);
  pinMode(RF_Link,OUTPUT);
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
  SearchRF();
  RF_GetData();
  Analyze_RFData();
  RF_SendData();
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
    Serial_Tx();
  }
}

/****************** Control The "FLOW" of data to pc. ***************************/ 
void Serial_Tx(){
if (Serial_Connection == true) {
 digitalWrite( PC_Link,HIGH);
 if (RF_Connection == 1) 
 {
 Serial.print("Current Servo Angle: ");
 Serial.println(myData.Angle);
 Serial.print("Current Motor Speed: ");
 Serial.println(Motor_Speed);
 Serial.print("Current Motor Direction: ");
 Serial.println(myData.Direction);
 }
}
else {
  digitalWrite( PC_Link,LOW);
}
}

/****************** Search For HOST ***************************/
void SearchRF(){
 if (RF_Connection == 0) {
  radio.stopListening();
  if (!radio.write( &devicetype, sizeof(int)))
  {Serial.println(F("failed"));}
  else
  {RF_Connection = 1; digitalWrite(RF_Link,HIGH);Serial.println("Rf Connection established");}
}
}

/****************** Get Data From PC ***************************/
void RF_GetData(){
if (RF_Connection){
radio.startListening();
 if( radio.available()){
        radio.read( &myData, sizeof(myData) );
  }
  else 
  {
  StartTime = micros();
  while (!radio.available()){
    if (micros() - StartTime > TimeOutTime ){ //If No RF Data Was Received for more then TimeOutTime.
      digitalWrite(RF_Link,LOW);
      RF_Connection = 0;
      Serial.println("Rf Connection lost");
      break; //Get Out from the loop to communicate with pc.
    }
  } 
  }
}
}


/****************** Send Data To PC ***************************/
void RF_SendData(){
  if (RF_Connection == 1){
  radio.stopListening();
  if (!radio.write( &myData, sizeof(myData))){}
}
}

/****************** Analyze Incoming RF Data ***************************/
void Analyze_RFData(){
 if (RF_Connection == 1) {
  Steering.write(myData.Angle);
  if (myData.Direction == 0)
   {
    Motor_Speed = -myData.Speed;
    analogWrite(MotorA_IA,LOW);
    analogWrite(MotorA_IB,255-Motor_Speed);
   }
  else
   {
    Motor_Speed = myData.Speed;
    analogWrite(MotorA_IA,255);
    analogWrite(MotorA_IB,255-Motor_Speed);
   }  
 }
 else
  {
   Steering.write(90);
   analogWrite(MotorA_IA,0);
   analogWrite(MotorA_IB,0);
  }
}









