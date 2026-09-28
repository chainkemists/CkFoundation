#pragma once

#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment_Data.h"

#include "CkProceduralAnimation/Core/CkProceduralSurfaceMotion.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_SurfaceMotion_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_SurfaceMotion_Setup;
    class FProcessor_SurfaceMotion_HandleRequests;
    class FProcessor_SurfaceMotion_Update;
    class FProcessor_ProceduralGait_Update;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_SurfaceMotion_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_SurfaceMotion_Params = FCk_SurfaceMotion_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    // Steering intent. The membership anchor: Has/Cast key on it.
    struct CKPROCEDURALANIMATION_API FFragment_SurfaceMotion
    {
    public:
        CK_GENERATED_BODY(FFragment_SurfaceMotion);

    public:
        friend class FProcessor_SurfaceMotion_Setup;
        friend class FProcessor_SurfaceMotion_HandleRequests;
        friend class FProcessor_SurfaceMotion_Update;
        friend class FProcessor_ProceduralGait_Update;
        friend class ::UCk_Utils_SurfaceMotion_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        FVector _Direction = FVector::ForwardVector;
        float _Speed = 0.0f;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // A live trusted plant and its hips in the final integration substep's rejected full-motion trial. The support's
    // accepted-body and frame stamps bound consumption; the gait also checks ownership and the unchanged plant.
    struct FProceduralSurfaceReachPaceFeedback
    {
        CK_GENERATED_BODY(FProceduralSurfaceReachPaceFeedback);

    private:
        FCk_Handle_ProceduralLeg _Leg;
        FVector _FootWorld = FVector::ZeroVector;
        FVector _TrialHipWorld = FVector::ZeroVector;
        TOptional<FVector> _TrialPosedHipWorld;

    public:
        CK_PROPERTY_GET(_Leg);
        CK_PROPERTY_GET(_FootWorld);
        CK_PROPERTY_GET(_TrialHipWorld);
        CK_PROPERTY_GET(_TrialPosedHipWorld);

    public:
        CK_DEFINE_CONSTRUCTORS(FProceduralSurfaceReachPaceFeedback, _Leg, _FootWorld, _TrialHipWorld, _TrialPosedHipWorld);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The accepted support frame, the pending contact that would replace it and the body's integrated motion, rewritten
    // every substep.
    struct CKPROCEDURALANIMATION_API FFragment_SurfaceMotion_Support
    {
    public:
        CK_GENERATED_BODY(FFragment_SurfaceMotion_Support);

    public:
        friend class FProcessor_SurfaceMotion_Setup;
        friend class FProcessor_SurfaceMotion_Update;
        friend class FProcessor_ProceduralGait_Update;
        friend class ::UCk_Utils_SurfaceMotion_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        FProceduralSurfaceMotionState _State;
        uint64 _EvaluatedFrame = 0;
        FTransform _EvaluatedBody = FTransform::Identity;
        float _ReachPaceScale = 1.0f;
        ECk_SurfaceMotion_ReachPaceState _ReachPaceState = ECk_SurfaceMotion_ReachPaceState::Free;
        int32 _ReachPaceTrials = 0;
        int32 _ReachPaceRays = 0;
        float _AttemptedStanceSpeed = 0.0f;
        TArray<FProceduralSurfaceReachPaceFeedback, TInlineAllocator<8>> _ReachPaceFeedback;

    public:
        CK_PROPERTY_GET(_ReachPaceScale);
        CK_PROPERTY_GET(_ReachPaceState);
        CK_PROPERTY_GET(_ReachPaceTrials);
        CK_PROPERTY_GET(_ReachPaceRays);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_SurfaceMotion_Failure
    {
    public:
        CK_GENERATED_BODY(FFragment_SurfaceMotion_Failure);

    private:
        ECk_SurfaceMotion_Failure _Reason = ECk_SurfaceMotion_Failure::None;

    public:
        CK_PROPERTY_GET(_Reason);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_SurfaceMotion_Failure, _Reason);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_SurfaceMotion_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_SurfaceMotion_Requests);

    public:
        friend class FProcessor_SurfaceMotion_HandleRequests;
        friend class ::UCk_Utils_SurfaceMotion_UE;

    private:
        TArray<FCk_Request_SurfaceMotion_Steering> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
