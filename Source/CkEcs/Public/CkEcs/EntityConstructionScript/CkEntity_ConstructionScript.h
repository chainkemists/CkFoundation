#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEntity_ConstructionScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKECS_API UCk_EntityBridge_ConstructionScript_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityBridge_ConstructionScript_PDA);

public:
    auto
    Construct(
        FCk_Handle InHandle) -> void;

protected:
    UFUNCTION(BlueprintNativeEvent,
              DisplayName = "Construct")
    void
    DoConstruct(
        const FCk_Handle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKECS_API UCk_EntityBridge_ConstructionScript_WithTransform_PDA : public UCk_EntityBridge_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityBridge_ConstructionScript_WithTransform_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn, AllowPrivateAccess = true))
    FTransform _EntityInitialTransform;

public:
    CK_PROPERTY(_EntityInitialTransform);
};

// --------------------------------------------------------------------------------------------------------------------
