#include "CkRenderTarget_RepData.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

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
