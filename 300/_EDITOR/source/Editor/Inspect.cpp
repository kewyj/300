/*!*************************************************************************
****
\file Inspect.cpp
\author Han ChengWen, Charissa Yip , Cheong Ming Lun
\par DP email: c.han@\digipen.edu, charissa.yip\@digipen.edu, m.cheong\@digipen.edu
\date 28-09-2023
\brief
This header file contains the inspector window and relative functions to run
the system logic.

The functions

- init
Initialize related data needed for Inspector window to draw

- update
Run Inspector logic that checks for component and render related data which
provides user to edit in run time


- Add_component
Add component button and logic implementation that allows user to add
components

- Transform::Inspect
Inspect display for Transform component
Included ImGui custom dragfloat3 function for data required in command system.

-PlaneCollider::Inspect
Inspect display for Collider component
Included ImGui custom dragfloat3 function for data required in command system.

- BoxCollider::Inspect
Inspect display for BoxCollider component
Included ImGui custom dragfloat3 function for data required in command system.

- CircleCollider::Inspect
Inspect display for CircleCollider component
Included ImGui custom dragfloat3 function for data required in command system.

- Texture::Inspect
Inspect display for Texture components
Error pop up will appear when user attempts to include the wrong asset type.

- Camera::Inspect
Inspect display for Camera components
Included ImGui custom dragfloat3 function for data required in command system.

-RigidBody::Inspect
Inspect display for RigidBody components

- Audio::Inspect
Inspect display for Audio components
****************************************************************************
***/

#include "Inspect.h"
#include "Editor.h"
#include "ECS/ECS_Components.h"
#include "ECS/ECS_Systems.h"
#include "Hierarchy.h"
#include <math.h>
#include "imgui_stdlib.h"
#include "ScriptingSystem.h"

#include "ResourceManagerTy.h"
#include "Debug/Logger.h"
//#include "Audio/AudioSystem.h"
#include "Audio/AudioSystemRevamp.h"
#include "Input/InputMapSystem.h"
#include <TextureCompressor.h>
#include "Script/Script.h"
#include "AI/AIManager.h"

#include <descriptor.h>
#include <string>
#include "EditorReflection/EditorReflection.h"


void popup(std::string name, _GEOM::Texture_DescriptorData& desc, std::string& dataname, ref& data, bool& trigger);
void popup(std::string name, _GEOM::DescriptorData& desc, std::string& dataname, ref& data, bool& trigger);
/***************************************************************************/
/*!
\brief
	Init function for Inspector Window
*/
/**************************************************************************/
void Inspect::init() {
//empty
}
/***************************************************************************/
/*!
\brief
	Update function for Inspector Window
*/
/**************************************************************************/
void Inspect::update() 
{
	if (Hierarchy::selectionOn) {
		
		Entity ent(Hierarchy::selectedId);

		General& Inspector = ent.GetComponent<General>();
		Inspector.Inspect();

		if (ent.HasComponent<Transform>()) {
			Transform& transform = ent.GetComponent<Transform>();
			transform.Inspect();
		}
		if (ent.HasComponent<Scripts>())
		{
			Scripts& scripts = ent.GetComponent<Scripts>();
			scripts.Inspect(ent.id);
		}
		if (ent.HasComponent<Animator>()) {
			Animator& ani = ent.GetComponent<Animator>();
			ani.Inspect();
		}
		if (ent.HasComponent<MeshRenderer>()) {
			MeshRenderer& Meshrender = ent.GetComponent<MeshRenderer>();
			Meshrender.Inspect();
		}
		if (ent.HasComponent<RigidBody>()) {
			RigidBody& rigidBody = ent.GetComponent<RigidBody>();
			rigidBody.Inspect();
		}
		if (ent.HasComponent<BoxCollider>()) {
			BoxCollider& boxCollider = ent.GetComponent<BoxCollider>();
			boxCollider.Inspect();
		}
		if (ent.HasComponent<SphereCollider>()) {
			SphereCollider& sphereCollider = ent.GetComponent<SphereCollider>();
			sphereCollider.Inspect();
		}
		if (ent.HasComponent<CapsuleCollider>()) {
			CapsuleCollider& capsuleCollider = ent.GetComponent<CapsuleCollider>();
			capsuleCollider.Inspect();
		}
		if (ent.HasComponent<MeshCollider>()) {
			MeshCollider& meshCollider = ent.GetComponent<MeshCollider>();
			meshCollider.Inspect();
		}
		if (ent.HasComponent<PointLight>()) {
			PointLight& pointLight = ent.GetComponent<PointLight>();
			pointLight.Inspect();
		}
		if (ent.HasComponent<Spotlight>()) {
			Spotlight& spotlight = ent.GetComponent<Spotlight>();
			spotlight.Inspect();
		}

		if (ent.HasComponent<Camera>()) {
			Camera& cam = ent.GetComponent<Camera>();
			cam.Inspect();
		}

		if (ent.HasComponent<Audio>()) {
			Audio& audio = ent.GetComponent<Audio>();
			audio.Inspect();
		}

		if (ent.HasComponent<AudioListener>()) {
			AudioListener& listener = ent.GetComponent<AudioListener>();
			listener.Inspect();
		}
		if (ent.HasComponent<UIrenderer>()) {
			UIrenderer& render = ent.GetComponent<UIrenderer>();
			render.Inspect();
		}

		if (ent.HasComponent<VFX>()) {
			VFX& vfx = ent.GetComponent<VFX>();
			vfx.Inspect();
		}

		if (ent.HasComponent<AISetting>()) {
			AISetting& aiSetting = ent.GetComponent<AISetting>();
			aiSetting.Inspect();
		}

		if (ent.HasComponent<Crosshair>()) {
			Crosshair& crosshair = ent.GetComponent<Crosshair>();
			crosshair.Inspect();
		}

		if (ent.HasComponent<Healthbar>()) {
			Healthbar& healthbar = ent.GetComponent<Healthbar>();
			healthbar.Inspect();
		}

		if (ent.HasComponent<Button>()) {
			Button& button = ent.GetComponent<Button>();
			button.Inspect();
		}
		if (ent.HasComponent<DirectionalLight>()) {
			DirectionalLight& dirLight = ent.GetComponent<DirectionalLight>();
			dirLight.Inspect();
		}

		if (ent.HasComponent<ParticleEmitter>()) {
			ParticleEmitter& emitter = ent.GetComponent<ParticleEmitter>();
			emitter.Inspect();
		}

		if (ent.HasComponent<Portal>()) {
			Portal& portal = ent.GetComponent<Portal>();
			portal.Inspect();
		}

		if (ent.HasComponent<Gun>())
		{
			Gun& gun = ent.GetComponent<Gun>();
			gun.Inspect();
		}
		//--------------------------------------------// must be at the LAST OF THIS LOOP
		Add_component(); 
	}

}
/***************************************************************************/
/*!
\brief
	Add Component function for Inspector Window
*/
/**************************************************************************/
void Inspect::Add_component() {


	auto initialCursorPos = ImGui::GetCursorPos();
	auto windowSize = ImGui::GetWindowSize();
	float centralizedCursorpos = (windowSize.x - 200) * 0.5f;
	ImGui::SetCursorPosX(centralizedCursorpos);


	ImGui::SetCursorPosX((ImGui::GetWindowSize().x )/2 - ImGui::CalcTextSize("      ComponentList ").x);

	if (ImGui::BeginCombo("##add","ComponentList"))
	{


		if (ImGui::Selectable("RigidBody")) {
			if(!Entity(Hierarchy::selectedId).HasComponent<RigidBody>())
				Entity(Hierarchy::selectedId).AddComponent<RigidBody>();
		}

		if (ImGui::Selectable("BoxCollider")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<BoxCollider>())
				Entity(Hierarchy::selectedId).AddComponent<BoxCollider>();
		}

		if (ImGui::Selectable("MeshCollider")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<MeshCollider>())
				Entity(Hierarchy::selectedId).AddComponent<MeshCollider>();
		}

		if (ImGui::Selectable("Scripts")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<Scripts>())
				Entity(Hierarchy::selectedId).AddComponent<Scripts>();
		}

		if (ImGui::Selectable("SphereCollider")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<SphereCollider>())
				Entity(Hierarchy::selectedId).AddComponent<SphereCollider>();
		}
		if (ImGui::Selectable("CapsuleCollider")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<CapsuleCollider>())
				Entity(Hierarchy::selectedId).AddComponent<CapsuleCollider>();
		}
		if (ImGui::Selectable("Animator")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<Animator>())
				Entity(Hierarchy::selectedId).AddComponent<Animator>();
		}

		if (ImGui::Selectable("PointLight"))
		{
			if (!Entity(Hierarchy::selectedId).HasComponent<PointLight>())
				Entity(Hierarchy::selectedId).AddComponent<PointLight>();
		}
		if (ImGui::Selectable("Spotlight"))
		{
			if (!Entity(Hierarchy::selectedId).HasComponent<Spotlight>())
				Entity(Hierarchy::selectedId).AddComponent<Spotlight>();
		}
		if (ImGui::Selectable("Directional Light"))
		{
			if (!Entity(Hierarchy::selectedId).HasComponent<DirectionalLight>())
				Entity(Hierarchy::selectedId).AddComponent<DirectionalLight>();
		}

		if (ImGui::Selectable("Particle Emitter"))
		{
			if (!Entity(Hierarchy::selectedId).HasComponent<ParticleEmitter>())
				Entity(Hierarchy::selectedId).AddComponent<ParticleEmitter>();
		}

		if (ImGui::Selectable("Audio")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<Audio>())
				Entity(Hierarchy::selectedId).AddComponent<Audio>();
		}	

		if (ImGui::Selectable("AudioListener")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<AudioListener>())
				Entity(Hierarchy::selectedId).AddComponent<AudioListener>();				
		}

		if (ImGui::Selectable("VFX")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<VFX>())
				Entity(Hierarchy::selectedId).AddComponent<VFX>();
		}
		if (ImGui::Selectable("MeshRenderer")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<MeshRenderer>()) {
				Entity(Hierarchy::selectedId).AddComponent<MeshRenderer>();

				Entity meshobj(Hierarchy::selectedId);

			}
		}
		if (ImGui::Selectable("UIrenderer")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<UIrenderer>())
				Entity(Hierarchy::selectedId).AddComponent<UIrenderer>();
		}
		if (ImGui::Selectable("AISetting")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<AISetting>())
				Entity(Hierarchy::selectedId).AddComponent<AISetting>();
		}

		if (ImGui::Selectable("Camera")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<Camera>())
				Entity(Hierarchy::selectedId).AddComponent<Camera>();
		}

		if (ImGui::Selectable("Crosshair")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<Crosshair>())
				Entity(Hierarchy::selectedId).AddComponent<Crosshair>();
		}

		if (ImGui::Selectable("Healthbar")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<Healthbar>())
				Entity(Hierarchy::selectedId).AddComponent<Healthbar>();
		}

		if (ImGui::Selectable("Button")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<Button>())
				Entity(Hierarchy::selectedId).AddComponent<Button>();
		}

		if (ImGui::Selectable("Portal")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<Portal>())
				Entity(Hierarchy::selectedId).AddComponent<Portal>();
		}

		if (ImGui::Selectable("Gun")) {
			if (!Entity(Hierarchy::selectedId).HasComponent<Gun>())
				Entity(Hierarchy::selectedId).AddComponent<Gun>();
		}


		ImGui::EndCombo();


	}

}
/***************************************************************************/
/*!
\brief
	Inspector functionality for General
*/
/**************************************************************************/
void General::Inspect() {


	ImGui::Text("Name");
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
		- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
	ImGui::InputText("##naming",&name);


	ImGui::Text("Tag");

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
		- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

	if (ImGui::BeginCombo("##Tag", ECS::mEntityTags[tagid].c_str())) {

		for (size_t i = 0; i < ECS::mEntityTags.size(); i++) {

			if (tagid == static_cast<unsigned char>(i))
				continue;

			if (ImGui::Selectable(ECS::mEntityTags[i].c_str())) {
				tagid = static_cast<unsigned char>(i);
			}
		}
		ImGui::EndCombo();
	}

	 
	//if (onselect)
	//	e.general.settag(onselect.string)

	//if (addnewtag)
	//	ecs::addtag(addnewtag.string)

	/*ImGui::Text("Tag");

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
		- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

	if (ImGui::BeginCombo("##Tag", enum_tag::ToString(tagid))) {

		for (int i = 0; i < enum_tag::COUNT; i++) {
			enum_tag::enum_tag temp = static_cast<enum_tag::enum_tag>(i);
			if (ImGui::Selectable(enum_tag::ToString(temp))) {
				tagid = static_cast<enum_tag::enum_tag>(i);
			}
		}
		ImGui::EndCombo();
	}*/
}

