#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "ER_AssetPreloadSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPreloadCompleteDelegate);

/**
 * 게임 인스턴스 전역에서 에셋 비동기 로드 및 메모리 유지를 담당하는 서브시스템.
 * 몬스터 메쉬가 끊김 없이 스폰되도록 미리 메모리에 올립니다.
 */
UCLASS()
class PROJECTER_API UER_AssetPreloadSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 서브시스템 초기화 및 해제
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * 특정 경로(예: MonsterData)의 에셋들을 스캔하고 비동기 로딩을 시작합니다.
	 * 서버/클라이언트 양쪽에서 호출 가능하며, 주로 클라이언트의 접속 지연을 해결하기 위해 씁니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectER|Preload")
	void StartPreloadMonsterAssets();

	// 로드가 완료되었을 때 알림을 보낼 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "ProjectER|Preload")
	FOnPreloadCompleteDelegate OnPreloadComplete;

private:
	// 비동기 로딩 핸들러 콜백
	void OnMonsterAssetsLoadedAsync();

protected:
	// 활성화된 비동기 로딩 핸들
	TSharedPtr<FStreamableHandle> AssetLoadHandle;

	// 가비지 컬렉션(GC)을 방지하기 위해 로드된 객체들을 들고 있는 배열
	UPROPERTY()
	TArray<UObject*> PreloadedAssets;
};
