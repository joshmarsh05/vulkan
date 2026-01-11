#ifndef SCENE0_H
#define SCENE0_H
#include "Scene.h"
#include "Vector.h"
#include "Renderer.h"
#include "Camera.h"
#include "CoreStructs.h"

#include <chrono>
#include <thread>
#include "XBoxController.h"

#include "Actor.h"

#include "irrKlang.h"

#include "SaveFile.h"

using namespace MATH;
using namespace irrklang;

/// Forward declarations 
union SDL_Event;


class Scene0 : public Scene {
private:

	float x = 16;
	float y = 9;

	XBoxController* controller;
	ISoundEngine* soundEngine;
	ISound* music;
	
	Renderer *renderer;
	Camera *camera;

	//std::vector<BufferMemory> cameraUBO;
	//CameraData camera;*/
	std::vector<BufferMemory> lightsUBO;
	LightsData lights;
	//CommandBufferData commandBufferData;

	DescriptorSetInfo descriptorSetInfo;
	PipelineInfo pipelineInfo;


	DescriptorSetInfo CPDescriptorSetInfo;
	ColorPickerPipelineInfo CPPipelineInfo;

	std::vector<Actor*> actorList;

	Actor *marioActor;
	Actor *warioActor;
	Actor *luigiActor;

	std::vector<VkDescriptorSet> descriptorSets;

public:

	explicit Scene0(Renderer* renderer_);
	virtual ~Scene0();

	virtual bool OnCreate() override;
	virtual void OnDestroy() override;
	virtual void Update(const float deltaTime) override;
	virtual void Render() const override;
	virtual void HandleEvents(const SDL_Event &sdlEvent) override;

	void MakeDescriptorSet(VulkanRenderer* vRenderer);

	void DrawActor(VulkanRenderer* vRenderer, int frameIndex, Actor* actor) const;

};


#endif // SCENE0_H