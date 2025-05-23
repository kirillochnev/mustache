#include "system.hpp"

#include <mustache/utils/profiler.hpp>

#include <mustache/ecs/job.hpp>

using namespace mustache;

// TODO: make this functions execute all patch from current state to target state
void ASystem::checkState(SystemState expected_state) const {
#if MUSTACHE_PROFILER_LVL >= 3
    const auto profiler_msg = name() + " | " + __FUNCTION__;
    MUSTACHE_PROFILER_BLOCK_LVL_3(profiler_msg.c_str());
#endif
    if (state_ != expected_state) {
        throw std::runtime_error("Invalid state");
    }
}

void ASystem::create(World& world) {
#if MUSTACHE_PROFILER_LVL >= 0
    const auto profiler_msg = name() + " | " + __FUNCTION__;
    MUSTACHE_PROFILER_BLOCK_LVL_0(profiler_msg.c_str());
#endif
    checkState(SystemState::kUninit);
    if (state_ != SystemState::kUninit) {
        throw std::runtime_error("Invalid state");
    }
    onCreate(world);
    state_ = SystemState::kInited;
}

void ASystem::configure(World& world, SystemConfig& config) {
#if MUSTACHE_PROFILER_LVL >= 0
    const auto profiler_msg = name() + " | " + __FUNCTION__;
    MUSTACHE_PROFILER_BLOCK_LVL_0(profiler_msg.c_str());
#endif
    checkState(SystemState::kInited);
    onConfigure(world, config);
    state_ = SystemState::kConfigured;
}

void ASystem::start(World& world) {
#if MUSTACHE_PROFILER_LVL >= 0
    const auto profiler_msg = name() + " | " + __FUNCTION__;
    MUSTACHE_PROFILER_BLOCK_LVL_0(profiler_msg.c_str());
#endif
    checkState(SystemState::kConfigured);
    onStart(world);
    state_ = SystemState::kActive;
}

void ASystem::update(World& world) {
#if MUSTACHE_PROFILER_LVL >= 0
    const auto profiler_msg = name() + " | " + __FUNCTION__;
    MUSTACHE_PROFILER_BLOCK_LVL_0(profiler_msg.c_str());
#endif
    checkState(SystemState::kActive);
    onUpdate(world);
}

void ASystem::pause(World& world) {
#if MUSTACHE_PROFILER_LVL >= 0
    const auto profiler_msg = name() + " | " + __FUNCTION__;
    MUSTACHE_PROFILER_BLOCK_LVL_0(profiler_msg.c_str());
#endif
    checkState(SystemState::kActive);
    onPause(world);
    state_ = SystemState::kPaused;
}

void ASystem::stop(World& world) {
#if MUSTACHE_PROFILER_LVL >= 0
    const auto profiler_msg = name() + " | " + __FUNCTION__;
    MUSTACHE_PROFILER_BLOCK_LVL_0(profiler_msg.c_str());
#endif
    checkState(SystemState::kPaused);
    onStop(world);
    state_ = SystemState::kStopped;
}

void ASystem::resume(World& world) {
#if MUSTACHE_PROFILER_LVL >= 0
    const auto profiler_msg = name() + " | " + __FUNCTION__;
    MUSTACHE_PROFILER_BLOCK_LVL_0(profiler_msg.c_str());
#endif
    checkState(SystemState::kPaused);
    onResume(world);
    state_ = SystemState::kActive;
}

void ASystem::destroy(World& world) {
#if MUSTACHE_PROFILER_LVL >= 0
    const auto profiler_msg = name() + " | " + __FUNCTION__;
    MUSTACHE_PROFILER_BLOCK_LVL_0(profiler_msg.c_str());
#endif
    if (state_ == SystemState::kActive) {
        pause(world);
    }
    if (state_ == SystemState::kPaused) {
        stop(world);
    }
    onDestroy(world);
    state_ = SystemState::kUninit;
}

void ASystem::onCreate(World&) {

}

void ASystem::onConfigure(World&, SystemConfig&) {

}

void ASystem::onStart(World&) {

}

void ASystem::onPause(World&) {

}

void ASystem::onStop(World&) {

}

void ASystem::onResume(World&) {

}

void ASystem::onDestroy(World&) {

}
