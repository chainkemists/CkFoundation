#include "CkCompositeAlgos_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CompositeAlgos_UE::
    FilterActors_ByPredicate(
        const TArray<AActor*>& InActors,
        FCk_Predicate_InActor_OutResult InPredicate)
    -> TArray<AActor*>
{
    return ck::algo::Filter(InActors, [&](AActor* InActor)
    {
        if (ck::Is_NOT_Valid(InActor))
        { return false; }

        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InActor, PredicateResult);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    FilterActors_ByIsValid(
        const TArray<AActor*>& InActors)
    -> TArray<AActor*>
{
    return ck::algo::Filter(InActors, [&](AActor* InActor)
    {
        return ck::IsValid(InActor);
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    SortActors_ByPredicate(
        TArray<AActor*>& InActors,
        FCk_Predicate_In2Actors_OutResult InPredicate)
    -> void
{
    ck::algo::Sort(InActors, [&](AActor* InA, AActor* InB) -> bool
    {
        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InA, InB, PredicateResult);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    SortActors_ByDistance(
        TArray<AActor*>& InActors,
        const FVector& InOrigin,
        ECk_DistanceSortingPolicy InSortingPolicy)
    -> void
{
    ck::algo::Sort(InActors, [&](AActor* InA, AActor* InB) -> bool
    {
        if (ck::Is_NOT_Valid(InA) || ck::Is_NOT_Valid(InB))
        { return false; }

        const auto& DistanceA = FVector::DistSquared(InOrigin, InA->GetActorLocation());
        const auto& DistanceB = FVector::DistSquared(InOrigin, InB->GetActorLocation());

        switch (InSortingPolicy)
        {
            case ECk_DistanceSortingPolicy::ClosestToFarthest:
            {
                return DistanceA < DistanceB;
            }
            case ECk_DistanceSortingPolicy::FarthestToClosest:
            {
                return DistanceA > DistanceB;
            }
            default:
            {
                CK_INVALID_ENUM(InSortingPolicy);
                return false;
            }
        }
    });
}

// --------------------------------------------------------------------------------------------------------------------
