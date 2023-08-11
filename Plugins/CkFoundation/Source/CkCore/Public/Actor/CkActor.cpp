#include "CkActor.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Actor_AttachmentRules::
    FCk_Actor_AttachmentRules(
        EAttachmentRule InLocationRule,
        EAttachmentRule InRotationRule,
        EAttachmentRule InScaleRule,
        bool InWeldSimulatedBodies)
    : _LocationRule      (InLocationRule)
    , _RotationRule      (InRotationRule)
    , _ScaleRule         (InScaleRule)
    , _WeldSimulatedBodies(InWeldSimulatedBodies)

{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Actor_AttachToActor_Params::
    FCk_Actor_AttachToActor_Params(
        AActor*                              InActorToAttachTo,
        FName                                InSocketName,
        FCk_Actor_AttachmentRules InAttachmentRules)
    : _ActorToAttachTo(InActorToAttachTo)
    , _SocketName     (InSocketName)
    , _AttachmentRules(InAttachmentRules)
{
}

// --------------------------------------------------------------------------------------------------------------------
