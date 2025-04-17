using System.IO;
using UnrealBuildTool;

public class CkThirdParty : ModuleRules
{
	public CkThirdParty(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicIncludePaths.AddRange(
			new[]
			{
				Path.Combine(ModuleDirectory, "Public/CkThirdParty/entt-3.12.2/src/"),
				Path.Combine(ModuleDirectory, "Public/CkThirdParty/ctti/include"),
				Path.Combine(ModuleDirectory, "Public/CkThirdParty/cleantype/src/include"),
				Path.Combine(ModuleDirectory, "Public/CkThirdParty/JoltPhysics"),
				Path.Combine(ModuleDirectory, "Public/CkThirdParty/delegate/include")
			}
		);

		IWYUSupport = IWYUSupport.None;

		// Use this conditional approach
		PublicDefinitions.Add("JPH_ENABLE_ASSERTS");
		if (Target.Type == TargetType.Server)
		{
			// Server build configuration
			// PublicDefinitions.Add("JPH_SHARED_LIBRARY");
			// Do NOT add JPH_BUILD_SHARED_LIBRARY for server
		}
		else
		{
			// Client and Editor configuration
			PublicDefinitions.Add("JPH_SHARED_LIBRARY");
			PrivateDefinitions.Add("JPH_BUILD_SHARED_LIBRARY");
		}

		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"GameplayTags",
				"DeveloperSettings"
			}
		);
	}
}