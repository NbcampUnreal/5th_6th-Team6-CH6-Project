#include "CharacterSystem/GameplayTags/GameplayTags.h"

namespace ProjectER
{
	namespace Ability
	{
		namespace Action
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(AutoAttack, "Ability.Action.AutoAttack", "Attack Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "Ability.Action.Death", "Death Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Revive, "Ability.Action.Revive", "Revive Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Move, "Ability.Action.Move", "Move Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chase, "Ability.Action.Chase", "Chase Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Return, "Ability.Action.Return", "Return Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sit, "Ability.Action.Sit", "Sit Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Idle, "Ability.Action.Idle", "Idle Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "Ability.Action.Combat", "Combat Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction, "Ability.Action.Interaction", "Interaction Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyStart, "Ability.Action.FlyStart", "FlyStart Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyIdle, "Ability.Action.FlyIdle", "FlyIdle Ability");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyEnd, "Ability.Action.FlyEnd", "FlyEnd Ability");
		}

		namespace Input
		{
			namespace Item
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_1, "Ability.Input.Item.Slot_1", "Item Slot 1 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_2, "Ability.Input.Item.Slot_2", "Item Slot 2 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_3, "Ability.Input.Item.Slot_3", "Item Slot 3 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_4, "Ability.Input.Item.Slot_4", "Item Slot 4 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_5, "Ability.Input.Item.Slot_5", "Item Slot 5 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_6, "Ability.Input.Item.Slot_6", "Item Slot 6 Input");

			}

