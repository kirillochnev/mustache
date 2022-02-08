#ifndef MUSTACHE_C_API_H
#define MUSTACHE_C_API_H

#include <cstdint>
#include <cstddef>
#include <mustache/utils/dll_export.h>

#ifdef __cplusplus
extern "C" {
#endif

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
typedef uint32_t TasksCount;
typedef uint32_t JobSize;

enum JobRunMode {
    kCurrentThread = 0,
    kParallel
};

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


typedef struct {
    void (* create)(void*);

    void (* copy)(void*, const void*);

    void (* move)(void*, void*);

    void (* move_constructor)(void*, void*);

    void (* destroy)(void*);
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

typedef void (* ForEachArrayCallback)(struct Job*, JobForEachArrayArg*);
typedef void (* JobEvent)(struct Job*, struct World*, TasksCount, JobSize, enum JobRunMode);
typedef struct {
    ForEachArrayCallback callback;
    JobEvent on_job_begin;
    JobEvent on_job_end;
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

typedef void (* SystemEvent)(struct World*, void* user_data);
typedef struct {
    const char* name;
    void* user_data;

    void (* free_user_data)(void* user_data);

    SystemEvent on_create;

    void (* on_configure)(struct World*, SystemConfig*, void* user_data);

    SystemEvent on_start;
    SystemEvent on_stop;
    SystemEvent on_update;
    SystemEvent on_pause;
    SystemEvent on_resume;
    SystemEvent on_destroy;
} SystemDescriptor;

MUSTACHE_EXPORT struct World* createWorld(WorldId id);
MUSTACHE_EXPORT void updateWorld(struct World* world);
MUSTACHE_EXPORT void clearWorldEntities(struct World*);
MUSTACHE_EXPORT void destroyWorld(struct World*);
MUSTACHE_EXPORT ComponentId registerComponent(TypeInfo info);
MUSTACHE_EXPORT bool hasComponent(struct World* world, Entity entity, ComponentId component_id);
MUSTACHE_EXPORT ComponentPtr assignComponent(struct World* world, Entity entity, ComponentId component_id, bool skip_constructor);
MUSTACHE_EXPORT ComponentPtr getComponent(struct World* world, Entity entity, ComponentId component_id, bool is_const);
MUSTACHE_EXPORT void removeComponent(struct World* world, Entity entity, ComponentId component_id);
MUSTACHE_EXPORT struct Job* makeJob(JobDescriptor info);
MUSTACHE_EXPORT void runJob(struct Job* job, struct World* world, enum JobRunMode mode);
MUSTACHE_EXPORT void destroyJob(struct Job*);
MUSTACHE_EXPORT Entity createEntity(struct World* world, struct Archetype* archetype);
MUSTACHE_EXPORT void createEntityGroup(struct World* world, struct Archetype* archetype, Entity* ptr, uint32_t count);
MUSTACHE_EXPORT void destroyEntities(struct World* world, Entity* entities, uint32_t count, bool now);
MUSTACHE_EXPORT struct Archetype* getArchetype(struct World* world, ComponentMask mask);
MUSTACHE_EXPORT struct Archetype* getArchetypeByBitsetMask(struct World* world, uint64_t mask);
MUSTACHE_EXPORT struct CSystem* createSystem(struct World* world, const SystemDescriptor* descriptor);
MUSTACHE_EXPORT void addSystem(struct World* world, struct CSystem* system);
#ifdef __cplusplus
}
#endif

#endif //MUSTACHE_C_API_H
