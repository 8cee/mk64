#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MK64GameMode.generated.h"

UCLASS()
class MK64REMAKE_API AMK64GameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AMK64GameMode();
    virtual void BeginPlay() override;
};
