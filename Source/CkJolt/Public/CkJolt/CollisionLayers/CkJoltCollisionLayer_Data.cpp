#include "CkJoltCollisionLayer_Data.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <Components/PrimitiveComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_collision_layer
{
    constexpr uint64 BitsPerChannel = 2;
    constexpr uint64 ChannelMask = 0b11;

    static auto Conv(ECollisionResponse InResponse) -> ECk_Jolt_PairInteraction
    {
        switch (InResponse)
        {
            case ECR_Ignore:  return ECk_Jolt_PairInteraction::Ignore;
            case ECR_Overlap: return ECk_Jolt_PairInteraction::Overlap;
            case ECR_Block:   return ECk_Jolt_PairInteraction::Block;
            default:
                // Engine enum — no Ck formatter; format the raw value.
                CK_TRIGGER_ENSURE(TEXT("Invalid ECollisionResponse [{}]"), static_cast<int32>(InResponse));
                return ECk_Jolt_PairInteraction::Ignore;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_CollisionSignature::
    Get_ResponseToChannel(
        ECollisionChannel InChannel) const
    -> ECk_Jolt_PairInteraction
{
    using namespace ck_jolt_collision_layer;

    const auto ChannelIndex = static_cast<uint64>(InChannel);
    const auto Bits = (_ResponseMask >> (ChannelIndex * BitsPerChannel)) & ChannelMask;
    return static_cast<ECk_Jolt_PairInteraction>(Bits);
}

auto
    FCk_Jolt_CollisionSignature::
    Set_ResponseToChannel(
        ECollisionChannel InChannel,
        ECk_Jolt_PairInteraction InResponse)
    -> void
{
    using namespace ck_jolt_collision_layer;

    const auto ChannelIndex = static_cast<uint64>(InChannel);
    _ResponseMask &= ~(ChannelMask << (ChannelIndex * BitsPerChannel));
    _ResponseMask |= static_cast<uint64>(InResponse) << (ChannelIndex * BitsPerChannel);
}

auto
    FCk_Jolt_CollisionSignature::
    Get_IsSolidObstacle() const
    -> bool
{
    // A QueryOnly volume reports overlaps but nothing ever collides with it, so it is not solid space
    // no matter what its responses say.
    if (NOT CollisionEnabledHasPhysics(_CollisionEnabled))
    { return false; }

    // Block (2) is the only response whose 2-bit group has the high bit set, so one masked test
    // answers all 32 channels at once — this runs per body inside per-cell query loops.
    static_assert(static_cast<uint64>(ECk_Jolt_PairInteraction::Block) == 0b10,
        "The high-bit test assumes Block is the only response with bit 1 set");

    constexpr auto BlockBitOfEveryChannel = 0xAAAAAAAAAAAAAAAAull;
    return (_ResponseMask & BlockBitOfEveryChannel) != 0;
}

auto
    FCk_Jolt_CollisionSignature::
    Make_FromComponent(
        const UPrimitiveComponent& InComponent,
        ECk_Jolt_BodyDomain InDomain)
    -> FCk_Jolt_CollisionSignature
{
    auto Signature = FCk_Jolt_CollisionSignature{};
    Signature.Set_ObjectChannel(InComponent.GetCollisionObjectType());
    Signature.Set_CollisionEnabled(InComponent.GetCollisionEnabled());
    Signature.Set_Domain(InDomain);

    const auto& Responses = InComponent.GetCollisionResponseToChannels();
    for (auto ChannelIndex = 0; ChannelIndex < 32; ++ChannelIndex)
    {
        const auto Channel = static_cast<ECollisionChannel>(ChannelIndex);
        Signature.Set_ResponseToChannel(Channel,
            ck_jolt_collision_layer::Conv(Responses.GetResponse(Channel)));
    }

    return Signature;
}

// --------------------------------------------------------------------------------------------------------------------
