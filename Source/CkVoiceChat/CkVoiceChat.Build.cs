using System.IO;
using UnrealBuildTool;

public class CkVoiceChat : CkModuleRules
{
    public CkVoiceChat(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        // Deps are deliberately minimal: a dep is added only when code that consumes it lands
        // (capture/playback earn AudioMixer/AudioCaptureCore/Voice; transport earns
        // NetCore/CkActorRelay; routing earns CkSpatialQuery; asset resolution earns
        // CkResourceLoader). CkRelationship is excluded by design - team channels are membership
        // configurations, not attitude queries.
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "AudioMixer",
            "Core",
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "GameplayTags",
            "Voice",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
        });
    }
}
