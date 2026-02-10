#include "Monster/Data/MonsterDataAsset.h"

FPrimaryAssetId UMonsterDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Monster"), GetFName());
}
