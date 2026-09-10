#include "CkGroundNavEditor/Cook/CkGroundNavCook_FieldCooker.h"

#include "../../../CkGroundNavEditor_Log.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend_Jolt.h"
#include "CkGroundNav/Bake/CkGroundNav_Fingerprint.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedFieldLoad.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedSourceManifest.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Field/CkGroundNav_FieldSerialize.h"
#include "CkGroundNav/Field/CkGroundNav_TileBake.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Utils.h"
#include "CkJoltEditor/Cook/CkJoltCook_AssetSave.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h"

#include <UObject/Package.h>
#include <UObject/SoftObjectPath.h>
#include <Math/NumericLimits.h>
#include <Misc/PackageName.h>
#include <Algo/AllOf.h>
#include <Algo/Sort.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::cook
{
    namespace groundnav_cook_private
    {
        auto Get_FieldFingerprint(const FCk_Fragment_GroundNavVolume_ParamsData& InParams) -> uint64
        {
            auto Variants = TArray<TPair<FName, FCk_GroundNav_AgentProfile>>{};
            Variants.Reserve(InParams.Get_ProfileVariants().Num());

            for (const auto& Variant : InParams.Get_ProfileVariants())
            { Variants.Emplace(Variant.Get_ProfileTag().GetTagName(), Variant.Get_Profile()); }

            return Get_InputFingerprint(InParams.Get_VolumeBounds(), InParams.Get_Config(), InParams.Get_Profile(),
                {}, {}, InParams.Get_MergeTunables(), InParams.Get_MaxClearanceUu(), Variants,
                InParams.Get_DataLayerSelector())._Value;
        }

        auto Save_Plan(const FCk_GroundNav_CookFieldPlan& InPlan) -> bool
        {
            auto TileRefs = TArray<TSoftObjectPtr<UCk_GroundNav_CookedTile_UE>>{};
            TileRefs.Reserve(InPlan._Tiles.Num());

            for (const auto& TilePlan : InPlan._Tiles)
            {
                const auto TilePath = Get_CookedTileAssetPath(kCookedDataRootPath,
                    InPlan._SourceLevelPackage.ToString(), InPlan._CookKey, TilePlan._Coord, InPlan._ProfileTag,
                    InPlan._DataLayerSelector);
                auto* Package = CreatePackage(*FPackageName::ObjectPathToPackageName(TilePath));
                const auto PackageIsValid = ck::IsValid(Package);
                CK_ENSURE_IF_NOT(PackageIsValid, TEXT("GroundNavCook: could not create tile package [{}]"), TilePath)
                { }
                if (NOT PackageIsValid)
                { return false; }

                auto* Tile = NewObject<UCk_GroundNav_CookedTile_UE>(Package,
                    *FPackageName::ObjectPathToObjectName(TilePath), RF_Public | RF_Standalone);

                const auto TileIsValid = ck::IsValid(Tile);
                CK_ENSURE_IF_NOT(TileIsValid, TEXT("GroundNavCook: could not create tile [{}]"), TilePath)
                { }
                if (NOT TileIsValid)
                { return false; }

                Tile->Set_FormatVersion(kFieldBlobFormatVersion);
                Tile->Set_TileCoord(TilePlan._Coord);
                Tile->Set_StreamingVolumeId(InPlan._StreamingVolumeId);
                Tile->Set_WorldBounds(TilePlan._WorldBounds);
                Tile->Set_DataLayerNames(InPlan._DataLayerSelector.Get_LayerNames());
                Tile->Set_Fingerprint(InPlan._Fingerprint);
                Tile->Set_ProfileTag(InPlan._ProfileTag);
                Tile->Set_LatticeKey(InPlan._LatticeKey);
                Tile->Set_Blob(TilePlan._Blob);
                Tile->Set_ContentHash(Get_CookedTileContentHash(Tile->Get_Blob()));

                if (NOT ck::jolt::cook::Save_CookedAsset(*Tile))
                { return false; }

                TileRefs.Emplace(Tile);
            }

            const auto IndexPath = Get_CookedIndexAssetPath(kCookedDataRootPath,
                InPlan._SourceLevelPackage.ToString(), InPlan._CookKey, InPlan._ProfileTag,
                InPlan._DataLayerSelector);
            auto* IndexPackage = CreatePackage(*FPackageName::ObjectPathToPackageName(IndexPath));
            const auto IndexPackageIsValid = ck::IsValid(IndexPackage);
            CK_ENSURE_IF_NOT(IndexPackageIsValid,
                TEXT("GroundNavCook: could not create index package [{}]"), IndexPath)
            { }
            if (NOT IndexPackageIsValid)
            { return false; }

            auto* Index = NewObject<UCk_GroundNav_CookedFieldIndex_UE>(IndexPackage,
                *FPackageName::ObjectPathToObjectName(IndexPath), RF_Public | RF_Standalone);

            const auto IndexIsValid = ck::IsValid(Index);
            CK_ENSURE_IF_NOT(IndexIsValid, TEXT("GroundNavCook: could not create index [{}]"), IndexPath)
            { }
            if (NOT IndexIsValid)
            { return false; }

            Index->Set_LevelPackage(InPlan._SourceLevelPackage);
            Index->Set_CookKey(InPlan._CookKey);
            Index->Set_StreamingVolumeId(InPlan._StreamingVolumeId);
            Index->Set_DataLayerNames(InPlan._DataLayerSelector.Get_LayerNames());
            Index->Set_ProfileTag(InPlan._ProfileTag);
            Index->Set_Fingerprint(InPlan._Fingerprint);
            Index->Set_FormatVersion(kFieldBlobFormatVersion);
            Index->Set_LatticeKey(InPlan._LatticeKey);
            Index->Set_Tiles(MoveTemp(TileRefs));

            return ck::jolt::cook::Save_CookedAsset(*Index);
        }

        auto Get_PlanPathsAreValid(const FCk_GroundNav_CookFieldPlan& InPlan) -> bool
        {
            const auto HasValidStreamingIdentity = InPlan._StreamingVolumeId == INDEX_NONE ||
                InPlan._StreamingVolumeId > 0;
            if (NOT HasValidStreamingIdentity || NOT InPlan._DataLayerSelector.Get_IsCanonical())
            { return false; }

            const auto IndexPath = Get_CookedIndexAssetPath(kCookedDataRootPath,
                InPlan._SourceLevelPackage.ToString(), InPlan._CookKey, InPlan._ProfileTag,
                InPlan._DataLayerSelector);
            if (NOT FPackageName::IsValidObjectPath(IndexPath))
            { return false; }

            for (const auto& Tile : InPlan._Tiles)
            {
                if (NOT Tile._WorldBounds.IsValid || Get_CookedTileContentHash(Tile._Blob) == 0)
                { return false; }

                const auto TilePath = Get_CookedTileAssetPath(kCookedDataRootPath,
                    InPlan._SourceLevelPackage.ToString(), InPlan._CookKey, Tile._Coord, InPlan._ProfileTag,
                    InPlan._DataLayerSelector);
                if (NOT FPackageName::IsValidObjectPath(TilePath))
                { return false; }
            }

            return true;
        }

        auto Get_IsStrictRowMajor(TConstArrayView<FIntPoint> InCoords) -> bool
        {
            for (auto Index = 1; Index < InCoords.Num(); ++Index)
            {
                const auto& Previous = InCoords[Index - 1];
                const auto& Current = InCoords[Index];
                if (Current.Y < Previous.Y || (Current.Y == Previous.Y && Current.X <= Previous.X))
                { return false; }
            }

            return true;
        }

        auto Get_SourceManifestAssetPath(
            const FCk_GroundNav_CookFieldPlan& InDefaultPlan,
            int32 InPartitionId)
            -> FString
        {
            return Get_CookedSourceManifestAssetPath(
                InDefaultPlan._SourceLevelPackage, InDefaultPlan._CookKey,
                InDefaultPlan._DataLayerSelector, InPartitionId);
        }

        auto Get_AreFieldPlansCompleteAndCompatible(
            TConstArrayView<FCk_GroundNav_CookFieldPlan> InPlans)
            -> bool
        {
            if (InPlans.IsEmpty())
            { return false; }

            const auto* DefaultPlan = InPlans.FindByPredicate([](const auto& InPlan)
            { return NOT InPlan._ProfileTag.IsValid(); });
            if (DefaultPlan == nullptr || DefaultPlan->_StreamingVolumeId <= 0 || DefaultPlan->_SourceLevelPackage.IsNone() ||
                DefaultPlan->_CookKey.IsNone() || NOT DefaultPlan->_DataLayerSelector.Get_IsCanonical() ||
                NOT Get_PlanPathsAreValid(*DefaultPlan))
            { return false; }

            auto ProfileTags = TSet<FGameplayTag>{};
            for (const auto& Plan : InPlans)
            {
                const auto MatchesIdentity = Plan._SourceLevelPackage == DefaultPlan->_SourceLevelPackage &&
                    Plan._CookKey == DefaultPlan->_CookKey &&
                    Plan._StreamingVolumeId == DefaultPlan->_StreamingVolumeId &&
                    Plan._DataLayerSelector.Get_LayerNames() == DefaultPlan->_DataLayerSelector.Get_LayerNames() &&
                    Plan._LatticeKey == DefaultPlan->_LatticeKey && Get_PlanPathsAreValid(Plan);
                if (NOT MatchesIdentity || ProfileTags.Contains(Plan._ProfileTag))
                { return false; }
                ProfileTags.Add(Plan._ProfileTag);
            }

            return true;
        }

        auto Get_IsManifestPlanValid(const FCk_GroundNav_CookSourceManifestPlan& InPlan) -> bool
        {
            if (InPlan._StreamingVolumeId <= 0 || InPlan._PartitionId <= 0 ||
                InPlan._SourceLevelPackage.IsNone() || InPlan._CookKey.IsNone() ||
                NOT InPlan._DataLayerSelector.Get_IsCanonical() || InPlan._TileCoords.IsEmpty() ||
                NOT Get_IsStrictRowMajor(InPlan._TileCoords) ||
                NOT FPackageName::IsValidObjectPath(InPlan._AssetPath) ||
                InPlan._ProfileTags.IsEmpty() || InPlan._ProfileTags.Num() != InPlan._ProfileFingerprints.Num() ||
                InPlan._ProfileTags.Num() != InPlan._ProfileTilePaths.Num() || InPlan._ProfileTags[0].IsValid())
            { return false; }

            auto ProfileTags = TSet<FGameplayTag>{};
            for (auto ProfileIndex = 0; ProfileIndex < InPlan._ProfileTags.Num(); ++ProfileIndex)
            {
                const auto ProfileIsNew = !ProfileTags.Contains(InPlan._ProfileTags[ProfileIndex]);
                const auto& TilePaths = InPlan._ProfileTilePaths[ProfileIndex];
                if (NOT ProfileIsNew || InPlan._ProfileFingerprints[ProfileIndex] == 0 ||
                    TilePaths.Num() != InPlan._TileCoords.Num())
                { return false; }
                ProfileTags.Add(InPlan._ProfileTags[ProfileIndex]);

                for (const auto& TilePath : TilePaths)
                {
                    if (NOT FPackageName::IsValidObjectPath(TilePath))
                    { return false; }
                }
            }

            return true;
        }

        auto Get_AreManifestPlansValidForSave(
            TConstArrayView<FCk_GroundNav_CookSourceManifestPlan> InPlans)
            -> bool
        {
            if (InPlans.IsEmpty())
            { return false; }

            auto ManifestPaths = TSet<FString>{};
            auto SourceIdentities = TSet<FString>{};
            auto ClaimedTiles = TSet<FString>{};

            for (const auto& Plan : InPlans)
            {
                if (NOT Get_IsManifestPlanValid(Plan) || ManifestPaths.Contains(Plan._AssetPath))
                { return false; }
                ManifestPaths.Add(Plan._AssetPath);

                const auto SourceIdentity = FString::Printf(TEXT("%d|%d"),
                    Plan._StreamingVolumeId, Plan._PartitionId);
                if (SourceIdentities.Contains(SourceIdentity))
                { return false; }
                SourceIdentities.Add(SourceIdentity);

                for (const auto& Coord : Plan._TileCoords)
                {
                    const auto TileIdentity = FString::Printf(TEXT("%d|%d|%d"),
                        Plan._StreamingVolumeId, Coord.X, Coord.Y);
                    if (ClaimedTiles.Contains(TileIdentity))
                    { return false; }
                    ClaimedTiles.Add(TileIdentity);
                }

                for (const auto& ProfileTilePaths : Plan._ProfileTilePaths)
                {
                    for (const auto& TilePath : ProfileTilePaths)
                    {
                        const auto TilePackagePath = FPackageName::ObjectPathToPackageName(TilePath);
                        if (NOT FPackageName::DoesPackageExist(TilePackagePath))
                        { return false; }
                    }
                }
            }

            return true;
        }
    }

    auto
        FCk_GroundNav_FieldCooker::
        Prepare_WorldGeometry(
            UWorld& InWorld)
        -> bool
    {
        auto* StaticWorld = InWorld.GetSubsystem<UCk_JoltStaticWorld_Subsystem_UE>();
        const auto StaticWorldIsValid = ck::IsValid(StaticWorld);
        CK_ENSURE_IF_NOT(StaticWorldIsValid,
            TEXT("GroundNavCook: editor Jolt static-world subsystem is unavailable"))
        { }
        if (NOT StaticWorldIsValid)
        { return false; }

        StaticWorld->Request_EnsureSwept();
        return true;
    }

    auto
        FCk_GroundNav_FieldCooker::
        Get_AreCookIdentitiesUnique(
            TConstArrayView<FCk_GroundNav_CookIdentity> InIdentities)
        -> bool
    {
        auto SeenCookAssets = TSet<FString>{};
        auto SeenStreamVolumes = TSet<FString>{};

        for (const auto& Identity : InIdentities)
        {
            const auto HasValidStreamingIdentity = Identity._StreamingVolumeId == INDEX_NONE ||
                Identity._StreamingVolumeId > 0;
            if (Identity._SourceLevelPackage.IsNone() || Identity._CookKey.IsNone() ||
                NOT HasValidStreamingIdentity || NOT Identity._DataLayerSelector.Get_IsCanonical())
            { return false; }

            // Selector-specific paths preserve successive cook variants, but do not create a second
            // live volume owner. {source level, cook key} remains the authored placement identity.
            const auto CookAssetKey = FString::Printf(TEXT("%s|%s"),
                *Identity._SourceLevelPackage.ToString(), *Identity._CookKey.ToString());
            if (SeenCookAssets.Contains(CookAssetKey))
            { return false; }
            SeenCookAssets.Add(CookAssetKey);

            // A positive streaming id identifies the world-side source owner, independent of the
            // cooked asset path. Two placed volumes in one source level must not silently claim it
            // under different cook keys or layer selectors. Separate source levels remain distinct
            // sources and the runtime 7C ownership transaction judges their coordinates on load.
            if (Identity._StreamingVolumeId > 0)
            {
                const auto StreamVolumeKey = FString::Printf(TEXT("%s|%d"),
                    *Identity._SourceLevelPackage.ToString(), Identity._StreamingVolumeId);
                if (SeenStreamVolumes.Contains(StreamVolumeKey))
                { return false; }
                SeenStreamVolumes.Add(StreamVolumeKey);
            }
        }

        return true;
    }

    auto
        FCk_GroundNav_FieldCooker::
        Cook_Volume(
            UWorld&                                        InWorld,
            const FCk_Fragment_GroundNavVolume_ParamsData& InParams,
            FName                                          InSourceLevelPackage,
            ECk_GroundNav_CookMode                         InMode,
            TArray<FCk_GroundNav_CookFieldPlan>&           OutPlans,
            const ICk_GroundNav_GeometryBackend*          InGeometryBackend)
        -> FCk_GroundNav_CookFieldStats
    {
        using namespace groundnav_cook_private;

        auto Stats = FCk_GroundNav_CookFieldStats{};
        OutPlans.Reset();

        const auto IsCookable = NOT InParams.Get_CookKey().IsNone() && NOT InSourceLevelPackage.IsNone();
        CK_ENSURE_IF_NOT(IsCookable, TEXT("GroundNavCook: volume needs a cook key and source level package"))
        { }
        if (NOT IsCookable)
        { return Stats; }

        auto CanonicalSelector = FCk_GroundNav_DataLayerSelector{};
        const auto SelectorIsValid = TryMake_DataLayerSelector(InParams.Get_DataLayerSelector().Get_LayerNames(), CanonicalSelector);
        const auto SelectorIsCanonical = SelectorIsValid;
        CK_ENSURE_IF_NOT(SelectorIsCanonical,
            TEXT("GroundNavCook: cook key [{}] has a non-canonical data-layer selector"), InParams.Get_CookKey())
        { }
        if (NOT SelectorIsCanonical)
        { return Stats; }

        auto CanonicalParams = InParams;
        CanonicalParams.Set_DataLayerSelector(MoveTemp(CanonicalSelector));

        const auto HasValidStreamingIdentity = CanonicalParams.Get_StreamingVolumeId() == INDEX_NONE ||
            CanonicalParams.Get_StreamingVolumeId() > 0;
        CK_ENSURE_IF_NOT(HasValidStreamingIdentity,
            TEXT("GroundNavCook: cook key [{}] has an invalid streaming volume id [{}]"),
            CanonicalParams.Get_CookKey(), CanonicalParams.Get_StreamingVolumeId())
        { }
        if (NOT HasValidStreamingIdentity)
        { return Stats; }

        auto RuntimeBackend = TUniquePtr<FCk_GroundNav_GeometryBackend_Jolt>{};
        auto* Backend = InGeometryBackend;
        if (Backend == nullptr)
        {
            RuntimeBackend = MakeUnique<FCk_GroundNav_GeometryBackend_Jolt>(
                &InWorld, CanonicalParams.Get_DataLayerSelector());
            Backend = RuntimeBackend.Get();
        }
        const auto BackendIsValid = Backend != nullptr && Backend->Get_IsValid();
        CK_ENSURE_IF_NOT(BackendIsValid, TEXT("GroundNavCook: Jolt geometry is unavailable"))
        { }
        if (NOT BackendIsValid)
        { return Stats; }

        auto FieldParams = Get_VolumeFieldParams(CanonicalParams, {}, {});
        auto FieldParamsByProfile = TArray<FCk_GroundNav_FieldParams>{FieldParams};
        auto ProfileTags = TArray<FGameplayTag>{FGameplayTag{}};
        auto SeenProfileTags = TSet<FGameplayTag>{};

        for (const auto& Variant : CanonicalParams.Get_ProfileVariants())
        {
            const auto VariantIsValid = Variant.Get_ProfileTag().IsValid();
            CK_ENSURE_IF_NOT(VariantIsValid, TEXT("GroundNavCook: cook key [{}] has an empty profile tag"),
                CanonicalParams.Get_CookKey())
            { }
            if (NOT VariantIsValid)
            { return Stats; }

            const auto IsNewProfile = !SeenProfileTags.Contains(Variant.Get_ProfileTag());
            CK_ENSURE_IF_NOT(IsNewProfile, TEXT("GroundNavCook: cook key [{}] repeats profile tag [{}]"),
                CanonicalParams.Get_CookKey(), Variant.Get_ProfileTag())
            { }
            if (NOT IsNewProfile)
            { return Stats; }

            SeenProfileTags.Add(Variant.Get_ProfileTag());

            auto VariantParams = FieldParams;
            VariantParams._Profile = Variant.Get_Profile();
            FieldParamsByProfile.Emplace(MoveTemp(VariantParams));
            ProfileTags.Emplace(Variant.Get_ProfileTag());
        }

        for (auto ProfileIndex = 0; ProfileIndex < FieldParamsByProfile.Num(); ++ProfileIndex)
        {
            const auto& ProfileParams = FieldParamsByProfile[ProfileIndex];
            const auto ParamsAreValid = ProfileParams.Get_IsValid();
            const auto ProfileRejection = Get_ProfileRejection(ProfileParams._Profile);
            const auto ProfileIsAdmissible = ProfileRejection == EProfileRejection::None;
            const auto IsAdmissible = ParamsAreValid && ProfileIsAdmissible;
            CK_ENSURE_IF_NOT(IsAdmissible,
                TEXT("GroundNavCook: cook key [{}] profile [{}] has invalid field params or agent profile"),
                CanonicalParams.Get_CookKey(), ProfileIndex)
            { }
            if (NOT IsAdmissible)
            {
                ck::groundnav_editor::Error(
                    TEXT("GroundNavCook: cook key [{}] profile [{}] rejected before bake: field params valid={}, "
                         "profile rejection={} (check bounds, bake config, clearance and standing shape)"),
                    CanonicalParams.Get_CookKey(), ProfileIndex, ParamsAreValid, static_cast<int32>(ProfileRejection));
                return Stats;
            }
        }

        auto State = FCk_GroundNav_FieldBuildState{};
        const auto BeginResult = Request_BeginBuild_MultiProfile(
            FieldParamsByProfile, FCk_GroundNav_Epoch{1}, State);
        const auto BuildWasAdmitted = BeginResult.Get_IsCompleted();
        CK_ENSURE_IF_NOT(BuildWasAdmitted, TEXT("GroundNavCook: field build admission failed for key [{}]"),
            CanonicalParams.Get_CookKey())
        { }
        if (NOT BuildWasAdmitted)
        {
            ck::groundnav_editor::Error(
                TEXT("GroundNavCook: field build admission failed for key [{}] with status {}"),
                CanonicalParams.Get_CookKey(), static_cast<int32>(BeginResult.Get_Status()));
            return Stats;
        }

        auto BakeResult = Request_AdvanceBuild(*Backend, TNumericLimits<int32>::Max(), State);
        while (BakeResult.Get_Status() == ECk_GroundNav_BakeStatus::BudgetExhausted)
        { BakeResult = Request_AdvanceBuild(*Backend, TNumericLimits<int32>::Max(), State); }

        const auto Fields = Get_CompletedFields(State);
        const auto HasEveryProfile = BakeResult.Get_IsCompleted() && Fields.Num() == ProfileTags.Num();
        CK_ENSURE_IF_NOT(HasEveryProfile, TEXT("GroundNavCook: pure bake failed for key [{}]"), CanonicalParams.Get_CookKey())
        { }
        if (NOT HasEveryProfile)
        { return Stats; }

        const auto Fingerprint = Get_FieldFingerprint(CanonicalParams);

        for (auto Index = 0; Index < Fields.Num(); ++Index)
        {
            const auto& Field = Fields[Index];
            auto Plan = FCk_GroundNav_CookFieldPlan{};
            Plan._SourceLevelPackage = InSourceLevelPackage;
            Plan._CookKey = CanonicalParams.Get_CookKey();
            Plan._StreamingVolumeId = CanonicalParams.Get_StreamingVolumeId();
            Plan._DataLayerSelector = CanonicalParams.Get_DataLayerSelector();
            Plan._ProfileTag = ProfileTags[Index];
            Plan._Fingerprint = Fingerprint;
            Plan._LatticeKey = Get_CookedLatticeKey(Field._Params);
            Write_Field(Field, Plan._SerializedField);

            for (const auto& Tile : Field._Tiles)
            {
                auto TilePlan = FCk_GroundNav_CookTilePlan{};
                TilePlan._Coord = FIntPoint{Tile._Coord._X, Tile._Coord._Y};
                TilePlan._WorldBounds = Get_TileBounds(
                    Field._Params.Get_TileBakeParams(Tile._Coord, FCk_GroundNav_Epoch{}));
                Write_Tile(Field, Tile._Coord, TilePlan._Blob);
                Plan._Tiles.Emplace(MoveTemp(TilePlan));
            }

            Stats._NumTiles += Plan._Tiles.Num();
            OutPlans.Emplace(MoveTemp(Plan));
        }

        Stats._NumFields = OutPlans.Num();

        if (InMode == ECk_GroundNav_CookMode::DryRun)
        {
            Stats._Success = true;
            return Stats;
        }

        return Save_Plans(OutPlans);
    }

    auto
        FCk_GroundNav_FieldCooker::
        Save_Plans(
            TConstArrayView<FCk_GroundNav_CookFieldPlan> InPlans)
        -> FCk_GroundNav_CookFieldStats
    {
        using namespace groundnav_cook_private;

        auto Stats = FCk_GroundNav_CookFieldStats{};
        Stats._NumFields = InPlans.Num();

        // Validate EVERY target before the first CreatePackage call. CookKey is authored FName data and
        // may contain characters that an asset path rejects; CreatePackage is not a safe validator.
        const auto AllPathsAreValid = Algo::AllOf(InPlans,
            [](const FCk_GroundNav_CookFieldPlan& InPlan) { return Get_PlanPathsAreValid(InPlan); });
        CK_ENSURE_IF_NOT(AllPathsAreValid,
            TEXT("GroundNavCook: one or more generated cooked asset paths are invalid; refusing all writes"))
        { }
        if (NOT AllPathsAreValid)
        { return Stats; }

        for (const auto& Plan : InPlans)
        {
            if (NOT Save_Plan(Plan))
            { return Stats; }

            Stats._NumTiles += Plan._Tiles.Num();
            Stats._NumAssetsWritten += Plan._Tiles.Num() + 1;
        }

        Stats._Success = true;
        return Stats;
    }

    auto
        FCk_GroundNav_FieldCooker::
        Try_BuildSourceManifestPlans(
            TConstArrayView<FCk_GroundNav_CookFieldPlan>      InFieldPlans,
            TConstArrayView<FCk_GroundNav_CookSourcePartition> InPartitions,
            TArray<FCk_GroundNav_CookSourceManifestPlan>&     OutPlans)
        -> bool
    {
        using namespace groundnav_cook_private;

        const auto FieldPlansAreCompatible = !InPartitions.IsEmpty() &&
            Get_AreFieldPlansCompleteAndCompatible(InFieldPlans);
        CK_ENSURE_IF_NOT(FieldPlansAreCompatible,
            TEXT("GroundNavCook: source manifests require a complete compatible default-plus-variant field plan set"))
        { }
        if (NOT FieldPlansAreCompatible)
        { return false; }

        const auto& FirstFieldPlan = InFieldPlans[0];
        auto OrderedFieldPlans = TArray<const FCk_GroundNav_CookFieldPlan*>{};
        OrderedFieldPlans.Reserve(InFieldPlans.Num());
        for (const auto& FieldPlan : InFieldPlans)
        { OrderedFieldPlans.Emplace(&FieldPlan); }
        OrderedFieldPlans.Sort([](const auto& InLeft, const auto& InRight)
        {
            if (NOT InLeft._ProfileTag.IsValid())
            { return true; }
            if (NOT InRight._ProfileTag.IsValid())
            { return false; }
            return InLeft._ProfileTag.GetTagName().LexicalLess(InRight._ProfileTag.GetTagName());
        });

        auto BuiltPlans = TArray<FCk_GroundNav_CookSourceManifestPlan>{};
        BuiltPlans.Reserve(InPartitions.Num());
        auto SeenPartitionIds = TSet<int32>{};
        auto ClaimedCoords = TSet<FIntPoint>{};
        auto ExpectedCoords = TSet<FIntPoint>{};
        for (const auto& Tile : FirstFieldPlan._Tiles)
        {
            const auto TileIsUniqueAndValid = Tile._WorldBounds.IsValid &&
                Get_CookedTileContentHash(Tile._Blob) != 0 && !ExpectedCoords.Contains(Tile._Coord);
            CK_ENSURE_IF_NOT(TileIsUniqueAndValid,
                TEXT("GroundNavCook: source manifests require one valid tile for every default lattice coordinate"))
            { }
            if (NOT TileIsUniqueAndValid)
            { return false; }
            ExpectedCoords.Add(Tile._Coord);
        }

        auto OrderedPartitions = TArray<const FCk_GroundNav_CookSourcePartition*>{};
        OrderedPartitions.Reserve(InPartitions.Num());
        for (const auto& Partition : InPartitions)
        { OrderedPartitions.Emplace(&Partition); }
        OrderedPartitions.Sort([](const auto& InLeft, const auto& InRight)
        { return InLeft._PartitionId < InRight._PartitionId; });

        for (const auto* PartitionPtr : OrderedPartitions)
        {
            const auto& Partition = *PartitionPtr;
            const auto PartitionIsValid = Partition._StreamingVolumeId == FirstFieldPlan._StreamingVolumeId &&
                Partition._StreamingVolumeId > 0 && Partition._PartitionId > 0 &&
                Partition._DataLayerSelector.Get_LayerNames() == FirstFieldPlan._DataLayerSelector.Get_LayerNames() &&
                Partition._DataLayerSelector.Get_IsCanonical() && NOT Partition._TileCoords.IsEmpty() &&
                Get_IsStrictRowMajor(Partition._TileCoords) && !SeenPartitionIds.Contains(Partition._PartitionId);
            CK_ENSURE_IF_NOT(PartitionIsValid,
                TEXT("GroundNavCook: source manifest partition needs a unique positive id, canonical selector, and sorted coordinates"))
            { }
            if (NOT PartitionIsValid)
            { return false; }

            auto ManifestPlan = FCk_GroundNav_CookSourceManifestPlan{};
            ManifestPlan._SourceLevelPackage = FirstFieldPlan._SourceLevelPackage;
            ManifestPlan._CookKey = FirstFieldPlan._CookKey;
            ManifestPlan._StreamingVolumeId = Partition._StreamingVolumeId;
            ManifestPlan._PartitionId = Partition._PartitionId;
            ManifestPlan._DataLayerSelector = Partition._DataLayerSelector;
            ManifestPlan._LatticeKey = FirstFieldPlan._LatticeKey;
            ManifestPlan._TileCoords = Partition._TileCoords;
            ManifestPlan._AssetPath = Get_SourceManifestAssetPath(*OrderedFieldPlans[0], Partition._PartitionId);

            for (const auto* FieldPlan : OrderedFieldPlans)
            {
                auto TilePaths = TArray<FString>{};
                TilePaths.Reserve(Partition._TileCoords.Num());

                for (const auto& Coord : Partition._TileCoords)
                {
                    const auto* Tile = FieldPlan->_Tiles.FindByPredicate([Coord](const auto& InTile)
                    { return InTile._Coord == Coord; });
                    const auto TileExists = Tile != nullptr && Tile->_WorldBounds.IsValid &&
                        Get_CookedTileContentHash(Tile->_Blob) != 0 && !ClaimedCoords.Contains(Coord);
                    CK_ENSURE_IF_NOT(TileExists,
                        TEXT("GroundNavCook: partition [{}] references a missing, invalid, or overlapping tile [{}, {}]"),
                        Partition._PartitionId, Coord.X, Coord.Y)
                    { }
                    if (NOT TileExists)
                    { return false; }

                    TilePaths.Emplace(Get_CookedTileAssetPath(kCookedDataRootPath,
                        FieldPlan->_SourceLevelPackage.ToString(), FieldPlan->_CookKey, Coord,
                        FieldPlan->_ProfileTag, FieldPlan->_DataLayerSelector));
                }

                ManifestPlan._ProfileTags.Emplace(FieldPlan->_ProfileTag);
                ManifestPlan._ProfileFingerprints.Emplace(FieldPlan->_Fingerprint);
                ManifestPlan._ProfileTilePaths.Emplace(MoveTemp(TilePaths));
            }

            for (const auto& Coord : Partition._TileCoords)
            { ClaimedCoords.Add(Coord); }
            SeenPartitionIds.Add(Partition._PartitionId);
            BuiltPlans.Emplace(MoveTemp(ManifestPlan));
        }

        const auto CoversDefaultLatticeExactlyOnce = ClaimedCoords.Num() == ExpectedCoords.Num() &&
            Algo::AllOf(ExpectedCoords, [&ClaimedCoords](const FIntPoint& InCoord)
            { return ClaimedCoords.Contains(InCoord); });
        CK_ENSURE_IF_NOT(CoversDefaultLatticeExactlyOnce,
            TEXT("GroundNavCook: source manifest partitions must cover every default lattice coordinate exactly once"))
        { }
        if (NOT CoversDefaultLatticeExactlyOnce)
        { return false; }

        const auto AllPlansAreValid = Algo::AllOf(BuiltPlans,
            [](const FCk_GroundNav_CookSourceManifestPlan& InPlan) { return Get_IsManifestPlanValid(InPlan); });
        CK_ENSURE_IF_NOT(AllPlansAreValid,
            TEXT("GroundNavCook: generated source manifest plan failed validation"))
        { }
        if (NOT AllPlansAreValid)
        { return false; }

        OutPlans = MoveTemp(BuiltPlans);
        return true;
    }

    auto
        FCk_GroundNav_FieldCooker::
        Save_SourceManifestPlans(
            TConstArrayView<FCk_GroundNav_CookSourceManifestPlan> InPlans)
        -> bool
    {
        using namespace groundnav_cook_private;

        const auto AllPlansAreValid = Get_AreManifestPlansValidForSave(InPlans);
        CK_ENSURE_IF_NOT(AllPlansAreValid,
            TEXT("GroundNavCook: source manifests need unique owners, non-overlapping tile identities, and persisted tile packages; refusing all writes"))
        { }
        if (NOT AllPlansAreValid)
        { return false; }

        auto OrderedPlans = TArray<const FCk_GroundNav_CookSourceManifestPlan*>{};
        OrderedPlans.Reserve(InPlans.Num());
        for (const auto& Plan : InPlans)
        { OrderedPlans.Emplace(&Plan); }
        OrderedPlans.Sort([](const auto& InLeft, const auto& InRight)
        {
            if (InLeft._StreamingVolumeId != InRight._StreamingVolumeId)
            { return InLeft._StreamingVolumeId < InRight._StreamingVolumeId; }
            return InLeft._PartitionId < InRight._PartitionId;
        });

        for (const auto* PlanPtr : OrderedPlans)
        {
            const auto& Plan = *PlanPtr;
            auto* Package = CreatePackage(*FPackageName::ObjectPathToPackageName(Plan._AssetPath));
            const auto PackageIsValid = ck::IsValid(Package);
            CK_ENSURE_IF_NOT(PackageIsValid,
                TEXT("GroundNavCook: could not create source manifest package [{}]"), Plan._AssetPath)
            { }
            if (NOT PackageIsValid)
            { return false; }

            auto* Manifest = NewObject<UCk_GroundNav_CookedSourceManifest_UE>(Package,
                *FPackageName::ObjectPathToObjectName(Plan._AssetPath), RF_Public | RF_Standalone);
            const auto ManifestIsValid = ck::IsValid(Manifest);
            CK_ENSURE_IF_NOT(ManifestIsValid,
                TEXT("GroundNavCook: could not create source manifest [{}]"), Plan._AssetPath)
            { }
            if (NOT ManifestIsValid)
            { return false; }

            auto Profiles = TArray<FCk_GroundNav_CookedSourceProfile_UE>{};
            Profiles.Reserve(Plan._ProfileTags.Num());
            for (auto ProfileIndex = 0; ProfileIndex < Plan._ProfileTags.Num(); ++ProfileIndex)
            {
                auto Profile = FCk_GroundNav_CookedSourceProfile_UE{};
                Profile.Set_ProfileTag(Plan._ProfileTags[ProfileIndex]);
                Profile.Set_Fingerprint(Plan._ProfileFingerprints[ProfileIndex]);

                auto TileRefs = TArray<TSoftObjectPtr<UCk_GroundNav_CookedTile_UE>>{};
                TileRefs.Reserve(Plan._ProfileTilePaths[ProfileIndex].Num());
                for (const auto& TilePath : Plan._ProfileTilePaths[ProfileIndex])
                { TileRefs.Emplace(FSoftObjectPath{TilePath}); }
                Profile.Set_Tiles(MoveTemp(TileRefs));
                Profiles.Emplace(MoveTemp(Profile));
            }

            Manifest->Set_FormatVersion(kFieldBlobFormatVersion);
            Manifest->Set_StreamingVolumeId(Plan._StreamingVolumeId);
            Manifest->Set_PartitionId(Plan._PartitionId);
            Manifest->Set_DataLayerNames(Plan._DataLayerSelector.Get_LayerNames());
            Manifest->Set_LatticeKey(Plan._LatticeKey);
            Manifest->Set_TileCoords(Plan._TileCoords);
            Manifest->Set_Profiles(MoveTemp(Profiles));

            if (NOT ck::jolt::cook::Save_CookedAsset(*Manifest))
            { return false; }
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------
