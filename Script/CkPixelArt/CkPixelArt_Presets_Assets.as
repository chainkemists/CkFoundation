// Language=angelscript

//============================================================================
// PIXEL ART PRESETS — the authored styles, applied via the subsystem:
//   UCkPixelArt_Subsystem::Get_PixelArtSubsystem().Apply_Preset(CkPixelArt::DA_PixelArt_Crisp16);
//   UCkPixelArt_Subsystem::Get_PixelArtSubsystem().Request_SetEnabled(ECk_EnableDisable::Enable);
//
// Enabling is separate from applying on purpose: a preset says what the style IS, and turning the
// renderer on is gated on engine settings that a preset has no business overriding.
//
// The look half needs the PixelArt master on disk (console: "Ck_Usf_GenerateLooks PixelArt" on a fresh
// checkout). The renderer half does not — it runs with _ApplyLook disabled and simply renders sharp
// low-resolution, which is also how to tell the two halves apart when something looks wrong.
//============================================================================

namespace CkPixelArt
{
    // The reference style: 360p internal, an 8-colour dusk palette, and band-shift edges so every
    // outline is a colour the palette already holds.
    asset DA_PixelArt_Crisp16 of UCkPixelArt_Preset
    {
        _ResolutionMode = ECk_PixelArt_ResolutionMode::FixedHeight;
        _InternalHeight = 360;
        _MarginTexels   = 2;
        _SnapEnabled    = ECk_EnableDisable::Enable;
        _UpscaleFilter  = ECk_PixelArt_UpscaleFilter::BoxFilter;

        _ApplyLook      = ECk_EnableDisable::Enable;
        _EnableOutline  = ECk_EnableDisable::Enable;
        _EdgeMode       = ECk_PixelArt_EdgeMode::BandShift;
        _Bands          = 6;
        _PaletteMode    = ECk_PixelArt_PaletteMode::CustomPalette;
        _PaletteCount   = 8;
    }

    // Softer and more forgiving: five bands with a wide transition, flat edge colours instead of a band
    // shift, and per-channel steps rather than a fixed palette. Reads as toon shading rather than as
    // pixel art, which is what makes it the useful A/B against Crisp16 — the palette and the edge rule
    // are what carry the style, not the low resolution on its own.
    asset DA_PixelArt_SoftRamp of UCkPixelArt_Preset
    {
        _ResolutionMode        = ECk_PixelArt_ResolutionMode::FixedHeight;
        _InternalHeight        = 360;
        _MarginTexels          = 2;
        _SnapEnabled           = ECk_EnableDisable::Enable;
        _UpscaleFilter         = ECk_PixelArt_UpscaleFilter::BoxFilter;

        _ApplyLook             = ECk_EnableDisable::Enable;
        _EnableOutline         = ECk_EnableDisable::Enable;
        _EdgeMode              = ECk_PixelArt_EdgeMode::FlatColors;
        _LineDarken            = 0.5;
        _CreaseBrighten        = 0.25;
        _Bands                 = 5;
        _ThresholdGradientSize = 0.35;
        _PaletteMode           = ECk_PixelArt_PaletteMode::ColorSteps;
        _PaletteCount          = 12;
    }

    // The renderer with no stylization at all: a sharp, snapped, low-resolution image and nothing else.
    // Kept as a shipped preset rather than as a gym-only toggle because it is the control every visual
    // verdict about the look has to be read against.
    asset DA_PixelArt_RendererOnly of UCkPixelArt_Preset
    {
        _ResolutionMode = ECk_PixelArt_ResolutionMode::FixedHeight;
        _InternalHeight = 360;
        _MarginTexels   = 2;
        _SnapEnabled    = ECk_EnableDisable::Enable;
        _UpscaleFilter  = ECk_PixelArt_UpscaleFilter::BoxFilter;
        _ApplyLook      = ECk_EnableDisable::Disable;
    }
}
