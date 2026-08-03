#include "Tests/CkPathNetwork_AuthoringTestDetector.h"

#include "CkPathNetworkEditor/Authoring/CkPathNetwork_AuthoringService.h"
#include "CkPathNetworkEditor/CkPathNetwork_EditorUtils.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"

#include <Editor.h>
#include <Editor/TransBuffer.h>
#include <Engine/Level.h>
#include <Engine/Selection.h>
#include <Engine/StaticMeshActor.h>
#include <Misc/AutomationTest.h>
#include <Tests/AutomationCommon.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::authoring::test
{
    auto
    Get_DetectionBounds() -> FBox
    {
        return FBox{
            FVector{-200.0f, -200.0f, 0.0f},
            FVector{200.0f, 200.0f, 100.0f}};
    }

    auto
    Make_PreviewRequest(
        UWorld& InWorld,
        const UCk_PathNetwork_AuthoringTestDetector& InDetector) -> FPreviewRequest
    {
        auto Request = FPreviewRequest{};
        Request._World = &InWorld;
        Request._DetectorTemplate = &InDetector;
        Request._DetectionBounds = Get_DetectionBounds();
        return Request;
    }

    auto
    Make_ApplyRequest(
        ULevel& InLevel,
        const UCk_PathNetwork_AuthoringTestDetector& InDetector,
        const FPreviewResult& InPreview) -> FApplyToLevelRequest
    {
        auto Request = FApplyToLevelRequest{};
        Request._TargetLevel = &InLevel;
        Request._DetectorTemplate = &InDetector;
        Request._DetectionBounds = Get_DetectionBounds();
        Request._Preview = &InPreview;
        return Request;
    }

    auto
    Make_RecommendedFollowerTuning() -> FCk_PathNetworkFollower_Tuning
    {
        auto Tuning = FCk_PathNetworkFollower_Tuning{};
        Tuning.Set_OffPathCostMultiplier(7.0f);
        Tuning.Set_NearEndpointCostMultiplier(1.25f);
        Tuning.Set_EndpointJoinMaxDistance(1200.0f);
        Tuning.Set_ComponentTransferMaxDistance(900.0f);
        Tuning.Set_LocalNetworkShortcutMaxDistance(700.0f);
        Tuning.Set_DirectTripGraceDistance(1800.0f);
        Tuning.Set_DirectRouteMinimumSavingsFraction(0.05f);
        Tuning.Set_SideKeepingFraction(0.35f);
        Tuning.Set_CorridorWaypointSpacing(175.0f);
        Tuning.Set_CornerSmoothingDistance(90.0f);
        Tuning.Set_DesiredNavmeshClearance(55.0f);
        Tuning.Set_NavmeshResolvedRibbonTolerance(12.0f);
        return Tuning;
    }

    auto
    Make_AuthoredRibbon() -> FCk_PathNetwork_Ribbon
    {
        auto Ribbon = FCk_PathNetwork_Ribbon{
            TArray<FCk_PathNetwork_RibbonPoint>{
                FCk_PathNetwork_RibbonPoint{FVector{-100.0f, 0.0f, 0.0f}, 50.0f},
                FCk_PathNetwork_RibbonPoint{FVector{100.0f, 0.0f, 0.0f}, 50.0f}}};
        Ribbon.Set_Source(ECk_PathNetwork_RibbonSource::Authored);
        return Ribbon;
    }

    auto
    Get_RibbonCountBySource(
        const ACk_PathNetwork_UE& InActor,
        const ECk_PathNetwork_RibbonSource InSource) -> int32
    {
        auto Count = 0;
        for (const auto& Ribbon : InActor.Get_WorldRibbons())
        {
            if (Ribbon.Get_Source() == InSource)
            { ++Count; }
        }
        return Count;
    }

    auto
    Get_PathNetworkActorCount(
        const ULevel& InLevel) -> int32
    {
        auto Count = 0;
        for (const auto& Actor : InLevel.Actors)
        {
            if (Cast<ACk_PathNetwork_UE>(Actor.Get()) != nullptr)
            { ++Count; }
        }
        return Count;
    }

    struct FPreviewEditorState
    {
        int32 _SelectedActorCount = INDEX_NONE;
        int32 _TransactionQueueLength = INDEX_NONE;
        int32 _TransactionUndoCount = INDEX_NONE;
        bool _TransactionIsActive = false;
    };

    auto
    Capture_PreviewEditorState() -> FPreviewEditorState
    {
        auto State = FPreviewEditorState{};
        if (GEditor == nullptr)
        { return State; }

        if (const auto* Selection = GEditor->GetSelectedActors())
        { State._SelectedActorCount = Selection->Num(); }
        if (auto* Transactions = GEditor->Trans.Get())
        {
            State._TransactionQueueLength = Transactions->GetQueueLength();
            State._TransactionUndoCount = Transactions->GetUndoCount();
            State._TransactionIsActive = Transactions->IsActive();
        }
        return State;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_UsableDetectorClasses_Test,
    "Ck.PathNetworkEditor.Authoring.UsableDetectorClasses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Authoring_UsableDetectorClasses_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::authoring;

    TestFalse(TEXT("null detector class is not usable"), Is_UsableDetectorClass(nullptr));
    TestFalse(TEXT("base abstract detector class is not usable"),
        Is_UsableDetectorClass(UCk_PathNetwork_Detector_UE::StaticClass()));
    TestTrue(TEXT("concrete test detector class is usable"),
        Is_UsableDetectorClass(UCk_PathNetwork_AuthoringTestDetector::StaticClass()));

    const auto Classes = Get_LoadedUsableDetectorClasses();
    TestTrue(TEXT("loaded usable detector classes include the concrete test detector"),
        Classes.Contains(UCk_PathNetwork_AuthoringTestDetector::StaticClass()));
    for (const auto* Class : Classes)
    { TestTrue(TEXT("loaded detector list contains only usable classes"), Is_UsableDetectorClass(Class)); }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_PreviewPurity_Test,
    "Ck.PathNetworkEditor.Authoring.PreviewPurity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Authoring_PreviewPurity_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::authoring;

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    if (World == nullptr)
    { return false; }

    auto* Detector = NewObject<UCk_PathNetwork_AuthoringTestDetector>(GetTransientPackage());
    TestNotNull(TEXT("test detector is created"), Detector);
    if (Detector == nullptr)
    { return false; }

    const auto ActorCountBefore = World->GetCurrentLevel()->Actors.Num();
    const auto WorldDirtyBefore = World->GetPackage()->IsDirty();
    const auto LevelDirtyBefore = World->GetCurrentLevel()->GetPackage()->IsDirty();
    const auto EditorStateBefore =
        ck::pathnetwork_editor::authoring::test::Capture_PreviewEditorState();
    constexpr auto ExpectedSimplifyTolerance = 432.0f;
    Detector->Set_ExpectedProcessSimplifyTolerance(
        ExpectedSimplifyTolerance);
    auto PreviewRequest =
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(
            *World,
            *Detector);
    PreviewRequest._VectorizeParams.Set_SimplifyTolerance(
        ExpectedSimplifyTolerance);
    const auto PreviewResult = Preview(PreviewRequest);
    const auto EditorStateAfter =
        ck::pathnetwork_editor::authoring::test::Capture_PreviewEditorState();

    TestTrue(TEXT("preview succeeds"), PreviewResult._Succeeded);
    TestTrue(TEXT("preview records the requested world"), PreviewResult._World.Get() == World);
    TestEqual(TEXT("preview emits one deterministic generated ribbon"),
        PreviewResult._GeneratedWorldRibbons.Num(), 1);
    TestEqual(TEXT("preview leaves actor membership unchanged"),
        World->GetCurrentLevel()->Actors.Num(), ActorCountBefore);
    TestEqual(TEXT("preview leaves world dirty state unchanged"),
        World->GetPackage()->IsDirty(), WorldDirtyBefore);
    TestEqual(TEXT("preview leaves level package dirty state unchanged"),
        World->GetCurrentLevel()->GetPackage()->IsDirty(), LevelDirtyBefore);
    TestEqual(TEXT("preview leaves actor selection unchanged"),
        EditorStateAfter._SelectedActorCount, EditorStateBefore._SelectedActorCount);
    TestEqual(TEXT("preview leaves transaction queue length unchanged"),
        EditorStateAfter._TransactionQueueLength, EditorStateBefore._TransactionQueueLength);
    TestEqual(TEXT("preview leaves transaction undo count unchanged"),
        EditorStateAfter._TransactionUndoCount, EditorStateBefore._TransactionUndoCount);
    TestEqual(TEXT("preview leaves transaction activity unchanged"),
        EditorStateAfter._TransactionIsActive, EditorStateBefore._TransactionIsActive);
    TestEqual(TEXT("preview callbacks run on a transient detector copy"),
        Detector->Get_CallbackCount(), 0);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_PreviewFailuresAreAtomic_Test,
    "Ck.PathNetworkEditor.Authoring.PreviewFailuresAreAtomic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Authoring_PreviewFailuresAreAtomic_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::authoring;

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    auto* Detector = NewObject<UCk_PathNetwork_AuthoringTestDetector>(GetTransientPackage());
    if (World == nullptr || Detector == nullptr)
    { return false; }

    const auto ActorCountBefore = World->GetCurrentLevel()->Actors.Num();
    const auto WorldDirtyBefore = World->GetPackage()->IsDirty();
    const auto LevelDirtyBefore = World->GetCurrentLevel()->GetPackage()->IsDirty();
    const auto EditorStateBefore =
        ck::pathnetwork_editor::authoring::test::Capture_PreviewEditorState();
    auto InvalidRequest = ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(*World, *Detector);
    InvalidRequest._DetectionBounds = FBox{ForceInit};
    AddExpectedError(
        TEXT("Path-network preview requires finite ordered detection bounds"),
        EAutomationExpectedErrorFlags::Contains,
        0);
    const auto InvalidPreview = Preview(InvalidRequest);
    TestFalse(TEXT("invalid bounds reject preview"), InvalidPreview._Succeeded);
    TestEqual(TEXT("invalid preview creates no actor"),
        World->GetCurrentLevel()->Actors.Num(), ActorCountBefore);

    Detector->Set_Behavior(
        ECk_PathNetwork_AuthoringTestDetectorBehavior::BoundsValidationFails);
    const auto BoundsValidationPreview = Preview(
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(*World, *Detector));
    TestFalse(
        TEXT("detector bounds rejection rejects preview"),
        BoundsValidationPreview._Succeeded);
    TestTrue(
        TEXT("bounds rejection records its reason"),
        BoundsValidationPreview._FailureReason.Contains(
            TEXT("Intentional bounds validation failure")));
    TestFalse(
        TEXT("bounds rejection invokes no rasterization"),
        BoundsValidationPreview._Mask.Get_IsValidMask());
    TestTrue(
        TEXT("bounds rejection invokes no downstream ribbon processing"),
        BoundsValidationPreview._GeneratedWorldRibbons.IsEmpty());
    TestEqual(TEXT("bounds rejection creates no actor"),
        World->GetCurrentLevel()->Actors.Num(), ActorCountBefore);

    Detector->Set_Behavior(ECk_PathNetwork_AuthoringTestDetectorBehavior::ProcessFails);
    const auto ProcessPreview = Preview(
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(*World, *Detector));
    TestFalse(TEXT("process failure rejects preview"), ProcessPreview._Succeeded);
    TestTrue(TEXT("process failure records its reason"),
        ProcessPreview._FailureReason.Contains(TEXT("Intentional process failure")));
    TestEqual(TEXT("process failure creates no actor"),
        World->GetCurrentLevel()->Actors.Num(), ActorCountBefore);

    Detector->Set_Behavior(ECk_PathNetwork_AuthoringTestDetectorBehavior::ValidationFails);
    const auto ValidationPreview = Preview(
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(*World, *Detector));
    TestFalse(TEXT("validation failure rejects preview"), ValidationPreview._Succeeded);
    TestTrue(TEXT("validation failure records its reason"),
        ValidationPreview._FailureReason.Contains(TEXT("Intentional validation failure")));
    TestEqual(TEXT("validation failure creates no actor"),
        World->GetCurrentLevel()->Actors.Num(), ActorCountBefore);
    const auto EditorStateAfter =
        ck::pathnetwork_editor::authoring::test::Capture_PreviewEditorState();
    TestEqual(TEXT("rejected previews leave world dirty state unchanged"),
        World->GetPackage()->IsDirty(), WorldDirtyBefore);
    TestEqual(TEXT("rejected previews leave level package dirty state unchanged"),
        World->GetCurrentLevel()->GetPackage()->IsDirty(), LevelDirtyBefore);
    TestEqual(TEXT("rejected previews leave actor selection unchanged"),
        EditorStateAfter._SelectedActorCount, EditorStateBefore._SelectedActorCount);
    TestEqual(TEXT("rejected previews leave transaction queue unchanged"),
        EditorStateAfter._TransactionQueueLength, EditorStateBefore._TransactionQueueLength);
    TestEqual(TEXT("rejected previews leave transaction undo count unchanged"),
        EditorStateAfter._TransactionUndoCount, EditorStateBefore._TransactionUndoCount);
    TestEqual(TEXT("rejected previews leave transaction activity unchanged"),
        EditorStateAfter._TransactionIsActive, EditorStateBefore._TransactionIsActive);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_StalePreviewRejected_Test,
    "Ck.PathNetworkEditor.Authoring.StalePreviewRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Authoring_StalePreviewRejected_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::authoring;

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    auto* Detector = NewObject<UCk_PathNetwork_AuthoringTestDetector>(GetTransientPackage());
    if (World == nullptr || Detector == nullptr)
    { return false; }

    const auto PathNetworkActorCountBefore =
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(*World->GetCurrentLevel());

    const auto PreviewResult = Preview(
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(*World, *Detector));
    TestTrue(TEXT("preview succeeds before configuration changes"), PreviewResult._Succeeded);

    Detector->Set_ConfigurationRevision(1);
    const auto FingerprintAfterChange = Compute_DetectorConfigurationFingerprint(Detector);
    TestTrue(TEXT("configuration change changes fingerprint"),
        FingerprintAfterChange != PreviewResult._DetectorConfigurationFingerprint);

    AddExpectedError(
        TEXT("ApplyPreview_ToLevel rejected a stale preview"),
        EAutomationExpectedErrorFlags::Contains,
        0);
    const auto ApplyResult = ApplyPreview_ToLevel(
        ck::pathnetwork_editor::authoring::test::Make_ApplyRequest(
            *World->GetCurrentLevel(), *Detector, PreviewResult));
    TestFalse(TEXT("stale preview is rejected before actor creation"), ApplyResult._Succeeded);
    TestEqual(TEXT("stale preview leaves level actor count unchanged"),
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(*World->GetCurrentLevel()),
        PathNetworkActorCountBefore);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_ChangedSourceGeometryRejected_Test,
    "Ck.PathNetworkEditor.Authoring.ChangedSourceGeometryRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Authoring_ChangedSourceGeometryRejected_Test::RunTest(
    const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::authoring;

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    auto* Detector = NewObject<UCk_PathNetwork_AuthoringTestDetector>(GetTransientPackage());
    auto* LocationSource =
        World != nullptr ? World->SpawnActor<AStaticMeshActor>() : nullptr;
    if (World == nullptr || Detector == nullptr || LocationSource == nullptr)
    { return false; }

    Detector->Set_LocationSource(LocationSource);
    const auto PreviewResult = Preview(
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(*World, *Detector));
    TestTrue(TEXT("preview succeeds before source geometry changes"), PreviewResult._Succeeded);
    if (NOT PreviewResult._Succeeded)
    { return false; }

    TestTrue(TEXT("source actor moves after preview"),
        LocationSource->SetActorLocation(FVector{500.0, 0.0, 0.0}));
    const auto PathNetworkActorCountBefore =
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(
            *World->GetCurrentLevel());
    AddExpectedError(
        TEXT("ApplyPreview_ToLevel detected changed source output"),
        EAutomationExpectedErrorFlags::Contains,
        0);
    const auto ApplyResult = ApplyPreview_ToLevel(
        ck::pathnetwork_editor::authoring::test::Make_ApplyRequest(
            *World->GetCurrentLevel(), *Detector, PreviewResult));

    TestFalse(TEXT("apply rejects a preview whose source geometry changed"),
        ApplyResult._Succeeded);
    TestTrue(TEXT("rejection tells the designer to preview again"),
        ApplyResult._FailureReason.Contains(TEXT("Run Preview again")));
    TestEqual(TEXT("rejected apply creates no path-network actor"),
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(
            *World->GetCurrentLevel()),
        PathNetworkActorCountBefore);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_LegacyBakeUsesTransientDetector_Test,
    "Ck.PathNetworkEditor.Authoring.LegacyBakeUsesTransientDetector",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Authoring_LegacyBakeUsesTransientDetector_Test::RunTest(
    const FString& InParameters)
{
    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    auto* Actor = World != nullptr ? World->SpawnActor<ACk_PathNetwork_UE>() : nullptr;
    auto* Detector = Actor != nullptr
        ? NewObject<UCk_PathNetwork_AuthoringTestDetector>(
            Actor, NAME_None, RF_Transactional)
        : nullptr;
    if (World == nullptr || Actor == nullptr || Detector == nullptr)
    { return false; }

    const auto BuildParams = FCk_PathNetwork_BuildParams{};
    const auto VectorizeParams = FCk_PathNetwork_VectorizeParams{};
    const auto DetectionExtents = FVector{400.0, 400.0, 100.0};
    TestTrue(TEXT("actor accepts its test detector configuration"),
        Actor->Set_EditorAuthoringConfiguration(
            BuildParams,
            VectorizeParams,
            Detector,
            DetectionExtents,
            ECk_EnableDisable::Disable));

    const auto BakeResult =
        UCk_Utils_PathNetworkEditor_UE::Bake_DetectorToActor(Actor);
    TestTrue(TEXT("legacy actor bake succeeds"), BakeResult.Get_Succeeded());
    TestTrue(TEXT("legacy actor bake preserves detector identity"),
        Actor->Get_Detector().Get() == Detector);
    TestEqual(TEXT("legacy actor bake never invokes the live detector"),
        Detector->Get_CallbackCount(), 0);
    TestTrue(TEXT("legacy actor bake preserves detector extents"),
        Actor->Get_DetectionExtents().Equals(DetectionExtents));
    TestEqual(TEXT("legacy actor bake writes one generated ribbon"),
        BakeResult.Get_GeneratedRibbonCount(), 1);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_ApplyCreationIsUndoable_Test,
    "Ck.PathNetworkEditor.Authoring.ApplyCreationIsUndoable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Authoring_ApplyCreationIsUndoable_Test::RunTest(
    const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::authoring;

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    auto* Detector = NewObject<UCk_PathNetwork_AuthoringTestDetector>(GetTransientPackage());
    if (World == nullptr || Detector == nullptr || GEditor == nullptr)
    { return false; }

    const auto PreviewResult = Preview(
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(*World, *Detector));
    TestTrue(TEXT("preview succeeds before undoable apply"), PreviewResult._Succeeded);
    if (NOT PreviewResult._Succeeded)
    { return false; }

    auto ApplyRequest = ck::pathnetwork_editor::authoring::test::Make_ApplyRequest(
        *World->GetCurrentLevel(), *Detector, PreviewResult);
    ApplyRequest._UseRecommendedFollowerTuning = ECk_EnableDisable::Enable;
    ApplyRequest._RecommendedFollowerTuning =
        ck::pathnetwork_editor::authoring::test::Make_RecommendedFollowerTuning();
    const auto ApplyResult = ApplyPreview_ToLevel(ApplyRequest);
    TestTrue(TEXT("apply creates the path-network actor"), ApplyResult._Succeeded);
    TestEqual(TEXT("level contains the applied path-network actor"),
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(
            *World->GetCurrentLevel()),
        1);
    auto* Actor = ApplyResult._Actor.Get();
    TestNotNull(TEXT("apply returns the created path-network actor"), Actor);
    if (Actor == nullptr)
    { return false; }
    TestEqual(TEXT("apply persists enabled route preferences"),
        Actor->Get_UseRecommendedFollowerTuning(), ECk_EnableDisable::Enable);
    const auto& PersistedTuning = Actor->Get_RecommendedFollowerTuning();
    TestEqual(TEXT("apply persists far off-path multiplier"),
        PersistedTuning.Get_OffPathCostMultiplier(), 7.0f);
    TestEqual(TEXT("apply persists near endpoint multiplier"),
        PersistedTuning.Get_NearEndpointCostMultiplier(), 1.25f);
    TestEqual(TEXT("apply persists endpoint join distance"),
        PersistedTuning.Get_EndpointJoinMaxDistance(), 1200.0f);
    TestEqual(TEXT("apply persists component transfer distance"),
        PersistedTuning.Get_ComponentTransferMaxDistance(), 900.0f);
    TestEqual(TEXT("apply persists local network shortcut distance"),
        PersistedTuning.Get_LocalNetworkShortcutMaxDistance(), 700.0f);
    TestEqual(TEXT("apply persists direct-trip grace distance"),
        PersistedTuning.Get_DirectTripGraceDistance(), 1800.0f);
    TestEqual(TEXT("apply persists the minimum direct-route saving"),
        PersistedTuning.Get_DirectRouteMinimumSavingsFraction(), 0.05f);
    TestEqual(TEXT("apply persists side-keeping fraction"),
        PersistedTuning.Get_SideKeepingFraction(), 0.35f);
    TestEqual(TEXT("apply persists corridor waypoint spacing"),
        PersistedTuning.Get_CorridorWaypointSpacing(), 175.0f);
    TestEqual(TEXT("apply persists corner smoothing distance"),
        PersistedTuning.Get_CornerSmoothingDistance(), 90.0f);
    TestEqual(TEXT("apply persists desired navmesh clearance"),
        PersistedTuning.Get_DesiredNavmeshClearance(), 55.0f);
    TestEqual(TEXT("apply persists post-nav ribbon tolerance"),
        PersistedTuning.Get_NavmeshResolvedRibbonTolerance(), 12.0f);

    TestTrue(TEXT("editor accepts undo for path-network creation"),
        GEditor->UndoTransaction());
    TestEqual(TEXT("undo removes the created path-network actor"),
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(
            *World->GetCurrentLevel()),
        0);

    TestTrue(TEXT("editor accepts redo for path-network creation"),
        GEditor->RedoTransaction());
    TestEqual(TEXT("redo restores the created path-network actor"),
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(
            *World->GetCurrentLevel()),
        1);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_InvalidEnabledRoutePreferencesAreAtomic_Test,
    "Ck.PathNetworkEditor.Authoring.InvalidEnabledRoutePreferencesAreAtomic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Authoring_InvalidEnabledRoutePreferencesAreAtomic_Test::RunTest(
    const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::authoring;

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    auto* Detector = NewObject<UCk_PathNetwork_AuthoringTestDetector>(GetTransientPackage());
    if (World == nullptr || Detector == nullptr)
    { return false; }

    const auto PreviewResult = Preview(
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(*World, *Detector));
    TestTrue(TEXT("preview succeeds before invalid route-preference apply"), PreviewResult._Succeeded);
    if (NOT PreviewResult._Succeeded)
    { return false; }

    const auto InitialApplyResult = ApplyPreview_ToLevel(
        ck::pathnetwork_editor::authoring::test::Make_ApplyRequest(
            *World->GetCurrentLevel(), *Detector, PreviewResult));
    TestTrue(TEXT("initial apply creates an actor to protect from invalid configuration"),
        InitialApplyResult._Succeeded);
    auto* Actor = InitialApplyResult._Actor.Get();
    TestNotNull(TEXT("initial apply returns its actor"), Actor);
    if (Actor == nullptr)
    { return false; }

    const auto ActorCountBefore =
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(*World->GetCurrentLevel());
    const auto RibbonCountBefore = Actor->Get_WorldRibbons().Num();
    const auto UseRecommendedTuningBefore = Actor->Get_UseRecommendedFollowerTuning();
    const auto TuningBefore = Actor->Get_RecommendedFollowerTuning();
    const auto WorldDirtyBefore = World->GetPackage()->IsDirty();
    const auto LevelDirtyBefore = World->GetCurrentLevel()->GetPackage()->IsDirty();
    const auto EditorStateBefore =
        ck::pathnetwork_editor::authoring::test::Capture_PreviewEditorState();

    auto InvalidRequest = ck::pathnetwork_editor::authoring::test::Make_ApplyRequest(
        *World->GetCurrentLevel(), *Detector, PreviewResult);
    InvalidRequest._ExplicitTargetActor = Actor;
    InvalidRequest._UseRecommendedFollowerTuning = ECk_EnableDisable::Enable;
    InvalidRequest._RecommendedFollowerTuning =
        ck::pathnetwork_editor::authoring::test::Make_RecommendedFollowerTuning();
    InvalidRequest._RecommendedFollowerTuning.Set_NearEndpointCostMultiplier(0.5f);
    AddExpectedError(
        TEXT("ApplyPreview_ToLevel requires valid bounds, vectorize parameters, build parameters, and route preferences"),
        EAutomationExpectedErrorFlags::Contains,
        0);
    const auto InvalidApplyResult = ApplyPreview_ToLevel(InvalidRequest);

    TestFalse(TEXT("invalid enabled route preferences reject apply"), InvalidApplyResult._Succeeded);
    TestEqual(TEXT("invalid apply creates no additional actor"),
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(*World->GetCurrentLevel()),
        ActorCountBefore);
    TestEqual(TEXT("invalid apply preserves the target actor route-preference enable flag"),
        Actor->Get_UseRecommendedFollowerTuning(), UseRecommendedTuningBefore);
    const auto& TuningAfter = Actor->Get_RecommendedFollowerTuning();
    TestEqual(TEXT("invalid apply preserves far off-path multiplier"),
        TuningAfter.Get_OffPathCostMultiplier(), TuningBefore.Get_OffPathCostMultiplier());
    TestEqual(TEXT("invalid apply preserves near endpoint multiplier"),
        TuningAfter.Get_NearEndpointCostMultiplier(), TuningBefore.Get_NearEndpointCostMultiplier());
    TestEqual(TEXT("invalid apply preserves endpoint join distance"),
        TuningAfter.Get_EndpointJoinMaxDistance(), TuningBefore.Get_EndpointJoinMaxDistance());
    TestEqual(TEXT("invalid apply preserves component transfer distance"),
        TuningAfter.Get_ComponentTransferMaxDistance(),
        TuningBefore.Get_ComponentTransferMaxDistance());
    TestEqual(TEXT("invalid apply preserves local network shortcut distance"),
        TuningAfter.Get_LocalNetworkShortcutMaxDistance(),
        TuningBefore.Get_LocalNetworkShortcutMaxDistance());
    TestEqual(TEXT("invalid apply preserves direct-trip grace distance"),
        TuningAfter.Get_DirectTripGraceDistance(), TuningBefore.Get_DirectTripGraceDistance());
    TestEqual(TEXT("invalid apply preserves the minimum direct-route saving"),
        TuningAfter.Get_DirectRouteMinimumSavingsFraction(),
        TuningBefore.Get_DirectRouteMinimumSavingsFraction());
    TestEqual(TEXT("invalid apply preserves side-keeping fraction"),
        TuningAfter.Get_SideKeepingFraction(), TuningBefore.Get_SideKeepingFraction());
    TestEqual(TEXT("invalid apply preserves corridor waypoint spacing"),
        TuningAfter.Get_CorridorWaypointSpacing(), TuningBefore.Get_CorridorWaypointSpacing());
    TestEqual(TEXT("invalid apply preserves corner smoothing distance"),
        TuningAfter.Get_CornerSmoothingDistance(), TuningBefore.Get_CornerSmoothingDistance());
    TestEqual(TEXT("invalid apply preserves desired navmesh clearance"),
        TuningAfter.Get_DesiredNavmeshClearance(), TuningBefore.Get_DesiredNavmeshClearance());
    TestEqual(TEXT("invalid apply preserves post-nav ribbon tolerance"),
        TuningAfter.Get_NavmeshResolvedRibbonTolerance(),
        TuningBefore.Get_NavmeshResolvedRibbonTolerance());
    TestEqual(TEXT("invalid apply preserves generated ribbons"),
        Actor->Get_WorldRibbons().Num(), RibbonCountBefore);
    TestEqual(TEXT("invalid apply leaves world dirty state unchanged"),
        World->GetPackage()->IsDirty(), WorldDirtyBefore);
    TestEqual(TEXT("invalid apply leaves level dirty state unchanged"),
        World->GetCurrentLevel()->GetPackage()->IsDirty(), LevelDirtyBefore);
    const auto EditorStateAfter =
        ck::pathnetwork_editor::authoring::test::Capture_PreviewEditorState();
    TestEqual(TEXT("invalid apply leaves actor selection unchanged"),
        EditorStateAfter._SelectedActorCount, EditorStateBefore._SelectedActorCount);
    TestEqual(TEXT("invalid apply leaves transaction queue unchanged"),
        EditorStateAfter._TransactionQueueLength, EditorStateBefore._TransactionQueueLength);
    TestEqual(TEXT("invalid apply leaves transaction undo count unchanged"),
        EditorStateAfter._TransactionUndoCount, EditorStateBefore._TransactionUndoCount);
    TestEqual(TEXT("invalid apply leaves transaction activity unchanged"),
        EditorStateAfter._TransactionIsActive, EditorStateBefore._TransactionIsActive);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_ApplyPreservesAuthoredRibbons_Test,
    "Ck.PathNetworkEditor.Authoring.ApplyPreservesAuthoredRibbons",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Authoring_ApplyPreservesAuthoredRibbons_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::authoring;

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    auto* Detector = NewObject<UCk_PathNetwork_AuthoringTestDetector>(GetTransientPackage());
    if (World == nullptr || Detector == nullptr)
    { return false; }

    const auto PreviewResult = Preview(
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(*World, *Detector));
    TestTrue(TEXT("preview succeeds before level apply"), PreviewResult._Succeeded);
    if (NOT PreviewResult._Succeeded)
    { return false; }

    const auto InitialApplyResult = ApplyPreview_ToLevel(
        ck::pathnetwork_editor::authoring::test::Make_ApplyRequest(
            *World->GetCurrentLevel(), *Detector, PreviewResult));
    TestTrue(TEXT("apply creates a same-level path network actor"), InitialApplyResult._Succeeded);
    TestTrue(TEXT("initial apply reports actor creation"), InitialApplyResult._CreatedActor);
    auto* Actor = InitialApplyResult._Actor.Get();
    TestNotNull(TEXT("initial apply returns its actor"), Actor);
    if (Actor == nullptr)
    { return false; }

    Actor->Set_Ribbons({Actor->Convert_WorldRibbonToRelative(
        ck::pathnetwork_editor::authoring::test::Make_AuthoredRibbon())});
    const auto ExistingApplyResult = ApplyPreview_ToExistingActor(Actor, PreviewResult);
    TestTrue(TEXT("current preview applies to the same actor"), ExistingApplyResult._Succeeded);
    TestFalse(TEXT("existing actor apply does not create another actor"), ExistingApplyResult._CreatedActor);
    TestEqual(TEXT("existing apply preserves one authored ribbon"),
        ExistingApplyResult._AuthoredRibbonCount, 1);
    TestEqual(TEXT("existing apply writes one generated ribbon"),
        ExistingApplyResult._GeneratedRibbonCount, 1);
    TestEqual(TEXT("existing apply reports both ribbons"), ExistingApplyResult._TotalRibbonCount, 2);
    TestEqual(TEXT("actor retains authored ribbon semantics"),
        ck::pathnetwork_editor::authoring::test::Get_RibbonCountBySource(
            *Actor, ECk_PathNetwork_RibbonSource::Authored), 1);
    TestEqual(TEXT("actor retains generated ribbon semantics"),
        ck::pathnetwork_editor::authoring::test::Get_RibbonCountBySource(
            *Actor, ECk_PathNetwork_RibbonSource::Generated), 1);
    TestEqual(TEXT("same-level apply leaves one path network actor"),
        ck::pathnetwork_editor::authoring::test::Get_PathNetworkActorCount(*World->GetCurrentLevel()), 1);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Authoring_ApplyRecenterPreservesRouteWatches_Test,
    "Ck.PathNetworkEditor.Authoring.ApplyRecenterPreservesRouteWatches",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool
    FCk_PathNetworkEditor_Authoring_ApplyRecenterPreservesRouteWatches_Test::
    RunTest(
        const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::authoring;

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(
        TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    auto* Detector = NewObject<UCk_PathNetwork_AuthoringTestDetector>(
        GetTransientPackage());
    if (World == nullptr || Detector == nullptr)
    { return false; }

    const auto InitialPreview = Preview(
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(
            *World,
            *Detector));
    TestTrue(
        TEXT("initial preview succeeds"),
        InitialPreview._Succeeded);
    if (NOT InitialPreview._Succeeded)
    { return false; }

    const auto InitialApply = ApplyPreview_ToLevel(
        ck::pathnetwork_editor::authoring::test::Make_ApplyRequest(
            *World->GetCurrentLevel(),
            *Detector,
            InitialPreview));
    auto* Actor = InitialApply._Actor.Get();
    TestNotNull(
        TEXT("initial apply creates a path-network actor"),
        Actor);
    if (Actor == nullptr)
    { return false; }

    const auto WorldRoute = FCk_PathNetwork_RepresentativeRoute{
        TEXT("Map Regression"),
        FVector{-700.0f, -500.0f, 30.0f},
        FVector{900.0f, 800.0f, 40.0f}};
    Actor->Set_RepresentativeRoutes(
        {Actor->Convert_WorldRepresentativeRouteToRelative(WorldRoute)});

    const auto BoundsOffset = FVector{5000.0f, -3000.0f, 200.0f};
    const auto InitialBounds =
        ck::pathnetwork_editor::authoring::test::Get_DetectionBounds();
    const auto ShiftedBounds = FBox{
        InitialBounds.Min + BoundsOffset,
        InitialBounds.Max + BoundsOffset};
    auto ShiftedPreviewRequest =
        ck::pathnetwork_editor::authoring::test::Make_PreviewRequest(
            *World,
            *Detector);
    ShiftedPreviewRequest._DetectionBounds = ShiftedBounds;
    const auto ShiftedPreview = Preview(ShiftedPreviewRequest);
    TestTrue(
        TEXT("shifted-bounds preview succeeds"),
        ShiftedPreview._Succeeded);
    if (NOT ShiftedPreview._Succeeded)
    { return false; }

    auto ShiftedApplyRequest =
        ck::pathnetwork_editor::authoring::test::Make_ApplyRequest(
            *World->GetCurrentLevel(),
            *Detector,
            ShiftedPreview);
    ShiftedApplyRequest._ExplicitTargetActor = Actor;
    ShiftedApplyRequest._DetectionBounds = ShiftedBounds;
    const auto ShiftedApply = ApplyPreview_ToLevel(
        ShiftedApplyRequest);
    TestTrue(
        TEXT("shifted-bounds preview reapplies to the existing actor"),
        ShiftedApply._Succeeded);
    TestTrue(
        TEXT("apply recenters the actor"),
        Actor->GetActorLocation().Equals(ShiftedBounds.GetCenter()));

    const auto PreservedRoutes = Actor->Get_WorldRepresentativeRoutes();
    TestEqual(
        TEXT("recentered actor retains its route-watch count"),
        PreservedRoutes.Num(),
        1);
    TestEqual(
        TEXT("recentered actor retains its route-watch name"),
        PreservedRoutes[0].Get_Name(),
        WorldRoute.Get_Name());
    TestTrue(
        TEXT("recentered actor preserves the world route start"),
        PreservedRoutes[0].Get_Start().Equals(WorldRoute.Get_Start()));
    TestTrue(
        TEXT("recentered actor preserves the world route goal"),
        PreservedRoutes[0].Get_Goal().Equals(WorldRoute.Get_Goal()));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
