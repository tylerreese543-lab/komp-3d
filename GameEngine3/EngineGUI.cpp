#include "EngineGUI.h"
#include "LuaResource.h"
#include "Mesh.h"
#include "Scene.h"
#include "Engine/tooltips.h"
#include "AABB.h"
#include "TextGeometry.h"
#include "SoundInstance.h"
#include "Light.h"
#include "Value.h"
int EngineGUI::axisSelected = -1;
int EngineGUI::selectedInstanceId = -1;
std::string EngineGUI::selectedInstanceHash = "";

float EngineGUI::kScaleHalfExtent = 1.0f;
float EngineGUI::kMinScaleSize = 0.01f;

int EngineGUI::gizmoScaleSign = 1;
float EngineGUI::gizmoStartSize = 1;
float EngineGUI::gizmoStartGrab = 0;
vec3 EngineGUI::gizmoFixedPoint = vec3(0, 0, 0);

const std::string EngineGUI::gizmoTypeNames[] = {
	"Translate",
	"Rotate",
	"Scale"
};


int EngineGUI::eyeDropperId = -1;
int EngineGUI::eyeDropperSpecificType = -1;
int EngineGUI::eyeDropperToUpdateId = -1;
int EngineGUI::eyeDropperReason = -1;
int EngineGUI::instanceAddParentId = -1;
int EngineGUI::currentSamplesAudioPlayback = 0;
int EngineGUI::maxSamplesAudioPlayback = 0;
int EngineGUI::currentSelectionDropdown = 0;
int EngineGUI::currentSelectionDropdown2 = 0;

vec3 EngineGUI::mouseRayDirection;
vec3 EngineGUI::originalObjectTransform;
vec3 EngineGUI::originalObjectAxis;
vec3 EngineGUI::newTransform;
float EngineGUI::gizmoAxisOffset;
float EngineGUI::gizmoSnapPhysical = 1;
float EngineGUI::gizmoSnapOrientation = 45;
pugi::xml_document EngineGUI::tooltipsXML;
bool EngineGUI::wasXMLSuccessful = false;
bool EngineGUI::isGizmoActive = false;
bool EngineGUI::gizmoLocalSpace = false;      // true = rotate around the object's own axes
mat3 EngineGUI::gizmoParentRotation;  // P
mat3 EngineGUI::gizmoStartWorldRot;   // W0

bool EngineGUI::isGizmoRender = false;
int EngineGUI::gizmoType = EngineGUI::GizmoType_Translate;

vec3 EngineGUI::gizmoStartScale;
const int EngineGUI::EDITOR_ADD_INSTANCE[] = {
	Instance::InstanceType_Camera,
	Instance::InstanceType_Mesh,
	Instance::InstanceType_Collision3D,
	Instance::InstanceType_ShaderProgram,
	Instance::InstanceType_ShaderResource,
	Instance::InstanceType_ImageResource,
	Instance::InstanceType_Material,
	Instance::InstanceType_LuaResource,
	Instance::InstanceType_Geometry,
	Instance::InstanceType_TextGeometry,
	Instance::InstanceType_Audio,
	Instance::InstanceType_Folder,
	Instance::InstanceType_IntegerValue,
	Instance::InstanceType_FloatValue,
	Instance::InstanceType_StringValue,
	Instance::InstanceType_BooleanValue,
	Instance::InstanceType_Vector3Value,
	Instance::InstanceType_InstanceValue,
	Instance::InstanceType_PointLight
};

const std::string EngineGUI::placeholderNoInstance = "No instance selected";
char EngineGUI::search[];
bool EngineGUI::isModalAddInstanceOpen = false;
bool EngineGUI::newInstanceSelected = false;
bool EngineGUI::isInspectorFocused = false;
bool EngineGUI::isHierarchyFocused = false;
bool EngineGUI::fullyCancelEyeDropper = false;
bool EngineGUI::isInEyeDropperMode = false;
std::vector<std::string> EngineGUI::tooltipStrings;
#define TOOLTIPS_FILEPATH "core://tooltips/tooltips.xml"
std::string EngineGUI::getTooltipString(int type) {
	if (wasXMLSuccessful && type >= 0 && type < tooltipStrings.size()) {
		return tooltipStrings[type];
	}
	else {
		return "No tooltip available.";
	}
}
void EngineGUI::InitializeEngineGUI() {

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(Graphics::getInstance()->window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	pugi::xml_parse_result result = tooltipsXML.load_file(Resource::resolveFilePath(TOOLTIPS_FILEPATH).c_str());

	if (!result) {
		wasXMLSuccessful = false;
	}
	else {
		wasXMLSuccessful = true;

		for (pugi::xml_node node : tooltipsXML.child("root").children("tooltip")) {
			tooltipStrings.push_back(node.child("text").text().get());
		}
	}

}

void EngineGUI::selectInstance(Instance* inst) {
	if (inst) {
		selectedInstanceId = inst->getId();
		newInstanceSelected = true;
		//debug_log("Selected instance ID #%d \"%s\"\n", selectedInstanceId, inst->name.c_str());
	}
	else {
		selectedInstanceId = -1;
		newInstanceSelected = true;
		//debug_log("Deselected instance\n");
	}
	updateSelectedInstanceHash();
}

void EngineGUI::updateSelectedInstanceHash() {
	if (selectedInstanceId != -1) {
		Instance* inst = Instance::getInstanceById(selectedInstanceId);
		if (inst) {
			selectedInstanceHash = inst->getEngineGUIHash();
		}
		else {
			selectedInstanceHash = "";
			selectedInstanceId = -1;
		}
	}
	else {
		selectedInstanceHash = "";
	}
}
void EngineGUI::DrawHierarchyNode(Instance* instance) {
	if (!instance) return;

	// 1. Configure visual flags
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	if (selectedInstanceHash == instance->getEngineGUIHash()) { // Replace ->id with your actual ID getter
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	// Check if this specific instance has children to look through
	bool has_children = !instance->children.empty();
	if (!has_children) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	// 2. Render the node (using ### to guarantee a unique hash via ID)
	std::string display_name = instance->getEngineGUIHash();
	bool is_open = ImGui::TreeNodeEx(display_name.c_str(), flags);


	if (ImGui::BeginDragDropSource() && instance->type != Instance::InstanceType_Scene) {
		// Pass a unique string ID (max 32 chars) and pointer/ID data

		int node_id = instance->getId();
		ImGui::SetDragDropPayload("SCENE_NODE", &node_id, sizeof(int));
		ImGui::Text("Moving %s", instance->name.c_str());
		ImGui::EndDragDropSource();
	}

	// 2. Allow other nodes to be dropped onto this node
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NODE")) {
			int dragged_node_id = *(const int*)payload->Data;
			// Handle reparenting or reordering logic here


			Instance* dragged_instance = Instance::getInstanceById(dragged_node_id);
			if (dragged_instance && dragged_instance != instance) {
				// Reparent the dragged instance to the current instance
				dragged_instance->setParent(instance);
				//debug_log("Reparented %s to %s\n", dragged_instance->name.c_str(), instance->name.c_str());
			}
		}
		ImGui::EndDragDropTarget();
	}

	if (ImGui::BeginPopupContextItem(display_name.c_str())) {
		//printf("Drawing for Instance %s\n", instance->name.c_str());
		if (ImGui::MenuItem("Clone")) {
			// Do action
			selectInstance(instance->clone());

		}
		if (ImGui::MenuItem("Delete")) {
			// Do action
			Instance::destroyQueue.insert(instance);
		}
		if (ImGui::MenuItem("Insert Object")) {
			isModalAddInstanceOpen = true;
			search[0] = '\0'; // Clear search text whenever they click add
			instanceAddParentId = instance->getId();
		}

		ImGui::EndPopup();
	}

	// 3. Handle selection input
	if (ImGui::IsItemClicked()) {

		if (isEyeDropping()) {
			eyeDropperId = instance->getId();
			EngineGUI::isInEyeDropperMode = false;
			debug_log("Eye dropped on ID #%d \"%s\"\n", eyeDropperId, instance->name.c_str());
		}
		else {
			selectInstance(instance);
		}
	}

	renderTooltipForItem(instance->getTypeTooltip());
	// 4. Cascade down if the node is expanded by the user
	if (is_open) {
		if (has_children) {
			for (Instance* child : instance->children) {
				// Recursively call the same function for the next layer down

				if (child->omitFromHierarchyRender()) {
					continue; // Skip this child if it's null or should be omitted
				}
				DrawHierarchyNode(child);
			}
		}
		ImGui::TreePop(); // Pop out of the current indentation layer
	}
}

