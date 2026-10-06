#include "MK64HUD.h"
#include "MK64PlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AMK64HUD::DrawHUD()
{
    Super::DrawHUD();
    auto* PC = Cast<AMK64PlayerController>(GetOwningPlayerController());
    if (!PC || !PC->IsOptionsVisible() || !Canvas) return;

    const float X = 40.0f;
    const float Y = 40.0f;
    const float W = FMath::Min(680.0f, Canvas->SizeX - 80.0f);
    const float H = 300.0f;

    DrawRect(FLinearColor(0.02f,0.03f,0.05f,0.92f), X, Y, W, H);
    DrawText(TEXT("MK64 REMAKE - OPTIONS"), FLinearColor::White, X+24, Y+22, GEngine->GetLargeFont(), 1.0f);
    DrawText(TEXT("F11  Close menu"), FLinearColor(0.8f,0.85f,0.9f,1), X+24, Y+74);
    DrawText(TEXT("W/S or Arrows  Accelerate / Brake"), FLinearColor(0.8f,0.85f,0.9f,1), X+24, Y+108);
    DrawText(TEXT("A/D or Arrows  Steer"), FLinearColor(0.8f,0.85f,0.9f,1), X+24, Y+142);
    DrawText(TEXT("Graphics pipeline: UE5 / Lumen-ready"), FLinearColor(0.4f,0.8f,1.0f,1), X+24, Y+190);
    DrawText(TEXT("Next: MK64 track/vehicle data integration"), FLinearColor(0.7f,0.7f,0.75f,1), X+24, Y+224);
}
