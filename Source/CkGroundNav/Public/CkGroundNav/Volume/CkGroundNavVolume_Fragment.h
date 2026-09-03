#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend_Jolt.h"
#include "CkGroundNav/Bake/CkGroundNav_MarkupTypes.h"
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

    /**
     * The volume's WALKABILITY markup changed: a record was admitted, updated, enabled, disabled or
     * released, and the ground it covers may now be standable where it was not or the reverse.
     *
     * Raised by the markup drain and CONSUMED by FProcessor_GroundNavVolume_MarkupWalkabilityRebuild,
     * which asks for a whole-volume rebuild through the ordinary build request. Whole-volume because
     * nothing in the build request or the field builder takes bounds; a scoped repair is a separate
     * capability neither of them has yet.
     */
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_MarkupWalkabilityDirty);

    /**
     * The volume's COST markup changed. Separate from the walkability tag because the two are
     * answered by different stages: a cost change only re-derives what a leg is priced at and
     * republishes, where a walkability change owes a re-bake. Folding them into one tag would make
     * every retint of a cost volume pay for a bake.
     *
     * Raised by the markup drain and CONSUMED by FProcessor_GroundNavVolume_MarkupCostDerive.
     */
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_MarkupCostDirty);

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
        friend class FProcessor_GroundNavVolume_MarkupCostDerive;
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

    /**
     * One markup the volume holds: the entity whose identity the record is keyed on, and the record.
     *
     * The handle is what crosses the seam — a caller names a markup entity and the volume answers for
     * that entity — while the record is the pure value the bake reduces to cells. Keeping the two side
     * by side rather than putting the handle inside the record is what keeps the record free of
     * anything that can dangle.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_MarkupEntry
    {
    public:
        CK_GENERATED_BODY(FCk_GroundNav_MarkupEntry);

        friend class FProcessor_GroundNavVolume_HandleMarkupRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    private:
        FCk_Handle _MarkupEntity;
        FCk_GroundNav_MarkupRecord _Record;

    public:
        CK_PROPERTY_GET(_MarkupEntity);
        CK_PROPERTY_GET(_Record);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Every area markup painted onto this volume.
     *
     * The volume owns the records, not the entities that requested them: an entity carries a
     * back-pointer and nothing else, so the set the bake reduces is one array in one place and cannot
     * be assembled differently by two readers.
     *
     * Ids are handed out monotonically and never reused. A released record's id is retired with it, so
     * a field baked against an older markup set can be diffed against a newer one without an id
     * meaning two different volumes at two different epochs.
     */
    struct CKGROUNDNAV_API FFragment_GroundNavVolume_Markup
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavVolume_Markup);

        friend class FProcessor_GroundNavVolume_HandleMarkupRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    private:
        TArray<FCk_GroundNav_MarkupEntry> _Entries;
        int32 _NextId = 0;

    public:
        CK_PROPERTY_GET(_Entries);
        CK_PROPERTY_GET(_NextId);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The markup entity's back-pointer to the volume holding its record.
     *
     * Composed on the MARKUP entity rather than on the volume, and carrying an id rather than a copy
     * of the record: a live probe asking what one markup currently does reads the volume's array
     * through this, so there is never a second copy of a record to drift from the first.
     */
    struct CKGROUNDNAV_API FFragment_GroundNav_MarkupRef
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNav_MarkupRef);

        friend class FProcessor_GroundNavVolume_HandleMarkupRequests;

    private:
        FCk_Handle _VolumeEntity;
        int32 _RecordId = INDEX_NONE;

    public:
        CK_PROPERTY_GET(_VolumeEntity);
        CK_PROPERTY_GET(_RecordId);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FFragment_GroundNavVolume_MarkupRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavVolume_MarkupRequests);

        friend class FProcessor_GroundNavVolume_HandleMarkupRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_GroundNavVolume_AreaMarkup,
            FCk_Request_GroundNavVolume_ReleaseAreaMarkup>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_GroundNavVolume_Requests);
    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_GroundNavVolume_MarkupRequests);
}

// --------------------------------------------------------------------------------------------------------------------
