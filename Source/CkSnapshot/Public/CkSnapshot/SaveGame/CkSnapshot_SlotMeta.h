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
    // own row description (BusterBlock: store level / xp / money / play time).
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
     * Pre-captured PNG thumbnail. When non-empty it is stored verbatim and NO capture is taken.
     *
     * This exists because the fallback capture below reads the back buffer, menus and all — and a
     * game almost always saves FROM a menu, so the automatic thumbnail is a picture of the save
     * screen. The fix needs no async machinery: call Capture_ViewportPng at the moment the menu is
     * opened, BEFORE the widget is pushed, and hand the bytes here.
     */
    UPROPERTY()
    TArray<uint8> _ScreenshotPng;

    // Fallback when _ScreenshotPng is empty: capture the game viewport during the save. Silently
    // yields no screenshot when there is no viewport (headless / dedicated server) — never an error.
    UPROPERTY()
    bool _CaptureScreenshot = true;

    // Thumbnail is downscaled to this width, aspect preserved. The menu shows it at ~240px; the
    // default doubles that so it survives a high-DPI slot row without bloating the sidecar.
    UPROPERTY()
    int32 _ScreenshotMaxWidth = 480;

public:
    CK_PROPERTY(_Title);
    CK_PROPERTY(_CustomFields);
    CK_PROPERTY(_ScreenshotPng);
    CK_PROPERTY(_CaptureScreenshot);
    CK_PROPERTY(_ScreenshotMaxWidth);
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
     * PNG thumbnail of InWorld's game viewport, downscaled to InMaxWidth. Empty (NOT an error) when
     * the world has no viewport — headless automation, -nullrhi and dedicated servers all take that
     * path, which is why saving never depends on the result.
     *
     * Synchronous: it flushes the render thread via FViewport::ReadPixels. That is deliberate —
     * Request_Save is synchronous and already pumps the world to quiescence, so folding in an async
     * screenshot request would make the whole save frame-spanning for a thumbnail.
     */
    CKSNAPSHOT_API auto Capture_ViewportPng(const UWorld* InWorld, int32 InMaxWidth) -> TArray<uint8>;

    /**
     * Decode PNG bytes into a transient UTexture2D, or nullptr when empty/undecodable. The texture
     * is NOT rooted — the caller must hold it in a UPROPERTY or it is collected on the next GC.
     */
    CKSNAPSHOT_API auto Decode_PngAsTexture(const TArray<uint8>& InPng) -> UTexture2D*;
}

// --------------------------------------------------------------------------------------------------------------------
