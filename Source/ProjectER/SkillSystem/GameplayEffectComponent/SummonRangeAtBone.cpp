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
#include "Components/SkeletalMeshComponent.h"
#include "SkillSystem/SkillNiagaraSpawnHelper.h"

USummonRangeAtBone::USummonRangeAtBone()
{
	ConfigClass = USummonRangeByBoneGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> USummonRangeAtBone::GetRequiredConfigClass() const
{
	return USummonRangeByBoneGECConfig::StaticClass();
}

void USummonRangeAtBone::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

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
	FTransform FinalTransform = CalculateSpawnLocation(SpawnInstigator, SpawnConfig);
	if (UWorld* World = EffectCauser->GetWorld())
	{
		const FTransform SummonerTransform = EffectCauser->GetActorTransform();
		const FVector RangeSpawnLocation = FinalTransform.GetLocation();
		SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(World, SpawnConfig->SummonerSpawnVfx, SummonerTransform, EffectCauser, &RangeSpawnLocation);
		SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(World, SpawnConfig->RangeSpawnVfx, FinalTransform, nullptr, nullptr);
	}

	// [5] 액터 지연 스폰 및 초기화
	UWorld* World = EffectCauser->GetWorld();
	ABaseRangeOverlapEffectActor* RangeActor = World->SpawnActorDeferred<ABaseRangeOverlapEffectActor>(SpawnConfig->RangeActorClass, FinalTransform, EffectCauser, SpawnInstigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (IsValid(RangeActor))
	{
		InitializeRangeActor(RangeActor, SpawnConfig, EffectCauser, ContextHandle);
		RangeActor->FinishSpawning(FinalTransform);
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

FTransform USummonRangeAtBone::CalculateSpawnLocation(const AActor* Instigator, const USummonRangeByBoneGECConfig* Config) const
{
	if (!Instigator || !Config) return FTransform::Identity;
	UWorld* World = Instigator->GetWorld();

	// 1. 기준 위치/회전 초기값 (액터 기준)
	FVector BaseLocation = Instigator->GetActorLocation();
	FRotator BaseRotation = Instigator->GetActorRotation();

	// 2. 메시에서 실제 본의 트랜스폼 가져오기
	if (USkeletalMeshComponent* Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (Mesh->DoesSocketExist(Config->BoneName))
		{
			// 실시간 애니메이션 포즈 반영
			Mesh->TickAnimation(0.f, false);
			Mesh->RefreshBoneTransforms();
			BaseLocation = Mesh->GetSocketLocation(Config->BoneName);
			BaseRotation = Mesh->GetSocketRotation(Config->BoneName);
		}
	}

	// 3. 최종 회전값(CombinedRotation) 결정 로직
	FRotator CombinedRotation;

	if (Config->bSpawnZeroRotation)
	{
		// [순위 1] 무조건 월드 기준 0도
		CombinedRotation = FRotator::ZeroRotator;
	}
	else if (Config->bUseInstigatorRotation)
	{
		// [순위 2] 캐릭터가 바라보는 방향 기준
		CombinedRotation = Instigator->GetActorRotation();
	}
	else
	{
		// [순위 3] 본의 현재 회전 + 에디터에서 설정한 오프셋
		CombinedRotation = BaseRotation + Config->RotationOffset;
	}

	// 4. 결정된 회전 방향을 기반으로 위치 오프셋 계산
	FVector TargetLocation = BaseLocation + CombinedRotation.RotateVector(Config->LocationOffset);

	// 5. 지형 안착 로직
	if (Config->bSnapToGround)
	{
		FHitResult FloorHit;
		// TraceStartHeight만큼 위에서 아래로 스캔
		FVector TraceStart = TargetLocation;
		FVector TraceEnd = TargetLocation;
		TraceEnd.Z -= 1000.f;

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Instigator);

		if (World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, Config->GroundTraceChannel, Params))
		{
			// 수평 위치는 유지, 높이만 지형에 맞춤
			TargetLocation.X = FloorHit.Location.X;
			TargetLocation.Y = FloorHit.Location.Y;

			float FinalZOffset = Config->FloatingHeight;
			if (Config->bUseBoxExtentOffset)
			{
				// 박스 절반 높이만큼 들어올려 바닥에 안착
				FinalZOffset += Config->CollisionRadius.Z;
			}

			TargetLocation.Z = FloorHit.Location.Z + FinalZOffset;
		}
	}

	// 디버그 드로잉 (최종 결과물 확인용)
	DrawDebugBox(World, TargetLocation, Config->CollisionRadius, CombinedRotation.Quaternion(), FColor::Red, false, 5.0f, 0, 2.0f);

	// 6. 계산된 트랜스폼 반환
	return FTransform(CombinedRotation, TargetLocation);
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
		RangeActor->InitializeEffectData(InitGEHandles, Causer, Config->CollisionRadius, Config->bHitOncePerTarget, Config->HitTargetVfx);
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
