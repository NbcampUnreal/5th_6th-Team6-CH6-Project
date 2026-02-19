// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/SphereOverlapEffectActor.h"
#include "Components/SphereComponent.h"

ASphereOverlapEffectActor::ASphereOverlapEffectActor()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(100.f);
	SetCollisionComponent(SphereComponent);
}

void ASphereOverlapEffectActor::ApplyCollisionSize(float InCollisionSize)
{
	if (IsValid(SphereComponent) && InCollisionSize > 0.f)
	{
		SphereComponent->SetSphereRadius(InCollisionSize);
	}
}
