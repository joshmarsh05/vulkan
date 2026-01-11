#include "Camera.h"

Camera::Camera(VulkanRenderer* renderer) {

	this->renderer = renderer;

	//orientationPosition = DualQuat(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	projectionMatrix = MMath::perspective(45.0f, (16.0f / 9.0f), 0.5f, 100.0f);
	projectionMatrix[5] *= -1.0f;

	orientation = DQMath::rotate(0.0f, Vec3(0.0f, 1.0f, 0.0f));
	position = DQMath::translate(Vec3(0.0f, 0.0f, -15.0f));
	UpdateView();

	cameraData.projectionMatrix = GetProjectionMatrix();
	cameraData.viewMatrix = GetViewMatrix();

	SetCameraUBO(renderer);
	
}

Camera::~Camera() {}

void Camera::SetPosition(const Vec3& position_) {
	position = DQMath::translate(position_);
	UpdateView();
}

void Camera::SetOrientation(const DualQuat& orientation_) {
	orientation = orientation_;
	UpdateView();
}


void Camera::UpdateView() {
	orientationPosition = orientation * position;
	cameraData.viewMatrix = DQMath::toMatrix4(orientationPosition);
}

void Camera::SetCameraUBO() {
	cameraUBO = renderer->CreateUniformBuffers<CameraData>();
}

void Camera::SetCameraUBO(VulkanRenderer* renderer_) {
	renderer = renderer_;
	cameraUBO = renderer->CreateUniformBuffers<CameraData>();
}

void Camera::SetProjectionMatrix(float fovy_, float aspectRatio_, float zNear_, float zFar_) {
	projectionMatrix = MMath::perspective(fovy_, aspectRatio_, zNear_, zFar_);
	projectionMatrix[5] *= -1.0f;
	cameraData.projectionMatrix = projectionMatrix;
}




