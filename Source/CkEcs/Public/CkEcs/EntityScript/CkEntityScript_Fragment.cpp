#include "CkEntityScript_Fragment.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKECS_API, entity_script, ck::FFragment_EntityScript_Current);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_EntityScript_Current::
        FFragment_EntityScript_Current(
            UCk_EntityScript_UE* InScript)
        : _Script(InScript)
    {
    }

    FRequest_EntityScript_Replicate::
        FRequest_EntityScript_Replicate(
            const FCk_Handle& InOwner,
            const FInstancedStruct& InSpawnParams,
            UCk_EntityScript_UE* InScript)
        : _Owner(InOwner)
        , _SpawnParams(InSpawnParams)
        , _Script(InScript)
    {
    }

    auto
        FFragment_PendingReplication::
        Add(
            UClass* InEntityScriptClass,
            FCk_Handle InPendingEntity)
        -> void
    {
        _PendingByClass.FindOrAdd(InEntityScriptClass).Add(InPendingEntity);
    }

    auto
        FFragment_PendingReplication::
        ConsumeFirst(
            UClass* InEntityScriptClass)
        -> FCk_Handle
    {
        if (auto* Pending = _PendingByClass.Find(InEntityScriptClass);
            Pending != nullptr && Pending->Num() > 0)
        {
            auto Result = (*Pending)[0];
            Pending->RemoveAt(0);

            if (Pending->Num() == 0)
            {
                _PendingByClass.Remove(InEntityScriptClass);
            }

            return Result;
        }

        return {};
    }

    auto
        FFragment_PendingReplication::
        CleanupRemaining()
        -> void
    {
        for (auto& [Class, PendingEntities] : _PendingByClass)
        {
            for (auto& PendingEntity : PendingEntities)
            {
                if (ck::IsValid(PendingEntity))
                {
                    ck::ecs::Warning(
                        TEXT("Pending replication entity [{}] was never resolved — "
                             "the replicated entity never arrived on this client"),
                        PendingEntity);

                    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PendingEntity);
                }
            }
        }

        _PendingByClass.Empty();
    }
}

// --------------------------------------------------------------------------------------------------------------------