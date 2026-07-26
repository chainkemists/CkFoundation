#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <EditorSubsystem.h>

#include "CkJoltCook_EditorSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/// Editor entry point for the Jolt static-world cooker. Callable from Blueprint editor utilities,
/// AngelScript editor scripts, and the Tools menu.
UCLASS()
class CKJOLTEDITOR_API UCk_JoltCook_EditorSubsystem_UE : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_JoltCook_EditorSubsystem_UE);

public:
    /// Cooks the currently-open editor map's static collision into Jolt data assets under the
    /// configured cooked-data root. Returns true when every asset saved.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Cook Static World (Current Map)")
    bool
    Cook_CurrentWorld();

    /// Dry run: extract + report without saving assets.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Cook Static World (Current Map, Dry Run)")
    bool
    Cook_CurrentWorld_DryRun();

    /// Recomputes source hashes for the current map against its cooked data; logs stale actors.
    /// Returns true when nothing is stale.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Validate Cooked Static World (Current Map)")
    bool
    Validate_CurrentWorld();
};

// --------------------------------------------------------------------------------------------------------------------
