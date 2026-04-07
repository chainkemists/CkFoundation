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

        ECk_ProcessorNetMode _NetModeRequirement = ECk_ProcessorNetMode::AllNetModes;
        ECk_ProcessorNetModeRequirement _NetModeRequirementValue = ECk_ProcessorNetModeRequirement::All;

        ECk_TickGroupMode _TickGroupMode = ECk_TickGroupMode::Inherit;
        ETickingGroup _TickGroupValue = TG_PrePhysics;

        FGameplayTag _SchedulerTag;
    };
}

// --------------------------------------------------------------------------------------------------------------------
