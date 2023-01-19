#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render_Week1();
		void Render_Week2();

		void ToggleRotation();
		void ToggleShadingMode();
		void ToggleDepthBuffer();
		void ToggleNormalMap();

		bool SaveBufferToImage() const;

	private:
		enum class ShadingMode
		{
			Combined, Diffuse, ObservedArea, Specular, DepthBuffer
		};

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Texture* m_pTextureGrid;
		Texture* m_pTuktukTexture;

		Texture* m_pVehicleDiffuse;
		Texture* m_pVehicleNormal;
		Texture* m_pVehicleSpecular;
		Texture* m_pVehicleGloss;

		Camera m_Camera{};

		std::vector<Mesh> m_MeshesWorld;

		int m_Width{};
		int m_Height{};

		ShadingMode m_CurrentShadingMode{ ShadingMode::Combined};
		ShadingMode m_ShadingMode{ ShadingMode::Diffuse };
		bool m_ShadeDepth;
		bool m_UseNormalMap;

		Vector3 m_LightDirection{ .577f,-.577f,.577f };

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const; //W1 Version
		void VertexTransformationFunction(Mesh& mesh) const; //W2 Version

		void RenderTriangleList(const Mesh& currentMesh);
		void RenderTriangleStrip(const Mesh& currentMesh);
		void LoopOverPixels(const Vertex_Out& ver0, const Vertex_Out& ver1, const Vertex_Out& ver2);

		void PixelShading(const Vertex_Out& v);


		
	};
}
