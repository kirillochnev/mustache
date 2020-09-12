#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/world.hpp>
#include <gtest/gtest.h>


TEST(Entity, reset) {
    for (uint32_t i = 0; i < 1024; ++i) {
        uint32_t entity_id_value = rand() % 2048;
        uint32_t entity_version_value = rand() % 2048;
        uint32_t entity_world_id_value = rand() % 128;
        mustache::WorldId world_id = mustache::WorldId::make(entity_world_id_value);
        mustache::EntityId entity_id = mustache::EntityId::make(entity_id_value);
        mustache::EntityVersion version = mustache::EntityVersion::make(entity_version_value);
        ASSERT_EQ(entity_id.toInt(), entity_id_value);
        ASSERT_EQ(world_id.toInt(), entity_world_id_value);
        ASSERT_EQ(version.toInt(), entity_version_value);
        {
            mustache::Entity entity;
            entity.reset(entity_id);
            ASSERT_EQ(entity.id(), entity_id);
        }
        {
            mustache::Entity entity;
            entity.reset(entity_id, version);
            ASSERT_EQ(entity.id(), entity_id);
            ASSERT_EQ(entity.version(), version);
        }
        {
            mustache::Entity entity;
            entity.reset(entity_id, version, world_id);
            ASSERT_EQ(entity.id(), entity_id);
            ASSERT_EQ(entity.version(), version);
            ASSERT_EQ(entity.worldId(), world_id);
        }
    }
}