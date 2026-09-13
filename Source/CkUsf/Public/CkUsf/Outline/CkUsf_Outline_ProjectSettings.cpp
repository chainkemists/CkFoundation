#include "CkUsf/Outline/CkUsf_Outline_ProjectSettings.h"

#include "CkUsf/Outline/CkUsf_OutlinePreset.h"
#include "CkCore/Validation/CkIsValid.h"
#include "Math/UnrealMathUtility.h"
#include "NativeGameplayTags.h"
#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Outline_Root, "Outline");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Outline_Debugger_Selection, "Outline.Debugger.Selection");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Outline_Gameplay_Emphasis, "Outline.Gameplay.Emphasis");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Outline_Gameplay_Interaction, "Outline.Gameplay.Interaction");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Outline_Gameplay_Guidance, "Outline.Gameplay.Guidance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Layer_Root, "Layer");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Layer_Debugger, "Layer.Debugger");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Layer_Gameplay_Emphasis, "Layer.Gameplay.Emphasis");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Layer_Gameplay_Interaction, "Layer.Gameplay.Interaction");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Layer_Gameplay_Guidance, "Layer.Gameplay.Guidance");

auto FCk_Usf_OutlineRuntimeConfig::TryGet(const FGameplayTag& InOutlineTag) const
    -> const FCk_Usf_OutlineRuntimeDefinition*
{
    return Definitions.FindByPredicate(
        [&InOutlineTag](const auto& InDefinition)
        { return InDefinition.OutlineTag.MatchesTagExact(InOutlineTag); });
}

UCk_Usf_Outline_ProjectSettings_UE::UCk_Usf_Outline_ProjectSettings_UE(
    const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    _LayerPrecedenceHighestFirst = {
        TAG_Layer_Debugger,
        TAG_Layer_Gameplay_Emphasis,
        TAG_Layer_Gameplay_Interaction,
        TAG_Layer_Gameplay_Guidance};

    const auto InteractionPreset = TSoftObjectPtr<UCkUsf_OutlinePreset>{
        FSoftObjectPath{TEXT("/Script/AngelscriptAssets.DA_Outline_Interactable")}};
    const auto SeeThroughPreset = TSoftObjectPtr<UCkUsf_OutlinePreset>{
        FSoftObjectPath{TEXT("/Script/AngelscriptAssets.DA_Outline_SeeThrough")}};
    const auto GuidancePreset = TSoftObjectPtr<UCkUsf_OutlinePreset>{
        FSoftObjectPath{TEXT("/Script/AngelscriptAssets.DA_Outline_MaskedObjective")}};
    _OutlineDefinitions = {
        FCk_Usf_OutlineDefinition{TAG_Outline_Debugger_Selection, TAG_Layer_Debugger, SeeThroughPreset},
        FCk_Usf_OutlineDefinition{TAG_Outline_Gameplay_Emphasis, TAG_Layer_Gameplay_Emphasis, SeeThroughPreset},
        FCk_Usf_OutlineDefinition{TAG_Outline_Gameplay_Interaction, TAG_Layer_Gameplay_Interaction, InteractionPreset},
        FCk_Usf_OutlineDefinition{TAG_Outline_Gameplay_Guidance, TAG_Layer_Gameplay_Guidance, GuidancePreset}};
}

auto UCk_Usf_Outline_ProjectSettings_UE::Invalidate_RuntimeConfig() -> void
{
    _RuntimeConfigIsCached = false;
    _CachedRuntimeConfig = {};
    _LoadedOutlinePresets.Reset();
}

void UCk_Usf_Outline_ProjectSettings_UE::PostReloadConfig(FProperty* InPropertyThatWasLoaded)
{
    Super::PostReloadConfig(InPropertyThatWasLoaded);
    Invalidate_RuntimeConfig();
}

#if WITH_EDITOR
void UCk_Usf_Outline_ProjectSettings_UE::PostEditChangeProperty(
    FPropertyChangedEvent& InPropertyChangedEvent)
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);
    Invalidate_RuntimeConfig();
}
#endif

auto UCk_Utils_Usf_Outline_Settings_UE::Get() -> const UCk_Usf_Outline_ProjectSettings_UE*
{ return GetDefault<UCk_Usf_Outline_ProjectSettings_UE>(); }

