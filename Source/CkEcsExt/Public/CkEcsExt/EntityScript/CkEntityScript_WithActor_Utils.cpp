#include "CkEntityScript_WithActor_Utils.h"

#include "CkEntityScript_WithActor.h"
#include "CkEntityScript_WithActor_Data.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
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

    return UCk_Utils_EntityScript_UE::Request_SpawnEntity(TransientEntity, InEntityScriptClass, SpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------
