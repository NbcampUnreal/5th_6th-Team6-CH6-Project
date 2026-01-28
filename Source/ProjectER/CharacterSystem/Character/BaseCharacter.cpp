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
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"
#include "DrawDebugHelpers.h"

#include "UI/UI_HUDFactory.h" // UI시스템 관리자
#include "Components/SceneCaptureComponent2D.h" // 미니맵용
ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));

	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CurrentPathIndex = INDEX_NONE;

	bReplicates = true;
	SetReplicateMovement(true);

	// 26.01.29. mpyi
	// 미니맵을 위한 씬 컴포넌트 2D <- 차후 '카메라' 시스템으로 이동할 예정
	MinimapCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCaptureComponent"));
	MinimapCaptureComponent->SetupAttachment(RootComponent);

	// 미니맵 캡처 기본 설정
	MinimapCaptureComponent->SetAbsolute(false, true, false); // 순서대로: 위치, 회전, 스케일
	// 위치는 캐릭터를 따라다녀야 함으로 앱솔루트 ㄴㄴ
	MinimapCaptureComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1000.0f));
	MinimapCaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	MinimapCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	MinimapCaptureComponent->OrthoWidth = 2048.0f; // 이거로 미니맵 확대/축소 조절

	/// 최적화 필요시 아래 플래그 조절해가면서 해결해 보기
	//MinimapCaptureComponent->ShowFlags.SetDynamicShadows(false); // 동적 그림자
	//MinimapCaptureComponent->ShowFlags.SetGlobalIllumination(false); // 루멘
	//MinimapCaptureComponent->ShowFlags.SetMotionBlur(false); // 잔상 제거용
	//MinimapCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_BaseColor; // 포스트 프로세싱 무효화
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorTickEnabled(false);
	
	// 클라이언트 초기화
	if (HeroData) 
	{
		InitVisuals();
	}

}
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/// UI TEST를 위함 지우진 말아주세요!! {차후 제거할 예정}
	/// mpyi
	//{
	//	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	//	UBaseAttributeSet* AS = Cast<UBaseAttributeSet>(GetPlayerState<ABasePlayerState>()->GetAttributeSet());
	//	if (ASC && AS) // 둘 다 존재해야 함
	//	{
	//		ASC->ApplyModToAttribute(AS->GetHealthAttribute(), EGameplayModOp::Additive, -1.0f);
	//		ASC->ApplyModToAttribute(AS->GetStaminaAttribute(), EGameplayModOp::Additive, -1.0f);
	//		ASC->ApplyModToAttribute(AS->GetLevelAttribute(), EGameplayModOp::Additive, 1.0f);
	//		ASC->ApplyModToAttribute(AS->GetAttackPowerAttribute(), EGameplayModOp::Additive, 1.0f);
	//		ASC->ApplyModToAttribute(AS->GetSkillAmpAttribute(), EGameplayModOp::Additive, 1.0f);
	//		ASC->ApplyModToAttribute(AS->GetAttackSpeedAttribute(), EGameplayModOp::Additive, 1.0f);
	//		ASC->ApplyModToAttribute(AS->GetDefenseAttribute(), EGameplayModOp::Additive, 1.0f);
	//		ASC->ApplyModToAttribute(AS->GetXPAttribute(), EGameplayModOp::Additive, 1.0f);
	//		
	//		
	//		// float CurrentHP = AS->GetHealth();
	//		// UE_LOG(LogTemp, Warning, TEXT("명령 전송 완료 | 현재 실제 HP 수치: %f"), CurrentHP);
	//	}
	//	else
	//	{
	//		UE_LOG(LogTemp, Error, TEXT("ASC(%s) 또는 AS(%s)가 NULL입니다!"),
	//			ASC ? TEXT("OK") : TEXT("NULL"),
	//			AS ? TEXT("OK") : TEXT("NULL"));
	//	}
	//}

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

	// UI 초기화
	InitUI();
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
	// UI 초기화
	InitUI();
}

float ABaseCharacter::GetCharacterLevel() const
{
	if (const UBaseAttributeSet* BaseSet = GetPlayerState<ABasePlayerState>() ? GetPlayerState<ABasePlayerState>()->GetAttributeSet() : nullptr)
	{
		return BaseSet->GetLevel();
	}

	return 1.0f; // 기본값 (1레벨 시작)
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
		/* // Ability 부여
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
		}*/
        
		// Attribute Set 초기화
		InitAttributes(); 
		
		UE_LOG(LogTemp, Warning, TEXT("Initialize Attribute."));
	}
}

