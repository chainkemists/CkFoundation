#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "CkUsf_GeneratorSubsystem.generated.h"

class UCkUsf_LookDefinition;

UCLASS()
class CKUSFEDITOR_API UCkUsf_GeneratorSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    // Regenerates every look master. Also reachable via the `Ck_Usf_GenerateLooks` console command.
    UFUNCTION(CallInEditor, Category = "CkUsf",
              DisplayName = "Generate Look Materials")
    void Generate_LookMaterials();

    // Regenerates one look's master (scriptable per-look entry point — editor utilities, AS, Python).
    // Console equivalent: `Ck_Usf_GenerateLooks <LookName>`.
    UFUNCTION(BlueprintCallable, Category = "CkUsf",
              DisplayName = "[Ck][Usf] Generate Look Material For")
    void Generate_LookMaterial_For(UCkUsf_LookDefinition* InLook);
};
