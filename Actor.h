#pragma once
#include <MMath.h>
#include "CoreStructs.h"
#include "VulkanRenderer.h"

using namespace MATH;

class VulkanRenderer;

class Actor {
private:

	Vec3 pos;
	Quaternion orientation;
	Vec3 scale;
	Matrix4 modelMatrix;

	IndexedVertexBuffer vMesh;
	Sampler2D vTexture;

	VulkanRenderer *vRenderer;

	DescriptorSetInfo descriptorSetInfo; // its own texture

	uint32_t colorID;

public:

	Actor(Vec3 pos_, Quaternion orientation_, Vec3 scale_, VulkanRenderer* vRenderer_, const char* meshFile_, const char* textureFile_);
	~Actor();
	void Update(const float deltaTime);
	void OnDestroy();

	Matrix4 getModelMatrix() const { return modelMatrix; }
	void SetModelMatrix(Matrix4 m);
	void SetPosition(Vec3 pos_) { pos = pos_; }
	Vec3 getPosition() { return pos; }

	IndexedVertexBuffer GetMesh() const { return vMesh; }
	Sampler2D GetTexture() const { return vTexture; }

	void DescriptorSetCreator(VulkanRenderer* vRenderer_);
	DescriptorSetInfo& GetDescriptorSetInfo() { return descriptorSetInfo; }

	void SetColorID(uint32_t colorID_) { colorID = colorID_; }
	uint32_t getColorID() { return colorID; }
};

