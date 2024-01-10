#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Delegates/CkDelegates.h"

#include "CkCompositeAlgos_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCOMPOSITEALGOS_API UCk_Utils_CompositeAlgos_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CompositeAlgos_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos")
    static TArray<AActor*>
    FilterActors_ByPredicate(
        const TArray<AActor*>& InActors,
        FCk_Predicate_InActor_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos")
    static TArray<AActor*>
    FilterActors_ByIsValid(
        const TArray<AActor*>& InActors);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos")
    static void
    SortActors_ByPredicate(
        UPARAM(ref) TArray<AActor*>& InActors,
        FCk_Predicate_In2Actors_OutResult InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Algos")
    static void
    SortActors_ByDistance(
        UPARAM(ref) TArray<AActor*>& InActors,
        const FVector& InOrigin,
        ECk_DistanceSortingPolicy InSortingPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

