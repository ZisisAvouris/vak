#pragma once
#include <Util/Singleton.hpp>
#include <windows.h>
#include <assert.h>
#include <iostream>
#include <Util/Containers.hpp>
#include <Renderer/Renderer.hpp>

namespace Core {

    class WindowManager final : public Core::Singleton<WindowManager> {
    public:
        void InitWindow( void );
        void Run( void );

        void PollEvents( void );

        void SetWindowResolution( uint2 resolution ) { mWinResolution = resolution; }

        HWND      GetWindowHandle( void ) const { return mWindowHandle; }
        HINSTANCE GetWindowInstance( void ) const { return mWinInstance; }

    private:
        HWND      mWindowHandle = nullptr;
        HINSTANCE mWinInstance  = nullptr;

        uint2     mWinResolution = { 800, 600 }; 
        bool      mShouldClose   = false;

        LARGE_INTEGER mLastTime;
        float         mFrequency;
    };

}
