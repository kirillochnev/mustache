//
// Created by kirill on 18.04.25.
//

#pragma once

#include <mustache/utils/span.hpp>
#include <mustache/utils/logger.hpp>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <utility>
#include <array>

namespace mustache {
    template <typename _Derived, typename _SizeType>
    class StableLatencyStorageBase {
        [[nodiscard]] _Derived& self() noexcept {
            return *static_cast<_Derived*>(this);
        }
        [[nodiscard]] const _Derived& self() const noexcept {
            return *static_cast<const _Derived*>(this);
        }
    public:
        using Size = _SizeType;

        [[nodiscard]] Size capacityFirst() const noexcept {
            return self().capacityFirstImpl();
        }

        [[nodiscard]] Size capacitySecond() const noexcept {
            return self().capacitySecondImpl();
        }

        [[nodiscard]] bool isMigrationStage() const noexcept {
            return size_ >= capacityFirst();
        }

        void grow() {
            self().reallocateChunks();
            migration_pos_ = size_;
        }

        [[nodiscard]] Size capacity() const noexcept {
            return max(capacityFirst(), capacitySecond());
        }

        [[nodiscard]] bool needGrow() const noexcept {
            return size_ >= capacityFirst() + capacitySecond() / 2;
        }

        [[nodiscard]] Size migrationPos() const noexcept {
            return migration_pos_;
        }

        template<typename... ARGS>
        void emplace_back(ARGS&&... args) {
            if (needGrow()) {
                grow();
            }
            if (isMigrationStage()) {
                self().migrationSteps(self().migrationStepsCountWhileInsert());
                self().insertAt(size_, std::forward<ARGS>(args)...);
            } else {
                self().insertAt(size_, std::forward<ARGS>(args)...);
                ++migration_pos_;
            }
            ++size_;
        }
        [[nodiscard]] Size size() const noexcept {
            return size_;
        }
    protected:
        Size size_ = 0;
        Size migration_pos_ = 0;
    };



    template<bool _Const, typename _Storage>
    class StableLatencyStorageIterator;

    template<typename T, typename Size>
    struct StableLatencyStorageBuffer {
        T* data_ = nullptr;
        ~StableLatencyStorageBuffer() {
            clear();
        }
        const T* data() const noexcept {
            return data_;
        }
        T* data() noexcept {
            return data_;
        }
        bool empty() const noexcept {
            return data_ == nullptr;
        }
        void resize(Size size, Size alignment = alignof(T)) {
            clear();
            const std::size_t total_size = sizeof(T) * size;
            const std::size_t aligned_size = (total_size + alignment - 1) & ~(alignment - 1);

            data_ = static_cast<T*>(aligned_alloc(alignment, aligned_size));
//            Logger{}.hideContext().debug("[[Resize]] allocated memory %p -> %p", data_, data_ + size);

//                std::cout << "memory allocated for "<< size << " items. begin: " << data_ << ", end: " << &data_[size] << " | ";
        }
        void clear() {
//            Logger{}.hideContext().debug("[[CLEAR]] released memory: %p", data_);

            free(data_);
            data_ = nullptr;
        }
        operator T*() noexcept {
            return data_;
        }
        operator const T*() const noexcept {
            return data_;
        }

        static void swap(StableLatencyStorageBuffer& a, StableLatencyStorageBuffer& b) noexcept {
            std::swap(a.data_, b.data_);
        }
    };

    template <typename T, typename _SizeType = std::size_t>
    class StableLatencyStorage : public StableLatencyStorageBase<StableLatencyStorage<T, _SizeType>, _SizeType> {

