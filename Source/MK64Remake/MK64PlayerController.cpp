#include "MK64PlayerController.h"
#include "Engine/Engine.h"

void AMK64PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("ToggleOptions"), IE_Pressed, this, &AMK64PlayerController::ToggleOptions);
}

void AMK64PlayerController::ToggleOptions()
{
    bOptionsVisible = !bOptionsVisible;
    bShowMouseCursor = bOptionsVisible;
    SetPause(bOptionsVisible);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 2.0f, FColor::Yellow,
            bOptionsVisible ? TEXT("F11 OPTIONS OPEN") : TEXT("F11 OPTIONS CLOSED"));
    }
}
