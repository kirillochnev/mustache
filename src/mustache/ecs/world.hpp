#pragma once

#include <mustache/utils/index_like.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <mustache/utils/memory_manager.hpp>
#include <mustache/utils/dispatch.hpp>

#include <cstdint>

namespace mustache {

    struct WorldContext {
        std::shared_ptr<MemoryManager> memory_manager;
        std::shared_ptr<Dispatcher> dispatcher;
    };

    class World : public Uncopiable {
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

        [[nodiscard]] Dispatcher& dispatcher() noexcept {
            if (!context_.dispatcher) {
                context_.dispatcher = std::make_shared<Dispatcher>();
            }
            return *context_.dispatcher;
        }

        /*[[nodiscard]] SystemManager& systems() noexcept {
            return systems_;
        }
        [[nodiscard]] EventManager& events() noexcept {
            return events_;
        }*/

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
        [[nodiscard]] MemoryManager& memoryManager() noexcept {
            if (!context_.memory_manager) {
                context_.memory_manager = std::make_shared<MemoryManager>();
            }
            return *context_.memory_manager;
        }
    private:
        WorldId id_;
        WorldContext context_;
        EntityManager entities_;
        WorldVersion version_ = WorldVersion::make(0u);
    };
}
