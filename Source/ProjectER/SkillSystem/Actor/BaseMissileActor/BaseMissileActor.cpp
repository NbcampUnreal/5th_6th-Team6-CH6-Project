// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/BaseMissileActor/BaseMissileActor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SceneComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

ABaseMissileActor::ABaseMissileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = true;
	SetReplicateMovement(true);

	// 충돌체 없는 경량 루트
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 유도 비행용 무브먼트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	
	// 초기 속도를 로컬 좌표계가 아닌 월드 좌표계(절대 방향) 기준으로 해석하도록 설정 (곡선 비행 방지 핵심)
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
	
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void ABaseMissileActor::InitializeMissile(
	const TArray<FGameplayEffectSpecHandle>& InEffectSpecHandles,
	AActor* InInstigatorActor,
	AActor* InHomingTarget,
	const FGameplayCueParameters& InHitVfxCueParameters,
	const FGameplayCueParameters& InHitSoundCueParameters,
	float InInitialSpeed,
	float InMaxSpeed,
	float InHomingAcceleration,
	float InReachThreshold,
	bool bInDestroyOnHit,
	const FVector& InInitialDirection)
{
	EffectSpecHandles = InEffectSpecHandles;
	InstigatorActor = InInstigatorActor;
	HomingTargetActor = InHomingTarget;
	HitVfxCueParameters = InHitVfxCueParameters;
	HitSoundCueParameters = InHitSoundCueParameters;
	ReachThreshold = InReachThreshold;
	bDestroyOnHit = bInDestroyOnHit;

	if (IsValid(ProjectileMovement))
	{
		ProjectileMovement->InitialSpeed = InInitialSpeed;
		ProjectileMovement->MaxSpeed = InMaxSpeed;

		if (IsValid(HomingTargetActor))
		{
			ProjectileMovement->bIsHomingProjectile = true;
			ProjectileMovement->HomingTargetComponent = HomingTargetActor->GetRootComponent();
			ProjectileMovement->HomingAccelerationMagnitude = InHomingAcceleration;
		}

		// 초기 발사 방향으로 속도 설정 (FinishSpawning 전이므로 GetActorForwardVector() 대신 직접 전달받은 방향 사용)
		InitialTargetRotation = InInitialDirection.Rotation();
		ProjectileMovement->Velocity = InInitialDirection.GetSafeNormal() * InInitialSpeed;
	}
}

void ABaseMissileActor::BeginPlay()
{
	Super::BeginPlay();

	// FinishSpawning 내부의 ExecuteConstruction/InitializeComponent에 의해 회전값과 Velocity가 왜곡되는 것을 방지하기 위해,
	// Transform이 확정된 시점(BeginPlay)에서 우리가 저장한 절대 방향으로 다시 한번 강제 설정합니다.
	if (IsValid(ProjectileMovement) && ProjectileMovement->InitialSpeed > 0.f)
	{
		SetActorRotation(InitialTargetRotation);
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
	}
}

void ABaseMissileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버에서만 적중 판정
	if (bHasReached || !HasAuthority())
	{
		return;
	}

	if (!IsValid(HomingTargetActor))
	{
		// 타겟이 사라졌으면 파괴
		if (bDestroyOnHit)
		{
			Destroy();
		}
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), HomingTargetActor->GetActorLocation());
	if (Distance <= ReachThreshold)
	{
		OnReachedTarget();
	}
}

void ABaseMissileActor::OnReachedTarget()
{
	if (bHasReached)
	{
		return;
	}
	bHasReached = true;

	// 1. 타겟에 효과 적용
	ApplyEffectsToTarget(HomingTargetActor);

	// 2. 적중 효과 실행
	ExecuteHitCues();

	// 3. 파괴 처리
	if (bDestroyOnHit)
	{
		Destroy();
	}
}

void ABaseMissileActor::ApplyEffectsToTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetASC))
	{
		return;
	}

	for (const FGameplayEffectSpecHandle& Handle : EffectSpecHandles)
	{
		if (Handle.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Handle.Data.Get());
		}
	}
}

void ABaseMissileActor::ExecuteHitCues()
{
	if (!IsValid(InstigatorActor))
	{
		return;
	}

	UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	// VFX
	if (HitVfxCueParameters.OriginalTag.IsValid())
	{
		FGameplayCueParameters Params = HitVfxCueParameters;
		Params.Location = GetActorLocation();
		Params.EffectCauser = this;
		Params.TargetAttachComponent = IsValid(HomingTargetActor) ? HomingTargetActor->GetRootComponent() : nullptr;
		InstigatorASC->ExecuteGameplayCue(HitVfxCueParameters.OriginalTag, Params);
	}

	// Sound
	if (HitSoundCueParameters.OriginalTag.IsValid())
	{
		FGameplayCueParameters Params = HitSoundCueParameters;
		Params.Location = GetActorLocation();
		Params.EffectCauser = this;
		Params.TargetAttachComponent = IsValid(HomingTargetActor) ? HomingTargetActor->GetRootComponent() : nullptr;
		InstigatorASC->ExecuteGameplayCue(HitSoundCueParameters.OriginalTag, Params);
	}
}
