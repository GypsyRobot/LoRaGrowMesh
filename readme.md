# SolarMeshSense

SolarMeshSense is an open-source LoRa-based mesh network for environmental monitoring, designed for agroforestry, energy research, and off-grid communities. Originally built to measure temperature and light for a solar concentrator, it has evolved into a scalable system that collects and shares data from multiple sensors over long distances. With Wi-Fi hotspots, HTTP servers, and flash memory storage, it supports real-time data access and integration with databases like PostgreSQL for visualization with tools like Grafana.

Learn more about the original solar application: [Sun Factory](https://www.gp-award.com/index.php/en/produkte/sun-factory)

Based on the [heltec_esp32_lora_v3](https://github.com/ropg/heltec_esp32_lora_v3) library for Heltec ESP32 LoRa V3 boards.

## Features

- **LoRa Mesh Network**: Nodes communicate via LoRa (868/915 MHz) to relay data over long distances.
- **Multi-Sensor Support**: Up to six sensors per node (e.g., DS18B20 for temperature, BME280 for humidity, BH1750 for light).
- **Wi-Fi Hotspot**: Nodes provide Wi-Fi access points for local data viewing via an HTML interface.
- **HTTP Server & API**: Real-time data access and API endpoints for sensor measurements.
- **Flash Memory**: Stores data locally with overflow handling and manual clear options.
- **Buzzer Alerts**: Configurable temperature threshold alerts, toggleable via the web interface.
- **Unique Node IDs**: Uses MAC addresses as LoRa SSIDs to prevent conflicts.
- **Data Replication**: LoRa messages are replicated across nodes and stored with sender IDs.
- **Power Autonomy**: Supports LiPo batteries or 6V solar panels for off-grid use.

## Applications

- **Agroforestry**: Monitor temperature, humidity, and light to optimize planting or study microclimates.
- **Energy Research**: Measure performance of solar concentrators, Peltier modules, or other systems.
- **Environmental Monitoring**: Track ecological interactions near rivers, wells, or other resources.
- **Off-Grid Communities**: Enable autonomous monitoring in remote areas.

## Hardware Requirements

- **Module**: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262 LoRa) or compatible ESP32 LoRa boards.
- **Sensors** (up to 6 per node):
  - DS18B20 (temperature)
  - BME280 (temperature, humidity, pressure)
  - BH1750 (light)
  - Others (configurable via firmware)
- **Power**: LiPo battery or 6V solar panel with regulator.
- **Optional**: Wi-Fi router and server (e.g., Raspberry Pi with Ubuntu) for data streaming.
- **Tools**: Smartphone or computer for setup and data access.

References:
- [Heltec WiFi LoRa 32 V3 Datasheet](https://resource.heltec.cn/download/WiFi_LoRa_32_V3/HTIT-WB32LA_V3.2.pdf)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)

## Installation

1. **Assemble**:
   - Connect sensors to I2C/SPI pins on the Heltec ESP32 LoRa V3. See schematics in the [repository](https://github.com/GypsyRobot/LoRaGrowMesh).

2. **Program**:
   - Download the firmware from [GitHub](https://github.com/GypsyRobot/LoRaGrowMesh).
   - Flash it using the Arduino IDE.

3. **Configure Sensors**:
   - Edit the firmware to specify sensor types and pins (e.g., DS18B20, BME280). Check the README for details.

4. **Set Up Network**:
   - Assign a unique node ID (based on MAC address).
   - Configure LoRa frequency (e.g., 868 MHz for Europe, 915 MHz for USA) per local regulations.

5. **Power**:
   - Connect a LiPo battery or solar panel with regulator.

6. **Test**:
   - Connect to the node’s Wi-Fi hotspot (SSID/password in docs).
   - Access the HTML interface (e.g., http://192.168.4.1) to verify sensor data.

## Roadmap

### Done
- Dual temperature monitoring (DS18B20)
- Light measurement (BH1750)
- Buzzer for temperature alerts
- Wi-Fi hotspot and HTTP server
- API for temperature data
- Flash memory with overflow handling
- LoRa send/receive with MAC-based SSIDs
- Message replication and storage with sender IDs
- Manual flash clear
- Web-based buzzer toggle

### In Progress
- Test SunFactory mesh LoRa protocol
- Add PUT requests for server database storage
- Support DHT sensors (e.g., DHT22)
- Enable generic hardware modules
- Improve lux measurement accuracy
- Stress-test flash memory overflow

### Planned
- Add Telegram integration for alerts
- Refactor code for modularity
- Optimize LoRa packets with fixed-length bytes
- Expand documentation and tutorials

## Contributing

We welcome contributions! To get started:
1. Fork the repository: [https://github.com/GypsyRobot/LoRaGrowMesh](https://github.com/GypsyRobot/LoRaGrowMesh).
2. Report bugs or suggest features via issues.
3. Submit pull requests with clear change descriptions.
4. Help improve the SunFactory mesh protocol or add sensor support.

See the [Code of Conduct](CODE_OF_CONDUCT.md) and [Contributing Guidelines](CONTRIBUTING.md).

## Acknowledgments

- **Everton Ramires** for firmware development ([GitHub](https://github.com/GypsyRobot/LoRaGrowMesh)).
- **Heltec Automation** for ESP32 LoRa V3 support.
- **ropg** for the [heltec_esp32_lora_v3](https://github.com/ropg/heltec_esp32_lora_v3) library.
- Inspired by the [Sun Factory](https://www.gp-award.com/index.php/en/produkte/sun-factory).

## License

MIT License. See [LICENSE](LICENSE) for details.
