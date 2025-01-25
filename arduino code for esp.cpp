// DHT
/////////////////////////////////////////////////////////////////////////////
#include <DHT.h>
DHT dht(5, DHT22);
float humidity;
float temperature;


// ULTRASONIC
/////////////////////////////////////////////////////////////////////////////
// Define the pins for the ultrasonic sensor
const int trigPin = 13;
const int echoPin = 35;
long duration;
int distance;
int fuelHeight;
int volumeInTank;


// WIFI
////////////////////////////////////////////////////////////
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Insert your network credentials
#define WIFI_SSID1 "Simba2.4G"
#define WIFI_PASSWORD1 "myWiFi2.4G"
#define WIFI_SSID2 "Pixel 6 Pro"
#define WIFI_PASSWORD2 "g7hcy5rahjpmi2i"
#define WIFI_SSID3 "Simba2.4G"
#define WIFI_PASSWORD3 "myWiFi2.4G"
#define WIFI_SSID4 "Kikota's Sony"
#define WIFI_PASSWORD4 "kikota4000"



// FIREBASE
////////////////////////////////////////////////////////////
// Insert Firebase project API Key
#define API_KEY "AIzaSyB8smbF627Xh_zZiagvIPqrKaoSlDumTJ4"
// #define API_KEY "AIzaSyDNWD1t0Z_awgc4lZDqKY69GClM9fKdeAU"
// Insert RTDB URL
#define DATABASE_URL "https://fsms-48cb1-default-rtdb.firebaseio.com/"
// #define DATABASE_URL "https://fuel-management-system-16fb2-default-rtdb.firebaseio.com/"
// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;


// FIREBASE DATA
////////////////////////////////////////////////////////////
float pricePerLitre = 0;
int oldPrice;


// FLOW
////////////////////////////////////////////////////////////
unsigned long oldTime = 0;
volatile int userPulseCount = 0;
volatile int stationPulseCount = 0;
float calibrationFactor = 4.5; // Adjust this value based on your sensors' specifications

volatile int flow_frequency_station; // Measures flow sensor pulses for station
unsigned int l_hour_station; // Calculated litres/hour for station
int flowsensor_station = 36; // Sensor Input for station
unsigned long currentTime;
unsigned long cloopTime;
float total_volume_station = 0; // Total volume of water for station (in liters)
float lastStationVolume;
float unitsUsedStation;
float fuelToBeFilled = 0;
float fuelfilled = 0;
int fuel;
void stationPulseCounter() {
  stationPulseCount++;
}
void flow_station() // Interrupt function for station sensor
{
   flow_frequency_station++;
}


// LCD
///////////////////////////////////////////////////////////////////////////////////////
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
int dp;


// TIME
////////////////////////////////////////////////////////////
#include "time.h"
#include "sntp.h"
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 10800;  // GMT+3 hours
const int   daylightOffset_sec = 0; // No daylight saving time
const char* time_zone = "EAT-3";  // TimeZone rule for East Africa Time (UTC+3)
String getLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    // Serial.println("No time available (yet)");
    return "";
  }

  // Format time as index = "YYYY_MM_DD_HH:MM:SS"
  char timeStringBuff[25]; // Buffer to store the formatted time
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y_%m_%d/%H:%M:%S", &timeinfo);
  String index = String(timeStringBuff);

  // Print the formatted time
  // Serial.println(index);

  return index;
}
// Callback function (gets called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  // Serial.println("Got time adjustment from NTP!");
  getLocalTime();
}


