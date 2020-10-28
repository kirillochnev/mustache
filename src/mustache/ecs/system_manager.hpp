#pragma once

#include <mustache/utils/uncopiable.hpp>
#include <memory>
#include <string>

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

        void update();
        void init();

        int32_t getGroupPriority(const std::string& group_name) const noexcept;
        void setGroupPriority(const std::string& group_name, int32_t priority) noexcept;
    private:
        void reorderSystems();
        struct Data;
        std::unique_ptr<Data> data_;
    };
}
