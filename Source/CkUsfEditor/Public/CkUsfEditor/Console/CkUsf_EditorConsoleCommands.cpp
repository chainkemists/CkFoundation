// Editor console entry points for the CkUsf look generator (the CallInEditor subsystem button is
// not reachable without scripting; this is). Registered for the editor process only.

#include "CkUsfEditor/Generator/CkUsf_Generator.h"
#include "CkUsfEditor_Log.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_usf_editor_console
{
    static auto Find_LookDefinition_ByName(const FString& InLookName) -> UCkUsf_LookDefinition*
    {
        const auto& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        TArray<FAssetData> Assets;
        ARM.GetAssetsByClass(UCkUsf_LookDefinition::StaticClass()->GetClassPathName(), Assets);

        for (const auto& Asset : Assets)
        {
            auto* Def = Cast<UCkUsf_LookDefinition>(Asset.GetAsset());
            if (Def != nullptr && Def->Get_EffectiveLookName() == FName{*InLookName})
            { return Def; }
        }
        return nullptr;
    }

    static FAutoConsoleCommand GCk_Usf_GenerateLooksCommand(
        TEXT("Ck_Usf_GenerateLooks"),
        TEXT("Ck_Usf_GenerateLooks [LookName]\n")
        TEXT("Regenerates CkUsf look master materials. No argument = every look; with an argument, ")
        TEXT("only the look whose effective name matches (e.g. Ck_Usf_GenerateLooks Hologram).\n")
        TEXT("For .ush-only edits you do not need to regenerate — run `recompileshaders changed` instead."),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs)
        {
            if (InArgs.IsEmpty())
            {
                ck::usf_editor::Generate_AllLookMaterials();
                return;
            }

            auto* Def = Find_LookDefinition_ByName(InArgs[0]);
            if (Def == nullptr)
            {
                ck::usf_editor::Warning(
                    TEXT("Ck_Usf_GenerateLooks: no UCkUsf_LookDefinition with effective name [{}]"), InArgs[0]);
                return;
            }
            ck::usf_editor::Generate_LookMaterial(Def);
        }));
}

// --------------------------------------------------------------------------------------------------------------------
