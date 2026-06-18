#include "Rendering/DirectX12Renderer.h"

#include <imgui_impl_dx12.h>

#include <wrl/client.h>
#include <array>
#include <algorithm>
#include <cstddef>
#include <dxgi1_6.h>

#if defined(_DEBUG)
#include <d3d12sdklayers.h>
#endif

namespace {
    constexpr UINT FrameCount{ 2 };
    constexpr UINT ShaderResourceDescriptorCount{ 64 };
    constexpr DXGI_FORMAT RenderTargetFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
}

namespace Core {
    namespace Rendering {
        struct DirectX12Renderer::RendererState {
            struct FrameContext {
                Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
                UINT64 mFenceValue;
            };

            HWND mWindowHandle{};
            UINT mWidth{};
            UINT mHeight{};
            UINT mFrameIndex{};
            UINT mRtvDescriptorSize{};
            UINT mSrvDescriptorSize{};
            UINT64 mFenceLastSignaledValue{};
            HANDLE mFenceEvent{};
            bool mFrameStarted{};
            D3D12_CPU_DESCRIPTOR_HANDLE mCurrentRenderTargetView{};
            std::array<FrameContext, FrameCount> mFrameContexts{};
            std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount> mRenderTargets{};
            std::array<bool, ShaderResourceDescriptorCount> mShaderResourceDescriptorUsed{};
            Microsoft::WRL::ComPtr<IDXGIFactory4> mFactory{};
            Microsoft::WRL::ComPtr<ID3D12Device> mDevice{};
            Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue{};
            Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain{};
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRenderTargetViewHeap{};
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mShaderResourceViewHeap{};
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList{};
            Microsoft::WRL::ComPtr<ID3D12Fence> mFence{};
        };

        DirectX12Renderer::DirectX12Renderer()
            : mRendererState{ std::make_unique<RendererState>() } {
        }

        DirectX12Renderer::~DirectX12Renderer() {
            Shutdown();
        }

        bool DirectX12Renderer::Initialize(HWND WindowHandle) {
            RendererState& State{ *mRendererState };
            State.mWindowHandle = WindowHandle;

            if (!CreateDevice()) {
                return false;
            }

            if (!CreateCommandObjects()) {
                return false;
            }

            if (!CreateDescriptorHeaps()) {
                return false;
            }

            if (!CreateSwapChain()) {
                return false;
            }

            return CreateRenderTargets();
        }

        void DirectX12Renderer::Shutdown() {
            if (mRendererState == nullptr) {
                return;
            }

            RendererState& State{ *mRendererState };
            WaitForGpu();
            CleanupRenderTargets();
            State.mCommandList.Reset();

            for (RendererState::FrameContext& FrameContext : State.mFrameContexts) {
                FrameContext.mCommandAllocator.Reset();
                FrameContext.mFenceValue = 0;
            }

            State.mFence.Reset();
            State.mSwapChain.Reset();
            State.mCommandQueue.Reset();
            State.mShaderResourceViewHeap.Reset();
            State.mRenderTargetViewHeap.Reset();
            State.mDevice.Reset();
            State.mFactory.Reset();
            State.mShaderResourceDescriptorUsed.fill(false);

            if (State.mFenceEvent != nullptr) {
                CloseHandle(State.mFenceEvent);
                State.mFenceEvent = nullptr;
            }
        }

        void DirectX12Renderer::Resize(UINT Width, UINT Height) {
            if (Width == 0 || Height == 0) {
                return;
            }

            RendererState& State{ *mRendererState };

            if (State.mSwapChain == nullptr) {
                return;
            }

            WaitForGpu();
            CleanupRenderTargets();

            State.mWidth = Width;
            State.mHeight = Height;

            const HRESULT ResizeResult{ State.mSwapChain->ResizeBuffers(FrameCount, State.mWidth, State.mHeight, RenderTargetFormat, 0) };

            if (FAILED(ResizeResult)) {
                return;
            }

            State.mFrameIndex = State.mSwapChain->GetCurrentBackBufferIndex();
            CreateRenderTargets();
        }

