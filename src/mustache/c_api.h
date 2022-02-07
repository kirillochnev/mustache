#ifndef MUSTACHE_C_API_H
#define MUSTACHE_C_API_H

#include <cstdint>
#include <cstddef>

extern "C" {
    struct World;
    struct Archetype;
    struct Job;
    struct CSystem;

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
        const void* default_value;
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
        ComponentId* check_update;
        uint32_t check_update_size;
        bool entity_required;
        const char* name;
    } JobDescriptor;

    typedef struct {
        uint32_t component_count;
        ComponentId* ids;
    } ComponentMask;

    typedef struct {
        const char* update_group;
        const char** update_before;
        uint32_t update_before_size;
        const char** update_after;
        uint32_t update_after_size;
        int32_t priority;
    } SystemConfig;

    typedef void (*SystemEvent)(struct World*, void* user_data);
    typedef struct {
        const char* name;
        void* user_data;
        void (*free_user_data)(void* user_data);
        SystemEvent on_create;
        void (*on_configure)(struct World*, SystemConfig*, void* user_data);
        SystemEvent on_start;
        SystemEvent on_stop;
        SystemEvent on_update;
        SystemEvent on_pause;
        SystemEvent on_resume;
        SystemEvent on_destroy;
    } SystemDescriptor;

struct World* createWorld(WorldId id);
    void updateWorld(struct World* world);
    void clearWorldEntities(struct World*);
    void destroyWorld(struct World*);
    ComponentId registerComponent(TypeInfo info);
    bool hasComponent(struct World* world, Entity entity, ComponentId component_id);
    ComponentPtr assignComponent(struct World* world, Entity entity, ComponentId component_id, bool skip_constructor);
    ComponentPtr getComponent(struct World* world, Entity entity, ComponentId component_id, bool is_const);
    void removeComponent(struct World* world, Entity entity, ComponentId component_id);
    struct Job* makeJob(JobDescriptor info);
    void runJob(struct Job* job, struct World* world);
    void destroyJob(struct Job*);
    Entity createEntity(struct World* world, struct Archetype* archetype);
    void createEntityGroup(struct World* world, struct Archetype* archetype, Entity* ptr, uint32_t count);
    void destroyEntities(struct World* world, Entity* entities, uint32_t count, bool now);
    struct Archetype* getArchetype(struct World* world, ComponentMask mask);
    struct CSystem* createSystem(struct World* world, const SystemDescriptor* descriptor);
    void addSystem(struct World* world, struct CSystem* system);
}

#endif //MUSTACHE_C_API_H
