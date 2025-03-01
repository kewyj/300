/*!*************************************************************************
****
\file			GraphicsSystem.cpp
\author			Richmond
\par DP email:	r.choo@digipen.edu
\date			28/9/23
\brief
	The function definitions for the functionalities of the graphics system

****************************************************************************
***/
#define  _ENABLE_ANIMATIONS					true
#define  ENABLE_MULTISAMPLE					true	
#define  _TEST_HEALTHBAR_SHADER				false
#define  ENABLE_UI_IN_EDITOR_SCENE			true
#define  ENABLE_CROSSHAIR_IN_EDITOR_SCENE	false
#define  TEST_DEFERRED_LIGHT				true	

#include <ECS/ECS_Components.h>
#include <Graphics/GraphicsSystem.h>
#include <Graphics/GraphicsSystemDefinitions.h>
#include "ResourceManagerTy.h"
#include <Graphics/Camera_Input.h>
#include "Debug/EnginePerformance.h"
#include "GameState/GameStateManager.h"
#include "Input/InputMapSystem.h"

#include "cstdlib"
#include <omp.h>


/***************************************************************************/
/*!
\brief
	Initialize the Graphics system
*/
/**************************************************************************/
float first_entitytime{};
float second_entitytime{};



void GraphicsSystem::Init()
{
	if (!m_SystemInitialized)
	{
		m_SystemInitialized = true;

		// -- Setup Storage Buffer Object
		SetupShaderStorageBuffers();

		// -- Setup UI stuffs
		InitUIMeshes();

		// Get Window Handle
		m_Window = systemManager->GetWindow();

		m_Width = m_Window->size().x;
		m_Height = m_Window->size().y;

		// Update the editor mode flag
		m_EditorMode = systemManager->IsEditor();

		// Create FBO, with the width and height of the window
		InitFramebuffers();

		// setup all the shaders
		SetupAllShaders();

		if (m_DebugDrawing) {
			m_GlobalTint.a = 0.3f;
		}
		else {
			m_GlobalTint.a = 1.f;
		}
	}
	
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	ignoredUIScenes.clear();

	// Set Cameras' starting position
	SetCameraPosition(CAMERA_TYPE::CAMERA_TYPE_EDITOR, {38, 20, 30});								// Position of camera
	SetCameraTarget(CAMERA_TYPE::CAMERA_TYPE_EDITOR, {0, 0, 0});									// Target of camera
	SetCameraProjection(CAMERA_TYPE::CAMERA_TYPE_ALL, GFX::CameraConstants::defaultFOV, m_Window->size(), 0.1f, 900.f);			// Projection of camera

	if (m_EditorMode)
	{
		// update both the editor and game camera
		UpdateCamera(CAMERA_TYPE::CAMERA_TYPE_ALL, 0.f);
	}
	else
	{
		// only update the game camera if editor mode is not enabled
		UpdateCamera(CAMERA_TYPE::CAMERA_TYPE_GAME, 0.f);
	}

	PINFO("Window size: %d, %d", m_Window->size().x, m_Window->size().y);
	PrintDeviceInfo();
}


/***************************************************************************/
/*!
\brief
	Update the Graphics system. Called every frame
	Updates the camera, and the renderer
*/
/**************************************************************************/
void GraphicsSystem::Update(float dt)
{
	m_RightClickHeld = systemManager->mInputActionSystem->GetKey(M_BUTTON_R);

	// Check window size for any updates
	CheckWindowSize();

	// update the camera's transformations, and its input
	if (m_EditorMode) {
		// update both the editor and game camera
		UpdateCamera(CAMERA_TYPE::CAMERA_TYPE_ALL, dt);
	}
	else {
		// only update the game camera if editor mode is not enabled
		UpdateCamera(CAMERA_TYPE::CAMERA_TYPE_GAME, dt);
	}

	Reset_Data();

	// Retrieve and update the mesh instances to be drawn
	auto meshRendererInstances = systemManager->ecs->GetEntitiesWith<MeshRenderer>();
	int animationID{ 0 };
	bool hasanimator{false};
	
	//for (Entity inst : meshRendererInstances)
#pragma omp parallel for
	for (int iter{}; iter < meshRendererInstances.size(); ++iter)
	{
		//if (systemManager->mIsInGamePause)		// Don't add models to be drawn if game pause
		//	break;

		Entity inst = meshRendererInstances[iter];
		mat4 final = mat4(1.f);

		// if the mesh instance is not active, skip it
		MeshRenderer& meshRenderer = inst.GetComponent<MeshRenderer>();
		Transform& transforminst = inst.GetComponent<Transform>();
		if (meshRenderer.mMeshRef.getdata(systemManager->mResourceTySystem->m_ResourceInstance) == nullptr)
			continue;
		
		hasanimator	= inst.HasComponent<Animator>();
		if (hasanimator) {
			// check if the mesh needs to be updated before taking the mesh data
			animationsToChangeMeshDelayedCheck(inst, dt);
		}

		void *tt = meshRenderer.mMeshRef.getdata(systemManager->mResourceTySystem->m_ResourceInstance);
		GFX::Mesh &meshinst = *reinterpret_cast<GFX::Mesh *>(tt);

		updateBloomValues(meshinst);

		update_CalculateLTW(inst, meshinst, transforminst, final);

		// Update the animation
		if (hasanimator && _ENABLE_ANIMATIONS && systemManager->mGraphicsSystem->m_EnableGlobalAnimations) {
			updateAnimations(inst, final, dt);
		}

		// pushback the relevant data to the instance buffer
		if (hasanimator) 
			AddInstance(meshinst, final, meshRenderer.mInstanceColor, static_cast<int>(m_Materials.size()), meshRenderer.mBloomThreshold, static_cast<unsigned>(inst.id), animationID++);

		else 
			AddInstance(meshinst, final, meshRenderer.mInstanceColor, static_cast<int>(m_Materials.size()), meshRenderer.mBloomThreshold, static_cast<unsigned>(inst.id));

		updateMaterials(meshRenderer);
	}

	update_NonMeshrendererColliders();

#pragma endregion

	// ---------------- PARTICLES WIP ----------------
	{
		auto emitterInstances = systemManager->ecs->GetEntitiesWith<ParticleEmitter>();

		for (Entity inst : emitterInstances)
		{
			ParticleEmitter& e = inst.GetComponent<ParticleEmitter>();
			vec3 pos = inst.GetComponent<Transform>().mTranslate;
			vec3 posOffset = e.mPosition;

			mat4 R = glm::toMat4(glm::quat(glm::radians(inst.GetComponent<Transform>().mRotate)));
			posOffset = vec3(R * vec4(posOffset, 1.f));
			pos += posOffset;	// emitter's position is an offset if has parent

			e.mCurrTime += dt;
			if (e.mEmit || e.mLoop && (e.mCurrTime > e.mLoopInterval))
			{
				EmitParticles(e, pos);
				e.mCurrTime = 0.f;
			}
			m_Renderer.AddCube(pos, vec3(0.2f, 0.2f, 0.2f));	// small white cube for each emitter
		}
	}
	
	update_Light();

	update_UI();

	updateSSBO_Data();

	update_Portals();

	// Render the depth scene first as shadow pass
	auto dirLightInstance = systemManager->ecs->GetEntitiesWith<DirectionalLight>();
	if (dirLightInstance.size())
	{
		Entity inst = dirLightInstance[0];
		DirectionalLight& dirLight = inst.GetComponent<DirectionalLight>();
		Transform& transform = inst.GetComponent<Transform>();
		dirLightPos = transform.mTranslate;

		mat4 view = glm::lookAt(dirLightPos, dirLight.mDirection + dirLightPos, { 0.f, 1.f, 0.f });
		mat4 proj = glm::ortho(-dirLight.mSize.x / 2, dirLight.mSize.x / 2, -dirLight.mSize.y / 2, dirLight.mSize.y / 2, dirLight.mNearFar.x, dirLight.mNearFar.y);
		lightSpaceMatrix = proj * view;

		RenderShadowMap();
		m_Renderer.AddCube(dirLightPos, vec3(1.f, 1.f, 1.f), { 1.f, 0.f, 1.f, 1.f });
		m_Renderer.AddLine(dirLightPos, dirLightPos + dirLight.mDirection * 3.f);
		m_Renderer.AddFrustum(lightSpaceMatrix, { 1.f, 0.f, 1.f, 1.f });
	}
	else
	{
		lightSpaceMatrix = mat4(1.f);
		dirLightPos = vec3(0.f);
	}
}



