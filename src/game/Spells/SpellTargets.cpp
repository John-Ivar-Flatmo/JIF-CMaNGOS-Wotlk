/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Spells/SpellTargets.h"
#include "Spells/SpellEffectDefines.h"
#include "Spells/SpellMgr.h"
#include "Server/DBCStructure.h"
#include "Server/SQLStorages.h"
#include "Log.h"

SpellTargetInfo::SpellTargetInfo(char const* name, SpellTargetImplicitType type, SpellTargetFilter filter, SpellTargetEnumerator enumerator, SpellTargetLOS los) :
    name(name), type(type), filter(filter), enumerator(enumerator), los(los)
{
}

SpellTargetInfo SpellTargetInfoTable[MAX_SPELL_TARGETS] =
{
    /*[0]*/     { "TARGET_NONE",                                          TARGET_TYPE_UNKNOWN                                                       },
    /*[1]*/     { "TARGET_UNIT_CASTER",                                   TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_SINGLE },
    /*[2]*/     { "TARGET_UNIT_ENEMY_NEAR_CASTER",                        TARGET_TYPE_UNIT,             TARGET_HARMFUL,    TARGET_ENUMERATOR_CHAIN  },
    /*[3]*/     { "TARGET_UNIT_FRIEND_NEAR_CASTER",                       TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_CHAIN  },
    /*[4]*/     { "TARGET_UNIT_NEAR_CASTER",                              TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_CHAIN  },
    /*[5]*/     { "TARGET_UNIT_CASTER_PET",                               TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_SINGLE },
    /*[6]*/     { "TARGET_UNIT_ENEMY",                                    TARGET_TYPE_UNIT,             TARGET_HARMFUL,    TARGET_ENUMERATOR_CHAIN  },
    /*[7]*/     { "TARGET_ENUM_UNITS_SCRIPT_AOE_AT_SRC_LOC",              TARGET_TYPE_UNIT,             TARGET_SCRIPT,     TARGET_ENUMERATOR_AOE,   TARGET_LOS_SRC  },
    /*[8]*/     { "TARGET_ENUM_UNITS_SCRIPT_AOE_AT_DEST_LOC",             TARGET_TYPE_UNIT,             TARGET_SCRIPT,     TARGET_ENUMERATOR_AOE,   TARGET_LOS_DEST },
    /*[9]*/     { "TARGET_LOCATION_CASTER_HOME_BIND",                     TARGET_TYPE_LOCATION_DEST                                                 },
    /*[10]*/    { "TARGET_LOCATION_CASTER_DIVINE_BIND_NYI",               TARGET_TYPE_LOCATION_DEST                                                 },
    /*[11]*/    { "TARGET_PLAYER_NYI",                                    TARGET_TYPE_PLAYER,           TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[12]*/    { "TARGET_PLAYER_NEAR_CASTER_NYI",                        TARGET_TYPE_PLAYER,           TARGET_NEUTRAL,    TARGET_ENUMERATOR_CHAIN  },
    /*[13]*/    { "TARGET_PLAYER_ENEMY_NYI",                              TARGET_TYPE_PLAYER,           TARGET_HARMFUL,    TARGET_ENUMERATOR_SINGLE },
    /*[14]*/    { "TARGET_PLAYER_FRIEND_NYI",                             TARGET_TYPE_PLAYER,           TARGET_HELPFUL,    TARGET_ENUMERATOR_SINGLE },
    /*[15]*/    { "TARGET_ENUM_UNITS_ENEMY_AOE_AT_SRC_LOC",               TARGET_TYPE_UNIT,             TARGET_HARMFUL,    TARGET_ENUMERATOR_AOE,   TARGET_LOS_SRC  },
    /*[16]*/    { "TARGET_ENUM_UNITS_ENEMY_AOE_AT_DEST_LOC",              TARGET_TYPE_UNIT,             TARGET_HARMFUL,    TARGET_ENUMERATOR_AOE,   TARGET_LOS_DEST },
    /*[17]*/    { "TARGET_LOCATION_DATABASE",                             TARGET_TYPE_LOCATION_DEST,    TARGET_SCRIPT                               },
    /*[18]*/    { "TARGET_LOCATION_CASTER_DEST",                          TARGET_TYPE_LOCATION_DEST                                                 },
    /*[19]*/    { "TARGET_UNK_19",                                        TARGET_TYPE_UNKNOWN                                                       },
    /*[20]*/    { "TARGET_ENUM_UNITS_PARTY_WITHIN_CASTER_RANGE",          TARGET_TYPE_UNIT,             TARGET_PARTY,      TARGET_ENUMERATOR_AOE    },
    /*[21]*/    { "TARGET_UNIT_FRIEND",                                   TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_SINGLE },
    /*[22]*/    { "TARGET_LOCATION_CASTER_SRC",                           TARGET_TYPE_LOCATION_SRC                                                  },
    /*[23]*/    { "TARGET_GAMEOBJECT",                                    TARGET_TYPE_GAMEOBJECT,       TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[24]*/    { "TARGET_ENUM_UNITS_ENEMY_IN_CONE_24",                   TARGET_TYPE_UNIT,             TARGET_HARMFUL,    TARGET_ENUMERATOR_CONE   },
    /*[25]*/    { "TARGET_UNIT",                                          TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[26]*/    { "TARGET_LOCKED",                                        TARGET_TYPE_LOCK,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[27]*/    { "TARGET_UNIT_CASTER_MASTER",                            TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_SINGLE },
    /*[28]*/    { "TARGET_ENUM_UNITS_ENEMY_AOE_AT_DYNOBJ_LOC",            TARGET_TYPE_UNIT,             TARGET_HARMFUL,    TARGET_ENUMERATOR_AOE    },
    /*[29]*/    { "TARGET_ENUM_UNITS_FRIEND_AOE_AT_DYNOBJ_LOC",           TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_AOE    },
    /*[30]*/    { "TARGET_ENUM_UNITS_FRIEND_AOE_AT_SRC_LOC",              TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_AOE,   TARGET_LOS_SRC  },
    /*[31]*/    { "TARGET_ENUM_UNITS_FRIEND_AOE_AT_DEST_LOC",             TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_AOE,   TARGET_LOS_DEST },
    /*[32]*/    { "TARGET_LOCATION_UNIT_MINION_POSITION",                 TARGET_TYPE_LOCATION_DEST                                                 },
    /*[33]*/    { "TARGET_ENUM_UNITS_PARTY_AOE_AT_SRC_LOC",               TARGET_TYPE_UNIT,             TARGET_PARTY,      TARGET_ENUMERATOR_AOE,   TARGET_LOS_SRC  },
    /*[34]*/    { "TARGET_ENUM_UNITS_PARTY_AOE_AT_DEST_LOC",              TARGET_TYPE_UNIT,             TARGET_PARTY,      TARGET_ENUMERATOR_AOE,   TARGET_LOS_DEST },
    /*[35]*/    { "TARGET_UNIT_PARTY",                                    TARGET_TYPE_UNIT,             TARGET_PARTY,      TARGET_ENUMERATOR_SINGLE },
    /*[36]*/    { "TARGET_ENUM_UNITS_ENEMY_WITHIN_CASTER_RANGE",          TARGET_TYPE_UNIT,             TARGET_HARMFUL,    TARGET_ENUMERATOR_AOE    },
    /*[37]*/    { "TARGET_UNIT_FRIEND_AND_PARTY",                         TARGET_TYPE_UNIT,             TARGET_PARTY,      TARGET_ENUMERATOR_CHAIN  },
    /*[38]*/    { "TARGET_UNIT_SCRIPT_NEAR_CASTER",                       TARGET_TYPE_UNIT,             TARGET_SCRIPT,     TARGET_ENUMERATOR_CHAIN  },
    /*[39]*/    { "TARGET_LOCATION_CASTER_FISHING_SPOT",                  TARGET_TYPE_LOCATION_DEST                                                 },
    /*[40]*/    { "TARGET_GAMEOBJECT_SCRIPT_NEAR_CASTER",                 TARGET_TYPE_GAMEOBJECT,       TARGET_SCRIPT,     TARGET_ENUMERATOR_CHAIN  },
    /*[41]*/    { "TARGET_LOCATION_CASTER_FRONT_RIGHT",                   TARGET_TYPE_LOCATION_DEST                                                 },
    /*[42]*/    { "TARGET_LOCATION_CASTER_BACK_RIGHT",                    TARGET_TYPE_LOCATION_DEST                                                 },
    /*[43]*/    { "TARGET_LOCATION_CASTER_BACK_LEFT",                     TARGET_TYPE_LOCATION_DEST                                                 },
    /*[44]*/    { "TARGET_LOCATION_CASTER_FRONT_LEFT",                    TARGET_TYPE_LOCATION_DEST                                                 },
    /*[45]*/    { "TARGET_UNIT_FRIEND_CHAIN_HEAL",                        TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_CHAIN  },
    /*[46]*/    { "TARGET_LOCATION_SCRIPT_NEAR_CASTER",                   TARGET_TYPE_LOCATION_DEST,    TARGET_SCRIPT                               },
    /*[47]*/    { "TARGET_LOCATION_CASTER_FRONT",                         TARGET_TYPE_LOCATION_DEST                                                 },
    /*[48]*/    { "TARGET_LOCATION_CASTER_BACK",                          TARGET_TYPE_LOCATION_DEST                                                 },
    /*[49]*/    { "TARGET_LOCATION_CASTER_LEFT",                          TARGET_TYPE_LOCATION_DEST                                                 },
    /*[50]*/    { "TARGET_LOCATION_CASTER_RIGHT",                         TARGET_TYPE_LOCATION_DEST                                                 },
    /*[51]*/    { "TARGET_ENUM_GAMEOBJECTS_SCRIPT_AOE_AT_SRC_LOC",        TARGET_TYPE_GAMEOBJECT,       TARGET_SCRIPT,     TARGET_ENUMERATOR_AOE,   TARGET_LOS_SRC  },
    /*[52]*/    { "TARGET_ENUM_GAMEOBJECTS_SCRIPT_AOE_AT_DEST_LOC",       TARGET_TYPE_GAMEOBJECT,       TARGET_SCRIPT,     TARGET_ENUMERATOR_AOE,   TARGET_LOS_DEST },
    /*[53]*/    { "TARGET_LOCATION_CASTER_TARGET_POSITION",               TARGET_TYPE_LOCATION_DEST                                                 },
    /*[54]*/    { "TARGET_ENUM_UNITS_ENEMY_IN_CONE_54",                   TARGET_TYPE_UNIT,             TARGET_HARMFUL,    TARGET_ENUMERATOR_CONE   },
    /*[55]*/    { "TARGET_LOCATION_CASTER_FRONT_LEAP",                    TARGET_TYPE_LOCATION_DEST                                                 },
    /*[56]*/    { "TARGET_ENUM_UNITS_RAID_WITHIN_CASTER_RANGE",           TARGET_TYPE_UNIT,             TARGET_GROUP,      TARGET_ENUMERATOR_AOE    },
    /*[57]*/    { "TARGET_UNIT_RAID",                                     TARGET_TYPE_UNIT,             TARGET_GROUP,      TARGET_ENUMERATOR_SINGLE },
    /*[58]*/    { "TARGET_UNIT_RAID_NEAR_CASTER",                         TARGET_TYPE_UNIT,             TARGET_GROUP,      TARGET_ENUMERATOR_CHAIN  },
    /*[59]*/    { "TARGET_ENUM_UNITS_FRIEND_IN_CONE",                     TARGET_TYPE_UNIT,             TARGET_HELPFUL,    TARGET_ENUMERATOR_CONE   },
    /*[60]*/    { "TARGET_ENUM_UNITS_SCRIPT_IN_CONE_60",                  TARGET_TYPE_UNIT,             TARGET_SCRIPT,     TARGET_ENUMERATOR_CONE   },
    /*[61]*/    { "TARGET_UNIT_RAID_AND_CLASS",                           TARGET_TYPE_UNIT,             TARGET_GROUP,      TARGET_ENUMERATOR_CHAIN  },
    /*[62]*/    { "TARGET_PLAYER_RAID_NYI",                               TARGET_TYPE_PLAYER,           TARGET_GROUP,      TARGET_ENUMERATOR_SINGLE },
    /*[63]*/    { "TARGET_LOCATION_UNIT_POSITION",                        TARGET_TYPE_LOCATION_DEST                                                 },
    /*[64]*/    { "TARGET_LOCATION_UNIT_FRONT",                           TARGET_TYPE_LOCATION_DEST                                                 },
    /*[65]*/    { "TARGET_LOCATION_UNIT_BACK",                            TARGET_TYPE_LOCATION_DEST                                                 },
    /*[66]*/    { "TARGET_LOCATION_UNIT_RIGHT",                           TARGET_TYPE_LOCATION_DEST                                                 },
    /*[67]*/    { "TARGET_LOCATION_UNIT_LEFT",                            TARGET_TYPE_LOCATION_DEST                                                 },
    /*[68]*/    { "TARGET_LOCATION_UNIT_FRONT_RIGHT",                     TARGET_TYPE_LOCATION_DEST                                                 },
    /*[69]*/    { "TARGET_LOCATION_UNIT_BACK_RIGHT",                      TARGET_TYPE_LOCATION_DEST                                                 },
    /*[70]*/    { "TARGET_LOCATION_UNIT_BACK_LEFT",                       TARGET_TYPE_LOCATION_DEST                                                 },
    /*[71]*/    { "TARGET_LOCATION_UNIT_FRONT_LEFT",                      TARGET_TYPE_LOCATION_DEST                                                 },
    /*[72]*/    { "TARGET_LOCATION_CASTER_RANDOM_SIDE",                   TARGET_TYPE_LOCATION_DEST                                                 },
    /*[73]*/    { "TARGET_LOCATION_CASTER_RANDOM_CIRCUMFERENCE",          TARGET_TYPE_LOCATION_DEST                                                 },
    /*[74]*/    { "TARGET_LOCATION_UNIT_RANDOM_SIDE",                     TARGET_TYPE_LOCATION_DEST                                                 },
    /*[75]*/    { "TARGET_LOCATION_UNIT_RANDOM_CIRCUMFERENCE",            TARGET_TYPE_LOCATION_DEST                                                 },
    /*[76]*/    { "TARGET_LOCATION_CHANNEL_TARGET_DEST",                  TARGET_TYPE_LOCATION_DEST                                                 },
    /*[77]*/    { "TARGET_UNIT_CHANNEL_TARGET",                           TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[78]*/    { "TARGET_LOCATION_NORTH",                                TARGET_TYPE_LOCATION_DEST                                                 },
    /*[79]*/    { "TARGET_LOCATION_SOUTH",                                TARGET_TYPE_LOCATION_DEST                                                 },
    /*[80]*/    { "TARGET_LOCATION_EAST",                                 TARGET_TYPE_LOCATION_DEST                                                 },
    /*[81]*/    { "TARGET_LOCATION_WEST",                                 TARGET_TYPE_LOCATION_DEST                                                 },
    /*[82]*/    { "TARGET_LOCATION_NE",                                   TARGET_TYPE_LOCATION_DEST                                                 },
    /*[83]*/    { "TARGET_LOCATION_NW",                                   TARGET_TYPE_LOCATION_DEST                                                 },
    /*[84]*/    { "TARGET_LOCATION_SE",                                   TARGET_TYPE_LOCATION_DEST                                                 },
    /*[85]*/    { "TARGET_LOCATION_SW",                                   TARGET_TYPE_LOCATION_DEST                                                 },
    /*[86]*/    { "TARGET_LOCATION_RANDOM_SIDE",                          TARGET_TYPE_LOCATION_DEST                                                 },
    /*[87]*/    { "TARGET_LOCATION_CURRENT_REFERENCE",                    TARGET_TYPE_LOCATION_DEST                                                 },
    /*[88]*/    { "TARGET_ENUM_UNITS_SCRIPT_AOE_AT_DYNOBJ_LOC",           TARGET_TYPE_UNIT,             TARGET_SCRIPT,     TARGET_ENUMERATOR_AOE    },
    /*[89]*/    { "TARGET_LOCATION_TRAJECTORY",                           TARGET_TYPE_LOCATION_DEST                                                 },
    /*[90]*/    { "TARGET_UNIT_CASTER_COMPANION",                         TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[91]*/    { "TARGET_LOCATION_RANDOM_CIRCUMFERENCE",                 TARGET_TYPE_LOCATION_DEST                                                 },
    /*[92]*/    { "TARGET_UNIT_CASTER_SUMMONER",                          TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[93]*/    { "TARGET_CORPSE_ENEMY_NEAR_CASTER",                      TARGET_TYPE_CORPSE,           TARGET_HARMFUL,    TARGET_ENUMERATOR_CHAIN  },
    /*[94]*/    { "TARGET_UNIT_CASTER_VEHICLE",                           TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[95]*/    { "TARGET_UNIT_CASTER_DRIVER",                            TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[96]*/    { "TARGET_UNIT_CASTER_PASSENGER_0",                       TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[97]*/    { "TARGET_UNIT_CASTER_PASSENGER_1",                       TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[98]*/    { "TARGET_UNIT_CASTER_PASSENGER_2",                       TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[99]*/    { "TARGET_UNIT_CASTER_PASSENGER_3",                       TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[100]*/   { "TARGET_UNIT_CASTER_PASSENGER_4",                       TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[101]*/   { "TARGET_UNIT_CASTER_PASSENGER_5",                       TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[102]*/   { "TARGET_UNIT_CASTER_PASSENGER_6",                       TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[103]*/   { "TARGET_UNIT_CASTER_PASSENGER_7",                       TARGET_TYPE_UNIT,             TARGET_NEUTRAL,    TARGET_ENUMERATOR_SINGLE },
    /*[104]*/   { "TARGET_ENUM_UNITS_ENEMY_IN_CONE_104",                  TARGET_TYPE_UNIT,             TARGET_HARMFUL,    TARGET_ENUMERATOR_CONE   },
    /*[105]*/   { "TARGET_UNIT_105_NYI",                                  TARGET_TYPE_UNIT                                                          },
    /*[106]*/   { "TARGET_LOCATION_106_NYI",                              TARGET_TYPE_LOCATION_DEST                                                 },
    /*[107]*/   { "TARGET_UNK_107",                                       TARGET_TYPE_UNKNOWN                                                       },
    /*[108]*/   { "TARGET_ENUM_GAMEOBJECTS_IN_CONE",                      TARGET_TYPE_GAMEOBJECT,       TARGET_NEUTRAL,    TARGET_ENUMERATOR_CONE   },
    /*[109]*/   { "TARGET_UNK_109",                                       TARGET_TYPE_UNKNOWN                                                       },
    /*[110]*/   { "TARGET_ENUM_UNITS_SCRIPT_IN_CONE_110",                 TARGET_TYPE_UNIT,             TARGET_SCRIPT,     TARGET_ENUMERATOR_CONE   },
};

SpellEffectInfo SpellEffectInfoTable[MAX_SPELL_EFFECTS] =
{
    /*[0]*/      { "SPELL_EFFECT_NONE",                         TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[1]*/      { "SPELL_EFFECT_INSTAKILL",                    TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[2]*/      { "SPELL_EFFECT_SCHOOL_DAMAGE",                TARGET_TYPE_UNIT,           TARGET_UNIT_ENEMY },
    /*[3]*/      { "SPELL_EFFECT_DUMMY",                        TARGET_TYPE_DYNAMIC,        TARGET_NONE }, // confirmed none
    /*[4]*/      { "SPELL_EFFECT_PORTAL_TELEPORT",              TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[5]*/      { "SPELL_EFFECT_TELEPORT_UNITS",               TARGET_TYPE_UNIT_DEST,      TARGET_NONE },
    /*[6]*/      { "SPELL_EFFECT_APPLY_AURA",                   TARGET_TYPE_UNIT,           TARGET_UNIT_CASTER },
    /*[7]*/      { "SPELL_EFFECT_ENVIRONMENTAL_DAMAGE",         TARGET_TYPE_NONE,           TARGET_NONE }, // none is a hack - should be unit - GO casting
    /*[8]*/      { "SPELL_EFFECT_POWER_DRAIN",                  TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[9]*/      { "SPELL_EFFECT_HEALTH_LEECH",                 TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[10]*/     { "SPELL_EFFECT_HEAL",                         TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[11]*/     { "SPELL_EFFECT_BIND",                         TARGET_TYPE_UNIT,           TARGET_UNIT }, // player target only
    /*[12]*/     { "SPELL_EFFECT_PORTAL",                       TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[13]*/     { "SPELL_EFFECT_RITUAL_BASE",                  TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[14]*/     { "SPELL_EFFECT_RITUAL_SPECIALIZE",            TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[15]*/     { "SPELL_EFFECT_RITUAL_ACTIVATE_PORTAL",       TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[16]*/     { "SPELL_EFFECT_QUEST_COMPLETE",               TARGET_TYPE_UNIT,           TARGET_NONE }, // player target only
    /*[17]*/     { "SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL",       TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[18]*/     { "SPELL_EFFECT_RESURRECT",                    TARGET_TYPE_CORPSE,         TARGET_UNIT }, // player target only
    /*[19]*/     { "SPELL_EFFECT_ADD_EXTRA_ATTACKS",            TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[20]*/     { "SPELL_EFFECT_DODGE",                        TARGET_TYPE_NONE,           TARGET_NONE }, // confirmed none
    /*[21]*/     { "SPELL_EFFECT_EVADE",                        TARGET_TYPE_NONE,           TARGET_NONE },
    /*[22]*/     { "SPELL_EFFECT_PARRY",                        TARGET_TYPE_NONE,           TARGET_NONE }, // confirmed none
    /*[23]*/     { "SPELL_EFFECT_BLOCK",                        TARGET_TYPE_NONE,           TARGET_NONE }, // confirmed none
    /*[24]*/     { "SPELL_EFFECT_CREATE_ITEM",                  TARGET_TYPE_UNIT,           TARGET_UNIT }, // player target only
    /*[25]*/     { "SPELL_EFFECT_WEAPON",                       TARGET_TYPE_NONE,           TARGET_NONE }, // confirmed none
    /*[26]*/     { "SPELL_EFFECT_DEFENSE",                      TARGET_TYPE_NONE,           TARGET_NONE },
    /*[27]*/     { "SPELL_EFFECT_PERSISTENT_AREA_AURA",         TARGET_TYPE_SPECIAL_DEST,   TARGET_NONE },
    /*[28]*/     { "SPELL_EFFECT_SUMMON",                       TARGET_TYPE_LOCATION_DEST,  TARGET_LOCATION_CASTER_DEST },
    /*[29]*/     { "SPELL_EFFECT_LEAP",                         TARGET_TYPE_UNIT_DEST,      TARGET_NONE },
    /*[30]*/     { "SPELL_EFFECT_ENERGIZE",                     TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[31]*/     { "SPELL_EFFECT_WEAPON_PERCENT_DAMAGE",        TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[32]*/     { "SPELL_EFFECT_TRIGGER_MISSILE",              TARGET_TYPE_DYNAMIC_ANY,    TARGET_NONE },
    /*[33]*/     { "SPELL_EFFECT_OPEN_LOCK",                    TARGET_TYPE_LOCK,           TARGET_NONE },
    /*[34]*/     { "SPELL_EFFECT_SUMMON_CHANGE_ITEM",           TARGET_TYPE_UNIT,           TARGET_UNIT_CASTER }, // caster item, target unit
    /*[35]*/     { "SPELL_EFFECT_APPLY_AREA_AURA_PARTY",        TARGET_TYPE_SPECIAL_UNIT,   TARGET_NONE },
    /*[36]*/     { "SPELL_EFFECT_LEARN_SPELL",                  TARGET_TYPE_UNIT,           TARGET_UNIT },
    /*[37]*/     { "SPELL_EFFECT_SPELL_DEFENSE",                TARGET_TYPE_NONE,           TARGET_NONE },
    /*[38]*/     { "SPELL_EFFECT_DISPEL",                       TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[39]*/     { "SPELL_EFFECT_LANGUAGE",                     TARGET_TYPE_NONE,           TARGET_NONE },
    /*[40]*/     { "SPELL_EFFECT_DUAL_WIELD",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[41]*/     { "SPELL_EFFECT_JUMP",                         TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[42]*/     { "SPELL_EFFECT_JUMP_DEST",                    TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[43]*/     { "SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER",   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[44]*/     { "SPELL_EFFECT_SKILL_STEP",                   TARGET_TYPE_UNKNOWN,        TARGET_UNIT },
    /*[45]*/     { "SPELL_EFFECT_ADD_HONOR",                    TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[46]*/     { "SPELL_EFFECT_SPAWN",                        TARGET_TYPE_UNIT,           TARGET_UNIT_CASTER },
    /*[47]*/     { "SPELL_EFFECT_TRADE_SKILL",                  TARGET_TYPE_UNIT,           TARGET_UNIT_CASTER },
    /*[48]*/     { "SPELL_EFFECT_STEALTH",                      TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[49]*/     { "SPELL_EFFECT_DETECT",                       TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[50]*/     { "SPELL_EFFECT_TRANS_DOOR",                   TARGET_TYPE_LOCATION_DEST,  TARGET_LOCATION_CASTER_DEST },
    /*[51]*/     { "SPELL_EFFECT_FORCE_CRITICAL_HIT",           TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[52]*/     { "SPELL_EFFECT_GUARANTEE_HIT",                TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[53]*/     { "SPELL_EFFECT_ENCHANT_ITEM",                 TARGET_TYPE_ITEM,           TARGET_NONE },
    /*[54]*/     { "SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY",       TARGET_TYPE_ITEM,           TARGET_NONE },
    /*[55]*/     { "SPELL_EFFECT_TAMECREATURE",                 TARGET_TYPE_UNIT,           TARGET_NONE }, // creature target only
    /*[56]*/     { "SPELL_EFFECT_SUMMON_PET",                   TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[57]*/     { "SPELL_EFFECT_LEARN_PET_SPELL",              TARGET_TYPE_UNIT,           TARGET_UNIT_CASTER_PET },
    /*[58]*/     { "SPELL_EFFECT_WEAPON_DAMAGE",                TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[59]*/     { "SPELL_EFFECT_CREATE_RANDOM_ITEM",           TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[60]*/     { "SPELL_EFFECT_PROFICIENCY",                  TARGET_TYPE_NONE,           TARGET_NONE }, // confirmed none
    /*[61]*/     { "SPELL_EFFECT_SEND_EVENT",                   TARGET_TYPE_UNIT,           TARGET_UNIT_CASTER },
    /*[62]*/     { "SPELL_EFFECT_POWER_BURN",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[63]*/     { "SPELL_EFFECT_THREAT",                       TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[64]*/     { "SPELL_EFFECT_TRIGGER_SPELL",                TARGET_TYPE_DYNAMIC,        TARGET_NONE }, // confirmed none
    /*[65]*/     { "SPELL_EFFECT_APPLY_AREA_AURA_RAID",         TARGET_TYPE_SPECIAL_UNIT,   TARGET_NONE },
    /*[66]*/     { "SPELL_EFFECT_POWER_FUNNEL",                 TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[67]*/     { "SPELL_EFFECT_HEAL_MAX_HEALTH",              TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[68]*/     { "SPELL_EFFECT_INTERRUPT_CAST",               TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[69]*/     { "SPELL_EFFECT_DISTRACT",                     TARGET_TYPE_UNIT_DEST,      TARGET_NONE },
    /*[70]*/     { "SPELL_EFFECT_PULL",                         TARGET_TYPE_UNIT_DEST,      TARGET_NONE }, // implicit guesswork
    /*[71]*/     { "SPELL_EFFECT_PICKPOCKET",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[72]*/     { "SPELL_EFFECT_ADD_FARSIGHT",                 TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[73]*/     { "SPELL_EFFECT_UNTRAIN_TALENTS",              TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[74]*/     { "SPELL_EFFECT_APPLY_GLYPH",                  TARGET_TYPE_NONE,           TARGET_NONE },
    /*[75]*/     { "SPELL_EFFECT_HEAL_MECHANICAL",              TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[76]*/     { "SPELL_EFFECT_SUMMON_OBJECT_WILD",           TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[77]*/     { "SPELL_EFFECT_SCRIPT_EFFECT",                TARGET_TYPE_DYNAMIC,        TARGET_NONE },
    /*[78]*/     { "SPELL_EFFECT_ATTACK",                       TARGET_TYPE_NONE,           TARGET_NONE },
    /*[79]*/     { "SPELL_EFFECT_SANCTUARY",                    TARGET_TYPE_UNIT,           TARGET_UNIT_CASTER },
    /*[80]*/     { "SPELL_EFFECT_ADD_COMBO_POINTS",             TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[81]*/     { "SPELL_EFFECT_CREATE_HOUSE",                 TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[82]*/     { "SPELL_EFFECT_BIND_SIGHT",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[83]*/     { "SPELL_EFFECT_DUEL",                         TARGET_TYPE_UNIT_DEST,      TARGET_NONE },
    /*[84]*/     { "SPELL_EFFECT_STUCK",                        TARGET_TYPE_NONE,           TARGET_NONE }, // confirmed none
    /*[85]*/     { "SPELL_EFFECT_SUMMON_PLAYER",                TARGET_TYPE_UNIT,           TARGET_UNIT }, // not meant to send anything - TODO: Fix
    /*[86]*/     { "SPELL_EFFECT_ACTIVATE_OBJECT",              TARGET_TYPE_GAMEOBJECT,     TARGET_NONE },
    /*[87]*/     { "SPELL_EFFECT_WMO_DAMAGE",                   TARGET_TYPE_GAMEOBJECT,     TARGET_NONE },
    /*[88]*/     { "SPELL_EFFECT_WMO_REPAIR",                   TARGET_TYPE_GAMEOBJECT,     TARGET_NONE },
    /*[89]*/     { "SPELL_EFFECT_WMO_CHANGE",                   TARGET_TYPE_GAMEOBJECT,     TARGET_NONE },
    /*[90]*/     { "SPELL_EFFECT_KILL_CREDIT_PERSONAL",         TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[91]*/     { "SPELL_EFFECT_THREAT_ALL",                   TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[92]*/     { "SPELL_EFFECT_ENCHANT_HELD_ITEM",            TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[93]*/     { "SPELL_EFFECT_BREAK_PLAYER_TARGETING",       TARGET_TYPE_UNIT,           TARGET_UNIT_CASTER },
    /*[94]*/     { "SPELL_EFFECT_SELF_RESURRECT",               TARGET_TYPE_NONE,           TARGET_NONE }, // confirmed none
    /*[95]*/     { "SPELL_EFFECT_SKINNING",                     TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[96]*/     { "SPELL_EFFECT_CHARGE",                       TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[97]*/     { "SPELL_EFFECT_SUMMON_ALL_TOTEMS",            TARGET_TYPE_NONE,           TARGET_NONE },
    /*[98]*/     { "SPELL_EFFECT_KNOCK_BACK",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[99]*/     { "SPELL_EFFECT_DISENCHANT",                   TARGET_TYPE_ITEM,           TARGET_NONE },
    /*[100]*/    { "SPELL_EFFECT_INEBRIATE",                    TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[101]*/    { "SPELL_EFFECT_FEED_PET",                     TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[102]*/    { "SPELL_EFFECT_DISMISS_PET",                  TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[103]*/    { "SPELL_EFFECT_REPUTATION",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[104]*/    { "SPELL_EFFECT_SUMMON_OBJECT_SLOT1",          TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[105]*/    { "SPELL_EFFECT_SUMMON_OBJECT_SLOT2",          TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[106]*/    { "SPELL_EFFECT_SUMMON_OBJECT_SLOT3",          TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[107]*/    { "SPELL_EFFECT_SUMMON_OBJECT_SLOT4",          TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[108]*/    { "SPELL_EFFECT_DISPEL_MECHANIC",              TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[109]*/    { "SPELL_EFFECT_SUMMON_DEAD_PET",              TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[110]*/    { "SPELL_EFFECT_DESTROY_ALL_TOTEMS",           TARGET_TYPE_NONE,           TARGET_NONE }, // confirmed none
    /*[111]*/    { "SPELL_EFFECT_DURABILITY_DAMAGE",            TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[112]*/    { "SPELL_EFFECT_112",                          TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[113]*/    { "SPELL_EFFECT_RESURRECT_NEW",                TARGET_TYPE_CORPSE,         TARGET_NONE },
    /*[114]*/    { "SPELL_EFFECT_ATTACK_ME",                    TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[115]*/    { "SPELL_EFFECT_DURABILITY_DAMAGE_PCT",        TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[116]*/    { "SPELL_EFFECT_SKIN_PLAYER_CORPSE",           TARGET_TYPE_CORPSE,         TARGET_UNIT },
    /*[117]*/    { "SPELL_EFFECT_SPIRIT_HEAL",                  TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[118]*/    { "SPELL_EFFECT_SKILL",                        TARGET_TYPE_NONE,           TARGET_UNIT },
    /*[119]*/    { "SPELL_EFFECT_APPLY_AREA_AURA_PET",          TARGET_TYPE_SPECIAL_UNIT,   TARGET_NONE },
    /*[120]*/    { "SPELL_EFFECT_TELEPORT_GRAVEYARD",           TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[121]*/    { "SPELL_EFFECT_NORMALIZED_WEAPON_DMG",        TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[122]*/    { "SPELL_EFFECT_122",                          TARGET_TYPE_UNKNOWN,        TARGET_NONE },
    /*[123]*/    { "SPELL_EFFECT_SEND_TAXI",                    TARGET_TYPE_UNIT,           TARGET_UNIT },
    /*[124]*/    { "SPELL_EFFECT_PULL_TOWARDS",                 TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[125]*/    { "SPELL_EFFECT_MODIFY_THREAT_PERCENT",        TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[126]*/    { "SPELL_EFFECT_STEAL_BENEFICIAL_BUFF",        TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[127]*/    { "SPELL_EFFECT_PROSPECTING",                  TARGET_TYPE_ITEM,           TARGET_NONE },
    /*[128]*/    { "SPELL_EFFECT_APPLY_AREA_AURA_FRIEND",       TARGET_TYPE_SPECIAL_UNIT,   TARGET_NONE },
    /*[129]*/    { "SPELL_EFFECT_APPLY_AREA_AURA_ENEMY",        TARGET_TYPE_SPECIAL_UNIT,   TARGET_NONE },
    /*[130]*/    { "SPELL_EFFECT_REDIRECT_THREAT",              TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[131]*/    { "SPELL_EFFECT_PLAY_SOUND",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[132]*/    { "SPELL_EFFECT_PLAY_MUSIC",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[133]*/    { "SPELL_EFFECT_UNLEARN_SPECIALIZATION",       TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[134]*/    { "SPELL_EFFECT_KILL_CREDIT_GROUP",            TARGET_TYPE_UNIT,           TARGET_UNIT_CASTER },
    /*[135]*/    { "SPELL_EFFECT_CALL_PET",                     TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[136]*/    { "SPELL_EFFECT_HEAL_PCT",                     TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[137]*/    { "SPELL_EFFECT_ENERGIZE_PCT",                 TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[138]*/    { "SPELL_EFFECT_LEAP_BACK",                    TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[139]*/    { "SPELL_EFFECT_CLEAR_QUEST",                  TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[140]*/    { "SPELL_EFFECT_FORCE_CAST",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[141]*/    { "SPELL_EFFECT_FORCE_CAST_WITH_VALUE",        TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[142]*/    { "SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE",     TARGET_TYPE_DYNAMIC,        TARGET_NONE },
    /*[143]*/    { "SPELL_EFFECT_APPLY_AREA_AURA_OWNER",        TARGET_TYPE_SPECIAL_UNIT,   TARGET_NONE },
    /*[144]*/    { "SPELL_EFFECT_KNOCKBACK_FROM_POSITION",      TARGET_TYPE_UNIT_DEST,      TARGET_NONE },
    /*[145]*/    { "SPELL_EFFECT_PULL_TOWARDS_DEST",            TARGET_TYPE_UNIT_DEST,      TARGET_NONE },
    /*[146]*/    { "SPELL_EFFECT_ACTIVATE_RUNE",                TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[147]*/    { "SPELL_EFFECT_QUEST_FAIL",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[148]*/    { "SPELL_EFFECT_TRIGGER_MISSILE_SPELL_WITH_VALUE", TARGET_TYPE_DYNAMIC_ANY,  TARGET_NONE },
    /*[149]*/    { "SPELL_EFFECT_CHARGE_DEST",                  TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[150]*/    { "SPELL_EFFECT_QUEST_OFFER",                  TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[151]*/    { "SPELL_EFFECT_TRIGGER_SPELL_2",              TARGET_TYPE_LOCATION_DEST,  TARGET_NONE },
    /*[152]*/    { "SPELL_EFFECT_SUMMON_RAF_FRIEND",            TARGET_TYPE_NONE,           TARGET_NONE }, // TODO: Investigate when implementing RAF
    /*[153]*/    { "SPELL_EFFECT_CREATE_PET",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[154]*/    { "SPELL_EFFECT_TEACH_TAXI_NODE",              TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[155]*/    { "SPELL_EFFECT_TITAN_GRIP",                   TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[156]*/    { "SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC",       TARGET_TYPE_ITEM,           TARGET_NONE },
    /*[157]*/    { "SPELL_EFFECT_CREATE_ITEM_2",                TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[158]*/    { "SPELL_EFFECT_MILLING",                      TARGET_TYPE_ITEM,           TARGET_NONE },
    /*[159]*/    { "SPELL_EFFECT_ALLOW_RENAME_PET",             TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[160]*/    { "SPELL_EFFECT_FORCE_CAST_2",                 TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[161]*/    { "SPELL_EFFECT_TALENT_SPEC_COUNT",            TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[162]*/    { "SPELL_EFFECT_TALENT_SPEC_SELECT",           TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[163]*/    { "SPELL_EFFECT_163",                          TARGET_TYPE_UNIT,           TARGET_NONE },
    /*[164]*/    { "SPELL_EFFECT_CANCEL_AURA",                  TARGET_TYPE_UNIT,           TARGET_NONE },
};

std::map<uint32, SpellTargetingData> SpellTargetMgr::spellTargetingData;

SpellTargetingData& SpellTargetMgr::GetSpellTargetingData(uint32 spellId)
{
    return (*spellTargetingData.find(spellId)).second;
}

// check if at least one target is different from different effIdx
bool CheckBoundsEqualForEffects(SQLMultiStorage::SQLMSIteratorBounds<SpellTargetEntry>& bounds, SpellEffectIndex effIdxSource, SpellEffectIndex effIdxTarget)
{
    for (SQLMultiStorage::SQLMultiSIterator<SpellTargetEntry> i_spellST = bounds.first; i_spellST != bounds.second; ++i_spellST)
    {
        SpellTargetEntry const* spellST = (*i_spellST);
        if (spellST->CanNotHitWithSpellEffect(effIdxSource) != spellST->CanNotHitWithSpellEffect(effIdxTarget))
            return false;
    }
    return true;
}

void SpellTargetMgr::Initialize()
{
    for (uint32 i = 0; i <= sSpellTemplate.GetMaxEntry(); ++i)
    {
        SpellEntry const* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(i);
        if (!spellInfo)
            continue;

        SpellTargetingData& data = spellTargetingData[i];
        // figure out what targeting dynamic effects should use
        for (uint32 effIdx = 0; effIdx < MAX_EFFECT_INDEX; ++effIdx)
        {
            if (!spellInfo->Effect[effIdx])
                continue;

            uint32 effect = spellInfo->Effect[effIdx];
            uint32 targetA = spellInfo->EffectImplicitTargetA[effIdx];
            uint32 targetB = spellInfo->EffectImplicitTargetB[effIdx];
            if (SpellEffectInfoTable[effect].requiredTarget != TARGET_TYPE_DYNAMIC && SpellEffectInfoTable[effect].requiredTarget != TARGET_TYPE_DYNAMIC_ANY)
                data.implicitType[effIdx] = SpellEffectInfoTable[effect].requiredTarget;
            else
            {
                if (SpellTargetInfoTable[targetA].type == TARGET_TYPE_LOCK || SpellTargetInfoTable[targetB].type == TARGET_TYPE_LOCK)
                {
                    data.implicitType[effIdx] = TARGET_TYPE_LOCK;
                    continue;
                }
                if (SpellTargetInfoTable[targetA].type == TARGET_TYPE_CORPSE || SpellTargetInfoTable[targetB].type == TARGET_TYPE_CORPSE)
                {
                    data.implicitType[effIdx] = TARGET_TYPE_CORPSE;
                    continue;
                }
                if (SpellTargetInfoTable[targetA].type == TARGET_TYPE_GAMEOBJECT || SpellTargetInfoTable[targetB].type == TARGET_TYPE_GAMEOBJECT)
                {
                    data.implicitType[effIdx] = TARGET_TYPE_GAMEOBJECT;
                    continue;
                }
                if (SpellTargetInfoTable[targetA].type == TARGET_TYPE_LOCATION_TRAJ || SpellTargetInfoTable[targetB].type == TARGET_TYPE_LOCATION_TRAJ)
                {
                    data.implicitType[effIdx] = TARGET_TYPE_LOCATION_TRAJ;
                    continue;
                }
                if ((SpellTargetInfoTable[targetA].type == TARGET_TYPE_LOCATION_DEST && SpellTargetInfoTable[targetB].type == TARGET_TYPE_UNIT)
                        || (SpellTargetInfoTable[targetA].type == TARGET_TYPE_UNIT && SpellTargetInfoTable[targetB].type == TARGET_TYPE_LOCATION_DEST))
                {
                    data.implicitType[effIdx] = TARGET_TYPE_UNIT_DEST;
                    continue;
                }
                if (SpellTargetInfoTable[targetA].type == TARGET_TYPE_UNIT || SpellTargetInfoTable[targetB].type == TARGET_TYPE_UNIT)
                {
                    data.implicitType[effIdx] = TARGET_TYPE_UNIT;
                    continue;
                }
                if (SpellTargetInfoTable[targetA].type == TARGET_TYPE_LOCATION_DEST || SpellTargetInfoTable[targetB].type == TARGET_TYPE_LOCATION_DEST ||
                    SpellEffectInfoTable[effect].requiredTarget == TARGET_TYPE_DYNAMIC_ANY) // some effects cant be none
                {
                    data.implicitType[effIdx] = TARGET_TYPE_LOCATION_DEST;
                    continue;
                }
                if ((SpellTargetInfoTable[targetA].type == TARGET_TYPE_LOCATION_SRC || targetA == TARGET_NONE) && targetB == TARGET_NONE)
                {
                    data.implicitType[effIdx] = TARGET_TYPE_NONE;
                    continue;
                }
                sLog.outError("Spell %u effect index %u failed to pick type for dynamic effect targeting type.", i, effIdx);
            }
        }
        // evaluate which targets should be evaluated on execution
        for (uint32 effIdxSource = 0; effIdxSource < MAX_EFFECT_INDEX; ++effIdxSource)
        {
            if (!spellInfo->Effect[effIdxSource])
                continue;

            SpellEffectInfo& effectTargetingSource = SpellEffectInfoTable[spellInfo->Effect[effIdxSource]];

            SpellTargetImplicitType implicitEffectType = data.implicitType[effIdxSource];
            for (uint8 rightSource = 0; rightSource < 2; ++rightSource)
            {
                uint32 targetSource;
                if (rightSource == 0)
                {
                    if (data.ignoredTargets[effIdxSource].first) // already evaluated as ignored
                        continue;
                    targetSource = spellInfo->EffectImplicitTargetA[effIdxSource];
                }
                else
                {
                    if (data.ignoredTargets[effIdxSource].second) // already evaluated as ignored
                        continue;
                    targetSource = spellInfo->EffectImplicitTargetB[effIdxSource];
                }
                if (!targetSource) // no target found
                    continue;
                // start from first next target
                for (uint32 effIdxTarget = effIdxSource; effIdxTarget < MAX_EFFECT_INDEX; ++effIdxTarget)
                {
                    if (!spellInfo->Effect[effIdxTarget])
                        continue;

                    SpellEffectInfo& effectTargetingTarget = SpellEffectInfoTable[spellInfo->Effect[effIdxTarget]];

                    SpellTargetImplicitType implicitEffectTypeTarget = data.implicitType[effIdxTarget];
                    for (uint8 rightTarget = effIdxSource == effIdxTarget ? rightSource + 1 : 0; rightTarget < 2; ++rightTarget)
                    {
                        uint32 targetTarget;
                        if (rightTarget == 0)
                        {
                            if (data.ignoredTargets[effIdxTarget].first) // already evaluated as ignored
                                continue;
                            targetTarget = spellInfo->EffectImplicitTargetA[effIdxTarget];
                        }
                        else
                        {
                            if (data.ignoredTargets[effIdxTarget].second) // already evaluated as ignored
                                continue;
                            targetTarget = spellInfo->EffectImplicitTargetB[effIdxTarget];
                        }
                        if (!targetTarget) // no target found
                            continue;
                        if (targetSource == targetTarget) // if both target types are the same check if subsequent can be skipped
                        {
                            SpellTargetInfo& info = SpellTargetInfoTable[targetTarget];
                            bool ignore = false;
                            // exception for area auras
                            if (implicitEffectType == TARGET_TYPE_SPECIAL_UNIT && info.type == TARGET_TYPE_UNIT && info.enumerator != TARGET_ENUMERATOR_SINGLE)
                                ignore = true;
                            else if (implicitEffectTypeTarget == TARGET_TYPE_LOCATION_DEST && info.type == TARGET_TYPE_UNIT)
                                ignore = true;
                            else
                            {
                                switch (info.type)
                                {
                                    case TARGET_TYPE_LOCATION_SRC:
                                    case TARGET_TYPE_LOCATION_DEST:
                                    case TARGET_TYPE_LOCATION_TRAJ:
                                    case TARGET_TYPE_LOCK:
                                    case TARGET_TYPE_CORPSE: ignore = true; break; // always ignore subsequent
                                    case TARGET_TYPE_GAMEOBJECT:
                                    case TARGET_TYPE_UNIT:
                                    case TARGET_TYPE_PLAYER:
                                    {
                                        // always ignore subsequent
                                        if (info.enumerator == TARGET_ENUMERATOR_SINGLE && CanEffectConsumeTarget(info.type, effectTargetingSource.requiredTarget))
                                        {
                                            ignore = true;
                                            break;
                                        }
                                        // aoe and chain case
                                        if (info.enumerator == TARGET_ENUMERATOR_CHAIN) // chain checks chaining
                                            // if chain targets are not the same, cant ignore
                                            if (spellInfo->EffectChainTarget[effIdxSource] != spellInfo->EffectChainTarget[effIdxTarget])
                                                continue;
                                        // different radiuses, cant ignore
                                        if (spellInfo->EffectRadiusIndex[effIdxSource] != spellInfo->EffectRadiusIndex[effIdxTarget])
                                            continue;
                                        if (info.filter == TARGET_SCRIPT && effIdxSource != effIdxTarget)
                                        {
                                            SQLMultiStorage::SQLMSIteratorBounds<SpellTargetEntry> bounds = sSpellScriptTargetStorage.getBounds<SpellTargetEntry>(i);
                                            if (!CheckBoundsEqualForEffects(bounds, SpellEffectIndex(effIdxSource), SpellEffectIndex(effIdxTarget)))
                                                continue;
                                        }
                                        ignore = true;
                                        break;
                                    }
                                    default: break;
                                }
                                if (ignore)
                                    data.targetMask[effIdxSource][rightSource] |= (1 << effIdxTarget);
                            }
                            if (ignore)
                            {
                                auto& ignored = data.ignoredTargets[effIdxTarget];
                                if (rightTarget == 0)
                                    ignored.first = true;
                                else
                                    ignored.second = true;
                            }
                        }
                    }
                }
            }
        }
        for (uint32 effIdx = 0; effIdx < MAX_EFFECT_INDEX; ++effIdx)
        {
            if (!spellInfo->Effect[effIdx])
                continue;
            SpellTargetImplicitType implicitEffectType = data.implicitType[effIdx];
            for (uint8 right = 0; right < 2; ++right)
            {
                uint32 target;
                if (right == 0)
                    target = spellInfo->EffectImplicitTargetA[effIdx];
                else
                    target = spellInfo->EffectImplicitTargetB[effIdx];

                if (target)
                {
                    SpellTargetFilterScheme scheme = SCHEME_RANDOM;
                    if (SpellTargetInfoTable[target].enumerator == TARGET_ENUMERATOR_CHAIN)
                    {
                        switch (target)
                        {
                            case TARGET_UNIT_FRIEND_CHAIN_HEAL: scheme = SCHEME_LOWEST_HP_CHAIN; break;
                            case TARGET_UNIT_ENEMY_NEAR_CASTER:
                            case TARGET_UNIT_FRIEND_NEAR_CASTER:
                            case TARGET_UNIT_NEAR_CASTER:
                            case TARGET_UNIT_ENEMY: scheme = SCHEME_CLOSEST_CHAIN; break;
                            case TARGET_UNIT: scheme = SCHEME_RANDOM_CHAIN; break;
                            default: break;
                        }
                    }
                    scheme = GetSpellTargetingFilterScheme(scheme, spellInfo->Id);

                    if (scheme != SCHEME_RANDOM)
                        data.filteringScheme[effIdx][right] = scheme;
                }
            }
        }
    }
}

bool SpellTargetMgr::CanEffectBeFilledWithMask(uint32 spellId, uint32 effIdx, uint32 mask)
{
    SpellTargetingData& data = GetSpellTargetingData(spellId);
    switch (data.implicitType[effIdx])
    {
        case TARGET_TYPE_SPECIAL_DEST:
        case TARGET_TYPE_LOCATION_DEST: return bool(mask & TARGET_FLAG_DEST_LOCATION);
        case TARGET_TYPE_LOCATION_TRAJ: return bool(mask & (TARGET_FLAG_SOURCE_LOCATION | TARGET_FLAG_DEST_LOCATION));
        case TARGET_TYPE_GAMEOBJECT: return bool(mask & (TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_LOCKED));
        case TARGET_TYPE_UNIT_DEST: return bool(mask & (TARGET_FLAG_UNIT_ALLY | TARGET_FLAG_UNIT | TARGET_FLAG_UNIT_ENEMY | TARGET_FLAG_UNIT_DEAD | TARGET_FLAG_DEST_LOCATION));
        case TARGET_TYPE_PLAYER:
        case TARGET_TYPE_UNIT: return bool(mask & (TARGET_FLAG_UNIT_ALLY | TARGET_FLAG_UNIT | TARGET_FLAG_UNIT_ENEMY | TARGET_FLAG_UNIT_DEAD));
        case TARGET_TYPE_CORPSE: return bool(mask & (TARGET_FLAG_CORPSE_ENEMY | TARGET_FLAG_CORPSE_ALLY));
        case TARGET_TYPE_LOCK: return bool(mask & (TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_LOCKED | TARGET_FLAG_ITEM));
        case TARGET_TYPE_ITEM: return bool(mask & (TARGET_FLAG_LOCKED | TARGET_FLAG_ITEM));
        default: return false;
    }
}

bool SpellTargetMgr::CanEffectConsumeTarget(SpellTargetImplicitType targetType, SpellTargetImplicitType effectTargetType)
{
    switch (targetType)
    {
        // incomplete mapping - adjust as needed
        case TARGET_TYPE_UNIT: if (effectTargetType == TARGET_TYPE_LOCATION_DEST) return false; // 43178
        default: break;
    }
    return true;
}

float SpellTargetMgr::GetJumpRadius(uint32 spellId)
{
    switch (spellId)
    {
        case 32445: // Holy Wrath - Maiden of Virtue
            return 7.f;
        case 1064: // Chain Heal - 3.2.0
        case 10622:
        case 10623:
        case 25422:
        case 25423:
        case 55458:
        case 55459:
            return 12.5f;
        case 40827: // Beams - Mother Shahraz
        case 40859:
        case 40860:
        case 40861:
            return 25.f;
        case 44537: // Fel Lightning - MgT / SWP
        case 46048:
        case 46480:
            return 50.f;
        case 45664: // Legion Lightning - KJ
            return 18.f;
        case 62131: // Thorim - Chain Lightning - 5yd
            return 5.f;
    }
    return CHAIN_SPELL_JUMP_RADIUS;
}

SpellTargetFilterScheme SpellTargetMgr::GetSpellTargetingFilterScheme(SpellTargetFilterScheme oldScheme, uint32 spellId)
{
    switch (spellId)
    {
        case 26052: // Poison Bolt Volley (spell hits only the 15 closest targets)
        case 26180: // Wyvern Sting (spell hits only the 10 closest targets)
        case 30284: // Change Facing - Chess event - QOL to pick deterministically closest target
        case 37144: // Move - Chess event - same QOL change
        case 37146:
        case 37148:
        case 37151:
        case 37152:
        case 37153:
        case 41294: // Fixate - Reliquary of Souls - Picks closest target
            return SCHEME_CLOSEST;
        case 28307:
            return SCHEME_HIGHEST_HP;
        case 42005: // Bloodboil (spell hits only the 5 furthest away targets)
            return SCHEME_FURTHEST;
    }
    return oldScheme;
}
