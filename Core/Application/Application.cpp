#include "Application/Application.h"

#include "Rendering/DirectX12Renderer.h"
#include "UserInterface/ImGuiLayer.h"

#include <dwmapi.h>

namespace {
    constexpr wchar_t WindowClassName[]{ L"MarketDockWindow" };
    constexpr wchar_t WindowTitle[]{ L"MarketDock" };
    constexpr int InitialWindowWidth{ 360 };
    constexpr int InitialWindowHeight{ 220 };
    constexpr int WindowCornerRadius{ 12 };
    constexpr int WindowCornerDiameter{ WindowCornerRadius * 2 };
    constexpr BYTE WindowAlpha{ 255 };
}

namespace Core {
    Application::Application(HINSTANCE Instance, int ShowCommand)
        : mInstance{ Instance },
          mShowCommand{ ShowCommand },
          mWindowHandle{},
          mRenderer{},
          mImGuiLayer{},
          mRunning{},
          mClickThrough{} {
    }

    Application::~Application() {
        Shutdown();
    }

    int Application::Run() {
        if (!Initialize()) {
            Shutdown();
            return -1;
        }

        MSG Message{};

        while (mRunning) {
            while (PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE)) {
                if (Message.message == WM_QUIT) {
                    mRunning = false;
                    break;
                }

                TranslateMessage(&Message);
                DispatchMessageW(&Message);
            }

            if (mRunning) {
                Render();
            }
        }

