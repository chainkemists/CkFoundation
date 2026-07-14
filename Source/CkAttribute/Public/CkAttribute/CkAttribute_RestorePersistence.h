#pragma once

#include "CkCore/Enums/CkEnums.h"          // ECk_MinMaxCurrent, ECk_AddedOrNot
#include "CkEcs/Handle/CkHandle.h"          // FCk_Handle, ck::Is_NOT_Valid
#include "CkEcs/Net/CkNet_Utils.h"          // TryAddContainerFragment, Get_LifetimeOwner
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped<T> body

#include <InstancedStruct.h>
#include <Misc/Optional.h>

// --------------------------------------------------------------------------------------------------------------------
// Shared Produce/SeedContainer for the attribute family (Float/Byte/Integer/Vector/Rotator), used by each kind's
// registrar. Replaces the deleted TProcessor_Attribute_ReplicateOnRestore_All. Empty-seed-and-defer, byte-for-byte
// the old restore behavior: Produce emits a SET-but-empty per-owner RepData so the generic re-drive seeds the
// owner's container; SeedContainer resolves the owner, adds the container, and re-arms the per-component
// FTag_MayRequireReplication triggers so FProcessor_Attribute_Replicate rebuilds the real payload. This sidesteps
// the per-owner upsert-merge that a live-payload Produce would need (multiple attributes share one owner container).
// --------------------------------------------------------------------------------------------------------------------

namespace ck::attribute_restore
{
    template <template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RepDataStruct>
    auto
        Produce(
            FCk_Handle& InEntity) -> TOptional<FInstancedStruct>
    {
        if (NOT InEntity.Has<T_DerivedAttribute<ECk_MinMaxCurrent::Current>>())
        { return {}; }

        return FInstancedStruct::Make(T_RepDataStruct{});
    }

    template <template <ECk_MinMaxCurrent> class T_DerivedAttribute, typename T_RepDataStruct>
    auto
        SeedContainer(
            FCk_Handle& InEntity,
            const FInstancedStruct& InData) -> ECk_AddedOrNot
    {
        using Current = T_DerivedAttribute<ECk_MinMaxCurrent::Current>;
        using Min     = T_DerivedAttribute<ECk_MinMaxCurrent::Min>;
        using Max     = T_DerivedAttribute<ECk_MinMaxCurrent::Max>;

        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InEntity);
        if (ck::Is_NOT_Valid(LifetimeOwner))
        { return ECk_AddedOrNot::NotAdded; }

        // The container is OWNER-hosted; the typed add self-gates on the owner's driver (NotAdded => the re-drive
        // retries next tick).
        const auto AddedOrNot = UCk_Utils_Net_UE::TryAddContainerFragment<T_RepDataStruct>(
            LifetimeOwner, InData.Get<T_RepDataStruct>());
        if (AddedOrNot == ECk_AddedOrNot::NotAdded)
        { return AddedOrNot; }

        // Re-arm the per-component replication triggers (Current always present; Min/Max only if composed).
        InEntity.AddOrGet<typename Current::FTag_MayRequireReplication>();
        if (InEntity.Has<Min>())
        { InEntity.AddOrGet<typename Min::FTag_MayRequireReplication>(); }
        if (InEntity.Has<Max>())
        { InEntity.AddOrGet<typename Max::FTag_MayRequireReplication>(); }

        return AddedOrNot;
    }
}
