#pragma once
#include "CkEnsure/CkEnsure.h"

#include "CkMacros/CkMacros.h"

#include "CkObject_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Object_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Object_UE);

public:
    template <typename T>
    static auto
    Request_CloneObject(UObject* Outer,
        const T* InObjectToClone) -> T*;

    template <typename T, typename T_Func>
    static auto
    Request_CloneObject(UObject* Outer,
        const T* InObjectToClone,
        T_Func InPostClone) -> T*;

    template <typename T>
    static auto
    Request_CloneObject(const T* InObjectToClone, ck::policy::TransientPackage) -> T*;

    template <typename T, typename T_Func>
    static auto
    Request_CloneObject(const T* InObjectToClone,
        T_Func InPostClone, ck::policy::TransientPackage) -> T*;

};

// --------------------------------------------------------------------------------------------------------------------

template <typename T>
auto UCk_Utils_Object_UE::Request_CloneObject(UObject* Outer, const T* InObjectToClone) -> T*
{
    static_assert(NOT std::is_base_of_v<AActor, T>, "Request_CloneObject cannot be used on an AActor. Use Request_CloneActor instead");

    CK_ENSURE_IF_NOT(ck::IsValid(InObjectToClone), TEXT("Unable to CloneObject with Outer [{}]. Object to clone is INVALID."), Outer)
    { return {}; }

    CK_ENSURE_IF_NOT(NOT InObjectToClone->template IsA<const AActor>(),
        TEXT("Unable to clone [{}] with Outer [{}]. Request_CloneObject cannot be used on an AActor. Use Request_CloneActor instead."),
        InObjectToClone, Outer)
    { return {}; }

    auto NonConstObjectToClone = const_cast<T*>(InObjectToClone);
    const auto& Class = NonConstObjectToClone->GetClass();

    return NewObject<T>(Outer, Class, NAME_None, RF_NoFlags, NonConstObjectToClone);
}

template <typename T, typename T_Func>
auto UCk_Utils_Object_UE::Request_CloneObject(UObject* Outer, const T* InObjectToClone,
    T_Func InPostClone) -> T*
{
    auto NewObj = Request_CloneObject(Outer, InObjectToClone, [](T*){});
    InPostClone(NewObj);

    return NewObj;
}

template <typename T>
auto UCk_Utils_Object_UE::Request_CloneObject(const T* InObjectToClone) -> T*
{
    return Request_CloneObject(GetTransientPackage(), InObjectToClone);
}

template <typename T, typename T_Func>
auto UCk_Utils_Object_UE::Request_CloneObject(const T* InObjectToClone, T_Func InPostClone) -> T*
{
    return Request_CloneObject(GetTransientPackage(), InObjectToClone, InPostClone);
}

// --------------------------------------------------------------------------------------------------------------------
