#pragma once

#include "graphics/mesh.h"
#include "graphics/transform.h"

#include <memory>
#include <vector>

namespace nft::graphics
{

class Object
{
  public:
	void SetMesh(TriMesh new_mesh) { mesh = std::make_unique<TriMesh>(new_mesh); }

	TransformHandler& GetTransforms() { return transforms; }

  private:
	//Scene&					 scene;
	std::unique_ptr<TriMesh> mesh;
	TransformHandler		 transforms;

	friend class Scene;
};

}	 // namespace nft::graphics