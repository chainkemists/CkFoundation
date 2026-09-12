#pragma once

#include "CoreMinimal.h"
#include "CkSlateLayout/CkUiDocument.h"

/**
 * Game-thread float samples shared by a host and custom UI widgets. Hosts retain the mutable
 * shared pointer; consumers receive only weak pointers to this const-facing model.
 */
class CKSLATELAYOUT_API FCkUiFloatSeries final
{
public:
    static auto TryCreate(TArray<float> InSamples, TSharedPtr<FCkUiFloatSeries>& OutSeries) -> FCkUiLoadResult;

    /** Publishes an all-or-nothing sample replacement. */
    auto TrySetSamples(TArray<float> InSamples) -> FCkUiLoadResult;
    const TArray<float>& GetSamples() const { return _Samples; }
    int64 GetRevision() const { return _Revision; }

private:
    explicit FCkUiFloatSeries(TArray<float>&& InSamples) : _Samples(MoveTemp(InSamples)) {}

    TArray<float> _Samples;
    int64 _Revision = 0;
};
