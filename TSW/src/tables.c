/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/
 
/***************************************************************************
*	ROM 2.4 is copyright 1993-1998 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@hypercube.org)				   *
*	    Gabrielle Taylor (gtaylor@hypercube.org)			   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "tables.h"

/* for position */
const struct position_type position_table[] =
{
    {	"dead",			"dead"	},
    {	"mortally wounded",	"mort"	},
    {	"incapacitated",	"incap"	},
    {	"stunned",		"stun"	},
    {	"sleeping",		"sleep"	},
    {	"resting",		"rest"	},
    {   "sitting",		"sit"   },
    {	"fighting",		"fight"	},
    {	"standing",		"stand"	},
    {	NULL,			NULL	}
};

/* for sex */
const struct sex_type sex_table[] =
{
   {	"none"		},
   {	"male"		},
   {	"female"	},
   {	"either"	},
   {	NULL		}
};

/* for sizes */
const struct size_type size_table[] =
{ 
    {	"tiny"		},
    {	"small" 	},
    {	"medium"	},
    {	"large"		},
    {	"huge", 	},
    {	"giant" 	},
    {	NULL		}
};

/* various flag tables - guild */
const struct flag_type guild_flags[] =
{
  {  "Independent",	GUILD_INDEPENDENT, TRUE 	},
  {	"Modified",	GUILD_CHANGED,	    FALSE	},
  {	"Delete",		GUILD_DELETED,	    TRUE	},
  {  "Immortal",	GUILD_IMMORTAL,    TRUE	},
  {	NULL,		0,			    FALSE }
};

/* various flag tables - sguild */
const struct flag_type sguild_flags[] =
{
  {  "Independent",	SGUILD_INDEPENDENT, TRUE 	},
  {	"Modified",	SGUILD_CHANGED,     FALSE	},
  {	"Delete",		SGUILD_DELETED,     TRUE	     },
  {  "Immortal",	SGUILD_IMMORTAL,    TRUE	     },
  {	NULL,		0,			     FALSE     }
};

/* various flag tables - ssguild */
const struct flag_type ssguild_flags[] =
{
  {  "Independent",	SSGUILD_INDEPENDENT, TRUE 	},
  {	"Modified",	SSGUILD_CHANGED,	 FALSE	},
  {	"Delete",		SSGUILD_DELETED,	 TRUE	},
  {  "Immortal",	SSGUILD_IMMORTAL,    TRUE	},
  {	NULL,		0,			      FALSE    }
};

const struct flag_type gain_flags[] =
{
	{ "skill",			GAIN_SKILL,			TRUE	},
	{ "weave",			GAIN_WEAVE,			TRUE	},
	{ "newbweapons",	GAIN_NEWBWEAPONS,	TRUE	},
	{ "blademaster",	GAIN_BLADEMASTER,	TRUE	},
	{ "speardancer",	GAIN_SPEARDANCER,	TRUE	},
	{ "double xp",		GAIN_DOUBLE_XP,		TRUE	},
	{ "duelling",		GAIN_DUELLING,		TRUE	},
	{ "masterforms",	GAIN_MASTERFORMS,	TRUE	},
	{  NULL   ,      0,                FALSE }
};

const struct flag_type train_flags[] =
{
	{ "skill",			TRAIN_SKILL,		TRUE	},
	{ "weave",			TRAIN_WEAVE,		TRUE	},
	{ "weapons",		TRAIN_WEAPONS,		TRUE	},
	{ "blademaster",	TRAIN_BLADEMASTER,	TRUE	},
	{ "speardancer",	TRAIN_SPEARDANCER,	TRUE	},
	{ "duelling",		TRAIN_DUELLING,		TRUE	},
	{ "masterforms",	TRAIN_MASTERFORMS,	TRUE	},
	{  NULL   ,     0,                FALSE }
};

const struct flag_type act_flags[] =
{
    {	"npc",			A,	FALSE	},
    {	"sentinel",		B,	TRUE	},
    {	"scavenger",		C,	TRUE	},
    {     "banker",           D,   TRUE },
    {     "repairer",         E,   TRUE },
    {	"aggressive",		F,	TRUE	},
    {	"stay_area",		G,	TRUE	},
    {	"wimpy",		H,	TRUE	},
    {	"pet",			I,	TRUE	},
    {	"train",		J,	TRUE	},
    {     "restring",    K, TRUE },
    {	"undead",		O,	TRUE	},
    {	"cleric",		Q,	TRUE	},
    {	"thief",		S,	TRUE	},
    {	"warrior",		T,	TRUE	},
    {	"noalign",		U,	TRUE	},
    {	"nopurge",		V,	TRUE	},
    {	"outdoors",		W,	TRUE	},
    {	"indoors",		Y,	TRUE	},
    {	"rideable",		Z,	TRUE	},
    {	"healer",		aa,	TRUE	     },
    {	"gain",			bb,	TRUE	},
    {	"update_always",	cc,	TRUE	},
    {	"changer",		dd,	TRUE	},
    {	"roamingtrainer",	ACT_ROAMERTRAINER,	TRUE	},
    {	NULL,			0,	FALSE	}
};

const struct flag_type guild_guard_flags[] =
{
  {	"aggressive",   GGUARD_AGGRESSIVE,	  TRUE },
  {  "report_guild", GGUARD_REPORT_GUILD, TRUE },
  {  "block",        GGUARD_BLOCK,        TRUE },
  {  "shadowspawn",  GGUARD_SHADOWSPAWN,  TRUE },
  {  "race",         GGUARD_RACE,         TRUE },
  {	NULL,			0,	FALSE	}
};

const struct flag_type plr_flags[] =
{
    {	"npc",			A,	FALSE	},
    {	"autoassist",		C,	FALSE	},
    {	"autoexit",		D,	FALSE	},
    {	"autoloot",		E,	FALSE	},
    {	"autosac",		F,	FALSE	},
    {	"autogold",		G,	FALSE	},
    {	"autosplit",		H,	FALSE	},
    {	"holylight",		N,	FALSE	},
    {	"can_loot",		P,	FALSE	},
    {	"nosummon",		Q,	FALSE	},
    {	"nofollow",		R,	FALSE	},
    {	"colour",		T,	FALSE	},
    {	"permit",		U,	TRUE	},
    {	"log",			W,	FALSE	},
    {	"deny",			X,	FALSE	},
    {	"freeze",		Y,	FALSE	},
    {	"thief",		Z,	FALSE	},
    {	"killer",		aa,	FALSE	},
    {	NULL,			0,	0	}
};

const struct flag_type affect_flags[] =
{
    {	"blind",		A,	TRUE	},
    {	"invisible",		B,	TRUE	},
//    {	"fade",			C, TRUE		},
    {	"detect_invis",		D,	TRUE	},
    {	"detect_magic",		E,	TRUE	},
    {	"detect_hidden",	F,	TRUE	},
    {	"camouflage",		G,  TRUE	},
    {	"sanctuary",		H,	TRUE	},
    {	"faerie_fire",		I,	TRUE	},
    {	"infrared",		J,	TRUE	},
    {	"curse",		K,	TRUE	},
    {	"poison",		M,	TRUE	},
    {	"blindfolded",		N,	TRUE	},
    //{	"protect_good",		O,	TRUE	},
    {     "bind",        O,   TRUE },
    {	"sneak",		P,	TRUE	},
    {	"hide",			Q,	TRUE	},
    {	"sleep",		R,	TRUE	},
    {	"charm",		S,	TRUE	},
    {	"flying",		T,	TRUE	},
    {	"pass_door",		U,	TRUE	},
    {	"haste",		V,	TRUE	},
    {	"calm",			W,	TRUE	},
    {	"plague",		X,	TRUE	},
    {	"weaken",		Y,	TRUE	},
    {	"suffocating",		Z,	TRUE	},
    {	"berserk",		aa,	TRUE	},
/*    {	"swim",			bb,	TRUE	}, */
    {     "channeling",       bb,	TRUE	},
    {	"regeneration",		cc,	TRUE	},
    {	"slow",			dd,	TRUE	},
    {	NULL,			0,	0	}
};

