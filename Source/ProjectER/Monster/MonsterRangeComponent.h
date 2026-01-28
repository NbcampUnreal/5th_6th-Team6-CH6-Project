#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MonsterRangeComponent.generated.h"

class USphereComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTER_API UMonsterRangeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMonsterRangeComponent();

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


private:

	UPROPERTY(EditAnywhere, Category = "MonsterRange")
	bool Debug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MonsterRange", meta = (AllowprivateAccess = "true"))
	float SphereRadius = 500.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MonsterRange",meta = (AllowprivateAccess = "true"))
	int32 PlayerCount = 0;

	TObjectPtr<USphereComponent> RangeSphere;
};