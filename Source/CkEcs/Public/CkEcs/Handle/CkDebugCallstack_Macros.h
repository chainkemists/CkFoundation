// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkDebugCallstack_Fragment.h"
#include "CkEcs/Handle/CkDebugCallstack_Utils.h"

// ============================================================================
// FRAGMENT DEFINITION MACRO
// ============================================================================

// Define a callstack fragment type for a tracked fragment type
// This creates both the fragment struct and the trait specialization
#define CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FragmentType) \
	struct FragmentType##_Callstack : public ck::TFragment_Debug_Callstack<FragmentType> \
	{ \
		CK_GENERATED_BODY(FragmentType##_Callstack); \
	}; \
	\
	template<> \
	struct ck::TCallstackFragmentTrait<FragmentType> \
	{ \
		using Type = FragmentType##_Callstack; \
	};

// ============================================================================
// CALLSTACK RECORDING MACROS (compile out in shipping builds)
// ============================================================================

#if !UE_BUILD_SHIPPING

	// Record callstack entry without message
	#define CK_CALLSTACK_RECORD(FragmentType, Entity) \
		ck::TCk_Utils_Debug_Callstack<FragmentType>::Add(Entity, __FUNCTION__, __LINE__)

	// Record callstack entry with formatted message
	#define CK_CALLSTACK_RECORD_MSG(FragmentType, Entity, _Format_, ...) \
		ck::TCk_Utils_Debug_Callstack<FragmentType>::Add( \
			Entity, \
			__FUNCTION__, \
			__LINE__, \
			ck::Format_UE(_Format_, ##__VA_ARGS__))

	// Clear all callstack entries for entity
	#define CK_CALLSTACK_CLEAR(FragmentType, Entity) \
		ck::TCk_Utils_Debug_Callstack<FragmentType>::Clear(Entity)

#else

	// No-op in shipping builds
	#define CK_CALLSTACK_RECORD(FragmentType, Entity)
	#define CK_CALLSTACK_RECORD_MSG(FragmentType, Entity, Format, ...)
	#define CK_CALLSTACK_CLEAR(FragmentType, Entity)

#endif // !UE_BUILD_SHIPPING
