#include "CkGameplayTag_EditorConsoleCommands.h"

#include "CkCoreEditor/CkCoreEditor_Log.h"

#include "HAL/IConsoleManager.h"

// ----------------------------------------------------------------------------------------------------------------

namespace ck
{
    static FAutoConsoleCommand AddGameplayTagCmd(
        TEXT("AddGameplayTag"),
        TEXT("AddGameplayTag <TagName> [OptionalDescription]\nAdds a gameplay tag. Can use '.' or '_' as separators."),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            if (Args.Num() < 1)
            {
                core_editor::Warning(TEXT("Usage: AddGameplayTag <TagName> [OptionalDescription]"));
                return;
            }

            auto TagName = Args[0];
            const auto& Description = Args.Num() > 1 ? Args[1] : TEXT("Added via AddGameplayTag console command");

            // if no period separators, assume this is using Angelscript tag name format which separates with underscores instead
            if (NOT TagName.Contains(TEXT(".")))
            {
                TagName = TagName.Replace(TEXT("_"), TEXT("."));
            }

            core_editor::Display(TEXT("Adding gameplay tag: [{}] (Description: [{}])"), TagName, Description);

            UCk_Utils_GameplayTag_UE::ResolveGameplayTag(FName(TagName), Description);
        })
    );
}

// ----------------------------------------------------------------------------------------------------------------
