#include "CkCueBase_EntityScript.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkTimer/CkTimer_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Timer_CueLifetime, TEXT("Timer.Cue.Lifetime"));
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Cue, TEXT("Cue"));

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueBase_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    switch (_LifetimeBehavior)
    {
        case ECk_Cue_LifetimeBehavior::AfterOneFrame:
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            break;
        }
        case ECk_Cue_LifetimeBehavior::Persistent:
        {
            break;
        }
        case ECk_Cue_LifetimeBehavior::Timed:
        {
            if (_LifetimeBehavior == ECk_Cue_LifetimeBehavior::Timed)
            {
                const auto TimerParams = FCk_Fragment_Timer_ParamsData{_LifetimeDuration}
                    .Set_TimerName(TAG_Label_Timer_CueLifetime)
                    .Set_Behavior(ECk_Timer_Behavior::StopOnDone)
                    .Set_StartingState(ECk_Timer_State::Running);

                auto LifetimeTimer = UCk_Utils_Timer_UE::Add(_AssociatedEntity, TimerParams);

                // Bind to timer completion to self-destruct
                auto OnDoneDelegate = FCk_Delegate_Timer();
                OnDoneDelegate.BindDynamic(this, &UCk_CueBase_EntityScript::OnLifetimeExpired);
                UCk_Utils_Timer_UE::BindTo_OnDone(LifetimeTimer, ECk_Signal_BindingPolicy::IgnorePayloadInFlight, OnDoneDelegate);
            }

            break;
        }
    }

    return Super::Construct(InHandle, InSpawnParams);
}

auto
    UCk_CueBase_EntityScript::
    OnLifetimeExpired(
        FCk_Handle_Timer InTimer,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT)
    -> void
{
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_AssociatedEntity);
}

//============================================================================
// ASSET REGISTRY INTEGRATION FOR EFFICIENT CUE DISCOVERY
//============================================================================

#if WITH_EDITOR
auto
    UCk_CueBase_EntityScript::
    GetAssetRegistryTags(
        FAssetRegistryTagsContext Context) const
    -> void
{
    Super::GetAssetRegistryTags(Context);

    // Tag this asset as a cue for efficient discovery without loading
    Context.AddTag(FAssetRegistryTag("IsCueAsset", "true", FAssetRegistryTag::TT_Hidden));

    // Add the cue name as a searchable tag for even faster lookup
    if (ck::IsValid(_CueName))
    {
        Context.AddTag(FAssetRegistryTag("CueName", _CueName.ToString(), FAssetRegistryTag::TT_Hidden));
    }

    // Add cue type information for specialized subsystems
    Context.AddTag(FAssetRegistryTag("CueBaseClass", GetClass()->GetName(), FAssetRegistryTag::TT_Hidden));
}
#endif

// --------------------------------------------------------------------------------------------------------------------
