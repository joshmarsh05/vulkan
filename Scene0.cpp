#include <glew.h>
#include <iostream>
#include "Debug.h"
#include "Scene0.h"
#include <MMath.h>
#include "Debug.h"
#include "VulkanRenderer.h"
#include "OpenGLRenderer.h"
#include "Camera.h"
#include "Actor.h"

Scene0::Scene0(Renderer *renderer_): 
	camera(nullptr),
	Scene(nullptr),renderer(renderer_) {
	Debug::Info("Created Scene0: ", __FILE__, __LINE__);
}

Scene0::~Scene0() {
	camera = nullptr;
}

bool Scene0::OnCreate() {

	int width = 0, height = 0;
	float aspectRatio;

	controller = new XBoxController(0);
	soundEngine = createIrrKlangDevice();

	//SavePositionBinary(Vec3(1, 1, 1), "SavePosition.bin");

	//music = soundEngine->play3D("audio/ForkScratching.wav", vec3df(0.0, 0.0, 0.0), true, true, true);
	//music = soundEngine->play2D("audio/ForkScratching.wav", true);

	switch (renderer->getRendererType()){
	case RendererType::VULKAN:
	{
		VulkanRenderer* vRenderer;
		vRenderer = dynamic_cast<VulkanRenderer*>(renderer);
		
		lightsUBO = vRenderer->CreateUniformBuffers<LightsData>();
		//cameraUBO = vRenderer->CreateUniformBuffers<CameraData>();

		SDL_GetWindowSize(vRenderer->getWindow(), &width, &height);
		aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		//camera.projectionMatrix = MMath::perspective(45.0f, aspectRatio, 0.5f, 100.0f);
		//camera.projectionMatrix[5] *= -1.0f;
		//camera.viewMatrix = MMath::translate(0.0f, 0.0f, -5.0f);
		camera = new Camera(vRenderer);
		camera->SetPosition(Vec3(0.0f, 0.0f, -5.0f));
		//camera->SetProjectionMatrix(45.0f, (100.0f / 1.0f), 0.5f, 100.0f);

		
		lights.diffuse[0] = Vec4(1.0, 0.0, 0.0, 0.0);
		lights.specular[0] = Vec4(0.0, 0.3, 0.0, 0.0);

		lights.diffuse[1] = Vec4(0.0, 1.0, 0.0, 0.0);
		lights.specular[1] = Vec4(0.0, 0.3, 0.0, 0.0);

		lights.diffuse[2] = Vec4(0.0, 0.0, 1.0, 0.0);
		lights.specular[2] = Vec4(0.0, 0.3, 0.0, 0.0);

		lights.ambient = Vec4(0.1, 0.1, 0.1, 0.0);
		lights.numLights = 3;
		lights.pos[0] = Vec4(4.0f, 0.0f, 5.0f, 0.0f);
		lights.pos[1] = Vec4(-4.0f, 0.0f, -5.0f, 0.0f);
		lights.pos[2] = Vec4(0.0f, -4.0f, -2.0f, 0.0f);
		vRenderer->UpdateUniformBuffer<LightsData>(lights, lightsUBO);
		//vRenderer->UpdateUniformBuffer<CameraData>(camera->GetCameraData(), cameraUBO);
		
		Vec3 savedPos;
		LoadPositionBinary(savedPos, "SavePosition.bin");
		std::cout << "Loaded pos: " << savedPos.x << "," << savedPos.y << "," << savedPos.z << std::endl;
		//Actors
		marioActor = new Actor(savedPos, Quaternion(0, Vec3(0, 1, 0)), Vec3(1,1,1),
			vRenderer,"./meshes/Mario.obj", "./textures/mario_fire.png");
		marioActor->SetColorID(255);
		std::cout << marioActor->getPosition().x << std::endl;
		// actor 2
		warioActor = new Actor(Vec3(2, 0, 0), Quaternion(0, Vec3(0, 0, 0)), Vec3(0.5, 0.5, 0.5),
			vRenderer, "./meshes/Skull.obj", "./textures/skull_texture.png");
		warioActor->SetColorID(255*255);
		// actor 3
		luigiActor = new Actor(Vec3(0, 1, 0), Quaternion(0, Vec3(0, 1, 0)), Vec3(1, 1, 1),
			vRenderer, "./meshes/Mario.obj", "./textures/mario_mime.png");
		luigiActor->SetColorID(255*255*255);

		actorList = { marioActor, warioActor, luigiActor };

		MakeDescriptorSet(vRenderer);

		std::vector<VkDescriptorSetLayout> layout(actorList.size() + 1);
		layout[0] = descriptorSetInfo.descriptorSetLayout;
		for (int i = 0; i < actorList.size(); i++) {
			layout[i + 1] = actorList[i]->GetDescriptorSetInfo().descriptorSetLayout;
		}

		pipelineInfo = vRenderer->CreateGraphicsPipeline(layout.size(), layout.data(), "shaders/multiPhong.vert.spv", "shaders/multiPhong.frag.spv");
		CPPipelineInfo = vRenderer->createColorPickerPipeline(CPDescriptorSetInfo.descriptorSetLayout, "shaders/colorPickingVert.vert.spv", "shaders/colorPickingFrag.frag.spv");
		
	}
		break;

	case RendererType::OPENGL:
		break;
	}

	return true;
}

