#include "CkGroundNav/Shadow/CkGroundNav_Shadow_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Shadow/CkGroundNav_Shadow_Report.h"

#include <Engine/World.h>
#include <UObject/Package.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_shadow_utils
{
    auto Get_World(
        const UObject* InWorldContextObject) -> UWorld*
    {
        return ck::IsValid(InWorldContextObject) ? InWorldContextObject->GetWorld() : nullptr;
    }

    // The PIE prefix is stripped because the map name is a REPORT key: the same map bakes a different
    // key per PIE instance otherwise, and two runs of the same suite would never diff.
    auto Get_MapKey(
        const UWorld* InWorld) -> FName
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return NAME_None; }

        return FName{*UWorld::RemovePIEPrefix(InWorld->GetOutermost()->GetName())};
    }

    auto TryGet_Diagnostics(
        const UObject* InWorldContextObject) -> ck::FFragment_GroundNav_ShadowDiagnostics*
    {
        auto* World = Get_World(InWorldContextObject);

        if (ck::Is_NOT_Valid(World))
        { return nullptr; }

        auto WorldEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(World);

        if (ck::Is_NOT_Valid(WorldEntity))
        { return nullptr; }

        return &WorldEntity.AddOrGet<ck::FFragment_GroundNav_ShadowDiagnostics>();
    }

    auto Get_ContextName(
        const UObject* InWorldContextObject) -> FString
    {
        return ck::IsValid(InWorldContextObject) ? InWorldContextObject->GetName() : FString{TEXT("NULL")};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNav_Shadow_UE::
    Request_BeginShadowFixture(
        const UObject* InWorldContextObject,
        FName InFixture)
    -> void
{
    auto* Diagnostics = ck_groundnav_shadow_utils::TryGet_Diagnostics(InWorldContextObject);

    const auto DiagnosticsAreReachable = Diagnostics != nullptr;

    CK_ENSURE_IF_NOT(DiagnosticsAreReachable,
        TEXT("Request_BeginShadowFixture could not resolve an ECS world from context object [{}]"),
        ck_groundnav_shadow_utils::Get_ContextName(InWorldContextObject))
    { return; }

    Diagnostics->_ActiveFixture = InFixture;

    // The row must exist from the moment the fixture opens: a fixture that recorded nothing and a
    // fixture that was never opened are different results, and an absent row cannot say which.
    Diagnostics->_PerFixture.FindOrAdd(InFixture);

    ck::groundnav::Display(TEXT("Shadow fixture [{}] opened"), InFixture);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNav_Shadow_UE::
    Request_EndShadowFixture(
        const UObject* InWorldContextObject)
    -> void
{
    auto* Diagnostics = ck_groundnav_shadow_utils::TryGet_Diagnostics(InWorldContextObject);

    const auto DiagnosticsAreReachable = Diagnostics != nullptr;

    CK_ENSURE_IF_NOT(DiagnosticsAreReachable,
        TEXT("Request_EndShadowFixture could not resolve an ECS world from context object [{}]"),
        ck_groundnav_shadow_utils::Get_ContextName(InWorldContextObject))
    { return; }

    ck::groundnav::Display(TEXT("Shadow fixture [{}] closed"), Diagnostics->_ActiveFixture);

    Diagnostics->_ActiveFixture = NAME_None;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNav_Shadow_UE::
    Get_ShadowReport(
        const UObject* InWorldContextObject)
    -> FString
{
    auto* Diagnostics = ck_groundnav_shadow_utils::TryGet_Diagnostics(InWorldContextObject);

    const auto DiagnosticsAreReachable = Diagnostics != nullptr;

    CK_ENSURE_IF_NOT(DiagnosticsAreReachable,
        TEXT("Get_ShadowReport could not resolve an ECS world from context object [{}]"),
        ck_groundnav_shadow_utils::Get_ContextName(InWorldContextObject))
    { return {}; }

    const auto ArtifactIdentity =
        Get_FallbackFixtureKey(InWorldContextObject).ToString();

    return ck::groundnav::shadow::Get_Report(*Diagnostics, ArtifactIdentity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNav_Shadow_UE::
    Get_ShadowComparisonCount(
        const UObject* InWorldContextObject,
        FName InFixture)
    -> int32
{
    auto* Diagnostics = ck_groundnav_shadow_utils::TryGet_Diagnostics(InWorldContextObject);

    const auto DiagnosticsAreReachable = Diagnostics != nullptr;

    CK_ENSURE_IF_NOT(DiagnosticsAreReachable,
        TEXT("Get_ShadowComparisonCount could not resolve an ECS world from context object [{}]"),
        ck_groundnav_shadow_utils::Get_ContextName(InWorldContextObject))
    { return 0; }

    const auto* Counters = Diagnostics->Get_PerFixture().Find(InFixture);

    return Counters != nullptr ? Counters->_Comparisons : 0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNav_Shadow_UE::
    Request_ResetShadowDiagnostics(
        const UObject* InWorldContextObject)
    -> void
{
    auto* Diagnostics = ck_groundnav_shadow_utils::TryGet_Diagnostics(InWorldContextObject);

    const auto DiagnosticsAreReachable = Diagnostics != nullptr;

    CK_ENSURE_IF_NOT(DiagnosticsAreReachable,
        TEXT("Request_ResetShadowDiagnostics could not resolve an ECS world from context object [{}]"),
        ck_groundnav_shadow_utils::Get_ContextName(InWorldContextObject))
    { return; }

    Diagnostics->_ActiveFixture = NAME_None;
    Diagnostics->_PerFixture.Reset();
    Diagnostics->_DivergingQueryIds.Reset();

    ck::groundnav::Display(TEXT("Shadow diagnostics reset"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GroundNav_Shadow_UE::
    Get_FallbackFixtureKey(
        const UObject* InWorldContextObject)
    -> FName
{
    return ck_groundnav_shadow_utils::Get_MapKey(ck_groundnav_shadow_utils::Get_World(InWorldContextObject));
}

// --------------------------------------------------------------------------------------------------------------------
