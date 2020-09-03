#pragma once

#include <mustache/utils/index_like.hpp>
#include <mustache/ecs/entity_manager.hpp>
#include <cstdint>

namespace mustache {

    class World : public Uncopiable {
    public:
        explicit World(WorldId id);

        [[nodiscard]] MUSTACHE_INLINE EntityManager& entities() noexcept {
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

        [[nodiscard]] MUSTACHE_INLINE  constexpr WorldId id() const noexcept {
            return id_;
        }
        [[nodiscard]] MUSTACHE_INLINE uint32_t version() const noexcept {
            return version_;
        }
    private:
        WorldId id_;
        uint32_t version_{0u};
        EntityManager entities_;
    };
}