        bool DirectX12Renderer::BeginFrame() {
            RendererState& State{ *mRendererState };

            if (State.mSwapChain == nullptr || State.mCommandList == nullptr) {
                return false;
            }

            State.mFrameIndex = State.mSwapChain->GetCurrentBackBufferIndex();
            RendererState::FrameContext& FrameContext{ State.mFrameContexts[State.mFrameIndex] };

            if (FrameContext.mFenceValue != 0 && State.mFence->GetCompletedValue() < FrameContext.mFenceValue) {
                State.mFence->SetEventOnCompletion(FrameContext.mFenceValue, State.mFenceEvent);
                WaitForSingleObject(State.mFenceEvent, INFINITE);
            }

            FrameContext.mFenceValue = 0;

            if (FAILED(FrameContext.mCommandAllocator->Reset())) {
                return false;
            }

            if (FAILED(State.mCommandList->Reset(FrameContext.mCommandAllocator.Get(), nullptr))) {
                return false;
            }

            D3D12_RESOURCE_BARRIER ResourceBarrier{};
            ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            ResourceBarrier.Transition.pResource = State.mRenderTargets[State.mFrameIndex].Get();
            ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            State.mCommandList->ResourceBarrier(1, &ResourceBarrier);

            D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView{ State.mRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart() };
            RenderTargetView.ptr += static_cast<SIZE_T>(State.mFrameIndex) * State.mRtvDescriptorSize;
            State.mCurrentRenderTargetView = RenderTargetView;

            const float ClearColor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
            State.mCommandList->OMSetRenderTargets(1, &State.mCurrentRenderTargetView, FALSE, nullptr);
            State.mCommandList->ClearRenderTargetView(State.mCurrentRenderTargetView, ClearColor, 0, nullptr);
            State.mFrameStarted = true;
            return true;
        }

        void DirectX12Renderer::RenderImGui(ImDrawData* DrawData) {
            RendererState& State{ *mRendererState };

            if (!State.mFrameStarted || DrawData == nullptr) {
                return;
            }

            ID3D12DescriptorHeap* DescriptorHeaps[]{ State.mShaderResourceViewHeap.Get() };
            State.mCommandList->SetDescriptorHeaps(1, DescriptorHeaps);
            ImGui_ImplDX12_RenderDrawData(DrawData, State.mCommandList.Get());
        }

        void DirectX12Renderer::EndFrame() {
            RendererState& State{ *mRendererState };

            if (!State.mFrameStarted) {
                return;
            }

            D3D12_RESOURCE_BARRIER ResourceBarrier{};
            ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            ResourceBarrier.Transition.pResource = State.mRenderTargets[State.mFrameIndex].Get();
            ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            State.mCommandList->ResourceBarrier(1, &ResourceBarrier);
            State.mCommandList->Close();

            ID3D12CommandList* CommandLists[]{ State.mCommandList.Get() };
            State.mCommandQueue->ExecuteCommandLists(1, CommandLists);
            State.mSwapChain->Present(1, 0);

            const UINT64 FenceValue{ State.mFenceLastSignaledValue + 1 };
            State.mCommandQueue->Signal(State.mFence.Get(), FenceValue);
            State.mFenceLastSignaledValue = FenceValue;
            State.mFrameContexts[State.mFrameIndex].mFenceValue = FenceValue;
            State.mFrameStarted = false;
        }

        ID3D12Device* DirectX12Renderer::GetDevice() const {
            return mRendererState->mDevice.Get();
        }

        ID3D12CommandQueue* DirectX12Renderer::GetCommandQueue() const {
            return mRendererState->mCommandQueue.Get();
        }

        ID3D12DescriptorHeap* DirectX12Renderer::GetShaderResourceDescriptorHeap() const {
            return mRendererState->mShaderResourceViewHeap.Get();
        }

        int DirectX12Renderer::GetFrameCount() const {
            return static_cast<int>(FrameCount);
        }

        DXGI_FORMAT DirectX12Renderer::GetRenderTargetFormat() const {
            return RenderTargetFormat;
        }

