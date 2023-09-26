#pragma once

#include "CkAlgorithms.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_algo_tests
{
    auto
        DoTest_ForEachIsValid()
    {
        {
            TArray<UObject*> Objects;
            ck::algo::ForEachIsValid(Objects.begin(), Objects.end(), [](auto InObj)
            {
                // work with a valid object here
            });
        }

        {
            TArray<AActor*> Actors;
            ck::algo::ForEachIsValid(Actors.begin(), Actors.end(), [](auto InActor)
            {
                // work with a valid actor here
            });
        }

        {
            TArray<UObject**> ObjectPointers;
            ck::algo::ForEachIsValid(ObjectPointers.begin(), ObjectPointers.end(), [](auto InObj)
            {
                // work with a valid object here
            }, [](auto InObjPtr) { return ck::IsValid(*InObjPtr); });
        }

        {
            struct ContainsAnObject
            {
                UObject* _MyObj;
            };

            TArray<ContainsAnObject> ObjStructs;
            ck::algo::ForEachIsValid(ObjStructs.begin(), ObjStructs.end(), [](auto InStruct)
            {
                // work with a valid object here
            }, [](auto InStruct) { return ck::IsValid(InStruct._MyObj); });
        }
    }
}
// --------------------------------------------------------------------------------------------------------------------
