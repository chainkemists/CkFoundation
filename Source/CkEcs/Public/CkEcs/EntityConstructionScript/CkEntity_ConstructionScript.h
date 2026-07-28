#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include <StructUtils/InstancedStruct.h>

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
    // Immediate mutator: the construction script runs inline and nothing is enqueued, so the completion
    // delegate fires synchronously with Succeeded on the caller's own stack.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|ConstructionScript",
              DisplayName = "[Ck] Request Construct Sub-ConstructionScript",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_Construct(
        UPARAM(ref) FCk_Handle& InHandle,
        TSubclassOf<UCk_Entity_ConstructionScript_PDA> InConstructionScript,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Immediate mutator — see Request_Construct.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|ConstructionScript",
              DisplayName = "[Ck] Request Construct Sub-ConstructionScript (Instanced)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_Construct_Instanced(
        UPARAM(ref) FCk_Handle& InHandle,
        const UCk_Entity_ConstructionScript_PDA* InConstructionScript,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Immediate mutator — see Request_Construct. Completion reports that the batch was walked, not that
    // every individual script was valid.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|ConstructionScript",
              DisplayName = "[Ck] Request Construct Sub-ConstructionScript (Multiple)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_Construct_Multiple(
        UPARAM(ref) FCk_Handle& InHandle,
        TArray<TSubclassOf<UCk_Entity_ConstructionScript_PDA>> InConstructionScript,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

protected:
    UFUNCTION(BlueprintNativeEvent,
              DisplayName = "Construct")
    void
    DoConstruct(
        UPARAM(ref) FCk_Handle& InHandle) const;

    UFUNCTION()
    virtual bool ShowReplicationInEditor() const;

private:
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess = true, EditCondition="ShowReplicationInEditor()", EditConditionHides))
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
