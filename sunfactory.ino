// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON // must be before "#include <heltec_unofficial.h>"

// creates 'radio', 'display' and 'button' instances
#include <heltec_unofficial.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <EEPROM.h>

#define WIFI_SSID "sunfactory"
#define WIFI_PASS "sunfactory"

// Constants for the Thermistors
#define THERMISTOR_PIN_A 7          // Analog pin where the voltage divider is connected for Thermistor A
#define THERMISTOR_PIN_B 6          // Analog pin where the voltage divider is connected for Thermistor B
#define SERIES_RESISTOR 10000.0     // 10k resistor value (in ohms)
#define NOMINAL_RESISTANCE 100000.0 // 100k thermistor at 25C
#define NOMINAL_TEMPERATURE 25.0    // 25°C
#define B_COEFFICIENT 3950.0        // Beta coefficient of thermistor
#define SUPPLY_VOLTAGE 3.3          // ESP8266 supply voltage
#define ADC_RESOLUTION 4095.0       // 12-bit ADC on ESP8266

// Constants from the LDR datasheet
#define LDR_PIN 5      // Analog pin where the voltage divider is connected for LDR
#define R10_LUX 8000.0 // Resistance at 10 Lux (8kΩ - lower bound)
#define GAMMA 0.7      // Gamma value from datasheet

#define BUZZER_PIN 4 // Buzzer

float targetTemperatures[] = {210, 240, 180, 15};
int targetIndex = 0;
float targetTemperature = targetTemperatures[targetIndex];

void setup()
{
  heltec_setup();

  display.resetOrientation();

  pinMode(BUZZER_PIN, OUTPUT); // Buzzer

  Serial.println("Serial works");

  // Display
  display.println("Display works");
  // Radio
  display.print("Radio ");
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE)
  {
    display.println("works");
  }
  else
  {
    display.printf("fail, code: %i\n", state);
  }
  // Battery
  float vbat = heltec_vbat();
  display.printf("Vbat: %.2fV (%d%%)\n", vbat, heltec_battery_percent(vbat));

  RADIOLIB_OR_HALT(radio.setFrequency(866.3));
  heltec_delay(3000);

  // Initialize EEPROM with maximum available size
  EEPROM.begin(EEPROM.length());

  // Read and print saved temperatures and luminosity from EEPROM
  float savedTemperatureA, savedTemperatureB, savedLux;
  EEPROM.get(0, savedTemperatureA);
  EEPROM.get(sizeof(float), savedTemperatureB);
  EEPROM.get(2 * sizeof(float), savedLux);

  Serial.println("Saved Data:");
  Serial.println("Temp A: ");
  Serial.print(savedTemperatureA);
  Serial.println("Temp B: ");
  Serial.print(savedTemperatureB);
  Serial.println("Lux: ");
  Serial.print(savedLux);

  // Clear EEPROM
  for (int i = 0; i < EEPROM.length(); i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.commit(); // Commit changes to EEPROM
  Serial.println("EEPROM cleared");

  // Reinitialize EEPROM with maximum available size
  EEPROM.begin(EEPROM.length());

  WiFiManager wm;
  // wm.setTimeout(30); // 30 secon
  // wm.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  heltec_delay(3000);

  char buffer[64]; // Buffer to store formatted string
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawStringf(4, 4, buffer, "ssid: ");
  display.drawStringf(50, 4, buffer, WIFI_SSID);
  display.drawStringf(4, 24, buffer, "pass: ");
  display.drawStringf(50, 24, buffer, WIFI_PASS);
  display.display();
  heltec_delay(3000);

  // // Try connecting to saved WiFi, or start AP for setup
  // if (!wm.autoConnect(WIFI_SSID, WIFI_PASS))
  // {
  //   Serial.println("Failed to connect and hit timeout");
  //   ESP.restart();
  // }

  // Serial.println("Connected to WiFi!");
  // Serial.println(WiFi.localIP());
  // // display.drawStringf(4, 44, buffer, "IP: ");
  // // display.drawStringf(50, 44, buffer, WiFi.localIP().toString().c_str());
  // delay(1000); // Allow time for serial monitor to open
}

