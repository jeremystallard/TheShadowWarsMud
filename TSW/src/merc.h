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

/*
 * Accommodate old non-Ansi compilers.
 */
#include <syslog.h>
#include <time.h>
#include <stdio.h>

#define LOG_BUILDER LOG_LOCAL1
#define LOG_PLAYER  LOG_LOCAL2

#define CURRENTLOG  LOG_BUILDER

#if defined(TRADITIONAL)
#define const
#define args( list )			( )
#define DECLARE_DO_FUN( fun )		void fun( )
#define DECLARE_SPEC_FUN( fun )		bool fun( )
#define DECLARE_SPELL_FUN( fun )	void fun( )
#define DECLARE_LOOKUP_FUN( fun )	int fun ( )
#else
#define args( list )			list
#define DECLARE_DO_FUN( fun )		DO_FUN    fun
#define DECLARE_SPEC_FUN( fun )		SPEC_FUN  fun
#define DECLARE_SPELL_FUN( fun )	SPELL_FUN fun
#define DECLARE_LOOKUP_FUN( fun )	LOOKUP_F  fun
#endif

// OLC2
#define SPELL(spell)		DECLARE_SPELL_FUN(spell);
#define SPELL_FUN_DEC(spell)	FRetVal spell (int sn, int level, Entity *caster, Entity * ent, int target)
#define COMMAND(cmd)		DECLARE_DO_FUN(cmd);
#define DO_FUN_DEC(x)		void x (CHAR_DATA *ch, char *argument)
#define NEW_DO_FUN_DEC(x)	FRetVal x (Entity *ent, char *argument)
#define DECLARE_SPELL_CB(x)	FRetVal x (Entity *ent)

/* system calls */
int unlink();
int system();

/*
 * Short scalar types.
 * Diavolo reports AIX compiler has bugs with short types.
 */
#if	!defined(FALSE)
#define FALSE	 0
#endif

#if	!defined(TRUE)
#define TRUE	 1
#endif

#if	defined(_AIX)
#if	!defined(const)
#define const
#endif
typedef int				sh_int;
typedef int				bool;
#define unix
#else
typedef short   int			sh_int;
typedef unsigned char			bool;
#endif

/* ea */
#define MSL MAX_STRING_LENGTH
#define MIL MAX_INPUT_LENGTH
// gsn
#define GSN(x)	extern sh_int x;
#include "gsn.h"
#undef GSN

/*
 * Structure types.
 */
typedef struct	affect_data		AFFECT_DATA;
typedef struct	area_data		AREA_DATA;
typedef struct	ban_data		BAN_DATA;
typedef struct 	buf_type	 	BUFFER;
typedef struct	char_data		CHAR_DATA;
typedef struct	char_in_data		CHAR_IN_DATA;
typedef struct	descriptor_data		DESCRIPTOR_DATA;
typedef struct	exit_data		EXIT_DATA;
typedef struct	extra_descr_data	EXTRA_DESCR_DATA;
typedef struct	help_data		HELP_DATA;
typedef struct	help_area_data		HELP_AREA;
typedef struct	kill_data		KILL_DATA;
typedef struct	link_data		LINK_DATA;
typedef struct	mem_data		MEM_DATA;
typedef struct	mob_index_data		MOB_INDEX_DATA;
typedef struct	note_data		NOTE_DATA;
typedef struct	obj_data		OBJ_DATA;
typedef struct	obj_index_data		OBJ_INDEX_DATA;
typedef struct	pc_data			PC_DATA;
typedef struct  rp_data			RP_DATA;
typedef struct sleep_data       SLEEP_DATA;
typedef struct  gen_data		GEN_DATA;
typedef struct	reset_data		RESET_DATA;
typedef struct	room_index_data		ROOM_INDEX_DATA;
typedef struct	gatekey_data		GATEKEY_DATA;
typedef struct	shop_data		SHOP_DATA;
typedef struct	time_info_data		TIME_INFO_DATA;
typedef struct	weather_data		WEATHER_DATA;
typedef struct	colour_data		COLOUR_DATA;
typedef struct  mprog_list		MPROG_LIST;
typedef struct  mprog_code		MPROG_CODE;
typedef struct  disabled_data		DISABLED_DATA;
typedef struct  profile_data     	PROFILE_DATA;
typedef struct  history_data     	HISTORY_DATA;  /*For 10 lines of history on channels */
typedef struct  idName             ID_NAME;
typedef struct  clan_type          CLAN_DATA;
typedef struct  sguild_type        SGUILD_DATA;
typedef struct  ssguild_type       SSGUILD_DATA;
typedef struct  residue_type       RESIDUE_DATA;
typedef struct  ward_type          WARD_DATA;
typedef struct  block_type         BLOCK_DATA;
typedef struct  masquerade_type    MASQUERADE_TYPE;
typedef struct  pkinfo_type        PKINFO_TYPE;

struct masquerade_type
{
  int anum;
  int vnum;
  sh_int race;
  bool on;
  char *set_by;
};

struct pkinfo_type
{
	char * character;
	int  pk_count;
	int  pk_death_count;	
	PKINFO_TYPE * next;
};
/* block struct */
struct block_type
{
  int vnum;
  int direction;
  bool blocking;
};

/* ward struct */
struct ward_type
{
  WARD_DATA * next;    /* Pointer            */ 
  bool valid;          /* valid ye/ne?       */
  int sn;              /* What weave?        */
  int sn_learned;      /* Caster learned sn  */
  int sex;             /* What sex?          */
  long world;          /* what world?        */
  int level;           /* Level?             */
  int duration;        /* tick tick tick     */
  int bitvector;       /* What ward type     */
  int strength;        /* Strength           */ 
  CHAR_DATA *caster;   /* the caster         */
  long casterId;       /* the caster         */
  int  tied_strength;
  bool inverted;
};

/* residuce struct */
struct residue_type
{
  RESIDUE_DATA * next; /* Pointer            */ 
  bool valid;          /* valid ye/ne?       */
  int sn;              /* What weave?        */
  int sex;             /* What sex?          */
  long world;          /* what world?        */
  int duration;        /* tick tick tick     */
  long casterId;       /* the caster         */
};

/* Intro struct */
struct idName
{
  char *name;
  long id;
  struct idName *next;
};

/* one disabled command */
struct disabled_data
{
  DISABLED_DATA *next;            /* pointer to next node */
  struct cmd_type const *command; /* pointer to the command struct*/
  char *disabled_by;              /* name of disabler */
  sh_int level;                   /* level of disabler */
};

/*
 *Structure for channel history information
 */
struct history_data
{
  char *line_data;
  char *player_name;
  int  invis_flag;      /* 0 = not invis, 1 = invis */
  int  wizinvis_level;  /* Level of wizinvis        */
  time_t when;
  bool sent_comm; // Whether or not the character in question sent the communication.. used in act_comm.c::do_tellh
};

/*
 * THis is used for the profile command 
 */
struct profile_data
{
  char name[128];
  char sex[128];
  char race[128];
  char level[128];
  char elevel[128];
  char lastsite[128];
  char lastlog[128];
  char email[128];
  char insanity[128];
  char plan[128];
  char guild[128];
  char guildtitle[128];
  char guildlevel[128];
  char hoursplayed[128];
  char hoursroleplayed[128];
  char spheres[128];
};


extern			DISABLED_DATA	  *		disabled_first;
/* interp.c */


/*
 * Function types.
 */
typedef	void DO_FUN	args( ( CHAR_DATA *ch, char *argument ) );
typedef bool SPEC_FUN	args( ( CHAR_DATA *ch ) );
typedef void SPELL_FUN	args( ( int sn, int level, CHAR_DATA *ch, void *vo,
				int target ) );
typedef int	LOOKUP_F	args( ( const char * ) );


/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH		 1024
#define MAX_STRING_LENGTH	 4608
#define MAX_INPUT_LENGTH	       712
#define PAGELEN			   22

/* I am lazy :) */
#define MSL MAX_STRING_LENGTH
#define MIL MAX_INPUT_LENGTH

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SOCIALS		  256
extern int MAX_SKILL;
extern int MAX_GROUP;
#define MAX_IN_GROUP        45
#define MAX_ALIAS		   20 
#define MAX_IGNORE                 10
#define MAX_SHEAT_LOC        2
#define MAX_CLASS		    3
#define MAX_PC_RACE		    5
#define MAX_CLAN		    50
#define MAX_RANK             9
#define MAX_DAMAGE_MESSAGE   46
#define MAX_PC_ONLINE       150
#define MAX_LEVEL		   100
#define LEVEL_HERO		   (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL	   (MAX_LEVEL - 8)
#define LI					LEVEL_IMMORTAL
#define LEVEL_BUILDER      (MAX_LEVEL - 6)
#define LEVEL_ADMIN        (MAX_LEVEL - 4)
#define LEVEL_NEWBIE         30
#define MAX_VNUM              65535
#define MAX_DISGUISE         10
#define MAX_ARROWS_BEFORE_DEATH 10
#define TARGET_WEAK_HP	      4000
#define TARGET_NORMAL_HP      5000
#define TARGET_TOUGH_HP       6000

#define PULSE_PER_SECOND	    4
#define PULSE_VIOLENCE		  ( 3 * PULSE_PER_SECOND)
#define PULSE_MOBILE		  ( 4 * PULSE_PER_SECOND)
#define PULSE_MUSIC		  ( 6 * PULSE_PER_SECOND)
#define PULSE_TICK		  (60 * PULSE_PER_SECOND)
#define PULSE_AREA		  (120 * PULSE_PER_SECOND)
#define PULSE_HINT         (60 * PULSE_PER_SECOND)
#define PULSE_WEB          (120 *  PULSE_PER_SECOND)
#define PULSE_DEATHTRAP	   (15 * PULSE_PER_SECOND)

#define IMPLEMENTOR		MAX_LEVEL
#define CREATOR		(MAX_LEVEL - 1)
#define SUPREME		(MAX_LEVEL - 2)
#define DEITY			(MAX_LEVEL - 3)
#define GOD			(MAX_LEVEL - 4)
#define IMMORTAL		(MAX_LEVEL - 5)
#define DEMI			(MAX_LEVEL - 6)
#define ANGEL			(MAX_LEVEL - 7)
#define AVATAR			(MAX_LEVEL - 8)
#define HERO			LEVEL_HERO

/*
 * ColoUr stuff v2.0, by Lope.
 */
#define CLEAR		"\e[0;37m"		/* Resets Colour	*/
#define C_RED		"\e[0;31m"	/* Normal Colours	*/
#define C_GREEN		"\e[0;32m"
#define C_YELLOW	"\e[0;33m"
#define C_BLUE		"\e[0;34m"
#define C_MAGENTA	"\e[0;35m"
#define C_CYAN		"\e[0;36m"
#define C_WHITE		"\e[0;37m"
#define C_D_GREY	"\e[1;30m"  	/* Light Colors		*/
#define C_B_RED		"\e[1;31m"
#define C_B_GREEN	"\e[1;32m"
#define C_B_YELLOW	"\e[1;33m"
#define C_B_BLUE	"\e[1;34m"
#define C_B_MAGENTA	"\e[1;35m"
#define C_B_CYAN	"\e[1;36m"
#define C_B_WHITE	"\e[1;37m"
#define BEEP            ''

#define COLOUR_NONE	7		/* White, hmm...	*/
#define RED		1		/* Normal Colours	*/
#define GREEN		2
#define YELLOW		3
#define BLUE		4
#define MAGENTA		5
#define CYAN		6
#define WHITE		7
#define BLACK		0

#define NORMAL		0		/* Bright/Normal colours */
#define BRIGHT		1

#define ALTER_COLOUR( type )	if( !str_prefix( argument, "red" ) )	\
				{					\
				    ch->pcdata->type[0] = NORMAL;	\
				    ch->pcdata->type[1] = RED;		\
				}					\
				else if( !str_prefix( argument, "hi-red" ) )\
				{					\
				    ch->pcdata->type[0] = BRIGHT;	\
				    ch->pcdata->type[1] = RED;		\
				}					\
				else if( !str_prefix( argument, "green" ) )\
				{					\
				    ch->pcdata->type[0] = NORMAL;	\
				    ch->pcdata->type[1] = GREEN;	\
				}					\
				else if( !str_prefix( argument, "hi-green" ) )\
				{					\
				    ch->pcdata->type[0] = BRIGHT;	\
				    ch->pcdata->type[1] = GREEN;	\
				}					\
				else if( !str_prefix( argument, "yellow" ) )\
				{					\
				    ch->pcdata->type[0] = NORMAL;	\
				    ch->pcdata->type[1] = YELLOW;	\
				}					\
				else if( !str_prefix( argument, "hi-yellow" ) )\
				{					\
				    ch->pcdata->type[0] = BRIGHT;	\
				    ch->pcdata->type[1] = YELLOW;	\
				}					\
				else if( !str_prefix( argument, "blue" ))\
				{					\
				    ch->pcdata->type[0] = NORMAL;	\
				    ch->pcdata->type[1] = BLUE;		\
				}					\
				else if( !str_prefix( argument, "hi-blue" ) )\
				{					\
				    ch->pcdata->type[0] = BRIGHT;	\
				    ch->pcdata->type[1] = BLUE;		\
				}					\
				else if( !str_prefix( argument, "magenta" ) )\
				{					\
				    ch->pcdata->type[0] = NORMAL;	\
				    ch->pcdata->type[1] = MAGENTA;	\
				}					\
				else if( !str_prefix( argument, "hi-magenta" ))\
				{					\
				    ch->pcdata->type[0] = BRIGHT;	\
				    ch->pcdata->type[1] = MAGENTA;	\
				}					\
				else if( !str_prefix( argument, "cyan" ) )\
				{					\
				    ch->pcdata->type[0] = NORMAL;	\
				    ch->pcdata->type[1] = CYAN;		\
				}					\
				else if( !str_prefix( argument, "hi-cyan" ) )\
				{					\
				    ch->pcdata->type[0] = BRIGHT;	\
				    ch->pcdata->type[1] = CYAN;		\
				}					\
				else if( !str_prefix( argument, "white" ) )\
				{					\
				    ch->pcdata->type[0] = NORMAL;	\
				    ch->pcdata->type[1] = WHITE;	\
				}					\
				else if( !str_prefix( argument, "hi-white" ) )\
				{					\
				    ch->pcdata->type[0] = BRIGHT;	\
				    ch->pcdata->type[1] = WHITE;	\
				}					\
				else if( !str_prefix( argument, "grey" ) )\
				{					\
				    ch->pcdata->type[0] = BRIGHT;	\
				    ch->pcdata->type[1] = BLACK;	\
				}					\
				else if( !str_prefix( argument, "beep" ))\
				{					\
				    ch->pcdata->type[2] = 1;		\
				}					\
				else if( !str_prefix( argument, "nobeep") )\
				{					\
				    ch->pcdata->type[2] = 0;		\
				}					\
				else					\
				{					\
		send_to_char_bw( "Unrecognised colour, unchanged.\n\r", ch );\
				    return;				\
				}

#define LOAD_COLOUR( field )	ch->pcdata->field[1] = fread_number( fp );\
				if( ch->pcdata->field[1] > 100 )	\
				{					\
				    ch->pcdata->field[1] -= 100;	\
				    ch->pcdata->field[2] = 1;		\
				}					\
				else					\
				{					\
				    ch->pcdata->field[2] = 0;		\
				}					\
				if( ch->pcdata->field[1] > 10 )		\
				{					\
				    ch->pcdata->field[1] -= 10;		\
				    ch->pcdata->field[0] = 1;		\
				}					\
				else					\
				{					\
				    ch->pcdata->field[0] = 0;		\
				}


/** Guild flags
 */
#define GUILD_DELETED		A
#define GUILD_CHANGED		B
#define GUILD_INDEPENDENT 	C /* a "loner" guild */
#define GUILD_WOLF            D
#define GUILD_IMMORTAL		E /* immortal only clan */

/** Sguild flags
 */
#define SGUILD_DELETED		A
#define SGUILD_CHANGED		B
#define SGUILD_INDEPENDENT 	C /* a "loner" guild */
#define SGUILD_WOLF           D
#define SGUILD_IMMORTAL		E /* immortal only clan */

/** SSguild flags
 */
#define SSGUILD_DELETED		A
#define SSGUILD_CHANGED		B
#define SSGUILD_INDEPENDENT 	C /* a "loner" guild */
#define SSGUILD_WOLF          D
#define SSGUILD_IMMORTAL		E /* immortal only clan */

/*
 * Site ban structure.
 */

#define BAN_SUFFIX		A
#define BAN_PREFIX		B
#define BAN_NEWBIES		C
#define BAN_ALL			D	
#define BAN_PERMIT		E
#define BAN_PERMANENT		F

struct	ban_data
{
    BAN_DATA *	next;
    bool	valid;
    sh_int	ban_flags;
    sh_int	level;
    char *	name;
};

extern SLEEP_DATA *first_sleep;

/*
 *  Sleeping prog data
 */
struct sleep_data
{
  SLEEP_DATA *next;
  SLEEP_DATA *prev;
  CHAR_DATA *ch;
  CHAR_DATA *mob;
  MPROG_CODE *prog;
  int valid;
  unsigned int vnum;
  int line;
  int timer;
};

struct char_in_data
{
   CHAR_DATA *ch;
   CHAR_IN_DATA *next;
};

struct buf_type
{
    BUFFER *    next;
    bool        valid;
    sh_int      state;  /* error state of the buffer */
    sh_int      size;   /* size in k */
    char *      string; /* buffer's string */
};

struct skhash
{
	struct skhash *	next;
	int		sn;
};

/*
 * Time and weather stuff.
 */
#define SUN_DARK		    0
#define SUN_RISE		    1
#define SUN_LIGHT		    2
#define SUN_SET			    3

#define SKY_CLOUDLESS		    0
#define SKY_CLOUDY		    1
#define SKY_RAINING		    2
#define SKY_LIGHTNING		    3

struct	time_info_data
{
    int		hour;
    int		day;
    int		month;
    int		year;
};

struct	weather_data
{
    int		mmhg;
    int		change;
    int		sky;
    int		sunlight;
};

struct quest_desc_type
{
    char * name;
    char * short_descr;
    char * long_descr;
};


/*
 * Connected state for a channel.
 */
#define CON_PLAYING			      0
#define CON_GET_NAME			 1
#define CON_GET_OLD_PASSWORD		 2
#define CON_CONFIRM_NEW_NAME		 3
#define CON_GET_FACELESS                 4  //Prompt for whether or not the character will be faceless
#define CON_GET_NEW_PASSWORD		 5
#define CON_CONFIRM_NEW_PASSWORD	 6
#define CON_GET_NEW_RACE	         7
#define CON_GET_NEW_SEX			 8
#define CON_GET_NEW_CLASS		 9
#define CON_GET_ALIGNMENT		10	
#define CON_GET_WAS_REFERRED		11
#define CON_GET_REFERRER		12
#define CON_DEFAULT_CHOICE		13 
#define CON_GEN_GROUPS			14 
#define CON_PICK_WEAPON			15
#define CON_READ_IMOTD			16
#define CON_READ_MOTD			17
#define CON_BREAK_CONNECT		18
#define CON_NOTE_TO                     19
#define CON_NOTE_SUBJECT                20
#define CON_NOTE_EXPIRE                 21
#define CON_NOTE_TEXT                   22
#define CON_NOTE_FINISH                 23
#define CON_COPYOVER_RECOVER            24
#define CON_ASK_COLOUR                  25
#define CON_GET_EMAIL                   26
#define CON_GET_APPEARANCE              27
#define CON_SELECTION_MENU              28
#define CON_GET_SPHERES                 29
#define CON_GET_STATS                   30
#define CON_GET_MERITS                  31
#define CON_GET_FLAWS                   32
#define CON_GET_TALENTS                 33


