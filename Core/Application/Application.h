#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <memory>

namespace Core {
    namespace Rendering {
        class DirectX12Renderer;
    }

    namespace UserInterface {
        class ImGuiLayer;
    }

    class Application {
    public:
        Application(HINSTANCE Instance, int ShowCommand);
        ~Application();

    public:
        int Run();

    private:
        bool Initialize();
        void Shutdown();
        bool CreateMainWindow();
        void DestroyMainWindow();
        bool ApplyWidgetWindowStyle();
        bool ApplyRoundedWindowRegion();
        void SetClickThrough(bool Enabled);
        void Render();
        LRESULT HandleWindowMessage(HWND WindowHandle, UINT Message, WPARAM WordParameter, LPARAM LongParameter);
        static LRESULT CALLBACK WindowProcedure(HWND WindowHandle, UINT Message, WPARAM WordParameter, LPARAM LongParameter);

    private:
        HINSTANCE mInstance;
        int mShowCommand;
        HWND mWindowHandle;
        std::unique_ptr<Rendering::DirectX12Renderer> mRenderer;
        std::unique_ptr<UserInterface::ImGuiLayer> mImGuiLayer;
        bool mRunning;
        bool mClickThrough;
    };
}
