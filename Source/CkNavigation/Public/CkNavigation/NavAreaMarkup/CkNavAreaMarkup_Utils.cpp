#include "CkNavAreaMarkup_Utils.h"

#include "CkNavigation/NavAreaMarkup/CkNavArea_Restricted.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <AI/NavigationModifier.h>
#include <NavigationSystem.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_NavAreaMarkup_UE::
    GetNavigationBounds() const
    -> FBox
{
    return FBox::BuildAABB(FVector::ZeroVector, _HalfExtents).TransformBy(_Transform);
}

auto
    UCk_NavAreaMarkup_UE::
    GetNavigationData(
        FNavigationRelevantData& Data) const
    -> void
{
    Data.Modifiers.Add(FAreaNavModifier{
        FBox::BuildAABB(FVector::ZeroVector, _HalfExtents), _Transform, _AreaClass});
}

auto
    UCk_NavAreaMarkup_UE::
    IsNavigationRelevant() const
    -> bool
{
    return ck::IsValid(_AreaClass.Get(), ck::IsValid_Policy_NullptrOnly{});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavAreaMarkup_UE::
    Request_Create(
        FCk_Handle& InOwnerEntity,
        const FTransform& InWorldTransform,
        const FVector& InHalfExtents,
        TSubclassOf<UNavArea> InAreaClass)
    -> UCk_NavAreaMarkup_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAreaClass.Get(), ck::IsValid_Policy_NullptrOnly{}),
        TEXT("NavAreaMarkup Request_Create on [{}] requires a valid AreaClass"), InOwnerEntity)
    { return {}; }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InOwnerEntity);
    CK_ENSURE_IF_NOT(ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("NavAreaMarkup Request_Create: no World for entity [{}]"), InOwnerEntity)
    { return {}; }

    auto Markup = NewObject<UCk_NavAreaMarkup_UE>(World);
    Markup->_Transform   = InWorldTransform;
    Markup->_HalfExtents = InHalfExtents;
    Markup->_AreaClass   = InAreaClass;

    UNavigationSystemV1::OnNavRelevantObjectRegistered(*Markup);
    return Markup;
}

auto
    UCk_Utils_NavAreaMarkup_UE::
    Request_Destroy(
        UCk_NavAreaMarkup_UE* InMarkup)
    -> void
{
    if (ck::Is_NOT_Valid(InMarkup, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    UNavigationSystemV1::OnNavRelevantObjectUnregistered(*InMarkup);
}
