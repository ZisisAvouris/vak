#include <Core/WindowManager.hpp>
#include <Core/Input.hpp>
#include <Renderer/GUI.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc( HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam ) {
    if ( ImGui_ImplWin32_WndProcHandler( windowHandle, msg, wParam, lParam ) )
        return true;

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
        case WM_KEYDOWN: {
            switch ( wParam ) {
                case VK_ESCAPE:  ShowCursor( !Core::WindowManager::Instance()->ToggleInputCapture() );   break;
                case VK_SPACE:   Input::KeyboardInputs::Instance()->SetKey( Input::Key_Space,    true ); break;
                case VK_SHIFT:   Input::KeyboardInputs::Instance()->SetKey( Input::Key_LShift,   true ); break;
                case VK_CONTROL: Input::KeyboardInputs::Instance()->SetKey( Input::Key_LControl, true ); break;
                case 'W':        Input::KeyboardInputs::Instance()->SetKey( Input::Key_W,        true ); break;
                case 'A':        Input::KeyboardInputs::Instance()->SetKey( Input::Key_A,        true ); break;
                case 'S':        Input::KeyboardInputs::Instance()->SetKey( Input::Key_S,        true ); break;
                case 'D':        Input::KeyboardInputs::Instance()->SetKey( Input::Key_D,        true ); break;
                case 'G':        Input::KeyboardInputs::Instance()->ToggleKey( Input::Key_G );           break;
            }
            return 0;
        }
        case WM_KEYUP: {
            switch ( wParam ) {
                case VK_SPACE:   Input::KeyboardInputs::Instance()->SetKey( Input::Key_Space,    false ); break;
                case VK_SHIFT:   Input::KeyboardInputs::Instance()->SetKey( Input::Key_LShift,   false ); break;
                case VK_CONTROL: Input::KeyboardInputs::Instance()->SetKey( Input::Key_LControl, false ); break;
                case 'W':        Input::KeyboardInputs::Instance()->SetKey( Input::Key_W,        false ); break;
                case 'A':        Input::KeyboardInputs::Instance()->SetKey( Input::Key_A,        false ); break;
                case 'S':        Input::KeyboardInputs::Instance()->SetKey( Input::Key_S,        false ); break;
                case 'D':        Input::KeyboardInputs::Instance()->SetKey( Input::Key_D,        false ); break;
            }
            return 0;
        }
        default:
            return DefWindowProcW( windowHandle, msg, wParam, lParam );
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

    ShowCursor( FALSE );
    RecenterCursor();

    Entity::Camera::Instance()->Init( glm::vec3( 0.0f, 200.0f, -50.0f ) );
    Input::KeyboardInputs::Instance()->SetKey( Input::Key_G, true );

    LARGE_INTEGER freq;
    QueryPerformanceFrequency( &freq );
    mFrequency = static_cast<float>( freq.QuadPart );
    QueryPerformanceCounter( &mLastTime );
}

void Core::WindowManager::Run( void ) {
    MSG msg = {};

    Rhi::Renderer::Instance()->Init( mWinResolution, mWindowHandle );

    LARGE_INTEGER currentTime;
    while ( !mShouldClose ) {
        PollEvents();

        QueryPerformanceCounter( &currentTime );
        const float deltaTime = ( currentTime.QuadPart - mLastTime.QuadPart ) / mFrequency;
        mLastTime = currentTime;

        mAccumulatedTime += deltaTime;
        ++mFramesRendered;

        if ( mAccumulatedTime > mSampleInterval ) {
            mFPS = mFramesRendered / mAccumulatedTime;
            mFramesRendered  = 0;
            mAccumulatedTime = 0;
        }

        if ( mShouldCaptureInputs )
            Entity::Camera::Instance()->ProcessKeyInput( deltaTime );
        Rhi::Renderer::Instance()->Render( Entity::Camera::Instance()->GetPosition(), Entity::Camera::Instance()->GetViewMatrix(), deltaTime );
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

    if ( mShouldCaptureInputs ) {
        // Skip one frame after recapturing mouse, so the accumulated delta doesn't make the camera jump to wherever
        if ( mJustCapturedInput ) {
            mJustCapturedInput = false;
            RecenterCursor();
            return;
        }

        POINT cursorPos;
        GetCursorPos( &cursorPos );
    
        const float dx = static_cast<float>( cursorPos.x - mWindowCenter.x );
        const float dy = static_cast<float>( cursorPos.y - mWindowCenter.y );
        Entity::Camera::Instance()->ProcessMouseMovement( dx, -dy );
        SetCursorPos( mWindowCenter.x, mWindowCenter.y );
    }
}

void Core::WindowManager::RecenterCursor( void ) {
    RECT windowRect;
    GetClientRect( mWindowHandle, &windowRect );
    MapWindowPoints( mWindowHandle, nullptr, (POINT*)&windowRect, 2 );
    mWindowCenter.x = ( windowRect.left + windowRect.right ) / 2;
    mWindowCenter.y = ( windowRect.top + windowRect.bottom ) / 2;
    SetCursorPos( mWindowCenter.x, mWindowCenter.y );
}
