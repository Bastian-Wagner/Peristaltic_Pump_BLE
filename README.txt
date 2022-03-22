## Peristaltic Pump BLE

BOM:
- Arduino Nano RP2040
= Peristaltic Pump Housing
- 2M x 8mm screws
- BLE PCB

You can find here:
- Arduino code which has to be uploaded to the microcontroller "Arduino Nano BLE 33". This code enables communication between the Trinamic stepper drivers and your personal computer
- PyQT5 code for the Syringe Pump app that is running on the personal comupter enables direct control over the pump via a garaphical user interface

Graphical user interface of the Syringe Pump app:

![BLE pump GUI logo](https://raw.githubusercontent.com/BastianWagner/Syringe_Pump/master/Syringe_pump_GUI_1.png)


GUI was coded in python with PyQt5:

PyQt5 has to be installed on MAC with the terminal command...
> pip install PyQt5




BreezeStyleSheets:

https://github.com/Alexhuszagh/BreezeStyleSheets

Folder:
- dark 
- light

Files:
- dark.qss
- light.qss
- breeze_resources.py
- breeze.qrc

To compile the stylesheet for use with PyQt5, compile with the following command...
pyrcc5 breeze.qrc -o breeze_resources.py



Py2app:

Folder:
- dist
- build

File:
- setup.py
- icon.icns

To build app use following comand...
python setup.py py2app

