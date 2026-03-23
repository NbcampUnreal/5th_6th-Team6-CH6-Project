#include "UI/UI_AMiniMapCapture.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"

// Sets default values
AUI_AMiniMapCapture::AUI_AMiniMapCapture()
{
	PrimaryActorTick.bCanEverTick = false;

    CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
    RootComponent = CaptureComponent;

    CaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;

    // CaptureComponent->CaptureSource = SCS_SceneColorHDR;
    CaptureComponent->bCaptureEveryFrame = true;
    CaptureComponent->bCaptureOnMovement = false;





    //// 미니맵 캡처 기본 설정
    //CaptureComponent->SetAbsolute(true, true, true); // 순서대로: 위치, 회전, 스케일
    //// 위치는 캐릭터를 따라다녀야 함으로 앱솔루트 ㄴㄴ
    //CaptureComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1000.0f));
    //CaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;	// 투명도 반영
}

// Called when the game starts or when spawned
void AUI_AMiniMapCapture::BeginPlay()
{
	Super::BeginPlay();

    SetActorLocation(FVector(0.0f, 0.0f, 5000.0f));
    CaptureComponent->OrthoWidth = 10000.0f; // 맵 전체가 다 들어올 정도의 너비
	
}

