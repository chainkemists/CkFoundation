#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#if WITH_EDITOR
#include <InstancedStruct.h>
#include <UObject/ObjectKey.h>
#include <UObject/UnrealType.h>
#endif

#include "CkLiveTune_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// LiveTune's change listener (docs/specs/2026-08-05-LiveTune-design.md §4.3): maps (tuning asset, member)
// edits back to linked entities and dispatches through FCk_LiveTuneHandlerRegistry, behind three mandatory
// gates — a per-(asset, member) value-diff cache (an AS hot reload heals ALL asset literals on every
// save; without the diff, every save would re-apply world-wide), a change-type policy (Interactive slider
// scrubs reach ViaReplace only; commits reach all tiers), and an authority gate (client-mode entities of
// replicated features are skipped — the server's own dispatch replicates down). Never instantiated
// outside editor builds.
UCLASS(DisplayName = "CkSubsystem_LiveTune")
class CKECS_API UCk_LiveTune_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_LiveTune_Subsystem_UE);

public:
    auto
    ShouldCreateSubsystem(
        UObject* InOuter) const -> bool override;

    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

#if WITH_EDITOR
public:
    auto
    Request_RegisterLink(
        FCk_Handle& InHandle,
        const UObject* InTuningAsset,
        FName InMemberName) -> void;

public:
    // Test seams (CkTests drives these through its own shim): broadcast a hand-built
    // FPropertyChangedEvent through the REAL FCoreUObjectDelegates path, so AutoTests exercise the full
    // subscription -> extraction -> gate -> dispatch pipeline without a details panel.
    auto
    Test_SimulatePropertyChange(
        const UObject* InTuningAsset,
        FName InMemberName,
        EPropertyChangeType::Type InChangeType) -> void;

    auto
    Test_Get_LinkCount(
        const UObject* InTuningAsset,
        FName InMemberName) const -> int32;

private:
    struct FStampKey
    {
        FObjectKey _Asset;
        FName _Member;

        auto operator==(const FStampKey&) const -> bool = default;

        friend auto GetTypeHash(const FStampKey& InKey) -> uint32
        {
            return HashCombineFast(GetTypeHash(InKey._Asset), GetTypeHash(InKey._Member));
        }
    };

    auto
    DoOnObjectPropertyChanged(
        UObject* InObject,
        FPropertyChangedEvent& InEvent) -> void;

    auto
    DoProcessMemberChange(
        const UObject* InAsset,
        FName InMemberName,
        EPropertyChangeType::Type InChangeType) -> void;

    auto
    OnStampDestroyed(
        ck::registry_table::EnttRegistryType& InRegistry,
        FCk_Entity::IdType InEntity) -> void;

    static auto
    DoTryReadMemberValue(
        const UObject* InAsset,
        FName InMemberName) -> FInstancedStruct;

private:
    TMap<FStampKey, TArray<FCk_Handle>> _LinkedEntities;
    TMap<FStampKey, FInstancedStruct> _LastDispatchedValues;
    FDelegateHandle _OnObjectPropertyChangedHandle;
    entt::scoped_connection _StampDestroyConnection;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
