#pragma once

#include <mustache/utils/uncopiable.hpp>
#include <mustache/utils/type_info.hpp>

#include <set>

namespace mustache {

    class World;
    class APerEntityJob;

    struct SystemConfig {
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
    };

    enum class SystemState : uint32_t {
        kUninit = 0, // After constructor or onDestroy call
        kConfigured, // After onConfigure call
        kInited, // After onCreate call
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
    struct ASystem : Uncopiable {
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
        virtual std::string name() const noexcept = 0;

    protected:
        virtual void onCreate(World&);
        virtual void onConfigure(World&, SystemConfig&);
        virtual void onStart(World&);
        virtual void onUpdate(World&) = 0;
        virtual void onPause(World&);
        virtual void onStop(World&);
        virtual void onResume(World&);
        virtual void onDestroy(World&);

        void checkState(SystemState expected_state);
        SystemState state_;
    };

    template<typename _SystemType>
    class System : public ASystem {
        std::string name() const noexcept override {
            return type_name<_SystemType>();
        }
    };


}