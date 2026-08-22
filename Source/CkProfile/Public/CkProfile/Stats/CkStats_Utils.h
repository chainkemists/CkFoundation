#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkStats_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Stats_MemoryPressureStatus : uint8
{
    Nominal,
    Warning,
    Critical,
    Unknown,
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Stats_NetworkConnectionType : uint8
{
    NotConnected   UMETA(DisplayName = "None"),
    AirplaneMode   UMETA(DisplayName = "Airplane Mode"),
    Cell           UMETA(DisplayName = "Cell"),
    WiFi           UMETA(DisplayName = "WiFi"),
    WiMAX          UMETA(DisplayName = "WiMAX"),
    Bluetooth      UMETA(DisplayName = "Bluetooth"),
    Ethernet       UMETA(DisplayName = "Ethernet"),
    Unknown        UMETA(DisplayName = "Unknown"),
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Whether a timing metric carries a real measurement, and if not, why not.
 * A metric that could not be measured reports its reason rather than a zero: a GPU time of zero
 * would read as an infinitely fast GPU and would corrupt anything derived from it.
 */
UENUM(BlueprintType)
enum class ECk_Stats_MetricAvailability : uint8
{
    Available,
    Unavailable_NullRhi,
    Unavailable_NoGpuTimestamps,
    Unavailable_NotYetSampled,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Stats_MetricAvailability);

// --------------------------------------------------------------------------------------------------------------------

/**
 * A coherent single-frame snapshot of the timings `stat unit` reports.
 * Every value is read from the same frame, so the numbers can be compared against one another.
 * GPU time is meaningful ONLY when Get_GpuAvailability() == Available.
 */
USTRUCT(BlueprintType)
struct CKPROFILE_API FCk_Stats_ThreadTimings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Stats_ThreadTimings);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _FrameTimeMs = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _GameThreadTimeMs = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _RenderThreadTimeMs = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _RhiThreadTimeMs = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _GpuTimeMs = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Stats_MetricAvailability _GpuAvailability = ECk_Stats_MetricAvailability::Unavailable_NotYetSampled;

public:
    CK_PROPERTY_GET(_FrameTimeMs);
    CK_PROPERTY_GET(_GameThreadTimeMs);
    CK_PROPERTY_GET(_RenderThreadTimeMs);
    CK_PROPERTY_GET(_RhiThreadTimeMs);
    CK_PROPERTY_GET(_GpuTimeMs);
    CK_PROPERTY_GET(_GpuAvailability);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Stats_ThreadTimings,
        _FrameTimeMs,
        _GameThreadTimeMs,
        _RenderThreadTimeMs,
        _RhiThreadTimeMs,
        _GpuTimeMs,
        _GpuAvailability);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Stats_ThreadTimings, [](const FCk_Stats_ThreadTimings& InObj)
{
    return ck::Format(TEXT("Frame: [{}ms], Game: [{}ms], Render: [{}ms], RHI: [{}ms], GPU: [{}ms / {}]"),
        InObj.Get_FrameTimeMs(),
        InObj.Get_GameThreadTimeMs(),
        InObj.Get_RenderThreadTimeMs(),
        InObj.Get_RhiThreadTimeMs(),
        InObj.Get_GpuTimeMs(),
        InObj.Get_GpuAvailability());
});

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROFILE_API UCk_Utils_Stats_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Stats_UE);

    // ----------------------------------------------------------------------------------------------------------------

