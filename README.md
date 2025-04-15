# mustache - A fast, modern C++ Entity Component System
[![Build Status](https://github.com/kirillochnev/mustache/workflows/build/badge.svg)](https://github.com/kirillochnev/mustache/actions)

## Why mustache

* [Super fast component iteration](#Performance)
* [Multithreading support out of the box](#Multithreading)
* Fully runtime: you can describe Components and Systems without using templates
* Has C-API and can be used with other programing languages like ex: [mustache-lua](https://github.com/kirillochnev/mustache-lua)
* Has integration with [profiler](#Profiling)
* [Built-in component lifecycle hooks](#Component-hooks)

## Introduction

The entity-component-system (also known as _ECS_) is an architectural pattern
used mostly in game development. For further details:

* [Entity Systems Wiki](http://entity-systems.wikidot.com/)
* [Evolve Your Hierarchy](http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/)
* [ECS on Wikipedia](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)
* [About ECS on github](https://github.com/SanderMertens/ecs-faq)
* [Telegram channel about ECS](https://t.me/ecscomrade)

## Code Example

```cpp
#include <mustache/ecs/ecs.hpp>

int main() {
    
    struct Position {
        float x, y, z;
    };
    struct Velocity {
        float x, y, z;
    };

    mustache::World world;
    for (uint32_t i = 0; i < 1000000; ++i) {
        (void) world.entities().create<Position, Velocity>();
    }

    const auto run_mode = mustache::JobRunMode::kCurrentThread; // or kParallel

    world.entities().forEach([](Position& pos, const Velocity& dir) {
        constexpr float dt = 1.0f / 60.0f;
        pos.x += dt * dir.x;
        pos.y += dt * dir.y;
        pos.z += dt * dir.z;
    }, run_mode);

    return 0;
}
```

## Requirements

To be able to use `mustache`, users must provide a full-featured compiler that supports at least C++17.<br/>
The requirements below are mandatory to compile the tests:

* `CMake` version 3.7 or later.

## Overview

### Entities

An `mustache::Entity` is a class wrapping an opaque `uint64_t` value allocated by the `mustache::EntityManager`.

Each `mustache::Entity` has `id` (identifier of Entity), `version` (to check if Entity is still alive) and `worldId`.

Creating an entity is as simple as:

```cpp
#include <mustache/ecs/ecs.hpp>

mustache::World world;

const auto entity = world.entities().create();
```

And destroying an entity is done with:

```cpp
world.entities().destroy(entity); // to destroy while the next world.update()
world.entities().destroyNow(entity); // to destroy right now
```

### Components (entity data)

The general idea of ECS is to have as little logic in components as possible. All logic should be contained in Systems.

But mustache has very weak requirements for struct / class to be a component.

You must provide the following public methods (trivial or not):

* default constructor
* operator=
* destructor

#### Creating components

As an example, position and direction information might be represented as:

```cpp
struct Position {
    float x, y, z;
};
struct Velocity {
    float x, y, z;
};
```

#### Assigning components to entities

To associate a Component with a previously created Entity:

```cpp
world.entities().assign<Position>(entity, 1.0f, 2.0f, 3.0f); 
```

There are two ways to create an Entity with a given set of Components:

```cpp
world.entities().create<C0, C1, C2>();
```

```cpp
world.entities().begin()
    .assign<C0>(/*args to create component C0*/)
    .assign<C1>(/*args to create component C1*/)
    .assign<C2>(/*args to create component C2*/)
.end();
```

#### Component version control

You may wish to iterate over only changed components. Mustache has a built-in version control system.

Each time you request a non-const component from an entity, the version is updated.

You can configure granularity by:

```cpp
world.entities().addChunkSizeFunction<Component0>(1, 32); // version per chunk of 32
world.entities().addChunkSizeFunction<Component1>(1, 1);  // version per instance
```

#### Component access

```cpp
world.entities().getComponent<C>(entity);       // mutable (and version will be updated)
world.entities().getComponent<const C>(entity); // const access
```

Returns `nullptr` if the component is missing or entity is invalid.

#### Querying entities and their components

```cpp
world.entities().forEach([](Entity entity, Component0& required0, const Component1& required1, const Component2* optional2) {
    // iterate over all entities with required0 and required1
    // optional2 may be nullptr if Component2 is missing
});
```

Alternative: use `PerEntityJob`:

```cpp
struct MySuperJob : public PerEntityJob<MySuperJob> {
    void operator()(const Component0&, Component1&) {
        // iterate over all entities with required0 and required1
    }
};

MySuperJob job;
job.run(world);
```

### Supported function arguments

The function passed to `EntityManager::forEach` or the `operator()` of a `PerEntityJob` can accept the following arguments (in any order):

1. **Components**
    - Passed as reference (`T&` or `const T&`) — required.
    - Passed as pointer (`T*` or `const T*`) — optional, `nullptr` if entity doesn't have the component.
    - If `const`, version tracking is **not** updated.
    - If non-const, version is **updated**.

2. **`Entity`** — the current entity.

3. **`World&`** — world reference.

4. **`JobInvocationIndex`** — provides per-invocation info:
```cpp
struct JobInvocationIndex {
    ParallelTaskId task_index;
    ParallelTaskItemIndexInTask entity_index_in_task;
    ParallelTaskGlobalItemIndex entity_index;
    ThreadId thread_id;
};
```

### Job customization

Jobs that inherit from `BaseJob` (or `PerEntityJob`) can override advanced hooks:

| Function | Description |
|---------|-------------|
| `checkMask` | Mask of components to track changes |
| `updateMask` | Mask of components to mark as updated |
| `name`, `nameCStr` | Job name string |
| `extraArchetypeFilterCheck` | Additional filter for archetypes |
| `extraChunkFilterCheck` | Additional filter per-chunk |
| `applyFilter` | Full override of entity filter logic |
| `taskCount` | Split job into N tasks |
| `onTaskBegin` / `onTaskEnd` | Hook per task |
| `onJobBegin` / `onJobEnd` | Hook before and after whole job |

See the source of `BaseJob` for more details.

### Multithreading

Mustache has built-in multithreading support:

```cpp
job.run(world, JobRunMode::kParallel);
world.entities().forEach(func, JobRunMode::kParallel);
```

---

### Component hooks

Mustache allows defining optional functions for components to control their lifecycle:

| Function         | When it's called                                                                 |
|------------------|----------------------------------------------------------------------------------|
| `static void afterAssign(...)` | After assigning the component to an entity                                |
| `static void beforeRemove(...)` | Before removing the component from an entity                            |
| `static void clone(...)` | When cloning a component during entity clone                         |
| `static void afterClone(...)` | After cloning is done, used for finalization (e.g. remap references) |

These functions are automatically detected and invoked by Mustache.

**Examples:**

- See [`example/component_add_remove_events.cpp`](example/component_add_remove_events.cpp) for `afterAssign` and `beforeRemove` usage.
- See [`example/clone_entity.cpp`](example/clone_entity.cpp) for cloning and `afterClone` use.

### Component dependencies

Automatically assign dependencies when assigning a component:

```cpp
world.entities().addDependency<MainComponent, Component0, Component1>();
```

### Systems

```cpp
struct System2 : public System<System2> {
    void onConfigure(World&, SystemConfig& config) override {
        config.update_after = {"System0", "System1"};
        config.update_before = {"System3"};
    }

    void onUpdate(World&) override {
        // Your logic
    }
};
```

### Events

Define:
```cpp
struct Collision { Entity left, right; };
```

Emit:
```cpp
world.events().post(Collision{first, second});
```

Subscribe:
```cpp
struct DebugSystem : public System<DebugSystem>, Receiver<Collision> {
    void onConfigure(World&, SystemConfig&) override {
        world.events().subscribe<Collision>(this);
    }
    void onEvent(const Collision &event) override {}
};
```

Lambda-style:
```cpp
auto sub = event_manager.subscribe<Collision>([](const Collision& event){ });
```

## Integration

```cmake
add_subdirectory(third_party/mustache)
target_link_libraries(${PROJECT_NAME} mustache)
```

## Performance

Create time:
![Create time](doc/create.png "Benchmark Results: Create entities")

Update time:
![Update time](doc/update.png "Benchmark Results: Update entities")

## Profiling

Enable with:
```bash
cmake -DMUSTACHE_BUILD_WITH_EASY_PROFILER=ON
```

Then use EasyProfiler viewer:

![Profiling result](doc/test_frofiling_result.png "Tests profiling result")

