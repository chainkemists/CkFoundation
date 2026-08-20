#include "CkPixelArtRender/CkPixelArtRender_State.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pixel_art_state
{
    using FRegistryMap = TMap<TWeakObjectPtr<const UWorld>, FCk_PixelArt_RenderConfig>;

    auto
        Get_Registry()
        -> FRegistryMap&
    {
        static FRegistryMap Registry;
        return Registry;
    }

    auto
        DoSweep_StaleKeys(
            FRegistryMap& InOutRegistry)
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
    FCk_PixelArtRender_StateRegistry::
    Set(
        const UWorld* InWorld,
        const FCk_PixelArt_RenderConfig& InConfig)
    -> void
{
    check(IsInGameThread());

    // ensureMsgf rather than the house CK_ENSURE_IF_NOT: this module cannot link CkCore at PostConfigInit. It is
    // still an ensure and not a log-and-continue — a null key here would otherwise become a registry entry that
    // no world can ever match or clear.
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
    FCk_PixelArtRender_StateRegistry::
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
    FCk_PixelArtRender_StateRegistry::
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