public:
    /** Returns the engine-smoothed average FPS (GAverageFPS). */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get FPS",
              Category = "Ck|Utils|Profile|Stats")
    static float
    Get_FPS();

    /** Returns the current frame's delta time in milliseconds. */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Frame Time (ms)",
              Category = "Ck|Utils|Profile|Stats")
    static float
    Get_FrameTimeMs();

    /** Returns the global engine frame counter. */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Frame Count",
              Category = "Ck|Utils|Profile|Stats")
    static int64
    Get_FrameCount();

    /**
     * Returns the frame, game-thread, render-thread, RHI-thread and GPU timings for the frame just
     * completed, read from the same engine counters `stat unit` uses.
     * These are the RAW values; `stat unit` additionally displays an exponentially smoothed variant,
     * which is deliberately not reproduced here because smoothing destroys percentile and outlier
     * statistics computed over a sample window.
     */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Thread Timings",
              Category = "Ck|Utils|Profile|Stats")
    static FCk_Stats_ThreadTimings
    Get_ThreadTimings();

    // ----------------------------------------------------------------------------------------------------------------

public:
    /** Returns the local player's ping in milliseconds. Returns -1 if unavailable. */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Ping (ms)",
              Category = "Ck|Utils|Profile|Stats",
              meta = (DefaultToSelf = "InContext"))
    static float
    Get_PingMs(
        const UObject* InContext = nullptr);

    /** Returns the server's FPS (replicated via GameState). Returns -1 if unavailable. */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Server FPS",
              Category = "Ck|Utils|Profile|Stats",
              meta = (DefaultToSelf = "InContext"))
    static float
    Get_ServerFPS(
        const UObject* InContext = nullptr);

    /** Returns the current network connection type (WiFi, Ethernet, Cell, etc.). */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Network Connection Type",
              Category = "Ck|Utils|Profile|Stats")
    static ECk_Stats_NetworkConnectionType
    Get_NetworkConnectionType();

    // ----------------------------------------------------------------------------------------------------------------

public:
    /** Returns physical RAM currently in use, in GB. */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get RAM Used (GB)",
              Category = "Ck|Utils|Profile|Stats")
    static float
    Get_RAM_UsedGB();

    /** Returns physical RAM available, in GB. */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get RAM Available (GB)",
              Category = "Ck|Utils|Profile|Stats")
    static float
    Get_RAM_AvailableGB();

    /** Returns total physical RAM, in GB. */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get RAM Total (GB)",
              Category = "Ck|Utils|Profile|Stats")
    static float
    Get_RAM_TotalGB();

    /**
     * Returns GPU/RHI memory currently in use, in GB.
     * Dev-only: requires the UE stats system (STATS macro). Returns 0 in Shipping.
     */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get VRAM Used (GB) [DEV]",
              Category = "Ck|Utils|Profile|Stats",
              meta = (DevelopmentOnly))
    static float
    Get_VRAM_UsedGB();

    /** Returns the OS-level memory pressure status (Nominal, Warning, Critical). */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Memory Pressure Status",
              Category = "Ck|Utils|Profile|Stats")
    static ECk_Stats_MemoryPressureStatus
    Get_MemoryPressureStatus();

    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get CPU Brand",
              Category = "Ck|Utils|Profile|Stats")
    static FString
    Get_CPUBrand();

    /** Returns the number of physical CPU cores. */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get CPU Core Count",
              Category = "Ck|Utils|Profile|Stats")
    static int32
    Get_CPUCoreCount();

    /** Returns the number of logical CPU threads (including hyper-threads). */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get CPU Thread Count",
              Category = "Ck|Utils|Profile|Stats")
    static int32
    Get_CPUThreadCount();

    /** Returns the primary GPU brand/adapter name (e.g. "NVIDIA GeForce RTX 4080"). */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get GPU Brand",
              Category = "Ck|Utils|Profile|Stats")
    static FString
    Get_GPUBrand();

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get OS Version",
              Category = "Ck|Utils|Profile|Stats")
    static FString
    Get_OSVersion();

    /** Returns "Shipping", "Test", or "Development". */
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Build Config",
              Category = "Ck|Utils|Profile|Stats")
    static FString
    Get_BuildConfig();

    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Stat Enabled",
              Category = "Ck|Utils|Profile|Stats")
    static bool
    Get_IsStatEnabled(
        const FString& InStatName);
};

// --------------------------------------------------------------------------------------------------------------------
