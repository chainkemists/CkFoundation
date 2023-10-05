using System;
using UnrealBuildTool;

public class CkModuleRules : ModuleRules
{
    enum BuildConfiguration
    {
        MatchWithUnreal,
        Profile
    }

    void SetBuildConfiguration()
    {
        // override this variable to change the configuration settings on a broad level
        var BuildConfigurationOverride = BuildConfiguration.MatchWithUnreal;

        switch(BuildConfigurationOverride)
        {
            case BuildConfiguration.MatchWithUnreal:
            {
                switch(Target.Configuration)
                {
                    case UnrealTargetConfiguration.Unknown:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=0");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        break;
                    case UnrealTargetConfiguration.Debug:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=0");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        break;
                    case UnrealTargetConfiguration.DebugGame:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=0");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        break;
                    case UnrealTargetConfiguration.Development:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=0");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        break;
                    case UnrealTargetConfiguration.Test:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=0");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        break;
                    case UnrealTargetConfiguration.Shipping:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=1");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=1");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=1");
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
                break;
            }
            case BuildConfiguration.Profile:
                PublicDefinitions.Add("CK_LOG_NO_CONTEXT=1");
                PublicDefinitions.Add("CK_BYPASS_ENSURES=1");
                break;
            default:
                throw new ArgumentOutOfRangeException();
        }
    }

    public CkModuleRules(ReadOnlyTargetRules Target) : base(Target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        SetBuildConfiguration();
    }
}

public class CkCore : CkModuleRules
{
    public CkCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CkThirdParty",
                // ... add other public dependencies that you statically link with here ...
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "GameplayTags",
                "DeveloperSettings",

                "CkThirdParty",
                // ... add private dependencies that you statically link with here ...
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );
    }
}