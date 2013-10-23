uCAN
====

Unified CAN driver

The purpose of this driver is to give a common interface for CAN interfaces.
The selected interface is very similar for SocketCAN (under Linux) to be able to make a cross platform applications.
This driver would be a middle-ware driver, which means it should not be a direct HW driver, but it can transfer the data between the application and the HW driver with unified interface.

The development is started on Windows (with GCC), but the goal is to produce a platform independent driver.

Currently 3 CAN HWs are supported:
- Vector CAN families (http://www.vector.de/)
- WeCAN (http://www.inventure.hu/)
- Peak - PCAN (http://www.peak-system.com/)

But any driver is welcome.

The project is coordinated by Istvan Baksa, Gabor Liptak and Laszlo Simon from Knorr-Bremse.
