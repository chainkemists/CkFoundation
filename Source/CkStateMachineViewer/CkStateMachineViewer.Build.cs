using UnrealBuildTool;

public class CkStateMachineViewer : CkModuleRules
{
    public CkStateMachineViewer(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "InputCore",
            "ImGui",

            "CkCore",
            "CkLog",
            "CkEcs",
            "CkStateMachine",
        });
    }
}
