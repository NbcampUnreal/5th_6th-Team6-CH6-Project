// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/CapsuleRangeOverlapEffectActor.h"
#include "Components/CapsuleComponent.h"

ACapsuleRangeOverlapEffectActor::ACapsuleRangeOverlapEffectActor()
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(100.f, 200.f);
	SetCollisionComponent(CapsuleComponent);
}

void ACapsuleRangeOverlapEffectActor::ApplyCollisionSize(float InCollisionSize)
{
	if (IsValid(CapsuleComponent) && InCollisionSize > 0.f)
	{
		CapsuleComponent->SetCapsuleRadius(InCollisionSize);
		CapsuleComponent->SetCapsuleHalfHeight(InCollisionSize * 2.f);
	}
}