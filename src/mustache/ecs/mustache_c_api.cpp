#include <mustache/ecs/ecs.hpp>
#include <mustache/ecs/mustache_c_api.h>
#include <mustache/ecs/non_template_job.hpp>

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
}

using namespace converter;

World* createWorld(WorldId id) {
    return convert(new mustache::World{mustache::WorldId::make(id)});
}

void destroyWorld(World* world) {
    delete convert(world);
}

ComponentId registerComponent(TypeInfo info) {
    return mustache::ComponentFactory::componentId(convert(info)).toInt();
}

ComponentPtr assignComponent(World* world, Entity entity, ComponentId component_id) {
    auto ptr = convert(world)->entities().assign(convert(entity), convert(component_id));
    return reinterpret_cast<ComponentPtr>(ptr);
}

Job* makeJob(JobDescriptor info) {
    auto job = new mustache::NonTemplateJob{};
    job->callback = convert(info.callback);
    job->job_name = info.name;
    job->component_requests.resize(info.component_info_arr_size);
    for (uint32_t i = 0; i < info.component_info_arr_size; ++i) {
        job->component_requests[i] = convert(info.component_info_arr[i]);
    }
    return convert(job);
}

void runJob(Job* job, World* world) {
    convert(job)->run(*convert(world));
}

Entity createEntity(World* world, Archetype* archetype) {
    return convert(convert(world)->entities().create(*convert(archetype)));
}
void createEntityGroup(World* world, Archetype* archetype_ptr, uint32_t count) {
    auto& entities = convert(world)->entities();
    auto& archetype = *convert(archetype_ptr);
    for (uint32_t i = 0; i < count; ++i) {
        (void )entities.create(archetype);
    }
}
Archetype* getArchetype(World* world, ComponentMask mask) {
    auto& entities = convert(world)->entities();
    return convert(&entities.getArchetype(convert(mask), mustache::SharedComponentsInfo{}));
}
