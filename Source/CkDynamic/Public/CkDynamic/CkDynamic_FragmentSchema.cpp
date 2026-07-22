#include "CkDynamic/CkDynamic_FragmentSchema.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Validation/CkUntracedStructSafety.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_fragment_schema
{
    using FValidation = ck::dynamic::FFragmentSchemaValidation;

    auto Safe() -> FValidation
    {
        return {.IsSafe = true};
    }

    auto Reject(FString InPath, FString InReason) -> FValidation
    {
        return {
            .IsSafe = false,
            .FailurePath = MoveTemp(InPath),
            .FailureReason = MoveTemp(InReason)};
    }

}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Validate_FragmentSchema(
        const UScriptStruct* InStructType)
    -> FFragmentSchemaValidation
{
    if (ck::Is_NOT_Valid(InStructType))
    { return ck_dynamic_fragment_schema::Reject(TEXT("<invalid>"), TEXT("fragment struct type is invalid")); }

    // Every dynamic-storage mutation is game-thread-only. A weak key prevents AS hot reload from serving a cached
    // result for a replacement UScriptStruct allocated at the same address.
    if (NOT IsInGameThread())
    {
        return ck_dynamic_fragment_schema::Reject(
            InStructType->GetName(), TEXT("dynamic-fragment schemas must be validated on the game thread"));
    }

    static TMap<TWeakObjectPtr<const UScriptStruct>, FFragmentSchemaValidation> Cache;
    if (const auto* Found = Cache.Find(InStructType))
    { return *Found; }

    const auto Analysis = ck::Analyze_UntracedStructSafety(InStructType);
    auto Result = Analysis.IsGcIndependent()
        ? ck_dynamic_fragment_schema::Safe()
        : ck_dynamic_fragment_schema::Reject(Analysis.FailurePath, Analysis.FailureReason);

    Cache.Add(InStructType, Result);
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
