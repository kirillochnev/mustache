#pragma once

#include <mustache/utils/index_like.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <cstdint>
#include <mustache/utils/memory_manager.hpp>

namespace mustache {

    class World : public Uncopiable {
    public:
        explicit World(WorldId id);

        [[nodiscard]] EntityManager& entities() noexcept {
            return entities_;
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
            return memory_manager_;
        }
    private:
        WorldId id_;
        WorldVersion version_ = WorldVersion::make(0u);
        EntityManager entities_;
        MemoryManager memory_manager_;
    };
}
