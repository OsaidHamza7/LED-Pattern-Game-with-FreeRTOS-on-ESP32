# LED Pattern Game with FreeRTOS on ESP32

## Overview
  This project is an interactive memory-based LED game built around an ESP32 running FreeRTOS, integrated with the Blynk mobile app. The goal is to replicate increasingly complex LED light sequences using virtual buttons in the Blynk app. Points are awarded for correctly matching the sequences, and the challenge escalates with each level.

## Key Features

- Memory Game Mechanics: Players watch an LED sequence, then try to replicate it via buttons in the Blynk app.
- FreeRTOS on ESP32: Uses multiple tasks (LED control, user input handling, LCD updates) for efficient multitasking.
- Blynk Integration: Virtual buttons on the Blynk app send input commands, while an additional Start Game button initiates each round.
- Scoring and Levels: Each round becomes more difficult by increasing the sequence length. Players earn points and move to higher levels if they match the sequence correctly.
- LCD Feedback: Game instructions, current score, and mistake count are displayed on an LCD for real-time feedback.

## Hardware & Components
- ESP32 Development Board.
- Breadboard.
- LEDs (4 different colors).
- Resistors (220 Ω for LEDs).
- LCD (16x2 character LCD).
- Wires.

## Circuit Setup
- LEDs: Each LED is connected to an ESP32 GPIO pin via a 220 Ω resistor.
- LCD: Connect the LCD to the ESP32 using I2C.
- ESP32: Provide stable 3.3V power and ground.

## FreeRTOS Tasks
### LED Control Task
- Displays the randomly generated pattern to the player.
Turns LEDs on and off with appropriate delays.

### User Input Task
- Receives button press data from the Blynk app.
Compares the player’s input sequence to the generated sequence.

### LCD Update Task
- Displays messages such as “Watch closely!” or “Repeat the sequence”.
Shows current score, mistakes, and other game status messages.

## Blynk App
- ### Virtual Pins:
- Start Game Button: Resets variables (score, level, mistakes) and initiates the game.
- Four LED Buttons: Each corresponds to one of the four LEDs; used to replicate the LED sequence.
