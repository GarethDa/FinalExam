#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Scene.h"

class EnemyBehaviour :
    public Gameplay::IComponent
{
public:
	typedef std::shared_ptr<EnemyBehaviour> Sptr;

	EnemyBehaviour();
	virtual ~EnemyBehaviour();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

	void SetProj(Gameplay::GameObject::Sptr object);

public:
	virtual void RenderImGui() override;

	MAKE_TYPENAME(EnemyBehaviour);
	virtual nlohmann::json ToJson() const override;
	static EnemyBehaviour::Sptr FromJson(const nlohmann::json& blob);

protected:
	float invTime;
	float throwTime;

	Gameplay::GameObject::Sptr projectile;
};

