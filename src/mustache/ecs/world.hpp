#pragma once

#include <mustache/utils/dispatch.hpp>
#include <mustache/utils/index_like.hpp>
#include <mustache/utils/memory_manager.hpp>

#include <mustache/ecs/world_storage.hpp>
#include <mustache/ecs/event_manager.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/system_manager.hpp>

#include <cstdint>

namespace mustache {

    // Shared data
    struct MUSTACHE_EXPORT WorldContext {
        std::shared_ptr<MemoryManager> memory_manager;
        std::shared_ptr<Dispatcher> dispatcher;
        std::shared_ptr<EventManager> events;
    };

    class MUSTACHE_EXPORT World : public Uncopiable {
    public:
        [[nodiscard]] static WorldId nextWorldId() noexcept;

        World(const WorldContext& context = {}, WorldId id = nextWorldId());
        World(WorldId id):
            World(WorldContext{}, id) {

        }

        ~World();

        [[nodiscard]] EntityManager& entities() noexcept {
            return entities_;
        }

        [[nodiscard]] SystemManager& systems() noexcept {
            if (!systems_) {
                systems_ = std::make_unique<SystemManager>(*this);
            }
            return *systems_;
        }

        [[nodiscard]] Dispatcher& dispatcher() noexcept {
            if (!context_.dispatcher) {
                context_.dispatcher = std::make_shared<Dispatcher>();
            }
            return *context_.dispatcher;
        }

        [[nodiscard]] MemoryManager& memoryManager() noexcept {
            if (!context_.memory_manager) {
                context_.memory_manager = std::make_shared<MemoryManager>();
            }
            return *context_.memory_manager;
        }


        [[nodiscard]] EventManager& events() noexcept {
            if (!context_.events) {
                context_.events = std::make_shared<EventManager>(memoryManager());
            }
            return *context_.events;
        }

        void init();
        void update();
        void pause();
        void resume();

        [[nodiscard]] constexpr WorldId id() const noexcept {
            return id_;
        }
        [[nodiscard]] WorldVersion version() const noexcept {
            return version_;
        }

        [[nodiscard]] WorldStorage& storage() noexcept {
            return world_storage_;
        }

        void incrementVersion() noexcept {
            ++version_;
        }
    private:
        WorldId id_;
        WorldContext context_;
        std::unique_ptr<SystemManager> systems_;
        EntityManager entities_;
        WorldStorage world_storage_;
        WorldVersion version_ = WorldVersion::make(0u);
    };
}
