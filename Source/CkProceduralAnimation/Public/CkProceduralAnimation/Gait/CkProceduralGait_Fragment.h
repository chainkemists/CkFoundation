#pragma once

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"
#include "CkProceduralAnimation/Core/CkProceduralFootProbe.h"
#include "CkProceduralAnimation/Core/CkProceduralGaitSolver.h"
#include "CkProceduralAnimation/Debug/CkProceduralAnimation_Debug.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ProceduralGait_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ProceduralGait_HandleRequests;
    class FProcessor_ProceduralGait_Update;
    class FProcessor_ProceduralRig_Update;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait_Tunables
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralGait_Tunables);

    private:
        FCk_ProceduralGait_Timing _Timing;
        FCk_ProceduralGait_Step _Step;
        FCk_ProceduralGait_Probe _Probe;

    public:
        CK_PROPERTY_GET(_Timing);
        CK_PROPERTY_GET(_Step);
        CK_PROPERTY_GET(_Probe);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ProceduralGait_Tunables, _Timing, _Step, _Probe);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralGait);

    public:
        friend class FProcessor_ProceduralGait_HandleRequests;
        friend class FProcessor_ProceduralGait_Update;
        friend class FProcessor_ProceduralRig_Update;
        friend class ::UCk_Utils_ProceduralGait_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        FProceduralGaitSolver _Solver;
        TArray<FCk_Handle_ProceduralLeg> _Legs;
        uint64 _EnabledMask = ~uint64{0};
        TArray<FProceduralFootProbeState> _Probes;
        FProceduralGaitVelocityTracker _VelocityTracker;
        FQuat _Basis = FQuat::Identity;
        uint64 _SolveSequence = 0;
        bool _Initialized = false;
        bool _Failed = false;
        bool _Ready = false;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait_Debug
    {
    public:
        CK_GENERATED_BODY(FFragment_ProceduralGait_Debug);

    public:
        friend class FProcessor_ProceduralGait_Update;
        friend class ::UCk_Utils_ProceduralGait_UE;
        friend class ::UCk_Utils_ProceduralAnimation_Debug_UE;

    private:
        FCk_ProceduralAnimation_DebugSnapshot _Snapshot;
        TArray<FCk_ProceduralAnimation_DebugLeg> _ScratchLegs;
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
