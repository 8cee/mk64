#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MK64TrackActor.generated.h"
class UStaticMeshComponent;

UCLASS()
class MK64REMAKE_API AMK64TrackActor : public AActor
{
    GENERATED_BODY()
public:
    AMK64TrackActor();
private:
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* TrackMesh;
};
