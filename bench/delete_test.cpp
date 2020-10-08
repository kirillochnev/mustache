#include <mustache/ecs/entity_manager.hpp>
#include <mustache/ecs/world.hpp>

#include <map>
#include <mustache/utils/logger.hpp>
#include <cstdarg>

namespace {
    std::map<void*, std::string> created_components;

    template<size_t _I>
    struct ComponentWithCheck {
        ComponentWithCheck() {
            created_components.emplace(this, "ComponentWithCheck" + std::to_string(_I));
        }
        ~ComponentWithCheck() {
            created_components.erase(this);
        }
    };
    template<size_t>
    struct PodComponent {

    };

    struct UnusedComponent {

    };

    struct FileLogger : public mustache::LogWriter {
        FILE* file_ = fopen("out.log", "w");
        virtual void onMessage(const Context& ctx, mustache::LogLevel lvl, std::string str, ...) override {
            /*if (ctx.show_context_) {
                fprintf(file_, "%s: ", toStr(lvl));
            }*/
            (void) ctx;
            (void) lvl;
            va_list args;
            va_start (args, str);
            vfprintf(file_, str.c_str(), args);
            va_end (args);
            /*if (ctx.show_context_) {
                fprintf(file_, " | at: %s:%d\n", ctx.file, ctx.line);
            } else */{
                fprintf(file_, "\n");
            }
            fflush(file_);
        }
    };
//    LogWriter::setActive(std::make_shared<FileLogger>());



#define ASSERT_FALSE(exp) if (exp) throw std::runtime_error(std::string (__FILE__) + ":" + std::to_string(__LINE__))
#define ASSERT_TRUE(exp) if (!(exp)) throw std::runtime_error(std::string (__FILE__) + ":" + std::to_string(__LINE__))
#define ASSERT_EQ(a, b) if ((a) != (b)) throw std::runtime_error(std::string (__FILE__) + ":" + std::to_string(__LINE__))
}
struct C0 {
    C0() = default;
    C0(uint64_t v):
            ptr{new uint64_t (v)} {

    }
    std::unique_ptr<uint64_t> ptr;
    bool isMatch(uint64_t v) const noexcept {
        if (!(ptr && v == *ptr)) {
            return false;
        }
        return ptr && v == *ptr;
    }
};

struct C1 {
    mustache::Entity entity;
};

struct C2 : public PodComponent<16 * 1024>{};



void remove_component_1() {
    ASSERT_EQ(created_components.size(), 0);
    {
        static uint32_t num_0 = 0;
        static uint32_t num_1 = 0;
        struct UIntComponent0 {
            uint32_t value = 777 + (++num_0);
        };
        struct UIntComponent1 {
            uint32_t value = 999 + (++num_1);
        };
        mustache::World world{mustache::WorldId::make(0)};
        auto &entities = world.entities();

        for (uint32_t i = 0; i < 100; ++i) {
            auto e = entities.create();

            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
            entities.assign<UIntComponent0>(e);
            entities.assign<UIntComponent1>(e);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));

            entities.assign<UnusedComponent>(e);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));

            entities.removeComponent<UIntComponent0>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));

            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 999 + num_1);

            entities.removeComponent<UIntComponent0>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 999 + num_1);

            entities.removeComponent<UIntComponent1>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));

            entities.assign<UIntComponent0>(e, 12345u);
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 12345u);
        }

        auto e = entities.create();
        for (uint32_t i = 0; i < 100; ++i) {

            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
            entities.assign<UIntComponent0>(e, UIntComponent0{777});
            entities.assign<UIntComponent1>(e, UIntComponent1{543});
            ASSERT_TRUE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent0>(e)->value, 777);
            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 543);

            entities.removeComponent<UIntComponent0>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 543);

            entities.removeComponent<UIntComponent0>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_TRUE(entities.hasComponent<UIntComponent1>(e));
            ASSERT_EQ(entities.getComponent<UIntComponent1>(e)->value, 543);

            entities.removeComponent<UIntComponent1>(e);
            ASSERT_FALSE(entities.hasComponent<UIntComponent0>(e));
            ASSERT_FALSE(entities.hasComponent<UIntComponent1>(e));
        }
    }
    ASSERT_EQ(created_components.size(), 0);
}

void remove_component_2() {
    using namespace mustache;

    World world{WorldId::make(0)};
    auto& entities = world.entities();

    std::vector<Entity> entities_arr;
    Logger{}.info("---------------------------------------------------------------------------------------");

    for (uint32_t i = 0; i < 1024; ++i) {
        auto e = entities.create();
        entities.assign<C0>(e, e.value);
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));

        entities.assign<C1>(e, e);
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.hasComponent<C1>(e));
        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));
        ASSERT_EQ(entities.getComponent<C1>(e)->entity, e);

        entities.assign<C2>(e);
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.hasComponent<C1>(e));
        ASSERT_TRUE(entities.hasComponent<C2>(e));

        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));
        ASSERT_EQ(entities.getComponent<C1>(e)->entity, e);
        entities_arr.push_back(e);
    }

    for (auto e : entities_arr) {
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.hasComponent<C1>(e));
        ASSERT_TRUE(entities.hasComponent<C2>(e));

        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));
        ASSERT_EQ(entities.getComponent<C1>(e)->entity, e);
    }

    Logger{}.info("---------------------------------------------------------------------------------------");
    for (uint32_t i = 0; i < 1024; ++i) {
        auto e = entities_arr[i];
        ASSERT_TRUE(entities.hasComponent<C0>(e));
        ASSERT_TRUE(entities.hasComponent<C1>(e));
        ASSERT_TRUE(entities.hasComponent<C2>(e));

//        std::cout << entities.getComponent<C1>(e)->entity.value << " vs " << e.value << std::endl;
        ASSERT_EQ(entities.getComponent<C1>(e)->entity, e);
        ASSERT_TRUE(entities.getComponent<C0>(e)->isMatch(e.value));
        if (i % 2 == 0) {
            entities.removeComponent<C0>(e);
            ASSERT_FALSE(entities.hasComponent<C0>(e));
            ASSERT_TRUE(entities.hasComponent<C1>(e));
            ASSERT_TRUE(entities.hasComponent<C2>(e));
        }
        if (i % 3 == 0) {
            entities.removeComponent<C1>(e);
            ASSERT_FALSE(entities.hasComponent<C1>(e));
            ASSERT_TRUE(entities.hasComponent<C2>(e));
        }
        if (i % 4 == 0) {
            entities.removeComponent<C2>(e);
            ASSERT_FALSE(entities.hasComponent<C0>(e));
            ASSERT_FALSE(entities.hasComponent<C2>(e));
        }
    }
}