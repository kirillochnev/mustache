#ifndef MUSTACHE_MUSTACHE_C_API_H
#define MUSTACHE_MUSTACHE_C_API_H

#include <cstdint>
#include <cstddef>

extern "C" {
    struct World;
    struct Archetype;
    struct Job;

    typedef uint32_t WorldId;
    typedef uint64_t Entity;
    typedef uint32_t ComponentId;
    typedef void* ComponentPtr;
    typedef const void* SharedComponentPtr;

    typedef uint32_t ThreadId;
    typedef uint32_t ParallelTaskId;
    typedef uint32_t ParallelTaskItemIndexInTask;
    typedef uint32_t ParallelTaskGlobalItemIndex;



    typedef struct {
        ParallelTaskId task_index;
        ParallelTaskItemIndexInTask entity_index_in_task;
        ParallelTaskGlobalItemIndex entity_index;
        ThreadId thread_id;
    } JobInvocationIndex;

    typedef struct {
        Entity* entities;
        ComponentPtr* components;
        SharedComponentPtr* shared_components;
        uint32_t array_size;

        JobInvocationIndex invocation_index;
    } JobForEachArrayArg;

    typedef void (*ForEachArrayCallback)(JobForEachArrayArg*);

    typedef struct {
        void (*create) (void*);
        void (*copy) (void*, const void*);
        void (*move) (void*, void*);
        void (*move_constructor) (void*, void*);
        void (*destroy) (void*);
    } TypeInfoFunctions;

    typedef struct {
        size_t size;
        size_t align;
        const char* name;
        TypeInfoFunctions functions;
    } TypeInfo;

    typedef struct {
        ComponentId component_id;
        bool is_required;
        bool is_const;
    } JobArgInfo;

    typedef struct {
        ForEachArrayCallback callback;
        JobArgInfo* component_info_arr;
        uint32_t component_info_arr_size;
        bool entity_required;
        const char* name;
    } JobDescriptor;

    typedef struct {
        uint32_t component_count;
        ComponentId* ids;
    } ComponentMask;

    World* createWorld(WorldId id);
    void destroyWorld(World*);
    ComponentId registerComponent(TypeInfo info);
    ComponentPtr assignComponent(World* world, Entity entity, ComponentId component_id);
    Job* makeJob(JobDescriptor info);
    void runJob(Job* job, World* world);
    Entity createEntity(World* world, Archetype* archetype);
    void createEntityGroup(World* world, Archetype* archetype, uint32_t count);
    Archetype* getArchetype(World* world, ComponentMask mask);
}

#endif //MUSTACHE_MUSTACHE_C_API_H
