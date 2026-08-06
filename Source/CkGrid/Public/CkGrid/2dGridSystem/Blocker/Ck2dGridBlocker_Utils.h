#pragma once

#include "Ck2dGridBlocker_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Data.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"

#include "Ck2dGridBlocker_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_2dGridBlocker"))
class CKGRID_API UCk_Utils_2dGridBlocker_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_2dGridBlocker_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_2dGridBlocker);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridBlocker",
              DisplayName="[Ck][2dGridBlocker] Add Feature")
    static FCk_Handle_2dGridBlocker
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_2dGridBlocker_Spec& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridBlocker",
              DisplayName="[Ck][2dGridBlocker] Create")
    static FCk_Handle_2dGridBlocker
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_2dGridBlocker_Spec& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridBlocker",
              DisplayName="[Ck][2dGridBlocker] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|2dGridBlocker",
        DisplayName="[Ck][2dGridBlocker] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_2dGridBlocker
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|2dGridBlocker",
        DisplayName="[Ck][2dGridBlocker] Handle -> Blocker Handle",
        meta = (CompactNodeTitle = "<As2dGridBlocker>", BlueprintAutocast))
    static FCk_Handle_2dGridBlocker
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Blocker Handle",
        Category = "Ck|Utils|2dGridBlocker",
        meta = (CompactNodeTitle = "INVALID_2dGridBlockerHandle", Keywords = "make"))
    static FCk_Handle_2dGridBlocker
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridBlocker",
              DisplayName="[Ck][2dGridBlocker] Request Set Active",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_2dGridBlocker
    Request_SetActive(
        UPARAM(ref) FCk_Handle_2dGridBlocker& InBlocker,
        bool InActive,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridBlocker",
              DisplayName="[Ck][2dGridBlocker] Get Grid")
    static FCk_Handle_2dGridSystem
    Get_Grid(
        const FCk_Handle_2dGridBlocker& InBlocker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridBlocker",
              DisplayName="[Ck][2dGridBlocker] Get Blocked Cells")
    static TArray<FIntPoint>
    Get_BlockedCells(
        const FCk_Handle_2dGridBlocker& InBlocker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridBlocker",
              DisplayName="[Ck][2dGridBlocker] Get Blockers")
    static TArray<FCk_Handle_2dGridBlocker>
    Get_Blockers(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridBlocker",
              DisplayName="[Ck][2dGridBlocker] Get Blocker With Tag")
    static FCk_Handle_2dGridBlocker
    Get_BlockerWithTag(
        const FCk_Handle_2dGridSystem& InGrid,
        FGameplayTag InTag);

private:
    // Death-watch handler: when the blocker entity is destroyed, decrement the
    // counted Disabled tag on every cell it stamped so the block is released.
    // Non-static: dynamic delegates bind to a UObject instance (the CDO here),
    // so this must be a member UFUNCTION even though it uses no instance state.
    UFUNCTION()
    void
    OnBlockerBeginDestroy(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
