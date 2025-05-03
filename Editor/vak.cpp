#include <Core/WindowManager.hpp>

int main( void ) {
    Core::WindowManager::Instance()->InitWindow();
    Core::WindowManager::Instance()->Run();
    return 0;
}
