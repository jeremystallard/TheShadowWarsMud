/***************************************************************************
 *  File: olc.c                                                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/



#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "olc.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"
#include "recycle.h"


/*
 * Local functions.
 */
AREA_DATA *get_area_data args ((int vnum));
char *colorstrem(char *argument);

COMMAND( do_purge )

MOB_INDEX_DATA		xMob;
OBJ_INDEX_DATA		xObj;
ROOM_INDEX_DATA		xRoom;
struct	skill_type	xSkill;
struct	race_type	xRace;
MPROG_CODE		xProg;
struct	cmd_type	xCmd;
struct	social_type	xSoc;

const	struct	olc_comm_type	mob_olc_comm_table	[]	=
{
 { "name",	(void *) &xMob.player_name,	ed_line_string,		NULL		},
 { "short",	(void *) &xMob.short_descr,	ed_line_string,		NULL		},
 { "long",	(void *) &xMob.long_descr,	ed_line_string,		(void *) 1	},
 { "material",	(void *) &xMob.material,	ed_line_string,		NULL		},
 { "desc",	(void *) &xMob.description,	ed_desc,		NULL		},
 { "level",	(void *) &xMob,			ed_mob_level,		NULL		},
/* { "level",	(void *) &xMob.level,		ed_number_niv,		NULL		},*/
 { "align",	(void *) &xMob.alignment,	ed_number_align,	NULL		},
 { "group",	(void *) &xMob.group,		ed_olded,		medit_group	},
 { "imm",	(void *) &xMob.imm_flags,	ed_flag_toggle,		imm_flags	},
 { "res",	(void *) &xMob.res_flags,	ed_flag_toggle,		res_flags	},
 { "vuln",	(void *) &xMob.vuln_flags,	ed_flag_toggle,		vuln_flags	},
 { "act",	(void *) &xMob.act,		ed_flag_toggle,		act_flags	},
 { "gain", (void *) &xMob.gain_flags,		ed_flag_toggle,		gain_flags	},
 { "train", (void *) &xMob.train_flags,		ed_flag_toggle,		train_flags	},
 { "tlevel", (void *) &xMob.train_level,	 ed_number_niv,		NULL		},
 { "affect",	(void *) &xMob.affected_by,	ed_flag_toggle,		affect_flags	},
 { "off",	(void *) &xMob.off_flags,	ed_flag_toggle,		off_flags	},
 { "form",	(void *) &xMob.form,		ed_flag_toggle,		form_flags	},
 { "parts",	(void *) &xMob.parts,		ed_flag_toggle,		part_flags	},
 { "shop",	(void *) &xMob,			ed_shop,		NULL		},
 { "create",	NULL,				ed_new_mob,		NULL		},
 { "spec",	(void *) &xMob.spec_fun,	ed_gamespec,		NULL		},
 { "recval",	(void *) &xMob,			ed_recval,		NULL		},
 { "sex",	(void *) &xMob.sex,		ed_shintlookup,		sex_lookup	},
 { "size",	(void *) &xMob.size,		ed_shintlookup,		size_lookup	},
 { "startpos",	(void *) &xMob.start_pos,	ed_shintlookup,		position_lookup	},
 { "defaultpos",(void *) &xMob.default_pos,	ed_shintlookup,		position_lookup	},
 { "damtype",	(void *) &xMob.dam_type,	ed_shintposlookup,	attack_lookup	},
 { "race",	(void *) &xMob,			ed_race,		NULL		},
 { "armor",	(void *) &xMob,			ed_ac,			NULL		},
 { "hitdice",	(void *) &xMob.hit[0],		ed_dice,		NULL		},
 { "endurancedice",	(void *) &xMob.endurance[0],		ed_dice,		NULL		},
 { "sphere",   (void *) &xMob,               ed_sphere, 			NULL		},
 { "world",	(void *) &xMob.world,		ed_flag_set_sh,		world_table	},
 { "damdice",	(void *) &xMob.damage[0],	ed_dice,		NULL		},
 { "hitroll",	(void *) &xMob.hitroll,		ed_number_s_pos,	NULL		},
 { "wealth",	(void *) &xMob.wealth,		ed_number_l_pos,	NULL		},
 { "guild",    (void *) &xMob,               ed_guild,           NULL		},
 { "guildflag",(void *) &xMob.guild_guard_flags,	ed_flag_toggle,		guild_guard_flags	},
 { "gflag",    (void *) &xMob.guild_guard_flags,	ed_flag_toggle,		guild_guard_flags	},
 { "addprog",	(void *) &xMob.mprogs,		ed_addprog,		NULL		},
 { "delprog",	(void *) &xMob.mprogs,		ed_delprog,		NULL		},
 { "mshow",	NULL,				ed_olded,		medit_show	},
 { "oshow",	NULL,				ed_olded,		oedit_show	},
 { "olist",	(void *) &xMob.area,		ed_olist,		NULL		},
 { "copy",	NULL,				ed_olded,		medit_copy	},
 { "commands",	NULL,				ed_olded,		show_commands	},
 { "?",		NULL,				ed_olded,		show_help	},
 { "version",	NULL,				ed_olded,		show_version	},
 { NULL,	NULL,				NULL,			NULL		}
};

const	struct	olc_comm_type	obj_olc_comm_table	[]	=
{
 { "name",	(void *) &xObj.name,		ed_line_string,		NULL		},
 { "short",	(void *) &xObj.short_descr,	ed_line_string,		NULL		},
 { "long",	(void *) &xObj.description,	ed_line_string,		NULL		},
 { "material",	(void *) &xObj.material,	ed_line_string,		NULL		},
 { "cost",	(void *) &xObj.cost,		ed_number_pos,		NULL		},
 { "level",	(void *) &xObj,			ed_object_level,	NULL		}, 
/* { "level",	(void *) &xObj.level,		ed_number_niv,		NULL		}, */
 { "condition",	(void *) &xObj.condition,	ed_number_s_pos,	NULL		},
 { "weight",	(void *) &xObj.weight,		ed_number_s_pos,	NULL		},
 { "extra",	(void *) &xObj.extra_flags,	ed_flag_toggle,		extra_flags	},
 { "wear",	(void *) &xObj.wear_flags,	ed_flag_toggle,		wear_flags	},
 { "ed",	(void *) &xObj.extra_descr,	ed_ed,			NULL		},
 { "type",	(void *) &xObj.item_type,	ed_flag_set_sh,		type_flags	},
 { "addaffect",	(void *) &xObj,			ed_addaffect,		NULL		},
 { "delaffect",	(void *) &xObj.affected,	ed_delaffect,		NULL		},
 { "addapply",	(void *) &xObj,			ed_addapply,		NULL		},
 { "v0",	NULL,				ed_value,		0		},
 { "v1",	NULL,				ed_value,		(void *) 1	},
 { "v2",	NULL,				ed_value,		(void *) 2	},
 { "v3",	NULL,				ed_value,		(void *) 3	},
 { "v4",	NULL,				ed_value,		(void *) 4	},
 { "create",	NULL,				ed_new_obj,		NULL		},
 { "mshow",	NULL,				ed_olded,		medit_show	},
 { "copy",	NULL,				ed_olded,		oedit_copy	},
 { "oshow",	NULL,				ed_olded,		oedit_show	},
 { "olist",	(void *) &xObj.area,		ed_olist,		NULL		},
 { "recval",	(void *) &xObj,			ed_objrecval,		NULL		},
 { "commands",	NULL,				ed_olded,		show_commands	},
 { "?",		NULL,				ed_olded,		show_help	},
 { "version",	NULL,				ed_olded,		show_version	},
 { NULL,	NULL,				NULL,			NULL		}
};

