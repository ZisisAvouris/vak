@echo off
setlocal

cd /d %~dp0assets\shaders

echo Compiling vertex shader...
glslangValidator -I%~dp0assets\shaders --target-env "vulkan1.3" -V main.vert.glsl -o shader.vert.spv
if errorlevel 1 (
    echo Failed to compile shader.vert
    exit /b 1
)

echo Compiling fragment shader...
glslangValidator -I%~dp0assets\shaders --target-env "vulkan1.3" -V main.frag.glsl -o shader.frag.spv
if errorlevel 1 (
    echo Failed to compile shader.frag
    exit /b 1
)

echo Compilation successful.
endlocal