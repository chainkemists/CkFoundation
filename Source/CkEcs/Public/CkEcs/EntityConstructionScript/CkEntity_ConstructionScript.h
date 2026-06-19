#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle.h"

#include <StructUtils/InstancedStruct.h>

#include "CkEntity_ConstructionScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Carries an optional, by-value construction config (a typed payload wrapped in an FInstancedStruct)
    // onto a freshly-built entity so a GENERIC construction script can read it instead of relying on a
    // per-type subclass CDO. Stamped (when present) immediately before Construct on both host and client
    // by the EntityReplicationDriver build loops, so the config rides replication via
    // FCk_EntityReplicationDriver_ConstructionInfo::_ConstructionConfig.
    struct CKECS_API FFragment_EntityConstructionConfig
    {
        CK_GENERATED_BODY(FFragment_EntityConstructionConfig);

    private:
        FInstancedStruct _Config;

    public:
        CK_PROPERTY_GET(_Config);

        CK_DEFINE_CONSTRUCTORS(FFragment_EntityConstructionConfig, _Config);
    };
}

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE()
class CKECS_API UCk_Entity_ConstructionScript_Interface : public UInterface { GENERATED_BODY() };
class CKECS_API ICk_Entity_ConstructionScript_Interface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent,
        Category = "Ck|Entity",
        meta=(ForceAsFunction),
        DisplayName = "ConstructionScript")
    void
    DoConstruct(
        UPARAM(ref) FCk_Handle& InHandle);
};

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
        UObject* InOptionalObjectConstructionScript = nullptr) const -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|ConstructionScript",
              DisplayName = "[Ck] Request Construct Sub-ConstructionScript")
    static FCk_Handle
    Request_Construct(
        UPARAM(ref) FCk_Handle& InHandle,
        TSubclassOf<UCk_Entity_ConstructionScript_PDA> InConstructionScript);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|ConstructionScript",
              DisplayName = "[Ck] Request Construct Sub-ConstructionScript (Instanced)")
    static FCk_Handle
    Request_Construct_Instanced(
        UPARAM(ref) FCk_Handle& InHandle,
        const UCk_Entity_ConstructionScript_PDA* InConstructionScript);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|ConstructionScript",
              DisplayName = "[Ck] Request Construct Sub-ConstructionScript (Multiple)")
    static FCk_Handle
    Request_Construct_Multiple(
        UPARAM(ref) FCk_Handle& InHandle,
        TArray<TSubclassOf<UCk_Entity_ConstructionScript_PDA>> InConstructionScript);

protected:
    UFUNCTION(BlueprintNativeEvent,
              DisplayName = "Construct")
    void
    DoConstruct(
        UPARAM(ref) FCk_Handle& InHandle) const;

    UFUNCTION()
    virtual bool ShowReplicationInEditor() const;

private:
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, EditCondition="ShowReplicationInEditor()", EditConditionHides))
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
