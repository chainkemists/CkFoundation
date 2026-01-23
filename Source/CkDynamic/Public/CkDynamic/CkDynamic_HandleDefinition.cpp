#include "CkDynamic_HandleDefinition.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto ExtractShortNameFromTypeName(const FString& InTypeName) -> FString
    {
        static const TArray<FString> KnownPrefixes =
        {
            TEXT("FCk_Handle_"),
            TEXT("Handle_"),
        };

        for (const auto& Prefix : KnownPrefixes)
        {
            if (InTypeName.StartsWith(Prefix))
            {
                return InTypeName.RightChop(Prefix.Len());
            }
        }

        return InTypeName;
    }
}

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
    GetShortName() const
    -> FString
{
    if (NOT ShortName.IsEmpty())
    {
        return ShortName;
    }

    return ExtractShortNameFromTypeName(TypeName);
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