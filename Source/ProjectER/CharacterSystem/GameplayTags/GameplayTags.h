#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace ProjectER
{
	namespace Ability // 어빌리티 식별 태그
	{
		namespace Action
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(AutoAttack); // 기본공격
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Revive);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chase);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Return);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sit);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Idle);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlyStart);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlyIdle);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlyEnd);
		}

		namespace Input
		{
			namespace Item
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_1);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_2);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_3);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_4);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_5);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_6);
			}

			namespace Skill
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Q);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(W);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(E);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(R);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(D);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(F);
			}
		}
	}

	namespace Cooldown // CoolDown -> Cooldown (표기법 통일)
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AutoAttack); // 기본 공격 쿨타임
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Global);     // 글로벌 쿨타임 (GCD)

		namespace Item
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Use);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Set);
		}

		namespace Skill
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Q);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(W);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(E);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(R);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(D);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(F);
		}
	}

	namespace Data // 데이터 - Set by Caller Magnitude 용도
	{
		namespace Damage 
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Physical); // 물리 피해
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Skill);    // 스킬 피해
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(True);     // 고정 피해 (방어력 무시)
		}

		namespace Amount // 수치
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Heal);   // 치유량 (HP or MP)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage); // 피해량
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(IncomingXP); // 경험치 습득량
		}
	}
	
	
	namespace Event
	{
		namespace Action
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hit);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BeginSearch);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EndSearch);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetOn);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetOff);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Return);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase1);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase2);
		}

		namespace Montage
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackHit);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpawnProjectile); 
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Active);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Casting);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(End);
		}

		namespace System
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Test);
		}

		namespace UI
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Test);
		}

		namespace Interact
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(OpenBox);    // 상자 오픈
		}
	}

	namespace GameplayCue
	{
		namespace State
		{
			namespace Life
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
			}
			
			namespace Action
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sit);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Idle);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chase);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Return);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlyStart);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlyIdle);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlyEnd);
			}
		}
	}

	namespace State // 현재 상태
	{
		namespace Action
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Casting);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sit);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Idle);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Return);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reviving); // 아군 살리기
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlyStart);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlyIdle);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlyEnd);
		}

		namespace Buff
		{
			namespace Immune
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(CC);     // CC 면역 (저지 불가)
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage); // 피해 면역 (무적)
			}
		}

		namespace Debuff
		{
			namespace Soft // 행동 제약
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slow);    // 둔화
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);    // 속박
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Silence); // 침묵
			}
			namespace Hard // 행동 불가
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stun);     // 기절
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Airborne); // 에어본
			}

			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BlockRegen);    // 회복 불가
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ReduceHealing); // 치유 감소 (치감)
		}

		namespace Life
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);    // 사망
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Down);     // 그로기
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Alive);    // 생존
		}
		
		/*namespace Status
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unselectable); // 적 타겟팅은 되지만 일부 스킬 불가(Down 시 부여)
		}*/
	}
	
	namespace Status // 스탯
	{
		// 레벨 및 경험치
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Level);    // 현재 레벨
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxLevel); // 최대 레벨
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(XP);       // 현재 경험치
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxXP);    // 레벨업 필요 경험치
		
		// 생존
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Health);       // 현재 체력
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxHealth);    // 최대 체력
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HealthRegen);  // 초당 체력 회복량
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stamina);      // 현재 자원 (스테미나/마나)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxStamina);   // 최대 자원
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StaminaRegen); // 초당 자원 회복량
		
		// 공격
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackPower); // 공격력
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackSpeed); // 공격 속도
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AttackRange); // 기본 공격 사거리
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillAmp);   // 스킬 증폭
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CritChance);  // 치명타 확률
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CritDamage);  // 치명타 피해량
		
		// 방어 및 유틸
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Defense);           // 방어력
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveSpeed);         // 이동 속도
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CooldownReduction); // 쿨타임 감소
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tenacity);          // 강인함 (CC 지속시간 감소)
	}
	
	namespace Team
	{
		namespace Relation
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Self);     // 본인
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Friendly); // 아군
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hostile);  // 적군
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Neutral);  // 중립
		}
	}
	
	namespace Unit 
	{
		namespace AttackType
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee);  // 근거리
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ranged); // 원거리
		}

		namespace Type 
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player);
			
			namespace Monster 
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mob);  // 일반 몬스터
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Epic); // 에픽 몬스터
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Boss); // 보스 몬스터
			}
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Structure); // 구조물
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Object);    // 상호작용 오브젝트
		}
	}

}