const struct flag_type off_flags[] =
{
    {	"area_attack",		A,	TRUE	},
    {	"backstab",		B,	TRUE	},
    {	"bash",			C,	TRUE	},
    {	"berserk",		D,	TRUE	},
    {	"disarm",		E,	TRUE	},
    {	"dodge",		F,	TRUE	},
    {	"fade",			G,	TRUE	},
    {	"fast",			H,	TRUE	},
    {	"kick",			I,	TRUE	},
    {	"dirt_kick",		J,	TRUE	},
    {	"parry",		K,	TRUE	},
    {	"rescue",		L,	TRUE	},
    {	"tail",			M,	TRUE	},
    {	"trip",			N,	TRUE	},
    {	"crush",		O,	TRUE	},
    {	"assist_all",		P,	TRUE	},
    {	"assist_align",		Q,	TRUE	},
    {	"assist_race",		R,	TRUE	},
    {	"assist_players",	S,	TRUE	},
    {	"assist_guard",		T,	TRUE	},
    {	"assist_vnum",		U,	TRUE	},
    {	"howling",		V,	TRUE	},
    {	"target_head",		W,	TRUE	},
    {	"target_neck",		X,	TRUE	},
    {	"target_torso",		Y,	TRUE	},
    {   "target_back",          Z,      TRUE    },
    {	"target_arms",	        aa,	TRUE	},
    {   "target_legs",          bb,     TRUE    },
    {   "target_feet",          cc,     TRUE    },
    {	"target_hands",		dd,	TRUE	},
    {	"target_general",	ee,	TRUE	},
    {	 NULL,			0,	0	}
};

const struct flag_type imm_flags[] =
{
    {	"summon",		A,	TRUE	},
    {	"charm",		B,	TRUE	},
    {	"magic",		C,	TRUE	},
    {	"weapon",		D,	TRUE	},
    {	"bash",			E,	TRUE	},
    {	"pierce",		F,	TRUE	},
    {	"slash",		G,	TRUE	},
    {	"fire",			H,	TRUE	},
    {	"cold",			I,	TRUE	},
    {	"lightning",		J,	TRUE	},
    {	"acid",			K,	TRUE	},
    {	"poison",		L,	TRUE	},
    {	"negative",		M,	TRUE	},
    {	"holy",			N,	TRUE	},
    {	"energy",		O,	TRUE	},
    {	"wind",			IMM_WIND,	TRUE	},
    {	"disease",		Q,	TRUE	},
    {	"drowning",		R,	TRUE	},
    {	"light",		S,	TRUE	},
    {	"sound",		T,	TRUE	},
    {	"wood",			X,	TRUE	},
    {	"silver",		Y,	TRUE	},
    {	"iron",			Z,	TRUE	},
    {	"sap",			IMM_SAP,	TRUE	},
    {	"push",			IMM_PUSH,	TRUE	},
    {	"assassinate",	IMM_ASSASSINATE,	TRUE	},
    {	"arrow",		IMM_ARROW,	TRUE	},
    {	"harm",			IMM_HARM,	TRUE	},
    {	NULL,			0,	0	}
};

const struct flag_type form_flags[] =
{
    {	"edible",		FORM_EDIBLE,		TRUE	},
    {	"poison",		FORM_POISON,		TRUE	},
    {	"magical",		FORM_MAGICAL,		TRUE	},
    {	"instant_decay",	FORM_INSTANT_DECAY,	TRUE	},
    {	"other",		FORM_OTHER,		TRUE	},
    {	"animal",		FORM_ANIMAL,		TRUE	},
    {	"sentient",		FORM_SENTIENT,		TRUE	},
    {	"undead",		FORM_UNDEAD,		TRUE	},
    {	"construct",		FORM_CONSTRUCT,		TRUE	},
    {	"mist",			FORM_MIST,		TRUE	},
    {	"intangible",		FORM_INTANGIBLE,	TRUE	},
    {	"biped",		FORM_BIPED,		TRUE	},
    {	"centaur",		FORM_CENTAUR,		TRUE	},
    {	"insect",		FORM_INSECT,		TRUE	},
    {	"spider",		FORM_SPIDER,		TRUE	},
    {	"crustacean",		FORM_CRUSTACEAN,	TRUE	},
    {	"worm",			FORM_WORM,		TRUE	},
    {	"blob",			FORM_BLOB,		TRUE	},
    {	"mammal",		FORM_MAMMAL,		TRUE	},
    {	"bird",			FORM_BIRD,		TRUE	},
    {	"reptile",		FORM_REPTILE,		TRUE	},
    {	"snake",		FORM_SNAKE,		TRUE	},
    {	"dragon",		FORM_DRAGON,		TRUE	},
    {	"amphibian",		FORM_AMPHIBIAN,		TRUE	},
    {	"fish",			FORM_FISH ,		TRUE	},
    {	"cold_blood",		FORM_COLD_BLOOD,	TRUE	},
    {	NULL,			0,			0	}
};

const struct flag_type part_flags[] =
{
    {	"head",			PART_HEAD,		TRUE	},
    {	"arms",			PART_ARMS,		TRUE	},
    {	"legs",			PART_LEGS,		TRUE	},
    {	"heart",		PART_HEART,		TRUE	},
    {	"brains",		PART_BRAINS,		TRUE	},
    {	"guts",			PART_GUTS,		TRUE	},
    {	"hands",		PART_HANDS,		TRUE	},
    {	"feet",			PART_FEET,		TRUE	},
    {	"fingers",		PART_FINGERS,		TRUE	},
    {	"ear",			PART_EAR,		TRUE	},
    {	"eye",			PART_EYE,		TRUE	},
    {	"long_tongue",		PART_LONG_TONGUE,	TRUE	},
    {	"eyestalks",		PART_EYESTALKS,		TRUE	},
    {	"tentacles",		PART_TENTACLES,		TRUE	},
    {	"fins",			PART_FINS,		TRUE	},
    {	"wings",		PART_WINGS,		TRUE	},
    {	"tail",			PART_TAIL,		TRUE	},
    {	"claws",		PART_CLAWS,		TRUE	},
    {	"fangs",		PART_FANGS,		TRUE	},
    {	"horns",		PART_HORNS,		TRUE	},
    {	"scales",		PART_SCALES,		TRUE	},
    {	"tusks",		PART_TUSKS,		TRUE	},
    {	NULL,			0,			0	}
};

const struct flag_type comm_flags[] =
{
    {	"quiet",		COMM_QUIET,		TRUE	},
    {   "deaf",			COMM_DEAF,		TRUE	},
    {   "nowiz",		COMM_NOWIZ,		TRUE	},
    {   "noclangossip",		COMM_NOAUCTION,		TRUE	},
    {   "nogossip",		COMM_NOGOSSIP,		TRUE	},
//    {   "noquestion",		COMM_NOQUESTION,	TRUE	},
    {   "nosguild",      COMM_NOSGUILD, TRUE },
    {   "nomusic",		COMM_NOMUSIC,		TRUE	},
    {   "noclan",		COMM_NOCLAN,		TRUE	},
//    {   "shoutsoff",		COMM_SHOUTSOFF,		TRUE	},
    {	"norace",		COMM_NORACE,		TRUE	},
    {   "compact",		COMM_COMPACT,		TRUE	},
    {   "brief",		COMM_BRIEF,		TRUE	},
    {   "prompt",		COMM_PROMPT,		TRUE	},
    {   "combine",		COMM_COMBINE,		TRUE	},
    {   "telnet_ga",		COMM_TELNET_GA,		TRUE	},
    //    {   "show_affects",		COMM_SHOW_AFFECTS,	TRUE	},
    {   "nossguild",      COMM_NOSSGUILD, TRUE },
    {   "noemote",		COMM_NOEMOTE,		FALSE	},
    {   "noshout",		COMM_NOSHOUT,		FALSE	},
    {   "notell",		COMM_NOTELL,		FALSE	},
    {   "nochannels",		COMM_NOCHANNELS,	FALSE	},
    {   "snoop_proof",		COMM_SNOOP_PROOF,	FALSE	},
    {   "afk",			COMM_AFK,		TRUE	},
    {	NULL,			0,			0	}
};

