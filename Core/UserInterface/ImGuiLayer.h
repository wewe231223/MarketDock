#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d12.h>

struct ImDrawData;
struct ImGui_ImplDX12_InitInfo;

namespace Core {
    namespace Rendering {
        class DirectX12Renderer;
    }

    namespace UserInterface {
        class ImGuiLayer {
        public:
            ImGuiLayer();
            ~ImGuiLayer();

        public:
            bool Initialize(HWND WindowHandle, Rendering::DirectX12Renderer& Renderer);
            void Shutdown();
            bool HandleWindowMessage(HWND WindowHandle, UINT Message, WPARAM WordParameter, LPARAM LongParameter);
            void BeginFrame();
            void Draw();
            void EndFrame();
            ImDrawData* GetDrawData() const;

        private:
            static void AllocateShaderResourceDescriptor(ImGui_ImplDX12_InitInfo* InitInfo, D3D12_CPU_DESCRIPTOR_HANDLE* CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* GpuHandle);
            static void FreeShaderResourceDescriptor(ImGui_ImplDX12_InitInfo* InitInfo, D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle);

        private:
            Rendering::DirectX12Renderer* mRenderer;
            bool mInitialized;
        };
    }
}
