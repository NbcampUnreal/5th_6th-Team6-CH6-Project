// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor.h"
#include "ProjectileBase.generated.h"

UCLASS()
class PROJECTER_API AProjectileBase : public ABaseRangeOverlapEffectActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectileBase();

	void SetProjectileMovement(float InitialSpeed, float GravityScale, bool bInDestroyOnHit);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnShapeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	bool bDestroyOnHit = true;
};
