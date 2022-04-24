#include "LerpBehaviour.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"

glm::vec3 Lerp(glm::vec3 point1, glm::vec3 point2, float t)
{
	return glm::vec3(
		(1 - t) * point1.x + t * point2.x,
		(1 - t) * point1.y + t * point2.y,
		(1 - t) * point1.z + t * point2.z
	);
}

void LerpBehaviour::Awake()
{
	_body = GetComponent<Gameplay::Physics::RigidBody>();
	if (_body == nullptr) {
		//IsEnabled = false;
	}
}

void LerpBehaviour::RenderImGui() {
}

nlohmann::json LerpBehaviour::ToJson() const {
	return nlohmann::json();
}

LerpBehaviour::LerpBehaviour() :
	IComponent()
{ }

LerpBehaviour::~LerpBehaviour() = default;

LerpBehaviour::Sptr LerpBehaviour::FromJson(const nlohmann::json & blob) {
	return LerpBehaviour::Sptr();
}

void LerpBehaviour::Update(float deltaTime) {
	if (points.size() >= 2)
	{
		currentTime += deltaTime;
		float t = currentTime / segmentTime;

		if (t >= 1.0f)
		{
			t = 0.0f;

			glm::vec3 p1, p2, p3;
			glm::vec3 origVec, newVec;

			if (forward)
			{
				currentInd++;

				if (currentInd == points.size() - 1)
				{
					forward = false;
					clockwise = !clockwise;
				}

				if (points.size() == 2)
				{
					p1 = points[0];
					p2 = points[1];
					p3 = points[0];
				}

				else
				{
					if (currentInd == points.size() - 1)
					{
						p1 = points[currentInd - 1];
						p2 = points[currentInd];
						p3 = points[currentInd - 1];
					}

					else
					{
						p1 = points[currentInd - 1];
						p2 = points[currentInd];
						p3 = points[currentInd + 1];
					}
				}
			}

			else
			{
				currentInd--;

				if (currentInd == 0)
				{
					forward = true;
					clockwise = !clockwise;
				}

				if (points.size() == 2)
				{
					p1 = points[1];
					p2 = points[0];
					p3 = points[1];
				}

				else
				{
					if (currentInd == 0)
					{
						p1 = points[1];
						p2 = points[0];
						p3 = points[1];
					}

					else
					{
						p1 = points[currentInd + 1];
						p2 = points[currentInd];
						p3 = points[currentInd - 1];
					}
				}
			}

			origVec = p2 - p1;
			newVec = p3 - p2;

			float lengths = (glm::length(origVec) * glm::length(newVec));
			float dots = origVec.x * newVec.x + origVec.y * newVec.y + origVec.z * newVec.z;
			float over = dots / lengths;

			over = std::clamp(over, -1.0f, 1.0f);

			float angle = acos(over);

			currentTime = 0.0f;

			glm::quat angleQuat;

			if (clockwise) angleQuat = glm::angleAxis(angle, glm::vec3(0, 1, 0));

			else angleQuat = glm::angleAxis(angle, glm::vec3(0, -1, 0));

			//else angleQuat = glm::angleAxis(angle, glm::vec3(0, 1, 0));

			GetGameObject()->SetRotation(GetGameObject()->GetRotation() * angleQuat);
		}

		if (forward) GetGameObject()->SetPostion(Lerp(points[currentInd], points[currentInd + 1], t));

		else GetGameObject()->SetPostion(Lerp(points[currentInd], points[currentInd - 1], t));
	}
}

void LerpBehaviour::SetParams(std::vector<glm::vec3> inPoints, float inSegmentTime, bool inForward, bool inClockwise)
{
	points = inPoints;
	segmentTime = inSegmentTime;
	forward = inForward;
	clockwise = inClockwise;
}