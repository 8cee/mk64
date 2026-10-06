#include "MK64TrackActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMK64TrackActor::AMK64TrackActor()
{
    PrimaryActorTick.bCanEverTick = false;
    TrackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrackMesh"));
    SetRootComponent(TrackMesh);
    TrackMesh->SetCollisionProfileName(TEXT("BlockAll"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded()) TrackMesh->SetStaticMesh(CubeMesh.Object);
    TrackMesh->SetRelativeScale3D(FVector(60.0f,60.0f,0.5f));
}
