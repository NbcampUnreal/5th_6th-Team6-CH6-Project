// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterSystem/Character/BaseProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/Interface/TargetableInterface.h"

ABaseProjectile::ABaseProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// 1. 충돌체 설정
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->InitSphereRadius(15.0f);
	SphereComp->SetCollisionProfileName(TEXT("Projectile")); // Projectile 프로필 필요 (Pawn과 Overlap)
	RootComponent = SphereComp;

	// 2. 무브먼트 설정
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->bRotationFollowsVelocity = true; // 날아가는 방향으로 회전
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 직선으로 날아가게 (필요 시 조절)

	bReplicates = true; // 서버에서 생성되어야 함
	
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &ABaseProjectile::OnOverlapBegin);
	SetLifeSpan(3.0f); // 3초 후 자동 삭제 (메모리 관리)
}

void ABaseProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 나 자신이나 이미 죽은 대상은 무시
	if (!OtherActor || OtherActor == GetInstigator()) return;

	// 아군 오사 방지 (Targetable Interface 활용)
	// ... (TeamID 확인 로직 추가 가능) ...

	// [핵심] 충돌 대상에게 GAS 효과 적용
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (TargetASC && DamageEffectSpecHandle.IsValid())
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data.Get());
	}

	// 이펙트 재생 및 소멸
	// UGameplayStatics::SpawnEmitterAtLocation(...);
	Destroy();
}