/***************************************************************************/
/*!
\brief
	Inspector functionality for Transform
*/
/**************************************************************************/
void Transform::Inspect() {
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		ReflectProperties(*this);
	}

}


void Gun::Inspect() 
{
	const char* axis[] = { "X" , "Y" , "Z"};

	if (ImGui::CollapsingHeader("Gun", ImGuiTreeNodeFlags_DefaultOpen)) 
	{
		//ImGui::Text("RecoilAngle"/*)*/;
		ImGui::DragFloat("Recoil Angle", (float*)&recoilAngle, 0.05f, 0.0f, 360.0f);
		//ImGui::DragFloat("##Recoil Angle", &recoilAngle, 0.2f, mMinRecoilAngle, mMaxRecoilAngle);
	}

}
/***************************************************************************/
/*!
\brief
	Inspector functionality for Camera
*/
/**************************************************************************/
void Camera::Inspect()
{

	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

		ImGui::Separator();

		vec3 camdir = mCamera.direction();
		ImGui::Text("Camera Direction");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::Text("%0.2f, %0.2f, %0.2f", camdir.x, camdir.y, camdir.z);

		ImGui::Text("Aspect Ratio");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::Text("%0.2f", &mCamera.mAspectRatio);

		ImGui::Text("FOV");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##FOV", &mCamera.mFovDegree, 0.2f, GFX::CameraConstants::minFOV, GFX::CameraConstants::maxFOV);

		if (ImGui::Button("Reset FOV"))
		{
			mCamera.mFovDegree = GFX::CameraConstants::defaultFOV;
		}

		ImGui::Text("Far Plane");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##FP", &mCamera.mFar);

		ImGui::Text("Near plane");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##NP", &mCamera.mNear);

		ImGui::Text("Camera Speed");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##CSG", &mCamera.mCameraSpeed);


		//ImGui::Text("Orthographic");
		//ImGui::SameLine();
		//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
		//	- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		//ImGui::Checkbox("##Orth", &Orthographic);
	}

}

