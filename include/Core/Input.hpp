#pragma once
#include <Util/Defines.hpp>
#include <Util/Singleton.hpp>

namespace Input {

    enum Key : _byte {
        Key_W,
        Key_A,
        Key_S,
        Key_D,
        Key_Space,
        Key_LShift,
        Key_LControl,
        Key_G,
        Key_MAX
    };

    class KeyboardInputs final : public Core::Singleton<KeyboardInputs> {
    public:
        void SetKey( Key key, bool value ) { mKeyInput[key] = value; }
        bool GetKey( Key key ) const { return mKeyInput[key]; }

        void ToggleKey( Key key ) { mKeyInput[key] = !mKeyInput[key]; }

    private:
        bool mKeyInput[Key_MAX] = { false };
    };


}
