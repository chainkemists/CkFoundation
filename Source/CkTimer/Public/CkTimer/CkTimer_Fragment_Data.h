#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkNet/CkNet_Common.h"

#include "CkCore/Format/CkFormat.h"

#include "CkTimer_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Timer_Manipulate : uint8
{
    Reset,
    Complete,
    Stop,
    Pause,
    Resume
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_Manipulate);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Timer_Behavior : uint8
{
    StopOnDone UMETA(DisplayName = "Reset And Pause On Done"),
    ResetOnDone UMETA(DisplayName = "Reset And Resume On Done"),
    PauseOnDone
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_Behavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Timer_State : uint8
{
    Paused,
    Running
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_State);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Timer_CountDirection : uint8
{
    CountUp,
    CountDown
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_CountDirection);

// --------------------------------------------------------------------------------------------------------------------

// Why not inherit from FCk_Handle? Solely to provide a way, in Blueprints, to convert from a type-safe Handle to a regular
// handle simply by breaking up the struct. Why not inherit and provide a custom break or a function to convert? To reduce
// some boilerplate in our utils library
USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Handle_Timer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_Timer);

private:
    UPROPERTY(BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    FCk_Handle _RawHandle;

public:
    template <typename T_Fragment, typename... T_Args>
    auto Add(T_Args&&... InArgs) -> T_Fragment& { return _RawHandle.Add<T_Fragment>(std::forward<T_Args>(InArgs)...); }

    template <typename T_Fragment, typename T_ValidationPolicy, typename... T_Args>
    auto Add(T_Args&&... InArgs) -> T_Fragment& { return _RawHandle.Add<T_Fragment, T_ValidationPolicy>(std::forward<T_Args>(InArgs)...); }

    template <typename T_Fragment, typename... T_Args>
    auto AddOrGet(T_Args&&... InArgs) -> T_Fragment& { return _RawHandle.AddOrGet<T_Fragment>(std::forward<T_Args>(InArgs)...); }

    template <typename T_Fragment, typename T_Func>
    auto Try_Transform(T_Func InFunc) -> void { _RawHandle.Try_Transform(InFunc); }

    template <typename T_Fragment, typename... T_Args>
    auto Replace(T_Args&&... InArgs) -> T_Fragment& { return _RawHandle.Replace<T_Fragment>(std::forward<T_Args>(InArgs)...); }

    template <typename T_Fragment>
    auto Remove() -> void { _RawHandle.Remove<T_Fragment>(); }

    template <typename T_Fragment>
    auto Try_Remove() -> void { _RawHandle.Try_Remove<T_Fragment>(); }

    template <typename... T_Fragment>
    auto View() -> FCk_Registry::RegistryViewType<T_Fragment...> { return _RawHandle.View<T_Fragment...>(); }

    template <typename... T_Fragment>
    auto View() const -> FCk_Registry::ConstRegistryViewType<T_Fragment...> { return _RawHandle.View<T_Fragment...>(); }

public:
    template <typename T_Fragment>
    auto Has() const -> bool { return _RawHandle.Has<T_Fragment>(); }

    template <typename... T_Fragment>
    auto Has_Any() const -> bool { return _RawHandle.Has_Any<T_Fragment...>(); }

    template <typename... T_Fragment>
    auto Has_All() const -> bool { return _RawHandle.Has_All<T_Fragment...>(); }

    template <typename T_Fragment>
    auto Get() -> T_Fragment& { return _RawHandle.Get<T_Fragment>(); }

    template <typename T_Fragment>
    auto Get() const -> const T_Fragment& { return _RawHandle.Get<T_Fragment>(); }

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Get() -> T_Fragment& { return _RawHandle.Get<T_Fragment, T_ValidationPolicy>(); }

    template <typename T_Fragment, typename T_ValidationPolicy>
    auto Get() const -> const T_Fragment& { return _RawHandle.Get<T_Fragment, T_ValidationPolicy>(); }

public:
    auto operator*() -> TOptional<FCk_Registry> { return _RawHandle.operator*(); }
    auto operator*() const -> TOptional<FCk_Registry> { return _RawHandle.operator*(); }

    auto operator->() -> TOptional<FCk_Registry> { return _RawHandle.operator->(); }
    auto operator->() const -> TOptional<FCk_Registry> { return _RawHandle.operator->(); }

public:
    auto IsValid(ck::IsValid_Policy_Default) const -> bool { return _RawHandle.IsValid(ck::IsValid_Policy_Default{}); }
    auto IsValid(ck::IsValid_Policy_IncludePendingKill) const -> bool { return _RawHandle.IsValid(ck::IsValid_Policy_IncludePendingKill{}); }

    auto Orphan() const -> bool { return _RawHandle.Orphan(); }
    auto Get_ValidHandle(FCk_Entity::IdType InEntity) const -> FCk_Handle { return _RawHandle.Get_ValidHandle(InEntity); }

    auto Get_Registry() -> FCk_Registry& { return _RawHandle.Get_Registry(); }
    auto Get_Registry() const -> const FCk_Registry& { return _RawHandle.Get_Registry(); }

public:
    CK_PROPERTY_GET(_RawHandle);
    CK_PROPERTY_GET_NON_CONST(_RawHandle);
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_Handle_Timer, [&]() { return ck::Format(TEXT("{}"), InObj.Get_RawHandle()); });

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Fragment_Timer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Timer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _TimerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time   _Duration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_CountDirection _CountDirection = ECk_Timer_CountDirection::CountUp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_Behavior _Behavior = ECk_Timer_Behavior::PauseOnDone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_State _StartingState = ECk_Timer_State::Paused;

public:
    CK_PROPERTY(_TimerName);
    CK_PROPERTY_GET(_Duration);
    CK_PROPERTY(_CountDirection);
    CK_PROPERTY(_Behavior);
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Timer_ParamsData, _Duration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Fragment_MultipleTimer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleTimer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Timer_ParamsData> _TimerParams;

public:
    CK_PROPERTY_GET(_TimerParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleTimer_ParamsData, _TimerParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Request_Timer_Jump
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_Jump);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _JumpDuration;

public:
    CK_PROPERTY_GET(_JumpDuration);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_Jump, _JumpDuration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Request_Timer_Consume
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_Consume);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _ConsumeDuration;

public:
    CK_PROPERTY_GET(_ConsumeDuration);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_Consume, _ConsumeDuration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Request_Timer_Manipulate
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_Manipulate);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_Manipulate _Manipulate = ECk_Timer_Manipulate::Reset;

public:
    CK_PROPERTY_GET(_Manipulate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_Manipulate, _Manipulate);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Request_Timer_ChangeDirection
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_ChangeDirection);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_CountDirection _CountDirection = ECk_Timer_CountDirection::CountUp;

public:
    CK_PROPERTY_GET(_CountDirection);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_ChangeDirection, _CountDirection);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Timer,
    FCk_Handle_Timer, InHandle,
    FCk_Chrono, InChrono);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Timer_MC,
    FCk_Handle_Timer, InHandle,
    FCk_Chrono, InChrono);

// --------------------------------------------------------------------------------------------------------------------

