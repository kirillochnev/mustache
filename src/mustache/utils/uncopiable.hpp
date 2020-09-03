#pragma once

namespace mustache {
    class Uncopiable {
    public:
        Uncopiable() = default;
        Uncopiable(Uncopiable&) = default;
        Uncopiable(const Uncopiable&) = delete;
        Uncopiable& operator=(const Uncopiable&) = delete;
        Uncopiable& operator=(Uncopiable&&) = default;
    };

}

