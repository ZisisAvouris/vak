# vak
The goal of this project is to create a modern Vulkan renderer with a few features that would make it resemble a basic game engine. <br>
The renderer is built around `VK_KHR_dynamic_rendering` and bindless resources.

![Latest Progress 15 May 2025](./latest_progress_15_May_2025.png)
**Latest Progress: 15 May 2025**

## Build Instructions
**Only works on Windows!** <br>
*As the project is still very WIP, paths for assets may require to be manually selected* <br>
The project has a single hard dependency to the Vulkan SDK (i.e. it needs to be in your PATH variables)
- `git submodule update --init --recursive`
- `./build.bat`
- `./compileShaders.bat`
- `build/Debug/vak.exe`

## Debugging with Visual Studio
**This ensures that when running the debugger, the program will be able to find the relative paths in the code**
- Right-Click `vak` project in the Solution Explorer, select Properties
- In `Configuration Properties/Debugging`, set the Working Directory to `$(ProjectDir)..`

## Capturing with RenderDoc
**This ensures that when launching the program via Renderdoc, the former will be able to find the relative paths in the code**
- Set Executable Path to `vak/Debug/vak.exe`
- Set Working Directory to `vak/`
