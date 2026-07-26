// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

#include "CkCore/Debug/CkDebug_Utils.h"

namespace ck
{
	template<typename TTrackedFragment>
	struct TCallstackFragmentTrait;

	template<typename TTrackedFragment>
	class TCk_Utils_Debug_Callstack;

	template<typename TTrackedFragment>
	struct TFragment_Debug_Callstack
	{
		CK_GENERATED_BODY(TFragment_Debug_Callstack);

		friend class TCk_Utils_Debug_Callstack<TTrackedFragment>;

		struct FCallEntry
		{
			uint64 FrameNumber = 0;
			const char* FunctionName = nullptr;
			int32 LineNumber = 0;
			FString Message;

			// Pointers into the global symbol cache: a background thread resolves each address in
			// place, so a live entry reads "<resolving...>" until it flips to the real symbol.
			TArray<const FString*> CppCallstackAddresses;

			TArray<FString> BlueprintCallstack;
			TArray<FString> AngelscriptCallstack;
		};

	private:
		TArray<FCallEntry> _Entries;

	public:
		CK_PROPERTY_GET(_Entries);
	};

	template<typename TTrackedFragment>
	struct TCallstackFragmentTrait
	{
		// Specialized only by CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR
	};

} // namespace ck