void PointLight::Inspect()
{
	bool delete_component = true;

	if (ImGui::CollapsingHeader("PointLight", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Color");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Light Color", (float*)&mLightColor, 0.01f);

		ImGui::Text("Linear Falloff");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Linear Falloff", (float*)&mLinearFalloff, 0.1f);

		ImGui::Text("Quadratic Falloff");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Quadratic Falloff", (float*)&mQuadraticFalloff, 0.1f);

		ImGui::Text("Intensity");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Intensity", (float*)&mIntensity, 0.1f);
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<PointLight>();
}

void Spotlight::Inspect()
{
	bool delete_component = true;

	if (ImGui::CollapsingHeader("Spotlight", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Color");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Light Color", (float*)&mColor, 0.01f);

		ImGui::Text("Spotlight Direction");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Direction ", (float*)&mDirection, 0.01f);

		ImGui::Text("Cutoff");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Linear Falloff", (float*)&mCutoff, 0.1f);

		ImGui::Text("Outer Cutoff");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Outer Cutoff", (float*)&mOuterCutoff, 0.1f);

		ImGui::Text("Intensity");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Intensity", (float*)&mIntensity, 0.1f);
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<Spotlight>();
}

void DirectionalLight::Inspect()
{
	bool delete_component = true;

	if (ImGui::CollapsingHeader("DirectionalLight", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Direction");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Light Direction ", (float*)&mDirection, 0.01f, -1.f, 1.f);

		ImGui::Text("Width Height");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat2("##Light Size ", (float*)&mSize, 1.f, 0.f);

		ImGui::Text("Near Far");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat2("##Light Near Far ", (float*)&mNearFar, 0.1f, 0.1f);
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<DirectionalLight>();
}

void ParticleEmitter::Inspect()
{
	//!< Helper
	auto getFilename = [](std::string filepath) -> std::string
		{
			// returns AT-AT
			std::string ret_str = filepath.substr(filepath.find_last_of("/") + 1);
			ret_str = ret_str.substr(0, ret_str.find_first_of("."));
			return ret_str;
		};
	bool delete_component = true;

	if (ImGui::CollapsingHeader("Particle Emitter", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorPicker4("Start Color", (float*)&mStartColor);
		ImGui::ColorPicker4("End Color", (float*)&mEndColor);

		ImGui::Text("Position");
		ImGui::SameLine();
		ImGui::DragFloat3("##Position ", &mPosition[0], 0.1f);

		ImGui::Text("Offset");
		ImGui::SameLine();
		ImGui::DragFloat3("##Offset ", &mOffset[0], 0.1f);

		ImGui::Text("Start Size");
		ImGui::SameLine();
		ImGui::DragFloat("##Start Size ", &mStartSize, 0.1f, 0.f);

		ImGui::Text("End Size");
		ImGui::SameLine();
		ImGui::DragFloat("##End Size ", &mEndSize, 0.1f, 0.f);

		ImGui::Text("Gravity");
		ImGui::SameLine();
		ImGui::DragFloat("##Gravity ", &mGravity, 0.1f);

		ImGui::Text("Rotation");
		ImGui::SameLine();
		ImGui::DragFloat("##Rotation ", &mRotation, 1.f);

		ImGui::Text("Lifetime");
		ImGui::SameLine();
		ImGui::DragFloat("##Lifetime ", &mLifetime, 0.1f, 0.f);

		ImGui::Text("Speed");
		ImGui::SameLine();
		ImGui::DragFloat("##Speed ", &mSpeed, 0.1f, 0.f);

		ImGui::Text("Count");
		ImGui::SameLine();
		ImGui::DragInt("##Count ", &mCount, 1, 0);

		if (ImGui::Button("Emit Particles"))
		{
			mEmit = true;	// Set to true to emit
		}
		ImGui::SameLine();
		ImGui::Checkbox("Loop: ", &mLoop);
		if (mLoop)
		{
			ImGui::Text("Loop Interval");
			ImGui::SameLine();
			ImGui::DragFloat("##Loop Interval ", &mLoopInterval, 0.1f, 0.f);
		}

		ImGui::NewLine();

		ImGui::Selectable("Particles   ");
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT")) {

				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);
				mTexPath = data_str;


				std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
				mTexPath = texturestr;
				std::string TEXTURE_Descriptor_Filepath;
				unsigned guid;
				// check and ensures that the descriptor file for the materials are created
				//bool descFilePresent = _GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
				std::string descfilepath = data_str + ".desc";
				guid = _GEOM::GetGUID(descfilepath);
				mTexture.data_uid = guid;
				mTexture.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
			}


			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT_UNCOMPRESS"))
			{
				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);

				std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
				//mMaterialInstancePath[i] = texturestr;

				std::string TEXTURE_Descriptor_Filepath;
				unsigned guid;

				// check and ensures that the descriptor file for the materials are created
				_GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
				std::string descfilepath = data_str + ".desc";
				guid = _GEOM::GetGUID(descfilepath);	// gets the guid from the png desc

				_GEOM::Texture_DescriptorData::DeserializeTEXTURE_DescriptorDataFromFile(mTextureDescriptorData, TEXTURE_Descriptor_Filepath);

				{
					std::cout << "\033[33mNOTE: >> Compressing texture: " << texturestr << "\033[0m" << std::endl;
					CompressImageFile(data_str.c_str(), systemManager->mResourceTySystem->compressed_texture_path.c_str(), _GEOM::Texture_DescriptorData::isGammaSpace(TEXTURE_Descriptor_Filepath));

					// Load the textures into the list of usable textures within the engine, if it doesnt already exist
					if (systemManager->mResourceTySystem->getMaterialInstance(guid) == nullptr) {
						systemManager->mResourceTySystem->texture_Load(getFilename(data_str), guid);
					}

					mTexture.data_uid = guid;
					mTexture.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::Text(mTexPath.c_str());

	}



	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<ParticleEmitter>();
}

/***************************************************************************/
/*!
\brief
	Inspector functionality for Script
*/
/**************************************************************************/
void Scripts::Inspect(entt::entity entityID) {
	bool delete_component = true;
	const char* data_script{};
	static std::string newScript;
	static bool open_popup{ false };
	static std::string deleteScript;
	bool emptyFile = false;

	//auto scriptEntities = systemManager->ecs->GetEntitiesWith<Scripts>();
	////Scripts& scripts = scriptEntities.get<Scripts>(Hierarchy::selectedId);

	if (ImGui::CollapsingHeader("Scripts", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_LUA"))
			{
				data_script = (const char*)payload->Data;
				//scripts.mScriptFile = std::string(data_script);
				std::string dataScript = std::string(data_script);

				// if entity does not contain any script, just add 
				if (scriptsContainer.size() == 0)
				{
					Script* script = AddScript(dataScript);
					script->Load(entityID);
					//std::cout << "Script " << script.scriptFile << " added to entity " << std::to_string((int)Hierarchy::selectedId) << std::endl;
				}
				// if entity already has scripts attached, check if duplicate 
				else
				{
					bool hasScript{ };

					for (auto& elem : scriptsContainer)
					{
						if (elem->scriptFile == dataScript)
						{
							hasScript = true;
							//std::cout << "Script is already attached! " << std::endl;
							PWARNING("Script is already attached! ");
							break;
						}
					}

					if (!hasScript)
					{
						Script* script = AddScript(dataScript);
						script->Load(entityID);
						//std::cout << "Script " << script.scriptFile << ".lua added to entity " << std::to_string((int)Hierarchy::selectedId) << std::endl;
						PINFO("Script %s added to entity %s", script->scriptFile.c_str(), std::to_string((int)Hierarchy::selectedId).c_str());
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::Text("Drag drop scripts to header above 'Scripts'");
		ImGui::Text("Entity contains scripts: ");
		for (auto& elem : scriptsContainer)
		{
			ImGui::SetNextItemOpen(true);
			if (ImGui::TreeNode(elem->scriptFile.c_str())) {
				if (ImGui::IsItemClicked(1)) {
					open_popup = true;
					deleteScript = elem->scriptFile;
				}
				InspectScript(elem);
				ImGui::TreePop();
			}

		}

		if (open_popup) {
			ImGui::OpenPopup("Delete_Script", ImGuiPopupFlags_MouseButtonRight);
		}
		if (ImGui::BeginPopup("Delete_Script"))
		{
			if (ImGui::Selectable("Delete"))
			{
				for (auto i = 0; i < scriptsContainer.size(); i++)
				{
					if (scriptsContainer[i]->scriptFile == deleteScript)
					{
						delete scriptsContainer[i];
						scriptsContainer.erase(scriptsContainer.begin() + i);
					}
				}
				open_popup = false;
			}
			ImGui::EndPopup();
		}
		open_popup = false;

		ImGui::Text("Create new script: ");
		ImGui::InputText(".lua", &newScript);
		if (ImGui::Button("Add Lua Script"))
		{
			// check if script name was empty
			if (newScript == "" || newScript == " ")
			{
				ImGui::OpenPopup("EmptyScript");
				emptyFile = true;
			}
			if (!emptyFile)
			{
				std::ofstream output;
				std::ifstream input{ "../assets/Scripts/DefaultTemplate.lua" };
				std::stringstream ss;
				std::string lua = ".lua";
				std::string line;
				ss << "../assets/Scripts/" << newScript << ".lua";
				std::cout << ss.str() << std::endl;
				std::string pathInString = ss.str();
				const char* path = pathInString.c_str();
				output.open(path, std::ios_base::out);
				while (getline(input, line))
				{
					output << line << std::endl;
				}
				input.close();
				Script* script = AddScript(ss.str());
				script->Load(Hierarchy::selectedId);
				newScript = " ";
			}
			ImGui::InputText(".lua", &newScript);
		}
	}
	if (ImGui::BeginPopupModal("EmptyScript", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Script file name is EMPTY !");
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<Scripts>();
}
/***************************************************************************/
/*!
\brief
	Inspector functionality for Animator
*/
/**************************************************************************/
void Animator::Inspect()
{
	bool delete_component{ true };
	if (ImGui::CollapsingHeader("Animator", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (mAnimator.m_CurrentAnimation == nullptr)
		{
			ImGui::Text("Mesh is not capable of animation");
		}

		else
		{
			ImGui::Text(std::to_string(mAnimator.m_CurrentTime).c_str());	ImGui::SameLine();
			ImGui::Text(" / ");												ImGui::SameLine();
			ImGui::Text(std::to_string(mAnimator.m_CurrentAnimation->m_Duration).c_str());
		
			ImGui::Checkbox("Pause Animaton", &mAnimator.mIsPaused);
		}
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<Animator	>();
}


/***************************************************************************/
/*!
\brief
	Inspector functionality for MeshRenderer
*/
/***************************************************************************/
void MeshRenderer::Inspect() 
{
	static bool meshbool {false}; // for deleting mesh
	static bool textbool{ false }; // for deleting texture (material)
	static int texIndex{ 0 }; // for deleting texture

	//!< Shader Helper 
	auto getShaderName = [](std::string shaderpath) -> std::string
	{
		// returns pointLight_vert.glsl
		return shaderpath.substr(shaderpath.find_last_of("/") + 1);
	};

	//!< Shader Helper 
	auto getShaderExtension = [](std::string shaderpath) -> std::string
	{
		// returns vert.glsl or frag.glsl
		return shaderpath.substr(shaderpath.find_last_of("_"));
	};

	//!< Helper
	auto getFilename = [](std::string filepath) -> std::string
	{
		// returns AT-AT
		std::string ret_str = filepath.substr(filepath.find_last_of("/") + 1);
		ret_str = ret_str.substr(0, ret_str.find_first_of("."));
		return ret_str;
	};


	bool delete_component{ true };
	if (ImGui::CollapsingHeader("MeshRenderer", &delete_component,ImGuiTreeNodeFlags_DefaultOpen)) 
	{
		// Bloom threshold values
		ImGui::DragFloat4("Bloom Threshold", (float*)&mBloomThreshold, 0.01f, 0.0f, 1.f, "%0.2f");


		int st = static_cast<int>(mMeshPath.find_last_of("/"));
		int ed = static_cast<int>(mMeshPath.find_last_of("."));
		std::string tempPath = mMeshPath.substr(st + 1, ed - (st + 1));

		Entity entins(Hierarchy::selectedId);

		// == >> Shaders << == //
		std::string shaderstr{" "};

		if (systemManager->mResourceTySystem->m_Shaders.find(mShaderRef.data_uid) != systemManager->mResourceTySystem->m_Shaders.end())
			shaderstr = systemManager->mResourceTySystem->m_Shaders[mShaderRef.data_uid].first;
		
		if (ImGui::BeginCombo("##Shaders", shaderstr.c_str())) {

			for (auto& data : systemManager->mResourceTySystem->m_Shaders) {

				if (ImGui::Selectable(data.second.first.c_str())) {
					mShaderRef.data_uid = static_cast<unsigned int>(data.first);
				}
			}

			ImGui::EndCombo();

		}
		ImGui::Separator();
		ImGui::Checkbox("Cast Shadow", &mCastShadow);

		ImGui::Separator();

		// == >> Mesh << == //
		ImGui::Text("MESH");
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_GEOM"))
			{

				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);
				mMeshPath = data_str;

				std::string descfilepath = data_str + ".desc";
				unsigned guid = _GEOM::GetGUID(descfilepath);
				mMeshRef.data_uid = guid;
				mMeshRef.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->get_mesh(guid));
				GFX::Mesh* meshinst = reinterpret_cast<GFX::Mesh*>(mMeshRef.data);

				if (entins.HasComponent<Animator>() && meshinst->mHasAnimation)
				{
					// if the new entity has both animations and the animator component, update the animator to use the new animation
					entins.GetComponent<Animator>().mAnimator.SetAnimation(&meshinst->mAnimation[0]);
				}
				else if (entins.HasComponent<Animator>() && !meshinst->mHasAnimation)
				{
					// if the new entity's mesh has no animation, but the entity still has the animator component
					entins.GetComponent<Animator>().mAnimator.m_CurrentAnimation = nullptr;
				}
			}


			ImGui::EndDragDropTarget();
		}


		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_FBX"))
			{
				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);
				mMeshPath = systemManager->mResourceTySystem->compiled_geom_path + getFilename(data_str) + ".geom";
				std::string GEOM_Descriptor_Filepath;
				unsigned guid;

				_GEOM::CheckAndCreateDescriptorFileMESH(data_str, GEOM_Descriptor_Filepath);
				std::string descfilepath = data_str + ".desc";
				guid = _GEOM::GetGUID(descfilepath);

				//if (!descFilePresent)
				// recompiles everything as long as its fbx
				{
					//!>> Calling the GEOM Compiler to load 
					std::cout << "\033[33mNOTE: >> Compiling mesh: " << data_str << "\033[0m" << std::endl;

					std::string command = "..\\_GEOM_COMPILER\\_GEOM_COMPILER.exe ";
					command += GEOM_Descriptor_Filepath;
					int result = system(command.c_str());

					if (result) {
						std::cout << "GEOM Compiler: " << GEOM_Descriptor_Filepath << " failed to compile!" << std::endl;
						assert(0);
					}

					if (systemManager->mResourceTySystem->get_mesh(guid) == nullptr)
					{
						// only load the mesh if its not yet been seen in the system
						std::string geompath = GEOM_Descriptor_Filepath.substr(0, GEOM_Descriptor_Filepath.find_last_of("."));
						systemManager->mResourceTySystem->mesh_Load(geompath, guid);
					}
				}

				mMeshRef.data_uid = guid;
				mMeshRef.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->get_mesh(guid));
				GFX::Mesh* meshinst = reinterpret_cast<GFX::Mesh*>(mMeshRef.data);
				
				// load the descriptor data into the GFX::mesh instance
				_GEOM::DescriptorData::DeserializeGEOM_DescriptorDataFromFile(meshinst->mMeshDescriptorData, GEOM_Descriptor_Filepath);

				if (entins.HasComponent<Animator>() && meshinst->mHasAnimation)
				{
					// if the new entity has both animations and the animator component, update the animator to use the new animation
					entins.GetComponent<Animator>().mAnimator.SetAnimation(&meshinst->mAnimation[0]);
				}
				else if (entins.HasComponent<Animator>() && !meshinst->mHasAnimation)
				{
					// if the new entity's mesh has no animation, but the entity still has the animator component
					entins.GetComponent<Animator>().mAnimator.m_CurrentAnimation = nullptr;
				}
			}


			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(tempPath.c_str()).x
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

		if( tempPath.size()>0){
		ImGui::Selectable(tempPath.c_str());

		//--------------------------------------------------------------------------------------------------------------// delete the mesh 
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {

				meshbool = true;
			}

		}
		GFX::Mesh* meshinst = reinterpret_cast<GFX::Mesh*>(mMeshRef.data);

		popup("Delete", meshinst->mMeshDescriptorData,mMeshPath, mMeshRef, meshbool);



		// == >> Textures << == //
		ImGui::Dummy(ImVec2(0.0f, 10.0f));



		std::string textures[6] = { "DIFFUSE","NORMAL", "SPECULAR","SHININESS","EMISSION", "AO"};

		for (int i{ 0 }; i <6; i++) 
		{
			if (mMaterialInstancePath[i] != "") 
			{
				ImGui::Text(textures[i].c_str());
				if (ImGui::BeginDragDropTarget())
				{
					// loading compressed texture
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT")) 
					{
						const char* data = (const char*)payload->Data;
						std::string data_str = std::string(data);

						//mMaterialInstancePath[i] = data_str;

						std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
						mMaterialInstancePath[i] = texturestr;
						std::string TEXTURE_Descriptor_Filepath;
						unsigned guid;

						// check and ensures that the descriptor file for the materials are created
						//bool descFilePresent = _GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
						std::string descfilepath = data_str + ".desc";
						guid = _GEOM::GetGUID(descfilepath);
						mTextureRef[i].data_uid = guid;
						mTextureRef[i].data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));

						_GEOM::Texture_DescriptorData::DeserializeTEXTURE_DescriptorDataFromFile(mTextureDescriptorData[i], TEXTURE_Descriptor_Filepath);
					}

					// file uncompressed texture for objects
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT_UNCOMPRESS")) 
					{
						const char* data = (const char*)payload->Data;
						std::string data_str = std::string(data);

						std::string texturestr =systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
						mMaterialInstancePath[i] = texturestr;

						std::string TEXTURE_Descriptor_Filepath;
						unsigned guid;

						// check and ensures that the descriptor file for the materials are created
						_GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
						std::string descfilepath = data_str + ".desc";
						guid = _GEOM::GetGUID(descfilepath);	// gets the guid from the png desc

						_GEOM::Texture_DescriptorData::DeserializeTEXTURE_DescriptorDataFromFile(mTextureDescriptorData[i], TEXTURE_Descriptor_Filepath);

						{
							std::cout << "\033[33mNOTE: >> Compressing texture: " << texturestr << "\033[0m" << std::endl;
							CompressImageFile(data_str.c_str(), systemManager->mResourceTySystem->compressed_texture_path.c_str(), _GEOM::Texture_DescriptorData::isGammaSpace(TEXTURE_Descriptor_Filepath));

							// Load the textures into the list of usable textures within the engine, if it doesnt already exist
							if (systemManager->mResourceTySystem->getMaterialInstance(guid) == nullptr) {
								systemManager->mResourceTySystem->texture_Load(getFilename(data_str), guid);
							}

							mTextureRef[i].data_uid = guid;
							mTextureRef[i].data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
						}
					}

					ImGui::EndDragDropTarget();
				}

				int posstart = static_cast<int>(mMaterialInstancePath[i].find_last_of("/"));
				int posend = static_cast<int>(mMaterialInstancePath[i].find_last_of("."));

				std::string newpath = mMaterialInstancePath[i].substr(posstart+1, posend-(posstart+1));

				ImGui::SameLine();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(newpath.c_str()).x
					- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

				if (newpath.size() > 0) {
					ImGui::Selectable(newpath.c_str());

					//--------------------------------------------------------------------------------------------------------------// delete the texture 
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
						texIndex = i;
						textbool = true;
					}
				}
				

				ImGui::Dummy(ImVec2(0.0f, 3.0f));
				std::string nodename = getFilename(mMaterialInstancePath[i]) + " DescriptorFile";
				if (ImGui::TreeNode(nodename.c_str()))
				{
					_GEOM::Texture_DescriptorData& texturedesc = mTextureDescriptorData[i];
					if (texturedesc.mGUID)
					{
						ImGui::Text("%d", texturedesc.mGUID);

						bool isSRGB = (texturedesc.mCompressionType == _GEOM::CompressionType::SRGB);
						ImGui::Checkbox("isSRGB", &isSRGB);

						// RGB
						if (isSRGB) {
							texturedesc.mCompressionType = _GEOM::CompressionType::SRGB;
						}
						else {
							texturedesc.mCompressionType = _GEOM::CompressionType::RGB;
						}

						if (texturedesc.mDescFilepath.size() > 0)
							ImGui::Text("%s", texturedesc.mDescFilepath.c_str());

						if (ImGui::Button("Save Descriptor Data"))
						{
							_GEOM::Texture_DescriptorData::SerializeTEXTURE_DescriptorDataToFile(mTextureDescriptorData[i].mDescFilepath, texturedesc);
						}
					}
					else
					{
						ImGui::Text("Empty\n");
					}
					ImGui::TreePop();
				}


				ImGui::Dummy(ImVec2(0.0f, 10.0f));
			}
			else {
				ImGui::Selectable(" ");

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT")) {

						const char* data = (const char*)payload->Data;
						std::string data_str = std::string(data);
						mMaterialInstancePath[i] = data_str;


						std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
						mMaterialInstancePath[i] = texturestr;
						std::string TEXTURE_Descriptor_Filepath;
						unsigned guid;
						// check and ensures that the descriptor file for the materials are created
						//bool descFilePresent = _GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
						std::string descfilepath = data_str + ".desc";
						guid = _GEOM::GetGUID(descfilepath);
						mTextureRef[i].data_uid = guid;
						mTextureRef[i].data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));

						//uid temp(mMaterialInstancePath[i]);
						//mTextureRef[i].data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(temp.id));

					}
					ImGui::EndDragDropTarget();
				}
			}
		}
		popup("DeleteTexture", mTextureDescriptorData[texIndex], mMaterialInstancePath[texIndex], mTextureRef[texIndex], textbool);

		ImGui::ColorPicker4("MeshColor", (float*)&mInstanceColor);
	}

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	// == >> Mesh Renderer GEOM Descriptor File << == //
	//ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("GEOM DescirptorFile"))
	{
		GFX::Mesh* meshinst = reinterpret_cast<GFX::Mesh*>(mMeshRef.data);

		// sanity check
		if (meshinst != nullptr)
		{
			ImGui::InputText("Desc Filepath", const_cast<char*>(meshinst->mMeshDescriptorPath.c_str()), meshinst->mMeshDescriptorPath.length() + 1);
			ImGui::InputInt("GUID", reinterpret_cast<int*>(&meshinst->mMeshDescriptorData.m_GUID));

			// Vert shader selection
			int selectedFBX{};
			_GEOM::DescriptorData& descInst = meshinst->mMeshDescriptorData;

			if (ImGui::BeginCombo("FBX Filepaths", descInst.m_Filepaths[selectedFBX].data(), 0))
			{
				for (int i{}; i < descInst.m_Filepaths.size(); ++i)
				{
					bool isItemSelected = (selectedFBX == i);
					if (ImGui::Selectable(descInst.m_Filepaths[i].data(), isItemSelected))
						selectedFBX = i;

					if (isItemSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::DragFloat3("Pre-Transforms", (float*)&descInst.m_Translate, 0.1f);
			ImGui::DragFloat3("Pre-Scale", (float*)&descInst.m_Scale, 0.1f);
			ImGui::DragFloat3("Pre-Rotate", (float*)&descInst.m_Rotate, 0.1f);

			if (ImGui::Button("Save Descriptor File"))
			{
				std::cout << "Saving Descriptor File to: " << meshinst->mMeshDescriptorPath << std::endl;
				_GEOM::DescriptorData::SerializeGEOM_DescriptorDataToFile(meshinst->mMeshDescriptorPath, descInst);
			}
		}
		ImGui::TreePop();
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<MeshRenderer>();
}

/***************************************************************************/
/*!
\brief
	Inspector functionality for Rigidbody
*/
/***************************************************************************/
void RigidBody::Inspect() {
	bool delete_component{ true };
	static const char* materials[] = { "RUBBER", "WOOD", "METAL", "ICE","CONCRETE","GLASS","UNDEFINED" };
	static const char* motions[] = { "STATIC", "DYNAMIC", "UNDEFINED" };
	if (ImGui::CollapsingHeader("RigidBody", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{


		ImGui::Text("Density");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Density", (float*)&mDensity);

		ImGui::Separator();





		

		ImGui::Text("Material");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

		if (ImGui::BeginCombo("##Material", (materials[static_cast<int>(mMaterial)]))) {

			for (unsigned char i{ 0 }; i < 6; i++) {

				if (i == static_cast<int>(mMaterial))
					continue;

				if (ImGui::Selectable(materials[i])) {
					mMaterial = static_cast<MATERIAL>(i);
					//mMaterial = (MATERIAL)i;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::Text("Motion");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

		if (ImGui::BeginCombo("##Motions", (motions[static_cast<int>(mMotion)]))) {

				for (unsigned char i{ 0 }; i < 2; i++) {
					if (ImGui::Selectable(motions[i])) {

						mMotion = (MOTION)i;
					}
				}

			ImGui::EndCombo();
		}

		ImGui::Text("Velocity");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##velocity", (float*) & mVelocity);



		ImGui::Text("Rotation Constraints");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::Checkbox("##X", &mRotationConstraints[0]);
		ImGui::SameLine();
		ImGui::Text("X");
		ImGui::SameLine();
		ImGui::Checkbox("##Y", &mRotationConstraints[1]);
		ImGui::SameLine();
		ImGui::Text("Y");
		ImGui::SameLine();
		ImGui::Checkbox("##Z", &mRotationConstraints[2]);
		ImGui::SameLine();
		ImGui::Text("Z");

		ImGui::Text("Gravity");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::Checkbox("##Gravity", &mGravity);


	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<RigidBody>();
}
/***************************************************************************/
/*!
\brief
	Inspector functionality for BoxCollider
*/
/***************************************************************************/
void BoxCollider::Inspect() {
	bool delete_component{ true };
	if (ImGui::CollapsingHeader("BoxCollider", &delete_component, ImGuiTreeNodeFlags_DefaultOpen)) {
		ReflectProperties(*this);
	}
	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<BoxCollider>();
}

/***************************************************************************/
/*!
\brief
	Inspector functionality for SphereCollider
*/
/***************************************************************************/
void SphereCollider::Inspect() {
	bool delete_component{ true };
	if (ImGui::CollapsingHeader("SphereCollider", &delete_component, ImGuiTreeNodeFlags_DefaultOpen)) {
		ReflectProperties(*this);
	}
	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<SphereCollider>();
}


/***************************************************************************/
/*!
\brief
	Inspector functionality for SphereCollider
*/
/***************************************************************************/
void CapsuleCollider::Inspect() {
	bool delete_component{ true };
	if (ImGui::CollapsingHeader("CapsuleCollider", &delete_component, ImGuiTreeNodeFlags_DefaultOpen)) {
		ReflectProperties(*this);
	}
	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<CapsuleCollider>();
}

void MeshCollider::Inspect() {
	bool delete_component{ true };
	if (ImGui::CollapsingHeader("MeshCollider", &delete_component, ImGuiTreeNodeFlags_DefaultOpen)) {
		ReflectProperties(*this);
	}
	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<MeshCollider>();
}

/***************************************************************************/
/*!
\brief
	Inspector functionality for Audio
*/
/***************************************************************************/
void Audio::Inspect() {
	bool delete_component = true;
	auto audioEntities = systemManager->ecs->GetEntitiesWith<Audio>();
	std::string full_file_path;  // With the Audio File Name  e.g "../assets\\Audio\\farm_ambience.wav"
	std::string file_path;		 // Only Audio Directory	  e.g "../assets\\Audio"
	std::string audio_name;      // Audio Name only.		  e.g "farm_ambience.wav"

	const char* audio_type[] = { "SFX" , "BGM" };

	Audio& audio_check = Entity(Hierarchy::selectedId).GetComponent<Audio>();

	if (!audio_check.mFileName.empty())
	{
		mIsEmpty = false;
	}

	// Audio Component (Bar)
	if (ImGui::CollapsingHeader("Audio", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_AUDIO"))
			{
				full_file_path = std::string((const char*)payload->Data);

				size_t audio_dir_pos = full_file_path.find_first_of("Audio");

				if (audio_dir_pos != std::string::npos) {
					// Use substr to extract the substring up to the found position
					file_path = full_file_path.substr(0, audio_dir_pos + 5); // Account for "Audio"
					//std::cout << result << std::endl;
				}

				size_t audio_name_start = full_file_path.find_last_of("\\");

				if (audio_name_start != std::string::npos) {
					audio_name = full_file_path.substr(audio_name_start + 1);
				}

				// Must be outside (what if i remove and add an already loaded audio)
				mFilePath = file_path;
				mFileName = audio_name;
				mFullPath = file_path + "/" + audio_name;

				// [3/13] - Start of the inclusion of <Multi-Audio> support
		/*		mFilePaths.push_back(file_path);
				mFileNames.push_back(audio_name);
				mAudioActiveStates.push_back(true);*/
			}

			ImGui::EndDragDropTarget();
		}
	}

	// <Audio> Component - assign m3DAudio flag (has been loaded as a "FMOD_3D" in "LoadFromDirectory()"
	if (mFileName.find("3D") != std::string::npos)
	{
		m3DAudio = true;
	}

	else
	{
		m3DAudio = false;
	}

	ImGui::Text("Drag drop 'Audio' files to header above 'Audio'");
	ImGui::Text("Audio File Selected: ");
	ImGui::Text(Entity(Hierarchy::selectedId).GetComponent<Audio>().mFullPath.c_str());

	// Display each loaded audio file and its full path
	//for (size_t i = 0; i < mFilePaths.size(); ++i) {
	//	if (i < mFileNames.size() && i < mFilePaths.size() && i < mAudioActiveStates.size())  // At any time ideally they'll always have the same size.
	//	{
	//		ImGui::Text("Audio %d:", i + 1);
	//		ImGui::Text("Name: %s", mFileNames[i].c_str());
	//		ImGui::Text("Path: %s", mFilePaths[i].c_str());

	//		if (i < mAudioActiveStates.size()) {
	//			// Checkbox to toggle active state
	//			bool activeState = static_cast<bool>(mAudioActiveStates[i]);
	//			ImGui::Checkbox(("Active##" + std::to_string(i)).c_str(), &activeState);
	//			mAudioActiveStates[i] = activeState;
	//		}

	//		if (!mIsEmpty && m3DAudio)
	//		{
	//			ImGui::Text("This is a 3D Audio");
	//		}
	//	}
	//}

	//ImGui::Text(Entity(Hierarchy::selectedId).GetComponent<Audio>().mFullPath.c_str());

	// Debugging (to show which audio are playing) - on editor
	if (mState == Audio::PLAYING)
	{
		//ImGui::Text("Audio: %s is currently playing on (ID = %u)", mCurrentlyPlaying.c_str(), mChannelID);
		std::string audio_type;
		switch (mAudioType)
		{
			case AUDIO_SFX:
				audio_type = "SFX";
				break;
			case AUDIO_BGM:
				audio_type = "BGM";
				break;
		}
		//ImGui::Text("On the %s group", audio_type.c_str());
	}
		
	if (mState == Audio::PAUSED)
		ImGui::Text("Audio Paused :o");


	static bool remove_audio_bool = false;
	if (!mIsEmpty)
	{
		ImGui::Checkbox("Remove Audio File.", &remove_audio_bool);

	}

	if (!mIsEmpty && remove_audio_bool) // if not empty
	{
		Entity(Hierarchy::selectedId).GetComponent<Audio>().ClearAudioComponent();
		remove_audio_bool = false;
		ClearAudioComponent();
		PINFO("Successfully Removed Audio.");
	}

	/*
	*	Once an audio file is tagged to this <Audio> component.
	*   - More interface will appear for customization.
	*	[a] Play On Awake
	*   [b] Is Looping 
	*   [c] Volume Control
	*   [d] Fade Speed Control
	*	================================================================
	*	(3D Parameters)
	*   [a] Min Distance
	*   [b] Max Distance
	*/
	if (!mIsEmpty)
	{	
		ImGui::Checkbox("Play on Awake", &mPlayonAwake);
		ImGui::Checkbox("Is Looping", &mIsLooping);
		ImGui::Checkbox("Persist upon game state switch", &mGameStateRetain);
		ImGui::Checkbox("Pan Audio", &mPanAudio);
		//ImGui::SliderFloat("Volume", &mVolume, 0.0f, 1.0f, "volume = %.3f");
		ImGui::DragFloat("Volume", (float*)&mVolume, 0.05f, 0.0f, 1.0f);
		ImGui::DragFloat("Fade Speed", (float*)&mFadeSpeedModifier, 0.05f, 0.0f);
		
		// Added [3/13] - Pan Audio
		if (mPanAudio)
		{	
			ImGui::Text("Audio Panning Attributes: ");
			ImGui::DragFloat("Starting Panning Balance", (float*)&mPanBalance, 0.05f, -1.0f, 1.0f); // Set Initial Balance (0.0 for center, -1.0 for left , 1.0 for right)
			ImGui::DragFloat("Panning Speed", (float*)&mPanSpeed, 0.01f, 0.0f, 1.0f);	// [3/13] irrelevant for now
			ImGui::Text("Panning Guide : [-1.0] for left, [0.0] for center, [1.0] for right)");
		}
	
	
		if (m3DAudio)
		{
			ImGui::DragFloat("Min Distance", (float*)&mMinDistance);
			ImGui::DragFloat("Max Distance", (float*)&mMaxDistance);
			/*ImGui::SliderFloat("Min Distance", &mMinDistance, 0.0f, 3000.0f, "%.3f");
			ImGui::SliderFloat("Max Distance", &mMaxDistance, 0.0f, 3000.0f, "%.3f");*/

			//systemManager->mAudioSystem.get()->Update3DChannelSettings(Entity(Hierarchy::selectedId)); 
		}

	}

	// AudioType (Initial) - based on component
	
	switch (mAudioType) // depending on what is the audio type
	{
	case AUDIO_SFX:
		mAudio = 0;
		//mTypeChanged = true;
		//systemManager->mAudioSystem.get()->UpdateChannelReference(Entity(Hierarchy::selectedId));
		break;
	case AUDIO_BGM:
		mAudio = 1;
		//mTypeChanged = true;
		//systemManager->mAudioSystem.get()->UpdateChannelReference(Entity(Hierarchy::selectedId));
		break;
	}
	

	//mAudio = mAudioType;
	if (ImGui::BeginCombo("Audio Type", audio_type[mAudio]))
	{
		for (unsigned char i{ 0 }; i < 2; i++) {
			if (ImGui::Selectable(audio_type[i])) {
				mAudio = i;
				switch (mAudio) // depending on what is the audio type
				{
				case 0:
					mAudioType = AUDIO_SFX;
					//mTypeChanged = true;
					//systemManager->mAudioSystem.get()->UpdateChannelReference(Entity(Hierarchy::selectedId));
					break;
				case 1:
					mAudioType = AUDIO_BGM;
					//mTypeChanged = true;
					//systemManager->mAudioSystem.get()->UpdateChannelReference(Entity(Hierarchy::selectedId));
					break;
				}
			}
		}

		ImGui::EndCombo();
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<Audio>();
}


/***************************************************************************/
/*!
\brief
	Inspector functionality for AudioListener
*/
/***************************************************************************/
void AudioListener::Inspect() {
	bool delete_component = true;

	// Audio Component (Bar)
	if (ImGui::CollapsingHeader("AudioListener", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("HELLO LISTENER");
	}

	
	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<AudioListener>();
}

void UIrenderer::Inspect() {

	auto getFilename = [](std::string filepath) -> std::string
	{
		// returns AT-AT
		std::string ret_str = filepath.substr(filepath.find_last_of("/") + 1);
		ret_str = ret_str.substr(0, ret_str.find_first_of("."));
		return ret_str;
	};

	bool delete_component = true;

	if (ImGui::CollapsingHeader("UIrenderer", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Selectable("UI   ");

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT")) {

				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);
				mTexPath = data_str;


				std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
				mTexPath = texturestr;
				std::string TEXTURE_Descriptor_Filepath;
				unsigned guid;
				// check and ensures that the descriptor file for the materials are created
				//bool descFilePresent = _GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
				std::string descfilepath = data_str + ".desc";
				guid = _GEOM::GetGUID(descfilepath);
				mTextureRef.data_uid = guid;
				mTextureRef.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
			}


			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT_UNCOMPRESS"))
			{
				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);

				std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
				//mMaterialInstancePath[i] = texturestr;

				std::string TEXTURE_Descriptor_Filepath;
				unsigned guid;

				// check and ensures that the descriptor file for the materials are created
				_GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
				std::string descfilepath = data_str + ".desc";
				guid = _GEOM::GetGUID(descfilepath);	// gets the guid from the png desc

				_GEOM::Texture_DescriptorData::DeserializeTEXTURE_DescriptorDataFromFile(mTextureDescriptorData, TEXTURE_Descriptor_Filepath);

				{
					std::cout << "\033[33mNOTE: >> Compressing texture: " << texturestr << "\033[0m" << std::endl;
					CompressImageFile(data_str.c_str(), systemManager->mResourceTySystem->compressed_texture_path.c_str(), _GEOM::Texture_DescriptorData::isGammaSpace(TEXTURE_Descriptor_Filepath));

					// Load the textures into the list of usable textures within the engine, if it doesnt already exist
					if (systemManager->mResourceTySystem->getMaterialInstance(guid) == nullptr) {
						systemManager->mResourceTySystem->texture_Load(getFilename(data_str), guid);
					}

					mTextureRef.data_uid = guid;
					mTextureRef.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
				}
			}

			ImGui::EndDragDropTarget();
		}
		
		if (mTexPath.size() > 0) {
			int st = static_cast<int>(mTexPath.find_last_of("/"));
			int ed = static_cast<int>(mTexPath.find_last_of("."));
			std::string tempPath = mTexPath.substr(st + 1, ed - (st + 1));

			ImGui::SameLine();

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(tempPath.c_str()).x
				- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);

			ImGui::Text(tempPath.c_str());
		}
		else {
			ImGui::Text(" ");
		}


		ImGui::Text("Degree");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Degree", (float*)&mDegree);
		ImGui::Separator();

		ImGui::Text("Slider");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Slider", (float*)&mSlider, 0.01 , 0, 1, "%.2f");
		ImGui::Separator();

		ImGui::Text("Layer");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragInt("##Layer", (int*)&mLayer);
		ImGui::Separator();

		ImGui::Checkbox("World Transform", &mWorldTransform);

		ImGui::ColorPicker4("Color", (float*)&mColor);
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<UIrenderer>();
}

void VFX::Inspect() {
	bool delete_component = true;
	if (ImGui::CollapsingHeader("VFX", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ReflectProperties(*this);
	}
	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<VFX>();
}

void AISetting::Inspect() {
	bool delete_component = true;
	if (ImGui::CollapsingHeader("AI Setting", &delete_component, ImGuiTreeNodeFlags_DefaultOpen)) {
		
		// E_MOVEMENT_TYPE mMovementType;	// AI's movement type

		auto const& movementTypeArr = systemManager->mAISystem->GetMovementTypeArray();
		
		if (ImGui::BeginCombo("Movement Type", movementTypeArr[static_cast<int>(mMovementType)].c_str())) {
			for (unsigned char i{ static_cast<int>(E_MOVEMENT_TYPE::BEGIN) + 1}; i < movementTypeArr.size(); i++) {
				if (ImGui::Selectable(movementTypeArr[i].c_str())) {
					mMovementType = static_cast<E_MOVEMENT_TYPE>(i);
				}
			}
			ImGui::EndCombo();
		}

		// float mStayAway;								// Distance to stay away from player
		if (mMovementType == E_MOVEMENT_TYPE::AIR_HOVER) {
			ImGui::Text("Horizontal Distance From Target");
			ImGui::DragFloat("##Distance From Target", &mStayAway);

			ImGui::Text("Vertical Elevation From Target");
			ImGui::DragFloat("##Vertical Elevation From Target", &mElevation);
		}
		ImGui::Separator();

		// float mSpreadOut;								// Degree of spreading out from another entity
		ImGui::Separator();
		ImGui::Text("Degree of Spreading Out");
		ImGui::DragFloat("##Degree of Spreading Out", &mSpreadOut);

		// Entity mTarget;								// AI's target
		ImGui::InputText("Target Name", &mTargetName);


		if (ImGui::Button("Update Target"))
			mTarget = systemManager->mGameStateSystem->GetEntity(mTargetName);

		if (ImGui::Button("Make Target The Global Player"))
			systemManager->GetAIManager()->SetPlayer(mTarget);

		if (mTargetName.size())
			ImGui::InputText("Pathfinder Name", &mGraphDataName);
		else
			ImGui::Text("Target name is required for pathfinder");
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<AISetting>();
	
	//if (ImGui::CollapsingHeader("UIrenderer", &delete_component, ImGuiTreeNodeFlags_DefaultOpen)) {
	//	ImGui::TextColored({ 0.f,1.f, 1.f, 1.f }, "Bloom Variables");
	//	ImGui::DragFloat3("Global Bloom Threshold", (float*)&mBloomThreshold, 0.01f, 0.f, 1.f);
	//}
}

void Crosshair::Inspect()
{
	bool delete_component = true;

	if (ImGui::CollapsingHeader("Crosshair", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Selectable("Crosshair   ");

		ImGui::Text("Thickness");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Thickness", (float*)&mThickness);
		ImGui::Separator();

		ImGui::Text("Inner Offset");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Inner Offset", (float*)&mInner);
		ImGui::Separator();

		ImGui::Text("Outer Limit");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Outer Limit", (float*)&mOuter);
		ImGui::Separator();

		ImGui::ColorPicker4("Crosshair Color", (float*)&mColor);
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<Crosshair>();
}

void popup(std::string name, _GEOM::Texture_DescriptorData& desc, std::string& dataname, ref& data, bool& trigger) {
	std::string hash ="##to"+name;
	if (trigger == true) {
		ImGui::OpenPopup(hash.c_str());
	}
	if (ImGui::BeginPopup(hash.c_str())) {
		
		if (ImGui::Selectable("Delete")) {
			data.data_uid = 0;
			data.data = nullptr;
			dataname = " ";
			trigger = false;
			desc.mGUID = 0;
		}

		ImGui::EndPopup();

	}
	trigger = false;
	
}

void popup(std::string name, _GEOM::DescriptorData& desc,  std::string& dataname, ref& data, bool& trigger) {
	std::string hash = "##to" + name;
	if (trigger == true) {
		ImGui::OpenPopup(hash.c_str());
	}
	if (ImGui::BeginPopup(hash.c_str())) {

		if (ImGui::Selectable("Delete")) {
			data.data_uid = 0;
			data.data = nullptr;
			dataname = " ";
			trigger = false;
			desc.m_GUID = 0;
		}

		ImGui::EndPopup();

	}
	trigger = false;

}



void Healthbar::Inspect()


{

	auto getFilename = [](std::string filepath) -> std::string
	{
		// returns AT-AT
		std::string ret_str = filepath.substr(filepath.find_last_of("/") + 1);
		ret_str = ret_str.substr(0, ret_str.find_first_of("."));
		return ret_str;
	};

	bool delete_component = true;

	if (ImGui::CollapsingHeader("Healthbar", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{




		ImGui::Selectable("Frame Texture ");

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT")) {

				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);
				//mTexPath = data_str;


				std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
				//mTexPath = texturestr;
				std::string TEXTURE_Descriptor_Filepath;
				unsigned guid;
				// check and ensures that the descriptor file for the materials are created
				//bool descFilePresent = _GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
				std::string descfilepath = data_str + ".desc";
				guid = _GEOM::GetGUID(descfilepath);
				mFrameTexture.data_uid = guid;
				mFrameTexture.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
			}


			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT_UNCOMPRESS"))
			{
				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);

				std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
				//mMaterialInstancePath[i] = texturestr;

				std::string TEXTURE_Descriptor_Filepath;
				unsigned guid;

				// check and ensures that the descriptor file for the materials are created
				_GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
				std::string descfilepath = data_str + ".desc";
				guid = _GEOM::GetGUID(descfilepath);	// gets the guid from the png desc

				_GEOM::Texture_DescriptorData::DeserializeTEXTURE_DescriptorDataFromFile(mFrameTextureDescriptorData, TEXTURE_Descriptor_Filepath);

				{
					std::cout << "\033[33mNOTE: >> Compressing texture: " << texturestr << "\033[0m" << std::endl;
					CompressImageFile(data_str.c_str(), systemManager->mResourceTySystem->compressed_texture_path.c_str(), _GEOM::Texture_DescriptorData::isGammaSpace(TEXTURE_Descriptor_Filepath));

					// Load the textures into the list of usable textures within the engine, if it doesnt already exist
					if (systemManager->mResourceTySystem->getMaterialInstance(guid) == nullptr) {
						systemManager->mResourceTySystem->texture_Load(getFilename(data_str), guid);
					}

					mFrameTexture.data_uid = guid;
					mFrameTexture.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
				}
			}

			ImGui::EndDragDropTarget();
		}


		ImGui::Selectable("Health Texture ");

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT")) {

				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);
				//mTexPath = data_str;


				std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
				//mTexPath = texturestr;
				std::string TEXTURE_Descriptor_Filepath;
				unsigned guid;
				// check and ensures that the descriptor file for the materials are created
				//bool descFilePresent = _GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
				std::string descfilepath = data_str + ".desc";
				guid = _GEOM::GetGUID(descfilepath);
				mHealthTexture.data_uid = guid;
				mHealthTexture.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
			}


			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_TEXT_UNCOMPRESS"))
			{
				const char* data = (const char*)payload->Data;
				std::string data_str = std::string(data);

				std::string texturestr = systemManager->mResourceTySystem->compressed_texture_path + getFilename(data_str) + ".ctexture";
				//mMaterialInstancePath[i] = texturestr;

				std::string TEXTURE_Descriptor_Filepath;
				unsigned guid;

				// check and ensures that the descriptor file for the materials are created
				_GEOM::CheckAndCreateDescriptorFileTEXTURE(data_str, TEXTURE_Descriptor_Filepath, texturestr);
				std::string descfilepath = data_str + ".desc";
				guid = _GEOM::GetGUID(descfilepath);	// gets the guid from the png desc

				_GEOM::Texture_DescriptorData::DeserializeTEXTURE_DescriptorDataFromFile(mHealthTextureDescriptorData, TEXTURE_Descriptor_Filepath);

				{
					std::cout << "\033[33mNOTE: >> Compressing texture: " << texturestr << "\033[0m" << std::endl;
					CompressImageFile(data_str.c_str(), systemManager->mResourceTySystem->compressed_texture_path.c_str(), _GEOM::Texture_DescriptorData::isGammaSpace(TEXTURE_Descriptor_Filepath));

					// Load the textures into the list of usable textures within the engine, if it doesnt already exist
					if (systemManager->mResourceTySystem->getMaterialInstance(guid) == nullptr) {
						systemManager->mResourceTySystem->texture_Load(getFilename(data_str), guid);
					}

					mHealthTexture.data_uid = guid;
					mHealthTexture.data = reinterpret_cast<void*>(systemManager->mResourceTySystem->getMaterialInstance(guid));
				}
			}
			
			ImGui::EndDragDropTarget();

		}

		
		//ImGui::Selectable("Healthbar   ");

		ImGui::Text("Width");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Width", (float*)&mWidth);
		ImGui::Separator();

		ImGui::Text("Height");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Height", (float*)&mHeight);
		ImGui::Separator();

		ImGui::Text("Max Health");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat("##Max Health", (float*)&mMaxHealth);
		ImGui::Separator();

		ImGui::Text("Offset");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Offset", (float*)&mPosition[0]);
		ImGui::Separator();

		ImGui::ColorPicker4("Health Color", (float*)&mHealthColor);

		ImGui::ColorPicker4("Back Color", (float*)&mBackColor);
	}



	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<Healthbar>();
}

void Button::Inspect()
{
	bool delete_component{ true };
	if (ImGui::CollapsingHeader("Button", &delete_component, ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Interactable", &mInteractable);
		ImGui::Checkbox("Hover", &mIsHover);
		ImGui::Checkbox("Click", &mIsClick);
		ImGui::Checkbox("Activated", &mActivated);
		ImGui::Checkbox("RenderFlag", &mRenderFlag);
	}
	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<Button>();
}

void Portal::Inspect()
{
	bool delete_component = true;

	if (ImGui::CollapsingHeader("Portal", &delete_component, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Selectable("Portal   ");

		ImGui::Text("Position 1");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Position 1", (float*)&mTranslate1[0]);
		ImGui::Separator();

		ImGui::Text("Scale 1");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Scale 1", (float*)&mScale1[0]);
		ImGui::Separator();

		ImGui::Text("Rotate 1");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Rotate 1", (float*)&mRotate1[0]);
		ImGui::Separator();

		ImGui::Text("Position 2");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Position 2", (float*)&mTranslate2[0]);
		ImGui::Separator();

		ImGui::Text("Scale 2");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Scale 2", (float*)&mScale2[0]);
		ImGui::Separator();

		ImGui::Text("Rotate 2");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcItemWidth()
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
		ImGui::DragFloat3("##Rotate 2", (float*)&mRotate2[0]);
		ImGui::Separator();
	}

	if (delete_component == false)
		Entity(Hierarchy::selectedId).RemoveComponent<Portal>();
}