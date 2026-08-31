#pragma once

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_NavAreaMarkup_UE;
class UCk_Utils_NavSurface_UE;

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
        ECk_NavSurface_ProviderHealth _Health = ECk_NavSurface_ProviderHealth::NoData;

    public:
        CK_PROPERTY_GET(_Provider);
        CK_PROPERTY_GET(_Health);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Last revision the rebuild signal was broadcast for. Lives on the world's transient entity.
    struct CKNAVIGATION_API FFragment_NavSurface_RevisionWatch
    {
        CK_GENERATED_BODY(FFragment_NavSurface_RevisionWatch);

        friend class FProcessor_NavSurface_RevisionWatch;

    private:
        int64 _LastBroadcastRevision = 0;

    public:
        CK_PROPERTY_GET(_LastBroadcastRevision);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKNAVIGATION_API FFragment_NavSurfaceMarkup_Current
    {
        CK_GENERATED_BODY(FFragment_NavSurfaceMarkup_Current);

        friend class FProcessor_NavSurfaceMarkup_HandleRequests;
        friend class FProcessor_NavSurfaceMarkup_EndPlay;
        friend class ::UCk_Utils_NavSurface_UE;

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
        FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------
