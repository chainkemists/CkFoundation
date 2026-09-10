#pragma once

#include "CkGroundNav/Streaming/CkGroundNav_StreamPartitionLifecycle.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedFieldLoad.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkGroundNav_StreamManifestSubsystem_UE.generated.h"

class ACk_GroundNav_StreamManifestBinding_UE;
class ULevel;
enum class EDataLayerRuntimeState : uint8;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Converts real ULevel streaming events into manifest source transactions. It never derives a World
 * Partition cell id. The binding actor carries the durable source identity; level lifetime creates
 * and removes sources, while the effective runtime state of every authored data layer masks an already
 * resident source without making a second identity from the engine event.
 */
UCLASS()
class CKGROUNDNAV_API UCk_GroundNav_StreamManifestSubsystem_UE : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;
    auto Tick(float InDeltaTime) -> void override;
    auto GetStatId() const -> TStatId override;
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;

    /** Pure all-selected policy for test fixtures: empty means active; every selected layer must be Activated. */
    static auto Get_AreDataLayerRuntimeStatesActivated(
        TConstArrayView<EDataLayerRuntimeState> InStates) -> bool;

    /** Pure authored-ownership gate: a cell binding may load only its exact generated coordinate set. */
    static auto Get_AreManifestTileCoordsCompatible(
        TConstArrayView<FIntPoint> InBindingCoords,
        TConstArrayView<FIntPoint> InManifestCoords) -> bool;

    /**
     * The binding-level atomic manifest boundary. A coordinate mismatch refuses before the loader can
     * touch OutTransitions; all deeper asset/blob checks remain in Try_LoadCookedSourceManifest.
     */
    static auto Try_LoadBoundCookedSourceManifest(
        const UCk_GroundNav_CookedSourceManifest_UE& InManifest,
        TConstArrayView<FIntPoint> InBindingCoords,
        const ck::groundnav::FCk_GroundNav_StreamFieldBundle& InTemplateBundle,
        uint64 InDefaultFingerprint,
        const TMap<FGameplayTag, uint64>& InVariantFingerprints,
        int32 InStreamingVolumeId,
        int32 InPartitionId,
        const ck::groundnav::FCk_GroundNav_DataLayerSelector& InDataLayerSelector,
        TArray<ck::groundnav::FCk_GroundNav_StreamTileTransition>& OutTransitions) -> ECk_GroundNav_CookStatus;

private:
    struct FBindingState
    {
        TWeakObjectPtr<ACk_GroundNav_StreamManifestBinding_UE> _Binding;
        TWeakObjectPtr<ULevel> _Level;
        ck::groundnav::stream_partitions::FCk_GroundNav_StreamPartitionKey _Key;
        ck::groundnav::FCk_GroundNav_DataLayerSelector _DataLayerSelector;
        TArray<FIntPoint> _TileCoords;
        uint64 _OwnerInstance = 0;
        uint64 _Generation = 0;
        bool _AwaitingOwner = true;
        bool _Loaded = false;
        bool _DataLayersActivated = false;
        bool _Unloading = false;
        bool _TerminalFailure = false;
    };

    auto DoHandle_LevelAdded(ULevel* InLevel, UWorld* InWorld) -> void;
    auto DoHandle_LevelRemoved(ULevel* InLevel, UWorld* InWorld) -> void;
    auto DoTry_Load(FBindingState& InOutState) -> void;
    auto DoUnload(FBindingState& InOutState) -> void;
    auto DoUpdate_DataLayerActivation(FBindingState& InOutState) -> void;
    auto Get_AreDataLayersActivated(const FBindingState& InState) const -> bool;
    auto Get_IsBlockedByUnloadingSource(const FBindingState& InState) const -> bool;
    auto Get_NextGeneration(const ck::groundnav::stream_partitions::FCk_GroundNav_StreamPartitionKey& InKey) -> uint64;

private:
    FDelegateHandle _LevelAddedHandle;
    FDelegateHandle _LevelRemovedHandle;
    TArray<FBindingState> _Bindings;
    TMap<ck::groundnav::stream_partitions::FCk_GroundNav_StreamPartitionKey, uint64> _GenerationLedger;
};

// --------------------------------------------------------------------------------------------------------------------
