#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace nft::graphics
{
class TransformHandler
{
  public:
	TransformHandler()	= default;
	~TransformHandler() = default;
	void	  SetPosition(const glm::vec3& position) { this->position = position; }
	glm::vec3 GetPosition() const { return position; }
	void	  SetRotation(const glm::vec3& rotation) { this->rotation = rotation; }
	glm::vec3 GetRotation() const { return rotation; }
	void	  SetScale(const glm::vec3& scale) { this->scale = scale; }
	glm::vec3 GetScale() const { return scale; }
	glm::mat4 GetModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model			= glm::translate(model, position);
		model			= glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
		model			= glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
		model			= glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
		model			= glm::scale(model, scale);
		return model;
	}

  private:
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale	   = glm::vec3(1.0f);
};
}	 // namespace nft::graphics