/* Install Required Libraies
    1.NTPClient by Fabrice Weinberg
    2.Base64 by Xander Electronics
    3.ESP32Servo by Kevin Harrington, John K. Bennett
    4.Firebase ESP32 Client by Mobizt
*/
//Libraries for Wi-Fi, Timestamps, Firebase, ฺBase64 and Camera
#include <WiFi.h>
#include "esp_camera.h"
#include <Firebase_ESP_Client.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <base64.h>
#include "driver/rtc_io.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

//Network Credentials
#define WIFI_SSID "Hestia"
#define WIFI_PASSWORD "88888888"

// Firebase Project API credentials
#define API_KEY "AIzaSyAHLRtssn4JO1hosTj7N6bnjzIgsIqfWCU"
#define DATABASE_URL "https://car-toll-gate-iot-df3fc-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Camera configuration for AI Thinker module
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Network Time Protocol (NTP) setup
const char* ntpServer = "pool.ntp.org";  // NTP server
const long utcOffsetInSeconds = 25200;   // Thailand (UTC + 7:00)

// Initialize the NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds);
String currentTime = "";

//Declare Sensor pin
int servoPin = 14; // GPIO pin for the servo motor
int entrySensorPin = 13; // GPIO pin for the entry sensor
int exitSensorPin = 15; // GPIO pin for the exit sensor

Servo myServo; // Servo object

// Variables to Display
String gateStatus = "";
String carStatus = "";
String imageStatus ="";
//+ String currentTime = ""; อยู่ด้านบนแล้ว

//Variables to System
int gateDelay = 1500;

bool signupOK = false

//Adjust Camera Setting Function
void adjustCameraSettings() {
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 2); 
  s->set_contrast(s, 1);
  s->set_saturation(s, 0);
  s->set_gainceiling(s, (gainceiling_t)6); 
  s->set_exposure_ctrl(s, 1); 
  s->set_aec2(s, 1); 
}

//Set up Function
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  
  pinMode(entrySensorPin, INPUT);
  pinMode(exitSensorPin, INPUT);

  //Initialize Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  //Sign up
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("Sign Up OK");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Camera config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Adjust frame size and quality based on PSRAM availability
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    Serial.println("PSRAM found");
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  } else {
    adjustCameraSettings();
  }
  
  // Initialize NTPClient
  timeClient.begin();
  timeClient.update();
  
  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myServo.setPeriodHertz(50);            // Standard 50 hz servo
  myServo.attach(servoPin, 1000, 2000);  // Attaches the servo on pin 18 to the servo object
  
  // Set the initial position of the servo (Gate Close)
  myServo.write(90);

}

//Open Gate Function
void openGate(){
  gateStatus = "Gate Opening";
  Serial.println("Gate Open");
  myServo.write(0);
  delay(gateDelay);
}

//Close Gate Function
void closeGate(){
  gateStatus = "Gate Closing";
  Serial.println("Gate Close");
  myServo.write(90);
  delay(gateDelay);
}

// Upload Entry Car Photo to Firebase RealTime Database Function
void uploadPhoto() {
  delay(1000); //Avoid green tint capture
  timeClient.update();
  currentTime = timeClient.getFormattedTime();

  if(Firebase.ready() && signupOK){
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
  
    imageStatus = "Image Capture Successful";

    //Convert Image to Base64
    String imageFile = "data:image/jpeg;base64,";
    imageFile += base64::encode(fb->buf, fb->len);
    esp_camera_fb_return(fb);
    
    //Upload Photo, Car status and Timestamp to Firebase Realtime Database
    FirebaseJson json;
    json.set("photo", imageFile); //Image Base64
    json.set("carStatus", carStatus); //Entry Car
    json.set("timestamp", currentTime); //Timestamp

    //Handle Upload Status
    if (Firebase.RTDB.pushJSON(&fbdo, "/esp32-cam-car-capture", &json)) {
      Serial.println("Photo uploaded:");
      Serial.println(fbdo.dataPath());
      Serial.println(fbdo.pushName());
    } else {
      Serial.println("Upload failed: " + fbdo.errorReason());
    }
  }
}

// Upload Exit Car status and Timestamp to Firebase RealTime Database Function
void uploadExit() {
  timeClient.update();
  currentTime = timeClient.getFormattedTime();

  if(Firebase.ready() && signupOK){
    //Upload
    FirebaseJson json;
    json.set("carStatus", carStatus); //Exit Car
    json.set("timestamp", currentTime); //Timestamp

    //Handle Upload Status
    if (Firebase.RTDB.pushJSON(&fbdo, "/esp32-cam-car-capture", &json)) {
      Serial.println("Exit Uploaded");
      Serial.println(fbdo.dataPath());
      Serial.println(fbdo.pushName());
    } else {
      Serial.println("Upload failed: " + fbdo.errorReason());
    }
  }
}

//Main Function
void loop() {
  timeClient.update();
  currentTime = timeClient.getFormattedTime();
    
  // Monitor Sensor States for Car Entry/Exit
  //Entry
  if (digitalRead(entrySensorPin) == LOW) {
    carStatus = "Entry";
    uploadPhoto();

    //Gate Operation
    openGate();
    delay(gateDelay); // delay for car need to be in a position
    closeGate();      
  }
  //Exit
  if (digitalRead(exitSensorPin) == LOW) {
    carStatus = "Exit";
    uploadExit();

    //Gate Operation
    openGate();
    delay(gateDelay);
    closeGate();   
  }
}







