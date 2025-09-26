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

// Small console helper (RAII) -- hides the cursor while running and provides
// small screen manipulation helpers without calling system("cls").
class Console {
public:
    Console() : hOut(GetStdHandle(STD_OUTPUT_HANDLE)) {
        if (hOut == INVALID_HANDLE_VALUE) throw std::runtime_error("Failed to get console output handle");
        // save existing cursor info
        CONSOLE_CURSOR_INFO info {};
        if (GetConsoleCursorInfo(hOut, &info)) savedCursorInfo = info;
        hideCursor();
    }

    ~Console() {
        // restore cursor
        if (hOut != INVALID_HANDLE_VALUE) SetConsoleCursorInfo(hOut, &savedCursorInfo);
    }

    void clear() {
        COORD topLeft {0, 0};
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
        DWORD len = static_cast<DWORD>(csbi.dwSize.X) * csbi.dwSize.Y;
        DWORD written = 0;
        FillConsoleOutputCharacterA(hOut, ' ', len, topLeft, &written);
        FillConsoleOutputAttribute(hOut, csbi.wAttributes, len, topLeft, &written);
        SetConsoleCursorPosition(hOut, topLeft);
    }

    void setCursor(int x, int y) {
        COORD coord { static_cast<SHORT>(x), static_cast<SHORT>(y) };
        SetConsoleCursorPosition(hOut, coord);
    }

    void hideCursor() {
        CONSOLE_CURSOR_INFO info {};
        info.bVisible = FALSE;
        info.dwSize = 1;
        SetConsoleCursorInfo(hOut, &info);
    }

    void writeAt(int x, int y, const std::string& s) {
        setCursor(x, y);
        std::cout << s;
    }

    static std::string bytesToHex(const uint8_t* data, size_t len) {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < len; ++i) {
            ss << std::setw(2) << static_cast<int>(data[i]) << ' ';
        }
        ss << std::dec;
        return ss.str();
    }

private:
    HANDLE hOut;
    CONSOLE_CURSOR_INFO savedCursorInfo {};
};

class PS4Visualizer {
public:
    PS4Visualizer() {
        registerWindowClass();
        createMessageWindow();
        registerRawInput();
        console.clear();
    }

    ~PS4Visualizer() {
        // unregister raw input listener
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
            // keyboard: ESC to quit
            if (_kbhit()) {
                int ch = _getch();
                if (ch == 27) { // ESC
                    done = true;
                    break;
                }
            }

            // process windows messages
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // small sleep to reduce CPU usage (~60 FPS)
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

private:
    // Window / raw input setup
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
        // create an invisible message-only window
        hwnd = CreateWindowW(windowClassName.c_str(), L"PS4RawInputHiddenWindow",
                             0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandle(nullptr), this);
        if (!hwnd) throw std::runtime_error("CreateWindow failed");
    }

    void registerRawInput() {
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01; // Generic Desktop
        rid.usUsage = 0x05;     // Game Pad
        rid.dwFlags = RIDEV_INPUTSINK;
        rid.hwndTarget = hwnd;

        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
            DWORD err = GetLastError();
            throw std::runtime_error("RegisterRawInputDevices failed: " + std::to_string(err));
        }
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        PS4Visualizer* self = nullptr;
        if (msg == WM_CREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<PS4Visualizer*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<PS4Visualizer*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
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

        RID_DEVICE_INFO rdi{};
        rdi.cbSize = sizeof(rdi);
        // (optional) We could call GetRawInputDeviceInfo to get more info if needed

        // raw->data.hid.dwSizeHid is the size of a single HID report; dwCount is number of reports
        if (raw->data.hid.dwSizeHid >= sizeof(PS4ControllerReport) && raw->data.hid.dwCount >= 1) {
            PS4ControllerReport report{};
            // copy the first report
            std::memcpy(&report, raw->data.hid.bRawData, sizeof(report));

            {
                std::lock_guard<std::mutex> lk(stateMutex);
                lastReport = report;
                controllerConnected = true;
            }
            updateDisplay();
        }
    }

    // Rendering helpers
    void printHeader() {
        console.setCursor(0, 0);
        std::cout << "=== PS4 Controller Raw Input Visualizer (refactored) ===\n";
        std::cout << "Press ESC to exit. Make sure controller is connected (USB/Bluetooth).\n\n";
        std::cout.flush();
    }

    void updateDisplay() {
        console.clear();
        printHeader();

        std::optional<PS4ControllerReport> snapshot;
        {
            std::lock_guard<std::mutex> lk(stateMutex);
            snapshot = lastReport;
        }

        if (!snapshot.has_value()) {
            console.writeAt(0, 4, "Waiting for controller data...");
            return;
        }

        const PS4ControllerReport& r = snapshot.value();

        drawStick(0, 4, r.leftStickX, r.leftStickY, "Left");
        drawStick(30, 4, r.rightStickX, r.rightStickY, "Right");

        drawTrigger(60, 4, r.leftTrigger, "L2");
        drawTrigger(60, 5, r.rightTrigger, "R2");

        console.writeAt(60, 7, "Battery: " + padNumber((int)r.battery, 3));

        drawButtons(0, 15, r);

        // raw hex dump (first N bytes of the report)
        constexpr size_t HEX_DUMP_BYTES = 24;
        console.writeAt(0, 18, "Raw Data: " + Console::bytesToHex(reinterpret_cast<const uint8_t*>(&r), min(sizeof(r), HEX_DUMP_BYTES)));
    }

