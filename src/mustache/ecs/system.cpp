#include "system.hpp"
#include <mustache/ecs/job.hpp>

using namespace mustache;

void ASystem::checkState(SystemState expected_state) {
    if (state_ != expected_state) {
        throw std::runtime_error("Invalid state");
    }
}

void ASystem::onCreate(World& world) {
    checkState(SystemState::kUninit);
    if (state_ != SystemState::kUninit) {
        throw std::runtime_error("Invalid state");
    }
    onCreateImpl(world);
    state_ = SystemState::kInited;
}

void ASystem::onConfigure(World& world, SystemConfig& config) {
    checkState(SystemState::kInited);
    onConfigureImpl(world, config);
    state_ = SystemState::kConfigured;
}

void ASystem::onStart(World& world) {
    checkState(SystemState::kConfigured);
    onStartImpl(world);
    state_ = SystemState::kActive;
}

void ASystem::onUpdate(World& world) {
    checkState(SystemState::kActive);
    onUpdateImpl(world);
}

void ASystem::onPause(World& world) {
    checkState(SystemState::kActive);
    onPauseImpl(world);
    state_ = SystemState::kPaused;
}

void ASystem::onStop(World& world) {
    checkState(SystemState::kPaused);
    onStopImpl(world);
    state_ = SystemState::kStopped;
}

void ASystem::onResume(World& world) {
    checkState(SystemState::kPaused);
    onResumeImpl(world);
    state_ = SystemState::kActive;
}

void ASystem::onDestroy(World& world) {
    checkState(SystemState::kStopped);
    onDestroyImpl(world);
    state_ = SystemState::kUninit;
}

void ASystem::onCreateImpl(World&) {

}

void ASystem::onConfigureImpl(World&, SystemConfig&) {

}

void ASystem::onStartImpl(World&) {

}

void ASystem::onPauseImpl(World&) {

}

void ASystem::onStopImpl(World&) {

}

void ASystem::onResumeImpl(World&) {

}

void ASystem::onDestroyImpl(World&) {

}
