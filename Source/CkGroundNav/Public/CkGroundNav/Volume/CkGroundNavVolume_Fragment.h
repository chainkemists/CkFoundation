#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend_Jolt.h"
#include "CkGroundNav/Field/CkGroundNav_FieldBuild.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_GroundNavVolume_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_NeedsBuild);
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_BuildInProgress);
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_Built);

    // ----------------------------------------------------------------------------------------------------------------

    using FFragment_GroundNavVolume_Params = FCk_Fragment_GroundNavVolume_ParamsData;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The published field.
     *
     * Held as a shared pointer to a CONST field and swapped whole at the end of a build. A rebuild
     * assembles its own field in the build state and never touches this one, so a query holding a copy
     * of the pointer keeps reading a complete, self-consistent structure for as long as it needs it —
     * which is what makes reads safe without a lock discipline.
     *
     * The epoch bumps on every completed build, so a path planned against an older field can tell it is
     * behind and replan. Staleness is derived from that comparison and never stored as a flag.
     */
    struct CKGROUNDNAV_API FFragment_GroundNavVolume_BuiltField
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavVolume_BuiltField);

        friend class FProcessor_GroundNavVolume_Build;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    private:
        // Null until the first build completes, and never null again: a failed rebuild leaves whatever
        // was published in place, because stale ground is still ground and dropping it would strand
        // every agent standing on it.
        groundnav::FCk_GroundNav_FieldPtr _Field;

        groundnav::FCk_GroundNav_Epoch _Epoch;

    public:
        CK_PROPERTY_GET(_Field);
        CK_PROPERTY_GET(_Epoch);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Everything a build in flight needs, and nothing the finished field carries.
     *
     * The backend is the CONCRETE Jolt one rather than the interface, because only the concrete type can
     * answer whether it reached a physics world at all — and a bake that skipped that check would
     * report a world with no geometry as a world with no obstacles. It is created when a build starts and
     * dropped the moment one ends, so a pinned physics session never outlives the bake.
     *
     * The pending request carries the caller's completion delegate across the whole multi-tick build:
     * the drain that accepted the request cannot report the outcome, because the outcome is ticks away.
     */
    struct CKGROUNDNAV_API FFragment_GroundNavVolume_BuildState
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavVolume_BuildState);

        friend class FProcessor_GroundNavVolume_HandleRequests;
        friend class FProcessor_GroundNavVolume_StartBuild;
        friend class FProcessor_GroundNavVolume_Build;
        friend class FProcessor_GroundNavVolume_CancelPendingRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    private:
        groundnav::FCk_GroundNav_FieldBuildState _Build;
        TUniquePtr<groundnav::FCk_GroundNav_GeometryBackend_Jolt> _Backend;
        FCk_Request_GroundNavVolume_Build _PendingRequest;

    public:
        CK_PROPERTY_GET(_Build);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FFragment_GroundNavVolume_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavVolume_Requests);

        friend class FProcessor_GroundNavVolume_HandleRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    public:
        using RequestType = std::variant<FCk_Request_GroundNavVolume_Build>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_GroundNavVolume_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
