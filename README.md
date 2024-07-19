# RubberRat

## Overview

RubberRat is an educational malware tool designed to demonstrate the risks and raise awareness about the potential dangers of allowing physical access to a machine, even for just a few seconds. It is developed solely for educational purposes and is not intended for malicious use. Use this tool responsibly and only on systems for which you have explicit permission to test and demonstrate vulnerabilities.

## How does it work?

RubberRat operates differently from other tools like Rubber Ducky, which typically require the user to physically retrieve data from a USB device. Instead, RubberRat utilizes the Digispark Attiny85 microcontroller to install and execute malware via a PowerShell script on the endpoint machine. This approach allows for remote access to the compromised system using the admin panel located on the C2 server.

![output2](https://github.com/user-attachments/assets/2a191cee-836e-40be-8700-6a5a27c24d55)

(Note: The cmd prompt is just for visuals it can be easily turned off)

## Features
- Keylogger
- USB Worm
- Screenshots, clipboard data, browser cookies of the compromised endpoint
- Remote Code Execution (CMD)
- Presistence using registry
- Command and control server handling muitple clients with unique IDs stored in a database
- Self destroy functionality
- Anti-Analysis measures
