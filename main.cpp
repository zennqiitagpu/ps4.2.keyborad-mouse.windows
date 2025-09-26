#include <windows.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <mutex>
#include <optional>
#include <thread>
#include <chrono>
#include <conio.h>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>

#pragma pack(push, 1)
struct PS4ControllerReport {
    uint8_t reportId;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t buttons1;      // D-pad (lower nibble) and face buttons (upper nibble)
    uint8_t buttons2;      // Shoulder buttons and stick clicks
    uint8_t buttons3;      // PS, touchpad, share, options (approx.)
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

// Input mapper class for converting controller input to mouse/keyboard
class InputMapper {
private:
    struct ButtonState {
        bool current = false;
        bool previous = false;
        
        bool justPressed() const { return current && !previous; }
        bool justReleased() const { return !current && previous; }
        void update(bool newState) {
            previous = current;
            current = newState;
        }
    };

    ButtonState crossButton;
    ButtonState squareButton;
    ButtonState circleButton;
    ButtonState triangleButton;
    ButtonState l1Button;
    ButtonState r1Button;
    ButtonState l3Button;
    ButtonState r3Button;
    ButtonState shareButton;
    ButtonState optionsButton;
    
    // Mouse sensitivity settings
    float mouseSensitivity = 2.0f;
    float deadzone = 0.15f;
    
    // Previous stick positions for smooth mouse movement
    float prevLeftX = 0.0f;
    float prevLeftY = 0.0f;
    float prevRightX = 0.0f;
    float prevRightY = 0.0f;

public:
    void processInput(const PS4ControllerReport& report) {
        // Update button states
        crossButton.update(report.buttons1 & 0x20);      // Cross button
        squareButton.update(report.buttons1 & 0x10);     // Square button
        circleButton.update(report.buttons1 & 0x40);     // Circle button
        triangleButton.update(report.buttons1 & 0x80);   // Triangle button
        l1Button.update(report.buttons2 & 0x01);         // L1 button
        r1Button.update(report.buttons2 & 0x02);         // R1 button
        l3Button.update(report.buttons2 & 0x40);         // L3 (left stick click)
        r3Button.update(report.buttons2 & 0x80);         // R3 (right stick click)
        shareButton.update(report.buttons2 & 0x10);      // Share button
        optionsButton.update(report.buttons2 & 0x20);    // Options button

        // Handle button presses
        handleButtonInputs();
        
        // Handle analog sticks
        handleAnalogInputs(report);
        
        // Handle D-pad
        handleDpadInput(report.buttons1 & 0x0F);
        
        // Handle triggers
        handleTriggers(report.leftTrigger, report.rightTrigger);
    }

private:
    void handleButtonInputs() {
        // Cross button - Left mouse click
        if (crossButton.justPressed()) {
            simulateMouseClick(MOUSEEVENTF_LEFTDOWN);
            std::cout << "Cross pressed - Left click\n";
        }
        if (crossButton.justReleased()) {
            simulateMouseClick(MOUSEEVENTF_LEFTUP);
        }

        // Circle button - Right mouse click
        if (circleButton.justPressed()) {
            simulateMouseClick(MOUSEEVENTF_RIGHTDOWN);
            std::cout << "Circle pressed - Right click\n";
        }
        if (circleButton.justReleased()) {
            simulateMouseClick(MOUSEEVENTF_RIGHTUP);
        }

        // Square button - Space key
        if (squareButton.justPressed()) {
            simulateKeyPress(VK_SPACE, true);
            std::cout << "Square pressed - Space key\n";
        }
        if (squareButton.justReleased()) {
            simulateKeyPress(VK_SPACE, false);
        }

        // Triangle button - Enter key
        if (triangleButton.justPressed()) {
            simulateKeyPress(VK_RETURN, true);
            std::cout << "Triangle pressed - Enter key\n";
        }
        if (triangleButton.justReleased()) {
            simulateKeyPress(VK_RETURN, false);
        }

        // L1 - Shift key
        if (l1Button.justPressed()) {
            simulateKeyPress(VK_LSHIFT, true);
            std::cout << "L1 pressed - Left Shift\n";
        }
        if (l1Button.justReleased()) {
            simulateKeyPress(VK_LSHIFT, false);
        }

        // R1 - Control key
        if (r1Button.justPressed()) {
            simulateKeyPress(VK_LCONTROL, true);
            std::cout << "R1 pressed - Left Ctrl\n";
        }
        if (r1Button.justReleased()) {
            simulateKeyPress(VK_LCONTROL, false);
        }

        // Share button - Tab key
        if (shareButton.justPressed()) {
            simulateKeyPress(VK_TAB, true);
            std::cout << "Share pressed - Tab key\n";
        }
        if (shareButton.justReleased()) {
            simulateKeyPress(VK_TAB, false);
        }

        // Options button - Escape key
        if (optionsButton.justPressed()) {
            simulateKeyPress(VK_ESCAPE, true);
            std::cout << "Options pressed - Escape key\n";
        }
        if (optionsButton.justReleased()) {
            simulateKeyPress(VK_ESCAPE, false);
        }

        // L3 - Middle mouse click
        if (l3Button.justPressed()) {
            simulateMouseClick(MOUSEEVENTF_MIDDLEDOWN);
            std::cout << "L3 pressed - Middle click\n";
        }
        if (l3Button.justReleased()) {
            simulateMouseClick(MOUSEEVENTF_MIDDLEUP);
        }
    }

