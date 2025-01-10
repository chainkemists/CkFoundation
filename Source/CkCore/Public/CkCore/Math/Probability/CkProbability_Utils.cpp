#include "CkProbability_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <numeric>
#include <random>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probability_UE::
    Get_RandomIndexByWeight_Stream(
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

auto
    UCk_Utils_Probability_UE::
    Get_RandomIndexByWeight(
        const TArray<double>& InWeights)
    -> FCk_Utils_Probability_RandomIndexByWeight_Result
{
    if (InWeights.IsEmpty())
    { return {}; }

    const auto& PartialSum = ck::algo::PartialSum(InWeights);

    const auto RandomNumber = FMath::FRandRange(0, PartialSum.Last());
    const auto FoundIndex = ck::algo::FindIndex(PartialSum, [&](double InPartialNum)
    {
        return InPartialNum > RandomNumber;
    });

    CK_ENSURE_IF_NOT(FoundIndex != INDEX_NONE,
        TEXT("We should ALWAYS find an index. If an index is NOT found, the above logic for the algorithm is faulty."))
    { return {}; }

    return FCk_Utils_Probability_RandomIndexByWeight_Result{FoundIndex, RandomNumber};
}

auto
    UCk_Utils_Probability_UE::
    Get_Random_NormalDistribution(
        float InStandardDeviation)
        -> float
{
    thread_local std::random_device Rd;
    thread_local std::mt19937 Gen(Rd());

    auto Distribution = std::normal_distribution{0.0f, InStandardDeviation};

    return Distribution(Gen);
}

auto
    UCk_Utils_Probability_UE::
    Get_Random_UniformDistribution(
        float InMax)
        -> float
{
    thread_local std::random_device Rd;
    thread_local std::mt19937 Gen(Rd());

    auto Distribution = std::uniform_real_distribution{-InMax, InMax};

    return Distribution(Gen);
}

auto
    UCk_Utils_Probability_UE::
    Get_Random_WeightedDistribution(
        float InStandardDeviation,
        float InMaxRange,
        float InWeight)
        -> float
{
    const auto Normal = Get_Random_NormalDistribution(InStandardDeviation);
    const auto Uniform = Get_Random_UniformDistribution(InMaxRange);

    return (Normal * InWeight) + (Uniform * (1 - InWeight));

}

// --------------------------------------------------------------------------------------------------------------------
