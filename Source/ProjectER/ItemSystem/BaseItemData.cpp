#include "ItemSystem/BaseItemData.h"

UBaseItemData::UBaseItemData()
{
    // 생성자
}

FPrimaryAssetId UBaseItemData::GetPrimaryAssetId() const
{
    return FPrimaryAssetId("Items", GetFName());
}