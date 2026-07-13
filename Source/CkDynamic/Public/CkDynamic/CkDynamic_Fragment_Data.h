#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

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
