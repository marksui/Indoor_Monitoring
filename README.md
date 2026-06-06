# Indoor Monitoring

Indoor Monitoring is an ESP32-based environmental monitor for classrooms, labs, and study spaces. The device reads indoor air and comfort metrics, then shows the live status on a TFT display.

## What It Measures

- CO2 from a Sensirion SCD4x sensor
- Temperature, humidity, pressure, gas resistance, and altitude from a BME680
- PM1.0, PM2.5, PM10, and AQI from an Adafruit PM2.5 sensor
- Sound level from an I2S microphone

## Code Structure

- `Code/src/main.cpp`: main Arduino firmware, sensor polling, AQI calculation, button navigation, and TFT display pages
- `Code/platformio.ini`: PlatformIO board, framework, and library configuration
- `Code/lib/TFT_eSPI/`: bundled TFT display library/configuration used by the firmware
- `doc/`: static project website for GitHub Pages

## Website

Site URL: https://marksui.github.io/Indoor_Monitoring/
