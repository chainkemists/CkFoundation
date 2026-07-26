// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

#include "CkEcs/Handle/CkDebugCallstack_Fragment.h"

namespace ck
{
	template<typename TTrackedFragment>
	class TCk_Utils_Debug_Callstack
	{
		using FCallstackFragment = typename TCallstackFragmentTrait<TTrackedFragment>::Type;

	public:
		static auto
		Add(
			FCk_Handle InEntity,
			const char* InFunction,
			int32 InLine)
			-> void;

		static auto
		Add(
			FCk_Handle InEntity,
			const char* InFunction,
			int32 InLine,
			const FString& InMessage)
			-> void;

		static auto
		Clear(FCk_Handle InEntity)
			-> void;
	};

} // namespace ck

#include "CkDebugCallstack_Utils.inl.h"
