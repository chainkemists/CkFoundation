#include "CkDependencyProvider_GameInstance_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    Register(
        const FCk_Request_DependencyProvider_Register& InRequest)
    -> void
{
    // See sibling World subsystem's Register for rationale on the FInstancedStruct
    // unboxing pattern — GetPtr<FCk_Handle> covers the typesafe-subclass case.
    const auto& Boxed = InRequest.Get_ProvidedHandle();

    // See sibling World subsystem for rationale on the const_cast — UScriptStruct
    // CDOs are engine-lifetime statics and the registry maps key on the
    // non-const pointer.
    auto* HandleType = const_cast<UScriptStruct*>(Boxed.GetScriptStruct());
    CK_ENSURE_IF_NOT(ck::IsValid(HandleType),
        TEXT("DependencyProvider Register called with empty FInstancedStruct (no UScriptStruct)"))
    { return; }

    const auto* ProvidedPtr = Boxed.GetPtr<FCk_Handle>();
    CK_ENSURE_IF_NOT(ProvidedPtr != nullptr,
        TEXT("DependencyProvider Register called with type [{}] that is not an FCk_Handle subclass"),
        HandleType)
    { return; }

    const auto& Provided = *ProvidedPtr;
    CK_ENSURE_IF_NOT(ck::IsValid(Provided),
        TEXT("DependencyProvider Register called with invalid handle for type [{}]"), HandleType)
    { return; }

    if (_Providers.Contains(HandleType))
    {
        switch (InRequest.Get_OverwritePolicy())
        {
            case ECk_DependencyProvider_OverwritePolicy::EnsureOnDuplicate:
            {
                CK_ENSURE_IF_NOT(false,
                    TEXT("DependencyProvider (GameInstance) duplicate registration for type [{}]. "
                         "Existing provider remains. Use Overwrite policy if intentional."),
                    HandleType) {}
                return;
            }
            case ECk_DependencyProvider_OverwritePolicy::Overwrite:
            {
                break;
            }
        }
    }

    _Providers.Add(HandleType, Provided);
    DoFirePendingFor(HandleType, Provided);
}

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    Unregister(
        UScriptStruct* InHandleType)
    -> void
{
    if (ck::Is_NOT_Valid(InHandleType))
    { return; }

    _Providers.Remove(InHandleType);
    _Factories.Remove(InHandleType);
}

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    Resolve(
        UScriptStruct* InHandleType) const
    -> FCk_Handle
{
    if (ck::Is_NOT_Valid(InHandleType))
    { return {}; }

    if (const auto* Found = _Providers.Find(InHandleType))
    { return *Found; }

    if (const auto* Factory = _Factories.Find(InHandleType))
    { return (*Factory)(FCk_Handle{}); }

    return {};
}

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    RegisterFactory(
        UScriptStruct* InHandleType,
        FFactory InFactory)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandleType),
        TEXT("RegisterFactory called with null HandleType"))
    { return; }

    CK_ENSURE_IF_NOT(static_cast<bool>(InFactory),
        TEXT("RegisterFactory called with null callable for type [{}]"), InHandleType)
    { return; }

    _Factories.Add(InHandleType, MoveTemp(InFactory));
}

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    RegisterPending(
        UScriptStruct* InHandleType,
        FPendingResolution InPending)
    -> void
{
    if (ck::Is_NOT_Valid(InHandleType))
    { return; }

    _PendingByType.FindOrAdd(InHandleType).Add(MoveTemp(InPending));
}

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    UnregisterPending(
        UCk_EntityScript_UE* InScript)
    -> void
{
    if (InScript == nullptr)
    { return; }

    for (auto& Pair : _PendingByType)
    {
        Pair.Value.RemoveAll([InScript](const FPendingResolution& InEntry)
        {
            return InEntry._Script.Get() == InScript;
        });
    }
}

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    Get_RegisteredTypes() const
    -> TArray<UScriptStruct*>
{
    auto Out = TArray<UScriptStruct*>{};
    Out.Reserve(_Providers.Num());
    for (const auto& Pair : _Providers)
    { Out.Add(Pair.Key); }
    return Out;
}

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    Get_PendingCount_ForType(
        UScriptStruct* InHandleType) const
    -> int32
{
    if (const auto* Bucket = _PendingByType.Find(InHandleType))
    { return Bucket->Num(); }

    return 0;
}

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    Deinitialize()
    -> void
{
    _Providers.Empty();
    _Factories.Empty();
    _PendingByType.Empty();
    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DependencyProvider_GameInstance_Subsystem_UE::
    DoFirePendingFor(
        UScriptStruct* InHandleType,
        const FCk_Handle& InResolved)
    -> void
{
    auto Bucket = TArray<FPendingResolution>{};
    {
        if (auto* Existing = _PendingByType.Find(InHandleType))
        {
            Bucket = MoveTemp(*Existing);
            _PendingByType.Remove(InHandleType);
        }
    }

    for (const auto& Entry : Bucket)
    {
        if (ck::Is_NOT_Valid(Entry._Script))
        { continue; }

        if (ck::Is_NOT_Valid(Entry._Entity))
        { continue; }

        if (NOT static_cast<bool>(Entry._OnResolved))
        { continue; }

        Entry._OnResolved(InResolved);
    }
}