        void DirectX12Renderer::AllocateShaderResourceDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* GpuHandle) {
            if (CpuHandle == nullptr || GpuHandle == nullptr) {
                return;
            }

            RendererState& State{ *mRendererState };
            const D3D12_CPU_DESCRIPTOR_HANDLE BaseCpuHandle{ State.mShaderResourceViewHeap->GetCPUDescriptorHandleForHeapStart() };
            const D3D12_GPU_DESCRIPTOR_HANDLE BaseGpuHandle{ State.mShaderResourceViewHeap->GetGPUDescriptorHandleForHeapStart() };

            for (UINT Index{}; Index < ShaderResourceDescriptorCount; ++Index) {
                if (!State.mShaderResourceDescriptorUsed[Index]) {
                    State.mShaderResourceDescriptorUsed[Index] = true;
                    CpuHandle->ptr = BaseCpuHandle.ptr + static_cast<SIZE_T>(Index) * State.mSrvDescriptorSize;
                    GpuHandle->ptr = BaseGpuHandle.ptr + static_cast<UINT64>(Index) * State.mSrvDescriptorSize;
                    return;
                }
            }

            *CpuHandle = D3D12_CPU_DESCRIPTOR_HANDLE{};
            *GpuHandle = D3D12_GPU_DESCRIPTOR_HANDLE{};
        }

