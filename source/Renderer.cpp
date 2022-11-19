//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	m_pDepthBufferPixels = new float[m_Width * m_Height] {INFINITY};
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	const int pixelCount{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, pixelCount, INFINITY);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	std::vector<Vertex> vertices_world
	{
		//Triangle 0
		{ {0.f,2.f,0.f},    {1,0,0} },
		{ {1.5f,-1.f,0.f},  {1,0,0} },
		{ {-1.5f,-1.f,0.f}, {1,0,0} },

		//Triangle 1
		{ {0.f,4.f,2.f},   {1,0,0} },
		{ {3.f,-2.f,2.f},  {0,1,0} },
		{ {-3.f,-2.f,2.f}, {0,0,1} }
	};

	VertexTransformationFunction(vertices_world, vertices_world);

	//RENDER LOGIC
	for (size_t trIndex = 0; trIndex < vertices_world.size(); trIndex+=3)
	{
		Vector2 v0{ vertices_world[trIndex].position.GetXY()};
		Vector2 v1{ vertices_world[trIndex + 1].position.GetXY() };
		Vector2 v2{ vertices_world[trIndex + 2].position.GetXY() };

		Vector2 topLeft{};
		topLeft.x = std::min(std::min(v0.x, v1.x), v2.x);
		topLeft.y = std::min(std::min(v0.y, v1.y), v2.y);

		Vector2 bottomRight{};
		bottomRight.x = std::max(std::max(v0.x, v1.x), v2.x);
		bottomRight.y = std::max(std::max(v0.y, v1.y), v2.y);


		Vector3 weight{};
		for (int px{}; px < m_Width; ++px)
		{
			if (px < topLeft.x || px > bottomRight.x)
				continue;

			for (int py{}; py < m_Height; ++py)
			{			
				if (py < topLeft.y || py > bottomRight.y)
					continue;

				if (Utils::HitTest_Triangle(Vector2{ static_cast<float>(px), static_cast<float>(py) }, v0,v1,v2, weight))
				{
					float currentDepth = vertices_world[trIndex].position.z * weight.x + vertices_world[trIndex].position.z * weight.y + vertices_world[trIndex].position.z * weight.z;
					if (currentDepth < m_pDepthBufferPixels[py * m_Width + px ])
					{
						m_pDepthBufferPixels[py * m_Width + px ] = currentDepth;
						ColorRGB finalColor = vertices_world[trIndex].color * weight.x + vertices_world[trIndex + 1].color * weight.y + vertices_world[trIndex+2].color * weight.z;

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}	
			}
		}
	}

	

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	float aspectRatio{ static_cast<float>(m_Width) / m_Height };

	for (size_t i = 0; i < vertices_in.size(); i++)
	{
		vertices_out[i] = vertices_in[i];

		vertices_out[i].position = m_Camera.viewMatrix.TransformPoint(vertices_out[i].position);

		//Perspective Divide
		vertices_out[i].position.x = vertices_out[i].position.x / vertices_out[i].position.z;
		vertices_out[i].position.y = vertices_out[i].position.y / vertices_out[i].position.z;

		//FOV + AR
		vertices_out[i].position.x = vertices_out[i].position.x / (m_Camera.fov * aspectRatio);
		vertices_out[i].position.y = vertices_out[i].position.y / (m_Camera.fov);


		vertices_out[i].position.x = (vertices_out[i].position.x + 1) / 2 * m_Width;
		vertices_out[i].position.y = (1 - vertices_out[i].position.y) / 2 * m_Height;
	}

}

bool Renderer::SaveBufferToImage() const
{

	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
