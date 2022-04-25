#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/Textures/Texture2DArray.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Components/Light.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/LerpBehaviour.h"
#include "Gameplay/Components/HealthManager.h"
#include "Gameplay/Components/EnemyBehaviour.h"
#include "Gameplay/Components/GoalBehaviour.h"
#include "Gameplay/Components/ProjBehaviour.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"
#include "Application/Layers/ImGuiDebugLayer.h"
#include "Application/Windows/DebugWindow.h"
#include "Gameplay/Components/ShadowCamera.h"
#include "Gameplay/Components/ShipMoveBehaviour.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		 
		// Basic gbuffer generation with no vertex manipulation
		ShaderProgram::Sptr deferredForward = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		deferredForward->SetDebugName("Deferred - GBuffer Generation");  

		// Our foliage shader which manipulates the vertices of the mesh
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});  
		foliageShader->SetDebugName("Foliage");   

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },  
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing"); 

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our cel shading example
		ShaderProgram::Sptr celShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/cel_shader.glsl" }
		});
		celShader->SetDebugName("Cel Shader");


		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr shipMesh   = ResourceManager::CreateAsset<MeshResource>("fenrir.obj");
		MeshResource::Sptr DoorMesh = ResourceManager::CreateAsset<MeshResource>("DoorNew.obj");
		MeshResource::Sptr KnightMesh = ResourceManager::CreateAsset<MeshResource>("Knight.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture   = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec      = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex    = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex      = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    grassTex = ResourceManager::CreateAsset<Texture2D>("textures/grass.png");
		grassTex->SetMinFilter(MinFilter::Nearest);
		grassTex->SetMagFilter(MagFilter::Nearest);

		Texture2D::Sptr    DoorTex = ResourceManager::CreateAsset<Texture2D>("textures/door.png");
		Texture2D::Sptr    KnightTex = ResourceManager::CreateAsset<Texture2D>("textures/knight.png");
		Texture2D::Sptr    redTex = ResourceManager::CreateAsset<Texture2D>("textures/red.png");

		// Load some images for drag n' drop
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight.png");
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight-2.png");
		ResourceManager::CreateAsset<Texture2D>("textures/light_projection.png");

		Texture2DArray::Sptr particleTex = ResourceManager::CreateAsset<Texture2DArray>("textures/particles3.png", 2, 2);

		//DebugWindow::Sptr debugWindow = app.GetLayer<ImGuiDebugLayer>()->GetWindow<DebugWindow>();

#pragma region Basic Texture Creation
		Texture2DDescription singlePixelDescriptor;
		singlePixelDescriptor.Width = singlePixelDescriptor.Height = 1;
		singlePixelDescriptor.Format = InternalFormat::RGB8;

		float normalMapDefaultData[3] = { 0.5f, 0.5f, 1.0f };
		Texture2D::Sptr normalMapDefault = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		normalMapDefault->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, normalMapDefaultData);

		float solidGrey[3] = { 0.5f, 0.5f, 0.5f };
		float solidBlack[3] = { 0.0f, 0.0f, 0.0f };
		float solidWhite[3] = { 1.0f, 1.0f, 1.0f };

		Texture2D::Sptr solidBlackTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidBlackTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidBlack);

		Texture2D::Sptr solidGreyTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidGreyTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidGrey);

		Texture2D::Sptr solidWhiteTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidWhiteTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidWhite);

