#include "ItemSystem/BaseItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "ItemSystem/BaseItemData.h"

ABaseItemActor::ABaseItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 컴포넌트 생성 및 루트 설정
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;

	// 상호작용용 콜리전 (클릭 가능하도록)
	ItemMesh->SetCollisionProfileName(TEXT("UI"));
}

void ABaseItemActor::BeginPlay()
{
	Super::BeginPlay();

	// 데이터 에셋이 할당되어 있다면 메시를 자동으로 변경
	if (ItemData && !ItemData->ItemMesh.IsNull())
	{
		ItemMesh->SetStaticMesh(ItemData->ItemMesh.LoadSynchronous());
	}
}