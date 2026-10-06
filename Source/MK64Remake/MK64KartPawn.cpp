#include "MK64KartPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

AMK64KartPawn::AMK64KartPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Player0;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KartBody"));
    SetRootComponent(Body);
    Body->SetSimulatePhysics(false);
    Body->SetCollisionProfileName(TEXT("Pawn"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded()) Body->SetStaticMesh(CubeMesh.Object);
    Body->SetRelativeScale3D(FVector(1.25f, 0.75f, 0.35f));

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(Body);
    SpringArm->TargetArmLength = 550.0f;
    SpringArm->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 8.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
}

void AMK64KartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("Throttle"), this, &AMK64KartPawn::SetThrottle);
    PlayerInputComponent->BindAxis(TEXT("Steer"), this, &AMK64KartPawn::SetSteer);
}
void AMK64KartPawn::SetThrottle(float Value){ ThrottleInput = FMath::Clamp(Value,-1.0f,1.0f); }
void AMK64KartPawn::SetSteer(float Value){ SteerInput = FMath::Clamp(Value,-1.0f,1.0f); }

void AMK64KartPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    constexpr float MaxForwardSpeed=1800.0f, MaxReverseSpeed=700.0f;
    constexpr float Acceleration=2600.0f, Braking=3400.0f, Drag=1200.0f, TurnRate=105.0f;
    if (FMath::Abs(ThrottleInput) > KINDA_SMALL_NUMBER) {
        const float Target = ThrottleInput >= 0.0f ? MaxForwardSpeed : -MaxReverseSpeed;
        const float Rate = (FMath::Sign(Target)==FMath::Sign(Speed) || FMath::IsNearlyZero(Speed)) ? Acceleration : Braking;
        Speed = FMath::FInterpConstantTo(Speed,Target,DeltaSeconds,Rate);
    } else {
        Speed = FMath::FInterpConstantTo(Speed,0.0f,DeltaSeconds,Drag);
    }
    const float SteeringStrength = FMath::GetMappedRangeValueClamped(FVector2D(0.0f,MaxForwardSpeed),FVector2D(0.35f,1.0f),FMath::Abs(Speed));
    AddActorLocalRotation(FRotator(0.0f,SteerInput*TurnRate*SteeringStrength*DeltaSeconds,0.0f));
    FHitResult Hit;
    AddActorWorldOffset(GetActorForwardVector()*Speed*DeltaSeconds,true,&Hit);
    if (Hit.IsValidBlockingHit()) Speed *= -0.2f;
}