    void drawStick(int x, int y, uint8_t rawX, uint8_t rawY, const std::string& name) {
        // scale to a small ascii grid
        constexpr int GRID_W = 11; // -5 .. +5
        constexpr int GRID_H = 5;  // -2 .. +2
        constexpr int HALF_W = 5;
        constexpr int HALF_H = 2;

        console.writeAt(x, y, name + " Stick:");

        // normalize from [0..255] to [-1..+1]
        auto norm = [](uint8_t v) -> float {
            // center is approximately 128
            return (static_cast<int>(v) - 128) / 127.0f; // [-1,1]
        };

        float nx = norm(rawX);
        float ny = norm(rawY);

        int posX = static_cast<int>(std::round(nx * HALF_W));
        int posY = static_cast<int>(std::round(ny * HALF_H));

        for (int row = -HALF_H; row <= HALF_H; ++row) {
            std::string line;
            line.reserve(GRID_W);
            for (int col = -HALF_W; col <= HALF_W; ++col) {
                if (col == posX && row == posY) line += "@"; // current stick position
                else if (col == 0 && row == 0) line += "+"; // center
                else line += ".";
            }
            console.writeAt(x, y + 1 + (row + HALF_H), line);
        }

        console.writeAt(x, y + 1 + GRID_H, "X: " + padNumber((int)rawX, 3) + " Y: " + padNumber((int)rawY, 3));
    }

    void drawTrigger(int x, int y, uint8_t value, const std::string& name) {
        int bars = (value * 10) / 255;
        std::ostringstream ss;
        ss << name << ": [";
        for (int i = 0; i < 10; ++i) ss << (i < bars ? "#" : ".");
        ss << "] " << std::setw(3) << (int)value;
        console.writeAt(x, y, ss.str());
    }

    void drawButtons(int x, int y, const PS4ControllerReport& r) {
        // Face buttons (upper nibble of buttons1)
        std::ostringstream ss1;
        ss1 << "Buttons: ";
        ss1 << (r.buttons1 & 0x10 ? "[SQR] " : " SQR  ");
        ss1 << (r.buttons1 & 0x20 ? "[CRO] " : " CRO  ");
        ss1 << (r.buttons1 & 0x40 ? "[CIR] " : " CIR  ");
        ss1 << (r.buttons1 & 0x80 ? "[TRI] " : " TRI  ");
        console.writeAt(x, y, ss1.str());

        // D-Pad (lower nibble)
        uint8_t dpad = r.buttons1 & 0x0F;
        std::string dpadStr = dpadToLabel(dpad);
        console.writeAt(x, y + 1, "D-Pad: " + dpadStr);

        // Shoulders and stick clicks
        std::ostringstream ss2;
        ss2 << (r.buttons2 & 0x01 ? "[L1] " : " L1  ");
        ss2 << (r.buttons2 & 0x02 ? "[R1] " : " R1  ");
        ss2 << (r.buttons2 & 0x40 ? "[L3] " : " L3  ");
        ss2 << (r.buttons2 & 0x80 ? "[R3] " : " R3  ");
        ss2 << " | ";
        ss2 << (r.buttons3 & 0x01 ? "[PS] " : " PS  ");
        ss2 << (r.buttons3 & 0x02 ? "[PAD] " : " PAD  ");
        ss2 << (r.buttons2 & 0x10 ? "[SHARE] " : " SHARE  ");
        ss2 << (r.buttons2 & 0x20 ? "[OPTIONS] " : " OPTIONS  ");

        console.writeAt(x, y + 2, ss2.str());
    }

    static std::string dpadToLabel(uint8_t d) {
        // 0..7 = directions, 8 = neutral (per many DualShock reports)
        switch (d) {
            case 0: return "Up";
            case 1: return "Up-Right";
            case 2: return "Right";
            case 3: return "Down-Right";
            case 4: return "Down";
            case 5: return "Down-Left";
            case 6: return "Left";
            case 7: return "Up-Left";
            default: return "Neutral";
        }
    }

    static std::string padNumber(int v, int w) {
        std::ostringstream ss;
        ss << std::setw(w) << v;
        return ss.str();
    }

private:
    HWND hwnd = nullptr;
    const std::wstring windowClassName = L"PS4RawInputClassRefactored";

    std::mutex stateMutex;
    std::optional<PS4ControllerReport> lastReport;
    bool controllerConnected = false;

    Console console;
};

int main() {
    try {
        PS4Visualizer viz;
        viz.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
