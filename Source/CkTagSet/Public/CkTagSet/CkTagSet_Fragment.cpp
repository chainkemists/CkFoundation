#include "CkTagSet_Fragment.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for TagSet

static struct FTagSetRepHandlerRegistrar
{
    FTagSetRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_TagSet::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    Entity.AddOrGet<ck::FFragment_TagSet_SyncReplication>(
                        New.Get<FCk_RepData_TagSet>().Tags);
                },
                .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    Entity.AddOrGet<ck::FFragment_TagSet_SyncReplication>(
                        Data.Get<FCk_RepData_TagSet>().Tags);
                }
            });
    }
} GTagSetRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
