// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON // must be before "#include <heltec_unofficial.h>"

// Uncomment this if you have Wireless Stick v3
// #define HELTEC_WIRELESS_STICK

// creates 'radio', 'display' and 'button' instances
#include <heltec_unofficial.h>

#define THERMISTOR_PIN_A 7          // Analog pin where the voltage divider is connected for Thermistor A
#define THERMISTOR_PIN_B 6          // Analog pin where the voltage divider is connected for Thermistor B
#define SERIES_RESISTOR 10000.0     // 10k resistor value (in ohms)
#define NOMINAL_RESISTANCE 100000.0 // 100k thermistor at 25C
#define NOMINAL_TEMPERATURE 25.0    // 25°C
#define B_COEFFICIENT 3950.0        // Beta coefficient of thermistor
#define SUPPLY_VOLTAGE 3.3          // ESP8266 supply voltage
#define ADC_RESOLUTION 4095.0       // 12-bit ADC on ESP8266

// Constants from the LDR datasheet
#define R10_LUX 8000.0 // Resistance at 10 Lux (8kΩ - lower bound)
#define GAMMA 0.7      // Gamma value from datasheet

#define BUZZER_PIN 4 // Buzzer

#define LDR_PIN 5 // Analog pin where the voltage divider is connected for LDR

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
    Serial.println("LDR voltage is 0V, possibly no light detected.");
    return;
  }

  // Calculate LDR resistance
  float ldrResistance = (SUPPLY_VOLTAGE / ldrVoltage - 1) * SERIES_RESISTOR;

  // Convert Resistance to Lux using the gamma formula
  float lux = 10.0 * pow((R10_LUX / ldrResistance), (1.0 / GAMMA));

  // Print values
  Serial.print("LDR Value: ");
  Serial.println(ldrValue);
  Serial.print("LDR Voltage: ");
  Serial.println(ldrVoltage);
  Serial.print("LDR Resistance: ");
  Serial.println(ldrResistance);
  Serial.print("Estimated Lux: ");
  Serial.println(lux);

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

  heltec_delay(100);
}
