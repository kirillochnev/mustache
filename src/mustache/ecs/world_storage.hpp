#pragma once

#include <mustache/ecs/id_deff.hpp>

#include <mustache/utils/crc32.hpp>
#include <mustache/utils/type_info.hpp>
#include <mustache/utils/array_wrapper.hpp>

#include <map>
#include <memory>

namespace mustache {

    struct SingletonId : public IndexLike<uint32_t, SingletonId> {

    };

    struct ObjectTag : public IndexLike<uint32_t, ObjectTag> {
        template<size_t _N>
        [[nodiscard]] static constexpr ObjectTag fromStr(const char (&str)[_N]) {
            return fromStr(str, _N);
        }

        [[nodiscard]] static ObjectTag fromStr(const std::string& str) {
            return fromStr(str.c_str(), str.size());
        }

        [[nodiscard]] static constexpr ObjectTag fromStr(const char* str, size_t size) {
            return ObjectTag::make(crc32(str, size));
        }

        struct Hash {
                size_t operator()(const ObjectTag& tag) const noexcept {
                    return tag.toInt<size_t >();
                }
        };
    };

    class WorldStorage {
    public:
        explicit WorldStorage(MemoryManager& manager);

        template<typename _Singleton>
        [[nodiscard]] static SingletonId registerSingleton() noexcept {
            static const SingletonId id = getSingletonId(makeTypeInfo<_Singleton>());
            return id;
        }

        [[nodiscard]] static SingletonId getSingletonId(const TypeInfo& info) noexcept;

        [[nodiscard]] void* getInstanceOf(SingletonId id) const noexcept {
            if (!singletons_.has(id)) {
                return nullptr;
            }
            return singletons_[id].get();
        }

        template<typename _Singleton>
        [[nodiscard]] _Singleton* getInstanceOf() const {
            return static_cast<_Singleton*>(getInstanceOf(registerSingleton<_Singleton>()));
        }

        template<typename _Singleton>
        void storeSingleton(const std::shared_ptr<_Singleton>& singleton) {
            const auto id = registerSingleton<_Singleton>();
            if (!singletons_.has(id)) {
                singletons_.resize(id.toInt() + 1);
            }
            singletons_[id] = std::static_pointer_cast<void>(singleton);
        }

        template<typename _Singleton, typename... ARGS>
        _Singleton& storeSingleton(ARGS&&... args) {
            const auto id = registerSingleton<_Singleton>();
            if (!singletons_.has(id)) {
                singletons_.resize(id.toInt() + 1);
            }
            singletons_[id] = std::static_pointer_cast<void>(std::make_shared<_Singleton>(std::forward<ARGS>(args)...));
            return *static_cast<_Singleton*>(singletons_[id].get());
        }

        template<typename T, typename... ARGS>
        T& store(ObjectTag tag, ARGS&&... args) {
            auto ptr = std::make_shared<T>(std::forward<ARGS>(args)...);
            objects_with_tag_[tag] = std::static_pointer_cast<void>(ptr);
            return *ptr;
        }

        template<typename T>
        T* load(ObjectTag tag) const noexcept {
            const auto find_res = objects_with_tag_.find(tag);
            if (find_res == objects_with_tag_.end()) {
                return nullptr;
            }
            return static_cast<T*>(find_res->second.get());
        }
        template<typename T>
        std::shared_ptr<T> loadShared(ObjectTag tag) const noexcept {
            const auto find_res = objects_with_tag_.find(tag);
            if (find_res == objects_with_tag_.end()) {
                return nullptr;
            }
            return std::static_pointer_cast<T*>(find_res->second);
        }
    private:
        ArrayWrapper<std::shared_ptr<void>, SingletonId, true > singletons_;
        std::unordered_map<ObjectTag, std::shared_ptr<void>, ObjectTag::Hash > objects_with_tag_;
    };
}
