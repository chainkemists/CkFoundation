#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend_Jolt.h"
#include "CkGroundNav/Bake/CkGroundNav_Fingerprint.h"
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
     * The published fields: the untagged default, and one per authored profile variant.
     *
     * Each is held as a shared pointer to a CONST field and swapped whole at the end of a build. A rebuild
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

        friend class FProcessor_GroundNavVolume_Setup;
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

        // One field per authored profile variant, keyed by its tag, published in the same call as the
        // default above so no publish ever leaves the two describing different worlds. Empty on a
        // volume that authored no variant, which is every volume until one does.
        //
        // Each entry keeps its OWN epoch, and _Epoch below is the NEWEST across the default and all of
        // them: a change that moves only one variant still has to be visible to a reader watching the
        // volume, and a per-field epoch is what stops the fields that did not move from claiming they
        // did. _Epoch is therefore at or past _Field->_Epoch rather than equal to it.
        TMap<FGameplayTag, groundnav::FCk_GroundNav_FieldPtr> _VariantFields;

        groundnav::FCk_GroundNav_Epoch _Epoch;

        // What the field standing here right now was PUBLISHED from: the fingerprint of the authored
        // inputs it went out with, and the world revision the geometry was at when it did. Both zero
        // until something publishes.
        //
        // EVERY PUBLISHER REFRESHES THEM - the build, the local repair and the cost and link derives -
        // because each of them puts a field out and this has to name the field that is out. A derive
        // that republished without restamping would leave the volume claiming a record list it had
        // already re-labelled itself past, which is the drift these two exist to make impossible.
        //
        // The revision is carried FORWARD by the publishers that have no backend to read one from: a
        // derive re-labels published ground and reads no geometry, so the revision its field was baked
        // against is still the right answer for it.
        //
        // The UNTAGGED DEFAULT's, on a volume that holds variants: every variant shares every one of
        // these inputs with it but the profile, and the variants themselves are one of the inputs, so
        // one identity per volume is one identity per set.
        groundnav::FCk_GroundNav_ContentFingerprint _BakedInputFingerprint;
        uint64 _BakedGeometryRevision = 0;

        // SETUP answers it - the cook is resolved there and nowhere else. A runtime build that
        // publishes over a cooked field DEMOTES it to StaleCook: the ground standing here stopped
        // being the cook's the moment that field was replaced, and only a fresh Setup reads one again.
        // The repair and the two derives carry whatever stands, for the reason they carry the geometry
        // revision forward - they re-label ground somebody else published.
        ECk_GroundNav_CookStatus _CookStatus = ECk_GroundNav_CookStatus::RuntimeOnly;

    public:
        CK_PROPERTY_GET(_Field);
        CK_PROPERTY_GET(_VariantFields);
        CK_PROPERTY_GET(_Epoch);
        CK_PROPERTY_GET(_BakedInputFingerprint);
        CK_PROPERTY_GET(_BakedGeometryRevision);
        CK_PROPERTY_GET(_CookStatus);
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

        // The profile tags this build began for, in the order their params went in. Completion keys the
        // fields it releases by this list and not by the params it can still read: _ProfileVariants is
        // writable, and a list edited mid-build would key a finished field under a tag it was never
        // baked for.
        TArray<FGameplayTag> _ProfileVariantTags;

        // The bake identity this build opened under, carried until it publishes and hands both to the
        // built field. Taken at the BEGIN, because that is what the finished field is a statement about:
        // a record admitted while the build ran is not in what it baked. Held here rather than written
        // straight onto the published field so a build that fails leaves the standing field still naming
        // the inputs it was actually produced from.
        //
        // The geometry half is the backend's WORLD REVISION. A tiled build never holds the region's
        // triangles at once - it collects one tile's halo per slice - so there is no batch to reduce, and
        // the revision is already the token that means "the static world the bake reads changed".
        groundnav::FCk_GroundNav_ContentFingerprint _BakedInputFingerprint;
        uint64 _BakedGeometryRevision = 0;

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