/***************************************************************************/
/*!
\brief
	Drawcall for the Graphics system. Called every frame
*/
/**************************************************************************/
void GraphicsSystem::Draw(float dt, bool forEditor)
{
	std::map<std::string, short> renderedMesh;
	auto meshRendererInstances = systemManager->ecs->GetEntitiesWith<MeshRenderer>();

	// Prepare and bind the Framebuffer to be rendered on
	m_MultisampleFBO.PrepForDraw();
	glDisable(GL_BLEND);				// Blending must be disabled for G-Buffer pass

	// Get the Camera View Projection
	mat4 camVP;
	if (forEditor)
		camVP = m_EditorCamera.viewProj();
	else
		camVP = GetCameraViewProjMatrix(CAMERA_TYPE::CAMERA_TYPE_GAME);

	// Update View projection matrix uniform
	m_GBufferShaderInst.Activate();
	glUniformMatrix4fv(m_GBufferShaderInst.GetUniformVP(), 1, GL_FALSE, &camVP[0][0]);
	GFX::Shader::Deactivate();
	m_AnimationShaderInst.Activate();
	glUniformMatrix4fv(m_AnimationShaderInst.GetUniformVP(), 1, GL_FALSE, &camVP[0][0]);
	GFX::Shader::Deactivate();

	GFX::FBO* fbo = forEditor ? &m_Fbo : &m_GameFbo;

#pragma region render all the mesh instances onto the editor camera framebuffer
	// Render all instances of a given mesh
	for (Entity inst : meshRendererInstances)
	{
		auto& meshrefptr = inst.GetComponent<MeshRenderer>();

		// the mesh instance has no meshrenderer
		if (meshrefptr.mMeshRef.getdata(systemManager->mResourceTySystem->m_ResourceInstance) == nullptr)
			continue;

		std::string meshstr = meshrefptr.mMeshPath;
		if (renderedMesh.find(meshstr) != renderedMesh.end()) {
			// the mesh has been rendered before, skip it
			continue;
		}

		// update the map of rendered meshes
		renderedMesh[meshstr] = 1;

		// render the mesh and its instances here
		GFX::Mesh& meshinst = *reinterpret_cast<GFX::Mesh*>(meshrefptr.mMeshRef.data);

		// Activate animation shader
		if (meshinst.mHasAnimation)
			m_AnimationShaderInst.Activate();
		else
			m_GBufferShaderInst.Activate();

		//Bind mesh's VAO, copy render data into VBO, Draw
		DrawAll(meshinst);

		//shaderinst.Deactivate();
		meshinst.UnbindVao();

		if (!forEditor)	// Clear all mesh instances after done drawing for game scene
			meshinst.ClearInstances();
	}
	GFX::Shader::Deactivate();

	if (forEditor)
		render_Debugdraw(camVP);

	// Perform blitting over pixel data from Multisample FBO -> intermediate FBO -> Destination FBO
	BlitMultiSampleToDestinationFBO(*fbo);

	// Compute the light pass with completed G-Buffers
	ComputeDeferredLight(forEditor);

	// Bind the appropriate FBO
	if (forEditor)		// Bind Editor FBO
	{
		m_Fbo.Bind();
		m_Fbo.DrawBuffers(true, true);
	}
	else				// Bind Game FBO
	{
		m_GameFbo.Bind();
		m_GameFbo.DrawBuffers(true);
	}
	glDepthMask(GL_TRUE);

	glEnable(GL_BLEND);			// Enable back blending for later draws

	// Set blend function back to usual for UI rendering
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Particles
	vec3 camPos = GetCamera(CAMERA_TYPE::CAMERA_TYPE_GAME).position();
	if (forEditor)
		camPos = GetCamera(CAMERA_TYPE::CAMERA_TYPE_EDITOR).position();
	ProcessEmitterAndParticle(camPos, dt);
	DrawAllParticles(forEditor);
	
	//!< === POST PROCESSING AND UI AREA ===
	if (!forEditor)
	{
		// Post Processing Bloom
		glDepthMask(GL_FALSE);

		if (systemManager->mGraphicsSystem->m_EnableBloom)
		{
			//!< Using Phys Based Bloom
			if (mBloomType == BloomType::PHYS_BASED_BLOOM)
			{
				m_PhysBloomRenderer.PrepForDraw();
				m_PhysBloomRenderer.RenderBloom(fbo->GetBrightColorsAttachment(), mFilterRadius);
				PostProcessing::AdditiveBlendFramebuffers(*fbo, fbo->GetColorAttachment(), m_PhysBloomRenderer.getBloomTexture());
			}

			//!< Using Gaussian Blur
			else
			{
				//Render the bloom for the Editor Framebuffer
				m_PingPongFbo.PrepForDraw();

				if (mBloomType == BloomType::GAUSSIANBLUR)
				{
					uid gaussianshaderstr("GaussianBlurShader");
					GFX::Shader& gaussianShaderInst = *systemManager->mResourceTySystem->get_Shader(gaussianshaderstr.id);
					m_PingPongFbo.GaussianBlur(gaussianShaderInst, *fbo, systemManager->mGraphicsSystem->mTexelOffset, systemManager->mGraphicsSystem->mSamplingWeight);
				}

				else if (mBloomType == BloomType::GAUSSIANBLUR_VER2)
				{
					uid gaussianshaderstr("GaussianBlurShaderVer2");
					GFX::Shader& gaussianShaderInst = *systemManager->mResourceTySystem->get_Shader(gaussianshaderstr.id);
					m_PingPongFbo.GaussianBlurShader(gaussianShaderInst, *fbo, systemManager->mGraphicsSystem->mSamplingWeight);
				}

				PostProcessing::AdditiveBlendFramebuffers(*fbo, fbo->GetColorAttachment(), m_PingPongFbo.pingpongColorbuffers[0]);
			}
		}

		glDepthMask(GL_TRUE);

		glEnable(GL_BLEND);			// Enable back blending for later draws

		// Set blend function back to usual for UI rendering
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Bind the appropriate FBO
		if (forEditor)		// Bind Editor FBO
		{
			m_Fbo.Bind();
			m_Fbo.DrawBuffers(true, true);
		}
		else				// Bind Game FBO
		{
			m_GameFbo.Bind();
			m_GameFbo.DrawBuffers(true);
		}

		if (!forEditor)
		{
			//!< UI Area
			{
				DrawAllWorldUI(forEditor);
				// Healthbar objects
				auto healthbarInstances = systemManager->ecs->GetEntitiesWith<Healthbar>();
				for (Entity inst : healthbarInstances)
				{
					Healthbar healthbar = inst.GetComponent<Healthbar>();
					unsigned healthTexID{}, frameTexID{};
					if (healthbar.mHealthTexture.getdata(systemManager->mResourceTySystem->m_ResourceInstance) != nullptr)
						healthTexID = reinterpret_cast<GFX::Texture*>(healthbar.mHealthTexture.data)->ID();
					if (healthbar.mFrameTexture.getdata(systemManager->mResourceTySystem->m_ResourceInstance) != nullptr)
						frameTexID = reinterpret_cast<GFX::Texture*>(healthbar.mFrameTexture.data)->ID();

					if (forEditor)
					{
						AddHealthbarInstance(inst, GetCameraPosition(CAMERA_TYPE::CAMERA_TYPE_EDITOR), frameTexID, true);
						AddHealthbarInstance(inst, GetCameraPosition(CAMERA_TYPE::CAMERA_TYPE_EDITOR), healthTexID);
					}
					else
					{
						AddHealthbarInstance(inst, GetCameraPosition(CAMERA_TYPE::CAMERA_TYPE_GAME), frameTexID, true);
						AddHealthbarInstance(inst, GetCameraPosition(CAMERA_TYPE::CAMERA_TYPE_GAME), healthTexID);
					}
				}
				m_HealthbarMesh.PrepForDraw();
				DrawAllHealthbarInstance(camVP);
				m_HealthbarMesh.ClearInstances();	// Clear data

				// Render UI objects
				m_UiShaderInst.Activate();		// Activate shader
				DrawAll2DInstances(m_UiShaderInst.GetHandle());
				GFX::Shader::Deactivate();	// Deactivate shader

				if (ENABLE_CROSSHAIR_IN_EDITOR_SCENE || !forEditor)
					DrawCrosshair();	// Render crosshair, if any
			}

			//!< Post Processing Chromatic Abberation and CRT
			{
				glDepthMask(GL_FALSE);

				//if (systemManager->mGraphicsSystem->m_EnableCRT && !systemManager->mGraphicsSystem->m_EnableChromaticAbberation)
				if (systemManager->mGraphicsSystem->m_EnableCRT)
				{
					m_ComputeCRTShader.Activate();

					glUniform1f(m_ComputeCRTTimeLocation, PostProcessing::getInstance().mCRT_AccumulationTime += dt);
					glUniform1f(m_ComputeCRTDistortionLocation, PostProcessing::getInstance().mCRT_DistortionValue);
					glUniform1f(m_ComputeCRTChromaticAbberationLocation, PostProcessing::getInstance().mCRT_ChromaticAbberationStrength);
					glUniform1i(m_ComputeCRTHeightOffsetLocation, PostProcessing::getInstance().mCRT_HeightOffset);

					// CRT post processing effect. Called here so it can be rendered over the UI
					PostProcessing::CRTBlendFramebuffers(*fbo, m_PingPongFbo, dt);

					m_ComputeCRTShader.Deactivate();
				}

				if (systemManager->mGraphicsSystem->m_EnableChromaticAbberation)
				{
					if (mBloomType == PHYS_BASED_BLOOM)
						PostProcessing::ChromaticAbbrebationBlendFramebuffers(*fbo, m_PhysBloomRenderer.getBloomTexture());
					else
						PostProcessing::ChromaticAbbrebationBlendFramebuffers(*fbo, m_PingPongFbo.pingpongColorbuffers[0]);
				}

				m_PingPongFbo.UnloadAndClear();
				glDepthMask(GL_TRUE);
			}
		}
	}

	ComputeGammaCorrection(forEditor);

	//// Bind the appropriate FBO
	//if (forEditor)		// Bind Editor FBO
	//{
	//	m_Fbo.Bind();
	//	m_Fbo.DrawBuffers(true, true);
	//}
	//else				// Bind Game FBO
	//{
	//	m_GameFbo.Bind();
	//	m_GameFbo.DrawBuffers(true);
	//}
	//glDepthMask(GL_TRUE);

	//glEnable(GL_BLEND);			// Enable back blending for later draws

	//// Set blend function back to usual for UI rendering
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//// Particles
	//DrawAllParticles();

#pragma endregion

	
	// Unbind FBO at end of draw frame
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/***************************************************************************/
/*!
\brief
	The Draw function isolated onto the editor framebuffer
*/
/**************************************************************************/
void GraphicsSystem::EditorDraw(float dt)
{
	(void)dt;

	std::map<std::string, short> renderedMesh;
	auto meshRendererInstances = systemManager->ecs->GetEntitiesWith<MeshRenderer>();

	// Prepare and bind the Framebuffer to be rendered on
	m_MultisampleFBO.PrepForDraw();
#if TEST_DEFERRED_LIGHT
	glDisable(GL_BLEND);
#endif

#if TEST_DEFERRED_LIGHT
	// Update View projection matrix uniform
	m_GBufferShaderInst.Activate();
	glUniformMatrix4fv(m_GBufferShaderInst.GetUniformVP(), 1, GL_FALSE, &m_EditorCamera.viewProj()[0][0]);
	GFX::Shader::Deactivate();
	m_AnimationShaderInst.Activate();
	glUniformMatrix4fv(m_AnimationShaderInst.GetUniformVP(), 1, GL_FALSE, &m_EditorCamera.viewProj()[0][0]);
	GFX::Shader::Deactivate();
#endif

#pragma region render all the mesh instances onto the editor camera framebuffer
	// Render all instances of a given mesh
	for (Entity inst : meshRendererInstances)
	{
		auto& meshrefptr = inst.GetComponent<MeshRenderer>();
		if (meshrefptr.mMeshRef.getdata(systemManager->mResourceTySystem->m_ResourceInstance) == nullptr)
			continue;

		std::string meshstr = meshrefptr.mMeshPath;
		if (renderedMesh.find(meshstr) != renderedMesh.end())
		{
			// the mesh has been rendered before, skip it
			continue;
		}

		// update the map of rendered meshes
		renderedMesh[meshstr] = 1;

		// render the mesh and its instances here
		GFX::Mesh &meshinst = *reinterpret_cast<GFX::Mesh *>(meshrefptr.mMeshRef.data);
#if TEST_DEFERRED_LIGHT

		// Activate animation shader
		if (meshinst.mHasAnimation)
			m_AnimationShaderInst.Activate();
		else
			m_GBufferShaderInst.Activate();

#endif

#if !TEST_DEFERRED_LIGHT
		GLuint debug_draw = glGetUniformLocation(shaderID, "globalTint");
		glUniform4fv(debug_draw, 1, glm::value_ptr(m_GlobalTint));
#endif
		 //Bind mesh's VAO, copy render data into VBO, Draw
		DrawAll(meshinst);

		//shaderinst.Deactivate();
		meshinst.UnbindVao();
	}
	m_Renderer.RenderAll(m_EditorCamera.viewProj());
#if TEST_DEFERRED_LIGHT
	GFX::Shader::Deactivate();
#endif

#if ENABLE_MULTISAMPLE
	// Perform blitting over pixel data from Multisample FBO -> intermediate FBO -> Destination FBO
	BlitMultiSampleToDestinationFBO(m_Fbo);
#if TEST_DEFERRED_LIGHT
	//DrawDeferredLight(m_EditorCamera.position(), m_Fbo);
	ComputeDeferredLight(true);
	glEnable(GL_BLEND);
#endif

#endif

	if (systemManager->mGraphicsSystem->m_EnableBloom)
	{
		if (mBloomType == BloomType::PHYS_BASED_BLOOM)
		{
			m_PhysBloomRenderer.PrepForDraw();
			m_PhysBloomRenderer.RenderBloom(m_Fbo.GetBrightColorsAttachment(), mFilterRadius);
			PostProcessing::AdditiveBlendFramebuffers(m_Fbo, m_Fbo.GetColorAttachment(), m_PhysBloomRenderer.getBloomTexture());
		}

		else
		{
			//Render the bloom for the Editor Framebuffer
			m_PingPongFbo.PrepForDraw();
	
			if (mBloomType == BloomType::GAUSSIANBLUR)
			{
				uid gaussianshaderstr("GaussianBlurShader");
				GFX::Shader& gaussianShaderInst = *systemManager->mResourceTySystem->get_Shader(gaussianshaderstr.id);
				m_PingPongFbo.GaussianBlur(gaussianShaderInst, m_Fbo, systemManager->mGraphicsSystem->mTexelOffset, systemManager->mGraphicsSystem->mSamplingWeight);
			}

			else if (mBloomType == BloomType::GAUSSIANBLUR_VER2)
			{
				uid gaussianshaderstr("GaussianBlurShaderVer2");
				GFX::Shader& gaussianShaderInst = *systemManager->mResourceTySystem->get_Shader(gaussianshaderstr.id);
				m_PingPongFbo.GaussianBlurShader(gaussianShaderInst, m_Fbo, systemManager->mGraphicsSystem->mSamplingWeight);
			}

			PostProcessing::AdditiveBlendFramebuffers(m_Fbo, m_Fbo.GetColorAttachment(), m_PingPongFbo.pingpongColorbuffers[0]);
		}
	}

	if (systemManager->mGraphicsSystem->m_EnableChromaticAbberation)
	{
		if (mBloomType == BloomType::PHYS_BASED_BLOOM) {
			PostProcessing::ChromaticAbbrebationBlendFramebuffers(m_Fbo, m_PhysBloomRenderer.getBloomTexture());
		}
		else {
			PostProcessing::ChromaticAbbrebationBlendFramebuffers(m_Fbo, m_PingPongFbo.pingpongColorbuffers[0]);
		}
	}


#if ENABLE_UI_IN_EDITOR_SCENE || ENABLE_CROSSHAIR_IN_EDITOR_SCENE
	m_Fbo.Bind();
	m_Fbo.DrawBuffers(true, true);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

#if ENABLE_UI_IN_EDITOR_SCENE
	// Render UI objects
	m_UiShaderInst.Activate();		// Activate shader
	DrawAll2DInstances(m_UiShaderInst.GetHandle());
	GFX::Shader::Deactivate();	// Deactivate shader
#endif

#if ENABLE_CROSSHAIR_IN_EDITOR_SCENE
	// Render crosshair, if any
	DrawCrosshair();
#endif
	 // Healthbar objects
	 auto healthbarInstances = systemManager->ecs->GetEntitiesWith<Healthbar>();
	 for (Entity inst : healthbarInstances)
	 {
		 AddHealthbarInstance(inst, GetCameraPosition(CAMERA_TYPE::CAMERA_TYPE_EDITOR), static_cast<int>(inst.id));
	 }
	 m_HealthbarMesh.PrepForDraw();
	 DrawAllHealthbarInstance(m_EditorCamera.viewProj());
	 m_HealthbarMesh.ClearInstances();	// Clear data

#pragma endregion

	m_Fbo.Unbind();
}

/***************************************************************************/
/*!
\brief
	The draw function isolated onto the game's framebuffer
*/
/**************************************************************************/
void GraphicsSystem::GameDraw(float dt)
{
	(void)dt;

	std::map<std::string, short> renderedMesh;
	auto meshRendererInstances = systemManager->ecs->GetEntitiesWith<MeshRenderer>();

	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return; // Cannot find camera Richmond
	Entity camera = localcamera.front();
	mat4 gameCamVP = camera.GetComponent<Camera>().mCamera.viewProj();

#if ENABLE_MULTISAMPLE
	m_MultisampleFBO.PrepForDraw();
#else
	// Prepare and bind the Framebuffer to be rendered on
	m_GameFbo.PrepForDraw();
#endif

	// Render all instances of a given mesh
	for (Entity inst : meshRendererInstances)
	{

		auto& meshrefptr = inst.GetComponent<MeshRenderer>();
		if (meshrefptr.mMeshRef.getdata(systemManager->mResourceTySystem->m_ResourceInstance) == nullptr)
			continue;

		std::string meshstr = meshrefptr.mMeshPath;
		if (renderedMesh.find(meshstr) != renderedMesh.end())
		{
			// the mesh has been rendered before, skip it
			continue;
		}

		// update the map of rendered meshes
		renderedMesh[meshstr] = 1;

		// render the mesh and its instances here
		GFX::Mesh &meshinst = *reinterpret_cast<GFX::Mesh *>(meshrefptr.mMeshRef.getdata(systemManager->mResourceTySystem->m_ResourceInstance));

		// gets the shader filepath
		std::string shader{};
		if (meshinst.mHasAnimation)
			shader = "AnimationShader";
		else
			shader = "PointLightShader";

		uid shaderstr(shader);
		GFX::Shader &shaderinst = *systemManager->mResourceTySystem->get_Shader(shaderstr.id);

		shaderinst.Activate();

		mat4 gameCamVP = camera.GetComponent<Camera>().mCamera.viewProj();
		glUniformMatrix4fv(shaderinst.GetUniformVP(), 1, GL_FALSE, &gameCamVP[0][0]);

		// Retrieve Point Light object
		auto lightEntity = systemManager->ecs->GetEntitiesWith<PointLight>();
		m_HasLight = !lightEntity.empty();
		GLuint mHasLightFlagLocation = shaderinst.GetUniformLocation("uHasLight");
		glUniform1i(mHasLightFlagLocation, m_HasLight);

		// Factor in Lighting if any
		if (m_HasLight)
		{
			GLuint mViewPosShaderLocation = shaderinst.GetUniformLocation("uViewPos");
			vec3 viewPos = camera.GetComponent<Camera>().mCamera.position();
			glUniform3fv(mViewPosShaderLocation, 1, &viewPos[0]);

			// Light count uniform
			GLuint mLightCountShaderLocation = shaderinst.GetUniformLocation("uLightCount");
			glUniform1i(mLightCountShaderLocation, m_LightCount);
		}

		// Bind mesh's VAO, copy render data into VBO, Draw
		DrawAll(meshinst);

		GLuint globalTintLocation = glGetUniformLocation(shaderinst.GetHandle(), "globalTint");
		glUniform4fv(globalTintLocation, 1, glm::value_ptr(vec4(1.f, 1.f, 1.f, 1.f)));

		meshinst.UnbindVao();
		shaderinst.Deactivate();

		// unbind the textures
		for (int i{0}; i < 4; i++)
		{
			if (meshrefptr.mTextureRef[i].data != nullptr)
			{
				glBindTextureUnit(i, 0);
			}
		}

		meshinst.ClearInstances();
		m_Renderer.ClearInstances();
	}

#if ENABLE_MULTISAMPLE
	m_GameFbo.Clear();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Copy pixel data from multisample FBO to editor FBO
	m_MultisampleFBO.BlitFramebuffer(m_GameFbo.GetID());
#endif

//	if (systemManager->mGraphicsSystem->m_EnableBloom)
//	{
//		m_PingPongFbo.PrepForDraw();
//
//		// Render the bloom for the Game Framebuffer
//
//#if 0
//		uid gaussianshaderstr("GaussianBlurShader");
//		GFX::Shader& gaussianShaderInst = *systemManager->mResourceTySystem->get_Shader(gaussianshaderstr.id);
//		m_PingPongFbo.GaussianBlur(gaussianShaderInst, m_GameFbo, systemManager->mGraphicsSystem->mTexelOffset, systemManager->mGraphicsSystem->mSamplingWeight);
//#else
//		uid gaussianshaderstr("GaussianBlurShaderVer2");
//		GFX::Shader& gaussianShaderInst = *systemManager->mResourceTySystem->get_Shader(gaussianshaderstr.id);
//		m_PingPongFbo.GaussianBlurShader(gaussianShaderInst, m_GameFbo, systemManager->mGraphicsSystem->mSamplingWeight);
//#endif
//
//		PostProcessing::AdditiveBlendFramebuffers(m_GameFbo, m_GameFbo.GetColorAttachment(), m_PingPongFbo.pingpongColorbuffers[0]);
//	}
//
//	if (systemManager->mGraphicsSystem->m_EnableChromaticAbberation)
//	{
//		//ChromaticAbbrebationBlendFramebuffers(m_GameFbo, m_GameFbo.GetBrightColorsAttachment());
//		PostProcessing::ChromaticAbbrebationBlendFramebuffers(m_GameFbo, m_PingPongFbo.pingpongColorbuffers[0]);
//	}

	m_GameFbo.Bind();
	m_GameFbo.DrawBuffers(true);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render UI objects
	m_UiShaderInst.Activate();		// Activate shader
	DrawAll2DInstances(m_UiShaderInst.GetHandle());
	m_UiShaderInst.Deactivate();	// Deactivate Shader

	// Render crosshair, if any
	DrawCrosshair();

	// Healthbar objects
	auto healthbarInstances = systemManager->ecs->GetEntitiesWith<Healthbar>();
	for (Entity inst : healthbarInstances)
	{
		AddHealthbarInstance(inst, GetCameraPosition(CAMERA_TYPE::CAMERA_TYPE_GAME), static_cast<int>(inst.id));
	}
	m_HealthbarMesh.PrepForDraw();
	DrawAllHealthbarInstance(gameCamVP);
	m_HealthbarMesh.ClearInstances();	// Clear data

	if (systemManager->mGraphicsSystem->m_EnableCRT && !systemManager->mGraphicsSystem->m_EnableChromaticAbberation)
	{
		// CRT post processing effect. Called here so it can be rendered over the UI
		PostProcessing::CRTBlendFramebuffers(m_GameFbo, m_PingPongFbo, dt);
	}

	if (systemManager->mGraphicsSystem->m_EnableBloom)
	{
		if (mBloomType == BloomType::PHYS_BASED_BLOOM)
		{
			m_PhysBloomRenderer.PrepForDraw();
			//m_PhysBloomRenderer.RenderBloom(m_GameFbo.GetColorAttachment(), mFilterRadius);
			m_PhysBloomRenderer.RenderBloom(m_GameFbo.GetBrightColorsAttachment(), mFilterRadius);
			PostProcessing::AdditiveBlendFramebuffers(m_GameFbo, m_GameFbo.GetColorAttachment(), m_PhysBloomRenderer.getBloomTexture());
		}

		else
		{
			// Render the bloom for the Game Framebuffer
			m_PingPongFbo.PrepForDraw();

			if (mBloomType == BloomType::GAUSSIANBLUR)
			{
				uid gaussianshaderstr("GaussianBlurShader");
				GFX::Shader& gaussianShaderInst = *systemManager->mResourceTySystem->get_Shader(gaussianshaderstr.id);
				m_PingPongFbo.GaussianBlur(gaussianShaderInst, m_GameFbo, systemManager->mGraphicsSystem->mTexelOffset, systemManager->mGraphicsSystem->mSamplingWeight);
			}

			else if (mBloomType == BloomType::GAUSSIANBLUR_VER2)
			{
				uid gaussianshaderstr("GaussianBlurShaderVer2");
				GFX::Shader& gaussianShaderInst = *systemManager->mResourceTySystem->get_Shader(gaussianshaderstr.id);
				m_PingPongFbo.GaussianBlurShader(gaussianShaderInst, m_GameFbo, systemManager->mGraphicsSystem->mSamplingWeight);
			}

			PostProcessing::AdditiveBlendFramebuffers(m_GameFbo, m_GameFbo.GetColorAttachment(), m_PingPongFbo.pingpongColorbuffers[0]);
		}
	}

	m_GameFbo.Unbind();
	m_PingPongFbo.UnloadAndClear();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void GraphicsSystem::DrawGameScene()
{
	// Blit data from Game FBO to default FBO
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_GameFbo.GetID());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glReadBuffer(GL_COLOR_ATTACHMENT0);	// Read from color attachment
	glDrawBuffer(GL_BACK);				// Draw to Front attachment for default FBO

	glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/***************************************************************************/
/*!
\brief
	Exit the Graphics system. Unalloc anything unnecessary
*/
/**************************************************************************/
void GraphicsSystem::Exit()
{
	m_Image2DMesh.Destroy();
	m_HealthbarMesh.Destroy();
	m_WorldUIMesh.Destroy();
}

/***************************************************************************/
/*!
\brief
	Adds an instance of a mesh to be drawn, For instancing
*/
/**************************************************************************/
void GraphicsSystem::AddInstance(GFX::Mesh &mesh, Transform transform, const vec4& color, int meshID, const vec4& bloomthreshold, unsigned entityID)
{
	// Local to world transformation
	mat4 scale = glm::scale(transform.mScale);
	mat4 translate = glm::translate(transform.mTranslate);

	glm::quat quatRotate = glm::quat(transform.mRotate); // Retrieve quaternions via euler angles
	glm::mat4 rotation = glm::toMat4(quatRotate);		 // Retrieve rotation matrix via quaternions

	mat4 world = translate * rotation * scale;
	mesh.mLTW.push_back(world);
	mesh.mTexEntID.push_back(vec4((float)meshID + 0.5f, (float)entityID + 0.5f, 0, 0));
	mesh.mColors.push_back(color);
	mesh.mBloomThresholds.push_back(bloomthreshold);
}

void GraphicsSystem::AddInstance(GFX::Mesh &mesh, mat4 transform, const vec4& color, int meshID, const vec4& bloomthreshold, unsigned entityID, int animInstanceID)
{
	mesh.mLTW.push_back(transform);
	mesh.mColors.push_back(color);
	if (animInstanceID < 0)
		animInstanceID = -2;
	mesh.mTexEntID.push_back(vec4((float)meshID + 0.5f, (float)entityID + 0.5f, (float)animInstanceID + 0.5f, 0));
	mesh.mBloomThresholds.push_back(bloomthreshold);
}

void GraphicsSystem::SetAllEntityBloomThreshold(glm::vec4 threshold, std::string meshName)
{
	//!< Helper
	auto getMeshname = [](std::string filepath) -> std::string
	{
		// returns AT-AT
		std::string ret_str = filepath.substr(filepath.find_last_of("/") + 1);
		ret_str = ret_str.substr(0, ret_str.find_first_of("."));
		return ret_str;
	};

	auto meshRendererInstances = systemManager->ecs->GetEntitiesWith<MeshRenderer>();
	for (Entity inst : meshRendererInstances)
	{
		auto& meshinst = inst.GetComponent<MeshRenderer>();
		if ( getMeshname(meshinst.mMeshPath) == meshName)
		{
			meshinst.mBloomThreshold = threshold;
		}
	}
}

/***************************************************************************/
/*!
\brief
	Updates the camera's position.
*/
/**************************************************************************/
void GraphicsSystem::SetCameraPosition(CAMERA_TYPE type, vec3 position)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return; // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		camera.GetComponent<Camera>().mCamera.SetPosition(position);
		break;

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		m_EditorCamera.SetPosition(position);
		break;

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		camera.GetComponent<Camera>().mCamera.SetPosition(position);
		m_EditorCamera.SetPosition(position);
		break;
	}
}

