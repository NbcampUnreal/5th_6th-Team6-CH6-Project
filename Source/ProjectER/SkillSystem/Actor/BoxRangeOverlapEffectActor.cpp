// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/BoxRangeOverlapEffectActor.h"
#include "Components/BoxComponent.h"

ABoxRangeOverlapEffectActor::ABoxRangeOverlapEffectActor()
{
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->InitBoxExtent(FVector(100.f));
	SetCollisionComponent(BoxComponent);
}

void ABoxRangeOverlapEffectActor::ApplyCollisionSize(float InCollisionSize)
{
	if (IsValid(BoxComponent) && InCollisionSize > 0.f)
	{
		BoxComponent->SetBoxExtent(FVector(InCollisionSize));
	}
}