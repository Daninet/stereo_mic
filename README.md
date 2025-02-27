# USB Stereo Microphone with RP2040 Zero and INMP441 MEMS module

3D case: https://www.thingiverse.com/thing:6962088

![main](images/main.jpg?raw=true "main")

## Pins

|     | RP2040 Zero | Left INMP441 | Right INMP441 |
| --- | ----------- | ------------ | ------------- |
| VDD | 3V3         | VDD          | VDD           |
| GND | GND         | GND          | GND           |
| SD  | GP07        | SD           | SD            |
| L/R | -           | GND          | 3V3           |
| WS  | GP09        | WS           | WS            |
| SCK | GP08        | SCK          | SCK           |

## RP2040 SDK patch needed

- https://github.com/hathach/tinyusb/pull/2937
