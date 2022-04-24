#include "Gameplay/Components/SimpleCameraControl.h"
#include <GLFW/glfw3.h>
#define  GLM_SWIZZLE
#include <GLM/gtc/quaternion.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"
#include "Application/Application.h"
#include "Application/Timing.h"

SimpleCameraControl::SimpleCameraControl() :
	IComponent(),
	_mouseSensitivity({ 0.3f, 0.3f }),
	_moveSpeeds(glm::vec3(0.1f)),
	_shiftMultipler(2.0f),
	_currentRot(glm::vec2(0.0f)),
	_isMousePressed(false),
	_moveSpeed(0.15f),
	_firstFrame(true)
{ }

SimpleCameraControl::~SimpleCameraControl() = default;

void SimpleCameraControl::Update(float deltaTime)
{
	if (Application::Get().IsFocused) {

		if (InputEngine::GetMouseState(GLFW_MOUSE_BUTTON_LEFT) == ButtonState::Pressed || 
			InputEngine::GetKeyState(GLFW_KEY_ENTER) == ButtonState::Pressed) {
			_prevMousePos = InputEngine::GetMousePos();
			LOG_INFO("doot");


			if (controlWithMouse)
			{
				glfwSetInputMode(Application::Get().GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				controlWithMouse = false;
			}

			else
			{
				glfwSetInputMode(Application::Get().GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				controlWithMouse = true;
			}

		}

		/*
		if (_firstFrame)
		{
			_prevMousePos = InputEngine::GetMousePos();
			glfwSetInputMode(Application::Get().GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			LOG_INFO("doot");
		}
		*/

		if (controlWithMouse) {
			//if (InputEngine::IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)){
			glm::dvec2 currentMousePos = InputEngine::GetMousePos();
			glm::dvec2 delta = currentMousePos - _prevMousePos;

			Timing& timing = Timing::Current();

			if (timing.TimeScale() != 0.0f)
			{
				_currentRot.x -= static_cast<float>(delta.x) * _mouseSensitivity.x;
				_currentRot.y -= static_cast<float>(delta.y) * _mouseSensitivity.y;
			}

			_currentRot.y = std::clamp(_currentRot.y, 1.0f, 179.0f);

			glm::quat rotX = glm::angleAxis(glm::radians(_currentRot.x), glm::vec3(0, 0, 1));
			glm::quat rotY = glm::angleAxis(glm::radians(_currentRot.y), glm::vec3(1, 0, 0));
			glm::quat currentRot = rotX * rotY;
			GetGameObject()->SetRotation(currentRot);

			_prevMousePos = currentMousePos;

			glm::vec3 input = glm::vec3(0.0f);
			if (InputEngine::IsKeyDown(GLFW_KEY_W)) {
				input.z -= _moveSpeeds.x;
			}
			if (InputEngine::IsKeyDown(GLFW_KEY_S)) {
				input.z += _moveSpeeds.x;
			}
			if (InputEngine::IsKeyDown(GLFW_KEY_A)) {
				input.x -= _moveSpeeds.y;
			}
			if (InputEngine::IsKeyDown(GLFW_KEY_D)) {
				input.x += _moveSpeeds.y;
			}

			if (InputEngine::IsKeyDown(GLFW_KEY_LEFT_SHIFT)) {
				input *= _shiftMultipler;
			}

			input *= deltaTime;

			//glm::vec3 worldMovement = currentRot * glm::vec4(input, 1.0f);
			glm::vec3 worldMovement = glm::vec3((currentRot * glm::vec4(input, 1.0f)).x, (currentRot * glm::vec4(input, 1.0f)).y, 0.0f);

			if (worldMovement != glm::vec3(0.0f))
			{
				worldMovement = _moveSpeed * glm::normalize(worldMovement);
			}

			GetGameObject()->SetPostion(GetGameObject()->GetPosition() + worldMovement);
		}

		if (score <= 0)
		{
			GetGameObject()->SetPostion(glm::vec3(1000, 100, 100));
			std::cout << "\n\n\nYOU WIN!!!!!!";
		}
	}
	_prevMousePos = InputEngine::GetMousePos();
}

void SimpleCameraControl::RenderImGui()
{
	LABEL_LEFT(ImGui::DragFloat2, "Mouse Sensitivity", &_mouseSensitivity.x, 0.01f);
	//LABEL_LEFT(ImGui::DragFloat3, "Move Speed       ", &_moveSpeed, 0.01f, 0.01f);
	LABEL_LEFT(ImGui::DragFloat, "Shift Multiplier ", &_shiftMultipler, 0.01f, 1.0f);
}

nlohmann::json SimpleCameraControl::ToJson() const {
	return {
		{ "mouse_sensitivity", _mouseSensitivity },
		{ "move_speed", _moveSpeeds },
		{ "shift_mult", _shiftMultipler }
	};
}

SimpleCameraControl::Sptr SimpleCameraControl::FromJson(const nlohmann::json & blob) {
	SimpleCameraControl::Sptr result = std::make_shared<SimpleCameraControl>();
	result->_mouseSensitivity = JsonGet(blob, "mouse_sensitivity", result->_mouseSensitivity);
	result->_moveSpeeds = JsonGet(blob, "move_speed", result->_moveSpeeds);
	result->_shiftMultipler = JsonGet(blob, "shift_mult", 2.0f);
	return result;
}
