#include "CkGroundNavVolume_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkGroundNav/Bake/CkGroundNav_Fingerprint.h"
#include "CkGroundNav/Bake/CkGroundNav_MarkupMask.h"
#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedFieldIndex.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedFieldLoad.h"
#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
#include "CkGroundNav/Streaming/CkGroundNav_StreamPartitionLifecycle.h"
#include "CkGroundNav/Field/CkGroundNav_FieldLinks.h"
#include "CkGroundNav/Field/CkGroundNav_FieldMarkupCost.h"
#include "CkGroundNav/Field/CkGroundNav_FieldSerialize.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Utils.h"

#include "CkNavigation/NavSurface/CkNavSurface_AreaPolicy.h"
#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_InvokerAggregation);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_HandleRepairRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_HandleMarkupRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_HandleLinkRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_StartBuild);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Build);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_StartRepair);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Repair);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_MarkupCostDerive);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_LinkDerive);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_CancelPendingRepairRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_CancelPendingMarkupRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_CancelPendingLinkRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Unpublish);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace ck_groundnav_volume_processor
    {
        // The records the volume currently holds, enabled and disabled alike: the bake decides per
        // record which of them reach a tile and whether a disabled one applies, and a caller filtering
        // either question here would be a second answer to it.
        auto Get_MarkupRecordsOf(
            TConstArrayView<FCk_GroundNav_MarkupEntry> InEntries) -> TArray<FCk_GroundNav_MarkupRecord>
        {
            return algo::Transform<TArray<FCk_GroundNav_MarkupRecord>>(InEntries,
                [](const FCk_GroundNav_MarkupEntry& InEntry) -> FCk_GroundNav_MarkupRecord
                {
                    return InEntry.Get_Record();
                });
        }

        // The links the volume currently holds, enabled and disabled alike, for the same reason the
        // markup records go out unfiltered: which of them resolve, and whether a disabled one joins
        // anything, is the composition's answer and a caller pre-filtering here would be a second one.
        auto Get_LinkRecordsOf(
            TConstArrayView<FCk_GroundNav_LinkEntry> InEntries) -> TArray<FCk_GroundNav_LinkRecord>
        {
            return algo::Transform<TArray<FCk_GroundNav_LinkRecord>>(InEntries,
                [](const FCk_GroundNav_LinkEntry& InEntry) -> FCk_GroundNav_LinkRecord
                {
                    return InEntry.Get_Record();
                });
        }

        /**
         * The identity a publish stamps onto the field it puts out: what the volume's authored inputs
         * fingerprint to, and the world revision that field was produced against.
         */
        struct FPublishedIdentity
        {
        public:
            groundnav::FCk_GroundNav_ContentFingerprint _InputFingerprint;
            uint64 _GeometryRevision = 0;
        };

        /**
         * ONE assembly of that identity, reached by all FOUR publishers - the build, the local repair
         * and the cost and link derives.
         *
         * A second way of gathering the inputs is a second answer to what the published ground was
         * produced from, and the two would part company the first time an input was added to one of
         * them. The geometry revision is a PARAMETER because only some of the four hold a backend to
         * read one from: the ones that do not re-label ground somebody else baked, and carry forward
         * the revision it was baked against.
         */
        auto Get_PublishedIdentity(
            const FCk_Handle_GroundNavVolume&       InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            uint64                                  InGeometryRevision,
            const groundnav::FCk_GroundNav_DataLayerSelector& InDataLayerSelector) -> FPublishedIdentity
        {
            // The bake layer holds no volume concepts, so the variants cross that boundary as tag names
            // beside their profiles rather than as the authored type they live in.
            const auto Variants = algo::Transform<TArray<TPair<FName, FCk_GroundNav_AgentProfile>>>(
                InParams.Get_ProfileVariants(),
                [](const FCk_GroundNav_ProfileVariant& InVariant) -> TPair<FName, FCk_GroundNav_AgentProfile>
                {
                    return TPair<FName, FCk_GroundNav_AgentProfile>{
                        InVariant.Get_ProfileTag().GetTagName(), InVariant.Get_Profile()};
                });

            const auto InputFingerprint = groundnav::Get_InputFingerprint(
                InParams.Get_VolumeBounds(),
                InParams.Get_Config(),
                InParams.Get_Profile(),
                Get_MarkupRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(InVolumeEntity)),
                Get_LinkRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_LinkEntries(InVolumeEntity)),
                InParams.Get_MergeTunables(),
                InParams.Get_MaxClearanceUu(),
                Variants,
                InDataLayerSelector);

            return FPublishedIdentity{InputFingerprint, InGeometryRevision};
        }

        auto Get_FieldParams(
            const FFragment_GroundNavVolume_Params&   InParams,
            const TArray<FCk_GroundNav_MarkupRecord>& InMarkupRecords,
            const TArray<FCk_GroundNav_LinkRecord>&   InLinkRecords)
            -> groundnav::FCk_GroundNav_FieldParams
        {
            return groundnav::Get_VolumeFieldParams(InParams, InMarkupRecords, InLinkRecords);
        }

        /**
         * Generated source blobs describe geometry/profile/config/selector only. Logical markup and
         * links are owner-level derives applied after a source is composed, so including them here
         * would falsely stale an immutable tile blob whenever those logical records change.
         * This deliberately matches FCk_GroundNav_FieldCooker's manifest fingerprint.
         */
        auto Get_ManifestSourceFingerprint(
            const FFragment_GroundNavVolume_Params& InParams,
            const groundnav::FCk_GroundNav_DataLayerSelector& InDataLayerSelector) -> uint64
        {
            auto Variants = TArray<TPair<FName, FCk_GroundNav_AgentProfile>>{};
            Variants.Reserve(InParams.Get_ProfileVariants().Num());
            for (const auto& Variant : InParams.Get_ProfileVariants())
            { Variants.Emplace(Variant.Get_ProfileTag().GetTagName(), Variant.Get_Profile()); }
            return groundnav::Get_InputFingerprint(
                InParams.Get_VolumeBounds(), InParams.Get_Config(), InParams.Get_Profile(), {}, {},
                InParams.Get_MergeTunables(), InParams.Get_MaxClearanceUu(), Variants, InDataLayerSelector)._Value;
        }

        /**
         * The params of every field this volume bakes: the untagged default FIRST, then one per
         * authored variant differing from it in nothing but the profile.
         *
         * Assembled by COPYING the default and replacing its profile, rather than built independently
         * per variant, because Request_BeginBuild_MultiProfile refuses a list that disagrees anywhere
         * else - and a second construction path is exactly how two entries drift apart on a field
         * nobody thought to keep in step.
         *
         * The default's position is load-bearing: it is the field a query carrying no profile tag is
         * answered from, and the fields come back from the build index-aligned with what went in.
         */
        auto Get_MultiProfileFieldParams(
            const FFragment_GroundNavVolume_Params&   InParams,
            const TArray<FCk_GroundNav_MarkupRecord>& InMarkupRecords,
            const TArray<FCk_GroundNav_LinkRecord>&   InLinkRecords)
            -> TArray<groundnav::FCk_GroundNav_FieldParams>
        {
            const auto DefaultParams = Get_FieldParams(InParams, InMarkupRecords, InLinkRecords);

            auto AllParams = TArray<groundnav::FCk_GroundNav_FieldParams>{};
            AllParams.Reserve(InParams.Get_ProfileVariants().Num() + 1);

            AllParams.Emplace(DefaultParams);

            for (const auto& Variant : InParams.Get_ProfileVariants())
            {
                auto VariantParams = DefaultParams;
                VariantParams._Profile = Variant.Get_Profile();

                AllParams.Emplace(MoveTemp(VariantParams));
            }

            return AllParams;
        }

        // A variant is reached ONLY by its tag, so an empty one names nothing and a repeated one names
        // two profiles at once. Both are refused where the params are judged rather than resolved to
        // some arbitrary field at query time.
        auto Get_ProfileVariantTagsAreUsable(const FFragment_GroundNavVolume_Params& InParams) -> bool
        {
            auto SeenTags = TSet<FGameplayTag>{};

            for (const auto& Variant : InParams.Get_ProfileVariants())
            {
                if (NOT Variant.Get_ProfileTag().IsValid())
                { return false; }

                auto TagWasAlreadySeen = false;
                SeenTags.Add(Variant.Get_ProfileTag(), &TagWasAlreadySeen);

                if (TagWasAlreadySeen)
                { return false; }
            }

            return true;
        }

        // A variant's PROFILE is judged by the very check the default's is, on params that differ from
        // the default's in nothing else - which is the only way the two can be held to one standard. A
        // variant admitted on the strength of the default's profile arms the volume, reaches the build,
        // and is refused there instead: a warning where a clean refusal at the params belongs.
        auto Get_ProfileVariantProfilesAreBakeable(
            const FFragment_GroundNavVolume_Params& InParams) -> bool
        {
            const auto DefaultParams = Get_FieldParams(InParams, {}, {});

            for (const auto& Variant : InParams.Get_ProfileVariants())
            {
                auto VariantParams = DefaultParams;
                VariantParams._Profile = Variant.Get_Profile();

                if (NOT VariantParams.Get_IsValid())
                { return false; }
            }

            return true;
        }

        // Markup is deliberately absent: what a volume is PAINTED with cannot make its bake settings
        // valid or invalid, and feeding the records in here would suggest it could.
        auto Get_ParamsAreBakeable(const FFragment_GroundNavVolume_Params& InParams) -> bool
        {
            const auto Bounds = InParams.Get_VolumeBounds();
            auto CanonicalSelector = groundnav::FCk_GroundNav_DataLayerSelector{};
            const auto SelectorIsValid = groundnav::TryMake_DataLayerSelector(
                InParams.Get_DataLayerSelector().Get_LayerNames(), CanonicalSelector);

            return Bounds.IsValid != 0 &&
                   Bounds.GetSize().X > 0.0 && Bounds.GetSize().Y > 0.0 && Bounds.GetSize().Z > 0.0 &&
                   Get_FieldParams(InParams, {}, {}).Get_IsValid() &&
                   Get_ProfileVariantTagsAreUsable(InParams) &&
                   Get_ProfileVariantProfilesAreBakeable(InParams) &&
                   SelectorIsValid &&
                   InParams.Get_ProbeBudgetPerTick() > 0 &&
                   (InParams.Get_StreamingBuildScope() == ECk_GroundNav_StreamingBuildScope::WholeVolume ||
                    groundnav::FCk_GroundNav_VolumeId{InParams.Get_StreamingVolumeId()}.Get_IsStreamingValid());
        }

        /**
         * Whether this volume's cook key is free in the world it lives in.
         *
         * A cooked field is keyed on {the level package it was cooked from, this key}, so two volumes
         * in one level sharing one would write over each other's tiles and read back whichever landed
         * last. NAME_None is not a key at all - it means runtime-only - so any number of volumes may
         * carry it, and every gym and test volume does.
         */
        auto Get_CookKeyIsUnique(
            const FCk_Handle_GroundNavVolume&       InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams) -> bool
        {
            const auto CookKey = InParams.Get_CookKey();

            if (CookKey.IsNone())
            { return true; }

            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);
            const auto SourceLevelPackage = InParams.Get_CookLevelPackage().IsNone()
                ? groundnav::Get_LevelPackageKey(World)
                : groundnav::Get_PackageLookupKey(InParams.Get_CookLevelPackage().ToString());
            const auto Self = InVolumeEntity.ConvertToHandle();

            for (auto OtherEntity : groundnav::world_fields::Get_VolumeEntities(World))
            {
                if (OtherEntity == Self)
                { continue; }

                if (NOT OtherEntity.Has<FFragment_GroundNavVolume_Params>())
                { continue; }

                const auto& OtherParams = OtherEntity.Get<FFragment_GroundNavVolume_Params>();
                const auto OtherSourceLevelPackage = OtherParams.Get_CookLevelPackage().IsNone()
                    ? groundnav::Get_LevelPackageKey(World)
                    : groundnav::Get_PackageLookupKey(OtherParams.Get_CookLevelPackage().ToString());

                if (OtherParams.Get_CookKey() == CookKey && OtherSourceLevelPackage == SourceLevelPackage)
                { return false; }
            }

            return true;
        }

        /**
         * What the cook had for this volume, and — when it had a usable field — everything a publish of
         * that field needs.
         *
         * One resolution rather than a status followed by a second call that loads the field, because
         * the two cannot be allowed to disagree: the status IS the outcome of the load, and asking
         * twice would be asking about two reads of an asset that can move between them.
         */
        struct FCookResolution
        {
        public:
            ECk_GroundNav_CookStatus _Status = ECk_GroundNav_CookStatus::RuntimeOnly;

            // Meaningful only under Cooked. Left as the empty field every other status describes.
            groundnav::FCk_GroundNav_Field _Field;
            TMap<FGameplayTag, groundnav::FCk_GroundNav_Field> _VariantFields;

            groundnav::FCk_GroundNav_ContentFingerprint _InputFingerprint;
            uint64 _GeometryRevision = 0;
        };

        /**
         * The state-selected cooked/runtime answer, in the order the states rule out one another.
         *
         * No key is RUNTIME-ONLY and nothing is looked up. A key with no index at the convention path is
         * MissingCook — absence is legal, a level opts into cooked ground. An index that cannot be used
         * is StaleCook. Only a load that held is Cooked, and only then is there a field to publish.
         *
         * The fingerprint compared against the index is assembled by the same helper every PUBLISH uses,
         * with the geometry half deliberately left out: what the index carries is the INPUT half, and a
         * cook cannot record a world revision yet.
         *
         * The revision stamped on a cooked publish is the backend's CURRENT one, or zero where no
         * physics world can be reached. Zero is honest rather than convenient — it is the revision an
         * unpublished volume already carries, so a cooked field in a world with no backend reads
         * not-build-current, which is what a field nothing can re-read against deserves.
         */
        auto Get_CookResolution(
            const FCk_Handle_GroundNavVolume&       InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams) -> FCookResolution
        {
            auto Resolution = FCookResolution{};

            if (InParams.Get_CookKey().IsNone())
            { return Resolution; }

            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);
            const auto SourceLevelPackage = InParams.Get_CookLevelPackage().IsNone()
                ? groundnav::Get_LevelPackageKey(World)
                : groundnav::Get_PackageLookupKey(InParams.Get_CookLevelPackage().ToString());

            constexpr auto NoGeometryRevision = uint64{0};

            auto DataLayerSelector = groundnav::FCk_GroundNav_DataLayerSelector{};
            if (NOT groundnav::TryMake_DataLayerSelector(
                InParams.Get_DataLayerSelector().Get_LayerNames(), DataLayerSelector))
            { return Resolution; }

            const auto CurrentIdentity = Get_PublishedIdentity(
                InVolumeEntity, InParams, NoGeometryRevision, DataLayerSelector);

            const auto MarkupRecords =
                Get_MarkupRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(InVolumeEntity));
            const auto LinkRecords =
                Get_LinkRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_LinkEntries(InVolumeEntity));

            const auto DefaultParams = Get_FieldParams(InParams, MarkupRecords, LinkRecords);
            const auto TryLoadProfile = [&](FGameplayTag InProfileTag, const groundnav::FCk_GroundNav_FieldParams& InFieldParams,
                                            groundnav::FCk_GroundNav_Field& OutField) -> bool
            {
                const auto* CookedIndex = groundnav::Find_CookedFieldIndex(
                    World, InParams.Get_CookKey(), InProfileTag, SourceLevelPackage,
                    DataLayerSelector);

                if (ck::Is_NOT_Valid(CookedIndex))
                {
                    Resolution._Status = ECk_GroundNav_CookStatus::MissingCook;
                    return false;
                }

                Resolution._Status = groundnav::Try_LoadCookedField(
                    *CookedIndex, SourceLevelPackage, InParams.Get_CookKey(), InFieldParams,
                    CurrentIdentity._InputFingerprint._Value, OutField, InProfileTag,
                    InParams.Get_StreamingVolumeId(), DataLayerSelector);
                return Resolution._Status == ECk_GroundNav_CookStatus::Cooked;
            };

            if (NOT TryLoadProfile({}, DefaultParams, Resolution._Field))
            { return Resolution; }

            for (const auto& Variant : InParams.Get_ProfileVariants())
            {
                auto VariantParams = DefaultParams;
                VariantParams._Profile = Variant.Get_Profile();
                auto VariantField = groundnav::FCk_GroundNav_Field{};

                if (NOT TryLoadProfile(Variant.Get_ProfileTag(), VariantParams, VariantField))
                {
                    Resolution._Field = {};
                    Resolution._VariantFields.Reset();
                    return Resolution;
                }

                Resolution._VariantFields.Emplace(Variant.Get_ProfileTag(), MoveTemp(VariantField));
            }

            // The identity was computed from the complete current volume input and compared by every
            // successful load. It names the default and every profile variant as one atomic publish.
            Resolution._InputFingerprint = CurrentIdentity._InputFingerprint;

            const auto Backend = groundnav::FCk_GroundNav_GeometryBackend_Jolt{World};

            Resolution._GeometryRevision = Backend.Get_IsValid()
                ? Backend.Get_WorldRevision()
                : NoGeometryRevision;

            return Resolution;
        }

        auto Get_MarkupKind(
            ECk_NavSurface_AreaPolicyKind InPolicyKind) -> ECk_GroundNav_MarkupKind
        {
            return InPolicyKind == ECk_NavSurface_AreaPolicyKind::Walkability
                ? ECk_GroundNav_MarkupKind::Walkability
                : ECk_GroundNav_MarkupKind::Cost;
        }

        // A box that bounds nothing must not be unioned in: FBox::operator+ takes the union with a box
        // at the origin, which would drag every dirty region back to the world centre.
        auto Get_UnionedBounds(
            const FBox& InAccumulated,
            const FBox& InAdded) -> FBox
        {
            if (InAdded.IsValid == 0)
            { return InAccumulated; }

            return InAccumulated.IsValid != 0 ? InAccumulated + InAdded : InAdded;
        }

        auto Get_MarkupEntryIndex(
            const FFragment_GroundNavVolume_Markup& InMarkup,
            const FCk_Handle&                       InMarkupEntity) -> int32
        {
            return algo::FindIndex(InMarkup.Get_Entries(),
                [&](const FCk_GroundNav_MarkupEntry& InEntry) -> bool
                {
                    return InEntry.Get_MarkupEntity() == InMarkupEntity;
                });
        }

        auto Get_LinkEntryIndex(
            const FFragment_GroundNavVolume_Links& InLinks,
            const FCk_Handle&                      InLinkEntity) -> int32
        {
            return algo::FindIndex(InLinks.Get_Entries(),
                [&](const FCk_GroundNav_LinkEntry& InEntry) -> bool
                {
                    return InEntry.Get_LinkEntity() == InLinkEntity;
                });
        }

        auto Get_LinkEntryIndexById(
            const FFragment_GroundNavVolume_Links& InLinks,
            int32                                  InRecordId) -> int32
        {
            return algo::FindIndex(InLinks.Get_Entries(),
                [&](const FCk_GroundNav_LinkEntry& InEntry) -> bool
                {
                    return InEntry.Get_Record().Get_Id() == InRecordId;
                });
        }

        auto Get_IsFinite(const FVector& InPoint) -> bool
        {
            return FMath::IsFinite(InPoint.X) && FMath::IsFinite(InPoint.Y) && FMath::IsFinite(InPoint.Z);
        }

        // The ONE place a link record is judged, and the whole of it: the sentence saying why the
        // record cannot be admitted, or nothing when it can. A single request states that sentence as
        // its own refusal and a batch names it beside the entry index that hit it, so the two ask the
        // same six questions in the same order and neither can drift from the other.
        //
        // A judgement about a RECORD only: nothing here bakes, resolves an endpoint or reads a cell.
        auto TryGet_LinkAdmissionRefusal(
            const FCk_Handle_GroundNavVolume&       InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FCk_Request_GroundNavVolume_Link& InRequest) -> TOptional<FString>
        {
            auto LinkEntity = InRequest.Get_LinkEntity();

            if (NOT ck::IsValid(LinkEntity))
            {
                return ck::Format_UE(
                    TEXT("Cannot link GroundNav Volume [{}] - the request names an invalid link Entity [{}], "
                         "which is the identity its record would be keyed on"),
                    InVolumeEntity, LinkEntity);
            }

            const auto& Requested = InRequest.Get_Record();

            const auto Start = Requested.Get_Start();
            const auto End = Requested.Get_End();

            // Two points that are not two points describe no traversal. A zero span would price at zero
            // however high the multiplier, and a non-finite one would project onto whatever cell the
            // arithmetic happened to land in.
            const auto EndpointsAreTwoFinitePoints =
                Get_IsFinite(Start) && Get_IsFinite(End) && NOT Start.Equals(End);

            if (NOT EndpointsAreTwoFinitePoints)
            {
                return ck::Format_UE(
                    TEXT("Cannot link GroundNav Volume [{}] from link Entity [{}] - its endpoints [{}] and [{}] "
                         "are not two distinct finite points"),
                    InVolumeEntity, LinkEntity, Start, End);
            }

            // A link is VOLUME-SCOPED: the field it resolves against covers this volume's ground and no
            // other, so an endpoint outside those bounds names ground this volume can never answer for.
            // Joining two volumes needs something that composes two fields, and nothing does yet.
            const auto VolumeBounds = InParams.Get_VolumeBounds();

            const auto BothEndpointsAreInTheVolume =
                VolumeBounds.IsInsideOrOn(Start) && VolumeBounds.IsInsideOrOn(End);

            if (NOT BothEndpointsAreInTheVolume)
            {
                return ck::Format_UE(
                    TEXT("Cannot link GroundNav Volume [{}] from link Entity [{}] - its endpoints [{}] and [{}] "
                         "are not both inside the volume's bounds [{}]. A link is volume-scoped"),
                    InVolumeEntity, LinkEntity, Start, End, VolumeBounds);
            }

            // At or above one, both ways. The multipliers price the link's own straight-line span, so a
            // factor below one would make an edge cost less than its Euclidean length and the search's
            // Euclidean heuristic would stop being admissible - it would return cheap paths that are not
            // the cheapest, and no consumer could tell.
            const auto MultipliersPriceAtLeastTheSpan =
                Requested.Get_CostMultiplierForward() >= 1.0f &&
                Requested.Get_CostMultiplierBackward() >= 1.0f;

            if (NOT MultipliersPriceAtLeastTheSpan)
            {
                return ck::Format_UE(
                    TEXT("Cannot link GroundNav Volume [{}] from link Entity [{}] - its cost multipliers "
                         "[{}] forward and [{}] backward must both be at least 1.0, or an edge would cost less "
                         "than its own length and the search heuristic would stop being admissible"),
                    InVolumeEntity, LinkEntity,
                    Requested.Get_CostMultiplierForward(), Requested.Get_CostMultiplierBackward());
            }

            // A link that admits nothing is a link nothing may use, which is a disabled link written in a
            // way no reader can see. Narrowing it for a ladder or a crawl is what the number is for.
            const auto ClearanceAdmitsSomething = Requested.Get_ClearanceUu() > 0.0f;

            if (NOT ClearanceAdmitsSomething)
            {
                return ck::Format_UE(
                    TEXT("Cannot link GroundNav Volume [{}] from link Entity [{}] - its clearance [{}]uu admits "
                         "no agent at all, which is a disabled link nothing downstream can read as one"),
                    InVolumeEntity, LinkEntity, Requested.Get_ClearanceUu());
            }

            // UNSET is admitted, unlike a markup's: a markup exists to say what ground MEANS and one with
            // no tag decides nothing, where a link's traversal stands on its own and a tag on it is extra.
            // A tag that IS carried goes through the same registry, because this module never decides what
            // a tag means.
            const auto AreaTagIsUsable = NOT Requested.Get_AreaTag().IsValid() ||
                                         nav_surface::TryGet_AreaPolicy(Requested.Get_AreaTag()).IsSet();

            if (NOT AreaTagIsUsable)
            {
                return ck::Format_UE(
                    TEXT("Cannot link GroundNav Volume [{}] from link Entity [{}] with area tag [{}] - nothing "
                         "published what that tag MEANS, and a record carrying it would resolve into a link "
                         "nothing knows how to apply"),
                    InVolumeEntity, LinkEntity, Requested.Get_AreaTag());
            }

            return {};
        }

        /**
         * Publishes the volume's complete field set through the identity selected by its authored
         * streaming id. Legacy volumes retain the handle-keyed registry exactly as before. A streamed
         * owner is registered only from a complete first field, then every later whole-field publish
         * refreshes its reserved bootstrap source. The registry owns the masked query publication;
         * this volume retains its complete producer bundle, including tiles a disabled source masks.
         */
        auto Publish_WholeVolume(
            UWorld*                                             InWorld,
            const FCk_Handle_GroundNavVolume&                   InVolumeEntity,
            const FFragment_GroundNavVolume_Params&             InParams,
            groundnav::FCk_GroundNav_FieldPtr&                  InOutField,
            TMap<FGameplayTag, groundnav::FCk_GroundNav_FieldPtr>& InOutVariantFields,
            groundnav::FCk_GroundNav_Epoch&                     InOutEpoch,
            bool&                                                InOutStreamOwnerRegistered,
            const groundnav::world_fields::FCk_GroundNav_PublishClaim& InClaim) -> bool
        {
            const auto VolumeId = groundnav::FCk_GroundNav_VolumeId{InParams.Get_StreamingVolumeId()};

            if (VolumeId.Get_IsLegacy())
            {
                groundnav::world_fields::Publish(
                    InWorld, InVolumeEntity, InOutField, InOutVariantFields, InClaim);
                return true;
            }

            auto CompleteBundleIsPresent = InOutField.IsValid();
            for (const auto& Variant : InOutVariantFields)
            { CompleteBundleIsPresent = CompleteBundleIsPresent && Variant.Key.IsValid() && Variant.Value.IsValid(); }
            CK_ENSURE_IF_NOT(CompleteBundleIsPresent,
                TEXT("GroundNav streamed Volume [{}] cannot publish an incomplete all-profile bundle"),
                InVolumeEntity)
            {}
            if (NOT CompleteBundleIsPresent)
            { return false; }

            auto Bundle = groundnav::FCk_GroundNav_StreamFieldBundle{};
            Bundle._DefaultField = *InOutField;
            for (const auto& Variant : InOutVariantFields)
            { Bundle._VariantFields.Emplace(Variant.Key, *Variant.Value); }
            const auto SubmittedEpoch = InOutEpoch;

            const auto Result = InOutStreamOwnerRegistered
                ? groundnav::world_fields::Refresh_StreamOwnerBundle(InWorld, VolumeId, Bundle, InClaim)
                : groundnav::world_fields::Register_StreamOwner(InWorld, InVolumeEntity, VolumeId, Bundle);

            const auto StreamPublishSucceeded = Result.Get_Succeeded();
            CK_ENSURE_IF_NOT(StreamPublishSucceeded,
                TEXT("GroundNav streamed Volume [{}] could not publish its all-profile bundle: registry status [{}]"),
                InVolumeEntity, static_cast<int32>(Result._Status))
            {}
            if (NOT StreamPublishSucceeded)
            { return false; }

            const auto Snapshot = groundnav::world_fields::TryGet_StreamOwnerSnapshot(InWorld, VolumeId);
            auto SnapshotIsComplete = Snapshot.IsSet() && Snapshot->_DefaultField.IsValid() &&
                Snapshot->_VariantFields.Num() == InOutVariantFields.Num();
            if (Snapshot.IsSet())
            {
                for (const auto& Variant : InOutVariantFields)
                {
                    const auto* PublishedVariant = Snapshot->_VariantFields.Find(Variant.Key);
                    SnapshotIsComplete = SnapshotIsComplete && PublishedVariant != nullptr && PublishedVariant->IsValid();
                }
            }
            CK_ENSURE_IF_NOT(SnapshotIsComplete,
                TEXT("GroundNav streamed Volume [{}] published without a complete owner snapshot"), InVolumeEntity)
            {}
            if (NOT SnapshotIsComplete)
            { return false; }

            // Disable/Unload masks tiles from the registry's query snapshot while retaining their
            // blobs in the owner. Do not feed that masked answer back into the volume: its next
            // complete derive must still be able to reserialize every retained source tile. The
            // successful transaction advanced the owner to Result._Epoch, so mirror that field epoch
            // in the producer bundle too. Preserve each field's changed-tile precision by remapping
            // only tiles carrying this submitted publish epoch: a local repair must still name only
            // the tiles it rebuilt, rather than its entire lattice, after the registry transaction.
            Bundle._DefaultField._Epoch = Result._Epoch;
            for (auto& Tile : Bundle._DefaultField._Tiles)
            {
                if (Tile._Epoch == SubmittedEpoch)
                { Tile._Epoch = Result._Epoch; }
            }
            for (auto& Variant : Bundle._VariantFields)
            {
                Variant.Value._Epoch = Result._Epoch;
                for (auto& Tile : Variant.Value._Tiles)
                {
                    if (Tile._Epoch == SubmittedEpoch)
                    { Tile._Epoch = Result._Epoch; }
                }
            }

            InOutField = MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(Bundle._DefaultField));
            InOutVariantFields.Reset();
            for (auto& Variant : Bundle._VariantFields)
            {
                InOutVariantFields.Emplace(
                    Variant.Key, MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(Variant.Value)));
            }
            InOutEpoch = Result._Epoch;
            InOutStreamOwnerRegistered = true;
            return true;
        }

        /** Make the invoker mode's declared lattice without touching physics or marking a tile built. */
        auto Get_EmptyStreamBundle(
            const FFragment_GroundNavVolume_Params& InParams,
            const TArray<FCk_GroundNav_MarkupRecord>& InMarkupRecords,
            const TArray<FCk_GroundNav_LinkRecord>& InLinkRecords) -> groundnav::FCk_GroundNav_StreamFieldBundle
        {
            auto Bundle = groundnav::FCk_GroundNav_StreamFieldBundle{};
            const auto Params = Get_MultiProfileFieldParams(InParams, InMarkupRecords, InLinkRecords);

            const auto MakeField = [](const groundnav::FCk_GroundNav_FieldParams& FieldParams) -> groundnav::FCk_GroundNav_Field
            {
                auto Field = groundnav::FCk_GroundNav_Field{};
                Field._Params = FieldParams;
                Field._Tiles.SetNum(FieldParams.Get_TileCount());
                for (auto TileIndex = 0; TileIndex < Field._Tiles.Num(); ++TileIndex)
                { Field._Tiles[TileIndex]._Coord = groundnav::Get_TileCoord(FieldParams._Divisions, TileIndex); }
                groundnav::DoDerive_SeamPortals(Field);
                groundnav::DoResolve_Links(Field);
                groundnav::DoLabel_Reachability(Field);
                return Field;
            };

            Bundle._DefaultField = MakeField(Params[0]);
            for (auto ProfileIndex = 0; ProfileIndex < InParams.Get_ProfileVariants().Num(); ++ProfileIndex)
            { Bundle._VariantFields.Emplace(InParams.Get_ProfileVariants()[ProfileIndex].Get_ProfileTag(), MakeField(Params[ProfileIndex + 1])); }
            return Bundle;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuiltField& InBuiltField)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        const auto ParamsAreBakeable = Get_ParamsAreBakeable(InParams);

        // The volume stays inert rather than arming a build that would bake nonsense. A later
        // Request_Build on it fails loudly through the same validation.
        CK_ENSURE_IF_NOT(ParamsAreBakeable,
            TEXT("GroundNav Volume [{}] cannot be baked. Bounds [{}], cell size [{}]uu, tile size [{}]uu, "
                 "clearance ceiling [{}]uu, probe budget [{}]"),
            InVolumeEntity, InParams.Get_VolumeBounds(), InParams.Get_Config().Get_CellSizeUu(),
            InParams.Get_Config().Get_TileSizeUu(), InParams.Get_MaxClearanceUu(),
            InParams.Get_ProbeBudgetPerTick())
        {}

        if (NOT ParamsAreBakeable)
        { return; }

        // Setup establishes the selector identity even for an invoker-driven empty lattice. Its first
        // automatic targeted build must collect the authored subset rather than mistaking the default
        // constructed empty selector for an intentional collect-all override.
        auto DataLayerSelector = groundnav::FCk_GroundNav_DataLayerSelector{};
        const auto SelectorWasCanonicalized = groundnav::TryMake_DataLayerSelector(
            InParams.Get_DataLayerSelector().Get_LayerNames(), DataLayerSelector);
        if (NOT SelectorWasCanonicalized)
        { return; }
        InBuiltField._DataLayerSelector = DataLayerSelector;

        // StartBuild and Build carry the same fragment for legacy and invoker modes; legacy simply
        // leaves it dormant. Composing it here prevents the scheduler from excluding the existing
        // whole-volume path while keeping stream handles world-local runtime state.
        InVolumeEntity.AddOrGet<FFragment_GroundNavVolume_InvokerState>();

        const auto CookKeyIsUnique = Get_CookKeyIsUnique(InVolumeEntity, InParams);

        // Judged BEFORE the registry insert below: a volume admitted under a key another volume in
        // this world already answers to would leave the two of them writing and reading one cooked
        // field, and nothing downstream could tell which of them the tiles came from.
        CK_ENSURE_IF_NOT(CookKeyIsUnique,
            TEXT("GroundNav Volume [{}] cannot be admitted: another volume in this world already "
                 "carries the cook key [{}]"),
            InVolumeEntity, InParams.Get_CookKey())
        {}

        if (NOT CookKeyIsUnique)
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        // A manifest-driven owner publishes only its canonical all-profile lattice here. Runtime-cell
        // manifests own every source handle and coordinate, so setup must not create the bootstrap
        // source used by whole-volume and invoker-driven producers or resolve a legacy full-field cook.
        if (InParams.Get_StreamingBuildScope() == ECk_GroundNav_StreamingBuildScope::ManifestDriven)
        {
            const auto VolumeId = groundnav::FCk_GroundNav_VolumeId{InParams.Get_StreamingVolumeId()};
            const auto MarkupRecords = Get_MarkupRecordsOf(
                UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(InVolumeEntity));
            const auto LinkRecords = Get_LinkRecordsOf(
                UCk_Utils_GroundNavVolume_UE::Get_LinkEntries(InVolumeEntity));
            auto Bundle = Get_EmptyStreamBundle(InParams, MarkupRecords, LinkRecords);
            const auto ManifestFingerprint = Get_ManifestSourceFingerprint(InParams, DataLayerSelector);
            auto VariantFingerprints = TMap<FGameplayTag, uint64>{};
            for (const auto& Variant : InParams.Get_ProfileVariants())
            { VariantFingerprints.Add(Variant.Get_ProfileTag(), ManifestFingerprint); }
            const auto SourceLevelPackage = InParams.Get_CookLevelPackage().IsNone()
                ? groundnav::Get_LevelPackageKey(World)
                : groundnav::Get_PackageLookupKey(InParams.Get_CookLevelPackage().ToString());
            const auto Registration = groundnav::stream_partitions::Register_ManifestOwner(
                World, InVolumeEntity, VolumeId, Bundle, ManifestFingerprint,
                VariantFingerprints, DataLayerSelector, SourceLevelPackage, InParams.Get_CookKey());
            const auto RegistrationSucceeded = Registration.Get_Succeeded() &&
                NOT Registration._Registry._Source.Get_IsValid() && Registration._OwnerInstance != 0;
            CK_ENSURE_IF_NOT(RegistrationSucceeded,
                TEXT("GroundNav manifest-driven Volume [{}] could not register its source-free canonical lattice: "
                     "registry status [{}]"),
                InVolumeEntity, static_cast<int32>(Registration._Status))
            {}
            if (NOT RegistrationSucceeded)
            { return; }

            Bundle._DefaultField._Epoch = Registration._Registry._Epoch;
            for (auto& Variant : Bundle._VariantFields)
            { Variant.Value._Epoch = Registration._Registry._Epoch; }
            InBuiltField._Field = MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(Bundle._DefaultField));
            InBuiltField._VariantFields.Reset();
            for (auto& Variant : Bundle._VariantFields)
            {
                InBuiltField._VariantFields.Emplace(
                    Variant.Key, MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(Variant.Value)));
            }
            InBuiltField._Epoch = Registration._Registry._Epoch;
            InBuiltField.Set_StreamOwnerRegistered(true);
            InBuiltField.Set_ManifestOwnerInstance(Registration._OwnerInstance);
            InBuiltField._CookStatus = ECk_GroundNav_CookStatus::RuntimeOnly;
            return;
        }

        auto CookResolution = Get_CookResolution(InVolumeEntity, InParams);

        // Stamped whatever the outcome. Three of the four values are the volume saying why the ground
        // it is about to bake is not coming from a cook, which is the half of the question a caller
        // asking "did this level ship cooked ground" actually needs answered.
        InBuiltField._CookStatus = CookResolution._Status;

        if (CookResolution._Status == ECk_GroundNav_CookStatus::Cooked)
        {
            // The epoch comes from what is PUBLISHED, exactly as a build takes it, so a reader
            // comparing epochs against a cooked volume is comparing the same counter it would against
            // a baked one.
            CookResolution._Field._Epoch = InBuiltField.Get_Epoch().Get_Next();

            // The loaded tiles carry the COOK's epochs, which count a run of publishes this volume
            // never made. Every one of them is restamped to the epoch going out - exactly what a bake
            // does to every tile it builds - so the changed-bounds below name the whole loaded field
            // rather than the empty set an epoch nothing carries would answer with.
            for (auto& LoadedTile : CookResolution._Field._Tiles)
            { LoadedTile._Epoch = CookResolution._Field._Epoch; }

            for (auto& VariantField : CookResolution._VariantFields)
            {
                VariantField.Value._Epoch = CookResolution._Field._Epoch;

                for (auto& LoadedTile : VariantField.Value._Tiles)
                { LoadedTile._Epoch = VariantField.Value._Epoch; }
            }

            auto PublishedEpoch = CookResolution._Field._Epoch;
            groundnav::FCk_GroundNav_FieldPtr PublishedField =
                MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(CookResolution._Field));

            auto PublishedVariantFields = TMap<FGameplayTag, groundnav::FCk_GroundNav_FieldPtr>{};

            for (auto& VariantField : CookResolution._VariantFields)
            {
                PublishedVariantFields.Emplace(
                    VariantField.Key,
                    MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(VariantField.Value)));
            }

            // The revision the world had when the cook was LOADED, not when it was baked: the cooker
            // records none. So Get_IsBuildCurrent reads current on a cooked volume until the world
            // moves after the load, which is the most a stored revision can honestly claim about
            // geometry nobody wrote down. Zero where no backend could be made, exactly as an
            // unpublished volume already reads.
            auto StreamOwnerRegistered = InBuiltField.Get_IsStreamOwnerRegistered();
            if (NOT Publish_WholeVolume(
                World, InVolumeEntity, InParams, PublishedField, PublishedVariantFields, PublishedEpoch,
                StreamOwnerRegistered,
                groundnav::world_fields::FCk_GroundNav_PublishClaim::Geometry()))
            { return; }

            InBuiltField._Epoch = PublishedEpoch;
            InBuiltField._Field = MoveTemp(PublishedField);
            InBuiltField._VariantFields = MoveTemp(PublishedVariantFields);
            InBuiltField._BakedInputFingerprint = CookResolution._InputFingerprint;
            InBuiltField._BakedGeometryRevision = CookResolution._GeometryRevision;
            InBuiltField._DataLayerSelector = DataLayerSelector;
            InBuiltField.Set_StreamOwnerRegistered(StreamOwnerRegistered);

            // The union of every tile the cook held, because the restamp above put this epoch on all
            // of them: a cooked publish is a fresh publish of everything, and that is the honest
            // payload for one. An invalid box goes out AS IS where the cook held no built tile, on the
            // same terms a build's publish sends one.
            auto ChangedBounds = groundnav::Get_ChangedTileBounds(*InBuiltField._Field, InBuiltField._Epoch);

            for (const auto& VariantField : InBuiltField._VariantFields)
            {
                ChangedBounds = Get_UnionedBounds(
                    ChangedBounds, groundnav::Get_ChangedTileBounds(*VariantField.Value, InBuiltField._Epoch));
            }

            nav_surface::Request_NotifySurfaceRebuilt(World, ChangedBounds);

            InVolumeEntity.AddOrGet<FTag_GroundNavVolume_Built>();

            // _AutoBuildOnSetup is moot here. It says whether the volume bakes itself without being
            // asked, and this volume's ground is already published.
            return;
        }

        // Invoker scope owns one canonical, all-profile unbuilt lattice from setup onward. It does not
        // touch Jolt or arm a whole-volume bake: the aggregation pass is the only authority that turns
        // a world-local invoker union into targeted work.
        if (InParams.Get_StreamingBuildScope() == ECk_GroundNav_StreamingBuildScope::InvokerDriven)
        {
            const auto VolumeId = groundnav::FCk_GroundNav_VolumeId{InParams.Get_StreamingVolumeId()};
            const auto StreamIdIsUsable = VolumeId.Get_IsStreamingValid();
            CK_ENSURE_IF_NOT(StreamIdIsUsable,
                TEXT("GroundNav invoker-driven Volume [{}] requires a positive streaming volume id"), InVolumeEntity)
            {}
            if (NOT StreamIdIsUsable)
            { return; }

            const auto MarkupRecords = Get_MarkupRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(InVolumeEntity));
            const auto LinkRecords = Get_LinkRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_LinkEntries(InVolumeEntity));
            auto Bundle = Get_EmptyStreamBundle(InParams, MarkupRecords, LinkRecords);
            const auto Registration = groundnav::world_fields::Register_StreamOwner(
                World, InVolumeEntity, VolumeId, Bundle);
            const auto RegistrationSucceeded = Registration.Get_Succeeded() && Registration._Source.Get_IsValid();
            CK_ENSURE_IF_NOT(RegistrationSucceeded,
                TEXT("GroundNav invoker-driven Volume [{}] could not register its canonical lattice: registry status [{}]"),
                InVolumeEntity, static_cast<int32>(Registration._Status))
            {}
            if (NOT RegistrationSucceeded)
            { return; }

            Bundle._DefaultField._Epoch = Registration._Epoch;
            for (auto& Variant : Bundle._VariantFields)
            { Variant.Value._Epoch = Registration._Epoch; }
            InBuiltField._Field = MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(Bundle._DefaultField));
            InBuiltField._VariantFields.Reset();
            for (auto& Variant : Bundle._VariantFields)
            { InBuiltField._VariantFields.Emplace(Variant.Key, MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(Variant.Value))); }
            InBuiltField._Epoch = Registration._Epoch;
            InBuiltField.Set_StreamOwnerRegistered(true);

            auto& InvokerState = InVolumeEntity.AddOrGet<FFragment_GroundNavVolume_InvokerState>();
            InvokerState.Set_Source(Registration._Source);
            return;
        }

        // Legacy volumes retain their early empty entry for provider discovery. A streaming owner is
        // intentionally absent until it has a complete bundle for its reserved bootstrap source.
        if (groundnav::FCk_GroundNav_VolumeId{InParams.Get_StreamingVolumeId()}.Get_IsLegacy())
        {
            groundnav::world_fields::Publish(
                World, InVolumeEntity, {}, {},
                groundnav::world_fields::FCk_GroundNav_PublishClaim::Geometry());
        }

        if (InParams.Get_AutoBuildOnSetup() == ECk_EnableDisable::Disable)
        { return; }

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsBuild>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_InvokerAggregation::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_InvokerState& InInvokerState)
        -> void
    {
        if (InParams.Get_StreamingBuildScope() != ECk_GroundNav_StreamingBuildScope::InvokerDriven)
        { return; }

        const auto Producer = InBuiltField.Get_Field();
        const auto ProducerIsValid = Producer.IsValid();
        CK_ENSURE_IF_NOT(ProducerIsValid,
            TEXT("GroundNav invoker-driven Volume [{}] has no retained producer field"), InVolumeEntity)
        {}
        if (NOT ProducerIsValid)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);
        if (World == nullptr)
        { return; }

        auto Points = TArray<groundnav::FCk_GroundNav_BuildInvokerPoint>{};
        this->_TransientEntity.View<groundnav::FFragment_GroundNav_BuildInvoker, FFragment_Transform,
            CK_IGNORE_PENDING_KILL>().ForEach(
            [&](FCk_Entity InEntity, const groundnav::FFragment_GroundNav_BuildInvoker& InInvoker,
                const FFragment_Transform& InTransform) -> void
            {
                const auto Handle = ck::MakeHandle(InEntity, this->_TransientEntity);
                if (UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(Handle) != World)
                { return; }
                Points.Emplace(groundnav::FCk_GroundNav_BuildInvokerPoint{
                    InTransform.Get_Transform().GetLocation(), InInvoker.Get_InnerGenerationRadiusUu(),
                    InInvoker.Get_OuterRemovalRadiusUu()});
            });

        auto Boxes = TArray<groundnav::FCk_GroundNav_BuildInvokerBox>{};
        this->_TransientEntity.View<groundnav::FFragment_GroundNav_BuildInvokerVolume,
            CK_IGNORE_PENDING_KILL>().ForEach(
            [&](FCk_Entity InEntity, const groundnav::FFragment_GroundNav_BuildInvokerVolume& InInvoker) -> void
            {
                const auto Handle = ck::MakeHandle(InEntity, this->_TransientEntity);
                if (UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(Handle) != World)
                { return; }
                Boxes.Emplace(groundnav::FCk_GroundNav_BuildInvokerBox{
                    InInvoker.Get_InnerBounds(), InInvoker.Get_OuterPaddingUu()});
            });

        auto Selection = groundnav::FCk_GroundNav_BuildInvokerSelection{};
        if (NOT groundnav::Request_ComputeBuildInvokerSelection(
            Producer->_Params, Points, Boxes, InInvokerState._EnabledTileIndices, Selection))
        { return; }

        auto Desired = InInvokerState._EnabledTileIndices;
        for (const auto TileIndex : Selection._BuildTileIndices)
        { Desired.Add(TileIndex); }
        for (const auto TileIndex : Selection._PurgeTileIndices)
        { Desired.Remove(TileIndex); }
        Desired.Sort();

        if (Desired == InInvokerState._DesiredTileIndices)
        { return; }

        const auto PreviousDesired = InInvokerState._DesiredTileIndices;
        const auto PreviousGeneration = InInvokerState._TargetGeneration;
        InInvokerState._DesiredTileIndices = MoveTemp(Desired);
        ++InInvokerState._TargetGeneration;

        auto ProducerBuilt = TSet<int32>{};
        for (auto TileIndex = 0; TileIndex < Producer->_Tiles.Num(); ++TileIndex)
        { if (Producer->_Tiles[TileIndex].Get_IsBuilt()) { ProducerBuilt.Add(TileIndex); } }

        // A tile returning to generation can be producer-built while absent from the enabled query
        // mask. It is therefore a zero-probe registry re-enable, not a targeted bake.
        auto MissingBuild = TArray<int32>{};
        for (const auto TileIndex : Selection._BuildTileIndices)
        {
            if (NOT ProducerBuilt.Contains(TileIndex))
            { MissingBuild.Emplace(TileIndex); }
        }

        // A purge or retained re-enable needs no geometry work. Commit its exact desired mask first;
        // local enabled state changes only after the registry accepted the same transaction.
        if (MissingBuild.IsEmpty() && InInvokerState._Source.Get_IsValid())
        {
            auto EnabledCoords = TArray<groundnav::FCk_GroundNav_TileCoord>{};
            EnabledCoords.Reserve(InInvokerState._DesiredTileIndices.Num());
            for (const auto TileIndex : InInvokerState._DesiredTileIndices)
            { EnabledCoords.Emplace(groundnav::Get_TileCoord(Producer->_Params._Divisions, TileIndex)); }
            const auto VolumeId = groundnav::FCk_GroundNav_VolumeId{InParams.Get_StreamingVolumeId()};
            const auto Result = groundnav::world_fields::Upsert_StreamSourceTiles(
                World, VolumeId, InInvokerState._Source, {}, EnabledCoords);
            if (NOT Result.Get_Succeeded())
            {
                InInvokerState._DesiredTileIndices = PreviousDesired;
                InInvokerState._TargetGeneration = PreviousGeneration;
                return;
            }
            InInvokerState._EnabledTileIndices = InInvokerState._DesiredTileIndices;
            if (Result._ChangedBounds.IsValid != 0)
            { nav_surface::Request_NotifySurfaceRebuilt(World, Result._ChangedBounds); }
        }

        // An active target is never mutated in place. Its completion compares this generation before
        // it is allowed to compose or publish, so a move during a budgeted bake cannot resurrect tiles
        // the newer world aggregation already removed.
        if (NOT MissingBuild.IsEmpty() &&
            NOT InVolumeEntity.Has<FTag_GroundNavVolume_BuildInProgress>())
        {
            InInvokerState._ActiveBuildTileIndices = MoveTemp(MissingBuild);
            InInvokerState._ActiveBuildGeneration = InInvokerState._TargetGeneration;
            InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsBuild>();
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Requests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InVolumeEntity, InParams, InBuiltField, InBuildState, InRepairState, InRequest);
            }));
    }

    auto
        FProcessor_GroundNavVolume_HandleRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FCk_Request_GroundNavVolume_Build& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        const auto& RequestedSelectorInput =
            InRequest.Get_OverrideDataLayerSelector() == ECk_EnableDisable::Enable
                ? InRequest.Get_DataLayerSelector()
                : InParams.Get_DataLayerSelector();
        auto RequestedSelector = groundnav::FCk_GroundNav_DataLayerSelector{};
        const auto SelectorIsValid = groundnav::TryMake_DataLayerSelector(
            RequestedSelectorInput.Get_LayerNames(), RequestedSelector);

        CK_ENSURE_IF_NOT(SelectorIsValid,
            TEXT("Cannot build GroundNav Volume [{}] - its requested data-layer selector is invalid"),
            InVolumeEntity)
        {}

        if (NOT SelectorIsValid)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto ParamsAreBakeable = Get_ParamsAreBakeable(InParams);

        CK_ENSURE_IF_NOT(ParamsAreBakeable,
            TEXT("Cannot build GroundNav Volume [{}] - its params are not bakeable"), InVolumeEntity)
        {}

        if (NOT ParamsAreBakeable)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        if (InParams.Get_StreamingBuildScope() == ECk_GroundNav_StreamingBuildScope::ManifestDriven)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto CookKeyIsUnique = Get_CookKeyIsUnique(InVolumeEntity, InParams);

        // Judged again here rather than trusted from admission: a volume whose key was free when it
        // was set up can be joined later by one that took the same name.
        CK_ENSURE_IF_NOT(CookKeyIsUnique,
            TEXT("Cannot build GroundNav Volume [{}] - another volume in this world already carries "
                 "its cook key [{}]"),
            InVolumeEntity, InParams.Get_CookKey())
        {}

        if (NOT CookKeyIsUnique)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto IsBuilding = InVolumeEntity.Has<FTag_GroundNavVolume_BuildInProgress>() ||
                                InVolumeEntity.Has<FTag_GroundNavVolume_NeedsBuild>();

        // A request arriving while work for the same selector is already armed is an idempotent no-op.
        // A selector change is new work even without ForceRestart: the current build cannot satisfy it.
        const auto ExistingSelector = InVolumeEntity.Has<FTag_GroundNavVolume_BuildInProgress>()
            ? InBuildState._ActiveDataLayerSelector.Get_LayerNames()
            : (InBuildState._HasPendingBuildRequest
                ? (InBuildState._PendingRequest.Get_OverrideDataLayerSelector() == ECk_EnableDisable::Enable
                    ? InBuildState._PendingRequest.Get_DataLayerSelector().Get_LayerNames()
                    : RequestedSelector.Get_LayerNames())
                : (InBuiltField.Get_Field().IsValid()
                    ? InBuiltField.Get_DataLayerSelector().Get_LayerNames()
                    : InParams.Get_DataLayerSelector().Get_LayerNames()));
        const auto ExistingBuildSatisfiesRequest =
            IsBuilding && ExistingSelector == RequestedSelector.Get_LayerNames();

        if (ExistingBuildSatisfiesRequest && InRequest.Get_ForceRestart() == ECk_EnableDisable::Disable)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        // A restart strands whoever was waiting on the build it replaces, so that build completes as
        // cancelled rather than silently never reporting.
        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);

        DoCancel_RepairInFlight(InVolumeEntity, InRepairState);

        InBuildState._Backend.Reset();
        InBuildState._PendingRequest = InRequest;
        InBuildState._PendingRequest.Set_DataLayerSelector(RequestedSelector);
        InBuildState._HasPendingBuildRequest = true;

        InVolumeEntity.Try_Remove<FTag_GroundNavVolume_BuildInProgress>();
        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsBuild>();
    }

    auto
        FProcessor_GroundNavVolume_HandleRequests::
        DoCancel_RepairInFlight(
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState)
        -> void
    {
        if (NOT InVolumeEntity.Has<FTag_GroundNavVolume_RepairInProgress>())
        { return; }

        for (const auto& InFlightRequest : InRepairState._InFlightRequests)
        {
            InFlightRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);
        }

        InRepairState._InFlightRequests.Reset();
        InRepairState._Repair = {};
        InRepairState._Backend.Reset();

        // A build taking a repair off the volume is no evidence its region was failing, so the retry
        // budget is not spent by one.
        InRepairState._StaleRetryCount = 0;

        InVolumeEntity.Remove<FTag_GroundNavVolume_RepairInProgress>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_HandleRepairRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_RepairRequests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InVolumeEntity, InBuiltField, InRepairState, InRequest);
            }));
    }

    auto
        FProcessor_GroundNavVolume_HandleRepairRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FCk_Request_GroundNavVolume_Repair& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        // Only a build that has not STARTED answers a region arriving now: it snapshots the volume when
        // it starts, so a region raised after that is outside what it bakes and waits for the publish.
        const auto BuildWillAnswerIt = InVolumeEntity.Has<FTag_GroundNavVolume_NeedsBuild>() ||
                                       InVolumeEntity.Has<FTag_GroundNavVolume_BuildInProgress>();

        // A repair carries every tile OUTSIDE its dirty box over from a previous bake, so a volume with
        // nothing published has nothing to carry and no repair to make. A retry does not change that -
        // a build is what bakes that ground - which is what Failed means. Unless a build is coming or
        // running: then the region waits for its publish, rides it if the build has not started, and
        // repairs what it published otherwise.
        if (NOT InBuiltField.Get_Field().IsValid() && NOT BuildWillAnswerIt)
        {
            groundnav::Verbose(
                TEXT("GroundNav Volume [{}] was handed a dirty region [{}] before it published a field - "
                     "dropping the region; a build is what bakes that ground"),
                InVolumeEntity, InRequest.Get_DirtyBounds());

            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        // Several bodies may dirty one volume in a frame, and one repair over their union costs less
        // than one repair each.
        InRepairState._PendingDirtyBounds =
            Get_UnionedBounds(InRepairState._PendingDirtyBounds, InRequest.Get_DirtyBounds());

        InRepairState._PendingRequests.Emplace(InRequest);

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsRepair>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Markup& InMarkup,
            FFragment_GroundNavVolume_MarkupRequests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InVolumeEntity, InBuiltField, InRepairState, InMarkup, InRequest);
            }));
    }

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Markup& InMarkup,
            const FCk_Request_GroundNavVolume_AreaMarkup& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        auto MarkupEntity = InRequest.Get_MarkupEntity();

        const auto MarkupEntityIsValid = ck::IsValid(MarkupEntity);

        CK_ENSURE_IF_NOT(MarkupEntityIsValid,
            TEXT("Cannot mark up GroundNav Volume [{}] - the request names an invalid markup Entity [{}], "
                 "which is the identity its record would be keyed on"),
            InVolumeEntity, MarkupEntity)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto Policy = nav_surface::TryGet_AreaPolicy(InRequest.Get_AreaTag());

        const auto AreaTagIsRegistered = Policy.IsSet();

        CK_ENSURE_IF_NOT(AreaTagIsRegistered,
            TEXT("Cannot mark up GroundNav Volume [{}] with area tag [{}] - nothing published what that "
                 "tag MEANS, and a record carrying it would bake into ground nothing knows how to apply"),
            InVolumeEntity, InRequest.Get_AreaTag())
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto ExistingIndex = Get_MarkupEntryIndex(InMarkup, MarkupEntity);
        const auto EntryExists = InMarkup._Entries.IsValidIndex(ExistingIndex);

        // The WHOLE previous record, because an update that MOVED one leaves the ground it used to
        // cover decided by nothing, and a copy is needed anyway: the entry is overwritten below.
        const auto PreviousRecord = EntryExists
            ? TOptional<FCk_GroundNav_MarkupRecord>{InMarkup._Entries[ExistingIndex].Get_Record()}
            : TOptional<FCk_GroundNav_MarkupRecord>{};

        const auto RecordId = PreviousRecord.IsSet()
            ? PreviousRecord->Get_Id()
            : InMarkup._NextId;

        const auto Kind = Get_MarkupKind(Policy->Get_Kind());

        // A Walkability record carries the identity multiplier by its own contract, so both kinds
        // multiply through one cost path downstream without a branch.
        const auto CostMultiplier = Kind == ECk_GroundNav_MarkupKind::Cost
            ? Policy->Get_CostMultiplier()
            : 1.0f;

        auto Record = FCk_GroundNav_MarkupRecord{
            RecordId, InRequest.Get_Shape(), InRequest.Get_WorldTransform(), Kind};

        Record.Set_AreaTag(InRequest.Get_AreaTag())
              .Set_Enable(InRequest.Get_Enable())
              .Set_CostMultiplier(CostMultiplier)
              .Set_RequestedAtEpoch(InBuiltField.Get_Epoch()._Value);

        const auto BoundsAreValid = groundnav::Get_MarkupWorldBounds(Record).IsValid != 0;

        CK_ENSURE_IF_NOT(BoundsAreValid,
            TEXT("Cannot mark up GroundNav Volume [{}] from markup Entity [{}] - its shape and transform "
                 "bound nothing. A degenerate volume and a volume that covers no ground are different "
                 "answers, and only the second is admissible."),
            InVolumeEntity, MarkupEntity)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        // A record whose bounds miss every tile, or land where nothing has baked, is still admitted: the
        // volume owns what was AUTHORED, and what that reaches is the bake's separate answer.
        if (EntryExists)
        {
            InMarkup._Entries[ExistingIndex]._Record = Record;
        }
        else
        {
            auto& NewEntry = InMarkup._Entries.AddDefaulted_GetRef();

            NewEntry._MarkupEntity = MarkupEntity;
            NewEntry._Record = Record;

            ++InMarkup._NextId;
        }

        auto& MarkupRef = MarkupEntity.AddOrGet<FFragment_GroundNav_MarkupRef>();

        MarkupRef._VolumeEntity = InVolumeEntity.ConvertToHandle();
        MarkupRef._RecordId = RecordId;

        // The record's OLD footprint owes its old kind an answer whether the update moved it, retagged
        // it or changed nothing: ground the previous record was deciding is decided by the new one only
        // where the two overlap, and a footprint left behind is ground nothing else will ever revisit.
        if (PreviousRecord.IsSet())
        {
            DoMark_MarkupDirty(InVolumeEntity, InBuiltField, InRepairState, PreviousRecord->Get_Kind(),
                groundnav::Get_MarkupWorldBounds(*PreviousRecord));
        }

        DoMark_MarkupDirty(InVolumeEntity, InBuiltField, InRepairState, Kind,
            groundnav::Get_MarkupWorldBounds(Record));

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Markup& InMarkup,
            const FCk_Request_GroundNavVolume_ReleaseAreaMarkup& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        auto MarkupEntity = InRequest.Get_MarkupEntity();

        // A release is not guarded on the entity being alive, because the ordinary release IS that
        // entity's own teardown: the neutral NavSurface EndPlay asks the provider to release, and the
        // volume drains that request a frame later, with the entity already dead. The entry is keyed on
        // the entity's IDENTITY - which FCk_Handle equality answers from the entity id and registry
        // alone, never from validity - so a dead handle still finds the entry stored beside its twin.
        // Refusing one here refuses the single moment a record must still be findable, and the ground
        // it covers is then decided by a record nothing will ever release.
        const auto ExistingIndex = Get_MarkupEntryIndex(InMarkup, MarkupEntity);

        // Releasing a record the volume does not hold leaves the caller's intent holding afterwards,
        // which is what Succeeded means.
        if (NOT InMarkup._Entries.IsValidIndex(ExistingIndex))
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        // Copied before the entry goes: the ground the release changed is the ground the record covered,
        // and after RemoveAt there is nothing left to ask.
        const auto ReleasedRecord = InMarkup._Entries[ExistingIndex].Get_Record();

        InMarkup._Entries.RemoveAt(ExistingIndex);

        // Only where there is still an entity to drop it from: a dead one carries no fragments, and the
        // record just removed above was the only state that outlived it.
        if (ck::IsValid(MarkupEntity))
        { MarkupEntity.Try_Remove<FFragment_GroundNav_MarkupRef>(); }

        // Ground a released record was deciding is decided by nothing now, which is as much a change of
        // that record's kind as painting it was.
        DoMark_MarkupDirty(InVolumeEntity, InBuiltField, InRepairState, ReleasedRecord.Get_Kind(),
            groundnav::Get_MarkupWorldBounds(ReleasedRecord));

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        DoMark_MarkupDirty(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            ECk_GroundNav_MarkupKind InKind,
            const FBox& InMarkupWorldBounds)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        if (InKind == ECk_GroundNav_MarkupKind::Cost)
        {
            InVolumeEntity.AddOrGet<FTag_GroundNavVolume_MarkupCostDirty>();
            return;
        }

        // A volume with nothing published has no field for a repair to carry its untouched tiles over
        // from. The record is already on the volume, so the first build bakes it in through
        // FCk_GroundNav_FieldParams::_MarkupRecords - the same wait the cost derive makes, and the
        // reason a paint before the first bake is never lost. A build already RUNNING is the exception:
        // it snapshotted its records when it started, so this record is outside what it bakes and its
        // ground is owed a repair the moment that build publishes.
        const auto ABuildIsRunning = InVolumeEntity.Has<FTag_GroundNavVolume_BuildInProgress>();

        if (NOT InBuiltField.Get_Field().IsValid() && NOT ABuildIsRunning)
        { return; }

        InRepairState._PendingDirtyBounds =
            Get_UnionedBounds(InRepairState._PendingDirtyBounds, InMarkupWorldBounds);

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsRepair>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_HandleLinkRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            FFragment_GroundNavVolume_LinkRequests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InVolumeEntity, InParams, InBuiltField, InLinks, InRequest);
            }));
    }

    auto
        FProcessor_GroundNavVolume_HandleLinkRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            const FCk_Request_GroundNavVolume_Link& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        const auto Refusal = TryGet_LinkAdmissionRefusal(InVolumeEntity, InParams, InRequest);

        const auto RecordIsAdmissible = NOT Refusal.IsSet();

        CK_ENSURE_IF_NOT(RecordIsAdmissible, TEXT("{}"), Refusal.Get(FString{}))
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        DoApply_Link(InVolumeEntity, InBuiltField, InLinks, InRequest);

        // A link whose ends reach no baked ground is still admitted: the volume owns what was AUTHORED,
        // and where its two points stand is the composition's separate answer.
        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_LinksDirty>();

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleLinkRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            const FCk_Request_GroundNavVolume_LinkBatch& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        const auto& Entries = InRequest.Get_Entries();

        auto FirstRefusedIndex = int32{INDEX_NONE};
        auto FirstRefusal = FString{};

        // Judged in full BEFORE anything is applied, which is the whole of what atomic means here:
        // there is no partial state to unwind because none was ever written.
        for (auto Index = int32{0}; Index < Entries.Num(); ++Index)
        {
            const auto Refusal = TryGet_LinkAdmissionRefusal(InVolumeEntity, InParams, Entries[Index]);

            if (NOT Refusal.IsSet())
            { continue; }

            FirstRefusedIndex = Index;
            FirstRefusal = Refusal.GetValue();

            break;
        }

        const auto EveryEntryIsAdmissible = FirstRefusedIndex == INDEX_NONE;

        // One line for the batch, naming the entry that decided it. A batch that admitted its good
        // half would leave the caller unable to say which part of its intent holds, with ids already
        // spent on the rest.
        CK_ENSURE_IF_NOT(EveryEntryIsAdmissible,
            TEXT("Cannot link GroundNav Volume [{}] from a batch of [{}] - entry [{}] is refused, and a "
                 "batch is admitted whole or not at all. {}"),
            InVolumeEntity, Entries.Num(), FirstRefusedIndex, FirstRefusal)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        // A batch carrying nothing changes nothing, and a derive over a list nothing touched would
        // publish an epoch nothing moved - the same answer the release of an unheld record gives.
        if (Entries.IsEmpty())
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        for (const auto& Entry : Entries)
        {
            DoApply_Link(InVolumeEntity, InBuiltField, InLinks, Entry);
        }

        // ONE derive for the whole batch: the tag is idempotent, and the derive re-resolves every
        // record from the whole list however many of them moved.
        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_LinksDirty>();

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleLinkRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            const FCk_Request_GroundNavVolume_ReleaseLink& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        auto LinkEntity = InRequest.Get_LinkEntity();

        // Not guarded on the entity being alive, for the same reason the markup release is not: the
        // ordinary release IS that entity's own teardown, and the entry is keyed on the entity's
        // IDENTITY - which FCk_Handle equality answers from the entity id and registry alone, never
        // from validity - so a dead handle still finds the entry stored beside its twin.
        const auto ExistingIndex = Get_LinkEntryIndex(InLinks, LinkEntity);

        // Releasing a record the volume does not hold leaves the caller's intent holding afterwards,
        // which is what Succeeded means.
        if (NOT InLinks._Entries.IsValidIndex(ExistingIndex))
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        DoRetire_LinkEntry(InLinks, ExistingIndex);

        // The connectivity a released link contributed is retired by the same derive that granted it:
        // every record is re-resolved from the whole list, so a list one shorter is the whole change.
        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_LinksDirty>();

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleLinkRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            const FCk_Request_GroundNavVolume_ReleaseLink_ById& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        // By RECORD ID rather than by entity: the id is what a caller that has outlived the link
        // entity still holds, and ids are never reused, so this can only ever name the record it was
        // handed out for.
        const auto ExistingIndex = Get_LinkEntryIndexById(InLinks, InRequest.Get_LinkId());

        // An id the volume does not hold - retired, or never handed out - leaves the caller's intent
        // holding afterwards, exactly as releasing an unheld entity does.
        if (NOT InLinks._Entries.IsValidIndex(ExistingIndex))
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        DoRetire_LinkEntry(InLinks, ExistingIndex);

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_LinksDirty>();

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleLinkRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            const FCk_Request_GroundNavVolume_ReleaseAllLinks& InRequest)
        -> void
    {
        // A volume that holds nothing already satisfies the caller's intent, and no derive is owed for
        // a change that changed nothing.
        if (InLinks._Entries.IsEmpty())
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        // Back to front, so each removal shifts nothing behind it, and through the same retirement one
        // release uses - the back-pointers go with their records rather than being left to name a
        // volume that no longer holds them.
        for (auto Index = InLinks._Entries.Num() - 1; Index >= 0; --Index)
        {
            DoRetire_LinkEntry(InLinks, Index);
        }

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_LinksDirty>();

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleLinkRequests::
        DoApply_Link(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            const FCk_Request_GroundNavVolume_Link& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        auto LinkEntity = InRequest.Get_LinkEntity();

        const auto& Requested = InRequest.Get_Record();

        const auto ExistingIndex = Get_LinkEntryIndex(InLinks, LinkEntity);
        const auto EntryExists = InLinks._Entries.IsValidIndex(ExistingIndex);

        // An update keeps the id the entity was first admitted under: the id is what every later
        // request and every consumer keys on, and renumbering it on a move would retire connectivity
        // nothing asked to retire.
        const auto RecordId = EntryExists
            ? InLinks._Entries[ExistingIndex].Get_Record().Get_Id()
            : InLinks._NextId;

        // Rebuilt rather than copied because the id and the two points are the record's identity and
        // carry no setters - which is what stops a caller renumbering one after the fact.
        auto Record = FCk_GroundNav_LinkRecord{RecordId, Requested.Get_Start(), Requested.Get_End()};

        Record.Set_Direction(Requested.Get_Direction())
              .Set_CostMultiplierForward(Requested.Get_CostMultiplierForward())
              .Set_CostMultiplierBackward(Requested.Get_CostMultiplierBackward())
              .Set_ClearanceUu(Requested.Get_ClearanceUu())
              .Set_AreaTag(Requested.Get_AreaTag())
              .Set_UserTypeTag(Requested.Get_UserTypeTag())
              .Set_Enable(Requested.Get_Enable())
              .Set_ProjectionMode(Requested.Get_ProjectionMode())
              .Set_ProjectionHorizontalExtentUu(Requested.Get_ProjectionHorizontalExtentUu())
              .Set_ProjectionVerticalExtentUu(Requested.Get_ProjectionVerticalExtentUu())
              .Set_RequestedAtEpoch(InBuiltField.Get_Epoch()._Value);

        // Re-stamped on every update, including one that only disabled the link: liveness asks whether
        // the field has been published PAST the change, and a stamp left at the first admission would
        // read a link as live on a publish that knew nothing of what it now says.
        if (EntryExists)
        {
            InLinks._Entries[ExistingIndex]._Record = Record;
        }
        else
        {
            auto& NewEntry = InLinks._Entries.AddDefaulted_GetRef();

            NewEntry._LinkEntity = LinkEntity;
            NewEntry._Record = Record;

            ++InLinks._NextId;
        }

        auto& LinkRef = LinkEntity.AddOrGet<FFragment_GroundNav_LinkRef>();

        LinkRef._VolumeEntity = InVolumeEntity.ConvertToHandle();
        LinkRef._RecordId = RecordId;
    }

    auto
        FProcessor_GroundNavVolume_HandleLinkRequests::
        DoRetire_LinkEntry(
            FFragment_GroundNavVolume_Links& InLinks,
            int32 InEntryIndex)
        -> void
    {
        auto LinkEntity = InLinks._Entries[InEntryIndex].Get_LinkEntity();

        // The id goes with the entry and is never handed out again, so a field resolved against an
        // older link set can be diffed against a newer one without an id meaning two different links.
        InLinks._Entries.RemoveAt(InEntryIndex);

        // Only where there is still an entity to drop it from: a dead one carries no fragments, and the
        // record just removed above was the only state that outlived it.
        if (ck::IsValid(LinkEntity))
        { LinkEntity.Try_Remove<FFragment_GroundNav_LinkRef>(); }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_StartBuild::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_InvokerState& InInvokerState) const
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        // Every dirty region waiting on a repair is inside the ground this build re-bakes from live
        // geometry under the records it snapshots below, so those regions are answered by it - and
        // answered better. They are taken over HERE, at the snapshot, and not at the publish: a region
        // raised while the build runs is outside what it baked and has to survive the publish as a
        // repair of the field it produces.
        InRepairState._RidingBuildRequests.Append(MoveTemp(InRepairState._PendingRequests));
        InRepairState._PendingRequests.Reset();
        InRepairState._PendingDirtyBounds = FBox{ForceInit};
        InRepairState._StaleRetryCount = 0;

        InVolumeEntity.Try_Remove<FTag_GroundNavVolume_NeedsRepair>();

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        auto EffectiveDataLayerSelector = InBuildState._HasPendingBuildRequest
            ? (InBuildState._PendingRequest.Get_OverrideDataLayerSelector() == ECk_EnableDisable::Enable
                ? InBuildState._PendingRequest.Get_DataLayerSelector()
                : InParams.Get_DataLayerSelector())
            : (InBuiltField.Get_Field().IsValid()
                ? InBuiltField.Get_DataLayerSelector()
                : InParams.Get_DataLayerSelector());
        auto CanonicalEffectiveSelector = groundnav::FCk_GroundNav_DataLayerSelector{};
        const auto EffectiveSelectorIsValid = groundnav::TryMake_DataLayerSelector(
            EffectiveDataLayerSelector.Get_LayerNames(), CanonicalEffectiveSelector);
        CK_ENSURE_IF_NOT(EffectiveSelectorIsValid,
            TEXT("GroundNav Volume [{}] cannot start a build with an invalid data-layer selector"), InVolumeEntity)
        {}
        if (NOT EffectiveSelectorIsValid)
        {
            InBuildState._HasPendingBuildRequest = false;
            InBuildState._PendingRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }
        EffectiveDataLayerSelector = MoveTemp(CanonicalEffectiveSelector);
        InBuildState._HasPendingBuildRequest = false;
        InBuildState._ActiveDataLayerSelector = EffectiveDataLayerSelector;

        InBuildState._Backend = MakeUnique<groundnav::FCk_GroundNav_GeometryBackend_Jolt>(
            World, EffectiveDataLayerSelector);

        const auto BackendIsUsable = InBuildState._Backend->Get_IsValid();

        CK_ENSURE_IF_NOT(BackendIsUsable,
            TEXT("GroundNav Volume [{}] cannot start a build: no physics world to read geometry from"),
            InVolumeEntity)
        {
            // Never a Built field with nothing in it. A backend that cannot answer means the world is
            // unknown, which is not the same as a world with no floor.
            InBuildState._Backend.Reset();

            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            // The repairs this start took over above rode a build that never began: their regions went
            // with the box that named them, and no publish is coming for them. They are told so here
            // rather than left holding a delegate nothing will ever fire.
            for (const auto& RidingRepairRequest : InRepairState._RidingBuildRequests)
            { RidingRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed); }

            InRepairState._RidingBuildRequests.Reset();

            return;
        }

        // Snapshotted here, out of the same array the params list below is assembled from. Completion
        // keys the fields it releases by THIS list and not by the params: _ProfileVariants is writable,
        // and a list edited while the build ran would key a finished field under a tag it was never
        // baked for.
        InBuildState._ProfileVariantTags = algo::Transform<TArray<FGameplayTag>>(
            InParams.Get_ProfileVariants(),
            [](const FCk_GroundNav_ProfileVariant& InVariant) -> FGameplayTag
            {
                return InVariant.Get_ProfileTag();
            });

        // The epoch comes from what is PUBLISHED, not from the build state: beginning a build resets
        // that state, so reading the counter from it would restart at one on every rebuild and every
        // reader comparing epochs would conclude it was up to date.
        //
        // The markup goes in HERE and not at the request, so every build — a plain Request_Build as
        // much as one a markup change asked for — bakes against what the volume currently holds. A
        // rebuild that took no records would silently unpaint the world.
        //
        // The links ride in the same way and for the same reason: a rebuild that took no links would
        // silently un-link the world, and it is what makes a link authored before the first bake - or
        // while one is running - resolved by the publish that follows rather than lost with it.
        //
        // ONE BUILD PER VOLUME whatever the profile count. The variants share this volume's lattice,
        // config, markup and links, so they ride the same pass over the geometry: the tile is fetched
        // once and baked under each profile before the resume point moves, which is what stops two
        // profiles ever being baked against two different worlds. A volume with no variant is the
        // one-element case of the same call rather than a second path through this processor.

        // Hoisted out of the call because the field params and the bake both read them. The identity
        // stamped below is taken from the same fragments in the same tick, so a record admitted after
        // this point is outside everything this build is a statement about.
        const auto MarkupRecords =
            Get_MarkupRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(InVolumeEntity));
        const auto LinkRecords =
            Get_LinkRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_LinkEntries(InVolumeEntity));

        const auto FieldParams = Get_MultiProfileFieldParams(InParams, MarkupRecords, LinkRecords);
        const auto IsInvokerBuild = InParams.Get_StreamingBuildScope() == ECk_GroundNav_StreamingBuildScope::InvokerDriven;
        const auto BeginResult = IsInvokerBuild
            ? groundnav::Request_BeginBuild_MultiProfile_Targeted(
                FieldParams, InBuiltField.Get_Epoch().Get_Next(), InInvokerState._ActiveBuildTileIndices, InBuildState._Build)
            : groundnav::Request_BeginBuild_MultiProfile(
                FieldParams, InBuiltField.Get_Epoch().Get_Next(), InBuildState._Build);

        CK_ENSURE_IF_NOT(BeginResult.Get_IsCompleted(),
            TEXT("GroundNav Volume [{}] could not begin a build: [{}]"),
            InVolumeEntity, BeginResult.Get_Status())
        {
            InBuildState._Backend.Reset();

            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            // On the same terms as the backend refusal above: nothing began, so nothing will publish
            // the ground these regions named.
            for (const auto& RidingRepairRequest : InRepairState._RidingBuildRequests)
            { RidingRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed); }

            InRepairState._RidingBuildRequests.Reset();

            return;
        }

        // What this build is a statement about, recorded where it BEGINS: a record admitted while it
        // runs is not in the ground it bakes. The tiled build never holds the region's triangles at
        // once - it collects one tile's halo per slice - so the backend's WORLD REVISION is the
        // geometry half, and it is the same token a later slice fails closed on when the world moves
        // underneath it.
        const auto Identity = Get_PublishedIdentity(
            InVolumeEntity, InParams, InBuildState._Backend->Get_WorldRevision(),
            InBuildState._ActiveDataLayerSelector);

        InBuildState._BakedInputFingerprint = Identity._InputFingerprint;
        InBuildState._BakedGeometryRevision = Identity._GeometryRevision;

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_BuildInProgress>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_Build::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_InvokerState& InInvokerState) const
        -> void
    {
        const auto BackendIsHeld = InBuildState._Backend.IsValid();

        CK_ENSURE_IF_NOT(BackendIsHeld,
            TEXT("GroundNav Volume [{}] is marked as building but holds no geometry backend"),
            InVolumeEntity)
        {
            InVolumeEntity.Remove<FTag_GroundNavVolume_BuildInProgress>();

            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            return;
        }

        const auto SliceResult = groundnav::Request_AdvanceBuild(
            *InBuildState._Backend, InParams.Get_ProbeBudgetPerTick(), InBuildState._Build);

        if (SliceResult.Get_Status() == ECk_GroundNav_BakeStatus::BudgetExhausted)
        { return; }

        InVolumeEntity.Remove<FTag_GroundNavVolume_BuildInProgress>();
        InBuildState._Backend.Reset();

        // The PLURAL release, index-aligned with the params the build began with: element zero is the
        // untagged default and the rest are this volume's variants in authored order. A volume with no
        // variant releases a one-element array, which is why there is no second shape here.
        auto CompletedFields = groundnav::Request_ReleaseCompletedFields(InBuildState._Build);

        auto Completed = CompletedFields.IsEmpty()
            ? groundnav::FCk_GroundNav_FieldPtr{}
            : CompletedFields[0];

        if (NOT SliceResult.Get_IsCompleted() || NOT Completed.IsValid())
        {
            // A failed REBUILD leaves the previously published field alone: stale ground is still
            // ground, and dropping it would strand every agent standing on it.
            groundnav::Error(TEXT("GroundNav Volume [{}] failed to build: [{}]"),
                InVolumeEntity, SliceResult.Get_Status());

            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            // The regions that rode this build were never baked by it. They fail with it rather than
            // being put back: a build is what bakes that ground, and their callers are told so now
            // rather than left waiting on a publish that is not coming.
            for (const auto& RidingRepairRequest : InRepairState._RidingBuildRequests)
            { RidingRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed); }

            InRepairState._RidingBuildRequests.Reset();

            return;
        }

        // The count is a CONTRACT with the build and not a bound to loop within: the release comes back
        // index-aligned with the params that went in, so a field per variant plus the default is the
        // only shape it can have. Truncating to whichever list is shorter would publish a map that
        // silently omits a profile, so a mismatch publishes nothing at all.
        const auto& ProfileVariantTags = InBuildState._ProfileVariantTags;

        const auto CompletedFieldCount = CompletedFields.Num();
        const auto ExpectedFieldCount = ProfileVariantTags.Num() + 1;

        CK_ENSURE_IF_NOT(CompletedFieldCount == ExpectedFieldCount,
            TEXT("GroundNav Volume [{}] finished a build holding [{}] field(s) where its profiles asked "
                 "for [{}]. Publishing nothing: a partial map would leave a profile answering from "
                 "another profile's ground"),
            InVolumeEntity, CompletedFieldCount, ExpectedFieldCount)
        {
            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            for (const auto& RidingRepairRequest : InRepairState._RidingBuildRequests)
            { RidingRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed); }

            InRepairState._RidingBuildRequests.Reset();

            return;
        }

        // Published by SWAPPING the pointer. Whoever is holding the previous field keeps reading it,
        // whole, for as long as they hold it.
        const auto IsInvokerBuild = InParams.Get_StreamingBuildScope() == ECk_GroundNav_StreamingBuildScope::InvokerDriven;
        if (IsInvokerBuild)
        {
            const auto ExistingProducer = InBuiltField.Get_Field();
            const auto ProducerIsUsable = ExistingProducer.IsValid() && InInvokerState._Source.Get_IsValid();
            CK_ENSURE_IF_NOT(ProducerIsUsable,
                TEXT("GroundNav invoker-driven Volume [{}] completed without its retained producer or source"), InVolumeEntity)
            {}
            if (NOT ProducerIsUsable)
            {
                InBuildState._PendingRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
                return;
            }

            // The world can move while the target is budgeted. Discard the completed subset before it
            // reaches the producer or registry when a later aggregation superseded this generation.
            if (InInvokerState._ActiveBuildGeneration != InInvokerState._TargetGeneration)
            {
                auto ExistingIndices = TSet<int32>{};
                for (auto TileIndex = 0; TileIndex < ExistingProducer->_Tiles.Num(); ++TileIndex)
                {
                    if (ExistingProducer->_Tiles[TileIndex].Get_IsBuilt())
                    { ExistingIndices.Add(TileIndex); }
                }
                InInvokerState._ActiveBuildTileIndices.Reset();
                for (const auto TileIndex : InInvokerState._DesiredTileIndices)
                {
                    if (NOT ExistingIndices.Contains(TileIndex))
                    { InInvokerState._ActiveBuildTileIndices.Emplace(TileIndex); }
                }
                if (NOT InInvokerState._ActiveBuildTileIndices.IsEmpty())
                {
                    InInvokerState._ActiveBuildGeneration = InInvokerState._TargetGeneration;
                    InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsBuild>();
                }
                InBuildState._PendingRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);
                return;
            }

            auto ProducerBundle = groundnav::FCk_GroundNav_StreamFieldBundle{};
            ProducerBundle._DefaultField = *ExistingProducer;
            for (const auto& Variant : InBuiltField.Get_VariantFields())
            { ProducerBundle._VariantFields.Emplace(Variant.Key, *Variant.Value); }

            auto Transitions = TArray<groundnav::FCk_GroundNav_StreamTileTransition>{};
            const auto VolumeId = groundnav::FCk_GroundNav_VolumeId{InParams.Get_StreamingVolumeId()};
            for (const auto TileIndex : InInvokerState._ActiveBuildTileIndices)
            {
                ProducerBundle._DefaultField._Tiles[TileIndex] = CompletedFields[0]->_Tiles[TileIndex];
                auto Transition = groundnav::FCk_GroundNav_StreamTileTransition{};
                Transition._TileId = groundnav::FCk_GroundNav_StreamTileId{
                    VolumeId, groundnav::Get_TileCoord(ProducerBundle._DefaultField._Params._Divisions, TileIndex)};
                Transition._Kind = groundnav::ECk_GroundNav_StreamTileTransitionKind::Replace;
                groundnav::Write_Tile(ProducerBundle._DefaultField, Transition._TileId._Coord, Transition._DefaultBlob);

                for (auto VariantIndex = 0; VariantIndex < ProfileVariantTags.Num(); ++VariantIndex)
                {
                    auto* Variant = ProducerBundle._VariantFields.Find(ProfileVariantTags[VariantIndex]);
                    if (Variant == nullptr)
                    { continue; }
                    Variant->_Tiles[TileIndex] = CompletedFields[VariantIndex + 1]->_Tiles[TileIndex];
                    groundnav::Write_Tile(*Variant, Transition._TileId._Coord,
                        Transition._VariantBlobs.FindOrAdd(ProfileVariantTags[VariantIndex]));
                }
                Transitions.Emplace(MoveTemp(Transition));
            }

            // The retained producer has just changed at selected coordinates. Re-derive the values
            // which are functions of more than one tile before it becomes the source for future
            // selection or zero-probe restore.
            auto BundleIsComplete = ProducerBundle._VariantFields.Num() == ProfileVariantTags.Num();
            CK_ENSURE_IF_NOT(BundleIsComplete,
                TEXT("GroundNav invoker-driven Volume [{}] lost a profile while merging a tile subset"), InVolumeEntity)
            {}
            if (NOT BundleIsComplete)
            {
                InBuildState._PendingRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
                return;
            }
            groundnav::DoDerive_SeamPortals(ProducerBundle._DefaultField);
            groundnav::DoResolve_Links(ProducerBundle._DefaultField);
            groundnav::DoLabel_Reachability(ProducerBundle._DefaultField);
            for (auto& Variant : ProducerBundle._VariantFields)
            {
                groundnav::DoDerive_SeamPortals(Variant.Value);
                groundnav::DoResolve_Links(Variant.Value);
                groundnav::DoLabel_Reachability(Variant.Value);
            }

            auto EnabledCoords = TArray<groundnav::FCk_GroundNav_TileCoord>{};
            EnabledCoords.Reserve(InInvokerState._DesiredTileIndices.Num());
            for (const auto TileIndex : InInvokerState._DesiredTileIndices)
            { EnabledCoords.Emplace(groundnav::Get_TileCoord(ProducerBundle._DefaultField._Params._Divisions, TileIndex)); }
            const auto Result = groundnav::world_fields::Upsert_StreamSourceTiles(
                UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity), VolumeId,
                InInvokerState._Source, Transitions, EnabledCoords);
            if (NOT Result.Get_Succeeded())
            {
                // No local mask or desired publication is committed on a refused transaction. Reset
                // to the last registry-confirmed mask so the next aggregation retries this target.
                InInvokerState._DesiredTileIndices = InInvokerState._EnabledTileIndices;
                InBuildState._PendingRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
                return;
            }

            ProducerBundle._DefaultField._Epoch = Result._Epoch;
            for (auto& Variant : ProducerBundle._VariantFields)
            { Variant.Value._Epoch = Result._Epoch; }
            InBuiltField._Field = MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(ProducerBundle._DefaultField));
            InBuiltField._VariantFields.Reset();
            for (auto& Variant : ProducerBundle._VariantFields)
            { InBuiltField._VariantFields.Emplace(Variant.Key, MakeShared<const groundnav::FCk_GroundNav_Field>(MoveTemp(Variant.Value))); }
            InBuiltField._Epoch = Result._Epoch;
            InInvokerState._EnabledTileIndices = InInvokerState._DesiredTileIndices;
            InBuiltField._BakedInputFingerprint = InBuildState._BakedInputFingerprint;
            InBuiltField._BakedGeometryRevision = InBuildState._BakedGeometryRevision;
            InBuiltField._DataLayerSelector = InBuildState._ActiveDataLayerSelector;
            nav_surface::Request_NotifySurfaceRebuilt(
                UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity), Result._ChangedBounds);
            InVolumeEntity.AddOrGet<FTag_GroundNavVolume_Built>();
            InBuildState._PendingRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        auto PublishedEpoch = Completed->_Epoch;
        auto PublishedField = MoveTemp(Completed);

        // The identity moves with the field it belongs to. Taken from the build state rather than
        // recomputed here, because what this field is a statement about is what the build OPENED
        // under - a record admitted while it ran is not in the ground it baked.
        // The variants are keyed on their tags HERE, where the fields and the order they came back in
        // are both in hand, against the tag list this build BEGAN with. Rebuilt from scratch rather
        // than merged into whatever was published before, so a variant this build no longer bakes
        // leaves nothing behind under its tag.
        auto PublishedVariantFields = TMap<FGameplayTag, groundnav::FCk_GroundNav_FieldPtr>{};

        for (auto VariantIndex = 0; VariantIndex < ProfileVariantTags.Num(); ++VariantIndex)
        {
            PublishedVariantFields.Emplace(
                ProfileVariantTags[VariantIndex], CompletedFields[VariantIndex + 1]);
        }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        // A published field nobody can find from a world answers nothing: this is what the NavSurface
        // provider adapter resolves against. The variants go out in the same call under the same lock,
        // so no reader can see one half of a publish.
        auto StreamOwnerRegistered = InBuiltField.Get_IsStreamOwnerRegistered();
        if (NOT ck_groundnav_volume_processor::Publish_WholeVolume(
            World, InVolumeEntity, InParams, PublishedField, PublishedVariantFields, PublishedEpoch,
            StreamOwnerRegistered,
            groundnav::world_fields::FCk_GroundNav_PublishClaim::Geometry()))
        {
            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);
            for (const auto& RidingRepairRequest : InRepairState._RidingBuildRequests)
            { RidingRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed); }
            InRepairState._RidingBuildRequests.Reset();
            return;
        }

        InBuiltField._Epoch = PublishedEpoch;
        InBuiltField._Field = MoveTemp(PublishedField);
        InBuiltField._VariantFields = MoveTemp(PublishedVariantFields);
        InBuiltField._BakedInputFingerprint = InBuildState._BakedInputFingerprint;
        InBuiltField._BakedGeometryRevision = InBuildState._BakedGeometryRevision;
        InBuiltField._DataLayerSelector = InBuildState._ActiveDataLayerSelector;
        InBuiltField.Set_StreamOwnerRegistered(StreamOwnerRegistered);

        // The ground standing here was the cook's until this build replaced it, and it is not any
        // more. StaleCook is exactly what that means - an index exists and the field published over it
        // did not come out of it - and a Setup on a fresh volume re-loads the cook rather than
        // inheriting this. Every other status already names why the ground is not the cook's and is
        // carried through unchanged.
        if (InBuiltField._CookStatus == ECk_GroundNav_CookStatus::Cooked)
        { InBuiltField._CookStatus = ECk_GroundNav_CookStatus::StaleCook; }

        // An invalid box goes out AS IS when no tile built: bounds-unknown is the honest payload, where
        // substituting the volume's own bounds would name ground this publish never produced.
        nav_surface::Request_NotifySurfaceRebuilt(World,
            groundnav::Get_ChangedTileBounds(*InBuiltField._Field, InBuiltField._Epoch));

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_Built>();

        // The regions this build took over when it started are published now, so their callers' intent
        // holds and they complete. Anything in _PendingRequests or _PendingDirtyBounds arrived AFTER the
        // snapshot and is left exactly as it is: it names ground this build did not bake, and the repair
        // it is waiting on opens against the field just published. _InFlightRequests is empty here by
        // the drain's own rule - arming a build cancels an open repair - and is drained anyway so a
        // broken invariant strands nobody.
        for (const auto& RidingRepairRequest : InRepairState._RidingBuildRequests)
        { RidingRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded); }

        for (const auto& InFlightRepairRequest : InRepairState._InFlightRequests)
        { InFlightRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded); }

        InRepairState._RidingBuildRequests.Reset();
        InRepairState._InFlightRequests.Reset();

        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_StartRepair::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState) const
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        // A LOCAL REPAIR IS SINGLE-FIELD, so a volume holding profile variants is repaired by a full
        // rebuild instead. Repairing the default alone would leave it describing the world as it is and
        // every variant describing it as it was - one volume answering two different worlds depending
        // on which profile asked, which is the split-brain the multi-profile build's own admission rule
        // exists to forbid.
        //
        // Nothing is moved here: arming the build is enough, because a build START already takes over
        // every pending region and the requests parked behind it, and publishes their ground. The cost
        // is a whole-volume bake where a repair would have done, paid on every dirty region: a repair
        // that runs over every profile is what removes it.
        if (NOT InBuiltField.Get_VariantFields().IsEmpty())
        {
            InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsBuild>();
            return;
        }

        // Snapshotted and cleared in ONE step. The tile set a repair fixes at Begin is the set this box
        // selects and no other, so a box arriving from here on belongs to the next repair - which is
        // also why the requests that named this box move with it.
        const auto DirtyBounds = InRepairState._PendingDirtyBounds;

        InRepairState._PendingDirtyBounds = FBox{ForceInit};

        auto Parked = MoveTemp(InRepairState._PendingRequests);
        InRepairState._PendingRequests.Reset();

        const auto DoFailParked = [&]() -> void
        {
            for (const auto& ParkedRequest : Parked)
            { ParkedRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed); }

            InRepairState._Repair = {};
            InRepairState._Backend.Reset();
            InRepairState._StaleRetryCount = 0;
        };

        const auto Published = InBuiltField.Get_Field();

        // Reachable without a defect: the drain parks a region behind a build when nothing is published
        // yet, and that build can fail. There is still no field to carry the untouched tiles over from,
        // and the next build - not a repair - is what bakes this ground.
        if (NOT Published.IsValid())
        {
            groundnav::Verbose(
                TEXT("GroundNav Volume [{}] holds a dirty region [{}] but has published no field to "
                     "repair - dropping the region; a build is what bakes that ground"),
                InVolumeEntity, DirtyBounds);

            DoFailParked();
            return;
        }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        InRepairState._Backend = MakeUnique<groundnav::FCk_GroundNav_GeometryBackend_Jolt>(
            World, InBuiltField.Get_DataLayerSelector());

        const auto BackendIsUsable = InRepairState._Backend->Get_IsValid();

        // Same reasoning as the build's, and worse here: a backend that cannot answer reports no
        // geometry, so a repair accepting one would re-bake the dirty tiles as open ground exactly where
        // something was reported to have changed.
        CK_ENSURE_IF_NOT(BackendIsUsable,
            TEXT("GroundNav Volume [{}] cannot start a repair: no physics world to read geometry from"),
            InVolumeEntity)
        {
            DoFailParked();
            return;
        }

        // The markup goes in HERE and not at the request, exactly as StartBuild does it, so a repair
        // bakes its tiles against what the volume currently holds. The records the SOURCE field's params
        // carry are the ones the last publish baked with - re-baking a dirty tile under those would
        // answer the very markup change that raised the region with the records it replaced. Only the
        // records are replaced: every other field of the source params places the lattice the untouched
        // tiles were produced on.
        const auto MarkupRecords =
            Get_MarkupRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(InVolumeEntity));

        const auto BeginResult = groundnav::Request_BeginRepair(
            InRepairState._Repair, Published, DirtyBounds, InBuiltField.Get_Epoch().Get_Next(),
            MarkupRecords);

        CK_ENSURE_IF_NOT(BeginResult.Get_IsCompleted(),
            TEXT("GroundNav Volume [{}] could not begin a repair of [{}]: [{}]"),
            InVolumeEntity, DirtyBounds, BeginResult.Get_Status())
        {
            DoFailParked();
            return;
        }

        InRepairState._InFlightRequests = MoveTemp(Parked);

        // A box that reached no tile leaves the repair already whole. It is still handed to the slice
        // below rather than published from here, so there is exactly ONE place a repair publishes from.
        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_RepairInProgress>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_Repair::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        const auto BackendIsHeld = InRepairState._Backend.IsValid();

        CK_ENSURE_IF_NOT(BackendIsHeld,
            TEXT("GroundNav Volume [{}] is marked as repairing but holds no geometry backend"),
            InVolumeEntity)
        {
            DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto SliceResult = groundnav::Request_AdvanceRepair(
            *InRepairState._Backend, InParams.Get_ProbeBudgetPerTick(), InRepairState._Repair);

        if (SliceResult.Get_Status() == ECk_GroundNav_BakeStatus::BudgetExhausted)
        { return; }

        // Read before anything spends the state that holds it.
        const auto SnapshotBounds = InRepairState._Repair.Get_DirtyBounds();

        if (NOT SliceResult.Get_IsCompleted())
        {
            groundnav::Display(TEXT("GroundNav Volume [{}] failed a local repair of [{}]: [{}]"),
                InVolumeEntity, SnapshotBounds, SliceResult.Get_Status());

            DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Failed);
            DoRetain_FailedRegion(InVolumeEntity, InRepairState, SnapshotBounds);

            return;
        }

        // A repair carried every tile outside its box across from the ONE field it opened against.
        // Publishing it over a field somebody swapped in meanwhile would silently undo that publish
        // everywhere the repair did not look, which is the one corruption this representation is
        // otherwise unable to express.
        const auto SourceIsStillPublished = InRepairState._Repair.Get_Source() == InBuiltField.Get_Field();

        if (NOT SourceIsStillPublished)
        {
            groundnav::Display(
                TEXT("GroundNav Volume [{}] lost a race on its local repair of [{}]: the field it opened "
                     "against was replaced while it was slicing, so the repaired field is dropped and the "
                     "region raised again"),
                InVolumeEntity, SnapshotBounds);

            DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Failed);
            DoRetain_FailedRegion(InVolumeEntity, InRepairState, SnapshotBounds);

            return;
        }

        auto Repaired = groundnav::Request_ReleaseRepairedField(InRepairState._Repair);

        const auto RepairProducedAField = Repaired.IsValid();

        CK_ENSURE_IF_NOT(RepairProducedAField,
            TEXT("GroundNav Volume [{}] completed a local repair of [{}] that yielded no field"),
            InVolumeEntity, SnapshotBounds)
        {
            DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Failed);
            DoRetain_FailedRegion(InVolumeEntity, InRepairState, SnapshotBounds);

            return;
        }

        InRepairState._StaleRetryCount = 0;

        // A box that reached no tile re-bakes nothing and moves no epoch. Swapping a byte-identical
        // field in would announce a rebuild that changed no ground, so the honest answer is the one the
        // cost derive gives when nothing it restamped moved: succeed, and publish nothing.
        if (Repaired->_Epoch.Get_IsNewerThan(InBuiltField.Get_Epoch()))
        {
            // The same swap the build publishes through: what is out stays out, whole, for whoever holds it.
            auto PublishedEpoch = Repaired->_Epoch;
            auto PublishedField = MoveTemp(Repaired);
            auto PublishedVariantFields = InBuiltField._VariantFields;

            // A repair re-bakes ground against the backend it has been slicing with, so it can answer
            // both halves: the records it published with, and the revision that backend is at.
            const auto Identity = Get_PublishedIdentity(
                InVolumeEntity, InParams, InRepairState._Backend->Get_WorldRevision(),
                InBuiltField.Get_DataLayerSelector());

            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

            // The variant map is empty by StartRepair's own rule - a volume holding one is rebuilt
            // rather than repaired - and rides along so this is the same whole swap the build publishes
            // through.
            auto StreamOwnerRegistered = InBuiltField.Get_IsStreamOwnerRegistered();
            if (NOT ck_groundnav_volume_processor::Publish_WholeVolume(
                World, InVolumeEntity, InParams, PublishedField, PublishedVariantFields, PublishedEpoch,
                StreamOwnerRegistered,
                groundnav::world_fields::FCk_GroundNav_PublishClaim::Geometry()))
            {
                DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Failed);
                return;
            }

            InBuiltField._Epoch = PublishedEpoch;
            InBuiltField._Field = MoveTemp(PublishedField);
            InBuiltField._VariantFields = MoveTemp(PublishedVariantFields);
            InBuiltField._BakedInputFingerprint = Identity._InputFingerprint;
            InBuiltField._BakedGeometryRevision = Identity._GeometryRevision;
            InBuiltField.Set_StreamOwnerRegistered(StreamOwnerRegistered);

            // Only the re-baked tiles carry the new epoch, so this names exactly the ground the repair
            // touched and nothing besides.
            nav_surface::Request_NotifySurfaceRebuilt(World,
                groundnav::Get_ChangedTileBounds(*InBuiltField._Field, InBuiltField._Epoch));
        }

        DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Succeeded);

        // Arrived while this repair was slicing, so it names ground this repair never re-read.
        if (InRepairState._PendingDirtyBounds.IsValid != 0)
        { InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsRepair>(); }
    }

    auto
        FProcessor_GroundNavVolume_Repair::
        DoEnd(
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            ECk_Request_OperationResult InResult)
        -> void
    {
        InVolumeEntity.Remove<FTag_GroundNavVolume_RepairInProgress>();

        InRepairState._Backend.Reset();
        InRepairState._Repair = {};

        for (const auto& InFlightRequest : InRepairState._InFlightRequests)
        { InFlightRequest.TryFireCompletion(InVolumeEntity, InResult); }

        InRepairState._InFlightRequests.Reset();
    }

    auto
        FProcessor_GroundNavVolume_Repair::
        DoRetain_FailedRegion(
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FBox& InFailedBounds)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        const auto RegionMayBeRetried = InRepairState._StaleRetryCount == 0;

        // Fail-closed with a BOUNDED escape. A region that fails twice running is failing for a reason
        // another attempt does not address, and one re-raised forever would open the same doomed repair
        // every tick for the life of the volume.
        CK_ENSURE_IF_NOT(RegionMayBeRetried,
            TEXT("GroundNav Volume [{}] failed a local repair of [{}] twice in a row - dropping the "
                 "region. The ground it covers keeps whatever the last successful bake published, which "
                 "is no longer trustworthy; a full build is what recovers it"),
            InVolumeEntity, InFailedBounds)
        {
            InRepairState._StaleRetryCount = 0;
            return;
        }

        ++InRepairState._StaleRetryCount;

        InRepairState._PendingDirtyBounds =
            Get_UnionedBounds(InRepairState._PendingDirtyBounds, InFailedBounds);

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsRepair>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_MarkupCostDerive::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Markup& InMarkup,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        const auto Published = InBuiltField.Get_Field();

        // Nothing published is nothing to derive from, and nothing to repair either: the records live
        // on the volume, so a build that STARTS after them prices them through its own params. A build
        // already in flight snapshotted its records before these arrived.
        if (NOT Published.IsValid())
        { return; }

        const auto Records = Get_MarkupRecordsOf(InMarkup.Get_Entries());

        // ONE epoch for the whole pass, and the same one for every profile. Whichever fields move take
        // it; the ones that do not keep the epoch they already carried, which is what makes "did this
        // field move" a comparison against its OWN epoch rather than against the volume's newest.
        const auto NextEpoch = InBuiltField.Get_Epoch().Get_Next();

        const auto Derived = groundnav::Get_FieldWithMarkupCost(*Published, Records, NextEpoch);

        const auto DeriveProducedAField = Derived.Value.Get_IsCompleted() && Derived.Key.IsValid();

        CK_ENSURE_IF_NOT(DeriveProducedAField,
            TEXT("GroundNav Volume [{}] could not derive a cost-only field from what it has published: [{}]"),
            InVolumeEntity, Derived.Value.Get_Status())
        { return; }

        // The records are the VOLUME's, so every profile's field is priced with the same ones: a paint
        // that made ground dear for one class of walker and left it cheap for another would be a cost
        // nobody authored. Only the walkable set differs between the profiles, and pricing does not
        // decide that.
        auto MovedVariants = TMap<FGameplayTag, groundnav::FCk_GroundNav_FieldPtr>{};

        for (const auto& VariantField : InBuiltField.Get_VariantFields())
        {
            if (NOT VariantField.Value.IsValid())
            { continue; }

            const auto DerivedVariant =
                groundnav::Get_FieldWithMarkupCost(*VariantField.Value, Records, NextEpoch);

            const auto VariantDeriveProducedAField =
                DerivedVariant.Value.Get_IsCompleted() && DerivedVariant.Key.IsValid();

            CK_ENSURE_IF_NOT(VariantDeriveProducedAField,
                TEXT("GroundNav Volume [{}] could not derive a cost-only field for profile [{}] from what "
                     "it has published: [{}]"),
                InVolumeEntity, VariantField.Key, DerivedVariant.Value.Get_Status())
            { return; }

            if (NOT DerivedVariant.Key->_Epoch.Get_IsNewerThan(VariantField.Value->_Epoch))
            { continue; }

            MovedVariants.Emplace(VariantField.Key, DerivedVariant.Key);
        }

        // A restamp that lands on the labels already published moves no epoch, so there is nothing for
        // a reader to notice and nothing worth swapping a pointer for. ANY field moving is enough,
        // though: a change that reached only one profile's ground still reached ground somebody walks
        // on, and leaving it unpublished would strand that profile on labels nothing will restamp again.
        const auto DefaultMoved = Derived.Key->_Epoch.Get_IsNewerThan(Published->_Epoch);

        if (NOT DefaultMoved && MovedVariants.IsEmpty())
        { return; }

        // The same swap the build publishes through: what is out stays out, whole, for whoever holds it.
        auto ChangedBounds = FBox{ForceInit};
        auto PublishedField = InBuiltField._Field;
        auto PublishedVariantFields = InBuiltField._VariantFields;
        auto PublishedEpoch = NextEpoch;

        if (DefaultMoved)
        {
            PublishedField = Derived.Key;

            ChangedBounds = Get_UnionedBounds(ChangedBounds,
                groundnav::Get_ChangedTileBounds(*Derived.Key, NextEpoch));
        }

        for (const auto& MovedVariant : MovedVariants)
        {
            PublishedVariantFields.Emplace(MovedVariant.Key, MovedVariant.Value);

            ChangedBounds = Get_UnionedBounds(ChangedBounds,
                groundnav::Get_ChangedTileBounds(*MovedVariant.Value, NextEpoch));
        }

        // The NEWEST epoch across every field the volume holds. Anything that moved took NextEpoch, and
        // NextEpoch is past every epoch the unmoved fields still carry, so it is that maximum.
        // A derive re-labels ground that is already published and reads no geometry, so the revision
        // that ground was baked against is carried FORWARD unchanged. What did move is the record list
        // this pass published with, which is the half refreshed here.
        const auto Identity = Get_PublishedIdentity(
            InVolumeEntity, InVolumeEntity.Get<FFragment_GroundNavVolume_Params>(),
            InBuiltField._BakedGeometryRevision, InBuiltField.Get_DataLayerSelector());

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        // Cost markup preserves geometry. The invalidator only repaths a cached route when its saved
        // filter now denies a plate the route used.
        auto StreamOwnerRegistered = InBuiltField.Get_IsStreamOwnerRegistered();
        if (NOT ck_groundnav_volume_processor::Publish_WholeVolume(
            World, InVolumeEntity, InVolumeEntity.Get<FFragment_GroundNavVolume_Params>(),
            PublishedField, PublishedVariantFields, PublishedEpoch,
            StreamOwnerRegistered,
            groundnav::world_fields::FCk_GroundNav_PublishClaim::CostOnly()))
        { return; }
        InBuiltField._Field = MoveTemp(PublishedField);
        InBuiltField._VariantFields = MoveTemp(PublishedVariantFields);
        InBuiltField._Epoch = PublishedEpoch;
        InBuiltField._BakedInputFingerprint = Identity._InputFingerprint;
        InBuiltField.Set_StreamOwnerRegistered(StreamOwnerRegistered);

        // Notify every changed field. Cached corridors keep their route unless their saved filter now
        // denies a used plate; in-flight searches still arm against the replaced prices.
        nav_surface::Request_NotifySurfaceRebuilt(World, ChangedBounds);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_LinkDerive::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Links& InLinks,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        // A build or a repair that is RUNNING, or a repair that is armed, publishes a field that does not
        // know the entries that landed after it snapshotted them - a build snapshots at start and a
        // repair inherits its links from the source field's params - so the tag is KEPT and this runs
        // again in the tick that publish lands, after it, against the field it produced.
        const auto APublishInFlightWouldMissTheEntries =
            InVolumeEntity.Has<FTag_GroundNavVolume_BuildInProgress>() ||
            InVolumeEntity.Has<FTag_GroundNavVolume_NeedsRepair>() ||
            InVolumeEntity.Has<FTag_GroundNavVolume_RepairInProgress>();

        if (APublishInFlightWouldMissTheEntries)
        { return; }

        InVolumeEntity.Remove<MarkedDirtyBy>();

        const auto Published = InBuiltField.Get_Field();

        // Nothing published is nothing to derive from: the records live on the volume, so the build
        // that publishes first resolves them through its own FCk_GroundNav_FieldParams::_Links. That is
        // the same wait the cost derive makes, and the reason a link authored before the first bake is
        // never lost.
        if (NOT Published.IsValid())
        { return; }

        // A build that is armed but not started snapshots the volume's entries when it starts, so it
        // resolves these itself between the seam portals and the reachability labels; deriving over the
        // field it is about to replace would spend the work twice and publish the older answer second.
        if (InVolumeEntity.Has<FTag_GroundNavVolume_NeedsBuild>())
        { return; }

        const auto Records = Get_LinkRecordsOf(InLinks.Get_Entries());

        // ONE epoch for the whole pass, on the same terms the cost derive takes one: whichever fields
        // move take it, and a field that did not keeps its own.
        const auto NextEpoch = InBuiltField.Get_Epoch().Get_Next();

        const auto Derived = groundnav::Get_FieldWithLinks(*Published, Records, NextEpoch);

        const auto DeriveProducedAField = Derived._Result.Get_IsCompleted() && Derived._Field.IsValid();

        CK_ENSURE_IF_NOT(DeriveProducedAField,
            TEXT("GroundNav Volume [{}] could not derive a link-only field from what it has published: [{}]"),
            InVolumeEntity, Derived._Result.Get_Status())
        { return; }

        // The records are the VOLUME's, so every profile resolves the same authored links - but not
        // necessarily to the same answer: an end that finds ground for one profile can find none for
        // another, because what is standable is exactly what a profile changes.
        auto MovedVariants = TMap<FGameplayTag, groundnav::FCk_GroundNav_FieldPtr>{};
        auto ChangedLinkIds = Derived._ChangedLinkIds;

        for (const auto& VariantField : InBuiltField.Get_VariantFields())
        {
            if (NOT VariantField.Value.IsValid())
            { continue; }

            const auto DerivedVariant =
                groundnav::Get_FieldWithLinks(*VariantField.Value, Records, NextEpoch);

            const auto VariantDeriveProducedAField =
                DerivedVariant._Result.Get_IsCompleted() && DerivedVariant._Field.IsValid();

            CK_ENSURE_IF_NOT(VariantDeriveProducedAField,
                TEXT("GroundNav Volume [{}] could not derive a link-only field for profile [{}] from what "
                     "it has published: [{}]"),
                InVolumeEntity, VariantField.Key, DerivedVariant._Result.Get_Status())
            { return; }

            if (NOT DerivedVariant._Field->_Epoch.Get_IsNewerThan(VariantField.Value->_Epoch))
            { continue; }

            MovedVariants.Emplace(VariantField.Key, DerivedVariant._Field);

            // The note narrows by link IDENTITY and the identity is the volume's, not a profile's, so
            // an id that moved on any one field is an id this publish changed. Narrower than the union
            // would be a note that failed to mention something it changed.
            for (const auto ChangedLinkId : DerivedVariant._ChangedLinkIds)
            { ChangedLinkIds.AddUnique(ChangedLinkId); }
        }

        // A re-resolution that lands on what is already published moves no epoch, so there is nothing
        // for a reader to notice and nothing worth swapping a pointer for. ANY field moving is enough,
        // on the same terms the cost derive publishes under.
        const auto DefaultMoved = Derived._Field->_Epoch.Get_IsNewerThan(Published->_Epoch);

        if (NOT DefaultMoved && MovedVariants.IsEmpty())
        { return; }

        // The same swap the build publishes through: what is out stays out, whole, for whoever holds it.
        auto ChangedBounds = FBox{ForceInit};
        auto PublishedField = InBuiltField._Field;
        auto PublishedVariantFields = InBuiltField._VariantFields;
        auto PublishedEpoch = NextEpoch;

        if (DefaultMoved)
        {
            PublishedField = Derived._Field;

            ChangedBounds = Get_UnionedBounds(ChangedBounds,
                groundnav::Get_ChangedTileBounds(*Derived._Field, NextEpoch));
        }

        for (const auto& MovedVariant : MovedVariants)
        {
            PublishedVariantFields.Emplace(MovedVariant.Key, MovedVariant.Value);

            ChangedBounds = Get_UnionedBounds(ChangedBounds,
                groundnav::Get_ChangedTileBounds(*MovedVariant.Value, NextEpoch));
        }

        // The NEWEST epoch across every field the volume holds, for the same reason the cost derive
        // takes it.
        // Carried forward and refreshed on the same terms the cost derive publishes under: no geometry
        // was read, so the revision stands, and the record list that moved is the half restamped.
        const auto Identity = Get_PublishedIdentity(
            InVolumeEntity, InVolumeEntity.Get<FFragment_GroundNavVolume_Params>(),
            InBuiltField._BakedGeometryRevision, InBuiltField.Get_DataLayerSelector());

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        // The one publisher that can say its ground did not move: this pass re-resolves links and
        // re-labels and touches no geometry, so the ids below name EVERYTHING it changed. A reader that
        // cached which links its route used can answer exactly here, where the endpoint-tile bounds
        // beneath can only answer "a route through this tile". The registry accumulates them onto
        // whatever run is open, so a reader that has missed several of these still reads one list.
        auto StreamOwnerRegistered = InBuiltField.Get_IsStreamOwnerRegistered();
        if (NOT ck_groundnav_volume_processor::Publish_WholeVolume(
            World, InVolumeEntity, InVolumeEntity.Get<FFragment_GroundNavVolume_Params>(),
            PublishedField, PublishedVariantFields, PublishedEpoch,
            StreamOwnerRegistered,
            groundnav::world_fields::FCk_GroundNav_PublishClaim::LinkOnly(MoveTemp(ChangedLinkIds))))
        { return; }
        InBuiltField._Field = MoveTemp(PublishedField);
        InBuiltField._VariantFields = MoveTemp(PublishedVariantFields);
        InBuiltField._Epoch = PublishedEpoch;
        InBuiltField._BakedInputFingerprint = Identity._InputFingerprint;
        InBuiltField.Set_StreamOwnerRegistered(StreamOwnerRegistered);

        // Past the no-change early-out above, so this notify is only ever raised for a publish that
        // moved something. The UNION over every field that moved. An invalid box is reported as-is for
        // the same reason the build reports one, and the note above narrows past it rather than
        // replacing it: bounds stay the floor.
        nav_surface::Request_NotifySurfaceRebuilt(World, ChangedBounds);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            const FFragment_GroundNavVolume_Requests& InRequests)
        -> void
    {
        // Two separate populations, and missing either one strands a caller. The QUEUE holds requests
        // the drain never reached; _PendingRequest holds the delegate that has been riding the
        // multi-tick build and is the one a caller is most likely actually waiting on.
        request::FireCancelledForPending(InVolumeEntity, InRequests.Get_Requests());

        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);

        // Drops the pinned physics session with the entity rather than leaving it to fragment teardown.
        InBuildState._Backend.Reset();
        InBuildState._HasPendingBuildRequest = false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_CancelPendingRepairRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FFragment_GroundNavVolume_RepairRequests& InRequests)
        -> void
    {
        // Four separate populations, and missing any one strands a caller. The QUEUE holds requests the
        // drain never reached, _PendingRequests those it parked against a repair that will now never
        // open, _InFlightRequests the delegates riding one mid-slice, and _RidingBuildRequests those a
        // build took over at its start and will now never publish for.
        request::FireCancelledForPending(InVolumeEntity, InRequests.Get_Requests());

        for (const auto& ParkedRequest : InRepairState._PendingRequests)
        {
            ParkedRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);
        }

        for (const auto& InFlightRequest : InRepairState._InFlightRequests)
        {
            InFlightRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);
        }

        for (const auto& RidingRequest : InRepairState._RidingBuildRequests)
        {
            RidingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);
        }

        InRepairState._PendingRequests.Reset();
        InRepairState._InFlightRequests.Reset();
        InRepairState._RidingBuildRequests.Reset();

        // Drops the pinned physics session with the entity rather than leaving it to fragment teardown.
        InRepairState._Backend.Reset();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_CancelPendingMarkupRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_MarkupRequests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InVolumeEntity, InRequests.Get_Requests());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_CancelPendingLinkRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_LinkRequests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InVolumeEntity, InRequests.Get_Requests());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_Unpublish::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField)
        -> void
    {
        // Whoever holds the field keeps it, whole; what ends here is only the world's way of finding it.
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);
        const auto VolumeId = groundnav::FCk_GroundNav_VolumeId{InParams.Get_StreamingVolumeId()};
        if (VolumeId.Get_IsLegacy())
        {
            groundnav::world_fields::Unpublish(World, InVolumeEntity);
            return;
        }

        const auto Result = InParams.Get_StreamingBuildScope() == ECk_GroundNav_StreamingBuildScope::ManifestDriven
            ? groundnav::stream_partitions::Purge_OwnerPartitions(World, InVolumeEntity, VolumeId)._Registry
            : groundnav::world_fields::Unregister_StreamOwner(World, InVolumeEntity, VolumeId);
        const auto TeardownSucceeded = Result.Get_Succeeded() ||
            Result._Status == groundnav::world_fields::ECk_GroundNav_StreamRegistryStatus::OwnerNotRegistered;
        CK_ENSURE_IF_NOT(TeardownSucceeded,
            TEXT("GroundNav streamed Volume [{}] could not unregister its stream owner: registry status [{}]"),
            InVolumeEntity, static_cast<int32>(Result._Status))
        {}
    }
}

// --------------------------------------------------------------------------------------------------------------------
