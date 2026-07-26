#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkVisibleRange/CkVisibleRange_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VisibleRange_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <int32 T_BucketIndex>
    class FProcessor_VisibleRange_Update_Bucket;
    class FProcessor_VisibleRange_HandleRequests;

    CK_DEFINE_ECS_TAG_COUNTED(FTag_VisibleRange_Hidden);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VisibleRange_Params = FCk_Fragment_VisibleRange_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVISIBLERANGE_API FFragment_VisibleRange_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VisibleRange_Current);

    public:
        template <int32 T_BucketIndex>
        friend class FProcessor_VisibleRange_Update_Bucket;
        friend class FProcessor_VisibleRange_HandleRequests;
        friend class ::UCk_Utils_VisibleRange_UE;

    private:
        float _Distance = 0.0f;

        float _FadeAlpha = 1.0f;

        bool _IsOutOfRange = false;
        bool _IsExplicitlyHidden = false;

    public:
        CK_PROPERTY_GET(_Distance);
        CK_PROPERTY_GET(_FadeAlpha);
        CK_PROPERTY_GET(_IsOutOfRange);
        CK_PROPERTY_GET(_IsExplicitlyHidden);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVISIBLERANGE_API FFragment_VisibleRange_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VisibleRange_Requests);

    public:
        template <int32 T_BucketIndex>
        friend class FProcessor_VisibleRange_Update_Bucket;
        friend class FProcessor_VisibleRange_HandleRequests;
        friend class ::UCk_Utils_VisibleRange_UE;

    public:
        using RequestType = std::variant<FCk_Request_VisibleRange_ApplyRangeState, FCk_Request_VisibleRange_SetVisibility>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVISIBLERANGE_API, OnVisibleRange_HiddenChanged, FCk_Delegate_VisibleRange_HiddenChanged, FCk_Handle_VisibleRange, bool);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_VisibleRange_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
