#include "CkDebugScene_Module.h"

#if WITH_EDITOR
#include <UObject/ICookInfo.h>
#endif

#if WITH_EDITOR
namespace ck_debug_scene_module
{
    auto
        Add_DebugMaterialCookRules(
            UE::Cook::ICookInfo&,
            TArray<UE::Cook::FPackageCookRule>& InOutCookRules)
            -> void
    {
        const auto AddRule = [&InOutCookRules](const TCHAR* InPackageName)
        {
            const auto PackageName = FName{InPackageName};
            if (InOutCookRules.ContainsByPredicate([PackageName](const UE::Cook::FPackageCookRule& InRule)
                {
                    return InRule.PackageName == PackageName &&
                        InRule.CookRule == UE::Cook::EPackageCookRule::AddToCook;
                }))
            {
                return;
            }

            auto& Rule = InOutCookRules.AddDefaulted_GetRef();
            Rule.PackageName = PackageName;
            Rule.InstigatorName = FName{TEXT("CkDebugScene")};
            Rule.CookRule = UE::Cook::EPackageCookRule::AddToCook;
        };

        AddRule(TEXT("/Engine/EngineDebugMaterials/M_SimpleOpaque"));
        AddRule(TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent"));
        AddRule(TEXT("/Engine/EngineDebugMaterials/WireframeMaterial"));
    }
}
#endif

auto
    FCkDebugSceneModule::
    StartupModule()
    -> void
{
#if WITH_EDITOR
    _ModifyCookHandle = UE::Cook::FDelegates::ModifyCook.AddStatic(&ck_debug_scene_module::Add_DebugMaterialCookRules);
#endif
}

auto
    FCkDebugSceneModule::
    ShutdownModule()
    -> void
{
#if WITH_EDITOR
    if (_ModifyCookHandle.IsValid())
    {
        UE::Cook::FDelegates::ModifyCook.Remove(_ModifyCookHandle);
        _ModifyCookHandle.Reset();
    }
#endif
}

IMPLEMENT_MODULE(FCkDebugSceneModule, CkDebugScene)
