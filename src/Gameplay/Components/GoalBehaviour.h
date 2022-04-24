#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Scene.h"

class GoalBehaviour :
    public Gameplay::IComponent
{
public:
	typedef std::shared_ptr<GoalBehaviour> Sptr;

	GoalBehaviour();
	virtual ~GoalBehaviour();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

	bool GameWon();

public:
	virtual void RenderImGui() override;

	MAKE_TYPENAME(GoalBehaviour);
	virtual nlohmann::json ToJson() const override;
	static GoalBehaviour::Sptr FromJson(const nlohmann::json& blob);

protected:
	bool wonGame;

	Gameplay::GameObject::Sptr _player;
};

