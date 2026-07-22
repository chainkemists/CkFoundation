#pragma once

#include "CkCore/Chrono/CkChrono.h"
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
    class FProcessor_VisibleRange_Update;
    class FProcessor_VisibleRange_HandleRequests;

    // Reference-counted: an entity's own out-of-range state and an explicit SetVisibility(Hide) request are two
    // INDEPENDENT votes toward "hidden" — either alone is enough, and both must clear before the entity is visible
    // again. A plain tag cannot express this (one source's clear would silently wipe the other source's still-active
    // vote) — see CkVisibleRange/Claude.md.
    CK_DEFINE_ECS_TAG_COUNTED(FTag_VisibleRange_Hidden);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VisibleRange_Params = FCk_Fragment_VisibleRange_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVISIBLERANGE_API FFragment_VisibleRange_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VisibleRange_Current);

    public:
        friend class FProcessor_VisibleRange_Update;
        friend class FProcessor_VisibleRange_HandleRequests;
        friend class ::UCk_Utils_VisibleRange_UE;

    private:
        // Last distance supplied via UCk_Utils_VisibleRange_UE::Update_Distance. Viewer resolution (which camera,
        // which local player) is entirely the caller's concern — this fragment only ever sees a plain float.
        float _Distance = 0.0f;

        // Cadence accumulator — its GoalValue IS the configured _UpdateInterval, ticked with ECk_Chrono_OverflowPolicy::Wrap.
        FCk_Chrono _CadenceChrono;

        float _FadeAlpha = 1.0f;

        // Per-source bookkeeping so each vote source adds/removes the counted tag exactly once, never leaking an
        // extra increment on a repeated identical call.
        bool _IsOutOfRange = false;
        bool _IsExplicitlyHidden = false;

    public:
        CK_PROPERTY_GET(_Distance);
        CK_PROPERTY_GET(_FadeAlpha);
        CK_PROPERTY_GET(_IsOutOfRange);
        CK_PROPERTY_GET(_IsExplicitlyHidden);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_VisibleRange_Current, _CadenceChrono);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVISIBLERANGE_API FFragment_VisibleRange_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VisibleRange_Requests);

    public:
        friend class FProcessor_VisibleRange_Update;
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
