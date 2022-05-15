#include <SoftwareSerial.h>
#include "dht.h"
SoftwareSerial esp8266(10,11);
#define serialCommunicationSpeed 115200
#define DEBUG true
#define dht_apin A0 // Analog Pin sensor is connected to
#define pirPin 8 

//Struct to store the configuraiton received, first time while seting up the system
struct Relays {
  int pin;
  String identifier;
};

//For now we have array of 10 as we are supporting up to 10 relays
Relays items[10];
Ex: //{1,bulb}, {2, switch1},{7, switch3}

//this stores the current length of the init.
int rLength =1;
//DHT sensor library variable
dht DHT;

// This is our setup method it will be executed once arduino is powered up.
void setup()
{
  Serial.begin(serialCommunicationSpeed);
  esp8266.begin(serialCommunicationSpeed);
  InitWifiModule();
  pinMode(7, OUTPUT);
  //for the demo purpose we are adding the entry to our relay configuration
  insertInRelays(&items[0], 7, "bulb");

  //Initially we are turning off all the relays
  for(int i=0;i<rLength;i++){
    digitalWrite(items[i].pin, HIGH);
  }  
}

void loop()
{
  //This receives the input from the ESP-01 <- wifi data <- raspberry pi
  String input = getData();
  // then on the basis of the input command we take the action.
  checkAction(input);
  // this checks the temp and humidity
  sendTempAndHumidity();
  //delay 
  delay(1000);
}

/*
 * This function checks if the given command is of type init or normal signal command.
 * If init then we call invokeInit.
 * Else we check and matches the command name with our sRelay struct idenitfier to find the pin
 * and then if it is accompanied with "on" then we send the LOW signal and if "off" then HIGH signal to that port.
 * bulb,on/off
 */
void checkAction(String input){
//  Serial.println(input);
  if(input.length()>0){
     Serial.println(input.substring(0,4));
     Serial.println(input);
     if(input.substring(0,4).equalsIgnoreCase("init")){       
      invokeInit(input);
     }
     else{
       int t = input.indexOf(',');
       for(int i=0;i<rLength;i++){
  //        Serial.println("input:" + input.substring(0,t));
  //        Serial.println(items[i].identifier.equalsIgnoreCase(input.substring(0,t)));
          if(items[i].identifier.equalsIgnoreCase(input.substring(0,t))){
  //          Serial.println("Mode" + input.substring(t+1));
  //          Serial.print("PIN::");
  //          Serial.println(items[i].pin);
            if(input.substring(t+1).equalsIgnoreCase("on")){
              Serial.println("Turing::HIGH");              
              digitalWrite(items[i].pin, LOW);
            }
            else if(input.substring(t+1).equalsIgnoreCase("off")){
              Serial.println("Turing::LOW");
              digitalWrite(items[i].pin, HIGH);
              
            }
            //Since we recieve one command at a time we are breaking it as soon as it matches.          
            break;
          }
       }
    }
  }
}


/*
 * This function is called when init command is invoked from the raspberry pi
 * Command looks like below
 * Ex:init<1,bulb><2,switch> 
 * And this will add the pair<1 = pin, bulb=identifier> to the Relay configuration array.
 */
void invokeInit(String input){
  Serial.println("invokeInit");
  int finalLength = 4;
  int i=0;
  if(DEBUG){
  Serial.print("length");
  Serial.println(input.length());
  }
  while(finalLength<input.length() && i<10){
    int startIndex = input.indexOf('<', finalLength) +1;
    if(DEBUG){
    Serial.print("startIndex::");
    Serial.println(startIndex);
    }
    int endIndex = input.indexOf('>', startIndex);
    if(DEBUG){
    Serial.print("endIndex::");
    Serial.println(endIndex);
    }
    String action = input.substring(startIndex, endIndex);
    if(DEBUG){
      Serial.print("action::");
      Serial.println(action);
    }
    String pinString = action.substring(0,1);
    if(DEBUG){
      Serial.print("pinString::");
      Serial.println(pinString);
      Serial.print("actionsubstring::");
      Serial.println(action.substring(2,endIndex));
    }
    insertInRelays(&items[i], pinString.toInt(), action.substring(2,endIndex));
    finalLength = endIndex+1;
    i++;
    rLength = i;
    if(DEBUG){
    Serial.print("finalLength::");
    Serial.println(finalLength);
    Serial.print("i::");
    Serial.println(i);
    Serial.print("rLength::");
    Serial.println(rLength);
    }
    
  }
}

//Helper method to add the entry to the relay array.
void insertInRelays(Relays *r, int pin, String iden){
  r->pin = pin;
  r->identifier = iden;
}



// Helper method to check the temperature and humidity
void sendTempAndHumidity(){
    DHT.read11(dht_apin);
    Serial.print("Current humidity = ");
    Serial.print(DHT.humidity);
    Serial.print("%  ");
    Serial.print("temperature = ");
    Serial.print(DHT.temperature); 
    Serial.println("C  ");
}

//this function checks and get data from the Raspberry PI
//Input -> <bulb,on>
//ouput -> bulb,on (which is our actual input command)

String getData(){
  int timeout = 1000;
  String response = "";
  long int time = millis();
  
  while( (time+timeout) > millis()){
      while(esp8266.available())
      {
        char c = esp8266.read();
        response+=c;
      }
  }
  int start = response.indexOf('<') +1;
  int endI = response.lastIndexOf('>');
  
  if(DEBUG){
//    Serial.println("Response" + response.substring(start,endI));
  }
  return response.substring(start,endI);
}


/*
 * Method for sending the command or data to ESP-01 or other host.
 * Used for initializing the ESP-01 module
 * And can be used for sending it to particular HOST using AT commands
 */
String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    esp8266.print(command);
    if(debug)
    {
      Serial.print("\nprinting Command" + command);
    }
    long int time = millis();
    while( (time+timeout) > millis())
    {
      while(esp8266.available())
      {
        char c = esp8266.read();
        response+=c;
      }  
    }    
    if(debug)
    {
      Serial.print("\nprinting response" + response);
      Serial.print("\nResponse Over\n");
    }
    
    return response;
}

//Series of function which are invoked to initialize the ESP-01
void InitWifiModule()
{
  sendData("AT+RST\r\n", 2000, DEBUG);
  sendData("AT+CWJAP=\"Airtel-Home\",\"Clever@95\"\r\n", 2000, DEBUG);
  delay (3000);
  sendData("AT+CWMODE=1\r\n", 1500, DEBUG);
  delay (1500);
  sendData("AT+CIFSR\r\n", 1500, DEBUG);
  delay (1500);
  sendData("AT+CIPMUX=1\r\n", 1500, DEBUG);
  delay (1500);
  sendData("AT+CIPSERVER=1,80\r\n", 1500, DEBUG);
}
