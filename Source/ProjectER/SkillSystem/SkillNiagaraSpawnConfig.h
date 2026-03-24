// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SkillNiagaraSpawnSettings.h"
#include "SkillNiagaraSpawnConfig.generated.h"

class UNiagaraSystem;

/**
 * FSkillNiagaraSpawnSettings를 대체하는 UObject 기반 클래스.
 * EditInlineNew + Instanced로 에디터 인스펙터에서 직접 편집 가능.
 * SourceObject로 전달하여 GCN에서 태그 대조 없이 즉시 사용 가능.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API USkillNiagaraSpawnConfig : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Niagara")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Attach")
	bool bAttachToSource = false;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Attach", meta = (EditCondition = "bAttachToSource"))
	FName SocketOrBoneName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Transform")
	bool bUseSourceRotationForLocationOffset = true;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Transform")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Transform")
	ENiagaraSpawnRotationMode RotationMode = ENiagaraSpawnRotationMode::SourceRotationPlusOffset;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Transform")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Transform")
	FVector Scale = FVector::OneVector;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|GameplayCue", meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Parameters")
	TMap<FName, float> FloatParameters;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Parameters")
	TMap<FName, FVector> VectorParameters;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Parameters")
	TMap<FName, FLinearColor> ColorParameters;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void RefreshParameters();
#endif

	/** UObject 필드를 기존 USTRUCT로 변환 (Helper 호환용) */
	FSkillNiagaraSpawnSettings ToSettings() const
	{
		FSkillNiagaraSpawnSettings S;
		S.NiagaraSystem = NiagaraSystem;
		S.bAttachToSource = bAttachToSource;
		S.SocketOrBoneName = SocketOrBoneName;
		S.bUseSourceRotationForLocationOffset = bUseSourceRotationForLocationOffset;
		S.LocationOffset = LocationOffset;
		S.RotationMode = RotationMode;
		S.RotationOffset = RotationOffset;
		S.Scale = Scale;
		S.CueTag = CueTag;
		S.FloatParameters = FloatParameters;
		S.VectorParameters = VectorParameters;
		S.ColorParameters = ColorParameters;
		return S;
	}
};
