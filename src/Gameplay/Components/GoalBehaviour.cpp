#include "GoalBehaviour.h"
#include "HealthManager.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Application/Application.h"

GoalBehaviour::GoalBehaviour()
	: IComponent(),
	wonGame(false)
{ }

GoalBehaviour::~GoalBehaviour() = default;

void GoalBehaviour::Awake()
{
}

void GoalBehaviour::Update(float deltaTime)
{
	Application& app = Application::Get();
	Gameplay::GameObject* camObject = app.CurrentScene()->MainCamera->GetGameObject();

	if (glm::length(camObject->GetPosition() - GetGameObject()->GetPosition()) < 2.0f)
	{
		wonGame = true;
	}

}

void GoalBehaviour::RenderImGui()
{
}

nlohmann::json GoalBehaviour::ToJson() const
{
	return nlohmann::json();
}

GoalBehaviour::Sptr GoalBehaviour::FromJson(const nlohmann::json & blob)
{
	return GoalBehaviour::Sptr();
}

bool GoalBehaviour::GameWon()
{
	return wonGame;
}
