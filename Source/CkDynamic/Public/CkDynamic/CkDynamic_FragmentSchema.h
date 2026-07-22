#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UScriptStruct;

namespace ck::dynamic
{
    struct CKDYNAMIC_API FFragmentSchemaValidation
    {
        bool IsSafe = false;
        FString FailurePath;
        FString FailureReason;
    };

    // Dynamic fragments live in named EnTT storage, outside Unreal's reflected owner graph. Strong UObject references
    // in that memory are therefore neither traced nor rewritten by GC and can become stale raw addresses. Validate the
    // complete reflected schema before any storage is created; only approved weak/soft references remain
    // generation/path based. Deprecated lazy and opaque native carriers fail closed.
    CKDYNAMIC_API auto
    Validate_FragmentSchema(
        const UScriptStruct* InStructType) -> FFragmentSchemaValidation;
}

// --------------------------------------------------------------------------------------------------------------------
