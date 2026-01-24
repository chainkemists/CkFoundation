#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"

#include "CkDynamicHandleSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkDynamic_HandleDefinition;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Editor subsystem for generating the JSON registry of dynamic handle types.
 *
 * Workflow:
 * 1. Define handle types via AS asset declarations (using UCkDynamic_HandleDefinition)
 * 2. Call GenerateHandleTypeRegistry() to create the JSON file
 * 3. Restart editor - C++ reads JSON during pre-compile, types are available
 *
 * The JSON file is read by FCkDynamic_HandleTypeRegistry during the AngelScript
 * pre-compile phase, ensuring types are registered before AS compilation begins.
 */
UCLASS()
class CKANGELSCRIPTGENERATOR_API UCkDynamicHandleSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    virtual auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    virtual auto Deinitialize() -> void override;

public:
    /**
     * Discovers all UCkDynamic_HandleDefinition assets and generates a JSON registry file.
     * The JSON file is read by C++ during pre-compile to register handle types.
     */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ck|DynamicHandles")
    void
    GenerateHandleTypeRegistry();

    /**
     * Returns the output path for the generated JSON file.
     */
    UFUNCTION(BlueprintCallable,
        Category = "Ck|DynamicHandles")
    static
    FString Get_RegistryFilePath();

    /**
     * Check if generation is needed (definitions have changed since last generation).
     */
    UFUNCTION(BlueprintCallable,
        Category = "Ck|DynamicHandles")
    bool
    IsRegenerationNeeded() const;

    /**
     * Get the number of handle types that would be generated.
     */
    UFUNCTION(BlueprintCallable,
        Category = "Ck|DynamicHandles")
    int32
    Get_DiscoveredDefinitionCount() const;

    UFUNCTION(BlueprintCallable, CallInEditor,
        Category = "Ck|DynamicHandles",
        meta = (DevelopmentOnly))
    int32
    ForceRefreshDynamicHandleBindings();

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FOnHandleTypeGenerationComplete,
        int32, GeneratedCount,
        bool, Success);

    UPROPERTY(BlueprintAssignable, Category = "Ck|DynamicHandles")
    FOnHandleTypeGenerationComplete OnGenerationComplete;

private:
    auto DiscoverAllDefinitions() const -> TArray<UCkDynamic_HandleDefinition*>;
    auto BuildJsonContent(const TArray<UCkDynamic_HandleDefinition*>& InDefinitions) const -> FString;
    auto ComputeDefinitionsHash(const TArray<UCkDynamic_HandleDefinition*>& InDefinitions) const -> uint32;

private:
    uint32 LastGeneratedHash = 0;
};

// --------------------------------------------------------------------------------------------------------------------