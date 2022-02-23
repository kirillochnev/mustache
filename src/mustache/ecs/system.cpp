#include "system.hpp"

#include <mustache/utils/profiler.hpp>

#include <mustache/ecs/job.hpp>

using namespace mustache;

// TODO: make this functions execute all patch from current state to target state
void ASystem::checkState(SystemState expected_state) const {
    MUSTACHE_PROFILER_BLOCK_LVL_3((name() + " | " + __FUNCTION__ ).c_str());

    if (state_ != expected_state) {
        throw std::runtime_error("Invalid state");
    }
}

void ASystem::create(World& world) {
    MUSTACHE_PROFILER_BLOCK_LVL_0((name() + " | " + __FUNCTION__ ).c_str());

    checkState(SystemState::kUninit);
    if (state_ != SystemState::kUninit) {
        throw std::runtime_error("Invalid state");
    }
    onCreate(world);
    state_ = SystemState::kInited;
}

void ASystem::configure(World& world, SystemConfig& config) {
    MUSTACHE_PROFILER_BLOCK_LVL_0((name() + " | " + __FUNCTION__ ).c_str());

    checkState(SystemState::kInited);
    onConfigure(world, config);
    state_ = SystemState::kConfigured;
}

void ASystem::start(World& world) {
    MUSTACHE_PROFILER_BLOCK_LVL_0((name() + " | " + __FUNCTION__ ).c_str());

    checkState(SystemState::kConfigured);
    onStart(world);
    state_ = SystemState::kActive;
}

void ASystem::update(World& world) {
    MUSTACHE_PROFILER_BLOCK_LVL_0((name() + " | " + __FUNCTION__ ).c_str());

    checkState(SystemState::kActive);
    onUpdate(world);
}

void ASystem::pause(World& world) {
    MUSTACHE_PROFILER_BLOCK_LVL_0((name() + " | " + __FUNCTION__ ).c_str());

    checkState(SystemState::kActive);
    onPause(world);
    state_ = SystemState::kPaused;
}

void ASystem::stop(World& world) {
    MUSTACHE_PROFILER_BLOCK_LVL_0((name() + " | " + __FUNCTION__ ).c_str());

    checkState(SystemState::kPaused);
    onStop(world);
    state_ = SystemState::kStopped;
}

void ASystem::resume(World& world) {
    MUSTACHE_PROFILER_BLOCK_LVL_0((name() + " | " + __FUNCTION__ ).c_str());

    checkState(SystemState::kPaused);
    onResume(world);
    state_ = SystemState::kActive;
}

void ASystem::destroy(World& world) {
    MUSTACHE_PROFILER_BLOCK_LVL_0((name() + " | " + __FUNCTION__ ).c_str());

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