void Scene0::MakeDescriptorSet(VulkanRenderer* vRenderer)
{
	vRenderer->DestroyDescriptorSet(descriptorSetInfo);

	DescriptorSetBuilder descriptorSetBuilder(vRenderer->getDevice());
	descriptorSetBuilder.add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, camera->GetCameraUBO());

	descriptorSetBuilder.add(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, lightsUBO);
	// give the info to the actor | baked
	descriptorSetInfo = descriptorSetBuilder.BuildDescriptorSet(vRenderer->getNumSwapchains());

	DescriptorSetBuilder descriptorSetBuilderCP(vRenderer->getDevice());
	descriptorSetBuilderCP.add(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, camera->GetCameraUBO());
	CPDescriptorSetInfo = descriptorSetBuilderCP.BuildDescriptorSet(vRenderer->getNumSwapchains());
}

void Scene0::HandleEvents(const SDL_Event& sdlEvent) {

	switch (sdlEvent.type) {
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
			printf("size changed %d %d\n", sdlEvent.window.data1, sdlEvent.window.data2);
			float aspectRatio = static_cast<float>(sdlEvent.window.data1) / static_cast<float>(sdlEvent.window.data2);
			camera->SetProjectionMatrix(45.0f, aspectRatio, 0.5f, 100.0f);
			if (renderer->getRendererType() == RendererType::VULKAN) {

				dynamic_cast<VulkanRenderer*>(renderer)->RecreateSwapChain();
				MakeDescriptorSet(dynamic_cast<VulkanRenderer*>(renderer));
				//marioActor->DescriptorSetCreator(dynamic_cast<VulkanRenderer*>(renderer));
			}
		}
		break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN: {
			if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
				int x, y;
				x = sdlEvent.button.x;
				y = sdlEvent.button.y;

				//start recording for the color picker command buffer
				if (renderer->getRendererType() == RendererType::VULKAN) {
					VulkanRenderer* vRenderer = dynamic_cast<VulkanRenderer*>(renderer);
					vRenderer->RecordCPCommandBuffer(Recording::START);
					//bind the color picker pipeline
					vRenderer->BindColorPickerPipeline(CPPipelineInfo.colorPickerPipeline);

					//descriptor set you need to pass here should be just with the cameraUBO data
					//bind the deescriptor set for color picking
					vRenderer->BindCPDescriptorSet(CPPipelineInfo.colorPickerPipelineLayout, CPDescriptorSetInfo.descriptorSet.front(), 0);
					//for each actor bind the mesh, texture whatever

					for (auto actor : actorList) {
						//remake the function specific to the colorpicker commandbuffer
						vRenderer->BindMeshCP(actor->GetMesh());
						vRenderer->SetPushConstantCP(CPPipelineInfo, actor);
						vRenderer->DrawIndexedCP(actor->GetMesh());
					}

					vRenderer->RecordCPCommandBuffer(Recording::STOP);

					uint32_t extractedColorID = vRenderer->extractColorID(x, y);
					std::cout << "COLOR ID: " << extractedColorID << std::endl;
					for (auto actor : actorList) {
						if (actor->getColorID() == extractedColorID)
							actor->SetModelMatrix(actor->getModelMatrix() * MMath::translate(Vec3(0.0f, 0.0f, -1.0f)));
					}


				}
			}
		}
		break;

		case SDL_EVENT_KEY_DOWN: {
			if (sdlEvent.key.scancode == SDL_SCANCODE_D) {
				marioActor->SetModelMatrix(marioActor->getModelMatrix() * MMath::translate(Vec3(1.0f, 0.0f, 0.0f)));
			}
			else if (sdlEvent.key.scancode == SDL_SCANCODE_A) {
				marioActor->SetModelMatrix(marioActor->getModelMatrix() * MMath::translate(Vec3(-1.0f, 0.0f, 0.0f)));
			}
			else if (sdlEvent.key.scancode == SDL_SCANCODE_S) {
				Vec3 pos = marioActor->getPosition();
				std::cout << "Saving pos: " << pos.x << "," << pos.y << "," << pos.z << std::endl;
				SavePositionBinary(pos, "SavePosition.bin");

				std::cout << "saved position\n";
			}
		}
		break;
	}
}