auto UCk_Utils_Usf_Outline_Settings_UE::TryBuild_RuntimeConfig(
    const TArray<FGameplayTag>& InLayerPrecedenceHighestFirst,
    const TArray<FCk_Usf_OutlineDefinition>& InDefinitions,
    FCk_Usf_OutlineRuntimeConfig& OutConfig) -> bool
{
    auto Candidate = FCk_Usf_OutlineRuntimeConfig{};

    const auto HasLayers = NOT InLayerPrecedenceHighestFirst.IsEmpty();
    CK_ENSURE_IF_NOT(HasLayers, TEXT("Outline settings: layer precedence is empty")) {}
    if (NOT HasLayers) { return false; }

    const auto HasDefinitions = NOT InDefinitions.IsEmpty();
    CK_ENSURE_IF_NOT(HasDefinitions, TEXT("Outline settings: outline definitions are empty")) {}
    if (NOT HasDefinitions) { return false; }

    auto SeenLayers = TSet<FGameplayTag>{};
    for (const auto& Layer : InLayerPrecedenceHighestFirst)
    {
        const auto LayerIsValid = Layer.IsValid() && Layer.MatchesTag(TAG_Layer_Root) &&
                                  NOT Layer.MatchesTagExact(TAG_Layer_Root);
        CK_ENSURE_IF_NOT(LayerIsValid,
            TEXT("Outline settings: layer [{}] must be a child of Layer.*"), Layer) {}
        if (NOT LayerIsValid) { return false; }

        const auto LayerIsUnique = NOT SeenLayers.Contains(Layer);
        CK_ENSURE_IF_NOT(LayerIsUnique, TEXT("Outline settings: duplicate layer [{}]"), Layer) {}
        if (NOT LayerIsUnique) { return false; }
        SeenLayers.Add(Layer);
    }

    auto SeenOutlines = TSet<FGameplayTag>{};
    auto PresetByLayer = TMap<FGameplayTag, TObjectPtr<UCkUsf_OutlinePreset>>{};
    for (const auto& Definition : InDefinitions)
    {
        const auto& OutlineTag = Definition.Get_OutlineTag();
        const auto& LayerTag = Definition.Get_LayerTag();

        const auto OutlineTagIsValid = OutlineTag.IsValid() && OutlineTag.MatchesTag(TAG_Outline_Root) &&
                                       NOT OutlineTag.MatchesTagExact(TAG_Outline_Root);
        CK_ENSURE_IF_NOT(OutlineTagIsValid,
            TEXT("Outline settings: outline tag [{}] must be a child of Outline.*"), OutlineTag) {}
        if (NOT OutlineTagIsValid) { return false; }

        const auto OutlineTagIsUnique = NOT SeenOutlines.Contains(OutlineTag);
        CK_ENSURE_IF_NOT(OutlineTagIsUnique,
            TEXT("Outline settings: duplicate outline definition [{}]"), OutlineTag) {}
        if (NOT OutlineTagIsUnique) { return false; }

        const auto LayerIsConfigured = SeenLayers.Contains(LayerTag);
        CK_ENSURE_IF_NOT(LayerIsConfigured,
            TEXT("Outline settings: [{}] references missing layer [{}]"), OutlineTag, LayerTag) {}
        if (NOT LayerIsConfigured) { return false; }

        auto* Preset = Definition.Get_Preset().LoadSynchronous();
        const auto PresetIsValid = ck::IsValid(Preset);
        CK_ENSURE_IF_NOT(PresetIsValid,
            TEXT("Outline settings: [{}] has an invalid preset [{}]"),
            OutlineTag, Definition.Get_Preset().ToSoftObjectPath().ToString()) {}
        if (NOT PresetIsValid) { return false; }

        if (const auto* LayerPreset = PresetByLayer.Find(LayerTag))
        {
            const auto SharedLayerPresetMatches = *LayerPreset == Preset;
            CK_ENSURE_IF_NOT(SharedLayerPresetMatches,
                TEXT("Outline settings: layer [{}] maps to multiple presets"), LayerTag) {}
            if (NOT SharedLayerPresetMatches) { return false; }
        }
        else { PresetByLayer.Add(LayerTag, Preset); }

        const auto LayerIndex = InLayerPrecedenceHighestFirst.IndexOfByKey(LayerTag);
        Candidate.Definitions.Add(FCk_Usf_OutlineRuntimeDefinition{OutlineTag, LayerTag, Preset, LayerIndex});
        SeenOutlines.Add(OutlineTag);
    }

    OutConfig = MoveTemp(Candidate);
    return true;
}