const struct flag_type mprog_flags[] =
{
    {	"act",			TRIG_ACT,		TRUE	},
    {	"bribe",		TRIG_BRIBE,		TRUE 	},
    {	"death",		TRIG_DEATH,		TRUE    },
    {	"entry",		TRIG_ENTRY,		TRUE	},
    {	"fight",		TRIG_FIGHT,		TRUE	},
    {	"give",			TRIG_GIVE,		TRUE	},
    {	"greet",		TRIG_GREET,		TRUE    },
    {	"grall",		TRIG_GRALL,		TRUE	},
    {	"kill",			TRIG_KILL,		TRUE	},
    {	"hpcnt",		TRIG_HPCNT,		TRUE    },
    {	"random",		TRIG_RANDOM,		TRUE	},
    {	"speech",		TRIG_SPEECH,		TRUE	},
    {	"exit",			TRIG_EXIT,		TRUE    },
    {	"exall",		TRIG_EXALL,		TRUE    },
    {	"delay",		TRIG_DELAY,		TRUE    },
    {	"surr",			TRIG_SURR,		TRUE    },
    {	NULL,			0,			TRUE	}
};

const struct flag_type area_flags[] =
{
    {	"none",			AREA_NONE,		FALSE	},
    {	"changed",		AREA_CHANGED,		TRUE	},
    {	"added",		AREA_ADDED,		TRUE	},
    {	"loading",		AREA_LOADING,		FALSE	},
    {	"open",		        AREA_OPEN,		TRUE	},
    {	"immortal",		AREA_IMMORTAL,		TRUE	},
    {	"building",		AREA_BUILDING,		TRUE	},
    {	"secret",	        AREA_SECRET,		TRUE	},
    {	NULL,			0,			0	}
};



const struct flag_type sex_flags[] =
{
    {	"male",			SEX_MALE,		TRUE	},
    {	"female",		SEX_FEMALE,		TRUE	},
    {	"neutral",		SEX_NEUTRAL,		TRUE	},
    {   "random",               3,                      TRUE    },   /* ROM */
    {	"none",			SEX_NEUTRAL,		TRUE	},
    {	NULL,			0,			0	}
};



const struct flag_type exit_flags[] =
{
    {   "door",			EX_ISDOOR,		TRUE    },
    {	"closed",		EX_CLOSED,		TRUE	},
    {	"locked",		EX_LOCKED,		TRUE	},
    {	"pickproof",		EX_PICKPROOF,		TRUE	},
    {   "nopass",		EX_NOPASS,		TRUE	},
    {   "easy",			EX_EASY,		TRUE	},
    {   "hard",			EX_HARD,		TRUE	},
    {	"infuriating",		EX_INFURIATING,		TRUE	},
    {	"noclose",		EX_NOCLOSE,		TRUE	},
    {	"nolock",		EX_NOLOCK,		TRUE	},
    {	"hidden",		EX_HIDDEN,		TRUE	},
    {     "nobash",      EX_NOBASH,          TRUE },
    {   "firewall",      EX_FIREWALL,        TRUE },
    {   "airwall",       EX_AIRWALL,         TRUE },
    {   "blocked",       EX_BLOCKED,         TRUE },
    {   "barred",        EX_BARRED,         TRUE },
    {	NULL,			0,			0	}
};



const struct flag_type door_resets[] =
{
    {	"open and unlocked",	0,		TRUE	},
    {	"closed and unlocked",	1,		TRUE	},
    {	"closed and locked",	2,		TRUE	},
    {	NULL,			0,		0	}
};



const struct flag_type room_flags[] =
{
  {	"dark",		ROOM_DARK,		TRUE	},
  {	"noquest",	ROOM_NOQUEST,		TRUE	},
  {	"no_mob",		ROOM_NO_MOB,		TRUE	},
  {	"indoors",	ROOM_INDOORS,		TRUE	},
  {	"vmap",		ROOM_VMAP,		TRUE	},
  {	"private",	ROOM_PRIVATE,		TRUE },
  {	"safe",		ROOM_SAFE,		TRUE	},
  {	"solitary",	ROOM_SOLITARY,		TRUE	},
  {	"pet_shop",	ROOM_PET_SHOP,		TRUE	},
  {	"no_recall",	ROOM_NO_RECALL,	TRUE	},
  {	"imp_only",	ROOM_IMP_ONLY,		TRUE },
  {	"gods_only",	ROOM_GODS_ONLY,	TRUE },
  {	"heroes_only",	ROOM_HEROES_ONLY,	TRUE	},
  {	"newbies_only",ROOM_NEWBIES_ONLY,	TRUE	},
  {	"law",		ROOM_LAW,		     TRUE	},
  {   "nowhere",	ROOM_NOWHERE,		TRUE	},
  {   "stedding",   	ROOM_STEDDING,      TRUE },
  {  "no_gate",     	ROOM_NO_GATE,       TRUE },
  {  "arena",     	ROOM_ARENA,       TRUE },
  {  "deathtrap",     	ROOM_DEATHTRAP,       TRUE },
  {  "mining_gold", 	ROOM_MINING_GOLD,	TRUE },
  {  "mining_silver", 	ROOM_MINING_SILVER,	TRUE },
  {  "mining_copper", 	ROOM_MINING_COPPER,	TRUE },
  {  "mining_emerald", 	ROOM_MINING_EMERALD,	TRUE },
  {  "mining_diamond",   ROOM_MINING_DIAMOND,    TRUE },
  {  "mining_ruby",      ROOM_MINING_RUBY,       TRUE },  
{	NULL,		0,			     0	}
};



const struct flag_type sector_flags[] =
{
    {	"inside",	SECT_INSIDE,		TRUE	},
    {	"city",		SECT_CITY,		   TRUE	},
    {	"field",	   SECT_FIELD,		   TRUE	},
    {	"forest",	SECT_FOREST,		TRUE	},
    {	"hills",	   SECT_HILLS,		   TRUE	},
    {	"mountain",	SECT_MOUNTAIN,		TRUE	},
    {	"swim",		SECT_WATER_SWIM,	TRUE	},
    {	"noswim",	SECT_WATER_NOSWIM,TRUE	},
    { "unused",	SECT_UNUSED,		TRUE	},
    {	"air",		SECT_AIR,		   TRUE	},
    {	"desert",	SECT_DESERT,		TRUE	},
	 { "ways",		SECT_WAYS,		   TRUE	},
	 { "rock_mountain",	SECT_ROCK_MOUNTAIN,		   TRUE	},
	 { "snow_mountain",	SECT_SNOW_MOUNTAIN,		   TRUE	},
	 { "road",		SECT_ROAD,		   TRUE	},
	 { "enter",		SECT_ENTER,		   TRUE	},
	 { "swamp",		SECT_SWAMP,		   TRUE	},
    { "jungle",		SECT_JUNGLE,		   TRUE	},
	 { "ruins",		SECT_RUINS,		   TRUE	},
	 { "ocean",		SECT_OCEAN,		   TRUE	},
	 { "river",		SECT_RIVER,		   TRUE	},
	 { "sand",		SECT_SAND,		   TRUE	},
	 { "blight",	SECT_BLIGHT,		   TRUE	},
	 { "island",	SECT_ISLAND,		   TRUE	},
	 { "lake",		SECT_LAKE,		   TRUE	},
    {	NULL,		   0,			         0	   }
   };