/*
 * Descriptor (channel) structure.
 */
struct	descriptor_data
{
    DESCRIPTOR_DATA *	next;
    DESCRIPTOR_DATA *	snoop_by;
    CHAR_DATA *		character;
    CHAR_DATA *		original;
    bool		valid;
    char *		ipaddr;
    char *		host;
    sh_int		descriptor;
    sh_int		connected;
    bool		fcommand;
    char		inbuf		[4 * MAX_INPUT_LENGTH];
    char		incomm		[MAX_INPUT_LENGTH];
    char		inlast		[MAX_INPUT_LENGTH];
    int			repeat;
    char *		outbuf;
    int			outsize;
    int			outtop;
    char *		showstr_head;
    char *		showstr_point;
    void *              pEdit;		/* OLC */
    char **             pString;	/* OLC */
    sh_int		editor;		/* OLC */
    sh_int		page;
    char *		screenmap;
    char *		oldscreenmap;
};



/*
 * Attribute bonus structures.
 */
struct	str_app_type
{
    sh_int	tohit;
    sh_int	todam;
    sh_int	carry;
    sh_int	wield;
};

struct	int_app_type
{
    sh_int	learn;
};

struct	wis_app_type
{
    sh_int	practice;
};

struct	dex_app_type
{
    sh_int	defensive;
};

struct	con_app_type
{
    sh_int	hitp;
    sh_int	shock;
};



/*
 * TO types for act.
 */
#define TO_ROOM		    0
#define TO_NOTVICT	    1
#define TO_VICT		    2
#define TO_CHAR		    3
#define TO_ALL		    4
#define TO_CONT             5
#define TO_OUTSIDE_CONT     6


/*
 * Help table types.
 */
struct	help_data
{
    HELP_DATA *	next;
    HELP_DATA * next_area;
    sh_int	level;
    char *	keyword;
    char *	text;
};

struct help_area_data
{
	HELP_AREA *	next;
	HELP_DATA *	first;
	HELP_DATA *	last;
	AREA_DATA *	area;
	char *		filename;
	bool		changed;
};


/*
 * Shop types.
 */
#define MAX_TRADE	 5

struct	shop_data
{
    SHOP_DATA *	next;			/* Next shop in list		*/
    unsigned int	keeper;			/* Vnum of shop keeper mob	*/
    sh_int	buy_type [MAX_TRADE];	/* Item types shop will buy	*/
    sh_int	profit_buy;		/* Cost multiplier for buying	*/
    sh_int	profit_sell;		/* Cost multiplier for selling	*/
    sh_int	open_hour;		/* First opening hour		*/
    sh_int	close_hour;		/* First closing hour		*/
};

struct gatekey_data 
{
    char *key_alias;
    char *key_value;
    GATEKEY_DATA * next;
};

/*
 * Per-class stuff.
 */
#define MIN_PC_HP   300
#define MAX_PC_HP   2500
#define MIN_PC_END  300
#define MAX_PC_END  3500
#define MAX_PC_TRAIN 100

#define MAX_GUILD 	2
#define MAX_STATS 	5
#define MAX_HIT_LOC 6
#define MAX_TOTAL_HP    15000
#define MAX_TOTAL_ENDURANCE    15000
#define MAX_STAT_VALUE_PC 25
#define STAT_STR 	0
#define STAT_INT	1
#define STAT_WIS	2
#define STAT_DEX	3
#define STAT_CON	4

#define LOC_NA    -1
#define LOC_LA     0
#define LOC_LL     1
#define LOC_HE     2
#define LOC_BD     3
#define LOC_RL     4
#define LOC_RA     5

/* Location div base for Hit values for each location */
/* Example: Head is ch->max_hit/6. */
#define LOC_MOD_LA 3
#define LOC_MOD_LL 4
#define LOC_MOD_HE 6
#define LOC_MOD_BD 2
#define LOC_MOD_RL 3
#define LOC_MOD_RA 4

#define LOC_MIN_HIT 10

#define MAX_FLOWS     10
#define SUSTAIN_WEAVE -2

#define MAX_SPHERE    5
#define SPHERE_AIR    0
#define SPHERE_EARTH  1
#define SPHERE_FIRE   2
#define SPHERE_SPIRIT 3
#define SPHERE_WATER  4

#define MAX_SPHERE_VALUE_MOB 500
#define MAX_SPHERE_VALUE_PC  250
#define MIN_SPHERE_VALUE_PC   10

/* These are used in creation */
#define MAX_SPHERE_BONUS_PC  10   /* Max to be set as bonus in creation       */
#define MAX_SPHERE_SUM       500  /* In Creation the max you can get it 500   */
#define MIN_SPHERE_SUM		 300  /* Creation cannot go less than 300		  */

struct   sphere_type
{
  char *name; /* Sphere name */
};

#define CLASS_CHANNELER  0
#define CLASS_THIEF      1
#define CLASS_WARRIOR    2

struct	class_type
{
  char *	     name;			/* the full name of the class      */
  char 	     who_name	[4];	     /* Three-letter name for 'who'     */
  sh_int	     attr_prime;		/* Prime attribute		          */
  sh_int	     weapon;			/* First weapon			     */
  unsigned int	guild[MAX_GUILD];	/* Vnum of guild rooms		     */
  sh_int	     skill_adept;		/* Maximum skill level		     */
  sh_int	     thac0_00;		     /* Thac0 for level  0		     */
  sh_int	     thac0_32;		     /* Thac0 for level 32		     */
  sh_int	     hp_min;			/* Min hp gained on leveling	     */
  sh_int	     hp_max;			/* Max hp gained on leveling	     */
  bool	     fEndurance;    	/* Class gains Endurance on level	*/
  char *	     base_group;		/* base skills gained		     */
  char *	     default_group;		/* default skills gained	          */
};

struct item_type
{
    int		type;
    char *	name;
};

struct weapon_type
{
  char *	name;
  unsigned int	vnum;
  sh_int	type;
  sh_int	*gsn;
};

struct wiznet_type
{
    char *	name;
    long 	flag;
    int		level;
};

struct attack_type
{
    char *	name;			/* name */
    char *	noun;			/* message */
    int   	damage;			/* damage class */
};

struct race_type
{
    char *	name;			/* call name of the race */
    bool	pc_race;		/* can be chosen by pcs */
    bool        granted;                /* granted race          */
    long	act;			/* act bits for the race */
    long	aff;			/* aff bits for the race */
    long	off;			/* off bits for the race */
    long	imm;			/* imm bits for the race */
    long        res;			/* res bits for the race */
    long	vuln;			/* vuln bits for the race */
    long	form;			/* default form flag for the race */
    long	parts;			/* default parts for the race */
    sh_int	race_id;
#if !defined(FIRST_BOOT)
    char *	who_name;
    sh_int     sphere[MAX_SPHERE];
    sh_int     sphere_points;      /* Creation sphere points */
    sh_int     stat_points;        /* Creation stat points */
    sh_int     misc_points;        /* Creation merit/flaw/talent points */
    sh_int	points;			/* cost in points of the race */
    sh_int	class_mult[MAX_CLASS];	/* exp multiplier for class, * 100 */
    char *	skills[5];		/* bonus skills for the race */
    sh_int 	stats[MAX_STATS];	/* starting stats */
    sh_int	max_stats[MAX_STATS];	/* maximum stats */
    sh_int	size;			/* aff bits for the race */
#endif
};

#if defined(FIRST_BOOT)
struct pc_race_type  /* additional data for pc races */
{
    char *	name;			/* MUST be in race_type */
    char *	who_name;
    sh_int	points;			/* cost in points of the race */
    sh_int	class_mult[MAX_CLASS];	/* exp multiplier for class, * 100 */
    char *	skills[5];		/* bonus skills for the race */
    sh_int 	stats[MAX_STATS];	/* starting stats */
    sh_int	max_stats[MAX_STATS];	/* maximum stats */
    sh_int	size;			/* aff bits for the race */
};
#endif

struct spec_type
{
    char * 	name;			/* special function name */
    SPEC_FUN *	function;		/* the function */
};



/*
 * Data structure for notes.
 */

struct	note_data
{
  NOTE_DATA *	next;
  bool 	valid;
  sh_int	type;
  char *	sender;
  char *  show_sender;
  char *	date;
  char *	to_list;
  char *  show_to_list;
  char *	subject;
  char *	text;
  time_t  	date_stamp;
  time_t	expire;
};




/*
 * An affect.
 */
struct	affect_data
{
  AFFECT_DATA *next;
  CHAR_DATA   *caster;
  long         casterId; /* Unique ID for caster (ch->id) */
  bool         valid;
  sh_int       where;
  sh_int         type;
  sh_int         type_learned;
  int          sex;
  long         world;
  sh_int       level;
  sh_int       duration;
  sh_int       location;
  sh_int       modifier;
  int          bitvector;
  int          tied_strength;
  bool         inverted;
};

/* where definitions */
#define TO_AFFECTS  0
#define TO_OBJECT   1
#define TO_IMMUNE   2
#define TO_RESIST   3
#define TO_VULN     4
#define TO_WEAPON   5
#define TO_AFF_ROOM 6
#define TO_APP      7


/*
 * A kill structure (indexed by level).
 */
struct	kill_data
{
    sh_int		number;
    sh_int		killed;
};



/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO		   3090
#define MOB_VNUM_CITYGUARD       3060
#define MOB_VNUM_VAMPIRE	        3404
#define MOB_VNUM_LEGION            7
#define MOB_VNUM_TROLLOC           8
#define MOB_VNUM_WOLF              10
#define MOB_VNUM_MRSMITH           15
#define MOB_VNUM_CHOSEN_LANFEAR    12312
#define MOB_VNUM_CHOSEN_ISHAMAEL   26400
#define MOB_VNUM_CHOSEN_MOGHEDIAN  40009
#define MOB_VNUM_LANFEARS_TROLLOC1 12313
#define MOB_VNUM_LANFEARS_TROLLOC2 12454
#define MOB_VNUM_LANFEARS_TROLLOC3 12455
#define MOB_VNUM_LANFEARS_TROLLOC4 12456
#define MOB_VNUM_LANFEARS_FADE1    12314
#define MOB_VNUM_LANFEARS_FADE2    12466


#define MOB_VNUM_PATROLMAN	   2106
#define GROUP_VNUM_TROLLS	   2100
#define GROUP_VNUM_OGRES	        2101


/* RT ASCII conversions -- used so we can have letters in this file */
#define A		  	1 
#define B			2
#define C			4
#define D			8
#define E			16
#define F			32
#define G			64
#define H			128
#define I			256
#define J			512
#define K		     1024
#define L		 	2048
#define M			4096
#define N		 	8192
#define O			16384
#define P			32768
#define Q			65536
#define R			131072
#define S			262144
#define T			524288
#define U			1048576
#define V			2097152
#define W			4194304
#define X			8388608
#define Y			16777216
#define Z			33554432
#define aa          67108864 	/* doubled due to conflicts */
#define bb          134217728
#define cc          268435456    
#define dd          536870912
#define ee          1073741824
#define ff          1073741826
#define gg	    2147483648

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC       (A)		/* Auto set for mobs	*/
#define ACT_SENTINEL     (B)		/* Stays in one room	*/
#define ACT_SCAVENGER    (C)		/* Picks up objects	     */
#define ACT_BANKER       (D)       /* Banker                */
#define ACT_REPAIRER     (E)       /* Repairer              */
#define ACT_AGGRESSIVE	(F)    	/* Attacks PC's		*/
#define ACT_STAY_AREA	(G)		/* Won't leave area	     */
#define ACT_WIMPY		(H)
#define ACT_PET		(I)		/* Auto set for pets	*/
#define ACT_TRAIN		(J)		/* Can train PC's	     */
#define ACT_IS_STRINGER  (K)
#define ACT_UNDEAD		(O)	
#define ACT_CLERIC		(Q)
//#define ACT_MAGE		(R)
#define ACT_ROAMERTRAINER 	(R)
#define ACT_THIEF		(S)
#define ACT_WARRIOR		(T)
#define ACT_NOALIGN		(U)
#define ACT_NOPURGE		(V)
#define ACT_OUTDOORS	(W)
#define ACT_INDOORS		(Y)
#define ACT_RIDEABLE	(Z)
#define ACT_IS_HEALER	(aa)
#define ACT_GAIN		(bb)
#define ACT_UPDATE_ALWAYS (cc)
#define ACT_IS_CHANGER	(dd)
#define ACT_BLACKWIND	(ee)

/*
 * Guild Guard bits for mobs
 *
 */
#define GGUARD_AGGRESSIVE   (A)
#define GGUARD_REPORT_GUILD (B)
#define GGUARD_BLOCK        (C)
#define GGUARD_SHADOWSPAWN  (D)
#define GGUARD_RACE         (E)

/*
 * GAIN bits for mobs.
 * Used in #MOBILES.
 */
#define GAIN_SKILL       (A)
#define GAIN_WEAVE       (B)
#define GAIN_BLADEMASTER (C)
#define GAIN_SPEARDANCER (D)
#define GAIN_DOUBLE_XP   (E)
#define GAIN_DUELLING    (F)
#define GAIN_MASTERFORMS (G)
#define	GAIN_NEWBWEAPONS	(H)

/*
 * TRAIN bits for mobs.
 * Used in #MOBILES.
 */
#define TRAIN_SKILL       (A)
#define TRAIN_WEAVE       (B)
#define TRAIN_BLADEMASTER (C)
#define TRAIN_SPEARDANCER (D)
#define TRAIN_DUELLING    (E)
#define TRAIN_MASTERFORMS (F)
#define TRAIN_WEAPONS		(G)

/* damage classes */
#define DAM_NONE                0
#define DAM_BASH                1
#define DAM_PIERCE              2
#define DAM_SLASH               3
#define DAM_FIRE                4
#define DAM_COLD                5
#define DAM_LIGHTNING           6
#define DAM_ACID                7
#define DAM_POISON              8
#define DAM_NEGATIVE            9
#define DAM_HOLY                10
#define DAM_ENERGY              11
#define DAM_WIND                12
#define DAM_DISEASE             13
#define DAM_DROWNING            14
#define DAM_LIGHT		15
#define DAM_OTHER               16
#define DAM_HARM		17
#define DAM_CHARM		18
#define DAM_SOUND		19
#define DAM_MF			20
#define DAM_TW			21
#define DAM_SAP		22
#define DAM_CRIT         23
#define DAM_SD           24
#define DAM_SUFFOCATE    25
#define DAM_DU           26 // Duelling
#define DAM_STILL        27 // Duelling

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK         (A)
#define OFF_BACKSTAB            (B)
#define OFF_BASH                (C)
#define OFF_BERSERK             (D)
#define OFF_DISARM              (E)
#define OFF_DODGE               (F)
#define OFF_FADE                (G)
#define OFF_FAST                (H)
#define OFF_KICK                (I)
#define OFF_KICK_DIRT           (J)
#define OFF_PARRY               (K)
#define OFF_RESCUE              (L)
#define OFF_TAIL                (M)
#define OFF_TRIP                (N)
#define OFF_CRUSH		(O)
#define ASSIST_ALL       	(P)
#define ASSIST_ALIGN	        (Q)
#define ASSIST_RACE    	     	(R)
#define ASSIST_PLAYERS      	(S)
#define ASSIST_GUARD        	(T)
#define ASSIST_VNUM		(U)
#define HOWLING			(V)

/* Target bits for mobs and players */
#define TARGET_HEAD			(W)
#define TARGET_NECK			(X)
#define TARGET_TORSO			(Y)
#define TARGET_BACK			(Z)
#define TARGET_ARMS			(aa)
#define TARGET_LEGS			(bb)
#define TARGET_FEET			(cc)
#define TARGET_HANDS			(dd)
#define TARGET_GENERAL			(ee)

/* return values for check_imm */
#define IS_NORMAL		0
#define IS_IMMUNE		1
#define IS_RESISTANT		2
#define IS_VULNERABLE		3

/* IMM bits for mobs */
#define IMM_SUMMON              (A)
#define IMM_CHARM               (B)
#define IMM_MAGIC               (C)
#define IMM_WEAPON              (D)
#define IMM_BASH                (E)
#define IMM_PIERCE              (F)
#define IMM_SLASH               (G)
#define IMM_FIRE                (H)
#define IMM_COLD                (I)
#define IMM_LIGHTNING           (J)
#define IMM_ACID                (K)
#define IMM_POISON              (L)
#define IMM_NEGATIVE            (M)
#define IMM_HOLY                (N)
#define IMM_ENERGY              (O)
#define IMM_WIND                (P)
#define IMM_DISEASE             (Q)
#define IMM_DROWNING            (R)
#define IMM_LIGHT		(S)
#define IMM_SOUND		(T)
#define IMM_WOOD                (X)
#define IMM_SILVER              (Y)
#define IMM_IRON                (Z)
#define IMM_SAP                 (aa)
#define IMM_PUSH                (bb)
#define IMM_ASSASSINATE         (cc)
#define IMM_ARROW	        (dd)
#define IMM_HARM	        (ee)
 
/* RES bits for mobs */
#define RES_SUMMON		(A)
#define RES_CHARM		(B)
#define RES_MAGIC               (C)
#define RES_WEAPON              (D)
#define RES_BASH                (E)
#define RES_PIERCE              (F)
#define RES_SLASH               (G)
#define RES_FIRE                (H)
#define RES_COLD                (I)
#define RES_LIGHTNING           (J)
#define RES_ACID                (K)
#define RES_POISON              (L)
#define RES_NEGATIVE            (M)
#define RES_HOLY                (N)
#define RES_ENERGY              (O)
#define RES_WIND                (P)
#define RES_DISEASE             (Q)
#define RES_DROWNING            (R)
#define RES_LIGHT		(S)
#define RES_SOUND		(T)
#define RES_WOOD                (X)
#define RES_SILVER              (Y)
#define RES_IRON                (Z)
#define RES_MINING              (aa)
 
