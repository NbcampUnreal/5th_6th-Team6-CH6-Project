#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace ProjectER
{
	namespace Data // 데이터 - Set by Caller Magnitude 용도
	{
		namespace Damage 
		{
			namespace Type 
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Physical); // 물리 피해
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill); // 스킬(마법) 피해
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(True); // 고정 피해
			}
		}

		namespace Amount // 수치
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Heal); // 회복량 (HP or MP)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage); // 데미지량 
		}
	}
	
	namespace Status // 스탯
	{
		// 레벨 및 경험치
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Level); // 현재 레벨
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxLevel); // 최대 레벨
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(XP); // 경험치
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxXP); // 최대 경험치
		
		// 생존
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Health); // 체력 (HP)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxHealth); // 최대 체력
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HealthRegen); // 체력 재생
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stamina); // 스테미나 (MP)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxStamina); // 최대 스테미나
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StaminaRegen); // 스테미나 재생
		
		// 공격
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackPower); // 공격력
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackSpeed); // 공격 속도
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillAmp); // 스킬 증폭
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CritChance); // 치명타 확률
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CritDamage); // 치명타 피해량
		
		// 방어 및 유틸
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Defense); // 방어력
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveSpeed); // 이동 속도
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CooldownReduction); // 쿨타임 감소
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tenacity); // 강인함 (CC 저항)
	}
	
	namespace Input
	{
		namespace Action
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move); // 이동 명령 입력
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pause); // 정지 명령 입력
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack); // 공격 명령 입력
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction); // 상호작용

			namespace Skill
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive); // 패시브 스킬 입력
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Q); // Q 스킬 입력
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(W); // W 스킬 입력
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(E); // E 스킬 입력
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(R); // R 스킬 입력
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(D); // D 스킬 입력 (무기 스킬)
			}

			namespace Item
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_1); // 아이템 슬롯 1번
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_2); // 아이템 슬롯 2번
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_3); // 아이템 슬롯 3번
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_4); // 아이템 슬롯 4번
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_5); // 아이템 슬롯 5번
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_6); // 아이템 슬롯 6번
			}
		}
	}
	
	namespace CoolDown // 쿨타임
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Global); // 글로벌 쿨타임 - 공통 쿨타임 (스킬 동시발동 방지)
		
		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive); // 패시브 스킬 쿨타임
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Q); // Q 스킬 쿨타임
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(W); // W 스킬 쿨타임
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(E); // E 스킬 쿨타임
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(R); // R 스킬 쿨타임
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(D); // D 스킬 쿨타임
		}
	}
	
	namespace State
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead); // 사망 (타겟 지정 불가 + 입력 차단)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stealth); // 은신 (타겟 지정 불가)

		namespace Combat
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(None); // 비전투
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(InCombat); // 전투 중
		}
		
		namespace Buff // 버프
		{
			namespace Immune // 면역
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(CC); // CC 면역
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage); // 데미지 면역
			}
		}
		
		namespace Debuff // 디버프
		{
			namespace CC // 군중제어(Crowd Control)
			{
				namespace Soft // 이동, 공격 중 일부만 불가
				{
					UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slow); // 느려짐
					UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root); // 속박
					UE_DECLARE_GAMEPLAY_TAG_EXTERN(Silence); // 침묵
				}
				namespace Hard // 이동+공격 불가
				{
					UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stun); // 기절
					UE_DECLARE_GAMEPLAY_TAG_EXTERN(Airborne); // 에어본(공중에 뜸)
				}
				
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(BlockRegen); // 치유 불가
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(ReduceHealing); // 치유 감소
			}
		}
	}
	
	namespace Team
	{
		namespace Relation
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Self); // 자신(나)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Friendly); // 아군
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hostile); // 적군
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Neutral); // 중립
		}
	}
	
	namespace Unit // 유닛
	{
		namespace Type // 종류 (타입)
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player);	// 플레이어
			
			namespace Monster // 몬스터 
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mob); // 일반 몬스터
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Epic); // 에픽 몬스터
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Boss); // 보스 몬스터
			}
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Structure); // 구조물 (CCTV, 트랩 등)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Object); // 맵 오브젝트 (상자, 하이퍼루프 등)
		}
		
		namespace AttackType // 공격 타입
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee); // 근거리
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ranged); // 원거리
		}
	}
}