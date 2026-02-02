// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkDebugCallstack_Fragment.h"
#include "CkEcs/Handle/CkDebugCallstack_Utils.h"

#if WITH_ANGELSCRIPT_CK
#include "CkCore/Debug/CkDebug_Utils.h"
#include <AngelscriptBinds.h>
#endif

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

// ============================================================================
// ANGELSCRIPT BINDINGS MACRO
// ============================================================================

#if WITH_ANGELSCRIPT_CK && !UE_BUILD_SHIPPING

// Define Angelscript bindings for callstack utils
// Creates namespace utils_{feature}_debug_callstack with Record and Clear functions
//
// Parameters:
//   _API_         - Module API macro (e.g., CKTIMER_API)
//   _feature_     - Feature name in lowercase (e.g., timer) - used in namespace name
//   _FragmentType_ - The fragment type being tracked (e.g., ck::FFragment_Timer_Current)
//
// Usage:
//   CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKTIMER_API, timer, ck::FFragment_Timer_Current);
//
// Angelscript:
//   utils_timer_debug_callstack::Record(MyTimer, "Message");
//   utils_timer_debug_callstack::Clear(MyTimer);
//
#define CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(_API_, _feature_, _FragmentType_) \
	AS_FORCE_LINK const FAngelscriptBinds::FBind _API_ CK_UNIQUE_NAME(Bind_Callstack_##_feature_)( \
		FAngelscriptBinds::EOrder::Late, \
		[]() \
		{ \
			auto Namespace = FAngelscriptBinds::FNamespace{FString(TEXT("utils_" #_feature_ "_debug_callstack"))}; \
			\
			FAngelscriptBinds::BindGlobalFunction( \
				"void Record(const FCk_Handle& in InEntity, const FString& in InMessage = FString())", \
				[](const FCk_Handle& InEntity, const FString& InMessage) -> void \
				{ \
					ck::TCk_Utils_Debug_Callstack<_FragmentType_>::Add(InEntity, nullptr, 0, InMessage); \
				}); \
			\
			FAngelscriptBinds::BindGlobalFunction( \
				"void Clear(const FCk_Handle& in InEntity)", \
				[](const FCk_Handle& InEntity) -> void \
				{ \
					ck::TCk_Utils_Debug_Callstack<_FragmentType_>::Clear(InEntity); \
				}); \
		});

#else

#define CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(_API_, _feature_, _FragmentType_)

#endif // WITH_ANGELSCRIPT_CK && !UE_BUILD_SHIPPING
