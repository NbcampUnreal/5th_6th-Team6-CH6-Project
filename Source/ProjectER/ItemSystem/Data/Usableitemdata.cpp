// File: 5th_6th-Team6-CH6-Project/Source/ProjectER/ItemSystem/Data/Usableitemdata.cpp

#include "ItemSystem/Data/UsableItemData.h"

UUsableItemData::UUsableItemData()
{
	EffectType = EItemEffectType::None;
	StatType = EItemStatType::AttackPower;
	EffectValue = 0.0f;
	TotalHealAmount = 0.0f;
	HealDurationSeconds = 10.0f;
	HealTickInterval = 1.0f;
	ItemStatEffectClass = nullptr;
	bConsumable = true;
}