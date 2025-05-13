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

    private:
        HWND      mWindowHandle = nullptr;
        HINSTANCE mWinInstance  = nullptr;

        uint2     mWinResolution = { 1920, 1080 };
        bool      mShouldClose   = false;

        LARGE_INTEGER mLastTime;
        float         mFrequency;

        bool  mShouldCaptureInputs = true;
        bool  mJustCapturedInput   = true;
        POINT mWindowCenter;

        float mAccumulatedTime = 0.0f;
        uint  mFramesRendered  = 0;
        float mSampleInterval  = 0.5f;

        void RecenterCursor( void );
    };

}
