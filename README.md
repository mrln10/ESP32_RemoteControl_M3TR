# ESP32 Remote Control M3TR

This project implements a **TCP/IP-based remote control system** for an external device (e.g. a radio) using an **ESP32-ETH01** Ethernet board.  
User interaction is handled via a **SPI TFT display**, **two navigation buttons**, and a **rotary encoder with push-button function**.

The software follows a **modular PlatformIO library architecture**, clearly separating hardware abstraction, user interface logic, and network communication.

---

## Features

- Ethernet-based TCP/IP communication (ESP32-ETH01)
- Modular PlatformIO library structure
- Graphical user interface on SPI TFT display
- User input via:
  - Rotary encoder (rotation + push button)
  - Two dedicated navigation buttons
- Centralized hardware and pin configuration
- Easily extendable and maintainable codebase

---

## Project Structure

```text
ESP32_REMOTECONTROL_M3TR
├── platformio.ini
├── src/
│   ├── main.cpp
│   ├── gui_config.cpp
│   └── radio_config.cpp
│
├── include/
│   ├── config.h          # Central pin & hardware configuration
│   ├── gui_config.h
│   ├── radio_config.h
│   └── README
│
├── lib/
│   ├── GUI/
│   │   ├── GUI.cpp
│   │   ├── GUI.h
│   │   ├── library.json
│   │   └── README.md
│   │
│   ├── NavButtons/
│   │   ├── NavButtons.cpp
│   │   ├── NavButtons.h
│   │   ├── library.json
│   │   └── README.md
│   │
│   ├── RadioTCP/
│   │   ├── RadioTCP.cpp
│   │   ├── RadioTCP.h
│   │   └── library.json
│   │
│   ├── RotaryEncoder/
│   │   ├── RotaryEncoder.cpp
│   │   ├── RotaryEncoder.h
│   │   ├── library.json
│   │   └── README.md
│   │
│   └── TFTDisplay/
│       ├── TFTDisplay.cpp
│       ├── TFTDisplay.h
│       ├── library.json
│       └── README.md
│
└── README.md


## Required Hardware
Component	Description
ESP32-ETH01	ESP32 board with Ethernet PHY
SPI TFT Display	Color display with SPI interface
Rotary Encoder	Incremental encoder with push button
2× Push Buttons	Navigation buttons
USB-to-TTL Adapter	Required to flash the ESP32-ETH01
Power Supply	3.3 V / 5 V depending on setup

## Pin Assignment

All pin assignments are centrally defined in:

include/config.h


➡️ Hardware pin changes must only be done in this file to keep the project consistent and maintainable.

## Configuration Concept

config.h
→ Hardware pins and general configuration

gui_config.*
→ UI layout and display behavior

radio_config.*
→ Network and device-specific parameters

## Project Status

🟡 Work in Progress

Core architecture implemented

Hardware interfaces under active development

GUI and TCP command handling continuously extended

## License

This project is currently not licensed.
A license (e.g. MIT or GPL) may be added later.