const	struct	olc_comm_type	room_olc_comm_table	[]	=
{
 { "n",		NULL,				ed_direccion,		(void *) DIR_NORTH	},
 { "north",	NULL,				ed_direccion,		(void *) DIR_NORTH	},
 { "s",		NULL,				ed_direccion,		(void *) DIR_SOUTH	},
 { "south",	NULL,				ed_direccion,		(void *) DIR_SOUTH	},
 { "e",		NULL,				ed_direccion,		(void *) DIR_EAST	},
 { "east",	NULL,				ed_direccion,		(void *) DIR_EAST	},
 { "w",		NULL,				ed_direccion,		(void *) DIR_WEST	},
 { "west",	NULL,				ed_direccion,		(void *) DIR_WEST	},
 { "u",	     NULL,				ed_direccion,		(void *) DIR_UP	},
 { "up",	     NULL,				ed_direccion,		(void *) DIR_UP	},
 { "d",		NULL,				ed_direccion,		(void *) DIR_DOWN	},
 { "down",	NULL,				ed_direccion,		(void *) DIR_DOWN	},
 { "name",	(void *) &xRoom.name,		ed_line_string,		NULL		},
 { "desc",	(void *) &xRoom.description,	ed_desc,		NULL		},
 { "ed",	(void *) &xRoom.extra_descr,	ed_ed,			NULL		},
 { "heal",	(void *) &xRoom.heal_rate,	ed_number_s_pos,	NULL		},
 { "endurance",	(void *) &xRoom.endurance_rate,	ed_number_s_pos,	NULL		},
 { "owner",	(void *) &xRoom.owner,		ed_line_string,		NULL		},
 { "roomflags",	(void *) &xRoom.room_flags,	ed_flag_toggle,		room_flags	},
 { "clan",	(void *) &xRoom.clan,		ed_shintlookup,		clan_lookup	},
 { "sector",	(void *) &xRoom.sector_type,	ed_flag_set_sh,		sector_flags	},
 
 /* Test section */
 { "enter north",	NULL,				ed_direccion,		(void *) ENTER_NORTH },
 { "enter east",	NULL,				ed_direccion,		(void *) ENTER_EAST },
 { "enter south",	NULL,				ed_direccion,		(void *) ENTER_SOUTH },
 { "enter west",	NULL,				ed_direccion,		(void *) ENTER_WEST },
 /* End test section */
 
 { "rlist",	NULL,				ed_olded,		redit_rlist	},
 { "mlist",	NULL,				ed_olded,		redit_mlist	},
 { "olist",	(void *) &xRoom.area,		ed_olist,		NULL		},
 { "copy",	NULL,				ed_olded,		redit_copy	},
 { "listreset",	NULL,				ed_olded,		redit_listreset },
 { "checkobj",	NULL,				ed_olded,		redit_checkobj	},
 { "checkmob",	NULL,				ed_olded,		redit_checkmob	},
 { "checkrooms",NULL,				ed_olded,		redit_checkrooms },
 { "mreset",	NULL,				ed_olded,		redit_mreset	},
 { "oreset",	NULL,				ed_olded,		redit_oreset	},
 { "create",	NULL,				ed_olded,		redit_create	},
 { "format",	NULL,				ed_olded,		redit_format	},
 { "mshow",	NULL,				ed_olded,		medit_show	},
 { "oshow",	NULL,				ed_olded,		oedit_show	},
 { "purge",	NULL,				ed_docomm,		do_purge	},
 { "clean",	NULL,				ed_olded,		redit_clean	},
 { "commands",	NULL,				ed_olded,		show_commands	},
 { "?",		NULL,				ed_olded,		show_help	},
 { "version",	NULL,				ed_olded,		show_version	},
 { NULL,	NULL,				NULL,			NULL		}
};

const struct olc_cmd_type polledit_table[] =
{
/*  {   command		function        }, */
    {	"show",		polledit_show	},
    {	"list",		polledit_list	},
    {	"create",	polledit_create	},
    {	"to",		polledit_to	},
    {   "topic",	polledit_topic	},
    {	"open",		polledit_open	},
    {	"close",	polledit_close	},
    {	"reopen",	polledit_reopen	},
    {	"choice",	polledit_choice	},
    {	"publish",	polledit_publish	},
    {	"delete",	polledit_delete	},
    {	"commands", 	show_cmd_commands	},
    {	"credits",	polledit_credits	},
    {	NULL,		0		}
};



void set_editor( DESCRIPTOR_DATA *d, int editor, void * param )
{
	d->editor = editor;
	d->pEdit = param;
	if (d->page < 1)
		d->page = 1;
	InitScreen(d);
}

/* Executed from comm.c.  Minimizes compiling when changes are made. */
bool    run_olc_editor (DESCRIPTOR_DATA * d, char * incomm)
{
	switch (d->editor)
	{
		case ED_AREA:
			aedit (d->character, incomm);
			break;
		case ED_ROOM:
			redit (d->character, incomm);
			break;
		case ED_OBJECT:
			oedit (d->character, incomm);
			break;
		case ED_MOBILE:
			medit (d->character, incomm);
			break;
		case ED_PROG:
			pedit (d->character, incomm);
			break;
		case ED_RACE:
			raedit (d->character, incomm);
			break;
		case ED_SOCIAL:
			sedit (d->character, incomm);
			break;
		case ED_SKILL:
			skedit (d->character, incomm);
			break;
		case ED_CMD:
			cmdedit (d->character, incomm);
			break;
		case ED_GROUP:
			gedit (d->character, incomm);
			break;
		case ED_HELP:
			hedit (d->character, incomm);
			break;
	     case ED_GUILD:
		  guildedit (d->character, d->incomm);
		  break;
     	case ED_SGUILD:
		  sguildedit (d->character, d->incomm);
		  break;
     	case ED_SSGUILD:
		  ssguildedit (d->character, d->incomm);
		  break;	  
             case ED_POLL:
                 polledit( d->character, d->incomm );
                 break;

		default:
			return FALSE;
	}
	return TRUE;
}

char   *olc_ed_name (CHAR_DATA * ch)
{
	static char buf[10];

	buf[0] = '\0';
	switch (ch->desc->editor)
	{
		case ED_AREA:
			sprintf (buf, "AEdit");
			break;
		case ED_ROOM:
			sprintf (buf, "REdit");
			break;
		case ED_OBJECT:
			sprintf (buf, "OEdit");
			break;
		case ED_MOBILE:
			sprintf (buf, "MEdit");
			break;
		case ED_PROG:
			sprintf (buf, "MPEdit");
			break;
		case ED_RACE:
			sprintf (buf, "RAEdit");
			break;
		case ED_SOCIAL:
			sprintf (buf, "SEdit");
			break;
		case ED_SKILL:
			sprintf (buf, "SKEdit");
			break;
		case ED_CMD:
			sprintf (buf, "CMDEdit");
			break;
		case ED_GROUP:
			sprintf (buf, "GEdit");
			break;
		case ED_HELP:
			sprintf (buf, "HEdit");
			break;
    		case ED_POLL:
        		sprintf( buf, "PoleEdit" );
        		break;
	     	case ED_GUILD:
	          	sprintf(buf, "GLEdit");
	          	break;
	     case ED_SGUILD:
	          sprintf(buf, "SGLEdit");
	          break;
	     case ED_SSGUILD:
	          sprintf(buf, "SSGLEdit");
	          break;
		default:
			sprintf (buf, " ");
			break;
	}
	return buf;
}



char   *olc_ed_vnum (CHAR_DATA * ch)
{
	AREA_DATA *pArea;
	ROOM_INDEX_DATA *pRoom;
	OBJ_INDEX_DATA *pObj;
	MOB_INDEX_DATA *pMob;
	MPROG_CODE *pMcode;
	HELP_DATA *pHelp;
	struct race_type *pRace;
	struct social_type *pSocial;
	struct skill_type *pSkill;
	struct cmd_type *pCmd;
	static char buf[10];

	buf[0] = '\0';
	switch (ch->desc->editor)
	{
		case ED_AREA:
			pArea = (AREA_DATA *) ch->desc->pEdit;
			sprintf (buf, "%d", pArea ? pArea->vnum : 0);
			break;
		case ED_ROOM:
			pRoom = ch->in_room;
			sprintf (buf, "%d", pRoom ? pRoom->vnum : 0);
			break;
		case ED_OBJECT:
			pObj = (OBJ_INDEX_DATA *) ch->desc->pEdit;
			sprintf (buf, "%d", pObj ? pObj->vnum : 0);
			break;
		case ED_MOBILE:
			pMob = (MOB_INDEX_DATA *) ch->desc->pEdit;
			sprintf (buf, "%d", pMob ? pMob->vnum : 0);
			break;
		case ED_PROG:
			pMcode = (MPROG_CODE *) ch->desc->pEdit;
			sprintf (buf, "%d", pMcode ? pMcode->vnum : 0);
			break;
		case ED_RACE:
			pRace = (struct race_type *) ch->desc->pEdit;
			sprintf (buf, "%s", pRace ? pRace->name : "");
			break;
		case ED_SOCIAL:
			pSocial = (struct social_type *) ch->desc->pEdit;
			sprintf (buf, "%s", pSocial ? pSocial->name : "");
			break;
		case ED_SKILL:
			pSkill = (struct skill_type *) ch->desc->pEdit;
			sprintf (buf, "%s", pSkill ? pSkill->name : "" );
			break;
		case ED_CMD:
			pCmd = (struct cmd_type *) ch->desc->pEdit;
			sprintf (buf, "%s", pCmd ? pCmd->name : "" );
			break;
		case ED_HELP:
			pHelp = (HELP_DATA *) ch->desc->pEdit;
			sprintf (buf, "%s", pHelp ? pHelp->keyword : "" );
			break;
		default:
			sprintf (buf, " ");
			break;
	}

	return buf;
}

const struct olc_comm_type * get_olc_table( int editor )
{
	switch(editor)
	{
		case ED_MOBILE:	return mob_olc_comm_table;
		case ED_OBJECT:	return obj_olc_comm_table;
		case ED_ROOM:	return room_olc_comm_table;
		case ED_SKILL:	return skill_olc_comm_table;
		case ED_RACE:	return race_olc_comm_table;
		case ED_CMD:	return cmd_olc_comm_table;
		case ED_PROG:	return prog_olc_comm_table;
		case ED_SOCIAL:	return social_olc_comm_table;
	     /* case ED_GUILD:      return guildedit_table; */

	}
	return NULL;
}

const struct olc_cmd_type * get_cmd_table( int editor )
{
	switch(editor)
	{
		case ED_HELP:  	 return hedit_table;
                case ED_AREA:    return aedit_table;
                case ED_GROUP:   return guildedit_table;
		case ED_POLL:	 return polledit_table;
		case ED_GUILD:   return guildedit_table;
		case ED_SGUILD:  return sguildedit_table;
		case ED_SSGUILD: return ssguildedit_table;
	}
	return NULL;
}

