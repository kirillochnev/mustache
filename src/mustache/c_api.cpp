#include "mustache/ecs/ecs.hpp"
#include "c_api.h"
#include "mustache/ecs/non_template_job.hpp"

#include <numeric>
#include <valarray>

namespace converter {
    mustache::Entity convert(Entity entity) noexcept {
        return mustache::Entity::makeFromValue(entity);
    }

    Entity convert(mustache::Entity entity) noexcept {
        return entity.value;
    }

    mustache::World* convert(World* world) noexcept {
        return reinterpret_cast<mustache::World*>(world);
    }

    World* convert(mustache::World* world) noexcept {
        return reinterpret_cast<World*>(world);
    }

    mustache::ComponentId convert(ComponentId id) noexcept {
        return mustache::ComponentId::make(id);
    }

    mustache::NonTemplateJob* convert(Job* job) noexcept {
        return reinterpret_cast<mustache::NonTemplateJob*>(job);
    }

    Job* convert(mustache::NonTemplateJob* job) noexcept {
        return reinterpret_cast<Job*>(job);
    }

    JobInvocationIndex convert(const mustache::JobInvocationIndex& index) noexcept {
        JobInvocationIndex result;
        result.entity_index = index.entity_index.toInt();
        result.entity_index_in_task = index.entity_index_in_task.toInt();
        result.thread_id = index.thread_id.toInt();
        result.task_index = index.task_index.toInt();
        return result;
    }

    mustache::NonTemplateJob::Callback convert(ForEachArrayCallback callback) noexcept {
        return [callback](mustache::NonTemplateJob::ForEachArrayArgs in_args) {
            JobForEachArrayArg args;
            args.entities = reinterpret_cast<Entity*>(in_args.entities);
            args.components = static_cast<ComponentPtr*>(in_args.components);
            args.shared_components = static_cast<SharedComponentPtr*>(in_args.shared_components);
            args.invocation_index = convert(in_args.invocation_index);
            args.array_size = in_args.count.toInt();
            callback(&args);
        };
    }

    mustache::TypeInfo convert(const TypeInfo& info) noexcept {
        mustache::TypeInfo type_info;
        type_info.name = info.name;
        type_info.size = info.size;
        type_info.align = info.align;
        type_info.type_id_hash_code = std::hash<std::string>{}(info.name);

        type_info.functions.create = info.functions.create;
        type_info.functions.copy = info.functions.copy;
        type_info.functions.move = info.functions.move;
        type_info.functions.move_constructor = info.functions.move_constructor;
        type_info.functions.destroy = info.functions.destroy;
        if (info.default_value != nullptr) {
            type_info.default_value.resize(info.size);
            memcpy(type_info.default_value.data(), info.default_value, info.size);
        }
        return type_info;
    }

    mustache::ComponentIdMask convert(const ComponentMask& mask) noexcept {
        mustache::ComponentIdMask result;
        for (uint32_t i = 0; i < mask.component_count; ++i) {
            result.set(convert(mask.ids[i]), true);
        }
        return result;
    }

    mustache::Archetype* convert(Archetype* archetype) noexcept {
        return reinterpret_cast<mustache::Archetype*>(archetype);
    }

    Archetype* convert(mustache::Archetype* archetype) noexcept {
        return reinterpret_cast<Archetype*>(archetype);
    }

    mustache::NonTemplateJob::ComponentRequest convert(const JobArgInfo& info) noexcept {
        mustache::NonTemplateJob::ComponentRequest result;
        result.id = convert(info.component_id);
        result.is_const = info.is_const;
        result.is_required = info.is_required;
        return result;
    }

    mustache::SystemConfig convert(const SystemConfig& cfg) {
        mustache::SystemConfig result;
        result.priority = cfg.priority;
        result.update_group = cfg.update_group;
        for (uint32_t i = 0; i < cfg.update_after_size; ++i) {
            result.update_after.insert(cfg.update_after[i]);
        }
        for (uint32_t i = 0; i < cfg.update_before_size; ++i) {
            result.update_after.insert(cfg.update_before[i]);
        }
        return result;
    }

    SystemConfig convert(mustache::SystemConfig& cfg) {
        SystemConfig result;
        result.priority = cfg.priority;
        result.update_group = cfg.update_group.c_str();
        result.update_after_size = static_cast<uint32_t >(cfg.update_after.size());
        result.update_before_size = static_cast<uint32_t >(cfg.update_before.size());
        if (result.update_after_size > 0) {
            result.update_after = new const char*[result.update_after_size];
            uint32_t i = 0;
            for (const auto& str : cfg.update_after) {
                result.update_after[i] = str.c_str();
                ++i;
            }
        }
        if (result.update_before_size > 0) {
            result.update_before = new const char*[result.update_before_size];
            uint32_t i = 0;
            for (const auto& str : cfg.update_before) {
                result.update_before[i] = str.c_str();
                ++i;
            }
        }
        return result;
    }
}

using namespace converter;

struct CSystem : mustache::ASystem {
    CSystem(const SystemDescriptor& descriptor):
            descriptor_{descriptor} {

    }
    ~CSystem() override {
        if (descriptor_.free_user_data) {
            descriptor_.free_user_data(descriptor_.user_data);
        }
    }
    std::string name() const noexcept override {
        return descriptor_.name;
    }
protected:
    SystemDescriptor descriptor_;