        void DirectX12Renderer::FreeShaderResourceDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle) {
            RendererState& State{ *mRendererState };
            const D3D12_CPU_DESCRIPTOR_HANDLE BaseCpuHandle{ State.mShaderResourceViewHeap->GetCPUDescriptorHandleForHeapStart() };
            UNREFERENCED_PARAMETER(GpuHandle);

            if (CpuHandle.ptr < BaseCpuHandle.ptr || State.mSrvDescriptorSize == 0) {
                return;
            }

            const SIZE_T Offset{ CpuHandle.ptr - BaseCpuHandle.ptr };
            const UINT Index{ static_cast<UINT>(Offset / State.mSrvDescriptorSize) };

            if (Index < ShaderResourceDescriptorCount) {
                State.mShaderResourceDescriptorUsed[Index] = false;
            }
        }

        bool DirectX12Renderer::CreateDevice() {
            RendererState& State{ *mRendererState };
            UINT FactoryFlags{};

#if defined(_DEBUG)
            Microsoft::WRL::ComPtr<ID3D12Debug> DebugController{};

            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController)))) {
                DebugController->EnableDebugLayer();
                FactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
#endif

            if (FAILED(CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(&State.mFactory)))) {
                return false;
            }

            if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&State.mDevice)))) {
                return true;
            }

            Microsoft::WRL::ComPtr<IDXGIAdapter> WarpAdapter{};

            if (FAILED(State.mFactory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter)))) {
                return false;
            }

            return SUCCEEDED(D3D12CreateDevice(WarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&State.mDevice)));
        }

        bool DirectX12Renderer::CreateCommandObjects() {
            RendererState& State{ *mRendererState };
            D3D12_COMMAND_QUEUE_DESC CommandQueueDescription{};
            CommandQueueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            CommandQueueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

            if (FAILED(State.mDevice->CreateCommandQueue(&CommandQueueDescription, IID_PPV_ARGS(&State.mCommandQueue)))) {
                return false;
            }

            for (RendererState::FrameContext& FrameContext : State.mFrameContexts) {
                if (FAILED(State.mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&FrameContext.mCommandAllocator)))) {
                    return false;
                }
            }

            if (FAILED(State.mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, State.mFrameContexts[0].mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&State.mCommandList)))) {
                return false;
            }

            State.mCommandList->Close();

            if (FAILED(State.mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&State.mFence)))) {
                return false;
            }

            State.mFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            return State.mFenceEvent != nullptr;
        }

        bool DirectX12Renderer::CreateDescriptorHeaps() {
            RendererState& State{ *mRendererState };
            D3D12_DESCRIPTOR_HEAP_DESC RenderTargetHeapDescription{};
            RenderTargetHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            RenderTargetHeapDescription.NumDescriptors = FrameCount;
            RenderTargetHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

            if (FAILED(State.mDevice->CreateDescriptorHeap(&RenderTargetHeapDescription, IID_PPV_ARGS(&State.mRenderTargetViewHeap)))) {
                return false;
            }

            D3D12_DESCRIPTOR_HEAP_DESC ShaderResourceHeapDescription{};
            ShaderResourceHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            ShaderResourceHeapDescription.NumDescriptors = ShaderResourceDescriptorCount;
            ShaderResourceHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            if (FAILED(State.mDevice->CreateDescriptorHeap(&ShaderResourceHeapDescription, IID_PPV_ARGS(&State.mShaderResourceViewHeap)))) {
                return false;
            }

            State.mRtvDescriptorSize = State.mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            State.mSrvDescriptorSize = State.mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            return true;
        }

        bool DirectX12Renderer::CreateSwapChain() {
            RendererState& State{ *mRendererState };
            RECT ClientRect{};
            GetClientRect(State.mWindowHandle, &ClientRect);
            State.mWidth = std::max<UINT>(1, static_cast<UINT>(ClientRect.right - ClientRect.left));
            State.mHeight = std::max<UINT>(1, static_cast<UINT>(ClientRect.bottom - ClientRect.top));

            DXGI_SWAP_CHAIN_DESC1 SwapChainDescription{};
            SwapChainDescription.BufferCount = FrameCount;
            SwapChainDescription.Width = State.mWidth;
            SwapChainDescription.Height = State.mHeight;
            SwapChainDescription.Format = RenderTargetFormat;
            SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            SwapChainDescription.SampleDesc.Count = 1;
            SwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            SwapChainDescription.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

            Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain{};

            if (FAILED(State.mFactory->CreateSwapChainForHwnd(State.mCommandQueue.Get(), State.mWindowHandle, &SwapChainDescription, nullptr, nullptr, &SwapChain))) {
                return false;
            }

            State.mFactory->MakeWindowAssociation(State.mWindowHandle, DXGI_MWA_NO_ALT_ENTER);

            if (FAILED(SwapChain.As(&State.mSwapChain))) {
                return false;
            }

            State.mFrameIndex = State.mSwapChain->GetCurrentBackBufferIndex();
            return true;
        }

        bool DirectX12Renderer::CreateRenderTargets() {
            RendererState& State{ *mRendererState };
            D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetViewHandle{ State.mRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart() };

            for (UINT Index{}; Index < FrameCount; ++Index) {
                if (FAILED(State.mSwapChain->GetBuffer(Index, IID_PPV_ARGS(&State.mRenderTargets[Index])))) {
                    return false;
                }

                State.mDevice->CreateRenderTargetView(State.mRenderTargets[Index].Get(), nullptr, RenderTargetViewHandle);
                RenderTargetViewHandle.ptr += State.mRtvDescriptorSize;
            }

            return true;
        }

        void DirectX12Renderer::CleanupRenderTargets() {
            RendererState& State{ *mRendererState };

            for (Microsoft::WRL::ComPtr<ID3D12Resource>& RenderTarget : State.mRenderTargets) {
                RenderTarget.Reset();
            }
        }

        void DirectX12Renderer::WaitForGpu() {
            RendererState& State{ *mRendererState };

            if (State.mCommandQueue == nullptr || State.mFence == nullptr || State.mFenceEvent == nullptr) {
                return;
            }

            const UINT64 FenceValue{ State.mFenceLastSignaledValue + 1 };

            if (FAILED(State.mCommandQueue->Signal(State.mFence.Get(), FenceValue))) {
                return;
            }

            State.mFenceLastSignaledValue = FenceValue;

            if (State.mFence->GetCompletedValue() < FenceValue) {
                State.mFence->SetEventOnCompletion(FenceValue, State.mFenceEvent);
                WaitForSingleObject(State.mFenceEvent, INFINITE);
            }

            for (RendererState::FrameContext& FrameContext : State.mFrameContexts) {
                FrameContext.mFenceValue = 0;
            }
        }
    }
}
