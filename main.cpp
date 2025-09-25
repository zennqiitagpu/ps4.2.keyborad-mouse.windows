#include <windows.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <conio.h>
#include <hidsdi.h>
#include <setupapi.h>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "user32.lib")

// PS4 Controller USB VID/PID
const USHORT PS4_VENDOR_ID = 0x054C;
const USHORT PS4_PRODUCT_ID = 0x05C4; // Wired controller
const USHORT PS4_PRODUCT_ID_WIRELESS = 0x09CC; // Wireless controller

// PS4 Controller input report structure (64 bytes for USB)
struct PS4InputReport {
    BYTE reportId;          // Report ID (0x01)
    BYTE leftStickX;        // Left stick X (0-255)
    BYTE leftStickY;        // Left stick Y (0-255)
    BYTE rightStickX;       // Right stick X (0-255)
    BYTE rightStickY;       // Right stick Y (0-255)
    BYTE buttons1;          // Button group 1
    BYTE buttons2;          // Button group 2
    BYTE buttons3;          // Button group 3
    BYTE leftTrigger;       // L2 trigger (0-255)
    BYTE rightTrigger;      // R2 trigger (0-255)
    BYTE timestamp[2];      // Timestamp
    BYTE batteryLevel;      // Battery level
    BYTE gyroX[2];          // Gyroscope X
    BYTE gyroY[2];          // Gyroscope Y
    BYTE gyroZ[2];          // Gyroscope Z
    BYTE accelX[2];         // Accelerometer X
    BYTE accelY[2];         // Accelerometer Y
    BYTE accelZ[2];         // Accelerometer Z
    BYTE reserved[5];       // Reserved bytes
    BYTE trackpadData[37];  // Trackpad and additional data
};

class PS4Controller {
private:
    HANDLE hDevice;
    bool connected;
    PS4InputReport lastReport;

public:
    PS4Controller() : hDevice(INVALID_HANDLE_VALUE), connected(false) {
        memset(&lastReport, 0, sizeof(lastReport));
    }

    ~PS4Controller() {
        disconnect();
    }

    bool connect() {
        // Get device information set for HID devices
        GUID hidGuid;
        HidD_GetHidGuid(&hidGuid);

        HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, 
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

        if (deviceInfoSet == INVALID_HANDLE_VALUE) {
            return false;
        }

        SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
        deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        // Enumerate through devices
        for (DWORD deviceIndex = 0; 
             SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, 
                                       deviceIndex, &deviceInterfaceData); 
             deviceIndex++) {
            
            // Get required size for device interface detail
            DWORD requiredSize = 0;
            SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, 
                                          NULL, 0, &requiredSize, NULL);

