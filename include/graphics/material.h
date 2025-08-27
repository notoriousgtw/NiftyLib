#pragma once

#include <glm/glm.hpp>

namespace nft::graphics
{

class AmbientComponent
{
  public:
	AmbientComponent(glm::vec3 color = glm::vec3(1.0f), uint32_t texture_index = UINT32_MAX):
		color(color), texture_index(texture_index)
	{
	}
	~AmbientComponent() = default;

  private:
	glm::vec3 color			= glm::vec3(1.0f);	  // Default white color
	uint32_t  texture_index = UINT32_MAX;		  // Index into texture array (UINT32_MAX = no texture)
};

class DiffuseComponent
{
  public:
	DiffuseComponent(glm::vec3 color = glm::vec3(1.0f), uint32_t texture_index = UINT32_MAX):
		color(color), texture_index(texture_index)
	{
	}
	~DiffuseComponent() = default;

  private:
	glm::vec3 color			= glm::vec3(1.0f);	  // Default white color
	uint32_t  texture_index = UINT32_MAX;		  // Index into texture array (UINT32_MAX = no texture)
};

class SpecularComponent
{
  public:
	SpecularComponent(glm::vec3 color = glm::vec3(1.0f), uint32_t texture_index = UINT32_MAX, float intensity = 32.0f):
		color(color), texture_index(texture_index), intensity(intensity)
	{
	}
	~SpecularComponent() = default;

  private:
	glm::vec3 color			= glm::vec3(1.0f);	  // Default white color
	uint32_t  texture_index = UINT32_MAX;		  // Index into texture array (UINT32_MAX = no texture)
	float	  intensity		= 32.0f;			  // Specular intensity
};

class Material
{
  public:
	Material() = default;
	Material(const AmbientComponent& ambient, const DiffuseComponent& diffuse, const SpecularComponent& specular):
		ambient(ambient), diffuse(diffuse), specular(specular)
	{
	}

  private:
	AmbientComponent  ambient;
	DiffuseComponent  diffuse;
	SpecularComponent specular;
};
}	 // namespace nft::graphics