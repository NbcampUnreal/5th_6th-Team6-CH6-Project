// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "BaseProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class PROJECTER_API ABaseProjectile : public AActor
{
	GENERATED_BODY()

public:    
	ABaseProjectile();
	
protected:
	virtual void BeginPlay() override;

public:
	// 발사체 설정 (속도, 생명주기 등)
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> SphereComp;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	// [핵심] GAS 효과를 담을 변수 (ExposeOnSpawn으로 생성 시 주입 가능하게)
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn = true))
	FGameplayEffectSpecHandle DamageEffectSpecHandle;

protected:
	// 충돌 처리 함수
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};