void loop()
{
  heltec_loop();

  // Button
  targetTemperature = targetTemperatures[targetIndex];

  if (button.isSingleClick())
  {
    targetIndex = (targetIndex + 1) % (sizeof(targetTemperatures) / sizeof(targetTemperatures[0])); // Cycle through targetTemperatures array

    // LED
    for (int n = 0; n <= 10; n++)
    {
      heltec_led(n);
      delay(5);
    }
    for (int n = 10; n >= 0; n--)
    {
      heltec_led(n);
      delay(5);
    }
    // display.println("LED works");
  }

  float ldrValue = analogRead(LDR_PIN);

  // Convert to voltage
  float ldrVoltage = (ldrValue / ADC_RESOLUTION) * SUPPLY_VOLTAGE;

  // Avoid division by zero
  if (ldrVoltage == 0)
  {
    // Serial.println("LDR voltage is 0V, possibly no light detected.");
    return;
  }

  // Calculate LDR resistance
  float ldrResistance = (SUPPLY_VOLTAGE / ldrVoltage - 1) * SERIES_RESISTOR;

  // Convert Resistance to Lux using the gamma formula
  float lux = 10.0 * pow((R10_LUX / ldrResistance), (1.0 / GAMMA));

  // // Print values
  // Serial.print("LDR Value: ");
  // Serial.println(ldrValue);
  // Serial.print("LDR Voltage: ");
  // Serial.println(ldrVoltage);
  // Serial.print("LDR Resistance: ");
  // Serial.println(ldrResistance);
  // Serial.print("Estimated Lux: ");
  // Serial.println(lux);

  // Read and calculate temperature for Thermistor A
  int adcValueA = analogRead(THERMISTOR_PIN_A);
  float voltageA = (adcValueA / ADC_RESOLUTION) * SUPPLY_VOLTAGE;
  float resistanceA = SERIES_RESISTOR * ((SUPPLY_VOLTAGE / voltageA) - 1);
  float thermistorTemperatureA = resistanceA / NOMINAL_RESISTANCE; // (R/Ro)
  thermistorTemperatureA = log(thermistorTemperatureA);            // ln(R/Ro)
  thermistorTemperatureA /= B_COEFFICIENT;                         // 1/B * ln(R/Ro)
  thermistorTemperatureA += 1.0 / (NOMINAL_TEMPERATURE + 273.15);  // + (1/To)
  thermistorTemperatureA = 1.0 / thermistorTemperatureA;           // Invert
  thermistorTemperatureA -= 273.15;

  // Read and calculate temperature for Thermistor B
  int adcValueB = analogRead(THERMISTOR_PIN_B);
  float voltageB = (adcValueB / ADC_RESOLUTION) * SUPPLY_VOLTAGE;
  float resistanceB = SERIES_RESISTOR * ((SUPPLY_VOLTAGE / voltageB) - 1);
  float thermistorTemperatureB = resistanceB / NOMINAL_RESISTANCE; // (R/Ro)
  thermistorTemperatureB = log(thermistorTemperatureB);            // ln(R/Ro)
  thermistorTemperatureB /= B_COEFFICIENT;                         // 1/B * ln(R/Ro)
  thermistorTemperatureB += 1.0 / (NOMINAL_TEMPERATURE + 273.15);  // + (1/To)
  thermistorTemperatureB = 1.0 / thermistorTemperatureB;           // Invert
  thermistorTemperatureB -= 273.15;

  display.clear();
  char buffer[64]; // Buffer to store formatted string

  // print layout structures
  display.drawHorizontalLine(0, 32, 128);
  display.drawVerticalLine(84, 0, 64);
  display.drawRect(0, 0, 128, 64);

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawStringf(87, 7, buffer, "%.0fC", targetTemperature);

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  if (thermistorTemperatureA < -100 || thermistorTemperatureA > 400)
  {
    display.drawStringf(2, 4, buffer, "A:-------");
  }
  else
  {
    display.drawStringf(2, 4, buffer, "A:%.0fC", thermistorTemperatureA);
  }

  // print temperature readings for sensor B
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  if (thermistorTemperatureB < -100 || thermistorTemperatureB > 400)
  {
    display.drawStringf(2, 34, buffer, "B:-------");
  }
  else
  {
    display.drawStringf(2, 34, buffer, "B:%.0fC", thermistorTemperatureB);
  }

  // print OK if temperature above target temperature
  if (thermistorTemperatureA >= targetTemperature || thermistorTemperatureB >= targetTemperature)
  {
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawStringf(106, 34, buffer, "OK");
    heltec_led(100);

    digitalWrite(BUZZER_PIN, HIGH); // Turn on the buzzer
    heltec_delay(200);
    digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer
    heltec_delay(200);
  }
  else
  {
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawStringf(106, 32, buffer, "%.0f", lux);
    display.drawStringf(106, 46, buffer, "lm");
    heltec_led(0);
  }

  display.display();

  static unsigned long lastSaveTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastSaveTime >= 1000) // 60000 milliseconds = 1 minute
  {
    lastSaveTime = currentTime;

    // Save thermistorTemperatureA
    EEPROM.put(0, thermistorTemperatureA);

    // Save thermistorTemperatureB
    EEPROM.put(sizeof(float), thermistorTemperatureB);

    // Save lux
    EEPROM.put(2 * sizeof(float), lux);

    EEPROM.commit(); // Commit changes to EEPROM
    EEPROM.end();    // End EEPROM access

    Serial.println("Saved temperatures and luminosity to EEPROM");
  }

  heltec_delay(100);
}
