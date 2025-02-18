#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCore/Macros/CkMacros.h"

#include <Subsystems/EngineSubsystem.h>

#include "CkMemory_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMEMORY_API FCk_Utils_Memory_MemoryCountSnapshot_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_Memory_MemoryCountSnapshot_Result);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _PhysicalMemoryUsed      = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _PhysicalMemoryAvailable = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _PhysicalMemoryTotal     = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _VirtualMemoryUsed       = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _VirtualMemoryAvailable  = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _VirtualMemoryTotal      = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _RHIMemoryUsed           = 0.0f;

public:
    CK_PROPERTY(_PhysicalMemoryUsed);
    CK_PROPERTY(_PhysicalMemoryAvailable);
    CK_PROPERTY(_PhysicalMemoryTotal);
    CK_PROPERTY(_VirtualMemoryUsed);
    CK_PROPERTY(_VirtualMemoryAvailable);
    CK_PROPERTY(_VirtualMemoryTotal);
    CK_PROPERTY(_RHIMemoryUsed);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_Stats")
class CKMEMORY_API UCk_Stats_Subsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    auto
        Initialize(
            FSubsystemCollectionBase& Collection)
            -> void override;
    auto
        Deinitialize()
            -> void override;
    static auto
        Get_MemoryCountSnapshot(
            const UObject* InContext) -> FCk_Utils_Memory_MemoryCountSnapshot_Result;

private:
    static TUniquePtr<class FMemoryStatsUpdateTask> _MemoryUpdateTask;
    static FRunnableThread* _MemoryUpdateThread;
};

// --------------------------------------------------------------------------------------------------------------------