/***************************************************************************/
/*!
\brief
	Updates the camera's target.
*/
/**************************************************************************/
void GraphicsSystem::SetCameraTarget(CAMERA_TYPE type, vec3 position)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return; // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		camera.GetComponent<Camera>().mCamera.SetTarget(position);
		break;

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		m_EditorCamera.SetTarget(position);
		break;

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		camera.GetComponent<Camera>().mCamera.SetTarget(position);
		m_EditorCamera.SetTarget(position);
		break;
	}
}

/***************************************************************************/
/*!
\brief
	Updates the camera's projection.
*/
/**************************************************************************/
void GraphicsSystem::SetCameraProjection(CAMERA_TYPE type, float fovDegree, ivec2 size, float nearZ, float farZ)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return; // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{

	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		camera.GetComponent<Camera>().mCamera.SetProjection(fovDegree, size, nearZ, farZ);
		break;

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		m_EditorCamera.SetProjection(fovDegree, size, nearZ, farZ);
		break;

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		camera.GetComponent<Camera>().mCamera.SetProjection(fovDegree, size, nearZ, farZ);
		m_EditorCamera.SetProjection(fovDegree, size, nearZ, farZ);
		break;
	}
}

/***************************************************************************/
/*!
\brief
	Sets the size of the camera according to the enum provided
*/
/**************************************************************************/
void GraphicsSystem::SetCameraSize(CAMERA_TYPE type, ivec2 size)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return; // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		camera.GetComponent<Camera>().mCamera.SetSize(size);
		break;

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		m_EditorCamera.SetSize(size);
		break;

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		camera.GetComponent<Camera>().mCamera.SetSize(size);
		m_EditorCamera.SetSize(size);
		break;
	}
}


