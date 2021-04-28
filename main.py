# This Python file uses the following encoding: utf-8
import sys
import os
import asyncio
import platform
import glob
import time

from PyQt5 import QtCore, QtWidgets
import breeze_resources

from bleak import BleakClient, BleakScanner

from form import Ui_main


class main(QtWidgets.QMainWindow):

    def __init__(self):
        super(main, self).__init__()
        self.ui = Ui_main()
        self.ui.setupUi(self)
        #Get BLE devices
        self.loop = asyncio.get_event_loop()
        self.loop.run_until_complete(self.get_BLE_devices())
        #Connect all GUI components
        self.connect_all_gui_components()
        #Disable buttons
        self.ui.pushButton_start.setEnabled(False)
        self.ui.pushButton_stop.setEnabled(False)
        self.ui.doubleSpinBox_flowrate.setEnabled(False)
        self.ui.doubleSpinBox_volume.setEnabled(False)
        self.ui.spinBox_calibration.setEnabled(False)
        self.ui.comboBox_microstepping.setEnabled(False)
        self.ui.comboBox_modus.setEnabled(False)


    def connect_all_gui_components(self):
        self.ui.comboBox_ports.currentIndexChanged.connect(self.set_BLEdevice_mac_addr)
        self.ui.pushButton_start.clicked.connect(self.start)
        self.ui.pushButton_stop.clicked.connect(self.stop)
        self.ui.pushButton_update.clicked.connect(self.update_BLE_devices)


    # Set the port that is selected from the dropdown menu
    def set_BLEdevice_mac_addr(self):
        self.BLEdevice_mac_addr = self.ui.comboBox_ports.currentText().split(":")[0]
        print(self.BLEdevice_mac_addr)

        #Enable buttons
        self.ui.pushButton_start.setEnabled(True)
        self.ui.pushButton_stop.setEnabled(True)
        self.ui.doubleSpinBox_flowrate.setEnabled(True)
        self.ui.doubleSpinBox_volume.setEnabled(True)
        self.ui.spinBox_calibration.setEnabled(True)
        self.ui.comboBox_microstepping.setEnabled(True)

    def update_BLE_devices(self):
        self.ui.comboBox_ports.clear()
        self.loop = asyncio.get_event_loop()
        self.loop.run_until_complete(self.get_BLE_devices())

    async def get_BLE_devices(self):
        #Scans all BLE decives
        devices = []
        device = await BleakScanner.discover()
        for i in range(0,len(device)):
            print("["+str(i)+"]"+str(device[i]))
            devices.append(str(device[i]))
        self.ui.comboBox_ports.addItems(devices)


    def start(self):
        #Fetch pump parameter
        self.flowrate = self.ui.doubleSpinBox_flowrate.value()
        self.volume = self.ui.doubleSpinBox_volume.value()
        self.conversion_mLTOsteps = self.ui.spinBox_calibration.value()
        self.steps_per_second = round((self.flowrate / 60) * self.conversion_mLTOsteps)
        self.total_steps = round(self.volume * self.conversion_mLTOsteps)
        #Send data to pump via BLE
        self.loop.run_until_complete(self.send_data())
        #Enable or disable Buttons
        self.ui.pushButton_start.setEnabled(False)
        self.ui.pushButton_stop.setEnabled(True)
        self.ui.doubleSpinBox_flowrate.setEnabled(False)
        self.ui.doubleSpinBox_volume.setEnabled(False)
        self.ui.spinBox_calibration.setEnabled(False)
        self.ui.comboBox_microstepping.setEnabled(False)
        #Animate ProgressBar
        self.ui.progressBar.setRange(0, 100*self.total_steps/self.conversion_mLTOsteps)
        self.timeLine = QtCore.QTimeLine(1000*(self.total_steps/self.steps_per_second), self) #Construct a timeline by passing its duration in milliseconds
        self.timeLine.setFrameRange(0, 100*self.total_steps/self.conversion_mLTOsteps)
        self.timeLine.setEasingCurve(QtCore.QEasingCurve.Linear)                    #Sets a LINEAR interpolation of the progress bar
        self.timeLine.frameChanged[int].connect(self.ui.progressBar.setValue)
        self.timeLine.start()

    def stop(self):
        #Send data to pump via BLE
        self.total_steps = 0
        self.steps_per_second = 0
        self.loop.run_until_complete(self.send_data())
        #Enable or disable Buttons
        self.ui.pushButton_start.setEnabled(True)
        self.ui.pushButton_stop.setEnabled(False)
        self.ui.doubleSpinBox_flowrate.setEnabled(True)
        self.ui.doubleSpinBox_volume.setEnabled(True)
        self.ui.spinBox_calibration.setEnabled(True)
        self.ui.comboBox_microstepping.setEnabled(True)
        #Stop ProgressBar Animation
        self.ui.progressBar.reset()
        self.timeLine.stop()

    async def send_data(self):
        #Send data to one specific BLE device
        #Data is send in seperate bytes, while 10 bytes from a "full package" (variable value)
        device = await BleakScanner.find_device_by_address(self.BLEdevice_mac_addr)
        async with BleakClient(device) as client:
            data_steps = str(self.total_steps)
            for x in range(10-len(data_steps)):
                await client.write_gatt_char("00002a00-0000-1000-8000-00805f9b34fb", b'0')
                await asyncio.sleep(0.05)
            for x in range(len(data_steps)):
                substring = data_steps[x:x+1]
                substring_as_bytes = str.encode(substring)
                await client.write_gatt_char("00002a00-0000-1000-8000-00805f9b34fb", substring_as_bytes)
                await asyncio.sleep(0.05)
            data_speed = str(self.steps_per_second)
            for x in range(10-len(data_speed)):
                await client.write_gatt_char("1205d6e3-1a79-4b03-aa02-ce0119f79263", b'0')
                await asyncio.sleep(0.05)
            for x in range(len(data_speed)):
                substring = data_speed[x:x+1]
                substring_as_bytes = str.encode(substring)
                await client.write_gatt_char("1205d6e3-1a79-4b03-aa02-ce0119f79263", substring_as_bytes)
                await asyncio.sleep(0.05)

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)

    #Set Stylesheet
    file = QtCore.QFile(":/dark.qss")
    file.open(QtCore.QFile.ReadOnly | QtCore.QFile.Text)
    stream = QtCore.QTextStream(file)
    app.setStyleSheet(stream.readAll())

    window = main()
    window.show()
    sys.exit(app.exec_())
