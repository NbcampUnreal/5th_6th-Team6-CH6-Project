#include "ItemSystem/BaseItemActor.h"
#include "ItemSystem/BaseItemData.h"
#include "ItemSystem/BaseInventoryComponent.h"
#include "Components/SphereComponent.h" // 추가

ABaseItemActor::ABaseItemActor()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    RootComponent = ItemMesh;

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(RootComponent);
    InteractionSphere->SetSphereRadius(150.f); // 캐릭터 습득 범위

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
    if (OtherActor && OtherActor != this)
    {
        APawn* OverlappedPawn = Cast<APawn>(OtherActor);
        if (OverlappedPawn)
        {
            PickupItem(OverlappedPawn);
        }
    }
}

void ABaseItemActor::PickupItem(APawn* InHandler)
{
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