inline void drawViewFrustum(Entity gameCamera)
{
	auto& camera = gameCamera.GetComponent<Camera>().mCamera;
	systemManager->mGraphicsSystem->m_Renderer.AddFrustum(camera.viewProj(), vec4(1.f, 1.f, 0.5f, 1.f));
	mat4 inv = glm::inverse(camera.mView);
}


/***************************************************************************/
/*!
\brief
	Updates the camera and its input according to the enum provided
*/
/**************************************************************************/
void GraphicsSystem::UpdateCamera(CAMERA_TYPE type, const float &dt)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	Entity GameCamera;

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		{
			if (localcamera.empty())
				return;
			GameCamera = localcamera.front();		// there will only be one game camera
			auto& GameCameraTransform = GameCamera.GetComponent<Transform>();
			auto& GameCameraComponent = GameCamera.GetComponent<Camera>();

			GameCameraComponent.mCamera.mTarget += (GameCameraTransform.mTranslate - GameCameraComponent.mCamera.mPosition);
			GameCameraComponent.mCamera.mPosition = GameCameraTransform.mTranslate;
			GameCameraComponent.mCamera.mPitch = GameCameraTransform.mRotate.y;
			GameCameraComponent.mCamera.mYaw = GameCameraTransform.mRotate.x;
			GameCameraComponent.mCamera.Update();

			// debug drawing
			if (systemManager->mGraphicsSystem->m_DebugDrawing) {
				drawViewFrustum(GameCamera);
			}
			break;
		}

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		{
			Camera_Input::getInstance().updateCameraInput(m_EditorCamera, dt);
			m_EditorCamera.Update();
			break;
		}

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		{
			// == update editor camera ==
			Camera_Input::getInstance().updateCameraInput(m_EditorCamera, dt);
			m_EditorCamera.Update();

			// == update game camera ==
			if (localcamera.empty())
				return;
			GameCamera = localcamera.front();		// there will only be one game camera

			auto& GameCameraTransform = GameCamera.GetComponent<Transform>();
			auto& GameCameraComponent = GameCamera.GetComponent<Camera>();

			GameCameraComponent.mCamera.mTarget += (GameCameraTransform.mTranslate - GameCameraComponent.mCamera.mPosition);
			GameCameraComponent.mCamera.mPosition = GameCameraTransform.mTranslate;
			GameCameraComponent.mCamera.mPitch = GameCameraTransform.mRotate.y;
			GameCameraComponent.mCamera.mYaw = GameCameraTransform.mRotate.x;

			GameCameraComponent.mCamera.Update();

			// debug drawing
			if (systemManager->mGraphicsSystem->m_DebugDrawing) {
				drawViewFrustum(GameCamera);
			}
			break;
		}
	}
}

GFX::Camera GraphicsSystem::GetCamera(CAMERA_TYPE type)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return GFX::Camera(); // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		return camera.GetComponent<Camera>().mCamera;

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		return m_EditorCamera;

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		break;
	}
	PERROR("camera spoil - graphicssystem.cpp line 751");
	return {};
}

/***************************************************************************/
/*!
\brief
	Gets the camera's position depending on the enum provided
*/
/**************************************************************************/
vec3 GraphicsSystem::GetCameraPosition(CAMERA_TYPE type)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return vec3(); // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		return camera.GetComponent<Camera>().mCamera.position();

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		return m_EditorCamera.position();

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		break;
	}
	PERROR("camera spoil - graphicssystem.cpp line 751");
	return {};
}

/***************************************************************************/
/*!
\brief
	Gets the camera's target depending on the enum provided
*/
/**************************************************************************/
vec3 GraphicsSystem::GetCameraTarget(CAMERA_TYPE type)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return vec3(); // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		return camera.GetComponent<Camera>().mCamera.target();

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		return m_EditorCamera.target();

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		return vec3();
		break;
	}
	PERROR("camera spoil - graphicssystem.cpp line 780");
	return {};
}

/***************************************************************************/
/*!
\brief
	Gets the direction the camera is pointint to depending on the enum
	provided
*/
/**************************************************************************/
vec3 GraphicsSystem::GetCameraDirection(CAMERA_TYPE type)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return vec3(); // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		return camera.GetComponent<Camera>().mCamera.direction();

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		return m_EditorCamera.direction();

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		return vec3();
		break;
	}
	PERROR("camera spoil - graphicssystem.cpp GetCameraDirection()");
	return {};
}

mat4 GraphicsSystem::GetCameraViewMatrix(CAMERA_TYPE type)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return mat4(1.0); // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		return camera.GetComponent<Camera>().mCamera.mView;

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		return m_EditorCamera.mView;

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		return mat4(1.0);
		break;
	}
	PERROR("camera spoil - graphicssystem.cpp GetCameraViewMatrix()");
	return {};
}

mat4 GraphicsSystem::GetCameraViewProjMatrix(CAMERA_TYPE type)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return mat4(1.0); // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		return camera.GetComponent<Camera>().mCamera.viewProj();

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		return m_EditorCamera.viewProj();

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		return mat4(1.0);
		break;
	}
	PERROR("camera spoil - graphicssystem.cpp GetCameraViewMatrix()");
	return {};
}

