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
			// Skip 1 frame to exclude this utility function
			Entry.CppCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace(
				ck::type_traits::AsArray{},
				1,
				ECk_StackTraceVerbosity_Policy::Compact);
		}

		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Blueprint())
		{
			Entry.BlueprintCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(
				ck::type_traits::AsArray{});
		}

		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Angelscript())
		{
			Entry.AngelscriptCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(
				ck::type_traits::AsArray{});
		}

		// Add to entries
		CallstackFragment._Entries.Add(Entry);
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
			// Skip 1 frame to exclude this utility function
			Entry.CppCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace(
				ck::type_traits::AsArray{},
				1,
				ECk_StackTraceVerbosity_Policy::Compact);
		}

		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Blueprint())
		{
			Entry.BlueprintCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(
				ck::type_traits::AsArray{});
		}

		if (UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Angelscript())
		{
			Entry.AngelscriptCallstack = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Angelscript(
				ck::type_traits::AsArray{});
		}

		// Add to entries
		CallstackFragment._Entries.Add(Entry);
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
