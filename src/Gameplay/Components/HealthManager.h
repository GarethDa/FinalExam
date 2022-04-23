#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Scene.h"

struct GLFWwindow;

class HealthManager :
	public Gameplay::IComponent
{

public:
	typedef std::shared_ptr<HealthManager> Sptr;

	HealthManager();
	virtual ~HealthManager();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

	float GetHealth();
	float GetMaxHealth();

	bool IsDead();

	void TakeHit();

public:
	virtual void RenderImGui() override;

	MAKE_TYPENAME(HealthManager);
	virtual nlohmann::json ToJson() const override;
	static HealthManager::Sptr FromJson(const nlohmann::json& blob);

protected:
	float _healthVal = 2.0f;
	float _maxHealth = 2.0f;
};


