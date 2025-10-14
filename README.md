# ESP32 Arduino ADC Example with PlatformIO

This project demonstrates how to use the ADC (Analog-to-Digital Converter) on the ESP32 microcontroller using the Arduino framework. The implementation is based on the official ESP32 Arduino ADC documentation, which can be found [here](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html). The main source code is located in `src/main.cpp`, and optional headers are in the `include/` directory.

## Prerequisites

- [PlatformIO Core](https://docs.platformio.org/page/core.html) installed
- Visual Studio Code (recommended)
- ESP32 board support installed via PlatformIO

## Project Structure

- `src/`: main source files (`main.cpp`)
- `include/`: header files
- `platformio.ini`: PlatformIO configuration

## Build and Upload

Open a terminal in the project folder and run:

```powershell
# Build the project
pio run

# Upload firmware to ESP32
pio run --target upload

# Clean build files
pio run --target clean
```

## Resources

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [ESP32 Arduino Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)