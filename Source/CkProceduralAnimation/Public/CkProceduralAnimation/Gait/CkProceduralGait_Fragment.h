#pragma once

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"
#include "CkProceduralAnimation/Core/CkProceduralFootProbe.h"
#include "CkProceduralAnimation/Core/CkProceduralFoothold.h"
#include "CkProceduralAnimation/Core/CkProceduralGaitSolver.h"
#include "CkProceduralAnimation/Core/CkProceduralSurfaceMotion.h"
#include "CkProceduralAnimation/Debug/CkProceduralAnimation_Debug.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Tag/CkTag.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ProceduralGait_UE;
class UCk_Utils_ProceduralBodyPose_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ProceduralGait_Setup;
    class FProcessor_ProceduralGait_HandleRequests;
    class FProcessor_ProceduralGait_Update;
    class FProcessor_SurfaceMotion_Update;
    class FProcessor_ProceduralBodyPose_Update;
    class FProcessor_ProceduralRig_Update;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_ProceduralGait_NeedsSetup);

    // A world-space plant and its hip in the body's local frame. SurfaceMotion consumes the previous gait solve before
    // the next gait update; it must never infer stance from a foothold target, which may differ from the planted foot.
    struct CKPROCEDURALANIMATION_API FProceduralGaitReachAnchor
    {
        CK_GENERATED_BODY(FProceduralGaitReachAnchor);

    private:
        FCk_Handle_ProceduralLeg _Leg;
        FVector _FootWorld = FVector::ZeroVector;
        FVector _HipLocal = FVector::ZeroVector;
        float _Reach = 0.0f;

    public:
        CK_PROPERTY_GET(_Leg);
        CK_PROPERTY_GET(_FootWorld);
        CK_PROPERTY_GET(_HipLocal);
        CK_PROPERTY_GET(_Reach);

    public:
        CK_DEFINE_CONSTRUCTORS(FProceduralGaitReachAnchor, _Leg, _FootWorld, _HipLocal, _Reach);
    };

    struct CKPROCEDURALANIMATION_API FProceduralGaitReachStance
    {
        CK_GENERATED_BODY(FProceduralGaitReachStance);

    public:
        friend class FProcessor_ProceduralGait_Update;

    private:
        FTransform _BodyAtSolve = FTransform::Identity;
        TArray<FProceduralGaitReachAnchor, TInlineAllocator<8>> _Anchors;
        uint64 _SolveSequence = 0;
        bool _HasSample = false;

    public:
        CK_PROPERTY_GET(_BodyAtSolve);
        CK_PROPERTY_GET(_Anchors);
        CK_PROPERTY_GET(_SolveSequence);
        CK_PROPERTY_GET(_HasSample);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Presence excludes the gait from Update; the reason is the diagnostic. Never removed: gait failure latches.
    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait_Failure
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralGait_Failure);

    private:
        ECk_ProceduralGait_Failure _Reason = ECk_ProceduralGait_Failure::None;

    public:
        CK_PROPERTY_GET(_Reason);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralGait_Failure, _Reason);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait_Tunables
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralGait_Tunables);

    private:
        FCk_ProceduralGait_Timing _Timing;
        FCk_ProceduralGait_Step _Step;
        FCk_ProceduralGait_Probe _Probe;
        FCk_ProceduralGait_Foothold _Foothold;

    public:
        CK_PROPERTY_GET(_Timing);
        CK_PROPERTY_GET(_Step);
        CK_PROPERTY_GET(_Probe);
        CK_PROPERTY_GET(_Foothold);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralGait_Tunables, _Timing, _Step, _Probe, _Foothold);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralGait);

    public:
        friend class FProcessor_ProceduralGait_Setup;
        friend class FProcessor_ProceduralGait_HandleRequests;
        friend class FProcessor_ProceduralGait_Update;
        friend class FProcessor_SurfaceMotion_Update;
        friend class FProcessor_ProceduralBodyPose_Update;
        friend class FProcessor_ProceduralRig_Update;
        friend class ::UCk_Utils_ProceduralGait_UE;
        friend class ::UCk_Utils_ProceduralBodyPose_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        FProceduralGaitSolver _Solver;
        TArray<FCk_Handle_ProceduralLeg> _Legs;
        uint64 _EnabledMask = ~uint64{0};
        TArray<FProceduralFootProbeState> _Probes;
        TArray<FProceduralFootholdState> _Footholds;
        int32 _NextSearchLeg = 0;
        FProceduralGaitVelocityTracker _VelocityTracker;
        FProceduralGaitYawRateTracker _YawRateTracker;
        FQuat _Basis = FQuat::Identity;
        uint64 _SolveSequence = 0;
        FProceduralGaitReachStance _ReachStance;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The plane through the ground the feet stand on and are about to land on, as of the last solve, with its frame and
    // footprint; SurfaceMotion reads it, nothing else writes it.
    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait_FeetPlane
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralGait_FeetPlane);

    public:
        friend class FProcessor_ProceduralGait_Setup;
        friend class FProcessor_ProceduralGait_Update;

    private:
        FProceduralSurfaceFeetSupport _Support;
        EProceduralGaitFeetPlane _State = EProceduralGaitFeetPlane::None;
        FCk_Time _SinceFit;

    public:
        CK_PROPERTY_GET(_Support);
        CK_PROPERTY_GET(_State);
        CK_PROPERTY_GET(_SinceFit);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait_Debug
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralGait_Debug);

    public:
        friend class FProcessor_ProceduralGait_Setup;
        friend class FProcessor_ProceduralGait_HandleRequests;
        friend class FProcessor_ProceduralGait_Update;
        friend class FProcessor_ProceduralRig_Update;
        friend class ::UCk_Utils_ProceduralGait_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        FCk_ProceduralAnimation_DebugSnapshot _Snapshot;
        TArray<FCk_ProceduralAnimation_DebugLeg> _ScratchLegs;
        float _ReachCadenceFloor = 0.0f;
        int32 _ReachSkippedLegs = 0;
        int32 _RaysLastSolve = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralGait_Requests);

    public:
        friend class FProcessor_ProceduralGait_HandleRequests;
        friend class ::UCk_Utils_ProceduralGait_UE;

    public:
        using RequestType = std::variant<FCk_Request_ProceduralGait_ApplyPreset>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPROCEDURALANIMATION_API, OnProceduralGait_LegSetChanged, FCk_Delegate_ProceduralGait_OnLegSetChanged, FCk_Handle_ProceduralGait, int32, int32);
}

// --------------------------------------------------------------------------------------------------------------------
