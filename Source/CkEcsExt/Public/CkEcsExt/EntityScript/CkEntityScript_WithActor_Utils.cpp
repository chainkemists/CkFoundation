#include "CkEntityScript_WithActor_Utils.h"

#include "CkEntityScript_WithActor.h"
#include "CkEntityScript_WithActor_Data.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <GameFramework/Actor.h>
#include <StructUtils/InstancedStruct.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityScript_WithActor_UE::
    Request_SpawnEntityScript_OnActor(
        AActor* InActor,
        TSubclassOf<UCk_EntityScript_WithActor_UE> InEntityScriptClass)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Request_SpawnEntityScript_OnActor called with an invalid Actor"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScriptClass),
        TEXT("Request_SpawnEntityScript_OnActor called with an invalid EntityScript class for Actor [{}]"), InActor)
    { return {}; }

    if (NOT InActor->HasAuthority())
    { return {}; }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InActor->GetWorld());
    const auto SpawnParams = FInstancedStruct::Make<FCk_EntityScript_WithActor_SpawnParams>(InActor);

    // EntityScript::Request_SpawnEntity derives NetParams from the class default object before spawn params are
    // injected. A WithActor script cannot know the actor's runtime replication state at that point: its CDO has no
    // OwningActor. Seed the entity's NetParams here, where both the actor and world connection settings are known,
    // so a non-replicated actor never acquires an impossible EntityReplicationDriver during Construct.
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(TransientEntity);
    const auto& WorldSettings = TransientEntity.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings();
    const auto* ScriptDefault = InEntityScriptClass->GetDefaultObject<UCk_EntityScript_WithActor_UE>();
    const auto Replication = InActor->GetIsReplicated() && ck::IsValid(ScriptDefault)
        ? ScriptDefault->Get_EffectiveReplication()
        : ECk_Replication::DoesNotReplicate;

    UCk_Utils_Net_UE::Add(NewEntity, FCk_Net_ConnectionSettings{
        Replication, WorldSettings.Get_NetMode(), WorldSettings.Get_NetRole()});

    return UCk_Utils_EntityScript_UE::Add(NewEntity, InEntityScriptClass, SpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------
