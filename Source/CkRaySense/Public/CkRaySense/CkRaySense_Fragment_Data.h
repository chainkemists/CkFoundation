#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>
#include <PhysicalMaterials/PhysicalMaterial.h>

#include "CkRaySense_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_RaySense_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RaySense_Async : uint8
{
    Synchronous,
    Asynchronous UMETA(DisplayName = "Asynchronous (NOT yet Supported)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RaySense_Async);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RaySense_CollisionQuality : uint8
{
    Sweep,
    Discrete
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RaySense_CollisionQuality);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RaySense_CollisionResponse_Policy : uint8
{
    Overlap,
    Collide
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RaySense_CollisionResponse_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRAYSENSE_API FCk_Handle_RaySense : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_RaySense); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_RaySense);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRAYSENSE_API FCk_RaySense_DataToIgnore
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RaySense_DataToIgnore);

private:
    // Weak by design: non-owning observation — a destroyed ignored actor must read as gone, never
    // as a recycled address that false-positively ignores a live one. UHT forbids Blueprint exposure
    // of weak-pointer ARRAYS, so these are editor + C++ surface only (they had zero BP/AS writers).
    UPROPERTY(EditAnywhere,
        meta = (AllowPrivateAccess = true))
    TArray<TWeakObjectPtr<AActor>> _ActorsToIgnore;

    UPROPERTY(EditAnywhere,
        meta = (AllowPrivateAccess = true))
    TArray<TWeakObjectPtr<UPrimitiveComponent>> _ComponentsToIgnore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle> _EntitiesToIgnore;

public:
    CK_PROPERTY(_ActorsToIgnore);
    CK_PROPERTY(_ComponentsToIgnore);
    CK_PROPERTY(_EntitiesToIgnore);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRAYSENSE_API FCk_Fragment_RaySense_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_RaySense_ParamsData);

private:
    // Discrete does NOT work for RaySense that do not have a shape
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_RaySense_CollisionQuality _CollisionQuality = ECk_RaySense_CollisionQuality::Sweep;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TEnumAsByte<ECollisionChannel> _CollisionChannel = ECollisionChannel::ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_RaySense_CollisionResponse_Policy _CollisionResponse = ECk_RaySense_CollisionResponse_Policy::Overlap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_RaySense_Async _Async = ECk_RaySense_Async::Synchronous;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_RaySense_DataToIgnore _DataToIgnore;

public:
    CK_PROPERTY_GET(_CollisionQuality);
    CK_PROPERTY_GET(_CollisionChannel);
    CK_PROPERTY(_CollisionResponse);
    CK_PROPERTY(_Async);
    CK_PROPERTY(_DataToIgnore);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_RaySense_ParamsData, _CollisionQuality, _CollisionChannel);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRAYSENSE_API FCk_Request_RaySense_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RaySense_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RaySense_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RaySense_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRAYSENSE_API FCk_RaySense_HitResult
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_RaySense_HitResult);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector _ImpactPoint = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector _ImpactNormal = FVector::ZeroVector;

    // Weak by design: a stored hit result observes actors/components that may be destroyed before
    // the result is read — the canonical TWeakObjectPtr case, and how FHitResult itself holds them.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UPhysicalMaterial> _ImpactPhysMat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<AActor> _MaybeHitActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UPrimitiveComponent> _MaybeHitComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle _MaybeHitHandle;

