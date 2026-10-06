#include "MK64GameMode.h"
#include "MK64KartPawn.h"
#include "MK64TrackActor.h"
#include "Engine/World.h"

AMK64GameMode::AMK64GameMode(){ DefaultPawnClass=AMK64KartPawn::StaticClass(); }
void AMK64GameMode::BeginPlay()
{
    Super::BeginPlay();
    if(UWorld* World=GetWorld()){
        World->SpawnActor<AMK64TrackActor>(AMK64TrackActor::StaticClass(),FVector(0,0,-100),FRotator::ZeroRotator);
    }
}
