#include "CkUsf/MultiPass/CkUsf_MultiPassRenderer.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"
#include "CkUsf/Apply/CkUsf_Utils.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetRenderingLibrary.h"

// --------------------------------------------------------------------------------------------------------------------

UCkUsf_MultiPassRenderer::UCkUsf_MultiPassRenderer()
{
    PrimaryComponentTick.bCanEverTick = true;
}

auto
    UCkUsf_MultiPassRenderer::
    Get_OutputRenderTarget() const
    -> UTextureRenderTarget2D*
{
    return _DisplayRT;
}

auto
    UCkUsf_MultiPassRenderer::
    BeginPlay()
    -> void
{
    Super::BeginPlay();
    DoSetup();
}

auto
    UCkUsf_MultiPassRenderer::
    DoSetup()
    -> void
{
    if (ck::Is_NOT_Valid(_Definition))
    {
        ck::usf::Warning(TEXT("CkUsf MultiPassRenderer: no definition assigned"));
        return;
    }

    const auto Res = FMath::Max(_Definition->_Resolution, 4);
    constexpr auto Format = ETextureRenderTargetFormat::RTF_RGBA16f;

    for (const auto& Pass : _Definition->_Passes)
    {
        if (ck::Is_NOT_Valid(Pass._Look))
        {
            ck::usf::Warning(TEXT("CkUsf MultiPassRenderer: pass [{}] has no look"), static_cast<int32>(Pass._Id));
            continue;
        }

        auto* MID = UCk_Utils_Usf_UE::Create_MID_ForLook(Pass._Look, this);
        if (ck::IsValid(MID, ck::IsValid_Policy_NullptrOnly{}))
        { _PassMIDs.Add(Pass._Id, MID); }

        if (Pass._Id != ECk_Usf_PassId::Image)
        {
            auto* RtA = UKismetRenderingLibrary::CreateRenderTarget2D(this, Res, Res, Format);
            auto* RtB = UKismetRenderingLibrary::CreateRenderTarget2D(this, Res, Res, Format);
            UKismetRenderingLibrary::ClearRenderTarget2D(this, RtA, FLinearColor::Black);
            UKismetRenderingLibrary::ClearRenderTarget2D(this, RtB, FLinearColor::Black);
            _BufferRead.Add(Pass._Id, RtA);
            _BufferWrite.Add(Pass._Id, RtB);
        }
    }

    _DisplayRT = UKismetRenderingLibrary::CreateRenderTarget2D(this, Res, Res, Format);
    UKismetRenderingLibrary::ClearRenderTarget2D(this, _DisplayRT, FLinearColor::Black);

    _Initialized = true;
}

auto
    UCkUsf_MultiPassRenderer::
    TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction)
    -> void
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (_Initialized == false)
    { return; }

    const auto Res = static_cast<float>(FMath::Max(_Definition->_Resolution, 4));

    for (const auto& Pass : _Definition->_Passes)
    {
        UMaterialInstanceDynamic* MID = _PassMIDs.FindRef(Pass._Id);
        if (ck::Is_NOT_Valid(MID, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        MID->SetVectorParameterValue(TEXT("iResolution"), FLinearColor(Res, Res, 0.0f, 1.0f));
        MID->SetScalarParameterValue(TEXT("iFrame"), static_cast<float>(_Frame));
        MID->SetScalarParameterValue(TEXT("iTimeDelta"), DeltaTime);

        for (const auto& Binding : Pass._BufferChannels)
        {
            UTextureRenderTarget2D* SourceRt = _BufferRead.FindRef(Binding._Buffer);
            if (ck::IsValid(SourceRt, ck::IsValid_Policy_NullptrOnly{}))
            {
                const auto ParamName = FName(*FString::Printf(TEXT("iChannel%d"), Binding._ChannelIndex));
                MID->SetTextureParameterValue(ParamName, SourceRt);
            }
        }

        UTextureRenderTarget2D* Target = Pass._Id == ECk_Usf_PassId::Image
            ? _DisplayRT.Get()
            : _BufferWrite.FindRef(Pass._Id).Get();

        if (ck::IsValid(Target, ck::IsValid_Policy_NullptrOnly{}))
        { UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, Target, MID); }
    }

    // Ping-pong every buffer so next frame reads what we just wrote.
    for (auto& Pair : _BufferRead)
    {
        UTextureRenderTarget2D* Written = _BufferWrite.FindRef(Pair.Key);
        UTextureRenderTarget2D* PrevRead = Pair.Value;
        Pair.Value = Written;
        _BufferWrite.Add(Pair.Key, PrevRead);
    }

    ++_Frame;
}

// --------------------------------------------------------------------------------------------------------------------
