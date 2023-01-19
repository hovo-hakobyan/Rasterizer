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
	m_pVehicleDiffuse = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pVehicleNormal = Texture::LoadFromFile("Resources/vehicle_normal.png");
	m_pVehicleGloss = Texture::LoadFromFile("Resources/vehicle_gloss.png");
	m_pVehicleSpecular = Texture::LoadFromFile("Resources/vehicle_specular.png");


	m_MeshesWorld.emplace_back(Mesh{});
	m_MeshesWorld.emplace_back(Mesh{});

	Utils::ParseOBJ("Resources/tuktuk.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices);
	m_MeshesWorld[0].primitiveTopology = PrimitiveTopology::TriangleList;
	m_MeshesWorld[0].vertices_out.reserve(m_MeshesWorld[0].vertices.size());


	Utils::ParseOBJ("Resources/vehicle.obj", m_MeshesWorld[1].vertices, m_MeshesWorld[1].indices);
	m_MeshesWorld[1].primitiveTopology = PrimitiveTopology::TriangleList;
	m_MeshesWorld[1].vertices_out.reserve(m_MeshesWorld[1].vertices.size());

	for (size_t mesh = 0; mesh < m_MeshesWorld.size(); mesh++)
	{
		for (size_t vert = 0; vert < m_MeshesWorld[mesh].vertices_out.capacity(); vert++)
		{
			m_MeshesWorld[mesh].vertices_out.emplace_back(Vertex_Out{});
		}
	}
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTextureGrid;
	delete m_pTuktukTexture;
	delete m_pVehicleDiffuse;
	delete m_pVehicleNormal;
	delete m_pVehicleGloss;
	delete m_pVehicleSpecular;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	const float yawAngle = 50.f;

	m_MeshesWorld[1].RotateY(yawAngle, pTimer->GetElapsed());

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
		//For each pixel
		for (int px{}; px < m_Width; ++px)
		{
			if (px < topLeft.x || px > bottomRight.x)
				continue;

			for (int py{}; py < m_Height; ++py)
			{			
				if (py < topLeft.y || py > bottomRight.y)
					continue;

				//Does pixel overlap with triangel?
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
	

	VertexTransformationFunction(m_MeshesWorld[1]);
	RenderTriangleList(m_MeshesWorld[1]);
	

	
	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::ToggleRotation()
{
	for (size_t i = 0; i < m_MeshesWorld.size(); i++)
	{
		m_MeshesWorld[i].shouldRotate = !m_MeshesWorld[i].shouldRotate;
	}
}

void dae::Renderer::ToggleShadingMode()
{

	switch (m_ShadingMode)
	{
	case ShadingMode::Combined:
		m_ShadingMode = ShadingMode::ObservedArea;
		break;
	case ShadingMode::Diffuse:
		m_ShadingMode = ShadingMode::Specular;
		break;
	case ShadingMode::ObservedArea:
		m_ShadingMode = ShadingMode::Diffuse;
		break;
	case ShadingMode::Specular:
		m_ShadingMode = ShadingMode::Combined;
		break;
	}

	if (!m_ShadeDepth)
	{
		m_CurrentShadingMode = m_ShadingMode;
	}

	
}

void dae::Renderer::ToggleDepthBuffer()
{
	m_ShadeDepth = !m_ShadeDepth;
	m_CurrentShadingMode = m_ShadeDepth== true ? ShadingMode::DepthBuffer : m_ShadingMode;
}

void dae::Renderer::ToggleNormalMap()
{
	m_UseNormalMap = !m_UseNormalMap;
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	float aspectRatio{ static_cast<float>(m_Width) / m_Height };

	for (size_t i = 0; i < vertices_in.size(); i++)
	{
		vertices_out[i] = vertices_in[i];

		vertices_out[i].position = m_Camera.invViewMatrix.TransformPoint(vertices_out[i].position);

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
	const float aspectRatio{ static_cast<float>(m_Width) / m_Height };
	const Matrix worldViewProjection{ mesh.worldMatrix * m_Camera.invViewMatrix * m_Camera.projectionMatrix };

	for (size_t i = 0; i < mesh.vertices.size(); i++)
	{
		mesh.vertices_out[i].color = mesh.vertices[i].color;
		mesh.vertices_out[i].position.x = mesh.vertices[i].position.x;
		mesh.vertices_out[i].position.y = mesh.vertices[i].position.y;
		mesh.vertices_out[i].position.z = mesh.vertices[i].position.z;
		mesh.vertices_out[i].uv = mesh.vertices[i].uv;
		mesh.vertices_out[i].normal = mesh.worldMatrix.TransformVector(mesh.vertices[i].normal).Normalized();
		mesh.vertices_out[i].tangent = mesh.worldMatrix.TransformVector(mesh.vertices[i].tangent).Normalized();
		mesh.vertices_out[i].viewDirection = mesh.worldMatrix.TransformPoint(mesh.vertices[i].position) - m_Camera.origin;


		mesh.vertices_out[i].position = worldViewProjection.TransformPoint(mesh.vertices_out[i].position);


		//Perspective Divide
		const float invW{ 1.f / mesh.vertices_out[i].position.w };

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
		LoopOverPixels(
			currentMesh.vertices_out[currentMesh.indices[idx]],
			currentMesh.vertices_out[currentMesh.indices[idx + 1]],
			currentMesh.vertices_out[currentMesh.indices[idx + 2]]);
	}
}

void Renderer::RenderTriangleStrip(const Mesh& currentMesh)
{

	for (size_t idx = 0; idx < currentMesh.indices.size() - 2; ++idx)
	{

		if (idx % 2 == 0)
		{
			LoopOverPixels(
				currentMesh.vertices_out[currentMesh.indices[idx]],
				currentMesh.vertices_out[currentMesh.indices[idx + 1]],
				currentMesh.vertices_out[currentMesh.indices[idx + 2]]);
		}
		else
		{
			//Fix counterclockwise order
			LoopOverPixels(
				currentMesh.vertices_out[currentMesh.indices[idx]], 
				currentMesh.vertices_out[currentMesh.indices[idx + 2]], 
				currentMesh.vertices_out[currentMesh.indices[idx + 1]]);
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
	for (int px{std::max(0,static_cast<int>(topLeft.x))}; px <= std::min(m_Width, static_cast<int>(bottomRight.x)) ; ++px)
	{
		for (int py{std::max(0,static_cast<int>(topLeft.y))}; py <=std::min(m_Height, static_cast<int>(bottomRight.y)); ++py)
		{
			Vector2 pixel{ static_cast<float>(px), static_cast<float>(py) };

			if (Utils::HitTest_Triangle(pixel, v0, v1, v2, weight))
			{
				//Z interpolated non-linear
				float currentDepth = 1.f / (weight.x / ver0.position.z + weight.y / ver1.position.z + weight.z / ver2.position.z);

				if (currentDepth < m_pDepthBufferPixels[px + (py * m_Width)])
				{
					//Z-interpolated, linear
					float wBuffer{ 1 / (1 / ver0.position.w * weight.x + 1 / ver1.position.w * weight.y + 1 / ver2.position.w * weight.z) };
					Vector2 uv{};
					uv = (
						ver0.uv / ver0.position.w * weight.x +
						ver1.uv / ver1.position.w * weight.y +
						ver2.uv / ver2.position.w * weight.z) * wBuffer;
					
					ColorRGB col
					{ (
						ver0.color * weight.x * ver0.position.w +
						ver1.color * weight.y * ver1.position.w + 
						ver2.color * weight.z * ver2.position.w
						) * wBuffer
					};
					Vector3 normal{ (
						ver0.normal * weight.x * ver0.position.w + 
						ver1.normal * weight.y * ver1.position.w + 
						ver2.normal * weight.z * ver2.position.w) * wBuffer };
				
					normal.Normalize();

					Vector3 tangent{(
						ver0.tangent * weight.x * ver0.position.w + 
						ver1.tangent * weight.y * ver1.position.w + 
						ver2.tangent * weight.z * ver2.position.w) * wBuffer};
					tangent.Normalize();

					Vector3 viewDir{ (
						ver0.viewDirection * weight.x * ver0.position.w + 
						ver1.viewDirection * weight.y * ver1.position.w + 
						ver2.viewDirection * weight.z * ver2.position.w) * wBuffer };
					viewDir.Normalize();

					Vertex_Out currentPixel
					{
						Vector4{static_cast<float>(px),static_cast<float>(py),currentDepth,wBuffer},
						col,
						uv,
						normal,
						tangent,
						viewDir
					};

					m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;
				
					PixelShading(currentPixel);
				}

			}
		}
	}

}

void Renderer::PixelShading(const Vertex_Out& v)
{
	ColorRGB finalColor{};
	float remapped{};
	const float intensity{ 7.f };
	const float shininess{ 25.f };
	
	Vector3 binormal{ Vector3::Cross(v.normal,v.tangent) };
	Matrix tangentSpaceAxis = Matrix{ v.tangent,binormal,v.normal,Vector3::Zero };

	ColorRGB normalSample{ m_pVehicleNormal->Sample(v.uv) };
	Vector3 normalSampleVec{ normalSample.r,normalSample.g,normalSample.b };

	Vector3 normal{ 2.f * normalSampleVec - Vector3{1.f,1.f,1.f} };
	normal = m_UseNormalMap ? tangentSpaceAxis.TransformVector(normal).Normalized(): v.normal;

	const float lambertCosine{ Vector3::Dot(normal, -m_LightDirection) };

	if (m_CurrentShadingMode == ShadingMode::DepthBuffer)
	{
		remapped = Remap(v.position.z);
		finalColor = { remapped,remapped,remapped };
	}
	else
	{
		if (lambertCosine > 0.f)
		{
			//Phong
			Vector3 reflect = -m_LightDirection - 2 * std::max(Vector3::Dot(normal, -m_LightDirection), 0.f) * normal;
			float alpha = std::max(Vector3::Dot(reflect, v.viewDirection), 0.f);
			ColorRGB specular = m_pVehicleSpecular->Sample(v.uv) * powf(alpha, shininess * m_pVehicleGloss->Sample(v.uv).r);

			specular.r = std::max(0.f, specular.r);
			specular.g = std::max(0.f, specular.g);
			specular.b = std::max(0.f, specular.b);

			ColorRGB ambient{ .025f,.025f, .025f };
			ColorRGB diffuse{ Utils::Lambert(intensity, m_pVehicleDiffuse->Sample(v.uv)) };

			switch (m_CurrentShadingMode)
			{
			case ShadingMode::Combined:
				finalColor = (diffuse + specular + ambient) * lambertCosine;
				break;
			case ShadingMode::Diffuse:
				finalColor = diffuse * lambertCosine;
				break;
			case ShadingMode::ObservedArea:
				finalColor = ColorRGB{ lambertCosine,lambertCosine,lambertCosine };
				break;
			case ShadingMode::Specular:
				finalColor = specular * lambertCosine;
				break;
			}
		}
	}

	//Update Color in Buffer
	finalColor.MaxToOne();

	m_pBackBufferPixels[static_cast<int>(v.position.x) + (static_cast<int>( v.position.y) * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
}


bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
