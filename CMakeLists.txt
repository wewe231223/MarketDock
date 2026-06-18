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

function(ConfigureVisualStudioTarget TargetName)
    if(MSVC)
        set_target_properties(${TargetName} PROPERTIES
            VS_GLOBAL_VcpkgEnableManifest true
            VS_GLOBAL_VcpkgManifestRoot "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()
endfunction()

add_library(Core STATIC
    Core/Core.cpp
    Core/Core.h
)

ConfigureVisualStudioTarget(Core)

target_include_directories(Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
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
