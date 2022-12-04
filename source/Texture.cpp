#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		SDL_Surface* surface = IMG_Load(path.c_str());
		Texture* texture{ new Texture{surface} };

		return texture;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		const int nrColors{ 3 };
		Uint8 rgb[nrColors];
		//px + (py * m_Width)
		int idx{ static_cast<int>(uv.x * m_pSurface->w) + (static_cast<int>(uv.y * m_pSurface->h) * m_pSurface->w) };

		Uint32 pixel{ m_pSurfacePixels[idx] };
		SDL_GetRGB(pixel, m_pSurface->format, &rgb[0], &rgb[1], &rgb[2]);


		return ColorRGB{static_cast<float>(rgb[0] / 255.0f),static_cast<float>(rgb[1]/ 255.0f) ,static_cast<float>(rgb[2] / 255.0f) };
	}
}