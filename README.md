# ArShr Rendering Pipeline & Aim Trainer

A basic 3D rendering pipeline, model editor, and aim trainer built with C++, Win32, and DirectX 12, without an external game engine.

## Features

- Edit model position, rotation, scale, color, and visibility.
- Render a checkerboard platform, training wall, and procedural geometry in solid or wireframe mode.
- Collapsible editor panels with Chinese UI labels, a grayscale background control, and fullscreen switching.
- Adjust light direction, intensity, color, ambient lighting, and light fluctuation; display a wireframe lighting guide.
- Customize the solid-dot crosshair and mouse sensitivity; display FPS, score, accuracy, and local high scores.
- Use a fixed camera position and relative mouse input during training, then restore the previous editor state when training ends.

## Training Modes

Every mode offers Easy, Medium, and Hard difficulty, with a separate high score for each mode and difficulty combination. The original Chinese UI names are included below to help locate each mode in the application.

| Mode | Gameplay |
| --- | --- |
| Quickdraw (快枪手) | Targets appear at fixed intervals, grow, and shrink to train fast target acquisition and shooting. Each session lasts 60 seconds. |
| Ranger's Eye (游侠之眼) | Moving targets have three health points. Center hits eliminate them instantly; outer hits deal one damage. Includes miss penalties and pursuit bonuses. Each session lasts 60 seconds. |
| Focus Fire (聚焦火力) | Hold the left mouse button and track a moving target. Score is based on time spent on target while firing. Each session lasts 60 seconds. |
| High Noon (午时已到) | Five rounds with six shots per round. Targets have spherical heads and cylindrical bodies, with separate hit detection, random appearance delays, countdowns, and time-based score multipliers. |

The crosshair is a visual aid. Hit detection uses a thin ray and does not change with crosshair size.
High scores are stored in `%LOCALAPPDATA%\ArShr的渲染管线与练枪软件\aim_high_scores.txt`. Personal score data is excluded from the repository.

## Build Requirements

- Windows 10/11 x64 with a DirectX 12 capable GPU and driver.
- Windows SDK, MSVC headers, and runtime libraries for Windows targets.
- A compiler supporting C++26 preview mode and a compatible CMake version. `CMakeLists.txt` currently sets `CXX_STANDARD 26`; CMake 3.30 or newer is recommended.
- LLVM `clang-cl` is recommended for daily development using the Windows SDK and Microsoft ABI. Visual Studio project files are also included.
- CLion's bundled MinGW/GCC toolchain is not supported by default. CMake reports an explicit error for this configuration; enabling the experimental override does not imply compatibility.

### CLion / clang-cl

Open the repository's root `CMakeLists.txt` in CLion. Configure a Visual Studio toolchain environment on Windows and select LLVM's `clang-cl` as the C++ compiler. Ensure the Windows SDK, Microsoft standard library, and linker are available. Ninja is recommended. Set the application's working directory to the repository root.

Alternatively, run these commands from a PowerShell session with the Windows SDK / Visual Studio development environment loaded:

```powershell
cmake -S . -B out/build/clang-debug -G Ninja -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/clang-debug
& '.\out\build\clang-debug\ArShr的渲染管线与练枪软件.exe'
```

### Visual Studio

Open `Dx12Cpp26Pipeline.sln` in Visual Studio Community. Install the Desktop development with C++ workload and the Windows SDK / platform toolset required by the project, then build and run with `Debug | x64`. If your installed toolset differs, select the appropriate version in the project properties.

The repository also includes `vs2026-debug` and `vs2022-debug` CMake presets. Each requires its corresponding Visual Studio version; usability also depends on the installed compiler's C++26 support.

```powershell
cmake --preset vs2026-debug
cmake --build --preset vs2026-debug
```

## Controls

| Input | Action |
| --- | --- |
| Hold right mouse button | Look around in editor mode |
| W / A / S / D | Move the camera in editor mode |
| Space / Ctrl | Move up / down in editor mode |
| Arrow keys | Rotate the selected model |
| Left mouse button | Fire a shot; hold to track in Focus Fire |
| T | Exit training during a session; toggle automatic rotation in editor mode |
| Tab | Show or hide the editor panels |
| F11 | Toggle fullscreen |
| F / C | Toggle wireframe / backface culling |
| R | Reload pipeline settings and rebuild the PSO |
| Esc | Exit the application |

## Project Structure

```text
src/
  App/                   Window handling, messages, lifecycle, and GPU synchronization
  Rendering/             Per-frame constant updates and DX12 draw commands
  Lighting/              Light settings and direction calculations
  Training/AimTrainer/   Input, modes, hit detection, scores, resources, and training lifecycle
  UI/                    Editor panels and model property synchronization
  Geometry.cpp           Procedural geometry
  RenderTypes.h          Shared data structures
  main.cpp               Win32 entry point
assets/
  pipeline_state.txt     Editable pipeline settings
  shaders/basic.hlsl     Standalone reference shader
tools/
  package-release.ps1    Windows release packaging script
```

The application currently uses HLSL embedded in `Training/AimTrainer/Resources.cpp`. Editing `assets/shaders/basic.hlsl` does not automatically change the active shader.
`src/Game.cpp` and `src/Game.h` are an SDL prototype excluded from the current CMake target. Their supporting dependencies are not included, and they are not the entry point for the DX12 application.

## License

This project is open source under the [MIT License](LICENSE).
