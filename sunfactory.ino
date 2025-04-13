// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON // must be before "#include <heltec_unofficial.h>"

#include <heltec_unofficial.h>
#include <SPIFFS.h>
#include <WebServer.h>
// #include <WiFi.h>
#include <time.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

// ###################### LORA - START ######################

// Pause between transmited packets in seconds.
// Set to zero to only transmit a packet when pressing the user button
// Will not exceed 1% duty cycle, even if you set a lower value.
#define PAUSE 300

// Frequency in MHz. Keep the decimal point to designate float.
// Check your own rules and regulations to see what is legal where you are.
#define FREQUENCY 866.3 // for Europe
// #define FREQUENCY           905.2       // for US

// LoRa bandwidth. Keep the decimal point to designate float.
// Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
#define BANDWIDTH 250.0

// Number from 5 to 12. Higher means slower but higher "processor gain",
// meaning (in nutshell) longer range and more robust against interference.
#define SPREADING_FACTOR 9

// Transmit power in dBm. 0 dBm = 1 mW, enough for tabletop-testing. This value can be
// set anywhere between -9 dBm (0.125 mW) to 22 dBm (158 mW). Note that the maximum ERP
// (which is what your antenna maximally radiates) on the EU ISM band is 25 mW, and that
// transmissting without an antenna can damage your hardware.
#define TRANSMIT_POWER 14

int16_t sensor1 = 0, sensor2 = 0, sensor3 = 0, sensor4 = 0, sensor5 = 0, sensor6 = 0;

String txData;
String rxData;
volatile bool rxFlag = false;
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
uint64_t minimum_pause;

// Example: Assign parsed values to variables
String receivedNodeId = "";
String receivedMsgId = "";
bool receivedConnected = false;
int16_t receivedSensor1 = 0;
int16_t receivedSensor2 = 0;
int16_t receivedSensor3 = 0;
int16_t receivedSensor4 = 0;
int16_t receivedSensor5 = 0;
int16_t receivedSensor6 = 0;

// ####################### LORA - END ############################

#define LOG_INTERVAL_MS 1000 // 60000 milliseconds = 1 minute

// For wifi connection
#define WIFI_TIMEOUT_MS 10000 // 10000 milliseconds = 10 seconds

#define WIFI_SSID "sunfactory"
#define WIFI_PASS "sunfactory"

const char *filename = "/data.csv"; // File path in SPIFFS

char buffer[64]; // Buffer to store formatted string

WiFiManager wm;

WebServer server(8080); // Create a web server on port 80

// NTP server settings
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;     // Adjust this according to your timezone
const int daylightOffset_sec = 0; // Adjust this according to your day light saving time

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

float targetTemperatures[] = {180, 200, 210, 220, 250, 25, 70, 90, 100};
int targetIndex = 0;
float targetTemperature = targetTemperatures[targetIndex];

bool connected = false;

float thermistorTemperatureA = 0;
float thermistorTemperatureB = 0;
float lux = 0;
float fileSize = 0;
int availableMemory = 99999;     // to make sure it's not 0
float availablePercentage = 100; // start with 100 to make sure
char timestamp[20];

bool enableBeep = true;

unsigned long lastSaveTime = 0;

String nodeId = "";

void setup()
{
  heltec_setup();

  // ###################### LORA ######################
  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  // Set the callback function for received packets
  radio.setDio1Action(rx);
  // Set radio parameters
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  // Start receiving
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  nodeId = getNodeId();
  Serial.print("nodeId: ");
  Serial.println(nodeId);
  delay(5000);

  // ###################################################

  // display.resetOrientation();

  pinMode(BUZZER_PIN, OUTPUT); // Buzzer

  Serial.println("Serial works");

  if (!SPIFFS.begin(true))
  {
    return;
  }
  delay(1000);

  if (SPIFFS.exists(filename))
  {
    Serial.println("File exists");
    // File file = SPIFFS.open(filename, FILE_APPEND);
  }
  else
  {
    Serial.println("File does not exist, creating new file with headers");
    File file = SPIFFS.open(filename, FILE_WRITE);
    delay(1000);
    if (file)
    {
      file.println("Timestamp, Temperature A, Temperature B, Lux"); // CSV headers
      file.close();
    }
  }
  delay(1000);

  // Display
  display.println("Display works");

  // Battery
  float vbat = heltec_vbat();
  display.printf("Vbat: %.2fV (%d%%)\n", vbat, heltec_battery_percent(vbat));

  yield();

  handleWifi();

  delay(1000);

  startHttpServer();
}

