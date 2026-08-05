#include "CkEntityVisualizer_Settings.h"

#include "HAL/IConsoleManager.h"
#include "Math/UnrealMathUtility.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_entity_visualizer_settings
{
    constexpr auto VisibilityModeCVar = TEXT("ck.EntityVisualizer.VisibilityMode");

    static int32 GVisibilityMode =
        static_cast<int32>(ECk_EntityVisualizer_VisibilityMode::SelectedOnly);

    static auto
        WriteVisibilityModeToSettings(IConsoleVariable* InCVar)
        -> void
    {
        if (InCVar == nullptr)
        { return; }

        const auto ClampedValue = FMath::Clamp(
            InCVar->GetInt(),
            static_cast<int32>(ECk_EntityVisualizer_VisibilityMode::Disabled),
            static_cast<int32>(ECk_EntityVisualizer_VisibilityMode::All));
        GVisibilityMode = ClampedValue;

        auto* Settings = GetMutableDefault<UCk_EntityVisualizer_UserSettings_UE>();
        if (NOT IsValid(Settings))
        { return; }

        const auto NewMode = static_cast<ECk_EntityVisualizer_VisibilityMode>(ClampedValue);
        if (Settings->Get_VisibilityMode() == NewMode)
        { return; }

        Settings->Set_VisibilityMode(NewMode);
        Settings->SaveConfig();
    }

    static FAutoConsoleVariableRef CVarVisibilityMode{
        VisibilityModeCVar,
        GVisibilityMode,
        TEXT("Controls retained probe previews and entity transform gizmos outside PIE.\n")
        TEXT("  0 = Disabled\n")
        TEXT("  1 = Selected actor entities only (default)\n")
        TEXT("  2 = All entities, including ownerless entities"),
        FConsoleVariableDelegate::CreateStatic(&WriteVisibilityModeToSettings),
        ECVF_Default};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::entity_visualizer::
    GetVisibilityMode()
    -> ECk_EntityVisualizer_VisibilityMode
{
    const auto ClampedValue = FMath::Clamp(
        ck_entity_visualizer_settings::GVisibilityMode,
        static_cast<int32>(ECk_EntityVisualizer_VisibilityMode::Disabled),
        static_cast<int32>(ECk_EntityVisualizer_VisibilityMode::All));
    return static_cast<ECk_EntityVisualizer_VisibilityMode>(ClampedValue);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityVisualizer_UserSettings_UE::
    PostInitProperties()
    -> void
{
    Super::PostInitProperties();

    if (NOT IsTemplate())
    { return; }

    const auto Mode = FMath::Clamp(
        static_cast<int32>(_VisibilityMode),
        static_cast<int32>(ECk_EntityVisualizer_VisibilityMode::Disabled),
        static_cast<int32>(ECk_EntityVisualizer_VisibilityMode::All));
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(
            ck_entity_visualizer_settings::VisibilityModeCVar))
    { CVar->Set(Mode, ECVF_SetByGameSetting); }
}

#if WITH_EDITOR
auto
    UCk_EntityVisualizer_UserSettings_UE::
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);

    if (InPropertyChangedEvent.GetPropertyName() !=
        GET_MEMBER_NAME_CHECKED(UCk_EntityVisualizer_UserSettings_UE, _VisibilityMode))
    { return; }

    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(
            ck_entity_visualizer_settings::VisibilityModeCVar))
    { CVar->Set(static_cast<int32>(_VisibilityMode), ECVF_SetByGameSetting); }
}
#endif

// --------------------------------------------------------------------------------------------------------------------
