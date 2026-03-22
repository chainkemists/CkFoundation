#include "CkTagSet_ProcessorInjector.h"

#include "CkTagSet/CkTagSet_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_TagSet_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    // 1. Client-side: reconcile replicated tag set state
    InWorld.Add<ck::FProcessor_TagSet_SyncReplication>(InWorld.Get_Registry());

    // 2. Authority: process add/remove tag requests
    InWorld.Add<ck::FProcessor_TagSet_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_TagSet_ProcessorInjector_Replicate_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    // 3. Server-side: push tag set to replicated container
    InWorld.Add<ck::FProcessor_TagSet_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