void loop()
{
  yield();
  heltec_loop();
  yield();

  // keep AP Portal Running
  wm.process();
  server.handleClient(); // Handle client requests

  // Serial.println("Connected: " + String(connected));

  if (!SPIFFS.exists(filename))
  {
    Serial.println("File does not exist, creating new file with headers");
    display.println("log wiped!");
    display.println("creating file");
    File file = SPIFFS.open(filename, FILE_WRITE);
    delay(1000);
    if (file)
    {
      file.println("Timestamp, Temperature A, Temperature B, Lux"); // CSV headers
      file.close();
    }
    delay(1000);
  }
  // Button
  targetTemperature = targetTemperatures[targetIndex];
  yield();

  if (button.isSingleClick())
  {
    Serial.println("Button pressed, changing target temperature...");
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

  yield();
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
  lux = pow((R10_LUX / ldrResistance), (1.0 / GAMMA));
  yield();
  // Read and calculate temperature for Thermistor A
  int adcValueA = analogRead(THERMISTOR_PIN_A);
  float voltageA = (adcValueA / ADC_RESOLUTION) * SUPPLY_VOLTAGE;
  float resistanceA = SERIES_RESISTOR * ((SUPPLY_VOLTAGE / voltageA) - 1);
  thermistorTemperatureA = resistanceA / NOMINAL_RESISTANCE;      // (R/Ro)
  thermistorTemperatureA = log(thermistorTemperatureA);           // ln(R/Ro)
  thermistorTemperatureA /= B_COEFFICIENT;                        // 1/B * ln(R/Ro)
  thermistorTemperatureA += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
  thermistorTemperatureA = 1.0 / thermistorTemperatureA;          // Invert
  thermistorTemperatureA -= 273.15;

  // Read and calculate temperature for Thermistor B
  int adcValueB = analogRead(THERMISTOR_PIN_B);
  float voltageB = (adcValueB / ADC_RESOLUTION) * SUPPLY_VOLTAGE;
  float resistanceB = SERIES_RESISTOR * ((SUPPLY_VOLTAGE / voltageB) - 1);
  thermistorTemperatureB = resistanceB / NOMINAL_RESISTANCE;      // (R/Ro)
  thermistorTemperatureB = log(thermistorTemperatureB);           // ln(R/Ro)
  thermistorTemperatureB /= B_COEFFICIENT;                        // 1/B * ln(R/Ro)
  thermistorTemperatureB += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
  thermistorTemperatureB = 1.0 / thermistorTemperatureB;          // Invert
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

  yield();
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
  if (enableBeep == true && (thermistorTemperatureA >= targetTemperature || thermistorTemperatureB >= targetTemperature))
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

  // Indicate that the memory is almost full
  if (availablePercentage < 20)
  {
    heltec_led(100);
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringf(3, 0, buffer, "BACKUP!");
    display.drawStringf(63, 0, buffer, "%.0f%%", availablePercentage);
  }
  else
  {
    heltec_led(0);
  }

  yield();
  display.display();

  unsigned long currentTime = millis();
  yield();

  if ((currentTime - lastSaveTime >= LOG_INTERVAL_MS) || lastSaveTime == 0) // 60000 milliseconds = 1 minute
  {
    lastSaveTime = currentTime;
    yield();

    // Generate a simple timestamp and dummy value
    snprintf(timestamp, sizeof(timestamp), "%lu", millis() / 1000);

    if (connected)
    {
      // Get current time from WiFi network
      time_t now;
      struct tm timeinfo;
      if (getLocalTime(&timeinfo))
      {
        time(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
      }
    }

    // Serial.println(timestamp);

    yield();
    File file = SPIFFS.open(filename, FILE_APPEND);
    yield();

    if (!file)
    {
      return;
    }

    try
    {
      // Write data to file
      file.printf("%s, %d, %d, %d,\n", timestamp, (int)thermistorTemperatureA, (int)thermistorTemperatureB, (int)lux);
      fileSize = file.size();
      // Send data via Lora
      buildLoraPackage();
      handleLoraTx();
    }
    catch (const std::exception &e)
    {
      Serial.print("Error writing to file: ");
      Serial.println(e.what());
    }
    file.close();

    availableMemory = SPIFFS.totalBytes() - SPIFFS.usedBytes();
    availablePercentage = (float)availableMemory / SPIFFS.totalBytes() * 100;
    // Serial.printf("File size: %.2f Kbytes\n", (int)fileSize / 1024);
    // Serial.printf("Available memory: %.2f Kbytes (%.2f%%)\n", availableMemory / 1024, availablePercentage);

    yield();
  }
  yield();

  // wipe file if memory is less than 10% to keep system running
  if (availablePercentage < 10)
  {
    SPIFFS.remove(filename);
    fileSize = 0;
  }

  sensor1 = (int)thermistorTemperatureA;
  sensor2 = (int)thermistorTemperatureB;
  sensor3 = (int)lux;

  handleLoraRx();

  heltec_delay(100);
}

String getNodeId()
{
  uint64_t chipId = ESP.getEfuseMac(); // Get unique chip ID
  Serial.print("Chip ID: ");
  Serial.println(chipId, HEX);

  uint16_t shortNodeId = (chipId & 0xFFFF) ^ ((chipId >> 16) & 0xFFFF) ^ ((chipId >> 32) & 0xFFFF) ^ ((chipId >> 48) & 0xFFFF);
  char nodeIdStr[5];
  snprintf(nodeIdStr, sizeof(nodeIdStr), "%04X", shortNodeId);
  return String(nodeIdStr);
}

void handleWifi()
{

  wm.setTitle("");

  // Create a menu list
  std::vector<const char *> menu = {"wifi", "exit"}; // You can add/remove these

  // Apply the custom menu
  wm.setMenu(menu);

  //
  // WiFi.mode(WIFI_STA);

  // wm.setTimeout(20);                 // 3 minutes before timeout on trying to automatically connect
  wm.setConfigPortalBlocking(false); // Set to false to allow non-blocking mode

  // wm.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
  wm.setCustomHeadElement("<style>.custom-btn{padding:10px 20px;background:#ffcc00;color:white;border:none;border-radius:5px;text-decoration:none;display:inline-block;}</style><a href='http://192.168.4.1:8080/raw' class='custom-btn'>Raw Data</a><a href='http://192.168.4.1:8080/' class='custom-btn'>Dashboard</a>");
  // wm.setCustomHeadElement("<button onclick=\"alert('Popup shown!')\">SHOW POPUP</button>");
  // wm.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
  // WiFiManagerParameter custom_text("<p>This is just a text paragraph</p>");
  // wm.addParameter(&custom_text);

  // Give user the option to press button to start WIFI AP Mode before timeout
  unsigned long startAttemptTime = millis();
  while (millis() - startAttemptTime < WIFI_TIMEOUT_MS)
  {

    yield();
    heltec_loop();
    yield();

    int secondsLeft = (WIFI_TIMEOUT_MS - (millis() - startAttemptTime)) / 1000;
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(4, 4, "Press button to ");
    display.drawString(4, 24, "Create Wifi AP");
    display.drawStringf(4, 44, buffer, "Timeout in: %d s", secondsLeft);
    display.display();

    if (button.isSingleClick())
    {
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

      Serial.println("Button pressed, starting AP mode...");
      // Show AP mode on display
      display.clear();
      display.setFont(ArialMT_Plain_16);
      display.drawStringf(4, 4, buffer, "%s_%s", WIFI_SSID, nodeId.c_str());
      display.drawStringf(4, 24, buffer, "PASS: %s", WIFI_PASS);
      display.drawStringf(4, 44, buffer, "IP: %s", "192.168.4.1");
      display.display();

      // Start the access point
      wm.startConfigPortal((String(WIFI_SSID) + "_" + nodeId).c_str(), WIFI_PASS);
      Serial.println("AP mode started");
    }
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    wm.setConnectTimeout(10); // Set timeout to 10 seconds
    // if user didnt press the button, try to connect to the saved WiFi network
    if (wm.autoConnect((String(WIFI_SSID) + "_" + nodeId).c_str(), WIFI_PASS))
    {
      display.clear();
      display.drawString(0, 0, "  oo WIFI ON oo");
      display.drawStringf(0, 16, buffer, "IP: %s", WiFi.localIP().toString().c_str());
      display.display();
      Serial.println("Connected to WiFi!");
      Serial.println(WiFi.localIP());
      connected = true;
      delay(5000);
    }
    else
    {
      display.clear();
      display.drawString(4, 4, "  xx NO WIFI xx");
      display.display();
      connected = false;
      delay(5000);
    }
  }
  else
  {
    display.clear();
    display.drawString(0, 0, "  oo WIFI ON oo");
    display.drawStringf(0, 16, buffer, "IP: %s", WiFi.localIP().toString().c_str());
    display.display();
    Serial.println("Connected to WiFi!");
    Serial.println(WiFi.localIP());
    connected = true;
    delay(5000);
  }
}

void startHttpServer()
{
  // Start the server
  server.on("/", HTTP_GET, handleRoot);             // Handle root request
  server.on("/download", HTTP_GET, handleDownload); // Handle download request
  server.on("/raw", HTTP_GET, handleGetValues);     // Handle getValues request
  server.on("/wipe", HTTP_GET, handleWipeFile);     // Handle wipeFile request
  server.on("/enable-beep", HTTP_GET, handleEnableBeep);
  server.begin();
  Serial.println("HTTP server started successfully.");

  yield();
  heltec_delay(1000);

  Serial.println("HTTP server started");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.println("Hostname: ");
  Serial.println(WiFi.getHostname());

  // Initialize NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}
// Handle the root URL
void handleRoot()
{
  String html = "<html><head><meta http-equiv='refresh' content='" + String(10) + "'></head>"
                                                                                  "<body style='background-color:#ffcc00; text-align:center;font-family:Arial;'><h1>SUNFACTORY</h1>"
                                                                                  "<table border='1' style='margin-left:auto; margin-right:auto; text-align:center;'>"
                                                                                  "<tr><th>Timestamp</th><th>Temperature A</th><th>Temperature B</th><th>Lux</th></tr>";
  if (timestamp == nullptr || strlen(timestamp) == 0)
  {
    html += "<tr><td><a href='/'wait</a></td><td>" + String((int)thermistorTemperatureA) + " C</td><td>" + String((int)thermistorTemperatureB) + " C</td><td>" + String((int)lux) + " lm</td></tr>";
  }
  else
  {
    html += "<tr><td>" + String(timestamp) + " </td><td>" + String((int)thermistorTemperatureA) + " C</td><td>" + String((int)thermistorTemperatureB) + " C</td><td>" + String((int)lux) + " lm</td></tr>";
  }
  html += "</table><p><a href='/download'>Download LOG</a></p>";
  html += "<p>File size: " + String(fileSize / 1024) + " Kbytes</p>";
  html += "<p>Available memory: " + String(availableMemory / 1024) + " Kbytes (" + String(availablePercentage, 2) + "%)</p>";
  html += "<p>Beep " + String(enableBeep ? "Enabled" : "Disabled") + "</p>";
  if (!enableBeep)
  {
    html += "<p><a href='/enable-beep'>Enable Beep</a></p>";
  }
  else
  {
    html += "<p><a href='/disable-beep'>Disable Beep</a></p>";
  }
  html += "<p style='text-align:right;'><a href='/wipe'>Wipe LOG</a></p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// Handle the download URL
void handleDownload()
{
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file)
  {
    server.send(500, "text/plain", "Failed to open file for reading");
    return;
  }

  // Send the file content as a downloadable file
  server.streamFile(file, "text/csv");
  file.close();
}

// Handle the getValues URL
void handleGetValues()
{
  String json = "{";
  json += "\"timestamp\":\"" + String(timestamp) + "\",";
  json += "\"temperatureA\":" + String((int)thermistorTemperatureA) + ",";
  json += "\"temperatureB\":" + String((int)thermistorTemperatureB) + ",";
  json += "\"lux\":" + String((int)lux);
  json += "}";

  server.send(200, "application/json", json);
}

void handleWipeFile()
{
  if (server.hasArg("confirm") && server.arg("confirm") == "yes")
  {
    SPIFFS.remove(filename);
    fileSize = 0;
    server.sendHeader("Location", "/");
    server.send(303);
  }
  else
  {
    String html = "<html><body style='background-color:#ffcc00;'><h1>Confirm Wipe</h1>";
    html += "<p>Are you sure you want to wipe the log file?</p>";
    html += "<a href='/wipe?confirm=yes'>Yes</a> | <a href='/'>No</a>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  }
}

void handleEnableBeep()
{
  enableBeep = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDisableBeep()
{
  enableBeep = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLoraTx()
{
  radio.transmit(txData.c_str());
  // Serial.print("Raw Lora Data: ");
  // Serial.println("TX [%s]\n", txData.c_str());

  // both.printf("TX [%s]\n", txData.c_str());
  // Serial.print("Bytes: ");
  // Serial.println(strlen(txData.c_str()));

  heltec_led(50); // 50% brightness is plenty for this LED
  tx_time = millis();
  // RADIOLIB(radio.transmit(String(counter++).c_str()));
  tx_time = millis() - tx_time;
  heltec_led(0);
  if (_radiolib_status == RADIOLIB_ERR_NONE)
  {
    yield();
    // both.printf("OK (%i ms)\n", (int)tx_time);
  }
  else
  {
    both.printf("fail (%i)\n", _radiolib_status);
  }

  radio.clearDio1Action();

  // Maximum 1% duty cycle
  minimum_pause = tx_time * 100;
  last_tx = millis();
  radio.setDio1Action(rx);
  // RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
}

void handleLoraRx()
{
  // If a packet was received, display it and the RSSI and SNR
  if (rxFlag)
  {
    rxFlag = false;
    radio.readData(rxData);

    if (_radiolib_status == RADIOLIB_ERR_NONE && rxData.length())
    {
      parseLoraPackage();

      // Filter out own packages sent via lora net
      bool ownPackage = receivedNodeId.equals(nodeId);
      if (!ownPackage)
      {
        // Retransmit the received package
        both.printf("RX [%s]\n", rxData.c_str());
        txData = rxData;
        handleLoraTx();
      }
      // else
      // {
      //   Serial.println("Own Package");
      //   both.printf("RX own [%s]\n", rxData.c_str());
      // }

      // both.printf("RX [%s]\n", rxData.c_str());
      // heltec_delay(1000);
      // both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
      // both.printf("  SNR: %.2f dB\n", radio.getSNR());
      // Serial.print("Received message: ");
      // Serial.println(rxData.c_str());
      // Serial.print("Bytes: ");
      // Serial.println(strlen(rxData.c_str()));
    }

    // RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
    radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  }
}

// Can't do Serial or display things here, takes too much time for the interrupt
void rx()
{
  rxFlag = true;
}

void buildLoraPackage()
{
  // TODO: wrap numerical values into a pair of bytes to save string space (12-04-2025 - EVERTON)

  // Generate a random 8-byte UUID
  uint64_t uuid = esp_random();
  // Serial.println(uuid, HEX);
  char uuidStr[5];
  snprintf(uuidStr, sizeof(uuidStr), "%04llX", uuid);
  String msgId = String(uuidStr);

  // Default Package Format (expected less than 50 bytes)
  //  nodeId,msgId,connected,sensor1,sensor2,sensor3,sensor4,sensor5,sensor6
  // Build the package to send
  String package = nodeId + "," + String(msgId) + "," + String(connected) + "," + String(sensor1) + "," + String(sensor2) + "," + String(sensor3) + "," + String(sensor4) + "," + String(sensor5) + "," + String(sensor6);

  Serial.println(package);

  txData = package;
}

void parseLoraPackage()
{

  // Default Package Format (expected less than 50 bytes)
  //  nodeId,msgId,connected,sensor1,sensor2,sensor3,sensor4,sensor5,sensor6

  // Split rxData by commas
  int index = 0;
  String parsedData = rxData;
  String tokens[9]; // Assuming there are 6 fields in the package
  while (parsedData.length() > 0 && index < 9)
  {
    int delimiterIndex = parsedData.indexOf(',');
    if (delimiterIndex == -1)
    {
      tokens[index++] = parsedData; // Last token
      break;
    }
    tokens[index++] = parsedData.substring(0, delimiterIndex);
    parsedData = parsedData.substring(delimiterIndex + 1);
  }

  // Example: Assign parsed values to variables
  receivedNodeId = tokens[0];
  receivedMsgId = tokens[1];
  receivedConnected = tokens[2].toInt();
  receivedSensor1 = tokens[3].toInt();
  receivedSensor2 = tokens[4].toInt();
  receivedSensor3 = tokens[5].toInt();
  receivedSensor4 = tokens[6].toInt();
  receivedSensor5 = tokens[7].toInt();
  receivedSensor6 = tokens[8].toInt();
}