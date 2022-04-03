#ifndef __ARC_CAMERA_H__
#define __ARC_CAMERA_H__


#include "vmath.h"

class ArcBallCamera
{
    public:
        //camera attributes
        vmath::vec3 vPosition;
        vmath::vec3 vPoint;
        
        //eular angles
        float fPitch;   //x
        float fYaw;     //y
        float fRoll;    //z
        
        //camera options
        float distanceFromPoint;
        float angleAroundPoint;

        //zoom sensitivity
        float fZoomChange;
        
        ArcBallCamera( vmath::vec3 position, vmath::vec3 point, float pitch, float yaw, float roll, float zoomSensitivity)
            : vPosition(position), fPitch(pitch), fYaw(yaw), fRoll(roll), vPoint(point), fZoomChange(zoomSensitivity)
        {
            //code
            this->distanceFromPoint = 0.0f;
        }

        void changeZoomLevel( float delta)
        {
            this->distanceFromPoint -= delta * this->fZoomChange;
        }
        
        void updatePitchAngle( float x)
        {
            this->fPitch = fmod(this->fPitch - x, 360.0f);
        }

        void updateAngleAroundPoint( float angle)
        {
            this->angleAroundPoint = fmod( this->angleAroundPoint + angle, 360.0f);
        }

        void calculateCameraLocationWithRespectToPoint( float pointYRotation, vmath::vec3 pointTranslate)
        {
            float horizontalDistance, verticalDistance;
            float angleInRadian;

            //code
            angleInRadian = vmath::radians( this->fPitch);
            horizontalDistance = this->distanceFromPoint * cos( angleInRadian);
            verticalDistance = this->distanceFromPoint * sin( angleInRadian);

            //calculate actual camera position
            float theta = pointYRotation + this->angleAroundPoint;
            float offsetX = horizontalDistance * sin( vmath::radians(theta));
            float offsetZ = horizontalDistance * cos( vmath::radians(theta));

            // this->vPosition[0] = pointTranslate[0] - offsetX;
            // this->vPosition[1] = pointTranslate[1] + verticalDistance;
            // this->vPosition[2] = pointTranslate[2] - offsetZ;

            this->vPoint[0] = this->vPoint[0] + pointTranslate[0];
            this->vPoint[1] = this->vPoint[1] + pointTranslate[1];
            this->vPoint[2] = this->vPoint[2] + pointTranslate[2];

            this->vPosition[0] = this->vPoint[0] - offsetX;
            this->vPosition[1] = this->vPoint[1] + verticalDistance;
            this->vPosition[2] = this->vPoint[2] - offsetZ;

            //yaw of camera
            this->fYaw = 180.0f - theta;
            this->fYaw = fmod(this->fYaw, 360.0f);
        }

        vmath::mat4 getViewMatrix( vmath::vec3 pointTranslate = vmath::vec3( 0.0f), float pointYRotate = 0.0f)
        {
           //code
           calculateCameraLocationWithRespectToPoint( pointYRotate, pointTranslate);

           vmath::mat4 viewMatrix = vmath::rotate( this->fRoll, 0.0f, 0.0f, 1.0f) * 
                                    vmath::rotate( this->fPitch, 1.0f, 0.0f, 0.0f) *
                                    vmath::rotate( this->fYaw, 0.0f, 1.0f, 0.0f) *
                                    vmath::translate( -vPosition);

            return(viewMatrix);
        }
};

#endif  //__ARC_CAMERA_H__

