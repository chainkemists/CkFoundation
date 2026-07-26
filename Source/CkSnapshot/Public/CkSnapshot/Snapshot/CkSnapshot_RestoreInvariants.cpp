#include "CkSnapshot/Snapshot/CkSnapshot_RestoreInvariants.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h" // ck::FFragment_LifetimeOwner / FFragment_LifetimeDependents
#include "CkEcs/Handle/CkHandle.h"                          // ck::FFragment_ContextOwner, FCk_Handle

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    namespace ck_snapshot_restore_invariants
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

        // Asserts BACK-POINTER CONSISTENCY, not resolvability: FFragment_LifetimeDependents is a lazily-pruned
        // WEAK-ref list, so a stale entry is by design (rationale in CkSnapshot/CLAUDE.md).
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
            ck_snapshot_restore_invariants::DoCheck_Handle(InRegistry, Fragment.Get_Entity(), TEXT("LifetimeOwner"), Entity, Dangling);
        }

        for (const auto Entity : InRegistry.view<ck::FFragment_ContextOwner>())
        {
            const auto& Fragment = InRegistry.get<ck::FFragment_ContextOwner>(Entity);
            ck_snapshot_restore_invariants::DoCheck_Handle(InRegistry, Fragment.Get_Entity(), TEXT("ContextOwner"), Entity, Dangling);
        }

        for (const auto Entity : InRegistry.view<ck::FFragment_LifetimeDependents>())
        {
            const auto& Fragment = InRegistry.get<ck::FFragment_LifetimeDependents>(Entity);
            for (const auto& Dependent : Fragment.Get_Entities())
            { ck_snapshot_restore_invariants::DoCheck_Dependent(InRegistry, Dependent, Entity, Dangling); }
        }

        return Dangling;
    }
}
