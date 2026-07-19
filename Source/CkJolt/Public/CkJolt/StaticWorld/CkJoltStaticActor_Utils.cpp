#include "CkJoltStaticActor_Utils.h"

#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment.h"

#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_JoltStaticActor_UE, FCk_Handle_JoltStaticActor, ck::FFragment_JoltStaticActor_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltStaticActor_UE::
    Get_SourceActor(
        const FCk_Handle_JoltStaticActor& InJoltStaticActor)
    -> AActor*
{
    return const_cast<AActor*>(InJoltStaticActor.Get<ck::FFragment_JoltStaticActor_Current>().Get_SourceActor().Get());
}

auto
    UCk_Utils_JoltStaticActor_UE::
    Get_SourceActorName(
        const FCk_Handle_JoltStaticActor& InJoltStaticActor)
    -> FName
{
    return InJoltStaticActor.Get<ck::FFragment_JoltStaticActor_Current>().Get_SourceActorName();
}

auto
    UCk_Utils_JoltStaticActor_UE::
    Get_NumBodies(
        const FCk_Handle_JoltStaticActor& InJoltStaticActor)
    -> int32
{
    return InJoltStaticActor.Get<ck::FFragment_JoltStaticActor_Current>().Get_BodyIds().Num();
}

// --------------------------------------------------------------------------------------------------------------------
