#pragma once

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"
#include "CkProceduralAnimation/Core/CkProceduralGaitSolver.h"
#include "CkProceduralAnimation/Core/CkProceduralFootProbe.h"
#include "CkProceduralAnimation/Debug/CkProceduralAnimation_Debug.h"

class UCk_Utils_ProceduralGait_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

namespace ck
{
    using FFragment_ProceduralGait_Params = FCk_Fragment_ProceduralGait_ParamsData;

    struct CKPROCEDURALANIMATION_API FFragment_ProceduralGait_Current
    {
        CK_GENERATED_BODY(FFragment_ProceduralGait_Current);
        friend class FProcessor_ProceduralGait_Update;
        friend class FProcessor_ProceduralRig_Update;
        friend class UCk_Utils_ProceduralGait_UE;
        friend class UCk_Utils_ProceduralAnimation_Debug_UE;
    private:
        FProceduralGaitSolver _Solver;
        TArray<FProceduralFootProbeState> _Probes;
        TArray<FProceduralGaitLegInput> _Inputs;
        TArray<FProceduralGaitLegOutput> _Outputs;
        TArray<FCk_ProceduralGait_Foot> _Feet;
        FProceduralGaitVelocityTracker _VelocityTracker;
        FQuat _Basis = FQuat::Identity;
        TArray<FCk_ProceduralAnimation_DebugLeg> _DebugScratchLegs;
        FCk_ProceduralAnimation_DebugSnapshot _DebugSnapshot;
        bool _Initialized = false;
        bool _Failed = false;
        bool _Ready = false;
    };
}
