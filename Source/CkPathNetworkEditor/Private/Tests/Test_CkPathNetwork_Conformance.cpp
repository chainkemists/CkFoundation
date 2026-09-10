#include "CkPathNetworkEditor/CkPathNetwork_EditorUtils.h"

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include <Misc/AutomationTest.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::conformance::test
{
    auto
    Make_ProjectionResult(
        const ECk_NavSurface_QueryStatus InStatus,
        const FVector& InLocation) -> FCk_NavSurface_ProjectionResult
    {
        auto Result = FCk_NavSurface_ProjectionResult{};
        Result.Set_Status(InStatus);
        Result.Set_Location(InLocation);
        return Result;
    }

    auto
    Get_SourcePoint() -> FVector
    {
        return FVector{100.0, 200.0, 50.0};
    }

    constexpr auto MaxPlanarDelta = 50.0f;
    constexpr auto MaxVerticalDelta = 50.0f;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Conformance_SuccessWithinDeltasIsConformant_Test,
    "Ck.PathNetworkEditor.Conformance.SuccessWithinDeltasIsConformant",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Conformance_SuccessWithinDeltasIsConformant_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::conformance::test;

    const auto SourcePoint = Get_SourcePoint();
    const auto Projected = Make_ProjectionResult(
        ECk_NavSurface_QueryStatus::Success, SourcePoint + FVector{10.0, 0.0, 10.0});

    const auto Conformance = ck_pathnetwork_editor::Make_NavmeshConformance(SourcePoint, Projected);

    TestTrue(TEXT("a Success projection reports Success"),
        Conformance._Status == ECk_NavSurface_QueryStatus::Success);
    TestTrue(TEXT("a Success projection is projected"), Conformance._Projected);
    TestTrue(TEXT("the projected point is carried through"),
        Conformance._ProjectedPoint.Equals(Projected.Get_Location()));
    TestTrue(TEXT("the planar delta is the 2D distance"),
        FMath::IsNearlyEqual(Conformance._PlanarDelta, 10.0f));
    TestTrue(TEXT("the vertical delta is the absolute Z difference"),
        FMath::IsNearlyEqual(Conformance._VerticalDelta, 10.0f));
    TestTrue(TEXT("a projection inside both deltas is conformant"),
        ck_pathnetwork_editor::Is_Conformant(Conformance, MaxPlanarDelta, MaxVerticalDelta));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Conformance_SuccessBeyondPlanarDeltaIsNotConformant_Test,
    "Ck.PathNetworkEditor.Conformance.SuccessBeyondPlanarDeltaIsNotConformant",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Conformance_SuccessBeyondPlanarDeltaIsNotConformant_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::conformance::test;

    const auto SourcePoint = Get_SourcePoint();
    const auto Projected = Make_ProjectionResult(
        ECk_NavSurface_QueryStatus::Success, SourcePoint + FVector{500.0, 0.0, 0.0});

    const auto Conformance = ck_pathnetwork_editor::Make_NavmeshConformance(SourcePoint, Projected);

    TestTrue(TEXT("a Success projection reports Success even when it lands far away"),
        Conformance._Status == ECk_NavSurface_QueryStatus::Success);
    TestTrue(TEXT("a Success projection is projected regardless of distance"), Conformance._Projected);
    TestTrue(TEXT("the planar delta records the full 2D distance"),
        FMath::IsNearlyEqual(Conformance._PlanarDelta, 500.0f));
    TestFalse(TEXT("a projection beyond the planar delta is not conformant"),
        ck_pathnetwork_editor::Is_Conformant(Conformance, MaxPlanarDelta, MaxVerticalDelta));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Conformance_UnbuiltIsReportedAsUnbuiltNotNoSurface_Test,
    "Ck.PathNetworkEditor.Conformance.UnbuiltIsReportedAsUnbuiltNotNoSurface",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Conformance_UnbuiltIsReportedAsUnbuiltNotNoSurface_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::conformance::test;

    const auto SourcePoint = Get_SourcePoint();
    const auto Projected = Make_ProjectionResult(
        ECk_NavSurface_QueryStatus::Unbuilt, FVector::ZeroVector);

    const auto Conformance = ck_pathnetwork_editor::Make_NavmeshConformance(SourcePoint, Projected);

    TestTrue(TEXT("an unbuilt region is reported as Unbuilt"),
        Conformance._Status == ECk_NavSurface_QueryStatus::Unbuilt);
    TestFalse(TEXT("an unbuilt region is NOT reported as NoSurface"),
        Conformance._Status == ECk_NavSurface_QueryStatus::NoSurface);
    TestFalse(TEXT("an unbuilt region did not project"), Conformance._Projected);
    TestFalse(TEXT("an unbuilt region is not conformant"),
        ck_pathnetwork_editor::Is_Conformant(Conformance, MaxPlanarDelta, MaxVerticalDelta));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_Conformance_NoSurfaceIsReportedAsNoSurface_Test,
    "Ck.PathNetworkEditor.Conformance.NoSurfaceIsReportedAsNoSurface",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_Conformance_NoSurfaceIsReportedAsNoSurface_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::conformance::test;

    const auto SourcePoint = Get_SourcePoint();
    const auto Projected = Make_ProjectionResult(
        ECk_NavSurface_QueryStatus::NoSurface, FVector::ZeroVector);

    const auto Conformance = ck_pathnetwork_editor::Make_NavmeshConformance(SourcePoint, Projected);

    TestTrue(TEXT("a region with nothing walkable is reported as NoSurface"),
        Conformance._Status == ECk_NavSurface_QueryStatus::NoSurface);
    TestFalse(TEXT("a region with nothing walkable is NOT reported as Unbuilt"),
        Conformance._Status == ECk_NavSurface_QueryStatus::Unbuilt);
    TestFalse(TEXT("a region with nothing walkable did not project"), Conformance._Projected);
    TestFalse(TEXT("a region with nothing walkable is not conformant"),
        ck_pathnetwork_editor::Is_Conformant(Conformance, MaxPlanarDelta, MaxVerticalDelta));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_PathNetworkEditor_GroundNavSnap_ProjectionStatusMapsWithoutMutation_Test,
    "Ck.PathNetworkEditor.GroundNavSnap.ProjectionStatusMapsWithoutMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_PathNetworkEditor_GroundNavSnap_ProjectionStatusMapsWithoutMutation_Test::RunTest(const FString& InParameters)
{
    using namespace ck::pathnetwork_editor::conformance::test;

    const auto SourcePoint = Get_SourcePoint();
    const auto Success = Make_ProjectionResult(ECk_NavSurface_QueryStatus::Success, SourcePoint);
    const auto NoSurface = Make_ProjectionResult(ECk_NavSurface_QueryStatus::NoSurface, SourcePoint);
    const auto Unbuilt = Make_ProjectionResult(ECk_NavSurface_QueryStatus::Unbuilt, SourcePoint);
    const auto NoProvider = Make_ProjectionResult(ECk_NavSurface_QueryStatus::NoProvider, SourcePoint);

    TestEqual(TEXT("a successful GroundNav projection is eligible to mutate"),
        ck_pathnetwork_editor::Get_GroundNavSnapStatus(Success),
        ECk_PathNetworkEditor_GroundNavSnapStatus::Projected);
    TestEqual(TEXT("a missing surface keeps the authored point and reports NoSurface"),
        ck_pathnetwork_editor::Get_GroundNavSnapStatus(NoSurface),
        ECk_PathNetworkEditor_GroundNavSnapStatus::NoSurface);
    TestEqual(TEXT("an unbuilt GroundNav field keeps the authored point and reports Unavailable"),
        ck_pathnetwork_editor::Get_GroundNavSnapStatus(Unbuilt),
        ECk_PathNetworkEditor_GroundNavSnapStatus::Unavailable);
    TestEqual(TEXT("an unavailable GroundNav provider keeps the authored point and reports Unavailable"),
        ck_pathnetwork_editor::Get_GroundNavSnapStatus(NoProvider),
        ECk_PathNetworkEditor_GroundNavSnapStatus::Unavailable);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
