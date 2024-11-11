#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle.h"

#include <InstancedStruct.h>

#include "CkEntity_ConstructionScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE()
class CKECS_API UCk_Entity_ConstructionScript_Interface : public UInterface { GENERATED_BODY() };
class CKECS_API ICk_Entity_ConstructionScript_Interface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent,
        Category = "Ck|Entity",
        DisplayName = "ConstructionScript")
    void
    DoConstruct(
        UPARAM(ref) FCk_Handle& InHandle) const;

};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKECS_API UCk_Entity_ConstructionScript_PDA : public UCk_DataAsset_PDA, public ICk_Entity_ConstructionScript_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Entity_ConstructionScript_PDA);

public:
    auto
    Construct(
        FCk_Handle& InHandle,
        const UObject* InOptionalObjectConstructionScript = nullptr) const -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|ConstructionScript",
              DisplayName = "[Ck] Request Construct Sub-ConstructionScript")
    static FCk_Handle
    Request_Construct(
        UPARAM(ref) FCk_Handle& InHandle,
        TSubclassOf<UCk_Entity_ConstructionScript_PDA> InConstructionScript,
        const FInstancedStruct& InOptionalParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|ConstructionScript",
              DisplayName = "[Ck] Request Construct Sub-ConstructionScript (Multiple)")
    static FCk_Handle
    Request_Construct_Multiple(
        UPARAM(ref) FCk_Handle& InHandle,
        TArray<TSubclassOf<UCk_Entity_ConstructionScript_PDA>> InConstructionScript,
        const FInstancedStruct& InOptionalParams);

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
