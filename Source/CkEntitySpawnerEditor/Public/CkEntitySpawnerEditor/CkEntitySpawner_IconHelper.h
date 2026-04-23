#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class AActor;

class CKENTITYSPAWNEREDITOR_API FCk_EntitySpawner_IconHelper
{
public:
    static UTexture2D* Get_Texture();
    static void ApplyToActor(AActor* InActor);
};
