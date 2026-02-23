// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/SphereOverlapEffectActor.h"
#include "Components/SphereComponent.h"

ASphereOverlapEffectActor::ASphereOverlapEffectActor()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(100.f);
	SetCollisionComponent(SphereComponent);
}

void ASphereOverlapEffectActor::ApplyCollisionSize(const FVector& InCollisionSize)
{
	if (!IsValid(SphereComponent))
	{
		return;
	}

	const float SphereRadius = InCollisionSize.GetMax();
	if (SphereRadius > 0.f)
	{
		SphereComponent->SetSphereRadius(SphereRadius);
	}
}

void ASphereOverlapEffectActor::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	if (GetWorld())
	{
		if (IsValid(SphereComponent))
		{
			const float SphereRadius = PendingCollisionSize.GetMax();
			DrawDebugSphere(GetWorld(), GetActorLocation(), SphereRadius, 16, FColor::Red, false, GetLifeSpan(), 0, 2.0f);
		}
	}
#endif
}