const struct flag_type type_flags[] =
{
    {	"light",		ITEM_LIGHT,		TRUE	},
    {	"scroll",		ITEM_SCROLL,		TRUE	},
    {	"wand",			ITEM_WAND,		TRUE	},
    {	"staff",		ITEM_STAFF,		TRUE	},
    {	"weapon",		ITEM_WEAPON,		TRUE	},
    {	"treasure",		ITEM_TREASURE,		TRUE	},
    {	"armor",		ITEM_ARMOR,		TRUE	},
    {	"potion",		ITEM_POTION,		TRUE	},
    {	"furniture",		ITEM_FURNITURE,		TRUE	},
    {	"trash",		ITEM_TRASH,		TRUE	},
    {	"container",		ITEM_CONTAINER,		TRUE	},
    {	"drinkcontainer",	ITEM_DRINK_CON,		TRUE	},
    {	"key",			ITEM_KEY,		TRUE	},
    {	"food",			ITEM_FOOD,		TRUE	},
    {	"money",		ITEM_MONEY,		TRUE	},
    {	"boat",			ITEM_BOAT,		TRUE	},
    {	"npccorpse",		ITEM_CORPSE_NPC,	TRUE	},
    {	"pc corpse",		ITEM_CORPSE_PC,		FALSE	},
    {	"fountain",		ITEM_FOUNTAIN,		TRUE	},
    {	"pill",			ITEM_PILL,		TRUE	},
    {	"protect",		ITEM_PROTECT,		TRUE	},
    {	"map",			ITEM_MAP,		TRUE	},
    {   "portal",		ITEM_PORTAL,		TRUE	},
    {   "warpstone",		ITEM_WARP_STONE,	TRUE	},
    {	"roomkey",		ITEM_ROOM_KEY,		TRUE	},
    { 	"gem",			ITEM_GEM,		TRUE	},
    {	"jewelry",		ITEM_JEWELRY,		TRUE	},
    {	"jukebox",		ITEM_JUKEBOX,		TRUE	},
    {   "token",                ITEM_TOKEN,             TRUE    },
    {   "angreal",              ITEM_ANGREAL,           TRUE    },
    {   "firewall",             ITEM_FIREWALL,        TRUE },
    {   "airwall",              ITEM_AIRWALL,         TRUE },
    {   "notepaper",            ITEM_NOTEPAPER,         TRUE },
    {	"vehicle",		ITEM_VEHICLE,		TRUE	},
    {	"ore",			ITEM_ORE,		TRUE	},
    {	"gemstone",		ITEM_GEMSTONE,		TRUE	},
    {	NULL,			0,			0	}
};


const struct flag_type ore_types[] = 
{
    {	"copper",		MINING_ORE_COPPER,		TRUE	},
    {	"silver",		MINING_ORE_SILVER,		TRUE	},
    {	"gold",			MINING_ORE_GOLD,		TRUE	},
    {	NULL,			0,			0	}
	
};
const struct flag_type gem_types[] = 
{
    {	"ruby",		        MINING_GEM_RUBY,		TRUE	},
    {	"diamond",		MINING_GEM_DIAMOND,		TRUE	},
    {	"emerald",		MINING_GEM_EMERALD,		TRUE	},
    {	NULL,			0,			0	}
	
};

const struct flag_type mining_quality[] = 
{
    {	"flawed",		MINING_QUALITY_FLAWED,		TRUE	},
    {	"flawless",		MINING_QUALITY_FLAWLESS,	TRUE	},
    {	"excellent",		MINING_QUALITY_EXCELLENT,	TRUE	},
    {	"perfect",		MINING_QUALITY_PERFECT,		TRUE	},
    {	NULL,			0,			0	}
	
};



const struct flag_type extra_flags[] =
{
    {	"glow",        ITEM_GLOW,           TRUE	},
    {	"hum",         ITEM_HUM,            TRUE	},
    {	"hidden",      ITEM_HIDDEN,         TRUE	},
    {	"dark",        ITEM_DARK,           TRUE	},
    {	"lock",        ITEM_LOCK,           TRUE	},
    {	"evil",        ITEM_EVIL,           TRUE	},
    {	"invis",       ITEM_INVIS,          TRUE	},
    {	"magic",       ITEM_MAGIC,          TRUE	},
    {	"nodrop",      ITEM_NODROP,         TRUE	},
    {	"bless",       ITEM_BLESS,          TRUE	},
    {	"randomlocation",    ITEM_REPOP_RANDOM_LOCATION,       TRUE	},
    {	"chancerepop", ITEM_REPOP_ON_CHANCE,       TRUE	},
    {	"fullrandom", ITEM_FULL_RANDOM,       TRUE	},
    {	"noremove",    ITEM_NOREMOVE,       TRUE	},
    {	"noremove",    ITEM_NOREMOVE,       TRUE	},
    {	"nosteal",     ITEM_NOSTEAL,        TRUE	},
    {	"inventory",   ITEM_INVENTORY,      TRUE	},
    {	"nopurge",     ITEM_NOPURGE,        TRUE	},
    {	"rotdeath",    ITEM_ROT_DEATH,      TRUE	},
    {	"visdeath",    ITEM_VIS_DEATH,      TRUE	},
    {     "nonmetal",    ITEM_NONMETAL,       TRUE	},
    {	"meltdrop",    ITEM_MELT_DROP,      TRUE	},
    {	"hadtimer",    ITEM_HAD_TIMER,      TRUE	},
    {	"sellextract", ITEM_SELL_EXTRACT,   TRUE	},
    {	"burnproof",   ITEM_BURN_PROOF,     TRUE	},
    {	"draggable",   ITEM_DRAGGABLE,      TRUE	},
    {	"nouncurse",   ITEM_NOUNCURSE,      TRUE	},
    {     "broken",     ITEM_BROKEN,         TRUE     },
    {     "nobreak",    ITEM_NO_BREAK,       TRUE     },
    {   "keeper",    	ITEM_KEEPER, 	        TRUE },
    {	NULL,          0,                   0	   }
};



const struct flag_type wear_flags[] =
{
    {	"take",			ITEM_TAKE,		TRUE	},
    {	"finger",		ITEM_WEAR_FINGER,	TRUE	},
    {	"neck",			ITEM_WEAR_NECK,		TRUE	},
    {	"body",			ITEM_WEAR_BODY,		TRUE	},
    {	"head",			ITEM_WEAR_HEAD,		TRUE	},
    {	"legs",			ITEM_WEAR_LEGS,		TRUE	},
    {	"feet",			ITEM_WEAR_FEET,		TRUE	},
    {	"hands",		ITEM_WEAR_HANDS,	TRUE	},
    {	"arms",			ITEM_WEAR_ARMS,		TRUE	},
    {	"shield",		ITEM_WEAR_SHIELD,	TRUE	},
    {	"about",		ITEM_WEAR_ABOUT,	TRUE	},
    {	"waist",		ITEM_WEAR_WAIST,	TRUE	},
    {	"wrist",		ITEM_WEAR_WRIST,	TRUE	},
    {	"wield",		ITEM_WIELD,		TRUE	},
    {	"hold",			ITEM_HOLD,		TRUE	},
    {   "nosac",		ITEM_NO_SAC,		TRUE	},
    {	"wearfloat",		ITEM_WEAR_FLOAT,	TRUE	},
    {	"tattoo",		ITEM_WEAR_TATTOO,	TRUE	},
    {	"stuck_in",		ITEM_STUCK_IN,	TRUE	},
    {	"back",			ITEM_WEAR_BACK,		TRUE	},
    {	"ear",			ITEM_WEAR_EAR,		TRUE	},
    {	"face",			ITEM_WEAR_FACE,		TRUE	},
    {	"maleonly",		ITEM_WEAR_MALE_ONLY,	TRUE	},
    {	"femaleonly",		ITEM_WEAR_FEMALE_ONLY,	TRUE	},
/*    {   "twohands",            ITEM_TWO_HANDS,         TRUE    }, */
    {	NULL,			0,			0	}
};

/*
 * Used when adding an affect to tell where it goes.
 * See addaffect and delaffect in act_olc.c
 */
