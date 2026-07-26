#include "CkSmState_Fingerprint.h"

#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::statemachine
{
    namespace fingerprint_detail
    {
        constexpr uint32 FnvOffsetBasis = 0x811c9dc5u;
        constexpr uint32 FnvPrime       = 0x01000193u;

        // XORed between sections so two empty sections in a different order, or the same class in
        // the tasks vs the compose section, hash distinctly.
        constexpr uint32 SaltTasks      = 0xAAAAAAAAu;
        constexpr uint32 SaltTrans      = 0xBBBBBBBBu;
        constexpr uint32 SaltCompose    = 0xCCCCCCCCu;

        // XORed between a transition's target and its condition list, and at that list's end —
        // without them consecutive transitions' condition classes smear together.
        constexpr uint32 SaltTransSep   = 0xDDDDDDDDu;
        constexpr uint32 SaltCondsEnd   = 0xEEEEEEEEu;

        auto
        MixClass(
            uint32 InAccumulator,
            const TSubclassOf<UObject>& InClass) -> uint32
        {
            const auto Name = ck::IsValid(InClass)
                ? InClass->GetFName()
                : FName{NAME_None};

            // Hash the name STRING, case-normalized — never the FName ComparisonIndex. Indices are
            // process-local and an FName's stored case is first-registration-wins, so neither
            // survives a packaged-client vs dedicated-server comparison.
            const auto NameString = Name.ToString().ToLower();

            auto H = InAccumulator;
            for (const auto& Char : NameString)
            {
                H ^= static_cast<uint32>(Char);
                H *= FnvPrime;
            }

            // Per-name terminator so consecutive names can't smear ("AB"+"C" vs "A"+"BC").
            H ^= 0xFFu;
            H *= FnvPrime;
            return H;
        }
    }

    auto
    ComputeFingerprint(
        const FFingerprintInputs& InInputs) -> int32
    {
        using namespace fingerprint_detail;

        auto H = FnvOffsetBasis;

        H ^= SaltTasks;
        for (const auto& Task : InInputs._TaskClasses)
        { H = MixClass(H, Task); }

        H ^= SaltTrans;
        const auto NumTransitions = InInputs._TransitionTargetClasses.Num();
        for (auto i = 0; i < NumTransitions; ++i)
        {
            H = MixClass(H, InInputs._TransitionTargetClasses[i]);
            H ^= SaltTransSep;

            if (InInputs._TransitionConditionLists.IsValidIndex(i))
            {
                for (const auto& Condition : InInputs._TransitionConditionLists[i])
                { H = MixClass(H, Condition); }
            }

            H ^= SaltCondsEnd;
        }

        H ^= SaltCompose;
        for (const auto& Composed : InInputs._ComposedFromClasses)
        { H = MixClass(H, Composed); }

        // uint32 to int32 is well-defined two's-complement in C++20 and the bit pattern round-trips,
        // which is what the int32 storage in the fragment / transition event needs.
        return static_cast<int32>(H);
    }
}

// --------------------------------------------------------------------------------------------------------------------
