#pragma once

#include "CkCore/Object/CkWorldContextObject.h"
#include "CkCore/ObjectPooling/CkObjectPooling_Params.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CoreMinimal.h"

#include "CkEntityScript.generated.h"

// -----------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityScript_SpawnEntity_HandleRequests;
}

// -----------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityScript_ConstructionFlow : uint8
{
    // Construction is finished. BeginPlay can be called
    Finished UMETA(DisplayName = "Finish Construction"),

    // Construction is still ongoing. ContinueConstruction event will be called and the user will need
    // to manually invoke FinishConstruction in order for BeginPlay to be called
    Continue UMETA(DisplayName = "Continue Construction")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityScript_ConstructionFlow);

// -----------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityScript_InstancingPolicy : uint8
{
    // This EntityScript is never instanced. Any entity that uses the EntityScript is operating on the CDO
    NotInstanced UMETA(DisplayName = "Not Instanced (uses CDO)"),

    // A new unique instance of the EntityScript created.
    InstancedPerEntity,

    // A unique instance per entity, recycled through the CkCore ObjectPooling subsystem: EndPlay
    // releases the instance to its pool; the next spawn of the same class+archetype re-issues it with
    // all reflected properties reset to the archetype (FCk_Handle_ObjectPoolingParticipant properties
    // are skipped so delegates bound on the instance survive). Construct/BeginPlay re-run per acquire.
    // Pool configuration comes from _PoolParams (visible when this policy is selected)
    InstancedPerEntity_Poolable UMETA(DisplayName = "Instanced Per Entity (Poolable)"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityScript_InstancingPolicy);

// -----------------------------------------------------------------------------------------------------------

UCLASS(Abstract, NotBlueprintable, BlueprintType, EditInlineNew)
class CKECS_API UCk_EntityScript_UE : public UCk_GameWorldContextObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_UE);

public:
    friend class ck::FProcessor_EntityScript_SpawnEntity_HandleRequests;
    friend class UCk_Utils_EntityScript_UE;

public:
    [[nodiscard]]
    virtual auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow;

    virtual auto
    ContinueConstruction(
        FCk_Handle InHandle) -> void;

    virtual auto
    BeginPlay() -> void;

    virtual auto
    EndPlay() -> void;

public:
    auto
    GetPrimaryAssetId() const -> FPrimaryAssetId override;

    UFUNCTION()
    virtual bool
    ShowReplicationInEditor() const;

    [[nodiscard]]
    virtual auto
    Get_AreSpawnParamsMatching(
        const FInstancedStruct& InClientSpawnParams,
        const UCk_EntityScript_UE* InConstructedScript) const -> bool;

    [[nodiscard]]
    virtual auto
    Get_EffectiveReplication() const -> ECk_Replication;

protected:
    UFUNCTION(BlueprintPure,
        Category = "Ck|EntityScript",
        DisplayName = "[Ck][EntityScript] Get Script Entity",
        meta = (CompactNodeTitle="ScriptEntity", Keywords="this, associated", HideSelfPin = true))
    FCk_Handle
    DoGet_ScriptEntity() const;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|EntityScript",
              DisplayName = "[Ck][EntityScript] Finish Construction",
              meta = (CompactNodeTitle="✔Constructed", HideSelfPin = true, Keywords = "ongoing"))
    void
    DoFinishConstruction();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess, EditCondition="ShowReplicationInEditor()", EditConditionHides))
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess, InvalidEnumValues = "NotInstanced"))
    ECk_EntityScript_InstancingPolicy _InstancingPolicy = ECk_EntityScript_InstancingPolicy::InstancedPerEntity;

    // Pool configuration for InstancedPerEntity_Poolable — ignored under any other policy. The
    // recycle policy is forced to Recycle at spawn; a project-settings entry for this class
    // overrides these values at pool creation
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess,
            EditCondition="_InstancingPolicy == ECk_EntityScript_InstancingPolicy::InstancedPerEntity_Poolable",
            EditConditionHides))
    FCk_ObjectPooling_PoolParams _PoolParams;

    // Opt-in: when true, this EntityScript appears in the 'Ck Entity Scripts' tab of the editor's
    // Place Actors panel and can be dragged straight into a level (the drag spawns a configured
    // ACk_EntitySpawner_UE). AngelScript: 'default _ShowInPlaceActors = true;'.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess = true))
    bool _ShowInPlaceActors = false;

    UPROPERTY(Transient)
    FCk_Handle _AssociatedEntity;

    UPROPERTY(EditDefaultsOnly,
        Category="Developer Settings", AssetRegistrySearchable, AdvancedDisplay,
        meta = (AllowPrivateAccess = true))
    FName _AssetRegistryCategory = TEXT("CkEntityScript");

public:
    // CK_PROPERTY_GET(_Replication); // Use Get_EffectiveReplication
    CK_PROPERTY_GET(_InstancingPolicy);
    CK_PROPERTY_GET(_PoolParams);
    CK_PROPERTY_GET(_AssociatedEntity);
    CK_PROPERTY_GET(_ShowInPlaceActors);
};

// ----------------------------------------------------------------------------------------------------------