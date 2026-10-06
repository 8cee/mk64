#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MK64KartPawn.generated.h"
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class MK64REMAKE_API AMK64KartPawn : public APawn
{
    GENERATED_BODY()
public:
    AMK64KartPawn();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
private:
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
    UPROPERTY(VisibleAnywhere) USpringArmComponent* SpringArm;
    UPROPERTY(VisibleAnywhere) UCameraComponent* Camera;
    float ThrottleInput = 0.0f;
    float SteerInput = 0.0f;
    float Speed = 0.0f;
    void SetThrottle(float Value);
    void SetSteer(float Value);
};
