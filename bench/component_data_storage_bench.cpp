#include <mustache/ecs/world.hpp>
#include <mustache/utils/benchmark.hpp>
#include <mustache/utils/dispatch.hpp>
#include <mustache/ecs/task_view.hpp>

void testGlobalIndex() {
    constexpr static uint32_t kNumItems = 1000000;
    struct C0 {
        uint32_t value;
    };
    using namespace mustache;
    World world{WorldId::make(0)};
    auto& entities = world.entities();

    auto& arch = entities.getArchetype<C0>();
    for (uint32_t i = 0; i < kNumItems; ++i) {
        (void) entities.create(arch);
    }

    Benchmark benchmark;
    bool status = true;
    benchmark.add([&arch, &status] {
        auto view = arch.getElementView(ArchetypeEntityIndex::make(0));
        for (uint32_t i = 0; i < kNumItems; ++i) {
            if (view.globalIndex().toInt() != i) {
                status = false;
            }
            ++view;
        }
    }, 1000);
    benchmark.show();
    if (!status) {
        throw std::runtime_error("invalid global index");
    }
}
using namespace mustache;

namespace {
    DefaultWorldFilterResult filter_result;

    template<typename... ARGS>
    void callMePerElement(ARGS&&...) {}

    template<typename... ARGS>
    void singleTask(ARGS&&...) {}

    void runParallel(World&, Dispatcher& dispatcher, uint32_t task_count) {
        (void )dispatcher;
        (void )task_count;
        /*for (const auto &info : TaskView{filter_result, task_count}) {
            dispatcher.addParallelTask([info] {
                singleTask(info.task_id, info.first_archetype, info.first_entity, info.current_task_size);
            });
        }
        dispatcher.waitForParallelFinish();*/
        /*for (const auto& task : splitByTasks(filter_result, task_count)) {
            dispatcher.addParallelTask([task] {
                for (auto element : task.elementView()) {
                    callMePerElement(element);
                }
            });
        }

        dispatcher.waitForParallelFinish();*/
    }
}

void POC() {
    struct C0 {};
    World world{WorldId::make(0)};
    auto& entities = world.entities();
    auto& archetype = entities.getArchetype<C0>();
    for (uint32_t i = 0; i < 3 * 16; ++i) {
        (void) entities.create(archetype);
    }

    filter_result.apply(world);
    /*filter_result.total_entity_count = 16 * 1024;
    DefaultWorldFilterResult::ArchetypeFilterResult archetype_filter_result;
    archetype_filter_result.archetype = &archetype;
    archetype_filter_result.entities_count = 16 * filter_result.total_entity_count;
    archetype_filter_result.blocks.push_back(DefaultWorldFilterResult::EntityBlock{0, 16 });
    archetype_filter_result.blocks.push_back(DefaultWorldFilterResult::EntityBlock{16, 32 });
    archetype_filter_result.blocks.push_back(DefaultWorldFilterResult::EntityBlock{32, 16 * 1024 });
    filter_result.filtered_archetypes.push_back(archetype_filter_result);*/
    ArchetypeView iterator{filter_result, TaskArchetypeIndex::make(0),
                           ArchetypeEntityIndex::make(3), 64};
    for (auto& it : iterator) {
        Entity* entities_ptr = it.getEntity();
        for (uint32_t i = 0; i < it.arraySize(); ++i) {
            std::cout << entities_ptr[i].value << std::endl;
        }
        std::cout << "-----------------------------------------" << std::endl;
    }
}