            // Allocate memory for device interface detail
            PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = 
                (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
            
            if (deviceInterfaceDetailData) {
                deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                // Get device interface detail
                if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData,
                                                  deviceInterfaceDetailData, requiredSize,
                                                  NULL, NULL)) {
                    
                    // Try to open device
                    HANDLE testHandle = CreateFile(deviceInterfaceDetailData->DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, 0, NULL);

                    if (testHandle != INVALID_HANDLE_VALUE) {
                        // Get device attributes
                        HIDD_ATTRIBUTES attributes;
                        attributes.Size = sizeof(HIDD_ATTRIBUTES);

                        if (HidD_GetAttributes(testHandle, &attributes)) {
                            // Check if this is a PS4 controller
                            if (attributes.VendorID == PS4_VENDOR_ID &&
                                (attributes.ProductID == PS4_PRODUCT_ID ||
                                 attributes.ProductID == PS4_PRODUCT_ID_WIRELESS)) {
                                
                                hDevice = testHandle;
                                connected = true;
                                free(deviceInterfaceDetailData);
                                SetupDiDestroyDeviceInfoList(deviceInfoSet);
                                return true;
                            }
                        }
                        CloseHandle(testHandle);
                    }
                }
                free(deviceInterfaceDetailData);
            }
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return false;
    }

    void disconnect() {
        if (hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(hDevice);
            hDevice = INVALID_HANDLE_VALUE;
        }
        connected = false;
    }

    bool readInput() {
        if (!connected || hDevice == INVALID_HANDLE_VALUE) {
            return false;
        }

        BYTE buffer[64];
        DWORD bytesRead;

        if (ReadFile(hDevice, buffer, sizeof(buffer), &bytesRead, NULL)) {
            if (bytesRead >= sizeof(PS4InputReport)) {
                memcpy(&lastReport, buffer, sizeof(PS4InputReport));
                return true;
            }
        }
        return false;
    }

    void displayInput() {
        system("cls"); // Clear screen
        
        std::cout << "=== PS4 Controller Input Visualization ===\n\n";

        // Analog Sticks
        std::cout << "Analog Sticks:\n";
        std::cout << "  Left Stick:  X=" << std::setw(3) << (int)lastReport.leftStickX 
                  << " Y=" << std::setw(3) << (int)lastReport.leftStickY << "\n";
        std::cout << "  Right Stick: X=" << std::setw(3) << (int)lastReport.rightStickX 
                  << " Y=" << std::setw(3) << (int)lastReport.rightStickY << "\n\n";

        // Triggers
        std::cout << "Triggers:\n";
        std::cout << "  L2: " << std::setw(3) << (int)lastReport.leftTrigger 
                  << " [" << std::string((int)lastReport.leftTrigger / 10, '|') 
                  << std::string(25 - (int)lastReport.leftTrigger / 10, ' ') << "]\n";
        std::cout << "  R2: " << std::setw(3) << (int)lastReport.rightTrigger 
                  << " [" << std::string((int)lastReport.rightTrigger / 10, '|') 
                  << std::string(25 - (int)lastReport.rightTrigger / 10, ' ') << "]\n\n";

        // Buttons - Group 1 (buttons1)
        std::cout << "Face Buttons:\n";
        std::cout << "  Square:   " << ((lastReport.buttons1 & 0x10) ? "[X]" : "[ ]") << "\n";
        std::cout << "  Cross:    " << ((lastReport.buttons1 & 0x20) ? "[X]" : "[ ]") << "\n";
        std::cout << "  Circle:   " << ((lastReport.buttons1 & 0x40) ? "[X]" : "[ ]") << "\n";
        std::cout << "  Triangle: " << ((lastReport.buttons1 & 0x80) ? "[X]" : "[ ]") << "\n\n";

        // Shoulder Buttons
        std::cout << "Shoulder Buttons:\n";
        std::cout << "  L1: " << ((lastReport.buttons1 & 0x01) ? "[X]" : "[ ]") << "\n";
        std::cout << "  R1: " << ((lastReport.buttons1 & 0x02) ? "[X]" : "[ ]") << "\n";
        std::cout << "  L2: " << ((lastReport.buttons1 & 0x04) ? "[X]" : "[ ]") << "\n";
        std::cout << "  R2: " << ((lastReport.buttons1 & 0x08) ? "[X]" : "[ ]") << "\n\n";

        // D-Pad
        std::cout << "D-Pad:\n";
        BYTE dpad = lastReport.buttons1 & 0x0F;
        if (dpad <= 7) {
            std::cout << "  Direction: ";
            switch(dpad) {
                case 0: std::cout << "North\n"; break;
                case 1: std::cout << "North-East\n"; break;
                case 2: std::cout << "East\n"; break;
                case 3: std::cout << "South-East\n"; break;
                case 4: std::cout << "South\n"; break;
                case 5: std::cout << "South-West\n"; break;
                case 6: std::cout << "West\n"; break;
                case 7: std::cout << "North-West\n"; break;
            }
        } else {
            std::cout << "  Direction: Center\n";
        }
        std::cout << "\n";

        // System Buttons
        std::cout << "System Buttons:\n";
        std::cout << "  Share:     " << ((lastReport.buttons2 & 0x10) ? "[X]" : "[ ]") << "\n";
        std::cout << "  Options:   " << ((lastReport.buttons2 & 0x20) ? "[X]" : "[ ]") << "\n";
        std::cout << "  L3:        " << ((lastReport.buttons2 & 0x40) ? "[X]" : "[ ]") << "\n";
        std::cout << "  R3:        " << ((lastReport.buttons2 & 0x80) ? "[X]" : "[ ]") << "\n";
        std::cout << "  PS Button: " << ((lastReport.buttons2 & 0x01) ? "[X]" : "[ ]") << "\n";
        std::cout << "  Trackpad:  " << ((lastReport.buttons2 & 0x02) ? "[X]" : "[ ]") << "\n\n";

        // Battery Level
        std::cout << "Battery Level: " << (int)lastReport.batteryLevel << "\n\n";

        std::cout << "Press ESC to exit...\n";
    }

    bool isConnected() const {
        return connected;
    }
};

int main() {
    std::cout << "PS4 Controller Raw Input Reader\n";
    std::cout << "Searching for PS4 controller...\n";

    PS4Controller controller;

    if (!controller.connect()) {
        std::cout << "Failed to find or connect to PS4 controller.\n";
        std::cout << "Make sure your PS4 controller is connected via USB.\n";
        std::cout << "Press any key to exit...";
        _getch();
        return -1;
    }

    std::cout << "PS4 Controller connected successfully!\n";
    std::cout << "Starting input visualization...\n";
    Sleep(1000);

    // Main input loop
    while (true) {
        // Check for ESC key to exit
        if (_kbhit() && _getch() == 27) {
            break;
        }

        if (controller.readInput()) {
            controller.displayInput();
        } else {
            std::cout << "Failed to read controller input. Controller may be disconnected.\n";
            break;
        }

        Sleep(50); // Update at ~20 FPS
    }

    std::cout << "\nExiting...\n";
    return 0;
}