const struct flag_type apply_flags[] =
{
    {	"none",			APPLY_NONE,		TRUE	},
    {	"strength",		APPLY_STR,		TRUE	},
    {	"dexterity",		APPLY_DEX,		TRUE	},
    {	"intelligence",		APPLY_INT,		TRUE	},
    {	"wisdom",		APPLY_WIS,		TRUE	},
    {	"constitution",		APPLY_CON,		TRUE	},
    {	"sex",			APPLY_SEX,		TRUE	},
    {	"class",		APPLY_CLASS,		TRUE	},
    {	"level",		APPLY_LEVEL,		TRUE	},
    {	"age",			APPLY_AGE,		TRUE	},
    {	"height",		APPLY_HEIGHT,		TRUE	},
    {	"weight",		APPLY_WEIGHT,		TRUE	},
    {	"endurance",			APPLY_ENDURANCE,		TRUE	},
    {	"hp",			APPLY_HIT,		TRUE	},
    {	"gold",			APPLY_GOLD,		TRUE	},
    {	"experience",		APPLY_EXP,		TRUE	},
    {	"ac",			APPLY_AC,		TRUE	},
    {	"hitroll",		APPLY_HITROLL,		TRUE	},
    {	"damroll",		APPLY_DAMROLL,		TRUE	},
    {	"saves",		APPLY_SAVES,		TRUE	},
    {	"savingpara",		APPLY_SAVING_PARA,	TRUE	},
    {	"savingrod",		APPLY_SAVING_ROD,	TRUE	},
    {	"savingpetri",		APPLY_SAVING_PETRI,	TRUE	},
    {	"savingbreath",		APPLY_SAVING_BREATH,	TRUE	},
    {	"savingspell",		APPLY_SAVING_SPELL,	TRUE	},
    {	"spellaffect",		APPLY_SPELL_AFFECT,	FALSE	},

    {     "air",              APPLY_SPHERE_AIR,    TRUE },
    {     "earth",            APPLY_SPHERE_EARTH,  TRUE },
    {     "fire",             APPLY_SPHERE_FIRE,   TRUE },
    {     "spirit",           APPLY_SPHERE_SPIRIT, TRUE },
    {     "water",            APPLY_SPHERE_WATER,  TRUE },

    {	NULL,			0,			0	}
};



/*
 * What is seen.
 */
const struct flag_type wear_loc_strings[] =
{
    {	"in the inventory",	WEAR_NONE,	TRUE	},
    {	"as a light",		WEAR_LIGHT,	TRUE	},
    {	"floating nearby",	WEAR_FLOAT,	TRUE	},
    {	"tattoed",		WEAR_TATTOO,	TRUE	},
    {	"on the left finger",	WEAR_FINGER_L,	TRUE	},
    {	"on the right finger",	WEAR_FINGER_R,	TRUE	},
    {	"around the neck (1)",	WEAR_NECK_1,	TRUE	},
    {	"around the neck (2)",	WEAR_NECK_2,	TRUE	},
    {	"on the body",		WEAR_BODY,	TRUE	},
    {	"over the head",	WEAR_HEAD,	TRUE	},
    {	"on the legs",		WEAR_LEGS,	TRUE	},
    {	"on the feet",		WEAR_FEET,	TRUE	},
    {	"on the hands",		WEAR_HANDS,	TRUE	},
    {	"on the arms",		WEAR_ARMS,	TRUE	},
    {	"as a shield",		WEAR_SHIELD,	TRUE	},
    {	"on the back",		WEAR_BACK,	TRUE	},
    {	"about the shoulders",	WEAR_ABOUT,	TRUE	},
    {	"around the waist",	WEAR_WAIST,	TRUE	},
    {	"on the left wrist",	WEAR_WRIST_L,	TRUE	},
    {	"on the right wrist",	WEAR_WRIST_R,	TRUE	},
    {	"wielded",		WEAR_WIELD,	TRUE	},
    {	"duel-wielded",		WEAR_SECOND_WIELD,	TRUE	},
    {	"held in the hands",	WEAR_HOLD,	TRUE	},
    {	"sheathed",		WEAR_SCABBARD_1,	TRUE	},
    {	"in the left ear",	WEAR_EAR_L,	TRUE	},
    {	"in the right ear",	WEAR_EAR_R,	TRUE	},
    {	"over the face",	WEAR_FACE,	TRUE	},

    {	NULL,			0	      , 0	}
};


const struct flag_type wear_loc_flags[] =
{
    {	"none",		WEAR_NONE,	TRUE	},
    {	"light",	WEAR_LIGHT,	TRUE	},
    {	"floating",	WEAR_FLOAT,	TRUE	},    
    {	"tattoo",	WEAR_TATTOO,	TRUE	},    
    {	"lfinger",	WEAR_FINGER_L,	TRUE	},
    {	"rfinger",	WEAR_FINGER_R,	TRUE	},
    {	"neck1",	WEAR_NECK_1,	TRUE	},
    {	"neck2",	WEAR_NECK_2,	TRUE	},
    {	"body",		WEAR_BODY,	TRUE	},
    {	"head",		WEAR_HEAD,	TRUE	},
    {	"legs",		WEAR_LEGS,	TRUE	},
    {	"feet",		WEAR_FEET,	TRUE	},
    {	"hands",	WEAR_HANDS,	TRUE	},
    {	"arms",		WEAR_ARMS,	TRUE	},
    {	"shield",	WEAR_SHIELD,	TRUE	},
    {	"back",		WEAR_BACK,	TRUE	},
    {	"about",	WEAR_ABOUT,	TRUE	},
    {	"waist",	WEAR_WAIST,	TRUE	},
    {	"lwrist",	WEAR_WRIST_L,	TRUE	},
    {	"rwrist",	WEAR_WRIST_R,	TRUE	},
    {	"wielded",	WEAR_WIELD,	TRUE	},
    {	"dual-wielded",	WEAR_SECOND_WIELD,	TRUE	},
    {	"hold",		WEAR_HOLD,	TRUE	},
    {	"sheathed",	WEAR_SCABBARD_1,	TRUE	},
    {	"lear",		WEAR_EAR_L,	TRUE	},
    {	"rear",		WEAR_EAR_R,	TRUE	},
    {	"face",		WEAR_FACE,	TRUE	},
    
    {	NULL,		0,		0	}
};

const struct flag_type container_flags[] =
{
    {	"closeable",		1,		TRUE	},
    {	"pickproof",		2,		TRUE	},
    {	"closed",		4,		TRUE	},
    {	"locked",		8,		TRUE	},
    {	"puton",		16,		TRUE	},
    {	"enterable",		32,		TRUE	},
    {	"soundproof",		64,		TRUE	},
    {	"seeout",		128,		TRUE	},
    {	"seein",		256,		TRUE	},
    {     "seethrough",  CONT_SEE_THROUGH, TRUE },
    {	NULL,			0,		0	}
};

/*****************************************************************************
                      ROM - specific tables:
 ****************************************************************************/




const struct flag_type ac_type[] =
{
    {   "pierce",        AC_PIERCE,            TRUE    },
    {   "bash",          AC_BASH,              TRUE    },
    {   "slash",         AC_SLASH,             TRUE    },
    {   "exotic",        AC_EXOTIC,            TRUE    },
    {   NULL,              0,                    0       }
};


const struct flag_type size_flags[] =
{
    {   "tiny",          SIZE_TINY,            TRUE    },
    {   "small",         SIZE_SMALL,           TRUE    },
    {   "medium",        SIZE_MEDIUM,          TRUE    },
    {   "large",         SIZE_LARGE,           TRUE    },
    {   "huge",          SIZE_HUGE,            TRUE    },
    {   "giant",         SIZE_GIANT,           TRUE    },
    {   NULL,              0,                    0       },
};


