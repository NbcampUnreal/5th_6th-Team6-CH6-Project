#include "CharacterSystem/GameplayTags/GameplayTags.h"

namespace ProjectER
{
	namespace Data
	{
		namespace Damage
		{
			namespace Type
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Physical, "Data.Damage.Type.Physical", "Physical Damage");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill, "Data.Damage.Type.Skill", "Skill (Magic) Damage");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(True, "Data.Damage.Type.True", "True Damage (Ignores Armor)");
			}
		}

		namespace Amount
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health, "Data.Amount.Heal", "Heal Amount");
		}
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
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackSpeed, "Status.AttackSpeed", "Attacks per second");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackRange, "Status.AttackRange", "Attacks Range");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkillAmp, "Status.SkillAmp", "Skill Amplification");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CritChance, "Status.CritChance", "Critical Hit Chance");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CritDamage, "Status.CritDamage", "Critical Hit Damage Multiplier");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Defense, "Status.Defense", "Physical/Skill Defense");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveSpeed, "Status.MoveSpeed", "Movement Speed");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CooldownReduction, "Status.CooldownReduction", "Cooldown Reduction %");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tenacity, "Status.Tenacity", "Crowd Control Reduction %");
	}
	
	namespace Input
	{
		namespace Action
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Move, "Input.Action.Move", "Move Command");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stop, "Input.Action.Stop", "Stop Command");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "Input.Action.Attack", "Attack Command");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction, "Input.Action.Interaction", "Interaction Command");

			namespace Skill
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Passive, "Input.Action.Skill.Passive", "Passive Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Q, "Input.Action.Skill.Q", "Q Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(W, "Input.Action.Skill.W", "W Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(E, "Input.Action.Skill.E", "E Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(R, "Input.Action.Skill.R", "Ultimate Skill Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(D, "Input.Action.Skill.D", "Weapon Skill Input");
			}

			namespace Item
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_1, "Input.Action.Item.Slot_1", "Item Slot 1 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_2, "Input.Action.Item.Slot_2", "Item Slot 2 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_3, "Input.Action.Item.Slot_3", "Item Slot 3 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_4, "Input.Action.Item.Slot_4", "Item Slot 4 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_5, "Input.Action.Item.Slot_5", "Item Slot 5 Input");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_6, "Input.Action.Item.Slot_6", "Item Slot 6 Input");
			}
		}
	}
	
	namespace CoolDown 
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Global, "Cooldown.Global", "Global Cooldown");

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
	
	namespace State
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dead, "State.Dead", "Unit is Dead");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stealth, "State.Stealth", "Unit is Stealth/Invisible");

		namespace Combat
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(None, "State.Combat.None", "Out of Combat");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(InCombat, "State.Combat.InCombat", "In Combat");
		}
		
		namespace Buff 
		{
			namespace Immune 
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(CC, "State.Buff.Immune.CC", "Immune to CC effects");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "State.Buff.Immune.Damage", "Immune to Damage (Invulnerable)");
			}
		}
		
		namespace Debuff 
		{
			namespace CC 
			{
				namespace Soft 
				{
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slow, "State.Debuff.CC.Soft.Slow", "Movement Speed Slow");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "State.Debuff.CC.Soft.Root", "Cannot Move");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Silence, "State.Debuff.CC.Soft.Silence", "Cannot use Skills");
				}
				namespace Hard 
				{
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stun, "State.Debuff.CC.Hard.Stun", "Cannot Move or Act");
					UE_DEFINE_GAMEPLAY_TAG_COMMENT(Airborne, "State.Debuff.CC.Hard.Airborne", "Knocked Up");
				}
				
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(BlockRegen, "State.Debuff.BlockRegen", "Cannot Regenerate Health");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReduceHealing, "State.Debuff.ReduceHealing", "Healing effectiveness reduced");
			}
		}
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
		namespace Type 
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player, "Unit.Type.Player", "Unit is a Player Character");
			
			namespace Monster 
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mob, "Unit.Type.Monster.Mob", "Normal Monster");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Epic, "Unit.Type.Monster.Epic", "Epic Monster");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Boss, "Unit.Type.Monster.Boss", "Boss Monster");
			}
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Structure, "Unit.Type.Structure", "Static Structures (ex: CCTV, Traps)");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Object, "Unit.Type.Object", "Interactable Objects (ex: Boxes)");
		}
		
		namespace AttackType 
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee, "Unit.AttackType.Melee", "Melee Attack Range");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ranged, "Unit.AttackType.Ranged", "Ranged Attack Range");
		}
	}
}