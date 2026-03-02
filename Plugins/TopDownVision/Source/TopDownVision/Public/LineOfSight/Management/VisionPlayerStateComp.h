// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineOfSight/VisionData.h"
#include "VisionPlayerStateComp.generated.h"

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionPlayerStateComp, Log, All);

UCLASS(ClassGroup=(Vision), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisionPlayerStateComp : public UActorComponent
{
	GENERATED_BODY()

public:
	UVisionPlayerStateComp();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// --- Team --- //

	UFUNCTION(BlueprintCallable, Category="Vision")
	void SetTeamChannel(EVisionChannel InTeam);

	UFUNCTION(BlueprintCallable, Category="Vision")
	EVisionChannel GetTeamChannel() const { return TeamChannel; }

	// --- All Reveal --- //

	/** Enable dead cam / spectator mode — reveals all actors regardless of team. */
	UFUNCTION(BlueprintCallable, Category="Vision")
	void SetAllReveal(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="Vision")
	bool IsAllReveal() const { return bAllReveal; }

	/** Returns true if this player can see targets belonging to the given team. */
	bool CanSeeTeam(EVisionChannel InTeam) const;

private:
	UPROPERTY(ReplicatedUsing=OnRep_TeamChannel)
	EVisionChannel TeamChannel = EVisionChannel::None;

	UPROPERTY(ReplicatedUsing=OnRep_AllReveal)
	bool bAllReveal = false;

	UFUNCTION()
	void OnRep_TeamChannel();

	UFUNCTION()
	void OnRep_AllReveal();
};