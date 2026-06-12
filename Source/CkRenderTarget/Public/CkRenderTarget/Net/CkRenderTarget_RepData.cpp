#include "CkRenderTarget_RepData.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

// Tier-A: pure value data, captured via tagged-property serialization of its SaveGame fields. The
// nested FCk_RenderTarget_InstructionBatch / FCk_RenderTarget_DrawCmd are reached through the same
// SaveGame-gated recursion (their own SaveGame fields), so only the owning fragment is registered.
CK_REGISTER_SNAPSHOTABLE(FFragment_RenderTarget_AuthoredLog);

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