const struct flag_type weapon_class[] =
{
    {   "exotic" ,	WEAPON_EXOTIC , TRUE    },
    {   "sword"  ,	WEAPON_SWORD  , TRUE    },
    {   "dagger" ,	WEAPON_DAGGER , TRUE    },
    {   "spear"  ,	WEAPON_SPEAR  , TRUE    },
    {   "mace"   ,	WEAPON_MACE   , TRUE    },
    {   "axe"    ,	WEAPON_AXE    , TRUE    },
    {   "flail"  ,	WEAPON_FLAIL  , TRUE    },
    {   "whip"   ,	WEAPON_WHIP   , TRUE    },
    {   "polearm",	WEAPON_POLEARM, TRUE    },
    {   "arrow"  ,	WEAPON_ARROW  , TRUE    },
    {   "bow"    ,	WEAPON_BOW    , TRUE    },
    {   "lance"  ,	WEAPON_LANCE  , TRUE    },
    {   "staff"  ,  WEAPON_STAFF  , TRUE    },
    {   NULL     ,	0             , 0       }
};


const struct flag_type weapon_type2[] =
{
    {   "flaming",   WEAPON_FLAMING,       TRUE    },
    {   "frost",     WEAPON_FROST,         TRUE    },
    {   "vampiric",  WEAPON_VAMPIRIC,      TRUE    },
    {   "sharp",     WEAPON_SHARP,         TRUE    },
    {   "vorpal",    WEAPON_VORPAL,        TRUE    },
    {   "twohands",  WEAPON_TWO_HANDS,     TRUE    },
    {   "shocking",	 WEAPON_SHOCKING,      TRUE    },
    {   "poison",	 WEAPON_POISON,        TRUE	 },
    {   NULL,        0,                    0       }
};

const struct flag_type res_flags[] =
{
    {	"summon",	 RES_SUMMON,		TRUE	},
    {   "charm",         RES_CHARM,            TRUE    },
    {   "magic",         RES_MAGIC,            TRUE    },
    {   "weapon",        RES_WEAPON,           TRUE    },
    {   "bash",          RES_BASH,             TRUE    },
    {   "pierce",        RES_PIERCE,           TRUE    },
    {   "slash",         RES_SLASH,            TRUE    },
    {   "fire",          RES_FIRE,             TRUE    },
    {   "cold",          RES_COLD,             TRUE    },
    {   "lightning",     RES_LIGHTNING,        TRUE    },
    {   "acid",          RES_ACID,             TRUE    },
    {   "poison",        RES_POISON,           TRUE    },
    {   "negative",      RES_NEGATIVE,         TRUE    },
    {   "holy",          RES_HOLY,             TRUE    },
    {   "energy",        RES_ENERGY,           TRUE    },
    {   "wind",          RES_WIND,           TRUE    },
    {   "disease",       RES_DISEASE,          TRUE    },
    {   "drowning",      RES_DROWNING,         TRUE    },
    {   "light",         RES_LIGHT,            TRUE    },
    {	"sound",	RES_SOUND,		TRUE	},
    {	"wood",		RES_WOOD,		TRUE	},
    {	"silver",	RES_SILVER,		TRUE	},
    {	"iron",		RES_IRON,		TRUE	},
    {   NULL,          0,            0    }
};


const struct flag_type vuln_flags[] =
{
    {	"summon",	 VULN_SUMMON,		TRUE	},
    {	"charm",	VULN_CHARM,		TRUE	},
    {   "magic",         VULN_MAGIC,           TRUE    },
    {   "weapon",        VULN_WEAPON,          TRUE    },
    {   "bash",          VULN_BASH,            TRUE    },
    {   "pierce",        VULN_PIERCE,          TRUE    },
    {   "slash",         VULN_SLASH,           TRUE    },
    {   "fire",          VULN_FIRE,            TRUE    },
    {   "cold",          VULN_COLD,            TRUE    },
    {   "lightning",     VULN_LIGHTNING,       TRUE    },
    {   "acid",          VULN_ACID,            TRUE    },
    {   "poison",        VULN_POISON,          TRUE    },
    {   "negative",      VULN_NEGATIVE,        TRUE    },
    {   "holy",          VULN_HOLY,            TRUE    },
    {   "energy",        VULN_ENERGY,          TRUE    },
    {   "wind",          VULN_WIND,          TRUE    },
    {   "disease",       VULN_DISEASE,         TRUE    },
    {   "drowning",      VULN_DROWNING,        TRUE    },
    {   "light",         VULN_LIGHT,           TRUE    },
    {	"sound",	 VULN_SOUND,		TRUE	},
    {   "wood",          VULN_WOOD,            TRUE    },
    {   "silver",        VULN_SILVER,          TRUE    },
    {   "iron",          VULN_IRON,            TRUE    },
    {   NULL,              0,                    0       }
};

const struct flag_type position_flags[] =
{
    {   "dead",           POS_DEAD,            FALSE   },
    {   "mortal",         POS_MORTAL,          FALSE   },
    {   "incap",          POS_INCAP,           FALSE   },
    {   "stunned",        POS_STUNNED,         FALSE   },
    {   "sleeping",       POS_SLEEPING,        TRUE    },
    {   "resting",        POS_RESTING,         TRUE    },
    {   "sitting",        POS_SITTING,         TRUE    },
    {   "fighting",       POS_FIGHTING,        FALSE   },
    {   "standing",       POS_STANDING,        TRUE    },
    {   NULL,              0,                    0       }
};

const struct flag_type portal_flags[]=
{
    {   "normal_exit",	  GATE_NORMAL_EXIT,	TRUE	},
    {	"no_curse",	  GATE_NOCURSE,		TRUE	},
    {   "go_with",	  GATE_GOWITH,		TRUE	},
    {   "buggy",	  GATE_BUGGY,		TRUE	},
    {	"random",	  GATE_RANDOM,		TRUE	},
    {   "waygate",	   GATE_WAYGATE,		},
    {   "dreamgate",   GATE_DREAMGATE, TRUE },
    {   NULL,		  0,			0	}

};

const struct flag_type furniture_flags[]=
{
    {   "stand_at",	  STAND_AT,		TRUE	},
    {	"stand_on",	  STAND_ON,		TRUE	},
    {	"stand_in",	  STAND_IN,		TRUE	},
    {	"sit_at",	  SIT_AT,		TRUE	},
    {	"sit_on",	  SIT_ON,		TRUE	},
    {	"sit_in",	  SIT_IN,		TRUE	},
    {	"rest_at",	  REST_AT,		TRUE	},
    {	"rest_on",	  REST_ON,		TRUE	},
    {	"rest_in",	  REST_IN,		TRUE	},
    {	"sleep_at",	  SLEEP_AT,		TRUE	},
    {	"sleep_on",	  SLEEP_ON,		TRUE	},
    {	"sleep_in",	  SLEEP_IN,		TRUE	},
    {	"put_at",	  PUT_AT,		TRUE	},
    {	"put_on",	  PUT_ON,		TRUE	},
    {	"put_in",	  PUT_IN,		TRUE	},
    {	"put_inside",	  PUT_INSIDE,		TRUE	},
    {	NULL,		  0,			0	}
};

const	struct	flag_type	apply_types	[]	=
{
	{	"affects",	TO_AFFECTS,	TRUE	},
	{	"object",	TO_OBJECT,	TRUE	},
	{	"immune",	TO_IMMUNE,	TRUE	},
	{	"resist",	TO_RESIST,	TRUE	},
	{	"vuln",		TO_VULN,	TRUE	},
	{	"weapon",	TO_WEAPON,	TRUE	},
	{    "room",   TO_AFF_ROOM,   TRUE },
	{	NULL,		0,		TRUE	}
};

const	struct	bit_type	bitvector_type	[]	=
{
	{	affect_flags,	"affect"	},
	{	apply_flags,	"apply"		},
	{	imm_flags,	"imm"		},
	{	res_flags,	"res"		},
	{	vuln_flags,	"vuln"		},
	{	weapon_type2,	"weapon"	}
};

const	struct	flag_type	target_table	[]	=
{
	{	"tar_ignore",		TAR_IGNORE,		TRUE	},
	{	"tar_char_offensive",	TAR_CHAR_OFFENSIVE,	TRUE	},
	{	"tar_char_defensive",	TAR_CHAR_DEFENSIVE,	TRUE	},
	{	"tar_char_self",	TAR_CHAR_SELF,		TRUE	},
	{	"tar_obj_inv",		TAR_OBJ_INV,		TRUE	},
	{	"tar_obj_char_def",	TAR_OBJ_CHAR_DEF,	TRUE	},
	{	"tar_obj_char_off",	TAR_OBJ_CHAR_OFF,	TRUE	},
	{    "tar_char_other",   TAR_CHAR_OTHER,     TRUE },
	{	NULL,			0,			TRUE	}
};