    void handleAnalogInputs(const PS4ControllerReport& report) {
        // Right stick controls mouse movement
        float rightX = normalizeStick(report.rightStickX);
        float rightY = normalizeStick(report.rightStickY);

        if (std::abs(rightX) > deadzone || std::abs(rightY) > deadzone) {
            // Apply smoothing
            rightX = (rightX + prevRightX) * 0.5f;
            rightY = (rightY + prevRightY) * 0.5f;
            
            int deltaX = static_cast<int>(rightX * mouseSensitivity * 10);
            int deltaY = static_cast<int>(rightY * mouseSensitivity * 10);
            
            simulateMouseMove(deltaX, deltaY);
        }
        
        prevRightX = rightX;
        prevRightY = rightY;

        // Left stick can be used for WASD movement
        float leftX = normalizeStick(report.leftStickX);
        float leftY = normalizeStick(report.leftStickY);
        
        handleWASDMovement(leftX, leftY);
    }

    void handleWASDMovement(float x, float y) {
        static bool wPressed = false, aPressed = false, sPressed = false, dPressed = false;
        
        // Handle horizontal movement (A/D)
        if (x < -deadzone && !aPressed) {
            simulateKeyPress('A', true);
            aPressed = true;
            if (dPressed) {
                simulateKeyPress('D', false);
                dPressed = false;
            }
        } else if (x > deadzone && !dPressed) {
            simulateKeyPress('D', true);
            dPressed = true;
            if (aPressed) {
                simulateKeyPress('A', false);
                aPressed = false;
            }
        } else if (std::abs(x) <= deadzone) {
            if (aPressed) {
                simulateKeyPress('A', false);
                aPressed = false;
            }
            if (dPressed) {
                simulateKeyPress('D', false);
                dPressed = false;
            }
        }

        // Handle vertical movement (W/S)
        if (y < -deadzone && !wPressed) {
            simulateKeyPress('W', true);
            wPressed = true;
            if (sPressed) {
                simulateKeyPress('S', false);
                sPressed = false;
            }
        } else if (y > deadzone && !sPressed) {
            simulateKeyPress('S', true);
            sPressed = true;
            if (wPressed) {
                simulateKeyPress('W', false);
                wPressed = false;
            }
        } else if (std::abs(y) <= deadzone) {
            if (wPressed) {
                simulateKeyPress('W', false);
                wPressed = false;
            }
            if (sPressed) {
                simulateKeyPress('S', false);
                sPressed = false;
            }
        }
    }

