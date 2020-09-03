#include "world.hpp"

using namespace mustache;

World::World(mustache::WorldId id):
    id_{id},
    entities_{*this} {

}
