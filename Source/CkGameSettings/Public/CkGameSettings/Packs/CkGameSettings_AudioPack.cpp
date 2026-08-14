#include "CkGameSettings_AudioPack.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkGameSettings/CkGameSettings_Log.h"
#include "CkGameSettings/Subsystem/CkGameSettings_Subsystem.h"

#include <Engine/GameInstance.h>
#include <Kismet/GameplayStatics.h>
#include <Sound/SoundClass.h>
#include <Sound/SoundMix.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_game_settings_audio_pack
{
    constexpr auto VolumePercentScale = 100.0f;
    constexpr auto VolumePercentStep = 0.01f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettings_AudioCategoryHandler_UE::
    Initialize_Handler(
        const FCk_GameSettings_AudioCategory& InCategory,
        const TSoftObjectPtr<USoundMix>& InAudioMix)
    -> void
{
    _Category = InCategory;
    _AudioMix = InAudioMix;
}

auto
    UCk_GameSettings_AudioCategoryHandler_UE::
    Shutdown_Handler()
    -> void
{
    if (_WorldInitHandle.IsValid())
    {
        FWorldDelegates::OnPostWorldInitialization.Remove(_WorldInitHandle);
        _WorldInitHandle.Reset();
    }
}

auto
    UCk_GameSettings_AudioCategoryHandler_UE::
    OnVolumeApplied(
        float InNewValue)
    -> void
{
    auto* World = GetWorld();

    if (ck::Is_NOT_Valid(World))
    {
        _PendingVolume = InNewValue;

        if (NOT _WorldInitHandle.IsValid())
        { _WorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisType::DoHandleWorldInitialized); }

        return;
    }

    DoApplyVolume(InNewValue, World);
}

auto
    UCk_GameSettings_AudioCategoryHandler_UE::
    DoApplyVolume(
        float InVolume,
        UWorld* InWorld)
    -> void
{
    auto* AudioMix = _AudioMix.LoadSynchronous();
    auto* SoundClass = _Category.Get_SoundClass().LoadSynchronous();

    const auto AssetsAreValid = ck::IsValid(AudioMix) && ck::IsValid(SoundClass);
    CK_ENSURE_IF_NOT(AssetsAreValid, TEXT("Audio pack category [{}] cannot apply its volume — SoundMix [{}] resolved [{}], SoundClass [{}] resolved [{}]"),
        _Category.Get_SettingKey(),
        _AudioMix.ToString(), ck::IsValid(AudioMix),
        _Category.Get_SoundClass().ToString(), ck::IsValid(SoundClass))
    {}
    if (NOT AssetsAreValid)
    { return; }

    constexpr auto Pitch = 1.0f;
    constexpr auto FadeInTime = 0.5f;
    constexpr auto ApplyToChildren = true;
    UGameplayStatics::SetSoundMixClassOverride(InWorld, AudioMix, SoundClass, InVolume, Pitch, FadeInTime, ApplyToChildren);

    if (NOT _MixPushed)
    {
        UGameplayStatics::PushSoundMixModifier(InWorld, AudioMix);
        _MixPushed = true;
    }
}

auto
    UCk_GameSettings_AudioCategoryHandler_UE::
    DoHandleWorldInitialized(
        UWorld* InWorld,
        const UWorld::InitializationValues InValues)
    -> void
{
    const auto* OwningSubsystem = Cast<UCk_GameSettings_Subsystem_UE>(GetOuter());

    if (ck::Is_NOT_Valid(InWorld) ||
        ck::Is_NOT_Valid(OwningSubsystem) ||
        InWorld->GetGameInstance() != OwningSubsystem->GetGameInstance())
    { return; }

    FWorldDelegates::OnPostWorldInitialization.Remove(_WorldInitHandle);
    _WorldInitHandle.Reset();

    if (NOT _PendingVolume.IsSet())
    { return; }

    DoApplyVolume(_PendingVolume.GetValue(), InWorld);
    _PendingVolume.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::game_settings
{
    auto
        RegisterAudioPack(
            UCk_GameSettings_Subsystem_UE& InSubsystem,
            const TSoftObjectPtr<USoundMix>& InAudioMix,
            const TArray<FCk_GameSettings_AudioCategory>& InCategories,
            TArray<TObjectPtr<UObject>>& InOutOwnedObjects)
        -> int32
    {
        using namespace ck_game_settings_audio_pack;

        auto RegisteredCount = int32{0};

        for (const auto& Category : InCategories)
        {
            const auto SettingKey = Category.Get_SettingKey();

            const auto CategoryIsUsable = NOT SettingKey.IsNone() && NOT Category.Get_SoundClass().IsNull();
            CK_ENSURE_IF_NOT(CategoryIsUsable, TEXT("Audio pack category with key [{}] has no key or no SoundClass, skipped"), SettingKey)
            {}
            if (NOT CategoryIsUsable)
            { continue; }

            if (InSubsystem.Get_IsSettingRegistered(SettingKey))
            { continue; }

            auto Definition = FCk_GameSettings_SettingDefinition{SettingKey, ECk_GameSettings_ValueType::Float, LexToString(Category.Get_DefaultVolume())};
            Definition.Set_ApplyBindingType(ECk_GameSettings_ApplyBindingType::Handler);
            Definition.Set_MinValue(TEXT("0"));
            Definition.Set_MaxValue(TEXT("1"));

            // A volume is a 0..1 multiplier everywhere except in front of a player, who reads percent.
            Definition.Set_OptionalDisplayScale(VolumePercentScale);
            Definition.Set_OptionalStepSize(VolumePercentStep);

            if (NOT Category.Get_DisplayName().IsEmpty())
            { Definition.Set_DisplayName(Category.Get_DisplayName()); }

            auto CategoryTags = FGameplayTagContainer{};
            if (Category.Get_CategoryTag().IsValid())
            { CategoryTags.AddTag(Category.Get_CategoryTag()); }
            Definition.Set_CategoryTags(CategoryTags);

            if (NOT InSubsystem.Request_RegisterSetting(Definition))
            { continue; }

            auto* Handler = NewObject<UCk_GameSettings_AudioCategoryHandler_UE>(&InSubsystem);
            Handler->Initialize_Handler(Category, InAudioMix);
            InOutOwnedObjects.Add(Handler);

            auto HandlerDelegate = FCk_Delegate_GameSettings_ApplyHandler_Float{};
            HandlerDelegate.BindDynamic(Handler, &UCk_GameSettings_AudioCategoryHandler_UE::OnVolumeApplied);
            InSubsystem.Request_RegisterApplyHandler_Float(SettingKey, HandlerDelegate);

            ++RegisteredCount;
        }

        return RegisteredCount;
    }
}

// --------------------------------------------------------------------------------------------------------------------
