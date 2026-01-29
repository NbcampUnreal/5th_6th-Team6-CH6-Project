#include "ItemSystem/ItemTestCharacter.h"
#include "ItemSystem/BaseInventoryComponent.h"

AItemTestCharacter::AItemTestCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	Inventory = CreateDefaultSubobject<UBaseInventoryComponent>(TEXT("Inventory"));
}