public:
    CK_PROPERTY_GET(_ImpactPoint);
    CK_PROPERTY_GET(_ImpactNormal);
    // Hand-written raw-resolving getters keep the C++/AS surface identical to the pre-weak shape
    // (consumers compare against live objects); the fluent setters accept the weak directly. The
    // CK_PROPERTY macro would register weak-TYPED AngelScript accessors instead, so the raw-typed
    // AS methods are hand-registered below via the same registration hook the macro uses.
    auto Get_ImpactPhysMat() const -> UPhysicalMaterial* { return _ImpactPhysMat.Get(); }
    auto Set_ImpactPhysMat(const TWeakObjectPtr<UPhysicalMaterial>& InValue) -> ThisType& { _ImpactPhysMat = InValue; return *this; }
    auto Get_MaybeHitActor() const -> AActor* { return _MaybeHitActor.Get(); }
    auto Set_MaybeHitActor(const TWeakObjectPtr<AActor>& InValue) -> ThisType& { _MaybeHitActor = InValue; return *this; }
    auto Get_MaybeHitComponent() const -> UPrimitiveComponent* { return _MaybeHitComponent.Get(); }
    auto Set_MaybeHitComponent(const TWeakObjectPtr<UPrimitiveComponent>& InValue) -> ThisType& { _MaybeHitComponent = InValue; return *this; }
    CK_PROPERTY(_MaybeHitHandle);

#if WITH_ANGELSCRIPT_CK
private:
    static void DoRegisterAngelScriptResolvedGetters()
    {
        const auto ClassTypeStr = ck::Get_RuntimeTypeToString_AngelScript<ThisType>();
        auto ExistingClass = FAngelscriptBinds::ExistingClass(FBindString(ClassTypeStr));
        CK_ENSURE_IF_NOT(ExistingClass.GetTypeInfo() != nullptr,
            TEXT("AngelScript has no bound type for [{}] — the raw-resolving hit-result getters "
                 "(Get_ImpactPhysMat/Get_MaybeHitActor/Get_MaybeHitComponent) cannot be registered, and every "
                 "AngelScript caller of them will fail to compile with no diagnostic at this site."),
            ClassTypeStr)
        { return; }

        // False means the key is already claimed by an earlier pass, so the getters are already bound.
        if (NOT FCkAngelScriptPropertyFunctionRegistration::TryRegisterProperty(
                ck::Format_UE(TEXT("{}::Get_MaybeHitActor"), ClassTypeStr)))
        { return; }

        auto* TypeInfo = ExistingClass.GetTypeInfo();

        if (TypeInfo->GetMethodByName("Get_ImpactPhysMat") == nullptr)
        {
            ExistingClass.Method("UPhysicalMaterial Get_ImpactPhysMat() const",
                [](const ThisType* Self) -> UPhysicalMaterial* { return Self->_ImpactPhysMat.Get(); });
            FScriptFunctionNativeForm::BindNativeMethod(ExistingClass, "Get_ImpactPhysMat", true);
        }
        if (TypeInfo->GetMethodByName("Get_MaybeHitActor") == nullptr)
        {
            ExistingClass.Method("AActor Get_MaybeHitActor() const",
                [](const ThisType* Self) -> AActor* { return Self->_MaybeHitActor.Get(); });
            FScriptFunctionNativeForm::BindNativeMethod(ExistingClass, "Get_MaybeHitActor", true);
        }
        if (TypeInfo->GetMethodByName("Get_MaybeHitComponent") == nullptr)
        {
            ExistingClass.Method("UPrimitiveComponent Get_MaybeHitComponent() const",
                [](const ThisType* Self) -> UPrimitiveComponent* { return Self->_MaybeHitComponent.Get(); });
            FScriptFunctionNativeForm::BindNativeMethod(ExistingClass, "Get_MaybeHitComponent", true);
        }

        FAngelscriptBinds::SetPreviousBindNoDiscard(false);
    }

    static inline bool _AngelScriptResolvedGettersRegistered = []() -> bool
    {
        FCkAngelScriptPropertyFunctionRegistration::RegisterPropertyFunction(&DoRegisterAngelScriptResolvedGetters);
        return true;
    }();

public:
#endif

    CK_DEFINE_CONSTRUCTORS(FCk_RaySense_HitResult, _ImpactPoint, _ImpactNormal);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_RaySense_LineTrace,
    FCk_Handle_RaySense, InHandle,
    FCk_RaySense_HitResult, InHitResult);

