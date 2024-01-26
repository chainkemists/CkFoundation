#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Delegates/CkDelegates.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkCompositeAlgos_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCOMPOSITEALGOS_API UCk_Utils_CompositeAlgos_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CompositeAlgos_UE);

private:
    // Returns true if at least one actor satisfies the predicate
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Any Actors If",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static bool
    AnyActors_If(
        const TArray<AActor*>& InActors,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InActor_OutResult InPredicate);

    // Returns true if all actors satisfy the predicate
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] All Actors If",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static bool
    AllActors_If(
        const TArray<AActor*>& InActors,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InActor_OutResult InPredicate);

    // Returns true if at least one entity satisfies the predicate
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Any Entities If",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static bool
    AnyEntities_If(
        const TArray<FCk_Handle>& InEntities,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate);

    // Returns true if all entities satisfy the predicate
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] All Entities If",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static bool
    AllEntities_If(
        const TArray<FCk_Handle>& InEntities,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Filter Actors By Predicate",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static TArray<AActor*>
    FilterActors_ByPredicate(
        const TArray<AActor*>& InActors,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InActor_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Filter Actors By IsValid",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static TArray<AActor*>
    FilterActors_ByIsValid(
        const TArray<AActor*>& InActors);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Filter Entities By Predicate",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static TArray<FCk_Handle>
    FilterEntities_ByPredicate(
        const TArray<FCk_Handle>& InEntities,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InHandle_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Filter Entities By IsValid")
    static TArray<FCk_Handle>
    FilterEntities_ByIsValid(
        const TArray<FCk_Handle>& InEntities);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Sort Actors By Predicate",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static void
    SortActors_ByPredicate(
        UPARAM(ref) TArray<AActor*>& InActors,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_In2Actors_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Sort Actors By Distance")
    static void
    SortActors_ByDistance(
        UPARAM(ref) TArray<AActor*>& InActors,
        const FVector& InOrigin,
        ECk_DistanceSortingPolicy InSortingPolicy);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Sort Entities By Predicate",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static void
    SortEntities_ByPredicate(
        UPARAM(ref) TArray<FCk_Handle>& InEntities,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_In2Handles_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos",
              DisplayName = "[Ck] Sort Entities By Distance")
    static void
    SortEntities_ByDistance(
        UPARAM(ref) TArray<FCk_Handle>& InEntities,
        const FVector& InOrigin,
        ECk_DistanceSortingPolicy InSortingPolicy,
        ECk_EntityFragmentRequirementPolicy InFragmentRequirementPolicy = ECk_EntityFragmentRequirementPolicy::EnsureAllFragments);

private:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Transform BasicDetails To Actors",
              Category = "Ck|Utils|Algos",
              meta = (CompactNodeTitle = "Transform_ToActors"))
    static TArray<AActor*>
    TransformBasicDetails_ToActors(
        const TArray<FCk_EntityOwningActor_BasicDetails>& InEntitiesWithActor);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Transform BasicDetails To Entities",
              Category = "Ck|Utils|Algos",
              meta = (CompactNodeTitle = "Transform_ToEntities"))
    static TArray<FCk_Handle>
    TransformBasicDetails_ToEntities(
        const TArray<FCk_EntityOwningActor_BasicDetails>& InEntitiesWithActor);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Transform Actors To Entities",
              Category = "Ck|Utils|Algos",
              meta = (CompactNodeTitle = "Transform_ToEntities"))
    static TArray<FCk_Handle>
    TransformActors_ToEntities(
        const TArray<AActor*>& InActors);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Transform Entities To Actors",
              Category = "Ck|Utils|Algos",
              meta = (CompactNodeTitle = "Transform_ToActors"))
    static TArray<AActor*>
    TransformEntities_ToActors(
        const TArray<FCk_Handle>& InEntities);
};

// --------------------------------------------------------------------------------------------------------------------

