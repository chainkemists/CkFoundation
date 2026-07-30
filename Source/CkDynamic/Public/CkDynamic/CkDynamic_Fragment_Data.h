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

// Snapshot-transient dynamic fragments derive from this marker (C++), or carry a field of this type
// (AngelScript — script structs cannot inherit), because USTRUCT metadata is unavailable in Game builds.
// BlueprintType is load-bearing: the AngelScript binder only auto-binds BlueprintType structs, and the
// script-side marker-field spelling needs the type visible in AS.
USTRUCT(BlueprintType)
struct CKDYNAMIC_API FCk_DynamicFragment_SnapshotTransient
{
    GENERATED_BODY()
};

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
// Save payload — every dynamic fragment on ONE entity. Save-only: it never rides a replicated container.

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
// Carries WHICH type changed, never the data — handlers read the new value via Get_Fragment(Handle, ChangedType).
// Rationale: the ProcessEvent frame-buffer staleness hazard documented in CkDynamic_Utils.h.

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
