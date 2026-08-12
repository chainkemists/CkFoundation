#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkInput/CkInputButtonMap_Fragment.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkInputButtonMap_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * The button space of one input source: stable identities for pressable things, each paired with the key that
 * currently produces it.
 *
 * A ButtonId is what a definition names instead of a key. Tier-1 (Mapped) identities are minted one per
 * Enhanced Input player-mappable MAPPING NAME and their key association follows the player's resolved
 * mappings; tier-2 (Physical) identities are minted one per raw key nothing maps and never move. Rebinding is
 * therefore invisible to everything downstream: the association moves, the identity does not, and no
 * definition needs an edit.
 *
 * The feature lives ON the input-source entity — there is no Create, because a button map on a child entity
 * would have no player whose profile to derive from. It is opt-in: a source without it simply has no button
 * space.
 *
 * The association is MANY-TO-MANY. Key -> button: two mappings in different categories may share a key, and
 * duplicate bindings occur in real profiles, so Get_ButtonIdsForKey answers with every holder; a caller that
 * wants "the" button has to say which tier and name it means. Button -> keys: a Mapped button carries one key
 * per bound SLOT — a keyboard binding in one slot and a gamepad binding in another both produce it — so
 * Get_KeysForButton answers with every key, primary (slot First) first, and TryGet_KeyForButton answers the
 * primary alone.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_InputButtonMap"))
class CKINPUT_API UCk_Utils_InputButtonMap_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_InputButtonMap_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_InputButtonMap);

public:
    /**
     * Composes the button space onto an input source and starts its first derivation. Rejects a handle that is
     * not a source, and rejects the whole composition — leaving nothing behind — if any declared physical key
     * is invalid or is claimed by another declaration.
     *
     * The initial derivation runs through the same deferred path a later Request_Rederive does, so the map is
     * EMPTY on the calling stack and fills in when the requests drain. There is no separate first-derive
     * codepath that could disagree with the steady-state one.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputButtonMap",
              DisplayName = "[Ck][InputButtonMap] Add Feature")
    static FCk_Handle_InputButtonMap
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_InputButtonMap_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputButtonMap",
              DisplayName = "[Ck][InputButtonMap] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_InputButtonMap
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputButtonMap",
              DisplayName = "[Ck][InputButtonMap] Handle -> InputButtonMap Handle",
              meta = (CompactNodeTitle = "<AsInputButtonMap>", BlueprintAutocast))
    static FCk_Handle_InputButtonMap
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid InputButtonMap Handle",
              Category = "Ck|Utils|InputButtonMap",
              meta = (CompactNodeTitle = "INVALID_InputButtonMapHandle", Keywords = "make"))
    static FCk_Handle_InputButtonMap
    Get_InvalidHandle() { return {}; }

public:
    /**
     * Every button this map has minted, in no guaranteed order. Identities accumulate and are never dropped, so
     * this only ever grows — a mapping that disappeared is still listed, holding an invalid key.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputButtonMap",
              DisplayName = "[Ck][InputButtonMap] Get All Buttons")
    static TArray<FCk_Input_ButtonId>
    Get_AllButtons(
        const FCk_Handle_InputButtonMap& InButtonMap);

    /**
     * Every button this key currently produces. Returns them ALL because two mappings may legitimately share a
     * key; an empty result means nothing is bound to it. An invalid key answers empty rather than listing the
     * unbound buttons — "no key" is a state those buttons are in, not a key they answer to.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputButtonMap",
              DisplayName = "[Ck][InputButtonMap] Get Button Ids For Key")
    static TArray<FCk_Input_ButtonId>
    Get_ButtonIdsForKey(
        const FCk_Handle_InputButtonMap& InButtonMap,
        const FKey& InKey);

    /**
     * The PRIMARY key currently producing this button — the First-slot resolution for a tier-1 button, the
     * fixed key for a tier-2 one. An INVALID key is returned both for a button this map has never minted and
     * for one that is currently unbound; both mean "pressing nothing produces it", which is all a consumer can
     * act on. A button may carry MORE keys than this answers — a mapping bound on several slots (keyboard in
     * one, gamepad in another) resolves them all; a consumer that must honour every device reads
     * Get_KeysForButton instead.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputButtonMap",
              DisplayName = "[Ck][InputButtonMap] Try Get Key For Button")
    static FKey
    TryGet_KeyForButton(
        const FCk_Handle_InputButtonMap& InButtonMap,
        const FCk_Input_ButtonId& InButtonId);

    /**
     * Every key currently producing this button, primary first (ascending slot order). A Mapped button carries
     * one key per BOUND slot — a keyboard and a gamepad binding on one mapping name are both here — and a
     * Physical button carries exactly its own key. Empty both for a button this map has never minted and for
     * one that is currently unbound; both mean "pressing nothing produces it".
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputButtonMap",
              DisplayName = "[Ck][InputButtonMap] Get Keys For Button")
    static TArray<FKey>
    Get_KeysForButton(
        const FCk_Handle_InputButtonMap& InButtonMap,
        const FCk_Input_ButtonId& InButtonId);

public:
    /**
     * Re-reads every tier-1 association from the player's resolved mappings. Newly seen mapping names mint new
     * identities; existing ones never change and are never removed, so a mapping that disappeared keeps its
     * button and is left with an invalid key. Tier-2 buttons are not touched.
     *
     * Deferred: the refresh lands when the request drains, which is before the next routing pass. A re-derive
     * against a profile that has not moved is an accepted no-op and completes Succeeded.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputButtonMap",
              DisplayName = "[Ck][InputButtonMap] Request Rederive",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_InputButtonMap
    Request_Rederive(
        UPARAM(ref) FCk_Handle_InputButtonMap& InButtonMap,
        const FCk_Request_InputButtonMap_Rederive& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /**
     * Mints a tier-2 button for a raw key. Deferred. Registering a key this map already carries — whether from
     * a declaration or an earlier request — completes Succeeded, because a physical button's association is
     * the key itself and the caller's intent already holds. An invalid key is rejected before enqueue and
     * completes Failed_NotEnqueued.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputButtonMap",
              DisplayName = "[Ck][InputButtonMap] Request Register Physical Button",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_InputButtonMap
    Request_RegisterPhysicalButton(
        UPARAM(ref) FCk_Handle_InputButtonMap& InButtonMap,
        const FCk_Request_InputButtonMap_RegisterPhysicalButton& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

private:
    // Composition is atomic: one rejected declaration invalidates the whole set, so this runs every rejection
    // before anything is composed.
    static auto
    DoGet_PhysicalButtonsAreValid(
        const FCk_Handle& InContext,
        const TArray<FKey>& InPhysicalButtons) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
