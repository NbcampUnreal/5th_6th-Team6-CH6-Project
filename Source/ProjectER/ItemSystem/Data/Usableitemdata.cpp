#include "ItemSystem/Data/UsableItemData.h"

UUsableItemData::UUsableItemData()
{
	EffectType = EItemEffectType::None;
	StatType = EItemStatType::AttackPower;
	EffectValue = 0.0f;
	bConsumable = true;
}