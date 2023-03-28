#include <Adafruit_GFX.h> // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include "pitches.h"
#include "virtuabotixRTC.h" // RTC library for the DS1302 chip

// TFT pins
#define TFT_PIN_CS 10 // chip selection
#define TFT_PIN_DC 9 // data command
#define TFT_PIN_RST 7 // reset
#define TFT_PIN_MOSI 11 // data output from master
#define TFT_PIN_SCLK 13 // serial clock

// TFT config
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_MOSI, TFT_PIN_SCLK, TFT_PIN_RST);
const uint16_t TFT_WIDTH = 128;
const uint16_t TFT_HEIGHT = 160;
const uint8_t TFT_ROTATION = 1;
const uint8_t TFT_TIME_TEXT_SIZE = 4;
const uint8_t TFT_TIME_CURSOR_POSITION_X = 20;
const uint8_t TFT_TIME_CURSOR_POSITION_Y = 50;
const String TFT_TIME_SEPERATOR = ":";
const uint8_t TFT_OPEN_STATUS_CURSOR_POSITION_X = 20;
const uint8_t TFT_OPEN_STATUS_CURSOR_POSITION_Y = 50;
const String TFT_OPEN_STATUS_TEXT = "OFFEN";

// RTC pins
#define RTC_PIN_CLK A5 // clock
#define RTC_PIN_DAT A4 // data
#define RTC_PIN_RST A3 // reset

// Instantiate RTC object
virtuabotixRTC rtc_module(RTC_PIN_CLK, RTC_PIN_DAT, RTC_PIN_RST);
const int RTC_SECONDS = 0;
const int RTC_MINUTES = 0;
const int RTC_HOURS = 12;
const int RTC_DAY_OF_WEEK = 1;
const int RTC_DAY_OF_MONTH = 1;
const int RTC_MONTH = 1;
const int RTC_YEAR = 2023;

// Motor pins
#define MOTOR_PIN_IN1 7
#define MOTOR_PIN_IN2 6
#define MOTOR_PIN_IN3 5
#define MOTOR_PIN_IN4 4

// Piezo pin
#define PIEZO_PIN 3
// Duration of the piezo notes
const int NOTE_DURATION = 650;

// Relay pin
#define RELAY_PIN 2

// Init values for the clock
uint8_t hours = -1;
uint8_t minutes = -1;

// Baudrate for the serial connection
const int BAUDRATE = 9600;

const int OPEN_RELAY_CODE = 100;
const int RESET_CODE = 200;
const int RESET_RELAY_ACTIVATION_TIME = 10000; // relay shall be active for 10 seconds during a reset

// This functions programmatically resets the arduino
void(* resetFunc) (void) = 0;

void init_pins(){
  pinMode(MOTOR_PIN_IN1, OUTPUT);
  pinMode(MOTOR_PIN_IN2, OUTPUT);
  pinMode(MOTOR_PIN_IN3, OUTPUT);
  pinMode(MOTOR_PIN_IN4, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
}

void init_tft(){
  tft.initR(INITR_GREENTAB);
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(TFT_TIME_TEXT_SIZE);
  tft.setTextColor(ST7735_WHITE);
}

void init_rtc(){
  rtc_module.setDS1302Time(RTC_SECONDS, RTC_MINUTES, RTC_HOURS, RTC_DAY_OF_WEEK, RTC_DAY_OF_MONTH, RTC_MONTH, RTC_YEAR); 
}

void setup() {
  // Start serial connection
  Serial.begin(BAUDRATE);
  // Setup pins
  init_pins();
  // Initialize and configure TFT screen
  init_tft();
  // Initialize RTC
  init_rtc();
}


void print_time_to_tft(int hours, int minutes) {
  // Clear screen and set cursor
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(TFT_TIME_CURSOR_POSITION_X, TFT_TIME_CURSOR_POSITION_Y);

  // Print prefix 0 if the hours are only one digit
  if (hours < 10) {
      tft.print("0");
  }
  tft.print(hours);

  // Print seperator
  tft.print(TFT_TIME_SEPERATOR);

  // Print prefix 0 if the minutes are only one digit
  if (minutes < 10) {
    tft.print("0");
  }
  tft.print(minutes);
}

void print_open_status_to_tft() {
  // Clear screen and set cursor
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(TFT_OPEN_STATUS_CURSOR_POSITION_X, TFT_OPEN_STATUS_CURSOR_POSITION_Y);
  tft.print(TFT_OPEN_STATUS_TEXT);
}

void update_clock() {
  // Update the time if a minute has passed or if it is the first time starting the program
  if(minutes == -1 || minutes != rtc_module.minutes) {
    // Get newest time values from the RTC
    minutes = rtc_module.minutes;
    hours = rtc_module.hours;
    // Print the time
    print_time_to_tft(hours, minutes);
  }
}

void open_door() {
  print_open_status_to_tft();
  digitalWrite(RELAY_PIN, HIGH);
}

void reset() {
  // Activate the relay, close it after a period of time
  open_door();
  delay(RESET_RELAY_ACTIVATION_TIME);
  digitalWrite(RELAY_PIN, LOW);
  
  // Reset the arduino
  resetFunc();
}

void handle_received_code(int code) {
  // Play different notes via the piezo according to the gpio pins 
  switch (code) {
    case 19:
    case 7:
    case 9:
      tone(PIEZO_PIN, NOTE_F5, NOTE_DURATION);
      break;
    
    case 16:
    case 13:
    case 10:
    case 17:
    case 21:
    case 25:
    case 20:
    case 3:
      tone(PIEZO_PIN, NOTE_G5, NOTE_DURATION);
      break;
      
    case 11:
    case 5:
    case 12:
      tone(PIEZO_PIN, NOTE_F7, NOTE_DURATION);
      break;
      
    case 4:
    case 26:
    case 24:
    case 22:
      tone(PIEZO_PIN, NOTE_C3, NOTE_DURATION);
      break;
      
    case 8:
    case 27:
      tone(PIEZO_PIN, NOTE_D6, NOTE_DURATION);
      break;
    
    case 2:
    case 6:
      tone(PIEZO_PIN, NOTE_E5, NOTE_DURATION);
      break;
    
    case OPEN_RELAY_CODE:
      open_door();
      break;
    
    case RESET_CODE:
      reset();
      break;
    
    default:
      break;
  }
}

void handle_input() {
  if (Serial.available()) {
    // Receive active code/GPIO pin from the Raspberry Pi
    uint8_t receivedCode;
    Serial.readBytes((char*)&receivedCode, sizeof(receivedCode));
    handle_received_code(static_cast<int>(receivedCode));
  }
}

void loop() {
  rtc_module.updateTime();
  update_clock();
  handle_input();
  delay(50); // delay so that there is a little bit of time between pressed buttons
}