    void handleDpadInput(uint8_t dpad) {
        static int lastDpad = 8; // neutral
        
        if (dpad != lastDpad) {
            // Release previous arrow key
            switch (lastDpad) {
                case 0: simulateKeyPress(VK_UP, false); break;
                case 2: simulateKeyPress(VK_RIGHT, false); break;
                case 4: simulateKeyPress(VK_DOWN, false); break;
                case 6: simulateKeyPress(VK_LEFT, false); break;
            }
            
            // Press new arrow key
            switch (dpad) {
                case 0: // Up
                    simulateKeyPress(VK_UP, true);
                    std::cout << "D-pad Up - Arrow Up\n";
                    break;
                case 2: // Right
                    simulateKeyPress(VK_RIGHT, true);
                    std::cout << "D-pad Right - Arrow Right\n";
                    break;
                case 4: // Down
                    simulateKeyPress(VK_DOWN, true);
                    std::cout << "D-pad Down - Arrow Down\n";
                    break;
                case 6: // Left
                    simulateKeyPress(VK_LEFT, true);
                    std::cout << "D-pad Left - Arrow Left\n";
                    break;
            }
            lastDpad = dpad;
        }
    }

    void handleTriggers(uint8_t leftTrigger, uint8_t rightTrigger) {
        static bool l2Pressed = false, r2Pressed = false;
        const uint8_t threshold = 50;
        
        // L2 trigger - Mouse wheel up
        if (leftTrigger > threshold && !l2Pressed) {
            simulateMouseWheel(1);
            l2Pressed = true;
            std::cout << "L2 trigger - Mouse wheel up\n";
        } else if (leftTrigger <= threshold) {
            l2Pressed = false;
        }
        
        // R2 trigger - Mouse wheel down
        if (rightTrigger > threshold && !r2Pressed) {
            simulateMouseWheel(-1);
            r2Pressed = true;
            std::cout << "R2 trigger - Mouse wheel down\n";
        } else if (rightTrigger <= threshold) {
            r2Pressed = false;
        }
    }

    float normalizeStick(uint8_t value) {
        return (static_cast<float>(value) - 128.0f) / 127.0f;
    }

    void simulateMouseClick(DWORD flags) {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = flags;
        SendInput(1, &input, sizeof(INPUT));
    }

    void simulateMouseMove(int deltaX, int deltaY) {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = deltaX;
        input.mi.dy = deltaY;
        SendInput(1, &input, sizeof(INPUT));
    }

    void simulateMouseWheel(int direction) {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = direction * WHEEL_DELTA;
        SendInput(1, &input, sizeof(INPUT));
    }

    void simulateKeyPress(WORD vkCode, bool keyDown) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.dwFlags = keyDown ? 0 : KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
};

// Console helper class (simplified for the mapper)
class Console {
public:
    Console() : hOut(GetStdHandle(STD_OUTPUT_HANDLE)) {
        if (hOut == INVALID_HANDLE_VALUE) throw std::runtime_error("Failed to get console output handle");
        hideCursor();
    }

    void clear() {
        system("cls"); // Simple clear for this version
    }

    void hideCursor() {
        CONSOLE_CURSOR_INFO info {};
        info.bVisible = FALSE;
        info.dwSize = 1;
        SetConsoleCursorInfo(hOut, &info);
    }

private:
    HANDLE hOut;
};

class PS4InputMapper {
public:
    PS4InputMapper() {
        registerWindowClass();
        createMessageWindow();
        registerRawInput();
        console.clear();
    }

