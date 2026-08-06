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

    // A unique instance per entity, recycled through the CkCore ObjectPooling subsystem (config in
    // _PoolParams). Construct/BeginPlay re-run per acquire. See ObjectPooling/README.md
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

    // Snapshot respawn opt-in, with exactly two consumers:
    //  (a) UCk_EntityScript_WithActor_UE::Construct stamps FFragment_ActorSpawnIntent when this returns true, which
    //      is what makes the capture record an actor recipe and the load re-spawn the bridged actor.
    //  (b) The v3 load reads the CDO of a RuntimeSpawned entry whose lifetime owner was NOT persisted (i.e. the world
    //      root / transient): opted-in classes are respawned there, the rest are skipped so the fresh world's own
    //      copy is not duplicated. Because that test also covers non-bridged scripts, the flag lives on this base
    //      (CkEcs) rather than on the WithActor subclass in CkEcsExt.
    // It is NOT a spawn guard for the whole load: Get_IsSpawnSuppressedByLoadGate (CkEntityScript_Utils.cpp)
    // does quarantine Request_SpawnEntity while the load GATE is active (outside the loader/rendezvous/
    // construction admission windows), but the gate drops partway through hydration while the load itself runs
    // on through Settling — and a producer whose spawn REQUESTS drain via a load-gated processor (e.g. an
    // entity script's DoBeginPlay enqueuing spawns) lands entirely after the gate lifts, where nothing
    // suppresses it. Protecting against a duplicate beside the loader's restore therefore remains the
    // producer's responsibility: gate construction-time seeding on UCk_Utils_Snapshot_UE::Get_IsLoadInProgress,
    // or reconcile (spawn only what is missing) after Promise_OnLoadComplete.
    // Default false: framework infrastructure (ActorRelay / CueRelay / StateMachineRelay, etc.) and ordinary scripts
    // spawn normally and are wiped by the restore's registry clear.
    //
    // The base implementation reads _SnapshotRespawnable, so AngelScript/Blueprint classes opt in with
    // `default _SnapshotRespawnable = true;` — no C++ shim base needed (same CDO-property-behind-a-virtual
    // shape as _Replication / Get_EffectiveReplication). C++ classes may still override the virtual outright.
    [[nodiscard]]
    virtual auto
    Get_IsSnapshotRespawnable() const -> bool;

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
        meta=(AllowPrivateAccess = true, EditCondition="ShowReplicationInEditor()", EditConditionHides))
    ECk_Replication _Replication = ECk_Replication::Replicates;

    // Backing data for Get_IsSnapshotRespawnable — see the opt-in contract on the virtual above.
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess = true))
    bool _SnapshotRespawnable = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess = true, InvalidEnumValues = "NotInstanced"))
    ECk_EntityScript_InstancingPolicy _InstancingPolicy = ECk_EntityScript_InstancingPolicy::InstancedPerEntity_Poolable;

    // pool config for InstancedPerEntity_Poolable (ignored otherwise); a per-class project setting overrides it
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess = true,
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
    // No getter for _Replication on purpose — read Get_EffectiveReplication instead.
    CK_PROPERTY_GET(_InstancingPolicy);
    CK_PROPERTY_GET(_PoolParams);
    CK_PROPERTY_GET(_AssociatedEntity);
    CK_PROPERTY_GET(_ShowInPlaceActors);
};

// ----------------------------------------------------------------------------------------------------------