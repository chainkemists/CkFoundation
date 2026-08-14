#include "CkGameSettings_VideoPack.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkGameSettings/CkGameSettings_Log.h"
#include "CkGameSettings/Settings/CkGameSettings_Settings.h"
#include "CkGameSettings/Subsystem/CkGameSettings_Subsystem.h"

#include <Engine/Engine.h>
#include <GameFramework/GameUserSettings.h>
#include <Misc/App.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_WindowMode()
    -> int32
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return 0; }

    return static_cast<int32>(Settings->GetFullscreenMode());
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_WindowMode(
        int32 InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetFullscreenMode(ck::game_settings::Get_WindowModeFromInt(InNewValue));
    DoApplyResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_Resolution()
    -> FString
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return ck::game_settings::Format_Resolution(Settings->GetScreenResolution());
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_Resolution(
        const FString& InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    auto NewResolution = FIntPoint{};
    const auto ResolutionParses = ck::game_settings::TryParse_Resolution(InNewValue, NewResolution);
    CK_ENSURE_IF_NOT(ResolutionParses, TEXT("Video pack resolution value [{}] is not of the form WIDTHxHEIGHT, not applied"), InNewValue)
    {}
    if (NOT ResolutionParses)
    { return; }

    Settings->SetScreenResolution(NewResolution);
    DoApplyResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_VSync()
    -> bool
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return false; }

    return Settings->IsVSyncEnabled();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_VSync(
        bool InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetVSyncEnabled(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_FpsCap()
    -> float
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return 0.0f; }

    return Settings->GetFrameRateLimit();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_FpsCap(
        float InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetFrameRateLimit(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_QualityPreset()
    -> int32
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return -1; }

    return Settings->GetOverallScalabilityLevel();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_QualityPreset(
        int32 InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    if (InNewValue < 0)
    { return; }

    Settings->SetOverallScalabilityLevel(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_ViewDistanceQuality()
    -> int32
{
    auto* Settings = DoGet_GameUserSettings();
    return ck::IsValid(Settings) ? Settings->GetViewDistanceQuality() : 0;
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_ViewDistanceQuality(
        int32 InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetViewDistanceQuality(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_AntiAliasingQuality()
    -> int32
{
    auto* Settings = DoGet_GameUserSettings();
    return ck::IsValid(Settings) ? Settings->GetAntiAliasingQuality() : 0;
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_AntiAliasingQuality(
        int32 InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetAntiAliasingQuality(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_ShadowQuality()
    -> int32
{
    auto* Settings = DoGet_GameUserSettings();
    return ck::IsValid(Settings) ? Settings->GetShadowQuality() : 0;
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_ShadowQuality(
        int32 InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetShadowQuality(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_PostProcessQuality()
    -> int32
{
    auto* Settings = DoGet_GameUserSettings();
    return ck::IsValid(Settings) ? Settings->GetPostProcessingQuality() : 0;
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_PostProcessQuality(
        int32 InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetPostProcessingQuality(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_TextureQuality()
    -> int32
{
    auto* Settings = DoGet_GameUserSettings();
    return ck::IsValid(Settings) ? Settings->GetTextureQuality() : 0;
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_TextureQuality(
        int32 InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetTextureQuality(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_EffectsQuality()
    -> int32
{
    auto* Settings = DoGet_GameUserSettings();
    return ck::IsValid(Settings) ? Settings->GetVisualEffectQuality() : 0;
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_EffectsQuality(
        int32 InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetVisualEffectQuality(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Get_FoliageQuality()
    -> int32
{
    auto* Settings = DoGet_GameUserSettings();
    return ck::IsValid(Settings) ? Settings->GetFoliageQuality() : 0;
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    Set_FoliageQuality(
        int32 InNewValue)
    -> void
{
    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->SetFoliageQuality(InNewValue);
    DoApplyNonResolution();
    Settings->SaveSettings();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    DoApplyNonResolution()
    -> void
{
    if (ck::game_settings::Get_IsHeadlessPresentation())
    {
        ck::game_settings::Display(TEXT("Video pack: ApplySettings suppressed (headless presentation)"));
        return;
    }

    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    constexpr auto CheckForCommandLineOverrides = false;
    Settings->ApplySettings(CheckForCommandLineOverrides);
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    DoApplyResolution()
    -> void
{
    if (ck::game_settings::Get_IsHeadlessPresentation())
    {
        ck::game_settings::Display(TEXT("Video pack: ApplyResolutionSettings suppressed (headless presentation)"));
        return;
    }

    auto* Settings = DoGet_GameUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    constexpr auto CheckForCommandLineOverrides = false;
    Settings->ApplyResolutionSettings(CheckForCommandLineOverrides);
}

auto
    UCk_GameSettings_VideoPackHandlers_UE::
    DoGet_GameUserSettings()
    -> UGameUserSettings*
{
    const auto EngineIsValid = ck::IsValid(GEngine);
    CK_ENSURE_IF_NOT(EngineIsValid, TEXT("No GEngine, cannot resolve GameUserSettings for the Video pack"))
    {}
    if (NOT EngineIsValid)
    { return nullptr; }

    auto* Settings = GEngine->GetGameUserSettings();
    const auto SettingsAreValid = ck::IsValid(Settings);
    CK_ENSURE_IF_NOT(SettingsAreValid, TEXT("GEngine->GetGameUserSettings() returned null, Video pack cannot operate"))
    {}
    if (NOT SettingsAreValid)
    { return nullptr; }

    return Settings;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::game_settings
{
    namespace ck_game_settings_video_pack
    {
        auto ParseDimension(const FString& InPart, int32& OutValue) -> bool
        {
            const auto Trimmed = InPart.TrimStartAndEnd();

            if (Trimmed.IsEmpty())
            { return false; }

            for (const auto& Char : Trimmed)
            {
                if (NOT FChar::IsDigit(Char))
                { return false; }
            }

            OutValue = FCString::Atoi(*Trimmed);
            return OutValue > 0;
        }
    }

    auto
        Get_VideoSettingKeys()
        -> const TArray<FName>&
    {
        static const auto Keys = TArray<FName>
        {
            Key_Video_WindowMode,
            Key_Video_Resolution,
            Key_Video_VSync,
            Key_Video_FpsCap,
            Key_Video_QualityPreset,
            Key_Video_Sg_ViewDistance,
            Key_Video_Sg_AntiAliasing,
            Key_Video_Sg_Shadow,
            Key_Video_Sg_PostProcess,
            Key_Video_Sg_Texture,
            Key_Video_Sg_Effects,
            Key_Video_Sg_Foliage,
        };
        return Keys;
    }

    auto
        TryParse_Resolution(
            const FString& InResolution,
            FIntPoint& OutResolution)
        -> bool
    {
        auto WidthPart = FString{};
        auto HeightPart = FString{};

        if (NOT InResolution.Split(TEXT("x"), &WidthPart, &HeightPart, ESearchCase::IgnoreCase))
        { return false; }

        auto Width = int32{};
        auto Height = int32{};

        if (NOT ck_game_settings_video_pack::ParseDimension(WidthPart, Width) ||
            NOT ck_game_settings_video_pack::ParseDimension(HeightPart, Height))
        { return false; }

        OutResolution = FIntPoint{Width, Height};
        return true;
    }

    auto
        Format_Resolution(
            const FIntPoint& InResolution)
        -> FString
    {
        return ck::Format_UE(TEXT("{}x{}"), InResolution.X, InResolution.Y);
    }

    auto
        Get_WindowModeFromInt(
            int32 InWindowMode)
        -> EWindowMode::Type
    {
        return EWindowMode::ConvertIntToWindowMode(InWindowMode);
    }

    auto
        Get_IsHeadlessPresentation()
        -> bool
    {
        return IsRunningCommandlet() || NOT FApp::CanEverRender();
    }

    auto
        RegisterVideoPack(
            UCk_GameSettings_Subsystem_UE& InSubsystem,
            TArray<TObjectPtr<UObject>>& InOutOwnedObjects)
        -> int32
    {
        auto* Handlers = NewObject<UCk_GameSettings_VideoPackHandlers_UE>(&InSubsystem);
        InOutOwnedObjects.Add(Handlers);

        auto RegisteredCount = int32{0};

        const auto MakeExternalDefinition = [](FName InKey, ECk_GameSettings_ValueType InValueType, const FString& InDefaultValue)
        {
            auto Definition = FCk_GameSettings_SettingDefinition{InKey, InValueType, InDefaultValue};
            Definition.Set_PersistencePolicy(ECk_GameSettings_PersistencePolicy::External);
            Definition.Set_ApplyBindingType(ECk_GameSettings_ApplyBindingType::Handler);

            if (const auto& CategoryTag = UCk_Utils_GameSettings_Settings_UE::Get_VideoPackCategoryTag();
                CategoryTag.IsValid())
            { Definition.Set_CategoryTags(FGameplayTagContainer{CategoryTag}); }

            return Definition;
        };

        const auto RegisterInt32 = [&](FName InKey, const FString& InDefault, const FString& InMin, const FString& InMax,
            FName InGetterName, FName InSetterName)
        {
            if (InSubsystem.Get_IsSettingRegistered(InKey))
            { return; }

            auto Definition = MakeExternalDefinition(InKey, ECk_GameSettings_ValueType::Int32, InDefault);
            Definition.Set_MinValue(InMin);
            Definition.Set_MaxValue(InMax);

            if (NOT InSubsystem.Request_RegisterSetting(Definition))
            { return; }

            auto Getter = FCk_Delegate_GameSettings_ExternalGetter_Int32{};
            Getter.BindUFunction(Handlers, InGetterName);
            auto Setter = FCk_Delegate_GameSettings_ApplyHandler_Int32{};
            Setter.BindUFunction(Handlers, InSetterName);
            InSubsystem.Request_RegisterExternalAccessors_Int32(InKey, Getter, Setter);
            ++RegisteredCount;
        };

        if (NOT InSubsystem.Get_IsSettingRegistered(Key_Video_Resolution))
        {
            if (InSubsystem.Request_RegisterSetting(MakeExternalDefinition(Key_Video_Resolution, ECk_GameSettings_ValueType::String, TEXT("1920x1080"))))
            {
                auto Getter = FCk_Delegate_GameSettings_ExternalGetter_String{};
                Getter.BindDynamic(Handlers, &UCk_GameSettings_VideoPackHandlers_UE::Get_Resolution);
                auto Setter = FCk_Delegate_GameSettings_ApplyHandler_String{};
                Setter.BindDynamic(Handlers, &UCk_GameSettings_VideoPackHandlers_UE::Set_Resolution);
                InSubsystem.Request_RegisterExternalAccessors_String(Key_Video_Resolution, Getter, Setter);
                ++RegisteredCount;
            }
        }

        if (NOT InSubsystem.Get_IsSettingRegistered(Key_Video_VSync))
        {
            if (InSubsystem.Request_RegisterSetting(MakeExternalDefinition(Key_Video_VSync, ECk_GameSettings_ValueType::Bool, TEXT("false"))))
            {
                auto Getter = FCk_Delegate_GameSettings_ExternalGetter_Bool{};
                Getter.BindDynamic(Handlers, &UCk_GameSettings_VideoPackHandlers_UE::Get_VSync);
                auto Setter = FCk_Delegate_GameSettings_ApplyHandler_Bool{};
                Setter.BindDynamic(Handlers, &UCk_GameSettings_VideoPackHandlers_UE::Set_VSync);
                InSubsystem.Request_RegisterExternalAccessors_Bool(Key_Video_VSync, Getter, Setter);
                ++RegisteredCount;
            }
        }

        if (NOT InSubsystem.Get_IsSettingRegistered(Key_Video_FpsCap))
        {
            auto Definition = MakeExternalDefinition(Key_Video_FpsCap, ECk_GameSettings_ValueType::Float, TEXT("0"));
            Definition.Set_MinValue(TEXT("0"));

            if (InSubsystem.Request_RegisterSetting(Definition))
            {
                auto Getter = FCk_Delegate_GameSettings_ExternalGetter_Float{};
                Getter.BindDynamic(Handlers, &UCk_GameSettings_VideoPackHandlers_UE::Get_FpsCap);
                auto Setter = FCk_Delegate_GameSettings_ApplyHandler_Float{};
                Setter.BindDynamic(Handlers, &UCk_GameSettings_VideoPackHandlers_UE::Set_FpsCap);
                InSubsystem.Request_RegisterExternalAccessors_Float(Key_Video_FpsCap, Getter, Setter);
                ++RegisteredCount;
            }
        }

        RegisterInt32(Key_Video_WindowMode, TEXT("0"), TEXT("0"), TEXT("2"),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Get_WindowMode),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Set_WindowMode));
        RegisterInt32(Key_Video_QualityPreset, TEXT("-1"), TEXT("-1"), TEXT("4"),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Get_QualityPreset),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Set_QualityPreset));
        RegisterInt32(Key_Video_Sg_ViewDistance, TEXT("3"), TEXT("0"), TEXT("4"),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Get_ViewDistanceQuality),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Set_ViewDistanceQuality));
        RegisterInt32(Key_Video_Sg_AntiAliasing, TEXT("3"), TEXT("0"), TEXT("4"),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Get_AntiAliasingQuality),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Set_AntiAliasingQuality));
        RegisterInt32(Key_Video_Sg_Shadow, TEXT("3"), TEXT("0"), TEXT("4"),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Get_ShadowQuality),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Set_ShadowQuality));
        RegisterInt32(Key_Video_Sg_PostProcess, TEXT("3"), TEXT("0"), TEXT("4"),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Get_PostProcessQuality),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Set_PostProcessQuality));
        RegisterInt32(Key_Video_Sg_Texture, TEXT("3"), TEXT("0"), TEXT("4"),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Get_TextureQuality),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Set_TextureQuality));
        RegisterInt32(Key_Video_Sg_Effects, TEXT("3"), TEXT("0"), TEXT("4"),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Get_EffectsQuality),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Set_EffectsQuality));
        RegisterInt32(Key_Video_Sg_Foliage, TEXT("3"), TEXT("0"), TEXT("4"),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Get_FoliageQuality),
            GET_FUNCTION_NAME_CHECKED(UCk_GameSettings_VideoPackHandlers_UE, Set_FoliageQuality));

        return RegisteredCount;
    }
}

// --------------------------------------------------------------------------------------------------------------------