        static void destroy(T* begin, T* end) {
            std::destroy(begin, end);
        }
        static void destroy(T* begin) {
            std::destroy(begin, begin + 1);
        }
        static void destroy(Span<T> span) {
            std::destroy(span.begin(), span.begin() + span.size());
        }
    public:
        using StorageType = StableLatencyStorage<T, _SizeType>;
        using Base = StableLatencyStorageBase<StorageType, _SizeType>;
        friend Base;
        using typename Base::Size;
        using Base::size_;
        using Base::migration_pos_;
        using element_type = T;
        using iterator = StableLatencyStorageIterator<false, StorageType>;
        using const_iterator = StableLatencyStorageIterator<true, StorageType>;
        static constexpr Size initial_capacity = 32;
        enum BufferName : Size {
            kFirst = Size(0ull),
            kSecond = ~kFirst
        };
        explicit StableLatencyStorage(Size capacity = initial_capacity): capacity_ {capacity} {
//            std::cout << "[Reallocate Chunks] before: capacity: {" << 0 << "; " << capacitySecondImpl() << "}  | ";
            buffers_[0].resize(capacity);
//            std::cout << "after: capacity: {" << capacityFirstImpl() << "; " << capacitySecondImpl() << "}" << std::endl;

        }
        StableLatencyStorage(const StableLatencyStorage& rhs) {
            copyFrom(rhs);
        }
        ~StableLatencyStorage() {
            clear();
        }
        void clear() {
            for (auto buffer_name : bufferNames()) {
                destroy(span(buffer_name));
            }
            buffers_[0].clear();
            buffers_[1].clear();
            capacity_ = 0;
            size_ = 0;
            migration_pos_ = 0;
        }
        static void swap(StableLatencyStorage& a, StableLatencyStorage& b) noexcept {
            if (&a != &b) {
                std::swap(a.size_, b.size_);
                std::swap(a.capacity_, b.capacity_);
                std::swap(a.migration_pos_, b.migration_pos_);
                CArray::swap(a.buffers_[0], b.buffers_[0]);
                CArray::swap(a.buffers_[1], b.buffers_[1]);
            }
        }
        StableLatencyStorage& operator=(StableLatencyStorage&& rhs) noexcept {
            if (this != &rhs) {
                swap(*this, rhs);
                rhs.clear();
            }
            return *this;
        }

        StableLatencyStorage& operator=(const StableLatencyStorage& rhs) {
            if (this != &rhs) {
                clear();
                copyFrom(rhs);
            }
            return *this;
        }

        constexpr std::array<BufferName, 2> bufferNames() const noexcept {
            return {BufferName::kFirst, BufferName::kSecond};
        }

        const T* rawBuffer(BufferName buffer_name) const noexcept {
            return buffers_[buffer_name & 1].data_;
        }
        T* rawBuffer(BufferName buffer_name) noexcept {
            return buffers_[buffer_name & 1].data_;
        }

        static constexpr Size select(BufferName buffer_name, Size first_value, Size second_value) noexcept {
            return ((~buffer_name) & first_value) | (buffer_name & second_value);
        }
        Span<T> span(BufferName buffer_name) noexcept {
            const auto ptr = buffers_[buffer_name & 1].data_ + (migration_pos_ & buffer_name);
            const auto len = select(buffer_name, migration_pos_, size_ - migration_pos_);
            return Span<T> {ptr, len};
        }

        Span<const T> span(BufferName buffer_name) const noexcept {
            const auto ptr = buffers_[buffer_name & 1].data_ + (migration_pos_ & buffer_name);
            const auto len = select(buffer_name, migration_pos_, size_ - migration_pos_);
            return Span<T> {ptr, len};
        }

        void reallocateChunks() {
//            std::cout << "[Reallocate Chunks] before: capacity: {" << capacityFirstImpl() << "; " << capacitySecondImpl() << "}  | ";
//            std::flush(std::cout);
            if (!buffers_[1].empty()) {
                CArray::swap(buffers_[0], buffers_[1]);
                capacity_ = (capacity_ == 0ull) ? initial_capacity : (capacity_ * 2ull);
            }
            buffers_[1].resize(capacity_ * 2);
//            std::cout << "after: capacity: {" << capacityFirstImpl() << "; " << capacitySecondImpl() << "}" << std::endl;
//            std::flush(std::cout);
            migration_pos_ = size_;
        }


