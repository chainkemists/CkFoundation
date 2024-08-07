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

        // normally, detailed formatting is invoked using {d}, this switch will force detailed formatting (if supported by formatter)
        PublicDefinitions.Add("CK_FORMAT_FORCE_DETAILED=0");
        PublicDefinitions.Add("CK_DEBUG_NAME_FORCE_VERBOSE=0");
        PublicDefinitions.Add("CK_DISABLE_ABILITY_SCRIPT_DEBUGGING=0");

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
                        PublicDefinitions.Add("CK_ECS_DISABLE_HANDLE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_COPY_NET_PARAMS_ON_EVERY_ENTITY=1");
                        PublicDefinitions.Add("CK_ENABLE_STAT_DESCRIPTION=1");
                        PublicDefinitions.Add("CK_VALIDATE_GAMEPLAYTAG_STALENESS=1");
                        break;
                    case UnrealTargetConfiguration.Debug:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=0");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        PublicDefinitions.Add("CK_ECS_DISABLE_HANDLE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_COPY_NET_PARAMS_ON_EVERY_ENTITY=1");
                        PublicDefinitions.Add("CK_ENABLE_STAT_DESCRIPTION=1");
                        PublicDefinitions.Add("CK_VALIDATE_GAMEPLAYTAG_STALENESS=1");
                        break;
                    case UnrealTargetConfiguration.DebugGame:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=0");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        PublicDefinitions.Add("CK_ECS_DISABLE_HANDLE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_COPY_NET_PARAMS_ON_EVERY_ENTITY=1");
                        PublicDefinitions.Add("CK_ENABLE_STAT_DESCRIPTION=1");
                        PublicDefinitions.Add("CK_VALIDATE_GAMEPLAYTAG_STALENESS=1");
                        break;
                    case UnrealTargetConfiguration.Development:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=0");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        PublicDefinitions.Add("CK_ECS_DISABLE_HANDLE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_COPY_NET_PARAMS_ON_EVERY_ENTITY=1");
                        PublicDefinitions.Add("CK_ENABLE_STAT_DESCRIPTION=1");
                        PublicDefinitions.Add("CK_VALIDATE_GAMEPLAYTAG_STALENESS=1");
                        break;
                    case UnrealTargetConfiguration.Test:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=0");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=1");
                        PublicDefinitions.Add("CK_ECS_DISABLE_HANDLE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_COPY_NET_PARAMS_ON_EVERY_ENTITY=0");
                        PublicDefinitions.Add("CK_ENABLE_STAT_DESCRIPTION=1");
                        PublicDefinitions.Add("CK_VALIDATE_GAMEPLAYTAG_STALENESS=1");
                        break;
                    case UnrealTargetConfiguration.Shipping:
                        PublicDefinitions.Add("CK_BYPASS_ENSURES=1");
                        PublicDefinitions.Add("CK_LOG_NO_CONTEXT=1");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=1");
                        PublicDefinitions.Add("CK_ECS_DISABLE_HANDLE_DEBUGGING=1");
                        PublicDefinitions.Add("CK_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_COPY_NET_PARAMS_ON_EVERY_ENTITY=0");
                        PublicDefinitions.Add("CK_ENABLE_STAT_DESCRIPTION=1");
                        PublicDefinitions.Add("CK_VALIDATE_GAMEPLAYTAG_STALENESS=0");
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
                break;
            }
            case BuildConfiguration.Profile:
                PublicDefinitions.Add("CK_LOG_NO_CONTEXT=1");
                PublicDefinitions.Add("CK_BYPASS_ENSURES=1");
                PublicDefinitions.Add("CK_MEMORY_TRACKING=1");
                PublicDefinitions.Add("CK_ECS_DISABLE_HANDLE_DEBUGGING=1");
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

public class CkBuildConfig : CkModuleRules
{
    public CkBuildConfig(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
        });
    }
}
