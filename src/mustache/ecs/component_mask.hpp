#pragma once

#include <mustache/utils/span.hpp>
#include <mustache/utils/invoke.hpp>
#include <mustache/utils/memory_manager.hpp>
#include <mustache/utils/container_vector.hpp>
#include <mustache/ecs/id_deff.hpp>

#include <bitset>
#include <memory>
#include <cstdint>
#include <initializer_list>

namespace mustache {

    template<typename _ItemType, size_t _MaxElements>
    struct ComponentMask {

        ComponentMask() = default;

        explicit ComponentMask(const std::initializer_list<_ItemType>& items) {
            for(auto id : items) {
                value_.set(id.toInt());
            }
        }

        [[nodiscard]] static constexpr size_t maxElementsCount() noexcept {
            return _MaxElements;
        }

        [[nodiscard]] bool isEmpty() const noexcept {
            return !value_.any();
        }

        [[nodiscard]] mustache::vector<_ItemType > items() const noexcept {
            mustache::vector<_ItemType > result;
            for (size_t i = 0u; i < value_.size(); ++i) {
                if (value_.test(i)) {
                    result.push_back(_ItemType::make(i));
                }
            }
            return result;
        }

        template<typename _F>
        void forEachItem(_F&& function) const noexcept {
            for (size_t i = 0u; i < value_.size(); ++i) {
                using ResultType = decltype(function(_ItemType::make(0)));
                constexpr bool can_be_interrupted = std::is_same<ResultType, bool>::value;
                if constexpr (can_be_interrupted) {
                    if (value_.test(i)) {
                        if (!function(_ItemType::make(i))) {
                            return;
                        }
                    }
                } else {
                    if (value_.test(i)) {
                        function(_ItemType::make(i));
                    }
                }
            }
        }

        [[nodiscard]] bool isMatch(const ComponentMask& rhs) const noexcept {
            return (value_ & rhs.value_) == rhs.value_;
        }

        [[nodiscard]] bool has(_ItemType item) const noexcept{
            return value_.test(item.toInt());
        }

        void add(_ItemType item) noexcept {
            value_.set(item.toInt());
        }

        [[nodiscard]] ComponentMask merge(const ComponentMask& extra) const noexcept {
            ComponentMask result;
            result.value_ = value_ | extra.value_;
            return result;
        }

        [[nodiscard]] ComponentMask inverse() const noexcept {
            ComponentMask result;
            result.value_ = ~value_;
            return result;
        }

        [[nodiscard]] ComponentMask intersection(const ComponentMask& oth) const noexcept {
            ComponentMask result;
            result.value_ = value_ & oth.value_;
            return result;
        }

        [[nodiscard]] ComponentMask subtract(const ComponentMask& oth) const noexcept {
            ComponentMask result;
            result.value_ = value_ & ~oth.value_;
            return result;
        }

        void set(_ItemType item, bool value) noexcept {
            value_.set(item.toInt(), value);
        }

        void reset(_ItemType item) noexcept {
            value_.reset(item.toInt());
        }

        [[nodiscard]] bool operator==(const ComponentMask& rhs) const noexcept {
            return value_ == rhs.value_;
        }

        [[nodiscard]] bool operator!=(const ComponentMask& rhs) const noexcept {
            return value_ != rhs.value_;
        }

        [[nodiscard]] static const ComponentMask& null() noexcept {
            static const ComponentMask instanse{};
            return instanse;
        }

        /*[[nodiscard]] bool operator<(const ComponentMask& rhs) const noexcept {
            return value_ < rhs.value_;
        }*/

        [[nodiscard]] uint32_t componentsCount() const noexcept {
            return static_cast<uint32_t>(value_.count());
        }
    private:
        std::bitset<_MaxElements> value_;
    };

    template<typename _ItemType>
    struct DynamicComponentMask {
        static constexpr size_t bitset_size = 8 * sizeof (std::pair<void*, size_t>);
        static constexpr size_t static_mask_size = bitset_size - 2ull;
        static constexpr size_t use_dynamic_bit = bitset_size - 1ull;
        static constexpr size_t inverted_tail_bit = bitset_size - 2ull;
        static constexpr size_t size_mask = ~(3ull << (sizeof(size_t) * 8 - 2));
        using  StaticBitset = std::bitset<bitset_size>;
        enum class Operation {
            kMerge,
            kIntersection,
            kSubtract
        };

        static const StaticBitset& staticDataMask() noexcept {
            static const auto result = []() {
                StaticBitset tmp;
                for (size_t i = 0; i < static_mask_size; ++i) {
                    tmp.set(i, true);
                }
                return tmp;
            }();
            return result;
        }

