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

        void onCreate(World&);
        void onConfigure(World&, SystemConfig&);
        void onStart(World&);
        void onUpdate(World&);
        void onPause(World&);
        void onStop(World&);
        void onResume(World&);
        void onDestroy(World&);

        virtual std::string name() const noexcept = 0;

    protected:
        virtual void onCreateImpl(World&);
        virtual void onConfigureImpl(World&, SystemConfig&);
        virtual void onStartImpl(World&);
        virtual void onUpdateImpl(World&) = 0;
        virtual void onPauseImpl(World&);
        virtual void onStopImpl(World&);
        virtual void onResumeImpl(World&);
        virtual void onDestroyImpl(World&);

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