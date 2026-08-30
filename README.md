# Automated Cat Food Dispenser
A pet food dispenser that automatically feeds your pet the right amount of food at the right time. Runs on Arduino Nano R4 or adjacent boards.

* Feeds your pet the desired amount of food
* Allows you to set the time of day you want it to dispense the food
* Spoken audio timer set interface
* Easily refillable lid
* Stylish (Looks like a cat / other pet if u want)

---

## What it does

To open the settings menu, press 'MENU/OK' and then press '+' to navigate the different settings. Pressing 'MENU/OK' selects the current option, and pressing '+' moves to the next option. You can set the time of day the feeder will dispense, set the actual time for the internal clock, as well as set the volume of food to be dispensed. The speaker will navigate through the different settings.

| Settings: | Page 1 | Page 2 | Page 3 | Additional Feeding Time Option |
| -------- | -------- | -------- | -------- | -------- |
| Set Time | Set Hour | Set Minutes | AM/PM | Plus Other Time? |
| Set Clock | Set Hour | Set Minutes | AM/PM | |
| Control Unit Weight | Set Unit | | | Plus Other Unit? |

You can set add multiple times of the day for feeding by selecting the last option, which will loop back to Page 1 to create another time.

---

## Libraries needed

* TalkiePCM
* Arduino Audio Tools: https://github.com/pschatzmann/arduino-audio-tools
* Arduino Audio Driver: https://github.com/pschatzmann/arduino-audio-driver

---

## Quick setup

1. Clone this repository or download the ZIP
2. Open Automated_Cat_Feeder.ino in Arduino IDE
3. Select Arduino NAno R4 as your board
4. Upload

---

## Building instructions

---

## AI usage

Only used AI to help learn how to use the talkie library commands.

---

Built by Liam Rauh
