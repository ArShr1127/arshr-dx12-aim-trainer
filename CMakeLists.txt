cmake_minimum_required(VERSION 3.25)

project(Dx12Cpp26Pipeline LANGUAGES CXX)

option(ARSHR_ALLOW_UNSUPPORTED_MINGW "Allow MinGW/GCC configure for portability experiments." OFF)

if(MINGW AND NOT ARSHR_ALLOW_UNSUPPORTED_MINGW)
    message(FATAL_ERROR
        "This DX12 project uses the Microsoft DirectX 12 / WRL / DirectXMath SDK stack. "
        "CLion's bundled MinGW/GCC is not a supported build toolchain for running it. "
        "Use CLion with a Visual Studio/MSVC toolchain, or clang-cl with the MSVC ABI. "
        "Set ARSHR_ALLOW_UNSUPPORTED_MINGW=ON only for experimental portability checks."
    )
endif()

add_executable(Dx12Cpp26Pipeline WIN32
    src/main.cpp
    src/App/AppConfig.h
    src/App/Dx12App.h
    src/App/Dx12AppLifecycle.cpp
    src/App/Dx12AppSync.cpp
    src/App/Dx12AppWindow.cpp
    src/DxHelpers.cpp
    src/DxHelpers.h
    src/Geometry.cpp
    src/Geometry.h
    src/Lighting/LightSettings.cpp
    src/Lighting/LightSettings.h
    src/Rendering/RendererPipeline.cpp
    src/RenderTypes.h
    src/Training/AimTrainer/Input.cpp
    src/Training/AimTrainer/Lifecycle.cpp
    src/Training/AimTrainer/Modes.cpp
    src/Training/AimTrainer/Resources.cpp
    src/Training/AimTrainer/Scores.cpp
    src/Training/AimTrainer/Targeting.cpp
    src/UI/EditorPanels.cpp
    src/UI/ModelEditor.cpp
)

set_property(TARGET Dx12Cpp26Pipeline PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_property(TARGET Dx12Cpp26Pipeline PROPERTY OUTPUT_NAME "ArShr的渲染管线与练枪软件")
set_property(TARGET Dx12Cpp26Pipeline PROPERTY CXX_STANDARD 26)
set_property(TARGET Dx12Cpp26Pipeline PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET Dx12Cpp26Pipeline PROPERTY CXX_EXTENSIONS OFF)

target_compile_definitions(Dx12Cpp26Pipeline PRIVATE UNICODE _UNICODE)

if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(Dx12Cpp26Pipeline PRIVATE
        /W4
        /permissive-
        /EHsc
        /utf-8
        /Zc:__cplusplus
    )
else()
    target_compile_options(Dx12Cpp26Pipeline PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

target_link_libraries(Dx12Cpp26Pipeline PRIVATE
    d3d12
    dxgi
    d3dcompiler
    comctl32
    user32
)