ivec2 GraphicsSystem::GetCameraSize(CAMERA_TYPE type)
{
	auto localcamera = systemManager->ecs->GetEntitiesWith<Camera>();
	if (localcamera.empty())
		return ivec2(); // Cannot find camera Richmond
	Entity camera = localcamera.front();

	switch (type)
	{
	case CAMERA_TYPE::CAMERA_TYPE_GAME:
		return camera.GetComponent<Camera>().mCamera.mSize;

	case CAMERA_TYPE::CAMERA_TYPE_EDITOR:
		return m_EditorCamera.mSize;

	case CAMERA_TYPE::CAMERA_TYPE_ALL:
		return ivec2();
		break;
	}
	PERROR("camera spoil - graphicssystem.cpp GetCameraSize()");
	return {};
}

/***************************************************************************/
/*!
\brief
	Draws all instances of this mesh
*/
/**************************************************************************/
void GraphicsSystem::DrawAll(GFX::Mesh &mesh)
{
	mesh.DrawAllInstances();
}



/***************************************************************************/
/*!
\brief
	Helper function -- to print mat4 matrices
*/
/**************************************************************************/
void GraphicsSystem::PrintMat4(const glm::mat4 &input)
{
	std::cout << "=============================================================================\n";
	std::cout << "| " << input[0][0] << " | " << input[1][0] << " | " << input[2][0] << " | " << input[3][0] << "\n";
	std::cout << "| " << input[0][1] << " | " << input[1][1] << " | " << input[2][1] << " | " << input[3][1] << "\n";
	std::cout << "| " << input[0][2] << " | " << input[1][2] << " | " << input[2][2] << " | " << input[3][2] << "\n";
	std::cout << "| " << input[0][3] << " | " << input[1][3] << " | " << input[2][3] << " | " << input[3][3] << "\n";
	std::cout << "=============================================================================\n";
}

/***************************************************************************/
/*!
\brief
	Global enum to unpause all animations
*/
/**************************************************************************/
void GraphicsSystem::UnpauseGlobalAnimation() 
{
	m_EnableGlobalAnimations = true;
}

void GraphicsSystem::CheckWindowSize()
{
	int width{}, height{};

	glfwGetFramebufferSize(m_Window->GetHandle(), &width, &height);

	if (width != m_Width || height != m_Height)
	{
		ResizeWindow({ width, height });
	}
}

void GraphicsSystem::SetFullscreen(bool flag)
{
	m_Window->SetFullScreenFlag(flag);
}

void GraphicsSystem::ResizeWindow(ivec2 newSize)
{
	if (!newSize.x || !newSize.y)
		return;

	// Update Window
	m_Window->SetWindowSize(newSize);

	m_Width = newSize.x;
	m_Height = newSize.y;

	// Update viewport
	glViewport(0, 0, newSize.x, newSize.y);

	// Update FBOs
	m_Fbo.Resize(newSize.x, newSize.y);
	m_GameFbo.Resize(newSize.x, newSize.y);
	m_MultisampleFBO.Resize(newSize.x, newSize.y);
	m_IntermediateFBO.Resize(newSize.x, newSize.y);
	m_PingPongFbo.Resize(newSize.x, newSize.y);
	m_PhysBloomRenderer.Resize(newSize.x, newSize.y);
	m_ShadowFbo.Resize(newSize.x * 4, newSize.y * 4);

	// Input
	glBindImageTexture(2, m_IntermediateFBO.GetBrightColorsAttachment(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(3, m_IntermediateFBO.GetFragPosAttachment(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(4, m_IntermediateFBO.GetNormalAttachment(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(5, m_IntermediateFBO.GetAlbedoSpecAttachment(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(6, m_IntermediateFBO.GetEmissionAttachment(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	SetCameraSize(CAMERA_TYPE::CAMERA_TYPE_ALL, newSize);
}

void GraphicsSystem::PrintDeviceInfo()
{
	printf("\n----------------- DEVICE INFO -----------------\n");

	GLint integer[3];
	glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, integer);
	printf("\n Max Compute Shader Storage Blocks: %d", integer[0]);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, integer);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, integer + 1);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, integer + 2);
	printf("\n Max Compute Shader Work Group Count: %d, %d, %d", integer[0], integer[1], integer[2]);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, 0, integer);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, 1, integer + 1);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, 2, integer + 2);
	printf("\n Max Compute Shader Work Group Invocations: %d, %d, %d", integer[0], integer[1], integer[2]);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, integer);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, integer + 1);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, integer + 2);
	printf("\n Max Compute Shader Work Group Size: %d, %d, %d", integer[0], integer[1], integer[2]);
}

void GraphicsSystem::ClampCursor()
{
	double posX, posY;

	m_Window->GetCursorPos(&posX, &posY);
	posX = glm::clamp(posX, -20.0, static_cast<double>(m_Window->GetScreenWidth()));
	posY = glm::clamp(posY, -20.0, static_cast<double>(m_Window->GetScreenHeight()));

	glfwSetCursorPos(m_Window->GetHandle(), posX, posY);
}

