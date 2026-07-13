#include "CkRenderTarget_RepData.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FFragment_RenderTarget_AuthoredLog::
    Record_PublishedBatch(
        const FCk_RenderTarget_InstructionBatch& InBatch,
        int32 InNextBatchSeq)
    -> void
{
    _Batches.Emplace(InBatch);

    if (_Batches.Num() > FCk_RenderTarget_ChannelState::RingSize)
    { _Batches.RemoveAt(0); }

    _NextBatchSeq = InNextBatchSeq;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_RepData_RenderTarget::
    Find_Channel(
        const FGameplayTag& InSyncName)
    -> FCk_RenderTarget_ChannelState*
{
    return _Channels.FindByPredicate([&](const FCk_RenderTarget_ChannelState& InChannel) -> bool
    {
        return InChannel.Get_SyncName() == InSyncName;
    });
}

auto
    FCk_RepData_RenderTarget::
    Find_Channel(
        const FGameplayTag& InSyncName) const
    -> const FCk_RenderTarget_ChannelState*
{
    return _Channels.FindByPredicate([&](const FCk_RenderTarget_ChannelState& InChannel) -> bool
    {
        return InChannel.Get_SyncName() == InSyncName;
    });
}

// --------------------------------------------------------------------------------------------------------------------
