#pragma once
#include <Util/Singleton.hpp>
#include <windows.h>
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <Util/Containers.hpp>
#include <Util/Defines.hpp>
#include <Renderer/Renderer.hpp>
#include <Entity/Camera.hpp>

namespace Core {

    enum Key : _byte {
        Key_W,
        Key_A,
        Key_S,
        Key_D,
        Key_MAX
    };

    class WindowManager final : public Core::Singleton<WindowManager> {
    public:
        void InitWindow( void );
        void Run( void );

        void PollEvents( void ); 

        void SetWindowResolution( uint2 resolution ) { mWinResolution = resolution; }

        HWND      GetWindowHandle( void ) const { return mWindowHandle; }
        HINSTANCE GetWindowInstance( void ) const { return mWinInstance; }

        bool ToggleInputCapture( void ) {
            mShouldCaptureInputs = !mShouldCaptureInputs;
            mJustCapturedInput   = mShouldCaptureInputs;
            return mShouldCaptureInputs;
        }
        void SetKey( Key key, bool value ) { mKeyInput[key] = value; } 

    private:
        HWND      mWindowHandle = nullptr;
        HINSTANCE mWinInstance  = nullptr;

        uint2     mWinResolution = { 1600, 900 }; 
        bool      mShouldClose   = false;

        LARGE_INTEGER mLastTime;
        float         mFrequency;

        bool  mShouldCaptureInputs = true;
        bool  mJustCapturedInput   = true;
        POINT mWindowCenter;
        bool  mKeyInput[Key_MAX] = { false };

        void RecenterCursor( void );
    };

}
