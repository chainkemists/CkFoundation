#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkRenderTarget_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Render Target"))
class CKRENDERTARGET_API UCk_RenderTarget_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_RenderTarget_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transport",
        meta = (AllowPrivateAccess = true, ClampMin = "1024", ClampMax = "61440",
            ToolTip = "Bytes per pixel-stream chunk. Must stay safely under the 64KB max-bunch ceiling."))
    int32 _ChunkSizeBytes = 32768;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Transport",
        meta = (AllowPrivateAccess = true, ClampMin = "1024",
            ToolTip = "Per-tick byte budget for one (player, sync) pixel stream. Bounds the per-tick reliable-bunch pressure."))
    int32 _MaxBytesPerStreamPerTick = 65536;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Pixels",
        meta = (AllowPrivateAccess = true, ClampMin = "4", ClampMax = "256",
            ToolTip = "Block size (in pixels) of the delta diff grid. Must match across all machines."))
    int32 _BlockSize = 16;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Pixels",
        meta = (AllowPrivateAccess = true, ClampMin = "16", ClampMax = "4096",
            ToolTip = "Per-axis cap for CreateManaged targets (and upload validation). 1024^2 RGBA8 = 4MB raw, ~800KB compressed FullSync — the upper edge of acceptable join-time burst."))
    int32 _MaxManagedSize = 1024;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Client Authoring",
        meta = (AllowPrivateAccess = true, ClampMin = "0",
            ToolTip = "Per-player budget for accepted upload bytes per second (0 = unlimited). Uploads beyond the budget are dropped with a warning."))
    int32 _ClientUploadMaxBytesPerSecond = 262144;

public:
    CK_PROPERTY_GET(_ChunkSizeBytes);
    CK_PROPERTY_GET(_MaxBytesPerStreamPerTick);
    CK_PROPERTY_GET(_BlockSize);
    CK_PROPERTY_GET(_MaxManagedSize);
    CK_PROPERTY_GET(_ClientUploadMaxBytesPerSecond);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRENDERTARGET_API UCk_Utils_RenderTarget_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RenderTarget_Settings_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget|Settings")
    static int32
    Get_ChunkSizeBytes();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget|Settings")
    static int32
    Get_MaxBytesPerStreamPerTick();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget|Settings")
    static int32
    Get_BlockSize();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget|Settings")
    static int32
    Get_MaxManagedSize();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget|Settings")
    static int32
    Get_ClientUploadMaxBytesPerSecond();
};

// --------------------------------------------------------------------------------------------------------------------