/* VULN bits for mobs */
#define VULN_SUMMON		(A)
#define VULN_CHARM		(B)
#define VULN_MAGIC              (C)
#define VULN_WEAPON             (D)
#define VULN_BASH               (E)
#define VULN_PIERCE             (F)
#define VULN_SLASH              (G)
#define VULN_FIRE               (H)
#define VULN_COLD               (I)
#define VULN_LIGHTNING          (J)
#define VULN_ACID               (K)
#define VULN_POISON             (L)
#define VULN_NEGATIVE           (M)
#define VULN_HOLY               (N)
#define VULN_ENERGY             (O)
#define VULN_WIND               (P)
#define VULN_DISEASE            (Q)
#define VULN_DROWNING           (R)
#define VULN_LIGHT		(S)
#define VULN_SOUND		(T)
#define VULN_WOOD               (X)
#define VULN_SILVER             (Y)
#define VULN_IRON		(Z)
 
/* body form */
#define FORM_EDIBLE             (A)
#define FORM_POISON             (B)
#define FORM_MAGICAL            (C)
#define FORM_INSTANT_DECAY      (D)
#define FORM_OTHER              (E)  /* defined by material bit */
 
/* actual form */
#define FORM_ANIMAL             (G)
#define FORM_SENTIENT           (H)
#define FORM_UNDEAD             (I)
#define FORM_CONSTRUCT          (J)
#define FORM_MIST               (K)
#define FORM_INTANGIBLE         (L)
 
#define FORM_BIPED              (M)
#define FORM_CENTAUR            (N)
#define FORM_INSECT             (O)
#define FORM_SPIDER             (P)
#define FORM_CRUSTACEAN         (Q)
#define FORM_WORM               (R)
#define FORM_BLOB		(S)
 
#define FORM_MAMMAL             (V)
#define FORM_BIRD               (W)
#define FORM_REPTILE            (X)
#define FORM_SNAKE              (Y)
#define FORM_DRAGON             (Z)
#define FORM_AMPHIBIAN          (aa)
#define FORM_FISH               (bb)
#define FORM_COLD_BLOOD		(cc)	
 
/* body parts */
#define PART_HEAD               (A)
#define PART_ARMS               (B)
#define PART_LEGS               (C)
#define PART_HEART              (D)
#define PART_BRAINS             (E)
#define PART_GUTS               (F)
#define PART_HANDS              (G)
#define PART_FEET               (H)
#define PART_FINGERS            (I)
#define PART_EAR                (J)
#define PART_EYE		(K)
#define PART_LONG_TONGUE        (L)
#define PART_EYESTALKS          (M)
#define PART_TENTACLES          (N)
#define PART_FINS               (O)
#define PART_WINGS              (P)
#define PART_TAIL               (Q)
/* for combat */
#define PART_CLAWS              (U)
#define PART_FANGS              (V)
#define PART_HORNS              (W)
#define PART_SCALES             (X)
#define PART_TUSKS		(Y)


/*
 * Bits for 'world'.
 *
 */
#define WORLD_NORMAL       (A)
#define WORLD_TAR_FLESH    (B)
#define WORLD_TAR_DREAM    (C)
#define WORLD_SKIMMING     (D)

/*
 * Bits for 'talents'.
 *
 */
#define TALENT_ALIGHMATRIX    (A) /* channeler only */
#define TALENT_CLOUDDANCING   (B) /* sea folk only  */
#define TALENT_COMPULSION     (C) /* channeler only */
#define TALENT_DELVING        (D) /* channeler only */
#define TALENT_DREAMING       (E) /* all            */
#define TALENT_FORETELLING    (F) /* all            */
#define TALENT_HEALING        (G) /* channeler only */
#define TALENT_ILLUSION       (H) /* channeler only */
#define TALENT_KEEPING        (I) /* channeler only */
#define TALENT_RESIDUES       (J) /* channeler only */
#define TALENT_SENSETAV       (K) /* all            */
#define TALENT_SNIFFING       (L) /* all            */
#define TALENT_TRAVELING      (M) /* channeler only */
#define TALENT_TREE_SINGING   (N) /* Ogier only     */
#define TALENT_UNWEAVING      (O) /* channeler only */
#define TALENT_VIEWING        (P) /* all            */
#define TALENT_WOLFKIN        (Q) /* all            */
#define TALENT_LOST           (R) /* channeler only */
#define TALENT_CREATE_ANGREAL (S) /* Lost talent 1  */

/*
 * Bits for 'merits'.
 *
 */
#define MERIT_ACUTESENSES    (A)
#define MERIT_AMBIDEXTROUS   (B)
#define MERIT_CALMHEART      (C)
#define MERIT_CONCENTRATION  (D)
#define MERIT_EFFICIENT      (E)
#define MERIT_FASTLEARNER    (F)
#define MERIT_HUGESIZE       (G)
#define MERIT_LONGWINDED     (H)
#define MERIT_PERFECTBALANCE (I)
#define MERIT_QUICKREFLEXES  (J)
#define MERIT_STEALTHY       (K)
#define MERIT_TOUGH          (L)
#define MERIT_WARDER         (M)

/*
#define MERIT_SPHEREBONUSAIR    (M)
#define MERIT_SPHEREBONUSEARTH  (N)
#define MERIT_SPHEREBONUSFIRE   (O)
#define MERIT_SPHEREBONUSSPIRIT (P)
#define MERIT_SPHEREBONUSWATER  (Q)
*/

/*
 * Bits for 'flaws'.
 *
 */
#define FLAW_INEFFICIENT     (A)
#define FLAW_INEPT           (B)
#define FLAW_ILLITERATE      (C)
#define FLAW_ONEARM          (D)
#define FLAW_SHORT           (E)
#define FLAW_SLOWLEARNER     (F)
#define FLAW_SLOWREFLEXES    (G)
#define FLAW_WEAK            (H)

/*
 * Bits for 'wards' set for a room
 *
 */
#define WARD_EAVESDROP        (A)
#define WARD_SHADOWSPAWN      (B)
#define WARD_LIGHT	      (C)

/*
 * Bits for 'weaves'
 * Used on rooms
 */
#define RAFF_EAVESDROP        (A)

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND		     (A)
#define AFF_INVISIBLE		(B)
#define AFF_DREAMWARD         (C)
#define AFF_DETECT_INVIS	     (D)
#define AFF_GAGGED	     (E)
#define AFF_DETECT_HIDDEN	(F)
#define AFF_CAMOUFLAGE		(G)
#define AFF_SANCTUARY		(H)
#define AFF_WRAPPED    		        (I)
#define AFF_INFRARED			(J)
#define AFF_LINKED			(K)
#define AFF_SHIELDED          (L) /* WoT channeling */
#define AFF_POISON			(M)
#define AFF_BLINDFOLDED	     (N)
#define AFF_BIND              (O)
#define AFF_SNEAK			(P)
#define AFF_HIDE			(Q)
#define AFF_SLEEP			(R)
#define AFF_CHARM			(S) /* Compulsion */
#define AFF_FLYING			(T)
#define AFF_PASS_DOOR		(U)
#define AFF_HASTE			(V)
#define AFF_CALM			(W)
#define AFF_PLAGUE			(X)
#define AFF_WEAKEN			(Y)
#define AFF_SUFFOCATING  		(Z)
#define AFF_BERSERK			(aa)
#define AFF_CHANNELING        (bb)
#define AFF_REGENERATION      (cc)
#define AFF_SLOW              (dd)
#define AFF_SAP               (ee)
#define AFF_MINING	      (gg)




/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL		      0
#define SEX_MALE		      1
#define SEX_FEMALE		      2

/* AC types */
#define AC_PIERCE             0
#define AC_BASH               1
#define AC_SLASH              2
#define AC_EXOTIC             3

/* dice */
#define DICE_NUMBER			0
#define DICE_TYPE			1
#define DICE_BONUS			2

/* size */
#define SIZE_TINY			0
#define SIZE_SMALL			1
#define SIZE_MEDIUM			2
#define SIZE_LARGE			3
#define SIZE_HUGE			4
#define SIZE_GIANT			5

/* history buffer */
#define HISTSIZE			15
#define PRAYHISTSIZE          25

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_SILVER_ONE	      1
#define OBJ_VNUM_GOLD_ONE	      2
#define OBJ_VNUM_GOLD_SOME	      3
#define OBJ_VNUM_SILVER_SOME	      4
#define OBJ_VNUM_COINS		      5
#define OBJ_VNUM_TOKEN              6

#define OBJ_VNUM_STEAK		      8

#define OBJ_VNUM_CORPSE_NPC	     10
#define OBJ_VNUM_CORPSE_PC	     11
#define OBJ_VNUM_SEVERED_HEAD	     12
#define OBJ_VNUM_TORN_HEART	     13
#define OBJ_VNUM_SLICED_ARM	     14
#define OBJ_VNUM_SLICED_LEG	     15
#define OBJ_VNUM_GUTS		     16
#define OBJ_VNUM_BRAINS		     17

#define OBJ_VNUM_BERRY             20
#define OBJ_VNUM_FLAME             21
#define OBJ_VNUM_SPRING		     22
#define OBJ_VNUM_DISC		     23
#define OBJ_VNUM_QUEST             24
#define OBJ_VNUM_PORTAL		     25
#define OBJ_VNUM_FIREWALL          26
#define OBJ_VNUM_AIRWALL           27
#define OBJ_VNUM_ARROW		   32

#define OBJ_VNUM_SKIMDISC          35

#define OBJ_VNUM_ROSE		     38
#define OBJ_VNUM_STONE		     39
#define OBJ_VNUM_BOF               40
#define OBJ_VNUM_HERON_SWORD       41
#define OBJ_VNUM_RAVEN_SPEAR       42
#define OBJ_VNUM_ANGREAL           43
#define OBJ_VNUM_DRAGON_DAGGER     46
#define OBJ_VNUM_LEGION_SWORD      47
#define OBJ_VNUM_MF_STAFF          48
#define OBJ_VNUM_MF_AXE            49
#define OBJ_VNUM_MF_FLAIL          40060
#define OBJ_VNUM_MF_WHIP           40061
#define OBJ_VNUM_MF_KNUCKLES       40062

#define OBJ_VNUM_PIT		   3010

#define OBJ_VNUM_SCHOOL_MACE	   3772
#define OBJ_VNUM_SCHOOL_DAGGER	   3701
#define OBJ_VNUM_SCHOOL_SWORD	   3702
#define OBJ_VNUM_SCHOOL_SPEAR	   3717
#define OBJ_VNUM_SCHOOL_STAFF	   3718
#define OBJ_VNUM_SCHOOL_AXE		   3719
#define OBJ_VNUM_SCHOOL_FLAIL	   3720
#define OBJ_VNUM_SCHOOL_WHIP	   3721
#define OBJ_VNUM_SCHOOL_POLEARM    3722
#define OBJ_VNUM_SCHOOL_BOW        3723
#define OBJ_VNUM_SCHOOL_LANCE      3724
#define OBJ_VNUM_SCHOOL_ARROW      3725
#define OBJ_VNUM_SCHOOL_VEST	   3703
#define OBJ_VNUM_SCHOOL_SHIELD	   3704
#define OBJ_VNUM_SCHOOL_BANNER     3716
#define OBJ_VNUM_MAP		   3162

#define OBJ_VNUM_WHISTLE	   2116
#define OBJ_VNUM_CALLANDOR	   30003

#define OBJ_VNUM_CRAFTING_ORE      40063
#define OBJ_VNUM_CRAFTING_GEM      40064 
#define OBJ_VNUM_CRAFTING_ARMOR    40065
#define OBJ_VNUM_CRAFTING_JEWEL    40066

#define OBJ_VNUM_FOXHEAD_MEDALLION 5876


/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT		      1
#define ITEM_SCROLL		      2
#define ITEM_WAND		      3
#define ITEM_STAFF		      4
#define ITEM_WEAPON		      5
#define ITEM_TREASURE		      8
#define ITEM_ARMOR		      9
#define ITEM_POTION		     10
#define ITEM_CLOTHING		     11
#define ITEM_FURNITURE		     12
#define ITEM_TRASH		     13
#define ITEM_CONTAINER		     15
#define ITEM_DRINK_CON		     17
#define ITEM_KEY		     18
#define ITEM_FOOD		     19
#define ITEM_MONEY		     20
#define ITEM_NOTEPAPER		     21
#define ITEM_BOAT		     22
#define ITEM_CORPSE_NPC		     23
#define ITEM_CORPSE_PC		     24
#define ITEM_FOUNTAIN		     25
#define ITEM_PILL		     26
#define ITEM_PROTECT		     27
#define ITEM_MAP		     28
#define ITEM_PORTAL		     29
#define ITEM_WARP_STONE       30
#define ITEM_ROOM_KEY         31
#define ITEM_GEM              32
#define ITEM_JEWELRY          33
#define ITEM_JUKEBOX          34
#define ITEM_TATTOO           35
#define ITEM_TOKEN            36
#define ITEM_ANGREAL          37
#define ITEM_FIREWALL         38
#define ITEM_AIRWALL          39
#define ITEM_VEHICLE          40
//the following two flags are used for mining
#define ITEM_ORE              41
#define ITEM_GEMSTONE         42

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW          (A)
#define ITEM_HUM           (B)
#define ITEM_DARK          (C)
#define ITEM_LOCK          (D)
#define ITEM_EVIL          (E)
#define ITEM_INVIS         (F)
#define ITEM_MAGIC         (G)
#define ITEM_NODROP        (H)
#define ITEM_BLESS         (I)
#define ITEM_REPOP_RANDOM_LOCATION (J)
#define ITEM_REPOP_ON_CHANCE	(K)
#define ITEM_FULL_RANDOM	(L)
#define ITEM_NOREMOVE		(M)
#define ITEM_INVENTORY		(N)
#define ITEM_NOPURGE		   (O)
#define ITEM_ROT_DEATH		(P)
#define ITEM_VIS_DEATH		(Q)
#define ITEM_BROKEN           (R)
#define ITEM_NONMETAL		(S)
#define ITEM_NOLOCATE		(T)
#define ITEM_MELT_DROP		(U)
#define ITEM_HAD_TIMER		(V)
#define ITEM_SELL_EXTRACT	(W)
#define ITEM_DRAGGABLE     (X)
#define ITEM_BURN_PROOF		(Y)
#define ITEM_NOUNCURSE		(Z)
#define ITEM_HIDDEN             (aa)
#define ITEM_NOSTEAL            (bb)
#define ITEM_NO_BREAK           (cc)
#define ITEM_KEEPER             (dd)


/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE		(A)
#define ITEM_WEAR_FINGER	(B)
#define ITEM_WEAR_NECK		(C)
#define ITEM_WEAR_BODY		(D)
#define ITEM_WEAR_HEAD		(E)
#define ITEM_WEAR_LEGS		(F)
#define ITEM_WEAR_FEET		(G)
#define ITEM_WEAR_HANDS		(H)
#define ITEM_WEAR_ARMS		(I)
#define ITEM_WEAR_SHIELD	(J)
#define ITEM_WEAR_ABOUT		(K)
#define ITEM_WEAR_WAIST		(L)
#define ITEM_WEAR_WRIST		(M)
#define ITEM_WIELD		(N)
#define ITEM_HOLD		(O)
#define ITEM_NO_SAC		(P)
#define ITEM_WEAR_FLOAT		(Q)
#define ITEM_WEAR_TATTOO	(R)
#define ITEM_WEAR_BACK		(S)
#define ITEM_STUCK_IN		(T)
#define ITEM_WEAR_EAR           (U)
#define ITEM_WEAR_FACE          (V)
#define ITEM_WEAR_MALE_ONLY     (W)
#define ITEM_WEAR_FEMALE_ONLY   (X)

/* weapon class */
#define WEAPON_EXOTIC		0
#define WEAPON_SWORD		1
#define WEAPON_DAGGER		2
#define WEAPON_SPEAR		3
#define WEAPON_MACE		     4
#define WEAPON_AXE		     5
#define WEAPON_FLAIL		6
#define WEAPON_WHIP		     7	
#define WEAPON_POLEARM		8
#define WEAPON_BOW		     9
#define WEAPON_ARROW		10
#define WEAPON_LANCE		11
#define WEAPON_STAFF          12

/* weapon types */
#define WEAPON_FLAMING		(A)
#define WEAPON_FROST		(B)
#define WEAPON_VAMPIRIC		(C)
#define WEAPON_SHARP		(D)
#define WEAPON_VORPAL		(E)
#define WEAPON_TWO_HANDS	(F)
#define WEAPON_SHOCKING		(G)
#define WEAPON_POISON		(H)

/* gate flags */
#define GATE_NORMAL_EXIT  (A)
#define GATE_NOCURSE      (B)
#define GATE_GOWITH       (C)
#define GATE_BUGGY        (D)
#define GATE_RANDOM       (E)
#define GATE_WAYGATE      (F)
#define GATE_DREAMGATE    (G)
#define GATE_SKIMMING_IN  (H)
#define GATE_SKIMMING_OUT (I)

/* furniture flags */
#define STAND_AT		(A)
#define STAND_ON		(B)
#define STAND_IN		(C)
#define SIT_AT			(D)
#define SIT_ON			(E)
#define SIT_IN			(F)
#define REST_AT			(G)
#define REST_ON			(H)
#define REST_IN			(I)
#define SLEEP_AT		(J)
#define SLEEP_ON		(K)
#define SLEEP_IN		(L)
#define PUT_AT			(M)
#define PUT_ON			(N)
#define PUT_IN			(O)
#define PUT_INSIDE		(P)




/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE		      0
#define APPLY_STR		      1
#define APPLY_DEX		      2
#define APPLY_INT		      3
#define APPLY_WIS		      4
#define APPLY_CON		      5
#define APPLY_SEX		      6
#define APPLY_CLASS		      7
#define APPLY_LEVEL		      8
#define APPLY_AGE		      9
#define APPLY_HEIGHT		     10
#define APPLY_WEIGHT		     11
#define APPLY_ENDURANCE		     12
#define APPLY_HIT		     13
/* #define APPLY_MOVE		     14 --- now AWAIL*/
#define APPLY_GOLD		     15
#define APPLY_EXP		     16
#define APPLY_AC		     17
#define APPLY_HITROLL		     18
#define APPLY_DAMROLL		     19
#define APPLY_SAVES		     20
#define APPLY_SAVING_PARA	     20
#define APPLY_SAVING_ROD	     21
#define APPLY_SAVING_PETRI	     22
#define APPLY_SAVING_BREATH	     23
#define APPLY_SAVING_SPELL	     24
#define APPLY_SPELL_AFFECT	     25

#define APPLY_SPHERE_AIR           26
#define APPLY_SPHERE_EARTH         27
#define APPLY_SPHERE_FIRE          28
#define APPLY_SPHERE_SPIRIT        29
#define APPLY_SPHERE_WATER         30

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE		      1
#define CONT_PICKPROOF		      2
#define CONT_CLOSED		           4
#define CONT_LOCKED		           8
#define CONT_PUT_ON		          16
#define CONT_ENTERABLE             32
#define CONT_SOUNDPROOF            64
#define CONT_SEE_OUT              128
#define CONT_SEE_IN               256
#define CONT_SEE_THROUGH          512


