cmake_minimum_required(VERSION 3.21)

project(MarketDock VERSION 0.1.0 LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "MarketDock supports Windows only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

if(MSVC)
    add_compile_options(/W3 /permissive- /sdl /utf-8)
endif()

add_compile_definitions(UNICODE _UNICODE)

set(MarketDockVcpkgTriplet "x64-windows")

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(MarketDockVcpkgTriplet "x86-windows")
endif()

set(MarketDockVcpkgPrefix "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${MarketDockVcpkgTriplet}")

if(EXISTS "${MarketDockVcpkgPrefix}/${MarketDockVcpkgTriplet}/include")
    set(MarketDockVcpkgPrefix "${MarketDockVcpkgPrefix}/${MarketDockVcpkgTriplet}")
endif()

if(EXISTS "${MarketDockVcpkgPrefix}/share/imgui")
    list(PREPEND CMAKE_PREFIX_PATH "${MarketDockVcpkgPrefix}")
endif()

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)

function(ConfigureVisualStudioTarget TargetName)
    if(MSVC)
        set_target_properties(${TargetName} PROPERTIES
            VS_GLOBAL_VcpkgEnableManifest true
            VS_GLOBAL_VcpkgManifestRoot "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()
endfunction()

add_library(Core STATIC
    Core/Application/Application.cpp
    Core/Application/Application.h
    Core/Core.cpp
    Core/Core.h
    Core/Rendering/DirectX12Renderer.cpp
    Core/Rendering/DirectX12Renderer.h
    Core/UserInterface/ImGuiLayer.cpp
    Core/UserInterface/ImGuiLayer.h
)

ConfigureVisualStudioTarget(Core)

target_include_directories(Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
)

target_link_libraries(Core PUBLIC
    imgui::imgui
    implot::implot
    d3d12
    dwmapi
    dxgi
)

add_library(Network STATIC
    Network/Network.cpp
    Network/Network.h
)

ConfigureVisualStudioTarget(Network)

target_include_directories(Network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Network
)

target_link_libraries(Network PUBLIC
    Core
)

add_executable(MarketDock WIN32
    MarketDock.cpp
    MarketDock.h
    framework.h
    targetver.h
    Resource.h
    MarketDock.rc
    MarketDock.ico
    small.ico
)

ConfigureVisualStudioTarget(MarketDock)

target_compile_definitions(MarketDock PRIVATE
    _WINDOWS
)

target_link_libraries(MarketDock PRIVATE
    Core
    Network
    d3d12
    dxgi
    d3dcompiler
)

add_executable(Test
    Test/Test.cpp
)

ConfigureVisualStudioTarget(Test)

target_compile_definitions(Test PRIVATE
    _CONSOLE
)

target_link_libraries(Test PRIVATE
    Core
    Network
)
