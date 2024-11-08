#pragma once

#include "CkECS/Handle/CkHandle.h"

#include "CkEntityExtension/CkEntityExtension_Fragment.h"
#include "CkEntityExtension/CkEntityExtension_Fragment_Data.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkEntityExtension_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKENTITYEXTENSION_API UCk_Utils_EntityExtension_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityExtension_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_EntityExtension);

private:
    struct RecordOfEntityExtensions_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfEntityExtensions> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityExtension",
              DisplayName="[Ck][EntityExtension] Add Entity As Extension")
    static FCk_Handle_EntityExtension
    Add(
        UPARAM(ref) FCk_Handle& InExtensionOwner,
        UPARAM(ref) FCk_Handle& InEntityToAddAsExtension,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityExtension",
              DisplayName="[Ck][EntityExtension] Remove Entity As Extension")
    static FCk_Handle_EntityExtension
    Remove(
        UPARAM(ref) FCk_Handle& InExtensionOwner,
        UPARAM(ref) FCk_Handle_EntityExtension& InEntityToRemoveAsExtension);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityExtension",
        DisplayName="[Ck][EntityExtension] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_EntityExtension
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|EntityExtension",
        DisplayName="[Ck][EntityExtension] Handle -> EntityExtension Handle",
        meta = (CompactNodeTitle = "<AsEntityExt>", BlueprintAutocast))
    static FCk_Handle_EntityExtension
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|EntityExtension",
        DisplayName="[Ck][EntityExtension] Cast")
    static FCk_Handle
    Get_ExtensionOwner(
        const FCk_Handle_EntityExtension& InEntityExtension);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityExtension",
              DisplayName="[Ck][EntityExtension] For Each",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_EntityExtension>
    ForEach_EntityExtension(
        UPARAM(ref) FCk_Handle& InEntityExtensionOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_EntityExtension(
        FCk_Handle& InEntityExtensionOwner,
        const TFunction<void(FCk_Handle_EntityExtension)>& InFunc) -> void;

};

// --------------------------------------------------------------------------------------------------------------------
