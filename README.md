# ps4.controller.raw-input.visualizer

This program is a Windows console application that listens for input from a connected **PlayStation 4 (DualShock 4) controller** via the Raw Input API and displays the controller’s state in real time. It provides a text-based visualization of the controller’s sticks, triggers, buttons, D-pad, and battery level directly in the console window.

---

## Features

* **Real-time input display**
  Captures HID reports from the PS4 controller using the Windows Raw Input system.
* **Analog stick visualization**
  Shows each stick’s position on a small ASCII grid with center and live position markers.
* **Trigger bars**
  Displays pressure-sensitive trigger values (L2, R2) as progress bars.
* **Button states**
  Highlights face buttons (Square, Cross, Circle, Triangle), D-pad directions, shoulder buttons, stick clicks, PS, Share, Options, and Touchpad buttons.
* **Battery level**
  Displays the controller’s reported battery level.
* **Hex dump**
  Prints a portion of the raw HID report for debugging.
* **ESC to exit**
  Press `ESC` at any time to quit the program.

## Console Output

```
=== PS4 Controller Raw Input Visualizer (refactored) ===
Press ESC to exit. Make sure controller is connected (USB/Bluetooth).

Left Stick:                   Right Stick:                  L2: [..........]   0
...........                   ...........                   R2: [######....] 174
..@........                   ...........
.....+.....                   .....+.....                   Battery:   0
...........                   .........@.
...........                   ...........
X:  52 Y:  61                 X: 235 Y: 203

Buttons:  SQR   CRO  [CIR]  TRI
D-Pad: Neutral
 L1   R1   L3   R3   |  PS   PAD   SHARE   OPTIONS
Raw Data: 01 34 3d eb cb 48 08 58 00 ae 4e c5 00 c7 ff fe ff fb ff 54 03 29 21 12
```

---

## Requirements

* Windows 10 or later
* Microsoft Visual C++ (MSVC) or another C++17-compatible compiler on Windows
* A PS4 controller connected via **USB or Bluetooth**

---

## Building

Open a **Developer Command Prompt for Visual Studio** and run:

```
cl /EHsc /std:c++17 main.cpp user32.lib
```

This produces `main.exe`.

---

## Running

1. Connect your PS4 controller via USB or pair it with Bluetooth.
2. Launch the executable from a console window.
3. Move sticks, press buttons, and pull triggers — the console will update with a live visualization.
4. Press `ESC` to exit.

---

## Notes

* HID report formats may vary slightly depending on whether the controller is connected via USB or Bluetooth.
* The ASCII art visualization uses standard characters for compatibility; glyphs may look different depending on your console font and encoding.
* The application uses Raw Input with `RIDEV_INPUTSINK`, meaning it receives controller input even if the console window does not have focus.

---

## License

This program is released under the MIT License. See the LICENSE file for details.

