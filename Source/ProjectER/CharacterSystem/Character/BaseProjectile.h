// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "BaseProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UProjectileData;

UCLASS()
class PROJECTER_API ABaseProjectile : public AActor
{
	GENERATED_BODY()

public:    
	ABaseProjectile();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void Tick(float DeltaTime) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
	
	UFUNCTION()
	void OnRep_ProjectileData();
	
	UFUNCTION()
	void OnRep_HomingTargetActor();
	
	void InitializeProjectile();
	
	// 충돌 처리 함수
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnImpactEffect(FVector Location);
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_ProjectileData, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Data")
	TObjectPtr<UProjectileData> ProjectileData;
	
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> SphereComp;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComp;
	
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UNiagaraComponent> ProjectileVFXComp;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<UNiagaraSystem> ImpactEffect; 
	
	UPROPERTY(BlueprintReadWrite, Meta = (ExposeOnSpawn = true))
	FGameplayEffectSpecHandle DamageEffectSpecHandle;

	UPROPERTY(ReplicatedUsing = OnRep_HomingTargetActor, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Combat")
	AActor* HomingTargetActor;
};
