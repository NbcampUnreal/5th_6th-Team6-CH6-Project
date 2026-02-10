#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ER_GameState.generated.h"


class AER_PlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedBP, int32, NewPhase);

UCLASS()
class PROJECTER_API AER_GameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void BuildTeamCache();
	void RemoveTeamCache();

	TArray<TWeakObjectPtr<AER_PlayerState>>& GetTeamArray(int32 TeamIdx);

	bool GetTeamEliminate(int32 idx);

	int32 GetLastTeamIdx();

	UFUNCTION()
	void OnRep_Phase();

	float GetPhaseRemainingTime() const;

	UFUNCTION(BlueprintCallable)
	int32 GetCurrentPhase() { return CurrentPhase; }

	UFUNCTION(BlueprintCallable)
	void SetCurrentPhase(int32 input) { CurrentPhase = input; }


public:
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, bool> TeamElimination;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Phase)
	float PhaseServerTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated)
	float PhaseDuration = 10.f;

	UPROPERTY(BlueprintAssignable)
	FOnPhaseChangedBP OnPhaseChanged;

private:
	TArray<TArray<TWeakObjectPtr<AER_PlayerState>>> TeamCache;

	UPROPERTY(ReplicatedUsing = OnRep_Phase)
	int32 CurrentPhase = 0;

};

