# ArShr 的渲染管线与练枪软件

使用 C++、Win32 和 DirectX 12 编写的基础三维渲染管线、模型编辑器和练枪软件，无外部游戏引擎。

## 功能

- 三维模型的位置、旋转、缩放、颜色和可见性编辑。
- 棋盘地基、训练墙、基础几何体、实体与线框渲染。
- 中文折叠式编辑面板、背景灰度调整、全屏切换。
- 光源方向、强度、颜色、环境光和波动开关，以及光照辅助线框。
- 可编辑实心准心、游戏灵敏度、FPS、分数、命中率和本地最高分。
- 训练时固定相机位置，鼠标相对位移控制视角；结束后恢复编辑状态。

## 训练模式

所有模式均提供简单、中等、困难三个难度，分别记录历史最高分。

| 模式 | 玩法 |
| --- | --- |
| 快枪手 | 目标定时出现并膨胀、收缩，训练快速发现和射击；每局 60 秒。 |
| 游侠之眼 | 三血移动目标，中心命中秒杀、外圈扣血，包含未命中扣分和追击奖励；每局 60 秒。 |
| 聚焦火力 | 按住左键持续跟随移动目标，按有效命中时间计分；每局 60 秒。 |
| 午时已到 | 五轮决斗训练，每轮六发；球形头部与圆柱身体分部位判定，包含随机等待、倒计时和时间倍率。 |

准心仅用于视觉显示，实际命中使用细射线，不随准心大小改变。
最高分保存在 `%LOCALAPPDATA%\ArShr的渲染管线与练枪软件\aim_high_scores.txt`，个人记录不随仓库发布。

## 构建环境

- Windows 10/11 x64、支持 DirectX 12 的显卡和驱动。
- Windows SDK，以及可用于 Windows 目标的 MSVC 头文件和运行库。
- 支持 C++26 预览模式的编译器与相应版本的 CMake。当前 `CMakeLists.txt` 设置 `CXX_STANDARD 26`；建议 CMake 3.30 或更新版本。
- 日常开发推荐 LLVM `clang-cl`，使用 Windows SDK 和微软 ABI；也保留 Visual Studio 工程。
- CLion 自带的 MinGW/GCC 默认不受支持，CMake 会给出明确错误。实验开关不代表已完成适配。

### CLion / clang-cl

在 CLion 中打开仓库根目录的 `CMakeLists.txt`。配置 Windows 的 Visual Studio 工具链环境，选择 LLVM 的 `clang-cl` 作为 C++ 编译器，确保能找到 Windows SDK、微软标准库和链接器。建议使用 Ninja，并将工作目录设为仓库根目录。

也可以在已加载 Windows SDK / Visual Studio 开发环境的 PowerShell 中执行：

```powershell
cmake -S . -B out/build/clang-debug -G Ninja -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/clang-debug
& '.\out\build\clang-debug\ArShr的渲染管线与练枪软件.exe'
```

### Visual Studio

用 Visual Studio Community 打开 `Dx12Cpp26Pipeline.sln`，安装 C++ 桌面开发工作负载和工程要求的 Windows SDK / 平台工具集，然后选择 `Debug | x64` 构建运行。若本机工具集不同，需要在项目属性中匹配已安装版本。

仓库也保留 `vs2026-debug` 和 `vs2022-debug` CMake 预设；预设要求对应版本的 Visual Studio，实际可用性取决于本机编译器对 C++26 的支持。

```powershell
cmake --preset vs2026-debug
cmake --build --preset vs2026-debug
```

## 操作

| 操作 | 功能 |
| --- | --- |
| 编辑状态按住鼠标右键 | 转动视角 |
| W / A / S / D | 编辑状态移动相机 |
| 空格 / Ctrl | 编辑状态上升 / 下降 |
| 方向键 | 旋转当前选中模型 |
| 鼠标左键 | 单发射击；聚焦火力中按住跟枪 |
| T | 游戏中退出训练；编辑状态切换自动旋转 |
| Tab | 显示或隐藏编辑面板 |
| F11 | 全屏切换 |
| F / C | 线框 / 背面剔除切换 |
| R | 重新加载管线配置并重建 PSO |
| Esc | 退出程序 |

## 代码结构

```text
src/
  App/                   窗口、消息、生命周期与 GPU 同步
  Rendering/             逐帧常量更新与 DX12 绘制命令
  Lighting/              光源参数及方向计算
  Training/AimTrainer/   输入、模式、命中、分数、资源和训练生命周期
  UI/                    编辑面板与模型属性同步
  Geometry.cpp           程序化几何体
  RenderTypes.h          共享数据结构
  main.cpp               Win32 入口
assets/
  pipeline_state.txt     可编辑的管线参数
  shaders/basic.hlsl     独立 shader 参考文件
tools/
  package-release.ps1    Windows 发布打包脚本
```

当前运行使用 `Training/AimTrainer/Resources.cpp` 中的内嵌 HLSL；修改 `assets/shaders/basic.hlsl` 不会自动改变实际 shader。
`src/Game.cpp` 和 `src/Game.h` 是未接入当前 CMake 目标的 SDL 原型，缺少其配套依赖，不属于当前 DX12 运行入口。

## 许可证

本项目按 [MIT License](LICENSE) 开源。
