#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend_Jolt.h"
#include "CkGroundNav/Bake/CkGroundNav_LinkTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_MarkupTypes.h"
#include "CkGroundNav/Field/CkGroundNav_FieldBuild.h"
#include "CkGroundNav/Field/CkGroundNav_FieldRepair.h"
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

    /** A dirty region is pending on the volume and no repair has opened for it yet. */
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_NeedsRepair);

    /** A repair state holds a source field and is being sliced. */
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_RepairInProgress);

    /**
     * The volume's COST markup changed.
     *
     * A tag rather than a box, unlike the WALKABILITY side, because the two are answered by different
     * stages: a cost change re-derives what a leg is priced at over every tile and republishes, where
     * a walkability change owes an actual re-bake of the ground the record covers - and that ground is
     * a REGION, so it accumulates onto _PendingDirtyBounds and raises FTag_GroundNavVolume_NeedsRepair
     * instead. Folding the two together would make every retint of a cost volume pay for a bake.
     *
     * Raised by the markup drain and CONSUMED by FProcessor_GroundNavVolume_MarkupCostDerive.
     */
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_MarkupCostDirty);

    /**
     * The volume's authored LINKS changed.
     *
     * A tag rather than a box, and for a sharper reason than the cost side: a link changes neither
     * cells nor plates. Nothing about which ground is walkable, how much room it has, or where a
     * lattice crossing lies depends on an authored link, so there is no ground to re-bake and no
     * REGION for a repair to take - only the field's own resolution of two world points to redo.
     *
     * Raised by the link drain and CONSUMED by FProcessor_GroundNavVolume_LinkDerive.
     */
    CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_LinksDirty);

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
        friend class FProcessor_GroundNavVolume_Repair;
        friend class FProcessor_GroundNavVolume_MarkupCostDerive;
        friend class FProcessor_GroundNavVolume_LinkDerive;
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

    /**
     * A local repair in flight, plus the dirty ground waiting for one.
     *
     * _PendingDirtyBounds ACCUMULATES: several bodies may dirty the same volume in one frame, and one
     * repair over their union costs less than one repair each. It is consumed - and reset - when a
     * repair opens, so a box arriving mid-repair opens the NEXT one rather than corrupting this one.
     *
     * The backend is the concrete Jolt one for the same reason the build state's is: only it can say
     * whether a physics world was reached at all, and a repair that skipped that check would read a
     * world it never touched as a world with nothing in it.
     *
     * _PendingRequests carries the callers' completion delegates until a repair opens for them, and
     * _InFlightRequests carries them across the whole multi-tick repair that did - for the same reason
     * the build state carries one: the drain that accepted them cannot report an outcome that is ticks
     * away. Lists rather than one request because dirty boxes coalesce, and their completion rides
     * whichever finishes the ground they named - the repair they opened, the next one, or a build that
     * took them over when it STARTED and published their ground (_RidingBuildRequests below).
     *
     * The two lists are SEPARATE so that a request arriving mid-repair parks against the NEXT repair
     * rather than riding one that will never look at its ground. StartRepair moves one list into the
     * other and resets the bounds in the same step, so a parked request and the box it named are never
     * separated.
     */
    struct CKGROUNDNAV_API FFragment_GroundNavVolume_RepairState
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavVolume_RepairState);

        friend class FProcessor_GroundNavVolume_HandleRequests;
        friend class FProcessor_GroundNavVolume_HandleRepairRequests;
        friend class FProcessor_GroundNavVolume_HandleMarkupRequests;
        friend class FProcessor_GroundNavVolume_StartBuild;
        friend class FProcessor_GroundNavVolume_StartRepair;
        friend class FProcessor_GroundNavVolume_Repair;
        friend class FProcessor_GroundNavVolume_Build;
        friend class FProcessor_GroundNavVolume_CancelPendingRepairRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    private:
        groundnav::FCk_GroundNav_FieldRepairState _Repair;
        TUniquePtr<groundnav::FCk_GroundNav_GeometryBackend_Jolt> _Backend;
        FBox _PendingDirtyBounds = FBox{ForceInit};
        TArray<FCk_Request_GroundNavVolume_Repair> _PendingRequests;
        TArray<FCk_Request_GroundNavVolume_Repair> _InFlightRequests;

        // Repair requests a build took over when it started: their regions are inside the ground that
        // build re-bakes from scratch, so they complete when it publishes. Kept apart from
        // _PendingRequests because a region raised AFTER the build snapshotted its records is not
        // answered by it and has to stay pending across the publish.
        TArray<FCk_Request_GroundNavVolume_Repair> _RidingBuildRequests;

        // Terminal repair failures in a row whose region was put back. The escape is BOUNDED at one
        // retry: a region that fails twice running is failing for a reason the retry does not address,
        // and re-raising it forever would open the same doomed repair every tick for the life of the
        // volume. Cleared by any repair that publishes and by any build that supersedes.
        int32 _StaleRetryCount = 0;

    public:
        CK_PROPERTY_GET(_Repair);
        CK_PROPERTY_GET(_PendingDirtyBounds);
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

    struct CKGROUNDNAV_API FFragment_GroundNavVolume_RepairRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavVolume_RepairRequests);

        friend class FProcessor_GroundNavVolume_HandleRepairRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    public:
        using RequestType = std::variant<FCk_Request_GroundNavVolume_Repair>;
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

    /**
     * One link the volume holds: the entity whose identity the record is keyed on, and the record.
     *
     * Side by side rather than the handle inside the record, for the same reason the markup entry keeps
     * them apart: the handle is what crosses a request, while the record is the pure value a
     * composition resolves, and a record carrying a handle is a value that can dangle.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_LinkEntry
    {
    public:
        CK_GENERATED_BODY(FCk_GroundNav_LinkEntry);

        friend class FProcessor_GroundNavVolume_HandleLinkRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    private:
        FCk_Handle _LinkEntity;
        FCk_GroundNav_LinkRecord _Record;

    public:
        CK_PROPERTY_GET(_LinkEntity);
        CK_PROPERTY_GET(_Record);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Every navigation link authored onto this volume.
     *
     * The volume owns the records, not the entities that requested them: an entity carries a
     * back-pointer and nothing else, so the set a composition resolves is one array in one place and
     * cannot be assembled differently by two readers.
     *
     * Ids are handed out monotonically and never reused. A released record's id is retired with it, so
     * a field resolved against an older link set can be diffed against a newer one without an id
     * meaning two different links at two different epochs.
     */
    struct CKGROUNDNAV_API FFragment_GroundNavVolume_Links
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavVolume_Links);

        friend class FProcessor_GroundNavVolume_HandleLinkRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    private:
        TArray<FCk_GroundNav_LinkEntry> _Entries;
        int32 _NextId = 0;

    public:
        CK_PROPERTY_GET(_Entries);
        CK_PROPERTY_GET(_NextId);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The link entity's back-pointer to the volume holding its record.
     *
     * Composed on the LINK entity rather than on the volume, and carrying an id rather than a copy of
     * the record: a live probe asking what one link currently does reads the volume's array through
     * this, so there is never a second copy of a record to drift from the first.
     */
    struct CKGROUNDNAV_API FFragment_GroundNav_LinkRef
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNav_LinkRef);

        friend class FProcessor_GroundNavVolume_HandleLinkRequests;

    private:
        FCk_Handle _VolumeEntity;
        int32 _RecordId = INDEX_NONE;

    public:
        CK_PROPERTY_GET(_VolumeEntity);
        CK_PROPERTY_GET(_RecordId);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FFragment_GroundNavVolume_LinkRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavVolume_LinkRequests);

        friend class FProcessor_GroundNavVolume_HandleLinkRequests;
        friend class ::UCk_Utils_GroundNavVolume_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_GroundNavVolume_Link,
            FCk_Request_GroundNavVolume_LinkBatch,
            FCk_Request_GroundNavVolume_ReleaseLink,
            FCk_Request_GroundNavVolume_ReleaseLink_ById,
            FCk_Request_GroundNavVolume_ReleaseAllLinks>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_GroundNavVolume_Requests);
    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_GroundNavVolume_RepairRequests);
    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_GroundNavVolume_MarkupRequests);
    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_GroundNavVolume_LinkRequests);
}

// --------------------------------------------------------------------------------------------------------------------
