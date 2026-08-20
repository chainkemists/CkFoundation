#include "CkPixelArtRenderer/CkPixelArtRenderer_State.h"

#include "CoreGlobals.h"
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pixel_art_state
{
    using FRegistryMap = TMap<TWeakObjectPtr<const UWorld>, FCk_PixelArt_RenderConfig>;
    using FFrameReportMap = TMap<TWeakObjectPtr<const UWorld>, FCk_PixelArt_FrameReport>;

    auto
        Get_Registry()
        -> FRegistryMap&
    {
        static FRegistryMap Registry;
        return Registry;
    }

    auto
        Get_FrameReports()
        -> FFrameReportMap&
    {
        static FFrameReportMap FrameReports;
        return FrameReports;
    }

    template <typename T_Map>
    auto
        DoSweep_StaleKeys(
            T_Map& InOutRegistry)
        -> void
    {
        for (auto It = InOutRegistry.CreateIterator(); It; ++It)
        {
            if (!It.Key().IsValid())
            { It.RemoveCurrent(); }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PixelArtRenderer_StateRegistry::
    Set(
        const UWorld* InWorld,
        const FCk_PixelArt_RenderConfig& InConfig)
    -> void
{
    check(IsInGameThread());

    if (!ensureMsgf(InWorld != nullptr,
        TEXT("CkPixelArt: Set was called with a null world. Nothing was stored.")))
    { return; }

    auto& Registry = ck_pixel_art_state::Get_Registry();

    // Worlds die without telling us; sweeping on write keeps the map bounded across level transitions without
    // needing a teardown hook per world.
    ck_pixel_art_state::DoSweep_StaleKeys(Registry);

    Registry.Add(InWorld, InConfig);
}

auto
    FCk_PixelArtRenderer_StateRegistry::
    Clear(
        const UWorld* InWorld)
    -> void
{
    check(IsInGameThread());

    // A null world is legitimate absence here, not an error: teardown paths run after the world is gone and must
    // be able to call this unconditionally.
    if (InWorld == nullptr)
    { return; }

    ck_pixel_art_state::Get_Registry().Remove(InWorld);
}

auto
    FCk_PixelArtRenderer_StateRegistry::
    TryGet(
        const UWorld* InWorld)
    -> TOptional<FCk_PixelArt_RenderConfig>
{
    check(IsInGameThread());

    if (InWorld == nullptr)
    { return {}; }

    const auto* Found = ck_pixel_art_state::Get_Registry().Find(InWorld);

    if (Found == nullptr)
    { return {}; }

    return *Found;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PixelArtRenderer_StateRegistry::
    Set_FrameReport(
        const UWorld* InWorld,
        const FCk_PixelArt_FrameReport& InReport)
    -> void
{
    check(IsInGameThread());

    if (InWorld == nullptr)
    { return; }

    auto& FrameReports = ck_pixel_art_state::Get_FrameReports();

    ck_pixel_art_state::DoSweep_StaleKeys(FrameReports);

    FrameReports.Add(InWorld, InReport);
}

auto
    FCk_PixelArtRenderer_StateRegistry::
    TryGet_FrameReport(
        const UWorld* InWorld)
    -> TOptional<FCk_PixelArt_FrameReport>
{
    check(IsInGameThread());

    if (InWorld == nullptr)
    { return {}; }

    const auto* Found = ck_pixel_art_state::Get_FrameReports().Find(InWorld);

    if (Found == nullptr)
    { return {}; }

    // A report from an earlier frame describes a camera that has already moved. Reporting absence is the only
    // answer that cannot be acted on by mistake.
    if (Found->FrameNumber != GFrameCounter)
    { return {}; }

    return *Found;
}

auto
    FCk_PixelArtRenderer_StateRegistry::
    TryGet_LastFrameReport(
        const UWorld* InWorld)
    -> TOptional<FCk_PixelArt_FrameReport>
{
    check(IsInGameThread());

    if (InWorld == nullptr)
    { return {}; }

    const auto* Found = ck_pixel_art_state::Get_FrameReports().Find(InWorld);

    if (Found == nullptr)
    { return {}; }

    if (Found->FrameNumber + 1 < GFrameCounter)
    { return {}; }

    return *Found;
}
