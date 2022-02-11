#pragma once

#include <mustache/utils/logger.hpp>
#include <mustache/utils/type_info.hpp>
#include <mustache/utils/index_like.hpp>
#include <mustache/utils/array_wrapper.hpp>
#include <memory>

namespace mustache {
    struct MUSTACHE_EXPORT EventId : public IndexLike<uint32_t, EventId>{};

    class EventManager;
    template <typename T>
    struct Receivers;

    template <typename T>
    class Receiver {
        friend EventManager;
        friend struct Receivers<T>;
    public:
        virtual ~Receiver() {
            unsubscribe();
        }
        void unsubscribe();
    private:
        virtual void onEvent(const T&) = 0;
        std::weak_ptr<EventManager> events_;
    };

    struct MUSTACHE_EXPORT AReceivers {
        virtual ~AReceivers() = default;
    };

    template <typename T>
    struct Receivers : public AReceivers {
        std::vector<Receiver<T>* > receivers;

        void add(Receiver<T>* ptr) {
            receivers.push_back(ptr);
            Logger{}.debug("Subscriber [%p] on event type: [%s] added", ptr, type_name <T>().c_str());
        }
        void remove(Receiver<T>* ptr) {
            for(auto it  = receivers.begin(); it != receivers.end(); ++it) {
                if(*it == ptr) {
                    receivers.erase(it);
                    Logger{}.debug("Subscriber [%p] on event type: [%s] removed", ptr, type_name <T>().c_str());
                    return;
                }
            }
        }
        void onEvent(const T& e) noexcept {
            for(auto& receiver : receivers) {
                receiver->onEvent(e);
            }
        }
    };

    class MUSTACHE_EXPORT EventManager : public Uncopiable {
    public:
        explicit EventManager(MemoryManager& memory_manager):
                subscriptions_{memory_manager} {

        }
        template <typename T>
        EventId registerEventType() noexcept {
            static EventId result = registerEventType(makeTypeInfo<T>().name);
            if(!subscriptions_.has(result) || !subscriptions_[result]) {
                subscriptions_.resize(result.toInt() + 1);
                subscriptions_[result].reset(new Receivers<T>{});
            }
            return result;
        }

        static EventId registerEventType(const std::string& type_name) noexcept;

        template <typename T, typename F>
        std::unique_ptr<Receiver<T> > subscribe(F&& func) {
            class FunctionWrapper final : public Receiver<T> {
            public:
                explicit FunctionWrapper(F&& _f):
                        f{std::forward<F>(_f)} {
                }
            private:
                void onEvent(const T& e) override {
                    f(e);
                }
                F f;
            };
            auto ptr = std::make_unique<FunctionWrapper>(std::forward<F>(func));
            subscribe_<T>(ptr.get());
            return ptr;
        }


        template <typename T, typename F>
        void subscribe_(F* sub) {
            static const EventId id = registerEventType<T>();
            sub->events_ = shared_from_this_;
            static_cast<Receivers<T>* >(subscriptions_[id].get())->add(sub);
        }

        template <typename T>
        void unsubscribe(Receiver<T>* sub) {
            static const EventId id = registerEventType<T>();
            static_cast<Receivers<T>* >(subscriptions_[id].get())->remove(sub);
        }

        template <typename T>
        void post(const T& event) noexcept {
            static const EventId id = registerEventType<T>();
            static_cast<Receivers<T>* >(subscriptions_[id].get())->onEvent(event);
        }
    private:
        // shared_from_this_ is used to check if EventManager is alive, while Receiver::unsubscribe
        // Can not use std::enable_shared_from_this, because EcsManager contains EventManager field,
        // not std::shared_ptr<EventManager>.
        std::shared_ptr<EventManager> shared_from_this_{this, [](EventManager*) noexcept {}};
        ArrayWrapper<std::unique_ptr<AReceivers>, EventId, true> subscriptions_;
    };

    template<typename T>
    void Receiver<T>::unsubscribe() {
        auto events = events_.lock();
        if(!events) {
            return;
        }
        events->unsubscribe(this);
    }
}
