// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

#include "CkEcs/Handle/CkDebugCallstack_Utils.h"
#include "CkEcs/Settings/CkEcs_Settings.h"
#include "CkCore/Debug/CkDebug_Utils.h"

namespace ck
{
	template<typename TTrackedFragment>
	auto
		TCk_Utils_Debug_Callstack<TTrackedFragment>::
		Add(
			FCk_Handle InEntity,
			const char* InFunction,
			int32 InLine)
		-> void
	{
		CK_ENSURE_IF_NOT(ck::IsValid(InEntity), TEXT("Invalid entity handle"))
		{ return; }

		// Get or add callstack fragment
		auto& CallstackFragment = InEntity.AddOrGet<FCallstackFragment>();

		// Create entry
		auto Entry = typename FCallstackFragment::FCallEntry{};
		Entry.FrameNumber = GFrameCounter;
		Entry.FunctionName = InFunction;
		Entry.LineNumber = InLine;
		Entry.Message = FString{};

		// Capture callstacks based on user settings
		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Cpp())
		{
			// Fast address-only capture (no symbol resolution)
			// Skip 2 frames: this utility function + CK_CALLSTACK_RECORD macro
			constexpr auto SkipFrames = 2;
			const auto MaxFrames = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackFrames_Cpp();
			const auto Addresses = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_AddressesOnly(
				MaxFrames,
				SkipFrames);

			// Convert addresses to pointers into global cache
			// Background thread will resolve symbols asynchronously
			Entry.CppCallstackAddresses.Reserve(Addresses.Num());
			for (const auto Address : Addresses)
			{
				const auto* SymbolPtr = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_ResolveAddress(Address);
				Entry.CppCallstackAddresses.Add(SymbolPtr);
			}
		}

		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Blueprint())
		{
			const auto Override = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackFrames_Blueprint_Override();
			if (Override > 0)
			{
				Entry.BlueprintCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(
					ck::type_traits::AsArray{},
					Override);
			}
			else
			{
				Entry.BlueprintCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(
					ck::type_traits::AsArray{});
			}
		}

		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Angelscript())
		{
			const auto Override = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackFrames_Angelscript_Override();
			if (Override > 0)
			{
				Entry.AngelscriptCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(
					ck::type_traits::AsArray{},
					Override);
			}
			else
			{
				Entry.AngelscriptCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(
					ck::type_traits::AsArray{});
			}
		}

		// Add to entries
		CallstackFragment._Entries.Add(Entry);

		// Enforce max entries limit (FIFO - remove oldest)
		const auto MaxEntries = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackEntries();
		if (CallstackFragment._Entries.Num() > MaxEntries)
		{
			const auto NumToRemove = CallstackFragment._Entries.Num() - MaxEntries;
			CallstackFragment._Entries.RemoveAt(0, NumToRemove, EAllowShrinking::No);
		}
	}

	template<typename TTrackedFragment>
	auto
		TCk_Utils_Debug_Callstack<TTrackedFragment>::
		Add(
			FCk_Handle InEntity,
			const char* InFunction,
			int32 InLine,
			const FString& InMessage)
		-> void
	{
		CK_ENSURE_IF_NOT(ck::IsValid(InEntity), TEXT("Invalid entity handle"))
		{ return; }

		// Get or add callstack fragment
		auto& CallstackFragment = InEntity.AddOrGet<FCallstackFragment>();

		// Create entry
		auto Entry = typename FCallstackFragment::FCallEntry{};
		Entry.FrameNumber = GFrameCounter;
		Entry.FunctionName = InFunction;
		Entry.LineNumber = InLine;
		Entry.Message = InMessage;

		// Capture callstacks based on user settings
		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Cpp())
		{
			// Fast address-only capture (no symbol resolution)
			// Skip 2 frames: this utility function + CK_CALLSTACK_RECORD_MSG macro
			constexpr auto SkipFrames = 2;
			const auto MaxFrames = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackFrames_Cpp();
			const auto Addresses = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_AddressesOnly(
				MaxFrames,
				SkipFrames);

			// Convert addresses to pointers into global cache
			// Background thread will resolve symbols asynchronously
			Entry.CppCallstackAddresses.Reserve(Addresses.Num());
			for (const auto Address : Addresses)
			{
				const auto* SymbolPtr = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_ResolveAddress(Address);
				Entry.CppCallstackAddresses.Add(SymbolPtr);
			}
		}

		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Blueprint())
		{
			const auto Override = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackFrames_Blueprint_Override();
			if (Override > 0)
			{
				Entry.BlueprintCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(
					ck::type_traits::AsArray{},
					Override);
			}
			else
			{
				Entry.BlueprintCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(
					ck::type_traits::AsArray{});
			}
		}

		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Angelscript())
		{
			const auto Override = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackFrames_Angelscript_Override();
			if (Override > 0)
			{
				Entry.AngelscriptCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(
					ck::type_traits::AsArray{},
					Override);
			}
			else
			{
				Entry.AngelscriptCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(
					ck::type_traits::AsArray{});
			}
		}

		// Add to entries
		CallstackFragment._Entries.Add(Entry);

		// Enforce max entries limit (FIFO - remove oldest)
		const auto MaxEntries = UCk_Utils_Ecs_Settings_UE::Get_MaxCallstackEntries();
		if (CallstackFragment._Entries.Num() > MaxEntries)
		{
			const auto NumToRemove = CallstackFragment._Entries.Num() - MaxEntries;
			CallstackFragment._Entries.RemoveAt(0, NumToRemove, EAllowShrinking::No);
		}
	}

	template<typename TTrackedFragment>
	auto
		TCk_Utils_Debug_Callstack<TTrackedFragment>::
		Clear(FCk_Handle InEntity)
		-> void
	{
		CK_ENSURE_IF_NOT(ck::IsValid(InEntity), TEXT("Invalid entity handle"))
		{ return; }

		// Check if fragment exists
		if (InEntity.Has<FCallstackFragment>() == false)
		{
			return;
		}

		// Clear all entries
		auto& CallstackFragment = InEntity.Get<FCallstackFragment>();
		CallstackFragment._Entries.Empty();
	}

} // namespace ck
