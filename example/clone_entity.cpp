//
// Created by kirill on 16.04.25.
//
#include <mustache/ecs/world.hpp>
#include <mustache/utils/logger.hpp>
#include <mustache/utils/benchmark.hpp>
#include <mustache/ecs/entity_builder.hpp>
#include <mustache/ecs/component_factory.hpp>

#include <vector>
#include <string>
#include <iostream>

using namespace mustache;

// Simple name tag
struct Name {
    std::string value;
};

// Component to store child entities
struct Children {
    std::vector<Entity> children;
    // Deep clone logic for children list
    static void clone(const CloneDest<Children>& dest, const CloneSource<Children>& src, CloneEntityMap& map, World& world) {
        new(dest.value) Children{};
        dest.value->children.reserve(src.value->children.size());
        for (const auto& child : src.value->children) {
            dest.value->children.push_back(world.entities().clone(child, map));
        }
    }
};

// Component to track the parent entity
struct Parent {
    Entity parent;

    // This function is automatically called **after** the component is copied during entity cloning.
    // It allows you to fix up parent-child relationships that might have been broken due to entity remapping.
    static void afterClone(CloneDest<Parent>& dest, const CloneEntityMap& map, World& world) {
        // Remember the original parent before cloning
        auto prev_parent = dest.value->parent;

        // Try to find the remapped parent entity (i.e., the cloned version if it exists)
        auto new_parent = map.remap(prev_parent);

        // Update the reference to point to the new (possibly same) parent
        dest.value->parent = new_parent;

        // If the parent was cloned and replaced (i.e. remapped),
        // then the clone logic for the parent will already take care of linking children.
        if (prev_parent != new_parent) {
            return;
        }

        // If the parent did NOT change, that means the parent was NOT cloned.
        // This can happen when cloning part of the hierarchy — for example,
        // cloning a wheel while its parent car is not cloned.
        // In such cases, we must manually register the new child in the parent's Children list.
        auto children = world.entities().getComponent<Children>(dest.value->parent);
        if (children == nullptr) {
            return; // Parent has no Children component, so nothing to update
        }

        // Avoid duplicates: only add the new entity if it's not already present
        //const auto find_res = std::find(children->children.begin(), children->children.end(), dest.entity);
        //if (find_res == children->children.end()) {
            children->children.push_back(dest.entity);
        //}
    }
};

// Get readable name of an entity
std::string getName(World& world, Entity e) {
    const auto name = world.entities().getComponent<const Name>(e);
    if (name) {
        return name->value + " (id: " + std::to_string(e.id().toInt()) + ")";
    }
    return "id: " + std::to_string(e.id().toInt());
}

// Recursively print entity hierarchy
void printTree(World& world, Entity entity, const std::string& prefix = "", bool is_last = true) {
    const auto name = world.entities().getComponent<const Name>(entity);

#ifdef _WIN32
    constexpr const char* branch = "+-- ";
    constexpr const char* pipe = "|   ";
    constexpr const char* space = "    ";
#else
    constexpr const char* branch = is_last ? "└─ " : "├─ ";
    constexpr const char* pipe = "│  ";
    constexpr const char* space = "   ";
#endif

    Logger{}.hideContext().info("%s%s%s",
        prefix.c_str(),
        branch,
        name ? name->value.c_str() : ("Entity " + std::to_string(entity.id().toInt())).c_str()
    );

    const auto children = world.entities().getComponent<const Children>(entity);
    if (!children || children->children.empty()) return;

    std::string new_prefix = prefix + (is_last ? space : pipe);
    for (size_t i = 0; i < children->children.size(); ++i) {
        bool last = i == children->children.size() - 1;
        printTree(world, children->children[i], new_prefix, last);
    }
}


// Create example prefab: a car with 4 wheels, each with sub-colliders
Entity createCar(World& world) {
    auto car = world.entities().begin()
            .assign<Name>("Car")
            .assign<Children>()
            .end();

    auto& car_children = world.entities().getComponent<Children>(car)->children;

    for (int i = 0; i < 4; ++i) {
        auto wheel = world.entities().begin()
                .assign<Name>("Wheel_" + std::to_string(i))
                .assign<Parent>(car)
                .assign<Children>()
                .end();

        auto& wheel_children = world.entities().getComponent<Children>(wheel)->children;

        auto collider = world.entities().begin()
                .assign<Name>("WheelCollider")
                .assign<Parent>(wheel)
                .end();
        wheel_children.push_back(collider);

        car_children.push_back(wheel);
    }

    return car;
}

int main() {
    World world;
    auto car = createCar(world);
    auto scene_root = world.entities().begin()
            .assign<Name>("root")
        .assign<Children>(std::vector<Entity>{ car })
    .end();
    //world.entities().assign<Parent>(car, Parent{ scene_root });

    Logger{}.hideContext().info("----------- Before clone -----------");
    printTree(world, scene_root);

    Logger{}.hideContext().info("\n----------- After clone -----------");
    auto cloned_car = world.entities().clone(car);
    printTree(world, scene_root);

    /* expected output:
    ----------- Before clone -----------
    └─ root
       └─ Car
          ├─ Wheel_0
          │  └─ WheelCollider
          ├─ Wheel_1
          │  └─ WheelCollider
          ├─ Wheel_2
          │  └─ WheelCollider
          └─ Wheel_3
             └─ WheelCollider

    ----------- After clone -----------
    └─ root
       ├─ Car
       │  ├─ Wheel_0
       │  │  └─ WheelCollider
       │  ├─ Wheel_1
       │  │  └─ WheelCollider
       │  ├─ Wheel_2
       │  │  └─ WheelCollider
       │  └─ Wheel_3
       │     └─ WheelCollider
       └─ Car
          ├─ Wheel_0
          │  └─ WheelCollider
          ├─ Wheel_1
          │  └─ WheelCollider
          ├─ Wheel_2
          │  └─ WheelCollider
          └─ Wheel_3
             └─ WheelCollider

     */
}
