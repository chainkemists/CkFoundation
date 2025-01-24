#include "CkEntityReplicationChannel_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityReplicationChannel_UE::
    Add(
        FCk_Handle& InHandle)
    -> FCk_Handle_EntityReplicationChannel
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle),
        TEXT("Cannot Add a EntityReplicationChannel to Entity [{}] because it does NOT have an Owning Actor"), InHandle)
    { return {}; }

    InHandle.Add<ck::FTag_EntityReplicationChannel>();
    InHandle.Add<ck::FTag_EntityReplicationChannel_NeedsSetup>();
    UCk_Utils_GameplayLabel_UE::Add(InHandle, TAG_Label_EntityReplicationChannel);

    return Cast(InHandle);
}

auto
    UCk_Utils_EntityReplicationChannel_UE::
    Get_ChannelActor(
        const FCk_Handle_EntityReplicationChannel& InEntityReplicationChannel)
    -> ACk_EcsChannel_Actor_UE*
{
    return ::Cast<ACk_EcsChannel_Actor_UE>(UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InEntityReplicationChannel));
}

// --------------------------------------------------------------------------------------------------------------------


CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_EntityReplicationChannel_UE, FCk_Handle_EntityReplicationChannel, ck::FTag_EntityReplicationChannel);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityReplicationChannelOwner_UE::
    AddNewChannel(
        FCk_Handle& InEntityReplicationChannelOwnerEntity,
        FCk_Handle_EntityReplicationChannel& InEcsChannel)
    -> void
{
    InEntityReplicationChannelOwnerEntity.AddOrGet<ck::FFragment_EntityReplicationChannelOwner>();
    RecordOfEntityReplicationChannels_Utils::AddIfMissing(InEntityReplicationChannelOwnerEntity, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfEntityReplicationChannels_Utils::Request_Connect(InEntityReplicationChannelOwnerEntity, InEcsChannel);
}

auto
    UCk_Utils_EntityReplicationChannelOwner_UE::
    Get_NextAvailableEcsChannel(
        FCk_Handle& InEntityReplicationChannelOwnerEntity)
    -> FCk_Handle_EntityReplicationChannel
{
    const auto& AllEcsChannels = RecordOfEntityReplicationChannels_Utils::Get_ValidEntries(InEntityReplicationChannelOwnerEntity);

    auto& EntityReplicationChannelOwner = InEntityReplicationChannelOwnerEntity.Get<ck::FFragment_EntityReplicationChannelOwner>();

    const auto& CurrentAvailableEcsChannel = AllEcsChannels[EntityReplicationChannelOwner.Get_NextAvailableEcsChannelIndex()];

    const auto& NextAvailableEcsChannelIndex = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap
    (
        EntityReplicationChannelOwner.Get_NextAvailableEcsChannelIndex(),
        FCk_IntRange{0, AllEcsChannels.Num()}
    );

    EntityReplicationChannelOwner._NextAvailableEcsChannelIndex = NextAvailableEcsChannelIndex;

    return CurrentAvailableEcsChannel;
}

auto
    UCk_Utils_EntityReplicationChannelOwner_UE::
    ForEach_EntityReplicationChannel(
        FCk_Handle& InEntityReplicationChannelOwnerEntity,
        const TFunction<void(FCk_Handle_EntityReplicationChannel&)>& InFunc)
    -> void
{
    RecordOfEntityReplicationChannels_Utils::ForEach_ValidEntry
    (
        InEntityReplicationChannelOwnerEntity,
        [&](FCk_Handle_EntityReplicationChannel InEntityReplicationChannel)
        {
            InFunc(InEntityReplicationChannel);
        }
    );
}

// --------------------------------------------------------------------------------------------------------------------
