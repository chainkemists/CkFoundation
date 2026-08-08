#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkInput/CkInputSource_Fragment.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkInputSource_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * The raw input surface: an input-source entity is one local player's inbox of device events, plus the
 * explicit list of devices that player owns.
 *
 * Producers (the Slate writer, tests, tools) append events through Request_InjectRawEvent; a later router
 * drains the inbox. Nothing infers who a device belongs to — a device reaches a source only because
 * Request_AssignDevice put it there.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_InputSource"))
class CKINPUT_API UCk_Utils_InputSource_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_InputSource_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_InputSource);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Add Feature")
    static FCk_Handle_InputSource
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_InputSource_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Create")
    static FCk_Handle_InputSource
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_InputSource_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_InputSource
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Handle -> InputSource Handle",
              meta = (CompactNodeTitle = "<AsInputSource>", BlueprintAutocast))
    static FCk_Handle_InputSource
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid InputSource Handle",
              Category = "Ck|Utils|InputSource",
              meta = (CompactNodeTitle = "INVALID_InputSourceHandle", Keywords = "make"))
    static FCk_Handle_InputSource
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Get Local Player Index")
    static int32
    Get_LocalPlayerIndex(
        const FCk_Handle_InputSource& InInputSource);

    /** The inbox contents, oldest first. Rows survive until a router drains them. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Get Pending Raw Events")
    static TArray<FCk_InputSource_RawEvent>
    Get_PendingRawEvents(
        const FCk_Handle_InputSource& InInputSource);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Get Num Pending Raw Events")
    static int32
    Get_NumPendingRawEvents(
        const FCk_Handle_InputSource& InInputSource);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Get Owned Devices")
    static TArray<FCk_InputSource_DeviceId>
    Get_OwnedDevices(
        const FCk_Handle_InputSource& InInputSource);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Get Owns Device")
    static bool
    Get_OwnsDevice(
        const FCk_Handle_InputSource& InInputSource,
        const FCk_InputSource_DeviceId& InDeviceId);

    /**
     * Which source a device belongs to, searched across every input source in InAnyEntityInWorld's world.
     * Returns an invalid handle when nothing has claimed the device — an unclaimed device is a normal state,
     * not an error, and its events belong to nobody until something assigns it.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Try Get Source Owning Device")
    static FCk_Handle_InputSource
    TryGet_SourceOwningDevice(
        FCk_Handle InAnyEntityInWorld,
        const FCk_InputSource_DeviceId& InDeviceId);

public:
    /**
     * Appends one raw device event to the source's inbox. This is the only way in: the Slate writer, the
     * synthetic test writer and tooling all go through it, so the inbox has one shape regardless of producer.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Request Inject Raw Event",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_InputSource
    Request_InjectRawEvent(
        UPARAM(ref) FCk_Handle_InputSource& InInputSource,
        const FCk_Request_InputSource_InjectRawEvent& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /**
     * Gives this source exclusive ownership of a device. Completes Failed when another source already owns
     * the device, and Succeeded when this source already does.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Request Assign Device",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_InputSource
    Request_AssignDevice(
        UPARAM(ref) FCk_Handle_InputSource& InInputSource,
        const FCk_Request_InputSource_AssignDevice& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
