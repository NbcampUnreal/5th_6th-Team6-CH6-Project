#include "ItemSystem/ItemTestCharacter.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"

AItemTestCharacter::AItemTestCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	Inventory = CreateDefaultSubobject<UBaseInventoryComponent>(TEXT("Inventory"));
}