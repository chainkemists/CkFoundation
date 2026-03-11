#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

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

UCLASS(NotBlueprintable)
class CKPROFILE_API UCk_Utils_Stats_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Stats_UE);

    // ================================================================================================================
    // Performance
    // ================================================================================================================

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

    // ================================================================================================================
    // Network
    // ================================================================================================================

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

    // ================================================================================================================
    // Memory
    // ================================================================================================================

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

    // ================================================================================================================
    // System Info
    // ================================================================================================================

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

    // ================================================================================================================
    // UE Stat System
    // ================================================================================================================

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Stat Enabled",
              Category = "Ck|Utils|Profile|Stats")
    static bool
    Get_IsStatEnabled(
        const FString& InStatName);
};

// --------------------------------------------------------------------------------------------------------------------
