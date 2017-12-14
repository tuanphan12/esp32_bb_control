//order: S0,S1,S2,S3

/* This code is meant for the Teensy, but will be ported to the ESP32 as needed*/

int centralPins[] = {12,11,10,9};
int auxPins[] = {8,7,6,5};
int SIG_pin = A0;
int EN_pin = 4;
int mode;
int values[50];
int average;
int sum;

int myChannel[16][4] = {
  {0,0,0,0}, //channel 0
  {1,0,0,0}, //channel 1
  {0,1,0,0}, //channel 2
  {1,1,0,0}, //channel 3
  {0,0,1,0}, //channel 4
  {1,0,1,0}, //channel 5
  {0,1,1,0}, //channel 6
  {1,1,1,0}, //channel 7
  {0,0,0,1}, //channel 8 
  {1,0,0,1}, //channel 9
  {0,1,0,1}, //channel 10
  {1,1,0,1}, //channel 11
  {0,0,1,1}, //channel 12
  {1,0,1,1}, //channel 13
  {0,1,1,1}, //channel 14
  {1,1,1,1}  //channel 15
};

String commandString;
boolean stringComplete = false;


uint16_t meas[129];

String upstreamStr;
String downstreamStr;
boolean complete = false;
int low_val = 0;
int high_val = 0;

void setup() {
  meas[128]=0xFFFF; //initialize ending value
  for (int i = 0; i < 4; i++)
  {
    pinMode(centralPins[i],OUTPUT);
    pinMode(auxPins[i],OUTPUT);
    digitalWrite(centralPins[i],LOW);
    digitalWrite(auxPins[i],LOW);
  }
  pinMode(EN_pin,OUTPUT);  //EN pin
  digitalWrite(EN_pin,LOW); //EN pin 
  Serial.begin(115200); //set to 115200 since what the ESP32 will like
  commandString.reserve(200);
}

void loop() {  
  //simple command query checking
  if (complete){
    Serial.println(upstreamStr);
    complete = false;
  }
  serialEvent();
  if (stringComplete){
    processString(commandString);
    Serial.println(low_val);
    Serial.println(high_val);
    upstreamStr = "";     //empty the sent string so no addition information will be overlap
    Calculation(mode,low_val,high_val); 
    low_val = 0;
    high_val = 0;
    commandString = "";
    stringComplete = false;
  }
}

int readChannel(int channel){
  for (int j = 0; j < 4; j++) digitalWrite(centralPins[j],myChannel[channel][j]);
  return analogRead(SIG_pin);
}

void serialEvent() {
  String temp;
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '*') {
      stringComplete = true;
      Serial.println("found");
      break;
    }
    else{
      commandString += inChar;
    }
  }
}

void processString(String pro_String){
  if (pro_String.equalsIgnoreCase("all")){
    mode = 1;
    Serial.println("mode1");
  }else if (pro_String.equalsIgnoreCase("off")){
      digitalWrite(EN_pin,HIGH);
  }else if (pro_String.equalsIgnoreCase("on")){
     digitalWrite(EN_pin,LOW);
  }else{
      String temp;
      for (int h = 0; h < commandString.length(); h++){
        if (commandString[h] == ','){
          low_val = high_val;
          temp = ""; //empty    
        }else{
          temp += commandString[h];
          high_val = (temp.toInt());
        }
      }
      mode = 2;
  }
}

void Calculation(int mode1, int low, int high){
    int pin = low;

    if (mode1 == 1){
      int count = 0;
      for (int i = 0; i < 8; i++){ //Read channel 0-7 of the central mux
        for (int j = 0; j < 16; j++){
          for (int sel = 0; sel < 4; sel++){ //Control the channel of the connecting mux
            digitalWrite(auxPins[sel],myChannel[j][sel]);
          }
          meas[count] = readChannel(i);
          pin++;
          count++;    
        }
      }
      //Serial.write((uint8_t*)meas,258);
    }
    //high_val is the sampling rate
    //low_val is the index of the node
    if (mode1 == 2){
      unsigned long starto = micros();
      int central_mux_channel, aux_mux_channel,delay_step;
      int duration = 20000;    //desired time to wait to acquire data in us
      delay_step = (1000000/high)-1;    //(1/f_sampling)
      int numberOfSamples = duration/delay_step;   //desired # of samples
      elapsedMicros delay_modifier;    //keeping track of the delay
      central_mux_channel = low/16;
      aux_mux_channel = low - (central_mux_channel*16);
      for (int sel = 0; sel < 4; sel++){ //Control the channel of the connecting mux
         digitalWrite(auxPins[sel],myChannel[aux_mux_channel][sel]);
      }
      int sampleI = 0;

      while (sampleI < numberOfSamples){
        while (delay_modifier <= delay_step);
        delay_modifier = 0;
        average = readChannel(central_mux_channel);
        //upstreamStr += low;
        //upstreamStr += ":";
//        downstreamStr += String(micros()-starto);
//        downstreamStr += ",";
        meas[sampleI]=average;
//        upstreamStr += average;
//        upstreamStr += ",";
        sampleI += 1;
      }    
      Serial.write((uint8_t*)meas,256);
      //Serial.println(micros()-starto);  
    }
    complete = true; 
}
