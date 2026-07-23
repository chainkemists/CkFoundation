#pragma once

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkDialog/CkDialogBank_DataAsset.h"
#include "CkDialog/Line/CkDialogLine_Fragment.h"
#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"

#include "CkDialogLine_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_DialogLine"))
class CKDIALOG_API UCk_Utils_DialogLine_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DialogLine_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_DialogLine);

private:
    struct RecordOfDialogConditions_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfDialogConditions> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Dialog|Line",
        DisplayName="[Ck][Dialog][Line] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_DialogLine
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Dialog|Line",
        DisplayName="[Ck][Dialog][Line] Handle -> DialogLine Handle",
        meta = (CompactNodeTitle = "<AsDialogLine>", BlueprintAutocast))
    static FCk_Handle_DialogLine
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid DialogLine Handle",
        Category = "Ck|Utils|Dialog|Line",
        meta = (CompactNodeTitle = "INVALID_DialogLineHandle", Keywords = "make"))
    static FCk_Handle_DialogLine
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Line",
              DisplayName="[Ck][Dialog][Line] Get Line ID")
    static FName
    Get_LineID(
        const FCk_Handle_DialogLine& InLine);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Line",
              DisplayName="[Ck][Dialog][Line] Get Text")
    static FText
    Get_Text(
        const FCk_Handle_DialogLine& InLine);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Line",
              DisplayName="[Ck][Dialog][Line] Get Enter Tag")
    static FGameplayTag
    Get_EventTag(
        const FCk_Handle_DialogLine& InLine);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Line",
              DisplayName="[Ck][Dialog][Line] Get Exit Tag")
    static FGameplayTag
    Get_LinkedEventTag(
        const FCk_Handle_DialogLine& InLine);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Line",
              DisplayName="[Ck][Dialog][Line] Get Has Exit Tag")
    static bool
    Get_HasLinkedEventTag(
        const FCk_Handle_DialogLine& InLine);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Line",
              DisplayName="[Ck][Dialog][Line] Get Filter Tags")
    static FGameplayTagContainer
    Get_FilterTags(
        const FCk_Handle_DialogLine& InLine);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Line",
              DisplayName="[Ck][Dialog][Line] Get Num Conditions")
    static int32
    Get_NumConditions(
        const FCk_Handle_DialogLine& InLine);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Line",
              DisplayName="[Ck][Dialog][Line] Get Conditions")
    static TArray<FCk_Handle>
    Get_Conditions(
        const FCk_Handle_DialogLine& InLine);

public:
    // C++-only iteration used by the query processor.
    static auto
    ForEach_Condition(
        const FCk_Handle_DialogLine& InLine,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

    // C++-only, subsystem-internal. Builds one line entity under InRegistryRoot (Params + FTag_DialogLine + debug
    // name + ENTER-tag registration into per-tag storage) and spawns one long-lived condition child entity per valid
    // archetype (each connected into the line's condition record on post-construction, then InOnConditionConstructed
    // fires). Returns the line handle; OutNumConditionsQueued receives the count of condition children queued (the
    // caller uses it to gate registry readiness on all conditions having constructed).
    static auto
    Create(
        const FCk_Handle& InRegistryRoot,
        const FGameplayTagContainer& InBankTags,
        const FCk_DialogBank_LineData& InLineData,
        const TFunction<void()>& InOnConditionConstructed,
        int32& OutNumConditionsQueued) -> FCk_Handle_DialogLine;
};

// --------------------------------------------------------------------------------------------------------------------
