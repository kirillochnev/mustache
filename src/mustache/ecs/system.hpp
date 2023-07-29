#pragma once

#include <cstdint>
#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/type_info.hpp>

#include <set>

namespace mustache {

    class World;

    struct MUSTACHE_EXPORT SystemConfig {
        template <typename... ARGS>
        void updateBefore() {
            update_before.insert(ARGS::systemName()...);
        }
        template <typename... ARGS>
        void updateAfter() {
            update_after.insert(ARGS::systemName()...);
        }
        std::set<std::string> update_before;
        std::set<std::string> update_after;
        std::string update_group = "";
        int32_t priority = 0;
    };

    enum class SystemState : uint32_t {
        kUninit = 0, // After constructor or onDestroy call
        kInited, // After onCreate call
        kConfigured, // After onConfigure call
        kStopped, // After onConfigure or onStop
        kActive, // After onStart or onResume call
        kPaused // After onPause call
    };
    /**
     * System life cycle:
     *  onCreate() -> onConfigure()
     *  onCreate() -> onOnDestroy()
     *
     *  onConfigure() -> onStart()
     *  onConfigure() -> onOnDestroy()
     *
     *  onStart() -> onUpdate()
     *  onStart() -> onPause()
     *
     *  onUpdate() -> onUpdate()
     *  onUpdate() -> onPause()
     *
     *  onPause() -> onResume()
     *  onPause() -> onStop()
     *
     *  onResume() -> onUpdate()
     *  onResume() -> onPause()
     *
     *  onStop() -> onStart()
     *  onStop() -> onDestroy()
     */
    class MUSTACHE_EXPORT ASystem : Uncopiable {
    public:
        virtual ~ASystem() = default;

        void create(World&);
        void configure(World&, SystemConfig&);
        void start(World&);
        void update(World&);
        void pause(World&);
        void stop(World&);
        void resume(World&);
        void destroy(World&);

        [[nodiscard]] SystemState state() const noexcept {
            return state_;
        }
        virtual std::string name() const noexcept {
            return nameCStr();
        }

        virtual const char* nameCStr() const noexcept = 0;

    protected:
        virtual void onCreate(World&); // after constructor call
        virtual void onConfigure(World&, SystemConfig&); // after on create and World::Init()
        virtual void onStart(World&); // before first update, after configure
        virtual void onUpdate(World&) = 0; // after start
        virtual void onPause(World&); // after start before stop
        virtual void onStop(World&); // after pause
        virtual void onResume(World&); // after pause
        virtual void onDestroy(World&); // after pause or create\configure

        void checkState(SystemState expected_state) const;
        SystemState state_ = SystemState::kUninit;
    };

    template<typename _SystemType>
    class System : public ASystem {
    public:
        static const std::string& systemName() noexcept {
            static const std::string& result = type_name<_SystemType>();
            return result;
        }

        const char* nameCStr() const noexcept override {
            static auto result = systemName().c_str();
            return result;
        }
    };
}
