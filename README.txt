
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

