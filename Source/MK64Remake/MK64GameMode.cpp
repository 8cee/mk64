#include "MK64GameMode.h"
#include "MK64KartPawn.h"
#include "MK64TrackActor.h"
#include "MK64PlayerController.h"
#include "MK64HUD.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"

AMK64GameMode::AMK64GameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = AMK64PlayerController::StaticClass();
    HUDClass = AMK64HUD::StaticClass();
}

void AMK64GameMode::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    World->SpawnActor<AMK64TrackActor>(
        AMK64TrackActor::StaticClass(),
        FVector(0.0f, 0.0f, -100.0f),
        FRotator::ZeroRotator);

    AMK64KartPawn* Kart = World->SpawnActor<AMK64KartPawn>(
        AMK64KartPawn::StaticClass(),
        FVector(0.0f, 0.0f, -50.0f),
        FRotator::ZeroRotator);

    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        PC->Possess(Kart);
    }

    ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
        ADirectionalLight::StaticClass(),
        FVector::ZeroVector,
        FRotator(-45.0f, -30.0f, 0.0f));
    if (Sun && Sun->GetLightComponent())
    {
        Sun->GetLightComponent()->SetIntensity(8.0f);
    }

    ASkyLight* Sky = World->SpawnActor<ASkyLight>();
    if (Sky && Sky->GetLightComponent())
    {
        Sky->GetLightComponent()->SetIntensity(1.0f);
        Sky->GetLightComponent()->MarkRenderStateDirty();
    }
}
