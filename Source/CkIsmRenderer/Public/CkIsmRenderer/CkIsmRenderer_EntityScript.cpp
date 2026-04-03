#include "CkIsmRenderer_EntityScript.h"

#include "CkIsmRenderer/CkIsmSubsystem.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_EntityScript_IsmRenderer_UE::
    UCk_EntityScript_IsmRenderer_UE()
{
    _Replication = ECk_Replication::DoesNotReplicate;
}

auto
    UCk_EntityScript_IsmRenderer_UE::
    ConstructWithActor(
        FCk_Handle& InHandle,
        AActor* InOwningActor)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto IsmActor = Cast<ACk_IsmRenderer_Actor_UE>(InOwningActor);

    CK_ENSURE_IF_NOT(ck::IsValid(IsmActor),
        TEXT("EntityScript_IsmRenderer expects an ACk_IsmRenderer_Actor_UE but got [{}]"),
        InOwningActor->GetClass()->GetName())
    { return ECk_EntityScript_ConstructionFlow::Finished; }

    UCk_Utils_IsmRenderer_UE::Add(InHandle, IsmActor->Get_RenderData());

    return ECk_EntityScript_ConstructionFlow::Finished;
}

// --------------------------------------------------------------------------------------------------------------------
