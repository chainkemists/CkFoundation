// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

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
			const char* FunctionName = nullptr;
			int32 LineNumber = 0;
			FString Message;

			// Captured callstacks (only populated if corresponding CVar is enabled)
			TArray<FString> CppCallstack;
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
