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
	FTransform FinalTransform = CalculateSpawnLocation(SpawnInstigator, SpawnConfig);
	UE_LOG(LogTemp, Warning, TEXT("FinalLocation At OnGameplayEffectExecuted : %s"), *FinalTransform.GetLocation().ToString());
	//FTransform SpawnTransform(SpawnConfig->RotationOffset, FinalLocation);

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

//FVector USummonRangeAtBone::CalculateSpawnLocation(const AActor* Instigator, const USummonRangeByBoneGECConfig* Config) const
//{
//	if (!Instigator) return FVector::ZeroVector;
//
//	// Instigator로부터 직접 월드를 가져옵니다.
//	UWorld* World = Instigator->GetWorld();
//	if (!World) return FVector::ZeroVector;
//
//	FVector BaseLocation = Instigator->GetActorLocation();
//	FRotator BaseRotation = Instigator->GetActorRotation();
//	DrawDebugBox(World, BaseLocation, FVector(20.f), FColor::Yellow, false, 5.0f, 0, 1.0f);
//
//	if (USkeletalMeshComponent* Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
//	{
//		Mesh->TickAnimation(0.f, false); // 애니메이션 로직 업데이트
//		Mesh->RefreshBoneTransforms();   // 실제 본 위치 계산 및 갱신
//		if (Mesh->DoesSocketExist(Config->BoneName))
//		{
//			BaseLocation = Mesh->GetSocketLocation(Config->BoneName);
//			BaseRotation = Mesh->GetSocketRotation(Config->BoneName);
//			DrawDebugBox(World, BaseLocation, FVector(15.f), BaseRotation.Quaternion(), FColor::Green, false, 5.0f, 0, 1.5f);
//		}
//	}
//
//	FVector FinalLocation = BaseLocation + BaseRotation.RotateVector(Config->LocationOffset);
//	FinalLocation.Z += Config->ZOffset;
//
//	FHitResult FloorHit;
//	FVector Start = FinalLocation + FVector(0.f, 0.f, 200.f); // 살짝 위에서 시작
//	FVector End = FinalLocation - FVector(0.f, 0.f, 1000.f);
//	if (GetWorld()->LineTraceSingleByChannel(FloorHit, Start, End, ECC_Visibility))
//	{
//		// 바닥을 찾았다면, 바닥 좌표에 약간의 오프셋(2cm 정도)을 더해 겹침 방지
//		FinalLocation = FloorHit.Location + FVector(0.f, 0.f, 2.f);
//	}
//
//	DrawDebugBox(World, FinalLocation, FVector(25.f), BaseRotation.Quaternion(), FColor::Red, false, 5.0f, 0, 2.0f);
//	DrawDebugLine(World, BaseLocation, FinalLocation, FColor::White, false, 5.0f, 0, 1.0f);
//	DrawDebugLine(World, Instigator->GetActorLocation(), BaseLocation, FColor::Blue, false, 5.0f, 0, 1.0f);
//
//	return FinalLocation;
//}

//FVector USummonRangeAtBone::CalculateSpawnLocation(const AActor* Instigator, const USummonRangeByBoneGECConfig* Config) const
//{
//	if (!Instigator || !Config) return FVector::ZeroVector;
//	UWorld* World = Instigator->GetWorld();
//
//	// 1. 기본 위치/회전 (본 정보가 없을 경우 대비)
//	FVector BaseLocation = Instigator->GetActorLocation();
//	FRotator BaseRotation = Instigator->GetActorRotation();
//
//	// 2. 메시에서 실제 본의 트랜스폼 가져오기
//	if (USkeletalMeshComponent* Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
//	{
//		if (Mesh->DoesSocketExist(Config->BoneName))
//		{
//			Mesh->TickAnimation(0.f, false); // 애니메이션 로직 업데이트
//			Mesh->RefreshBoneTransforms();   // 실제 본 위치 계산 및 갱신
//			BaseLocation = Mesh->GetSocketLocation(Config->BoneName);
//			BaseRotation = Mesh->GetSocketRotation(Config->BoneName);
//		}
//	}
//
//	// 3. [핵심] 최종 회전값 계산 (본의 회전 + 설정에 정의된 회전 오프셋)
//	// 두 회전값을 더하여 '최종적으로 바라보는 방향'을 구합니다.
//	FRotator CombinedRotation = BaseRotation + Config->RotationOffset;
//
//	// 4. 최종 회전이 적용된 방향으로 위치 오프셋 계산
//	// CombinedRotation에 의해 오프셋이 대각선 위나 아래로 꺾여도 이 시점엔 그대로 계산합니다.
//	FVector TargetLocation = BaseLocation + CombinedRotation.RotateVector(Config->LocationOffset);
//	TargetLocation.Z += Config->ZOffset;
//
//	// 5. 바닥 강제 고정 로직 (bSnapToGround가 켜져 있을 때)
//	if (Config->bSnapToGround)
//	{
//		FHitResult FloorHit;
//		// 회전 때문에 Z값이 너무 낮아질 수 있으므로 시작점은 넉넉히 위에서 잡습니다.
//		FVector TraceStart = TargetLocation;
//		TraceStart.Z += 300.f; // 현재 예상 위치보다 3미터 위
//
//		FVector TraceEnd = TargetLocation;
//		TraceEnd.Z -= 1000.f; // 아래로 10미터 스캔
//
//		FCollisionQueryParams Params;
//		Params.AddIgnoredActor(Instigator);
//
//		if (World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, Config->GroundTraceChannel, Params))
//		{
//			// X, Y는 회전이 반영된 위치를 유지하되, Z만 바닥 높이로 고정합니다.
//			TargetLocation.X = FloorHit.Location.X;
//			TargetLocation.Y = FloorHit.Location.Y;
//
//			// 바닥 높이 + (설정 높이) + (장판 두께의 절반)
//			float FinalZOffset = Config->ZOffset;
//			if (Config->bUseBoxExtentOffset)
//			{
//				FinalZOffset += (Config->CollisionRadius.Z + 1.0f); // 1.0f는 Z-Fighting 방지
//			}
//
//			TargetLocation.Z = FloorHit.Location.Z + FinalZOffset;
//		}
//	}
//
//	// 디버그 드로잉 (실제 바닥에 안착된 박스)
//	DrawDebugBox(World, TargetLocation, Config->CollisionRadius, CombinedRotation.Quaternion(), FColor::Red, false, 5.0f, 0, 2.0f);
//
//	return TargetLocation;
//}

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

			float FinalZOffset = Config->GroundOffset;
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
