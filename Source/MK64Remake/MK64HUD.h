#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MK64HUD.generated.h"

UCLASS()
class MK64REMAKE_API AMK64HUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
