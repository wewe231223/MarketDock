#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d12.h>
#include <dxgiformat.h>

#include <memory>

struct ImDrawData;

namespace Core {
    namespace Rendering {
        class DirectX12Renderer {
        private:
            struct RendererState;

        public:
            DirectX12Renderer();
            ~DirectX12Renderer();

        public:
            bool Initialize(HWND WindowHandle);
            void Shutdown();
            void Resize(UINT Width, UINT Height);
            bool BeginFrame();
            void RenderImGui(ImDrawData* DrawData);
            void EndFrame();
            ID3D12Device* GetDevice() const;
            ID3D12CommandQueue* GetCommandQueue() const;
            ID3D12DescriptorHeap* GetShaderResourceDescriptorHeap() const;
            int GetFrameCount() const;
            DXGI_FORMAT GetRenderTargetFormat() const;
            void AllocateShaderResourceDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* GpuHandle);
            void FreeShaderResourceDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle);

        private:
            bool CreateDevice();
            bool CreateCommandObjects();
            bool CreateDescriptorHeaps();
            bool CreateSwapChain();
            bool CreateRenderTargets();
            void CleanupRenderTargets();
            void WaitForGpu();

        private:
            std::unique_ptr<RendererState> mRendererState;
        };
    }
}
