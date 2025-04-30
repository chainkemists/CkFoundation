#pragma once

#include "CkFeedbackCascade_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkFeedbackCascade_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKFEEDBACK_API UCk_Utils_FeedbackCascade_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FeedbackCascade_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_FeedbackCascade);

private:
    struct RecordOfFeedbackCascade_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFeedbackCascade> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][FeedbackCascade] Add New FeedbackCascade")
    static FCk_Handle_FeedbackCascade
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_FeedbackCascade_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FeedbackCascade",
              DisplayName="[Ck][FeedbackCascade] Add Multiple New FeedbackCascades")
    static TArray<FCk_Handle_FeedbackCascade>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MultipleFeedbackCascade_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|FeedbackCascade",
        DisplayName="[Ck][FeedbackCascade] Has Any FeedbackCascade")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|FeedbackCascade",
              DisplayName="[Ck][FeedbackCascade] Try Get FeedbackCascade")
    static FCk_Handle_FeedbackCascade
    TryGet_FeedbackCascade(
        const FCk_Handle& InFeedbackCascadeOwnerEntity,
        UPARAM(meta = (Categories = "FeedbackCascade")) FGameplayTag InFeedbackCascadeName);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|FeedbackCascade",
        DisplayName="[Ck][FeedbackCascade] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_FeedbackCascade
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|FeedbackCascade",
        DisplayName="[Ck][FeedbackCascade] Handle -> FeedbackCascade Handle",
        meta = (CompactNodeTitle = "<AsFeedbackCascade>", BlueprintAutocast))
    static FCk_Handle_FeedbackCascade
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid FeedbackCascade Handle",
        Category = "Ck|Utils|FeedbackCascade",
        meta = (CompactNodeTitle = "INVALID_FeedbackCascadeHandle", Keywords = "make"))
    static FCk_Handle_FeedbackCascade
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
        DisplayName="[Ck][FeedbackCascade] Request Play (At Location)",
        meta = (DefaultToSelf = "InContextObject"))
    static FCk_Handle_FeedbackCascade
    Request_PlayAtLocation(
        UObject* InContextObject,
        UPARAM(ref) FCk_Handle_FeedbackCascade& InFeedbackCascadeHandle,
        const FCk_Request_FeedbackCascade_PlayAtLocation& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
        DisplayName="[Ck][FeedbackCascade] Request Play (Attached)")
    static FCk_Handle_FeedbackCascade
    Request_PlayAttached(
        UPARAM(ref) FCk_Handle_FeedbackCascade& InFeedbackCascadeHandle,
        const FCk_Request_FeedbackCascade_PlayAttached& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------