/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO		      2
#define ROOM_VNUM_DEATH		   4
#define ROOM_VNUM_SKIMMING       20
#define ROOM_VNUM_CHAT		   1200
#define ROOM_VNUM_TEMPLE	   35501
#define ROOM_VNUM_ALTAR		   3700
#define ROOM_VNUM_SCHOOL	        35501
#define ROOM_VNUM_RECALL        8 // linked to Cairhien 
#define ROOM_VNUM_RECOVERY      37017 // Cairhien 
#define ROOM_VNUM_ARENA_A	29199
#define ROOM_VNUM_ARENA_B	29198
#define ROOM_VNUM_ARENA_C	29084
#define ROOM_VNUM_ARENA_D	29086
#define ROOM_VNUM_DFRECALL	12698


//Recall rooms
#define ROOM_VNUM_RECALL_MAYENE 12239
#define ROOM_VNUM_RECALL_TEAR   30011
#define ROOM_VNUM_RECALL_SHAIDO 16897
#define ROOM_VNUM_RECALL_CAEMLYN 63013
#define ROOM_VNUM_RECALL_FALDARA 30356
#define ROOM_VNUM_RECALL_MARADON 21069
#define ROOM_VNUM_RECALL_OSENREIN 24105
#define ROOM_VNUM_RECALL_FALME   8405
#define ROOM_VNUM_RECALL_CAIRHIEN 4269 // Cairhien 
#define ROOM_VNUM_RECALL_MALDEN   719
#define ROOM_VNUM_RECALL_SHOLARBELLA 39558

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK		      (A)
#define ROOM_NOQUEST                  (B)
#define ROOM_NO_MOB		      (C)
#define ROOM_INDOORS		 (D)
#define ROOM_VMAP              (E)

//Mining flags
#define ROOM_MINING_GOLD            (F)
#define ROOM_MINING_SILVER          (G)
#define ROOM_MINING_COPPER          (H)
#define ROOM_MINING_EMERALD         (I)

#define ROOM_PRIVATE		(J)
#define ROOM_SAFE		     (K)
#define ROOM_SOLITARY		(L)
#define ROOM_PET_SHOP		(M)
#define ROOM_NO_RECALL		(N)
#define ROOM_IMP_ONLY		(O)
#define ROOM_GODS_ONLY		(P)
#define ROOM_HEROES_ONLY	     (Q)
#define ROOM_NEWBIES_ONLY	(R)
#define ROOM_LAW		     (S)
#define ROOM_NOWHERE		(T)
#define ROOM_STEDDING         (U)
#define ROOM_NO_GATE          (V)
#define ROOM_ARENA	      (W)
#define ROOM_DEATHTRAP	      (X)
#define ROOM_MINING_RUBY      (Y)
#define ROOM_MINING_DIAMOND   (Z)


/*
 * Mining flags for quality and type
 */
#define MINING_QUALITY_FLAWED    (A)
#define MINING_QUALITY_FLAWLESS  (B)
#define MINING_QUALITY_EXCELLENT (C)
#define MINING_QUALITY_PERFECT   (D)

#define MINING_ORE_COPPER        (A)
#define MINING_ORE_SILVER        (B)
#define MINING_ORE_GOLD          (C)

#define MINING_GEM_RUBY          (A)
#define MINING_GEM_DIAMOND       (B)
#define MINING_GEM_EMERALD       (C)


/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH		      0
#define DIR_EAST		      1
#define DIR_SOUTH		      2
#define DIR_WEST		      3
#define DIR_UP			      4
#define DIR_DOWN		      5

#define ENTER_NORTH            6
#define ENTER_EAST             7
#define ENTER_SOUTH            8
#define ENTER_WEST             9


/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR           (A)
#define EX_CLOSED           (B)
#define EX_LOCKED           (C)
#define EX_PICKPROOF        (F)
#define EX_NOPASS           (G)
#define EX_EASY             (H)
#define EX_HARD             (I)
#define EX_INFURIATING      (J)
#define EX_NOCLOSE          (K)
#define EX_NOLOCK           (L)
#define EX_HIDDEN           (M)
#define EX_NOBASH           (N)
#define EX_FIREWALL         (O)
#define EX_AIRWALL          (P)
#define EX_BLOCKED          (R)
#define EX_BARRED           (S)


/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE		      0
#define SECT_CITY		         1
#define SECT_FIELD		      2
#define SECT_FOREST		      3
#define SECT_HILLS		      4
#define SECT_MOUNTAIN		   5
#define SECT_WATER_SWIM		   6
#define SECT_WATER_NOSWIM	   7
#define SECT_UNUSED		      8
#define SECT_AIR		         9
#define SECT_DESERT		     10
#define SECT_WAYS		        11
#define SECT_ROCK_MOUNTAIN   12
#define SECT_SNOW_MOUNTAIN   13
#define SECT_ROAD            14
#define SECT_ENTER           15
#define SECT_SWAMP           16
#define SECT_JUNGLE          17
#define SECT_RUINS           18
#define SECT_OCEAN           19
#define SECT_RIVER           20
#define SECT_SAND            21
#define SECT_BLIGHT          22
#define SECT_ISLAND          23
#define SECT_LAKE            24
#define SECT_MAX             25




/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE		     -1
#define WEAR_LIGHT		      0
#define WEAR_FLOAT		      1
#define WEAR_TATTOO		      2
#define WEAR_FINGER_L		      3
#define WEAR_FINGER_R		      4
#define WEAR_NECK_1		      5
#define WEAR_NECK_2		      6
#define WEAR_BODY		      7
#define WEAR_HEAD		      8
#define WEAR_LEGS		      9
#define WEAR_FEET		     10
#define WEAR_HANDS		     11
#define WEAR_ARMS		     12
#define WEAR_SHIELD		     13
#define WEAR_BACK		     14
#define WEAR_ABOUT		     15
#define WEAR_WAIST		     16
#define WEAR_WRIST_L		     17
#define WEAR_WRIST_R		     18
#define WEAR_HOLD		     19
#define WEAR_WIELD		     20
#define WEAR_SECOND_WIELD	     21
#define WEAR_STUCK_IN		     22
#define WEAR_EAR_L		     23
#define WEAR_EAR_R		     24
#define WEAR_FACE		     25
#define WEAR_SCABBARD_1              26
#define WEAR_SCABBARD_2              27
#define MAX_WEAR		     28
#define WEAR_MALE_ONLY               29
#define WEAR_FEMALE_ONLY             30

/*
 * Command logging types.
 */
#define LOG_NORMAL	0
#define LOG_ALWAYS	1
#define LOG_NEVER	2
#define LOG_ROLEPLAY	3

/*
 * Command types.
 */
#define TYP_NUL 0
#define TYP_UNDEF 1
#define TYP_CMM	10
#define TYP_CBT	2
#define TYP_ESP 3
#define TYP_GRP 4
#define TYP_OBJ 5
#define TYP_INF 6
#define TYP_OTH 7
#define TYP_MVT 8
#define TYP_CNF 9
#define TYP_LNG 11
#define TYP_PLR 12
#define TYP_OLC 13


/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK		      0
#define COND_FULL		      1
#define COND_THIRST		      2
#define COND_HUNGER		      3



/*
 * Positions.
 */
#define POS_DEAD		      0
#define POS_MORTAL		      1
#define POS_INCAP		      2
#define POS_STUNNED		      3
#define POS_SLEEPING           4
#define POS_RESTING            5
#define POS_SITTING            6
#define POS_FIGHTING           7
#define POS_STANDING           8


/*
 * APP bits for players.
 * bits that affect their appearance
 */
#define APP_CLOAKED           (A)
#define APP_HOODED            (B)
#define APP_WOLFSHAPE         (C)
#define APP_DISGUISED         (D)
#define APP_COLORCLOAKED      (E) // Warders
#define APP_VEILED            (F) // Aiels
#define APP_ILLUSION          (G) // Mirror of Mists
#define APP_SIZE_ILLUSION     (H)
#define APP_MASQUERADE        (I)

/*
 * New AUTO flags. Extended from PLR_AUTOxxx flags
 */
#define AUTO_PARRY            (A)
#define AUTO_DODGE            (B)
#define AUTO_SHIELDBLOCK      (C)
#define AUTO_MASTERFORMS      (D)
#define AUTO_DRAW             (E)
#define AUTO_BIND             (F)
#define AUTO_VOTEREMINDER     (G)
#define AUTO_BLINDFOLD        (H)

/*
 * Channeler flags
 */
#define CHAN_SEEAREAWEAVES    (A)  // See area weaves
#define CHAN_SEECHANNELING    (B)  // See embrace/seize messages

/*
 * 'IC' In Character flags
 */
#define IC_RP                 (A) // IS Role Playing
#define IC_DRAGON_REBORN      (B) // IS -the- Dragon Reborn
#define IC_FORSAKEN           (C) // IS one of the Forsaken
#define IC_TAVEREN            (D) // IS a taveren
#define IC_SPECIAL            (E) // IS a taveren

/*
 * ACT bits for players.
 */
#define PLR_IS_NPC            (A)		/* Don't EVER set.	*/

/* RT auto flags */
#define PLR_AUTOASSIST		(C)
#define PLR_AUTOEXIT		(D)
#define PLR_AUTOLOOT		(E)
#define PLR_AUTOSAC           (F)
#define PLR_AUTOGOLD		(G)
#define PLR_AUTOSPLIT		(H)
#define PLR_SEESELF           (I)

/* Channeling flags */
#define PLR_CAN_CHANNEL       (J)
#define PLR_STILLED           (K)
#define PLR_AUTOSLICE         (L)

/* Guild flags */
#define PLR_MORTAL_LEADER     (M)

/* RT personal flags */
#define PLR_HOLYLIGHT		(N)
#define PLR_CANLEVEL          (O)
#define PLR_CANLOOT		     (P)
#define PLR_NOSUMMON		(Q)
#define PLR_NOFOLLOW		(R)
#define PLR_COLOUR		     (T)
/* 1 bit reserved, S */

/* penalty flags */
#define PLR_PERMIT		     (U)
#define PLR_UNVALIDATED       (V)
#define PLR_LOG			(W)
#define PLR_DENY		     (X)
#define PLR_FREEZE		     (Y)
#define PLR_THIEF		     (Z)
#define PLR_KILLER		    (aa)
#define PLR_GRADUATED        (bb)
#define PLR_AUTOEXAMINE      (cc)
#define PLR_IS_NEWBIEHELPER  (dd)
#define PLR_QUESTING            (ff)



/*  New Player flags to be used on the act2 field */
#define PLR2_FACELESS	(A)	//is a faceless character
#define PLR2_PKILLER	(B)	//is a player killer
#define PLR2_NOFINGER   (C)     //Player doesn't want fingered
#define PLR2_MASKCHAN   (D)     //Player is masking the ability to channel
#define PLR2_SEEKINGRP  (E)     //Seeking rp - show the R flag
#define PLR2_WOLFFORM   (F)     //Use wolf form in tel'aran'rhiod



#define COMM2_NOOGUILD      (A)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET            (A)
#define COMM_DEAF            	(B)
#define COMM_NOWIZ            (C)
#define COMM_NOAUCTION        (D)
#define COMM_NOGOSSIP         (E)
#define COMM_NOGUILDLEADER    (F)
#define COMM_NOSGUILD         (F)
#define COMM_NOMUSIC          (G)
#define COMM_NOCLAN		     (H)
#define COMM_NODFT		     (I)
						
//#define COMM_SHOUTSOFF		(J)
#define COMM_NOHINT           (J)
#define COMM_OLCX		     (K)
#define COMM_NORACE		     (L)
#define COMM_NOCHAT           (M)
#define COMM_NOGAME           (aa)
#define COMM_NONEWBIE         (bb)
#define COMM_FORETELLING      (cc)
#define COMM_NOWKT            (dd)
#define COMM_NOMINION         (ee)

/* display flags */
#define COMM_COMPACT		(N)
#define COMM_BRIEF		     (O)
#define COMM_PROMPT		     (P)
#define COMM_COMBINE		(Q)
#define COMM_TELNET_GA		(R)
#define COMM_NOSSGUILD        (S)
#define COMM_NOSHADOW         (T)

/* penalties */
#define COMM_NOEMOTE		(U)
#define COMM_NOSHOUT		(V)
#define COMM_NOTELL		     (W)
#define COMM_NOCHANNELS		(X) 
#define COMM_SNOOP_PROOF	     (Y)
#define COMM_AFK		     (Z)

/* WIZnet flags */
#define WIZ_ON			(A)
#define WIZ_TICKS		(B)
#define WIZ_LOGINS		(C)
#define WIZ_SITES		(D)
#define WIZ_LINKS		(E)
#define WIZ_DEATHS		(F)
#define WIZ_RESETS		(G)
#define WIZ_MOBDEATHS		(H)
#define WIZ_FLAGS		(I)
#define WIZ_PENALTIES		(J)
#define WIZ_SACCING		(K)
#define WIZ_LEVELS		(L)
#define WIZ_SECURE		(M)
#define WIZ_SWITCHES		(N)
#define WIZ_SNOOPS		(O)
#define WIZ_RESTORE		(P)
#define WIZ_LOAD		(Q)
#define WIZ_NEWBIE		(R)
#define WIZ_PREFIX		(S)
#define WIZ_SPAM		(T)
#define WIZ_CHANNELING   	(U)
#define WIZ_ROLEPLAY  		(V)
#define WIZ_MINION  		(W)
#define WIZ_GUILD  		(X)
#define WIZ_SUBGUILD  		(Y)
#define WIZ_SUBSUBGUILD 	(Z)

/* Prototypes for clan data */

typedef struct guild_rank       rankdata;

struct guild_rank
{
  char *rankname;               /* name of rank                         */
  char *skillname;              /* name of skill earned at this rank    */
};

/* mortal leader rights are as follows;
 ml[0] = can_guild
 ml[1] = can_deguild
 ml[2] = can_promote
 ml[3] = can_demote
*/

struct clan_type
{
  long flags;              /* flags for guild                      */
  char *name;			  /* name of guild                        */
  char *who_name;		  /* name sent for "who" command		  */
  unsigned int room[3];          /* hall/morgue/temple rooms             */
  rankdata rank[MAX_RANK]; /* guilds rank system                   */
  int ml[4];			  /* mortal leader rights 		       */
};

struct sguild_type
{
  long flags;              /* flags for guild                      */
  char *name;			  /* name of guild                        */
  char *who_name;		  /* name sent for "who" command		  */
  unsigned int room[3];          /* hall/morgue/temple rooms             */
  rankdata rank[MAX_RANK]; /* guilds rank system                   */
  int ml[4];			  /* mortal leader rights 		       */
};

struct ssguild_type
{
  long flags;              /* flags for guild                      */
  char *name;			  /* name of guild                        */
  char *who_name;		  /* name sent for "who" command		  */
  unsigned int room[3];          /* hall/morgue/temple rooms             */
  rankdata rank[MAX_RANK]; /* guilds rank system                   */
  int ml[4];			  /* mortal leader rights 		       */
};

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct	mob_index_data
{
  MOB_INDEX_DATA *	next;
  SPEC_FUN     *spec_fun;
  SHOP_DATA    *pShop;
  MPROG_LIST   *mprogs;
  AREA_DATA    *area;		/* OLC */
  unsigned int	vnum;
  sh_int		group;
  bool		new_format;
  sh_int		count;
  sh_int		killed;
  char *		player_name;
  char *		short_descr;
  char *		long_descr;
  char *		description;
  long		act;
  long		affected_by;
  
  /* New gain/train controll */
  long         gain_flags;
  long         train_flags;
  sh_int       train_level;
  
  sh_int		alignment;
  sh_int		level;
  sh_int		hitroll;
  sh_int		hit[3];
  sh_int		endurance[3];
  
  /* WoT channeling for mobiles */
  sh_int       sphere[MAX_SPHERE];

  /* World flag for mobiles */
  long         world;
  
  sh_int		damage[3];
  sh_int		ac[4];
  sh_int 		dam_type;
  long		off_flags;
  long		imm_flags;
  long		res_flags;
  long		vuln_flags;
  sh_int		start_pos;
  sh_int		default_pos;
  sh_int		sex;
  sh_int		race;
  long		wealth;
  long		form;
  long		parts;
  sh_int		size;
  sh_int		reset_num;
  char *		material;
  long		mprog_flags;

  sh_int       guild_guard;        /* Guild guards            */
  long         guild_guard_flags;  /* Action for guild guards */  
};



/* memory settings */
#define MEM_CUSTOMER	A	
#define MEM_SELLER	B
#define MEM_HOSTILE	C
#define MEM_AFRAID	D

/* memory for mobs */
struct mem_data
{
    MEM_DATA 	*next;
    bool	valid;
    int		id; 	
    int 	reaction;
    time_t 	when;
};

#define MAX_BURNOUT 20
#define MAX_NORMAL_LEVEL 250
#define MAX_TRAINABLE_SKILL_LEVEL 300

#include "board.h"

/*
 * One character (PC or NPC).
 */
struct	char_data
{
  CHAR_DATA *		next;
  CHAR_DATA *		next_in_room;
  CHAR_DATA *		master;
  CHAR_DATA *		leader;
  CHAR_DATA *		fighting;
  CHAR_DATA *		reply;        /* Last tell PC     */
  bool              replyWintro;  /* False intro flag */
  CHAR_DATA *       dfreply;      /* DF tell          */
  CHAR_DATA *		pet;
  CHAR_DATA *       mount;
  CHAR_DATA *       to_hunt;
  bool		     riding;	    /* mount data       */
  CHAR_DATA *		mprog_target;
  MEM_DATA *		memory;
  OBJ_DATA *		in_obj;
  SPEC_FUN *		spec_fun;
  MOB_INDEX_DATA *	pIndexData;
  DESCRIPTOR_DATA *	desc;
  AFFECT_DATA *	affected;
  OBJ_DATA *		carrying;
  OBJ_DATA *		on;
  ROOM_INDEX_DATA *	in_room;
  ROOM_INDEX_DATA *	was_in_room;
  AREA_DATA *		zone;
  PC_DATA *		pcdata;
  GEN_DATA *		gen_data;
  bool		valid;
  bool         editor;
  sh_int      arena;
  char *		name;
  char *       wkname;
  char *       real_name;
  long		id;
  sh_int		version;
  char *		short_descr;
  char *		long_descr;
  char *       description;
  char *       hood_description;
  char *       wolf_description;
  char *       wound_description;
  char *       aura_description;
  char *       veil_description;
  char *       sheat_where_name[MAX_SHEAT_LOC];
  char *		prompt;
  char *		prefix;
  sh_int		group;
  BLOCK_DATA        exit_block;

  /* Guild */
  sh_int		clan;
  sh_int       rank;
  char *       gtitle;
  bool         ginvis;
  bool         gmute;

  /* oGuild */
  sh_int       trank; /* temp for usage */
  sh_int       oguild;
  sh_int       oguild_rank;
  char *       oguild_title;
  bool         oguild_invis;
  bool         oguild_mute;
  
  /* sGuild */
  sh_int       sguild;
  sh_int       sguild_rank;
  char *       sguild_title;
  bool         sguild_invis;

  /* ssGuild */
  sh_int       ssguild;
  sh_int       ssguild_rank;
  char *       ssguild_title;
  bool         ssguild_invis;

