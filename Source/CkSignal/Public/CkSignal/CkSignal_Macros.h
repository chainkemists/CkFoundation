#pragma once

#include "CkSignal/CkSignal_Fragment.h"
#include "CkSignal/CkSignal_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Granular macros for defining Signals and Utilities individually.
// Consider using the 'meta' macros found below

#define CK_DEFINE_SIGNAL(_API_, _SignalName_, ...)\
    _API_ struct FFragment_Signal_##_SignalName_ \
    : public ck::TFragment_Signal<__VA_ARGS__> {}

#define CK_DEFINE_SIGNAL_UTILS(_API_, _SignalName_)\
    _API_ class UUtilsSignal_##_SignalName_ \
    : public ck::TUtils_Signal<FFragment_Signal_##_SignalName_> {}

#define CK_DEFINE_SIGNAL_WITH_DELEGATE(_API_, _SignalName_, _MulticastName_, ...)\
    _API_ struct FFragment_Signal_UnrealMulticast_##_SignalName_ \
    : public ck::TFragment_Signal_UnrealMulticast<_MulticastName_, ECk_Signal_PostFireBehavior::DoNothing, __VA_ARGS__> {};\
    _API_ struct FFragment_Signal_UnrealMulticast_##_SignalName_##_PostFireUnbind \
    : public ck::TFragment_Signal_UnrealMulticast<_MulticastName_, ECk_Signal_PostFireBehavior::Unbind, __VA_ARGS__> {}

#define CK_DEFINE_SIGNAL_WITH_DELEGATE_UTILS(_API_, _SignalName_)\
    _API_ class UUtils_Signal_##_SignalName_ \
    : public ck::TUtils_Signal_UnrealMulticast<FFragment_Signal_##_SignalName_, FFragment_Signal_UnrealMulticast_##_SignalName_> {};\
    _API_ class UUtils_Signal_##_SignalName_##_PostFireUnbind\
    : public ck::TUtils_Signal_UnrealMulticast<FFragment_Signal_##_SignalName_, FFragment_Signal_UnrealMulticast_##_SignalName_##_PostFireUnbind> {}

// --------------------------------------------------------------------------------------------------------------------
// 'Meta' Macros for defining Signals and Utilities

// Define Signal that works ONLY within C++
#define CK_DEFINE_SIGNAL_AND_UTILS(_API_, _SignalName_, ...)\
    CK_DEFINE_SIGNAL(_API_, _SignalName_, __VA_ARGS__)\
    CK_DEFINE_SIGNAL_UTILS(_API_, _SignalName_)\

// Define Signal that works in C++ AND supports Unreal Delegates with the same interface
#define CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(_API_, _SignalName_, _MulticastDelegate_, ...)\
    CK_DEFINE_SIGNAL(_API_, _SignalName_, __VA_ARGS__);\
    CK_DEFINE_SIGNAL_WITH_DELEGATE(_API_, _SignalName_, _MulticastDelegate_, __VA_ARGS__);\
    CK_DEFINE_SIGNAL_WITH_DELEGATE_UTILS(_API_, _SignalName_)

#define CK_SIGNAL_BIND_PROMISE(_Handle_, _PromiseDelegate_, _SignalUtils_)\
if (_PromiseDelegate_.IsBound())\
{\
    _SignalUtils_##::Bind(_Handle_, _PromiseDelegate_, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);\
}

// --------------------------------------------------------------------------------------------------------------------
