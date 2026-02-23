// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/BaseRangeOverlapEffectActor.h"
#include "Components/ShapeComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/Interface/TargetableInterface.h"


// Sets default values
ABaseRangeOverlapEffectActor::ABaseRangeOverlapEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ABaseRangeOverlapEffectActor::InitializeEffectData(const TArray<FGameplayEffectSpecHandle>& InEffectSpecHandles, AActor* InInstigatorActor, const FVector& InCollisionSize, bool bInHitOncePerTarget)
{
	EffectSpecHandles = InEffectSpecHandles;
	InstigatorActor = InInstigatorActor;
	bHitOncePerTarget = bInHitOncePerTarget;

	PendingCollisionSize = InCollisionSize;
	bHasPendingCollisionSize = true;
	ApplyCollisionSize(PendingCollisionSize);
}

//void ABaseRangeOverlapEffectActor::InitializeEffectData(const TArray<FGameplayEffectSpecHandle>& InEffectSpecHandles, AActor* InInstigatorActor, bool bInHitOncePerTarget)
//{
//	EffectSpecHandles = InEffectSpecHandles;
//	InstigatorActor = InInstigatorActor;
//	bHitOncePerTarget = bInHitOncePerTarget;
//	bHasPendingCollisionSize = true;
//}

//void ABaseRangeOverlapEffectActor::SetCollisionSize(const FVector& InCollisionSize)
//{
//	ApplyCollisionSize(InCollisionSize);
//}

// Called when the game starts or when spawned
void ABaseRangeOverlapEffectActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && IsValid(CollisionComponent))
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABaseRangeOverlapEffectActor::OnShapeBeginOverlap);
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

	if (bHitOncePerTarget && HitActors.Contains(OtherActor))
	{
		return;
	}

	if (ITargetableInterface* MyInstigatorTargetable = Cast<ITargetableInterface>(InstigatorActor))
	{
		if (ITargetableInterface* OtherTargetable = Cast<ITargetableInterface>(OtherActor))
		{
			if (MyInstigatorTargetable->GetTeamType() == OtherTargetable->GetTeamType())
			{
				return;
			}
		}
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!IsValid(TargetASC) || EffectSpecHandles.Num() <= 0)
	{
		return;
	}

	for (const FGameplayEffectSpecHandle& EffectSpecHandle : EffectSpecHandles)
	{
		if (EffectSpecHandle.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		}
	}

	if (bHitOncePerTarget)
	{
		HitActors.Add(OtherActor);
	}
}

