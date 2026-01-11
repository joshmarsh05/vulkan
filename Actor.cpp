#include "Actor.h"

Actor::Actor(Vec3 pos_, Quaternion orientation_, Vec3 scale_, VulkanRenderer* vRenderer_, const char* meshFile_, const char* textureFile_) {

	pos = pos_;
	orientation = orientation_;
	scale = scale_;

	modelMatrix = MMath::translate(pos) * MMath::toMatrix4(orientation) * MMath::scale(scale_);

	vRenderer = vRenderer_;
	
	vMesh = vRenderer->LoadModelIndexed(meshFile_);
	vTexture = vRenderer->Create2DTextureImage(textureFile_);

	DescriptorSetCreator(vRenderer);

}

Actor::~Actor() {}

void Actor::Update(const float deltaTime) {
	//pos += vel * deltaTime + 0.5f * accel * deltaTime * deltaTime;
	//vel += accel * deltaTime;
}

void Actor::DescriptorSetCreator(VulkanRenderer* vRenderer_) {
	//DescriptorSetCreator(vRenderer);
	DescriptorSetBuilder descriptorSetBuilder(vRenderer->getDevice());
	// fills out the info of the actor | recipe
	descriptorSetBuilder.add(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, &vTexture);
	// give the info to the actor | baked
	descriptorSetInfo = descriptorSetBuilder.BuildDescriptorSet(vRenderer->getNumSwapchains());
}

void Actor::OnDestroy() {
	vRenderer->DestroyIndexedMesh(vMesh);
	vRenderer->DestroySampler2D(vTexture);
	vRenderer->DestroyDescriptorSet(descriptorSetInfo);
	//vRenderer->DestroyPipeline(pipelineInfo);
}

void Actor::SetModelMatrix(Matrix4 m)
{
	modelMatrix = m;
	pos = Vec3(modelMatrix[12], modelMatrix[13], modelMatrix[14]);
}
