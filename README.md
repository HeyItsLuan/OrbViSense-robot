# OrbViSense Robot

Open-source robot platform developed as part of the OrbViSense project.

This repository contains the firmware used to control the robot through an ESP32 and a PlayStation 4 controller.

## Overview

The robot is based on an **ESP32 Dev Module** and uses a differential-drive configuration with two powered DC motors and two free/caster wheels for mechanical support.

The current firmware provides wireless control using a **PlayStation 4 (DualShock 4) controller** through Bluetooth.

## Hardware

The current robot configuration includes:

* ESP32 Dev Module
* TB6612 motor driver
* 2 DC motors
* 2 free/caster wheels
* 5 V power supply
* PlayStation 4 (DualShock 4) controller

### Robot configuration

```text
                    Front
                      ↑

              ┌─────────────────┐
      Caster  │                 │  Caster
              │                 │
              │                 │
       Motor  │                 │  Motor
        Left  │      ESP32      │  Right
              └─────────────────┘
                    
```

The two driven motors are controlled through the TB6612 motor driver, while the ESP32 handles the control logic and communication with the PS4 controller.

## Firmware

The firmware is located at:

```text
firmware/
└── arduino/
    └── OrbVIsenseRobot.ino
```

`OrbVIsenseRobot.ino` contains the control program for the robot.

## Dependencies

The firmware requires the following third-party libraries:

### ArduinoJson

* **Version:** 7.4.3
* **Author:** Benoit Blanchon
* **License:** MIT
* **Repository:** https://github.com/bblanchon/ArduinoJson

ArduinoJson is used for JSON data handling in the firmware.

### WebSockets

* **Version:** 2.7.2
* **Author:** Markus Sattler
* **License:** LGPL-2.1
* **Repository:** https://github.com/Links2004/arduinoWebSockets

The WebSockets library provides WebSocket communication for the firmware.

### Bluepad32

* **Author:** Ricardo Quesada
* **License:** Apache-2.0
* **Repository:** https://github.com/ricardoquesada/bluepad32

Bluepad32 provides Bluetooth gamepad support on the ESP32 and allows the robot to be controlled using a PlayStation 4 DualShock 4 controller.

The required third-party libraries are **not included in this repository**. Install them separately before compiling the firmware.

Each dependency remains subject to its respective license and is maintained by its original authors.

## Controller

The robot is controlled using a **PlayStation 4 DualShock 4 controller**.

Bluepad32 provides support for the DualShock 4 over Bluetooth, allowing the controller to communicate directly with the ESP32.

## Motor Control

The ESP32 communicates with the two DC motors through a **TB6612 motor driver**.

The TB6612 provides the interface between the ESP32 control signals and the motors, allowing the firmware to control the direction and speed of each motor.

## Software

The firmware is intended to be developed and uploaded using the **Arduino IDE** with an ESP32-compatible board configuration and the required third-party dependencies installed.

Before compiling the firmware, make sure that:

1. The ESP32 board support package is installed.
2. The required third-party libraries are installed.
3. The correct ESP32 board is selected in Arduino IDE.
4. The `ControlConPs4Controller.ino` sketch is opened.
5. The ESP32 is connected through USB.

## Repository Structure

```text
OrbViSense-robot/
├── README.md
└── firmware/
    └── arduino/
        └── ControlConPs4Controller.ino
```
## Robot Images

The `images/` directory contains photographs of the assembled robot and the PCB, including both the front and back sides.

## Project Status

This repository currently contains the robot's Arduino control firmware.

Additional robot hardware documentation, CAD files, PCB design files, schematics, and manufacturing files may be added separately in the future.

## License

The firmware contained in this repository is part of the OrbViSense project.

Third-party dependencies are not included in this repository and remain under their respective licenses.

See the individual dependency repositories for their complete license terms.
