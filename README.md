# English|[中文](/README-ZH.md) 
# Android USB OTG Serial Port Remote Access

## Project Introduction

This project is an Android-based application designed to connect serial port devices via USB OTG and enable remote access to these devices using the RFC2217 service. It is built upon the [mik3y/usb-serial-for-android](https://github.com/mik3y/usb-serial-for-android) library, which provides robust functionality for accessing serial port devices. This application offers a convenient solution for communication between Android devices and serial port devices. By using `rfc2217://[device IP]:2217`, users can access supported serial port devices connected to the OTG. This allows for the operation and management of serial port devices from various devices and network environments, expanding the application scenarios and usage range of serial port devices.

## Features

- **USB OTG Connection**: Supports connecting various serial port devices, such as industrial sensors and smart meters, via the USB OTG interface, bridging the communication between Android devices and serial port devices.
- **RFC2217 Service**: Adheres to the RFC2217 protocol standard to enable remote access to serial ports. Users can connect to this service over the network to perform read and write operations on serial port devices, breaking the limitations of physical location. Access to supported serial port devices connected to the OTG can be achieved through `rfc2217://[device IP]:2217`.
- **Multi-device Support**: Compatible with multiple models of serial port devices, capable of automatically identifying and connecting devices without a complex configuration process, enhancing the application's versatility and ease of use.
- **Stability and Reliability**: After rigorous testing and optimization, it ensures stable and reliable communication performance even during long-term operation and high-concurrency access, ensuring accurate data transmission.

## Usage

1. **Install the Application**: Install this application [AndroidOTGSerialRemote.apk](https://vip.123pan.cn/1812665715/files/AndroidOTGSerialRemote.apk) on an Android device that supports USB OTG functionality.
2. **Connect Serial Port Device**: Use a USB OTG cable to connect the serial port device to the USB port of the Android device.
3. **Launch the Application**: Open the application, and it will automatically detect and connect to the connected serial port device.
4. **Configure RFC2217 Service**: Configure the parameters of the RFC2217 service in the application, such as port number and authentication information, to meet the access requirements of different network environments. By default, the service listens on the `0.0.0.0:2217` port.
5. **Remote Access**: Connect to `rfc2217://[device IP]:2217` using a client software that supports the RFC2217 protocol (such as PuTTY) to perform remote read and write operations on the serial port device.

## Technical Architecture

### Frontend

- **Development Language**: Java/Kotlin
- **Framework**: Android SDK
- **Serial Communication Library**: [mik3y/usb-serial-for-android](https://github.com/mik3y/usb-serial-for-android)
- **Functionality**: Implements the connection and communication functions of USB OTG devices, and provides a user interface for configuring and managing the RFC2217 service.

### Backend

- **RFC2217 Service**: Implemented based on the Python package [esp_rfc2217_server](https://github.com/espressif/esptool/blob/master/esp_rfc2217_server.py)
- **Principle**:
  - **Python Packaging**: The Python code is packaged into a shared library (.so file).
  - **JNI Invocation**: The Java Native Interface (JNI) is used to call the Python shared library in the Android application to implement the functionality of the RFC2217 service.
- **Functionality**:
  - **Socket Programming**: Listens on a specified port (default is 2217) to receive connection requests from clients.
  - **RFC2217 Protocol**: Encapsulates and forwards serial port data according to the RFC2217 protocol specification to enable remote access to serial ports.

### Communication Process

1. **Device Connection**: The user connects the serial port device to the Android device via a USB OTG cable.
2. **Application Launch**: The application is opened, and it automatically detects and connects to the serial port device.
3. **Service Startup**: The application starts the RFC2217 service, listening on the `0.0.0.0:2217` port.
4. **Client Connection**: The user connects to `rfc2217://[device IP]:2217` using a client software that supports the RFC2217 protocol (such as PuTTY).
5. **Data Transmission**: The client communicates with the server through the RFC2217 protocol, and the server forwards the data to the serial port device, enabling remote read and write operations.

## Project Contribution

Contributions from developers are welcome to improve and optimize the application's functionality. You can start from the following aspects:

- **Code Optimization**: Optimize the existing code to enhance the application's performance and stability.
- **Function Expansion**: Add new functional modules according to actual needs, such as supporting more types of serial port devices, adding data encryption features, etc.
- **Documentation Improvement**: Supplement and improve the project's documentation, including development documents and user manuals, to facilitate better understanding and use of the project by other developers and users.

## Contact Information

If you encounter any issues during use or have good suggestions and ideas, please feel free to contact me through the following methods:

- **Email**: [panxuesen520@gmail.com](mailto:panxuesen520@gmail.com)
- **GitHub Issues**: Submit Issues in the project's GitHub repository, and I will reply and handle them in a timely manner.

I hope this project can be helpful to you. If you think the project is good, please don't hesitate to like and Star it. Thank you!
