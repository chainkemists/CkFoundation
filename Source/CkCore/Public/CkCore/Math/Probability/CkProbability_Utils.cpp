#include "CkProbability_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <numeric>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probability_UE::
    Get_RandomIndexByWeight(
        const TArray<double>& InWeights,
        const FRandomStream& InRandomStream)
    -> FCk_Utils_Probability_RandomIndexByWeight_Result
{
    if (InWeights.IsEmpty())
    { return {}; }

    const auto& PartialSum = ck::algo::PartialSum(InWeights);

    const auto RandomNumber = InRandomStream.FRandRange(0, PartialSum.Last());
    const auto FoundIndex = ck::algo::FindIndex(PartialSum, [&](double InPartialNum)
    {
        return InPartialNum > RandomNumber;
    });

    CK_ENSURE_IF_NOT(FoundIndex != INDEX_NONE,
        TEXT("We should ALWAYS find an index. If an index is NOT found, the above logic for the algorithm is faulty."))
    { return {}; }

    return FCk_Utils_Probability_RandomIndexByWeight_Result{FoundIndex, RandomNumber};
}

// --------------------------------------------------------------------------------------------------------------------