const  struct	flag_type restriction_table [] =
{
  { "res_normal"  , RES_NORMAL , TRUE },
  { "res_noteach" , RES_NOTEACH, TRUE },
  { "res_nogain"  , RES_NOGAIN , TRUE },
  { "res_granted" , RES_GRANTED, TRUE },
  { "res_male"    , RES_MALE,    TRUE },
  { "res_female"  , RES_FEMALE,  TRUE },
  { "res_talent"  , RES_TALENT,  TRUE },
  { "res_notrain" , RES_NOTRAIN, TRUE },  
  { "res_trainsamesex" , RES_TRAINSAMESEX, TRUE },  
  {	NULL      , 0          , TRUE }
};

const struct flag_type world_table [] =
{
  { "Normal",       WORLD_NORMAL,    TRUE },
  { "Dreamwalking", WORLD_TAR_FLESH, TRUE },
  { "Dreaming",     WORLD_TAR_DREAM, TRUE },
  { "Skimming",     WORLD_SKIMMING,  TRUE },
  { NULL,           0,               TRUE }
};
 

const	struct	recval_type	recval_table	[]	=
{
	/*      2d6	    +   10,     AC,     dam			}, */
	{	2,	6,	10,	9,	1,	4,	0	},
	{	2,	7,	21,	8,	1,	5,	0	},
	{	2,	6,	35,	7,	1,	6,	0	},
	{	2,	7,	46,	6,	1,	5,	1	},
	{	2,	6,	60,	5,	1,	6,	1	},
	{	2,	7,	71,	4,	1,	7,	1	},
	{	2,	6,	85,	4,	1,	8,	1	},
	{	2,	7,	96,	3,	1,	7,	2	},
	{	2,	6,	110,	2,	1,	8,	2	},
	{	2,	7,	121,	1,	2,	4,	2	}, /* 10 */
	{	2,	8,	134,	1,	1,	10,	2	},
	{	2,	10,	150,	0,	1,	10,	3	},
	{	2,	10,	170,	-1,	2,	5,	3	},
	{	2,	10,	190,	-1,	1,	12,	3	},
	{	3,	9,	208,	-2,	2,	6,	3	},
	{	3,	9,	233,	-2,	2,	6,	4	},
	{	3,	9,	258,	-3,	3,	4,	4	},
	{	3,	9,	283,	-3,	2,	7,	4	},
	{	3,	9,	308,	-4,	2,	7,	5	},
	{	3,	9,	333,	-4,	2,	8,	5	}, /* 20 */
	{	4,	10,	360,	-5,	4,	4,	5	},
	{	5,	10,	400,	-5,	4,	4,	6	},
	{	5,	10,	450,	-6,	3,	6,	6	},
	{	5,	10,	500,	-6,	2,	10,	6	},
	{	5,	10,	550,	-7,	2,	10,	7	},
	{	5,	10,	600,	-7,	3,	7,	7	},
	{	5,	10,	650,	-8,	5,	4,	7	},
	{	6,	12,	703,	-8,	2,	12,	8	},
	{	6,	12,	778,	-9,	2,	12,	8	},
	{	6,	12,	853,	-9,	4,	6,	8	}, /* 30 */
	{	6,	12,	928,	-10,	6,	4,	9	},
	{	10,	10,	1000,	-10,	4,	7,	9	},
	{	10,	10,	1100,	-11,	7,	4,	10	},
	{	10,	10,	1200,	-11,	5,	6,	10	},
	{	10,	10,	1300,	-11,	6,	5,	11	},
	{	10,	10,	1400,	-12,	4,	8,	11	},
	{	10,	10,	1500,	-12,	8,	4,	12	},
	{	10,	10,	1600,	-13,	16,	2,	12	},
	{	15,	10,	1700,	-13,	17,	2,	13	},
	{	15,	10,	1850,	-13,	6,	6,	13	}, /* 40 */
	{	25,	10,	2000,	-14,	12,	3,	14	},
	{	25,	10,	2250,	-14,	5,	8,	14	},
	{	25,	10,	2500,	-15,	10,	4,	15	},
	{	25,	10,	2750,	-15,	5,	9,	15	},
	{	25,	10,	3000,	-15,	9,	5,	16	},
	{	25,	10,	3250,	-16,	7,	7,	16	},
	{	25,	10,	3500,	-17,	13,	4,	17	},
	{	25,	10,	3750,	-18,	5,	11,	18	},
	{	50,	10,	4000,	-19,	11,	5,	18	},
	{	50,	10,	4500,	-20,	10,	6,	19	}, /* 50 */
	{	50,	10,	5000,	-21,	5,	13,	20	},
	{	50,	10,	5500,	-22,	15,	5,	20	},
	{	50,	10,	6000,	-23,	15,	6,	21	},
	{	50,	10,	6500,	-24,	15,	7,	22	},
	{	50,	10,	7000,	-25,	15,	8,	23	},
	{	50,	10,	7500,	-26,	15,	9,	24	},
	{	50,	10,	8000,	-27,	15,	10,	24	},
	{	50,	10,	8500,	-28,	20,	10,	25	},
	{	50,	10,	9000,	-29,	30,	10,	26	},
	{	50,	10,	9500,	-30,	50,	10,	28	}  /* 60 */
};

const	struct	flag_type	dam_classes	[]	=
{
	{	"dam_bash",	DAM_BASH,	TRUE	},
	{	"dam_pierce",	DAM_PIERCE,	TRUE	},
	{	"dam_slash",	DAM_SLASH,	TRUE	},
	{	"dam_fire",	DAM_FIRE,	TRUE	},
	{	"dam_cold",	DAM_COLD,	TRUE	},
	{	"dam_lightning",DAM_LIGHTNING,	TRUE	},
	{	"dam_acid",	DAM_ACID,	TRUE	},
	{	"dam_poison",	DAM_POISON,	TRUE	},
	{	"dam_negative",	DAM_NEGATIVE,	TRUE	},
	{	"dam_holy",	DAM_HOLY,	TRUE	},
	{	"dam_energy",	DAM_ENERGY,	TRUE	},
	{	"dam_wind",	DAM_WIND,	TRUE	},
	{	"dam_disease",	DAM_DISEASE,	TRUE	},
	{	"dam_drowning",	DAM_DROWNING,	TRUE	},
	{	"dam_light",	DAM_LIGHT,	TRUE	},
	{	"dam_other",	DAM_OTHER,	TRUE	},
	{	"dam_harm",	DAM_HARM,	TRUE	},
	{	"dam_charm",	DAM_CHARM,	TRUE	},
	{	"dam_sound",	DAM_SOUND,	TRUE	},
	{	"dam_suffocate",DAM_SUFFOCATE,	TRUE	},
	{	"dam_still", 	DAM_STILL,	TRUE	},
	{	NULL,		0,		TRUE	}
};

const	struct	flag_type	log_flags	[]	=
{
	{	"log_normal",	LOG_NORMAL,	TRUE	},
	{	"log_always",	LOG_ALWAYS,	TRUE	},
	{	"log_never",	LOG_NEVER,	TRUE	},
	{	"log_roleplay",	LOG_ROLEPLAY,	TRUE	},
	{	NULL,		0,		TRUE	}
};