void Scene0::Update(const float deltaTime) {
	static float elapsedTime = 0.0f;
	elapsedTime += deltaTime;
	//marioActor->SetModelMatrix(MMath::rotate(elapsedTime * 90.0f, Vec3(0.0f, 1.0f, 0.0f)));
	//marioActor->Update(deltaTime);
	marioActor->SetModelMatrix(marioActor->getModelMatrix() * MMath::rotate(deltaTime * 90.0f, Vec3(1.0f, 0.0f, 0.0f)));
	//warioActor->SetModelMatrix(warioActor->getModelMatrix() * MMath::rotate(deltaTime * 90.0f, Vec3(0.0f, 1.0f, 0.0f)));
	luigiActor->SetModelMatrix(luigiActor->getModelMatrix() * MMath::rotate(deltaTime * 90.0f, Vec3(0.0f, 0.0f, 1.0f)));

	/////////////////////////////////////////////
	// Xbox Controller
	/////////////////////////////////////////////

	XINPUT_STATE xinput_state = controller->GetState();

	/////////////////////////////////////////////
	// Buttons
	/////////////////////////////////////////////
	if (xinput_state.Gamepad.wButtons == XINPUT_GAMEPAD_A) {
		std::cout << "A button was pressed\n";
		warioActor->SetModelMatrix(warioActor->getModelMatrix() * MMath::rotate(90.0f, Vec3(0.0f, 1.0f, 0.0f)));
	}

	/////////////////////////////////////////////
	// Left Thumb Stick
	/////////////////////////////////////////////
	float xAxisL = xinput_state.Gamepad.sThumbLX;
	float yAxisL = xinput_state.Gamepad.sThumbLY;

	const SHORT LDEADZONE = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

	float normLX = 0.0f;
	float normLY = 0.0f;

	// Apply deadzone correctly
	if (abs(xAxisL) > LDEADZONE) {
		normLX = xAxisL / 32767.0f;
	}
	if (abs(yAxisL) > LDEADZONE) {
		normLY = yAxisL / 32767.0f;
	}

	// Movement speed
	float speed = 2.0f;

	warioActor->SetModelMatrix(warioActor->getModelMatrix() *
		MMath::translate(Vec3(normLX * speed * deltaTime, normLY * speed * deltaTime, 0.0f)));
	if (abs(xAxisL) > LDEADZONE || abs(yAxisL) > LDEADZONE) {
		soundEngine->play3D("audio/Scratching.wav", vec3df(warioActor->getPosition().x, warioActor->getPosition().y, warioActor->getPosition().z), true, false, true);
	}
	 //music->setIsPaused(true);

	/////////////////////////////////////////////
	// Right Thumb Stick
	/////////////////////////////////////////////
	float xAxisR = xinput_state.Gamepad.sThumbRX;
	float yAxisR = xinput_state.Gamepad.sThumbRY;

	const SHORT RDEADZONE = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

	float normRX = 0.0f;
	float normRY = 0.0f;

	// Apply deadzone correctly
	if (abs(xAxisR) > RDEADZONE) {
		normRX = xAxisR / 32767.0f;
	}
	if (abs(yAxisR) > RDEADZONE) {
		normRY = yAxisR / 32767.0f;
	}

	x += normRX;
	y += normRY;

	camera->SetProjectionMatrix(45.0f, (x / y), 0.5f, 100.0f);

}

