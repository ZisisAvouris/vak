#include <Entity/Camera.hpp>
#include <Core/Input.hpp>

void Entity::Camera::Init( glm::vec3 startPosition ) {
    mPosition    = startPosition;
    mYaw         = 0.0f;
    mPitch       = 0.0f;
    mMoveSpeed   = 200.0f;
    mSensitivity = 0.1f;
    mWorldUp     = glm::vec3( 0.0f, 1.0f, 0.0f );
    UpdateCameraVectors();
}

glm::mat4 Entity::Camera::GetViewMatrix( void ) const {
    return glm::lookAt( mPosition, mPosition + mForward, mUp );
}

void Entity::Camera::ProcessMouseMovement( float xoff, float yoff ) {
    mYaw   += xoff * mSensitivity;
    mPitch += yoff * mSensitivity;
    if ( mPitch < -89.0f ) mPitch = -89.0f;
    if ( mPitch > 89.0f ) mPitch = 89.0f;
    UpdateCameraVectors();
}

void Entity::Camera::ProcessKeyInput( float deltaTime ) {
    mMoveSpeed = Input::KeyboardInputs::Instance()->GetKey( Input::Key_LShift ) ? 500.0f : 200.0f;
    const float velocity = deltaTime * mMoveSpeed;

    if ( Input::KeyboardInputs::Instance()->GetKey( Input::Key_W        ) ) mPosition += mForward * velocity;
    if ( Input::KeyboardInputs::Instance()->GetKey( Input::Key_A        ) ) mPosition -= mRight   * velocity;
    if ( Input::KeyboardInputs::Instance()->GetKey( Input::Key_S        ) ) mPosition -= mForward * velocity;
    if ( Input::KeyboardInputs::Instance()->GetKey( Input::Key_D        ) ) mPosition += mRight   * velocity;
    if ( Input::KeyboardInputs::Instance()->GetKey( Input::Key_Space    ) ) mPosition += mWorldUp * velocity;
    if ( Input::KeyboardInputs::Instance()->GetKey( Input::Key_LControl ) ) mPosition -= mWorldUp * velocity;
}

void Entity::Camera::UpdateCameraVectors( void ) {
    glm::vec3 newFront;
    newFront.x = cosf( glm::radians( mYaw ) ) * cosf( glm::radians( mPitch ) );
    newFront.y = sinf( glm::radians( mPitch ) );
    newFront.z = sinf( glm::radians( mYaw ) ) * cosf( glm::radians( mPitch ));
    mForward = glm::normalize( newFront );
    mRight   = glm::normalize( glm::cross( mForward, glm::vec3( 0.0f, 1.0f, 0.0f ) ) );
    mUp      = glm::normalize( glm::cross( mRight, mForward ) );
}
