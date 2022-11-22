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

void Renderer::Render_Week1()
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
					float currentDepth = vertices_world[trIndex].position.z * weight.x + vertices_world[trIndex+1].position.z * weight.y + vertices_world[trIndex+2].position.z * weight.z;
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

void Renderer::Render_Week2()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	const int pixelCount{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, pixelCount, INFINITY);

	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	std::vector<Mesh> meshes_World
	{
		Mesh
		{
			{
				Vertex{{-3.f,3.f,-2.f},colors::White,{0,0}},
				Vertex{{0.f,3.f,-2.f},colors::White,{0.5f,0}},
				Vertex{{3.f,3.f,-2.f},colors::White,{1.f,0}},
				Vertex{{-3.f,0.f,-2.f},colors::White,{0.f,0.5f}},
				Vertex{{0.f,0.f,-2.f},colors::White,{0.5f,0.5f}},
				Vertex{{3.f,0.f,-2.f},colors::White,{1.f,0.5f}},
				Vertex{{-3.f,-3.f,-2.f},colors::White,{0.f,1.f}},
				Vertex{{0.f,-3.f,-2.f},colors::White,{0.5f,1.f}},
				Vertex{{3.f,-3.f,-2.f},colors::White,{1.f,1.f}}
			},
			{
				{
					3,0,4,1,5,2,
					2,6,
					6,3,7,4,8,5
				}
			},
			PrimitiveTopology::TriangleStrip
		}

		/*Mesh
		{
			{
				Vertex{{-3.f,3.f,-2.f}},
				Vertex{{0.f,3.f,-2.f}},
				Vertex{{3.f,3.f,-2.f}},
				Vertex{{-3.f,0.f,-2.f}},
				Vertex{{0.f,0.f,-2.f}},
				Vertex{{3.f,0.f,-2.f}},
				Vertex{{-3.f,-3.f,-2.f}},
				Vertex{{0.f,-3.f,-2.f}},
				Vertex{{3.f,-3.f,-2.f}}
			},
			{
				{
					3,0,1,	1,4,3,	4,1,2,
					2,5,4,	6,3,4,	4,7,6,
					7,4,5,	5,8,7
				}
			},
			PrimitiveTopology::TriangleList
		}*/
	};


	VertexTransformationFunction(meshes_World);

	for (size_t i = 0; i < meshes_World.size(); i++)
	{
		if (meshes_World[i].primitiveTopology == PrimitiveTopology::TriangleList)
		{
			//Loop over pixel gets called from this function
			RenderTriangleList(meshes_World[i]);
		}
		else if (meshes_World[i].primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			//Loop over pixel gets called from this function
			RenderTriangleStrip(meshes_World[i]);
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

void Renderer::VertexTransformationFunction( std::vector<Mesh>& meshes) const
{
	float aspectRatio{ static_cast<float>(m_Width) / m_Height };
	
	for (size_t currentMesh = 0; currentMesh < meshes.size(); currentMesh++)
	{
		meshes[currentMesh].vertices_out.reserve(meshes[currentMesh].vertices.size());

		for (size_t i = 0; i < meshes[currentMesh].vertices.size(); i++)
		{
			Vertex_Out& currentVert = meshes[currentMesh].vertices_out[i];

			currentVert.color = meshes[currentMesh].vertices[i].color;
			currentVert.position.x = meshes[currentMesh].vertices[i].position.x;
			currentVert.position.y = meshes[currentMesh].vertices[i].position.y;
			currentVert.position.z = meshes[currentMesh].vertices[i].position.z;
			currentVert.uv = meshes[currentMesh].vertices[i].uv;

			currentVert.position = m_Camera.viewMatrix.TransformPoint(currentVert.position);

			//Perspective Divide
			currentVert.position.x = currentVert.position.x / currentVert.position.z;
			currentVert.position.y = currentVert.position.y / currentVert.position.z;

			//FOV + AR
			currentVert.position.x = currentVert.position.x / (m_Camera.fov * aspectRatio);
			currentVert.position.y = currentVert.position.y / (m_Camera.fov);


			currentVert.position.x = (currentVert.position.x + 1) / 2 * m_Width;
			currentVert.position.y = (1 - currentVert.position.y) / 2 * m_Height;
		}
	}

}

void Renderer::RenderTriangleList(const Mesh& currentMesh)
{
	std::vector<Vertex_Out> vertices{};
	vertices.reserve(3);
	for (size_t idx = 0; idx < currentMesh.indices.size(); idx+=3)
	{
		vertices[0] = currentMesh.vertices_out[currentMesh.indices[idx]];
		vertices[1] = currentMesh.vertices_out[currentMesh.indices[idx + 1]];
		vertices[2] = currentMesh.vertices_out[currentMesh.indices[idx + 2]];

		LoopOverPixels(vertices);
	}
}

void Renderer::RenderTriangleStrip(const Mesh& currentMesh)
{
	std::vector<Vertex_Out> vertices{};
	vertices.reserve(3);
	for (size_t idx = 0; idx < currentMesh.indices.size() - 2; ++idx)
	{
		if (idx % 2 == 0)
		{
			vertices[0] = currentMesh.vertices_out[currentMesh.indices[idx]];
			vertices[1] = currentMesh.vertices_out[currentMesh.indices[idx + 1]];
			vertices[2] = currentMesh.vertices_out[currentMesh.indices[idx + 2]];
		}
		else
		{

			vertices[0] = currentMesh.vertices_out[currentMesh.indices[idx]];
			vertices[1] = currentMesh.vertices_out[currentMesh.indices[idx + 2]];
			vertices[2] = currentMesh.vertices_out[currentMesh.indices[idx + 1]];
		}

		LoopOverPixels(vertices);
	}


}

void Renderer::LoopOverPixels(const std::vector<Vertex_Out>& vertices)
{
	Vector2 v0 = vertices[0].position.GetXY();
	Vector2 v1 = vertices[1].position.GetXY();
	Vector2 v2 = vertices[2].position.GetXY();

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

			if (Utils::HitTest_Triangle(Vector2{ static_cast<float>(px), static_cast<float>(py) }, v0, v1, v2, weight))
			{
				float currentDepth = vertices[0].position.z * weight.x + vertices[1].position.z * weight.y + vertices[2].position.z * weight.z;
				if (currentDepth < m_pDepthBufferPixels[py * m_Width + px])
				{
					m_pDepthBufferPixels[py * m_Width + px] = currentDepth;
					ColorRGB finalColor = vertices[0].color * weight.x + vertices[1].color * weight.y + vertices[2].color * weight.z;

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


bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
