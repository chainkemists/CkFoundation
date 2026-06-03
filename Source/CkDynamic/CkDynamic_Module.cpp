#include "CkDynamic_Module.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_Fragment.h"
#include "CkDynamic/CkDynamic_ScriptProcessor_Host.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#define LOCTEXT_NAMESPACE "FCkDynamicModule"

void FCkDynamicModule::StartupModule()
{
    ck::FScriptProcessor_Host::Startup();

    // Generic catch-all for replicated dynamic fragments: their payload UScriptStruct is only known at
    // runtime, so no per-type handler can be registered. On a client, when a replicated dynamic-fragment
    // payload arrives/changes, stash it onto the entity for FProcessor_DynamicFragment_SyncReplication to
    // apply + signal on the game thread (never write storage / broadcast inline on the OnRep path).
    FCk_ReplicatedFragmentHandlerRegistry::RegisterFallback(
    {
        .OnChange = [](FCk_Handle& InEntity, const FInstancedStruct& InNew, const FInstancedStruct& /*InOld*/)
        {
            if (ck::Is_NOT_Valid(InNew.GetScriptStruct()))
            { return; }

            InEntity.AddOrGet<ck::FFragment_DynamicFragment_SyncReplication>().Get_PendingPayloads().Add(InNew);
        },
        .OnAdd = [](FCk_Handle& InEntity, const FInstancedStruct& InData)
        {
            if (ck::Is_NOT_Valid(InData.GetScriptStruct()))
            { return; }

            InEntity.AddOrGet<ck::FFragment_DynamicFragment_SyncReplication>().Get_PendingPayloads().Add(InData);
        }
    });
}

void FCkDynamicModule::ShutdownModule()
{
    ck::FScriptProcessor_Host::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkDynamicModule, CkDynamic)
