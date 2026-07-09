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
        // FNV-1a 32-bit constants. We mix the case-normalized NAME STRING of each class into
        // a running 32-bit accumulator — the string is the only identity that is stable
        // across processes, which is what cross-machine determinism requires here.
        constexpr uint32 FnvOffsetBasis = 0x811c9dc5u;
        constexpr uint32 FnvPrime       = 0x01000193u;

        // Section salts. XORed into the accumulator between sections so that two empty
        // sections in different orders, or a class appearing in the "tasks" section vs the
        // "compose" section, hash distinctly.
        constexpr uint32 SaltTasks      = 0xAAAAAAAAu;
        constexpr uint32 SaltTrans      = 0xBBBBBBBBu;
        constexpr uint32 SaltCompose    = 0xCCCCCCCCu;

        // Marker XORed into the accumulator between the target and the condition list of one
        // transition, and at the end of each transition's condition list. Without these, the
        // condition classes of consecutive transitions would smear together and identical
        // condition-count permutations could collide.
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

            // Hash the name STRING, case-normalized — never the FName ComparisonIndex.
            // Comparison indices are process-local (assigned in name-table population order;
            // the accessor is literally ToUnstableInt), and an FName's stored case is
            // first-registration-wins, so neither survives a packaged-client vs
            // dedicated-server comparison. A false mismatch trips FTag_Sm_DeterminismFault
            // and permanently quiesces a healthy SM — and PIE (one shared name table) can
            // never reproduce it.
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

        // Tasks (declaration-order — semantic).
        H ^= SaltTasks;
        for (const auto& Task : InInputs._TaskClasses)
        { H = MixClass(H, Task); }

        // Transitions: target class + condition class list, both declaration-order.
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

        // Compose-from classes (declaration-order).
        H ^= SaltCompose;
        for (const auto& Composed : InInputs._ComposedFromClasses)
        { H = MixClass(H, Composed); }

        // uint32 → int32 by value reinterpretation. In C++20 the conversion is well-defined
        // two's-complement; the bit pattern survives and round-trips, which is what we need
        // for the int32 storage in FFragment_SmState_Fingerprint / FCk_Sm_TransitionEvent.
        return static_cast<int32>(H);
    }
}

// --------------------------------------------------------------------------------------------------------------------
