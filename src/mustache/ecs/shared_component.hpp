#pragma once

namespace mustache {
    struct SharedComponentTag {
        virtual ~SharedComponentTag() = default;
        virtual bool isEq(const SharedComponentTag& oth) const noexcept = 0;
    };

    template <typename T>
    struct TSharedComponentTag : public SharedComponentTag {
        virtual bool isEq(const SharedComponentTag& oth) const noexcept override {
            const T& self = *static_cast<const T*>(this);
            const T& rhs = static_cast<const T&>(oth);
            return self == rhs;
        }
    };
}
