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
            { DoCheck_Handle(InRegistry, Dependent, TEXT("LifetimeDependents"), Entity, Dangling); }
        }

        return Dangling;
    }
}
