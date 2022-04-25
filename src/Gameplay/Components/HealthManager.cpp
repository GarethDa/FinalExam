#include "HealthManager.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"

HealthManager::HealthManager()
	: IComponent()
{ }

HealthManager::~HealthManager() = default;

void HealthManager::Awake()
{

}

void HealthManager::Update(float deltaTime)
{
}

float HealthManager::GetHealth()
{
	return _healthVal;
}

float HealthManager::GetMaxHealth()
{
	return _maxHealth;
}

void HealthManager::TakeHit()
{
	_healthVal--;
	
	std::cout << "Health: " << _healthVal << '\n';
}

bool HealthManager::IsDead()
{
 	return _healthVal <= 0.0f;
}

void HealthManager::RenderImGui()
{
}

nlohmann::json HealthManager::ToJson() const
{
	return nlohmann::json();
}

HealthManager::Sptr HealthManager::FromJson(const nlohmann::json & blob)
{
	return HealthManager::Sptr();
}
