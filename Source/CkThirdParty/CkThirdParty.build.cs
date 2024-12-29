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
				Path.Combine(ModuleDirectory, "Public/CkThirdParty/JoltPhysics")
			}
		);

		IWYUSupport = IWYUSupport.None;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicDefinitions.Add("_HAS_EXCEPTIONS=1");
			PublicDefinitions.Add("JPH_SHARED_LIBRARY");
			PrivateDefinitions.Add("JPH_BUILD_SHARED_LIBRARY");
			bUseUnity = false;
		}

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