auto UCk_Utils_Usf_Outline_Settings_UE::TryGet_RuntimeConfig(
    FCk_Usf_OutlineRuntimeConfig& OutConfig) -> bool
{
    auto* Settings = GetMutableDefault<UCk_Usf_Outline_ProjectSettings_UE>();
    const auto SettingsAreValid = ck::IsValid(Settings);
    CK_ENSURE_IF_NOT(SettingsAreValid, TEXT("Outline settings object is invalid")) {}
    if (NOT SettingsAreValid) { return false; }

    if (Settings->_RuntimeConfigIsCached)
    {
        OutConfig = Settings->_CachedRuntimeConfig;
        return true;
    }

    auto Candidate = FCk_Usf_OutlineRuntimeConfig{};
    if (NOT TryBuild_RuntimeConfig(
        Settings->Get_LayerPrecedenceHighestFirst(), Settings->Get_OutlineDefinitions(), Candidate))
    { return false; }

    auto LoadedPresets = TArray<TObjectPtr<UCkUsf_OutlinePreset>>{};
    for (const auto& Definition : Candidate.Definitions)
    { LoadedPresets.AddUnique(Definition.Preset); }

    Settings->_LoadedOutlinePresets = MoveTemp(LoadedPresets);
    Settings->_CachedRuntimeConfig = MoveTemp(Candidate);
    Settings->_RuntimeConfigIsCached = true;
    OutConfig = Settings->_CachedRuntimeConfig;
    return true;
}

auto UCk_Utils_Usf_Outline_Settings_UE::TryValidate_ThicknessSettings(
    const FCk_Usf_OutlineThicknessSettings& InSettings) -> bool
{
    const auto SpaceIsValid = InSettings.Get_Space() == ECk_Usf_OutlineThicknessSpace::WorldSpace ||
                              InSettings.Get_Space() == ECk_Usf_OutlineThicknessSpace::ScreenSpace;
    CK_ENSURE_IF_NOT(SpaceIsValid,
        TEXT("Outline thickness settings: space [{}] is invalid"), InSettings.Get_Space()) {}
    if (NOT SpaceIsValid) { return false; }

    const auto WorldSpaceThicknessIsValid = FMath::IsFinite(InSettings.Get_WorldSpaceThickness()) &&
                                          InSettings.Get_WorldSpaceThickness() > 0.0f;
    CK_ENSURE_IF_NOT(WorldSpaceThicknessIsValid,
        TEXT("Outline thickness settings: world-space thickness [{}] must be finite and positive"),
        InSettings.Get_WorldSpaceThickness()) {}
    if (NOT WorldSpaceThicknessIsValid) { return false; }

    const auto ScreenSpaceThicknessIsValid = FMath::IsFinite(InSettings.Get_ScreenSpaceThickness()) &&
                                           InSettings.Get_ScreenSpaceThickness() > 0.0f;
    CK_ENSURE_IF_NOT(ScreenSpaceThicknessIsValid,
        TEXT("Outline thickness settings: screen-space thickness [{}] must be finite and positive"),
        InSettings.Get_ScreenSpaceThickness()) {}
    if (NOT ScreenSpaceThicknessIsValid) { return false; }

    return true;
}

auto UCk_Utils_Usf_Outline_Settings_UE::Get_ThicknessSettings()
    -> FCk_Usf_OutlineThicknessSettings
{
    const auto* Settings = Get();
    if (ck::Is_NOT_Valid(Settings)) { return {}; }
    return Settings->Get_ThicknessSettings();
}

auto UCk_Utils_Usf_Outline_Settings_UE::Get_SelectionOutlineTag() -> FGameplayTag
{ return TAG_Outline_Debugger_Selection; }

auto UCk_Utils_Usf_Outline_Settings_UE::Get_GameplayInteractionOutlineTag() -> FGameplayTag
{ return TAG_Outline_Gameplay_Interaction; }

auto UCk_Utils_Usf_Outline_Settings_UE::Get_GameplayEmphasisOutlineTag() -> FGameplayTag
{ return TAG_Outline_Gameplay_Emphasis; }

auto UCk_Utils_Usf_Outline_Settings_UE::Get_GameplayGuidanceOutlineTag() -> FGameplayTag
{ return TAG_Outline_Gameplay_Guidance; }