/*****************************************************************************
 Name:		show_olc_cmds
 Purpose:	Format up the commands from given table.
 Called by:	show_commands(olc_act.c).
 ****************************************************************************/
void show_olc_cmds( CHAR_DATA *ch )
{
	char    buf[MAX_STRING_LENGTH];
	char    buf1[MAX_STRING_LENGTH];
	const struct olc_comm_type * table;
	int     cmd;
	int     col;

	buf1[0] = '\0';
	col = 0;

	table = get_olc_table(ch->desc->editor);

	if (table == NULL)
	{
		bugf( "slow_olc_cmds : table NULL, editor %d",
			ch->desc->editor );
		return;
	}

	for (cmd = 0; table[cmd].name != NULL; cmd++)
	{
		sprintf (buf, "%-15.15s", table[cmd].name);
		strcat (buf1, buf);
		if (++col % 5 == 0)
			strcat (buf1, "\n\r");
	}

	if (col % 5 != 0)
		strcat (buf1, "\n\r");

	send_to_char (buf1, ch);
	return;
}

/*****************************************************************************
 Name:		show_cmd_cmds
 Purpose:	Format up the commands from given editor table.
 Called by:	show_cmd_commands(olc_act.c).
 ****************************************************************************/
void show_cmd_cmds( CHAR_DATA *ch )
{
	char    buf[MAX_STRING_LENGTH];
	char    buf1[MAX_STRING_LENGTH];
	const struct olc_cmd_type * table;
	int     cmd;
	int     col;

	buf1[0] = '\0';
	col = 0;

	table = get_cmd_table(ch->desc->editor);

	if (table == NULL)
	{
		bugf( "show_cmd_cmds : table NULL, editor %d",
			ch->desc->editor );
		return;
	}

	for (cmd = 0; table[cmd].name != NULL; cmd++)
	{
		sprintf (buf, "%-15.15s", table[cmd].name);
		strcat (buf1, buf);
		if (++col % 5 == 0)
			strcat (buf1, "\n\r");
	}

	if (col % 5 != 0)
		strcat (buf1, "\n\r");

	send_to_char (buf1, ch);
	return;
}

/*****************************************************************************
 Name:		show_commands
 Purpose:	Display all olc commands.
 Called by:	olc interpreters.
 ****************************************************************************/
bool    show_commands (CHAR_DATA * ch, char *argument)
{
	show_olc_cmds(ch);

	return FALSE;
}

/*****************************************************************************
 Name:		show_cmd_commands
 Purpose:	Display all cmd commands.
 Called by:	cmd interpreters.
 ****************************************************************************/
bool    show_cmd_commands (CHAR_DATA * ch, char *argument)
{
	show_cmd_cmds(ch);

	return FALSE;
}

/*****************************************************************************
 *                           Interpreter Tables.                             *
 *****************************************************************************/
const struct olc_cmd_type aedit_table[] =
{
/* {	command		function	}, */

   {	"age", 		aedit_age	},
   {	"builder", 	aedit_builder	},	/* s removed -- Hugin */
   {	"commands", 	show_cmd_commands	},
   {	"create", 	aedit_create	},
   {	"filename", 	aedit_file	},
   {	"name", 	aedit_name	},
/* {	"recall",	aedit_recall    },   ROM OLC */
   {	"reset", 	aedit_reset	},
   {	"security", 	aedit_security	},
   {	"show", 	aedit_show	},
   {	"vnum", 	aedit_vnum	},
   {	"lvnum", 	aedit_lvnum	},
   {	"uvnum", 	aedit_uvnum	},
   {	"credits", 	aedit_credits	},
   {	"lowrange", 	aedit_lowrange	},
   {	"highrange", 	aedit_highrange	},
   {	"delete", 	aedit_delete	},

   {	"?",		show_help	},
   {	"version", 	show_version	},

   {	NULL, 		0		}
};

/* Guild edit command table */
const struct olc_cmd_type guildedit_table[] =
{
    {   "commands",	show_cmd_commands   },
    {   "create",	guildedit_create	},
    {   "flag",		guildedit_flags	},
    {   "list",		guildedit_list	},
    {   "ml",           guildedit_ml        },
    {   "name",		guildedit_name	},
    {   "whoname",	guildedit_whoname	},
    {   "rank",		guildedit_rank	},
    {   "rooms",        guildedit_rooms     },
    {   "show",		guildedit_show	},
    {   "skill",	guildedit_skill	},
    {   "?",		show_help	},
    {   NULL,		0		}
};

/* SGuild edit command table */
const struct olc_cmd_type sguildedit_table[] =
{
    {   "commands",	show_cmd_commands   },
    {   "create",	sguildedit_create	},
    {   "flag",		sguildedit_flags	},
    {   "list",		sguildedit_list	},
    {   "ml",           sguildedit_ml        },
    {   "name",		sguildedit_name	},
    {   "whoname",	sguildedit_whoname	},
    {   "rank",		sguildedit_rank	},
    {   "rooms",        sguildedit_rooms     },
    {   "show",		sguildedit_show	},
    {   "skill",	sguildedit_skill	},
    {   "?",		show_help	},
    {   NULL,		0		}
};

/* SSGuild edit command table */
const struct olc_cmd_type ssguildedit_table[] =
{
    {   "commands",	show_cmd_commands   },
    {   "create",	ssguildedit_create	},
    {   "flag",		ssguildedit_flags	},
    {   "list",		ssguildedit_list	},
    {   "ml",           ssguildedit_ml        },
    {   "name",		ssguildedit_name	},
    {   "whoname",	ssguildedit_whoname	},
    {   "rank",		ssguildedit_rank	},
    {   "rooms",        ssguildedit_rooms     },
    {   "show",		ssguildedit_show	},
    {   "skill",	ssguildedit_skill	},
    {   "?",		show_help	},
    {   NULL,		0		}
};
/*****************************************************************************
 *                          End Interpreter Tables.                          *
 *****************************************************************************/


/** Function: get_clan_data
  * Descr   : Returns a pointer to the clan_table of the requested clan #
  * Returns : (CLAN_DATA *)
  * Syntax  : (n/a)
  * Written : v1.0 3/98
  * Author  : Gary McNickle <gary@dharvest.com>
  */
CLAN_DATA *get_clan_data( int clan )
{
  if ( clan <= MAX_CLAN && 
       (clan_table[clan].name != NULL && clan_table[clan].name[0] != '\0' ))
    return &clan_table[clan];
  
  return &clan_table[0]; /* null clan */
}

SGUILD_DATA *get_sguild_data( int sguild )
{
  if ( sguild <= MAX_CLAN && 
       (sguild_table[sguild].name != NULL && sguild_table[sguild].name[0] != '\0' ))
    return &sguild_table[sguild];
  
  return &sguild_table[0]; /* null clan */
}

SSGUILD_DATA *get_ssguild_data( int ssguild )
{
  if ( ssguild <= MAX_CLAN && 
       (ssguild_table[ssguild].name != NULL && ssguild_table[ssguild].name[0] != '\0' ))
    return &ssguild_table[ssguild];
  
  return &ssguild_table[0]; /* null clan */
}

/*****************************************************************************
 Name:		get_area_data
 Purpose:	Returns pointer to area with given vnum.
 Called by:	do_aedit(olc.c).
 ****************************************************************************/
AREA_DATA *get_area_data (int vnum)
{
	AREA_DATA *pArea;

	for (pArea = area_first; pArea; pArea = pArea->next)
	{
		if (pArea->vnum == vnum)
			return pArea;
	}

	return 0;
}



/*****************************************************************************
 Name:		edit_done
 Purpose:	Resets builder information on completion.
 Called by:	aedit, redit, oedit, medit(olc.c)
 ****************************************************************************/
bool    edit_done (CHAR_DATA * ch)
{
COMMAND(do_clear)

	if (ch->desc->editor != ED_NONE)
		send_to_char("Closing the editor.\n\r", ch);
	ch->desc->pEdit = NULL;
	ch->desc->editor = ED_NONE;
	ch->desc->page = 0;
	if (IS_SET(ch->comm, COMM_OLCX))
	{
		do_clear(ch,"reset");
		InitScreen(ch->desc);
	}
	return FALSE;
}

/*****************************************************************************
 *                              Interpreters.                                *
 *****************************************************************************/

/** Function: guildedit
  * Descr   : Interprets commands sent while inside the guild editor.
  *         : Passing appropriate commands on to editor, rest to mud.
  * Returns : (N/A)
  * Syntax  : (N/A| called by do_guildedit only)
  * Written : v1.0 3/98
  * Author  : Gary McNickle <gary@dharvest.com>
  */
void guildedit( CHAR_DATA *ch, char *argument )
{
  CLAN_DATA *pClan;
  char command[MIL];
  char arg[MIL];
  
  int cmd;
  
  EDIT_GUILD(ch, pClan);
  smash_tilde( argument );
  strcpy( arg, argument );
  argument = one_argument( argument, command );
 
  if (!IS_IMMORTAL(ch)) 
  {
    send_to_char("Insuffecient security to modify guild data", ch);
    edit_done( ch );
    return;
  }

  if ( !str_cmp(command, "done") )
  {
    edit_done( ch );
    return;
  }

  if ( command[0] == '\0' )
  {
    guildedit_show( ch, argument );
    return;
  }
  
  /* Search Table and Dispatch Command. */
  for ( cmd = 0; guildedit_table[cmd].name != NULL; cmd++ )
  {
    if ( !str_prefix( command, guildedit_table[cmd].name ) )
    {
      if ( (*guildedit_table[cmd].olc_fun) ( ch, argument ) )
      {
	SET_BIT( pClan->flags, GUILD_CHANGED );
	return;
      }
      else
       return;
    }
  }

  /* Default to Standard Interpreter. */
  interpret( ch, arg );
  return;
}