        static const StaticBitset one_mask;

        static constexpr StaticBitset zero_mask = StaticBitset{};

        constexpr DynamicComponentMask() noexcept:
                static_mask_(0) {

        }

        explicit DynamicComponentMask(const std::initializer_list<_ItemType>& items) noexcept:
                static_mask_(0) {
            for (auto item : items) {
                add(item);
            }
        }

        DynamicComponentMask(DynamicComponentMask&& other) noexcept {
            if (!other.isDynamic()) {
                static_mask_ = other.static_mask_;
            } else {
                dynamic_.array_ = other.dynamic_.array_;
                dynamic_.size_ = other.dynamic_.size_;
                static_mask_.set(use_dynamic_bit);

                other.dynamic_.array_ = nullptr;
                other.dynamic_.size_ = 0;
            }
        }
        DynamicComponentMask(const DynamicComponentMask& other) noexcept {
            copyFrom(other);
        }

        DynamicComponentMask& operator=(DynamicComponentMask&& other) noexcept {
            if (this == &other) {
                return *this;
            }

            if (isDynamic()) {
                delete[] dynamic_.array_;
            }

            if (!other.isDynamic()) {
                static_mask_ = other.static_mask_;
            } else {
                dynamic_.array_ = other.dynamic_.array_;
                dynamic_.size_ = other.dynamic_.size_;
                static_mask_.set(use_dynamic_bit);

                other.dynamic_.array_ = nullptr;
                other.dynamic_.size_ = 0;
            }

            return *this;
        }
        DynamicComponentMask& operator=(const DynamicComponentMask& other) noexcept {
            if (this == &other) {
                return *this;
            }

            if (isDynamic()) {
                if (other.isDynamic()) {
                    const size_t old_size = dynamic_.size();
                    const size_t new_size = other.dynamic_.size();
                    if (new_size <= old_size) {
                        for (size_t i = 0; i < new_size; ++i) {
                            dynamic_.array_[i] = other.dynamic_.array_[i];
                        }
                        for (size_t i = new_size; i < old_size; ++i) {
                            dynamic_.array_[i].reset();
                        }
                        dynamic_.size_ = (new_size << 1ull) | 1ull;
                        return *this;
                    }
                    delete[] dynamic_.array_;
                } else {
                    delete[] dynamic_.array_;
                }
            }

            copyFrom(other);
            return *this;
        }

        ~DynamicComponentMask() noexcept {
            if (isDynamic()) {
                delete[] dynamic_.array_;
            }
        }