        template<typename... ARGS>
        void insertAt(Size index, ARGS&&... args) {
//            std::cout << "[INSERT] ";
            if (buffers_[1].empty()) {
//                std::cout << "Placement new at: " << buffers_[0].data_ + index << std::endl;
                new (buffers_[0].data_ + index) T(std::forward<ARGS>(args)...);
//                std::cout << "Set data[0][" << index << "] = " << T(args...) << std::endl;
            } else {
//                std::cout << "Placement new at: " << buffers_[1].data_ + index << std::endl;
                new (buffers_[1].data_ + index) T(std::forward<ARGS>(args)...);
//                std::cout << "Set data[1][" << index << "] = " << T(args...) << std::endl;
            }
        }

        bool migrationSteps(Size count = 0) {
            if (migration_pos_ == 0 || buffers_[1].empty()) {
                return false;
            }
            count = count == 0 ? migration_pos_ : std::min(count, migration_pos_);
            const auto offset = migration_pos_ - count;
            const auto source = buffers_[0].data_ + offset;
            const auto dest = buffers_[1].data_ + offset;

            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(dest, source, sizeof(T) * count);
            } else {
                for (Size i = 0; i < count; ++i) {
                    dest[i] = std::move(source[i]);
//                    std::cout << "Moving " << dest[i] << ", at " << offset + i << std::endl;
                }
            }
            destroy(source, source + count);
            migration_pos_ -= count;
            return migration_pos_ == 0;
        }

        bool shrink() noexcept {
            if (migration_pos_ <= size_ || buffers_[1].empty()) {
                return false;
            }
            CArray::swap(buffers_[0], buffers_[1]);
            buffers_[1].clear();
            migration_pos_ = size_;
            return true;
        }
        void shrink_to_fit() noexcept {
            migrationSteps(0);
            shrink();
        }
        T& operator[](Size index) noexcept {
            const Size buffer_index = index >= migration_pos_;
//            std::cout << "Capacity: " << capacity_ << ", size: "
//            << size_ << ", data[" << buffer_index << "][" << index << "] = " << buffers_[buffer_index][index] << std::endl;
            return buffers_[buffer_index][index];
        }

        iterator begin() noexcept { return iterator(this, 0); }
        iterator end() noexcept { return iterator(this, size_); }

        const_iterator begin() const noexcept { return const_iterator(this, 0); }
        const_iterator end() const noexcept { return const_iterator(this, size_); }

        const_iterator cbegin() const noexcept { return const_iterator(this, 0); }
        const_iterator cend() const noexcept { return const_iterator(this, size_); }

        // Обратные итераторы можно реализовать через STL адаптеры
        std::reverse_iterator<iterator> rbegin() noexcept { return std::reverse_iterator<iterator>(end()); }
        std::reverse_iterator<iterator> rend() noexcept { return std::reverse_iterator<iterator>(begin()); }

        std::reverse_iterator<const_iterator> rbegin() const noexcept {
            return std::reverse_iterator<const_iterator>(end());
        }
        std::reverse_iterator<const_iterator> rend() const noexcept {
            return std::reverse_iterator<const_iterator>(begin());
        }

        std::reverse_iterator<const_iterator> crbegin() const noexcept {
            return std::reverse_iterator<const_iterator>(cend());
        }
        std::reverse_iterator<const_iterator> crend() const noexcept {
            return std::reverse_iterator<const_iterator>(cbegin());
        }
    protected:
        void copyFrom(const StableLatencyStorage& rhs) {
            if (this == &rhs) {
                return;
            }

            const Size required_capacity = rhs.size_;
            const bool using_first = capacityFirstImpl() >= required_capacity;
            const bool using_second = !using_first && (capacitySecondImpl() >= required_capacity);

            destroy(span(kFirst));
            destroy(span(kSecond));
            if (!using_first) {
                if (using_second) {
                    CArray::swap(buffers_[0], buffers_[1]);
                    capacity_ *= 2;
                } else {
                    buffers_[0].resize(required_capacity);
                    capacity_ = required_capacity;
                }
            }
            size_ = rhs.size_;
            migration_pos_ = size_;
            buffers_[1].clear();

            auto* dst = buffers_[0].data_;
            if constexpr (std::is_trivially_copyable_v<T>) {
                for (auto buffer_name : rhs.bufferNames()) {
                    auto src = rhs.span(buffer_name);
                    std::memcpy(dst, src.begin(), sizeof(T) * src.size());
                    dst += src.size();
                }
            } else {
                for (auto buffer_name : rhs.bufferNames()) {
                    for (const auto& val : rhs.span(buffer_name)) {
                        new (dst++) T(val);
                    }
                }
            }
        }


