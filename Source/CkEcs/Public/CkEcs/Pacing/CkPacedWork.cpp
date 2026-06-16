#include "CkEcs/Pacing/CkPacedWork.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_PacedWork::FFragment_PacedWork() = default;

    FFragment_PacedWork::FFragment_PacedWork(int32 InStepsPerPass, int32 InMaxStepsPerFrame)
        : _StepsPerPass(FMath::Max(1, InStepsPerPass))
        , _MaxStepsPerFrame(FMath::Max(1, InMaxStepsPerFrame))
        // Seed the budget so the first pump pass after Add can already do work this frame.
        , _StepsRemainingThisFrame(FMath::Max(1, InMaxStepsPerFrame))
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------
