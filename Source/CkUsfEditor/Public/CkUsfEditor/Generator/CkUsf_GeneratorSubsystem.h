#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "CkUsf_GeneratorSubsystem.generated.h"

UCLASS()
class CKUSFEDITOR_API UCkUsf_GeneratorSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(CallInEditor, Category = "CkUsf",
              DisplayName = "Generate Look Materials")
    void Generate_LookMaterials();
};