			namespace Skill
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Passive, "Ability.Input.Skill.Passive", "Passive Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "Ability.Input.Skill.Q", "Q Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "Ability.Input.Skill.W", "W Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "Ability.Input.Skill.E", "E Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "Ability.Input.Skill.R", "Ultimate Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(D, "Ability.Input.Skill.D", "Weapon Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(F, "Ability.Input.Skill.F", "");
			}
		}
	}

	namespace Cooldown 
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AutoAttack, "Cooldown.AutoAttack", "Auto Attack Cooldown");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Global, "Cooldown.Global", "Global Cooldown");

		namespace Item
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Use, "Cooldown.Item.Use", "Using Item Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Set, "Cooldown.Item.Set", "Setting Item Cooldown");

		}

		namespace Skill
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Passive, "Cooldown.Skill.Passive", "Passive Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "Cooldown.Skill.Q", "Q Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "Cooldown.Skill.W", "W Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "Cooldown.Skill.E", "E Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "Cooldown.Skill.R", "Ultimate Skill Cooldown");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(D, "Cooldown.Skill.D", "Weapon Skill Cooldown");
		}
	}

	namespace Data
	{
		namespace DamageType
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Physical, "Data.DamageType.Physical", "Physical Damage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill, "Data.DamageType.Skill", "Skill (Magic) Damage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(True, "Data.DamageType.True", "True Damage (Ignores Armor)");
		}

		namespace Amount
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health, "Data.Amount.Heal", "Heal Amount");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "Data.Amount.Damage", "Damage Amount");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(IncomingXP, "Data.Amount.IncomingXP", "Incoming XP Amount");
		} 
	}


	
	namespace Event
	{
		namespace Action
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hit, "Event.Action.Hit", "Event for Hit");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "Event.Action.Attack", "Event for Attack");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(BeginSearch, "Event.Action.BeginSearch", "Monster BeginSearch");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(EndSearch, "Event.Action.EndSearch", "Monster EndSearch");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TargetOn, "Event.Action.TargetOn", "Event for TargetOn");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TargetOff, "Event.Action.TargetOff", "Event for TargetOff");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction, "Event.Action.Interaction", "Event for Interaction");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "Event.Action.Death", "Event for Death");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Return, "Event.Action.Return", "Event for Return");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase1, "Event.Action.Phase1", "Event for Phase1");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase2, "Event.Action.Phase2", "Event for Phase2");
		}

		namespace Montage
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackHit, "Event.Montage.AttackHit", "Montage Event for Attack Hit");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(SpawnProjectile, "Event.Montage.SpawnProjectile", "Montage Event for Spawn Projectile");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Active, "Event.Montage.Active", "Montage Event for Active");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Casting, "Event.Montage.Casting", "Montage Event for Casting");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(End, "Event.Montage.End", "Montage Event for End");
		}

		namespace System
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Test, "Event.System.Test", "");
		}

		namespace UI
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Test, "Event.UI.Test", "");
		}

		namespace Interact
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(OpenBox, "Event.Interact.OpenBox", "");
		}
	}

	namespace GameplayCue
	{
		namespace State
		{
			namespace Life
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "GameplayCue.State.Life.Death", "");
			}

			namespace Action
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sit, "GameplayCue.State.Action.Sit", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Idle, "GameplayCue.State.Action.Idle", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "GameplayCue.State.Action.Combat", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chase, "GameplayCue.State.Action.Chase", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "GameplayCue.State.Action.Attack", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Return, "GameplayCue.State.Action.Return", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyStart, "GameplayCue.State.Action.FlyStart", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyIdle, "GameplayCue.State.Action.FlyIdle", "");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyEnd, "GameplayCue.State.Action.FlyEnd", "");
			}
		}
	}
	
	namespace State
	{
		namespace Action
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction, "State.Action.Interaction", "Interaction State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Casting, "State.Action.Casting", "Casting State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "State.Action.Combat", "Combat State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sit, "State.Action.Sit", "Sit State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Idle, "State.Action.Idle", "Idle State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Move, "State.Action.Move", "Move State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "State.Action.Attack", "Attack State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Return, "State.Action.Return", "Return State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reviving, "State.Action.Reviving", "Reviving State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyStart, "State.Action.FlyStart", "FlyStart State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyIdle, "State.Action.FlyIdle", "FlyIdle State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlyEnd, "State.Action.FlyEnd", "FlyEnd State");
		}

		namespace Buff
		{
			namespace Immune
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(CC, "State.Buff.Immune.CC", "Immune to CC effects State");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "State.Buff.Immune.Damage", "Immune to Damage (Invulnerable) State");
			}
		}

		namespace Debuff
		{
			namespace Soft
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slow, "State.Debuff.Soft.Slow", "Movement Speed Slow");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "State.Debuff.Soft.Root", "Cannot Move");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Silence, "State.Debuff.Soft.Silence", "Cannot use Skills");
			}
			namespace Hard
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stun, "State.Debuff.Hard.Stun", "Cannot Move or Act");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Airborne, "State.Debuff.Hard.Airborne", "Knocked Up (Airborne)");
			}

			UE_DEFINE_GAMEPLAY_TAG_COMMENT(BlockRegen, "State.Debuff.BlockRegen", "Cannot Regenerate Health");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReduceHealing, "State.Debuff.ReduceHealing", "Healing effectiveness reduced");
		}

		namespace Life
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "State.Life.Death", "Death State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Down, "State.Life.Down", "Groggy State");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Alive, "State.Life.Alive", "Alive State");
		}
		
		/*namespace Status
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unselectable, "State.Status.Unselectable", "Unselectable State"); // 적 타겟팅은 되지만 일부 스킬 불가(Down 시 부여)
		}*/
	}

	namespace Status
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Level, "Status.Level", "Current Level");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxLevel, "Status.MaxLevel", "Max Level");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(XP, "Status.XP", "Current XP");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxXP, "Status.MaxXP", "XP required for next level");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health, "Status.Health", "Current Health");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxHealth, "Status.MaxHealth", "Maximum Health");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(HealthRegen, "Status.HealthRegen", "Health Regeneration per second");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stamina, "Status.Stamina", "Current Stamina/Mana");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxStamina, "Status.MaxStamina", "Maximum Stamina/Mana");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(StaminaRegen, "Status.StaminaRegen", "Stamina Regeneration per second");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackPower, "Status.AttackPower", "Physical Attack Power");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackSpeed, "Status.AttackSpeed", "Attack Speed"); 
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackRange, "Status.AttackRange", "Attack Range"); 
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillAmp, "Status.SkillAmp", "Skill Amplification");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CritChance, "Status.CritChance", "Critical Hit Chance");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CritDamage, "Status.CritDamage", "Critical Hit Damage Multiplier");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Defense, "Status.Defense", "Physical/Skill Defense");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveSpeed, "Status.MoveSpeed", "Movement Speed");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CooldownReduction, "Status.CooldownReduction", "Cooldown Reduction %");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tenacity, "Status.Tenacity", "Crowd Control Reduction %");
	}
	
	namespace Team
	{
		namespace Relation
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Self, "Team.Relation.Self", "Target is Self");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Friendly, "Team.Relation.Friendly", "Target is Friendly");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hostile, "Team.Relation.Hostile", "Target is Hostile");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Neutral, "Team.Relation.Neutral", "Target is Neutral");
		}
	}
	
	namespace Unit 
	{
		namespace AttackType
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee, "Unit.AttackType.Melee", "Melee Attack Range");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ranged, "Unit.AttackType.Ranged", "Ranged Attack Range");
		}

		namespace Type 
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player, "Unit.Type.Player", "Unit is a Player Character");
			
			namespace Monster 
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mob, "Unit.Type.Monster.Mob", "Normal Monster");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Epic, "Unit.Type.Monster.Epic", "Epic Monster");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Boss, "Unit.Type.Monster.Boss", "Boss Monster");
			}
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Structure, "Unit.Type.Structure", "Static Structures");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Object, "Unit.Type.Object", "Interactable Objects");
		}
		

	}

}