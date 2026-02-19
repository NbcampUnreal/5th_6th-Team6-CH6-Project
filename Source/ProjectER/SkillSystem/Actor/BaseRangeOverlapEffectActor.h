// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "BaseRangeOverlapEffectActor.generated.h"

class UShapeComponent;
class USphereComponent;
class UBoxComponent;
class UCapsuleComponent;

UCLASS()
class PROJECTER_API ABaseRangeOverlapEffectActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseRangeOverlapEffectActor();

	void InitializeEffectData(const TArray<FGameplayEffectSpecHandle>& InEffectSpecHandles, AActor* InInstigatorActor, float InCollisionSize, bool bInHitOncePerTarget);

protected:
	virtual void BeginPlay() override;
	virtual void ApplyCollisionSize(float InCollisionSize);
	void SetCollisionComponent(UShapeComponent* InCollisionComponent);

	UFUNCTION()
	void OnShapeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
public:	
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	TObjectPtr<UShapeComponent> CollisionComponent;

	UPROPERTY()
	TArray<FGameplayEffectSpecHandle> EffectSpecHandles;

	UPROPERTY()
	TObjectPtr<AActor> InstigatorActor;

	UPROPERTY()
	bool bHitOncePerTarget = true;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActors;
};
