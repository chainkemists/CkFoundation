#pragma once

#include "CkAlgorithms.h"

#include "CkValidation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_algo_tests
{
    auto
        DoTest_ForEachIsValid()
    {
        {
            TArray<UObject*> objects;
            ck::algo::ForEachIsValid(objects.begin(), objects.end(), [](auto InObj)
            {
                // work with a valid object here
            });
        }

        {
            TArray<AActor*> actors;
            ck::algo::ForEachIsValid(actors.begin(), actors.end(), [](auto InActor)
            {
                // work with a valid actor here
            });
        }

        {
            TArray<UObject**> objectPointers;
            ck::algo::ForEachIsValid(objectPointers.begin(), objectPointers.end(), [](auto InObj)
            {
                // work with a valid object here
            }, [](auto InObjPtr) { return ck::IsValid(*InObjPtr); });
        }

        {
            struct ContainsAnObject
            {
                UObject* myObj;
            };

            TArray<ContainsAnObject> objStrcuts;
            ck::algo::ForEachIsValid(objStrcuts.begin(), objStrcuts.end(), [](auto InStruct)
            {
                // work with a valid object here
            }, [](auto InStruct) { return ck::IsValid(InStruct.myObj); });
        }
    }
}
// --------------------------------------------------------------------------------------------------------------------
