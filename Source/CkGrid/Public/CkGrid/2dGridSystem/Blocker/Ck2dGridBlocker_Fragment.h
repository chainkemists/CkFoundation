#pragma once

#include "Ck2dGridBlocker_Fragment_Data.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_2dGridBlocker_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_2dGridBlocker_Params = FCk_2dGridBlocker_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    // Grid-side record of the blockers currently registered on this grid. CkRecord auto-prunes a
    // dead blocker (reverse-link) when its entity is destroyed. Named blockers connect with their
    // GameplayLabel so Get_BlockerWithTag can resolve them; anonymous blockers connect Optional.
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOf_GridBlockers, FCk_Handle_2dGridBlocker);

    using RecordOf_GridBlockers_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOf_GridBlockers>;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_2dGridBlocker_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRID_API FFragment_2dGridBlocker_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridBlocker_Current);

    public:
        friend class FProcessor_2dGridBlocker_Setup;
        friend class FProcessor_2dGridBlocker_Requests;
        friend class ::UCk_Utils_2dGridBlocker_UE;

    private:
        bool _IsActive = true;
        TArray<FIntPoint> _StampedCells;

    public:
        CK_PROPERTY_GET(_IsActive);
        CK_PROPERTY_GET(_StampedCells);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRID_API FFragment_2dGridBlocker_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridBlocker_Requests);

    public:
        friend class FProcessor_2dGridBlocker_Requests;
        friend class ::UCk_Utils_2dGridBlocker_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_2dGridBlocker_SetActive
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_2dGridBlocker_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