void EngineGUI::DrawInspectorNode() {
	// Check if we have a valid selection string

	if (isEyeDropping()) {
		//ImGui::TextDisabled("Click on an object in the Hierarchy to select it as the target.");
	}
	if (!selectedInstanceHash.empty()) {

		// Safely look up the selected pointer in your global world map
		auto it = Instance::world.find(selectedInstanceId);

		if (it != Instance::world.end() && it->second) {
			Instance* selected_instance = it->second;

			// Pass the active instance to our property renderer function
			DrawInspectorPanel(selected_instance);

		}
		else {
			ImGui::TextDisabled("Selected object no longer exists in world.");
		}
	}
	else {
		ImGui::TextDisabled("Select an object from the Hierarchy to view properties.");
	}
}

void EngineGUI::DrawInspectorPanel(Instance* inst) {

	ImGui::InputText("Name", &inst->name);
	renderTooltipForItem(IDS_IATTR_NAME);
	std::string id = std::to_string(inst->getId());
	ImGui::InputText("ID", &id, ImGuiInputTextFlags_ReadOnly);
	renderTooltipForItem(IDS_IATTR_IID);
	std::string type = Instance::getInstanceNameFromType(inst->type);
	ImGui::InputText("Class Name", &type, ImGuiInputTextFlags_ReadOnly);
	renderTooltipForItem(IDS_IATTR_CLASSNAME);

	if (hasEyeDropCompleted(inst->getId())) {
		inst->setParent(Instance::getInstanceById(eyeDropperId));
		endEyeDropper(nullptr);

	}
	ImGui::InputText("Parent", placeholderName(inst->parent), ImGuiInputTextFlags_ReadOnly);
	if (ImGui::Button("Select Parent...")) {
		startEyeDropper(inst->getId());
	}

	if (ImGui::CollapsingHeader("Attributes", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Indent();

		if (Instance::isInstanceOf(inst->type, Instance::InstanceType_Mesh)) {
			Mesh* mesh = (Mesh*)inst;

			if (hasEyeDropCompleted(inst->getId(), 1)) {
				endEyeDropper(&mesh->material);
				mesh->VAODirty = true;
			}
			if (hasEyeDropCompleted(inst->getId(), 2)) {
				endEyeDropper(&mesh->geometry);
				mesh->VAODirty = true;
			}
			ImGui::InputFloat3("Position", (float*)&mesh->position);
			renderTooltipForItem(IDS_IATTR_POSITION);
			DrawRotationFloat3(&mesh->rotation);
			renderTooltipForItem(IDS_IATTR_ROTATION);
			ImGui::InputFloat3("Scale", (float*)&mesh->scale);
			renderTooltipForItem(IDS_IATTR_SCALE);
			Instance* meshMat = Instance::getInstanceById(mesh->material);

			ImGui::InputText("Material", placeholderName(meshMat), ImGuiInputTextFlags_ReadOnly);
			renderTooltipForItem(IDS_IATTR_MATERIAL);
			if (ImGui::Button("Select Material...")) {
				startEyeDropper(inst->getId(), 1);
				//eyeDropperSpecificType = Instance::InstanceType_Material;
			}

			if (inst->type == Instance::InstanceType_Mesh) {
				Instance* meshGeom = Instance::getInstanceById(mesh->geometry);
				ImGui::InputText("Geometry", placeholderName(meshGeom), ImGuiInputTextFlags_ReadOnly);
				renderTooltipForItem(IDS_IATTR_GEOMETRY);
				if (ImGui::Button("Select Geometry...")) {
					startEyeDropper(inst->getId(), 2);
					eyeDropperSpecificType = Instance::InstanceType_Geometry;
				}

			}
			if (inst->type == Instance::InstanceType_TextGeometry) {
				TextGeometry* m = (TextGeometry*)inst;
				if (newInstanceSelected) {
					auto it = TextGeometry::fontCharacterMaps.find(m->font);
					if (it != TextGeometry::fontCharacterMaps.end()) {
						currentSelectionDropdown = std::distance(TextGeometry::fontCharacterMaps.begin(), it);
					}
					else {
						currentSelectionDropdown = -1;
					}
				}
				if (DrawDropdown("Font", TextGeometry::fontNames.data(), TextGeometry::fontNames.size()) != -1) {
					m->setFont(TextGeometry::fontNames[currentSelectionDropdown]);
				}

				if (ImGui::InputText("Text", &m->text)) {
					m->isDirty = true;
				}

			}

			ImGui::Checkbox("Backface Culling", &(mesh->cull));
			if (newInstanceSelected) {
				currentSelectionDropdown2 = mesh->renderLayer;
			}
			if (DrawDropdown("Render Layer", (std::string*)Mesh::renderLayerNames, eRenderLayer_MAX, &currentSelectionDropdown2) != -1) {
				mesh->renderLayer = currentSelectionDropdown2;
			}
		}

		if (inst->type == Instance::InstanceType_Camera) {
			Camera* m = (Camera*)inst;
			ImGui::InputFloat3("Position", (float*)&m->position);
			renderTooltipForItem(IDS_IATTR_POSITION);
			DrawRotationFloat3(&m->rotation);
			renderTooltipForItem(IDS_IATTR_ROTATION);
			ImGui::Checkbox("Orthographic", &m->isOrthographic);
			ImGui::SliderFloat("Field Of View", &m->fov, 0.0f, 180.0f, "FOV: %.3f deg");
		}
		if (inst->type == Instance::InstanceType_Scene) {
			Scene* m = (Scene*)inst;
			if (hasEyeDropCompleted(inst->getId(), 2)) {
				endEyeDropper(&m->startCamera);
			}
			if (hasEyeDropCompleted(inst->getId(), 3)) {
				endEyeDropper(&m->renderShader);
			}
			ImGui::InputText("Start Camera", placeholderName(Instance::getInstanceById(m->startCamera)), ImGuiInputTextFlags_ReadOnly);
			renderTooltipForItem(IDS_IATTR_GEOMETRY);
			if (ImGui::Button("Select Start Camera...")) {
				startEyeDropper(inst->getId(), 2);
				eyeDropperSpecificType = Instance::InstanceType_Camera;
			}

			ImGui::InputText("Output Shader", placeholderName(Instance::getInstanceById(m->renderShader)), ImGuiInputTextFlags_ReadOnly);
			renderTooltipForItem(IDS_IATTR_GEOMETRY);
			if (ImGui::Button("Select Output Shader...")) {
				startEyeDropper(inst->getId(), 3);
				eyeDropperSpecificType = Instance::InstanceType_ShaderProgram;
			}

			ImGui::ColorEdit3("Ambient Light", (float*)&Light::ambient);
			ImGui::ColorEdit4("Background Color", (float*)&Graphics::backgroundColor);
			if (ImGui::InputFloat("Move Snap", (float*)&gizmoSnapPhysical)) {
				if (gizmoSnapPhysical < 0.0f) {
					gizmoSnapPhysical = 0.0f;
				}
			}
			if (ImGui::InputFloat("Rotate Snap", (float*)&gizmoSnapOrientation)) {
				if (gizmoSnapOrientation < 0.0f) {
					gizmoSnapOrientation = 0.0f;
				}
			}


			if (newInstanceSelected) {
				if (gizmoType == GizmoType_Translate) {
					currentSelectionDropdown = 0;
				}
				else if (gizmoType == GizmoType_Rotate) {
					currentSelectionDropdown = 1;
				}
			}
			if (DrawDropdown("Gizmo Type", (std::string*)gizmoTypeNames, 3) != -1) {
				gizmoType = currentSelectionDropdown;
			}

			ImGui::Checkbox("Rotate In Local Space", &gizmoLocalSpace);
		}
		if (inst->type == Instance::InstanceType_Collision3D) {
			Collision3D* m = (Collision3D*)inst;
			vec3 position = m->position;
			vec3 size = m->scale;
			if (ImGui::InputFloat3("Position", (float*)&position)) {
				m->setPosition(position);
			};
			DrawRotationFloat3(&m->rotation);
			renderTooltipForItem(IDS_IATTR_POSITION);
			if (ImGui::InputFloat3("Size", (float*)&size)) {
				m->setSize(size);
			};
			renderTooltipForItem(IDS_IATTR_ROTATION);


			ImGui::Checkbox("Preserve Dimensions", &(m->preserveRotatedSize));
			ImGui::Checkbox("Represent Parent", &(m->wrapAroundParentMesh));
			ImGui::Checkbox("Always Update", &(m->alwaysUpdate));
		}

		if (Instance::isInstanceOf(inst->type, Instance::InstanceType_Value)) {

			if (inst->type == Instance::InstanceType_IntegerValue) {
				IntegerValue* m = (IntegerValue*)inst;
				ImGui::InputInt("Value", &m->value);
			}

			if (inst->type == Instance::InstanceType_FloatValue) {
				FloatValue* m = (FloatValue*)inst;
				ImGui::InputFloat("Value", &m->value);
			}

			if (inst->type == Instance::InstanceType_BooleanValue) {
				BooleanValue* m = (BooleanValue*)inst;
				ImGui::Checkbox("Value", &m->value);
			}

			if (inst->type == Instance::InstanceType_Vector3Value) {
				Vector3Value* m = (Vector3Value*)inst;
				ImGui::InputFloat3("Value", (float*)&m->value);
			}

			if (inst->type == Instance::InstanceType_StringValue) {
				StringValue* m = (StringValue*)inst;
				ImGui::InputText("Value", &m->value);
			}

			if (inst->type == Instance::InstanceType_InstanceValue) {

				if (hasEyeDropCompleted(inst->getId(), 1)) {
					endEyeDropper(&((InstanceValue*)inst)->value);
				}
				InstanceValue* m = (InstanceValue*)inst;
				startEyeDropper(inst->getId(), 1, true);
			}
		}
		if (inst->type == Instance::InstanceType_Material) {
			Material* m = (Material*)inst;

			for (int i = 0; i < eMaterialTexture_MAX; i++) {
				if (hasEyeDropCompleted(inst->getId(), i + 1)) {
					debug_log("Eye dropper completed for target %d\n", i);
					if (isEyeDropperValidAndOfType(Instance::InstanceType_ImageResource)) {
						m->setTextureById(eyeDropperId, i);
					}
					else {
						m->setTextureById(-1, i);
					}
					endEyeDropper(nullptr);
				}
			}

			if (hasEyeDropCompleted(inst->getId(), eMaterialTexture_MAX + 2)) {
				debug_log("Eye dropper completed for shader\n");
				if (isEyeDropperValidAndOfType(Instance::InstanceType_ShaderProgram)) {
					m->setShader((ShaderProgram*)Instance::getInstanceById(eyeDropperId));
				}
				else {
					m->setShader(nullptr);
				}
				endEyeDropper(nullptr);
			}

			ImGui::ColorEdit4("Albedo Color", (float*)&m->diffuseColor);
			ImGui::SliderInt("Specularity", &m->specularity, 0, 256);
			ImGui::SliderFloat("Roughness", &m->roughness, 0, 1);
			renderTooltipForItem(IDS_IATTR_POSITION);
			ImGui::Text("Texture Targets");

			for (int i = 0; i < eMaterialTexture_MAX; i++) {
				ImGui::Checkbox((std::string("Use ") + ImageResource::textureTypeNames[i + 1]).c_str(), &m->textureUseFlags[i]);
				renderTooltipForItem(IDS_IATTR_GEOMETRY);
			}
			for (int i = 0; i < eMaterialTexture_MAX; i++) {
				ImGui::InputText((ImageResource::textureTypeNames[i + 1] + " Texture").c_str(), placeholderName(Instance::getInstanceById(m->getTextureInstanceId(i))), ImGuiInputTextFlags_ReadOnly);
				renderTooltipForItem(IDS_IATTR_GEOMETRY);
				if (ImGui::Button(("Select " + ImageResource::textureTypeNames[i + 1] + " Texture...").c_str())) {
					startEyeDropper(inst->getId(), i + 1, true);
				}
			}
			ImGui::InputText("Shader", placeholderName(Instance::getInstanceById(m->getShaderInstanceId())), ImGuiInputTextFlags_ReadOnly);
			renderTooltipForItem(IDS_IATTR_GEOMETRY);
			if (ImGui::Button("Select Shader...")) {
				startEyeDropper(inst->getId(), eMaterialTexture_MAX + 2, true);
			}
			if (ImGui::Button("Auto-Prepare Material")) {
				m->autoPrepare(true);
			};

			ImGui::Checkbox("Transparent", &(m->isTransparent));
			ImGui::Checkbox("Lit", &(m->isLit));
		}
		if (inst->type == Instance::InstanceType_PointLight) {
			Light* m = (Light*)inst;

			ImGui::InputFloat3("Position", (float*)&m->position);
			ImGui::ColorEdit3("Color", (float*)&m->color);
			renderTooltipForItem(IDS_IATTR_POSITION);
			ImGui::InputFloat("Intensity", (float*)&m->intensity);
			DrawRotationFloat3(&m->rotation);

			float range = m->rangeAngle * (180.0f / PI);
			if (ImGui::SliderFloat("Range", &range, 0, 90, "%.1f degrees")) {
				m->rangeAngle = range * (PI / 180.0f);
			};

			ImGui::Checkbox("Directional", &m->isDirectional);
			ImGui::Checkbox("Sun", &m->isSunLight);
		}
		if (Instance::isInstanceOf(inst->type, Instance::InstanceType_Resource)) {
			Resource* m = (Resource*)inst;
			if (ImGui::InputText("File Path", &m->filePath, ImGuiInputTextFlags_EnterReturnsTrue)) {
				if (inst->type == Instance::InstanceType_LuaResource) {
					LuaResource* luaRes = (LuaResource*)inst;
					luaRes->loadFromFile(m->filePath);
				}

				if (inst->type == Instance::InstanceType_ImageResource) {
					ImageResource* imgRes = (ImageResource*)inst;
					imgRes->loadFromFile(m->filePath);
				}
				if (inst->type == Instance::InstanceType_ShaderResource) {
					ShaderResource* imgRes = (ShaderResource*)inst;
					imgRes->loadFromFile(m->filePath.c_str());
				}
				if (inst->type == Instance::InstanceType_Geometry) {
					Geometry* imgRes = (Geometry*)inst;
					imgRes->loadFromFile(m->filePath.c_str());
				}

				if (inst->type == Instance::InstanceType_ShaderProgram) {
					ShaderProgram* imgRes = (ShaderProgram*)inst;
					imgRes->createProgram();
				}
			}
			renderTooltipForItem(IDS_IATTR_FILEPATH);
			if (inst->type == Instance::InstanceType_ImageResource) {
				ImageResource* imgRes = (ImageResource*)inst;
				if (newInstanceSelected) {
					printf("New instance selected, resetting dropdown to %d\n", ((ImageResource*)inst)->textureType);
					currentSelectionDropdown = ((ImageResource*)inst)->textureType + 1;
				}

				if (DrawDropdown("Texture Port", (std::string*)ImageResource::textureTypeNames, eMaterialTexture_MAX + 1) != -1) {
					imgRes->textureType = currentSelectionDropdown - 1;
					imgRes->loadFromFile(imgRes->filePath);
				}
			}

			if (inst->type == Instance::InstanceType_ShaderResource) {
				ShaderResource* m = (ShaderResource*)inst;
				if (newInstanceSelected) {
					printf("New instance selected, resetting dropdown to %d\n", ((ShaderResource*)inst)->shaderType);
					if (((ShaderResource*)inst)->shaderType == GL_VERTEX_SHADER) {
						currentSelectionDropdown = 0;
					}
					else if (((ShaderResource*)inst)->shaderType == GL_FRAGMENT_SHADER) {
						currentSelectionDropdown = 1;
					}
				}
				if (DrawDropdown("Shader Type", (std::string*)ShaderResource::shaderTypeNames, 2) != -1) {
					m->shaderType = ShaderResource::shaderTypeEnums[currentSelectionDropdown];
					m->loadFromFile(m->filePath.c_str());
				}
			}

			if (inst->type == Instance::InstanceType_Geometry) {
				Geometry* m = (Geometry*)inst;
				if (ImGui::Checkbox("Generate With Smoothed Normals", &m->autoCreateSmoothedNormals)) {
					m->loadFromFile(m->filePath);
				}
			}

		}

		if (inst->type == Instance::InstanceType_ShaderProgram) {
			if (ImGui::Button("Compile Shader...")) {
				ShaderProgram* m = (ShaderProgram*)inst;
				m->createProgram(true);
			}
		}
		if (inst->type == Instance::InstanceType_Folder) {
			Folder* m = (Folder*)inst;
			ImGui::Checkbox("Storage Folder", &m->storageFolder);
		}
		if (inst->type == Instance::InstanceType_UIRootNode) {
			UIRootNode* m = (UIRootNode*)inst;

			if (hasEyeDropCompleted(inst->getId(), 1)) {
				endEyeDropper(&m->camera);
			}

			ImGui::InputText("Camera", placeholderName(Instance::getInstanceById(m->camera)), ImGuiInputTextFlags_ReadOnly);
			renderTooltipForItem(IDS_IATTR_GEOMETRY);
			if (ImGui::Button("Select Camera...")) {
				startEyeDropper(inst->getId(), 1);
				eyeDropperSpecificType = Instance::InstanceType_Camera;
			}
		}

		if (inst->type == Instance::InstanceType_Audio) {
			Audio* m = (Audio*)inst;

			if (newInstanceSelected) {
				currentSamplesAudioPlayback = m->getTimestampSamples();
				maxSamplesAudioPlayback = m->getMaxSamples();
			}
			if (hasEyeDropCompleted(inst->getId(), 1)) {
				endEyeDropper(&m->group);
			}
			if (ImGui::InputText("File Path", &m->audioName, ImGuiInputTextFlags_EnterReturnsTrue)) {
				Audio::loadSound(m->audioName);
				m->setAudioSource(m->audioName);
			}
			if (ImGui::Button("Play")) {
				m->play();
			}
			if (ImGui::Button("Stop")) {
				m->stop();
			}
			ImGui::PushItemWidth(-1.0f);
			if (ImGui::SliderInt("##AudioSeeker", &currentSamplesAudioPlayback, 0, m->getMaxSamples(), "")) {
				// Handle manual dragging adjustments here
			}
			ImGui::PopItemWidth();

			if (ImGui::IsItemDeactivatedAfterEdit()) {
				// The user just let go of the mouse! 
				m->setTimestampSamples(currentSamplesAudioPlayback);
			}
			// Keep your standard audio engine safety check
			if (!ImGui::IsItemActive()) {
				currentSamplesAudioPlayback = m->getTimestampSamples();
			}

			char timeOverlay[32];
			snprintf(timeOverlay, sizeof(timeOverlay), "%02d:%02d / %02d:%02d",
				(int)floor(m->getTimestampSeconds()) / 60, (int)floor(m->getTimestampSeconds()) % 60,
				(int)floor(m->getMaxSeconds()) / 60, (int)floor(m->getMaxSeconds()) % 60);
			// 4. Position and render the text directly below
			// You can use ImGui::SameLine() variations if you want to align it to the right, 
			// but by default, this prints left-aligned on the very next line:
			ImGui::Text("%s", timeOverlay);


			DrawEyeDropper("Sound Group", inst, &m->group, 1, IDS_IATTR_GEOMETRY, Instance::InstanceType_Audio);


		}

		ImGui::Unindent();
	}


	if (ImGui::CollapsingHeader("Tags", ImGuiTreeNodeFlags_DefaultOpen)) {
		std::vector<std::string> tags = inst->tags.getTagsAsVector();
		int indexToRemove = -1; // Initialize to -1 to indicate no removal

		if (tags.empty()) {
			ImGui::TextDisabled("No tags available.");
		}
		else if (ImGui::BeginTable("TagsTable", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed);

			for (int i = 0; i < (int)tags.size(); ++i) {
				ImGui::TableNextRow();
				ImGui::PushID(i); // Required so every "X" button has a unique ID

				// Column 0: tag label
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(tags[i].c_str());

				// Column 1: remove button
				ImGui::TableSetColumnIndex(1);
				if (ImGui::SmallButton("X")) {
					indexToRemove = i; // Defer removal until outside the loop
				}

				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		// Safely erase after ImGui finishes iterating
		if (indexToRemove != -1) {
			inst->removeTag(tags[indexToRemove]);
		}

		    ImGui::Spacing();

    // --- Add New Tag Input + Button ---
    static char newTagBuffer[64] = "";
    
    // Make the input box fill available width minus the "Add" button width
    float addButtonWidth = ImGui::CalcTextSize("Add").x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - addButtonWidth);

    bool enterPressed = ImGui::InputTextWithHint(
        "##NewTagInput", 
        "New tag...", 
        newTagBuffer, 
        IM_ARRAYSIZE(newTagBuffer), 
        ImGuiInputTextFlags_EnterReturnsTrue
    );

    ImGui::SameLine();
    bool addClicked = ImGui::Button("Add");

    // Submit when either "Add" is clicked or Enter is pressed
    if ((enterPressed || addClicked) && newTagBuffer[0] != '\0') {
        std::string newTag(newTagBuffer);

        inst->addTag(newTag);

        newTagBuffer[0] = '\0'; // Clear input box
        if (enterPressed) {
            ImGui::SetKeyboardFocusHere(-1); // Keep focus in the box for rapid entry
        }
    }
	}


}

void EngineGUI::DrawEyeDropper(std::string text, Instance* inst, int* id, int reason, int toolTip, int specificType) {

	ImGui::InputText(text.c_str(), placeholderName(Instance::getInstanceById(*id)), ImGuiInputTextFlags_ReadOnly);
	if (toolTip > -1) {
		renderTooltipForItem(toolTip);
	}
	if (ImGui::Button(("Select" + text + "...").c_str())) {
		startEyeDropper(inst->getId(), reason);
	}
}
void EngineGUI::startEyeDropper(int ID, int reason, bool fullyCancel) {
	eyeDropperId = -1;
	eyeDropperToUpdateId = ID;
	eyeDropperReason = reason;
	fullyCancelEyeDropper = fullyCancel;
	isInEyeDropperMode = true;
	eyeDropperSpecificType = -1;
}

bool EngineGUI::isEyeDropping() {
	return isInEyeDropperMode;
}

void EngineGUI::endEyeDropper(int* port) {
	if (port && isEyeDropperValidAndOfType(eyeDropperSpecificType)) {
		*port = eyeDropperId;
	}
	eyeDropperId = -1;
	eyeDropperToUpdateId = -1;
	eyeDropperReason = -1;
	isInEyeDropperMode = false;
	fullyCancelEyeDropper = true;
}

bool EngineGUI::hasEyeDropCompleted(int id, int reason) {
	return eyeDropperToUpdateId == id && !isEyeDropping() && eyeDropperReason == reason;
}

std::string* EngineGUI::placeholderName(Instance* inst) {
	if (inst) {
		return &inst->name;
	}
	else {
		return (std::string*)&placeholderNoInstance;
	}
}

void EngineGUI::DrawRightClickVoidContextMenuNode() {
	if (ImGui::BeginPopupContextVoid("Edit")) {
		ImGui::EndPopup();
	}
}

void EngineGUI::DrawAddInstanceModalNode() {

	if (!isModalAddInstanceOpen) return;
	ImGui::OpenPopup("Add Instance Type");
	ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal("Add Instance Type", &isModalAddInstanceOpen, ImGuiWindowFlags_NoResize)) {
		//printf("e\n");
		// 5. The Search Bar inside the popup
		ImGui::Text("Filter:");
		ImGui::SameLine();
		ImGui::InputTextWithHint("##InstanceSearch", "Type to filter...", search, IM_ARRAYSIZE(search));

		ImGui::Separator();

		// 6. Scrollable child region for the matching results
		if (ImGui::BeginChild("TypeList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
			//printf("b\n");
			for (const int type : EDITOR_ADD_INSTANCE) {

				// Basic case-insensitive conversion helper
				auto matches_search = [](const std::string& str, const std::string& query) {
					if (query.empty()) return true;
					auto it = std::search(str.begin(), str.end(), query.begin(), query.end(),
						[](char a, char b) { return std::tolower(a) == std::tolower(b); });
					return it != str.end();
					};

				// Filter the results
				if (!matches_search(Instance::getInstanceNameFromType(type), search)) {
					continue;
				}

				// Render matching types as selectable items
				if (ImGui::Selectable(Instance::getInstanceNameFromType(type).c_str())) {

					newInstanceFromType(type, instanceAddParentId);

					isModalAddInstanceOpen = false; // Close the window on selection
					instanceAddParentId = -1;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndChild();
		}

		ImGui::Separator();

		// 7. Cancel button at the bottom
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			isModalAddInstanceOpen = false;
			instanceAddParentId = -1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

Instance* EngineGUI::newInstanceFromType(int type, int p) {
	Instance* i = Scene::newInstanceFromType(type);
	if (i && p) {
		i->setParent(Instance::getInstanceById(p));
	}
	if (i->type == Instance::InstanceType_Material) {
		Material* m = (Material*)i;
	}
	return i;
}

void EngineGUI::renderTooltipForItem(int tooltipType) {

	if (tooltipType < 0) return;
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(getTooltipString(tooltipType).c_str());
		ImGui::EndTooltip();
	}
}

int EngineGUI::DrawDropdown(std::string name, std::string options[], int numChoices, int* selection) {
	int selectionDropdown = -1;
	std::string previewText = "";
	if (*selection != -1) {
		previewText = options[*selection];
	}
	if (ImGui::BeginCombo(name.c_str(), previewText.c_str())) {

		if (ImGui::IsWindowAppearing()) {
			*selection = 0;
		}
		if (numChoices == 0) {
			numChoices = IM_ARRAYSIZE(options);
		}
		for (int n = 0; n < numChoices; n++) {
			bool is_selected = (*selection == n);
			if (ImGui::Selectable(options[n].c_str(), is_selected)) {
				*selection = n;
				selectionDropdown = n;
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	return selectionDropdown;
}

bool EngineGUI::isEyeDropperValidAndOfType(int type) {
	Instance* i = Instance::getInstanceById(eyeDropperId);
	if (i) {
		return Instance::isInstanceOf(i->type, type) || type == -1;
	}
	else {
		return false;
	}
}

void EngineGUI::DrawRotationFloat3(vec3* edit) {
	vec3 degrees = *edit * (180 / PI);

	if (ImGui::InputFloat3("Rotation", (float*)&degrees)) {
		*edit = degrees * (PI / 180);
	}
}

void EngineGUI::calculateMouseRayDirection() {
	MouseData md = Graphics::getInstance()->mouse_data;
	vec2 mouseNDC = vec2(md.screenNormalized.X, -md.screenNormalized.Y);

	// 1. Unproject from Clip Space to Eye (Camera) Space
	vec4 clipCoords = vec4(mouseNDC.X, mouseNDC.Y, -1.0f, 1.0f);
	Camera* cam = Scene::getCurrentCamera();
	vec4 eyeCoords = cam->projection.inverse() * clipCoords;

	// Perform perspective divide so X, Y, and Z share the exact same scale
	if (eyeCoords.W != 0.0f) {
		eyeCoords.X /= eyeCoords.W;
		eyeCoords.Y /= eyeCoords.W;
		eyeCoords.Z /= eyeCoords.W;
	}
	eyeCoords.W = 0.0f; // Treat as a direction vector (ignore camera translation)

	// 2. Transform from Camera Space to World Space (NO .inverse() here!)
	vec4 worldDirVec4 = cam->getWorldTransform() * eyeCoords;
	mouseRayDirection = vec3(worldDirVec4.X, worldDirVec4.Y, worldDirVec4.Z).smag(1.0f);


}

void EngineGUI::DrawGizmo() {

	isGizmoRender = false;
	if (!Scene::isStudio) return;
	//many many many sanity checks
	if (selectedInstanceId == -1) return;
	SpatialInstance* si = (SpatialInstance*)Instance::getInstanceById(selectedInstanceId);
	if (!si) return;
	if (!Instance::isInstanceOf(si->type, Instance::InstanceType_Spatial)) return;

	if (gizmoType == GizmoType_Scale) {
		DrawScaleGizmo(si);
		return;
	}
	Mesh* tx = (Mesh*)Instance::getInstanceById(Scene::translateInstance[eAxis_X - 1]);
	if (!tx) return;
	if (!Instance::isInstanceOf(tx->type, Instance::InstanceType_Mesh)) return;

	Mesh* ty = (Mesh*)Instance::getInstanceById(Scene::translateInstance[eAxis_Y - 1]);
	if (!ty) return;
	if (!Instance::isInstanceOf(ty->type, Instance::InstanceType_Mesh)) return;

	Mesh* tz = (Mesh*)Instance::getInstanceById(Scene::translateInstance[eAxis_Z - 1]);
	if (!tz) return;
	if (!Instance::isInstanceOf(tz->type, Instance::InstanceType_Mesh)) return;

	Mesh* orgn = (Mesh*)Instance::getInstanceById(Scene::originTransformInstance);
	if (!orgn) return;
	if (!Instance::isInstanceOf(orgn->type, Instance::InstanceType_Mesh)) return;

	Mesh* gizmoAnchor = (Mesh*)tz->parent;
	if (!gizmoAnchor) return;

	gizmoAnchor->position = si->getWorldPosition();

	gizmoAnchor->scale = vec3(1.0f, 1.0f, 1.0f) * (si->getWorldPosition() - Scene::getCurrentCamera()->position).gmag() * 0.05f;

	if (gizmoLocalSpace) gizmoAnchor->rotation = si->getWorldRotationMatrix().toEuler();
	else gizmoAnchor->rotation = vec3(0, 0, 0);

	glClearStencil(100);
	glStencilMask(0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glDisable(GL_STENCIL_TEST);

	orgn->render(Scene::getCurrentCamera(), true);
	glEnable(GL_STENCIL_TEST);

	if (!isGizmoActive || axisSelected == eAxis_X) {
		glStencilFunc(GL_ALWAYS, eAxis_X + 10, 0xFF);
		tx->render(Scene::getCurrentCamera(), true);
	}
	if (!isGizmoActive || axisSelected == eAxis_Y) {
		glStencilFunc(GL_ALWAYS, eAxis_Y + 10, 0xFF);
		ty->render(Scene::getCurrentCamera(), true);
	}
	if (!isGizmoActive || axisSelected == eAxis_Z) {
		glStencilFunc(GL_ALWAYS, eAxis_Z + 10, 0xFF);
		tz->render(Scene::getCurrentCamera(), true);
	}
	isGizmoRender = true;
	MouseData md = Graphics::getInstance()->mouse_data;

	//Coordinates should be from 0 - windowWidth and 0 - windowHeight, with (0,0) being the bottom left corner of the window
	int glMouseX = (int)((md.screenNormalized.X + 1) * 0.5 * Graphics::getInstance()->windowWidth);
	int glMouseY = (int)((1 - ((md.screenNormalized.Y + 1) * 0.5)) * Graphics::getInstance()->windowHeight);
	GLubyte hoveredID = 0;


	glReadPixels(glMouseX, glMouseY, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &hoveredID);

	if (!isGizmoActive) {
		axisSelected = hoveredID - 10;
	}
	//Works
	glStencilMask(0xFF);
	glDisable(GL_STENCIL_TEST);
}

void EngineGUI::setAxisMovementMode(bool isMoving) {

	printf("Moving %d, %d\n", isMoving, axisSelected);
	if (isMoving) {
		if (selectedInstanceId == -1) return;
		SpatialInstance* si = (SpatialInstance*)Instance::getInstanceById(selectedInstanceId);
		if (!si) return;
		if (!Instance::isInstanceOf(si->type, Instance::InstanceType_Spatial)) return;

		isGizmoActive = true;

		if (axisSelected != eAxis_X && axisSelected != eAxis_Y && axisSelected != eAxis_Z) {
			isGizmoActive = false;
			return;
		}

		if (gizmoType == GizmoType_Translate) {
			originalObjectTransform = si->getWorldPosition();
			originalObjectAxis = vec3::axis(axisSelected);
			gizmoAxisOffset = getMoveDistanceAlongAxis();
		}
		else if (gizmoType == GizmoType_Rotate) {


			SpatialInstance* parentSpatial = si->getNextSpatialParent();
			gizmoParentRotation = parentSpatial ? parentSpatial->getWorldRotationMatrix() : mat3();
			gizmoStartWorldRot = si->getWorldRotationMatrix();
			originalObjectTransform = si->rotation;

			vec3 localAxis = vec3::axis(axisSelected);
			originalObjectAxis = gizmoLocalSpace ? gizmoStartWorldRot * localAxis : localAxis; // world-space axis
			gizmoAxisOffset = getRotateDistanceAlongAxis();
		}
		else if (gizmoType == GizmoType_Scale) {
			gizmoStartScale = si->scale;

			vec3 center = si->getWorldPosition();
			vec3 worldAxis = (si->getWorldRotationMatrix() * vec3::axis(axisSelected)).smag(1.0f);
			vec3 worldScale = si->getWorldScale();

			gizmoStartSize = fmaxf(axisRef(worldScale, axisSelected) * kScaleHalfExtent * 2.0f, kMinScaleSize);
			originalObjectAxis = worldAxis * (float)gizmoScaleSign;       // points from fixed face toward grabbed face
			gizmoFixedPoint = center - originalObjectAxis * (gizmoStartSize * 0.5f);
			gizmoStartGrab = getDragDistanceAlongWorldAxis(gizmoFixedPoint, originalObjectAxis);
		}
	}
	else {
		axisSelected = 0;
		isGizmoActive = false;
	}
}
void EngineGUI::updateGizmoObjectScale() {
	if (!isGizmoActive) return;
	SpatialInstance* si = dynamic_cast<SpatialInstance*>(Instance::getInstanceById(selectedInstanceId));
	if (!si) return;

	// New world size of the box along the axis: original size + how far the mouse has moved.
	float newSize = gizmoStartSize + (getDragDistanceAlongWorldAxis(gizmoFixedPoint, originalObjectAxis) - gizmoStartGrab);

	if (gizmoSnapPhysical > 0.0f) {
		newSize = roundf(newSize / gizmoSnapPhysical) * gizmoSnapPhysical;
	}
	newSize = fmaxf(newSize, kMinScaleSize);

	// Scale: local scale is proportional to world size
	vec3 newScale = gizmoStartScale;
	axisRef(newScale, axisSelected) *= newSize / gizmoStartSize;
	si->scale = newScale;

	// Position: the box center sits halfway between the fixed face and the dragged face
	vec3 newCenter = gizmoFixedPoint + originalObjectAxis * (newSize * 0.5f);

	SpatialInstance* parentSpatial = si->getNextSpatialParent();
	if (parentSpatial) {
		vec4 local = parentSpatial->getWorldTransform().inverse() * vec4(newCenter.X, newCenter.Y, newCenter.Z, 1.0f);
		si->position = vec3(local.X, local.Y, local.Z);
	}
	else {
		si->position = newCenter;
	}
}

float EngineGUI::getMoveDistanceAlongAxis() {
	if (!isGizmoActive) return 0;

	SpatialInstance* si =
		dynamic_cast<SpatialInstance*>(Instance::getInstanceById(selectedInstanceId));

	if (!si) return 0;

	Mesh* tx = (Mesh*)Instance::getInstanceById(Scene::translateInstance[0]);
	if (!tx || !Instance::isInstanceOf(tx->type, Instance::InstanceType_Mesh)) return 0;

	Mesh* gizmoAnchor = (Mesh*)tx->parent;
	if (!gizmoAnchor) return 0;

	Camera* cam = Scene::getCurrentCamera();
	if (!cam) return 0;

	vec3 cameraPosition = cam->getWorldPosition();
	vec3 gizmoPosition = originalObjectTransform;

	// Transform the selected object's local X axis into world space.
	vec4 worldAxis = si->getWorldTransform() * vec4(originalObjectAxis.X, originalObjectAxis.Y, originalObjectAxis.Z, 0.0f);

	vec3 axis = vec3(worldAxis.X, worldAxis.Y, worldAxis.Z).smag(1.0f);

	vec3 cameraToGizmo = (gizmoPosition - cameraPosition).smag(1.0f);

	vec3 planeNormal = (axis.cross(cameraToGizmo.cross(axis))).smag(1.0f);

	float denominator = mouseRayDirection.dot(planeNormal);
	if (fabs(denominator) < 0.0001f) return 0;

	float t = ((gizmoPosition - cameraPosition).dot(planeNormal)) / denominator;

	vec3 mousePosition = cameraPosition + mouseRayDirection * t;

	float axisDistance = (mousePosition - gizmoPosition).dot(axis);

	return axisDistance;
}

float EngineGUI::getRotateDistanceAlongAxis() {
	if (!isGizmoActive) return 0;
	Camera* cam = Scene::getCurrentCamera();
	SpatialInstance* si = dynamic_cast<SpatialInstance*>(Instance::getInstanceById(selectedInstanceId));
	if (!cam || !si) return 0;

	vec3 origin = si->getWorldPosition();
	vec3 n = originalObjectAxis;
	vec3 camPos = cam->getWorldPosition();

	float denom = n.dot(mouseRayDirection);
	if (fabsf(denom) < 1e-5f) return 0; // ray parallel to the plane
	float t = n.dot(origin - camPos) / denom;
	vec3 hit = camPos + mouseRayDirection * t - origin;

	// Build a basis in the plane
	vec3 ref = fabsf(n.Y) < 0.99f ? vec3(0, 1, 0) : vec3(1, 0, 0);
	vec3 u = n.cross(ref).smag(1.0f);
	vec3 v = n.cross(u);
	return atan2f(hit.dot(v), hit.dot(u));
}

void EngineGUI::updateGizmoObjectRotate() {


	if (!isGizmoActive) return;
	float delta = getRotateDistanceAlongAxis() - gizmoAxisOffset;

	if (gizmoSnapOrientation > 0.0f) {
		delta = roundf(delta / (gizmoSnapOrientation * (PI / 180.0f))) * (gizmoSnapOrientation * (PI / 180.0f));
	}

	SpatialInstance* si = dynamic_cast<SpatialInstance*>(Instance::getInstanceById(selectedInstanceId));
	if (!si) return;
	// Euler rotation is stored in radians (see DrawRotationFloat3)
	mat3 D = mat3::axisAngleToMat3(originalObjectAxis, delta);
	mat3 newWorld = D * gizmoStartWorldRot;
	mat3 newLocal = gizmoParentRotation.transposed() * newWorld;
	si->rotation = newLocal.toEuler();
}
void EngineGUI::updateGizmoObjectTranslate() {


	if (!isGizmoActive) return;
	vec3 gizmoPosition = originalObjectTransform;
	SpatialInstance* si = (SpatialInstance*)(Instance::getInstanceById(selectedInstanceId));
	if (!si) return;

	vec4 worldAxis = si->getWorldTransform() * vec4(originalObjectAxis.X, originalObjectAxis.Y, originalObjectAxis.Z, 0.0f);
	vec3 axis = vec3(worldAxis.X, worldAxis.Y, worldAxis.Z).smag(1.0f);

	float axisDistance = getMoveDistanceAlongAxis() - gizmoAxisOffset;

	if (gizmoSnapPhysical > 0.0f) {
		axisDistance = roundf(axisDistance / gizmoSnapPhysical) * gizmoSnapPhysical;
	}
	const vec3 newWorldPosition = gizmoPosition + axis * axisDistance;

	SpatialInstance* parentSpatial = si->getNextSpatialParent();
	if (parentSpatial) {
		const mat4 inverseParentTransform =
			parentSpatial->getWorldTransform().inverse();



		const vec4 localPosition =
			inverseParentTransform *
			vec4(newWorldPosition.X, newWorldPosition.Y, newWorldPosition.Z, 1.0f);

		si->position = vec3(
			localPosition.X,
			localPosition.Y,
			localPosition.Z
		);
	}
	else {
		si->position = newWorldPosition;

	}
}

void EngineGUI::updateGizmoObject() {
	if (gizmoType == GizmoType_Translate) {
		updateGizmoObjectTranslate();
	}
	else if (gizmoType == GizmoType_Rotate) {
		updateGizmoObjectRotate();
	}
	else if (gizmoType == GizmoType_Scale) {
		updateGizmoObjectScale();
	}
}

float EngineGUI::getDragDistanceAlongWorldAxis(vec3 origin, vec3 worldAxis) {
	Camera* cam = Scene::getCurrentCamera();
	if (!cam) return 0;

	vec3 camPos = cam->getWorldPosition();
	vec3 axis = worldAxis.smag(1.0f);
	vec3 camToOrigin = (origin - camPos).smag(1.0f);
	vec3 planeNormal = axis.cross(camToOrigin.cross(axis)).smag(1.0f);

	float denom = mouseRayDirection.dot(planeNormal);
	if (fabsf(denom) < 0.0001f) return 0;

	float t = (origin - camPos).dot(planeNormal) / denom;
	vec3 hit = camPos + mouseRayDirection * t;
	return (hit - origin).dot(axis);
}

void EngineGUI::DrawScaleGizmo(Instance* s) {

	SpatialInstance* si = dynamic_cast<SpatialInstance*>(s);
	Mesh* handles[6];
	for (int i = 0; i < 6; i++) {
		handles[i] = (Mesh*)Instance::getInstanceById(Scene::scaleInstance[i]);
		if (!handles[i] || !Instance::isInstanceOf(handles[i]->type, Instance::InstanceType_Mesh)) return;
	}
	Mesh* anchor = (Mesh*)handles[0]->parent;
	if (!anchor) return;

	Camera* cam = Scene::getCurrentCamera();
	anchor->position = si->getWorldPosition();
	anchor->rotation = si->getWorldRotationMatrix().toEuler(); // scaling always follows the object's own axes
	anchor->scale = vec3(1.0f, 1.0f, 1.0f);

	vec3 half = si->getWorldScale() * kScaleHalfExtent;
	float handleSize = (si->getWorldPosition() - cam->position).gmag() * 0.03f;

	for (int a = eAxis_X; a <= eAxis_Z; a++) {
		for (int s = 0; s < 2; s++) {
			Mesh* h = handles[(a - 1) * 2 + s];
			vec3 p(0, 0, 0);
			axisRef(p, a) = (s ? -1.0f : 1.0f) * axisRef(half, a);
			h->position = p;
			h->scale = vec3(handleSize, handleSize, handleSize);
		}
	}

	glClearStencil(100);
	glStencilMask(0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glEnable(GL_STENCIL_TEST);

	// Stencil IDs: 11-13 = +X/+Y/+Z, 21-23 = -X/-Y/-Z
	for (int a = eAxis_X; a <= eAxis_Z; a++) {
		for (int s = 0; s < 2; s++) {
			if (isGizmoActive && !(axisSelected == a && (s ? -1 : 1) == gizmoScaleSign)) continue;
			glStencilFunc(GL_ALWAYS, 10 + a + (s ? 10 : 0), 0xFF);
			handles[(a - 1) * 2 + s]->render(cam, true);
		}
	}

	if (!isGizmoActive) {
		MouseData md = Graphics::getInstance()->mouse_data;
		int glMouseX = (int)((md.screenNormalized.X + 1) * 0.5 * Graphics::getInstance()->windowWidth);
		int glMouseY = (int)((1 - ((md.screenNormalized.Y + 1) * 0.5)) * Graphics::getInstance()->windowHeight);
		GLubyte id = 0;
		glReadPixels(glMouseX, glMouseY, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &id);

		if (id >= 11 && id <= 13) { axisSelected = id - 10; gizmoScaleSign = 1; }
		else if (id >= 21 && id <= 23) { axisSelected = id - 20; gizmoScaleSign = -1; }
		else { axisSelected = 0; }
	}
	isGizmoRender = true;
	glStencilMask(0xFF);
	glDisable(GL_STENCIL_TEST);
}


bool EngineGUI::isEditorConsumeKeyInput(int key, int scancode, int action, int mods) {

	if (isGizmoRender && !isGizmoActive) {
		if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_1) {
				gizmoType = GizmoType_Translate;
				return true;
			}
			else if (key == GLFW_KEY_2) {
				gizmoType = GizmoType_Rotate;
				return true;
			}
			else if (key == GLFW_KEY_3) {
				gizmoType = GizmoType_Scale;
				return true;
			}
		}
	}

	return false;
}