#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>
#include <StructUtils/InstancedStruct.h>

#include "CkDynamic_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_DestroyFilter : uint8
{
    None,
    IgnorePendingKill,
    Teardown
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_DestroyFilter);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKDYNAMIC_API FCk_Fragment_DynamicFragment_Data
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_DynamicFragment_Data);

private:
    UPROPERTY(meta=(SaveGame))
    FInstancedStruct _StructData;

public:
    CK_PROPERTY_GET(_StructData);
    CK_PROPERTY_GET_NON_CONST(_StructData);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_DynamicFragment_Data, _StructData);
};

// --------------------------------------------------------------------------------------------------------------------
// Save payload (G2) — every dynamic fragment on ONE entity, carried as a single hydration-registry type.
//
// Dynamic-fragment payload types are only known at runtime, so the network path routes them through the single
// runtime-typed FALLBACK handler (CkDynamic_Module.cpp). The save census (Get_SaveHandlerTypes) enumerates only
// per-type handlers and structurally cannot see that fallback — which is WHY this wrapper exists. It is registered
// as a REAL per-type handler (CkDynamic_Fragment.cpp) so a Produce/HydrationApply pair participates in the v3 save,
// carrying every stored FInstancedStruct verbatim (replicated or not). Handles nested inside those instanced structs
// remap through the CkEcs HandleWalk FInstancedStruct recursion. Save-only — this type never rides a replicated
// container.

USTRUCT()
struct CKDYNAMIC_API FCk_SaveData_DynamicFragments
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SaveData_DynamicFragments);

private:
    UPROPERTY()
    TArray<FInstancedStruct> _Fragments;

public:
    CK_PROPERTY(_Fragments);
};

// --------------------------------------------------------------------------------------------------------------------
// RepNotify payload — carries WHICH dynamic-fragment type changed (not the data, to avoid the
// ProcessEvent frame-buffer staleness hazard documented in CkDynamic_Utils.h). The bound handler
// reads the new value via UCk_Utils_DynamicFragment_UE::Get_Fragment(Handle, ChangedType).

USTRUCT(BlueprintType)
struct CKDYNAMIC_API FCk_DynamicFragment_RepNotifyInfo
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Ck|DynamicFragment")
    TObjectPtr<UScriptStruct> ChangedType = nullptr;
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_DynamicFragment_OnRepNotify,
    FCk_Handle, InHandle,
    FCk_DynamicFragment_RepNotifyInfo, InInfo);

// --------------------------------------------------------------------------------------------------------------------
