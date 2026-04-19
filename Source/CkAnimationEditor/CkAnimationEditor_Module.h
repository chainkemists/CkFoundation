#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleInterface.h>
#include <Framework/Docking/TabManager.h>

// --------------------------------------------------------------------------------------------------------------------

class FCk_AnimationEditorModule : public IModuleInterface
{
public:
    // IModuleInterface
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;

public:
    static inline const FName TabName = TEXT("CkAnimationScrunchFixer");
    static inline const FString TabDisplayName = TEXT("Anim Scrunch Fixer");

private:
    auto DoRegisterTabSpawner() -> void;
    auto DoUnregisterTabSpawner() -> void;
    auto DoOnSpawnTab(const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>;
};

// --------------------------------------------------------------------------------------------------------------------
