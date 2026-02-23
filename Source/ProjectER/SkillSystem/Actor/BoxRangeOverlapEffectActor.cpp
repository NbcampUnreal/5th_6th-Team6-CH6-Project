// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/BoxRangeOverlapEffectActor.h"
#include "Components/BoxComponent.h"

ABoxRangeOverlapEffectActor::ABoxRangeOverlapEffectActor()
{
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));

	// 이 한 줄로 변수 할당, 루트 설정, 프로필 설정이 모두 끝납니다.
	SetCollisionComponent(BoxComponent);

	// 초기 크기 설정
	BoxComponent->InitBoxExtent(FVector(50.f));
    BoxComponent->SetHiddenInGame(true);
}

void ABoxRangeOverlapEffectActor::ApplyCollisionSize(const FVector& InCollisionSize)
{
    if (!IsValid(BoxComponent)) return;
    BoxComponent->SetBoxExtent(InCollisionSize);
}

void ABoxRangeOverlapEffectActor::BeginPlay()
{
    Super::BeginPlay();

#if WITH_EDITOR
    if (GetWorld())
    {
        DrawDebugBox(GetWorld(), GetActorLocation(), PendingCollisionSize, GetActorQuat(), FColor::Magenta, false, GetLifeSpan(), 0, 2.0f);
    }
#endif
}