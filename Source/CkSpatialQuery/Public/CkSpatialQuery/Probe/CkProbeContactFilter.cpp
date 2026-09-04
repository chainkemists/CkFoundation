#include "CkProbeContactFilter.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::spatialquery
{
    FCk_ProbeContactFilter::
        FCk_ProbeContactFilter(
            int32 InMaxSignatures)
        : _MaxSignatures(FMath::Max(0, InMaxSignatures))
    {
        // Jolt workers read published entries while the game thread may register another signature. Never reallocate.
        _Signatures.Reserve(_MaxSignatures);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_ProbeContactFilter::
        Get_OrRegisterSignature(
            const FCk_Fragment_Probe_ParamsData& InParams)
        -> uint32
    {
        const auto Signature = FCk_ProbeContactSignature{
            .ProbeName = InParams.Get_ProbeName(),
            .ResponsePolicy = InParams.Get_ResponsePolicy(),
            .Filter = InParams.Get_Filter()};

        for (uint32 Index = 0; Index < static_cast<uint32>(_Signatures.Num()); ++Index)
        {
            const auto& Existing = _Signatures[Index];
            if (Existing.ProbeName == Signature.ProbeName
                && Existing.ResponsePolicy == Signature.ResponsePolicy
                && Existing.Filter == Signature.Filter)
            {
                return Index;
            }
        }

        const auto HasSignatureCapacity = _Signatures.Num() < _MaxSignatures;
        CK_ENSURE_IF_NOT(HasSignatureCapacity,
            TEXT("Probe contact-filter signature capacity [{}] exhausted; probe body creation will be rejected."),
            _MaxSignatures)
        { return JPH::CollisionGroup::cInvalidSubGroup; }

        const auto SignatureId = static_cast<uint32>(_Signatures.Num());
        _Signatures.Emplace(Signature);

        // Publish only after every value field is fully written. Acquire readers may now index this entry safely.
        _PublishedSignatureCount.store(static_cast<uint32>(_Signatures.Num()), std::memory_order_release);
        return SignatureId;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_ProbeContactFilter::
        CanCollide(
            const JPH::CollisionGroup& InGroupA,
            const JPH::CollisionGroup& InGroupB) const
        -> bool
    {
        // A Jolt CollisionGroup calls the first non-null filter. Never suppress a body we do not own.
        if (InGroupA.GetGroupFilter() != this || InGroupB.GetGroupFilter() != this)
        { return true; }

        const auto SignatureA = InGroupA.GetSubGroupID();
        const auto SignatureB = InGroupB.GetSubGroupID();
        if (NOT Get_IsSignaturePublished(SignatureA) || NOT Get_IsSignaturePublished(SignatureB))
        { return true; }

        // Reserve() makes this address stable for the filter's lifetime. Avoid TArray::operator[] here: its
        // development range check reads ArrayNum while the game thread may be publishing a different entry.
        const auto* Signatures = _Signatures.GetData();
        const auto& A = Signatures[SignatureA];
        const auto& B = Signatures[SignatureB];

        // Preserve game-thread semantics: either notifying side can receive the pair. Same signatures are legal.
        return Get_CanReceive(A, B) || Get_CanReceive(B, A);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_ProbeContactFilter::
        Get_IsSignaturePublished(
            uint32 InSignatureId) const
        -> bool
    {
        return InSignatureId < _PublishedSignatureCount.load(std::memory_order_acquire);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_ProbeContactFilter::
        Get_CanReceive(
            const FCk_ProbeContactSignature& InReceiver,
            const FCk_ProbeContactSignature& InOther) const
        -> bool
    {
        if (InReceiver.ResponsePolicy == ECk_ProbeResponse_Policy::Silent)
        { return false; }

        if (InReceiver.Filter.IsEmpty())
        { return true; }

        return InOther.ProbeName.MatchesAny(InReceiver.Filter);
    }

    // ----------------------------------------------------------------------------------------------------------------

    FCk_ProbeContactFilter_Context::
        FCk_ProbeContactFilter_Context()
        : Filter(new FCk_ProbeContactFilter{}) {}

    auto
        FCk_ProbeContactFilter_Context::
        Get_OrRegisterSignature(
            const FCk_Fragment_Probe_ParamsData& InParams)
        const -> uint32
    {
        return Filter != nullptr
            ? Filter->Get_OrRegisterSignature(InParams)
            : JPH::CollisionGroup::cInvalidSubGroup;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::spatialquery
{
    JPH_IMPLEMENT_RTTI_VIRTUAL(FCk_ProbeContactFilter)
    {
        JPH_ADD_BASE_CLASS(FCk_ProbeContactFilter, JPH::GroupFilter)
    }
}

// --------------------------------------------------------------------------------------------------------------------
