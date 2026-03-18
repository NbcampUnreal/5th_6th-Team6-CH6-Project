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
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void ABaseMissileActor::InitializeMissile(
	const TArray<FGameplayEffectSpecHandle>& InEffectSpecHandles,
	AActor* InInstigatorActor,
	AActor* InHomingTarget,
	const FGameplayCueParameters& InHitCueParameters,
	float InInitialSpeed,
	float InMaxSpeed,
	float InHomingAcceleration,
	float InReachThreshold,
	bool bInDestroyOnHit)
{
	EffectSpecHandles = InEffectSpecHandles;
	InstigatorActor = InInstigatorActor;
	HomingTargetActor = InHomingTarget;
	HitCueParameters = InHitCueParameters;
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

		// 초기 발사 방향으로 속도 설정
		ProjectileMovement->Velocity = GetActorForwardVector() * InInitialSpeed;
	}
}

void ABaseMissileActor::BeginPlay()
{
	Super::BeginPlay();
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

	// 2. 적중 VFX 실행
	ExecuteHitVfx();

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

void ABaseMissileActor::ExecuteHitVfx()
{
	if (!IsValid(InstigatorActor) || !HitCueParameters.OriginalTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	FGameplayCueParameters Params = HitCueParameters;
	Params.Location = GetActorLocation();
	Params.EffectCauser = this;
	InstigatorASC->ExecuteGameplayCue(HitCueParameters.OriginalTag, Params);
}
