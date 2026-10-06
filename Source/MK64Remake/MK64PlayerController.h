#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MK64PlayerController.generated.h"

UCLASS()
class MK64REMAKE_API AMK64PlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void SetupInputComponent() override;
    bool IsOptionsVisible() const { return bOptionsVisible; }
private:
    bool bOptionsVisible = false;
    void ToggleOptions();
};
