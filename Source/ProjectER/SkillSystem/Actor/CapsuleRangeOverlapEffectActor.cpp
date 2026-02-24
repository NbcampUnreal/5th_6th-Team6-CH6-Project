// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/CapsuleRangeOverlapEffectActor.h"
#include "Components/CapsuleComponent.h"

ACapsuleRangeOverlapEffectActor::ACapsuleRangeOverlapEffectActor()
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(100.f, 200.f);
	SetCollisionComponent(CapsuleComponent);
}

void ACapsuleRangeOverlapEffectActor::ApplyCollisionSize(const FVector& InCollisionSize)
{
	if (!IsValid(CapsuleComponent))
	{
		return;
	}

	const float CapsuleRadius = FMath::Max(InCollisionSize.X, InCollisionSize.Y);
	const float CapsuleHalfHeight = InCollisionSize.Z;
	if (CapsuleRadius > 0.f && CapsuleHalfHeight > 0.f)
	{
		CapsuleComponent->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	}
}

void ACapsuleRangeOverlapEffectActor::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	if (GetWorld())
	{
		if (IsValid(CapsuleComponent))
		{
			float CapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
			float CapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
			DrawDebugCapsule(GetWorld(), GetActorLocation(), CapsuleHalfHeight, CapsuleRadius, GetActorQuat(), FColor::Cyan, false, GetLifeSpan(), 0, 2.0f);
		}
	}
#endif
}
