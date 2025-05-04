#include <Entity/Camera.hpp>

void Entity::Camera::Init( glm::vec3 startPosition ) {
    mPosition    = startPosition;
    mYaw         = -90.0f;
    mPitch       = 0.0f;
    mMoveSpeed   = 25.0f;
    mSensitivity = 0.1f;
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

void Entity::Camera::ProcessKeyInput( span<bool> inputs, float deltaTime ) {
    const float velocity = deltaTime * mMoveSpeed;
    if ( inputs[0] ) mPosition += mForward * velocity;
    if ( inputs[1] ) mPosition -= mRight   * velocity;
    if ( inputs[2] ) mPosition -= mForward * velocity;
    if ( inputs[3] ) mPosition += mRight   * velocity;
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
