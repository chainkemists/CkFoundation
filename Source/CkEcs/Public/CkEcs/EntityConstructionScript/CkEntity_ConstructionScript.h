#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include <InstancedStruct.h>

#include "CkEntity_ConstructionScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKECS_API FCk_Handle_UnderConstruction : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_UnderConstruction); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_UnderConstruction);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKECS_API UCk_Entity_ConstructionScript_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Entity_ConstructionScript_PDA);

public:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams) const -> void;

protected:
    UFUNCTION(BlueprintNativeEvent,
              DisplayName = "Construct")
    void
    DoConstruct(
        UPARAM(ref) FCk_Handle_UnderConstruction& InHandle,
        const FInstancedStruct& InOptionalParams) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKECS_API UCk_Entity_ConstructionScript_WithTransform_PDA : public UCk_Entity_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Entity_ConstructionScript_WithTransform_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|Config",
              meta = (ExposeOnSpawn, AllowPrivateAccess = true))
    FTransform _EntityInitialTransform;

public:
    CK_PROPERTY(_EntityInitialTransform);
};

// --------------------------------------------------------------------------------------------------------------------
