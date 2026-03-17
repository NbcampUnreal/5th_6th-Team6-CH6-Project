#include "GameModeBase/Subsystem/Preload/ER_AssetPreloadSubsystem.h"
#include "Engine/AssetManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void UER_AssetPreloadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UER_AssetPreloadSubsystem::Deinitialize()
{
	// 에셋 로딩 핸들러 해제 및 배열 클리어
	if (AssetLoadHandle.IsValid())
	{
		AssetLoadHandle->CancelHandle();
		AssetLoadHandle.Reset();
	}
	PreloadedAssets.Empty();

	Super::Deinitialize();
}

void UER_AssetPreloadSubsystem::StartPreloadMonsterAssets()
{
	// 이미 로드 중이라면 무시
	if (AssetLoadHandle.IsValid() && AssetLoadHandle->IsLoadingInProgress())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Preload] Preload already in progress..."));
		return;
	}

	// 에셋 레지스트리 검색
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;

	// 검색할 폴더 경로 (유저 지정 경로)
	FARFilter Filter;
	Filter.PackagePaths.Add(FName("/Game/BCW/Monster/MonsterData"));
	// 하위 폴더까지 검색
	Filter.bRecursivePaths = true;

	// AssetRegistry에 요청해서 특정 폴더에 있는 UMonsterDataAsset 클래스를 상속받는 모든 에셋을 검색
	Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/ProjectER"), TEXT("MonsterDataAsset")));
	
	// 검색한 에셋들을 AssetDataList에 저장
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	if (AssetDataList.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Preload] No MonsterDataAssets found in /Game/BCW/Monster/MonsterData. Broadcasting complete immediately."));
		OnPreloadComplete.Broadcast();
		return;
	}

	// AssetDataList에 저장된 에셋들을 AssetsToLoad에 추가(경로 문자열)
	TArray<FSoftObjectPath> AssetsToLoad;
	for (const FAssetData& AssetData : AssetDataList)
	{
		AssetsToLoad.Add(AssetData.ToSoftObjectPath());
	}

	UE_LOG(LogTemp, Warning, TEXT("[Preload] Starting async load for %d monster assets..."), AssetsToLoad.Num());

	// StreamableManager를 통해 비동기 로딩을 요청
	// 로드 완료 시 OnMonsterAssetsLoadedAsync 실행
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	AssetLoadHandle = StreamableManager.RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateUObject(this, &UER_AssetPreloadSubsystem::OnMonsterAssetsLoadedAsync)
	);
}

void UER_AssetPreloadSubsystem::OnMonsterAssetsLoadedAsync()
{
	if (AssetLoadHandle.IsValid())
	{
		// 로드된 에셋들(MonsterDataAsset 및 내부 Mesh/Anim 등 연쇄 로드됨)을 배열에 넣어 GC 방지
		AssetLoadHandle->GetLoadedAssets(PreloadedAssets);
		UE_LOG(LogTemp, Warning, TEXT("[Preload] Monster Assets loading complete! Loaded %d assets. Ready to spawn"), PreloadedAssets.Num());
	}

	// 로딩 왼료 이벤트를 Broadcasting 하여 PlayerController 등에서 처리할 수 있게 함
	OnPreloadComplete.Broadcast();
}

void UER_AssetPreloadSubsystem::ShowLoadingScreen(TSubclassOf<UUserWidget> LoadingUIClass)
{
	if (!LoadingUIClass || LoadingUIInstance) return;

	LoadingUIInstance = CreateWidget<UUserWidget>(GetGameInstance(), LoadingUIClass);
	if (LoadingUIInstance)
	{
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->AddViewportWidgetContent(LoadingUIInstance->TakeWidget(), 100);
		}
	}
}

void UER_AssetPreloadSubsystem::HideLoadingScreen()
{
	if (!LoadingUIInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Preload] HideLoadingScreen: No active LoadingUIInstance to remove."));
		return;
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LoadingUIInstance->TakeWidget());
	}

	LoadingUIInstance = nullptr;
}
