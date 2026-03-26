#include "ItemSystem/Actor/BaseItemActor.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ABaseItemActor::ABaseItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(150.f);

	ApplyWorldItemCollisionSettings();
}

void ABaseItemActor::BeginPlay()
{
	Super::BeginPlay();

	RefreshVisualFromItemData();

	if (InteractionSphere)
	{
		InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseItemActor::OnOverlapBegin);
	}
}

void ABaseItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseItemActor, ItemData);
}

void ABaseItemActor::InitializeFromItemData(UBaseItemData* InItemData, APawn* InDropperPawn)
{
	if (!HasAuthority())
	{
		return;
	}

	ItemData = InItemData;
	LastDropperPawn = InDropperPawn;
	DroppedAtTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	RefreshVisualFromItemData();
	ForceNetUpdate();
}

void ABaseItemActor::OnRep_ItemData()
{
	RefreshVisualFromItemData();
}

void ABaseItemActor::RefreshVisualFromItemData()
{
	if (!ItemMesh)
	{
		return;
	}

	if (ItemData && !ItemData->ItemMesh.IsNull())
	{
		if (UStaticMesh* Mesh = ItemData->ItemMesh.LoadSynchronous())
		{
			ItemMesh->SetStaticMesh(Mesh);

			// [중요]
			// 메시 에셋 자체에 박혀 있는 Collision 설정이 다시 살아날 수 있으므로
			// 메시를 갈아끼운 뒤에도 월드 드랍 아이템용 충돌 설정을 다시 강제 적용
			ApplyWorldItemCollisionSettings();
			return;
		}
	}

	ItemMesh->SetStaticMesh(nullptr);
	ApplyWorldItemCollisionSettings();
}

void ABaseItemActor::ApplyWorldItemCollisionSettings()
{
	// -------------------------------------------------
	// ItemMesh:
	// 실제 바닥 충돌/길막/끼임을 만들던 원인이므로 완전히 비활성화
	// -------------------------------------------------
	if (ItemMesh)
	{
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetGenerateOverlapEvents(false);
		ItemMesh->CanCharacterStepUpOn = ECB_No;
		ItemMesh->SetCanEverAffectNavigation(false);
	}

	// -------------------------------------------------
	// InteractionSphere:
	// 1) 자동 줍기용 Pawn Overlap 유지
	// 2) 수동 클릭/마우스 hover용 Visibility Block 유지
	// 3) 물리 충돌은 하지 않음(QueryOnly)
	// -------------------------------------------------
	if (InteractionSphere)
	{
		InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
		InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		InteractionSphere->SetGenerateOverlapEvents(true);
		InteractionSphere->CanCharacterStepUpOn = ECB_No;
		InteractionSphere->SetCanEverAffectNavigation(false);
	}
}

void ABaseItemActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor == LastDropperPawn)
	{
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		if (Now - DroppedAtTime < InitialPickupIgnoreTime)
		{
			return;
		}
	}

	if (OtherActor && OtherActor != this && ItemData)
	{
		if (ItemData->PickupType == EItemPickupType::Automatic)
		{
			APawn* OverlappedPawn = Cast<APawn>(OtherActor);
			if (OverlappedPawn)
			{
				PickupItem(OverlappedPawn);
			}
		}
	}
}

void ABaseItemActor::PickupItem(APawn* InHandler)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!InHandler || !ItemData)
	{
		return;
	}

	UBaseInventoryComponent* Inventory = InHandler->FindComponentByClass<UBaseInventoryComponent>();
	if (Inventory)
	{
		if (Inventory->AddItem(ItemData))
		{
			UE_LOG(LogTemp, Warning, TEXT("Item Picked Up: %s"), *ItemData->ItemName.ToString());
			Destroy();
		}
	}
}