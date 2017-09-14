#include <Servo.h>
#include <SoftwareSerial.h>

#define GLOBAL_PERIOD_DELAY 50

#define MESSAGE_BUFFER_EXPIRE_TIME  5000


//////// tones related declarations ////////////
//tones are saved in that header file
#include "pitches.h"

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

void playAMelody()
{
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(8, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(8);
  }
}

void playATone(int thisNote) // from 0 to 7
{
  if (thisNote < 0 || thisNote > 7)
    thisNote = 3;
    
  int noteDuration = 1000 / noteDurations[thisNote];
  tone(8, melody[thisNote], noteDuration);
  delay(500);
  noTone(8);
}
////////////////////////////////////////////////


#define AUTOLOCK_TIMEOUT  5


#define SERVO_DOOR_LOCK 50
#define SERVO_DOOR_OPEN 150

int xPin = A1;
int yPin = A0;
int joystickButtonPin = 12;

#define THUMBSTICK_CONFIRMATION_ACCEPTANCE_NUM  5
int ThumbsticConfirmationCount = 0;

SoftwareSerial mySerial(3, 4); // RX, TX
String message = ""; //string that stores the incoming message
int message_buffer_expire_counter = 0;
boolean stringComplete = false;

Servo myservo;  // create servo object to control a servo

int potpin = 0;  // analog pin used to connect the potentiometer
int val;    // variable to read the value from the analog 

int xPosition = 0;
int yPosition = 0;
int joystickButtonState = 0;

int DoorState = 0; // 0-lock; 1-open

//security related
#define PIN_UNLOCK_EXPIRE 5000
String pinCodeStr = "0000";
boolean pinTemporaryUnlocked = false;
int pinUnlockCounter = 0;

void setup()
{
  Serial.begin(9600);
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  myservo.write(SERVO_DOOR_LOCK);
  delay(1000);
  myservo.detach();
  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600); 
  
  pinMode(13, OUTPUT);
  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);

  // joystick button
  pinMode(joystickButtonPin, INPUT_PULLUP);
  
}

void loop() 
{
  xPosition = analogRead(xPin);
  yPosition = analogRead(yPin);

  /*
  while(mySerial.available())
  {
    Serial.println("keldi");
    //while there is data available on the serial monitor
    message+=char(mySerial.read());//store string from serial command
  }*/
  serialEvent();

  
    
  if(stringComplete)
  {

    if (pinTemporaryUnlocked) // if pin is unlocked then go for command processing
    {
      //if data is available
      Serial.println(message); //show the data
      if (message == "21")
      {
        Serial.println("open door"); //show the data
          myservo.attach(9);
          myservo.write(SERVO_DOOR_OPEN);                  // sets the servo position according to the scaled value 
          playAMelody();
          delay(15);                           // waits for the servo to get there 
          
          digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
          mySerial.println(message);
          DoorState = 1;
          delay(1000);
          myservo.detach();
      }
      if (message == "20")
      {
        myservo.attach(9);
        myservo.write(SERVO_DOOR_LOCK);                  // sets the servo position according to the scaled value 
        delay(15);                           // waits for the servo to get there 
        delay(1000);
        myservo.detach();
  
        Serial.println("close door bluee"); //show the data
        mySerial.println(message);
        digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level)      
        DoorState = 0;
      }
    }
    else
    {
      if (message == pinCodeStr){
        pinUnlockCounter = PIN_UNLOCK_EXPIRE;
        pinTemporaryUnlocked = true;
      }
    }
    message=""; //clear the data
    stringComplete = false;
  }


  //check the joystick button push
  joystickButtonState = digitalRead(joystickButtonPin);
  if (joystickButtonState == 0)
  {
    
    // play a 5 second timer
    for (int autolock_i=0;autolock_i<AUTOLOCK_TIMEOUT;autolock_i++)
    {
      playATone(1);
      delay(1000);
    }

    /// close the door
    myservo.attach(9);
    myservo.write(SERVO_DOOR_LOCK);                  // sets the servo position according to the scaled value 
                           // waits for the servo to get there 
    Serial.println("close door joystick"); //show the data
    Serial.println();
    digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level)
    delay(1000);
    myservo.detach();

    DoorState = 0x0; //state of the door must be 0, means locked
  }
   
/*
  /// activate timer for closing the door 
  if (yPosition < 300 || yPosition > 500)
  {
    // play a 5 second timer
    for (int autolock_i=0;autolock_i<AUTOLOCK_TIMEOUT;autolock_i++)
    {
      playATone(1);
      delay(1000);
    }

    /// close the door
    myservo.attach(9);
    myservo.write(SERVO_DOOR_LOCK);                  // sets the servo position according to the scaled value 
                           // waits for the servo to get there 
    Serial.println("close door joystick"); //show the data
    digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level)
    delay(1000);
    myservo.detach();

    DoorState = 0x0; //state of the door must be 0, means locked
  }
*/
  
  /// manual control closing and opening the door
  if ((xPosition < 300 || xPosition > 500 )||
      (yPosition < 300 || yPosition > 500))
  {
    ThumbsticConfirmationCount = ThumbsticConfirmationCount + 1;
    
    if (ThumbsticConfirmationCount > THUMBSTICK_CONFIRMATION_ACCEPTANCE_NUM)
    {
    
      DoorState = DoorState ^ 0x1;
      if (DoorState)
      {
        myservo.attach(9);
        myservo.write(SERVO_DOOR_OPEN);                  // sets the servo position according to the scaled value 
                                   // waits for the servo to get there 
        digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(1000);
        myservo.detach();
      }
      else
      {
        myservo.attach(9);
        myservo.write(SERVO_DOOR_LOCK);                  // sets the servo position according to the scaled value 
                           // waits for the servo to get there 
        Serial.println("close door joystick2"); //show the data
        digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level)
        delay(1000);
        myservo.detach();
      }
      
      ThumbsticConfirmationCount = 0;
     
      //Serial.print(xPosition);
      //Serial.print(":");
      //Serial.println(yPosition);
      
    }
  }
  else
  {
    ThumbsticConfirmationCount = 0;
  }

  //Serial.print(xPosition);
  //Serial.print(":");
  //Serial.println(yPosition);
  
  delay(GLOBAL_PERIOD_DELAY);
  if (pinUnlockCounter > 0)
  {  
    pinUnlockCounter -= GLOBAL_PERIOD_DELAY;
  }
  else
  {
    pinTemporaryUnlocked = false;
  }
  
}  


void serialEvent(){
  while (mySerial.available()) {
    char inChar = (char)mySerial.read();

    if (inChar == '\n' || inChar == '\0'){
      stringComplete = true;
    }
    else
      message += inChar;
  }

  //Serial.println(message);
  if (message != "" && message_buffer_expire_counter > 0)
  {
    message_buffer_expire_counter -= GLOBAL_PERIOD_DELAY;
    if (message_buffer_expire_counter <= 0)
    {
      message_buffer_expire_counter = 0;
      message = "";
    }   
  }
  else
  {
    if (message != "") message_buffer_expire_counter = MESSAGE_BUFFER_EXPIRE_TIME;
  }
  
}
