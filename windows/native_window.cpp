#include "include/window_manager/window_manager_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <codecvt>
#include <map>
#include <memory>
#include <sstream>

#define STATE_NORMAL                0
#define STATE_MAXIMIZED             1
#define STATE_MINIMIZED             2
#define STATE_FULLSCREEN_ENTERED    3

namespace {
    class NativeWindow : public flutter::Plugin {
    public:
        NativeWindow(flutter::PluginRegistrarWindows* registrar);

        virtual ~NativeWindow();

        int last_state = STATE_NORMAL;
        
        // The minimum size set by the platform channel.
        POINT minimum_size = { 0, 0 };
        // The maximum size set by the platform channel.
        POINT maximum_size = { -1, -1 };

        HWND GetMainWindow();
        
        void NativeWindow::Focus();
        void NativeWindow::Blur();
        void NativeWindow::Show();
        void NativeWindow::Hide();
        bool NativeWindow::IsVisible();
        bool NativeWindow::IsMaximized();
        void NativeWindow::Maximize();
        void NativeWindow::Unmaximize();
        bool NativeWindow::IsMinimized();
        void NativeWindow::Minimize();
        void NativeWindow::Restore();
        bool NativeWindow::IsFullScreen();
        void NativeWindow::SetFullScreen(const flutter::EncodableMap& args);
        flutter::EncodableMap NativeWindow::GetBounds(const flutter::EncodableMap& args);
        void NativeWindow::SetBounds(const flutter::EncodableMap& args);
        void NativeWindow::SetMinimumSize(const flutter::EncodableMap& args);
        void NativeWindow::SetMaximumSize(const flutter::EncodableMap& args);
        bool NativeWindow::IsAlwaysOnTop();
        void NativeWindow::SetAlwaysOnTop(const flutter::EncodableMap& args);
        std::string NativeWindow::GetTitle();
        void NativeWindow::SetTitle(const flutter::EncodableMap& args);
        void NativeWindow::StartDragging();
        void NativeWindow::Terminate();

    private:
        flutter::PluginRegistrarWindows* registrar;

        bool g_is_window_fullscreen = false;
        RECT g_frame_before_fullscreen;
    };


    NativeWindow::NativeWindow(flutter::PluginRegistrarWindows* registrar) {
        this->registrar = registrar;
    }

    NativeWindow::~NativeWindow() {}

