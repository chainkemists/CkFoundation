// Copyright (C) Crank Kit - All Rights Reserved

#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkDebugCallstack_Fragment.h"
#include "CkEcs/Handle/CkDebugCallstack_Utils.h"

#if WITH_ANGELSCRIPT_CK
#include "CkCore/Debug/CkDebug_Utils.h"
#include <AngelscriptBinds.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING

	#define CK_CALLSTACK_RECORD(FragmentType, Entity) \
		ck::TCk_Utils_Debug_Callstack<FragmentType>::Add(Entity, __FUNCTION__, __LINE__)

	#define CK_CALLSTACK_RECORD_MSG(FragmentType, Entity, _Format_, ...) \
		ck::TCk_Utils_Debug_Callstack<FragmentType>::Add( \
			Entity, \
			__FUNCTION__, \
			__LINE__, \
			ck::Format_UE(_Format_, ##__VA_ARGS__))

	#define CK_CALLSTACK_CLEAR(FragmentType, Entity) \
		ck::TCk_Utils_Debug_Callstack<FragmentType>::Clear(Entity)

#else

	#define CK_CALLSTACK_RECORD(FragmentType, Entity)
	#define CK_CALLSTACK_RECORD_MSG(FragmentType, Entity, Format, ...)
	#define CK_CALLSTACK_CLEAR(FragmentType, Entity)

#endif // !UE_BUILD_SHIPPING

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK && !UE_BUILD_SHIPPING

// Generates the AngelScript namespace utils_<_feature_>_debug_callstack with Record and Clear.
// _feature_ is an unquoted lowercase token pasted into that namespace name (e.g. timer).
// Usage: CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKTIMER_API, timer, ck::FFragment_Timer_Requests);
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
