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

	// Accessors
	const glm::vec3& GetColor() const { return color; }
	uint32_t GetTextureIndex() const { return texture_index; }
	void SetColor(const glm::vec3& new_color) { color = new_color; }
	void SetTextureIndex(uint32_t index) { texture_index = index; }

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

	// Accessors
	const glm::vec3& GetColor() const { return color; }
	uint32_t GetTextureIndex() const { return texture_index; }
	void SetColor(const glm::vec3& new_color) { color = new_color; }
	void SetTextureIndex(uint32_t index) { texture_index = index; }

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

	// Accessors
	const glm::vec3& GetColor() const { return color; }
	uint32_t GetTextureIndex() const { return texture_index; }
	float GetIntensity() const { return intensity; }
	void SetColor(const glm::vec3& new_color) { color = new_color; }
	void SetTextureIndex(uint32_t index) { texture_index = index; }
	void SetIntensity(float new_intensity) { intensity = new_intensity; }

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

	// Accessors
	const AmbientComponent& GetAmbient() const { return ambient; }
	const DiffuseComponent& GetDiffuse() const { return diffuse; }
	const SpecularComponent& GetSpecular() const { return specular; }
	AmbientComponent& GetAmbient() { return ambient; }
	DiffuseComponent& GetDiffuse() { return diffuse; }
	SpecularComponent& GetSpecular() { return specular; }

	// Convenience methods for vulkan integration
	glm::vec3 GetAmbientColor() const { return ambient.GetColor(); }
	glm::vec3 GetDiffuseColor() const { return diffuse.GetColor(); }
	glm::vec3 GetSpecularColor() const { return specular.GetColor(); }
	float GetSpecularIntensity() const { return specular.GetIntensity(); }
	uint32_t GetAmbientTextureIndex() const { return ambient.GetTextureIndex(); }
	uint32_t GetDiffuseTextureIndex() const { return diffuse.GetTextureIndex(); }
	uint32_t GetSpecularTextureIndex() const { return specular.GetTextureIndex(); }

  private:
	AmbientComponent  ambient;
	DiffuseComponent  diffuse;
	SpecularComponent specular;
};
}	 // namespace nft::graphics