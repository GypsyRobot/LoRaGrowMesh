#line 1 "/home/everton/sunfactory/readme.md"
Dual temperature monitor with temperature level buzzer and light measurement.

Learn more:
https://www.gp-award.com/index.php/en/produkte/sun-factory

Based on this repository:
https://github.com/ropg/heltec_esp32_lora_v3

References:
https://resource.heltec.cn/download/WiFi_LoRa_32_V3/HTIT-WB32LA_V3.2.pdf
https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf

Roadmap:

- DONE - hello world
- DONE - display
- DONE - temperatures
- DONE - buzzer
- DONE - temperature monitor
- DONE - light
- DONE - button
- DONE - flash memory
- DONE - wifi
- DONE - hotspot
- DONE - show current values on http server
- DONE - api endpoint to see temperature
- DONE - clear flash manualy
- DONE - certify that it handles flash overflow correctly
- DONE - enable or disable beep via webserver
- DONE - lora send and receive
- DONE - lora ssid per module
- DONE - use mac address as ssid
- lora replicate messages and save to database with sender id
- dht
- generic hardware modules
- fine tune lux measurements
- more tests to bulletproof flash memory overflow
- maybe - telegram integration
- refactor
- wrap sensors values into fixed lenght bytes to optimize lora sending reliability
