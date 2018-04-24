#pragma once

template<class T>
T alignPowerOfTwo(T size, T alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}