#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessorScript.h"

#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkDataOnlyFragment_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTEMPLATE_API FCk_Fragment_DataOnlyFragment
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_DataOnlyFragment);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    int32 _Variable;

public:
    CK_PROPERTY_GET(_Variable);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_DataOnlyFragment, _Variable);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class UCk_Processor_DataOnlyFragment_UE : public UCk_Ecs_ProcessorScript_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Processor_DataOnlyFragment_UE);

public:
    CK_DEFINE_STAT(STAT_ForEachEntity, UCk_Processor_DataOnlyFragment_UE, FStatGroup_STATGROUP_CkProcessors_Details);
    CK_DEFINE_STAT(STAT_Tick, UCk_Processor_DataOnlyFragment_UE, FStatGroup_STATGROUP_CkProcessors);

public:
    auto Tick(
        TimeType InTime) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKTEMPLATE_API UCk_Utils_DataOnlyFragment_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DataOnlyFragment_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Fragments|DataOnlyFragment",
              DisplayName="Add New DataOnlyFragment")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_DataOnlyFragment& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Has Timer")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Timer",
              DisplayName="Ensure Has Timer")
    static bool
    Ensure(
        FCk_Handle InTimer);
};
// --------------------------------------------------------------------------------------------------------------------

