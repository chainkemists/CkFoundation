#pragma once
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPoi/CkPoi_Fragment.h"
#include "CkPoi/CkPoi_Fragment_Data.h"

#include <NativeGameplayTags.h>

#include "CkPoi_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Root for POI category tags (Poi.Category.*), handed to CkEntityTag Add_UsingGameplayTag by Add.
CKPOI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Poi_CategoryName);

// The enable/disable convention tag: a disabled POI carries this EntityTag; projectors skip it. Add/remove via
// UCk_Utils_EntityTag_UE (deferred one pump, COUNTED — N disables need N enables).
CKPOI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Poi_DisabledName);

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
    // POI is a META-FEATURE: Add composes ck::FTag_Poi (identity) + a CkEntityTag category (Params._Category) and,
    // when Params._Label is set, a CkLabel. The entity must already carry the Transform feature — a POI's world
    // position is its own entity transform location (Get_WorldLocation). No child entity is created; one POI per
    // entity. Category / label / presentation / range all live in their own modules now (CkEntityTag / CkLabel /
    // CkPoiDisplayDefinition / CkVisibleRange) — see the CkPoi v2 refactor.
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
    // The POI entity's own Transform location — the position every projector (compass/minimap/world indicator) reads.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get World Location")
    static FVector
    Get_WorldLocation(
        const FCk_Handle_Poi& InPoi);

    // The POI's category tags: the entity's CkEntityTag set filtered to Poi.Category descendants. Empty when the
    // EntityTag store is not materialized yet (Add is deferred one pump) or the POI carries no category.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Poi",
              DisplayName="[Ck][Poi] Get Category Tags")
    static FGameplayTagContainer
    Get_CategoryTags(
        const FCk_Handle_Poi& InPoi);
};

// --------------------------------------------------------------------------------------------------------------------
