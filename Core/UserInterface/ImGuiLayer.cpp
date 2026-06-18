#include "UserInterface/ImGuiLayer.h"

#include "Rendering/DirectX12Renderer.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <implot.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND WindowHandle, UINT Message, WPARAM WordParameter, LPARAM LongParameter);

namespace Core {
    namespace UserInterface {
        ImGuiLayer::ImGuiLayer()
            : mRenderer{},
              mInitialized{} {
        }

        ImGuiLayer::~ImGuiLayer() {
            Shutdown();
        }

        bool ImGuiLayer::Initialize(HWND WindowHandle, Rendering::DirectX12Renderer& Renderer) {
            mRenderer = &Renderer;

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImPlot::CreateContext();

            ImGuiIO& Io{ ImGui::GetIO() };
            Io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            Io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            Io.IniFilename = nullptr;

            ImGui::StyleColorsDark();
            ImGuiStyle& Style{ ImGui::GetStyle() };
            Style.WindowRounding = 0.0f;
            Style.FrameRounding = 3.0f;
            Style.TabRounding = 3.0f;

            if (!ImGui_ImplWin32_Init(WindowHandle)) {
                ImPlot::DestroyContext();
                ImGui::DestroyContext();
                mRenderer = nullptr;
                return false;
            }

            ImGui_ImplWin32_EnableAlphaCompositing(WindowHandle);

            ImGui_ImplDX12_InitInfo InitInfo{};
            InitInfo.Device = Renderer.GetDevice();
            InitInfo.CommandQueue = Renderer.GetCommandQueue();
            InitInfo.NumFramesInFlight = Renderer.GetFrameCount();
            InitInfo.RTVFormat = Renderer.GetRenderTargetFormat();
            InitInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
            InitInfo.SrvDescriptorHeap = Renderer.GetShaderResourceDescriptorHeap();
            InitInfo.UserData = &Renderer;
            InitInfo.SrvDescriptorAllocFn = ImGuiLayer::AllocateShaderResourceDescriptor;
            InitInfo.SrvDescriptorFreeFn = ImGuiLayer::FreeShaderResourceDescriptor;

            if (!ImGui_ImplDX12_Init(&InitInfo)) {
                ImGui_ImplWin32_Shutdown();
                ImPlot::DestroyContext();
                ImGui::DestroyContext();
                mRenderer = nullptr;
                return false;
            }

            mInitialized = true;
            return true;
        }

        void ImGuiLayer::Shutdown() {
            if (!mInitialized) {
                return;
            }

            ImGui_ImplDX12_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImPlot::DestroyContext();
            ImGui::DestroyContext();
            mRenderer = nullptr;
            mInitialized = false;
        }

        bool ImGuiLayer::HandleWindowMessage(HWND WindowHandle, UINT Message, WPARAM WordParameter, LPARAM LongParameter) {
            if (!mInitialized) {
                return false;
            }

            return ImGui_ImplWin32_WndProcHandler(WindowHandle, Message, WordParameter, LongParameter) != 0;
        }

        void ImGuiLayer::BeginFrame() {
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        void ImGuiLayer::Draw() {
            const ImGuiViewport* MainViewport{ ImGui::GetMainViewport() };
            ImGui::SetNextWindowPos(MainViewport->WorkPos);
            ImGui::SetNextWindowSize(MainViewport->WorkSize);
            ImGui::SetNextWindowViewport(MainViewport->ID);

            const ImGuiWindowFlags WindowFlags{ ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse };
            bool Open{ true };

            ImGui::Begin("MarketDock", &Open, WindowFlags);
            const ImVec2 WindowPosition{ ImGui::GetWindowPos() };
            const ImVec2 WindowSize{ ImGui::GetWindowSize() };
            const ImVec2 WindowEnd{ WindowPosition.x + WindowSize.x, WindowPosition.y + WindowSize.y };
            ImDrawList* DrawList{ ImGui::GetWindowDrawList() };

            DrawList->AddRectFilled(WindowPosition, WindowEnd, IM_COL32(18, 22, 29, 232), 12.0f);
            DrawList->AddRect(WindowPosition, WindowEnd, IM_COL32(92, 116, 144, 190), 12.0f, 0, 1.0f);

            ImGui::SetCursorPos(ImVec2{ 18.0f, 16.0f });
            ImGui::TextUnformatted("MarketDock");
            ImGui::Separator();
            ImGui::TextUnformatted("Ready");
            ImGui::End();
        }

        void ImGuiLayer::EndFrame() {
            ImGui::Render();
        }

        ImDrawData* ImGuiLayer::GetDrawData() const {
            return ImGui::GetDrawData();
        }

        void ImGuiLayer::AllocateShaderResourceDescriptor(ImGui_ImplDX12_InitInfo* InitInfo, D3D12_CPU_DESCRIPTOR_HANDLE* CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* GpuHandle) {
            if (InitInfo == nullptr || InitInfo->UserData == nullptr) {
                return;
            }

            Rendering::DirectX12Renderer* Renderer{ static_cast<Rendering::DirectX12Renderer*>(InitInfo->UserData) };
            Renderer->AllocateShaderResourceDescriptor(CpuHandle, GpuHandle);
        }

        void ImGuiLayer::FreeShaderResourceDescriptor(ImGui_ImplDX12_InitInfo* InitInfo, D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle) {
            if (InitInfo == nullptr || InitInfo->UserData == nullptr) {
                return;
            }

            Rendering::DirectX12Renderer* Renderer{ static_cast<Rendering::DirectX12Renderer*>(InitInfo->UserData) };
            Renderer->FreeShaderResourceDescriptor(CpuHandle, GpuHandle);
        }
    }
}
