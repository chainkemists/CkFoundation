#include "CkEntityScript_Fragment.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
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
            FCk_Handle InPendingEntity,
            FInstancedStruct InSpawnParams)
        -> void
    {
        _PendingByClass.FindOrAdd(InEntityScriptClass).Add(
            FPendingEntry{MoveTemp(InPendingEntity), MoveTemp(InSpawnParams)});
    }

    auto
        FFragment_PendingReplication::
        ConsumeFirst(
            UClass* InEntityScriptClass,
            const UCk_EntityScript_UE* InCDO,
            const FCk_Handle& InConstructedEntity)
        -> FCk_Handle
    {
        auto* Pending = _PendingByClass.Find(InEntityScriptClass);
        if (Pending == nullptr || Pending->Num() == 0)
        { return {}; }

        const auto* ConstructedScript = static_cast<const UCk_EntityScript_UE*>(nullptr);
        if (InConstructedEntity.Has<FFragment_EntityScript_Current>())
        {
            ConstructedScript = InConstructedEntity.Get<FFragment_EntityScript_Current>().Get_Script().Get();
        }

        for (auto Index = 0; Index < Pending->Num(); ++Index)
        {
            auto& Entry = (*Pending)[Index];

            if (ck::IsValid(InCDO, ck::IsValid_Policy_NullptrOnly{})
                && NOT InCDO->Get_AreSpawnParamsMatching(Entry._SpawnParams, ConstructedScript))
            { continue; }

            auto Result = Entry._Entity;
            Pending->RemoveAt(Index);

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
        for (auto& [Class, PendingEntries] : _PendingByClass)
        {
            for (auto& Entry : PendingEntries)
            {
                if (ck::IsValid(Entry._Entity))
                {
                    ck::ecs::Warning(
                        TEXT("Pending replication entity [{}] was never resolved — "
                             "the replicated entity never arrived on this client"),
                        Entry._Entity);

                    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Entry._Entity);
                }
            }
        }

        _PendingByClass.Empty();
    }
}

// --------------------------------------------------------------------------------------------------------------------