#include <windows.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <conio.h>

// PS4 Controller HID Report Structure
#pragma pack(push, 1)
struct PS4ControllerReport {
    uint8_t reportId;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t buttons1;      // D-pad and face buttons
    uint8_t buttons2;      // Shoulder buttons and stick clicks
    uint8_t buttons3;      // PS, touchpad, share, options
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint8_t unknown1[2];
    uint8_t gyroX[2];
    uint8_t gyroY[2];
    uint8_t gyroZ[2];
    uint8_t accelX[2];
    uint8_t accelY[2];
    uint8_t accelZ[2];
    uint8_t unknown2[5];
    uint8_t battery;
    uint8_t unknown3[4];
    uint8_t touchpad[3];
    uint8_t unknown4[21];
};
#pragma pack(pop)

class PS4Visualizer {
private:
    HWND hwnd;
    PS4ControllerReport lastReport = {};
    bool controllerConnected = false;

    void clearScreen() {
        system("cls");
    }

    void gotoxy(int x, int y) {
        COORD coord;
        coord.X = x;
        coord.Y = y;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    }

    void drawStick(int x, int y, uint8_t stickX, uint8_t stickY, const std::string& name) {
        gotoxy(x, y);
        std::cout << name << " Stick:";
        
        // Draw stick position (127 is center)
        int posX = (stickX - 127) / 25; // Scale to -5 to +5
        int posY = (stickY - 127) / 25;
        
        for (int row = -2; row <= 2; row++) {
            gotoxy(x, y + 2 + row);
            for (int col = -5; col <= 5; col++) {
                if (col == posX && row == posY) {
                    std::cout << "●";
                } else if (col == 0 && row == 0) {
                    std::cout << "+";
                } else {
                    std::cout << "·";
                }
            }
        }
        
        gotoxy(x, y + 6);
        std::cout << "X: " << std::setw(3) << (int)stickX << " Y: " << std::setw(3) << (int)stickY;
    }

    void drawTrigger(int x, int y, uint8_t value, const std::string& name) {
        gotoxy(x, y);
        std::cout << name << ": ";
        
        // Draw trigger bar
        int bars = (value * 10) / 255;
        std::cout << "[";
        for (int i = 0; i < 10; i++) {
            if (i < bars) {
                std::cout << "█";
            } else {
                std::cout << "·";
            }
        }
        std::cout << "] " << std::setw(3) << (int)value;
    }

    void drawButtons() {
        gotoxy(0, 15);
        std::cout << "Buttons: ";
        
        // Face buttons (buttons1)
        if (lastReport.buttons1 & 0x10) std::cout << "[□] "; else std::cout << "□ ";
        if (lastReport.buttons1 & 0x20) std::cout << "[✕] "; else std::cout << "✕ ";
        if (lastReport.buttons1 & 0x40) std::cout << "[○] "; else std::cout << "○ ";
        if (lastReport.buttons1 & 0x80) std::cout << "[△] "; else std::cout << "△ ";
        
        std::cout << " | ";
        
        // D-pad (buttons1)
        if ((lastReport.buttons1 & 0x0F) == 0) std::cout << "[↑] "; else std::cout << "↑ ";
        if ((lastReport.buttons1 & 0x0F) == 4) std::cout << "[↓] "; else std::cout << "↓ ";
        if ((lastReport.buttons1 & 0x0F) == 6) std::cout << "[←] "; else std::cout << "← ";
        if ((lastReport.buttons1 & 0x0F) == 2) std::cout << "[→] "; else std::cout << "→ ";
        
        gotoxy(0, 16);
        // Shoulder buttons (buttons2)
        if (lastReport.buttons2 & 0x01) std::cout << "[L1] "; else std::cout << "L1 ";
        if (lastReport.buttons2 & 0x02) std::cout << "[R1] "; else std::cout << "R1 ";
        if (lastReport.buttons2 & 0x40) std::cout << "[L3] "; else std::cout << "L3 ";
        if (lastReport.buttons2 & 0x80) std::cout << "[R3] "; else std::cout << "R3 ";
        
        std::cout << " | ";
        
        // System buttons (buttons3)
        if (lastReport.buttons3 & 0x01) std::cout << "[PS] "; else std::cout << "PS ";
        if (lastReport.buttons3 & 0x02) std::cout << "[PAD] "; else std::cout << "PAD ";
        if (lastReport.buttons2 & 0x10) std::cout << "[SHARE] "; else std::cout << "SHARE ";
        if (lastReport.buttons2 & 0x20) std::cout << "[OPTIONS] "; else std::cout << "OPTIONS ";
    }

