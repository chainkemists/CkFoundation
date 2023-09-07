#pragma once

#include <tuple>

namespace ck
{
	template <typename... T_Args>
	struct TPayload
	{
		std::tuple<T_Args...> Payload;
	};

	template<typename... T_Args>
	auto MakePayload(T_Args&&... InArgs)
	{
		return TPayload<T_Args...>{ {std::forward<T_Args>(InArgs)...} };
	}
}
