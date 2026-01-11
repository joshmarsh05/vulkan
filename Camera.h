#pragma once
#include "VulkanRenderer.h"
#include <DualQuat.h>
#include <MMath.h>
//#include <QMath.h> 
#include <DQMath.h> 

using namespace MATH;
using namespace MATHEX;

class Camera {
private:
    DualQuat orientationPosition;
    DualQuat orientation;
    DualQuat position;

    Matrix4 projectionMatrix;
    Matrix4 viewMatrix;

    CameraData cameraData;
    std::vector<BufferMemory> cameraUBO;

    VulkanRenderer* renderer;

    void UpdateView();

public:
    Camera(VulkanRenderer* renderer);
    ~Camera();

    //Matrix4 GetViewMatrix() const { return  MMath::toMatrix4(orientation) * MMath::translate(position); }
    //Quaternion GetOrientation() const { return orientation; }

    //void SetView(const Quaternion& orientation_, const Vec3& position_);

    Matrix4 GetViewMatrix() const { return DQMath::toMatrix4(orientationPosition); }
    CameraData GetCameraData() { return cameraData; }
    std::vector<BufferMemory> GetCameraUBO() { return cameraUBO; }
    DualQuat GetOrientation() const { return orientation; }
    DualQuat GetOrientationDQ() const { return orientationPosition; }
    Matrix4 GetProjectionMatrix() const { return projectionMatrix; }
    Vec3 GetPosition() const { return DQMath::getTranslation(position); }
   
    
    void SetPosition(const Vec3& position_);
    void SetOrientation(const DualQuat& orientation_);
    void SetCameraUBO();
    void SetCameraUBO(VulkanRenderer* renderer_);
    void SetProjectionMatrix(float fovy_, float aspectRatio_, float zNear_, float zFar_);

};