void sguildedit( CHAR_DATA *ch, char *argument )
{
  SGUILD_DATA *pSguild;
  char command[MIL];
  char arg[MIL];
  
  int cmd;
  
  EDIT_SGUILD(ch, pSguild);
  smash_tilde( argument );
  strcpy( arg, argument );
  argument = one_argument( argument, command );
 
  if (!IS_IMMORTAL(ch)) 
  {
    send_to_char("Insuffecient security to modify sguild data", ch);
    edit_done( ch );
    return;
  }

  if ( !str_cmp(command, "done") )
  {
    edit_done( ch );
    return;
  }

  if ( command[0] == '\0' )
  {
    sguildedit_show( ch, argument );
    return;
  }
  
  /* Search Table and Dispatch Command. */
  for ( cmd = 0; sguildedit_table[cmd].name != NULL; cmd++ )
  {
    if ( !str_prefix( command, sguildedit_table[cmd].name ) )
    {
      if ( (*sguildedit_table[cmd].olc_fun) ( ch, argument ) )
      {
	SET_BIT( pSguild->flags, SGUILD_CHANGED );
	return;
      }
      else
	   return;
    }
  }
  
  /* Default to Standard Interpreter. */
  interpret( ch, arg );
  return;
}

void ssguildedit( CHAR_DATA *ch, char *argument )
{
  SSGUILD_DATA *pSguild;
  char command[MIL];
  char arg[MIL];
  
  int cmd;
  
  EDIT_SGUILD(ch, pSguild);
  smash_tilde( argument );
  strcpy( arg, argument );
  argument = one_argument( argument, command );
 
  if (!IS_IMMORTAL(ch)) 
  {
    send_to_char("Insuffecient security to modify sguild data", ch);
    edit_done( ch );
    return;
  }

  if ( !str_cmp(command, "done") )
  {
    edit_done( ch );
    return;
  }

  if ( command[0] == '\0' )
  {
    ssguildedit_show( ch, argument );
    return;
  }
  
  /* Search Table and Dispatch Command. */
  for ( cmd = 0; ssguildedit_table[cmd].name != NULL; cmd++ )
  {
    if ( !str_prefix( command, ssguildedit_table[cmd].name ) )
    {
      if ( (*ssguildedit_table[cmd].olc_fun) ( ch, argument ) )
      {
	SET_BIT( pSguild->flags, SSGUILD_CHANGED );
	return;
      }
      else
	   return;
    }
  }
  
  /* Default to Standard Interpreter. */
  interpret( ch, arg );
  return;
}

/* Area Interpreter, called by do_aedit. */
void    aedit (CHAR_DATA * ch, char *argument)
{
	AREA_DATA *pArea;
	char    command[MAX_INPUT_LENGTH];
	char    arg[MAX_INPUT_LENGTH];
	int     cmd;
	int     value;

	EDIT_AREA (ch, pArea);

	strcpy (arg, argument);
	argument = one_argument (argument, command);

	if (!IS_BUILDER (ch, pArea))
	{
		send_to_char ("AEdit:  Insufficient security to modify area.\n\r", ch);
		edit_done (ch);
		return;
	}

	if (!str_cmp (command, "done"))
	{
		edit_done (ch);
		return;
	}

	if (command[0] == '\0')
	{
		aedit_show (ch, argument);
		return;
	}

	if ((value = flag_value (area_flags, command)) != NO_FLAG)
	{
		TOGGLE_BIT (pArea->area_flags, value);

		send_to_char ("Flag toggled.\n\r", ch);
		return;
	}

	/* Search Table and Dispatch Command. */
	for (cmd = 0; aedit_table[cmd].name != NULL; cmd++)
	{
		if (!str_prefix (command, aedit_table[cmd].name))
		{
			if ((*aedit_table[cmd].olc_fun) (ch, argument))
			{
				SET_BIT (pArea->area_flags, AREA_CHANGED);
				return;
			}
			else
				return;
		}
	}

	/* Default to Standard Interpreter. */
	interpret (ch, arg);
	return;
}

/* Room Interpreter, called by do_redit. */
void    redit (CHAR_DATA * ch, char *argument)
{
	ROOM_INDEX_DATA *pRoom;
	AREA_DATA *pArea;

	EDIT_ROOM (ch, pRoom);
	pArea = pRoom->area;

	if (!IS_BUILDER (ch, pArea))
	{
		send_to_char ("REdit:  Insufficient security to modify room.\n\r", ch);
		edit_done (ch);
		return;
	}

	if (!str_cmp (argument, "done"))
	{
		edit_done (ch);
		return;
	}

	if ( emptystring(argument) )
	{
		redit_show (ch, argument);
		return;
	}

	/* Search Table and Dispatch Command. */
	if ( !procesar_comando_olc(ch, argument, room_olc_comm_table) )
		interpret(ch, argument);

	return;
}

/* Object Interpreter, called by do_oedit. */
void    oedit (CHAR_DATA * ch, char *argument)
{
	AREA_DATA *pArea;
	OBJ_INDEX_DATA *pObj;

	EDIT_OBJ (ch, pObj);
	pArea = pObj->area;

	if (!IS_BUILDER (ch, pArea))
	{
		send_to_char ("OEdit: Insufficient security to modify area.\n\r", ch);
		edit_done (ch);
		return;
	}

	if (!str_cmp (argument, "done"))
	{
		edit_done (ch);
		return;
	}

	if (emptystring(argument))
	{
		oedit_show (ch, argument);
		return;
	}

	/* Search Table and Dispatch Command. */
	if ( !procesar_comando_olc(ch, argument, obj_olc_comm_table) )
		interpret(ch, argument);

	return;
}

/* Mobile Interpreter, called by do_medit. */
void    medit (CHAR_DATA * ch, char *argument)
{
	AREA_DATA *pArea;
	MOB_INDEX_DATA *pMob;

	EDIT_MOB (ch, pMob);
	pArea = pMob->area;

	if (!IS_BUILDER (ch, pArea))
	{
		send_to_char ("MEdit: Insufficient security to modify area.\n\r", ch);
		edit_done (ch);
		return;
	}

	if (!str_cmp (argument, "done"))
	{
		edit_done (ch);
		return;
	}

	if (emptystring(argument))
	{
		medit_show (ch, argument);
		return;
	}

	/* Search Table and Dispatch Command. */
	if (!procesar_comando_olc(ch, argument, mob_olc_comm_table))
		interpret (ch, argument);

	return;
}




const struct editor_cmd_type editor_table[] =
{
/* {	command		function	}, */

   {	"area",		do_aedit	},
   {	"room",		do_redit	},
   {	"object",	do_oedit	},
   {	"mobile",	do_medit	},
   {	"program",	do_pedit	},
   {	"race",		do_raedit	},
   {	"social",	do_sedit	},
   {	"skill",	do_skedit	},
   {	"command",	do_cmdedit	},
   {	"group",	do_gedit	},
   {  "guild",		do_guildedit	},
   {	"polls",	do_polledit	},
   {	"help",		do_hedit	},

   {	NULL,		0		}
};


/* Entry point for all editors. */
void    do_olc (CHAR_DATA * ch, char *argument)
{
	char    command[MAX_INPUT_LENGTH];
	int     cmd;

	argument = one_argument (argument, command);

	if (command[0] == '\0')
	{
		do_help (ch, "olc");
		return;
	}

	/* Search Table and Dispatch Command. */
	for (cmd = 0; editor_table[cmd].name != NULL; cmd++)
	{
		if (!str_prefix (command, editor_table[cmd].name))
		{
			(*editor_table[cmd].do_fun) (ch, argument);
			return;
		}
	}

	/* Invalid command, send help. */
	do_help (ch, "olc");
	return;
}

/** Function: do_guildedit
  * Descr   : Places user into the guild editor. Verifying Security.
  * Returns : Message if improper security level.
  * Syntax  : guildedit [guild #|create]
  * Written : v1.0 3/98
  * Author  : Gary McNickle <gary@dharvest.com>
  */
void do_guildedit( CHAR_DATA *ch, char *argument )
{
  CLAN_DATA *pClan;
  int value;
  char arg[MAX_STRING_LENGTH];
  
  if ( IS_NPC(ch) )
    return;
  
  pClan = &clan_table[1]; /* set default */
  
  argument	= one_argument(argument,arg);
  
  if ( is_number( arg ) ) {
    value = atoi( arg );
    pClan = get_clan_data( value );
    if (pClan->name[0] == '\0') {
	 send_to_char( "That guild does not exist.\n\r", ch );
	 return;
    }
  }
  else
    if ( !str_cmp( arg, "create" ) ) {
	 if ( ch->pcdata->security < 9 )
	   {
		send_to_char( "GuildEdit : Insuffecient security to create new guilds.\n\r", ch );
		return;
	   }
	 
	 guildedit_create( ch, "" );
	 ch->desc->editor = ED_GUILD;
	 return;
    }
  
  if (!IS_IMMORTAL(ch)) {
    send_to_char("Insuffecient security to edit Guilds.\n\r",ch);
    return;
  }
  
  ch->desc->pEdit = pClan;
  ch->desc->editor = ED_GUILD;
  return;
}

