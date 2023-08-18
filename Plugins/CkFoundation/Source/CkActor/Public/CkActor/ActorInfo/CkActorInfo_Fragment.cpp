#include "CkActorInfo_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Fragment_ActorInfo_Params::
        FCk_Fragment_ActorInfo_Params(
            ParamsType InParams)
        : _Params(MoveTemp(InParams))
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    FCk_Fragment_ActorInfo_Current::
        FCk_Fragment_ActorInfo_Current(
            AActor* InEntityActor)
        : _EntityActor(InEntityActor)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------


    FCk_Fragment_OwningActor::FCk_Fragment_OwningActor(AActor* InOwningActor, UCk_EcsBootstrapper_Base_UE* InBootstrapper)
        : _OwningActor(InOwningActor)
        , _Bootstrapper(InBootstrapper)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------

