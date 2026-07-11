//Filename:Lab2Task11.ino
//Author: Tommy Zhao
//Date: 07/10/2026
//Description: a rule-based monitoring system for a compact smart device placed in a real indoor environment,
//             outputs 4 modalities based on humidity, temprature, magentic disturbance, and ambient color. 

#include <Arduino_HS300x.h> //humidity & temp
#include <Arduino_APDS9960.h> //light
#include <Arduino_BMI270_BMM150.h> //motion & magnetic
#include <math.h> 

void setup() {
  Serial.begin(115200);
  delay(1500);

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize HS300x sensor."); //humidity&temp
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor."); //light
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed ot initialize IMU."); //motion &magnetic 
    while (1);
  }

  Serial.println("started");
}

void loop() {
  static int r, g, b, c;
  float humidity, temp;
  static float x, y, z, magnetic_norm;

  static float rh_prev = 0, temp_prev = 0, mag_prev = 0;
  static float r_prev = 0, g_prev = 0, b_prev = 0, clear_prev = 0;
  static bool first_run = true;

  static const float humid_threshold = 10.0;   // %RH
  static const float temp_threshold  = 1.0;   // °C
  static const float mag_threshold   = 20.0;  // uT
  static const float color_threshold = 2000;  // raw ADC counts
  static const int   COOLDOWN_CYCLES = 4;    // 4 * 500ms = 2s refractory
  static int cooldown[3] = {0, 0, 0};         // 0=breath/warm air, 1=magnetic, 2=light/color

  temp = HS300x.readTemperature();
  humidity = HS300x.readHumidity();

  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, c);
  }

  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(x, y, z);
    magnetic_norm = sqrt(x*x + y*y + z*z);
  }

  float rh_dev    = humidity - rh_prev;
  float temp_dev  = temp - temp_prev;
  float mag_dev   = magnetic_norm - mag_prev;
  float r_dev     = r - r_prev;
  float g_dev     = g - g_prev;
  float b_dev     = b - b_prev;
  float clear_dev = c - clear_prev;

  int humid_jump = 0, temp_rise = 0, mag_shift = 0, light_or_color_change = 0;

  if (!first_run) {
    humid_jump = (rh_dev > humid_threshold) ? 1 : 0;             // directional: rising only
    temp_rise  = (temp_dev > temp_threshold) ? 1 : 0;             // directional: rising only
    mag_shift  = (fabs(mag_dev) > mag_threshold) ? 1 : 0;         // bidirectional
    light_or_color_change =
        (fabs(r_dev) > color_threshold || fabs(g_dev) > color_threshold ||
         fabs(b_dev) > color_threshold || fabs(clear_dev) > color_threshold) ? 1 : 0;
  }
  first_run = false;

  rh_prev = humidity;
  temp_prev = temp;
  mag_prev = magnetic_norm;
  r_prev = r;
  g_prev = g;
  b_prev = b;
  clear_prev = c;

  const char* label;

  if (mag_shift && cooldown[1] == 0) {
    label = "MAGNETIC_DISTURBANCE_EVENT";
    cooldown[1] = COOLDOWN_CYCLES;
  } else if (light_or_color_change && cooldown[2] == 0) {
    label = "LIGHT_OR_COLOR_CHANGE_EVENT";
    cooldown[2] = COOLDOWN_CYCLES;
  } else if ((humid_jump || temp_rise) && cooldown[0] == 0) {
    label = "BREATH_OR_WARM_AIR_EVENT";
    cooldown[0] = COOLDOWN_CYCLES;
  } else {
    label = "BASELINE_NORMAL";
  }

  for (int i = 0; i < 3; i++) {
    if (cooldown[i] > 0) cooldown[i]--;
  }

  Serial.print("raw,rh=");
  Serial.print(humidity);
  Serial.print(",temp=");
  Serial.print(temp);
  Serial.print(",mag=");
  Serial.print(magnetic_norm);
  Serial.print(",r=");
  Serial.print(r);
  Serial.print(",g=");
  Serial.print(g);
  Serial.print(",b=");
  Serial.print(b);
  Serial.print(",clear=");
  Serial.println(c);

  Serial.print("flags,humid_jump=");
  Serial.print(humid_jump);
  Serial.print(",temp_rise=");
  Serial.print(temp_rise);
  Serial.print(",mag_shift=");
  Serial.print(mag_shift);
  Serial.print(",light_or_color_change=");
  Serial.println(light_or_color_change);

  Serial.print("event,");
  Serial.println(label);

  delay(500);
}