    HWND NativeWindow::GetMainWindow() {
        return ::GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);
    }

    void NativeWindow::Focus() {
        SetForegroundWindow(GetMainWindow());
    }

    void NativeWindow::Blur() {
    }

    void NativeWindow::Show() {
        ShowWindowAsync(GetMainWindow(), SW_SHOW);
        SetForegroundWindow(GetMainWindow());
    }

    void NativeWindow::Hide() {
        ShowWindow(GetMainWindow(), SW_HIDE);
    }

    bool NativeWindow::IsVisible() {
        bool isVisible = IsWindowVisible(GetMainWindow());
        return isVisible;
    }

    bool NativeWindow::IsMaximized() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        return windowPlacement.showCmd == SW_MAXIMIZE;
    }

    void NativeWindow::Maximize() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_MAXIMIZE) {
            windowPlacement.showCmd = SW_MAXIMIZE;
            SetWindowPlacement(mainWindow, &windowPlacement);
        }
    }

    void NativeWindow::Unmaximize() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_NORMAL) {
            windowPlacement.showCmd = SW_NORMAL;
            SetWindowPlacement(mainWindow, &windowPlacement);
        }
    }

    bool NativeWindow::IsMinimized() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        return windowPlacement.showCmd == SW_SHOWMINIMIZED;
    }

    void NativeWindow::Minimize() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_SHOWMINIMIZED) {
            windowPlacement.showCmd = SW_SHOWMINIMIZED;
            SetWindowPlacement(mainWindow, &windowPlacement);
        }
    }

    void NativeWindow::Restore() {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_NORMAL) {
            windowPlacement.showCmd = SW_NORMAL;
            SetWindowPlacement(mainWindow, &windowPlacement);
        }
    }

    bool NativeWindow::IsFullScreen() {
        return g_is_window_fullscreen;
    }

    void NativeWindow::SetFullScreen(const flutter::EncodableMap& args) {
        bool isFullScreen = std::get<bool>(args.at(flutter::EncodableValue("isFullScreen")));

        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        // https://github.com/alexmercerind/flutter-desktop-embedding/blob/da98a3b5a0e2b9425fbcb2a3e4b4ba50754abf93/plugins/window_size/windows/window_size_plugin.cpp#L258
        if (isFullScreen) {
            HMONITOR monitor = ::MonitorFromWindow(mainWindow, MONITOR_DEFAULTTONEAREST);
            MONITORINFO info;
            info.cbSize = sizeof(MONITORINFO);
            ::GetMonitorInfo(monitor, &info);
            ::SetWindowLongPtr(mainWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            ::GetWindowRect(mainWindow, &g_frame_before_fullscreen);
            ::SetWindowPos(
                mainWindow, NULL, info.rcMonitor.left, info.rcMonitor.top,
                info.rcMonitor.right - info.rcMonitor.left,
                info.rcMonitor.bottom - info.rcMonitor.top, 
                SWP_SHOWWINDOW);
            ::ShowWindow(mainWindow, SW_MAXIMIZE);
        }
        else {
            g_is_window_fullscreen = false;
            ::SetWindowLongPtr(mainWindow, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
            ::SetWindowPos(
                mainWindow, NULL, g_frame_before_fullscreen.left,
                g_frame_before_fullscreen.top,
                g_frame_before_fullscreen.right - g_frame_before_fullscreen.left,
                g_frame_before_fullscreen.bottom - g_frame_before_fullscreen.top,
                SWP_SHOWWINDOW);
            ::ShowWindow(mainWindow, SW_RESTORE);
        }
    }

    flutter::EncodableMap NativeWindow::GetBounds(const flutter::EncodableMap& args) {
        double devicePixelRatio = std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));

        flutter::EncodableMap resultMap = flutter::EncodableMap();
        RECT rect;
        if (GetWindowRect(GetMainWindow(), &rect)) {
            double x = rect.left / devicePixelRatio * 1.0f;
            double y = rect.top / devicePixelRatio * 1.0f;
            double width = (rect.right - rect.left) / devicePixelRatio * 1.0f;
            double height = (rect.bottom - rect.top) / devicePixelRatio * 1.0f;

            resultMap[flutter::EncodableValue("x")] = flutter::EncodableValue(x);
            resultMap[flutter::EncodableValue("y")] = flutter::EncodableValue(y);
            resultMap[flutter::EncodableValue("width")] = flutter::EncodableValue(width);
            resultMap[flutter::EncodableValue("height")] = flutter::EncodableValue(height);
        }
        return resultMap;
    }

    void NativeWindow::SetBounds(const flutter::EncodableMap& args) {
        double devicePixelRatio = std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));
        double x = std::get<double>(args.at(flutter::EncodableValue("x")));
        double y = std::get<double>(args.at(flutter::EncodableValue("y")));
        double width = std::get<double>(args.at(flutter::EncodableValue("width")));
        double height = std::get<double>(args.at(flutter::EncodableValue("height")));

        SetWindowPos(
            GetMainWindow(),
            HWND_TOP,
            int(x * devicePixelRatio),
            int(y * devicePixelRatio),
            int(width * devicePixelRatio),
            int(height * devicePixelRatio),
            SWP_SHOWWINDOW);
    }

    void NativeWindow::SetMinimumSize(const flutter::EncodableMap& args) {
        double devicePixelRatio = std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));
        double width = std::get<double>(args.at(flutter::EncodableValue("width")));
        double height = std::get<double>(args.at(flutter::EncodableValue("height")));

        if (width >= 0 && height >= 0) {
            POINT point = {};
            point.x = static_cast<LONG>(width * devicePixelRatio);
            point.y = static_cast<LONG>(height * devicePixelRatio);
            minimum_size = point;
        }
    }

    void NativeWindow::SetMaximumSize(const flutter::EncodableMap& args) {
        double devicePixelRatio = std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));
        double width = std::get<double>(args.at(flutter::EncodableValue("width")));
        double height = std::get<double>(args.at(flutter::EncodableValue("height")));

        if (width >= 0 && height >= 0) {
            POINT point = {};
            point.x = static_cast<LONG>(width * devicePixelRatio);
            point.y = static_cast<LONG>(height * devicePixelRatio);
            maximum_size = point;
        }
    }

    bool NativeWindow::IsAlwaysOnTop() {
        DWORD dwExStyle = GetWindowLong(GetMainWindow(), GWL_EXSTYLE);
        return (dwExStyle & WS_EX_TOPMOST) != 0;
    }

    void NativeWindow::SetAlwaysOnTop(const flutter::EncodableMap& args) {
        bool isAlwaysOnTop = std::get<bool>(args.at(flutter::EncodableValue("isAlwaysOnTop")));
        SetWindowPos(GetMainWindow(), isAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    
    std::string NativeWindow::GetTitle() {
        int const bufferSize = 1 + GetWindowTextLength(GetMainWindow());
        std::wstring title( bufferSize, L'\0' );
        GetWindowText(GetMainWindow(), &title[0], bufferSize);

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return (converter.to_bytes(title)).c_str();
    }
    
    void NativeWindow::SetTitle(const flutter::EncodableMap& args) {
        std::string title = std::get<std::string>(args.at(flutter::EncodableValue("title")));

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        SetWindowText(GetMainWindow(), converter.from_bytes(title).c_str());

        int style = GetWindowLong(GetMainWindow(), GWL_STYLE);
        SetWindowLong(GetMainWindow(), GWL_STYLE, (int)(style & ~(WS_CAPTION)));
    }

    void NativeWindow::StartDragging() {
        ReleaseCapture();
        SendMessage(GetMainWindow(), WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
    }

    void NativeWindow::Terminate() {
        ExitProcess(1);
    }
} // namespace
