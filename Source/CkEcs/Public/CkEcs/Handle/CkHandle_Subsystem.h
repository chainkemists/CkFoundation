#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Entity/CkEntity.h"

#include "CkEcs/Handle/CkHandle_Debugging_Data.h"

#include "Subsystems/EngineSubsystem.h"

#include "CkHandle_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_FragmentsDebug_SharingInfo
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_FragmentsDebug_SharingInfo);

public:
    UPROPERTY(Transient)
    TObjectPtr<UCk_Handle_FragmentsDebug> _FragmentsDebug;

    UPROPERTY(Transient)
    int32 _ShareCount = 0;

public:
    CK_PROPERTY(_FragmentsDebug);
    CK_PROPERTY(_ShareCount);

    CK_DEFINE_CONSTRUCTORS(FCk_FragmentsDebug_SharingInfo, _FragmentsDebug, _ShareCount);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECS_API UCk_HandleDebugger_Subsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_HandleDebugger_Subsystem_UE);

public:
    auto
    GetOrAdd_FragmentsDebug(const FCk_Handle& InHandle) -> UCk_Handle_FragmentsDebug*;

    auto
    Remove_FragmentsDebug(const FCk_Handle& InHandle) -> void;

private:
    UPROPERTY(Transient)
    TMap<FCk_Entity, FCk_FragmentsDebug_SharingInfo> _EntityToDebug;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_HandleDebugger_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_HandleDebugger_Subsystem_UE;

public:
    static auto
    GetOrAdd_FragmentsDebug(
        const FCk_Handle& InHandle) -> UCk_Handle_FragmentsDebug*;

    static auto
    Remove_FragmentsDebug(
        const FCk_Handle& InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
