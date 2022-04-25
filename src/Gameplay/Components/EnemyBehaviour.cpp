#include "EnemyBehaviour.h"
#include "HealthManager.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Application/Application.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Components/ProjBehaviour.h"

EnemyBehaviour::EnemyBehaviour()
	: IComponent(),
	invTime(0.0f)
{ }

EnemyBehaviour::~EnemyBehaviour() = default;

void EnemyBehaviour::Awake()
{
}

void EnemyBehaviour::Update(float deltaTime)
{
	Application& app = Application::Get();
	Gameplay::GameObject* camObject = app.CurrentScene()->MainCamera->GetGameObject();

	invTime -= deltaTime;

	throwTime -= deltaTime;

	if (glm::length(camObject->GetPosition() - GetGameObject()->GetPosition()) < 2.5f && invTime <= 0.0f)
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

	if (throwTime <= 0.0f)
	{
		projectile->SetPostion(GetGameObject()->GetPosition());

		int newTime = rand() % 3 + 1;

		projectile->Get<ProjBehaviour>()->SetParams(
			GetGameObject()->GetPosition(), camObject->GetPosition(), newTime
		);

		throwTime = newTime;
	}

}

void EnemyBehaviour::SetProj(Gameplay::GameObject::Sptr object)
{
	projectile = object;
}

void EnemyBehaviour::RenderImGui()
{
}

nlohmann::json EnemyBehaviour::ToJson() const
{
	return nlohmann::json();
}

EnemyBehaviour::Sptr EnemyBehaviour::FromJson(const nlohmann::json & blob)
{
	return EnemyBehaviour::Sptr();
}
