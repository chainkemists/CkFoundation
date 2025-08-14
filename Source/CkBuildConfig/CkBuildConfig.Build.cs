using System;
using UnrealBuildTool;
using EpicGames.Core;
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
        const BuildConfiguration BuildConfigurationOverride = BuildConfiguration.MatchWithUnreal;

        // normally, detailed formatting is invoked using {d}, this switch will force detailed formatting (if supported by formatter)
        PublicDefinitions.Add("CK_FORMAT_FORCE_DETAILED=0");
        PublicDefinitions.Add("CK_DEBUG_NAME_FORCE_VERBOSE=0");

        if (JsonObject.TryRead(Target.ProjectFile, out var RawObject))
        {
            if (RawObject.TryGetObjectArrayField("Plugins", out var PluginObjects))
            {
                foreach (JsonObject PluginObject in PluginObjects)
                {
                    PluginObject.TryGetStringField("Name", out var PluginName);

                    PluginObject.TryGetBoolField("Enabled", out var PluginEnabled);

                    if (PluginName == "AngelScript" && PluginEnabled)
                    {
                        PrivateDependencyModuleNames.Add("AngelScript");
                        PrivateDependencyModuleNames.Add("AngelscriptCode");
                        PublicDefinitions.Add("WITH_ANGELSCRIPT_CK=1");
                    }
                }
            }
        }
        else
        {
            // Explicitly define it as 0 when not available
            PublicDefinitions.Add("WITH_ANGELSCRIPT_CK=0");
        }

        switch(BuildConfigurationOverride)
        {
            case BuildConfiguration.MatchWithUnreal:
            {
                switch(Target.Configuration)
                {
                    case UnrealTargetConfiguration.Unknown:
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_CHECKS=0");
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_DISABLE_LOG_CONTEXT=1");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        PublicDefinitions.Add("CK_DISABLE_ECS_HANDLE_DEBUGGING=1");
                        PublicDefinitions.Add("CK_ENABLE_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_DISABLE_NET_PARAM_COPY_PER_ENTITY=0");
                        PublicDefinitions.Add("CK_DISABLE_STAT_DESCRIPTION=1");
                        PublicDefinitions.Add("CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION=1");
                        PublicDefinitions.Add("CK_DISABLE_ABILITY_SCRIPT_DEBUGGING=0");
                        break;
                    case UnrealTargetConfiguration.Debug:
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_CHECKS=0");
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_DISABLE_LOG_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        PublicDefinitions.Add("CK_DISABLE_ECS_HANDLE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_ENABLE_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_DISABLE_NET_PARAM_COPY_PER_ENTITY=0");
                        PublicDefinitions.Add("CK_DISABLE_STAT_DESCRIPTION=0");
                        PublicDefinitions.Add("CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION=0");
                        PublicDefinitions.Add("CK_DISABLE_ABILITY_SCRIPT_DEBUGGING=0");
                        break;
                    case UnrealTargetConfiguration.DebugGame:
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_CHECKS=0");
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_DISABLE_LOG_CONTEXT=0");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                        PublicDefinitions.Add("CK_DISABLE_ECS_HANDLE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_ENABLE_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_DISABLE_NET_PARAM_COPY_PER_ENTITY=0");
                        PublicDefinitions.Add("CK_DISABLE_STAT_DESCRIPTION=0");
                        PublicDefinitions.Add("CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION=0");
                        PublicDefinitions.Add("CK_DISABLE_ABILITY_SCRIPT_DEBUGGING=0");
                        break;
                    case UnrealTargetConfiguration.Development:
                        if (Target.bBuildEditor)
                        {
                            PublicDefinitions.Add("CK_DISABLE_ENSURE_CHECKS=0");
                            PublicDefinitions.Add("CK_DISABLE_ENSURE_DEBUGGING=0");
                            PublicDefinitions.Add("CK_DISABLE_LOG_CONTEXT=0");
                            PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                            PublicDefinitions.Add("CK_DISABLE_ECS_HANDLE_DEBUGGING=0");
                            PublicDefinitions.Add("CK_ENABLE_MEMORY_TRACKING=0");
                            PublicDefinitions.Add("CK_DISABLE_NET_PARAM_COPY_PER_ENTITY=0");
                            PublicDefinitions.Add("CK_DISABLE_STAT_DESCRIPTION=0");
                            PublicDefinitions.Add("CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION=0");
                            PublicDefinitions.Add("CK_DISABLE_ABILITY_SCRIPT_DEBUGGING=0");
                        }
                        else
                        {
                            PublicDefinitions.Add("CK_DISABLE_ENSURE_CHECKS=0");
                            PublicDefinitions.Add("CK_DISABLE_ENSURE_DEBUGGING=0");
                            PublicDefinitions.Add("CK_DISABLE_LOG_CONTEXT=1");
                            PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=0");
                            PublicDefinitions.Add("CK_DISABLE_ECS_HANDLE_DEBUGGING=1");
                            PublicDefinitions.Add("CK_ENABLE_MEMORY_TRACKING=0");
                            PublicDefinitions.Add("CK_DISABLE_NET_PARAM_COPY_PER_ENTITY=0");
                            PublicDefinitions.Add("CK_DISABLE_STAT_DESCRIPTION=0");
                            PublicDefinitions.Add("CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION=1");
                            PublicDefinitions.Add("CK_DISABLE_ABILITY_SCRIPT_DEBUGGING=1");
                        }
                        break;
                    case UnrealTargetConfiguration.Test:
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_CHECKS=0");
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_DEBUGGING=1");
                        PublicDefinitions.Add("CK_DISABLE_LOG_CONTEXT=1");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=1");
                        PublicDefinitions.Add("CK_DISABLE_ECS_HANDLE_DEBUGGING=1");
                        PublicDefinitions.Add("CK_ENABLE_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_DISABLE_NET_PARAM_COPY_PER_ENTITY=0");
                        PublicDefinitions.Add("CK_DISABLE_STAT_DESCRIPTION=1");
                        PublicDefinitions.Add("CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION=1");
                        PublicDefinitions.Add("CK_DISABLE_ABILITY_SCRIPT_DEBUGGING=1");
                        break;
                    case UnrealTargetConfiguration.Shipping:
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_CHECKS=1");
                        PublicDefinitions.Add("CK_DISABLE_ENSURE_DEBUGGING=0");
                        PublicDefinitions.Add("CK_DISABLE_LOG_CONTEXT=1");
                        PublicDefinitions.Add("CK_DISABLE_STACK_TRACE=1");
                        PublicDefinitions.Add("CK_DISABLE_ECS_HANDLE_DEBUGGING=1");
                        PublicDefinitions.Add("CK_ENABLE_MEMORY_TRACKING=0");
                        PublicDefinitions.Add("CK_DISABLE_NET_PARAM_COPY_PER_ENTITY=0");
                        PublicDefinitions.Add("CK_DISABLE_STAT_DESCRIPTION=1");
                        PublicDefinitions.Add("CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION=1");
                        PublicDefinitions.Add("CK_DISABLE_ABILITY_SCRIPT_DEBUGGING=1");
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
                break;
            }
            // ReSharper disable once UnreachableSwitchCaseDueToIntegerAnalysis
            case BuildConfiguration.Profile:
                PublicDefinitions.Add("CK_DISABLE_LOG_CONTEXT=1");
                PublicDefinitions.Add("CK_DISABLE_ENSURE_CHECKS=1");
                PublicDefinitions.Add("CK_ENABLE_MEMORY_TRACKING=1");
                PublicDefinitions.Add("CK_DISABLE_ECS_HANDLE_DEBUGGING=1");
                break;
            default:
                throw new ArgumentOutOfRangeException();
        }
    }

    public CkModuleRules(ReadOnlyTargetRules Target, bool UseUnityBuild = false) : base(Target)
    {
        bUseUnity = UseUnityBuild;
        CppStandard = CppStandardVersion.Cpp20;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        SetupIrisSupport(Target);

        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "ApplicationCore",
            "Core",
            "CoreUObject",
            "Engine",
        });

        SetBuildConfiguration();
    }
}

public class CkBuildConfig : CkModuleRules
{
    public CkBuildConfig(ReadOnlyTargetRules Target) : base(Target)
    {
    }
}
