#include "system.hpp"
#include <mustache/ecs/job.hpp>

using namespace mustache;

// TODO: make this functions execute all patch from current state to target state

void ASystem::checkState(SystemState expected_state) {
    if (state_ != expected_state) {
        throw std::runtime_error("Invalid state");
    }
}

void ASystem::create(World& world) {
    checkState(SystemState::kUninit);
    if (state_ != SystemState::kUninit) {
        throw std::runtime_error("Invalid state");
    }
    onCreate(world);
    state_ = SystemState::kInited;
}

void ASystem::configure(World& world, SystemConfig& config) {
    checkState(SystemState::kInited);
    onConfigure(world, config);
    state_ = SystemState::kConfigured;
}

void ASystem::start(World& world) {
    checkState(SystemState::kConfigured);
    onStart(world);
    state_ = SystemState::kActive;
}

void ASystem::update(World& world) {
    checkState(SystemState::kActive);
    onUpdate(world);
}

void ASystem::pause(World& world) {
    checkState(SystemState::kActive);
    onPause(world);
    state_ = SystemState::kPaused;
}

void ASystem::stop(World& world) {
    checkState(SystemState::kPaused);
    onStop(world);
    state_ = SystemState::kStopped;
}

void ASystem::resume(World& world) {
    checkState(SystemState::kPaused);
    onResume(world);
    state_ = SystemState::kActive;
}

void ASystem::destroy(World& world) {
    checkState(SystemState::kStopped);
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