    template<typename _F, typename... ARGS>
    void invoke(_F ptr, mustache::World& world, ARGS&&... args) {
        if (ptr != nullptr) {
            mustache::invoke(ptr, convert(&world), descriptor_.user_data, std::forward<ARGS>(args)...);
        }
    }
    void onCreate(mustache::World& world) override {
        invoke(descriptor_.on_create, world);
    }

    void onConfigure(mustache::World& world, mustache::SystemConfig& config) override {
        SystemConfig system_config = convert(config);
        invoke(descriptor_.on_configure, world, &system_config);
        config = convert(system_config);
    }

    void onStart(mustache::World& world) override {
        invoke(descriptor_.on_start, world);
    }

    void onUpdate(mustache::World& world) override {
        invoke(descriptor_.on_update, world);
    }

    void onPause(mustache::World& world) override {
        invoke(descriptor_.on_pause, world);
    }

    void onStop(mustache::World& world) override {
        invoke(descriptor_.on_stop, world);
    }

    void onResume(mustache::World& world) override {
        invoke(descriptor_.on_resume, world);
    }

    void onDestroy(mustache::World& world) override {
        invoke(descriptor_.on_destroy, world);
    }
};


World* createWorld(WorldId id) {
    const auto world_id = mustache::WorldId::make(id);
    if (world_id.isValid()) {
        return convert(new mustache::World{world_id});
    }
    return convert(new mustache::World{});
}

void updateWorld(World* world) {
    convert(world)->update();
}

void clearWorldEntities(World* world) {
    convert(world)->entities().clear();
}

void destroyWorld(World* world) {
    delete convert(world);
}

ComponentId registerComponent(TypeInfo info) {
    return mustache::ComponentFactory::componentId(convert(info)).toInt();
}

ComponentPtr assignComponent(World* world, Entity entity, ComponentId component_id, bool skip_constructor) {
    auto ptr = convert(world)->entities().assign(convert(entity), convert(component_id), skip_constructor);
    return reinterpret_cast<ComponentPtr>(ptr);
}
bool hasComponent(World* world, Entity entity, ComponentId component_id) {
    return convert(world)->entities().hasComponent(convert(entity), convert(component_id));
}
void removeComponent(World* world, Entity entity, ComponentId component_id) {
    convert(world)->entities().removeComponent(convert(entity), convert(component_id));
}
ComponentPtr getComponent(World* world, Entity entity, ComponentId component_id, bool is_const) {
//    const auto type_id = mustache::ComponentFactory::componentInfo(convert(component_id));
//    mustache::Logger{}.info("World: %p, Entity: %d, Component: %d, const: %s, Name: %s",
//                            world, entity, component_id, is_const ? "const" : "mutable", type_id.name.c_str());
    const void* ptr = nullptr;
    if (is_const) {
        ptr = convert(world)->entities().getComponent<true>(convert(entity), convert(component_id));
    } else {
        ptr = convert(world)->entities().getComponent<false>(convert(entity), convert(component_id));
    }
    return const_cast<ComponentPtr>(ptr);
}

Job* makeJob(JobDescriptor info) {
    auto job = new mustache::NonTemplateJob{};
    job->callback = convert(info.callback);
    job->job_name = info.name;
    job->component_requests.resize(info.component_info_arr_size);
    for (uint32_t i = 0; i < info.component_info_arr_size; ++i) {
        job->component_requests[i] = convert(info.component_info_arr[i]);
    }
    for (uint32_t i = 0; i < info.check_update_size; ++i) {
        job->version_check_mask.set(convert(info.check_update[i]), true);
    }
    return convert(job);
}

void runJob(Job* job, World* world) {
    convert(job)->run(*convert(world));
}

void destroyJob(Job* job) {
    delete convert(job);
}

Entity createEntity(World* world, Archetype* archetype) {
    return convert(convert(world)->entities().create(*convert(archetype)));
}
void createEntityGroup(World* world, Archetype* archetype_ptr, Entity* ptr, uint32_t count) {
    std::unique_ptr<mustache::Entity[]> result {new mustache::Entity[count]};
    auto& entities = convert(world)->entities();
    auto& archetype = *convert(archetype_ptr);
    auto* out = reinterpret_cast<mustache::Entity*>(ptr);
    if (out) {
        for (uint32_t i = 0; i < count; ++i) {
            result[i] = entities.create(archetype);
        }
    } else {
        for (uint32_t i = 0; i < count; ++i) {
            (void) entities.create(archetype);
        }
    }
}

void destroyEntities(World* world, Entity* entities, uint32_t count, bool now) {
    auto& manager = convert(world)->entities();
    const auto mustache_entities = reinterpret_cast<mustache::Entity*>(entities);
    for (uint32_t i = 0; i < count; ++i) {
        if (now) {
            manager.destroyNow(mustache_entities[i]);
        } else {
            manager.destroy(mustache_entities[i]);
        }
    }
}

Archetype* getArchetype(World* world, ComponentMask mask) {
    auto& entities = convert(world)->entities();
    return convert(&entities.getArchetype(convert(mask), mustache::SharedComponentsInfo{}));
}

CSystem* createSystem(World* world, const SystemDescriptor* descriptor) {
    std::unique_ptr<CSystem> result{new CSystem{*descriptor}};
    if (world != nullptr) {
        mustache::SystemManager::SystemPtr system {result.release()};
        convert(world)->systems().addSystem(system);
        return static_cast<CSystem*>(system.get());
    }
    return result.release();
}

void addSystem(World* world, CSystem* system) {
    convert(world)->systems().addSystem(mustache::SystemManager::SystemPtr{system, [](CSystem*){}});
}