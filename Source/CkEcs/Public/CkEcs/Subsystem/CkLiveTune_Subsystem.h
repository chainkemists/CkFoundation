#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#if WITH_EDITOR
#include <InstancedStruct.h>
#include <UObject/ObjectKey.h>
#include <UObject/StrongObjectPtr.h>
#include <UObject/UnrealType.h>
#endif

#include "CkLiveTune_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_SpawnRecipe_UE;
class UCk_PendingHydrationPayloads_UE;

// --------------------------------------------------------------------------------------------------------------------

// LiveTune's change listener (docs/specs/2026-08-05-LiveTune-design.md §4.3): maps (tuning asset, member)
// edits back to linked entities and dispatches through FCk_LiveTuneHandlerRegistry, behind three mandatory
// gates — a per-(asset, member) value-diff cache (an AS hot reload heals ALL asset literals on every
// save; without the diff, every save would re-apply world-wide), a change-type policy (Interactive slider
// scrubs reach ViaReplace only; commits reach all tiers), and an authority gate (client-mode entities of
// replicated features are skipped — the server's own dispatch replicates down). Also owns the ViaRebuild
// driver: capture (persistence Produce) -> deferred destroy -> re-Add once the dying entity is actually
// gone (records disconnect during teardown, so re-Add can never collide with a same-named dying entry) ->
// hydrate via FProcessor_Hydration_Dispatch -> re-link. Ticks only to advance pending rebuilds. Never
// instantiated outside editor builds.
UCLASS(DisplayName = "CkSubsystem_LiveTune")
class CKECS_API UCk_LiveTune_Subsystem_UE : public UCk_Game_TickableWorldSubsystem_Base_UE
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

    auto
    Tick(
        float InDeltaTime) -> void override;

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

    auto
    Test_Get_PendingRebuildCount() const -> int32;

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

    struct FPendingRebuild
    {
        ECk_LiveTune_RebuildScope _Scope = ECk_LiveTune_RebuildScope::Feature;
        FCk_Handle _DyingEntity;
        FCk_Handle _Owner;
        FCk_LiveTuneHandlerRegistry::FReAddFn _ReAdd;
        FInstancedStruct _FreshParams;
        FStampKey _Key;

        // The pin holders root any object refs inside the captured payloads / fresh params through GC —
        // a plain FInstancedStruct member of a non-reflected struct is not traced. _FreshParams stays
        // safe alongside its pin because both copies point at the same (pinned) objects.
        TStrongObjectPtr<UCk_PendingHydrationPayloads_UE> _PinnedFreshParams;
        TStrongObjectPtr<UCk_PendingHydrationPayloads_UE> _PinnedLinkedPayloads;
        TStrongObjectPtr<UCk_EntityScript_SpawnRecipe_UE> _Recipe;

        float _PendingForSeconds = 0.0f;
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
    DoBeginRebuild(
        FCk_Handle& InEntity,
        const FCk_LiveTuneHandlerRegistry::FHandler& InHandler,
        const FInstancedStruct& InFreshParams,
        const FStampKey& InKey) -> bool;

    auto
    DoFinishRebuild(
        FPendingRebuild& InPending) -> void;

    auto
    DoEnqueueHydration(
        FCk_Handle& InTarget,
        FInstancedStruct InPayload) -> void;

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
    TArray<FPendingRebuild> _PendingRebuilds;
    FDelegateHandle _OnObjectPropertyChangedHandle;
    entt::scoped_connection _StampDestroyConnection;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
