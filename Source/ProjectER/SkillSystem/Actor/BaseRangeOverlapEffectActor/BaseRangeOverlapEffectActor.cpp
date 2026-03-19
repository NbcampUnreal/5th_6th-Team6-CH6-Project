// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "Components/ShapeComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "SkillSystem/Component/AreaPeriodicEffectComponent.h"
#include "UObject/Object.h"

// Sets default values
ABaseRangeOverlapEffectActor::ABaseRangeOverlapEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ABaseRangeOverlapEffectActor::InitializeEffectData(const TArray<FGameplayEffectSpecHandle>& InEffectSpecHandles, AActor* InInstigatorActor, const FVector& InCollisionSize, bool bInHitOncePerTarget, const UObject* InHitTargetCueSourceObject, const FGameplayCueParameters& InHitTargetCueParameters)
{
	EffectSpecHandles = InEffectSpecHandles;
	InstigatorActor = InInstigatorActor;
	SetInstigator(Cast<APawn>(InInstigatorActor));
	bHitOncePerTarget = bInHitOncePerTarget;
	HitTargetCueSourceObject = InHitTargetCueSourceObject;
	HitTargetCueParameters = InHitTargetCueParameters;

	PendingCollisionSize = InCollisionSize;
	bHasPendingCollisionSize = true;
	ApplyCollisionSize(PendingCollisionSize);
}

// Called when the game starts or when spawned
void ABaseRangeOverlapEffectActor::BeginPlay()
{
	Super::BeginPlay();
    
	if (HasAuthority() && IsValid(CollisionComponent))
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABaseRangeOverlapEffectActor::OnShapeBeginOverlap);
		CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &ABaseRangeOverlapEffectActor::OnShapeEndOverlap);
	}
}

void ABaseRangeOverlapEffectActor::ApplyCollisionSize(const FVector& InCollisionSize)
{
	//
}

void ABaseRangeOverlapEffectActor::SetCollisionComponent(UShapeComponent* InCollisionComponent)
{
	if (!IsValid(InCollisionComponent))
	{
		return;
	}

	// 1. 멤버 변수 할당
	CollisionComponent = InCollisionComponent;

	// 2. 물리 및 충돌 설정 (공통)
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);

	if (GetRootComponent() != CollisionComponent)
	{
		SetRootComponent(CollisionComponent);
	}

	if (CollisionComponent->IsRegistered())
	{
		CollisionComponent->UpdateBounds();
		CollisionComponent->MarkRenderStateDirty();
		CollisionComponent->UpdateBodySetup(); // 물리 모양 갱신
	}
}

void ABaseRangeOverlapEffectActor::OnShapeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValid(OtherActor) || OtherActor == this || OtherActor == InstigatorActor)
	{
		return;
	}

	ITargetableInterface* MyInstigatorTargetable = Cast<ITargetableInterface>(InstigatorActor);
	ITargetableInterface* OtherTargetable = Cast<ITargetableInterface>(OtherActor);
	if(!MyInstigatorTargetable || !OtherTargetable) return;
	if (MyInstigatorTargetable->GetTeamType() == OtherTargetable->GetTeamType()) {
		return;
	}

	// 주기적 효과가 설정되어 있다면 컴포넌트에 타겟 추가
	if (IsValid(AreaPeriodicComponent))
	{
		AreaPeriodicComponent->AddTarget(OtherActor);
		return;
	}

	if (bHitOncePerTarget && HitActors.Contains(OtherActor))
	{
		return;
	}

	ApplyEffectsToTarget(OtherActor);

	if (bHitOncePerTarget)
	{
		HitActors.Add(OtherActor);
	}

	if (bDestroyOnOverlap)
	{
		Destroy();
	}
}

void ABaseRangeOverlapEffectActor::OnShapeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (HasAuthority() && IsValid(OtherActor) && IsValid(AreaPeriodicComponent))
	{
		AreaPeriodicComponent->RemoveTarget(OtherActor);
	}
}

void ABaseRangeOverlapEffectActor::SetAreaPeriodicComponent(UAreaPeriodicEffectComponent* InComponent)
{
	if (IsValid(InComponent))
	{
		AreaPeriodicComponent = InComponent;
		AreaPeriodicComponent->OnAreaPeriodicTrigger.AddDynamic(this, &ABaseRangeOverlapEffectActor::OnAreaPeriodicTrigger);
	}
}

void ABaseRangeOverlapEffectActor::OnAreaPeriodicTrigger(const TArray<AActor*>& Targets)
{
	ApplyEffectsToTargets(Targets);
}

void ABaseRangeOverlapEffectActor::ApplyEffectsToTargets(const TArray<AActor*>& Targets)
{
	for (AActor* Target : Targets)
	{
		ApplyEffectsToTarget(Target);
	}
}

void ABaseRangeOverlapEffectActor::ApplyEffectsToTarget(AActor* TargetActor)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);

	if (!IsValid(InstigatorASC)) return;
	if (!IsValid(TargetASC)) return;
	if (EffectSpecHandles.Num() <= 0) return;

	for (const FGameplayEffectSpecHandle& EffectSpecHandle : EffectSpecHandles)
	{
		if (EffectSpecHandle.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*EffectSpecHandle.Data.Get(), TargetASC);
		}
	}

	if (HitTargetCueParameters.OriginalTag.IsValid())
	{
		FGameplayCueParameters CueParameters = HitTargetCueParameters;
		CueParameters.Location = TargetActor->GetActorLocation();
		CueParameters.EffectCauser = this;
		CueParameters.SourceObject = HitTargetCueSourceObject;
		{
			FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
			InstigatorASC->ExecuteGameplayCue(CueParameters.OriginalTag, CueParameters);
		}
	}
}

