#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "CkAngelscriptGenerator_DriftCommandlet.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * CI guardrail for the Rev 10 AS bootstrap self-heal — runs the
 * deterministic generators (EntitySpawnParams, AutoTestActors,
 * DynamicHandleTypes.json) headlessly and exits cleanly, leaving any
 * drift visible to the surrounding CI script via `git diff --exit-code`.
 *
 * Invocation:
 *   UnrealEditor-Cmd.exe <Project.uproject> -run=CkAngelscriptGeneratorDrift
 *
 * Workflow in CI:
 *   1. Checkout merge commit's worktree.
 *   2. Run this commandlet — generators write fresh files. The
 *      EntitySpawnParams + AutoTestActors generators short-circuit if
 *      content is already current (LF-normalized compare). The
 *      DynamicHandleTypes.json regen uses the editor subsystem path
 *      (GEditor IS available in commandlet context with IsEditor=true).
 *   3. CI runs `git diff --exit-code <Script/Generated paths>`. If the
 *      working tree has any modification (because the committed file
 *      doesn't match fresh regen), the diff fails and so does CI —
 *      the author needs to commit the regenerated file.
 *
 * The commandlet itself always exits 0 unless a generator throws or
 * GEditor isn't available. The drift verdict lives in the post-
 * commandlet `git diff` step. This split keeps the failure log helpful
 * (specific drifted files appear in the diff) instead of generic
 * ("commandlet failed, look elsewhere").
 *
 * Scope: tier-1 reflection-only (EntitySpawnParams, AutoTestActors)
 * and tier-2 AR-scoped + JSON (DynamicHandleTypes). Tier-3 (the AR-
 * driven asset-registry generator) is async-by-design and would need
 * engine ticks to complete inside the commandlet — out of scope here;
 * the editor-time AR-change listener keeps `BusterBlockAssets.as` in
 * sync during normal development.
 */
UCLASS()
class CKANGELSCRIPTGENERATOR_API UCkAngelscriptGenerator_DriftCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCkAngelscriptGenerator_DriftCommandlet();

    virtual int32 Main(const FString& Params) override;
};

// --------------------------------------------------------------------------------------------------------------------
