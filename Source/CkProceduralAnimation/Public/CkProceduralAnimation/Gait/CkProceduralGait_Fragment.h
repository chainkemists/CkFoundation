#pragma once

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"
#include "CkProceduralAnimation/Core/CkProceduralFootProbe.h"
#include "CkProceduralAnimation/Core/CkProceduralFoothold.h"
#include "CkProceduralAnimation/Core/CkProceduralGaitSolver.h"
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
    class FProcessor_ProceduralBodyPose_Update;
    class FProcessor_ProceduralRig_Update;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_ProceduralGait_NeedsSetup);

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
