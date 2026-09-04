#pragma once

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_NavAreaMarkup_UE;
class UCk_Utils_NavSurface_UE;
class UWorld;

namespace ck::nav_surface
{
    CKNAVIGATION_API auto Request_NotifySurfaceRebuilt(
        UWorld*     InWorld,
        const FBox& InChangedBounds) -> void;
}

namespace ck::nav_surface_recast
{
    CKNAVIGATION_API auto Apply_AreaMarkup(
        UWorld*                                  InWorld,
        FCk_Handle&                              InMarkupEntity,
        const FCk_Request_NavSurface_AreaMarkup& InRequest) -> bool;

    CKNAVIGATION_API auto Release_AreaMarkup(
        UWorld*     InWorld,
        FCk_Handle& InMarkupEntity) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Which provider answers navigation-surface queries in this world, and whether it can answer
    // right now. Lives on the world's transient entity.
    struct CKNAVIGATION_API FFragment_NavSurface_Provider
    {
        CK_GENERATED_BODY(FFragment_NavSurface_Provider);

        friend class FProcessor_NavSurface_RevisionWatch;
        friend class ::UCk_Utils_NavSurface_UE;

    private:
        ECk_NavSurface_Provider _Provider = ECk_NavSurface_Provider::Recast;
        ECk_NavSurface_ShadowMode _ShadowMode = ECk_NavSurface_ShadowMode::Off;
        ECk_NavSurface_ProviderHealth _Health = ECk_NavSurface_ProviderHealth::NoData;

    public:
        CK_PROPERTY_GET(_Provider);
        CK_PROPERTY_GET(_ShadowMode);
        CK_PROPERTY_GET(_Health);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Last revision the rebuild signal was broadcast for, and the provider that produced it. Lives on
    // the world's transient entity.
    struct CKNAVIGATION_API FFragment_NavSurface_RevisionWatch
    {
        CK_GENERATED_BODY(FFragment_NavSurface_RevisionWatch);

        friend class FProcessor_NavSurface_RevisionWatch;

    private:
        int64 _LastBroadcastRevision = 0;

        /** Whose counter the revision above is. Providers count their own rebuilds, so a switch moves
         *  the number without anything having been rebuilt: what changed is which surface answers, not
         *  the surface. Unset until the watch has observed a provider at all. */
        TOptional<ECk_NavSurface_Provider> _LastProvider;

    public:
        CK_PROPERTY_GET(_LastBroadcastRevision);
        CK_PROPERTY_GET(_LastProvider);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Rebuild bounds a provider has published and the watch has not broadcast yet, in publish order.
    // Lives on the world's transient entity.
    struct CKNAVIGATION_API FFragment_NavSurface_PendingRebuilds
    {
        CK_GENERATED_BODY(FFragment_NavSurface_PendingRebuilds);

        friend class FProcessor_NavSurface_RevisionWatch;
        friend auto nav_surface::Request_NotifySurfaceRebuilt(
            UWorld*     InWorld,
            const FBox& InChangedBounds) -> void;

    private:
        TArray<FBox> _Bounds;

    public:
        CK_PROPERTY_GET(_Bounds);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKNAVIGATION_API FFragment_NavSurfaceMarkup_Current
    {
        CK_GENERATED_BODY(FFragment_NavSurfaceMarkup_Current);

        friend class FProcessor_NavSurfaceMarkup_HandleRequests;
        friend class FProcessor_NavSurfaceMarkup_EndPlay;
        friend class ::UCk_Utils_NavSurface_UE;
        friend auto nav_surface_recast::Apply_AreaMarkup(
            UWorld*                                  InWorld,
            FCk_Handle&                              InMarkupEntity,
            const FCk_Request_NavSurface_AreaMarkup& InRequest) -> bool;
        friend auto nav_surface_recast::Release_AreaMarkup(
            UWorld*     InWorld,
            FCk_Handle& InMarkupEntity) -> void;

    private:
        // WEAK — lifetime owned by the CkCore ObjectPooling subsystem through UCk_Utils_NavAreaMarkup_UE.
        TWeakObjectPtr<UCk_NavAreaMarkup_UE> _Markup;

        FGameplayTag _AreaTag;
        FVector _Location = FVector::ZeroVector;
        FVector _HalfExtents = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_Markup);
        CK_PROPERTY_GET(_AreaTag);
        CK_PROPERTY_GET(_Location);
        CK_PROPERTY_GET(_HalfExtents);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKNAVIGATION_API FFragment_NavSurfaceMarkup_Requests
    {
        CK_GENERATED_BODY(FFragment_NavSurfaceMarkup_Requests);

        friend class FProcessor_NavSurfaceMarkup_HandleRequests;
        friend class ::UCk_Utils_NavSurface_UE;

        using AreaMarkupRequestType = FCk_Request_NavSurface_AreaMarkup;
        using RequestType = std::variant<AreaMarkupRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKNAVIGATION_API,
        NavSurface_OnSurfaceRebuilt,
        FCk_Delegate_NavSurface_OnSurfaceRebuilt,
        FCk_Handle,
        FBox);
}

// --------------------------------------------------------------------------------------------------------------------
