#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Player/BasePlayerState.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "CharacterSystem/Data/CharacterData.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create the camera boom component
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	// Create the camera component
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));

	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	CurrentPathIndex = INDEX_NONE;
	
	bReplicates = true;
	SetReplicateMovement(true);
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorTickEnabled(false);
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Tick 활성화 시 경로 탐색 (서버)
	UpdatePathFollowing();
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	// ASC 초기화 (서버)
	InitAbilitySystem();
	
	// 비주얼 초기화 (서버)
	if (HeroData)
	{
		InitVisuals();
	}
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABaseCharacter, HeroData);
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

void ABaseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	// ASC 초기화 (클라이언트)
	InitAbilitySystem();
}

void ABaseCharacter::OnRep_HeroData()
{
	// 비주엃 초기화 (클라이언트)
	InitVisuals();
}

void ABaseCharacter::InitAbilitySystem()
{
	ABasePlayerState* PS = GetPlayerState<ABasePlayerState>();
	if (!PS) return;
	
	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return;
	
	// ASC 캐싱 / Actor Info 설정
	AbilitySystemComponent = ASC;
	ASC->InitAbilityActorInfo(PS, this);
	
	if (HasAuthority() && HeroData)
	{
		// Ability 부여
		for (const auto& AbilityPair : HeroData->Abilities)
		{
			FGameplayTag InputTag = AbilityPair.Key;
			TSoftClassPtr<UGameplayAbility> AbilityPtr = AbilityPair.Value;

			// SoftClassPtr 동기 로드 (게임 시작 시점이므로 즉시 로드)
			if (UClass* AbilityClass = AbilityPtr.LoadSynchronous())
			{
				// Spec 생성 (레벨 1, InputID는 태그의 해시값 등을 사용하거나 별도 매핑 필요)
				// 여기서는 예시로 1레벨 부여
				FGameplayAbilitySpec Spec(AbilityClass, 1);
                
				// 동적 Input Tag깅 (Enhanced Input과 연동하기 위해 Spec에 태그 추가 가능)
				Spec.DynamicAbilityTags.AddTag(InputTag);
                
				ASC->GiveAbility(Spec);
			}
		}

		// B. 스탯(Attributes) 초기화
		// 방법 1: GE_InitStats를 만들고 SetByCaller나 CurveTable을 이용해 초기화
		// 방법 2: AttributeSet에 직접 접근 (초기화 단계에서만 허용되는 방식)
        
		// MOBA에서는 보통 CurveTable을 이용한 GE 초기화를 권장함.
		// 이 부분은 별도의 GE_InitStats 구현이 필요하므로 주석 처리
		// InitializeAttributes(ASC, HeroData->StatusRowName); 
	}
}

void ABaseCharacter::InitVisuals()
{
	if (!HeroData) return;

	// 1. 스켈레탈 메시 로드 및 설정
	// TSoftObjectPtr을 동기 로드(LoadSynchronous)합니다.
	// 최적화 Tip: AssetManager를 통해 게임 시작 전(로딩 화면)에 미리 AsyncLoad 해두면 
	// 여기서 LoadSynchronous를 호출해도 딜레이가 0입니다.
	if (!HeroData->Mesh.IsNull())
	{
		if (USkeletalMesh* LoadedMesh = HeroData->Mesh.LoadSynchronous())
		{
			GetMesh()->SetSkeletalMesh(LoadedMesh);
            
			// 메시 크기에 맞춰 캡슐 컴포넌트 조정 (필요 시)
			// GetCapsuleComponent()->SetCapsuleSize(...);
            
			// 메시 위치 조정 (데이터에 오프셋이 있다면 적용)
			// GetMesh()->SetRelativeLocation(...);
		}
	}

	// ABP 로드 및 설정
	if (!HeroData->AnimClass.IsNull())
	{
		if (UClass* LoadedAnimClass = HeroData->AnimClass.LoadSynchronous())
		{
			GetMesh()->SetAnimInstanceClass(LoadedAnimClass);
		}
	}
}

void ABaseCharacter::Server_MoveToLocation_Implementation(FVector TargetLocation)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys == nullptr) return;

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(this, GetActorLocation(), TargetLocation);
	
	if (NavPath != nullptr && NavPath->PathPoints.Num() > 1)
	{
		PathPoints = NavPath->PathPoints;
		CurrentPathIndex = 1;
		SetActorTickEnabled(true);
	}
	else
	{
		// 경로 탐색 실패 시 중지
		StopPathFollowing();
	}
}

bool ABaseCharacter::Server_MoveToLocation_Validate(FVector TargetLocation)
{
	return true;
}

void ABaseCharacter::MoveToLocation(FVector TargetLocation)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return;
	
	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(this, GetActorLocation(), TargetLocation);

	if (NavPath && NavPath->PathPoints.Num() > 1)
	{
		PathPoints = NavPath->PathPoints;
		CurrentPathIndex = 1;
		SetActorTickEnabled(true);
	}
	else
	{
		// 경로 탐색 실패 시 중지
		StopPathFollowing();
	}
	
	if (!HasAuthority())
	{
		Server_MoveToLocation(TargetLocation);
	}
}

void ABaseCharacter::UpdatePathFollowing()
{
	if (CurrentPathIndex == INDEX_NONE || CurrentPathIndex >= PathPoints.Num())
	{
		StopPathFollowing();
		return;
	}

	FVector CurrentLocation = GetActorLocation();
	FVector TargetPoint = PathPoints[CurrentPathIndex];

	FVector Direction = (TargetPoint - CurrentLocation);
	Direction.Z = 0.f;

	if (Direction.SizeSquared() < ArrivalDistanceSq)
	{
		CurrentPathIndex++;
		if (CurrentPathIndex >= PathPoints.Num())
		{
			StopPathFollowing();
			return;
		}
		
		TargetPoint = PathPoints[CurrentPathIndex];
		Direction = (TargetPoint - CurrentLocation);
		Direction.Z = 0.f;
	}
	
	AddMovementInput(Direction.GetSafeNormal(), 1.0f);
}

void ABaseCharacter::StopPathFollowing()
{
	PathPoints.Empty();
	CurrentPathIndex = INDEX_NONE;
	SetActorTickEnabled(false);
}
