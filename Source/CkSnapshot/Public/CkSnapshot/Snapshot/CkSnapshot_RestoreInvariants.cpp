#include "CkSnapshot/Snapshot/CkSnapshot_RestoreInvariants.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h" // ck::FFragment_LifetimeOwner / FFragment_LifetimeDependents
#include "CkEcs/Handle/CkHandle.h"                          // ck::FFragment_ContextOwner, FCk_Handle

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    namespace
    {
        auto
            DoCheck_Handle(
                ck::SnapshotRegistryType& InRegistry,
                const FCk_Handle& InHandle,
                const TCHAR* InFragmentName,
                ck::SnapshotEntityType InOwnerEntity,
                TArray<FString>& OutDangling) -> void
        {
            const auto Entity = InHandle.Get_Entity();

            // Unset / tombstone handles are not references — skip them.
            if (Entity.Get_IsTombstone())
            { return; }

            const auto Id = Entity.Get_ID();
            if (Id == entt::null)
            { return; }

            if (InRegistry.valid(Id))
            { return; }

            OutDangling.Add(FString::Printf(
                TEXT("entity [%u] fragment [%s] -> dangling target entity [%u] (not present in restored registry)"),
                static_cast<uint32>(entt::to_integral(InOwnerEntity)),
                InFragmentName,
                static_cast<uint32>(entt::to_integral(Id))));
        }

        // FFragment_LifetimeDependents is a LAZILY-PRUNED weak-reference list, UNLIKE the hard LifetimeOwner /
        // ContextOwner refs: entity destruction does NOT remove the entity from its owner's dependents
        // (CkEntityLifetime_Utils.cpp:153-155 — a deliberate perf choice; every consumer filters via ck::IsValid,
        // e.g. Get_LifetimeDependents at :151-160). So a STALE (destroyed) dependent — e.g. the transient
        // spawn-request child every EntityScript creates and immediately destroys (CkEntityScript_Utils.cpp:244 +
        // CkEntityScript_Processor.cpp:54) — is BY DESIGN, not a dangling reference; a strict resolve would false-
        // positive on every EntityScript world, saved or not. The genuine dependents-side invariant is BACK-POINTER
        // CONSISTENCY: a still-LIVE dependent's FFragment_LifetimeOwner must name this owner (TransferLifetimeOwner
        // keeps the two symmetric — CkEntityLifetime_Utils.cpp:570,583). A mismatch is the cross-entity aliasing /
        // registry-rehome corruption this sweep exists to catch.
        auto
            DoCheck_Dependent(
                ck::SnapshotRegistryType& InRegistry,
                const FCk_Handle& InDependent,
                ck::SnapshotEntityType InOwnerEntity,
                TArray<FString>& OutDangling) -> void
        {
            const auto DependentEntity = InDependent.Get_Entity();
            if (DependentEntity.Get_IsTombstone())
            { return; }

            const auto DependentId = DependentEntity.Get_ID();
            if (DependentId == entt::null)
            { return; }

            // Stale weak reference (the dependent was destroyed) — lazily pruned by design, not a dangler.
            if (NOT InRegistry.valid(DependentId))
            { return; }

            // A live dependent without a LifetimeOwner is outside this leg's contract.
            const auto* BackOwner = InRegistry.try_get<ck::FFragment_LifetimeOwner>(DependentId);
            if (BackOwner == nullptr)
            { return; }

            const auto BackOwnerId = BackOwner->Get_Entity().Get_Entity().Get_ID();
            if (BackOwnerId == InOwnerEntity)
            { return; }

            OutDangling.Add(FString::Printf(
                TEXT("entity [%u] fragment [LifetimeDependents] -> live dependent [%u] whose LifetimeOwner names a ")
                TEXT("different entity [%u] (aliasing / registry-rehome corruption)"),
                static_cast<uint32>(entt::to_integral(InOwnerEntity)),
                static_cast<uint32>(entt::to_integral(DependentId)),
                static_cast<uint32>(entt::to_integral(BackOwnerId))));
        }
    }

    auto
        Verify_AllStoredHandlesResolve(
            ck::SnapshotRegistryType& InRegistry) -> TArray<FString>
    {
        auto Dangling = TArray<FString>{};

        for (const auto Entity : InRegistry.view<ck::FFragment_LifetimeOwner>())
        {
            const auto& Fragment = InRegistry.get<ck::FFragment_LifetimeOwner>(Entity);
            DoCheck_Handle(InRegistry, Fragment.Get_Entity(), TEXT("LifetimeOwner"), Entity, Dangling);
        }

        for (const auto Entity : InRegistry.view<ck::FFragment_ContextOwner>())
        {
            const auto& Fragment = InRegistry.get<ck::FFragment_ContextOwner>(Entity);
            DoCheck_Handle(InRegistry, Fragment.Get_Entity(), TEXT("ContextOwner"), Entity, Dangling);
        }

        for (const auto Entity : InRegistry.view<ck::FFragment_LifetimeDependents>())
        {
            const auto& Fragment = InRegistry.get<ck::FFragment_LifetimeDependents>(Entity);
            for (const auto& Dependent : Fragment.Get_Entities())
            { DoCheck_Dependent(InRegistry, Dependent, Entity, Dangling); }
        }

        return Dangling;
    }
}
