#include "UI/UI_AMiniMapCapture.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"

// Sets default values
AUI_AMiniMapCapture::AUI_AMiniMapCapture()
{
	PrimaryActorTick.bCanEverTick = false;

    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootScene;

    CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
    CaptureComponent->SetupAttachment(RootComponent);

    // 기본 설정 (블루프린트에서 수정 가능하도록 최소화)
    CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
    CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;


    CaptureComponent->SetAbsolute(false, true, false);
    CaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
  

    CaptureComponent->ShowFlags.SetDynamicShadows(false); // 동적 그림자
    CaptureComponent->ShowFlags.SetGlobalIllumination(false); // 루멘

    // 생성자나 BeginPlay에서 설정
    CaptureComponent->bCaptureEveryFrame = false; // 매 프레임 캡처 중지
    CaptureComponent->bCaptureOnMovement = false; // 움직일 때마다 캡처 중지

    //CaptureComponent->ShowFlags.SetMotionBlur(false); // 잔상 제거용
    //CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_BaseColor; // 포스트 프로세싱 무효화
}

void AUI_AMiniMapCapture::UpdateMiniMap()
{
    CaptureComponent->CaptureScene();
}

// Called when the game starts or when spawned
void AUI_AMiniMapCapture::BeginPlay()
{
	Super::BeginPlay();

    CaptureComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 10000.0f));
    CaptureComponent->OrthoWidth = 25000.0f; // 맵 전체가 다 들어올 정도의 너비
	
}