    ~PS4InputMapper() {
        RAWINPUTDEVICE removeRid{};
        removeRid.usUsagePage = 0x01;
        removeRid.usUsage = 0x05;
        removeRid.dwFlags = RIDEV_REMOVE;
        removeRid.hwndTarget = nullptr;
        RegisterRawInputDevices(&removeRid, 1, sizeof(removeRid));

        if (hwnd) DestroyWindow(hwnd);
        UnregisterClassW(windowClassName.c_str(), GetModuleHandle(nullptr));
    }

    void run() {
        console.clear();
        printHeader();
        MSG msg;
        bool done = false;

        while (!done) {
            if (_kbhit()) {
                int ch = _getch();
                if (ch == 27) { // ESC
                    done = true;
                    break;
                }
            }

            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

private:
    void registerWindowClass() {
        WNDCLASSW wc{};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = windowClassName.c_str();
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClassW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                throw std::runtime_error("RegisterClass failed: " + std::to_string(err));
            }
        }
    }

    void createMessageWindow() {
        hwnd = CreateWindowW(windowClassName.c_str(), L"PS4InputMapperHiddenWindow",
                             0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandle(nullptr), this);
        if (!hwnd) throw std::runtime_error("CreateWindow failed");
    }

    void registerRawInput() {
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x05;
        rid.dwFlags = RIDEV_INPUTSINK;
        rid.hwndTarget = hwnd;

        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
            DWORD err = GetLastError();
            throw std::runtime_error("RegisterRawInputDevices failed: " + std::to_string(err));
        }
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        PS4InputMapper* self = nullptr;
        if (msg == WM_CREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<PS4InputMapper*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<PS4InputMapper*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self) return self->handleMessage(msg, wParam, lParam);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_INPUT:
                handleRawInput(reinterpret_cast<HRAWINPUT>(lParam));
                return 0;
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    void handleRawInput(HRAWINPUT hRaw) {
        UINT size = 0;
        if (GetRawInputData(hRaw, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1) return;
        if (size == 0) return;

        std::vector<BYTE> buffer(size);
        if (GetRawInputData(hRaw, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) return;

        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
        if (raw->header.dwType != RIM_TYPEHID) return;

        if (raw->data.hid.dwSizeHid >= sizeof(PS4ControllerReport) && raw->data.hid.dwCount >= 1) {
            PS4ControllerReport report{};
            std::memcpy(&report, raw->data.hid.bRawData, sizeof(report));

            // Process the input through our mapper
            inputMapper.processInput(report);
        }
    }

    void printHeader() {
        std::cout << "=== PS4 Controller to Mouse/Keyboard Mapper ===\n";
        std::cout << "Press ESC to exit. Controller mapping active:\n\n";
        std::cout << "Button Mappings:\n";
        std::cout << "  Cross (X)     -> Left Mouse Click\n";
        std::cout << "  Circle (O)    -> Right Mouse Click\n";
        std::cout << "  Square        -> Spacebar\n";
        std::cout << "  Triangle      -> Enter\n";
        std::cout << "  L1            -> Left Shift\n";
        std::cout << "  R1            -> Left Ctrl\n";
        std::cout << "  L3 (L-stick)  -> Middle Mouse Click\n";
        std::cout << "  Share         -> Tab\n";
        std::cout << "  Options       -> Escape\n\n";
        std::cout << "Analog Controls:\n";
        std::cout << "  Right Stick   -> Mouse Movement\n";
        std::cout << "  Left Stick    -> WASD Movement\n";
        std::cout << "  L2 Trigger    -> Mouse Wheel Up\n";
        std::cout << "  R2 Trigger    -> Mouse Wheel Down\n";
        std::cout << "  D-pad         -> Arrow Keys\n\n";
        std::cout << "Status: Ready for input...\n\n";
        std::cout.flush();
    }

private:
    HWND hwnd = nullptr;
    const std::wstring windowClassName = L"PS4InputMapperClass";
    InputMapper inputMapper;
    Console console;
};

int main() {
    try {
        std::cout << "Starting PS4 Controller to Mouse/Keyboard Mapper...\n";
        PS4InputMapper mapper;
        mapper.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