        const int ExitCode{ static_cast<int>(Message.wParam) };
        Shutdown();
        return ExitCode;
    }

    bool Application::Initialize() {
        if (!CreateMainWindow()) {
            return false;
        }

        mRenderer = std::make_unique<Rendering::DirectX12Renderer>();

        if (!mRenderer->Initialize(mWindowHandle)) {
            return false;
        }

        mImGuiLayer = std::make_unique<UserInterface::ImGuiLayer>();

        if (!mImGuiLayer->Initialize(mWindowHandle, *mRenderer)) {
            return false;
        }

        ShowWindow(mWindowHandle, mShowCommand);
        UpdateWindow(mWindowHandle);
        mRunning = true;
        return true;
    }

    void Application::Shutdown() {
        mRunning = false;

        if (mImGuiLayer != nullptr) {
            mImGuiLayer->Shutdown();
            mImGuiLayer.reset();
        }

        if (mRenderer != nullptr) {
            mRenderer->Shutdown();
            mRenderer.reset();
        }

        DestroyMainWindow();
    }

    bool Application::CreateMainWindow() {
        WNDCLASSEXW WindowClass{};
        WindowClass.cbSize = sizeof(WindowClass);
        WindowClass.style = CS_HREDRAW | CS_VREDRAW;
        WindowClass.lpfnWndProc = Application::WindowProcedure;
        WindowClass.hInstance = mInstance;
        WindowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        WindowClass.lpszClassName = WindowClassName;

        const ATOM WindowClassAtom{ RegisterClassExW(&WindowClass) };

        if (WindowClassAtom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }

        const DWORD WindowStyle{ WS_POPUP };
        const DWORD WindowExtendedStyle{ WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST };
        RECT WindowRect{ 0, 0, InitialWindowWidth, InitialWindowHeight };

        if (!AdjustWindowRectEx(&WindowRect, WindowStyle, FALSE, WindowExtendedStyle)) {
            return false;
        }

        const int WindowWidth{ WindowRect.right - WindowRect.left };
        const int WindowHeight{ WindowRect.bottom - WindowRect.top };
        const int WindowX{ (GetSystemMetrics(SM_CXSCREEN) - WindowWidth) / 2 };
        const int WindowY{ (GetSystemMetrics(SM_CYSCREEN) - WindowHeight) / 2 };

        mWindowHandle = CreateWindowExW(WindowExtendedStyle, WindowClassName, WindowTitle, WindowStyle, WindowX, WindowY, WindowWidth, WindowHeight, nullptr, nullptr, mInstance, this);

        if (mWindowHandle == nullptr) {
            return false;
        }

        return ApplyWidgetWindowStyle();
    }

    void Application::DestroyMainWindow() {
        if (mWindowHandle != nullptr) {
            HWND WindowHandle{ mWindowHandle };
            mWindowHandle = nullptr;
            DestroyWindow(WindowHandle);
        }

        UnregisterClassW(WindowClassName, mInstance);
    }

    bool Application::ApplyWidgetWindowStyle() {
        if (!SetLayeredWindowAttributes(mWindowHandle, 0, WindowAlpha, LWA_ALPHA)) {
            return false;
        }

        MARGINS Margins{ -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(mWindowHandle, &Margins);

        if (!ApplyRoundedWindowRegion()) {
            return false;
        }

        SetClickThrough(false);
        SetWindowPos(mWindowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        return true;
    }

    bool Application::ApplyRoundedWindowRegion() {
        if (mWindowHandle == nullptr) {
            return false;
        }

        RECT WindowRect{};

        if (!GetWindowRect(mWindowHandle, &WindowRect)) {
            return false;
        }

        const int WindowWidth{ WindowRect.right - WindowRect.left };
        const int WindowHeight{ WindowRect.bottom - WindowRect.top };
        HRGN WindowRegion{ CreateRoundRectRgn(0, 0, WindowWidth + 1, WindowHeight + 1, WindowCornerDiameter, WindowCornerDiameter) };

        if (WindowRegion == nullptr) {
            return false;
        }

        if (SetWindowRgn(mWindowHandle, WindowRegion, TRUE) == 0) {
            DeleteObject(WindowRegion);
            return false;
        }

        return true;
    }

    void Application::SetClickThrough(bool Enabled) {
        LONG_PTR ExtendedStyle{ GetWindowLongPtrW(mWindowHandle, GWL_EXSTYLE) };

        if (Enabled) {
            ExtendedStyle |= WS_EX_TRANSPARENT;
        }
        else {
            ExtendedStyle &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
        }

        SetWindowLongPtrW(mWindowHandle, GWL_EXSTYLE, ExtendedStyle);
        SetWindowPos(mWindowHandle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        mClickThrough = Enabled;
    }

    void Application::Render() {
        if (mRenderer == nullptr || mImGuiLayer == nullptr) {
            return;
        }

        mImGuiLayer->BeginFrame();
        mImGuiLayer->Draw();
        mImGuiLayer->EndFrame();

        if (mRenderer->BeginFrame()) {
            mRenderer->RenderImGui(mImGuiLayer->GetDrawData());
            mRenderer->EndFrame();
        }
    }

    LRESULT Application::HandleWindowMessage(HWND WindowHandle, UINT Message, WPARAM WordParameter, LPARAM LongParameter) {
        if (mImGuiLayer != nullptr && mImGuiLayer->HandleWindowMessage(WindowHandle, Message, WordParameter, LongParameter)) {
            return 1;
        }

        switch (Message) {
        case WM_NCHITTEST:
            if (mClickThrough) {
                return HTTRANSPARENT;
            }
            break;
        case WM_KEYDOWN:
            if (WordParameter == VK_F8) {
                SetClickThrough(!mClickThrough);
                return 0;
            }
            break;
        case WM_SIZE:
            if (mRenderer != nullptr && WordParameter != SIZE_MINIMIZED) {
                const UINT Width{ static_cast<UINT>(LOWORD(LongParameter)) };
                const UINT Height{ static_cast<UINT>(HIWORD(LongParameter)) };
                mRenderer->Resize(Width, Height);
            }

            if (mWindowHandle != nullptr && WordParameter != SIZE_MINIMIZED) {
                ApplyRoundedWindowRegion();
            }

            return 0;
        case WM_SYSCOMMAND:
            if ((WordParameter & 0xfff0) == SC_KEYMENU) {
                return 0;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(WindowHandle);
            return 0;
        case WM_DESTROY:
            mRunning = false;
            PostQuitMessage(0);
            return 0;
        case WM_NCDESTROY:
            SetWindowLongPtrW(WindowHandle, GWLP_USERDATA, 0);
            if (mWindowHandle == WindowHandle) {
                mWindowHandle = nullptr;
            }
            break;
        default:
            break;
        }

        return DefWindowProcW(WindowHandle, Message, WordParameter, LongParameter);
    }

    LRESULT CALLBACK Application::WindowProcedure(HWND WindowHandle, UINT Message, WPARAM WordParameter, LPARAM LongParameter) {
        Application* ApplicationPointer{};

        if (Message == WM_NCCREATE) {
            CREATESTRUCTW* CreateStruct{ reinterpret_cast<CREATESTRUCTW*>(LongParameter) };
            ApplicationPointer = static_cast<Application*>(CreateStruct->lpCreateParams);
            SetWindowLongPtrW(WindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ApplicationPointer));
        }
        else {
            ApplicationPointer = reinterpret_cast<Application*>(GetWindowLongPtrW(WindowHandle, GWLP_USERDATA));
        }

        if (ApplicationPointer != nullptr) {
            return ApplicationPointer->HandleWindowMessage(WindowHandle, Message, WordParameter, LongParameter);
        }

        return DefWindowProcW(WindowHandle, Message, WordParameter, LongParameter);
    }
}
