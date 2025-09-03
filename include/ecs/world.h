#pragma once

#include <memory>
#include <unordered_map>
#include <set>
#include <typeinfo>
#include <stdexcept>

namespace nft::ecs
{
using EntityId = uint32_t;
constexpr EntityId INVALID_ENTITY = 0;

// Base class for component storage to enable polymorphism
class ComponentStorageBase
{
  public:
	virtual ~ComponentStorageBase()														= default;
	virtual void	EraseComponent(EntityId entity)				= 0;
};

// Template specialization for storing components of a specific type
template<typename ComponentType>
class ComponentStorage : public ComponentStorageBase
{
  public:
	void EraseComponent(EntityId entity) override
	{
		components.erase(entity);
	}

	std::unordered_map<EntityId, std::unique_ptr<ComponentType>> components;
};

class World
{
  public:
	World()	 = default;
	~World() = default;
	EntityId CreateEntity()
	{
		EntityId new_id = next_entity_id++;
		entities.insert(new_id);
		return new_id;
	}
	void DestroyEntity(EntityId entity)
	{
		entities.erase(entity);
		// Remove all components associated with this entity
		for (auto& [type_hash, storage] : component_storages)
		{
			storage->EraseComponent(entity);
		}
	}
	template<typename ComponentType, typename... Args>
	void AddComponent(EntityId entity, Args&&... args)
	{
		if (entities.find(entity) == entities.end())
			throw std::runtime_error("Entity does not exist");

		size_t type_hash = typeid(ComponentType).hash_code();

		// Get or create storage for this component type
		auto& storage_ptr = component_storages[type_hash];
		if (!storage_ptr)
		{
			storage_ptr = std::make_unique<ComponentStorage<ComponentType>>();
		}

		// Cast to the specific storage type and add the component
		auto* typed_storage = static_cast<ComponentStorage<ComponentType>*>(storage_ptr.get());
		typed_storage->components[entity] = std::make_unique<ComponentType>(std::forward<Args>(args)...);
	}
	template<typename ComponentType>
	void RemoveComponent(EntityId entity)
	{
		size_t type_hash = typeid(ComponentType).hash_code();
		auto it = component_storages.find(type_hash);
		if (it != component_storages.end())
		{
			auto* typed_storage = static_cast<ComponentStorage<ComponentType>*>(it->second.get());
			typed_storage->components.erase(entity);
		}
	}
	template<typename ComponentType>
	ComponentType* GetComponent(EntityId entity)
	{
		size_t type_hash = typeid(ComponentType).hash_code();
		auto it = component_storages.find(type_hash);
		if (it != component_storages.end())
		{
			auto* typed_storage = static_cast<ComponentStorage<ComponentType>*>(it->second.get());
			auto comp_it = typed_storage->components.find(entity);
			if (comp_it != typed_storage->components.end())
			{
				return comp_it->second.get();
			}
		}
		return nullptr;
	}

  private:
	EntityId next_entity_id = 1;
	std::set<EntityId> entities;
	std::unordered_map<size_t, std::unique_ptr<ComponentStorageBase>> component_storages;
};
}	 // namespace nft::ecs