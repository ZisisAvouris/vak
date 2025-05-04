# vak
The goal of this project is to create a modern Vulkan renderer with a few features that would make it resemble a basic game engine. <br>
The renderer is built around `VK_KHR_dynamic_rendering` and bindless resources.

## Build Instructions
**Only works on Windows!** <br>
The project has a single hard dependency to the Vulkan SDK (i.e. it needs to be in your PATH variables)
- `git submodule update --init --recursive`
- `./build.bat`
- `./compilerShaders.bat`
- `build/Debug/vak.exe`

## Debugging with Visual Studio
**This ensures that when running the debugger, the program will be able to find the relative paths in the code**
- Right-Click `vak` project in the Solution Explorer, select Properties
- In `Configuration Properties/Debugging`, set the Working Directory to `$(ProjectDir)..`