void ABaseCharacter::InitAttributes()
{
	if (!AbilitySystemComponent.IsValid() || !HeroData || !InitStatusEffectClass) return;
	
	// 커브 테이블 로드
	UCurveTable* CurveTable = HeroData->StatCurveTable.LoadSynchronous();
	if (!CurveTable) return;
	
	float Level = GetCharacterLevel();
	if (Level <= 0.f) Level = 1.0f;
	
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(InitStatusEffectClass, Level, Context);
    
	if (SpecHandle.IsValid())
	{
		auto SetStat = [&](FGameplayTag AttributeTag, FString RowSuffix)
		{
			FName RowName = FName(*HeroData->StatusRowName.ToString().Append(RowSuffix));
			
			FRealCurve* Curve = CurveTable->FindCurve(RowName, FString());
			if (Curve)
			{
				float Value = Curve->Eval(Level);
                
				// GE Spec 값 주입 (SetByCaller)
				SpecHandle.Data->SetSetByCallerMagnitude(AttributeTag, Value);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Curve Row Not Found: %s"), *RowName.ToString());
			}
		};
		
		// 레벨 설정
		SpecHandle.Data->SetSetByCallerMagnitude(ProjectER::Status::Level, Level);
		
		// 스탯별 값 주입 실행
		SetStat(ProjectER::Status::MaxLevel,   "_MaxLevel");
		SetStat(ProjectER::Status::MaxXP,   "_MaxXp");
		SetStat(ProjectER::Status::MaxHealth,   "_MaxHealth");
		SetStat(ProjectER::Status::HealthRegen, "_HealthRegen");
		SetStat(ProjectER::Status::MaxStamina,  "_MaxStamina");
		SetStat(ProjectER::Status::StaminaRegen,  "_StaminaRegen");
		SetStat(ProjectER::Status::AttackPower, "_AttackPower");
		SetStat(ProjectER::Status::AttackSpeed, "_AttackSpeed");
		SetStat(ProjectER::Status::SkillAmp, "_SkillAmp");
		SetStat(ProjectER::Status::CritChance, "_CritChance");
		SetStat(ProjectER::Status::CritDamage, "_CritDamage");
		SetStat(ProjectER::Status::Defense,     "_Defense");
		SetStat(ProjectER::Status::MoveSpeed,   "_MoveSpeed");
		SetStat(ProjectER::Status::CooldownReduction,   "_CooldownReduction");
		SetStat(ProjectER::Status::Tenacity,   "_Tenacity");
		
		// 적용
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
}

void ABaseCharacter::InitVisuals()
{
	if (!HeroData) return;

	// 스켈레탈 메시 로드 및 설정
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

void ABaseCharacter::Server_StopMove_Implementation()
{
	StopMove();
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

void ABaseCharacter::StopMove()
{
	StopPathFollowing();
	
	GetCharacterMovement()->StopMovementImmediately();
	
	if (!HasAuthority())
	{
		Server_StopMove();
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
	
	if (!Direction.IsNearlyZero())
	{
		FVector NormalDirection = Direction.GetSafeNormal();
        
		// 캐릭터 이동 입력 
		AddMovementInput(NormalDirection, 1.0f);

		// 캐릭터 회전
		FRotator TargetRot = NormalDirection.Rotation();
		FRotator CurrentRot = GetActorRotation();
		
		FRotator NewRotation = FMath::RInterpTo(CurrentRot, TargetRot, GetWorld()->GetDeltaSeconds(), 20.0f);
        
		SetActorRotation(NewRotation);
	}
	
	FRotator MyRot = GetActorRotation();
	UE_LOG(LogTemp, Warning, TEXT("Rotation Check -> Pitch: %f | Yaw: %f"), MyRot.Pitch, MyRot.Yaw);
	
#if WITH_EDITOR
	// [디버깅] 경로 및 이동 방향 시각화
	if (bShowMovementDebug)
	{
		// 전체 경로 그리기 (초록색 선)
		for (int32 i = 0; i < PathPoints.Num() - 1; ++i)
		{
			DrawDebugLine(
				GetWorld(),
				PathPoints[i],
				PathPoints[i + 1],
				FColor::Green,
				false, -1.0f, 0, 3.0f // 두께 3.0
			);
		}

		// 현재 목표 지점 (빨간색 구체)
		if (PathPoints.IsValidIndex(CurrentPathIndex))
		{
			DrawDebugSphere(
				GetWorld(),
				PathPoints[CurrentPathIndex],
				30.0f, // 반지름
				12,
				FColor::Red,
				false, -1.0f, 0, 2.0f
			);
            
			// 내 위치에서 목표 지점까지 연결선 (노란색 점선)
			DrawDebugLine(
				GetWorld(),
				GetActorLocation(),
				PathPoints[CurrentPathIndex],
				FColor::Yellow,
				false, -1.0f, 0, 1.5f
			);
		}

		// 실제 이동 방향 (파란색 화살표)
		FVector Velocity = GetVelocity();
		if (!Velocity.IsNearlyZero())
		{
			DrawDebugDirectionalArrow(
				GetWorld(),
				GetActorLocation(),
				GetActorLocation() + Velocity.GetSafeNormal() * 100.0f, // 1m 길이
				50.0f, // 화살표 크기
				FColor::Blue,
				false, -1.0f, 0, 5.0f // 두께
			);
		}
	}
#endif
}

void ABaseCharacter::StopPathFollowing()
{
	PathPoints.Empty();
	CurrentPathIndex = INDEX_NONE;
	SetActorTickEnabled(false);
}

void ABaseCharacter::InitUI()
{

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (IsValid(PC) && PC->IsLocalController())
	{
		AHUD* GenericHUD = PC->GetHUD();
		if (GenericHUD == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("!!! HUD NONE CREATED!!!"));
			return;
		}

		if (AUI_HUDFactory* HUD = Cast<AUI_HUDFactory>(GenericHUD))
		{
			HUD->InitOverlay(PC, GetPlayerState(), GetAbilitySystemComponent(), GetPlayerState<ABasePlayerState>()->GetAttributeSet());
			HUD->InitMinimapComponent(MinimapCaptureComponent);
			UE_LOG(LogTemp, Warning, TEXT("HUD InitOverlay Success!"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("!!! HUD Casting Fail! address : %s !!!"), *GenericHUD->GetName());
		}

	}

	/// 미니맵 설정
	if (!IsLocallyControlled())
	{
		/// '나' 이외는 캡쳐 컴포넌트를 꺼서 성능 최적화~
		MinimapCaptureComponent->Deactivate();
	}
}
