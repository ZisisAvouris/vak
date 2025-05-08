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

        float GetFPS( void ) const { return mFPS; }

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

        float mAccumulatedTime = 0.0f;
        uint  mFramesRendered  = 0;
        float mFPS             = 0.0f;
        float mSampleInterval  = 0.5f;

        void RecenterCursor( void );
    };

}