  /* Minion */
  long        minion;
  sh_int      mrank;
  char *      mtitle;
  char *      mname;

  /* World */
  long         world;
  unsigned int world_vnum;

  /* Used for horse mounting */
  bool         mount_quiet;

  sh_int		sex;
  sh_int		class;
  sh_int		race;
  sh_int		level;
  sh_int		trust;
  int		played;
  long		roleplayed;
  int		insanity_points;
  time_t        lastrpupdate;
  int		lines;  /* for the pager */
  time_t	logon;
  time_t        surrender_timeout;

  bool         study;
  sh_int       study_pulse;  /* for study code */
  char *       study_name;  /* for gate key alias */

  sh_int		timer;
  sh_int		wait;
  sh_int		daze;
     int		hit;
     int		max_hit;
     int		endurance;
     int		max_endurance;

  /* WoT channeling */
  bool           bIsReadyForLink;
  bool           bIsLinked;
  CHAR_DATA *    pIsLinkedBy;
  LINK_DATA *    link_info;
  sh_int         cre_sphere[MAX_SPHERE];  /* Creation sphere                        */
  int            perm_sphere[MAX_SPHERE]; /* Original sphere strenght               */
  sh_int         mod_sphere[MAX_SPHERE];  /* Modifier to spheres from angreals      */
  sh_int         main_sphere;             /* Main sphere                            */
  unsigned long  holding;                 /* How much OP CH is holding              */
  sh_int         burnout;                 /* Current BO value                       */
  sh_int         max_burnout;             /* MAX BO value for CH                    */
  sh_int         channeling_pulse;        /* Tick tick tick                         */
  sh_int         autoholding;             /* Percent how much to channel of your OP */
  long           chan_flags;              /* See area weaves etc., flag toggle      */

  /* merits/flaws/talents */
  long           merits;
  long           flaws;
  long           talents;

  /* Gold / Silver */
  long           gold;
  long           silver;

  bool           gain_xp; /* Flag to id if player get exp this round of combat */
  int            exp;
  long           act;
  long 		 act2;

  long         app;      /* appearance flags   */
  long         auto_act; /* more auto flags    */
  long         ic_flags; /* In character flags */  
		
  /* New gain/train controll */
  long         gain_flags;
  long         train_flags;
  sh_int       train_level;
  time_t       next_trainmove;        /* Move mob to random location after 3 minutes if set with correct bit         */

  long		comm;                      /* RT added to pad the vector         */
  long          comm2;
  long		wiznet;                    /* wiznet stuff                       */
  long		imm_flags;
  long		res_flags;
  long		vuln_flags;
  sh_int		invis_level;
  sh_int		incog_level;
  long		affected_by;
  sh_int		position;
/*  sh_int		practice; */
  sh_int		train;
  sh_int		carry_weight;
  sh_int		carry_number;
  sh_int		saving_throw;
  sh_int		alignment;
  sh_int		hitroll;
  sh_int		damroll;
  sh_int		armor[4];
  sh_int		wimpy;

  /* stats */
  sh_int		perm_stat[MAX_STATS];
  sh_int		mod_stat[MAX_STATS];

  /* body stats */
  int          hit_loc[MAX_HIT_LOC];
  int          target_loc;
  int          defend_loc;
 
  /* parts stuff */
  long		form;
  long		parts;
  sh_int		size;
  char*		material;

  // Guild guard
  sh_int       guild_guard;
  long         guild_guard_flags;

  /* PK Stuff */
  time_t 	next_pkill;        /* Safe for 30 minutes         */
  sh_int	pk_count;
  sh_int	pk_died_count;
  /* Arrow Stuff  */
  sh_int	arrow_count;
  /* mobile stuff */
  long		off_flags;
  sh_int		damage[3];
  sh_int		dam_type;
  sh_int		start_pos;
  sh_int		default_pos;
  
  sh_int		mprog_delay;

  // Skimming
  bool             skim;
  sh_int           skim_pulse;
  bool             skim_motion;
  bool             skim_follow;
  unsigned int     skim_to;
};

/*
 *  Linked list of people linked by channelling
 */
struct link_data 
{
   LINK_DATA * next;
   CHAR_DATA * linked;
};

/*
 * RP Data - A PC's RP background data
 */
struct rp_data
{
	//Attributes
	int strength;
	int dexterity;
	int stamina;
	int charisma;
	int manipulation;
	int appearance;
	int perception;
	int intelligence;
	int wits;
	//Abilities
	//Talents
	int alertness;
	int athletics;
	int brawl;
	int empathy;
	int expression;
	int intimidation;
	int leadership;
	int streetwise;
	int subterfuge;

	//skills
	int crafts;
	int etiquette;
	int ranged_weapons;
	int melee_weapons;
	int security;
	int stealth;
	int survival;

	//knowledge
	int finance;
	int investigation;
	int law;
	int medicine;
	int politics;
	int science;
	int sales;	

	//advantages
	int nobility;
	int warrior_tradition;
	int rogue_tradition;
	int merchant_tradition;
	int innkeeper_tradition;

};
/*
 * Data which only PC's have.
 */
struct	pc_data
{
  PC_DATA *		next;
  BUFFER * 		buffer;
  COLOUR_DATA *	code;		/* Data for coloUr configuration	*/
  bool		     valid;
  char *            email;
  char *	          pwd;
  char *            bamfin;
  char *            bamfout;
  char *            title;
  char *            appearance;
  char *            hood_appearance;
  char *            wolf_appearance;
  char *            veil_appearance;
  char *            illusion_appearance;
  char *            masquerade_appearance;
  char *            dreaming_appearance;
  char *            ictitle;
  char *            imm_info;
  char *            lastsite;
  char *            lastlog;
  char *            afkmsg;
  char *            semote;
  char *	    referrer;
  sh_int		     perm_hit;
  sh_int	          perm_endurance;
  sh_int	          true_sex;
  int		    extended_level;
  int               last_level;
  int		    home;
  int		    iclocrecall;
  sh_int            condition[4];

  //BLOCK_DATA        exit_block;

  // Voices - PCs can set theis voice
  char *            say_voice;
  char *            ask_voice;
  char *            exclaim_voice;
  char *            battlecry_voice;

  // Bank
  long			gold_bank;
  long			silver_bank;

  // PC Guard stuff
  CHAR_DATA        *guarded_by;  // who is guarding you?
  CHAR_DATA        *guarding;    // who are you guarding?

  //QUEST STUFF
  long	 	    quest_curr; //current number of quest points
  long		    quest_accum;  //quest points accumulated in the players life
  CHAR_DATA        *questgiver;  //who gave the quest
  unsigned int 	    questgivervnum;  //who gave the quest
  long		    questpoints;
  int               nextquest;
  int               countdown;
  int               questobj;
  int               questmob;
  int               forcespark;
  int               forceinsanity;

#if defined(FIRST_BOOT)
  sh_int		learned		[MAX_SKILL];
  bool		group_known	[MAX_GROUP];
#else
  sh_int *     learned;
  bool *	     group_known;
#endif

  sh_int       points;
  
  sh_int       sphere_points;
  sh_int       stat_points;
  sh_int       misc_points;
  
  bool         confirm_delete;
  char *		alias[MAX_ALIAS];
  char * 		alias_sub[MAX_ALIAS];
  char *       ignore[MAX_IGNORE];
  char *       disguise[MAX_DISGUISE];  /* Disguise                  */

  BOARD_DATA *	board;			     /* The current board 	    */
  time_t		last_note[MAX_BOARD];	/* last note for the boards  */
  int          bounty;
  NOTE_DATA *	in_progress;
  NOTE_DATA *  last_read_note;
  HISTORY_DATA tell_history[HISTSIZE];
  ID_NAME *    names;                   /* Intro system              */
  int          bondcount;               //Number of times the person has bonded someone
  int          bondedbysex;               
  char *       bondedby;
  char *       restoremessage;          //Imm Restore Message */
  HISTORY_DATA bond_history[HISTSIZE];

  //Gatekeys saved on pfile
  GATEKEY_DATA * keys; 

  /* Masterform train timers */
  time_t next_bmtrain;            /* Blademaster         */
  time_t next_sdtrain;            /* Speardancer         */
  time_t next_dutrain;            /* Duelling            */
  
  time_t next_assassinate;        /* Assassinate         */
  time_t next_charge;             /* Charge time         */
  time_t next_hide;               /* Hide after revealed */
  time_t next_quit;               /* When you can quit after a fight */

  time_t next_mine;               /* When the character can next mine */


  /*Create Angreal Times */
  time_t next_createangreal;
  time_t next_24hourangreal;
  int	 createangrealcount;

  // Channeling timers
  time_t next_wrap;               /* Wrap timer          */
  
  time_t next_recall;             /* Recall time         */
  
  /* Subdue timer */
  time_t       last_subdue;

  /* Backup time */
  time_t       last_backup;
  
  time_t       next_warderupdate; /* Warder sense time              */
  time_t       next_trupdate;     /* Trolloc sense human sent timer */

  /* vote web click time */
  time_t       last_web_vote;

  /* extra xp multiplier reward */
  time_t       reward_time;
  int	       reward_multiplier;

  /* RP Reward Timer */
  time_t       rprewardtimer;
  int          rpbonus;

  /* Darkfriend stuff */
  int           df_level;
  char        * df_name;
  
  /* Trolloc stuff */
  int           clan_kill_cnt;
  int           pc_kill_cnt;
  
  /* Guild stuff */
  long		guild_requestor;
  long		guilded_by;
  sh_int		guild_offer;

  /* oGuild stuff */
  long		oguild_requestor;
  long		oguilded_by;
  sh_int		oguild_offer;

  /* sGuild stuff */
  long         sguild_requestor;
  long         sguilded_by;
  sh_int       sguild_offer;

  /* ssGuild stuff */
  long         ssguild_requestor;
  long         ssguilded_by;
  sh_int       ssguild_offer;
  
  /* Minion stuff */
  long         minion_requestor;
  long         minioned_by;
  long         minion_offer;
  HISTORY_DATA minion_history[HISTSIZE];

  /* Group talk */
  HISTORY_DATA group_history[HISTSIZE];
 
  /*Polls*/
  char *  polls;          /* record of polls voted on */

  /*	Can they re-arrange their spheres?	*/
  int		spheretrain ;

  
  /*
   * Colour data stuff for config.
   */
    int                   auction[3];             /* {a */
  						  /* {b = blue */
    						  /* {c = cyan */
    int                   gossip[3];              /* {d */
    int                   music[3];               /* {e */
    int                   chat[3];                /* {f */
						  /* {g = green */
    int		 	  minion[3];              /* {h */
    int                   immtalk[3];             /* {i */
    int                   game[3];   		  /* {j */
    int                   tell[3];                /* {k */
    int                   reply[3];               /* {l */
						  /* {m = magenta */
    int                   gtell[3];               /* {n */
    int                   room_exits[3];          /* {o */
    int                   room_things[3];         /* {O */
    int                   prompt[3];              /* {p */
    int                   room_people[3];         /* {P */
    int                   room[3];                /* {q */
						  /* {r = red */
    int                   bondtalk[3];            /* {s */
    int                   channel_name[3];        /* {t */
    int                   wiznet[3];              /* {u */
    int                   fight_death[3];         /* {1 */
    int                   fight_yhit[3];          /* {2 */
    int                   fight_ohit[3];          /* {3 */
    int                   fight_thit[3];          /* {4 */
    int                   fight_skill[3];         /* {5 */
    int                   wolfkin_talk[3];        /* {6 */
    int                   sayt[3];                /* {7 */
    int                   guild_talk[3];          /* {8 */
    int                   osay[3];                /* {9 */
    int                   race_talk[3];           /* {v */
    int                   df_talk[3];             /* {X */
    int                   newbie[3];              /* {z */
    int                   sguild_talk[3];         /* {S */
    int                   ssguild_talk[3];        /* {U */
    int                   leader_talk[3];         /* {L */
    int                   oguild_talk[3];         /* {N */
    int 		security;	/* OLC */ /* Builder security */
    //recreation stuff
    bool keepoldstats;
  sh_int prev_class;
    time_t  		   timeoutstamp;          //Timeout timestamp.  If set, the player is in timeout until the given time
};

/* Data for generating characters -- only used during generation */
struct gen_data
{
    GEN_DATA	*next;
    bool	valid;
#if defined(FIRST_BOOT)
    bool	skill_chosen[MAX_SKILL];
    bool	group_chosen[MAX_GROUP];
#else
    bool *	skill_chosen;
    bool *	group_chosen;
#endif
    int		points_chosen;


};



/*
 * Liquids.
 */
#define LIQ_WATER        0

struct	liq_type
{
    char *	liq_name;
    char *	liq_color;
    sh_int	liq_affect[5];
};



/*
 * Extra description data for a room or object.
 */
struct	extra_descr_data
{
    EXTRA_DESCR_DATA *next;	/* Next in list                     */
    bool valid;
    char *keyword;              /* Keyword in look/examine          */
    char *description;          /* What to see                      */
};



/*
 * Prototype for an object.
 */
struct	obj_index_data
{
    OBJ_INDEX_DATA *	next;
    EXTRA_DESCR_DATA *	extra_descr;
    AFFECT_DATA *	affected;
    AREA_DATA *		area;		/* OLC */
    bool		new_format;
    char *		name;
    char *		short_descr;
    char *		description;
    unsigned int		vnum;
    sh_int		reset_num;
    char *		material;
    sh_int		item_type;
    int			extra_flags;
    int			wear_flags;
    sh_int		level;
    sh_int 		condition;
    sh_int		count;
    sh_int		weight;
    int			cost;
    int			value[5];
};



/*
 * One object.
 */
struct	obj_data
{
    OBJ_DATA *		next;
    OBJ_DATA *		next_content;
    OBJ_DATA *		contains;
    OBJ_DATA *		in_obj;
    OBJ_DATA *		on;
    CHAR_DATA *		carried_by;
    EXTRA_DESCR_DATA *	extra_descr;
    AFFECT_DATA *	affected;
    OBJ_INDEX_DATA *	pIndexData;
    ROOM_INDEX_DATA *	in_room;
    bool		valid;
    bool		enchanted;
    char *	        owner;
    char *		name;
    char *		short_descr;
    char *		description;
    sh_int		item_type;
    int			extra_flags;
    int			wear_flags;
    sh_int		wear_loc;
    sh_int		weight;
    int			cost;
    sh_int		level;
    sh_int 		condition;
    char *		material;
    sh_int		timer;
    int			value	[5];
    CHAR_IN_DATA *      who_in;
//    long 		world; //Which world does the object reset in
};



/*
 * Exit data.
 */
struct	exit_data
{
    union
    {
	ROOM_INDEX_DATA *	to_room;
	unsigned int			vnum;
    } u1;
    long		exit_info;
    unsigned int		key;
    char *		keyword;
    char *		description;
    EXIT_DATA *		next;		/* OLC */
    int			rs_flags;	/* OLC */
    int			orig_door;	/* OLC */
};



/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile 
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct	reset_data
{
  RESET_DATA *	next;
  char		command;
  unsigned int arg1;     /* large vnums */
  unsigned int arg2;     /* large vnums */
  unsigned int arg3;     /* large vnums */
  unsigned int arg4;     /* large vnums */

/*
    sh_int		arg1;
    sh_int		arg2;
    sh_int		arg3;
    sh_int		arg4;
*/
};



/*
 * Area definition.
 */
struct	area_data
{
  AREA_DATA *	next;
  HELP_AREA *	helps;
  char *		file_name;
  char *		name;
  char *		credits;
  sh_int		age;
  sh_int		nplayer;
  sh_int		low_range;
  sh_int		high_range;
  unsigned int min_vnum;
  unsigned int	max_vnum;
  bool		empty;
  char *		builders;	   /* OLC */ /* Listing of */
  unsigned int	vnum;	   /* OLC */ /* Area vnum  */
  int		area_flags;  /* OLC */
  int		security;	   /* OLC */ /* Value 1-9  */
  int          version;     /* Version of the area file */

  WARD_DATA * area_wards;   /* Area wards */
  int         wards_cnt;    /* Counter of how many room wards set in this area */
  int         weave_cnt;    /* Counter of how many room weaves set in this area */
};



/*
 * Room type.
 */
struct	room_index_data
{
  AFFECT_DATA      * weaves;
  WARD_DATA        * wards;
  RESIDUE_DATA     * residues;
  ROOM_INDEX_DATA  * next;
  CHAR_DATA        * people;
  OBJ_DATA         * contents;
  OBJ_DATA	   * inside_of;
  EXTRA_DESCR_DATA * extra_descr;
  AREA_DATA        * area;
  EXIT_DATA        * exit	[10];
  RESET_DATA       * reset_first;	/* OLC */
  RESET_DATA       * reset_last;	/* OLC */
  char             * name;
  char             * description;
  char             * owner;
  unsigned int		vnum;
  int               room_flags;
  sh_int            light;
  sh_int            sector_type;
  sh_int            heal_rate;
  sh_int            endurance_rate;
  sh_int            clan;
  sh_int            reset_num;
};



/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED               -1
#define TYPE_HIT                     2000



/*
 *  Target types.
 */
#define TAR_IGNORE		    0
#define TAR_CHAR_OFFENSIVE	    1
#define TAR_CHAR_DEFENSIVE	    2
#define TAR_CHAR_SELF		    3
#define TAR_OBJ_INV		    4
#define TAR_OBJ_CHAR_DEF	    5
#define TAR_OBJ_CHAR_OFF	    6
#define TAR_CHAR_OTHER       7

#define TARGET_CHAR		    0
#define TARGET_OBJ		    1
#define TARGET_ROOM		    2
#define TARGET_NONE		    3

/* Restrictions for skills:
   Normal  - normal
   Nogain  - Not gainable
   Noteach - can't teach to others
   Granted - granted only
*/
#define RES_NORMAL           0
#define RES_NOTEACH          1
#define RES_NOGAIN           2
#define RES_GRANTED          3
#define RES_MALE             4
#define RES_FEMALE           5
#define RES_TALENT           6
#define RES_NOTRAIN          7
#define RES_TRAINSAMESEX     8

/*
 * Skills include spells as a particular case.
 */
struct	skill_type
{
    char *	name;                    /* Name of skill               */
    sh_int  restriction;             /* noteach, teach, granted etc */
    int     talent_req;                /* Is there a talent required? */
    sh_int	skill_level[MAX_CLASS];	/* Level needed by class       */
    sh_int	rating[MAX_CLASS];	     /* How hard it is to learn     */	
    SPELL_FUN *	spell_fun;		/* Spell pointer (for spells)  */
    sh_int	target;                  /* Legal targets               */
    sh_int	minimum_position;        /* Position for caster / user  */
    sh_int *	pgsn;                    /* Pointer to associated gsn   */
    sh_int	slot;                    /* Slot for #OBJECT loading    */
    sh_int     spheres[MAX_SPHERE];     /* Minimum sphere needed for usage */
    sh_int	min_endurance;           /* Minimum endurance used      */
    sh_int	beats;                   /* Waiting time after use      */
    char *	noun_damage;             /* Damage message              */
    char *	msg_off;                 /* Wear off message	           */
    char *	msg_obj;                 /* Wear off message for obects */
};