// KEYPAD
///////////////////////////////////////////////////////////////////////////////////////
#include <Keypad.h>
const byte ROWS = 5; 
const byte COLS = 4; 
char keys[ROWS][COLS] = { // keymap
  {'F', 'G', '#', '*'},
  {'1', '2', '3', 'U'},
  {'4', '5', '6', 'D'},
  {'7', '8', '9', 'E'},
  {'L', '0', 'R', 'N'}
};
#define ROW0 33
#define ROW1 25
#define ROW2 26
#define ROW3 27
#define ROW4 14
#define COL0 15
#define COL1 4
#define COL2 2 // *******
#define COL3 12
// Connect keypad ROW0, ROW1, ROW2, ROW3, ROW4 to these Arduino pins
byte rowPins[ROWS] = {ROW0, ROW1, ROW2, ROW3, ROW4}; 
// Connect keypad COL0, COL1, COL2, COL3 to these Arduino pins
byte colPins[COLS] = {COL0, COL1, COL2, COL3}; 
// Create the Keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
int inputIndex=0;
String digit0,digit1,digit2,digit3,digit4,digit5,digit6;
String amount;



// OTHER PINS
///////////////////////////////////////////////////////////////////////////////////////
int pumpPin = 19;
int valvePin = 18;



void WifiSetup() {
  const char* ssids[] = {WIFI_SSID1, WIFI_SSID2, WIFI_SSID3, WIFI_SSID4};
  const char* passwords[] = {WIFI_PASSWORD1, WIFI_PASSWORD2, WIFI_PASSWORD3, WIFI_PASSWORD4};

  Serial.print("Connecting to Wi-Fi");
  
  for (int i = 0; i < 4; i++) {
    WiFi.begin(ssids[i], passwords[i]);

    int attemptCounter = 0;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(300);

      attemptCounter++;
      if (attemptCounter >= 10) { // Try each network for 10 attempts (3 seconds each)
        break;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("Connected to ");
      Serial.print(ssids[i]);
      Serial.print(" with IP: ");
      Serial.println(WiFi.localIP());
      Serial.println();

      // Display on the LCD
      lcd.clear();
      lcd.print("Connected to ");
      lcd.print(ssids[i]);
      lcd.setCursor(0, 1); // Move to the second line of the LCD
      lcd.print("IP: ");
      lcd.print(WiFi.localIP());

      delay(2000); // Delay for 2 seconds

      return;
    }
  }

  Serial.println();
  Serial.println("Failed to connect to any Wi-Fi network.");
}

void bootUpLCDDisplay(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("System");
  
  lcd.setCursor(0,1);
  lcd.print("Initializing");
  
  lcd.setCursor(0,2);
  lcd.print("Connecting Wi-Fi");
  
  if(dp==5){ dp=0; }
  lcd.setCursor(dp,3);
  lcd.print(".");
  dp++;
}