// frameIndex being which swapchain
void Scene0::DrawActor(VulkanRenderer* vRenderer, int frameIndex, Actor* actor) const {
	std::vector<VkDescriptorSet> descriptorSets = {
		descriptorSetInfo.descriptorSet[frameIndex],           // global (camera + lights)
		actor->GetDescriptorSetInfo().descriptorSet[frameIndex] // actor texture
	};

	vRenderer->BindDescriptorSet(pipelineInfo.pipelineLayout, descriptorSets);
	vRenderer->BindMesh(actor->GetMesh());
	vRenderer->SetPushConstant(pipelineInfo, actor->getModelMatrix());
	vRenderer->DrawIndexed(actor->GetMesh());
}

void Scene0::Render() const {
		switch (renderer->getRendererType()) {

	case RendererType::VULKAN:
		VulkanRenderer* vRenderer;
		vRenderer = dynamic_cast<VulkanRenderer*>(renderer);
		
		
		vRenderer->RecordCommandBuffers(Recording::START);

		vRenderer->UpdateUniformBuffer<CameraData>(camera->GetCameraData(), camera->GetCameraUBO());

		vRenderer->BindPipeline(pipelineInfo.pipeline);

		for (int i = 0; i < vRenderer->getNumSwapchains(); i++) {

			for (Actor* actor : actorList) {
				DrawActor(vRenderer, i, actor);
			}

		}

		vRenderer->RecordCommandBuffers(Recording::STOP);
		vRenderer->Render();
		break;

	case RendererType::OPENGL:
		OpenGLRenderer* glRenderer;
		glRenderer = dynamic_cast<OpenGLRenderer*>(renderer);
		/// Clear the screen
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		/// Draw your scene here
		glUseProgram(0);
		
		break;
	}
}


void Scene0::OnDestroy() {

	delete controller;
	if (soundEngine) soundEngine->drop();

	VulkanRenderer* vRenderer;
	vRenderer = dynamic_cast<VulkanRenderer*>(renderer);
	if(vRenderer){
		vkDeviceWaitIdle(vRenderer->getDevice());

		vRenderer->DestroyPipeline(pipelineInfo);
		vRenderer->DestroyCPPipeline(CPPipelineInfo);
		vRenderer->DestroyDescriptorSet(descriptorSetInfo);
		vRenderer->DestroyCPDescriptorSet(CPDescriptorSetInfo);
		for (Actor* actor : actorList) {
			actor->OnDestroy();
			delete actor;
		}
		actorList.clear();

		vRenderer->DestroyUBO(lightsUBO);
		vRenderer->DestroyUBO(camera->GetCameraUBO());
		vRenderer->DestroyCommandBuffers();

	}
}