struct  group_type
{
    char *	name;
    sh_int	rating[MAX_CLASS];
    char *	spells[MAX_IN_GROUP];
};

/*
 * MOBprog definitions
 */                   
#define TRIG_ACT	(A)
#define TRIG_BRIBE	(B)
#define TRIG_DEATH	(C)
#define TRIG_ENTRY	(D)
#define TRIG_FIGHT	(E)
#define TRIG_GIVE	(F)
#define TRIG_GREET	(G)
#define TRIG_GRALL	(H)
#define TRIG_KILL	(I)
#define TRIG_HPCNT	(J)
#define TRIG_RANDOM	(K)
#define TRIG_SPEECH	(L)
#define TRIG_EXIT	(M)
#define TRIG_EXALL	(N)
#define TRIG_DELAY	(O)
#define TRIG_SURR	(P)

struct mprog_list
{
    int			trig_type;
    char *		trig_phrase;
    unsigned int		vnum;
    char *  		code;
    MPROG_LIST * 	next;
    bool		valid;
};

struct mprog_code
{
    unsigned int vnum;
    bool		changed;
    char *		code;
    MPROG_CODE *	next;
};


/*
 * Utility macros.
 */
#define IS_WEAPON_SKILL(sn)	( (sn) == gsn_arrow || (sn) == gsn_axe || (sn) == gsn_bow  || \
				(sn) == gsn_dagger  || (sn) == gsn_arrow || (sn) == gsn_flail  || (sn) == gsn_staff || \
				(sn) == gsn_lance || (sn) == gsn_mace || (sn) == gsn_polearm || (sn) == gsn_spear || \
				(sn) == gsn_sword || (sn) == gsn_whip )
#define SPHERE_TOTAL(ch)	((ch)->cre_sphere[0] + (ch)->cre_sphere[1] + (ch)->cre_sphere[2] + (ch)->cre_sphere[3] + (ch)->cre_sphere[4])
#define IS_EMOTE(string)        ((string[0] == '*' && strlen(string) > 1 && string[strlen(string)-1] == '*') ? 1 : 0 )
#define IS_VALID(data)		((data) != NULL && (data)->valid)
#define VALIDATE(data)		((data)->valid = TRUE)
#define INVALIDATE(data)	((data)->valid = FALSE)
#define UMIN(a, b)		((a) < (b) ? (a) : (b))
#define UMAX(a, b)		((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)		((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)		((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)		((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#define IS_SET(flag, bit)	((flag) & (bit))
#define SET_BIT(var, bit)	((var) |= (bit))
#define IS_WATER( var )         (((var)->sector_type == SECT_WATER_SWIM) ||\
		((var)->sector_type == SECT_WATER_NOSWIM))

#define REMOVE_BIT(var, bit)	((var) &= ~(bit))
#define TOGGLE_BIT(var, bit)    ((var) ^= (bit))
#define IS_NULLSTR(str)		((str) == NULL || (str)[0] == '\0')
#define CHECKNULLSTR(str) ( (str) == NULL ? "" : (str) )
#define CH(d)		((d)->original ? (d)->original : (d)->character )
#define ENTRE(min,num,max)	( ((min) < (num)) && ((num) < (max)) )
#define ENTRE_I(x,y,z)	(((x) <= (y)) && ((y) <= (z)))
#define CHECK_POS(a, b, c)	{					\
					(a) = (b);			\
					if ( (a) < 0 )			\
					bug( "CHECK_POS : " c " == %d < 0", a );\
				}

#define ARRAY_COPY( array1, array2, length )				\
		{							\
			int _xxx_;					\
			for ( _xxx_ = 0; _xxx_ < length; _xxx_++ )	\
				array1[_xxx_] = array2[_xxx_];		\
		}

/*
 * Granted macros
 */
bool is_fade_granted args( (CHAR_DATA *ch) );
#define IS_FADE_GRANTED(ch) (is_fade_granted(ch))

/*
 * Character macros.
 */
#define IS_BLOCKING(ch) (ch->exit_block.blocking)
#define IS_DR(ch) (IS_SET(ch->ic_flags, IC_DRAGON_REBORN))
#define IS_TAVEREN(ch) (IS_SET(ch->ic_flags, IC_TAVEREN))
#define IS_SPECIAL(ch) (IS_SET(ch->ic_flags, IC_SPECIAL))
#define IS_RP(ch) (IS_SET(ch->ic_flags, IC_RP))
#define IS_BOTH_RP(ch, vch) (IS_RP(ch) && IS_RP(vch))
#define IS_GUARDING(ch) (!IS_NPC(ch) ? ch->pcdata->guarding != NULL : FALSE)
#define IS_GUARDED(ch) (!IS_NPC(ch) ? ch->pcdata->guarded_by != NULL : FALSE) 
#define IS_LINKED(ch) (ch->bIsLinked)
#define IS_WARDER(ch) (IS_SET(ch->merits, MERIT_WARDER))
#define IS_DISGUISED(ch) (IS_SET(ch->app, APP_DISGUISED))
#define IS_ILLUSIONAPP(ch) (IS_SET(ch->app, APP_ILLUSION))
#define IS_GIANTILLUSION(ch) (IS_SET(ch->app, APP_SIZE_ILLUSION))
#define IS_FORSAKEN(ch) (IS_SET(ch->ic_flags, IC_FORSAKEN))
#define IS_WOLFSHAPE(ch) (IS_SET(ch->app, APP_WOLFSHAPE) && IS_SET(ch->act2, PLR2_WOLFFORM))
#define IS_SUFFOCATING(ch) (IS_SET(ch->affected_by, AFF_SUFFOCATING))
#define IS_HOODED(ch)   (IS_SET(ch->app, APP_HOODED) && !IS_NULLSTR(ch->pcdata->hood_appearance))
#define IS_VEILED(ch)   (IS_SET(ch->app, APP_VEILED) && !IS_NULLSTR(ch->pcdata->veil_appearance))
#define IS_CLOAKED(ch)  (IS_SET(ch->app, APP_CLOAKED))
#define IS_COLORCLOAKED(ch) (IS_SET(ch->app, APP_COLORCLOAKED))
#define IS_MASQUERADED(ch) (IS_SET(ch->app, APP_MASQUERADE))
#define IS_SKIMMING(ch) (IS_SET(ch->world, WORLD_SKIMMING))
#define IS_DREAMING(ch) (IS_SET(ch->world,  WORLD_TAR_DREAM))
#define IS_DREAMWALKING(ch) (IS_SET(ch->world,  WORLD_TAR_FLESH))
#define IS_SAME_WORLD(ch, vch) ((ch->world == vch->world) || \
						  (IS_SET(ch->world,  WORLD_TAR_FLESH) && IS_SET(vch->world, WORLD_TAR_DREAM)) || \
						  (IS_SET(vch->world, WORLD_TAR_FLESH) && IS_SET(ch->world,  WORLD_TAR_DREAM)))

// Races macros	
#define IS_SHADOWSPAWN(ch) ( ch->race == race_lookup("trolloc") || ch->race == race_lookup("fade"))
#define IS_FADE(ch) (ch->race == race_lookup("fade"))
#define IS_TROLLOC(ch) (ch->race == race_lookup("trolloc"))
#define IS_AIEL(ch) (ch->race == race_lookup("aiel"))
#define IS_GHOLAM(ch) (ch->race == race_lookup("gholam"))
#define IS_TARABONER(ch) (ch->race == race_lookup("taraboner"))

#define IS_WOLFKIN(ch)  (IS_SET(ch->talents, TALENT_WOLFKIN) && ch->level >= 20 && IS_SET(ch->affected_by, AFF_INFRARED))

#define IS_CODER(ch)    (!str_cmp (ch->name, "Swordfish") || !str_cmp (ch->name, "Zandor") || !str_cmp (ch->name, "Atwain") || !str_cmp(ch->name, "Basyn"))

#define CAN_CHANNEL(ch)  ((get_skill(ch, gsn_seize) != 0) || (get_skill(ch, gsn_embrace) != 0))
#define CAN_DREAM(ch) ((get_skill(ch,find_spell(ch,"dreamgate")) >= 150 ) || (get_skill(ch,gsn_dream) > 0) || IS_IMMORTAL(ch))

#define IS_CHANNELING(ch) (IS_AFFECTED(ch ,AFF_CHANNELING))

#define IS_NPC(ch)		(IS_SET((ch)->act, ACT_IS_NPC))
#define IS_ADMIN(ch)		(get_trust(ch) >= LEVEL_ADMIN)
#define IS_IMMORTAL(ch)		(get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_HERO(ch)		(get_trust(ch) >= LEVEL_HERO)
#define IS_NEWBIE(ch)           (ch->level <= LEVEL_NEWBIE)
#define IS_NEWBIEHELPER(ch)     (IS_SET(ch->act, PLR_IS_NEWBIEHELPER))
#define IS_TRUSTED(ch,level)	(get_trust((ch)) >= (level))
#define IS_AFFECTED(ch, sn)	(IS_SET((ch)->affected_by, (sn)))
#define IS_WARDED(room, sn)     (is_ward_set((room), (sn)))
#define IS_RAFFECTED(room, sn)  (is_room_weave_set((room), (sn)))

#define GET_AGE(ch)		((int) (17 + ((ch)->played \
				    + current_time - (ch)->logon )/72000))

#define IS_GOOD(ch)		(ch->alignment >= 350)
#define IS_EVIL(ch)		(ch->alignment <= -350)
#define IS_NEUTRAL(ch)		(!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_SLEEPING(ch)       (ch->position == POS_SLEEPING)
#define IS_AWAKE(ch)		(ch->position > POS_SLEEPING)
#define GET_AC(ch,type)		((ch)->armor[type]			    \
		        + ( IS_AWAKE(ch)			    \
			? dex_app[get_curr_stat(ch,STAT_DEX)].defensive : 0 ))  
#define GET_HITROLL(ch)	\
		((ch)->hitroll+str_app[get_curr_stat(ch,STAT_STR)].tohit)
#define GET_DAMROLL(ch) \
		((ch)->damroll+str_app[get_curr_stat(ch,STAT_STR)].todam)

#define IS_OUTSIDE(ch)		(!IS_SET(				    \
				    (ch)->in_room->room_flags,		    \
				    ROOM_INDOORS) &&   \
				    !(ch)->in_room->sector_type == SECT_INSIDE)
#define CAN_SEE_OUTSIDE(ch)  (IS_OUTSIDE(ch) && (!(ch)->in_obj || \
                              (IS_SET(ch->in_obj->value[1], CONT_SEE_OUT) \
                              || !IS_SET(ch->in_obj->value[1], CONT_CLOSED))))
#define IS_INOBJECT(ch)      (ch->in_obj != NULL )

#define WAIT_STATE(ch, npulse)	((ch)->wait = UMAX((ch)->wait, (npulse)))
#define DAZE_STATE(ch, npulse)  ((ch)->daze = UMAX((ch)->daze, (npulse)))
#define get_carry_weight(ch)	((ch)->carry_weight + (ch)->silver/10 +  \
						      (ch)->gold * 2 / 5)

#define MOUNTED(ch)	((!IS_NPC(ch) && ch->mount && ch->riding) ? \
				ch->mount : NULL)
#define RIDDEN(ch)	((IS_NPC(ch) && ch->mount && ch->riding) ? \
				ch->mount : NULL)
#define IS_DRUNK(ch)	(IS_NPC(ch)  ? \
			      FALSE : ch->pcdata->condition[COND_DRUNK] > 10)

#define act(format,ch,arg1,arg2,type)\
	act_new((format),(ch),(arg1),(arg2),(type),POS_RESTING)

#define HAS_TRIGGER(ch,trig)	(IS_SET((ch)->pIndexData->mprog_flags,(trig)))
#define IS_SWITCHED( ch )       ( ch->desc && ch->desc->original )
#define IS_BUILDER(ch, Area)	( !IS_NPC(ch) && !IS_SWITCHED( ch ) &&	  \
				( ch->pcdata->security >= Area->security  \
				|| strstr( Area->builders, ch->name )	  \
				|| strstr( Area->builders, "All" ) ) )


#define IS_FACELESS(ch)		(IS_SET(ch->act2,PLR2_FACELESS))
#define IS_PKILLER(ch)		(IS_SET(ch->act2,PLR2_PKILLER))
/* Guild.c */
#define EDIT_GUILD(Ch, Clan)    ( Clan = Ch->desc->pEdit )

#define EDIT_SGUILD(Ch, sguild)  ( sguild = Ch->desc->pEdit )

#define EDIT_SSGUILD(Ch, ssguild)  ( ssguild = Ch->desc->pEdit )
 


/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)	(IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, stat)	(IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj,stat)(IS_SET((obj)->value[4],(stat)))
#define WEIGHT_MULT(obj)	((obj)->item_type == ITEM_CONTAINER ? \
	(obj)->value[4] : 100)



/*
 * Description macros.
 */
/*
#define PERS_OLD(ch, looker)	( can_see( looker, (ch) ) ?		\
				( IS_NPC(ch) ? (ch)->short_descr	\
				: (ch)->short_descr[0] ? ch->short_descr : \
                                  (ch)->name ) : "Someone" )
*/
#define PERS_OLD(ch, looker)    ( get_trust(looker) >= ch->invis_level ? \
                                ( IS_NPC(ch) ? (ch)->short_descr        \
                                : (ch)->short_descr[0] ? ch->short_descr : \
                                  (ch)->name ) : "Someone" )

#define COLORNAME(ch)           ( (ch)->short_descr[0] ? ch->short_descr : \
                                  (ch)->name )
/*
 * Structure for a social in the socials table.
 */
struct	social_type
{
    char *	name;
    char *    char_no_arg;
    char *    others_no_arg;
    char *    char_found;
    char *    others_found;
    char *    vict_found;
    char *    char_not_found;
    char *      char_auto;
    char *      others_auto;
};



/*
 * Global constants.
 */
extern	const	struct	str_app_type	str_app		[31];
extern	const	struct	int_app_type	int_app		[31];
extern	const	struct	wis_app_type	wis_app		[31];
extern	const	struct	dex_app_type	dex_app		[31];
extern	const	struct	con_app_type	con_app		[31];

extern    const     struct    sphere_type    sphere_table   [];
extern	const	struct	class_type	class_table	[MAX_CLASS];
extern	const	struct	weapon_type	weapon_table	[];
extern  const   struct  item_type	item_table	[];
extern	const	struct	wiznet_type	wiznet_table	[];
extern	const	struct	attack_type	attack_table	[];
#if defined(FIRST_BOOT)
extern  const	struct  race_type	race_table	[];
extern	const	struct	skill_type	skill_table	[MAX_SKILL];
extern          struct social_type      social_table	[MAX_SOCIALS];
extern  const   struct  group_type      group_table	[MAX_GROUP];
extern	const	struct	pc_race_type	pc_race_table	[];
#else
extern		struct	race_type *	race_table;
extern		struct	skill_type *	skill_table;
extern		struct	social_type *	social_table;
extern		struct	group_type *	group_table;
#endif
extern  const	struct	spec_type	spec_table	[];
extern	const	struct	liq_type	liq_table	[];
extern	char *	const			title_table	[MAX_CLASS]
							[MAX_LEVEL+1]
							[2];



/*
 * Global variables.
 */
extern		HELP_DATA	  *	help_first;
extern		SHOP_DATA	  *	shop_first;

extern		CHAR_DATA	  *	char_list;
extern		DESCRIPTOR_DATA   *	descriptor_list;
extern		OBJ_DATA	  *	object_list;

extern		MPROG_CODE	  *	mprog_list;

extern		char			bug_buf		[];
extern		time_t			current_time;
extern		bool			fLogAll;
extern		FILE *			fpReserve;
extern		KILL_DATA		kill_table	[];
extern		char			log_buf		[];
extern		TIME_INFO_DATA		time_info;
extern          char                    last_command[MAX_STRING_LENGTH+20];
extern		WEATHER_DATA		weather_info;
extern		bool			MOBtrigger;
extern          NOTE_DATA         *     note_free;
extern          OBJ_DATA          *     obj_free;
extern 		PKINFO_TYPE	  *     pkranks;

/* Global variables. */
extern HISTORY_DATA chat_history[HISTSIZE];
extern HISTORY_DATA game_history[HISTSIZE];
extern HISTORY_DATA guildleader_history[HISTSIZE];
extern HISTORY_DATA imm_history[HISTSIZE];
extern HISTORY_DATA pray_history[PRAYHISTSIZE];
extern HISTORY_DATA gossip_history[HISTSIZE];
extern HISTORY_DATA newbie_history[HISTSIZE];
extern HISTORY_DATA guild_history[MAX_CLAN][HISTSIZE];
extern bool g_plevelFlag;
extern bool g_extraplevelFlag;


//For keepeer objects
extern unsigned int g_CurrentKeeper;
extern bool g_FreezeWeather;


/*
 * OS-dependent declarations.
 * These are all very standard library functions,
 *   but some systems have incomplete or non-ansi header files.
 */
#if	defined(_AIX)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(apollo)
int	atoi		args( ( const char *string ) );
void *	calloc		args( ( unsigned nelem, size_t size ) );
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(hpux)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(linux)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(macintosh)
#define NOCRYPT
#if	defined(unix)
#undef	unix
#endif
#endif

#if	defined(MIPS_OS)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(MSDOS)
#define NOCRYPT
#if	defined(unix)
#undef	unix
#endif
#endif

#if	defined(NeXT)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif

#if	defined(sequent)
char *	crypt		args( ( const char *key, const char *salt ) );
int	fclose		args( ( FILE *stream ) );
int	fprintf		args( ( FILE *stream, const char *format, ... ) );
int	fread		args( ( void *ptr, int size, int n, FILE *stream ) );
int	fseek		args( ( FILE *stream, long offset, int ptrname ) );
void	perror		args( ( const char *s ) );
int	ungetc		args( ( int c, FILE *stream ) );
#endif

#if	defined(sun)
char *	crypt		args( ( const char *key, const char *salt ) );
int	fclose		args( ( FILE *stream ) );
int	fprintf		args( ( FILE *stream, const char *format, ... ) );
#if	defined(SYSV)
siz_t	fread		args( ( void *ptr, size_t size, size_t n, 
			    FILE *stream) );
#elif !defined(__SVR4)
int	fread		args( ( void *ptr, int size, int n, FILE *stream ) );
#endif
int	fseek		args( ( FILE *stream, long offset, int ptrname ) );
void	perror		args( ( const char *s ) );
int	ungetc		args( ( int c, FILE *stream ) );
#endif

#if	defined(ultrix)
char *	crypt		args( ( const char *key, const char *salt ) );
#endif



/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#if	defined(NOCRYPT)
#define crypt(s1, s2)	(s1)
#endif

extern int port;
extern int control;


/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */
#if defined(macintosh)
#define PLAYER_DIR	""			/* Player files	*/
#define TEMP_FILE	"romtmp"
#define NULL_FILE	"proto.are"		/* To reserve one stream */
#endif

