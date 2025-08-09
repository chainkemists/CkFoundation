#pragma once

#include "CkECS/Handle/CkHandle.h"

#include "CkEntityExtension/CkEntityExtension_Fragment.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEntityExtension_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_EntityExtension"))
class CKENTITYEXTENSION_API UCk_Utils_EntityExtension_UE : public UBlueprintFunctionLibrary
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
        UPARAM(ref) FCk_Handle& InEntityToAddAsExtension);

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

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid EntityExtension Handle",
        Category = "Ck|Utils|EntityExtension",
        meta = (CompactNodeTitle = "INVALID_EntityExtensionHandle", Keywords = "make"))
    static FCk_Handle_EntityExtension
    Get_InvalidHandle() { return {}; };

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
        DisplayName="[Ck][EntityExtension] Bind to OnExtensionAdded")
    static FCk_Handle
    BindTo_OnExtensionAdded(
        UPARAM(ref) FCk_Handle& InExtensionOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityExtension_OnExtensionAdded& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityExtension",
        DisplayName="[Ck][EntityExtension] Unbind from OnExtensionAdded")
    static FCk_Handle
    UnbindFrom_OnExtensionAdded(
        UPARAM(ref) FCk_Handle& InExtensionOwner,
        const FCk_Delegate_EntityExtension_OnExtensionAdded& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityExtension",
        DisplayName="[Ck][EntityExtension] Bind to OnExtensionRemoved")
    static FCk_Handle
    BindTo_OnExtensionRemoved(
        UPARAM(ref) FCk_Handle& InExtensionOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityExtension_OnExtensionRemoved& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityExtension",
        DisplayName="[Ck][EntityExtension] Unbind from OnExtensionRemoved")
    static FCk_Handle
    UnbindFrom_OnExtensionRemoved(
        UPARAM(ref) FCk_Handle& InExtensionOwner,
        const FCk_Delegate_EntityExtension_OnExtensionRemoved& InDelegate);

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

private:
    static auto
    DoBoadcast_ExtensionAdded(
        const FCk_Handle& InExtensionOwner,
        const FCk_Handle_EntityExtension& InEntityExtension) -> void;

    static auto
    DoBoadcast_ExtensionRemoved(
        const FCk_Handle& InExtensionOwner,
        const FCk_Handle_EntityExtension& InEntityExtension) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
