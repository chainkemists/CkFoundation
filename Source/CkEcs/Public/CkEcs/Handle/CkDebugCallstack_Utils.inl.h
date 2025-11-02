// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

#include "CkEcs/Handle/CkDebugCallstack_Utils.h"

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
