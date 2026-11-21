#include "GameInstance.h"
#include "Mesh.h"
#include "Camera.h"
#include "Shader.h"
#include "TextureResource.h"
#include "Scene.h"
#include "Material.h"
#include "LuaResource.h"
#include "EngineGUI.h"
#include "TextGeometry.h"
#include "SoundInstance.h"
#include "Stream.h"
#include "Light.h"
int main() {

	Instance::registerHierarchy();
	//GRAPHICS - get the graphics instance and initialize
	Graphics* graphics = Graphics::getInstance();

	graphics->initializeWindow(800, 600, "Game Engine 3");
	graphics->matchViewportToWindow = true;
	graphics->initializeDeferredRendering();
	LuaResource::InitializeLua();

	ShaderResource::LoadCoreShaderSources();
	EngineGUI::InitializeEngineGUI();
	Audio::InitializeAudioEngine();
	Light::InitializeLightingUBO();
    if (FT_Init_FreeType(&Font::ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

#ifndef STUDIO_EDITOR
	Scene::loadSceneFromFile("scene.bin");
	Scene* scene = Scene::getCurrentScene();
	scene->currentCamera = scene->startCamera;
	debug_log("Loaded scene '%s' (ID %d) from file 'scene.bin'. Starting with camera ID %d.\n", scene->name.c_str(), scene->getId(), scene->currentCamera);
	scene->startCamera = -1;
#else
	Scene* scene = new Scene();
	Camera* camera = new Camera(vec3(0, 0, -2));
	camera->fov = 90.0f;
	camera->isEditorIntrinsic = true;
	scene->currentCamera = camera->getId();
	
	UIRootNode* uir = new UIRootNode();
	uir->setParent(scene);
	scene->uiRoot = uir->getId();
	Folder* resources = new Folder();
	resources->name = "Resources";
	resources->setParent(scene);
	LuaResource* script = new LuaResource("core://scripts/editor_camera_controls.lua");
	script->isEditorIntrinsic = true;
	script->loadScript();

	TextGeometry::loadFont("res://fonts/DroidSans.ttf");
	//render_shader->isEditorIntrinsic = true;
	ShaderProgram* pp_shader = new ShaderProgram();
	pp_shader->filePath = "core://shader/quad_shader.glsl";
	pp_shader->createProgram();
	pp_shader->setParent(resources);
	pp_shader->name = "PostProcessingShader";
	scene->renderShader = pp_shader->getId();

		Material* GizmoMaterialX = new Material();
	GizmoMaterialX->setShader(nullptr);
	GizmoMaterialX->diffuseColor = vec4(1, 0, 0, 0.5);
	GizmoMaterialX->isEditorIntrinsic = true;

	Material* GizmoMaterialY = new Material();
	GizmoMaterialY->setShader(nullptr);
	GizmoMaterialY->diffuseColor = vec4(0, 1, 0, 0.5);
	GizmoMaterialY->isEditorIntrinsic = true;

	Material* GizmoMaterialZ = new Material();
	GizmoMaterialZ->setShader(nullptr);
	GizmoMaterialZ->diffuseColor = vec4(0, 0, 1, 0.5);
	GizmoMaterialZ->isEditorIntrinsic = true;
	Geometry* cubeGeom = new Geometry();
	cubeGeom->isEditorIntrinsic = true;
	cubeGeom->name = "CubeGeom";
	cubeGeom->loadFromFile("res://models/Cube.fbx");

	Mesh* positionGizmoMesh = new Mesh();
	positionGizmoMesh->isEditorIntrinsic = true;

	Mesh* gizmoMeshTX = new Mesh();
	gizmoMeshTX->isEditorIntrinsic = true;
	gizmoMeshTX->name = "MeshTranslateX";
	gizmoMeshTX->setParent(positionGizmoMesh);
	gizmoMeshTX->position = vec3(5, 0, 0);
	gizmoMeshTX->scale = vec3(3.0, 0.3, 0.3);
	gizmoMeshTX->material = GizmoMaterialX->getId();
	gizmoMeshTX->geometry = cubeGeom->getId();
	Scene::translateInstance[eAxis_X - 1] = gizmoMeshTX->getId();

	Mesh* gizmoMeshTY = new Mesh();
	gizmoMeshTY->isEditorIntrinsic = true;
	gizmoMeshTY->name = "MeshTranslateY";
	gizmoMeshTY->setParent(positionGizmoMesh);
	gizmoMeshTY->position = vec3(0, 5, 0);
	gizmoMeshTY->scale = vec3(0.3, 3.0, 0.3);
	gizmoMeshTY->material = GizmoMaterialY->getId();
	gizmoMeshTY->geometry = cubeGeom->getId();
	Scene::translateInstance[eAxis_Y - 1] = gizmoMeshTY->getId();

	Mesh* gizmoMeshTZ = new Mesh();
	gizmoMeshTZ->isEditorIntrinsic = true;
	gizmoMeshTZ->name = "MeshTranslateZ";
	gizmoMeshTZ->setParent(positionGizmoMesh);
	gizmoMeshTZ->position = vec3(0, 0, 5);
	gizmoMeshTZ->scale = vec3(0.3, 0.3, 3.0);
	gizmoMeshTZ->material = GizmoMaterialZ->getId();
	gizmoMeshTZ->geometry = cubeGeom->getId();
	Scene::translateInstance[eAxis_Z - 1] = gizmoMeshTZ->getId();

#endif

	ShaderProgram* core_shader = new ShaderProgram("core://shader/core_vertex.glsl", "core://shader/core_fragment.glsl");
	core_shader->isEditorIntrinsic = true;
	core_shader->createProgram();
	core_shader->name = "MainShader";

	Scene::coreShader = core_shader->getId();
	
	ProceduralGeometry* shaderQuadGeom = new ProceduralGeometry();
	shaderQuadGeom->name = "RenderQuadGeom";
	shaderQuadGeom->isEditorIntrinsic = true;
	shaderQuadGeom->drawElements = true;
	shaderQuadGeom->vboFlags[eVBO_Positions] = true;
	shaderQuadGeom->vboFlags[eVBO_UV] = true;
	shaderQuadGeom->addVertex(ProceduralVertex(
		vec3(-1, -1, 1), vec3(0, 0, 0), vec2(0, 0), vec3()
	));
	shaderQuadGeom->addVertex(ProceduralVertex(
		vec3(1, -1, 1), vec3(0, 0, 0), vec2(1, 0), vec3()
	));
	shaderQuadGeom->addVertex(ProceduralVertex(
		vec3(1, 1, 1), vec3(0, 0, 0), vec2(1, 1), vec3()
	));
	shaderQuadGeom->addVertex(ProceduralVertex(
		vec3(-1, 1, 1), vec3(0, 0, 0), vec2(0, 1), vec3()
	));
	shaderQuadGeom->addTriangle(0, 1, 2);
	shaderQuadGeom->addTriangle(0, 2, 3);

	Mesh* renderQuad = new Mesh();
	renderQuad->isEditorIntrinsic = true;
	renderQuad->cull = false;

	ShaderProgram* debug_shader = new ShaderProgram("core://shader/debug_vertex.glsl", "core://shader/debug_fragment.glsl");
	debug_shader->setDefaultOutputPort(eMaterialTexture_Diffuse, "color");
	debug_shader->isEditorIntrinsic = true;
	debug_shader->createProgram();
	debug_shader->name = "DebugShader";

	Scene::debugShader = debug_shader->getId();

	
	ShaderProgram* render_shader = new ShaderProgram("core://shader/lighting_vertex.glsl", "core://shader/lighting_fragment.glsl");
	render_shader->isEditorIntrinsic = true;
	render_shader->createProgram();
	render_shader->name = "RenderShader";

	/*
	* 
	TODO: Add static instance ID ports for debug shader 
	 
	 that are consistent across all load 
	/ save for editor intrinsic resources.
	This will allow for consistent references to these resources across scenes and saves. */



	ImageResource* diffuse_FBOPortRender = new ImageResource(eMaterialTexture_Diffuse);
	diffuse_FBOPortRender->name = "DiffuseLocation";
	diffuse_FBOPortRender->isEditorIntrinsic = true;
	diffuse_FBOPortRender->frameBufferType = eFramebuffer_Lighting;
	diffuse_FBOPortRender->resizeToWindow = true;
	diffuse_FBOPortRender->loadAsFramebufferTarget();
	
	ImageResource* diffuse_FBOPortPP = new ImageResource(eMaterialTexture_Diffuse);
	diffuse_FBOPortPP->name = "DiffuseLocation";
	diffuse_FBOPortPP->isEditorIntrinsic = true;
	diffuse_FBOPortPP->frameBufferType = eFramebuffer_PostProcess;
	diffuse_FBOPortPP->resizeToWindow = true;
	diffuse_FBOPortPP->loadAsFramebufferTarget();

	ImageResource* normal_FBOPort = new ImageResource(eMaterialTexture_Normal);
	normal_FBOPort->name = "NormalLocation";
	normal_FBOPort->isEditorIntrinsic = true;
	normal_FBOPort->frameBufferType = eFramebuffer_Lighting;
	normal_FBOPort->resizeToWindow = true;
	normal_FBOPort->loadAsFramebufferTarget();

	ImageResource* specular_FBOPort = new ImageResource(eMaterialTexture_Specular);
	specular_FBOPort->name = "NormalLocation";
	specular_FBOPort->isEditorIntrinsic = true;
	specular_FBOPort->frameBufferType = eFramebuffer_Lighting;
	specular_FBOPort->resizeToWindow = true;
	specular_FBOPort->loadAsFramebufferTarget();

	ImageResource* position_FBOPort = new ImageResource(eMaterialTexture_Position);
	position_FBOPort->name = "NormalLocation";
	position_FBOPort->isEditorIntrinsic = true;
	position_FBOPort->frameBufferType = eFramebuffer_Lighting;
	position_FBOPort->resizeToWindow = true;
	position_FBOPort->loadAsFramebufferTarget();

	ImageResource* roughness_FBOPort = new ImageResource(eMaterialTexture_Roughness);
	roughness_FBOPort->name = "RoughnessLocation";
	roughness_FBOPort->isEditorIntrinsic = true;
	roughness_FBOPort->frameBufferType = eFramebuffer_Lighting;
	roughness_FBOPort->resizeToWindow = true;
	roughness_FBOPort->loadAsFramebufferTarget();

	Material* renderQuadMaterial = new Material();
	renderQuadMaterial->setTexture(diffuse_FBOPortRender);
	renderQuadMaterial->setTexture(normal_FBOPort);
	renderQuadMaterial->setTexture(position_FBOPort);
	renderQuadMaterial->setTexture(roughness_FBOPort);
	renderQuadMaterial->setTexture(specular_FBOPort);
	renderQuadMaterial->isEditorIntrinsic = true;
	renderQuadMaterial->textureUseFlags[eMaterialTexture_Diffuse] = true;
	renderQuadMaterial->textureUseFlags[eMaterialTexture_Position] = true;
	renderQuadMaterial->textureUseFlags[eMaterialTexture_Normal] = true;
	renderQuadMaterial->textureUseFlags[eMaterialTexture_Roughness] = true;
	renderQuadMaterial->textureUseFlags[eMaterialTexture_Specular] = true;
	renderQuadMaterial->frameBufferType = eFramebuffer_Lighting;

	Material* ppQuadMaterial = new Material();
	ppQuadMaterial->setTexture(diffuse_FBOPortPP);
	ppQuadMaterial->isEditorIntrinsic = true;
	ppQuadMaterial->textureUseFlags[eMaterialTexture_Diffuse] = true;
	ppQuadMaterial->frameBufferType = eFramebuffer_PostProcess;

	renderQuad->geometry = shaderQuadGeom->getId();
	renderQuad->material = renderQuadMaterial->getId();
	renderQuad->renderLayer = eRenderLayer_UI;
	renderQuad->isFramebuffer = true;
	glfwSetFramebufferSizeCallback(graphics->window, &graphics->resizeFramebufferCallback);
	Light::bindLightsToRender();


	while (graphics->shouldOpenWindow()) {

		if ((glfwGetTime() - graphics->lastFrameTime) < (1.0 / (float)graphics->targetFPS)) continue;
		Scene::getCurrentScene()->PreRender();
		ppQuadMaterial->setShader((ShaderProgram*)Instance::getInstanceById(Scene::getCurrentScene()->renderShader));
		ppQuadMaterial->applyMaterialFramebuffer();
		renderQuadMaterial->setShader(render_shader);
		renderQuadMaterial->applyMaterialFramebuffer();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

#ifdef _DEBUG
		{
			ImGui::Begin("Scene Hierarchy");

			EngineGUI::newInstanceSelected = false;
			EngineGUI::isHierarchyFocused = ImGui::IsWindowFocused();
			EngineGUI::updateSelectedInstanceHash();
			EngineGUI::DrawHierarchyNode(scene->getCurrentScene());

			ImGui::End();
		}
#endif
#ifdef STUDIO_EDITOR

		{
			ImGui::Begin("File Settings");

			if (ImGui::Button("Save Scene As...")) {
				//Add a file dialog here to select the scene file to save
				Scene::saveSceneToFile("scene.bin");
			}

			if (ImGui::Button("Load Scene From ...")) {
				//Add a file dialog here to select the scene file to load

				Scene::getCurrentScene()->destroy();
				Scene::loadSceneFromFile("scene.bin");
				if (script) {
					script->loadScript();
				}
			}
			ImGui::End();
		}
		// Loop through all instances in your world map/vector

		{
			ImGui::Begin("Inspector");
			EngineGUI::isInspectorFocused = ImGui::IsWindowFocused();
			EngineGUI::DrawInspectorNode();

			ImGui::End();
		}

		if (EngineGUI::isEyeDropping() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !EngineGUI::isInspectorFocused && !EngineGUI::isHierarchyFocused) {
			if (!EngineGUI::fullyCancelEyeDropper) {
				EngineGUI::eyeDropperId = -1;
			}

			EngineGUI::isInEyeDropperMode = false;
			printf("Eye dropper cancelled.\n");
		}

		EngineGUI::DrawRightClickVoidContextMenuNode();
		EngineGUI::DrawAddInstanceModalNode();
#endif
		ImGui::Render();


		for (auto i : Instance::world) {
			if (i.second) {
				bool active = !i.second->isInstanceDescendantOfInactiveFolder();
				//i.second->update(graphics->deltaTime, Scene::isStudio);

				if (active) {
					i.second->makeActive();
				}
				else {
					i.second->makeInactive();
				}
			}
		}

		for (int i : Instance::activeWorld) {
			if (Instance::getInstanceById(i)) {
				Instance::getInstanceById(i)->update(graphics->deltaTime, Scene::isStudio);
			}
		}

		Light::updateLightsToUBO();
		/*
			old pipeline

			Render all of the data to the G-Buffer to FBO 1
			Render quad with super fancy lighting to the screene
		*/
		/*
		TODO: change render pipeline

		2 FBOS (one for lighting, one for post-process)
		Render all G-Buffer data to FBO 1 for lit objects only
		Render quad to FBO 2 with lighting pass shader

		Copy ONLY depth to FBO 2
		Render all unlit objects (objects that are customly shaded) to FBO 2
		Render all transparent unlit/lit objects to main FBO 2
		Render quad again to FBO 0 with post-processing pass

		*/
		//printf("%s\n", std::string(EngineGUI::mouseRayDirection).c_str());

		Mesh::setMeshDrawCallGroups();
		Scene::getCurrentScene()->getCamera()->updateMatrices();
		glBindVertexArray(0);
		graphics->getFramebuffer(eFramebuffer_Lighting)->bind();
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		Scene::getCurrentScene()->RenderScene(0);
		graphics->getFramebuffer(eFramebuffer_Lighting)->unbind();
		graphics->getFramebuffer(eFramebuffer_Lighting)->rebindAs(GL_READ_FRAMEBUFFER, false);
		graphics->getFramebuffer(eFramebuffer_PostProcess)->rebindAs(GL_DRAW_FRAMEBUFFER, false);
		glDepthMask(GL_TRUE);
		glBlitFramebuffer(0, 0, graphics->viewportWidth, graphics->viewportHeight, 0, 0, graphics->viewportWidth, graphics->viewportHeight, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
		graphics->getFramebuffer(eFramebuffer_Lighting)->rebindAs(GL_FRAMEBUFFER);
		graphics->getFramebuffer(eFramebuffer_PostProcess)->rebindAs(GL_FRAMEBUFFER);
		graphics->getFramebuffer(eFramebuffer_PostProcess)->bind();
		renderQuad->material = renderQuadMaterial->getId();
		glStencilMask(0x00);
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		renderQuad->render(Scene::getCurrentCamera(), false, eRenderLayer_UI);
		glStencilMask(0xFF);
		glDisable(GL_STENCIL_TEST);
		graphics->getFramebuffer(eFramebuffer_Lighting)->unbind();
		graphics->getFramebuffer(eFramebuffer_PostProcess)->bind();
		Scene::getCurrentScene()->RenderScene(1);
		graphics->getFramebuffer(eFramebuffer_PostProcess)->unbind();
		renderQuad->material = ppQuadMaterial->getId();
		renderQuad->render(Scene::getCurrentCamera(), false, eRenderLayer_UI);
		Scene::getCurrentScene()->RenderScene(2);

		if (Scene::isStudio) {
			EngineGUI::calculateMouseRayDirection();
			EngineGUI::DrawTranslateGizmo();
			EngineGUI::updateGizmoObject();
		}
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		graphics->windowUpdate();
	}


	glfwTerminate();
	TextGeometry::destroyFreeType();
	Scene::getCurrentScene()->destroy();
	return 0;

}