#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Emits, per plugin (plus one bucket for the game module), a namespace per UCk_EntityScript_UE
// subclass exposing a USTRUCT mirror of its UPROPERTY(ExposeOnSpawn) properties, flattened across
// the hierarchy because AngelScript has no struct inheritance.
//
// Output:   <PluginBaseDir|ProjectDir>/Script/Generated/<Name>_EntitySpawnParams.as
// Triggers: FCoreDelegates::OnPostEngineInit + FAngelscriptCodeModule::GetPostCompile().
class CKANGELSCRIPTGENERATOR_API FCkAngelscriptEntityScriptParamsGenerator
{
public:
    static auto
    GenerateAll() -> void;

    // Exposed for unit tests: the Blueprint-generated-class exclusion is load-bearing for
    // cross-process determinism (BP classes only exist in processes that loaded the BP).
    static auto
    Is_IncludedEntityScriptClass(
        UClass* InClass) -> bool;

    // EntitySpawnParams are retained in untraced FInstancedStruct storage before injection into the traced
    // EntityScript UObject. Direct strong UObject properties therefore become weak references in the generated
    // mirror; already-weak/soft wrappers and non-object types retain their reflected AngelScript spelling.
    static auto
    Get_RetainedPropertyType(
        FProperty* InProperty) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
