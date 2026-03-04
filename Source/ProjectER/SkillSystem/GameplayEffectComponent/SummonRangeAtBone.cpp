// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/GameAbility/SkillBase.h"

USummonRangeAtBone::USummonRangeAtBone()
{
	ConfigClass = USummonRangeByBoneGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> USummonRangeAtBone::GetRequiredConfigClass() const
{
	return USummonRangeByBoneGECConfig::StaticClass();
}

void USummonRangeAtBone::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	AActor* EffectCauser = ContextHandle.GetEffectCauser();
	if (!IsValid(EffectCauser) || !EffectCauser->HasAuthority()) return;

	// [2] 설정 데이터(Config) 추출
	const USummonRangeByBoneGECConfig* SpawnConfig = GetSpawnConfig(GESpec);
	if (!IsValid(SpawnConfig) || !IsValid(SpawnConfig->RangeActorClass)) return;

	// [3] 인스티게이터 확인
	APawn* SpawnInstigator = Cast<APawn>(ContextHandle.GetInstigator());
	if (!IsValid(SpawnInstigator)) return;

	// [4] 트랜스폼 계산
	FVector FinalLocation = CalculateSpawnLocation(SpawnInstigator, SpawnConfig);
	FTransform SpawnTransform(SpawnConfig->SpawnRotation, FinalLocation);

	// [5] 액터 지연 스폰 및 초기화
	UWorld* World = EffectCauser->GetWorld();
	ABaseRangeOverlapEffectActor* RangeActor = World->SpawnActorDeferred<ABaseRangeOverlapEffectActor>(SpawnConfig->RangeActorClass, SpawnTransform, EffectCauser, SpawnInstigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (IsValid(RangeActor))
	{
		InitializeRangeActor(RangeActor, SpawnConfig, EffectCauser, ContextHandle);
		RangeActor->FinishSpawning(SpawnTransform);
	}
}

const USummonRangeByBoneGECConfig* USummonRangeAtBone::GetSpawnConfig(const FGameplayEffectSpec& GESpec) const
{
	const USkillEffectDataAsset* SkillDataAsset = Cast<USkillEffectDataAsset>(GESpec.GetEffectContext().GetSourceObject());
	if (!IsValid(SkillDataAsset)) return nullptr;

	const FGameplayTag IndexTag = SkillDataAsset->GetIndexTag();
	const int32 DataIndex = FMath::RoundToInt(GESpec.GetSetByCallerMagnitude(IndexTag, false, -1.f));

	const FSkillEffectContainer& SkillContainer = SkillDataAsset->GetData();
	if (!SkillContainer.SkillEffectDefinition.IsValidIndex(DataIndex)) return nullptr;

	return Cast<USummonRangeByBoneGECConfig>(SkillContainer.SkillEffectDefinition[DataIndex].Config);
}

FVector USummonRangeAtBone::CalculateSpawnLocation(const AActor* Instigator, const USummonRangeByBoneGECConfig* Config) const
{
	FVector BaseLocation = Instigator->GetActorLocation();
	FRotator BaseRotation = Instigator->GetActorRotation();

	if (USkeletalMeshComponent* Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (Mesh->DoesSocketExist(Config->BoneName))
		{
			BaseLocation = Mesh->GetSocketLocation(Config->BoneName);
			BaseRotation = Mesh->GetSocketRotation(Config->BoneName);
		}
	}

	FVector FinalLocation = BaseLocation + BaseRotation.RotateVector(Config->LocationOffset);
	FinalLocation.Z += Config->ZOffset;

	return FinalLocation;
}

void USummonRangeAtBone::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeByBoneGECConfig* Config, AActor* Causer, const FGameplayEffectContextHandle& Context) const
{
	UAbilitySystemComponent* CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Causer);
	USkillBase* NonConstSkill = const_cast<USkillBase*>(Cast<USkillBase>(Context.GetAbility()));

	if (CauserASC && NonConstSkill)
	{
		TArray<FGameplayEffectSpecHandle> InitGEHandles;
		for (USkillEffectDataAsset* SkillEffectDataAsset : Config->Applied)
		{
			if (IsValid(SkillEffectDataAsset))
			{
				InitGEHandles.Append(SkillEffectDataAsset->MakeSpecs(CauserASC, NonConstSkill, Causer, Context));
			}
		}
		RangeActor->InitializeEffectData(InitGEHandles, Causer, Config->CollisionRadius, Config->bHitOncePerTarget);
		RangeActor->SetLifeSpan(Config->LifeSpan);
	}
}

FText USummonRangeByBoneGECConfig::BuildTooltipDescription(float InLevel) const
{
	TArray<FString> AppliedDescriptions;

	for (const USkillEffectDataAsset* SkillEffectDataAsset : Applied)
	{
		if (!IsValid(SkillEffectDataAsset))
		{
			continue;
		}

		const FString Desc = SkillEffectDataAsset->BuildEffectDescription(InLevel).ToString();
		if (!Desc.IsEmpty())
		{
			AppliedDescriptions.Add(Desc);
		}
	}

	if (AppliedDescriptions.IsEmpty())
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Join(AppliedDescriptions, TEXT("\n")));
}
