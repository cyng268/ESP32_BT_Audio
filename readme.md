# ESP32 Bluetooth Audio Streaming

This project enables real-time audio streaming from an ESP32 microcontroller with an INMP441 I2S MEMS microphone to a mobile device via Bluetooth. It features a interactive GUI for controlling audio recording and playback.

## Hardware Requirements

- ESP32 Development Board
- INMP441 I2S MEMS Microphone
- Jumper wires

## Pin Connections

| INMP441 | ESP32 |
|---------|-------|
| WS (Word Select) | GPIO 23 |
| SD (Serial Data) | GPIO 21 |
| SCK (Serial Clock) | GPIO 22 |
| L/R | GND |
| VDD | 3.3V |
| GND | GND |

## Software Requirements

- PlatformIO IDE
- DumbDisplay Mobile App (Android)
- Arduino framework for ESP32

## Dependencies

Add these dependencies to your `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps = trevorwslee/DumbDisplay Arduino Library@^0.9.933
            https://github.com/pschatzmann/ESP32-A2DP.git

```

## Features

- Real-time audio streaming via Bluetooth


## Configuration

Key parameters that can be modified:

```cpp
#define BLUETOOTH "ESP32BT"        // Bluetooth device name
#define SoundSampleRate 16000      // Audio sample rate
#define SoundNumChannels 1         // Mono audio
#define StreamBufferNumBytes 256   // Buffer size
#define MaxAmplifyFactor 20        // Maximum amplification
```

## Usage

1. Install the DumbDisplay app on your android device
2. Upload the code to your ESP32
3. Open DumbDisplay and connect to "ESP32BT" via Bluetooth
4. Use the GUI to:
   - Start the microphone and stream audio data real time from esp32 to mobile device

## Troubleshooting

- Ensure Bluetooth is enabled on your mobile device
- Check I2S pin connections
- Verify the ESP32 is receiving power
- Monitor Serial output at 115200 baud for debugging