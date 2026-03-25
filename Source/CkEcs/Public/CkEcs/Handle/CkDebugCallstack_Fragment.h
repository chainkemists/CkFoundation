// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

#include "CkCore/Debug/CkDebug_Utils.h"

namespace ck
{
	// Forward declarations
	template<typename TTrackedFragment>
	struct TCallstackFragmentTrait;

	template<typename TTrackedFragment>
	class TCk_Utils_Debug_Callstack;

	// Template base fragment for debug callstack tracking
	template<typename TTrackedFragment>
	struct TFragment_Debug_Callstack
	{
		CK_GENERATED_BODY(TFragment_Debug_Callstack);

		friend class TCk_Utils_Debug_Callstack<TTrackedFragment>;

		struct FCallEntry
		{
			uint64 FrameNumber = 0;
			const char* FunctionName = nullptr;  // Compile-time __FUNCTION__
			int32 LineNumber = 0;                // Compile-time __LINE__
			FString Message;

			// C++ callstack: Pointers to global symbol cache
			// Background thread resolves addresses asynchronously
			// VS Debugger: Just expand this array to see symbols!
			// Initially shows "<resolving...>", updates to actual symbols automatically
			TArray<const FString*> CppCallstackAddresses;

			// Blueprint/Angelscript callstacks: Resolved (expensive)
			// Only populated if corresponding CVars are enabled
			TArray<FString> BlueprintCallstack;
			TArray<FString> AngelscriptCallstack;
		};

	private:
		TArray<FCallEntry> _Entries;

	public:
		CK_PROPERTY_GET(_Entries);
	};

	// Trait for mapping tracked fragment type to callstack fragment type
	template<typename TTrackedFragment>
	struct TCallstackFragmentTrait
	{
		// Specializations defined via CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR macro
	};

} // namespace ck