#pragma endregion 

		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" } 
		});
		  
		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();  

		scene->SetAmbientLight({ 0.52, 0.75, 0.97 });

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");   

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.AlbedoMap", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
			boxMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			grassMaterial->Name = "Grass";
			grassMaterial->Set("u_Material.AlbedoMap", grassTex);
			grassMaterial->Set("u_Material.Shininess", 0.1f);
			grassMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr redMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			redMaterial->Name = "Red";
			redMaterial->Set("u_Material.AlbedoMap", redTex);
			redMaterial->Set("u_Material.Shininess", 0.1f);
			redMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr DoorMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			DoorMaterial->Name = "Door";
			DoorMaterial->Set("u_Material.AlbedoMap", DoorTex);
			DoorMaterial->Set("u_Material.NormalMap", normalMapDefault);
			DoorMaterial->Set("u_Material.Shininess", 0.5f);
		}

		Material::Sptr KnightMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			KnightMaterial->Name = "Knight";
			KnightMaterial->Set("u_Material.AlbedoMap", KnightTex);
			KnightMaterial->Set("u_Material.NormalMap", normalMapDefault);
			KnightMaterial->Set("u_Material.Shininess", 0.7f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.AlbedoMap", monkeyTex);
			monkeyMaterial->Set("u_Material.NormalMap", normalMapDefault);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 50% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(deferredForward); 
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.AlbedoMap", boxTexture); 
			testMaterial->Set("u_Material.Specular", boxSpec);
			testMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr grey = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			grey->Name = "Grey";
			grey->Set("u_Material.AlbedoMap", solidGreyTex);
			grey->Set("u_Material.Specular", solidBlackTex);
			grey->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.AlbedoMap", diffuseMap);
			normalmapMat->Set("u_Material.NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		// Create some lights for our scene
		/*
		GameObject::Sptr lightParent = scene->CreateGameObject("Lights");

		for (int ix = 0; ix < 50; ix++) {
			GameObject::Sptr light = scene->CreateGameObject("Light");
			light->SetPostion(glm::vec3(glm::diskRand(25.0f), 1.0f));
			lightParent->AddChild(light);

			Light::Sptr lightComponent = light->Add<Light>();
			lightComponent->SetColor(glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f)));
			lightComponent->SetRadius(glm::linearRand(0.1f, 10.0f));
			lightComponent->SetIntensity(glm::linearRand(1.0f, 2.0f));
		}
		*/
		/*
		GameObject::Sptr light = scene->CreateGameObject("Light");
		{
			light->SetPostion(glm::vec3(0.0f, 0.0f, 5.0f));

			Light::Sptr lightComponent = light->Add<Light>();
			lightComponent->SetRadius(100.0f);
			lightComponent->SetIntensity(5.0f);
		}
		*/

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ -9, 2, 2 });
			camera->LookAt(glm::vec3(0.0f));

			camera->Add<SimpleCameraControl>();

			camera->Add<HealthManager>();

			RenderComponent::Sptr renderer = camera->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			
			RigidBody::Sptr physics = camera->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(SphereCollider::Create(1.5f));
			physics->SetMass(0.0000001f);
			physics->SetAngularFactor({ 0, 0, 0 });

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}

		GameObject::Sptr walls = scene->CreateGameObject("Walls");
		{
			BoxCollider::Sptr collider = BoxCollider::Create({ 30.0f, 10.0f, 1.0f });
			collider->SetRotation({ 90.0f, 0.0f, 0.0f });
			collider->SetPosition({ 10.0f, -13.0f, 1.0f });

			BoxCollider::Sptr collider2 = BoxCollider::Create({ 30.0f, 10.0f, 1.0f });
			collider2->SetRotation({ 90.0f, 0.0f, 0.0f });
			collider2->SetPosition({ 10.0f, 13.0f, 1.0f });

			BoxCollider::Sptr collider3 = BoxCollider::Create({ 20.0f, 10.0f, 1.0f });
			collider3->SetRotation({ 90.0f, 0.0f, 90.0f });
			collider3->SetPosition({ -14.0f, 0.0f, 1.0f });

			BoxCollider::Sptr collider4 = BoxCollider::Create({ 20.0f, 10.0f, 1.0f });
			collider4->SetRotation({ 90.0f, 0.0f, 90.0f });
			collider4->SetPosition({ 39.0f, 0.0f, 1.0f });

			RigidBody::Sptr physics = walls->Add<RigidBody>();
			physics->AddCollider(collider);
			physics->AddCollider(collider2);
			physics->AddCollider(collider3);
			physics->AddCollider(collider4);
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			plane->SetScale({ 0.25f, 0.25f, 1.0f });

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}

		// Set up all our sample objects
		GameObject::Sptr plane2 = scene->CreateGameObject("Plane2");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			plane2->SetScale({ 0.25f, 0.25f, 1.0f });
			plane2->SetPostion({ 25.0f, 0.0f, 0.0f });

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane2->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane2->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}

		// Set up all our sample objects
		GameObject::Sptr goal = scene->CreateGameObject("Goal");
		{
			goal->SetPostion({ 34.0f, 0.0f, 0.0f });
			goal->SetRotation({ 90.0f, 0.0f, -90.0f });
			goal->SetScale({ 1.5f, 1.5f, 1.5f });

			goal->Add<GoalBehaviour>();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = goal->Add<RenderComponent>();
			renderer->SetMesh(DoorMesh);
			renderer->SetMaterial(DoorMaterial);

			GameObject::Sptr particles = scene->CreateGameObject("Particles");
			goal->AddChild(particles);
			particles->SetPostion({ 0.0f, 0.0f, 0.0f });

			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			particleManager->_gravity = glm::vec3(0.0f);

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 4;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.93f, 1.0f, 0.01f, 1.0f);
			emitter.Lifetime = 2.0f / 50.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 50.0f;
			emitter.SphereEmitterData.Velocity = 0.8f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 3.0f };
			emitter.SphereEmitterData.Radius = 0.8f;
			emitter.SphereEmitterData.SizeRange = { 0.3f, 1.0f };

			particleManager->AddEmitter(emitter);
		}

		GameObject::Sptr projectile = scene->CreateGameObject("Projectile1");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreateIcoSphere({ 0.0f, 0.0f, 0.0f }, 2));
			tiledMesh->GenerateMesh();

			projectile->SetScale({ 0.15f, 0.15f, 0.15f });
			projectile->SetPostion({ 25.0f, 0.0f, 0.0f });

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = projectile->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(redMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = projectile->Add<RigidBody>(RigidBodyType::Kinematic);

			projectile->Add<ProjBehaviour>()->SetParams({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 1.0f);
		}

		GameObject::Sptr car1 = scene->CreateGameObject("Knight 1");
		{
			// Set position in the scene
			car1->SetScale(glm::vec3(2.5f, 2.5f, 2.5f));
			car1->SetPostion(glm::vec3(1.5f, 0.0f, 0.0f));
			car1->SetRotation(glm::vec3(90.0f, 0.0f, 175.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = car1->Add<RenderComponent>();
			renderer->SetMesh(KnightMesh);
			renderer->SetMaterial(KnightMaterial);

			std::vector<glm::vec3> points{ glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(2.8f, 12.8f, 0.0f), 
				glm::vec3(-1.5f, -11.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f) };

			car1->Add<LerpBehaviour>()->SetParams(points, 2.0f);

			car1->Add<EnemyBehaviour>()->SetProj(projectile);

			GameObject::Sptr particles = scene->CreateGameObject("Particles");
			car1->AddChild(particles);
			particles->SetPostion({ 0.0f, 0.0f, 0.0f });

			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			particleManager->_gravity = glm::vec3(0.0f);

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.6f, 0.43f, 0.17f, 1.0f);
			emitter.Lifetime = 1.0f / 50.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 50.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 3.0f };
			emitter.SphereEmitterData.Radius = 0.5f;
			emitter.SphereEmitterData.SizeRange = { 0.5f, 1.0f };

			particleManager->AddEmitter(emitter);
		}

		GameObject::Sptr projectile2 = scene->CreateGameObject("Projectile2");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreateIcoSphere({ 0.0f, 0.0f, 0.0f }, 2));
			tiledMesh->GenerateMesh();

			projectile2->SetScale({ 0.15f, 0.15f, 0.15f });
			projectile2->SetPostion({ 25.0f, 0.0f, 0.0f });

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = projectile2->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(redMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = projectile2->Add<RigidBody>(RigidBodyType::Kinematic);

			projectile2->Add<ProjBehaviour>()->SetParams({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 1.0f);
		}

		GameObject::Sptr car2 = scene->CreateGameObject("Knight 2");
		{
			// Set position in the scene
			car2->SetScale(glm::vec3(2.5f, 2.5f, 2.5f));
			car2->SetPostion(glm::vec3(10.0f, 11.0f, 0.0f));
			car2->SetRotation(glm::vec3(90.0f, 0.0f, 50.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = car2->Add<RenderComponent>();
			renderer->SetMesh(KnightMesh);
			renderer->SetMaterial(KnightMaterial);

			std::vector<glm::vec3> points{ glm::vec3(10.0f, 11.0f, 0.0f), glm::vec3(16.0f, 5.0f, 0.0f), 
				glm::vec3(11.0f, -10.5f, 0.0f), glm::vec3(2.5f, -5.0f, 0.0f) };

			car2->Add<LerpBehaviour>()->SetParams(points, 1.5f, true, false);

			car2->Add<EnemyBehaviour>()->SetProj(projectile2);

			GameObject::Sptr particles = scene->CreateGameObject("Particles");
			car2->AddChild(particles);
			particles->SetPostion({ 0.0f, 0.0f, 0.0f });

			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			particleManager->_gravity = glm::vec3(0.0f);

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.6f, 0.43f, 0.17f, 1.0f);
			emitter.Lifetime = 1.0f / 50.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 50.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 3.0f };
			emitter.SphereEmitterData.Radius = 0.5f;
			emitter.SphereEmitterData.SizeRange = { 0.5f, 1.0f };

			particleManager->AddEmitter(emitter);
		}

		/*
		GameObject::Sptr car3 = scene->CreateGameObject("Car 3");
		{
			// Set position in the scene
			car3->SetScale(glm::vec3(2.5f, 2.5f, 2.5f));
			car3->SetPostion(glm::vec3(20.0f, 11.0f, 0.0f));
			car3->SetRotation(glm::vec3(90.0f, 0.0f, 30.0f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = car3->Add<RenderComponent>();
			renderer->SetMesh(CarMesh);
			renderer->SetMaterial(CarMaterial);

			std::vector<glm::vec3> points{ glm::vec3(20.0f, 11.0f, 0.0f), glm::vec3(25.0f, 0.0f, 0.0f), 
				glm::vec3(20.0f, -11.0f, 0.0f), glm::vec3(17.0f, 0.0f, 0.0f) };

			car3->Add<LerpBehaviour>()->SetParams(points, 1.0f, true, false);

			car3->Add<EnemyBehaviour>();

			GameObject::Sptr particles = scene->CreateGameObject("Particles");
			car3->AddChild(particles);
			particles->SetPostion({ 0.0f, 0.0f, 0.0f });

			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			particleManager->_gravity = glm::vec3(0.0f);

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.6f, 0.43f, 0.17f, 1.0f);
			emitter.Lifetime = 1.0f / 50.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 50.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 3.0f };
			emitter.SphereEmitterData.Radius = 0.5f;
			emitter.SphereEmitterData.SizeRange = { 0.5f, 1.0f };

			particleManager->AddEmitter(emitter);
		}
		*/
		
		GameObject::Sptr shadowCaster = scene->CreateGameObject("Shadow Light");
		{
			// Set position in the scene
			shadowCaster->SetPostion(glm::vec3(-3.0f, -7.0f, 6.4f));
			shadowCaster->LookAt(glm::vec3(0.0f));
			shadowCaster->SetRotation({ 36.0f, -6.0f, -61.0f });
			shadowCaster->SetScale({ 0.8f, 1.0f, 1.0f });

			// Create and attach a renderer for the monkey
			ShadowCamera::Sptr shadowCam = shadowCaster->Add<ShadowCamera>();
			shadowCam->SetProjection(glm::perspective(glm::radians(120.0f), 1.0f, 0.1f, 100.0f));

			shadowCam->Bias = 0.0002f;
			shadowCam->NormalBias = 0.0f;
			shadowCam->SetBufferResolution({ 1080, 1080 });
			shadowCam->Intensity = 6.0f;
			shadowCam->Range = 150.0f;
		}


		/////////////////////////// UI //////////////////////////////
		
		GameObject::Sptr winText = scene->CreateGameObject("Win Text"); 
		{
			RectTransform::Sptr transform = winText->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2 - app.GetWindowSize().x / 3, app.GetWindowSize().y / 2 - app.GetWindowSize().x / 4 });
			transform->SetMax({ app.GetWindowSize().x / 2 + app.GetWindowSize().x / 3, app.GetWindowSize().y / 2 + app.GetWindowSize().x / 4 });

			GuiPanel::Sptr canPanel = winText->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/winText.png"));
			canPanel->SetTransparency(0.0f);
		}

		GameObject::Sptr loseText = scene->CreateGameObject("Lose Text");
		{
			RectTransform::Sptr transform = loseText->Add<RectTransform>();
			transform->SetMin({ app.GetWindowSize().x / 2 - app.GetWindowSize().x / 3, app.GetWindowSize().y / 2 - app.GetWindowSize().x / 4 });
			transform->SetMax({ app.GetWindowSize().x / 2 + app.GetWindowSize().x / 3, app.GetWindowSize().y / 2 + app.GetWindowSize().x / 4 });

			GuiPanel::Sptr canPanel = loseText->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/loseText.png"));
			canPanel->SetTransparency(0.0f);

		}

		GameObject::Sptr heart1 = scene->CreateGameObject("Heart 1");
		{
			RectTransform::Sptr transform = heart1->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ 100, 100 });

			GuiPanel::Sptr canPanel = heart1->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/Heart.png"));
			canPanel->SetTransparency(1.0f);
		}

		GameObject::Sptr heart2 = scene->CreateGameObject("Heart 2");
		{
			RectTransform::Sptr transform = heart2->Add<RectTransform>();
			transform->SetMin({ 100, 0 });
			transform->SetMax({ 200, 100 });

			GuiPanel::Sptr canPanel = heart2->Add<GuiPanel>();
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/Heart.png"));
			canPanel->SetTransparency(1.0f);
		}

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