void FirebaseSetup(){
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void ultrasonicSetup(){
  // Set the trigPin as an OUTPUT
  pinMode(trigPin, OUTPUT);
  // Set the echoPin as an INPUT
  pinMode(echoPin, INPUT);
}

void keyCapture(){
  char key = keypad.getKey();
  if (key) {
    if(key == 'N'){
      if(amount.toFloat() > 0){
        inputIndex = 0; // reset digit capturing

        fuelToBeFilled = amount.toFloat();

        //turn on pump and valve
        analogWrite(pumpPin, 255);
        analogWrite(valvePin, 255);
        
        fuelToBeFilled = amount.toFloat()/pricePerLitre;

        fuelfilled = 0;

        // UPLOAD DHT and Volume
        updateDHTVariables();
        updateUltrasonicVariables();

        // UPLOAD SALE
        UploadSale(amount.toFloat());
        
        // Get price per litre
        getPricePerLitre();

        takeflowReadings(fuelToBeFilled);

        // delay(10000);

        analogWrite(pumpPin, 0);
        analogWrite(valvePin, 0);
      }
    }

    if (key=='1' || key=='2' || key=='3' || key=='4' || 
        key=='5' || key=='6' || key=='7' || key=='8' || 
        key=='9' || key=='0'){
      if(inputIndex == 0 && key){
        digit0=key;
        amount = digit0;
        lcdDisplayAmount(amount);
        inputIndex++;
      }
      else if(inputIndex == 1){
        digit1=key;
        amount = digit0+digit1;
        lcdDisplayAmount(amount);
        inputIndex++;
      }
      else if(inputIndex == 2){
        digit2=key;
        amount = digit0 + digit1 + digit2;
        lcdDisplayAmount(amount);
        inputIndex++;
      }
      else if(inputIndex == 3){
        digit3=key;
        amount = digit0 + digit1 + digit2 + digit3;
        lcdDisplayAmount(amount);
        inputIndex++;
      }
      else if(inputIndex == 4){
        digit4=key;
        amount = digit0 + digit1 + digit2 + digit3 + digit4;
        lcdDisplayAmount(amount);
        inputIndex++;
      }
      else if(inputIndex == 5){
        digit5=key;
        amount = digit0 + digit1 + digit2 + digit3 + digit4 + digit5;
        lcdDisplayAmount(amount);
        inputIndex++;
      }
      else if(inputIndex == 6){
        digit6=key;
        amount = digit0 + digit1 + digit2 + digit3 + digit4 + digit5 + digit6;
        lcdDisplayAmount(amount);
        inputIndex++;
      }
    }
  }
}

void lcdDisplayAmount(String amount){
  lcd.clear();

  lcd.setCursor(0, 0);
  if(signupOK == true) {lcd.print("DATABASE CONNECTED");   }
  else                 {lcd.print("DATABASE UNCONNECTED"); }

  lcd.setCursor(0, 1);
  lcd.print("PRICE/LITRE:  ");
  lcd.print(pricePerLitre);

  lcd.setCursor(0, 2);
  lcd.print("LITRES FILLED:0.00");

  lcd.setCursor(0, 3);
  lcd.print("TOTAL PRICE:  ");
  lcd.print(amount);
}

void lcdDisplayOnFilling(String fuel){
  lcd.clear();

  lcd.setCursor(0, 0);
  if(signupOK == true) {lcd.print("DATABASE CONNECTED");   }
  else                 {lcd.print("DATABASE UNCONNECTED"); }

  lcd.setCursor(0, 1);
  lcd.print("PRICE/LITRE:  ");
  lcd.print(pricePerLitre);

  lcd.setCursor(0, 2);
  lcd.print("LITRES FILLED:");
  lcd.print(fuel);

  lcd.setCursor(0, 3);
  lcd.print("TOTAL PRICE:  ");
  lcd.print(amount.toFloat());
}

void getPricePerLitre(){
  if (Firebase.ready() && signupOK) {
    // Retrieve odometer values
    if (Firebase.RTDB.getInt(&fbdo, "Price")) {
      pricePerLitre = fbdo.intData();
      Serial.print("new price:");
      Serial.println(pricePerLitre);
    } else {
      Serial.println(fbdo.errorReason());
    }

    if (oldPrice != pricePerLitre){
      lcdDisplayAmount("0");
      oldPrice = pricePerLitre;
    }
    // Wait a bit before the next retrieval
    // delay(1000);  // Adjust the delay as needed
  }
}

void flowSetup(){
  pinMode(flowsensor_station, INPUT);
  digitalWrite(flowsensor_station, HIGH); // Optional Internal Pull-Up
  
  attachInterrupt(digitalPinToInterrupt(flowsensor_station), stationPulseCounter, FALLING);
}

void takeflowReadings(float fuelToBeFilled){
  currentTime = millis();
  // Every second, calculate and print litres/hour
  while(fuelToBeFilled>0){
    if((millis() - oldTime) > 1000)
    {
      detachInterrupt(digitalPinToInterrupt(flowsensor_station));

      float stationFlowRate = ((1000.0 / (millis() - oldTime)) * stationPulseCount) / calibrationFactor;

      oldTime = millis();
      stationPulseCount = 0;
      userPulseCount = 0;

      float prevStationVolume = total_volume_station;

      attachInterrupt(digitalPinToInterrupt(flowsensor_station), stationPulseCounter, FALLING);
        
      // Update total volume (liters)
      total_volume_station += stationFlowRate / 60.0; // Convert L/min to L/s and accumulate
      
      // Finding units used
      fuelToBeFilled -=  stationFlowRate / 60.0;;

      fuelfilled += stationFlowRate / 60.0;;

      // display status
      lcdDisplayOnFilling(String(fuelfilled));

      // Reset Counter
      flow_frequency_station = 0;
    }
  }
  lcdDisplayOnFilling(String(amount.toFloat()/pricePerLitre));
}

void updateDHTVariables(){
  // delay(2000);  // Delay between readings
  
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" ͦC");
}

