#include "CkIskmRenderer_Settings.h"

#include "HAL/IConsoleManager.h"
#include "Math/UnrealMathUtility.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_iskm_renderer_settings
{
    constexpr auto EditorPreviewAnimationModeCVar = TEXT("ck.Iskm.EditorPreviewAnimationMode");
    constexpr auto EditorPreviewAnimationFrequencyCVar = TEXT("ck.Iskm.EditorPreviewAnimationFrequency");

    static int32 GEditorPreviewAnimationMode = static_cast<int32>(ECk_Iskm_EditorPreviewAnimationMode::SelectedOnly);
    static int32 GEditorPreviewAnimationFrequency = 30;

    static auto
        WriteModeToSettings(IConsoleVariable* InCVar)
        -> void
    {
        if (InCVar == nullptr)
        { return; }

        const auto ClampedValue = FMath::Clamp(InCVar->GetInt(),
            static_cast<int32>(ECk_Iskm_EditorPreviewAnimationMode::Disabled),
            static_cast<int32>(ECk_Iskm_EditorPreviewAnimationMode::All));
        GEditorPreviewAnimationMode = ClampedValue;

        auto* Settings = GetMutableDefault<UCk_IskmRenderer_UserSettings_UE>();
        if (NOT IsValid(Settings))
        { return; }

        const auto NewMode = static_cast<ECk_Iskm_EditorPreviewAnimationMode>(ClampedValue);
        if (Settings->Get_EditorPreviewAnimationMode() == NewMode)
        { return; }

        Settings->Set_EditorPreviewAnimationMode(NewMode);
        Settings->SaveConfig();
    }

    static auto
        WriteFrequencyToSettings(IConsoleVariable* InCVar)
        -> void
    {
        if (InCVar == nullptr)
        { return; }

        const auto ClampedValue = FMath::Clamp(InCVar->GetInt(), 5, 60);
        GEditorPreviewAnimationFrequency = ClampedValue;

        auto* Settings = GetMutableDefault<UCk_IskmRenderer_UserSettings_UE>();
        if (NOT IsValid(Settings) || Settings->Get_EditorPreviewAnimationFrequency() == ClampedValue)
        { return; }

        Settings->Set_EditorPreviewAnimationFrequency(ClampedValue);
        Settings->SaveConfig();
    }

    static FAutoConsoleVariableRef CVarEditorPreviewAnimationMode{
        EditorPreviewAnimationModeCVar,
        GEditorPreviewAnimationMode,
        TEXT("Controls continuous batched ISKM animation outside PIE.\n")
        TEXT("  0 = Disabled\n")
        TEXT("  1 = Selected actor previews only (default)\n")
        TEXT("  2 = All editor preview crowds"),
        FConsoleVariableDelegate::CreateStatic(&WriteModeToSettings),
        ECVF_Default};

    static FAutoConsoleVariableRef CVarEditorPreviewAnimationFrequency{
        EditorPreviewAnimationFrequencyCVar,
        GEditorPreviewAnimationFrequency,
        TEXT("Maximum batched ISKM non-PIE preview update frequency in Hz (5..60, default 30)."),
        FConsoleVariableDelegate::CreateStatic(&WriteFrequencyToSettings),
        ECVF_Default};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IskmRenderer_UserSettings_UE::
    PostInitProperties()
    -> void
{
    Super::PostInitProperties();

    if (NOT IsTemplate())
    { return; }

    const auto Mode = FMath::Clamp(static_cast<int32>(_EditorPreviewAnimationMode),
        static_cast<int32>(ECk_Iskm_EditorPreviewAnimationMode::Disabled),
        static_cast<int32>(ECk_Iskm_EditorPreviewAnimationMode::All));
    const auto Frequency = FMath::Clamp(_EditorPreviewAnimationFrequency, 5, 60);

    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(
            ck_iskm_renderer_settings::EditorPreviewAnimationModeCVar))
    { CVar->Set(Mode, ECVF_SetByGameSetting); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(
            ck_iskm_renderer_settings::EditorPreviewAnimationFrequencyCVar))
    { CVar->Set(Frequency, ECVF_SetByGameSetting); }
}

#if WITH_EDITOR
auto
    UCk_IskmRenderer_UserSettings_UE::
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);

    const auto PropertyName = InPropertyChangedEvent.GetPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_IskmRenderer_UserSettings_UE, _EditorPreviewAnimationMode))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(
                ck_iskm_renderer_settings::EditorPreviewAnimationModeCVar))
        { CVar->Set(static_cast<int32>(_EditorPreviewAnimationMode), ECVF_SetByGameSetting); }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(
        UCk_IskmRenderer_UserSettings_UE, _EditorPreviewAnimationFrequency))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(
                ck_iskm_renderer_settings::EditorPreviewAnimationFrequencyCVar))
        {
            CVar->Set(FMath::Clamp(_EditorPreviewAnimationFrequency, 5, 60), ECVF_SetByGameSetting);
        }
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------
