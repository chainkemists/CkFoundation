#pragma once
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkPoi/CkPoi_Fragment.h"

#include "CkPoi/CkPoi_Fragment_Data.h"

#include <NativeGameplayTags.h>

#include "CkPoi_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKPOI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Poi_CategoryName);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Poi"))
class CKPOI_API UCk_Utils_Poi_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Poi_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Poi);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Compose the POI feature DIRECTLY onto InHandle (no child entity). The entity must already have the
    // Transform feature — the POI's world position is EntityTransform.TransformPosition(Params._RelativeLocation).
    // An entity hosts at most ONE POI; spawn one entity per POI (UCk_Poi_EntityScript is the standalone recipe).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Add Feature")
    static FCk_Handle_Poi
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Poi_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Poi",
        DisplayName="[Ck][Poi] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Poi
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Poi",
        DisplayName="[Ck][Poi] Handle -> Poi Handle",
        meta = (CompactNodeTitle = "<AsPoi>", BlueprintAutocast))
    static FCk_Handle_Poi
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Poi Handle",
        Category = "Ck|Utils|Poi",
        meta = (CompactNodeTitle = "INVALID_PoiHandle", Keywords = "make"))
    static FCk_Handle_Poi
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get Category")
    static FGameplayTag
    Get_Category(
        const FCk_Handle_Poi& InPoi);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get Display Name")
    static FText
    Get_DisplayName(
        const FCk_Handle_Poi& InPoi);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get Priority")
    static int32
    Get_Priority(
        const FCk_Handle_Poi& InPoi);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get Max Visible Range")
    static float
    Get_MaxVisibleRange(
        const FCk_Handle_Poi& InPoi);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get Offscreen Policy")
    static ECk_Poi_OffscreenPolicy
    Get_OffscreenPolicy(
        const FCk_Handle_Poi& InPoi);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get Display Asset")
    static TSoftObjectPtr<UCk_Poi_DisplayDefinition_PDA>
    Get_DisplayAsset(
        const FCk_Handle_Poi& InPoi);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get Enable Disable")
    static ECk_EnableDisable
    Get_EnableDisable(
        const FCk_Handle_Poi& InPoi);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get State Tags")
    static FGameplayTagContainer
    Get_StateTags(
        const FCk_Handle_Poi& InPoi);

    // EntityTransform.TransformPosition(_RelativeLocation) — the position every projector (compass/minimap/world
    // indicator) reads. The transform is the POI entity's own (the entity Add was called on).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get World Location")
    static FVector
    Get_WorldLocation(
        const FCk_Handle_Poi& InPoi);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Request Enable Disable")
    static FCk_Handle_Poi
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Poi& InPoi,
        const FCk_Request_Poi_EnableDisable& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Request Add State Tag")
    static FCk_Handle_Poi
    Request_AddStateTag(
        UPARAM(ref) FCk_Handle_Poi& InPoi,
        const FCk_Request_Poi_AddStateTag& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Request Remove State Tag")
    static FCk_Handle_Poi
    Request_RemoveStateTag(
        UPARAM(ref) FCk_Handle_Poi& InPoi,
        const FCk_Request_Poi_RemoveStateTag& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Request Set State Tags")
    static FCk_Handle_Poi
    Request_SetStateTags(
        UPARAM(ref) FCk_Handle_Poi& InPoi,
        const FCk_Request_Poi_SetStateTags& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poi",
              DisplayName = "[Ck][Poi] Bind To OnStateChanged")
    static FCk_Handle_Poi
    BindTo_OnStateChanged(
        UPARAM(ref) FCk_Handle_Poi& InPoi,
        const FCk_Delegate_Poi_StateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poi",
              DisplayName = "[Ck][Poi] Bind To OnEnableDisable")
    static FCk_Handle_Poi
    BindTo_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Poi& InPoi,
        const FCk_Delegate_Poi_EnableDisable& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poi",
              DisplayName = "[Ck][Poi] Unbind From OnStateChanged")
    static FCk_Handle_Poi
    UnbindFrom_OnStateChanged(
        UPARAM(ref) FCk_Handle_Poi& InPoi,
        const FCk_Delegate_Poi_StateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Poi",
              DisplayName = "[Ck][Poi] Unbind From OnEnableDisable")
    static FCk_Handle_Poi
    UnbindFrom_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Poi& InPoi,
        const FCk_Delegate_Poi_EnableDisable& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