    void drawHeader() {
        gotoxy(0, 0);
        std::cout << "=== PS4 Controller Raw Input Visualizer ===\n";
        std::cout << "Controller: " << (controllerConnected ? "Connected" : "Disconnected") << "\n";
        std::cout << "Press ESC to exit\n\n";
    }

public:
    PS4Visualizer() {
        // Create invisible window for Raw Input
        WNDCLASS wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"PS4RawInput";
        RegisterClass(&wc);

        hwnd = CreateWindow(L"PS4RawInput", L"PS4 Raw Input", 0,
            0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandle(nullptr), this);

        // Register for Raw Input from HID devices
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01;  // Generic Desktop
        rid.usUsage = 0x05;      // Game Pad
        rid.dwFlags = RIDEV_INPUTSINK;
        rid.hwndTarget = hwnd;

        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
            std::cerr << "Failed to register Raw Input device\n";
        }

        clearScreen();
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        PS4Visualizer* visualizer = nullptr;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            visualizer = (PS4Visualizer*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)visualizer);
        } else {
            visualizer = (PS4Visualizer*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (visualizer) {
            return visualizer->HandleMessage(msg, wParam, lParam);
        }
        
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_INPUT: {
            UINT size;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
            
            std::vector<BYTE> buffer(size);
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) == size) {
                RAWINPUT* raw = (RAWINPUT*)buffer.data();
                
                if (raw->header.dwType == RIM_TYPEHID) {
                    // Check if this is a PS4 controller (approximate check)
                    if (raw->data.hid.dwSizeHid >= sizeof(PS4ControllerReport)) {
                        controllerConnected = true;
                        memcpy(&lastReport, raw->data.hid.bRawData, sizeof(PS4ControllerReport));
                        updateDisplay();
                    }
                }
            }
            break;
        }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void updateDisplay() {
        drawHeader();
        
        // Draw left stick
        drawStick(0, 4, lastReport.leftStickX, lastReport.leftStickY, "Left");
        
        // Draw right stick
        drawStick(25, 4, lastReport.rightStickX, lastReport.rightStickY, "Right");
        
        // Draw triggers
        drawTrigger(50, 4, lastReport.leftTrigger, "L2");
        drawTrigger(50, 5, lastReport.rightTrigger, "R2");
        
        // Draw battery
        gotoxy(50, 7);
        std::cout << "Battery: " << std::setw(3) << (int)lastReport.battery;
        
        // Draw buttons
        drawButtons();
        
        // Draw raw data (for debugging)
        gotoxy(0, 18);
        std::cout << "Raw Data: ";
        for (int i = 0; i < 16 && i < sizeof(PS4ControllerReport); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << (int)((uint8_t*)&lastReport)[i] << " ";
        }
        std::cout << std::dec;
    }

    void run() {
        std::cout << "Waiting for PS4 controller input...\n";
        std::cout << "Make sure your PS4 controller is connected via USB or Bluetooth.\n";
        std::cout << "Press ESC to exit.\n\n";

        MSG msg;
        while (true) {
            // Check for ESC key
            if (_kbhit()) {
                char key = _getch();
                if (key == 27) { // ESC key
                    break;
                }
            }

            // Process Windows messages
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            Sleep(16); // ~60 FPS
        }
    }

    ~PS4Visualizer() {
        if (hwnd) {
            DestroyWindow(hwnd);
        }
    }
};

int main() {
    std::cout << "PS4 Controller Raw Input Visualizer\n";
    std::cout << "===================================\n\n";

    try {
        PS4Visualizer visualizer;
        visualizer.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