void GraphicsSystem::HideCursor(bool hideCursor)
{
	// if there is no change in the state, then return
	if(m_IsCursorEnabledForEditor == !hideCursor)
		return;

	m_IsCursorEnabledForEditor = !hideCursor;

	if (hideCursor) {
		glfwSetInputMode(m_Window->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	else {
		glfwSetInputMode(m_Window->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void GraphicsSystem::ComputeGammaCorrection(bool forEditor)
{
	if (forEditor)	// Draw to Editor FBO
	{
		glBindImageTexture(0, m_Fbo.GetColorAttachment(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	}
	else
	{
		glBindImageTexture(0, m_GameFbo.GetColorAttachment(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	}

	m_ComputeGammaShader.Activate();

	glUniform1f(m_ComputeGammaLocation, m_Gamma);

	int num_group_x = glm::ceil(m_Width / 29);
	int num_group_y = glm::ceil(m_Height / 29);
	glDispatchCompute(num_group_x, num_group_y, 1);
	// make sure writing to image is done before reading
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	m_ComputeGammaShader.Deactivate();
}

void GraphicsSystem::Unload()
{
	auto meshRendererInstances = systemManager->ecs->GetEntitiesWith<MeshRenderer>();

	for (Entity inst : meshRendererInstances)
	{
		GFX::Mesh* meshinst = reinterpret_cast<GFX::Mesh*>(inst.GetComponent<MeshRenderer>().mMeshRef.getdata(systemManager->mResourceTySystem->m_ResourceInstance));
		if (meshinst == nullptr)
			continue;

		// clears all instances of LTW, Colors for every mesh
		meshinst->ClearInstances();
	}

	// clears all debug draw instances
	m_Renderer.ClearInstances();
}

GLuint64 GraphicsSystem::GetAndStoreBindlessTextureHandle(int texID)
{
	if (texID < 0)
		return 0;

	// Store the 64-bit handle
	GLuint64 handle = glGetTextureHandleARB((unsigned)texID);
	m_MaterialHandles[(unsigned)texID] = handle;

	if (!glIsTextureHandleResidentARB(handle))
	{
		glMakeTextureHandleResidentARB(handle);
	}

	return handle;
}

int GraphicsSystem::StoreUITexture(int texID)
{
	if (texID < 0)
		return -1;

	// Store the 64-bit handle
	GLuint64 handle = glGetTextureHandleARB((unsigned)texID);
	m_UIHandles[(unsigned)texID] = handle;

	if (!glIsTextureHandleResidentARB(handle))
	{
		glMakeTextureHandleResidentARB(handle);
	}

	UITextures.emplace_back(handle);

	return UITextures.size() - 1;
}

void GraphicsSystem::SetupShaderStorageBuffers()
{
	// All materials of each instance -- Location 1
	m_MaterialSsbo.Create(sizeof(MaterialSSBO) * MAX_INSTANCES, 1);

	// All Point Lights in the Scene -- Location 2
	m_PointLightSsbo.Create(sizeof(PointLightSSBO) * MAX_POINT_LIGHT, 2);

	// Final Bone Matrix for Animation -- Location 3
	m_FinalBoneMatrixSsbo.Create(sizeof(mat4) * MAX_NUM_BONES * MAX_INSTANCES, 3);

	// All spotlight in the scene -- Location 4
	m_SpotlightSsbo.Create(sizeof(SpotLightSSBO) * MAX_SPOTLIGHT, 4);

	// All particle emitters in the scene -- Location 5
	m_ParticleEmitterSsbo.Create(sizeof(ParticleEmitterSSBO) * MAX_PARTICLE_EMITTER, 5);

	// All particles in the scene -- Location 6
	m_ParticleSsbo.Create(sizeof(ParticleSSBO) * MAX_PARTICLES, 6);

	// Particle Freelist -- Location 7
	m_ParticleFreelistSsbo.Create(sizeof(int) * (MAX_PARTICLES + 1), 7);
	std::vector<int>indices(MAX_PARTICLES + 1, 0);
	indices[0] = MAX_PARTICLES;
	for (int i{1}, j{ MAX_PARTICLES - 1 }; i < MAX_PARTICLES + 1; ++i, --j) indices[i] = j;
	m_ParticleFreelistSsbo.SubData(sizeof(int) * (MAX_PARTICLES + 1), indices.data());

	// All UI textures in the scene -- Location 8
	m_UITextureSsbo.Create(sizeof(GLuint64) * MAX_UI, 8);
}

void GraphicsSystem::DrawAll2DInstances(unsigned shaderID)
{
	(void)shaderID;

	//if (m_Image2DStore.size() == 0)
	//	return;

	//// Bind Textures to OpenGL context
	//for (size_t i{}; i < m_Image2DStore.size(); ++i)
	//{
	//	glBindTextureUnit(static_cast<GLuint>(i), m_Image2DStore[static_cast<GLuint>(i)]);
	//}

	// Bind 2D quad VAO
	m_Image2DMesh.BindVao();

	// Draw call for UI
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(m_Image2DMesh.mLTW.size()));

	// Unbind 2D quad VAO
	m_Image2DMesh.UnbindVao();

	//// Unbind Textures from openGL context
	//for (size_t i{}; i < m_Image2DStore.size(); ++i)
	//{
	//	glBindTextureUnit(static_cast<GLuint>(i), 0);
	//}
}

void GraphicsSystem::Add2DImageInstance(float width, float height, vec3 const& position, unsigned texHandle, unsigned entityID, float degree, vec4 const& color, float slider)
{
	float half_w = m_Width * 0.5f;
	float half_h = m_Height * 0.5f;

	mat4 world =
	{
		vec4(width / half_w, 0.f, 0.f, 0.f),
		vec4(0.f, height / half_h, 0.f, 0.f),
		vec4(0.f, 0.f, 1.f, 0.f),
		vec4(position.x / m_Width, position.y / m_Height, -position.z, 1.f)
	};

	int texIndex{};
	if (texHandle >= 0)
		texIndex = texHandle;
	else
		texIndex = -2;

	m_Image2DMesh.mLTW.push_back(world);
	m_Image2DMesh.mColors.push_back(color);
	m_Image2DMesh.mTexEntID.push_back(vec4((float)texIndex + 0.5f, (float)entityID + 0.5f, degree, slider));
}

void GraphicsSystem::Add2DImageWorldInstance(Transform transform, unsigned texHandle, unsigned entityID, float degree, vec4 const& color, float slider)
{
	mat4 S = glm::scale(transform.mScale);
	mat4 R = glm::toMat4(glm::quat(glm::radians(transform.mRotate)));
	mat4 T = glm::translate(transform.mTranslate);

	mat4 world = T * R * S;

	int texIndex{};
	if (texHandle >= 0)
		texIndex = texHandle;
	else
		texIndex = -2;

	m_WorldUIMesh.mLTW.push_back(world);
	m_WorldUIMesh.mColors.push_back(color);
	m_WorldUIMesh.mTexEntID.push_back(vec4((float)texIndex + 0.5f, (float)entityID + 0.5f, degree, slider));
}
int GraphicsSystem::StoreTextureIndex(unsigned texHandle)
{
	if (m_Image2DStore.size() >= 32)
	{
		std::cout << "Too many different textures to be rendered in 1 frame, texture discarded\n";
		return -1;
	}

	// Locate if ID already in container
	auto it = std::find(m_Image2DStore.cbegin(), m_Image2DStore.cend(), (GLint)texHandle);
	if (it == m_Image2DStore.cend())	// ID not in container
	{
		m_Image2DStore.push_back((GLint)texHandle);
		return (int)m_Image2DStore.size() - 1;
	}

	// ID is already in container
	int pos = (int)(it - m_Image2DStore.cbegin());
	return pos;
}

void GraphicsSystem::DrawAllHealthbarInstance(const mat4& viewProj)
{
	if (m_HealthbarMesh.mLTW.size() == 0)
		return;

	// Bind Textures to OpenGL context
	for (size_t i{}; i < m_Image2DStore.size(); ++i)
	{
		glBindTextureUnit(static_cast<GLuint>(i), m_Image2DStore[static_cast<GLuint>(i)]);
	}

	// Bind shader and VAO
	m_HealthbarShaderInst.Activate();
	m_HealthbarMesh.BindVao();

	// Set view projection matrix uniform
	glUniformMatrix4fv(m_HealthbarViewProjLocation, 1, GL_FALSE, &viewProj[0][0]);

	// Draw call
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(m_HealthbarMesh.mLTW.size()));

	// Unbinding shader and VAO
	GFX::Shader::Deactivate();
	m_HealthbarMesh.UnbindVao();

	// Unbind Textures from openGL context
	for (size_t i{}; i < m_Image2DStore.size(); ++i)
	{
		glBindTextureUnit(static_cast<GLuint>(i), 0);
	}
}

void GraphicsSystem::AddHealthbarInstance(Entity e, const vec3& camPos, unsigned texHandle, bool forFrame)
{
	Healthbar& healthbar = e.GetComponent<Healthbar>();
	vec3 originPos = e.GetComponent<Transform>().mTranslate;

	// Compute the rotation vectors
	vec3 normal = camPos - originPos;
	vec3 up = { 0, 1, 0 };
	vec3 right = glm::cross(up, normal);
	vec3 forward = glm::cross(right, normal);

	mat4 scale = {
		vec4(healthbar.mWidth, 0.f, 0.f, 0.f),
		vec4(0.f, healthbar.mHeight, 0.f, 0.f),
		vec4(0.f, 0.f, 1.f, 0.f),
		vec4(0.f, 0.f, 0.f, 1.f)
	};

	// Rotation matrix to always face the camera
	mat4 rotate = {
		vec4(glm::normalize(right), 0.0f),
		vec4(glm::normalize(forward), 0.0f),
		vec4(glm::normalize(normal), 0.0f),
		vec4(healthbar.mPosition + originPos, 1.0f)
	};

	mat4 world = rotate * scale;

	// Update colors
	float healthbarRatio{};
	if (healthbar.mMaxHealth == 0) {
		healthbarRatio = 0;
		PWARNING("Max health of an enemy is 0, should not be!")
	} else {
		healthbarRatio = (healthbar.mHealth/healthbar.mMaxHealth) * 100.f;
		healthbarRatio = healthbarRatio < 0 ? 0 : healthbarRatio; // Cap healthbar ratio min to 0
	}

	// Texture UI for Health
	int texIndex{};
	if (texHandle > 0)
		texIndex = StoreTextureIndex(texHandle);
	else
		texIndex = -2;

	vec4 healthColor = vec4(healthbar.mBackColor.x, healthbar.mBackColor.y, healthbar.mBackColor.z, healthbarRatio);
	m_HealthbarMesh.mColors.push_back(healthColor);

	float frameFlag{ -5.f };
	if (forFrame) frameFlag = 5.f;

	if (texHandle > 0)
		m_HealthbarMesh.mTexEntID.push_back(vec4(texIndex + 0.5f, -2.f, frameFlag, 0.f));	// set y component for checking at shader side, z for frame
	else
		m_HealthbarMesh.mTexEntID.push_back(healthbar.mHealthColor);
	// Update LTW matrix
	m_HealthbarMesh.mLTW.push_back(world);
}

void GraphicsSystem::SetupCrosshairShaderLocations()
{
	m_CrosshairThicknessLocation = m_CrosshairShaderInst.GetUniformLocation("uThickness");
	m_CrosshairInnerLocation = m_CrosshairShaderInst.GetUniformLocation("uInner");
	m_CrosshairOuterLocation = m_CrosshairShaderInst.GetUniformLocation("uOuter");
	m_CrosshairColorLocation = m_CrosshairShaderInst.GetUniformLocation("uColor");
}

void GraphicsSystem::DrawCrosshair()
{
	// Get entity with crosshair component
	auto ent = systemManager->ecs->GetEntitiesWith<Crosshair>();

	if(ent.size() == 0)
		return;

	// Use crosshair shader program adn VAO
	m_CrosshairShaderInst.Activate();
	m_Image2DMesh.BindVao();

	for (Entity inst : ent)	// only if entity exists
	{
		auto crosshairInst = inst.GetComponent<Crosshair>();

		glUniform1f(m_CrosshairThicknessLocation, crosshairInst.mThickness);
		glUniform1f(m_CrosshairInnerLocation, crosshairInst.mInner);
		glUniform1f(m_CrosshairOuterLocation, crosshairInst.mOuter);
		glUniform4fv(m_CrosshairColorLocation, 1, &crosshairInst.mColor[0]);
	}

	// Draw call
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 1);

	// Stop using crosshair shader program and VAO
	m_CrosshairShaderInst.Deactivate();
	m_Image2DMesh.UnbindVao();
}

void GraphicsSystem::DrawDeferredLight(const vec3& camPos, GFX::FBO& destFbo)
{
	destFbo.Bind();
	// Bind VAO and activate shader
	m_Image2DMesh.BindVao();
	m_DeferredLightShaderInst.Activate();
	
	// Set Shader uniforms
	glUniform3fv(m_DeferredCamPosLocation, 1, &camPos[0]);
	glUniform1i(m_DeferredLightCountLocation, m_LightCount);

	// Draw
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 1);
	
	// Unbind VAO and Deactivate shader
	m_DeferredLightShaderInst.Deactivate();
	m_Image2DMesh.UnbindVao();
	destFbo.Unbind();
}

void GraphicsSystem::BlitMultiSampleToDestinationFBO(GFX::FBO& destFbo)
{
	// Clear out intermediate before blitting
	m_IntermediateFBO.Clear();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Copy pixel data from multisample FBO to intermediate FBO
	m_MultisampleFBO.BlitFramebuffer(m_IntermediateFBO.GetID());

	// Clear out destination FBO before blitting
	destFbo.Clear();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Copies only the entity ID data to editor FBO
	m_IntermediateFBO.BlitFramebuffer(destFbo.GetID());
}

void GraphicsSystem::SetupAllShaders()
{
	// Initialize the UI Shader
	uid uiShaderstr("UIShader");
	m_UiShaderInst = *systemManager->mResourceTySystem->get_Shader(uiShaderstr.id);
	
	// Uniforms of UI Shader
	m_UiShaderInst.Activate();
	GLuint uniform_tex = glGetUniformLocation(m_UiShaderInst.GetHandle(), "uTex2d");
	glUniform1iv(uniform_tex, (GLsizei)m_Textures.size(), m_Textures.data()); // Passing texture Binding units to frag shader [0 - 31]
	m_UiShaderInst.Deactivate();

	// Initialize the Crosshair shader
	uid crosshairShaderstr("CrosshairShader");
	m_CrosshairShaderInst = *systemManager->mResourceTySystem->get_Shader(crosshairShaderstr.id);
	m_CrosshairShaderInst.Activate();
	SetupCrosshairShaderLocations();
	m_CrosshairShaderInst.Deactivate();

	// Initialize the Healthbar shader and uniform location
	uid healthbarShaderstr("healthbarShader");
	m_HealthbarShaderInst = *systemManager->mResourceTySystem->get_Shader(healthbarShaderstr.id);
	m_HealthbarShaderInst.Activate();
	m_HealthbarViewProjLocation = m_HealthbarShaderInst.GetUniformLocation("uViewProj");
	GLuint health_uniform_tex = glGetUniformLocation(m_HealthbarShaderInst.GetHandle(), "uTex2d");
	glUniform1iv(health_uniform_tex, (GLsizei)m_Textures.size(), m_Textures.data()); // Passing texture Binding units to frag shader [0 - 31]
	m_HealthbarShaderInst.Deactivate();

	// Initialize the Healthbar shader and uniform location
	uid animationShaderstr("AnimationShader");
	m_AnimationShaderInst = *systemManager->mResourceTySystem->get_Shader(animationShaderstr.id);
	
	// Initialize the deferred light shader and uniform location
	uid deferredLightShaderstr("deferredLightShader");
	m_DeferredLightShaderInst = *systemManager->mResourceTySystem->get_Shader(deferredLightShaderstr.id);
	m_DeferredLightShaderInst.Activate();
	m_DeferredCamPosLocation = m_DeferredLightShaderInst.GetUniformLocation("uCamPos");
	m_DeferredLightCountLocation = m_DeferredLightShaderInst.GetUniformLocation("uLightCount");

	// Bind Textures
	GLuint64 handle = glGetTextureHandleARB(m_IntermediateFBO.GetFragPosAttachment());
	glMakeTextureHandleResidentARB(handle);
	glUniform1ui64ARB(m_DeferredLightShaderInst.GetUniformLocation("gPosition"), handle);

	handle = glGetTextureHandleARB(m_IntermediateFBO.GetNormalAttachment());
	glMakeTextureHandleResidentARB(handle);
	glUniform1ui64ARB(m_DeferredLightShaderInst.GetUniformLocation("gNormal"), handle);

	handle = glGetTextureHandleARB(m_IntermediateFBO.GetAlbedoSpecAttachment());
	glMakeTextureHandleResidentARB(handle);
	glUniform1ui64ARB(m_DeferredLightShaderInst.GetUniformLocation("gAlbedoSpec"), handle);

	handle = glGetTextureHandleARB(m_IntermediateFBO.GetEmissionAttachment());
	glMakeTextureHandleResidentARB(handle);
	glUniform1ui64ARB(m_DeferredLightShaderInst.GetUniformLocation("gEmission"), handle);
	m_DeferredLightShaderInst.Deactivate();

	// Initialize the G-buffer shader and uniform location
	uid gBufferShaderstr("gBufferShader");
	m_GBufferShaderInst = *systemManager->mResourceTySystem->get_Shader(gBufferShaderstr.id);

	// Initialize the quad 3D Shader
	uid quad3DShaderstr("Quad3D");
	m_Quad3DShaderInst = *systemManager->mResourceTySystem->get_Shader(quad3DShaderstr.id);
	m_Quad3DShaderInst.Activate();
	uniform_tex = glGetUniformLocation(m_Quad3DShaderInst.GetHandle(), "uTex2d");
	glUniform1iv(uniform_tex, (GLsizei)m_Textures.size(), m_Textures.data()); // Passing texture Binding units to frag shader [0 - 31]
	m_Quad3DShaderInst.Deactivate();

	// Initialize the Shadow Map shader
	uid shadowMapShaderstr("ShadowMapShader");
	shadowMapShaderInst = *systemManager->mResourceTySystem->get_Shader(shadowMapShaderstr.id);
	shadowMapShaderInst.Activate();
	shadowMapLightSpaceMatrixLocation = shadowMapShaderInst.GetUniformLocation("uLightSpaceMatrix");
	shadowMapShaderInst.Deactivate();


	//!<< Compile Compute shader >>
	computeDeferred.CreateShaderFromFile("../assets/shader_files/computePBR.glsl");
	computeDeferred.Activate();
	m_ComputeDeferredLightCountLocation = computeDeferred.GetUniformLocation("uLightCount");
	m_ComputeDeferredSpotlightCountLocation = computeDeferred.GetUniformLocation("uSpotlightCount");
	m_ComputeDeferredCamPosLocation = computeDeferred.GetUniformLocation("uCamPos");
	m_ComputeDeferredGlobalTintLocation = computeDeferred.GetUniformLocation("uGlobalTint");
	m_ComputeDeferredGlobalBloomLocation = computeDeferred.GetUniformLocation("uGlobalBloomThreshold");
	computePBRLightSpaceMatrixLocation = computeDeferred.GetUniformLocation("uLightSpaceMatrix");
	m_ComputeDirLightPosLocation = computeDeferred.GetUniformLocation("uDirLightPos");
	GFX::Shader::Deactivate();

	m_ComputeCRTShader.CreateShaderFromFile("../assets/shader_files/computeCRT.glsl");
	m_ComputeCRTShader.Activate();
	m_ComputeCRTTimeLocation = m_ComputeCRTShader.GetUniformLocation("mCRT_AccumulationTime");
	m_ComputeCRTHeightOffsetLocation = m_ComputeCRTShader.GetUniformLocation("heightoffset");
	m_ComputeCRTDistortionLocation = m_ComputeCRTShader.GetUniformLocation("distortion_value");
	m_ComputeCRTChromaticAbberationLocation = m_ComputeCRTShader.GetUniformLocation("chromatic_abberation");
	GFX::Shader::Deactivate();

	m_ComputeAddBlendShader.CreateShaderFromFile("../assets/shader_files/computeCRT.glsl");
	m_ComputeAddBlendShader.Activate();
	m_ComputeAddBlendExposureLocation = m_ComputeAddBlendShader.GetUniformLocation("Exposure");
	GFX::Shader::Deactivate();

	m_ComputeEmitterShader.CreateShaderFromFile("../assets/shader_files/computeEmitter.glsl");
	m_ComputeEmitterShader.Activate();
	m_ComputeEmitterCountLocation = m_ComputeEmitterShader.GetUniformLocation("uEmitterCount");
	m_ComputeEmitterCamPosLocation = m_ComputeEmitterShader.GetUniformLocation("uCamPos");
	m_ComputeEmitterSeedLocation = m_ComputeEmitterShader.GetUniformLocation("uSeed");
	GFX::Shader::Deactivate();

	m_ComputeParticleShader.CreateShaderFromFile("../assets/shader_files/computeParticle.glsl");
	m_ComputeParticleShader.Activate();
	m_ComputeParticleDeltaTimeLocation = m_ComputeParticleShader.GetUniformLocation("uDeltaTime");
	m_ComputeParticleCamPosLocation = m_ComputeParticleShader.GetUniformLocation("uCamPos");
	m_ComputeParticleCountLocation = m_ComputeParticleShader.GetUniformLocation("uParticleCount");
	GFX::Shader::Deactivate();

	uid particleShaderStr("ParticleShader");
	m_ParticleShaderInst = *systemManager->mResourceTySystem->get_Shader(particleShaderStr.id);

	m_ComputeGammaShader.CreateShaderFromFile("../assets/shader_files/computeGamma.glsl");
	m_ComputeGammaShader.Activate();
	m_ComputeGammaLocation = m_ComputeGammaShader.GetUniformLocation("uGamma");
	GFX::Shader::Deactivate();
}

mat4 GraphicsSystem::GetPortalViewMatrix(GFX::Camera const& camera, Transform const& sourcePortal, Transform const& destPortal)
{
	mat4 camView = camera.mView;

	mat4 destT = glm::translate(destPortal.mTranslate);
	mat4 destR = glm::toMat4(glm::quat(glm::radians(destPortal.mRotate)));
	mat4 destFinal = destT * destR;

	mat4 srcT = glm::translate(sourcePortal.mTranslate);
	mat4 srcR = glm::toMat4(glm::quat(glm::radians(sourcePortal.mRotate)));
	mat4 srcFinal = srcT * srcR;

	mat4 destinationView = camView * srcFinal * glm::rotate(glm::radians(180.f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::inverse(destFinal);

	mat4 obliqueProj = ObliqueNearPlaneClipping(camera.mProjection, destinationView, sourcePortal, destPortal);

	return obliqueProj * destinationView;
}

mat4 GraphicsSystem::ObliqueNearPlaneClipping(mat4 proj, mat4 view, Transform const& srcPortal, Transform const& destPortal)
{
	// Calculate the near plane
	mat3 destR = glm::toMat3(glm::quat(glm::radians(destPortal.mRotate)));
	vec3 planeNormal = destR * vec3(0.f, 0.f, 1.f);
	//float d = glm::dot(planeNormal, srcPortal.mTranslate);
	float d = glm::length(destPortal.mTranslate);
	vec4 nearPlane = vec4(planeNormal, d);
	nearPlane = glm::inverse(view) * nearPlane;

	if (nearPlane.w > 0.f)
		return proj;

	vec4 q = glm::inverse(proj) * vec4(
										glm::sign(nearPlane.x),
										glm::sign(nearPlane.y),
										1.f,
										1.f);


	// Calculate the Scale plane vector
	vec4 c = nearPlane * (2.f / glm::dot(nearPlane, q));

	mat4 newProj = proj;
	newProj = glm::row(newProj, 2, c - glm::row(newProj, 3));

	return newProj;
}

void GraphicsSystem::AddPortalInstance(Entity portal)
{
	float entID = static_cast<float>(portal.id);

	Portal p = portal.GetComponent<Portal>();
	mat4 S1 = glm::scale(p.mScale1);
	mat4 R1 = glm::toMat4(glm::quat(glm::radians(p.mRotate1)));
	mat4 T1 = glm::translate(p.mTranslate1);
	mat4 ltw1 = T1 * R1 * S1;

	m_WorldUIMesh.mLTW.push_back(ltw1);
	m_WorldUIMesh.mColors.push_back(vec4(1.f, 1.f, 1.f, 1.f));
	m_WorldUIMesh.mTexEntID.push_back(vec4(- 2.f, entID + 0.5f, 0.f, 0.f));

	mat4 S2 = glm::scale(p.mScale2);
	mat4 R2 = glm::toMat4(glm::quat(glm::radians(p.mRotate2)));
	mat4 T2 = glm::translate(p.mTranslate2);
	mat4 ltw2 = T2 * R2 * S2;

	m_WorldUIMesh.mLTW.push_back(ltw2);
	m_WorldUIMesh.mColors.push_back(vec4(1.f, 1.f, 1.f, 1.f));
	m_WorldUIMesh.mTexEntID.push_back(vec4(-2.f, entID + 0.5f, 0.f, 0.f));

	if (m_DebugDrawing)
	{
		vec4 normal1 = vec4(0.f, 0.f, 1.f, 0.f);
		normal1 = R1 * normal1;
		m_Renderer.AddLine(p.mTranslate1, p.mTranslate1 + vec3(normal1) * 5.f, vec4(0.f, 1.f, 0.f, 1.f));

		vec4 normal2 = vec4(0.f, 0.f, 1.f, 0.f);
		normal2 = R2 * normal2;
		m_Renderer.AddLine(p.mTranslate2, p.mTranslate2 + vec3(normal2) * 5.f, vec4(0.f, 1.f, 0.f, 1.f));
	}
}

void GraphicsSystem::DrawAllWorldUI(bool editorDraw)
{
	if (m_WorldUIMesh.mLTW.empty())
		return;

	// Get appropriate camera
	GFX::Camera camera = GetCamera(CAMERA_TYPE::CAMERA_TYPE_GAME);
	if (editorDraw)
		camera = GetCamera(CAMERA_TYPE::CAMERA_TYPE_EDITOR);

	mat4 camVP = camera.viewProj();

	m_WorldUIMesh.BindVao();
	m_Quad3DShaderInst.Activate();
	glUniformMatrix4fv(m_Quad3DShaderInst.GetUniformVP(), 1, GL_FALSE, &camVP[0][0]);

	glDepthFunc(GL_LEQUAL);
	//glDisable(GL_CULL_FACE);
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, GLsizei(m_WorldUIMesh.mLTW.size()));
	//glEnable(GL_CULL_FACE);

	m_WorldUIMesh.UnbindVao();
	GFX::Shader::Deactivate();
}

void GraphicsSystem::DrawAllParticles(bool forEditor)
{
	GFX::Camera camera = GetCamera(CAMERA_TYPE::CAMERA_TYPE_GAME);
	if (forEditor)
		camera = GetCamera(CAMERA_TYPE::CAMERA_TYPE_EDITOR);

	mat4 camVP = camera.viewProj();

	m_ParticleMesh.BindVao();
	m_ParticleShaderInst.Activate();
	glUniformMatrix4fv(m_ParticleShaderInst.GetUniformVP(), 1, GL_FALSE, &camVP[0][0]);

	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, MAX_PARTICLES);
	glDepthMask(GL_TRUE);

	m_ParticleMesh.UnbindVao();
	GFX::Shader::Deactivate();
}

void GraphicsSystem::ProcessEmitterAndParticle(vec3 const& camPos, float dt)
{
	// Send data over to the SSBO
	m_ParticleEmitterSsbo.SubData(m_Emitters.size() * sizeof(ParticleEmitterSSBO), m_Emitters.data());

	// Update new emitters to create particles
	UpdateEmitters(camPos);

	// Update the properties of all particles
	UpdateParticles(camPos, dt);
}

void GraphicsSystem::UpdateEmitters(vec3 const& camPos)
{
	//if (m_Emitters.empty())
	//	return;

	int emitterCount = m_Emitters.size();

	int num_group_x = emitterCount;

	// Activate Compute shader
	m_ComputeEmitterShader.Activate();

	// Send uniforms to shader
	glUniform1i(m_ComputeEmitterCountLocation, emitterCount);
	glUniform1f(m_ComputeEmitterSeedLocation, m_Window->GetTime());
	glUniform3fv(m_ComputeEmitterCamPosLocation, 1, &camPos[0]);

	glDispatchCompute(num_group_x, 1, 1);
	// make sure writing to image is done before reading
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// Deactivate compute shader
	m_ComputeEmitterShader.Deactivate();

	// Remove emitters
	m_Emitters.clear();
}

void GraphicsSystem::UpdateParticles(vec3 const& camPos, float dt)
{
	int num_group_x = 32;
	int num_group_y = ((MAX_PARTICLES - 1)/ (num_group_x + 1) - 1) / num_group_x + 1;

	// Activate Compute shader
	m_ComputeParticleShader.Activate();

	glUniform1f(m_ComputeParticleDeltaTimeLocation, dt);
	glUniform1i(m_ComputeParticleCountLocation, MAX_PARTICLES);
	glUniform3fv(m_ComputeParticleCamPosLocation, 1, &camPos[0]);

	glDispatchCompute(num_group_x, num_group_y, 1);
	// make sure writing to image is done before reading
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// Deactivate compute shader
	m_ComputeParticleShader.Deactivate();
}

void GraphicsSystem::EmitParticles(ParticleEmitter& e, vec3 const& position)
{
	ParticleEmitterSSBO eSsbo;
	eSsbo.Init(e);
	eSsbo.mPositionSpeed = vec4(position, e.mSpeed);

	// TODO: To add texture of particles
	unsigned texID{};
	if (e.mTexture.getdata(systemManager->mResourceTySystem->m_ResourceInstance) != nullptr)
		texID = reinterpret_cast<GFX::Texture*>(e.mTexture.data)->ID();
	eSsbo.mTexture = GetAndStoreBindlessTextureHandle(texID);

	// Add emitter into container
	m_Emitters.emplace_back(eSsbo);

	if (!e.mLoop)
		e.mEmit = false;	// Set to false after emission if not looping
}

void GraphicsSystem::RenderShadowMap()
{
	glCullFace(GL_FRONT);

	// Bind shadow FBO to be rendered to
	m_ShadowFbo.PrepForDraw();

	// set viewport size to be shadow map size
	glViewport(0, 0, m_ShadowFbo.mWidth, m_ShadowFbo.mHeight);

	// Bind Shaders and update uniforms
	m_AnimationShaderInst.Activate();	// Animation Shader
	glUniformMatrix4fv(m_AnimationShaderInst.GetUniformVP(), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
	GFX::Shader::Deactivate();

	shadowMapShaderInst.Activate();		// Shadow mapping shader for non-animated meshes
	glUniformMatrix4fv(shadowMapLightSpaceMatrixLocation, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
	GFX::Shader::Deactivate();

	// Rendering of meshes
	std::map<std::string, short> renderedMesh;
	auto meshRendererInstances = systemManager->ecs->GetEntitiesWith<MeshRenderer>();

	for (Entity inst : meshRendererInstances)
	{
		auto& meshrefptr = inst.GetComponent<MeshRenderer>();

		if (meshrefptr.mCastShadow == false)	// Don't render objects that do not cast shadows
			continue;

		// the mesh instance has no meshrenderer
		if (meshrefptr.mMeshRef.getdata(systemManager->mResourceTySystem->m_ResourceInstance) == nullptr)
			continue;

		std::string meshstr = meshrefptr.mMeshPath;
		if (renderedMesh.find(meshstr) != renderedMesh.end()) {
			// the mesh has been rendered before, skip it
			continue;
		}

		// update the map of rendered meshes
		renderedMesh[meshstr] = 1;

		// render the mesh and its instances here
		GFX::Mesh& meshinst = *reinterpret_cast<GFX::Mesh*>(meshrefptr.mMeshRef.data);

		// Activate animation shader
		if (meshinst.mHasAnimation)
			m_AnimationShaderInst.Activate();
		else
			shadowMapShaderInst.Activate();

		//Bind mesh's VAO, copy render data into VBO, Draw
		DrawAll(meshinst);

		//shaderinst.Deactivate();
		meshinst.UnbindVao();
	}
	GFX::Shader::Deactivate();

	m_ShadowFbo.Unbind();

	glCullFace(GL_NONE);

	// Set viewport back to original
	glViewport(0, 0, m_Width, m_Height);
}

void GraphicsSystem::ComputeDeferredLight(bool editorDraw)
{
	if (editorDraw)	// Draw to Editor FBO
	{
		glBindImageTexture(0, m_Fbo.GetColorAttachment(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_Fbo.GetBrightColorsAttachment(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}
	else
	{
		glBindImageTexture(0, m_GameFbo.GetColorAttachment(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_GameFbo.GetBrightColorsAttachment(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}

	// Set appropriate camera position
	vec3 camPos = GetCameraPosition(CAMERA_TYPE::CAMERA_TYPE_GAME);
	if (editorDraw)
		camPos = m_EditorCamera.position();

	computeDeferred.Activate();

	glUniform1i(m_ComputeDeferredLightCountLocation, m_LightCount);
	glUniform1i(m_ComputeDeferredSpotlightCountLocation, m_SpotlightCount);
	glUniform3fv(m_ComputeDeferredCamPosLocation, 1, &camPos[0]);
	glUniform3fv(m_ComputeDirLightPosLocation, 1, &dirLightPos[0]);
	glUniform4fv(m_ComputeDeferredGlobalTintLocation, 1, &m_GlobalTint[0]);
	vec4 globalBloom = vec4(mAmbientBloomThreshold, m_EnableBloom);
	glUniform4fv(m_ComputeDeferredGlobalBloomLocation, 1, &globalBloom[0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ShadowFbo.GetDepthMap());

	// Light space matrix
	glUniformMatrix4fv(computePBRLightSpaceMatrixLocation, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

	int num_group_x = glm::ceil(m_Width / 29);
	int num_group_y = glm::ceil(m_Height / 29);
	glDispatchCompute(num_group_x, num_group_y, 1);
	// make sure writing to image is done before reading
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTexture(GL_TEXTURE_2D, 0);
	computeDeferred.Deactivate();
}

/***************************************************************************/
/*!
\brief
	Global enum to pause all animations
*/
/**************************************************************************/
void GraphicsSystem::PauseGlobalAnimation()
{
	m_EnableGlobalAnimations = false;
}


void MeshRenderer::SetColor(const vec4& color)
{
	mInstanceColor = color;
}


void MeshRenderer::SetMesh(const std::string& meshName, Entity inst)
{
	// gets the guid from the fbx descriptor file
	std::string descFilepath = systemManager->mResourceTySystem->fbx_path + meshName + ".fbx.desc";
	unsigned guid = _GEOM::GetGUID(descFilepath);

	mMeshRef.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->get_mesh(guid));
	mMeshPath = systemManager->mResourceTySystem->compiled_geom_path + meshName + ".geom";
	mMeshName = meshName;

	GFX::Mesh* meshinst = reinterpret_cast<GFX::Mesh*>(mMeshRef.data);
	if (inst.HasComponent<Animator>())
	{
		if (meshinst->mHasAnimation) {
			// change the animation to the new mesh's
			inst.GetComponent<Animator>().mAnimator.SetAnimation(&meshinst->mAnimation[0]);
		}

		else {
			// the new mesh has no animation, but the current entity has an animator component
			inst.GetComponent<Animator>().mAnimator.SetAnimation(nullptr);
		}
	}
}


//void MeshRenderer::SetMesh(const std::string& meshName, Entity inst, unsigned int index)
//{
//	// gets the guid from the fbx descriptor file
//	std::string descFilepath = systemManager->mResourceTySystem->fbx_path + meshName + ".fbx.desc";
//	unsigned guid = _GEOM::GetGUID(descFilepath);
//
//	mMeshRef.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->get_mesh(guid));
//	mMeshPath = systemManager->mResourceTySystem->compiled_geom_path + meshName + ".geom";
//
//	GFX::Mesh* meshinst = reinterpret_cast<GFX::Mesh*>(mMeshRef.data);
//	if (inst.HasComponent<Animator>())
//	{
//		if (meshinst->mHasAnimation) {
//			// change the animation to the new mesh's
//			inst.GetComponent<Animator>().mAnimator.SetAnimation(&meshinst->mAnimation[index]);
//		}
//
//		else {
//			// the new mesh has no animation, but the current entity has an animator component
//			inst.GetComponent<Animator>().mAnimator.SetAnimation(nullptr);
//		}
//	}
//}
// 

void MeshRenderer::SetMeshDelayed(const std::string& meshName, Entity inst)
{
	// gets the guid from the fbx descriptor file
	std::string descFilepath = systemManager->mResourceTySystem->fbx_path + meshName + ".fbx.desc";
	unsigned guid = _GEOM::GetGUID(descFilepath);

	GFX::Mesh* meshinst = systemManager->mResourceTySystem->get_mesh(guid);

	if (inst.HasComponent<Animator>() && meshinst->mHasAnimation)
	{
		// animation is available, then set the animation after current animation is done
		auto& animatorinst = inst.GetComponent<Animator>();
		animatorinst.mAnimator.mToChangeMeshDelayed.first = true;
		animatorinst.mAnimator.mToChangeMeshDelayed.second = meshName;
	}

	else
	{
		// there is no animation, then set the mesh immediately
		mMeshRef.data	= reinterpret_cast<void*>(meshinst);
		mMeshPath		= systemManager->mResourceTySystem->compiled_geom_path + meshName + ".geom";
		mMeshName		= meshName;

		if (inst.HasComponent<Animator>())
			inst.GetComponent<Animator>().mAnimator.SetAnimation(nullptr);	// reset the animation as needed
	}
}


std::string MeshRenderer::GetMeshName()
{
	return mMeshName;
}


void MeshRenderer::SetTexture(MaterialType MaterialType, const std::string& Texturename)
{
	std::string MaterialInstancePath = systemManager->mResourceTySystem->compressed_texture_path + Texturename + ".ctexture";
	mMaterialInstancePath[MaterialType] = MaterialInstancePath;

	std::string descFilepath = MaterialInstancePath + ".desc";
	unsigned guid = _GEOM::GetGUID(descFilepath);

	mTextureRef[MaterialType].data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
}


void PointLight::SetColor(const vec3& color)
{
	mLightColor = color;
}


void UIrenderer::SetTexture(const std::string& Texturename)
{
	std::string MaterialInstancePath = systemManager->mResourceTySystem->compressed_texture_path + Texturename + ".ctexture";

	std::string descFilepath = MaterialInstancePath + ".desc";
	unsigned guid = _GEOM::GetGUID(descFilepath);

	mTextureRef.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
}