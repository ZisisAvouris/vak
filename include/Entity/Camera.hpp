#pragma once
#include <Util/Defines.hpp>
#include <Util/Singleton.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Entity {

    class Camera final : public Core::Singleton<Camera> {
    public:
        void Init( glm::vec3 );

        glm::mat4 GetViewMatrix( void ) const;
        glm::vec3 GetPosition( void ) const { return mPosition; }

        void ProcessMouseMovement( float, float );
        void ProcessKeyInput( float );

    private:
        glm::vec3 mPosition;
        glm::vec3 mForward;
        glm::vec3 mRight;
        glm::vec3 mUp;
        glm::vec3 mWorldUp;

        float mYaw, mPitch;
        float mMoveSpeed, mSensitivity;

        void UpdateCameraVectors( void );
    };
}
