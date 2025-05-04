#include <Core/WindowManager.hpp>

LRESULT CALLBACK WindowProc( HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam ) {
    switch ( msg ) {
        case WM_DESTROY: {
            PostQuitMessage( 0 );
            return 0;
        }
        case WM_SIZE: {
            uint width = LOWORD( lParam ), height = HIWORD( lParam ); 
            Core::WindowManager::Instance()->SetWindowResolution( { width, height } );
            if ( Rhi::Renderer::Instance()->IsReady() )
                Rhi::Renderer::Instance()->Resize( { width, height } );
            return 0;
        }
        default:
            return DefWindowProc( windowHandle, msg, wParam, lParam );
    }
}

void Core::WindowManager::InitWindow( void ) {
    mWinInstance = GetModuleHandleA( nullptr );
    assert( mWinInstance );

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.hInstance     = mWinInstance;
    wc.lpfnWndProc   = WindowProc;
    wc.hCursor       = LoadCursor( mWinInstance, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wc.lpszClassName = L"vak engine";
    if ( !RegisterClassExW( &wc ) ) {
        DWORD error = GetLastError();
        std::wcerr << L"Class Register Failed " << error << std::endl;
        assert(false);
    }

    mWindowHandle = CreateWindowExW( 0, L"vak engine", L"vak engine", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, mWinResolution.x, mWinResolution.y,
        nullptr, nullptr, mWinInstance, nullptr
    );
    if ( !mWindowHandle ) {
        DWORD error = GetLastError();
        std::wcerr << L"Create Window Failed " << error << std::endl;
        assert(false);
    }

    ShowWindow( mWindowHandle, SW_SHOW );

    LARGE_INTEGER freq;
    QueryPerformanceFrequency( &freq );
    mFrequency = static_cast<float>( freq.QuadPart );
    QueryPerformanceCounter( &mLastTime );
}

void Core::WindowManager::Run( void ) {
    MSG msg = {};

    Rhi::Renderer::Instance()->Init( mWinResolution );

    LARGE_INTEGER currentTime;
    while ( !mShouldClose ) {
        PollEvents();

        QueryPerformanceCounter( &currentTime );
        float deltaTime = ( currentTime.QuadPart - mLastTime.QuadPart ) / mFrequency;
        mLastTime = currentTime;

        Rhi::Renderer::Instance()->Render( deltaTime );
    }

    Rhi::Renderer::Instance()->Destroy();
}

void Core::WindowManager::PollEvents( void ) {
    MSG msg = {};
    while ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) ) {
        if ( msg.message == WM_QUIT )
            mShouldClose = true;
        
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}
