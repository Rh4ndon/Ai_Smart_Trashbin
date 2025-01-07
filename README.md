# Hardware Connection Documentation

## Overview
This document details the connections between the ESP32-CAM and Arduino UNO for the smart recycling system. The system uses serial communication between the devices and controls various components including a servo motor, relay, TFT display, and a button.

## ESP32-CAM Connections

### Power
- VCC → 5V
- GND → GND

### Communication with Arduino UNO
- GPIO 12 (RX) → Arduino Pin 9 (TX)
- GPIO 13 (TX) → Arduino Pin 8 (RX)

### Button
- GPIO 4 → Button Pin
- Button GND → GND

### Relay Control for Arduino Power
- GPIO 14 → Relay Control Signal
- Base of PN2222A Transistor → GPIO 14 via 1kΩ Resistor
- Base of PN2222A Transistor → GND via 20kΩ Resistor
- Emitter of PN2222A → GND
- Collector of PN2222A → Relay Module IN Pin

### Camera Module
The camera module is directly integrated into the ESP32-CAM board.

## Arduino UNO Connections

## Power
- Vin → ESP32 Relay NC 1
- GND → ESP32 Relay NC 2

### TFT Display
- CS  → A5
- CD  → A3
- RST → A4
- LED → A0
- VCC → 5V
- GND → GND

### Servo Motor
- Signal → Pin 7
- VCC → 5V
- GND → GND

### Relay
- Control → Pin 6
- VCC → 5V
- GND → GND

### Charger
- 5v positive → Relay NC
- GND → GND

### Communication with ESP32-CAM
- Pin 8 (RX) → ESP32-CAM GPIO 13 (TX)
- Pin 9 (TX) → ESP32-CAM GPIO 12 (RX)

## Communication Protocol
- Baud Rate: 9600
- Data Format: 8N1 (8 data bits, no parity, 1 stop bit)

## System Operation Flow

1. **Initial State**
   - ESP32-CAM waits for button press
   - Arduino displays welcome message on TFT

2. **When Button Pressed**
   - ESP32-CAM captures image
   - Processes image through Edge Impulse model
   - Sends classification result to Arduino

3. **Arduino Response**
   - If plastic bottle detected:
     - Servo moves to accept position
     - Activates charging relay
     - Displays charging status
   - If non-plastic item detected:
     - Servo moves to reject position
     - Displays rejection message

4. **Charging Cycle**
   - 15-minute timer starts
   - TFT displays countdown
   - System returns to initial state after completion

## Important Notes

1. **Power Supply**
   - Both devices need stable 5V power supply
   - Consider separate power supply for servo if using larger model

2. **Ground Connection**
   - All grounds (GND) should be connected together
   - This ensures proper communication and operation

3. **Serial Communication**
   - Communication is bidirectional
   - ESP32-CAM sends classification results
   - Arduino sends charging status updates

4. **Safety Considerations**
   - Double-check all connections before powering on
   - Ensure proper insulation of all connections
   - Monitor system temperature during operation

5. **Serial Communication Conflict**
   - The ESP32-Cam must boot first before the Arduino to avoid conflict in Serial Communication
   - Relay is added to power the Adrduino so that it won't boot while ESP32-Cam is booting or else it cause error to the ESP32-Cam

## Troubleshooting Tips

1. **If Communication Fails**
   - Verify TX/RX connections are correct
   - Confirm baud rate settings match (9600)
   - Check ground connections

2. **If Servo Doesn't Respond**
   - Verify 5V power is sufficient
   - Check signal wire connection
   - Confirm servo library is properly initialized

3. **If TFT Display Issues**
   - Verify SPI connections
   - Check power supply voltage
   - Confirm library configuration matches connections

4. **If Camera Doesn't Work**
   - Check camera module connection
   - Verify ESP32-CAM initialization
   - Confirm proper power supply