        Size migrationStepsCountWhileInsert() const noexcept {
            constexpr Size result = std::max(1ull, 128ull / sizeof(T));
            return result;
        }
        Size capacityFirstImpl() const noexcept {
            return capacity_;
        }

        Size capacitySecondImpl() const noexcept {
            return buffers_[1].empty() ? 0 : capacity_ * 2;
        }

        using CArray = StableLatencyStorageBuffer<T, Size>;
        CArray buffers_[2];
        Size capacity_;
    };

    template<bool _Const, typename _Storage>
    class StableLatencyStorageIterator {
    private:
        using StableLatencyStorage = _Storage;
        using Size = typename _Storage::Size;
        _Storage* container_;
        Size index_;

    public:
        using value_type = std::conditional_t<_Const, const typename _Storage::element_type, typename _Storage::element_type>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::random_access_iterator_tag;

        // Конструкторы
        StableLatencyStorageIterator() noexcept : container_(nullptr), index_(0) {}
        StableLatencyStorageIterator(StableLatencyStorage* container, Size index) noexcept
                : container_(container), index_(index) {}

        // Операторы сравнения
        bool operator==(const StableLatencyStorageIterator& other) const noexcept {
            return container_ == other.container_ && index_ == other.index_;
        }

        bool operator!=(const StableLatencyStorageIterator& other) const noexcept {
            return !(*this == other);
        }

        // Доступ к элементу
        reference operator*() const noexcept {
            return (*container_)[index_];
        }

        pointer operator->() const noexcept {
            return &(operator*());
        }

        // Инкремент/декремент
        StableLatencyStorageIterator& operator++() noexcept {
            ++index_;
            return *this;
        }

        StableLatencyStorageIterator operator++(int) noexcept {
            StableLatencyStorageIterator tmp = *this;
            ++*this;
            return tmp;
        }

        StableLatencyStorageIterator& operator--() noexcept {
            --index_;
            return *this;
        }

        StableLatencyStorageIterator operator--(int) noexcept {
            StableLatencyStorageIterator tmp = *this;
            --*this;
            return tmp;
        }

        // Арифметические операции
        StableLatencyStorageIterator& operator+=(difference_type n) noexcept {
            index_ += n;
            return *this;
        }

        StableLatencyStorageIterator operator+(difference_type n) const noexcept {
            return iterator(container_, index_ + n);
        }

        friend StableLatencyStorageIterator operator+(difference_type n, const StableLatencyStorageIterator& it) noexcept {
            return it + n;
        }

        StableLatencyStorageIterator& operator-=(difference_type n) noexcept {
            index_ -= n;
            return *this;
        }

        StableLatencyStorageIterator operator-(difference_type n) const noexcept {
            return iterator(container_, index_ - n);
        }

        difference_type operator-(const StableLatencyStorageIterator& other) const noexcept {
            return index_ - other.index_;
        }

        // Операторы сравнения для случайного доступа
        bool operator<(const StableLatencyStorageIterator& other) const noexcept {
            return index_ < other.index_;
        }

        bool operator>(const StableLatencyStorageIterator& other) const noexcept {
            return other < *this;
        }

        bool operator<=(const StableLatencyStorageIterator& other) const noexcept {
            return !(other < *this);
        }

        bool operator>=(const StableLatencyStorageIterator& other) const noexcept {
            return !(*this < other);
        }

        // Индексация
        reference operator[](difference_type n) const noexcept {
            return (*container_)[index_ + n];
        }
    };
}
