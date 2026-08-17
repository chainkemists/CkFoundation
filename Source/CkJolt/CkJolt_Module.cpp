#include "CkJolt_Module.h"

#if WITH_EDITOR
#include <UObject/ICookInfo.h>
#endif

#define LOCTEXT_NAMESPACE "FCkJoltModule"

#if WITH_EDITOR
namespace ck_jolt_module
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
                { return InRule.PackageName == PackageName && InRule.CookRule == UE::Cook::EPackageCookRule::AddToCook; }))
            { return; }

            auto& Rule = InOutCookRules.AddDefaulted_GetRef();
            Rule.PackageName = PackageName;
            Rule.InstigatorName = FName{TEXT("CkJoltDebugDraw")};
            Rule.CookRule = UE::Cook::EPackageCookRule::AddToCook;
        };

        // The debugger loads these by name at runtime; no content asset hard-references them. Registering explicit
        // cook rules keeps Development packages from silently falling back to the default material.
        AddRule(TEXT("/Engine/EngineDebugMaterials/M_SimpleOpaque"));
        AddRule(TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent"));
        AddRule(TEXT("/Engine/EngineDebugMaterials/WireframeMaterial"));
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

void FCkJoltModule::StartupModule()
{
#if WITH_EDITOR
    _ModifyCookHandle = UE::Cook::FDelegates::ModifyCook.AddStatic(&ck_jolt_module::Add_DebugMaterialCookRules);
#endif
}

void FCkJoltModule::ShutdownModule()
{
#if WITH_EDITOR
    if (_ModifyCookHandle.IsValid())
    {
        UE::Cook::FDelegates::ModifyCook.Remove(_ModifyCookHandle);
        _ModifyCookHandle.Reset();
    }
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkJoltModule, CkJolt)
