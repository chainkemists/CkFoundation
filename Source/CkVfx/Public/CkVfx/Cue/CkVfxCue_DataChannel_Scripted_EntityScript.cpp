#include "CkVfxCue_DataChannel_Scripted_EntityScript.h"

#include "CkVfx/CkVfx_Log.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <NiagaraDataChannelPublic.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_VfxCue_DataChannel_Scripted_EntityScript::
    UCk_VfxCue_DataChannel_Scripted_EntityScript(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    this->_LifetimeBehavior = ECk_Cue_LifetimeBehavior::AfterOneFrame;
}

auto
    UCk_VfxCue_DataChannel_Scripted_EntityScript::
    Get_CueName_Implementation() const
    -> FGameplayTag
{
    if (GetClass() == StaticClass())
    { return TAG_Cue_DoNotExecute; }

    return Super::Get_CueName_Implementation();
}

auto
    UCk_VfxCue_DataChannel_Scripted_EntityScript::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    CK_ENSURE_IF_NOT(Get_IsConfigurationValid(), TEXT("VfxCue DataChannel Scripted [{}] has invalid configuration"), Get_CueName())
    {
        ck::vfx::Warning(TEXT("VfxCue DataChannel Scripted [{}] destroying due to invalid configuration"), Get_CueName());
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_AssociatedEntity);
        return;
    }

    if (_BatchCount <= 0)
    {
        ck::vfx::Warning(TEXT("VfxCue DataChannel Scripted [{}] has invalid batch count [{}]"), Get_CueName(), _BatchCount);
        return;
    }

    auto World = GetWorld();
    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("VfxCue DataChannel Scripted [{}] has invalid world"), Get_CueName())
    { return; }

    const auto DebugSource = FString::Printf(TEXT("VfxCue_DataChannel_Scripted_%s"), *Get_CueName().ToString());

    auto SearchParams = FNiagaraDataChannelSearchParameters{};
    SearchParams.Location = _Location;

    auto Writer = UNiagaraDataChannelLibrary::WriteToNiagaraDataChannel(
        World,
        _DataChannel,
        SearchParams,
        _BatchCount,
        _VisibleToGame,
        _VisibleToNiagaraCPU,
        _VisibleToNiagaraGPU,
        DebugSource);

    CK_ENSURE_IF_NOT(ck::IsValid(Writer), TEXT("VfxCue DataChannel Scripted [{}] failed to get writer"), Get_CueName())
    { return; }

    WriteDataChannelParameters(Writer, _BatchCount);

    ck::vfx::Verbose(TEXT("VfxCue DataChannel Scripted [{}] wrote [{}] batch entries to data channel [{}] at location [{}]"),
        Get_CueName(), _BatchCount, _DataChannel->GetName(), _Location.ToString());
}

auto
    UCk_VfxCue_DataChannel_Scripted_EntityScript::
    Get_IsConfigurationValid() const
    -> bool
{
    return ck::IsValid(_DataChannel);
}

auto
    UCk_VfxCue_DataChannel_Scripted_EntityScript::
    WriteDataChannelParameters_Implementation(
        UNiagaraDataChannelWriter* InWriter,
        int32 InBatchCount)
    -> void
{
    ck::vfx::Warning(TEXT("VfxCue DataChannel Scripted [{}] WriteDataChannelParameters not implemented - override this function"), Get_CueName());
}

// --------------------------------------------------------------------------------------------------------------------
