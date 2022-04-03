#ifndef __CAMERA__H_
#define __CAMERA__H_

#include "vmath.h"

class Camera
{
    public:
        //camera attributes
        vmath::vec3 vPosition;
        vmath::vec3 vFront;
        vmath::vec3 vUp;
        vmath::vec3 vRight;
        vmath::vec3 vWorldUp;
        
        //eular angles
        float fYaw;
        float fPitch;
        
        //camera options
        float movementSpeed;
        float mouseSensitivity;
        
        Camera( float mouseSensitivity, float movementSpeed, vmath::vec3 position, vmath::vec3 up, float yaw = -90.0f, float pitch = 0.0f)
        {
            //code
			this->mouseSensitivity = mouseSensitivity;
			this->movementSpeed = movementSpeed;
            this->vPosition = position;
            this->vWorldUp = up;
            this->fYaw = yaw;
            this->fPitch = pitch;
            
            updateCameraVectors();
        }
        
        void moveForward( float deltaTime)
        {
            float velocity = movementSpeed * deltaTime;
            this->vPosition += vFront * velocity;
        }
        
        void moveBackward( float deltaTime)
        {
            float velocity = movementSpeed * deltaTime;
            this->vPosition -= vFront * velocity;
        }
        
        void moveRight( float deltaTime)
        {
            float velocity = movementSpeed * deltaTime;
            this->vPosition += vRight * velocity;
        }
        
        void moveLeft( float deltaTime)
        {
            float velocity = movementSpeed * deltaTime;
            this->vPosition -= vRight * velocity;
        }
  
        void moveUp( float deltaTime)
        {
            float velocity = movementSpeed * deltaTime;
            this->vPosition += vUp * velocity;
        }
        
        void moveDown( float deltaTime)
        {
            float velocity = movementSpeed * deltaTime;
            this->vPosition -= vUp * velocity;
        }
  
        vmath::mat4 getViewMatrix( void)
        {
            return( vmath::lookat( vPosition, vPosition + vFront, vUp));
        }
  
        void rotate( float xOffset, float yOffset, bool constraintPitch = true)
        {
            xOffset *= mouseSensitivity;
            yOffset *= mouseSensitivity;
            
			fPitch += xOffset;
			fYaw += yOffset;
            
            //make sure that when pitch is out of bounds, screen doesn't get flipped
            if( constraintPitch)
            {
                if( fPitch > 89.0f)
                    fPitch = 89.0f;
                if(fPitch < -89.0f)
                    fPitch = -89.0f;
            }
            
            updateCameraVectors();
        }
    
        void updateCameraVectors( void)
        {
            //code
            vmath::vec3 front;
            
            front[0] = cos( vmath::radians(fYaw)) * cos( vmath::radians(fPitch));
            front[1] = sin( vmath::radians(fPitch));
            front[2] = sin( vmath::radians(fYaw)) * cos( vmath::radians(fPitch));
            
            this->vFront = vmath::normalize( front);
            this->vRight = vmath::normalize( vmath::cross( vFront, vWorldUp));
            this->vUp = vmath::normalize( vmath::cross( vRight, vFront));
        }
};

#endif
