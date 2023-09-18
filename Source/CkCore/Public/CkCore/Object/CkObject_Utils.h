#pragma once

#include "CkCore/Ensure/CkEnsure.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkObject_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Utils_Object_CopyAllProperties_Result : uint8
{
    Failed   ,
    Succeeded,
    AllPropertiesIdentical
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Utils_Object_CopyAllProperties_Result);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Utils_Object_CopyAllProperties_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_Object_CopyAllProperties_Params);

public:
    FCk_Utils_Object_CopyAllProperties_Params() = default;
    FCk_Utils_Object_CopyAllProperties_Params(
        UObject* InDestination,
        UObject* InSource);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Destination;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Source;

public:
    CK_PROPERTY_GET(_Destination);
    CK_PROPERTY_GET(_Source);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Object_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Object_UE);

public:
    template <typename T_Object>
    static auto
    Get_ClassDefaultObject ()
    -> T_Object*;

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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Object")
    static ECk_Utils_Object_CopyAllProperties_Result
    Request_CopyAllProperties(
        const FCk_Utils_Object_CopyAllProperties_Params& InParams);

    // Unreal prefixes some classes with REINST_. This is because REINST_ is a newer version of a static class.
    // This function gets the most up to date default class.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Object")
    static UClass*
    Get_DefaultClass_UpToDate(
        UClass* InClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Object")
    static bool
    Get_IsDefaultObject(
        UObject* InObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Object",
              meta     = (DeterminesOutputType = "InObject"))
    static UObject*
    Get_ClassDefaultObject(
        TSubclassOf<UObject> InObject);

};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Object>
auto
    UCk_Utils_Object_UE::
    Get_ClassDefaultObject()
    -> T_Object*
{
    return Cast<T_Object>(Get_ClassDefaultObject(T_Object::StaticClass()));
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CloneObject(
        UObject* Outer,
        const T* InObjectToClone)
    -> T*
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
auto
    UCk_Utils_Object_UE::
    Request_CloneObject(
        UObject* Outer,
        const T* InObjectToClone,
        T_Func InPostClone)
    -> T*
{
    auto NewObj = Request_CloneObject(Outer, InObjectToClone, [](T*){});
    InPostClone(NewObj);

    return NewObj;
}

template <typename T>
auto
    UCk_Utils_Object_UE::
    Request_CloneObject(
        const T* InObjectToClone,
        ck::policy::TransientPackage)
    -> T*
{
    return Request_CloneObject(GetTransientPackage(), InObjectToClone);
}

template <typename T, typename T_Func>
auto
    UCk_Utils_Object_UE::
    Request_CloneObject(
        const T* InObjectToClone,
        T_Func InPostClone,
        ck::policy::TransientPackage)
    -> T*
{
    return Request_CloneObject(GetTransientPackage(), InObjectToClone, InPostClone);
}

// --------------------------------------------------------------------------------------------------------------------
