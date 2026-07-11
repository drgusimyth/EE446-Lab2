//Filename:Lab2Task10.ino
//Author: Tommy Zhao
//Date: 07/10/2026
//Description: a rule-based “workspace awareness” system for a desk device that 
//            outputs 4 modalities based on noise level, light, proximity, and motion 

#include <PDM.h> //audio activity 
#include <Arduino_APDS9960.h> //light & proximity 
#include <Arduino_BMI270_BMM150.h> //motion 
#include <math.h> 

short sampleBuffer[256];
volatile int samplesRead = 0;

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}


void setup() {
  Serial.begin(115200);
  delay(1500);
  
  PDM.onReceive(onPDMdata);

  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM microphone."); //mic
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor."); //proximity & color
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed ot initialize IMU."); //motion 
    while (1);
  }



  Serial.println("started");
}

void loop() {
  static int r, g, b, c; 
  static int level;
  static int proximity; 
  static float x, y, z; 
  static float a_norm; 
  static int sound_threshold = 20;
  static int dark_threshold = 10; 
  static float moving_threshold = 1; 
  static int near_threshold = 50; //thresholds

  if (samplesRead) {
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    level = sum / samplesRead;
    samplesRead = 0;
  }

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    a_norm = sqrt(x*x + y*y +(z-1)*(z-1)); 
  }

  if(APDS.colorAvailable()) {
    APDS.readColor(r,g,b,c); 
  }

  if (APDS.proximityAvailable()) {
    proximity = APDS.readProximity();
  }

  int sound, dark, moving, near; 
  if (level > sound_threshold) {
    sound = 1; 
  } else {
    sound = 0; 
  }
  if (c < dark_threshold) {
    dark = 1; 
  } else {
    dark = 0; 
  }
  if (a_norm > moving_threshold) {
    moving = 1; 
  } else {
    moving = 0; 
  }
  if (proximity < near_threshold) {
    near = 1; 
  } else {
    near = 0; 
  }

  const char* label;
  bool matched = true;

  if (!sound && !dark && !moving && !near) {
    label = "QUIET_BRIGHT_STEADY_FAR";
  } else if (sound && !dark && !moving && !near) {
    label = "NOISY_BRIGHT_STEADY_FAR";
  } else if (!sound && dark && !moving && near) {
    label = "QUIET_DARK_STEADY_NEAR";
  } else if (sound && !dark && moving && near) {
    label = "NOISY_BRIGHT_MOVING_NEAR";
  } else {
    matched = false;
  }

  if (matched) {
    Serial.print("raw,mic=");
    Serial.print(level);
    Serial.print(",clear=");
    Serial.print(c);
    Serial.print(",motion=");
    Serial.print(a_norm);
    Serial.print(",prox=");
    Serial.println(proximity);

    Serial.print("flags,sound=");
    Serial.print(sound);
    Serial.print(",dark=");
    Serial.print(dark);
    Serial.print(",moving=");
    Serial.print(moving);
    Serial.print(",near=");
    Serial.println(near);

    Serial.print("state,");
    Serial.println(label);
  }

  delay(100);
}



