//
// Created by kirill on 10/24/2025.
//
#pragma once

#include <new>
#include <utility>

namespace mustache {
	template<typename T, typename... Args>
	constexpr T* constructAt(void* p, Args&&... args) {
		return ::new (const_cast<void*>(static_cast<const volatile void*>(p)))
			T(std::forward<Args>(args)...);
	}
}
