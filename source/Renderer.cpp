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


	//Initialize Camera
	m_Camera.Initialize(static_cast<float>(m_Width) / m_Height,60.f, { .0f,.0f,-30.f });

	//Initialize depthBuffer
	m_pDepthBufferPixels = new float[m_Width * m_Height] {INFINITY};

	//Load in textures
	m_pTextureGrid = Texture::LoadFromFile("Resources/uv_grid_2.png");
	m_pTuktukTexture = Texture::LoadFromFile("Resources/tuktuk.png");


	m_MeshesWorld.emplace_back(Mesh{});

	Utils::ParseOBJ("Resources/tuktuk.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices);
	m_MeshesWorld[0].primitiveTopology = PrimitiveTopology::TriangleList;
	m_MeshesWorld[0].vertices_out.reserve(m_MeshesWorld[0].vertices.size());

	for (size_t i = 0; i < m_MeshesWorld[0].vertices_out.capacity(); i++)
	{
		m_MeshesWorld[0].vertices_out.emplace_back(Vertex_Out{});
	}

}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTextureGrid;
	delete m_pTuktukTexture;
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
	

	for (size_t i = 0; i < m_MeshesWorld.size(); i++)
	{
		VertexTransformationFunction(m_MeshesWorld[i]);
		if (m_MeshesWorld[i].primitiveTopology == PrimitiveTopology::TriangleList)
		{
			//Loop over pixel gets called from this function
			RenderTriangleList(m_MeshesWorld[i]);
		}
		else if (m_MeshesWorld[i].primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			//Loop over pixel gets called from this function
			RenderTriangleStrip(m_MeshesWorld[i]);
		}

	}

	
	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::ToggleRenderMode()
{
	if (m_CurrentRenderMode == RenderMode::FinalColor)
	{
		m_CurrentRenderMode = RenderMode::DepthBuffer;
		return;
	}

	m_CurrentRenderMode = RenderMode::FinalColor;
		
		
	
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

void Renderer::VertexTransformationFunction( Mesh& mesh) const
{
	float aspectRatio{ static_cast<float>(m_Width) / m_Height };
	Matrix worldViewProjection{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

	for (size_t i = 0; i < mesh.vertices.size(); i++)
	{
		mesh.vertices_out[i].color = mesh.vertices[i].color;
		mesh.vertices_out[i].position.x = mesh.vertices[i].position.x;
		mesh.vertices_out[i].position.y = mesh.vertices[i].position.y;
		mesh.vertices_out[i].position.z = mesh.vertices[i].position.z;
		mesh.vertices_out[i].uv = mesh.vertices[i].uv;

		mesh.vertices_out[i].position = worldViewProjection.TransformPoint(mesh.vertices_out[i].position);


		//Perspective Divide
		float invW{ 1.f / mesh.vertices_out[i].position.w };

		mesh.vertices_out[i].position.x *= invW;	
		mesh.vertices_out[i].position.y *= invW;	
		mesh.vertices_out[i].position.z *= invW;	


		mesh.vertices_out[i].position.x = (mesh.vertices_out[i].position.x + 1) / 2 * m_Width;
		mesh.vertices_out[i].position.y = (1 - mesh.vertices_out[i].position.y) / 2 * m_Height;

	}

}

void Renderer::RenderTriangleList(const Mesh& currentMesh)
{
	for (size_t idx = 0; idx < currentMesh.indices.size(); idx+=3)
	{
		LoopOverPixels(currentMesh.vertices_out[currentMesh.indices[idx]], currentMesh.vertices_out[currentMesh.indices[idx + 1]], currentMesh.vertices_out[currentMesh.indices[idx + 2]]);
	}
}

void Renderer::RenderTriangleStrip(const Mesh& currentMesh)
{

	for (size_t idx = 0; idx < currentMesh.indices.size() - 2; ++idx)
	{
		if (idx % 2 == 0)
		{
			LoopOverPixels(currentMesh.vertices_out[currentMesh.indices[idx]], currentMesh.vertices_out[currentMesh.indices[idx + 1]], currentMesh.vertices_out[currentMesh.indices[idx + 2]]);
		}
		else
		{
			LoopOverPixels(currentMesh.vertices_out[currentMesh.indices[idx]], currentMesh.vertices_out[currentMesh.indices[idx + 2]], currentMesh.vertices_out[currentMesh.indices[idx + 1]]);
		}

	}


}

void Renderer::LoopOverPixels(const Vertex_Out& ver0, const Vertex_Out& ver1, const Vertex_Out& ver2)
{

	//Frustrum culling
	if (ver0.position.z < 0.f || ver0.position.z > 1.f)
		return;
	if (ver1.position.z < 0.f || ver1.position.z > 1.f)
		return;
	if (ver2.position.z < 0.f || ver2.position.z > 1.f)
		return;

	Vector2 v0 = ver0.position.GetXY();
	Vector2 v1 = ver1.position.GetXY();
	Vector2 v2 = ver2.position.GetXY();

	Vector2 topLeft{};
	topLeft.x = std::min(std::min(v0.x, v1.x), v2.x);
	topLeft.y = std::min(std::min(v0.y, v1.y), v2.y);

	Vector2 bottomRight{};
	bottomRight.x = std::max(std::max(v0.x, v1.x), v2.x);
	bottomRight.y = std::max(std::max(v0.y, v1.y), v2.y);


	Vector3 weight{};
	for (int px{static_cast<int>(topLeft.x)}; px < static_cast<int>(bottomRight.x); ++px)
	{
		for (int py{ static_cast<int>(topLeft.y)}; py < static_cast<int>(bottomRight.y); ++py)
		{
			Vector2 pixel{ static_cast<float>(px), static_cast<float>(py) };

			if (Utils::HitTest_Triangle(pixel, v0, v1, v2, weight))
			{

				float currentDepth =1 / (1.f / ver0.position.z * weight.x + 1.f / ver1.position.z * weight.y + 1.f / ver2.position.z * weight.z);
				if (currentDepth < m_pDepthBufferPixels[px + (py * m_Width)])
				{
					m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

					ColorRGB finalColor{};
					float wBuffer{};
					Vector2 uv{};
					switch (m_CurrentRenderMode)
					{
					case dae::Renderer::RenderMode::FinalColor:
						 wBuffer = 1 / (1 / ver0.position.w * weight.x + 1 / ver1.position.w * weight.y + 1 / ver2.position.w * weight.z);
						 uv = (
							 ver0.uv / ver0.position.w * weight.x +
							 ver1.uv / ver1.position.w * weight.y +
							 ver2.uv / ver2.position.w * weight.z) * wBuffer;
						finalColor = m_pTuktukTexture->Sample(uv);
						break;
					case dae::Renderer::RenderMode::DepthBuffer:
						float remapped{ Remap(currentDepth) };
						finalColor = { remapped,remapped,remapped };
						break;
					}
					
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
