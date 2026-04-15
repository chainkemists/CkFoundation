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
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto Flow = Super::Construct(InHandle, InSpawnParams);

    const auto IsmActor = Cast<ACk_IsmRenderer_Actor_UE>(Get_OwningActor());

    CK_ENSURE_IF_NOT(ck::IsValid(IsmActor),
        TEXT("EntityScript_IsmRenderer expects an ACk_IsmRenderer_Actor_UE but got [{}]"),
        Get_OwningActor()->GetClass()->GetName())
    { return Flow; }

    UCk_Utils_IsmRenderer_UE::Add(InHandle, IsmActor->Get_RenderData());

    return Flow;
}

// --------------------------------------------------------------------------------------------------------------------
