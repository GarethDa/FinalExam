#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"

/// <summary>
/// A simple behaviour that applies an impulse along the Z axis to the 
/// rigidbody of the parent when the space key is pressed
/// </summary>
class ProjBehaviour : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<ProjBehaviour> Sptr;

	std::weak_ptr<Gameplay::IComponent> Panel;

	ProjBehaviour();
	virtual ~ProjBehaviour();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(ProjBehaviour);
	virtual nlohmann::json ToJson() const override;
	static ProjBehaviour::Sptr FromJson(const nlohmann::json& blob);

	void ProjBehaviour::SetParams(glm::vec3 inMinPos, glm::vec3 inMaxPos, float time);

protected:

	float maxTime;
	float timeTaken;
	float invTime;
	glm::vec3 minPos;
	glm::vec3 maxPos;

	Gameplay::Physics::RigidBody::Sptr _body;
};