const	struct	flag_type	show_flags	[]	=
{
	{	"undef",	TYP_UNDEF,	TRUE	},
	{	"comunicacion",	TYP_CMM,	TRUE	},
	{	"combate",	TYP_CBT,	TRUE	},
	{	"especiales",	TYP_ESP,	TRUE	},
	{	"grupo",	TYP_GRP,	TRUE	},
	{	"objetos",	TYP_OBJ,	TRUE	},
	{	"informacion",	TYP_INF,	TRUE	},
	{	"otros",	TYP_OTH,	TRUE	},
	{	"movimiento",	TYP_MVT,	TRUE	},
	{	"configuracion",TYP_CNF,	TRUE	},
	{	"lenguajes",	TYP_LNG,	TRUE	},
	{	"manejo",	TYP_PLR,	TRUE	},
	{	"olc",		TYP_OLC,	TRUE	},
	{	NULL,		0,		TRUE	}
};

const struct flag_type stat_table[] =
{
	{	"str",		STAT_STR,	TRUE	},
	{	"int",		STAT_INT,	TRUE	},
	{	"wis",		STAT_WIS,	TRUE	},
	{	"dex",		STAT_DEX,	TRUE	},
	{	"con",		STAT_CON,	TRUE	},
	{	NULL,		0,		TRUE	}
};

const struct flag_type ic_flags[] =
{
  { "role_playing",  IC_RP,            TRUE },
  { "dragon_reborn", IC_DRAGON_REBORN, TRUE },
  { "forsaken",      IC_FORSAKEN,      TRUE },
  { "taveren",       IC_TAVEREN,       TRUE },
  { "special",       IC_SPECIAL,       TRUE },
  {	NULL,		 0,		         TRUE }
  
};

const struct background_type merit_table[]=
{
  /* named          , bit                 , cost, class(-1 = all), race   , settable */
  { "acute_senses"  , MERIT_ACUTESENSES   ,  50 , -1             , NULL, NULL, TRUE },
  { "ambidextrous"  , MERIT_AMBIDEXTROUS  ,  50 , -1             , NULL, NULL, TRUE },
  { "calm_heart"    , MERIT_CALMHEART     ,  50 , -1             , NULL, NULL, TRUE },
  { "concentration" , MERIT_CONCENTRATION ,  50 , -1             , NULL, NULL, TRUE },
  { "efficient"     , MERIT_EFFICIENT     ,  50 , CLASS_CHANNELER, NULL, NULL, TRUE },
  { "fast_learner"  , MERIT_FASTLEARNER   ,  50 , -1             , NULL, NULL, TRUE },
  { "huge_size"     , MERIT_HUGESIZE      ,  50 , -1             , NULL, NULL, TRUE },
  { "longwinded"    , MERIT_LONGWINDED    ,  50 , -1             , NULL, NULL, TRUE },
  { "perfect_balance",MERIT_PERFECTBALANCE,  50 , -1             , NULL, NULL, TRUE },
  { "quick_reflexes", MERIT_QUICKREFLEXES ,  50 , -1             , NULL, NULL, TRUE },
  { "stealthy"      , MERIT_STEALTHY      ,  50 , -1             , NULL, "trolloc", TRUE },
  { "tough"         , MERIT_TOUGH         ,  50 , -1             , NULL, NULL, TRUE },
  { "warder"        , MERIT_WARDER        ,   0 , CLASS_WARRIOR  , NULL, "ogier trolloc fade", FALSE },
  { NULL            , 0                   ,   0 , -1             , NULL, NULL, TRUE }
};

const struct background_type flaw_table[]=
{
  /* named          , bit                 , cost, class(-1 = all), race   , settable */
  { "inefficient"   , FLAW_INEFFICIENT    ,  50 , CLASS_CHANNELER, NULL, NULL, TRUE },
  { "inept"         , FLAW_INEPT          ,  50 , -1             , NULL, NULL, TRUE },
  { "illiterate"    , FLAW_ILLITERATE     ,  50 , -1             , NULL, NULL, FALSE },
  { "one_arm"       , FLAW_ONEARM         ,  50 , -1             , NULL, NULL, FALSE },
  { "short"         , FLAW_SHORT          ,  50 , -1             , NULL, NULL, TRUE },
  { "slow_learner"  , FLAW_SLOWLEARNER    ,  50 , -1             , NULL, NULL, TRUE },
  { "slow_reflexes" , FLAW_SLOWREFLEXES   ,  50 , -1             , NULL, NULL, TRUE },
  { "weak"          , FLAW_WEAK           ,  50 , -1             , NULL, NULL, TRUE },
  { NULL            , 0                   ,   0 , -1             , NULL, NULL, TRUE }
};

const struct background_type talent_table[]=
{
  /* named          , bit                 , cost, class(-1 = all), race   , nrace, settable */
  { "align_matrix"  , TALENT_ALIGHMATRIX  , 50 , CLASS_CHANNELER, NULL   , NULL   , TRUE },
  { "cloud_dancing" , TALENT_CLOUDDANCING , 50 , CLASS_CHANNELER, NULL   , NULL   , TRUE },
  { "compulsion"    , TALENT_COMPULSION   , 50 , CLASS_CHANNELER, NULL   , NULL   , TRUE },
  { "create_angreal", TALENT_CREATE_ANGREAL, 50, CLASS_CHANNELER, NULL   , NULL   , FALSE},
  { "delving"       , TALENT_DELVING      ,  50 , CLASS_CHANNELER, NULL   , NULL   , FALSE },
  { "dreaming"      , TALENT_DREAMING     , 50 , -1             , NULL   , "ogier trolloc fade"   , TRUE },
  { "foretelling"   , TALENT_FORETELLING  , 50 , CLASS_CHANNELER, NULL   , NULL   , TRUE },
  { "healing"       , TALENT_HEALING      , 50 , CLASS_CHANNELER, NULL   , NULL   , TRUE },
  { "illusion"      , TALENT_ILLUSION     , 50 , CLASS_CHANNELER, NULL   , NULL   , TRUE },
  { "keeping"       , TALENT_KEEPING      , 50 , CLASS_CHANNELER, NULL   , NULL   , TRUE },
  { "lost_talent"   , TALENT_LOST         , 50 , CLASS_CHANNELER, NULL   , NULL   , FALSE },
  { "residues"      , TALENT_RESIDUES     , 50 , CLASS_CHANNELER, NULL   , NULL   , TRUE },
  { "sense_taveren" , TALENT_SENSETAV     , 50 , CLASS_CHANNELER, NULL   , NULL   , TRUE },
  { "sniffing"      , TALENT_SNIFFING     , 50 , -1             , NULL   , "ogier fade", TRUE },
  { "traveling"     , TALENT_TRAVELING    , 50 , CLASS_CHANNELER, NULL   , NULL   , FALSE },
  { "tree_singing"  , TALENT_TREE_SINGING , 50 , -1             , "ogier", NULL   , TRUE },
  { "unweaving"     , TALENT_UNWEAVING    , 50 , CLASS_CHANNELER, "aiel" , NULL   , FALSE },
  { "viewing"       , TALENT_VIEWING      , 50 , -1             , NULL   , "trolloc fade"   , TRUE },
  { "wolfkin"       , TALENT_WOLFKIN      , 50 , CLASS_WARRIOR  , NULL   , "aiel ogier seanchan trolloc fade malkieri", TRUE },
  { NULL            , 0                   ,   0 , CLASS_CHANNELER, NULL   , NULL   , TRUE }
};

const	struct	flag_type	hit_flags	[]	=
{
	{	"left arm",	LOC_LA,	TRUE	},
	{	"left leg",	LOC_LL,	TRUE	},
	{	"head",        LOC_HE,	TRUE	},
	{	"body",        LOC_BD,	TRUE	},
	{	"right leg",   LOC_RL,	TRUE	},
	{	"right arm",   LOC_RA,	TRUE	},
	{	NULL,		0,		TRUE	}
};

const	struct	flag_type	short_hit_flags	[]	=
{
	{	"larm",	LOC_LA,	TRUE	},
	{	"lleg",	LOC_LL,	TRUE	},
	{	"head",        LOC_HE,	TRUE	},
	{	"body",        LOC_BD,	TRUE	},
	{	"rleg",   LOC_RL,	TRUE	},
	{	"rarm",   LOC_RA,	TRUE	},
	{	NULL,		0,		TRUE	}
};
