#include "CkNavAreaMarkup_Utils.h"

#include "CkNavigation/NavAreaMarkup/CkNavArea_Restricted.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/ObjectPooling/CkObjectPooling_Params.h"
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
    return ck::IsValid(_AreaClass.Get());
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
    const auto OwnerIsValid = ck::IsValid(InOwnerEntity);
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("NavAreaMarkup Request_Create requires a valid owner entity"))
    { return {}; }

    const auto TransformIsFinite = NOT InWorldTransform.ContainsNaN();
    CK_ENSURE_IF_NOT(TransformIsFinite,
        TEXT("NavAreaMarkup Request_Create on [{}] requires a finite transform"), InOwnerEntity)
    { return {}; }

    const auto HalfExtentsAreValid = FMath::IsFinite(InHalfExtents.X)
        && FMath::IsFinite(InHalfExtents.Y)
        && FMath::IsFinite(InHalfExtents.Z)
        && InHalfExtents.X > 0.0f
        && InHalfExtents.Y > 0.0f
        && InHalfExtents.Z > 0.0f;
    CK_ENSURE_IF_NOT(HalfExtentsAreValid,
        TEXT("NavAreaMarkup Request_Create on [{}] requires finite positive half extents [{}]"),
        InOwnerEntity, InHalfExtents)
    { return {}; }

    const auto AreaClassIsValid = ck::IsValid(InAreaClass.Get());
    CK_ENSURE_IF_NOT(AreaClassIsValid,
        TEXT("NavAreaMarkup Request_Create on [{}] requires a valid AreaClass"), InOwnerEntity)
    { return {}; }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InOwnerEntity);
    const auto WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid,
        TEXT("NavAreaMarkup Request_Create: no World for entity [{}]"), InOwnerEntity)
    { return {}; }

    const auto PoolParams = FCk_ObjectPooling_PoolParams{}
        .Set_RecyclePolicy(ECk_ObjectPooling_RecyclePolicy::DestroyOnRelease);
    auto* Markup = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_NavAreaMarkup_UE>(
        World,
        UCk_NavAreaMarkup_UE::StaticClass(),
        nullptr,
        PoolParams,
        [&](auto* InMarkup)
        {
            InMarkup->_Transform = InWorldTransform;
            InMarkup->_HalfExtents = InHalfExtents;
            InMarkup->_AreaClass = InAreaClass;
            InMarkup->_IsRegistered = false;
        });
    const auto MarkupIsValid = ck::IsValid(Markup);
    CK_ENSURE_IF_NOT(MarkupIsValid,
        TEXT("NavAreaMarkup Request_Create on [{}] could not acquire a rooted markup object"), InOwnerEntity)
    { return {}; }

    UNavigationSystemV1::OnNavRelevantObjectRegistered(*Markup);
    Markup->_IsRegistered = true;
    return Markup;
}

auto
    UCk_Utils_NavAreaMarkup_UE::
    Request_Destroy(
        UCk_NavAreaMarkup_UE* InMarkup)
    -> void
{
    if (ck::Is_NOT_Valid(InMarkup))
    { return; }

    if (InMarkup->_IsRegistered)
    {
        UNavigationSystemV1::OnNavRelevantObjectUnregistered(*InMarkup);
        InMarkup->_IsRegistered = false;
    }

    UCk_Utils_Object_UE::TryReleaseToPool(InMarkup);
}
