#pragma once

#include "CoreMinimal.h"

#include "CkPixelArtRender/CkPixelArtRender_State.h"

// --------------------------------------------------------------------------------------------------------------------

// Console overrides for the pixel-art renderer, all under `ck.PixelArt.*`.
//
// EVERY override uses a NEGATIVE value as "no override", so the game's configuration stays the source of truth
// and the console is strictly a developer overlay on top. That convention works because every setting these reach
// has a non-negative domain, so one number means both "absent" and "leave it alone" — which is what makes it safe
// to leave one set in an .ini. Mirrors the `ck.Usf.*` Stylize overrides.
//
// The overlay is folded in on the way to the renderer and is never written back into the registry, so what the
// game asked for keeps reading back unchanged while the screen shows what the console asked for, and clearing a
// CVar back to -1 restores the configured value with no re-apply.
namespace ck::pixel_art
{
    DECLARE_MULTICAST_DELEGATE(FCk_PixelArt_OnCVarChanged);

    // Broadcast on every change to any of the overrides below. Without it a console flip would only take effect
    // at whatever later moment something else happened to touch the configuration.
    CKPIXELARTRENDER_API auto Get_OnCVarChanged() -> FCk_PixelArt_OnCVarChanged&;

    // Applies the console overlay to a configuration read out of the registry. Pure overlay: it neither reads nor
    // writes the registry.
    CKPIXELARTRENDER_API auto Fold_Overrides(
        FCk_PixelArt_RenderConfig& InOutConfig) -> void;

    // Whether `ck.PixelArt.Debug.LogState` asked for per-transition state logging.
    CKPIXELARTRENDER_API auto Get_LogStateEnabled() -> bool;
}
