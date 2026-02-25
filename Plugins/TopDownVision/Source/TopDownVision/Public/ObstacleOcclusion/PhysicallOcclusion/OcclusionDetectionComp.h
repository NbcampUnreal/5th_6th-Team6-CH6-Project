// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OcclusionDetectionComp.generated.h"

/*
 * This is a comp for making occlusion vision area using array of multiple sphere collision comps
 *
 *
 *
 *	this will handle the overlap and signal the 
 * 
 */

//ForwardDeclare
class USphereComponent;
class UOcclusionComp;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UOcclusionDetectionComp : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOcclusionDetectionComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;//for resetting the overlapped list

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	//Detection
	UFUNCTION()
	void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	//Private helper
private:
	void SetSphereColliderSetting(USphereComponent* SphereCollider);
	// in here, it will set the collision setting and bind the overlap events

protected:
	UPROPERTY()
	TArray<USphereComponent*> SphereComponents;// the array of sphere collisions

	//Overlapping counter
	TMap<TWeakObjectPtr<UPrimitiveComponent>, int32> OverlapCounter;//-> when the count 0-> definitely left the range
	// 1<=0 in the range

};
