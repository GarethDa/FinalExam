#include "ProjBehaviour.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"
#include "Application/Application.h"
#include "HealthManager.h"
#include "Gameplay/Components/GUI/GuiPanel.h"

glm::vec3 ProjLerp(glm::vec3 point1, glm::vec3 point2, float t)
{
	return glm::vec3(
		(1 - t) * point1.x + t * point2.x,
		(1 - t) * point1.y + t * point2.y,
		(1 - t) * point1.z + t * point2.z
	);
}

void ProjBehaviour::Awake()
{
	_body = GetComponent<Gameplay::Physics::RigidBody>();
	if (_body == nullptr) {
		//IsEnabled = false;
	}
}

void ProjBehaviour::RenderImGui() {
}

nlohmann::json ProjBehaviour::ToJson() const {
	return nlohmann::json();
}

ProjBehaviour::ProjBehaviour() :
	IComponent(),
	invTime(0.0f)
{ }

ProjBehaviour::~ProjBehaviour() = default;

ProjBehaviour::Sptr ProjBehaviour::FromJson(const nlohmann::json & blob) {
	return ProjBehaviour::Sptr();
}

void ProjBehaviour::Update(float deltaTime) {

	Application& app = Application::Get();
	Gameplay::GameObject* camObject = app.CurrentScene()->MainCamera->GetGameObject();

	timeTaken += deltaTime;
	invTime -= deltaTime;

	if (timeTaken > maxTime)
	{
		timeTaken = maxTime;
	}

	GetGameObject()->SetPostion(ProjLerp(minPos, maxPos, timeTaken / maxTime));

	if (glm::length(camObject->GetPosition() - GetGameObject()->GetPosition()) < 1.5f && invTime <= 0.0f)
	{
		camObject->Get<HealthManager>()->TakeHit();

		if (camObject->Get<HealthManager>()->GetHealth() == 1)
		{
			app.CurrentScene()->FindObjectByName("Heart 2")->Get<GuiPanel>()->SetTransparency(0.0f);
		}

		else
		{
			app.CurrentScene()->FindObjectByName("Heart 1")->Get<GuiPanel>()->SetTransparency(0.0f);
		}

		invTime = 1.5f;
	}
}

void ProjBehaviour::SetParams(glm::vec3 inMinPos, glm::vec3 inMaxPos, float time)
{
	minPos = inMinPos;
	maxPos = inMaxPos;
	maxTime = time;

	timeTaken = 0.0f;
}