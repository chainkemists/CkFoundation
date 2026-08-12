#include "CkInputButtonMap_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkInput/CkInput_Log.h"
#include "CkInput/CkKeyBinding_Utils.h"

#include <Engine/GameInstance.h>
#include <Engine/LocalPlayer.h>
#include <Engine/World.h>
#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_InputButtonMap_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_InputButtonMap_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_input_button_map_processor
{
    // TPair's .Key/.Value would make the FKey the ".Value" of a pair whose ".Key" is a slot — named members
    // keep the derive loop readable.
    struct FSlotKey
    {
        EPlayerMappableKeySlot Slot = EPlayerMappableKeySlot::Unspecified;
        FKey Key;
    };

    auto
        Get_IndexOfButton(
            const TArray<FCk_Input_ButtonAssociation>& InAssociations,
            const FCk_Input_ButtonId& InButtonId)
        -> int32
    {
        return InAssociations.IndexOfByPredicate(
        [&](const FCk_Input_ButtonAssociation& InExisting) -> bool
        {
            return InExisting.Get_ButtonId() == InButtonId;
        });
    }

    // The profile lives on the local player, so the map has to walk out of ECS to reach it. Every step is a
    // quiet early-out rather than an ensure: a source composed before the engine has handed its player a
    // controller is the normal startup order, not a mistake, and the same walk succeeds a frame later.
    auto
        TryGet_PlayerController(
            const FCk_Handle& InButtonMap,
            int32 InLocalPlayerIndex)
        -> APlayerController*
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InButtonMap);
        if (ck::Is_NOT_Valid(World))
        { return nullptr; }

        const auto* GameInstance = World->GetGameInstance();
        if (ck::Is_NOT_Valid(GameInstance))
        { return nullptr; }

        const auto* LocalPlayer = GameInstance->GetLocalPlayerByIndex(InLocalPlayerIndex);
        if (ck::Is_NOT_Valid(LocalPlayer))
        { return nullptr; }

        return LocalPlayer->GetPlayerController(World);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_InputButtonMap_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InButtonMap,
            const FFragment_InputSource_Params& InSourceParams,
            FFragment_InputButtonMap_Current& InCurrent,
            FFragment_InputButtonMap_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InButtonMap, Result);

            Result = DoHandleRequest(InButtonMap, InSourceParams, InCurrent, InRequest);
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InButtonMap.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_InputButtonMap_HandleRequests::
        DoHandleRequest(
            HandleType InButtonMap,
            const FFragment_InputSource_Params& InSourceParams,
            FFragment_InputButtonMap_Current& InCurrent,
            const FCk_Request_InputButtonMap_Rederive& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto* PlayerController = ck_input_button_map_processor::TryGet_PlayerController(
            InButtonMap, InSourceParams.Get_LocalPlayerIndex());

        // No profile to read means there is nothing for the mapped tier to disagree with, so the map is already
        // consistent with what the player has: leave it untouched and let the next re-derive do the work.
        if (ck::Is_NOT_Valid(PlayerController))
        {
            input::VeryVerbose
            (
                TEXT("InputButtonMap [{}] skipped a re-derive: local player [{}] has no PlayerController yet"),
                InButtonMap, InSourceParams.Get_LocalPlayerIndex()
            );

            return ECk_Request_OperationResult::Succeeded;
        }

        // Associations are rebuilt from the profile rather than patched against it, so a mapping that stopped
        // being player-mappable is left holding no keys instead of its last ones. Identities are never
        // touched here — that is the stability contract the whole tier exists for.
        for (auto& Association : InCurrent._Associations)
        {
            if (Association.Get_ButtonId().Get_Tier() != ECk_Input_ButtonTier::Mapped)
            { continue; }

            Association.Set_Keys({});
        }

        const auto Mappings = UCk_Utils_KeyBinding_UE::Get_AllRemappableKeys(PlayerController);

        // A mapping name owns one button but may own several slots — a keyboard binding in one and a gamepad
        // binding in another — and the association carries EVERY bound slot's key. Gathered before writing
        // because the profile hands slots back in no contractual order while the key list is contractually
        // primary-first: ascending slot puts First at index 0 (the engine orders Unspecified after the named
        // slots), which is the key every scalar query in this module answers with.
        auto SlotKeysByIndex = TMap<int32, TArray<ck_input_button_map_processor::FSlotKey>>{};

        for (const auto& Mapping : Mappings)
        {
            const auto ButtonId = FCk_Input_ButtonId{ECk_Input_ButtonTier::Mapped, Mapping.GetMappingName()};

            const auto ExistingIndex = ck_input_button_map_processor::Get_IndexOfButton(
                InCurrent._Associations, ButtonId);

            const auto Index = ExistingIndex != INDEX_NONE
                ? ExistingIndex
                : InCurrent._Associations.Emplace(FCk_Input_ButtonAssociation{ButtonId});

            const auto& CurrentKey = Mapping.GetCurrentKey();

            // An unbound slot contributes nothing rather than an invalid entry, so an empty key list IS the
            // unbound state and no consumer has to filter invalid keys out of the middle of it.
            if (NOT CurrentKey.IsValid())
            { continue; }

            SlotKeysByIndex.FindOrAdd(Index).Emplace(
                ck_input_button_map_processor::FSlotKey{Mapping.GetSlot(), CurrentKey});
        }

        for (auto& Kvp : SlotKeysByIndex)
        {
            auto& SlotKeys = Kvp.Value;

            SlotKeys.Sort([](const ck_input_button_map_processor::FSlotKey& InA,
                             const ck_input_button_map_processor::FSlotKey& InB) -> bool
            {
                return static_cast<uint8>(InA.Slot) < static_cast<uint8>(InB.Slot);
            });

            auto Keys = TArray<FKey>{};
            Keys.Reserve(SlotKeys.Num());

            for (const auto& SlotKey : SlotKeys)
            {
                // Two slots may name one key; the association answers WHICH keys produce the button, not how
                // many slots do, so the duplicate collapses.
                Keys.AddUnique(SlotKey.Key);
            }

            InCurrent._Associations[Kvp.Key].Set_Keys(MoveTemp(Keys));
        }

        input::VeryVerbose
        (
            TEXT("InputButtonMap [{}] re-derived [{}] mapped rows from the profile of local player [{}]"),
            InButtonMap, Mappings.Num(), InSourceParams.Get_LocalPlayerIndex()
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_InputButtonMap_HandleRequests::
        DoHandleRequest(
            HandleType InButtonMap,
            const FFragment_InputSource_Params& InSourceParams,
            FFragment_InputButtonMap_Current& InCurrent,
            const FCk_Request_InputButtonMap_RegisterPhysicalButton& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto& Key = InRequest.Get_Key();
        const auto ButtonId = FCk_Input_ButtonId{ECk_Input_ButtonTier::Physical, Key.GetFName()};

        const auto ExistingIndex = ck_input_button_map_processor::Get_IndexOfButton(
            InCurrent._Associations, ButtonId);

        // A physical button's association is the key itself, so re-registering one cannot change anything the
        // caller asked for — the intent already holds.
        if (ExistingIndex != INDEX_NONE)
        { return ECk_Request_OperationResult::Succeeded; }

        auto Association = FCk_Input_ButtonAssociation{ButtonId};
        Association.Set_Keys(TArray<FKey>{Key});

        InCurrent._Associations.Emplace(Association);

        input::VeryVerbose
        (
            TEXT("InputButtonMap [{}] registered physical button [{}] on key [{}]"),
            InButtonMap, ButtonId.Get_Name(), Key
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InputButtonMap_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InButtonMap,
            const FFragment_InputButtonMap_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InButtonMap, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
