#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "GameFramework/SaveGame.h"

#include "UObject/SoftObjectPath.h"

#include "CkSnapshot_SlotMeta.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UTexture2D;
class UWorld;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Everything a save/load MENU needs to draw one slot row, with no world payload attached.
 *
 * It lives in a SIDECAR save slot (UCk_Snapshot_SlotMetaSaveGame) rather than inside
 * UCk_Snapshot_SaveGame because reading a snapshot's own header costs a full LoadGameFromSlot,
 * which deserializes the entire world blob — a nine-slot menu would deserialize nine worlds to
 * draw nine rows.
 *
 * The sidecar is written by the same Request_Save that wrote the snapshot, so it is only ever as
 * stale as its slot. A slot whose snapshot exists but whose sidecar does not (a save written
 * before this existed, or a half-deleted slot) reads back DEFAULT-constructed — check
 * Get_TimestampUTC against FDateTime{} rather than assuming a sidecar is present.
 */
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_SlotMeta
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_SlotMeta);

private:
    UPROPERTY()
    FName _SlotName;

    // Player-authored slot name. Empty until the game supplies one — a menu falls back to its own
    // default copy rather than showing the raw slot name.
    UPROPERTY()
    FText _Title;

    UPROPERTY()
    FDateTime _TimestampUTC = FDateTime{};

    // The world the snapshot was captured in, i.e. the map a load of THIS slot must travel to.
    UPROPERTY()
    FSoftObjectPath _WorldAssetPath;

    // PNG bytes of the capture-time thumbnail. Empty when the save requested no screenshot, or ran
    // with no game viewport (headless / -nullrhi / dedicated server).
    UPROPERTY()
    TArray<uint8> _ScreenshotPng;

    // Game-defined summary fields, opaque to CkSnapshot — round-tripped so the game can format its
    // own row description (e.g. level / xp / currency / play time).
    UPROPERTY()
    TMap<FName, FString> _CustomFields;

public:
    CK_PROPERTY(_SlotName);
    CK_PROPERTY(_Title);
    CK_PROPERTY(_TimestampUTC);
    CK_PROPERTY(_WorldAssetPath);
    CK_PROPERTY(_ScreenshotPng);
    CK_PROPERTY(_CustomFields);

public:
    /** True when this came off disk rather than being the default returned for a missing sidecar. */
    auto Get_IsPopulated() const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

/** The sidecar slot's payload. One per snapshot slot; see FCk_Snapshot_SlotMeta for why it is split out. */
UCLASS()
class CKSNAPSHOT_API UCk_Snapshot_SlotMetaSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Snapshot_SlotMetaSaveGame);

public:
    UPROPERTY()
    FCk_Snapshot_SlotMeta _Meta;
};

// --------------------------------------------------------------------------------------------------------------------

/** What Request_Save_WithMetadata records alongside the snapshot. */
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_SaveMetadata
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_SaveMetadata);

private:
    UPROPERTY()
    FText _Title;

    UPROPERTY()
    TMap<FName, FString> _CustomFields;

    /**
     * PNG thumbnail for the slot, stored verbatim. Empty simply means the slot has no picture.
     *
     * The save does NOT capture one for you, deliberately: a capture is frame-deferred (see
     * Request_CaptureViewportPng), so folding it into the synchronous save would make the whole save
     * span frames for a thumbnail — and capturing at save time photographs the menu the player saved
     * from. Request it with UCk_Utils_Snapshot_UE::Request_CaptureViewportThumbnail when the menu
     * OPENS, then hand the bytes here. It carries its own downscale width.
     */
    UPROPERTY()
    TArray<uint8> _ScreenshotPng;

public:
    CK_PROPERTY(_Title);
    CK_PROPERTY(_CustomFields);
    CK_PROPERTY(_ScreenshotPng);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot::slot_meta
{
    /**
     * Sidecar slots share the snapshot slot's namespace with this suffix appended, so the platform
     * save system enumerates BOTH. Every enumeration must therefore filter with Get_IsMetaSlotName.
     */
    CKSNAPSHOT_API extern const FString MetaSlotSuffix;

    CKSNAPSHOT_API auto Get_MetaSlotName(FName InSlotName) -> FString;
    CKSNAPSHOT_API auto Get_IsMetaSlotName(const FString& InSlotName) -> bool;

    /**
     * PNG thumbnail of the game viewport, delivered on the NEXT rendered frame. The ONLY capture in
     * this module — a synchronous one cannot exist correctly.
     *
     * Why: reading the viewport from the game thread only works while Slate composites it into its own
     * buffer (the editor). A packaged game builds its viewport widget with RenderDirectlyToWindow, so
     * FSceneViewport holds the backbuffer only between BeginRenderFrame and EndRenderFrame on the
     * RENDER thread, and its game-thread texture ref is permanently null. Reading it anyway does not
     * fail — D3D12's RHIReadSurfaceData memzeroes the output for a null texture — so the caller gets a
     * fully black image that reports success. This routes through the engine's own screenshot pipeline
     * instead, whose readback runs INSIDE the render frame. It also reads before Slate composites UMG,
     * so the result is the gameplay frame even when the request is issued from a menu.
     *
     * InOnCaptured always runs exactly once: with the PNG bytes, or with an empty array when there is
     * no viewport (headless / -nullrhi / dedicated server) or nothing arrived within the timeout.
     *
     * The engine's screenshot request is a process-wide singleton, so a capture in flight will also
     * consume a user-initiated screenshot taken on the same frame.
     */
    CKSNAPSHOT_API auto Request_CaptureViewportPng(
        const UWorld* InWorld,
        int32 InMaxWidth,
        TFunction<void(TArray<uint8>)> InOnCaptured) -> void;

    /**
     * Decode PNG bytes into a transient UTexture2D, or nullptr when empty/undecodable. The texture
     * is NOT rooted — the caller must hold it in a UPROPERTY or it is collected on the next GC.
     */
    CKSNAPSHOT_API auto Decode_PngAsTexture(const TArray<uint8>& InPng) -> UTexture2D*;
}

// --------------------------------------------------------------------------------------------------------------------
