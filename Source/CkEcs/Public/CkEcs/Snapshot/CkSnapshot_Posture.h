#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot_Posture.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Declared snapshot posture, no default. Every data-bearing fragment type answers exactly one question: is its
// content DURABLE (part of the saved world) or SESSION (rebuilt by the feature's normal setup)? The two markers
// below are the declaration; ck::Get_FragmentPosture is the single authority that reads them, plus the derived
// rules that PROVE a posture for shapes where the alternative is impossible (a tag, a delegate carrier, a request
// queue). Anything else resolves Undeclared — never silently Durable, never silently Session.
//
// The markers live in CkEcs, not CkDynamic, because posture is an ECS-level concept consumed by both the
// dynamic-fragment path and the registered-handler path.
// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Snapshot_Posture : uint8
{
    // No declaration and no derivation. Captured and hydrated field-wise, i.e. the pre-posture behaviour: the
    // transitional path the posture ratchet's allow-list enumerates, impossible once that list drains to zero.
    Undeclared,
    // Captured whole at save, hydrated whole at load.
    Durable,
    // Never captured, never hydrated: the feature's construction/setup rebuilds it.
    Session
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Snapshot_Posture);

// --------------------------------------------------------------------------------------------------------------------

// Declares the fragment DURABLE: derive from it (C++) or carry a field of this type (AngelScript, whose structs
// cannot inherit). BlueprintType is load-bearing — the AngelScript binder only auto-binds BlueprintType structs,
// which is what makes the script-side marker-field spelling possible.
USTRUCT(BlueprintType)
struct CKECS_API FCk_Snapshot_Durable
{
    GENERATED_BODY()
};

// Declares the fragment SESSION-scoped. Symmetric with FCk_Snapshot_Durable in every respect, deliberately:
// neither posture is the special case.
USTRUCT(BlueprintType)
struct CKECS_API FCk_Snapshot_Session
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------

class UScriptStruct;

namespace ck
{
    // Why a type resolved the way it did. The posture ratchet prints this verbatim for every discovered type, so
    // no classification is silent; Reason is never empty.
    struct CKECS_API FCk_Snapshot_PostureResolution
    {
        ECk_Snapshot_Posture Posture = ECk_Snapshot_Posture::Undeclared;

        FString Reason;

        // True when the type matches a derived-Session shape (delegate carrier / request queue) yet ALSO carries
        // durable value fields, or declares Durable alongside a field-level opt-out. The posture stays Undeclared
        // so nothing is dropped, and the ratchet reports "split the fragment".
        bool RequiresSplit = false;
    };

    // Resolution order (fragment-granular, no field-level mixing):
    //   1. both markers            -> ensure, Undeclared            (a contradiction)
    //   2. Session marker          -> Session
    //   3. Durable marker          -> Durable, unless a derived-Session shape or a UPROPERTY(Transient) game
    //                                 field contradicts it -> ensure, Undeclared ("split the fragment")
    //   4. derived shapes: a field-less tag / a type reaching a delegate at any depth / an AngelScript
    //      "...Requests" queue or a type reaching an FCk_Request_Base
    //          pure  -> Session
    //          mixed with value fields -> Undeclared + RequiresSplit (a derivation may never drop durable data)
    //   5. otherwise               -> Undeclared
    //
    // Memoised per UScriptStruct. Resolutions that ENSURED are deliberately not cached, so an author error stays
    // loud on every subsequent resolution instead of going quiet after the first.
    CKECS_API auto
    Resolve_FragmentPosture(
        const UScriptStruct* InStructType) -> FCk_Snapshot_PostureResolution;

    // Posture only — the form capture/hydration and the fences call.
    CKECS_API auto
    Get_FragmentPosture(
        const UScriptStruct* InStructType) -> ECk_Snapshot_Posture;
}

// --------------------------------------------------------------------------------------------------------------------
