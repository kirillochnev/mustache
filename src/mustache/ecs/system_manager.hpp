#pragma once

#include <mustache/utils/uncopiable.hpp>
#include <vector>

namespace mustache {
    class World;

    class SystemManager : public Uncopiable {
    public:
        explicit SystemManager(World& world);
        void update();

    private:
//        std::vector<
    };
}
