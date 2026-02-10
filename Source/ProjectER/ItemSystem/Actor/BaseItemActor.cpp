#include "ItemSystem/Actor/BaseItemActor.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "Components/SphereComponent.h"

ABaseItemActor::ABaseItemActor()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    RootComponent = ItemMesh;

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(RootComponent);
    InteractionSphere->SetSphereRadius(150.f);

    InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
}

void ABaseItemActor::BeginPlay()
{
    Super::BeginPlay();

    if (InteractionSphere)
    {
        InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseItemActor::OnOverlapBegin);
    }
}

void ABaseItemActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    // 데이터가 있고, 타입이 Automatic일 때만 바로 습득 진행
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
        // Interaction 타입이라면 아무것도 하지 않음 (우클릭을 기다림)
    }
}

void ABaseItemActor::PickupItem(APawn* InHandler)
{

    if (!HasAuthority())
        return;

    if (!InHandler || !ItemData) return;

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