#pragma once

#include "CkCore/Payload/CkPayload.h"

#include "CkSignal/CkSignal_Fragment.h"
#include "CkSignal/CkSignal_Utils.h"

#include "CkEcs/Handle/CkHandle.h"

#define CK_DEFINE_SIGNAL(_API_, _SignalName_, ...)\
    _API_ struct FFragment_Signal_##_SignalName_ \
    : public ck::TFragment_Signal<FFragment_Signal_##_SignalName_, __VA_ARGS__> {}

#define CK_DEFINE_SIGNAL_UTILS(_API_, _SignalName_)\
    _API_ class UUtilsSignal_##_SignalName_ \
    : public ck::TUtils_Signal<FFragment_Signal_##_SignalName_> {}

#define CK_DEFINE_SIGNAL_WITH_DELEGATE(_API_, _SignalName_, _MulticastName_, ...)\
    _API_ struct FFragment_Signal_UnrealMulticast_##_SignalName_ \
    : public ck::TFragment_Signal_UnrealMulticast<FFragment_Signal_##_SignalName_, _MulticastName_, __VA_ARGS__> {}

#define CK_DEFINE_SIGNAL_WITH_DELEGATE_UTILS(_API_, _SignalName_)\
    _API_ class UUtils_Signal_UnrealMulticast_##_SignalName_ \
    : public ck::TUtils_Signal_UnrealMulticast<FFragment_Signal_##_SignalName_, FFragment_Signal_UnrealMulticast_##_SignalName_> {}
