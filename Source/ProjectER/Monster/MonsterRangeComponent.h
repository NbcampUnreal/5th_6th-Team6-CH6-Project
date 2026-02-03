#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MonsterRangeComponent.generated.h"

class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerCountChanged);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTER_API UMonsterRangeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMonsterRangeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetPlayerCount(int32 Amount);
	int32 GetPlayerCount();

protected:
	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void OnPlayerCountingBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnPlayerCountingEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


public:

	UPROPERTY()
	FOnPlayerCountChanged OnPlayerCountOne;

	UPROPERTY()
	FOnPlayerCountChanged OnPlayerCountZero;

private:

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "MonsterRange",meta = (AllowprivateAccess = "true"))
	int32 PlayerCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterRange", meta = (AllowprivateAccess = "true"), meta = (ClampMin = "0.0"))
	float SphereRadius = 500.f;

	UPROPERTY(EditAnywhere, Category = "MonsterRange")
	bool Debug = false;

	TObjectPtr<USphereComponent> RangeSphere;

};