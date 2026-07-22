#include "CkPoi_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkPoi/CkPoi_Fragment.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Poi_CategoryName, TEXT("Poi.Category"))
UE_DEFINE_GAMEPLAY_TAG(Tag_Poi_DisabledName, TEXT("Poi.Disabled"))

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Poi_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Poi_ParamsData& InParams)
    -> FCk_Handle_Poi
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle supplied to Poi Add"))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT Has(InHandle),
        TEXT("Handle [{}] already has the Poi feature. An entity hosts at most ONE POI — spawn one entity per POI"),
        InHandle)
    { return Cast(InHandle); }

    CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
        TEXT("Poi Add requires the Entity [{}] to have the Transform feature — the POI's world position is "
             "derived from the entity's transform"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Category()),
        TEXT("Poi Add on Entity [{}] requires a valid Category tag (Poi.Category.*)"), InHandle)
    { return {}; }

    InHandle.Add<ck::FTag_Poi>();

    UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(InHandle, InParams.Get_Category());

    if (ck::IsValid(InParams.Get_Label()))
    {
        UCk_Utils_GameplayLabel_UE::Add(InHandle, InParams.Get_Label());
    }

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Poi_UE, FCk_Handle_Poi, ck::FTag_Poi);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Poi_UE::
    Get_WorldLocation(
        const FCk_Handle_Poi& InPoi)
    -> FVector
{
    return UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InPoi).GetLocation();
}

auto
    UCk_Utils_Poi_UE::
    Get_CategoryTags(
        const FCk_Handle_Poi& InPoi)
    -> FGameplayTagContainer
{
    // Absence-safe: Get_AllTagsAsContainer returns empty when the EntityTag store is not present yet (Add is
    // deferred one pump), so Filter yields empty rather than ensuring.
    const auto AllTags = UCk_Utils_EntityTag_UE::Get_AllTagsAsContainer(InPoi);

    return AllTags.Filter(FGameplayTagContainer{Tag_Poi_CategoryName});
}

//--------------------------------------------------------------------------------------------------------------------