#if defined(MSDOS)
#define PLAYER_DIR	""			/* Player files */
#define TEMP_FILE	"romtmp"
#define NULL_FILE	"nul"			/* To reserve one stream */
#endif

#if defined(unix)
#define PLAYER_DIR      "../player/"        	 /* Player files */
#define PLAYER_BACKUP_DIR "../player/backup/" /* Player backup directory */
#define PLAYER_DISGUISE_DIR "../disguise/"    /* Plauer disguise directory */
#define GOD_DIR         "../gods/"  		 /* list of gods */
#define TEMP_FILE	"../player/romtmp"        /* Temp */
#define NULL_FILE	"/dev/null"               /* To reserve one stream */
#endif

#define LAST_COMMAND    "../last_command.txt"  /*For the signal handler.*/
#define AREA_LIST       "area.lst"  /* List of areas*/
#define AREA_DIR	"../area/"
#define DATA_DIR	"../data/"
#define PROG_DIR	"../data/progs/"
#define LOG_DIR    	 "../log/"
#define LOG_RP_DIR    	 "../logged_rp/current/"
#define PKRANK_FILE	"pkrank.txt"
#define BUG_FILE        "bugs.txt"     /* For 'bug' and bug()*/
#define TYPO_FILE       "typos.txt"    /* For 'typo'*/
#define SHUTDOWN_FILE   "shutdown.txt" /* For 'shutdown'*/
#define GUILD_FILE      "guilds.dat"
#define SGUILD_FILE     "sguilds.dat"
#define SSGUILD_FILE    "ssguilds.dat"
#define BAN_FILE	"ban.txt"
#define MUSIC_FILE	"music.txt"
#define DISABLED_FILE	"disabled.txt"  /* disabled commands */
#define HELP_FILE       "help.txt"       /* For undefined helps */
#define COUNTER_FILE    "counter.dat"    /* for persistent counters */
#define WEB_DIR     "../web/"

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD	CHAR_DATA
#define MID	MOB_INDEX_DATA
#define OD	OBJ_DATA
#define OID	OBJ_INDEX_DATA
#define RID	ROOM_INDEX_DATA
#define SF	SPEC_FUN
#define AD	AFFECT_DATA
#define MPC	MPROG_CODE

/* hunt.c */
int find_path( unsigned int in_room_vnum, unsigned int out_room_vnum, CHAR_DATA *ch, 
	       int depth, int in_zone );
/* act_comm.c */
char *emote_parse   	args( ( CHAR_DATA *ch,char *argument ) );
void    reward_rp	args( ( CHAR_DATA *ch ) );
void  	check_sex	args( ( CHAR_DATA *ch ) );
void	add_follower	args( ( CHAR_DATA *ch, CHAR_DATA *master ) );
void	stop_follower	args( ( CHAR_DATA *ch ) );
void 	nuke_pets	args( ( CHAR_DATA *ch ) );
void	die_follower	args( ( CHAR_DATA *ch ) );
bool	is_same_group	args( ( CHAR_DATA *ach, CHAR_DATA *bch ) );
void 	_logf 		args( ( char * fmt, ...));
bool	is_bonded_to	args( ( CHAR_DATA *ach, CHAR_DATA *bch ) );
void	do_bondfind	args( ( CHAR_DATA *ch, char *argument ) );
void     do_hack_look (CHAR_DATA * ch, char *argument);
void log_comm_string( char * category, const char *str );

/* masterforms.c */
void initmasterformslookup();
 bool check_mf(CHAR_DATA *ch, sh_int gsn);
int getmfskill(CHAR_DATA * ch);
char * get_master_defend(sh_int masterform, int index);
char * get_master_attack(sh_int masterform, int index);
sh_int find_relevant_masterform(CHAR_DATA *ch);
char * get_master_extended_attack(CHAR_DATA *ch, int index);
bool is_masterform(sh_int gsn);
bool char_knows_masterform(CHAR_DATA *ch);
unsigned int get_mf_weapon(sh_int gsn_mf);

 

/* quest.c */
extern void quest_update	args(( void ));
bool has_exits args( (ROOM_INDEX_DATA *room));

/* darkfriend.c */
int compare_dfranks(const void *vi, const void *vw);
bool can_dfpromot(CHAR_DATA *ch);
void set_dftitle( CHAR_DATA *ch, char *dfname );
bool can_dfpromote(CHAR_DATA *ch);

/* act_enter.c */
RID  *find_city         args ( (CHAR_DATA *ch, char *city_name) );
RID  *get_random_room   args ( (CHAR_DATA *ch, bool use_vmap) );

/* hedit.c */
char *color2web args( (char *text) );

/* act_info.c */
bool can_use_masquerade args((CHAR_DATA *ch));
void lore_item args( (CHAR_DATA *ch, OBJ_DATA *obj) );
bool vote_click_check args( (CHAR_DATA *ch) );
void do_webwho      args ( () );
bool check_blind    args( ( CHAR_DATA *ch ) );
void	set_title	     args( ( CHAR_DATA *ch, char *title ) );
void set_imminfo    args( ( CHAR_DATA *ch, char *title ) );
char *indefinite    args( ( char *str ) );
void set_appearance args( ( CHAR_DATA *ch, char *app ) );
void set_dfname args( ( CHAR_DATA *ch, char *app ) );
int   colorstrlen   args( (char *argument ) );
int   compare_char_names args( (const void *v1, const void *v2) );
void  add_know      args( ( CHAR_DATA *ch, long id, char *name ) );
char *PERS          args( ( CHAR_DATA *ch, CHAR_DATA *looker) );
char *PERS_NAME     args( ( CHAR_DATA *ch, CHAR_DATA *looker) );
bool IS_INTRONAME   args( ( CHAR_DATA *ch, char *argument ) );
int  key2vnum       args( (CHAR_DATA *ch, char *asciikey)        );
char *vnum2key      args( (CHAR_DATA *ch, int vnum)         );
char * getkey       args( (CHAR_DATA *ch, char*argument)    );

/* act_move.c */
void	move_char	args( ( CHAR_DATA *ch, int door, bool follow ) );
void    do_mount        args( ( CHAR_DATA *ch, char *argument));
CHAR_DATA *get_blocker  args( (ROOM_INDEX_DATA *loc, int direction) );
void stop_exit_block    args( (CHAR_DATA *ch) );
/* act_obj.c */
bool can_loot		args( (CHAR_DATA *ch, OBJ_DATA *obj) );
void	wear_obj	args( (CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, char * location) );
void    get_obj         args( ( CHAR_DATA *ch, OBJ_DATA *obj,
                            OBJ_DATA *container ) );

/* act_wiz.c */
ROOM_INDEX_DATA *	find_location	args( ( CHAR_DATA *ch, char *arg ) );
void wiznet		args( (char *string, CHAR_DATA *ch, OBJ_DATA *obj,
			       long flag, long flag_skip, int min_level ) );
void copyover_recover args((void));
void do_warmboot args((CHAR_DATA *ch, char * argument));
void do_copyinboot args((CHAR_DATA *ch, char * argument));
char *colorstrem(char *argument);
			       
/* alias.c */
void 	substitute_alias args( (DESCRIPTOR_DATA *d, char *input) );

/* ban.c */
bool	check_ban	args( ( char *site, int type) );

/* bit.c */

/* comm.c */
char *get_hit_loc_col args((CHAR_DATA *ch, CHAR_DATA *victim, int location));
char * get_hit_loc_str args((CHAR_DATA *ch, CHAR_DATA *victim));
char *get_hit_loc_wound_str args((CHAR_DATA *victim, int location));
void	show_string	args( ( struct descriptor_data *d, char *input) );
void	close_socket	args( ( DESCRIPTOR_DATA *dclose ) );
void	write_to_buffer	args( ( DESCRIPTOR_DATA *d, const char *txt,
			    int length ) );
void	send_to_char	args( ( const char *txt, CHAR_DATA *ch ) );
void	page_to_char	args( ( const char *txt, CHAR_DATA *ch ) );
void	act		args( ( const char *format, CHAR_DATA *ch,
				   const void *arg1, const void *arg2, int type ) );
void	act_new		args( ( const char *format, CHAR_DATA *ch, 
					   const void *arg1, const void *arg2, int type,
					   int min_pos) );
void	act_old		args( ( const char *format, CHAR_DATA *ch, 
					   const void *arg1, const void *arg2, int type,
					   int min_pos) );
void	act_w_intro	args( ( const char *format, CHAR_DATA *ch, 
					   const void *arg1, const void *arg2, int type,
					   int min_pos) );

void	printf_to_char	args( ( CHAR_DATA *, char *, ... ) );
void	bugf		args( ( char *, ... ) );
void	flog		args( ( char *, ... ) );
void send_to_malchan   args( ( const char *txt, CHAR_DATA *ch ) );
void send_to_femalchan args( ( const char *txt, CHAR_DATA *ch ) );
void send_chpower_to_channelers args( (CHAR_DATA *ch, int sn) );

 /*
  * Colour stuff by Lope
  */
int	colour		args( ( char type, CHAR_DATA *ch, char *string ) );
void	colourconv	args( ( char *buffer, const char *txt, CHAR_DATA *ch ) );
void	send_to_char_bw	args( ( const char *txt, CHAR_DATA *ch ) );
void	page_to_char_bw	args( ( const char *txt, CHAR_DATA *ch ) );

/* db.c */
void	load_disabled	args( ( void ) );
void	save_disabled	args( ( void ) );
void	reset_area      args( ( AREA_DATA * pArea ) );		/* OLC */
void	reset_room	args( ( ROOM_INDEX_DATA *pRoom ) );	/* OLC */
char *	print_flags	args( ( int flag ));
void	boot_db		args( ( void ) );
void	area_update	args( ( void ) );
CD *	create_mobile	args( ( MOB_INDEX_DATA *pMobIndex ) );
void	clone_mobile	args( ( CHAR_DATA *parent, CHAR_DATA *clone) );
OD *	create_object	args( ( OBJ_INDEX_DATA *pObjIndex, int level ) );
void	clone_object	args( ( OBJ_DATA *parent, OBJ_DATA *clone ) );
void	clear_char	args( ( CHAR_DATA *ch ) );
char *	get_extra_descr	args( ( const char *name, EXTRA_DESCR_DATA *ed ) );
MID *	get_mob_index	args( ( unsigned int vnum ) );
OID *	get_obj_index	args( ( unsigned int vnum ) );
RID *	get_room_index	args( ( unsigned int vnum ) );
MPC *	get_mprog_index args( ( unsigned int vnum ) );
char	fread_letter	args( ( FILE *fp ) );
int	fread_number	args( ( FILE *fp ) );
long 	fread_flag	args( ( FILE *fp ) );
char *	fread_string	args( ( FILE *fp ) );
char *  fread_string_eol args(( FILE *fp ) );
void	fread_to_eol	args( ( FILE *fp ) );
char *	fread_word	args( ( FILE *fp ) );
long	flag_convert	args( ( char letter) );
void *	alloc_mem	args( ( int sMem ) );
void *	alloc_perm	args( ( int sMem ) );
void	free_mem	args( ( void *pMem, int sMem ) );
char *	str_dup		args( ( const char *str ) );
void	free_string	args( ( char *pstr ) );
int	number_fuzzy	args( ( int number ) );
bool    number_chance   args( ( int num    ) );
int	number_range	args( ( int from, int to ) );
int	number_percent	args( ( void ) );
int	number_door	args( ( void ) );
int	number_bits	args( ( int width ) );
long     number_mm       args( ( void ) );
int	dice		args( ( int number, int size ) );
int	interpolate	args( ( int level, int value_00, int value_32 ) );
void	smash_tilde	args( ( char *str ) );
bool	str_cmp		args( ( const char *astr, const char *bstr ) );
bool	str_prefix	args( ( const char *astr, const char *bstr ) );
bool	str_infix	args( ( const char *astr, const char *bstr ) );
bool	str_suffix	args( ( const char *astr, const char *bstr ) );
char *	capitalize	args( ( const char *str ) );
void	append_file	args( ( CHAR_DATA *ch, char *file, char *str ) );
void	bug		args( ( const char *str, int param ) );
void	log_string	args( ( const char *str ) );
void	log_rp_string	args( ( CHAR_DATA *ch, const char *str ) );
void	tail_chain	args( ( void ) );
OBJ_DATA * create_ore   args( (int nType, int nQuality) );
OBJ_DATA * create_gem   args( (int nType, int nQuality) );

/* effect.c */
void	acid_effect	args( (void *vo, int level, int dam, int target) );
void	cold_effect	args( (void *vo, int level, int dam, int target) );
void	fire_effect	args( (void *vo, int level, int dam, int target) );
void	poison_effect	args( (void *vo, int level, int dam, int target) );
void	shock_effect	args( (void *vo, int level, int dam, int target) );

/* act_obj.c */
void save_vehicle(CHAR_DATA * ch, OBJ_DATA * obj);
void save_keeper(CHAR_DATA * ch, OBJ_DATA * obj);

/* fight.c */
bool    check_valid_pkill args( (CHAR_DATA *ch, CHAR_DATA *victim) );
void    check_trolloc_kill args( (CHAR_DATA *ch, CHAR_DATA *victim) );
void    remove_guard    args ( (CHAR_DATA *ch, bool msg) );
bool 	is_safe		args( (CHAR_DATA *ch, CHAR_DATA *victim ) );
bool 	is_safe_spell	args( (CHAR_DATA *ch, CHAR_DATA *victim, bool area ) );
void	violence_update	args( ( void ) );
void	multi_hit	args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
bool	damage		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
			        int dt, int class, bool show ) );
bool    damage_old      args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
                                int dt, int class, bool show ) );
void	update_pos	args( ( CHAR_DATA *victim ) );
void	stop_fighting	args( ( CHAR_DATA *ch, bool fBoth ) );
void	check_killer	args( ( CHAR_DATA *ch, CHAR_DATA *victim) );
void raw_kill       args( ( CHAR_DATA *victim ) );
void group_gain     args( ( CHAR_DATA *ch, CHAR_DATA *victim, bool bAssassinated) );
int xp_compute      args( ( CHAR_DATA *gch, CHAR_DATA *victim, int total_levels ) );
void death_cry      args( ( CHAR_DATA *ch ) );
int get_skill_difference args( (CHAR_DATA *ch, CHAR_DATA *victim, int gsn) );

/* guild.c */
struct	clan_type	clan_table[MAX_CLAN];
char *  guild_bit_name  args( ( int guild_flags ) );
bool    is_clan         args( (CHAR_DATA *ch) );
bool    is_same_clan    args( (CHAR_DATA *ch, CHAR_DATA *victim) );
int 	clan_lookup	args( (const char *name) );
void 	do_guild	args( (CHAR_DATA *ch, char *argument ) );
void	do_promote	args( (CHAR_DATA *ch, char *argument) );
char 	*player_rank	args( (CHAR_DATA *ch) );
char	*player_clan	args( (CHAR_DATA *ch) );
bool	can_guild	args( (CHAR_DATA *ch) );
bool    can_deguild     args( (CHAR_DATA *ch) );
bool    can_promote     args( (CHAR_DATA *ch) );
bool    can_demote      args( (CHAR_DATA *ch) );
bool is_guild_guard     args( (CHAR_DATA *guard, CHAR_DATA *ch) );
bool is_in_guild_group  args( (CHAR_DATA *guard, CHAR_DATA *ch) );
int compare_ranks       args( (const void *v1, const void *v2) );
bool is_leader          args( (CHAR_DATA * ch) );

/* oguild.c */
bool    is_oguild         args( (CHAR_DATA *ch) );
bool    is_same_oguild    args( (CHAR_DATA *ch, CHAR_DATA *victim) );
void 	do_oguild	args( (CHAR_DATA *ch, char *argument ) );
void	do_oguild_promote	args( (CHAR_DATA *ch, char *argument) );
char 	*player_oguild_rank	args( (CHAR_DATA *ch) );
char	*player_oguild	args( (CHAR_DATA *ch) );
int compare_cross_ranks       args( (const void *v1, const void *v2) );

/* sguild.c */
struct  sguild_type sguild_table[MAX_CLAN];
char *  sguild_bit_name  args( ( int sguild_flags ) );
bool    is_sguild         args( (CHAR_DATA *ch) );
bool    is_same_sguild    args( (CHAR_DATA *ch, CHAR_DATA *victim) );
int 	sguild_lookup	args( (const char *name) );
void 	do_sguild	args( (CHAR_DATA *ch, char *argument ) );
void	do_sguild_promote	args( (CHAR_DATA *ch, char *argument) );
char 	*player_sguild_rank	args( (CHAR_DATA *ch) );
char	*player_sguild	args( (CHAR_DATA *ch) );
bool	can_sguild	args( (CHAR_DATA *ch) );
bool    can_desguild     args( (CHAR_DATA *ch) );
bool    can_sguild_promote     args( (CHAR_DATA *ch) );
bool    can_sguild_demote      args( (CHAR_DATA *ch) );
void    sguild_tr_promote      args( (CHAR_DATA * ch, bool promote, bool pc_kill_promote) );

/* ssguild.c */
struct  ssguild_type ssguild_table[MAX_CLAN];
char *  ssguild_bit_name  args( ( int ssguild_flags ) );
bool    is_ssguild         args( (CHAR_DATA *ch) );
bool    is_same_ssguild    args( (CHAR_DATA *ch, CHAR_DATA *victim) );
int 	ssguild_lookup	args( (const char *name) );
void 	do_ssguild	args( (CHAR_DATA *ch, char *argument ) );
void	do_ssguild_promote	args( (CHAR_DATA *ch, char *argument) );
char 	*player_ssguild_rank	args( (CHAR_DATA *ch) );
char	*player_ssguild	args( (CHAR_DATA *ch) );
bool	can_ssguild	args( (CHAR_DATA *ch) );
bool    can_dessguild     args( (CHAR_DATA *ch) );
bool    can_ssguild_promote     args( (CHAR_DATA *ch) );
bool    can_ssguild_demote      args( (CHAR_DATA *ch) );

/* handler.c */
int get_max_hit_loc args( (CHAR_DATA *ch, int location) );
int get_level args((CHAR_DATA *ch));
AD  	*affect_find args( (AFFECT_DATA *paf, int sn));
void	affect_check	args( (CHAR_DATA *ch, int where, int vector) );
int	count_users	args( (OBJ_DATA *obj) );
void 	deduct_cost	args( (CHAR_DATA *ch, int cost) );
void	affect_enchant	args( (OBJ_DATA *obj) );
int 	check_immune	args( (CHAR_DATA *ch, int dam_type) );
int 	material_lookup args( ( const char *name) );
int	weapon_lookup	args( ( const char *name) );
int	weapon_type	args( ( const char *name) );
char 	*weapon_name	args( ( int weapon_Type) );
char	*item_name	args( ( int item_type) ); 
int	attack_lookup	args( ( const char *name) );
long	wiznet_lookup	args( ( const char *name) );
int	class_lookup	args( ( const char *name) );
bool	is_old_mob	args ( (CHAR_DATA *ch) );
int	get_skill	args( ( CHAR_DATA *ch, int sn ) );
int	get_weapon_sn	args( ( CHAR_DATA *ch ) );

