#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float aspectRatio;

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		float far{ 100.f };
		float near{ .1f };

		void Initialize(float ar, float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);
			aspectRatio = ar;
			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			Matrix rot{ Matrix::CreateRotation(Vector3(totalPitch, totalYaw,0)) };
			forward = rot.TransformVector(Vector3::UnitZ);
			forward.Normalize();

			viewMatrix = Matrix::CreateLookAtLH(origin, forward, Vector3::UnitY);

			invViewMatrix = Matrix::Inverse(viewMatrix);

			right = invViewMatrix.GetAxisX();
			up = invViewMatrix.GetAxisY();
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, near, far);
			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();
			const float moveSpeed{ 17.f };
			const float rotSpeed{ 4.5f };

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			if (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP])
			{
				origin += forward * moveSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN])
			{
				origin -= forward * moveSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT])
			{
				origin -= right * moveSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT])
			{
				origin += right * moveSpeed * deltaTime;
			}



			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);


			if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT) && mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				if (mouseY > 0)
				{
					origin -= up * moveSpeed * deltaTime;
				}
				if (mouseY < 0)
				{
					origin += up * moveSpeed * deltaTime;
				}
				if (mouseX > 0)
				{
					origin += right * moveSpeed * deltaTime;
				}
				if (mouseX < 0)
				{
					origin -= right * moveSpeed * deltaTime;
				}

			}
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				if (mouseY > 0)
				{
					origin -= forward * moveSpeed * deltaTime;
				}
				if (mouseY < 0)
				{
					origin += forward * moveSpeed * deltaTime;
				}
				if (mouseX < 0)
				{
					totalYaw -= rotSpeed * deltaTime;
				}
				if (mouseX > 0)
				{
					totalYaw += rotSpeed * deltaTime;
				}
			}
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				if (mouseX < 0)
				{
					totalYaw -= rotSpeed * deltaTime;

				}
				if (mouseX > 0)
				{
					totalYaw += rotSpeed * deltaTime;

				}
				if (mouseY > 0)
				{
					totalPitch -= rotSpeed * deltaTime;

				}
				if (mouseY < 0)
				{
					totalPitch += rotSpeed * deltaTime;

				}
			}
		

			//Camera Update Logic
			//...

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