void do_sguildedit( CHAR_DATA *ch, char *argument )
{
  SGUILD_DATA *pSguild;
  int value;
  char arg[MAX_STRING_LENGTH];
  
  if ( IS_NPC(ch) )
    return;
  
  pSguild = &sguild_table[1]; /* set default */
  
  argument	= one_argument(argument,arg);
  
  if ( is_number( arg ) ) {
    value = atoi( arg );
    pSguild = get_sguild_data( value );
    if (pSguild->name[0] == '\0') {
	 send_to_char( "That sguild does not exist.\n\r", ch );
	 return;
    }
  }
  else
    if ( !str_cmp( arg, "create" ) ) {
	 if ( ch->pcdata->security < 9 )
	   {
		send_to_char( "SGuildEdit : Insuffecient security to create new sguilds.\n\r", ch );
		return;
	   }
	 
	 sguildedit_create( ch, "" );
	 ch->desc->editor = ED_SGUILD;
	 return;
    }
  
  if (!IS_IMMORTAL(ch)) {
    send_to_char("Insuffecient security to edit SGuilds.\n\r",ch);
    return;
  }
  
  ch->desc->pEdit = pSguild;
  ch->desc->editor = ED_SGUILD;
  return;
}

void do_ssguildedit( CHAR_DATA *ch, char *argument )
{
  SSGUILD_DATA *pSguild;
  int value;
  char arg[MAX_STRING_LENGTH];
  
  if ( IS_NPC(ch) )
    return;
  
  pSguild = &ssguild_table[1]; /* set default */
  
  argument	= one_argument(argument,arg);
  
  if ( is_number( arg ) ) {
    value = atoi( arg );
    pSguild = get_ssguild_data( value );
    if (pSguild->name[0] == '\0') {
	 send_to_char( "That sguild does not exist.\n\r", ch );
	 return;
    }
  }
  else
    if ( !str_cmp( arg, "create" ) ) {
	 if ( ch->pcdata->security < 9 )
	   {
		send_to_char( "SSGuildEdit : Insuffecient security to create new ssguilds.\n\r", ch );
		return;
	   }
	 
	 ssguildedit_create( ch, "" );
	 ch->desc->editor = ED_SSGUILD;
	 return;
    }
  
  if (!IS_IMMORTAL(ch)) {
    send_to_char("Insuffecient security to edit SSGuilds.\n\r",ch);
    return;
  }
  
  ch->desc->pEdit = pSguild;
  ch->desc->editor = ED_SSGUILD;
  return;
}

/* Entry point for editing area_data. */
void    do_aedit (CHAR_DATA * ch, char *argument)
{
	AREA_DATA *pArea;
	int     value;
	char    arg[MAX_STRING_LENGTH];

	pArea = ch->in_room->area;

	argument = one_argument (argument, arg);
	if (is_number (arg))
	{
		value = atoi (arg);
		if (!(pArea = get_area_data (value)))
		{
			send_to_char ("That area vnum does not exist.\n\r", ch);
			return;
		}
	}
	else if (!str_cmp (arg, "create"))
	{
		if (!aedit_create (ch, argument))
			return;
		else
			pArea = area_last;
	}

	if (!IS_BUILDER (ch, pArea) || ch->pcdata->security < 9)
	{
		send_to_char ("Insuffient security to modify an area.\n\r", ch);
		return;
	}

	set_editor(ch->desc, ED_AREA, pArea);
/*	ch->desc->pEdit = (void *) pArea;
	ch->desc->editor = ED_AREA; */
	return;
}

/* Entry point for editing room_index_data. */
void    do_redit (CHAR_DATA * ch, char *argument)
{
	ROOM_INDEX_DATA *pRoom;
	ROOM_INDEX_DATA *location;
	char    arg1[MIL];

	argument = one_argument (argument, arg1);

	pRoom = ch->in_room;

	if (!str_cmp (arg1, "reset"))
	{
		if (!IS_BUILDER (ch, pRoom->area))
		{
			send_to_char ("Insufficient memory to modify the room.\n\r", ch);
			return;
		}

		reset_room (pRoom);
		send_to_char ("Room Reset.\n\r", ch);
		return;
	}
	else if (!str_cmp (arg1, "create"))
	{
		if (argument[0] == '\0' || atoi (argument) == 0)
		{
			send_to_char ("Syntax : edit room create [vnum]\n\r", ch);
			return;
		}

		if (redit_create (ch, argument))
		{
		   char_from_room (ch);
		   if ( ( location = find_location( ch, argument ) ) == NULL )
		    {
		        send_to_char( "No such location.\n\r", ch );
		        return;
		    }

			char_to_room (ch, location);
			SET_BIT (pRoom->area->area_flags, AREA_CHANGED);
			pRoom = ch->in_room;
		}
	}
	else if ( !IS_NULLSTR(arg1) )
	{
		pRoom = get_room_index (atoi (arg1));

		if ( pRoom == NULL )
		{
			send_to_char( "Room doesn't exist.\n\r", ch );
			return;
		}
		if ((room_is_private(pRoom)) && (!IS_TRUSTED(ch,IMPLEMENTOR)) )
		{
			send_to_char("That room is private right now.\n\r",ch);
			return;
		}
                if (( IS_SET(pRoom->room_flags, ROOM_IMP_ONLY) )
                    && (!IS_TRUSTED(ch,IMPLEMENTOR)))
                {
			send_to_char("Only the IMP can change that room.\r\n",ch);
			return;
                }
	}

	if (!IS_BUILDER (ch, pRoom->area))
	{
		send_to_char ("Insufficient security to modify the room.\n\r", ch);
		return;
	}

	if ( pRoom == NULL )
		bugf( "do_redit : NULL pRoom, ch %s!", ch->name );

	if (ch->in_room != pRoom)
	{
		char_from_room(ch);
		char_to_room(ch, pRoom);
	}

	set_editor(ch->desc, ED_ROOM, pRoom);

/*	ch->desc->editor	= ED_ROOM;
	ch->desc->pEdit		= pRoom;

	if (ch->desc->page == 0)
		ch->desc->page = 1; */

	return;
}

/* Entry point for editing obj_index_data. */
void    do_oedit (CHAR_DATA * ch, char *argument)
{
	OBJ_INDEX_DATA *pObj;
	AREA_DATA *pArea;
	char    arg1[MAX_STRING_LENGTH];
	int     value;

	if (IS_NPC (ch))
		return;

	argument = one_argument (argument, arg1);

	if (is_number (arg1))
	{
		value = atoi (arg1);
		if (!(pObj = get_obj_index (value)))
		{
			send_to_char ("OEdit:  That vnum does not exist.\n\r", ch);
			return;
		}

		if (!IS_BUILDER (ch, pObj->area))
		{
			send_to_char ("Insufficient security to modify the object.\n\r", ch);
			return;
		}

		set_editor(ch->desc, ED_OBJECT, pObj);
/*		ch->desc->pEdit = (void *) pObj;
		ch->desc->editor = ED_OBJECT; */
		return;
	}
	else
	{
		if (!str_cmp (arg1, "create"))
		{
			value = atoi (argument);
			if (argument[0] == '\0' || value == 0)
			{
				send_to_char ("Syntax:  edit object create [vnum]\n\r", ch);
				return;
			}

			pArea = get_vnum_area (value);

			if (!pArea)
			{
				send_to_char ("OEdit:  That vnum is not assigned an area.\n\r", ch);
				return;
			}

			if (!IS_BUILDER (ch, pArea))
			{
				send_to_char ("Insufficient security to modify the object.\n\r", ch);
				return;
			}

			if (oedit_create (ch, argument))
			{
				SET_BIT (pArea->area_flags, AREA_CHANGED);
				ch->desc->editor = ED_OBJECT;
			}
			return;
		}
	}

	send_to_char ("OEdit:  There is no default object to edit.\n\r", ch);
	return;
}



/* Entry point for editing mob_index_data. */
void    do_medit (CHAR_DATA * ch, char *argument)
{
	MOB_INDEX_DATA *pMob;
	AREA_DATA *pArea;
	int     value;
	char    arg1[MAX_STRING_LENGTH];

	argument = one_argument (argument, arg1);

	if (is_number (arg1))
	{
		value = atoi (arg1);
		if (!(pMob = get_mob_index (value)))
		{
			send_to_char ("MEdit:  That vnum does not exist.\n\r", ch);
			return;
		}

		if (!IS_BUILDER (ch, pMob->area))
		{
			send_to_char ("Insufficient security to modify the mob.\n\r", ch);
			return;
		}

		set_editor(ch->desc, ED_MOBILE, pMob);
/*		ch->desc->pEdit = (void *) pMob;
		ch->desc->editor = ED_MOBILE; */
		return;
	}
	else
	{
		if (!str_cmp (arg1, "create"))
		{
			value = atoi (argument);
			if (arg1[0] == '\0' || value == 0)
			{
				send_to_char ("Syntax:  edit mobile create [vnum]\n\r", ch);
				return;
			}

			pArea = get_vnum_area (value);

			if (!pArea)
			{
				send_to_char ("MEdit:  That vnum is not assigned an area.\n\r", ch);
				return;
			}

			if (!IS_BUILDER (ch, pArea))
			{
				send_to_char ("Insufficient security to modify the mob.\n\r", ch);
				return;
			}

			if (ed_new_mob ("create", ch, argument, NULL, NULL) )
			{
				SET_BIT (pArea->area_flags, AREA_CHANGED);
				ch->desc->editor = ED_MOBILE;
			}
			return;
		}
	}

	send_to_char ("MEdit:  There is no default mobile to edit.\n\r", ch);
	return;
}

