#include "CkMinimap_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkMinimap/CkMinimap_Log.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_PoiConsumer_Minimap, TEXT("Poi.Consumer.Minimap"))

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Minimap_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Minimap_ParamsData& InParams)
    -> FCk_Handle_Minimap
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle supplied to Minimap Add"))
    { return {}; }

    // NO owner-Transform requirement — a FixedBounds world map on a transform-less HUD entity is legal;
    // only the OBSERVER needs a Transform, and the Update processor ensures that

    CK_ENSURE_IF_NOT(NOT Has(InHandle),
        TEXT("Handle [{}] already has the Minimap feature. Compose additional minimaps as child entities via Create"), InHandle)
    { return Cast(InHandle); }

    InHandle.Add<ck::FFragment_Minimap_Params>(InParams);
    InHandle.Add<ck::FFragment_Minimap_Current>();
    InHandle.Add<ck::FTag_Minimap_NeedsSetup>();

    return Cast(InHandle);
}

auto
    UCk_Utils_Minimap_UE::
    Create(
        FCk_Handle& InLifetimeOwner,
        const FCk_Fragment_Minimap_ParamsData& InParams)
    -> FCk_Handle_Minimap
{
    CK_ENSURE_IF_NOT(ck::IsValid(InLifetimeOwner), TEXT("Invalid Lifetime Owner supplied to Minimap Create"))
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    auto NewMinimapEntity = Add(NewEntity, InParams);

    if (ck::Is_NOT_Valid(NewMinimapEntity))
    { return {}; }

    // Minimap children carry NO GameplayLabel (there is no category) — record ByTag lookups are unusable,
    // enumeration is ForEach_ValidEntry only
    RecordOfMinimaps_Utils::AddIfMissing(InLifetimeOwner, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfMinimaps_Utils::Request_Connect(InLifetimeOwner, NewMinimapEntity, ECk_Record_LabelRequirementPolicy::Optional);

    // Standalone child: the lifetime owner is the observer (the entity whose position/view drives the projection)
    Request_SetObserver(NewMinimapEntity, InLifetimeOwner);

    return NewMinimapEntity;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Minimap_UE, FCk_Handle_Minimap, ck::FFragment_Minimap_Current, ck::FFragment_Minimap_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Minimap_UE::
    Has_Any(
        const FCk_Handle& InMinimapOwnerEntity)
    -> bool
{
    return RecordOfMinimaps_Utils::Has(InMinimapOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Minimap_UE::
    ForEach_Minimap(
        const FCk_Handle& InMinimapOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Minimap>
{
    auto Minimaps = TArray<FCk_Handle_Minimap>{};

    ForEach_Minimap(InMinimapOwnerEntity, [&](FCk_Handle_Minimap InMinimap)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InMinimap, InOptionalPayload); }
        else
        { Minimaps.Emplace(InMinimap); }
    });

    return Minimaps;
}

auto
    UCk_Utils_Minimap_UE::
    ForEach_Minimap(
        const FCk_Handle& InMinimapOwnerEntity,
        const TFunction<void(FCk_Handle_Minimap)>& InFunc)
    -> void
{
    RecordOfMinimaps_Utils::ForEach_ValidEntry(InMinimapOwnerEntity, InFunc);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Minimap_UE::
    Get_Entries(
        const FCk_Handle_Minimap& InMinimap)
    -> TArray<FCk_Minimap_Entry>
{
    return InMinimap.Get<ck::FFragment_Minimap_Current>().Get_Entries();
}

auto
    UCk_Utils_Minimap_UE::
    Get_Observer(
        const FCk_Handle_Minimap& InMinimap)
    -> FCk_Handle
{
    return InMinimap.Get<ck::FFragment_Minimap_Current>().Get_Observer();
}

auto
    UCk_Utils_Minimap_UE::
    Get_ViewOrigin(
        const FCk_Handle_Minimap& InMinimap)
    -> FVector
{
    return InMinimap.Get<ck::FFragment_Minimap_Current>().Get_ViewOrigin();
}

auto
    UCk_Utils_Minimap_UE::
    Get_ViewYawDegrees(
        const FCk_Handle_Minimap& InMinimap)
    -> float
{
    return InMinimap.Get<ck::FFragment_Minimap_Current>().Get_ViewYawDegrees();
}

auto
    UCk_Utils_Minimap_UE::
    Get_ViewExtent(
        const FCk_Handle_Minimap& InMinimap)
    -> float
{
    return InMinimap.Get<ck::FFragment_Minimap_Current>().Get_ViewExtent();
}

auto
    UCk_Utils_Minimap_UE::
    Get_ProjectionMode(
        const FCk_Handle_Minimap& InMinimap)
    -> ECk_Minimap_ProjectionMode
{
    return InMinimap.Get<ck::FFragment_Minimap_Params>().Get_ProjectionMode();
}

auto
    UCk_Utils_Minimap_UE::
    Get_RotationMode(
        const FCk_Handle_Minimap& InMinimap)
    -> ECk_Minimap_RotationMode
{
    return InMinimap.Get<ck::FFragment_Minimap_Current>().Get_RotationMode();
}

auto
    UCk_Utils_Minimap_UE::
    Get_FrameShape(
        const FCk_Handle_Minimap& InMinimap)
    -> ECk_Minimap_FrameShape
{
    return InMinimap.Get<ck::FFragment_Minimap_Params>().Get_FrameShape();
}

auto
    UCk_Utils_Minimap_UE::
    Get_FixedBounds(
        const FCk_Handle_Minimap& InMinimap)
    -> FCk_Minimap_WorldBounds
{
    return InMinimap.Get<ck::FFragment_Minimap_Params>().Get_FixedBounds();
}

auto
    UCk_Utils_Minimap_UE::
    Get_FrameToWorld(
        const FCk_Handle_Minimap& InMinimap,
        FVector2D InFramePosition)
    -> FVector2D
{
    const auto& Params = InMinimap.Get<ck::FFragment_Minimap_Params>();
    const auto& Current = InMinimap.Get<ck::FFragment_Minimap_Current>();

    if (Params.Get_ProjectionMode() == ECk_Minimap_ProjectionMode::FixedBounds)
    {
        return Get_FrameToWorld_Bounds(InFramePosition, Params.Get_FixedBounds());
    }

    return Get_FrameToWorld(InFramePosition, Current.Get_ViewOrigin(),
        Current.Get_ViewYawDegrees(), Current.Get_ViewExtent(), Current.Get_RotationMode());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Minimap_UE::
    Request_SetViewExtent(
        FCk_Handle_Minimap& InMinimap,
        float InViewExtent)
    -> FCk_Handle_Minimap
{
    CK_CALLSTACK_RECORD(ck::FFragment_Minimap_Requests, InMinimap);

    InMinimap.AddOrGet<ck::FFragment_Minimap_Requests>()._Requests.Emplace(
        FCk_Request_Minimap_SetViewExtent{InViewExtent});

    return InMinimap;
}

auto
    UCk_Utils_Minimap_UE::
    Request_SetCategoryFilter(
        FCk_Handle_Minimap& InMinimap,
        const FCk_Request_Minimap_SetCategoryFilter& InRequest)
    -> FCk_Handle_Minimap
{
    CK_CALLSTACK_RECORD(ck::FFragment_Minimap_Requests, InMinimap);

    InMinimap.AddOrGet<ck::FFragment_Minimap_Requests>()._Requests.Emplace(InRequest);

    return InMinimap;
}

auto
    UCk_Utils_Minimap_UE::
    Request_SetObserver(
        FCk_Handle_Minimap& InMinimap,
        FCk_Handle InObserver)
    -> FCk_Handle_Minimap
{
    CK_CALLSTACK_RECORD(ck::FFragment_Minimap_Requests, InMinimap);

    InMinimap.AddOrGet<ck::FFragment_Minimap_Requests>()._Requests.Emplace(
        FCk_Request_Minimap_SetObserver{InObserver});

    return InMinimap;
}

auto
    UCk_Utils_Minimap_UE::
    Request_SetRotationMode(
        FCk_Handle_Minimap& InMinimap,
        ECk_Minimap_RotationMode InRotationMode)
    -> FCk_Handle_Minimap
{
    CK_CALLSTACK_RECORD(ck::FFragment_Minimap_Requests, InMinimap);

    InMinimap.AddOrGet<ck::FFragment_Minimap_Requests>()._Requests.Emplace(
        FCk_Request_Minimap_SetRotationMode{InRotationMode});

    return InMinimap;
}

auto
    UCk_Utils_Minimap_UE::
    Request_SetFogOfWar(
        FCk_Handle_Minimap& InMinimap,
        FCk_Handle_FogOfWar InFogOfWar)
    -> FCk_Handle_Minimap
{
    CK_CALLSTACK_RECORD(ck::FFragment_Minimap_Requests, InMinimap);

    InMinimap.AddOrGet<ck::FFragment_Minimap_Requests>()._Requests.Emplace(
        FCk_Request_Minimap_SetFogOfWar{InFogOfWar});

    return InMinimap;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Minimap_UE::
    BindTo_OnEntryAppeared(
        FCk_Handle_Minimap& InMinimap,
        const FCk_Delegate_Minimap_EntryAppeared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Minimap
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnMinimapEntryAppeared, InMinimap, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InMinimap;
}

auto
    UCk_Utils_Minimap_UE::
    BindTo_OnEntryDisappeared(
        FCk_Handle_Minimap& InMinimap,
        const FCk_Delegate_Minimap_EntryDisappeared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Minimap
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnMinimapEntryDisappeared, InMinimap, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InMinimap;
}

auto
    UCk_Utils_Minimap_UE::
    UnbindFrom_OnEntryAppeared(
        FCk_Handle_Minimap& InMinimap,
        const FCk_Delegate_Minimap_EntryAppeared& InDelegate)
    -> FCk_Handle_Minimap
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnMinimapEntryAppeared, InMinimap, InDelegate);

    return InMinimap;
}

auto
    UCk_Utils_Minimap_UE::
    UnbindFrom_OnEntryDisappeared(
        FCk_Handle_Minimap& InMinimap,
        const FCk_Delegate_Minimap_EntryDisappeared& InDelegate)
    -> FCk_Handle_Minimap
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnMinimapEntryDisappeared, InMinimap, InDelegate);

    return InMinimap;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Minimap_UE::
    Get_WorldToFrame(
        const FVector& InWorldPos,
        const FVector& InViewOrigin,
        float InViewYawDegrees,
        float InViewExtent,
        ECk_Minimap_RotationMode InRotationMode)
    -> FVector2D
{
    if (InViewExtent <= 0.0f)
    { return FVector2D::ZeroVector; }

    auto Delta = FVector2D{InWorldPos.X - InViewOrigin.X, InWorldPos.Y - InViewOrigin.Y};

    if (InRotationMode == ECk_Minimap_RotationMode::RotateWithObserver)
    {
        Delta = Delta.GetRotated(-InViewYawDegrees);
    }

    return FVector2D{Delta.Y / InViewExtent, -Delta.X / InViewExtent};
}

auto
    UCk_Utils_Minimap_UE::
    Get_FrameToWorld(
        const FVector2D& InFramePos,
        const FVector& InViewOrigin,
        float InViewYawDegrees,
        float InViewExtent,
        ECk_Minimap_RotationMode InRotationMode)
    -> FVector2D
{
    // Invert the axis swap first (dX = -Frame.Y*E, dY = Frame.X*E), then undo the view rotation
    auto Delta = FVector2D{-InFramePos.Y * InViewExtent, InFramePos.X * InViewExtent};

    if (InRotationMode == ECk_Minimap_RotationMode::RotateWithObserver)
    {
        Delta = Delta.GetRotated(InViewYawDegrees);
    }

    return FVector2D{InViewOrigin.X + Delta.X, InViewOrigin.Y + Delta.Y};
}

auto
    UCk_Utils_Minimap_UE::
    Get_BoundsToFrame(
        const FVector& InWorldPos,
        const FCk_Minimap_WorldBounds& InBounds)
    -> FVector2D
{
    if (ck::Is_NOT_Valid(InBounds))
    { return FVector2D::ZeroVector; }

    const auto& Center = InBounds.Get_Center();
    const auto& HalfExtents = InBounds.Get_HalfExtents();

    return FVector2D
    {
        static_cast<float>((InWorldPos.Y - Center.Y) / HalfExtents.Y),
        static_cast<float>(-(InWorldPos.X - Center.X) / HalfExtents.X)
    };
}

auto
    UCk_Utils_Minimap_UE::
    Get_FrameToWorld_Bounds(
        const FVector2D& InFramePos,
        const FCk_Minimap_WorldBounds& InBounds)
    -> FVector2D
{
    if (ck::Is_NOT_Valid(InBounds))
    { return FVector2D::ZeroVector; }

    const auto& Center = InBounds.Get_Center();
    const auto& HalfExtents = InBounds.Get_HalfExtents();

    return FVector2D
    {
        Center.X - InFramePos.Y * HalfExtents.X,
        Center.Y + InFramePos.X * HalfExtents.Y
    };
}

auto
    UCk_Utils_Minimap_UE::
    Get_IsInsideFrame(
        const FVector2D& InFramePos,
        ECk_Minimap_FrameShape InFrameShape)
    -> bool
{
    switch (InFrameShape)
    {
        case ECk_Minimap_FrameShape::Rectangle:
        {
            return FMath::Abs(InFramePos.X) <= 1.0 && FMath::Abs(InFramePos.Y) <= 1.0;
        }
        case ECk_Minimap_FrameShape::Circle:
        {
            return InFramePos.SizeSquared() <= 1.0;
        }
        default:
        {
            CK_INVALID_ENUM(InFrameShape);
            return true;
        }
    }
}

auto
    UCk_Utils_Minimap_UE::
    Get_ClampToFrame(
        const FVector2D& InFramePos,
        ECk_Minimap_FrameShape InFrameShape)
    -> FVector2D
{
    if (Get_IsInsideFrame(InFramePos, InFrameShape))
    { return InFramePos; }

    switch (InFrameShape)
    {
        case ECk_Minimap_FrameShape::Rectangle:
        {
            const auto MaxComponent = FMath::Max(FMath::Abs(InFramePos.X), FMath::Abs(InFramePos.Y));

            if (MaxComponent <= 0.0)
            { return FVector2D::ZeroVector; }

            return InFramePos / MaxComponent;
        }
        case ECk_Minimap_FrameShape::Circle:
        {
            const auto Length = InFramePos.Size();

            if (Length <= 0.0)
            { return FVector2D::ZeroVector; }

            return InFramePos / Length;
        }
        default:
        {
            CK_INVALID_ENUM(InFrameShape);
            return InFramePos;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
