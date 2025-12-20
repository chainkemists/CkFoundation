#include "CkVfxCue_DataChannel_EntityScript.h"

#include "CkVfx/CkVfx_Log.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <NiagaraDataChannelPublic.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_VfxCue_DataChannel_EntityScript::
    UCk_VfxCue_DataChannel_EntityScript(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    this->_LifetimeBehavior = ECk_Cue_LifetimeBehavior::AfterOneFrame;
}

auto
    UCk_VfxCue_DataChannel_EntityScript::
    Get_CueName_Implementation() const
    -> FGameplayTag
{
    if (GetClass() == StaticClass())
    { return TAG_Cue_DoNotExecute; }

    return Super::Get_CueName_Implementation();
}

auto
    UCk_VfxCue_DataChannel_EntityScript::
    BeginPlay()
    -> void
{
    Super::BeginPlay();
    WriteBatchDataToChannel();
}

auto
    UCk_VfxCue_DataChannel_EntityScript::
    Restart()
    -> void
{
    Super::Restart();
    WriteBatchDataToChannel();
}

auto
    UCk_VfxCue_DataChannel_EntityScript::
    Get_IsConfigurationValid() const
    -> bool
{
    return ck::IsValid(_DataChannel);
}

auto
    UCk_VfxCue_DataChannel_EntityScript::
    WriteBatchDataToChannel()
    -> void
{
    CK_ENSURE_IF_NOT(Get_IsConfigurationValid(), TEXT("VfxCue DataChannel [{}] has invalid configuration"), Get_CueName())
    {
        ck::vfx::Warning(TEXT("VfxCue DataChannel [{}] destroying due to invalid configuration"), Get_CueName());
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_AssociatedEntity);
        return;
    }

    if (_BatchData.IsEmpty())
    {
        ck::vfx::Warning(TEXT("VfxCue DataChannel [{}] has no batch data - nothing to write"), Get_CueName());
        return;
    }

    auto World = GetWorld();
    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("VfxCue DataChannel [{}] has invalid world"), Get_CueName())
    { return; }

    const auto DebugSource = ck::Format_UE(TEXT("VfxCue_DataChannel_{}"), Get_CueName().ToString());

    for (int32 Index = 0; Index < _BatchData.Num(); ++Index)
    {
        const auto& BatchEntry = _BatchData[Index];

        auto SearchParams = FNiagaraDataChannelSearchParameters{};
        SearchParams.Location = BatchEntry.Get_SearchLocation();

        auto Writer = UNiagaraDataChannelLibrary::WriteToNiagaraDataChannel(
            World,
            _DataChannel,
            SearchParams,
            1,
            _VisibleToGame,
            _VisibleToNiagaraCPU,
            _VisibleToNiagaraGPU,
            DebugSource);

        CK_ENSURE_IF_NOT(ck::IsValid(Writer), TEXT("VfxCue DataChannel [{}] failed to get writer for batch [{}]"), Get_CueName(), Index)
        { continue; }

        for (const auto& Parameter : BatchEntry.Get_Parameters())
        {
            WriteParameterToChannel(Writer, 0, Parameter);
        }
    }

    ck::vfx::Verbose(TEXT("VfxCue DataChannel [{}] wrote [{}] batch entries to data channel [{}]"),
        Get_CueName(), _BatchData.Num(), _DataChannel->GetName());
}

auto
    UCk_VfxCue_DataChannel_EntityScript::
    WriteParameterToChannel(
        UNiagaraDataChannelWriter* InWriter,
        int32 InIndex,
        const FCk_VfxCue_UserParameter& InParameter)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWriter), TEXT("Writer is invalid"))
    { return; }

    const auto ParameterName = InParameter.Get_ParameterName();

    switch (InParameter.Get_ParameterType())
    {
        case ECk_VfxCue_ParameterType::Float:
        {
            InWriter->WriteFloat(ParameterName, InIndex, InParameter.Get_FloatValue());
            break;
        }
        case ECk_VfxCue_ParameterType::Vector:
        {
            InWriter->WritePosition(ParameterName, InIndex, InParameter.Get_VectorValue());
            break;
        }
        case ECk_VfxCue_ParameterType::Color:
        {
            InWriter->WriteLinearColor(ParameterName, InIndex, InParameter.Get_ColorValue());
            break;
        }
        case ECk_VfxCue_ParameterType::Bool:
        {
            InWriter->WriteBool(ParameterName, InIndex, InParameter.Get_BoolValue());
            break;
        }
        case ECk_VfxCue_ParameterType::Int:
        {
            InWriter->WriteInt(ParameterName, InIndex, InParameter.Get_IntValue());
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InParameter.Get_ParameterType());
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------