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

    mustache::NonTemplateJob::Callback convert(ForEachArrayCallback callback, Job* job) noexcept {
        return [callback, job](mustache::NonTemplateJob::ForEachArrayArgs in_args) {
            JobForEachArrayArg args;
            args.entities = reinterpret_cast<Entity*>(in_args.entities);
            args.components = static_cast<ComponentPtr*>(in_args.components);
            args.shared_components = static_cast<SharedComponentPtr*>(in_args.shared_components);
            args.invocation_index = convert(in_args.invocation_index);
            args.array_size = in_args.count.toInt();
            callback(job, &args);
        };
    }

    mustache::JobRunMode convert(JobRunMode mode) noexcept {
        if (mode == JobRunMode::kParallel) {
            return mustache::JobRunMode::kParallel;
        }
        return mustache::JobRunMode::kCurrentThread;
    }
    JobRunMode convert(mustache::JobRunMode mode) noexcept {
        if (mode == mustache::JobRunMode::kParallel) {
            return JobRunMode::kParallel;
        }
        return JobRunMode::kCurrentThread;
    }
    mustache::NonTemplateJob::JobEvent convert(JobEvent callback, Job* job) noexcept {
        if (callback == nullptr) {
            return {};
        }
        return [callback, job](mustache::World& world, mustache::TasksCount tasks_count,
                          mustache::JobSize job_size, mustache::JobRunMode mode) {
            callback(job, convert(&world), tasks_count.toInt(), job_size.toInt(), convert(mode));
        };
    }
    mustache::TypeInfo::Constructor convert(void(*create)(void*)) {
        if (create == nullptr) {
            return mustache::TypeInfo::Constructor{};
        }
        return [create](void* ptr, const mustache::Entity&, mustache::World&) {
            if (create != nullptr) {
                create(ptr);
            } else {
                mustache::Logger{}.error("Create function is null!");
            }
        };
    }

    mustache::TypeInfo convert(const TypeInfo& info) noexcept {
        mustache::TypeInfo type_info;
        type_info.name = info.name;
        type_info.size = info.size;
        type_info.align = info.align;
        type_info.type_id_hash_code = std::hash<std::string>{}(info.name);

        type_info.functions.create = convert(info.functions.create);

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
    const char* nameCStr() const noexcept override {
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
    const auto type_id = mustache::ComponentFactory::componentInfo(convert(component_id));
    const void* ptr = nullptr;
    if (is_const) {
        ptr = convert(world)->entities().getComponent<true>(convert(entity), convert(component_id));
    } else {
        ptr = convert(world)->entities().getComponent<false>(convert(entity), convert(component_id));
    }
//    mustache::Logger{}.info("Ptr: %p, Entity: %d, Component: %d, const: %s, Name: %s",
//                            ptr, entity, component_id, is_const ? "const" : "mutable", type_id.name.c_str());
    return const_cast<ComponentPtr>(ptr);
}

Job* makeJob(JobDescriptor info) {
    auto job = new mustache::NonTemplateJob{};
    job->callback = convert(info.callback, convert(job));
    job->job_name = info.name;
    job->component_requests.resize(info.component_info_arr_size);
    job->job_begin = convert(info.on_job_begin, convert(job));
    job->job_end = convert(info.on_job_end, convert(job));
//    mustache::Logger{}.info("Args count: %d", info.component_info_arr_size);
    for (uint32_t i = 0; i < info.component_info_arr_size; ++i) {
        auto t = info.component_info_arr[i];
//        mustache::Logger{}.info("%d) Id: %d, Required: %d, Const: %d", i, t.component_id, t.is_required, t.is_const);
        job->component_requests[i] = convert(info.component_info_arr[i]);
    }
    for (uint32_t i = 0; i < info.check_update_size; ++i) {
        job->version_check_mask.set(convert(info.check_update[i]), true);
    }
//    mustache::Logger{}.info("New job has been create, name: [%s], ptr: %p", info.name, job);
    return convert(job);
}

void runJob(Job* job, World* world, JobRunMode mode) {
    convert(job)->run(*convert(world), convert(mode));
}

void destroyJob(Job* job) {
    delete convert(job);
}

Entity createEntity(World* world, Archetype* archetype) {
    return convert(convert(world)->entities().create(*convert(archetype)));
}
void createEntityGroup(World* world, Archetype* archetype_ptr, Entity* ptr, uint32_t count) {
    auto& entities = convert(world)->entities();
    auto& archetype = *convert(archetype_ptr);
    auto* out = reinterpret_cast<mustache::Entity*>(ptr);
    if (out) {
        for (uint32_t i = 0; i < count; ++i) {
            out[i] = entities.create(archetype);
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
Archetype* getArchetypeByBitsetMask(World* world, uint64_t bits)
{
    auto& entities = convert(world)->entities();
    mustache::ComponentIdMask mask;
    for (uint64_t i = 0; i < 64; ++i) {
        const auto is_set = ((1ull << i) & bits) != 0u;
        mask.set(mustache::ComponentId::make(i), is_set);
    }
    return convert(&entities.getArchetype(mask, mustache::SharedComponentsInfo{}));
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

struct Foo {
    static uint32_t next() noexcept {
        static uint32_t value = 0;
        return value++;
    }

    template<typename T>
    static uint32_t getId() noexcept {
        static auto result = next();
        return result;
    }
};