void    display_resets (CHAR_DATA * ch, ROOM_INDEX_DATA * pRoom)
{
	RESET_DATA *pReset;
	MOB_INDEX_DATA *pMob = NULL;
	char    buf[MAX_STRING_LENGTH];
	char    final[MAX_STRING_LENGTH];
	int     iReset = 0;

	final[0] = '\0';

	send_to_char (
	   " No.  Loads    Description       Location         Vnum   Ar Rm Description"
	   "\n\r"
	   "==== ======== ============= =================== ======== ===== ==========="
	   "\n\r", ch);

	for (pReset = pRoom->reset_first; pReset; pReset = pReset->next)
	{
		OBJ_INDEX_DATA *pObj;
		MOB_INDEX_DATA *pMobIndex;
		OBJ_INDEX_DATA *pObjIndex;
		OBJ_INDEX_DATA *pObjToIndex;
		ROOM_INDEX_DATA *pRoomIndex;

		final[0] = '\0';
		sprintf (final, "{x[%2d] ", ++iReset);

		switch (pReset->command)
		{
			default:
				sprintf (buf, "Bad reset command: %c.", pReset->command);
				strcat (final, buf);
				break;

			case 'M':
				if (!(pMobIndex = get_mob_index (pReset->arg1)))
				{
					sprintf (buf, "Load Mobile - Bad Mob %d\n\r", pReset->arg1);
					strcat (final, buf);
					continue;
				}

				if (!(pRoomIndex = get_room_index (pReset->arg3)))
				{
					sprintf (buf, "Load Mobile - Bad Room %d\n\r", pReset->arg3);
					strcat (final, buf);
					continue;
				}

				pMob = pMobIndex;
				sprintf (buf, "{rM{x[%5d] %-13.13s {xon the ground       {yR{x[%5d] %2d-%2d %-15.15s\n\r",
				   pReset->arg1, colorstrem(pMob->short_descr), pReset->arg3,
				   pReset->arg2, pReset->arg4, colorstrem(pRoomIndex->name));
				strcat (final, buf);

				/*
				 * Check for pet shop.
				 * -------------------
				 */
				{
					ROOM_INDEX_DATA *pRoomIndexPrev;

					pRoomIndexPrev = get_room_index (pRoomIndex->vnum - 1);
					if (pRoomIndexPrev
					   && IS_SET (pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
						final[5] = 'P';
				}

				break;

			case 'O':
				if (!(pObjIndex = get_obj_index (pReset->arg1)))
				{
					sprintf (buf, "Load Object - Bad Object %d\n\r",
					   pReset->arg1);
					strcat (final, buf);
					continue;
				}

				pObj = pObjIndex;

				if (!(pRoomIndex = get_room_index (pReset->arg3)))
				{
					sprintf (buf, "Load Object - Bad Room %d\n\r", pReset->arg3);
					strcat (final, buf);
					continue;
				}

				sprintf (buf, "O[%5d] %-13.13s {xon the ground       {yR{x[%5d]       %-15.15s\n\r",
				   pReset->arg1, colorstrem(pObj->short_descr),
				   pReset->arg3, colorstrem(pRoomIndex->name));
				strcat (final, buf);

				break;

			case 'P':
				if (!(pObjIndex = get_obj_index (pReset->arg1)))
				{
					sprintf (buf, "Put Object - Bad Object %d\n\r",
					   pReset->arg1);
					strcat (final, buf);
					continue;
				}

				pObj = pObjIndex;

				if (!(pObjToIndex = get_obj_index (pReset->arg3)))
				{
					sprintf (buf, "Put Object - Bad To Object %d\n\r",
					   pReset->arg3);
					strcat (final, buf);
					continue;
				}

				sprintf (buf,
				   "O[%5d] %-13.13s {xinside              O[%5d] %2d-%2d %-15.15s\n\r",
				   pReset->arg1,
				   colorstrem(pObj->short_descr),
				   pReset->arg3,
				   pReset->arg2,
				   pReset->arg4,
				   colorstrem(pObjToIndex->short_descr));
				strcat (final, buf);

				break;

			case 'G':
			case 'E':
				if (!(pObjIndex = get_obj_index (pReset->arg1)))
				{
					sprintf (buf, "Give/Equip Object - Bad Object %d\n\r",
					   pReset->arg1);
					strcat (final, buf);
					continue;
				}

				pObj = pObjIndex;

				if (!pMob)
				{
					sprintf (buf, "Give/Equip Object - No Previous Mobile\n\r");
					strcat (final, buf);
					break;
				}

				if (pMob->pShop)
				{
					sprintf (buf,
					   "O[%5d] %-13.13s {xin the inventory to {DS{x[%5d]       %-15.15s\n\r",
					   pReset->arg1,
					   colorstrem(pObj->short_descr),
					   pMob->vnum,
					   colorstrem(pMob->short_descr));
				}
				else {
					sprintf (buf,
					   "O[%5d] %-13.13s {x%-19.19s {rM{x[%5d]       %-15.15s\n\r",
					   pReset->arg1,
					   colorstrem(pObj->short_descr),
					   (pReset->command == 'G') ?
					   flag_string (wear_loc_strings, WEAR_NONE)
					   : flag_string (wear_loc_strings, pReset->arg3),
					   pMob->vnum,
					   colorstrem(pMob->short_descr));
            }
				strcat (final, buf);
				break;

				/*
				 * Doors are set in rs_flags don't need to be displayed.
				 * If you want to display them then uncomment the new_reset
				 * line in the case 'D' in load_resets in db.c and here.
				 */
			case 'D':
				pRoomIndex = get_room_index (pReset->arg1);
				sprintf (buf, "{yR{x[%5d] %s door of %-19.19s reset to %s\n\r",
				   pReset->arg1,
				   capitalize (dir_name[pReset->arg2]),
				   pRoomIndex->name,
				   flag_string (door_resets, pReset->arg3));
				strcat (final, buf);

				break;
				/*
				 * End Doors Comment.
				 */
			case 'R':
				if (!(pRoomIndex = get_room_index (pReset->arg1)))
				{
					sprintf (buf, "Randomize Exits - Bad Room %d\n\r",
					   pReset->arg1);
					strcat (final, buf);
					continue;
				}

				sprintf (buf, "{yR{x[%5d] Exits are randomized in %s\n\r",
				   pReset->arg1, pRoomIndex->name);
				strcat (final, buf);

				break;
		}
		send_to_char (final, ch);
	}

	return;
}



/*****************************************************************************
 Name:		add_reset
 Purpose:	Inserts a new reset in the given index slot.
 Called by:	do_resets(olc.c).
 ****************************************************************************/
void    add_reset (ROOM_INDEX_DATA * room, RESET_DATA * pReset, int indice)
{
	RESET_DATA *reset;
	int     iReset = 0;

	if (!room->reset_first)
	{
		room->reset_first = pReset;
		room->reset_last = pReset;
		pReset->next = NULL;
		return;
	}

	indice--;

	if (indice == 0)				/* First slot (1) selected. */
	{
		pReset->next = room->reset_first;
		room->reset_first = pReset;
		return;
	}

	/*
	 * If negative slot( <= 0 selected) then this will find the last.
	 */
	for (reset = room->reset_first; reset->next; reset = reset->next)
	{
		if (++iReset == indice)
			break;
	}

	pReset->next = reset->next;
	reset->next = pReset;
	if (!pReset->next)
		room->reset_last = pReset;
	return;
}

void    do_resets (CHAR_DATA * ch, char *argument)
{
	char    arg1[MAX_INPUT_LENGTH];
	char    arg2[MAX_INPUT_LENGTH];
	char    arg3[MAX_INPUT_LENGTH];
	char    arg4[MAX_INPUT_LENGTH];
	char    arg5[MAX_INPUT_LENGTH];
	char    arg6[MAX_INPUT_LENGTH];
	char    arg7[MAX_INPUT_LENGTH];
	RESET_DATA *pReset = NULL;

	argument = one_argument (argument, arg1);
	argument = one_argument (argument, arg2);
	argument = one_argument (argument, arg3);
	argument = one_argument (argument, arg4);
	argument = one_argument (argument, arg5);
	argument = one_argument (argument, arg6);
	argument = one_argument (argument, arg7);

	if (!IS_BUILDER (ch, ch->in_room->area))
	{
		send_to_char ("Resets: Invalid security for editing this area.\n\r",
		   ch);
		return;
	}

	/*
	 * Display resets in current room.
	 * -------------------------------
	 */
	if (arg1[0] == '\0')
	{
		if (ch->in_room->reset_first)
		{
			send_to_char (
			   "Resets: {rM{x = mobile, {yR{x = room, O = object, "
			   "{GP{x = pet, {DS{x = shopkeeper\n\r", ch);
			display_resets (ch, ch->in_room);
		}
		else
			send_to_char ("No resets in this room.\n\r", ch);
	}

	/*
	 * Take index number and search for commands.
	 * ------------------------------------------
	 */
	if (is_number (arg1))
	{
		ROOM_INDEX_DATA *pRoom = ch->in_room;

		/*
		 * Delete a reset.
		 * ---------------
		 */
		if (!str_cmp (arg2, "delete"))
		{
			int     insert_loc = atoi (arg1);

			if (!ch->in_room->reset_first)
			{
				send_to_char ("No resets in this area.\n\r", ch);
				return;
			}

			if (insert_loc - 1 <= 0)
			{
				pReset = pRoom->reset_first;
				pRoom->reset_first = pRoom->reset_first->next;
				if (!pRoom->reset_first)
					pRoom->reset_last = NULL;
			}
			else
			{
				int     iReset = 0;
				RESET_DATA *prev = NULL;

				for (pReset = pRoom->reset_first;
				   pReset;
				   pReset = pReset->next)
				{
					if (++iReset == insert_loc)
						break;
					prev = pReset;
				}

				if (!pReset)
				{
					send_to_char ("Reset not found.\n\r", ch);
					return;
				}

				if (prev)
					prev->next = prev->next->next;
				else
					pRoom->reset_first = pRoom->reset_first->next;

				for (pRoom->reset_last = pRoom->reset_first;
				   pRoom->reset_last->next;
				   pRoom->reset_last = pRoom->reset_last->next) ;
			}

			free_reset_data (pReset);
			send_to_char ("Reset deleted.\n\r", ch);
			SET_BIT (ch->in_room->area->area_flags, AREA_CHANGED);
		}
		else
			/*
			 * Add a reset.
			 * ------------
			 */
			if ((!str_cmp (arg2, "mob") && is_number (arg3))
		   || (!str_cmp (arg2, "obj") && is_number (arg3)))
		{
			/*
			 * Check for Mobile reset.
			 * -----------------------
			 */
			if (!str_cmp (arg2, "mob"))
			{
				if (get_mob_index (is_number (arg3) ? atoi (arg3) : 1) == NULL)
				{
					send_to_char ("Mob no existe.\n\r", ch);
					return;
				}
				pReset = new_reset_data ();
				pReset->command = 'M';
				pReset->arg1 = atoi (arg3);
				pReset->arg2 = is_number (arg4) ? atoi (arg4) : 1;	/* Max # */
				pReset->arg3 = ch->in_room->vnum;
				pReset->arg4 = is_number (arg5) ? atoi (arg5) : 1;	/* Min # */
			}
			else
				/*
				 * Check for Object reset.
				 * -----------------------
				 */
			if (!str_cmp (arg2, "obj"))
			{
				/*
				 * Inside another object.
				 * ----------------------
				 */
				if (!str_prefix (arg4, "inside"))
				{
					OBJ_INDEX_DATA *temp;

					temp = get_obj_index (is_number (arg5) ? atoi (arg5) : 1);
					if ((temp->item_type != ITEM_CONTAINER) &&
					   (temp->item_type != ITEM_CORPSE_NPC))
					{
						send_to_char ("Objeto 2 no es container.\n\r", ch);
						return;
					}
					pReset = new_reset_data ();
					pReset->arg1 = atoi (arg3);
					pReset->command = 'P';
					pReset->arg2 = is_number (arg6) ? atoi (arg6) : 1;
					pReset->arg3 = is_number (arg5) ? atoi (arg5) : 1;
					pReset->arg4 = is_number (arg7) ? atoi (arg7) : 1;
				}
				else
					/*
					 * Inside the room.
					 * ----------------
					 */
				if (!str_cmp (arg4, "room"))
				{
					if (get_obj_index (atoi (arg3)) == NULL)
					{
						send_to_char ("Vnum no existe.\n\r", ch);
						return;
					}
					pReset = new_reset_data ();
					pReset->arg1 = atoi (arg3);
					pReset->command = 'O';
					pReset->arg2 = 0;
					pReset->arg3 = ch->in_room->vnum;
					pReset->arg4 = 0;
				}
				else
					/*
					 * Into a Mobile's inventory.
					 * --------------------------
					 */
				{
					int blah = flag_value(wear_loc_flags, arg4);

					if (blah == NO_FLAG)
					{
						send_to_char ("Resets: '? wear-loc'\n\r", ch);
						return;
					}
					if (get_obj_index (atoi (arg3)) == NULL)
					{
						send_to_char ("Vnum no existe.\n\r", ch);
						return;
					}
					pReset = new_reset_data ();
					pReset->arg1 = atoi (arg3);
					pReset->arg3 = blah;
					if (pReset->arg3 == WEAR_NONE)
						pReset->command = 'G';
					else
						pReset->command = 'E';
				}
			}
			add_reset (ch->in_room, pReset, atoi (arg1));
			SET_BIT (ch->in_room->area->area_flags, AREA_CHANGED);
			send_to_char ("Reset added.\n\r", ch);
		}
		else if (!str_cmp (arg2, "random") && is_number (arg3))
		{
			if (atoi (arg3) < 1 || atoi (arg3) > 6)
			{
				send_to_char ("Invalid argument.\n\r", ch);
				return;
			}
			pReset = new_reset_data ();
			pReset->command = 'R';
			pReset->arg1 = ch->in_room->vnum;
			pReset->arg2 = atoi (arg3);
			add_reset (ch->in_room, pReset, atoi (arg1));
			SET_BIT (ch->in_room->area->area_flags, AREA_CHANGED);
			send_to_char ("Random exits reset added.\n\r", ch);
		}
		else
		{
			send_to_char ("Syntax: RESET <number> OBJ <vnum> <wear_loc>\n\r", ch);
			send_to_char ("        RESET <number> OBJ <vnum> inside <vnum> [limit] [count]\n\r", ch);
			send_to_char ("        RESET <number> OBJ <vnum> room\n\r", ch);
			send_to_char ("        RESET <number> MOB <vnum> [max #x area] [max #x room]\n\r", ch);
			send_to_char ("        RESET <number> DELETE\n\r", ch);
			send_to_char ("        RESET <number> RANDOM [#x exits]\n\r", ch);
		}
	}
	else // arg1 no es un number
	{
		if ( !str_cmp(arg1, "area") )
		{
			int tvar = 0, found = 0;
			char * arg;

			if ( is_number(arg2) )
			{
				send_to_char(	"Syntax Error.\n\r"
						"The Possiblities are:\n\r"
						"reset area mob [vnum/name]\n\r"
						"reset area obj [vnum/name]\n\r"
						"reset area [name]\n\r", ch );
				return;
			}

			if ( !str_cmp(arg2, "mob") )
			{
				tvar = 1;
				arg = arg3;
			}
			else
			if ( !str_cmp(arg2, "obj") )
			{
				tvar = 2;
				arg = arg3;
			}
			else
				arg = arg2;

			if ( tvar == 0 || tvar == 1 )
			{
				if ( is_number(arg) )
					found = get_mob_index(atoi(arg)) ? atoi(arg) : 0;
				else
					found = get_vnum_mob_name_area(arg, ch->in_room->area);
				if (found)
					tvar = 1;
			}

			if ( found == 0 && (tvar == 0 || tvar == 2) )
			{
				if ( is_number(arg) )
					found = get_obj_index(atoi(arg)) ? atoi(arg) : 0;
				else
					found = get_vnum_obj_name_area(arg, ch->in_room->area);
				if (found)
					tvar = 2;
			}

			if (found == 0)
			{
				printf_to_char(ch, "%s is not contained in this area.\n\r",
					(tvar == 0) ? "Mob/object" : ((tvar == 1) ? "Mob" : "Objeto") );
				return;
			}
			pReset		= new_reset_data ();
			pReset->command	= tvar == 1 ? 'M' : 'O';
			pReset->arg1	= found;
			pReset->arg2	= (tvar == 2) ? 0 : MAX_MOB;	/* Max # */
			pReset->arg3	= ch->in_room->vnum;
			pReset->arg4	= (tvar == 2) ? 0 : MAX_MOB;	/* Min # */

			printf_to_char(ch, "Area reset for %s %d...", tvar == 1 ? "mob" : "objeto", found );
			add_reset(ch->in_room, pReset, -1); // al final
			SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
			send_to_char("hecho.\n\r", ch);
		} // area
	}

	return;
}

int compare_area(const void *v1, const void *v2)
{
  return (*(AREA_DATA**)v2)->min_vnum - (*(AREA_DATA**)v1)->min_vnum;
}
int compare_area_names(const void *v1, const void *v2)
{
  return strcmp((*(AREA_DATA**)v1)->name, (*(AREA_DATA**)v2)->name);
}
/*****************************************************************************
 Name:		do_alist
 Purpose:	Normal command to list areas and display area information.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void    do_alist (CHAR_DATA * ch, char *argument)
{
  char    buf[MAX_STRING_LENGTH];
  char    result[MAX_STRING_LENGTH * 2];	/* May need tweaking. */
  BUFFER *output;
  AREA_DATA *pArea;
  AREA_DATA *areas[top_area];
  int i=0;
  int cnt=0;

  output = new_buf();

  /* Old Look and feel - Atwain */
  if (argument[0] == '\0') {
    sprintf (result, "{D[{W%3s{D] [{W%-27s{D] ({W%-5s-%5s{D) [{W%-10s{D]{W %3s {D[{W%-10s{D]{x\n\r",
		   "Num", "Area Name", "lvnum", "uvnum", "Filename", "Sec", "Builders");
    add_buf(output, result);
    for (pArea = area_first; pArea; pArea = pArea->next) {
	 sprintf (result, "[%3d] %-29.29s (%-5d-%5d) %-12.12s [%d] [%-10.10s]\n\r",
			pArea->vnum,
			pArea->name,
			pArea->min_vnum,
			pArea->max_vnum,
			pArea->file_name,
			pArea->security,
			pArea->builders);
	 add_buf(output, result);
    }
  }

  /* New look and feel */
  if (argument[0] != '\0') {
    
    /* Init areas array for sorting */
    for (pArea = area_first; pArea; pArea = pArea->next)
	 areas[i++] = pArea;

    sprintf (result, "{D[{W%3s{D] [{W%-27s{D] ({W%-5s-%5s{D) [{W%-10s{D]{W %3s {D[{W%-10s{D]{x\n\r",
		   "Num", "Area Name", "lvnum", "uvnum", "Filename", "Sec", "Builders");
    add_buf(output, result);
    
    if (!str_cmp(argument, "vnum")) {
	 qsort(areas, i, sizeof(pArea), compare_area);
	 while (--i >= 0) {
	   sprintf (result, "[%3d] %-29.29s (%-5d-%5d) %-12.12s [%d] [%-10.10s]\n\r",
			  areas[i]->vnum,
			  areas[i]->name,
			  areas[i]->min_vnum,
			  areas[i]->max_vnum,
			  areas[i]->file_name,
			  areas[i]->security,
			  areas[i]->builders);
	   add_buf(output, result);
	 }	 
    }

    else if(!str_cmp(argument, "name")) {
	 qsort(areas, i, sizeof(pArea), compare_area_names);
	 for (cnt = 0; cnt < i; cnt++) {
	   sprintf (result, "[%3d] %-29.29s (%-5d-%5d) %-12.12s [%d] [%-10.10s]\n\r",
			  areas[cnt]->vnum,
			  areas[cnt]->name,
			  areas[cnt]->min_vnum,
			  areas[cnt]->max_vnum,
			  areas[cnt]->file_name,
			  areas[cnt]->security,
			  areas[cnt]->builders);
	   add_buf(output, result);
	 }	 
    }
    else if(str_cmp(argument, "info")) {
	 send_to_char("Syntax: alist [vnum/name/info]\n\r", ch);
	 free_buf(output);
	 return;
    }
  }

  if (!str_cmp(argument, "info") || (argument[0] != '\0')) {
    sprintf(buf, "\n\rTotal areas on the {DShadow {rWars{x: {W%d{x\n\r", top_area);
    add_buf(output, buf);
    sprintf(buf, "Maximum available vnum's: {r%d{x\n\r", MAX_VNUM);
    add_buf(output, buf);
  }
  page_to_char( buf_string(output), ch );
  free_buf(output);
  return;
}

/*****************************************************************************
 Name:		do_alist
 Purpose:	Normal command to list areas and display area information.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void    do_alist_OLD (CHAR_DATA * ch, char *argument)
{
  char    buf[MAX_STRING_LENGTH];
  char    result[MAX_STRING_LENGTH * 2];	/* May need tweaking. */
  BUFFER *output;
  AREA_DATA *pArea;
  
  output = new_buf();
  
  sprintf (result, "{D[{W%3s{D] [{W%-27s{D] ({W%-5s-%5s{D) [{W%-10s{D]{W %3s {D[{W%-10s{D]{x\n\r",
		 "Num", "Area Name", "lvnum", "uvnum", "Filename", "Sec", "Builders");
  
  for (pArea = area_first; pArea; pArea = pArea->next) {
    sprintf (buf, "[%3d] %-29.29s (%-5d-%5d) %-12.12s [%d] [%-10.10s]\n\r",
		   pArea->vnum,
		   pArea->name,
		   pArea->min_vnum,
		   pArea->max_vnum,
		   pArea->file_name,
		   pArea->security,
		   pArea->builders);
    strcat (result, buf);
  }

  add_buf(output, result);
  page_to_char( buf_string(output), ch );
  free_buf(output);
  return;
}