void updateUltrasonicVariables(){
  // Clear the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Set the trigPin on HIGH state for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the duration from echoPin
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance (in cm) based on the speed of sound
  distance = duration * 0.034 / 2;

  // Water level
  fuelHeight = 21 - distance;
  volumeInTank = 330*fuelHeight;

  // Delay before the next measurement
  // delay(500);
}

void timeSetup(){
  sntp_set_time_sync_notification_cb(timeavailable);
  sntp_servermode_dhcp(1);    // (optional)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  configTzTime(time_zone, ntpServer1, ntpServer2);
}

void UploadSale(int money){

  String humidityPathString = "Humidity/";
  humidityPathString += getLocalTime();
  const char* humidityPath = humidityPathString.c_str();  // Convert String to const char*

  String temperaturePathString = "Temperature/";
  temperaturePathString += getLocalTime();
  const char* temperaturePath = temperaturePathString.c_str();  // Convert String to const char*

  String volumePathString = "Volume/";
  volumePathString += getLocalTime();
  const char* volumePath = volumePathString.c_str();  // Convert String to const char*

  String salesPathString = "Sales/";
  salesPathString += getLocalTime();
  const char* salesPath = salesPathString.c_str();  // Convert String to const char*

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Update user units used
    if (Firebase.RTDB.setInt(&fbdo, salesPath, money)){
      // Serial.println("PASSED");
      // Serial.println("PATH: " + String(fbdo.dataPath()));
      // Serial.println("TYPE: " + String(fbdo.dataType()));
    }
    else {
      // Serial.println("FAILED");
      // Serial.println("REASON: " + String(fbdo.errorReason()));
    }
    if (Firebase.RTDB.setInt(&fbdo, humidityPath, humidity)){
      // Serial.println("PASSED");
      // Serial.println("PATH: " + String(fbdo.dataPath()));
      // Serial.println("TYPE: " + String(fbdo.dataType()));
    }
    else {
      // Serial.println("FAILED");
      // Serial.println("REASON: " + String(fbdo.errorReason()));
    }
    
    // Update users units
    if (Firebase.RTDB.setInt(&fbdo, temperaturePath, temperature)){
      // Serial.println("PASSED");
      // Serial.println("PATH: " + String(fbdo.dataPath()));
      // Serial.println("TYPE: " + String(fbdo.dataType()));
    }
    else {
      // Serial.println("FAILED");
      // Serial.println("REASON: " + String(fbdo.errorReason()));
    }

    // Update users units
    if (Firebase.RTDB.setInt(&fbdo, volumePath, volumeInTank)){
      // Serial.println("PASSED");
      // Serial.println("PATH: " + String(fbdo.dataPath()));
      // Serial.println("TYPE: " + String(fbdo.dataType()));
    }
    else {
      // Serial.println("FAILED");
      // Serial.println("REASON: " + String(fbdo.errorReason()));
    }
  }
}

void setup() {
  Serial.begin(9600);
  
  dht.begin();
  
  lcd.init();
  lcd.backlight();

  ultrasonicSetup();

  WifiSetup();

  FirebaseSetup();

  flowSetup();
  
  getPricePerLitre();

  timeSetup();

  pinMode(pumpPin,OUTPUT);
  pinMode(valvePin,OUTPUT);
}

void loop() {
  
  keyCapture();

}