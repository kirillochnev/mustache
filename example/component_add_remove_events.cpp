// Example: assigning and removing various types of components
// - Some use built-in helpers (ComponentWithNotify)
// - Some define afterAssign / beforeRemove manually
// - Some have no special behavior
//
// The system emits both type-safe and generic (common) events.

#include <mustache/ecs/world.hpp>
#include <mustache/ecs/event_manager.hpp>
#include <mustache/ext/add_remove_events.hpp>
#include <cstdlib>

using namespace mustache;
// handle afterAssign and beforeRemove
struct ComponentWithAssignRemoveEvent : public ext::ComponentWithNotify<ComponentWithAssignRemoveEvent> {

};
// handle afterAssign; beforeRemove not handled
struct ComponentWithAssignEvent  : public ext::ComponentWithNotify<ComponentWithAssignEvent, true, false> {

};
// handle beforeRemove; afterAssign not handled
struct ComponentWithRemoveEvent  : public ext::ComponentWithNotify<ComponentWithAssignEvent, false, true> {

};
// beforeRemove, afterAssign not handled
struct ComponentWithNoEvent {

};

struct ComponentWithCustomHandles {
    // NOTE: arguments self, entity, world are optional and may come in any order, all function will be called via
    // mustache::invoke what analise function signature and place arguments in right order and reject redundant
    static void afterAssign(ComponentWithCustomHandles* self, Entity entity, World& world) {
        Logger{}.hideContext().info("Hello, from ComponentWithCustomHandles it just was added to entity %d in the world: %d",
                      entity.id().toInt(), world.id().toInt());
    }

    static void beforeRemove(Entity entity) {
        Logger{}.hideContext().info("Hello, from ComponentWithCustomHandles it just was removed from entity %d",
                      entity.id().toInt());
    }
};

struct ComponentWithCommonEvents_0 : public ext::ComponentWithNotifyCommon<ComponentWithCommonEvents_0> {

};
struct ComponentWithCommonEvents_1 : public ext::ComponentWithNotifyCommon<ComponentWithCommonEvents_1> {

};
struct ComponentWithCommonEvents_2 : public ext::ComponentWithNotifyCommon<ComponentWithCommonEvents_2> {

};

int main() {
    World world;
    const auto event_handler = [](const auto& event) { // auto lambda can handle any event, but you always may provide typed one
        using EventType = typename std::remove_reference<decltype(event)>::type;
        using ComponentType = typename EventType::ComponentType;
        constexpr bool is_assign_event = std::is_same_v<const ext::ComponentAssignEvent<ComponentType>, EventType >;
        const char* event_str = (is_assign_event ? "added to" : "removed from");
        mustache::Logger{}.hideContext().info("Component %s was %s  entity: %d",
                                              mustache::type_name<ComponentType>().c_str(),
                                              event_str,
                                              event.entity.id().toInt());
    };

    const auto event_common_handler = [](const auto& event) {
        using EventType = typename std::remove_reference<decltype(event)>::type;
        const auto info = mustache::ComponentFactory::instance().componentInfo(event.component_id);
        constexpr bool is_assign_event = std::is_same_v<const ext::CommonComponentAssignedEvent, EventType >;
        const char* event_str = (is_assign_event ? "added to" : "removed from");
        mustache::Logger{}.hideContext().info("Component %s was %s  entity: %d",
                                              info.name.c_str(),
                                              event_str,
                                              event.entity.id().toInt());
    };

    std::vector<std::shared_ptr<void> > subscribes { // std::shared_ptr<void> can safely store any subscribers
        world.events().subscribe<ext::ComponentAssignEvent<ComponentWithAssignRemoveEvent> >(event_handler),
        world.events().subscribe<ext::ComponentRemovedEvent<ComponentWithAssignRemoveEvent> >(event_handler),

        world.events().subscribe<ext::ComponentAssignEvent<ComponentWithAssignEvent> >(event_handler),
        world.events().subscribe<ext::ComponentRemovedEvent<ComponentWithAssignEvent> >(event_handler),

        world.events().subscribe<ext::ComponentAssignEvent<ComponentWithRemoveEvent> >(event_handler),
        world.events().subscribe<ext::ComponentRemovedEvent<ComponentWithRemoveEvent> >(event_handler),

        world.events().subscribe<ext::ComponentAssignEvent<ComponentWithNoEvent> >(event_handler),
        world.events().subscribe<ext::ComponentRemovedEvent<ComponentWithNoEvent> >(event_handler),

        world.events().subscribe<ext::ComponentAssignEvent<ComponentWithCustomHandles> >(event_handler),
        world.events().subscribe<ext::ComponentRemovedEvent<ComponentWithCustomHandles> >(event_handler),


        world.events().subscribe<ext::CommonComponentAssignedEvent>(event_common_handler),
        world.events().subscribe<ext::CommonComponentRemovedEvent>(event_common_handler)

    };

    const std::vector<ComponentId> components {
        mustache::ComponentFactory::instance().registerComponent<ComponentWithAssignRemoveEvent>(),
        mustache::ComponentFactory::instance().registerComponent<ComponentWithAssignEvent>(),
        mustache::ComponentFactory::instance().registerComponent<ComponentWithRemoveEvent>(),
        mustache::ComponentFactory::instance().registerComponent<ComponentWithNoEvent>(),
        mustache::ComponentFactory::instance().registerComponent<ComponentWithCustomHandles>(),
        mustache::ComponentFactory::instance().registerComponent<ComponentWithCommonEvents_0>(),
        mustache::ComponentFactory::instance().registerComponent<ComponentWithCommonEvents_1>(),
        mustache::ComponentFactory::instance().registerComponent<ComponentWithCommonEvents_2>(),
    };

    auto entity = world.entities().create();
    Logger{}.hideContext().info("=================== Assigning all components ===================");
    for (auto id : components) {
        world.entities().assign(entity, id);
    }
    Logger{}.hideContext().info("\n=================== Removing all components ===================");

    for (auto id : components) {
        world.entities().removeComponent(entity, id);
    }


    /* expected output:

    =================== Assigning all components ===================
    Component ComponentWithAssignRemoveEvent was added to  entity: 0
    Component ComponentWithAssignEvent was added to  entity: 0
    Hello, from ComponentWithCustomHandles it just was added to entity 0 in the world: 0
    Component ComponentWithCommonEvents_0 was added to  entity: 0
    Component ComponentWithCommonEvents_1 was added to  entity: 0
    Component ComponentWithCommonEvents_2 was added to  entity: 0

    =================== Removing all components ===================
    Component ComponentWithAssignRemoveEvent was removed from  entity: 0
    Component ComponentWithAssignEvent was removed from  entity: 0
    Hello, from ComponentWithCustomHandles it just was removed from entity 0
    Component ComponentWithCommonEvents_0 was removed from  entity: 0
    Component ComponentWithCommonEvents_1 was removed from  entity: 0
    Component ComponentWithCommonEvents_2 was removed from  entity: 0


     * */
}
