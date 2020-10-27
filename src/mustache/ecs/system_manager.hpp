#pragma once

#include <mustache/utils/uncopiable.hpp>
#include <memory>

namespace mustache {
    class World;
    class ASystem;

    class SystemManager : public Uncopiable {
    public:
        using SystemPtr = std::shared_ptr<ASystem>;

        explicit SystemManager(World& world);
        ~SystemManager();

        void addSystem(SystemPtr system);

        template<typename _SystemType, typename... _ARGS>
        SystemPtr addSystem(_ARGS&&... args) {
            auto system_ptr = std::make_shared<_SystemType>(std::forward<_ARGS>(args)...);
            addSystem(system_ptr);
            return system_ptr;
        }

        void update(World&);
        void init();

    private:
        struct Data;
        std::unique_ptr<Data> data_;
    };
}