int	get_weapon_skill args(( CHAR_DATA *ch, int sn ) );
int     get_age         args( ( CHAR_DATA *ch ) );
void	reset_char	args( ( CHAR_DATA *ch )  );
int	get_trust	args( ( CHAR_DATA *ch ) );
int	get_curr_stat	args( ( CHAR_DATA *ch, int stat ) );
int  get_tot_stat args( ( CHAR_DATA *ch) );
unsigned long get_curr_op args( ( CHAR_DATA *ch) );
int  get_curr_flows args( ( CHAR_DATA *ch) );
int 	get_max_train	args( ( CHAR_DATA *ch, int stat ) );
int	can_carry_n	args( ( CHAR_DATA *ch ) );
int	can_carry_w	args( ( CHAR_DATA *ch ) );
bool	is_name		args( ( char *str, char *namelist ) );
bool    is_full_name    args( ( const char *str, char *namelist ) );
bool	is_exact_name	args( ( char *str, char *namelist ) );
void    ward_to_room args( (ROOM_INDEX_DATA *room, WARD_DATA *wd) );
void    weave_to_room args( (ROOM_INDEX_DATA *room, AFFECT_DATA *wd) );
void    residue_to_room args( (ROOM_INDEX_DATA *room, RESIDUE_DATA *rd) );
void	affect_to_char	args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void	affect_to_obj	args( ( OBJ_DATA *obj, AFFECT_DATA *paf ) );
void    ward_remove  args( (ROOM_INDEX_DATA *Room, WARD_DATA *Ward) );
bool    is_ward_set  args ( ( ROOM_INDEX_DATA *room, long ward_flag ) );
void    room_weave_remove  args( (ROOM_INDEX_DATA *Room, AFFECT_DATA *Ward) );
bool    is_room_weave_set  args ( ( ROOM_INDEX_DATA *room, long weave_flag ) );
void    residue_remove  args( (ROOM_INDEX_DATA *Room, RESIDUE_DATA *Residue) );
void	affect_remove	args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void	affect_remove_obj args( (OBJ_DATA *obj, AFFECT_DATA *paf ) );
void	affect_strip	args( ( CHAR_DATA *ch, int sn ) );
bool	is_affected	args( ( CHAR_DATA *ch, int sn ) );
void	affect_join	args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
char    * name_list     args( ( CHAR_IN_DATA *who_in, CHAR_DATA *to));
char    * obj_list     args( ( OBJ_DATA *obj, CHAR_DATA *to));
void	char_from_room	args( ( CHAR_DATA *ch ) );
void	char_to_room	args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex ) );
void	obj_to_char	args( ( OBJ_DATA *obj, CHAR_DATA *ch ) );
void	obj_from_char	args( ( OBJ_DATA *obj ) );
int	apply_ac	args( ( OBJ_DATA *obj, int iWear, int type ) );
OD *	get_eq_char	args( ( CHAR_DATA *ch, int iWear ) );
void	equip_char	args( ( CHAR_DATA *ch, OBJ_DATA *obj, int iWear ) );
void	unequip_char	args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
int	count_obj_list	args( ( OBJ_INDEX_DATA *obj, OBJ_DATA *list ) );
void	obj_from_room	args( ( OBJ_DATA *obj ) );
void	obj_to_room	args( ( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex ) );
void	obj_to_obj	args( ( OBJ_DATA *obj, OBJ_DATA *obj_to ) );
void	obj_from_obj	args( ( OBJ_DATA *obj ) );
void	char_to_obj	args( ( CHAR_DATA *ch, OBJ_DATA *obj_to ) );
void	char_from_obj	args( ( CHAR_DATA *ch ) );
int	put_char_in_obj	args( ( CHAR_DATA *ch, OBJ_DATA *iobj ) );
void	remove_char_from_obj	args( ( CHAR_DATA *ch) );
void	extract_obj	args( ( OBJ_DATA *obj ) );
void	extract_char    args( ( CHAR_DATA *ch, bool fPull, bool fKeepConnection));
CD *	find_char	args( ( CHAR_DATA *ch, char *argument, int door, int range) );
CD *	get_char_area	args( ( CHAR_DATA *ch, char *argument ) );
CD *	get_char_room	args( ( CHAR_DATA *ch, char *argument ) );
CD *	get_introname_char_room	args( ( CHAR_DATA *ch, char *argument ) );
CD *	get_char_room2	args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *room,char *argument, int *number ) );
CD *	get_char_world	args( ( CHAR_DATA *ch, char *argument ) );
CD *	get_char_same_world	args( ( CHAR_DATA *ch, char *argument ) );
CD *	get_char_anywhere args( ( CHAR_DATA *ch, char *argument ) );
CD *	get_introname_char_world	args( ( CHAR_DATA *ch, char *argument ) );
CD *	get_realname_char_world	args( ( CHAR_DATA *ch, char *argument ) );
CD * get_charId_world args( ( CHAR_DATA *ch, long id ) );
OD *	get_obj_type	args( ( OBJ_INDEX_DATA *pObjIndexData, unsigned int vnum ) );
OD *	get_obj_list	args( ( CHAR_DATA *ch, char *argument,
			    OBJ_DATA *list ) );
OD *	get_obj_carry	args( ( CHAR_DATA *ch, char *argument, 
			    CHAR_DATA *viewer ) );
OD *	get_obj_wear	args( ( CHAR_DATA *ch, char *argument ) );
OD *	get_obj_here	args( ( CHAR_DATA *ch, char *argument ) );
OD *	get_obj_world	args( ( CHAR_DATA *ch, char *argument ) );
OD *	create_money	args( ( int gold, int silver ) );
int	get_obj_number	args( ( OBJ_DATA *obj ) );
int	get_obj_weight	args( ( OBJ_DATA *obj ) );
int	get_true_weight	args( ( OBJ_DATA *obj ) );
bool	room_is_dark	args( ( ROOM_INDEX_DATA *pRoomIndex ) );
bool	is_room_owner	args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *room) );
bool	room_is_private	args( ( ROOM_INDEX_DATA *pRoomIndex ) );
bool	can_see		args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	can_see_channel	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	can_see_obj	args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
bool	can_see_room	args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex) );
bool	can_drop_obj	args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
char *	affect_loc_name	args( ( int location ) );
char *	affect_bit_name	args( ( int vector ) );
char *	extra_bit_name	args( ( int extra_flags ) );
char * 	wear_bit_name	args( ( int wear_flags ) );
char *	act_bit_name	args( ( int act_flags ) );
char *	off_bit_name	args( ( int off_flags ) );
char *  imm_bit_name	args( ( int imm_flags ) );
char * 	form_bit_name	args( ( int form_flags ) );
char *	part_bit_name	args( ( int part_flags ) );
char *	weapon_bit_name	args( ( int weapon_flags ) );
char *  comm_bit_name	args( ( int comm_flags ) );
char *	cont_bit_name	args( ( int cont_flags) );
char *  item_type_name  args( ( OBJ_DATA * obj) );
char *removechars(char *source, char *charset);
void    reset_dead_character args( (CHAR_DATA *ch, char * argument, bool keep));

/*
  * Colour Config
  */
void	default_colour	args( ( CHAR_DATA *ch ) );
void	all_colour	args( ( CHAR_DATA *ch, char *argument ) );

bool	emptystring	args( ( const char * ) );
char *	itos		args( ( int ) );
int	get_vnum_mob_name_area	args( ( char *, AREA_DATA * ) );
int	get_vnum_obj_name_area	args( ( char *, AREA_DATA * ) );
int	get_points	( int race, int args );

/* interp.c */
void	interpret	args( ( CHAR_DATA *ch, char *argument ) );
bool	is_number	args( ( char *arg ) );
int	number_argument	args( ( char *argument, char *arg ) );
int	mult_argument	args( ( char *argument, char *arg) );
char *	one_argument	args( ( char *argument, char *arg_first ) );

/* magic.c */
int	find_spell	args( ( CHAR_DATA *ch, const char *name) );
int 	endurance_cost 	(CHAR_DATA *ch, int min_endurance, int level);
int	skill_lookup	args( ( const char *name ) );
int	slot_lookup	args( ( int slot ) );
bool	saves_spell	args( ( int level, CHAR_DATA *victim, int dam_type ) );
void	obj_cast_spell	args( ( int sn, int level, CHAR_DATA *ch,
				    CHAR_DATA *victim, OBJ_DATA *obj ) );

/* weaves.c */
bool foxhead_immune     args( (CHAR_DATA *ch, int gsn) ); 
bool is_wearing_foxhead(CHAR_DATA *ch);

/* magic2.c */
void release_sustained_weaves args ( (CHAR_DATA *ch) );
void remove_sustained_weaves  args ( (CHAR_DATA *ch) );
void update_sustained_weaves  args ( (CHAR_DATA *ch) );
void update_rpcounter         args ( (CHAR_DATA *ch) );
void handle_mc_insanity       args ( (CHAR_DATA *ch) );
void handle_wolfkin_insanity       args ( (CHAR_DATA *ch) );
bool break_shield             args ( (CHAR_DATA *ch, char *argument) );
void extract_wof              args ( (CHAR_DATA *ch) );
void extract_woa              args ( (CHAR_DATA *ch) );

bool check_dispel( int dis_level, CHAR_DATA *victim, int sn);
char *flow_text(sh_int sn, CHAR_DATA *ch);
bool is_gate_room args( (CHAR_DATA *ch, ROOM_INDEX_DATA *to_room) );

/* mob_prog.c */
void	program_flow	args( ( unsigned int vnum, char *source, CHAR_DATA *mob, CHAR_DATA *ch,
				const void *arg1, const void *arg2, int numlines ) );
MPROG_CODE *get_mprog_by_vnum args( (int vnum) );
void	mp_act_trigger	args( ( char *argument, CHAR_DATA *mob, CHAR_DATA *ch,
				const void *arg1, const void *arg2, int type ) );
bool	mp_percent_trigger args( ( CHAR_DATA *mob, CHAR_DATA *ch, 				
				const void *arg1, const void *arg2, int type ) );
void	mp_bribe_trigger  args( ( CHAR_DATA *mob, CHAR_DATA *ch, int amount ) );
bool	mp_exit_trigger   args( ( CHAR_DATA *ch, int dir ) );
void	mp_give_trigger   args( ( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj ) );
void 	mp_greet_trigger  args( ( CHAR_DATA *ch ) );
void	mp_hprct_trigger  args( ( CHAR_DATA *mob, CHAR_DATA *ch ) );

/* mob_cmds.c */
void	mob_interpret	args( ( CHAR_DATA *ch, char *argument ) );
char *	mprog_type_to_name	args( ( int ) );

/* save.c */
void	save_char_obj	args( ( CHAR_DATA *ch, bool backup ) );
bool	load_char_obj	args( ( DESCRIPTOR_DATA *d, char *name, bool disguise ) );

/* skills.c */
bool 	parse_gen_groups args( ( CHAR_DATA *ch,char *argument ) );
void 	list_group_costs args( ( CHAR_DATA *ch ) );
void    list_group_known args( ( CHAR_DATA *ch ) );
long exp_next_level args ( (CHAR_DATA *ch) );
int 	exp_per_level	args( ( CHAR_DATA *ch, int points ) );
void 	check_improve	args( ( CHAR_DATA *ch, int sn, bool success, 
				    int multiplier ) );
int 	group_lookup	args( (const char *name) );
void	gn_add		args( ( CHAR_DATA *ch, int gn) );
void 	gn_remove	args( ( CHAR_DATA *ch, int gn) );
void 	group_add	args( ( CHAR_DATA *ch, const char *name, bool deduct) );
void	group_remove	args( ( CHAR_DATA *ch, const char *name) );
int	race_exp_per_level	( int race, int class, int points );
void    do_unlink_char       args( ( CHAR_DATA *ch, CHAR_DATA * victim));
int     get_males_in_link    args( (CHAR_DATA *ch) );
int     get_females_in_link  args( (CHAR_DATA *ch) );
bool 	check_can_lead_link(CHAR_DATA * ch);
unsigned int	get_newbie_weapon( sh_int sn_weapon ) ;

/* special.c */
SF *	spec_lookup	args( ( const char *name ) );
char *	spec_name	args( ( SPEC_FUN *function ) );

/* teleport.c */
RID *	room_by_name	args( ( char *target, int level, bool error) );

/* update.c */
void	advance_level	args( ( CHAR_DATA *ch, bool hide, bool extended ) );
void	gain_exp	args( ( CHAR_DATA *ch, int gain ) );
void	gain_condition	args( ( CHAR_DATA *ch, int iCond, int value ) );
void	update_handler	args( ( void ) );
bool can_handle_weave args( (CHAR_DATA *ch, int sn) );

/* string.c */
void	string_edit	args( ( CHAR_DATA *ch, char **pString ) );
void    string_append   args( ( CHAR_DATA *ch, char **pString ) );
char *	string_replace	args( ( char * orig, char * old, char * new ) );
void    string_add      args( ( CHAR_DATA *ch, char *argument ) );
char *  format_string   args( ( char *oldstring /*, bool fSpace */ ) );
char *  first_arg       args( ( char *argument, char *arg_first, bool fCase ) );
char *	string_unpad	args( ( char * argument ) );
char *	string_proper	args( ( char * argument ) );

/* olc.c */
bool	run_olc_editor	args( ( DESCRIPTOR_DATA *d, char *incomm ) );
char	*olc_ed_name	args( ( CHAR_DATA *ch ) );
char	*olc_ed_vnum	args( ( CHAR_DATA *ch ) );
CLAN_DATA *get_clan_data	args( ( int clan ) );

/* olc_save.c */
void  save_guilds   args( ( CHAR_DATA *ch, char *argument ) );
char  *fwrite_flag	args( ( long flags, char buf[] ) );

/* lookup.c */
int	race_lookup	args( ( const char *name) );
int	item_lookup	args( ( const char *name) );
int	liq_lookup	args( ( const char *name) );

/* recycle.c */
ID_NAME *new_idname args( (void) );
bool  add_buf       args( (BUFFER *buffer, char *string) );
char  *buf_string   args( (BUFFER *buffer) );
BUFFER *new_buf     args( (void) );
void  free_buf      args( (BUFFER *buffer) );
LINK_DATA * new_link_info();
void free_link_info(LINK_DATA * ptrLinkData);

/* vmap.c */
void do_map (CHAR_DATA *ch, char *argument);
void do_vmapdesc(CHAR_DATA *ch, char *argument);
void do_vmapdesc_all(CHAR_DATA *ch, char *argument);

PKINFO_TYPE * pkupdate(CHAR_DATA * ch);

#undef	CD
#undef	MID
#undef	OD
#undef	OID
#undef	RID
#undef	SF
#undef   AD

/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define OBJ_VNUM_DUMMY	30

/*
 * Area flags.
 */
#define         AREA_NONE       0
#define         AREA_CHANGED    1	/* Area has been modified. */
#define         AREA_ADDED      2	/* Area has been added to. */
#define         AREA_LOADING    4	/* Used for counting in db.c */
#define         AREA_OPEN       8       /* Area is complete and ready for public */
#define         AREA_IMMORTAL  32       /* Only for Immortals */
#define         AREA_BUILDING  64       /* Building */
#define         AREA_SECRET   128       /* Secret area */

#define MAX_DIR	6
#define NO_FLAG -99	/* Must not be used in flags or stats. */

/*
 * Global Constants
 */
extern	char *	const	dir_name        [];
extern	const	sh_int	rev_dir         [];          /* sh_int - ROM OLC */
extern	const	struct	spec_type	spec_table	[];

/*
 * Global variables
 */
extern		AREA_DATA *		area_first;
extern		AREA_DATA *		area_last;
extern		SHOP_DATA *		shop_last;

extern		int			top_affect;
extern		int			top_area;
extern		int			top_ed;
extern		int			top_exit;
extern		int			top_help;
extern		int			top_mob_index;
extern		int			top_obj_index;
extern		int			top_reset;
extern		int			top_room;
extern		int			top_shop;

extern		unsigned int			top_vnum_mob;
extern		unsigned int			top_vnum_obj;
extern		unsigned int			top_vnum_room;

extern		char			str_empty       [1];

extern		MOB_INDEX_DATA *	mob_index_hash  [MAX_KEY_HASH];
extern		OBJ_INDEX_DATA *	obj_index_hash  [MAX_KEY_HASH];
extern		ROOM_INDEX_DATA *	room_index_hash [MAX_KEY_HASH];

/* Global variables related to warmboot and shutdown*/
extern bool iswarmboot;
extern bool isshutdown;
extern bool iscopyin;
extern int  pulse_warmboot;
extern int  pulse_shutdown;
extern char warmbootMsg[MAX_STRING_LENGTH];
extern CHAR_DATA *warmboot_pc;

/* Global Variable used for reward multiplication/power leveling */
extern int  reward_multiplier;
extern time_t reward_time;

//Vote.h included here

/**************************************************************************
*     Voting system designed and written for use on Legend of the Nobles  *
*         by Valnir (valnir@legendofthenobles.com).                       *
*                                                                         *
*     - http://www.legendofthenobles.com                                  *
*     - telnet://game.legendofthebobles.com:5400                          *
*                                                                         *
*     This code is free for use by any person wishing to use it with the  *
*         restriction that this information is kept intact and unchanged. *
*     Original version created on 05-26-2004                              *
**************************************************************************/

#define POLL_CREDITS	"Voting system designed and written for use on Legend of the Nobles\n\r"	\
	"    by Valnir (valnir@legendofthenobles.com).\n\r"				\
			"\n\r"										\
			"- http://www.legendofthenobles.com\n\r"					\
			"- telnet://game.legendofthebobles.com:5400\n\r\n\r"

/* definitions */
#define POLL_FILE	"polls.dat"	/* actual data file */
#define MAX_CHOICE	15
#define PEDIT( fun )           bool fun( CHAR_DATA *ch, char *argument )

/* define poll->flags */
#define POLL_HIDDEN	(A)
#define POLL_PUBLISHED	(B)
#define POLL_DELETE	(C)
#define POLL_CHANGED	(D)

typedef struct		poll_type	POLL_DATA;

#define KEY( literal, field, value )					\
				if ( !str_cmp( string, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}


/*
 * Skill / Spell Group definition
 */
struct poll_type
{
    POLL_DATA *	next;
    bool	valid;
    char *	topic;
    char *	to;
    char *	creator;
    long	open;
    long	close;
    long	created;
    int		flags;
    int		id;
    char *	choice[MAX_CHOICE];
    sh_int	votes[MAX_CHOICE];
};

/* olc.c */
#define EDIT_POLL(Ch, Poll)     ( Poll = (POLL_DATA *)Ch->desc->pEdit )
extern          int                     top_poll;
void     save_polls      args( ( CHAR_DATA *ch, char *argument ) );

bool is_poll_to (CHAR_DATA *ch, POLL_DATA * poll);

