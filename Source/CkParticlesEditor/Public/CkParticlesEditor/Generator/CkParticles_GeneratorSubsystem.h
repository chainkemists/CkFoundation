#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "CkParticles_GeneratorSubsystem.generated.h"

UCLASS()
class CKPARTICLESEDITOR_API UCkParticles_GeneratorSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    // Build the seed template Niagara System from C++ (no hand-authored asset). Run this once before generating.
    UFUNCTION(CallInEditor, Category = "CkParticles",
              DisplayName = "Create Template System")
    void Create_TemplateSystem();

    UFUNCTION(CallInEditor, Category = "CkParticles",
              DisplayName = "Generate Particle Systems")
    void Generate_ParticleSystems();
};
