// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

#include "CkCore/CkCoreMinimal.h"
#include "CkEcs/CkEcsMinimal.h"
#include "CkEcs/Handle/CkDebugCallstack_Fragment.h"

namespace ck
{
	// Templated utils class for debug callstack operations
	template<typename TTrackedFragment>
	class TCk_Utils_Debug_Callstack
	{
		using FCallstackFragment = typename TCallstackFragmentTrait<TTrackedFragment>::Type;

	public:
		// Add callstack entry without message
		static auto
		Add(
			FCk_Handle InEntity,
			const char* InFunction,
			int32 InLine)
			-> void;

		// Add callstack entry with formatted message
		static auto
		Add(
			FCk_Handle InEntity,
			const char* InFunction,
			int32 InLine,
			const FString& InMessage)
			-> void;

		// Clear all callstack entries for entity
		static auto
		Clear(FCk_Handle InEntity)
			-> void;
	};

} // namespace ck

#include "CkDebugCallstack_Utils.inl.h"
