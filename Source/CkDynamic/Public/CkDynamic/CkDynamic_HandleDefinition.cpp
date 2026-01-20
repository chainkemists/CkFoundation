#include "CkDynamic/CkDynamic_HandleDefinition.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamic_HandleDefinition::
    GetRequiredFragmentNames() const
    -> TArray<FString>
{
    auto Result = TArray<FString>{};
    Result.Reserve(RequiredFragments.Num());

    for (const auto* Fragment : RequiredFragments)
    {
        if (ck::Is_NOT_Valid(Fragment))
        {
            continue;
        }

        Result.Add(Fragment->GetName());
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamic_HandleDefinition::
    IsValidDefinition() const
    -> bool
{
    if (TypeName.IsEmpty())
    {
        return false;
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkDynamic_HandleDefinition::
    GetDisplayName() const
    -> FString
{
    if (NOT TypeName.IsEmpty())
    {
        return TypeName;
    }

    return GetName();
}

// --------------------------------------------------------------------------------------------------------------------