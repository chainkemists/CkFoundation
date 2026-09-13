#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/ProjectSettings/CkProjectSettings.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkUsf_Outline_ProjectSettings.generated.h"

class UCkUsf_OutlinePreset;
class FProperty;
struct FPropertyChangedEvent;

USTRUCT(BlueprintType)
struct CKUSF_API FCk_Usf_OutlineDefinition
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Usf_OutlineDefinition);

private:
    UPROPERTY(EditAnywhere, Category = "Outline", meta = (Categories = "Outline", AllowPrivateAccess = true))
    FGameplayTag _OutlineTag;

    UPROPERTY(EditAnywhere, Category = "Outline", meta = (Categories = "Layer", AllowPrivateAccess = true))
    FGameplayTag _LayerTag;

    UPROPERTY(EditAnywhere, Category = "Outline", meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCkUsf_OutlinePreset> _Preset;

public:
    CK_PROPERTY_GET(_OutlineTag);
    CK_PROPERTY_GET(_LayerTag);
    CK_PROPERTY_GET(_Preset);

    FCk_Usf_OutlineDefinition() = default;
    FCk_Usf_OutlineDefinition(
        FGameplayTag InOutlineTag,
        FGameplayTag InLayerTag,
        TSoftObjectPtr<UCkUsf_OutlinePreset> InPreset)
        : _OutlineTag{InOutlineTag}
        , _LayerTag{InLayerTag}
        , _Preset{MoveTemp(InPreset)}
    {}
};

struct CKUSF_API FCk_Usf_OutlineRuntimeDefinition
{
    FGameplayTag OutlineTag;
    FGameplayTag LayerTag;
    TObjectPtr<UCkUsf_OutlinePreset> Preset;
    int32 LayerIndex = INDEX_NONE;
};

struct CKUSF_API FCk_Usf_OutlineRuntimeConfig
{
    TArray<FCk_Usf_OutlineRuntimeDefinition> Definitions;

    auto TryGet(const FGameplayTag& InOutlineTag) const -> const FCk_Usf_OutlineRuntimeDefinition*;
};

UCLASS(meta = (DisplayName = "Usf Outline"))
class CKUSF_API UCk_Usf_Outline_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Usf_Outline_ProjectSettings_UE);

    explicit UCk_Usf_Outline_ProjectSettings_UE(const FObjectInitializer& InObjectInitializer);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Resolution",
              meta = (Categories = "Layer", AllowPrivateAccess = true))
    TArray<FGameplayTag> _LayerPrecedenceHighestFirst;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Resolution",
              meta = (TitleProperty = "_OutlineTag", AllowPrivateAccess = true))
    TArray<FCk_Usf_OutlineDefinition> _OutlineDefinitions;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCkUsf_OutlinePreset>> _LoadedOutlinePresets;

    FCk_Usf_OutlineRuntimeConfig _CachedRuntimeConfig;
    bool _RuntimeConfigIsCached = false;

    friend class UCk_Utils_Usf_Outline_Settings_UE;

    auto Invalidate_RuntimeConfig() -> void;

public:
    CK_PROPERTY_GET(_LayerPrecedenceHighestFirst);
    CK_PROPERTY_GET(_OutlineDefinitions);

    virtual void PostReloadConfig(FProperty* InPropertyThatWasLoaded) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) override;
#endif
};

UCLASS()
class CKUSF_API UCk_Utils_Usf_Outline_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Usf_Outline_Settings_UE);

public:
    static const UCk_Usf_Outline_ProjectSettings_UE* Get();

    static bool
    TryBuild_RuntimeConfig(
        const TArray<FGameplayTag>& InLayerPrecedenceHighestFirst,
        const TArray<FCk_Usf_OutlineDefinition>& InDefinitions,
        FCk_Usf_OutlineRuntimeConfig& OutConfig);

    static bool TryGet_RuntimeConfig(FCk_Usf_OutlineRuntimeConfig& OutConfig);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Outline")
    static FGameplayTag Get_SelectionOutlineTag();

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Outline")
    static FGameplayTag Get_GameplayInteractionOutlineTag();

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Outline")
    static FGameplayTag Get_GameplayEmphasisOutlineTag();

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Outline")
    static FGameplayTag Get_GameplayGuidanceOutlineTag();
};
