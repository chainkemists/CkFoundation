#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "CkDynamic_HandleDefinition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Data asset that defines a dynamic type-safe handle for AngelScript.
 *
 * These definitions are discovered at editor startup and used to generate
 * a JSON registry file that C++ reads during pre-compile to create
 * AngelScript bindings.
 *
 * Usage in AngelScript:
 *   asset WeaponHandle of UCkDynamic_HandleDefinition
 *   {
 *       TypeName = "Handle_Weapon";
 *       RequiredFragments.Add(FFragment_Weapon::StaticStruct());
 *       Description = "A weapon entity handle.";
 *   }
 */
UCLASS(BlueprintType)
class CKDYNAMIC_API UCkDynamic_HandleDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkDynamic_HandleDefinition);

public:
    /**
     * The type name for this handle (e.g., "Handle_Weapon").
     * Should start with "Handle_" by convention.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handle Definition")
    FString TypeName;

    /**
     * Fragment types that must ALL be present for an entity to be valid as this handle type.
     * Using UScriptStruct* provides autocomplete and refactoring support in AngelScript.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handle Definition")
    TArray<UScriptStruct*> RequiredFragments;

    /**
     * Optional description for documentation purposes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handle Definition")
    FString Description;

public:
    /**
     * Get the fragment names as strings for JSON serialization.
     */
    UFUNCTION(BlueprintCallable,
        Category = "Handle Definition")
    TArray<FString>
    GetRequiredFragmentNames() const;

    /**
     * Check if this definition has valid configuration.
     */
    UFUNCTION(BlueprintCallable,
        Category = "Handle Definition")
    bool 
    IsValidDefinition() const;

    /**
     * Get a display-friendly name for this definition.
     */
    auto GetDisplayName() const -> FString;
};

// --------------------------------------------------------------------------------------------------------------------