        [[nodiscard]] bool isEmpty() const noexcept {
            if (!static_mask_.any()) {
                return true;
            }
            if (!isDynamic()) {
                return false;
            }
            for (size_t i = 0; i < dynamic_.size(); ++i) {
                if (dynamic_.array_[i].any()) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool isDynamic() const noexcept {
            return static_mask_.test(use_dynamic_bit);
        }

        template<typename _F>
        void forEachItem(_F&& function) const noexcept {
            if (!isDynamic()) {
                for (size_t i = 0u; i < static_mask_size; ++i) {
                    if (static_mask_.test(i)) {
                        function(_ItemType::make(i));
                    }
                }
            } else {
                size_t offset = 0;
                for (const auto& item : dynamic_.span()) {
                    for (size_t i = 0u; i < bitset_size; ++i) {
                        if (item.test(i)) {
                            function(_ItemType::make(offset + i));
                        }
                    }
                    offset += bitset_size;
                }
            }
        }

        template<typename _Result>
        [[nodiscard]] _Result asStaticMask() const noexcept {
            _Result result;
            forEachItem([&result](const auto i) {
                result.add(i);
            });
            return result;
        }
        [[nodiscard]] mustache::vector<_ItemType > items() const noexcept {
            mustache::vector<_ItemType > result;
            forEachItem([&result](auto item) {
                result.push_back(item);
            });
            return result;
        }

        [[nodiscard]] bool isMatch(const DynamicComponentMask& rhs) const noexcept {
            const bool lhs_dynamic = isDynamic();
            const bool rhs_dynamic = rhs.isDynamic();

            if (!lhs_dynamic && !rhs_dynamic) {
                return (static_mask_ & rhs.static_mask_) == rhs.static_mask_;
            }
            const bool lhs_inverted_tail = static_mask_.test(inverted_tail_bit);
            const bool rhs_inverted_tail = rhs.static_mask_.test(inverted_tail_bit);

            if (!lhs_inverted_tail && rhs_inverted_tail) {
                return false;
            }

            const StaticBitset* lhs_array;
            const StaticBitset* rhs_array;
            size_t lhs_size;
            size_t rhs_size;

            if (lhs_dynamic) {
                lhs_array = dynamic_.array_;
                lhs_size = dynamic_.size();
            } else {
                lhs_array = &static_mask_;
                lhs_size = 1;
            }

            if (rhs_dynamic) {
                rhs_array = rhs.dynamic_.array_;
                rhs_size = rhs.dynamic_.size();
            } else {
                rhs_array = &rhs.static_mask_;
                rhs_size = 1;
            }

            const size_t min_size = (lhs_size < rhs_size) ? lhs_size : rhs_size;
            for (size_t i = 0; i < min_size; ++i) {
                if ((lhs_array[i] & rhs_array[i]) != rhs_array[i]) {
                    return false;
                }
            }
            if (!lhs_inverted_tail) {
                for (size_t i = lhs_size; i < rhs_size; ++i) {
                    if (rhs_array[i].any()) {
                        return false;
                    }
                }
            }
            return true;
        }
        [[nodiscard]] bool has(_ItemType item) const noexcept {
            const auto id = item.toInt();

            if (!isDynamic()) {
                if (id < static_mask_size) {
                    return static_mask_.test(id);
                }
            } else {
                const size_t real_blocks = dynamic_.size();
                const size_t block_id = id / bitset_size;
                const size_t bit_id = id % bitset_size;

                if (block_id < real_blocks) {
                    return dynamic_.array_[block_id].test(bit_id);
                }
            }
            return static_mask_.test(inverted_tail_bit);
        }

        template<Operation op>
        [[nodiscard]] DynamicComponentMask combineWith(const DynamicComponentMask& other) const noexcept {
            const auto apply = [](const StaticBitset& lhs, const StaticBitset& rhs) {
                if constexpr (op == Operation::kMerge) {
                    return lhs | rhs;
                }
                if constexpr (op == Operation::kIntersection) {
                    return lhs & rhs;
                }
                if constexpr (op == Operation::kSubtract) {
                    return lhs & ~rhs;
                }
                return lhs; // unreachable
            };

            DynamicComponentMask result;
            const bool lhs_dynamic = isDynamic();
            const bool rhs_dynamic = other.isDynamic();
            if (!lhs_dynamic && !rhs_dynamic) {
                result.static_mask_ = apply(static_mask_, other.static_mask_);
                result.static_mask_.set(use_dynamic_bit, false);
                return result;
            }

            const auto lhs_span = makeEndlessSpan(*this);
            const auto rhs_span = makeEndlessSpan(other);
            const size_t result_size = [&]{
                if constexpr (op == Operation::kMerge) {
                    return std::max(lhs_span.size(), rhs_span.size());
                } else {
                    return std::min(lhs_span.size(), rhs_span.size());
                }
            }();
            result.reallocDynamicStorage(result_size);
            for (size_t i = 0; i < result_size; ++i) {
                result.dynamic_.array_[i] = apply(lhs_span[i], rhs_span[i]);
            }
            return result;
        }

        [[nodiscard]] DynamicComponentMask merge(const DynamicComponentMask& extra) const noexcept {
            return combineWith<Operation::kMerge>(extra);
        }

        [[nodiscard]] DynamicComponentMask intersection(const DynamicComponentMask& oth) const noexcept {
            return combineWith<Operation::kIntersection>(oth);
        }

        [[nodiscard]] DynamicComponentMask subtract(const DynamicComponentMask& oth) const noexcept {
            return combineWith<Operation::kSubtract>(oth);
        }

        [[nodiscard]] bool operator==(const DynamicComponentMask& rhs) const noexcept {
            const auto lhs_span = makeEndlessSpan(*this);
            const auto rhs_span = makeEndlessSpan(rhs);
            const size_t max_size = std::max(lhs_span.size(), rhs_span.size());

            for (size_t i = 0; i < max_size; ++i) {
                if (lhs_span[i] != rhs_span[i]) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool operator!=(const DynamicComponentMask& rhs) const noexcept {
            return !(*this == rhs);
        }

        [[nodiscard]] uint32_t componentsCount() const noexcept {
            if (!isDynamic()) {
                return static_cast<uint32_t> ((static_mask_ & staticDataMask()).count());
            }
            size_t count = 0;
            for (const auto& item : dynamic_.span()) {
                count += item.count();
            }
            return static_cast<uint32_t> (count);
        }

        void set(_ItemType item, bool value) noexcept {
            constexpr size_t elements_per_cache_line = MemoryManager::cache_line_size / sizeof(StaticBitset);
            const auto id = item.toInt();
            if (!isDynamic()) {
                if (id < static_mask_size) {
                    static_mask_.set(id, value);
                    return;
                }
                if (!value) {
                    return;
                }
                size_t blocks = (id / bitset_size) + 1;
                blocks = ((blocks + elements_per_cache_line - 1) / elements_per_cache_line) * elements_per_cache_line;
                reallocDynamicStorage(blocks);
                dynamic_.array_[id / bitset_size].set(id % bitset_size, value);
            } else {
                const size_t blocks = dynamic_.size();
                size_t need_blocks = (id / bitset_size) + 1;
                if (need_blocks > blocks) {
                    need_blocks = ((need_blocks + elements_per_cache_line - 1) / elements_per_cache_line) * elements_per_cache_line;
                    reallocDynamicStorage(need_blocks, blocks);
                }

                dynamic_.array_[id / bitset_size].set(id % bitset_size, value);
            }
        }

        [[nodiscard]] DynamicComponentMask inverse() const noexcept {
            DynamicComponentMask result;

            if (!isDynamic()) {
                result.static_mask_ = static_mask_;
                result.static_mask_.flip();
                result.static_mask_.set(use_dynamic_bit, false);
                // Переворачиваем флаг inverted_tail_bit
                result.static_mask_.flip(inverted_tail_bit);
            } else {
                const size_t size = dynamic_.size();
                StaticBitset* new_array = new StaticBitset[size]();
                for (size_t i = 0; i < size; ++i) {
                    new_array[i] = ~dynamic_.array_[i];
                }
                result.dynamic_.array_ = new_array;
                result.dynamic_.size_ = (size << 1ull) | 1ull;
                result.static_mask_.set(use_dynamic_bit);
                result.static_mask_.set(inverted_tail_bit, !static_mask_.test(inverted_tail_bit)); // flip tail flag
            }

            return result;
        }
        [[nodiscard]] static constexpr DynamicComponentMask null() noexcept {
            return DynamicComponentMask{};
        }

        void add(_ItemType item) noexcept {
            set(item, true);
        }

        [[nodiscard]] Span<const StaticBitset > span() const noexcept {
            if (!isDynamic()) {
                return {&static_mask_, 1ull};
            }
            return dynamic_.span();
        }
        [[nodiscard]] Span<StaticBitset > span() noexcept {
            if (!isDynamic()) {
                return {&static_mask_, 1ull};
            }
            return dynamic_.span();
        }
    private:
        static auto makeEndlessSpan(const DynamicComponentMask& mask) {
            struct EndlessSpan {
                StaticBitset* ptr_;
                size_t size_;
                StaticBitset tail;
                StaticBitset data;
                size_t size() const noexcept { return size_; }
                const StaticBitset& operator[](size_t i) const noexcept {
                    return i < size_ ? (ptr_ ? ptr_[i] : data) : tail;
                }
            };
            const auto tail = mask.static_mask_.test(inverted_tail_bit) ? one_mask : zero_mask;
            if (mask.isDynamic()) {
                return EndlessSpan{mask.dynamic_.array_, mask.dynamic_.size(), tail, {}};
            } else {
                return EndlessSpan{nullptr, 1, tail, mask.static_mask_ & staticDataMask()};
            }
        };

        void reallocDynamicStorage(size_t new_blocks, size_t old_blocks = 0) noexcept {
            const bool inverted_tail = static_mask_.test(inverted_tail_bit);
            StaticBitset* new_array = new StaticBitset[new_blocks]();
            const bool tail = static_mask_.test(inverted_tail_bit);
            new_array[0] = static_mask_ & staticDataMask();
            for (size_t i = 0; i < old_blocks; ++i) {
                new_array[i] = dynamic_.array_[i];
            }

            if (inverted_tail) {
                for (size_t i = old_blocks; i < new_blocks; ++i) {
                    new_array[i].set();
                }
            }

            if (isDynamic()) {
                delete[] dynamic_.array_;
            }

            dynamic_.array_ = new_array;
            dynamic_.size_ = new_blocks;
            static_mask_.set(use_dynamic_bit);
            static_mask_.set(inverted_tail_bit, tail);
        }

        void copyFrom(const DynamicComponentMask& other) noexcept {
            if (!other.isDynamic()) {
                static_mask_ = other.static_mask_;
            } else {
                const size_t size = other.dynamic_.size();
                auto new_array = new StaticBitset[size]();
                for (size_t i = 0; i < size; ++i) {
                    new_array[i] = other.dynamic_.array_[i];
                }
                dynamic_.array_ = new_array;
                dynamic_.size_ = (size << 1ull) | 1ull; // dynamic bit
                static_mask_.set(use_dynamic_bit);
            }
        }

        struct DynamicBlock {
            StaticBitset* array_;
            size_t size_;

            [[nodiscard]] size_t size() const noexcept {
                return size_ & size_mask;
            }
            [[nodiscard]] Span<const StaticBitset> span() const noexcept {
                return {array_, size()};
            }
            [[nodiscard]] Span<StaticBitset> span() noexcept {
                return {array_, size()};
            }
        };
        union {
            StaticBitset static_mask_;
            DynamicBlock dynamic_;
        };
    };

    template<typename _ItemType>
    const typename DynamicComponentMask<_ItemType>::StaticBitset DynamicComponentMask<_ItemType>::one_mask = []() {
        return ~StaticBitset{};
    }();

    struct MUSTACHE_EXPORT ComponentIdMask : public DynamicComponentMask<ComponentId> {
        ComponentIdMask(const DynamicComponentMask<ComponentId>& oth):
                DynamicComponentMask{oth} {

        }
        ComponentIdMask(DynamicComponentMask<ComponentId>&& oth):
                DynamicComponentMask{std::move(oth)} {

        }

        std::string toString() const noexcept;
        using DynamicComponentMask::DynamicComponentMask;
    };

    struct SharedComponentIdMask : public ComponentMask<SharedComponentId, 128> {
        SharedComponentIdMask(const ComponentMask<SharedComponentId, 128>& oth):
                ComponentMask{oth} {

        }
        SharedComponentIdMask(ComponentMask<SharedComponentId, 128>&& oth):
                ComponentMask{std::move(oth)} {

        }

        std::string toString() const noexcept;
        using ComponentMask::ComponentMask;
    };

    struct SharedComponentTag;

    using SharedComponentPtr = std::shared_ptr<const SharedComponentTag>;
    
    using SharedComponentsData = mustache::vector<SharedComponentPtr>;

    struct SharedComponentsInfo {

        void add(SharedComponentId id, const SharedComponentPtr& value) {
            if (!mask_.has(id)) {
                mask_.set(id, true);
                ids_.push_back(id);
                data_.push_back(value);
            } else {
                data_[indexOf(id).toInt()] = value;
            }
        }

        void remove(SharedComponentId id) {
            if (has(id)) {
                const auto index = indexOf(id);
                data_.erase(data_.begin() + index.toInt());
                ids_.erase(ids_.begin() + index.toInt());
            }
            mask_.set(id, false);
        }

        template <typename _F>
        void forEach(_F&& function) const noexcept {
            uint32_t index = 0;
            for (auto id : ids_) {
                return invoke(function, id, data_[index++]);
            }
        }
        [[nodiscard]] bool has(SharedComponentId id) const noexcept {
            return mask_.has(id);
        }

        [[nodiscard]] bool isMatch(const SharedComponentIdMask& mask) const noexcept {
            return mask_.isMatch(mask);
        }
        [[nodiscard]] const SharedComponentIdMask& mask() const noexcept {
            return mask_;
        }

        [[nodiscard]] const SharedComponentsData& data() const noexcept {
            return data_;
        }
        
        [[nodiscard]] bool empty() const noexcept {
            return data_.empty();
        }

        [[nodiscard]] SharedComponentIndex indexOf(SharedComponentId id) const noexcept {
            for (uint32_t i = 0; i < ids_.size(); ++i) {
                if (ids_[i] == id) {
                    return SharedComponentIndex::make(i);
                }
            }
            return SharedComponentIndex::null();
        }

        std::shared_ptr<const SharedComponentTag> get(SharedComponentIndex index) const noexcept {
            return data_[index.toInt()];
        }

        SharedComponentsInfo merge(const SharedComponentsInfo& oth) const noexcept {
            // TODO: check me!
            SharedComponentsInfo result;
            result.mask_ = oth.mask_.merge(mask_);
            result.data_ = oth.data_;
            for (const auto& id : data_) {
                result.data_.push_back(id);
            }

            for (const auto& id : ids_) {
                result.ids_.push_back(id);
            }

            return result;
        }

        [[nodiscard]] static const SharedComponentsInfo& null() noexcept {
            static const SharedComponentsInfo instanse{};
            return instanse;
        }
    private:
        SharedComponentIdMask mask_;
        mustache::vector<SharedComponentId> ids_;
        SharedComponentsData data_;

    };

    using ComponentIndexMask = ComponentMask<ComponentIndex, 64>;
//    struct ComponentIndexMask : public ComponentMask<ComponentIndex, 64> {
//        using ComponentMask::ComponentMask;
//    };
}
