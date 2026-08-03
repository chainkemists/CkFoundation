#include "Tests/CkPathNetwork_AuthoringTestDetector.h"

#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerEdMode.h"
#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerLauncher.h"
#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerPreset.h"
#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerSession.h"

#include <Editor.h>
#include <EditorModeManager.h>
#include <EditorModes.h>
#include <Components/BoxComponent.h>
#include <Engine/LevelBounds.h>
#include <Framework/Docking/TabManager.h>
#include <GameFramework/Actor.h>
#include <LevelEditor.h>
#include <Misc/AutomationTest.h>
#include <Misc/ScopeExit.h>
#include <Modules/ModuleManager.h>
#include <Subsystems/AssetEditorSubsystem.h>
#include <Tests/AutomationCommon.h>
#include <ToolMenus.h>
#include <Toolkits/BaseToolkit.h>
#include <Widgets/Docking/SDockTab.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::designer::test
{
    auto
    Make_Preset(
        const FName InOwner,
        const FName InId,
        const TCHAR* InDisplayName,
        const int32 InSortPriority = 0) -> FPreset
    {
        auto Preset = FPreset{};
        Preset._Owner = InOwner;
        Preset._Id = InId;
        Preset._DisplayName = FText::FromString(InDisplayName);
        Preset._Description = FText::FromString(TEXT("Designer automation preset"));
        Preset._DetectorClass = UCk_PathNetwork_AuthoringTestDetector::StaticClass();
        Preset._DetectionExtents = FVector{200.0, 200.0, 100.0};
        Preset._UseRecommendedFollowerTuning = ECk_EnableDisable::Enable;
        auto RecommendedTuning = FCk_PathNetworkFollower_Tuning{};
        RecommendedTuning.Set_OffPathCostMultiplier(7.0f);
        RecommendedTuning.Set_NearEndpointCostMultiplier(1.25f);
        RecommendedTuning.Set_EndpointJoinMaxDistance(1200.0f);
        RecommendedTuning.Set_ComponentTransferMaxDistance(900.0f);
        RecommendedTuning.Set_LocalNetworkShortcutMaxDistance(700.0f);
        RecommendedTuning.Set_DirectTripGraceDistance(1800.0f);
        RecommendedTuning.Set_DirectRouteMinimumSavingsFraction(0.05f);
        RecommendedTuning.Set_SideKeepingFraction(0.35f);
        RecommendedTuning.Set_CorridorWaypointSpacing(175.0f);
        RecommendedTuning.Set_CornerSmoothingDistance(90.0f);
        RecommendedTuning.Set_DesiredNavmeshClearance(55.0f);
        RecommendedTuning.Set_NavmeshResolvedRibbonTolerance(12.0f);
        Preset._RecommendedFollowerTuning = RecommendedTuning;
        Preset._SortPriority = InSortPriority;
        return Preset;
    }

    auto
    Count_PresetsByOwner(
        const TArray<FPreset>& InPresets,
        const FName InOwner) -> int32
    {
        auto Count = 0;
        for (const auto& Preset : InPresets)
        {
            if (Preset._Owner == InOwner)
            { ++Count; }
        }
        return Count;
    }

    auto
    Find_PresetIndex(
        const TArray<FPreset>& InPresets,
        const FName InOwner,
        const FName InId) -> int32
    {
        return InPresets.IndexOfByPredicate(
            [&](const FPreset& InPreset)
            {
                return InPreset._Owner == InOwner
                    && InPreset._Id == InId;
            });
    }

    auto
    Add_BoxBounds(
        AActor& InActor,
        const FVector& InExtents) -> UBoxComponent*
    {
        auto* Box = NewObject<UBoxComponent>(
            &InActor,
            NAME_None,
            RF_Transient);
        if (Box == nullptr)
        { return nullptr; }

        InActor.AddInstanceComponent(Box);
        InActor.SetRootComponent(Box);
        Box->SetBoxExtent(InExtents);
        Box->RegisterComponent();
        return Box;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Designer_PresetRegistryLifecycle_Test,
    "Ck.PathNetworkEditor.Designer.PresetRegistryLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Designer_PresetRegistryLifecycle_Test::RunTest(
    const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::designer;

    const auto OwnerA = FName{TEXT("CkPathNetworkDesignerTestOwnerA")};
    const auto OwnerB = FName{TEXT("CkPathNetworkDesignerTestOwnerB")};
    Unregister_PresetsByOwner(OwnerA);
    Unregister_PresetsByOwner(OwnerB);
    ON_SCOPE_EXIT
    {
        Unregister_PresetsByOwner(OwnerA);
        Unregister_PresetsByOwner(OwnerB);
    };

    const auto PresetA = ck::pathnetwork_editor::designer::test::Make_Preset(
        OwnerA,
        TEXT("Sidewalks"),
        TEXT("Designer Test Sidewalks"),
        2000);
    const auto PresetB = ck::pathnetwork_editor::designer::test::Make_Preset(
        OwnerB,
        TEXT("Paths"),
        TEXT("Designer Test Paths"),
        1000);

    TestTrue(TEXT("first owner registers its preset"), Register_Preset(PresetA));
    TestTrue(TEXT("second owner registers its preset"), Register_Preset(PresetB));

    auto Presets = Get_Presets();
    TestEqual(
        TEXT("registry contains one preset for the first owner"),
        ck::pathnetwork_editor::designer::test::Count_PresetsByOwner(Presets, OwnerA),
        1);
    TestEqual(
        TEXT("registry contains one preset for the second owner"),
        ck::pathnetwork_editor::designer::test::Count_PresetsByOwner(Presets, OwnerB),
        1);

    const auto PresetAIndex =
        ck::pathnetwork_editor::designer::test::Find_PresetIndex(
            Presets, OwnerA, PresetA._Id);
    const auto PresetBIndex =
        ck::pathnetwork_editor::designer::test::Find_PresetIndex(
            Presets, OwnerB, PresetB._Id);
    TestTrue(
        TEXT("higher-priority preset sorts before lower-priority preset"),
        PresetAIndex != INDEX_NONE
        && PresetBIndex != INDEX_NONE
        && PresetAIndex < PresetBIndex);

    AddExpectedError(
        TEXT("is already registered"),
        EAutomationExpectedErrorFlags::Contains,
        0);
    TestFalse(
        TEXT("duplicate owner and id are rejected"),
        Register_Preset(PresetA));

    Unregister_PresetsByOwner(OwnerA);
    Presets = Get_Presets();
    TestEqual(
        TEXT("unregister removes only the requested owner's presets"),
        ck::pathnetwork_editor::designer::test::Count_PresetsByOwner(Presets, OwnerA),
        0);
    TestEqual(
        TEXT("unregister preserves another owner's preset"),
        ck::pathnetwork_editor::designer::test::Count_PresetsByOwner(Presets, OwnerB),
        1);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Designer_ModeActivationConstructsToolkit_Test,
    "Ck.PathNetworkEditor.Designer.ModeActivationConstructsToolkit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Designer_ModeActivationConstructsToolkit_Test::RunTest(
    const FString& InParameters)
{
    if (GEditor == nullptr)
    {
        AddError(TEXT("Editor singleton is required for editor-mode activation"));
        return false;
    }

    const auto ModeId =
        UCk_PathNetworkDesigner_EdMode::EM_CkPathNetworkDesignerModeId;
    auto* AssetEditorSubsystem =
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
    TestNotNull(
        TEXT("asset-editor subsystem is available"),
        AssetEditorSubsystem);
    if (AssetEditorSubsystem == nullptr)
    { return false; }

    auto ModeInfo = FEditorModeInfo{};
    TestTrue(
        TEXT("Ck Path Network mode is auto-discovered"),
        AssetEditorSubsystem->FindEditorModeInfo(ModeId, ModeInfo));
    TestTrue(
        TEXT("Ck Path Network mode is visible in the Modes menu"),
        ModeInfo.IsVisible());
    TestEqual(
        TEXT("Ck Path Network mode has its designer-facing name"),
        ModeInfo.Name.ToString(),
        FString{TEXT("Ck Path Network")});

    auto* ToolMenus = UToolMenus::Get();
    TestNotNull(
        TEXT("tool-menu registry is available"),
        ToolMenus);
    auto* ToolsMenu = ToolMenus != nullptr
        ? ToolMenus->FindMenu(TEXT("LevelEditor.MainMenu.Tools"))
        : nullptr;
    TestNotNull(
        TEXT("level-editor Tools menu is registered"),
        ToolsMenu);
    auto* ToolsSection = ToolsMenu != nullptr
        ? ToolsMenu->FindSection(TEXT("CkPathNetwork"))
        : nullptr;
    TestNotNull(
        TEXT("Tools menu contains the Ck Path Network section"),
        ToolsSection);
    TestNotNull(
        TEXT("Tools menu contains the Ck Path Network designer launcher"),
        ToolsSection != nullptr
            ? ToolsSection->FindEntry(TEXT("CkPathNetwork_OpenDesigner"))
            : nullptr);

    auto& ModeTools = GLevelEditorModeTools();
    const bool WasAlreadyActive = ModeTools.IsModeActive(ModeId);
    auto& LevelEditor =
        FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
    const auto TabManager = LevelEditor.GetLevelEditorTabManager();
    TestTrue(
        TEXT("level-editor tab manager is available"),
        TabManager.IsValid());
    const bool HasToolboxTabSpawner =
        TabManager.IsValid()
        && TabManager->HasTabSpawner(
            LevelEditorTabIds::LevelEditorToolBox);
    const bool WasToolboxTabOpen =
        TabManager.IsValid()
        && TabManager->FindExistingLiveTab(
            LevelEditorTabIds::LevelEditorToolBox).IsValid();
    ON_SCOPE_EXIT
    {
        if (NOT WasAlreadyActive)
        {
            ModeTools.DeactivateMode(ModeId);
            if (NOT ModeTools.IsModeActive(FBuiltinEditorModes::EM_Default))
            { ModeTools.ActivateMode(FBuiltinEditorModes::EM_Default); }
        }

        if (NOT WasToolboxTabOpen && TabManager.IsValid())
        {
            if (const auto ToolboxTab =
                    TabManager->FindExistingLiveTab(
                        LevelEditorTabIds::LevelEditorToolBox);
                ToolboxTab.IsValid())
            {
                ToolboxTab->RequestCloseTab();
            }
        }
    };

    ck::pathnetwork_editor::designer::Open_Designer();
    TestTrue(
        TEXT("designer launcher activates the Ck Path Network mode"),
        ModeTools.IsModeActive(ModeId));
    if (HasToolboxTabSpawner)
    {
        TestTrue(
            TEXT("interactive designer launcher opens or focuses the docked level-editor toolbox"),
            TabManager->FindExistingLiveTab(
                LevelEditorTabIds::LevelEditorToolBox).IsValid());
    }

    auto* Mode = Cast<UCk_PathNetworkDesigner_EdMode>(
        ModeTools.GetActiveScriptableMode(ModeId));
    TestNotNull(
        TEXT("active mode is the Ck Path Network designer mode"),
        Mode);
    if (Mode == nullptr)
    { return false; }

    TestNotNull(
        TEXT("mode activation constructs its designer session"),
        Mode->Get_Session());
    const auto Toolkit = Mode->GetToolkit().Pin();
    TestTrue(
        TEXT("mode activation constructs its toolkit"),
        Toolkit.IsValid());
    if (Toolkit.IsValid())
    {
        TestTrue(
            TEXT("designer toolkit constructs inline content"),
            Toolkit->GetInlineContent().IsValid());
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Designer_FitLoadedWorldIgnoresLevelBoundsActor_Test,
    "Ck.PathNetworkEditor.Designer.FitLoadedWorldIgnoresLevelBoundsActor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Designer_FitLoadedWorldIgnoresLevelBoundsActor_Test::RunTest(
    const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::designer;

    const auto Owner = FName{TEXT("CkPathNetworkDesignerBoundsTestOwner")};
    const auto PresetId = FName{TEXT("Bounds")};
    Unregister_PresetsByOwner(Owner);
    ON_SCOPE_EXIT
    {
        Unregister_PresetsByOwner(Owner);
    };

    TestTrue(
        TEXT("bounds test preset registers"),
        Register_Preset(
            ck::pathnetwork_editor::designer::test::Make_Preset(
                Owner,
                PresetId,
                TEXT("Designer Bounds"),
                4000)));

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(
        TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    auto* ContentActor = World != nullptr
        ? World->SpawnActor<AActor>()
        : nullptr;
    auto* LevelBounds = World != nullptr
        ? World->SpawnActor<ALevelBounds>()
        : nullptr;
    if (World == nullptr || ContentActor == nullptr || LevelBounds == nullptr)
    { return false; }

    auto* ContentBounds = ck::pathnetwork_editor::designer::test::Add_BoxBounds(
        *ContentActor,
        FVector{400.0, 500.0, 100.0});
    if (ContentBounds == nullptr || LevelBounds->BoxComponent == nullptr)
    { return false; }
    ContentActor->SetActorLocation(FVector{1000.0, -2000.0, 300.0});

    LevelBounds->bAutoUpdateBounds = false;
    LevelBounds->BoxComponent->SetRelativeScale3D(
        FVector{1397139.94, 1397139.94, 1397139.94});
    TestTrue(
        TEXT("fixture level-bounds actor reproduces the reported world-scale extent"),
        LevelBounds->GetComponentsBoundingBox(true).GetExtent().X > 698000.0);

    auto* Session = NewObject<UCk_PathNetworkDesigner_Session_UE>(
        GetTransientPackage(),
        NAME_None,
        RF_Transient);
    TestNotNull(TEXT("designer session is created"), Session);
    if (Session == nullptr)
    { return false; }

    Session->Initialize(World);
    TestTrue(
        TEXT("loaded-world fit succeeds with a level-bounds helper present"),
        Session->Fit_BoundsToLoadedWorld());

    const auto ExpectedBounds = ContentActor->GetComponentsBoundingBox(true);
    const auto FittedBounds = Session->Get_DetectionBounds();
    TestTrue(
        TEXT("loaded-world fit uses the relevant content center"),
        FittedBounds.GetCenter().Equals(ExpectedBounds.GetCenter()));
    TestTrue(
        TEXT("loaded-world fit excludes the aggregate level-bounds extent"),
        FittedBounds.GetExtent().Equals(ExpectedBounds.GetExtent()));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Designer_SessionClearsPreviewCacheOnLifecycleReset_Test,
    "Ck.PathNetworkEditor.Designer.SessionClearsPreviewCacheOnLifecycleReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Designer_SessionClearsPreviewCacheOnLifecycleReset_Test::RunTest(
    const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::designer;

    const auto Owner = FName{TEXT("CkPathNetworkDesignerSessionTestOwner")};
    const auto PresetId = FName{TEXT("Lifecycle")};
    Unregister_PresetsByOwner(Owner);
    ON_SCOPE_EXIT
    {
        Unregister_PresetsByOwner(Owner);
    };

    TestTrue(
        TEXT("session test preset registers"),
        Register_Preset(
            ck::pathnetwork_editor::designer::test::Make_Preset(
                Owner,
                PresetId,
                TEXT("Designer Session Lifecycle"),
                3000)));

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(
        TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    if (World == nullptr)
    { return false; }

    auto* Session = NewObject<UCk_PathNetworkDesigner_Session_UE>(
        GetTransientPackage(),
        NAME_None,
        RF_Transient);
    TestNotNull(
        TEXT("designer session is created"),
        Session);
    if (Session == nullptr)
    { return false; }

    Session->Initialize(World);
    TestTrue(
        TEXT("session accepts the test preset"),
        Session->Apply_Preset(Owner, PresetId));
    TestTrue(
        TEXT("preset enables its recommended follower tuning"),
        Session->Get_UseRecommendedFollowerTuning()
            == ECk_EnableDisable::Enable);
    const auto& RecommendedTuning = Session->Get_RecommendedFollowerTuning();
    TestEqual(TEXT("preset copies far off-path multiplier"),
        RecommendedTuning.Get_OffPathCostMultiplier(), 7.0f);
    TestEqual(TEXT("preset copies near endpoint multiplier"),
        RecommendedTuning.Get_NearEndpointCostMultiplier(), 1.25f);
    TestEqual(TEXT("preset copies endpoint join distance"),
        RecommendedTuning.Get_EndpointJoinMaxDistance(), 1200.0f);
    TestEqual(TEXT("preset copies component transfer distance"),
        RecommendedTuning.Get_ComponentTransferMaxDistance(), 900.0f);
    TestEqual(TEXT("preset copies local network shortcut distance"),
        RecommendedTuning.Get_LocalNetworkShortcutMaxDistance(), 700.0f);
    TestEqual(TEXT("preset copies direct-trip grace distance"),
        RecommendedTuning.Get_DirectTripGraceDistance(), 1800.0f);
    TestEqual(TEXT("preset copies the minimum direct-route saving"),
        RecommendedTuning.Get_DirectRouteMinimumSavingsFraction(), 0.05f);
    TestEqual(TEXT("preset copies side-keeping fraction"),
        RecommendedTuning.Get_SideKeepingFraction(), 0.35f);
    TestEqual(TEXT("preset copies corridor waypoint spacing"),
        RecommendedTuning.Get_CorridorWaypointSpacing(), 175.0f);
    TestEqual(TEXT("preset copies corner smoothing distance"),
        RecommendedTuning.Get_CornerSmoothingDistance(), 90.0f);
    TestEqual(TEXT("preset copies desired navmesh clearance"),
        RecommendedTuning.Get_DesiredNavmeshClearance(), 55.0f);
    TestEqual(TEXT("preset copies post-nav ribbon tolerance"),
        RecommendedTuning.Get_NavmeshResolvedRibbonTolerance(), 12.0f);
    TestTrue(
        TEXT("session previews in the editor world"),
        Session->Run_Preview());
    TestTrue(
        TEXT("preview result is available"),
        Session->Get_Preview()._Succeeded);
    TestTrue(
        TEXT("preview caches prospective topology analysis"),
        Session->Get_HasTopologyAnalysis());
    TestTrue(
        TEXT("preview topology contains generated graph nodes"),
        Session->Get_TopologyAnalysis()._NodeCount > 0);
    TestFalse(
        TEXT("preview caches occupied mask points for viewport drawing"),
        Session->Get_MaskDrawPoints().IsEmpty());

    Session->Clear_Preview();
    TestFalse(
        TEXT("clear removes the preview result"),
        Session->Get_Preview()._Succeeded);
    TestFalse(
        TEXT("clear removes cached topology analysis"),
        Session->Get_HasTopologyAnalysis());
    TestTrue(
        TEXT("clear removes cached mask points"),
        Session->Get_MaskDrawPoints().IsEmpty());

    TestTrue(
        TEXT("session can preview again after clear"),
        Session->Run_Preview());
    Session->Notify_ConfigurationEdited();
    TestFalse(
        TEXT("configuration edit invalidates the preview result"),
        Session->Get_Preview()._Succeeded);
    TestFalse(
        TEXT("configuration edit clears cached topology analysis"),
        Session->Get_HasTopologyAnalysis());
    TestTrue(
        TEXT("configuration edit clears cached mask points"),
        Session->Get_MaskDrawPoints().IsEmpty());

    TestTrue(
        TEXT("session can preview again before lifecycle reset"),
        Session->Run_Preview());
    Session->Initialize(nullptr);
    TestNull(
        TEXT("lifecycle reset clears the editor world"),
        Session->GetWorld());
    TestNull(
        TEXT("lifecycle reset clears the target level"),
        Session->Get_TargetLevel());
    TestFalse(
        TEXT("lifecycle reset invalidates the preview result"),
        Session->Get_Preview()._Succeeded);
    TestFalse(
        TEXT("lifecycle reset clears cached topology analysis"),
        Session->Get_HasTopologyAnalysis());
    TestTrue(
        TEXT("lifecycle reset clears cached mask points"),
        Session->Get_MaskDrawPoints().IsEmpty());
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Designer_RoutePreviewUsesPresetTuning_Test,
    "Ck.PathNetworkEditor.Designer.RoutePreviewUsesPresetTuning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Designer_RoutePreviewUsesPresetTuning_Test::RunTest(
    const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::designer;

    const auto Owner = FName{TEXT("CkPathNetworkDesignerRoutePreviewTestOwner")};
    const auto PresetId = FName{TEXT("RoutePreview")};
    const auto DirectPresetId = FName{TEXT("RoutePreviewDirectCost")};
    const auto SavingsPresetId =
        FName{TEXT("RoutePreviewMinimumDirectSavings")};
    const auto DisabledPresetId = FName{TEXT("RoutePreviewDisabled")};
    Unregister_PresetsByOwner(Owner);
    ON_SCOPE_EXIT
    {
        Unregister_PresetsByOwner(Owner);
    };

    TestTrue(
        TEXT("route-preview preset registers"),
        Register_Preset(
            ck::pathnetwork_editor::designer::test::Make_Preset(
                Owner,
                PresetId,
                TEXT("Designer Route Preview"),
                3000)));

    auto DirectPreset =
        ck::pathnetwork_editor::designer::test::Make_Preset(
            Owner,
            DirectPresetId,
            TEXT("Designer Route Preview Direct Cost"),
            3001);
    DirectPreset._DetectionExtents = FVector{2000.0, 2000.0, 100.0};
    DirectPreset._RecommendedFollowerTuning.Set_EndpointJoinMaxDistance(
        600.0f);
    DirectPreset._RecommendedFollowerTuning.Set_DirectTripGraceDistance(
        3000.0f);
    TestTrue(
        TEXT("direct-cost route-preview preset registers"),
        Register_Preset(DirectPreset));

    auto SavingsPreset =
        ck::pathnetwork_editor::designer::test::Make_Preset(
            Owner,
            SavingsPresetId,
            TEXT("Designer Route Preview Minimum Direct Savings"),
            3002);
    SavingsPreset._DetectionExtents =
        FVector{2000.0, 2000.0, 100.0};
    SavingsPreset._RecommendedFollowerTuning.Set_OffPathCostMultiplier(
        1.0f);
    SavingsPreset._RecommendedFollowerTuning.Set_NearEndpointCostMultiplier(
        1.0f);
    SavingsPreset._RecommendedFollowerTuning.Set_EndpointJoinMaxDistance(
        600.0f);
    SavingsPreset._RecommendedFollowerTuning.Set_DirectTripGraceDistance(
        0.0f);
    SavingsPreset._RecommendedFollowerTuning
        .Set_DirectRouteMinimumSavingsFraction(0.01f);
    TestTrue(
        TEXT("minimum-savings route-preview preset registers"),
        Register_Preset(SavingsPreset));

    auto DisabledPreset =
        ck::pathnetwork_editor::designer::test::Make_Preset(
            Owner,
            DisabledPresetId,
            TEXT("Designer Route Preview Disabled"),
            3003);
    DisabledPreset._UseRecommendedFollowerTuning = ECk_EnableDisable::Disable;
    TestTrue(
        TEXT("disabled-profile migration preset registers"),
        Register_Preset(DisabledPreset));

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(
        TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    if (World == nullptr)
    { return false; }

    auto* Session = NewObject<UCk_PathNetworkDesigner_Session_UE>(
        GetTransientPackage(),
        NAME_None,
        RF_Transient);
    TestNotNull(TEXT("designer session is created"), Session);
    if (Session == nullptr)
    { return false; }

    Session->Initialize(World);
    TestTrue(
        TEXT("session accepts the route-preview preset"),
        Session->Apply_Preset(Owner, PresetId));
    TestTrue(
        TEXT("geometry preview succeeds before route preview"),
        Session->Run_Preview());

    // The authoring detector emits a diagonal from the detection bounds' southwest
    // toward northeast. Extend that diagonal in both directions so each endpoint
    // can reach only its own network projection, while the whole trip exceeds the
    // direct-grace distance. The high long-trip cost must select the network.
    Session->Set_RoutePreviewEndpoints(
        FVector{-800.0f, -800.0f, 0.0f},
        FVector{800.0f, 800.0f, 0.0f});
    TestTrue(
        TEXT("route preview succeeds across the generated ribbon"),
        Session->Run_RoutePreview());
    const auto& RoutePreview = Session->Get_RoutePreview();
    TestTrue(TEXT("route preview stores a successful result"), RoutePreview._Succeeded);
    TestTrue(TEXT("route preview compiles a traversable path"),
        RoutePreview._CompiledWaypoints.Num() >= 2);
    TestTrue(TEXT("route preview reports positive estimated cost"),
        RoutePreview._EstimatedCost > 0.0f);
    TestTrue(TEXT("route preview uses the generated network"), RoutePreview._UsesNetwork);
    TestTrue(TEXT("route preview records an on-network leg"),
        RoutePreview._OnNetworkLegCount > 0);
    TestEqual(TEXT("route preview reports the local-shortcut distance policy"),
        RoutePreview._LocalNetworkShortcutMaxDistance, 700.0f);
    TestEqual(TEXT("straight fixture offers no local shortcut links"),
        RoutePreview._LocalNetworkShortcutLinkCount, 0);
    TestEqual(TEXT("straight fixture uses no local shortcut legs"),
        RoutePreview._LocalNetworkShortcutLegCount, 0);
    TestEqual(TEXT("route preview reports the selected sidewalk decision"),
        RoutePreview._Decision,
        ERoutePreviewDecision::NetworkSelected);

    Session->Set_RoutePreviewEndpoints(
        FVector{-5000.0f, -5000.0f, 0.0f},
        FVector{-4000.0f, -4000.0f, 0.0f});
    TestTrue(
        TEXT("far endpoints retain the explicit direct fallback preview"),
        Session->Run_RoutePreview());
    const auto& FarRoutePreview = Session->Get_RoutePreview();
    TestEqual(TEXT("far route explains the missing start sidewalk join"),
        FarRoutePreview._Decision,
        ERoutePreviewDecision::NoStartJoin);
    TestEqual(TEXT("far route reports no start candidates"),
        FarRoutePreview._StartCandidateCount,
        0);
    TestTrue(TEXT("far route retains nearest-network distance evidence"),
        FarRoutePreview._NearestStartNetworkDistance
            > Session->Get_RecommendedFollowerTuning()
                .Get_EndpointJoinMaxDistance());

    TestTrue(
        TEXT("session accepts the direct-cost route-preview preset"),
        Session->Apply_Preset(Owner, DirectPresetId));
    TestTrue(
        TEXT("expanded geometry preview succeeds before direct-cost route preview"),
        Session->Run_Preview());
    Session->Set_RoutePreviewEndpoints(
        FVector{-1060.66f, -353.55f, 0.0f},
        FVector{353.55f, 1060.66f, 0.0f});
    TestTrue(
        TEXT("direct-cost route preview succeeds"),
        Session->Run_RoutePreview());
    const auto& DirectRoutePreview = Session->Get_RoutePreview();
    TestEqual(TEXT("short parallel trip reports that direct cost won"),
        DirectRoutePreview._Decision,
        ERoutePreviewDecision::DirectCostWon);
    TestTrue(TEXT("direct-cost route preserves its sidewalk alternative"),
        DirectRoutePreview._HasNetworkAlternative);
    TestTrue(TEXT("direct-cost route records the completed network diagnostic"),
        DirectRoutePreview._NetworkDiagnosticOutcome == TEXT("Complete"));
    TestTrue(TEXT("direct-cost route reports the sidewalk alternative as more expensive"),
        DirectRoutePreview._BestNetworkEstimatedCost
            > DirectRoutePreview._EstimatedCost);
    TestTrue(TEXT("short direct-cost route reports the grace bypass"),
        DirectRoutePreview._DirectTripGraceApplied);

    TestTrue(
        TEXT("session accepts the minimum-savings route-preview preset"),
        Session->Apply_Preset(Owner, SavingsPresetId));
    TestTrue(
        TEXT("minimum-savings geometry preview succeeds"),
        Session->Run_Preview());
    Session->Set_RoutePreviewEndpoints(
        FVector{-1967.929f, -1982.071f, -100.0f},
        FVector{1982.071f, 1967.929f, 100.0f});
    TestTrue(
        TEXT("minimum-savings representative route succeeds"),
        Session->Run_RoutePreview());
    const auto& SavingsRoutePreview =
        Session->Get_RoutePreview();
    TestEqual(TEXT("sub-threshold shortcut saving selects the sidewalk"),
        SavingsRoutePreview._Decision,
        ERoutePreviewDecision::NetworkPreferredByMinimumSavings);
    TestTrue(TEXT("minimum-savings decision uses the network"),
        SavingsRoutePreview._UsesNetwork);
    TestTrue(TEXT("minimum-savings decision keeps the cheaper direct-cost evidence"),
        SavingsRoutePreview._DirectEstimatedCost
            < SavingsRoutePreview._BestNetworkEstimatedCost);
    TestTrue(TEXT("minimum-savings decision reports a positive direct saving"),
        SavingsRoutePreview._DirectRouteSavingsFraction > 0.0f);
    TestTrue(TEXT("reported direct saving remains below the configured minimum"),
        SavingsRoutePreview._DirectRouteSavingsFraction
            < SavingsRoutePreview._DirectRouteMinimumSavingsFraction);
    TestFalse(TEXT("long representative route does not apply short-trip grace"),
        SavingsRoutePreview._DirectTripGraceApplied);

    TestTrue(
        TEXT("session restores the original route-preview preset"),
        Session->Apply_Preset(Owner, PresetId));
    TestTrue(
        TEXT("original geometry preview succeeds before no-sidewalk route preview"),
        Session->Run_Preview());
    Session->Set_RoutePreviewEndpoints(
        FVector{-800.0f, -800.0f, 0.0f},
        FVector{-700.0f, -700.0f, 0.0f});
    TestTrue(
        TEXT("nearby off-network route preview succeeds"),
        Session->Run_RoutePreview());
    const auto& NoSidewalkRoutePreview = Session->Get_RoutePreview();
    TestEqual(TEXT("route with no traversable sidewalk span explains the fallback"),
        NoSidewalkRoutePreview._Decision,
        ERoutePreviewDecision::NoConnectedNetworkRoute);
    TestTrue(TEXT("no-sidewalk route completes its network diagnostic"),
        NoSidewalkRoutePreview._NetworkDiagnosticOutcome == TEXT("Complete"));
    TestFalse(TEXT("no-sidewalk route does not invent a network alternative"),
        NoSidewalkRoutePreview._HasNetworkAlternative);

    Session->Clear_Preview();
    TestFalse(TEXT("clear preview invalidates the route-preview cache"),
        Session->Get_RoutePreview()._Succeeded);

    TestTrue(
        TEXT("session can rebuild geometry preview after route-cache clear"),
        Session->Run_Preview());
    Session->Set_RoutePreviewEndpoints(
        FVector{-800.0f, -800.0f, 0.0f},
        FVector{800.0f, 800.0f, 0.0f});
    TestTrue(
        TEXT("session can rebuild route preview after route-cache clear"),
        Session->Run_RoutePreview());

    TestTrue(
        TEXT("session accepts an old-map-style disabled route profile"),
        Session->Apply_Preset(Owner, DisabledPresetId));
    TestTrue(
        TEXT("geometry preview remains available with map preferences disabled"),
        Session->Run_Preview());
    Session->Set_RoutePreviewEndpoints(
        FVector{-800.0f, -800.0f, 0.0f},
        FVector{800.0f, 800.0f, 0.0f});
    TestFalse(
        TEXT("disabled map preferences cannot preview a policy runtime will not publish"),
        Session->Run_RoutePreview());
    TestTrue(
        TEXT("disabled-profile rejection explains how to restore runtime parity"),
        Session->Get_RoutePreview()._FailureReason.Contains(
            TEXT("Enable Use Map Route Preferences")));

    Session->Initialize(nullptr);
    TestFalse(TEXT("lifecycle reset invalidates the route-preview cache"),
        Session->Get_RoutePreview()._Succeeded);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Designer_RepresentativeRouteWatchlistRoundTrip_Test,
    "Ck.PathNetworkEditor.Designer.RepresentativeRouteWatchlistRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool
    FCk_PathNetworkEditor_Designer_RepresentativeRouteWatchlistRoundTrip_Test::
    RunTest(
        const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::designer;

    const auto Owner =
        FName{TEXT("CkPathNetworkDesignerRouteWatchlistTestOwner")};
    const auto PresetId = FName{TEXT("RouteWatchlist")};
    Unregister_PresetsByOwner(Owner);
    ON_SCOPE_EXIT
    {
        Unregister_PresetsByOwner(Owner);
        if (GEditor != nullptr)
        { GEditor->SelectNone(false, true, false); }
    };

    TestTrue(
        TEXT("route-watchlist preset registers"),
        Register_Preset(
            ck::pathnetwork_editor::designer::test::Make_Preset(
                Owner,
                PresetId,
                TEXT("Designer Route Watchlist"),
                3100)));

    auto WorldWrapper = FTestWorldWrapper{};
    TestTrue(
        TEXT("temporary editor world is created"),
        WorldWrapper.CreateTestWorld(EWorldType::Editor));
    auto* World = WorldWrapper.GetTestWorld();
    if (World == nullptr)
    { return false; }

    auto* Session = NewObject<UCk_PathNetworkDesigner_Session_UE>(
        GetTransientPackage(),
        NAME_None,
        RF_Transient);
    TestNotNull(TEXT("designer session is created"), Session);
    if (Session == nullptr)
    { return false; }

    Session->Initialize(World);
    TestTrue(
        TEXT("session accepts the route-watchlist preset"),
        Session->Apply_Preset(Owner, PresetId));
    TestTrue(
        TEXT("geometry preview succeeds before route-watchlist apply"),
        Session->Run_Preview());
    TestTrue(
        TEXT("preview applies and creates the route-watchlist owner"),
        Session->Apply_ToLevel());

    auto* Actor = Session->Get_VisualizedActor();
    TestNotNull(
        TEXT("apply exposes the path-network actor"),
        Actor);
    if (Actor == nullptr)
    { return false; }

    const auto NorthboundStart = FVector{-800.0f, -800.0f, 0.0f};
    const auto NorthboundGoal = FVector{800.0f, 800.0f, 0.0f};
    Session->Set_RoutePreviewEndpoints(
        NorthboundStart,
        NorthboundGoal);
    TestTrue(
        TEXT("first named route watch is saved"),
        Session->Add_RouteWatch(TEXT("Northbound")));

    const auto CrossStreetStart = FVector{-600.0f, -600.0f, 0.0f};
    const auto CrossStreetGoal = FVector{600.0f, 600.0f, 0.0f};
    Session->Set_RoutePreviewEndpoints(
        CrossStreetStart,
        CrossStreetGoal);
    TestTrue(
        TEXT("second named route watch is saved"),
        Session->Add_RouteWatch(TEXT("CrossStreet")));

    const auto& RelativeRoutes = Actor->Get_RepresentativeRoutes();
    TestEqual(
        TEXT("actor persists both route watches"),
        RelativeRoutes.Num(),
        2);
    TestEqual(
        TEXT("actor preserves route-watch insertion order"),
        RelativeRoutes[0].Get_Name(),
        FName{TEXT("Northbound")});
    const auto WorldRoutes = Actor->Get_WorldRepresentativeRoutes();
    TestTrue(
        TEXT("actor-relative storage round-trips the first world start"),
        WorldRoutes[0].Get_Start().Equals(NorthboundStart));
    TestTrue(
        TEXT("actor-relative storage round-trips the first world goal"),
        WorldRoutes[0].Get_Goal().Equals(NorthboundGoal));

    TestTrue(
        TEXT("first saved route watch can be selected"),
        Session->Select_RouteWatch(0));
    TestTrue(
        TEXT("selected route watch restores its start"),
        Session->Get_RoutePreviewStart().Equals(NorthboundStart));
    TestTrue(
        TEXT("selected route watch restores its goal"),
        Session->Get_RoutePreviewGoal().Equals(NorthboundGoal));
    TestTrue(
        TEXT("all saved route watches refresh successfully"),
        Session->Refresh_AllRouteWatches());
    TestEqual(
        TEXT("refresh all caches one independent result per saved route"),
        Session->Get_RouteWatchPreviews().Num(),
        2);
    const auto& ActiveWatchPreview =
        Session->Get_RouteWatchPreviews()[0]._Preview;
    TestEqual(
        TEXT("refresh all installs the refreshed active-watch decision"),
        Session->Get_RoutePreview()._Decision,
        ActiveWatchPreview._Decision);
    TestEqual(
        TEXT("refresh all installs the refreshed active-watch waypoint count"),
        Session->Get_RoutePreview()._CompiledWaypoints.Num(),
        ActiveWatchPreview._CompiledWaypoints.Num());

    TestTrue(
        TEXT("a new geometry preview succeeds after refreshing all watches"),
        Session->Run_Preview());
    TestEqual(
        TEXT("a new geometry preview invalidates watch paths evaluated on the old snapshot"),
        Session->Get_RouteWatchPreviews().Num(),
        0);

    const auto UpdatedStart = FVector{-700.0f, -700.0f, 0.0f};
    const auto UpdatedGoal = FVector{700.0f, 700.0f, 0.0f};
    Session->Set_RoutePreviewEndpoints(UpdatedStart, UpdatedGoal);
    TestTrue(
        TEXT("selected route watch can be renamed and updated atomically"),
        Session->Save_ActiveRouteWatch(TEXT("Northbound Updated")));
    TestTrue(
        TEXT("second route watch can be selected"),
        Session->Select_RouteWatch(1));
    TestTrue(
        TEXT("selected second route watch can be removed"),
        Session->Remove_ActiveRouteWatch());
    TestEqual(
        TEXT("removing one route watch preserves the other"),
        Actor->Get_RepresentativeRoutes().Num(),
        1);

    auto* FreshSession = NewObject<UCk_PathNetworkDesigner_Session_UE>(
        GetTransientPackage(),
        NAME_None,
        RF_Transient);
    FreshSession->Initialize(World);
    TestTrue(
        TEXT("fresh session automatically restores the level-owned path network"),
        FreshSession->Get_VisualizedActor() == Actor);
    TestTrue(
        TEXT("fresh session automatically restores the level-owned detection bounds"),
        FreshSession->Get_DetectionBounds().Equals(
            Actor->Get_DetectionBounds()));
    TestTrue(
        TEXT("fresh session can target the level-owned path network"),
        FreshSession->Use_CurrentLevel());
    TestTrue(
        TEXT("retargeting the current level restores the level-owned path network"),
        FreshSession->Get_VisualizedActor() == Actor);
    TestTrue(
        TEXT("retargeting the current level restores the level-owned detection bounds"),
        FreshSession->Get_DetectionBounds().Equals(
            Actor->Get_DetectionBounds()));
    TestEqual(
        TEXT("fresh session reloads the surviving route watch"),
        FreshSession->Get_RouteWatchCount(),
        1);
    TestEqual(
        TEXT("fresh session restores the surviving route-watch name"),
        FreshSession->Get_RouteWatchNames()[0],
        FName{TEXT("Northbound Updated")});
    TestTrue(
        TEXT("fresh session restores the updated start"),
        FreshSession->Get_RoutePreviewStart().Equals(UpdatedStart));
    TestTrue(
        TEXT("fresh session restores the updated goal"),
        FreshSession->Get_RoutePreviewGoal().Equals(UpdatedGoal));

    Actor->Set_RepresentativeRoutes(
        TArray<FCk_PathNetwork_RepresentativeRoute>{});
    Session->Synchronize_RouteWatchesFromActor();
    TestEqual(
        TEXT("external undo-style removal clears an obsolete active index"),
        Session->Get_ActiveRouteWatchIndex(),
        INDEX_NONE);
    TestEqual(
        TEXT("external undo-style removal invalidates cached watch previews"),
        Session->Get_RouteWatchPreviews().Num(),
        0);

    auto* SecondActor = World->SpawnActor<ACk_PathNetwork_UE>();
    auto* SecondDetector = SecondActor != nullptr
        ? NewObject<UCk_PathNetwork_AuthoringTestDetector>(
            SecondActor,
            NAME_None,
            RF_Transactional)
        : nullptr;
    TestNotNull(
        TEXT("second path-network actor is created for ambiguity coverage"),
        SecondActor);
    TestNotNull(
        TEXT("second path-network detector is created for ambiguity coverage"),
        SecondDetector);
    if (SecondActor == nullptr || SecondDetector == nullptr)
    { return false; }
    TestTrue(
        TEXT("second path-network actor accepts a usable configuration"),
        SecondActor->Set_EditorAuthoringConfiguration(
            FCk_PathNetwork_BuildParams{},
            FCk_PathNetwork_VectorizeParams{},
            SecondDetector,
            FVector{400.0, 400.0, 100.0},
            ECk_EnableDisable::Disable));

    auto* AmbiguousSession = NewObject<UCk_PathNetworkDesigner_Session_UE>(
        GetTransientPackage(),
        NAME_None,
        RF_Transient);
    AmbiguousSession->Initialize(World);
    TestNull(
        TEXT("fresh session does not guess between multiple level path networks"),
        AmbiguousSession->Get_VisualizedActor());
    TestEqual(
        TEXT("multiple level path networks leave the session in an actionable error state"),
        AmbiguousSession->Get_Status(),
        ECk_PathNetworkDesigner_Status::Error);
    TestTrue(
        TEXT("multiple level path networks instruct the designer to select one"),
        AmbiguousSession->Get_StatusMessage().Contains(
            TEXT("Load Selected Network")));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
