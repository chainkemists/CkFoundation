#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkFuzzyMatch_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Utils_FuzzyMatch_CaseSensitivity : uint8
{
    CaseInsensitive,
    CaseSensitive
};

UENUM(BlueprintType)
enum class ECk_Utils_FuzzyMatch_Strategy : uint8
{
    Subsequence,
    Substring
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_FuzzyMatch_Params
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Utils_FuzzyMatch_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Utils_FuzzyMatch_CaseSensitivity _CaseSensitivity = ECk_Utils_FuzzyMatch_CaseSensitivity::CaseInsensitive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Utils_FuzzyMatch_Strategy _Strategy = ECk_Utils_FuzzyMatch_Strategy::Subsequence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _MaxResults = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _MinScoreThreshold = 0;

public:
    CK_PROPERTY(_CaseSensitivity);
    CK_PROPERTY(_Strategy);
    CK_PROPERTY(_MaxResults);
    CK_PROPERTY(_MinScoreThreshold);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_FuzzyMatch_Result
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Utils_FuzzyMatch_Result);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _IsMatch = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Score = MIN_int32;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<int32> _MatchedIndices;

public:
    CK_PROPERTY(_IsMatch);
    CK_PROPERTY(_Score);
    CK_PROPERTY(_MatchedIndices);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_FuzzyMatch_RankedEntry
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Utils_FuzzyMatch_RankedEntry);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Candidate;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _OriginalIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Utils_FuzzyMatch_Result _Result;

public:
    CK_PROPERTY(_Candidate);
    CK_PROPERTY(_OriginalIndex);
    CK_PROPERTY(_Result);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::fuzzy
{
    CKCORE_API auto
    Match(
        FStringView InQuery,
        FStringView InCandidate,
        const FCk_Utils_FuzzyMatch_Params& InParams) -> FCk_Utils_FuzzyMatch_Result;

    CKCORE_API auto
    Score(
        FStringView InQuery,
        FStringView InCandidate,
        const FCk_Utils_FuzzyMatch_Params& InParams) -> TOptional<int32>;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_FuzzyMatch_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FuzzyMatch_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|FuzzyMatch",
              DisplayName = "[Ck][FuzzyMatch] Match")
    static FCk_Utils_FuzzyMatch_Result
    Match(
        const FString& InQuery,
        const FString& InCandidate,
        const FCk_Utils_FuzzyMatch_Params& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FuzzyMatch",
              DisplayName = "[Ck][FuzzyMatch] Rank Candidates")
    static TArray<FCk_Utils_FuzzyMatch_RankedEntry>
    Rank(
        const FString& InQuery,
        const TArray<FString>& InCandidates,
        const FCk_Utils_FuzzyMatch_Params& InParams);
};

// --------------------------------------------------------------------------------------------------------------------
