#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle.h"

#include <InstancedStruct.h>

#include "CkEntity_ConstructionScript.generated.h"

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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|ConstructionScript",
              DisplayName = "[Ck] Request Construct Sub-ConstructionScript")
    static FCk_Handle
    Request_Construct(
        UPARAM(ref) FCk_Handle& InHandle,
        TSubclassOf<UCk_Entity_ConstructionScript_PDA> InConstructionScript,
        const FInstancedStruct& InOptionalParams);

protected:
    UFUNCTION(BlueprintNativeEvent,
              DisplayName = "Construct")
    void
    DoConstruct(
        UPARAM(ref) FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams) const;

private:
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
    ECk_Replication _Replication = ECk_Replication::Replicates;

public:
    CK_PROPERTY_GET(_Replication);
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
