#pragma once

#include "CkCore/Concepts/CkConcepts.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Registry/CkRegistry.h"

#include <GameplayTagContainer.h>

#include "CkProcessorDescriptor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_ProcessorNetMode : uint8
{
    AllNetModes,
    Override
};

UENUM()
enum class ECk_TickGroupMode : uint8
{
    Inherit,
    Explicit
};

UENUM()
enum class ECk_UnresolvedRefPolicy : uint8
{
    Permissive,
    Strict
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FProcessorFactory = TFunction<concepts::FTickableType(const FCk_Registry&)>;
    using FDirtyChecker = TFunction<bool(const FCk_Registry&)>;

    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FProcessorDescriptor
    {
        CK_GENERATED_BODY(FProcessorDescriptor);

        FName _Name;

        FProcessorFactory _Factory;

        FName _GroupName;

        TArray<FName> _RunAfter;
        TArray<FName> _RunBefore;

        FGameplayTagContainer _RunAfterTags;
        FGameplayTagContainer _RunBeforeTags;

        bool _HasDirtyMarker = false;
        FDirtyChecker _IsDirtyChecker;
        uint32 _DirtyMarkerHash = 0;

        // Fragment access metadata inferred from TReadOnly<F>/TReadWrite<F> wrappers on the processor's
        // template parameter list. Used by FProcessorGraphBuilder to detect write-write conflicts between
        // processors that touch the same fragment without an explicit RunAfter/RunBefore ordering.
        // The name arrays are index-aligned with their matching hash arrays and exist purely for
        // human-readable diagnostics (debugger surfaces, log messages).
        TArray<uint32> _RO_FragmentHashes;
        TArray<uint32> _RW_FragmentHashes;
        TArray<FName>  _RO_FragmentNames;
        TArray<FName>  _RW_FragmentNames;

        ECk_ProcessorNetMode _NetModeRequirement = ECk_ProcessorNetMode::AllNetModes;
        ECk_ProcessorNetModeRequirement _NetModeRequirementValue = ECk_ProcessorNetModeRequirement::All;

        ECk_TickGroupMode _TickGroupMode = ECk_TickGroupMode::Inherit;
        ETickingGroup _TickGroupValue = TG_PrePhysics;

        FGameplayTag _SchedulerTag;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // One record per detected write-write conflict. Populated by DoValidateAndResolveWriteConflicts and forwarded
    // to the per-partition conflict list so the scheduler debugger can surface it in the Inspector panel.
    // A conflict is: two processors that both write the same fragment, in the same tick group, with no explicit
    // RunAfter/RunBefore ordering between them. _AutoInserted is true when the Permissive policy auto-inserted an
    // edge to resolve the conflict; false when Strict mode reported it and the Build() failed.
    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FCk_WriteConflictInfo
    {
        CK_GENERATED_BODY(FCk_WriteConflictInfo);

        FName _First;
        FName _Second;
        FName _FragmentName;
        bool  _AutoInserted = false;
    };
}

// --------------------------------------------------------------------------------------------------------------------
