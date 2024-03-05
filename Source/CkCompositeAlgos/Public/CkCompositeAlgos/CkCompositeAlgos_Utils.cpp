#include "CkCompositeAlgos_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcsBasics/Public/CkEcsBasics/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CompositeAlgos_UE::
    AnyActors_If(
        const TArray<AActor*>& InActors,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InActor_OutResult InPredicate)
    -> bool
{
    return ck::algo::AnyOf(InActors, [&](AActor* InActor)
    {
        if (ck::Is_NOT_Valid(InActor))
        { return false; }

        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InActor, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    AllActors_If(
        const TArray<AActor*>& InActors,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InActor_OutResult InPredicate)
    -> bool
{
    return ck::algo::AllOf(InActors, [&](AActor* InActor)
    {
        if (ck::Is_NOT_Valid(InActor))
        { return false; }

        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InActor, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    NoneActors_If(
        const TArray<AActor*>& InActors,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InActor_OutResult InPredicate)
    -> bool
{
    return ck::algo::NoneOf(InActors, [&](AActor* InActor)
    {
        if (ck::Is_NOT_Valid(InActor))
        { return false; }

        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InActor, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    AnyEntities_If(
        const TArray<FCk_Handle>& InEntities,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> bool
{
    return ck::algo::AnyOf(InEntities, [&](const FCk_Handle& InEntity)
    {
        if (ck::Is_NOT_Valid(InEntity))
        { return false; }

        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InEntity, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    AllEntities_If(
        const TArray<FCk_Handle>& InEntities,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> bool
{
    return ck::algo::AllOf(InEntities, [&](const FCk_Handle& InEntity)
    {
        if (ck::Is_NOT_Valid(InEntity))
        { return false; }

        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InEntity, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    NoneEntities_If(
        const TArray<FCk_Handle>& InEntities,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> bool
{
    return ck::algo::NoneOf(InEntities, [&](const FCk_Handle& InEntity)
    {
        if (ck::Is_NOT_Valid(InEntity))
        { return false; }

        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InEntity, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    IntersectEntities(
        const TArray<FCk_Handle>& InEntitiesA,
        const TArray<FCk_Handle>& InEntitiesB)
    -> TArray<FCk_Handle>
{
   return ck::algo::Intersect(InEntitiesA, InEntitiesB);
}

auto
    UCk_Utils_CompositeAlgos_UE::
    ExceptEntities(
        const TArray<FCk_Handle>& InEntitiesA,
        const TArray<FCk_Handle>& InEntitiesB)
    -> TArray<FCk_Handle>
{
    return ck::algo::Except(InEntitiesA, InEntitiesB);
}

auto
    UCk_Utils_CompositeAlgos_UE::
    FilterActors_ByPredicate(
        const TArray<AActor*>& InActors,
        const FInstancedStruct& InOptionalPayload,
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
            InPredicate.Execute(InActor, PredicateResult, InOptionalPayload);
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
    FilterEntities_ByPredicate(
        const TArray<FCk_Handle>& InEntities,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate)
    -> TArray<FCk_Handle>
{
    return ck::algo::Filter(InEntities, [&](const FCk_Handle& InEntity)
    {
        if (ck::Is_NOT_Valid(InEntity))
        { return false; }

        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InEntity, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    FilterEntities_ByIsValid(
        const TArray<FCk_Handle>& InEntities)
    -> TArray<FCk_Handle>
{
    return ck::algo::Filter(InEntities, [&](const FCk_Handle& InEntity)
    {
        return ck::IsValid(InEntity);
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    SortActors_ByPredicate(
        TArray<AActor*>& InActors,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_In2Actors_OutResult InPredicate)
    -> void
{
    ck::algo::Sort(InActors, [&](AActor* InA, AActor* InB) -> bool
    {
        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InA, InB, PredicateResult, InOptionalPayload);
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

auto
    UCk_Utils_CompositeAlgos_UE::
    SortEntities_ByPredicate(
        TArray<FCk_Handle>& InEntities,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_In2Handles_OutResult InPredicate)
    -> void
{
    ck::algo::Sort(InEntities, [&](const FCk_Handle& InA, const FCk_Handle& InB) -> bool
    {
        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InA, InB, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    SortEntities_ByDistance(
        TArray<FCk_Handle>& InEntities,
        const FVector& InOrigin,
        ECk_DistanceSortingPolicy InSortingPolicy,
        ECk_EntityFragmentRequirementPolicy InFragmentRequirementPolicy)
    -> void
{
    ck::algo::Sort(InEntities, [&](const FCk_Handle& InA, const FCk_Handle& InB) -> bool
    {
        if (ck::Is_NOT_Valid(InA) || ck::Is_NOT_Valid(InB))
        { return false; }

        const auto& HasTransformFragmentA = UCk_Utils_Transform_UE::Has(InA);
        const auto& HasTransformFragmentB = UCk_Utils_Transform_UE::Has(InB);

        switch (InFragmentRequirementPolicy)
        {
            case ECk_EntityFragmentRequirementPolicy::EnsureAllFragments:
            {
                CK_ENSURE_IF_NOT(HasTransformFragmentA, TEXT("Entity [{}] is MISSING the required Transform fragment when trying to sort"), InA)
                { return false; }

                CK_ENSURE_IF_NOT(HasTransformFragmentB, TEXT("Entity [{}] is MISSING the required Transform fragment when trying to sort"), InB)
                { return true; }

                break;
            }
            case ECk_EntityFragmentRequirementPolicy::IgnoreIfFragmentMissing:
            {
                if (NOT HasTransformFragmentA)
                { return false; }

                if (NOT HasTransformFragmentB)
                { return true; }

                break;
            }
            default:
            {
                CK_INVALID_ENUM(InFragmentRequirementPolicy);
                return false;
            }
        }

        const auto& DistanceA = FVector::DistSquared(InOrigin, UCk_Utils_Transform_UE::Get_EntityCurrentLocation(InA));
        const auto& DistanceB = FVector::DistSquared(InOrigin, UCk_Utils_Transform_UE::Get_EntityCurrentLocation(InB));

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

auto
    UCk_Utils_CompositeAlgos_UE::
    TransformBasicDetails_ToActors(
        const TArray<FCk_EntityOwningActor_BasicDetails>& InEntitiesWithActor)
    -> TArray<AActor*>
{
    return ck::algo::Transform<TArray<AActor*>>(InEntitiesWithActor, [](const FCk_EntityOwningActor_BasicDetails& InBasicDetails)
    {
        return InBasicDetails.Get_Actor().Get();
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    TransformBasicDetails_ToEntities(
        const TArray<FCk_EntityOwningActor_BasicDetails>& InEntitiesWithActor)
    -> TArray<FCk_Handle>
{
    return ck::algo::Transform<TArray<FCk_Handle>>(InEntitiesWithActor, [](const FCk_EntityOwningActor_BasicDetails& InBasicDetails)
    {
        return InBasicDetails.Get_Handle();
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    TransformActors_ToEntities(
        const TArray<AActor*>& InActors)
    -> TArray<FCk_Handle>
{
    return ck::algo::Transform<TArray<FCk_Handle>>(InActors, [](AActor* InActor) -> FCk_Handle
    {
        CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(InActor), TEXT("Actor [{}] is NOT Ecs Ready when trying to transform"), InActor)
        { return {}; }

        return UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(InActor);
    });
}

auto
    UCk_Utils_CompositeAlgos_UE::
    TransformEntities_ToActors(
        const TArray<FCk_Handle>& InEntities)
    -> TArray<AActor*>
{
    return ck::algo::Transform<TArray<AActor*>>(InEntities, [](const FCk_Handle& InEntity) -> AActor*
    {
        CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InEntity), TEXT("Entity [{}] is MISSING an OwningActor Fragment when trying to transform"), InEntity)
        { return {}; }

        return UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InEntity);
    });
}

// --------------------------------------------------------------------------------------------------------------------
