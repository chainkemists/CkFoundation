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
    class FProcessor_ProceduralGait_Setup;
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
        friend class FProcessor_ProceduralGait_Setup;
        friend class FProcessor_ProceduralGait_Update;
        friend class ::UCk_Utils_ProceduralLeg_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        FCk_ProceduralLeg_Foot _Foot;
        ECk_ProceduralLeg_FootholdVerdict _IdealVerdict = ECk_ProceduralLeg_FootholdVerdict::Miss;

    public:
        CK_PROPERTY_GET(_Foot);
        CK_PROPERTY_GET(_IdealVerdict);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Present only while the leg is disabled: the body-local foot pose captured on the transition, which the gait
    // re-expresses in world space every frame so the frozen foot rides with the body.
    struct CKPROCEDURALANIMATION_API FFragment_ProceduralLeg_FrozenPose
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralLeg_FrozenPose);

    private:
        FVector _FootLocal = FVector::ZeroVector;
        FQuat _RotationLocal = FQuat::Identity;

    public:
        CK_PROPERTY(_FootLocal);
        CK_PROPERTY(_RotationLocal);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralLeg_FrozenPose, _FootLocal, _RotationLocal);
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
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPROCEDURALANIMATION_API, OnProceduralLeg_Planted, FCk_Delegate_ProceduralLeg_OnPlanted, FCk_Handle_ProceduralLeg, FCk_ProceduralLeg_Footfall);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPROCEDURALANIMATION_API, OnProceduralLeg_Lifted, FCk_Delegate_ProceduralLeg_OnLifted, FCk_Handle_ProceduralLeg, FCk_ProceduralLeg_Footfall);
}

// --------------------------------------------------------------------------------------------------------------------
