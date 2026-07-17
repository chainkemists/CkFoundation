#pragma once

#include "CkJolt/Character/CkJoltCharacter_Fragment_Data.h"
#include "CkJolt/World/CkJoltWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // One shared CharacterContactListener owned by FJoltWorld; every CharacterVirtual is pointed at it via
    // SetListener during setup. On each character-vs-body contact it fills the push behaviour from that
    // character's authored PushPolicy, resolved back through the FJoltWorld character registry (the listener
    // callback provides inCharacter; the registry is stable during the step — the game thread never mutates
    // it mid-step per FJoltWorld's threading contract).
    class CKJOLT_API CkJoltCharacterContactListener final : public JPH::CharacterContactListener
    {
    public:
        explicit CkJoltCharacterContactListener(FJoltWorld* InJoltWorld)
            : _JoltWorld(InJoltWorld)
        {
        }

        auto
            OnContactAdded(
                const JPH::CharacterVirtual* inCharacter,
                const JPH::BodyID& inBodyID2,
                const JPH::SubShapeID& inSubShapeID2,
                JPH::RVec3Arg inContactPosition,
                JPH::Vec3Arg inContactNormal,
                JPH::CharacterContactSettings& ioSettings)
            -> void override
        {
            // PushPolicy -> {mCanPushCharacter, mCanReceiveImpulses}. mCanPushCharacter = the body may push the
            // character; mCanReceiveImpulses = the character may impulse (push) the body. Absent registry (torn
            // down / never registered) falls back to Jolt's default {true, true} — a correct silent path.
            const auto PushPolicy = _JoltWorld != nullptr
                ? _JoltWorld->Get_CharacterPushPolicy(inCharacter)
                : ECk_JoltCharacter_PushPolicy::PushAndBePushed;

            switch (PushPolicy)
            {
                case ECk_JoltCharacter_PushPolicy::PushAndBePushed:
                {
                    ioSettings.mCanPushCharacter = true;
                    ioSettings.mCanReceiveImpulses = true;
                    break;
                }
                case ECk_JoltCharacter_PushPolicy::PushOnly:
                {
                    ioSettings.mCanPushCharacter = false;
                    ioSettings.mCanReceiveImpulses = true;
                    break;
                }
                case ECk_JoltCharacter_PushPolicy::BePushedOnly:
                {
                    ioSettings.mCanPushCharacter = true;
                    ioSettings.mCanReceiveImpulses = false;
                    break;
                }
                case ECk_JoltCharacter_PushPolicy::Neither:
                {
                    ioSettings.mCanPushCharacter = false;
                    ioSettings.mCanReceiveImpulses = false;
                    break;
                }
            }
        }

    private:
        FJoltWorld* _JoltWorld = nullptr;
    };
}

// --------------------------------------------------------------------------------------------------------------------