bool procesar_comando_olc( CHAR_DATA *ch, char *argument, const struct olc_comm_type * table )
{
	char arg[MIL];
	MOB_INDEX_DATA *pMob;
	OBJ_INDEX_DATA *pObj;
	ROOM_INDEX_DATA *pRoom;
	struct race_type *pRace;
	struct skill_type *pSkill;
	struct cmd_type *pCmd;
	AREA_DATA *tArea;
	MPROG_CODE * pProg;
	struct social_type *pSoc;
	int temp;
	void * leader;

	argument = one_argument( argument, arg );

	for ( temp = 0; table[temp].name; temp++ )
	{
		if ( LOWER(arg[0]) == LOWER(table[temp].name[0])
		&&  !str_prefix(arg, table[temp].name) )
		{
			switch(ch->desc->editor)
			{
				case ED_MOBILE:
				EDIT_MOB(ch, pMob);
				tArea = pMob->area;
				if (table[temp].argument)
					leader = (void *) ((int) table[temp].argument - (int) &xMob + (int) pMob);
				else
					leader = NULL;
				if ( (*table[temp].function) (table[temp].name, ch, argument, leader, table[temp].parameter )
				     && tArea )
					SET_BIT(tArea->area_flags, AREA_CHANGED);
				return TRUE;
				break;

				case ED_OBJECT:
				EDIT_OBJ(ch, pObj);
				tArea = pObj->area;
				if (table[temp].argument)
					leader = (void *) ((int) table[temp].argument - (int) &xObj + (int) pObj);
				else
					leader = NULL;
				if ( (*table[temp].function) (table[temp].name, ch, argument, leader, table[temp].parameter )
				     && tArea != NULL )
					SET_BIT(tArea->area_flags, AREA_CHANGED);
				return TRUE;
				break;

				case ED_ROOM:
				EDIT_ROOM(ch, pRoom);
				tArea = pRoom->area;
				if (table[temp].argument)
					leader = (void *) ((int) table[temp].argument - (int) &xRoom + (int) pRoom);
				else
					leader = NULL;
				if ( (*table[temp].function) (table[temp].name, ch, argument, leader, table[temp].parameter )
				     && tArea != NULL )
					SET_BIT(tArea->area_flags, AREA_CHANGED);
				return TRUE;
				break;

				case ED_SKILL:
				EDIT_SKILL(ch, pSkill);
				if (table[temp].argument)
					leader = (void *) ((int) table[temp].argument - (int) &xSkill + (int) pSkill);
				else
					leader = NULL;
				if ( (*table[temp].function) (table[temp].name, ch, argument, leader, table[temp].parameter ) )
					save_skills();
				return TRUE;
				break;

				case ED_RACE:
				EDIT_RACE(ch, pRace);
				if (table[temp].argument)
					leader = (void *) ((int) table[temp].argument - (int) &xRace + (int) pRace);
				else
					leader = NULL;
				if ( (*table[temp].function) (table[temp].name, ch, argument, leader, table[temp].parameter ) )
					save_races();
				return TRUE;
				break;

				case ED_PROG:
				EDIT_PROG(ch, pProg);
				if (table[temp].argument)
					leader = (void *) ((int) table[temp].argument - (int) &xProg + (int) pProg);
				else
					leader = NULL;
				if ( (*table[temp].function) (table[temp].name, ch, argument, leader, table[temp].parameter ) )
					pProg->changed = TRUE;
				return TRUE;
				break;

				case ED_CMD:
				EDIT_CMD(ch, pCmd);
				if (table[temp].argument)
					leader = (void *) ((int) table[temp].argument - (int) &xCmd + (int) pCmd);
				else
					leader = NULL;
				if ( (*table[temp].function) (table[temp].name, ch, argument, leader, table[temp].parameter ) )
					save_table_commands();
				return TRUE;
				break;

				case ED_SOCIAL:
				EDIT_SOCIAL(ch, pSoc);
				if (table[temp].argument)
					leader = (void *) ((int) table[temp].argument - (int) &xSoc + (int) pSoc);
				else
					leader = NULL;
				if ( (*table[temp].function) (table[temp].name, ch, argument, leader, table[temp].parameter ) )
					save_socials();
				return TRUE;
				break;
			}
		}
	}

	return FALSE;
}

DO_FUN_DEC(do_page)
{
extern	void UpdateOLCScreen	(DESCRIPTOR_DATA *);
	int num;

	if (IS_NPC(ch)
	||  ch->desc == NULL
	||  ch->desc->editor == ED_NONE )
	{
		send_to_char( "You aren't editing anything.\n\r", ch );
		return;
	}

	if (!is_number(argument))
	{
		send_to_char( "What page do you want to change to?\n\r", ch );
		return;
	}

	num = atoi(argument);

	if ( num <= 0 )
	{
		send_to_char( "Now that's stupid.\n\r", ch );
		return;
	}

	ch->desc->page = num;

	InitScreen(ch->desc);
	UpdateOLCScreen(ch->desc);

	send_to_char( "Page changed. If you don't see See it, change to another.\n\r", ch );
	return;
}

