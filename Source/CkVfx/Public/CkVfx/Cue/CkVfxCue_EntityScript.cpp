#include "CkVfxCue_EntityScript.h"

#include "CkVfx/CkVfx_Log.h"
#include "CkVfxCue_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkTimer/CkTimer_Utils.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Timer_VfxCueStartDelay, TEXT("Timer.VfxCue.StartDelay"));

// --------------------------------------------------------------------------------------------------------------------

UCk_VfxCue_EntityScript::
    UCk_VfxCue_EntityScript(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    this->_LifetimeBehavior = ECk_Cue_LifetimeBehavior::Custom;
}

auto
    UCk_VfxCue_EntityScript::
    Get_CueName_Implementation() const
    -> FGameplayTag
{
    if (GetClass() == StaticClass())
    { return TAG_Cue_DoNotExecute; }

    return Super::Get_CueName_Implementation();
}

auto
    UCk_VfxCue_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto Ret = Super::Construct(InHandle, InSpawnParams);
    auto VfxCueHandle = UCk_Utils_VfxCue_UE::Add(_AssociatedEntity, *this);
    return Ret;
}

auto
    UCk_VfxCue_EntityScript::
    BeginPlay()
    -> void
{
    auto VfxCueHandle = UCk_Utils_VfxCue_UE::Cast(_AssociatedEntity);

    if (Get_LifetimeBehavior() == ECk_Cue_LifetimeBehavior::Custom)
    {
        DoBindToFinished(VfxCueHandle);
    }

    switch (_PlaybackBehavior)
    {
        case ECk_VfxCue_PlaybackBehavior::AutoPlay:
        {
            const auto PlayRequest = FCk_Request_VfxCue_Play{};
            UCk_Utils_VfxCue_UE::Request_Play(VfxCueHandle, PlayRequest);
            ck::vfx::Verbose(TEXT("VfxCue EntityScript [{}] auto-started playback"), Get_CueName());
            break;
        }
        case ECk_VfxCue_PlaybackBehavior::Manual:
        {
            ck::vfx::Verbose(TEXT("VfxCue EntityScript [{}] waiting for manual play request"), Get_CueName());
            break;
        }
        case ECk_VfxCue_PlaybackBehavior::DelayedPlay:
        {
            const auto TimerParams = FCk_Fragment_Timer_ParamsData{_DelayTime}
                .Set_TimerName(TAG_Label_Timer_VfxCueStartDelay)
                .Set_CountDirection(ECk_Timer_CountDirection::CountUp)
                .Set_Behavior(ECk_Timer_Behavior::PauseOnDone)
                .Set_StartingState(ECk_Timer_State::Running);

            auto DelayTimer = UCk_Utils_Timer_UE::Add(_AssociatedEntity, TimerParams);

            auto Delegate = FCk_Delegate_Timer{};
            Delegate.BindDynamic(this, &ThisType::OnDelayTimerComplete);
            UCk_Utils_Timer_UE::BindTo_OnDone(DelayTimer, ECk_Signal_BindingPolicy::FireIfPayloadInFlight, Delegate);

            ck::vfx::Verbose(TEXT("VfxCue EntityScript [{}] will start playback after [{}] seconds"),
                Get_CueName(), _DelayTime.Get_Seconds());
            break;
        }
        default:
        {
            CK_INVALID_ENUM(_PlaybackBehavior);
            break;
        }
    }

    Super::BeginPlay();
}

auto
    UCk_VfxCue_EntityScript::
    OnDelayTimerComplete(
        FCk_Handle_Timer InHandle,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT)
    -> void
{
    auto VfxCueHandle = UCk_Utils_VfxCue_UE::Cast(_AssociatedEntity);

    CK_ENSURE_IF_NOT(ck::IsValid(VfxCueHandle), TEXT("VfxCue handle invalid"))
    { return; }

    const auto PlayRequest = FCk_Request_VfxCue_Play{};
    UCk_Utils_VfxCue_UE::Request_Play(VfxCueHandle, PlayRequest);
    ck::vfx::Verbose(TEXT("VfxCue EntityScript [{}] delayed playback started"), Get_CueName());
}

auto
    UCk_VfxCue_EntityScript::
    Get_IsConfigurationValid() const
    -> bool
{
    return ck::IsValid(_Effect);
}

auto
    UCk_VfxCue_EntityScript::
    DoBindToFinished(
        FCk_Handle_VfxCue InVfxCueHandle)
    -> void
{
    auto Delegate = FCk_Delegate_VfxCue_OnFinished{};
    Delegate.BindDynamic(this, &UCk_VfxCue_EntityScript::OnEffectFinished);

    UCk_Utils_VfxCue_UE::BindTo_OnFinished(InVfxCueHandle,
        ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing,
        Delegate);

    ck::vfx::Verbose(TEXT("VfxCue EntityScript [{}] bound to OnFinished for custom lifetime management"), Get_CueName());
}

auto
    UCk_VfxCue_EntityScript::
    OnEffectFinished(
        FCk_Handle_VfxCue InVfxCue)
    -> void
{
    ck::vfx::Verbose(TEXT("VfxCue EntityScript [{}] received OnFinished"), Get_CueName());

    switch (_PoolingBehavior)
    {
        case ECk_VfxCue_PoolingBehavior::AutoDestroy:
        {
            ck::vfx::Verbose(TEXT("VfxCue EntityScript [{}] auto-destroying entity"), Get_CueName());
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_AssociatedEntity);
            break;
        }
        case ECk_VfxCue_PoolingBehavior::AutoPool:
        {
            ck::vfx::Warning(TEXT("VfxCue EntityScript [{}] pooling not yet implemented - destroying"), Get_CueName());
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_AssociatedEntity);
            break;
        }
        case ECk_VfxCue_PoolingBehavior::Manual:
        {
            ck::vfx::Verbose(TEXT("VfxCue EntityScript [{}] manual lifetime - no auto cleanup"), Get_CueName());
            break;
        }
        default:
        {
            CK_INVALID_ENUM(_PoolingBehavior);
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
