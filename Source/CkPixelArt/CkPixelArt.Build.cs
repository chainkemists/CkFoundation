using UnrealBuildTool;

// The game-facing half of the pixel-art renderer: presets, project settings, and the world subsystem that
// decides what CkPixelArtRender does. Loads at the ordinary Default phase, which is exactly why the split
// exists — CkPixelArtRender has to load at PostConfigInit (before the global shader map is built) and can
// therefore link no Ck module at all.
public class CkPixelArt : CkModuleRules
{
    public CkPixelArt(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "GameplayTags",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSettings",
            "CkUsf",

            "CkPixelArtRender",
        });
    }
}
