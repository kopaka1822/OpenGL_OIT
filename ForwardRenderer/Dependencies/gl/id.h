#pragma once
#include <algorithm>

namespace gl
{
	// struct that ensures that a value can only be moved ond not copied
	// if the move constructor is called, the value will be initialized to the provided default value
	template<class T, T defaultValue = T(0)>
	struct unique
	{
		T value;
		unique() : value(defaultValue) {}
		~unique() = default;
		explicit unique(const T& v) : value(v) {}
		unique(const unique&) = delete;
		unique& operator=(const unique&) = delete;
		unique(unique&& m) noexcept
			: value(m.value)
		{
			m.value = defaultValue;
		}
		unique& operator=(unique&& m) noexcept
		{
			std::swap(value, m.value);
			return *this;
		}
		unique& operator=(const T& v)
		{
			value = v;
			return *this;
		}
		operator T() const noexcept
		{
			return value;
		}
		T* operator&() noexcept // NOLINT
		{
			return &value;
		}
	};
}