#pragma once

#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ProceduralLeg_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ProceduralLeg_HandleRequests;
    class FProcessor_ProceduralGait_Update;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_ProceduralLeg_Disabled);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_ProceduralLeg_Params = FCk_ProceduralLeg_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralLeg
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralLeg);

    public:
        friend class FProcessor_ProceduralGait_Update;
        friend class ::UCk_Utils_ProceduralLeg_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        FCk_ProceduralLeg_Foot _Foot;
        // Body-local pose captured on the enabled-to-disabled transition; the gait re-expresses it in world space
        // each frame so the frozen foot rides with the body.
        FVector _FrozenFootLocal = FVector::ZeroVector;
        FQuat _FrozenRotationLocal = FQuat::Identity;
        bool _Frozen = false;

    public:
        CK_PROPERTY_GET(_Foot);
        CK_PROPERTY_GET(_Frozen);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralLeg_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralLeg_Requests);

    public:
        friend class FProcessor_ProceduralLeg_HandleRequests;
        friend class ::UCk_Utils_ProceduralLeg_UE;

    public:
        using RequestType = std::variant<FCk_Request_ProceduralLeg_EnableDisable, FCk_Request_ProceduralLeg_Detach>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_AND_UTILS_TRANSIENT(FUtils_RecordOfProceduralLegs, FFragment_RecordOfProceduralLegs, FCk_Handle_ProceduralLeg);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPROCEDURALANIMATION_API, OnProceduralLeg_Detached, FCk_Delegate_ProceduralLeg_OnDetached, FCk_Handle_ProceduralLeg, FCk_ProceduralLeg_ReleasedParts);
}

// --------------------------------------------------------------------------------------------------------------------
