#include "CkJoltCollisionLayerTable.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Log.h"

#include <Engine/CollisionProfile.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    FCk_Jolt_CollisionLayerTable::
        FCk_Jolt_CollisionLayerTable()
    {
        // Fixed capacity — Jolt filter callbacks read the array from worker threads during
        // Update; it must NEVER reallocate after construction.
        _Signatures.Reserve(MaxLayers);
    }

    auto
        FCk_Jolt_CollisionLayerTable::
        Build_FromCollisionProfiles()
        -> void
    {
        const auto* CollisionProfiles = UCollisionProfile::Get();

        CK_ENSURE_IF_NOT(ck::IsValid(CollisionProfiles, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("UCollisionProfile unavailable — the Jolt layer table cannot seed from profiles"))
        { return; }

        // Deterministic seed order: engine + project profiles in registration order, each
        // registered for both domains (a profile authored for statics may be used by dynamics).
        for (auto ProfileIndex = 0; ProfileIndex < CollisionProfiles->GetNumOfProfiles(); ++ProfileIndex)
        {
            const auto* Template = CollisionProfiles->GetProfileByIndex(ProfileIndex);

            if (Template == nullptr || Template->CollisionEnabled == ECollisionEnabled::NoCollision)
            { continue; }

            auto Signature = FCk_Jolt_CollisionSignature{};
            Signature.Set_ObjectChannel(Template->ObjectType);
            Signature.Set_CollisionEnabled(Template->CollisionEnabled);

            for (auto ChannelIndex = 0; ChannelIndex < 32; ++ChannelIndex)
            {
                const auto Channel = static_cast<ECollisionChannel>(ChannelIndex);
                const auto Response = Template->ResponseToChannels.GetResponse(Channel);

                Signature.Set_ResponseToChannel(Channel,
                    Response == ECR_Block ? ECk_Jolt_PairInteraction::Block :
                    Response == ECR_Overlap ? ECk_Jolt_PairInteraction::Overlap :
                    ECk_Jolt_PairInteraction::Ignore);
            }

            Signature.Set_Domain(ECk_Jolt_BodyDomain::Static);
            Get_OrRegisterLayer(Signature);

            Signature.Set_Domain(ECk_Jolt_BodyDomain::Dynamic);
            Get_OrRegisterLayer(Signature);
        }

        _ProbeLayer = Get_OrRegisterLayer(Get_ProbeSignature());

        ck::jolt::Log(TEXT("Jolt layer table: [{}] layers from [{}] collision profiles (probe layer [{}])"),
            Get_NumLayers(), CollisionProfiles->GetNumOfProfiles(), _ProbeLayer);
    }

    auto
        FCk_Jolt_CollisionLayerTable::
        Get_OrRegisterLayer(
            const FCk_Jolt_CollisionSignature& InSignature)
        -> uint16
    {
        if (const auto* Existing = _SignatureToLayer.Find(InSignature))
        { return *Existing; }

        CK_ENSURE_IF_NOT(_Signatures.Num() < MaxLayers,
            TEXT("Jolt layer table capacity [{}] EXHAUSTED — pathological unique-signature explosion "
                 "(per-component custom responses?). New bodies will be refused a layer."), MaxLayers)
        { return JPH::cObjectLayerInvalid; }

        const auto NewLayer = static_cast<uint16>(_Signatures.Num());
        _Signatures.Emplace(InSignature);
        _SignatureToLayer.Add(InSignature, NewLayer);

        // Publish AFTER the entry is fully written — lock-free readers never see a torn entry.
        _PublishedCount.store(_Signatures.Num(), std::memory_order_release);

        return NewLayer;
    }

    auto
        FCk_Jolt_CollisionLayerTable::
        Get_ProbeLayer() const
        -> uint16
    {
        return _ProbeLayer;
    }

    auto
        FCk_Jolt_CollisionLayerTable::
        Get_ProbeSignature()
        -> FCk_Jolt_CollisionSignature
    {
        auto Signature = FCk_Jolt_CollisionSignature{};
        Signature.Set_ObjectChannel(ECC_WorldDynamic);
        Signature.Set_CollisionEnabled(ECollisionEnabled::QueryOnly);
        Signature.Set_Domain(ECk_Jolt_BodyDomain::Dynamic);
        Signature.Set_ResponseToChannel(ECC_WorldDynamic, ECk_Jolt_PairInteraction::Overlap);
        return Signature;
    }

    auto
        FCk_Jolt_CollisionLayerTable::
        Get_NumLayers() const
        -> int32
    {
        return _PublishedCount.load(std::memory_order_acquire);
    }

    auto
        FCk_Jolt_CollisionLayerTable::
        Get_Signature(
            uint16 InLayer) const
        -> const FCk_Jolt_CollisionSignature&
    {
        static const auto InvalidSignature = FCk_Jolt_CollisionSignature{};

        if (InLayer >= Get_NumLayers())
        { return InvalidSignature; }

        return _Signatures[InLayer];
    }

    auto
        FCk_Jolt_CollisionLayerTable::
        Get_PairInteraction(
            uint16 InLayerA,
            uint16 InLayerB) const
        -> ECk_Jolt_PairInteraction
    {
        const auto Count = Get_NumLayers();
        if (InLayerA >= Count || InLayerB >= Count)
        { return ECk_Jolt_PairInteraction::Ignore; }

        const auto& SignatureA = _Signatures[InLayerA];
        const auto& SignatureB = _Signatures[InLayerB];

        const auto AToB = SignatureA.Get_ResponseToChannel(SignatureB.Get_ObjectChannel());
        const auto BToA = SignatureB.Get_ResponseToChannel(SignatureA.Get_ObjectChannel());

        return AToB < BToA ? AToB : BToA;
    }

    auto
        FCk_Jolt_CollisionLayerTable::
        Get_ResponseOfLayerToChannel(
            uint16 InLayer,
            ECollisionChannel InChannel) const
        -> ECk_Jolt_PairInteraction
    {
        if (InLayer >= Get_NumLayers())
        { return ECk_Jolt_PairInteraction::Ignore; }

        return _Signatures[InLayer].Get_ResponseToChannel(InChannel);
    }

    auto
        FCk_Jolt_CollisionLayerTable::
        Get_Domain(
            uint16 InLayer) const
        -> ECk_Jolt_BodyDomain
    {
        if (InLayer >= Get_NumLayers())
        { return ECk_Jolt_BodyDomain::Dynamic; }

        return _Signatures[InLayer].Get_Domain();
    }
}

// --------------------------------------------------------------------------------------------------------------------
