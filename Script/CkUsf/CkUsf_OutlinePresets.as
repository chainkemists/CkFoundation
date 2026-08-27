// --------------------------------------------------------------------------------------------------------------------
// Sample solid-outline presets for the CkUsf SolidOutline look. Apply via the outline subsystem:
//   UCkUsf_OutlineSubsystem::Get_OutlineSubsystem().Apply_Outline_To_Actor(SomeActor, CkUsf::DA_Outline_Interactable);
// Needs the project's Custom Depth-Stencil pass enabled with stencil (Config/DefaultEngine.ini r.CustomDepth=3).
// --------------------------------------------------------------------------------------------------------------------

namespace CkUsf
{
    // Standard interactable highlight: solid blue silhouette, hidden when the object is behind a wall.
    asset DA_Outline_Interactable of UCkUsf_OutlinePreset
    {
        _OutlineType       = ECk_Usf_OutlineType::Normal;
        _OutlineColor      = FLinearColor(0.10, 0.60, 1.00, 1.0);
        _OutlineBrightness = 1.5;
        _ThicknessScale    = 1.0;
    }

    // See-through green highlight with a faint fill - visible even when the object is occluded.
    asset DA_Outline_SeeThrough of UCkUsf_OutlinePreset
    {
        _OutlineType       = ECk_Usf_OutlineType::SeeThrough;
        _OutlineColor      = FLinearColor(0.20, 1.00, 0.30, 1.0);
        _OutlineBrightness = 1.0;
        _ThicknessScale    = 1.0;
        _FillEnabled       = true;
        _FillColor         = FLinearColor(0.20, 1.00, 0.30, 1.0);
        _FillOpacity       = 0.15;
    }

    // Masked see-through (stippled fill) - e.g. an objective/quest marker readable through cover.
    asset DA_Outline_MaskedObjective of UCkUsf_OutlinePreset
    {
        _OutlineType       = ECk_Usf_OutlineType::MaskedSeeThrough;
        _OutlineColor      = FLinearColor(1.00, 0.85, 0.10, 1.0);
        _OutlineBrightness = 1.2;
        _ThicknessScale    = 1.25;
        _FillEnabled       = true;
        _FillColor         = FLinearColor(1.00, 0.85, 0.10, 1.0);
        _FillOpacity       = 0.40;
    }
}
