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
 *       TypeName = "FCk_Handle_Weapon";
 *       RequiredFragments.Add(FFragment_Weapon::StaticStruct());
 *       Description = "A weapon entity handle.";
 *   }
 *
 * Supported TypeName formats:
 *   - "FCk_Handle_Weapon" -> ShortName: "Weapon"
 *   - "Handle_Weapon"     -> ShortName: "Weapon"
 *   - "Weapon"            -> ShortName: "Weapon"
 *   - Or set ShortName explicitly for full control
 */
UCLASS(BlueprintType)
class CKDYNAMIC_API UCkDynamic_HandleDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkDynamic_HandleDefinition);

public:
    /**
     * The type name for this handle (e.g., "FCk_Handle_Weapon", "Handle_Weapon", or "Weapon").
     * This becomes the AngelScript type name.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handle Definition")
    FString TypeName;

    /**
     * Optional short name override for As_X() and Is_X() methods.
     * If empty, it will be extracted from TypeName by removing known prefixes.
     *
     * Examples:
     *   - TypeName "FCk_Handle_Weapon" with empty ShortName -> "Weapon"
     *   - TypeName "Handle_Weapon" with empty ShortName -> "Weapon"
     *   - TypeName "MyCustomHandle" with ShortName "Custom" -> "Custom"
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handle Definition",
        meta = (DisplayName = "Short Name (Optional)"))
    FString ShortName;

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
     * Get the short name for this handle type.
     * Returns the explicit ShortName if set, otherwise extracts from TypeName.
     */
    UFUNCTION(BlueprintCallable,
        Category = "Handle Definition")
    FString
    GetShortName() const;

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