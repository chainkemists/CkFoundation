#include "CkCue_EntityScript.h"

#include "CkCue/CkCue_Fragment.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkTimer/CkTimer_Utils.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Timer_CueLifetime, TEXT("Timer.Cue.Lifetime"));
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Cue, TEXT("Cue"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Cue_DoNotExecute, TEXT("Cue.DoNotExecute"));

// --------------------------------------------------------------------------------------------------------------------

UCk_CueBase_EntityScript::
    UCk_CueBase_EntityScript(
        const FObjectInitializer& InInitializer)
    : Super(InInitializer)
{
    _Replication = ECk_Replication::DoesNotReplicate;
}

auto
    UCk_CueBase_EntityScript::
    Get_CueName_Implementation() const
    -> FGameplayTag
{
    auto ClassName = GetClass()->GetName();

    if (ClassName.EndsWith(TEXT("_C")))
    { ClassName = ClassName.LeftChop(2); }

    ClassName = ClassName.Replace(TEXT("_"), TEXT("."));

    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(*ClassName,
        ck::Format_UE(TEXT("Auto-generated cue tag for {}"), GetClass()));
}

auto
    UCk_CueBase_EntityScript::
    Restart()
    -> void
{
    switch (_LifetimeBehavior)
    {
        case ECk_Cue_LifetimeBehavior::AfterOneFrame:
        case ECk_Cue_LifetimeBehavior::Persistent:
        {
            break;
        }
        case ECk_Cue_LifetimeBehavior::Timed:
        {
            auto LifetimeTimer = UCk_Utils_Timer_UE::TryGet_Timer(_AssociatedEntity, TAG_Label_Timer_CueLifetime);
            UCk_Utils_Timer_UE::Request_Reset(LifetimeTimer);
            UCk_Utils_Timer_UE::Request_Resume(LifetimeTimer);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(_LifetimeBehavior);
            break;
        }
    }

    DoRestart(_AssociatedEntity);
}

auto
    UCk_CueBase_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    // TODO: This should be done through the Utils_Cue::Add(...) function
    UCk_Utils_GameplayLabel_UE::Add(InHandle, Get_CueName());

    auto CueOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
    ck::ActiveCues_Utils::AddIfMissing(CueOwner);
    ck::ActiveCues_Utils::Request_Connect(CueOwner, InHandle);

    return Super::Construct(InHandle, InSpawnParams);;
}

auto
    UCk_CueBase_EntityScript::
    BeginPlay()
    -> void
{
    switch (_LifetimeBehavior)
    {
        case ECk_Cue_LifetimeBehavior::AfterOneFrame:
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_AssociatedEntity);
            break;
        }
        case ECk_Cue_LifetimeBehavior::Custom:
        case ECk_Cue_LifetimeBehavior::Persistent:
        {
            break;
        }
        case ECk_Cue_LifetimeBehavior::Timed:
        {
            const auto TimerParams = FCk_Fragment_Timer_ParamsData{_LifetimeDuration}
                .Set_TimerName(TAG_Label_Timer_CueLifetime)
                .Set_Behavior(ECk_Timer_Behavior::StopOnDone)
                .Set_StartingState(ECk_Timer_State::Running);

            auto LifetimeTimer = UCk_Utils_Timer_UE::Add(_AssociatedEntity, TimerParams);

            // Bind to timer completion to self-destruct
            auto OnDoneDelegate = FCk_Delegate_Timer();
            OnDoneDelegate.BindDynamic(this, &UCk_CueBase_EntityScript::OnLifetimeExpired);
            UCk_Utils_Timer_UE::BindTo_OnDone(LifetimeTimer, OnDoneDelegate);

            break;
        }
        default:
        {
            CK_INVALID_ENUM(_LifetimeBehavior);
            break;
        };
    }

    Super::BeginPlay();
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

    Context.AddTag(FAssetRegistryTag("IsCueAsset", "true", FAssetRegistryTag::TT_Hidden));

    const auto CueName = Get_CueName();
    if (ck::IsValid(CueName))
    {
        Context.AddTag(FAssetRegistryTag("CueName", CueName.ToString(), FAssetRegistryTag::TT_Hidden));
    }

    Context.AddTag(FAssetRegistryTag("CueBaseClass", GetClass()->GetName(), FAssetRegistryTag::TT_Hidden));
}
#endif

// --------------------------------------------------------------------------------------------------------------------
