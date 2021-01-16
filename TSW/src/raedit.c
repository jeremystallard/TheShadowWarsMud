#include "include.h"
#include "lookup.h"
#include "olc.h"
#include "tables.h"
#include "recycle.h"

#if !defined(FIRST_BOOT)
struct	race_type	*race_table;
#endif
int	maxrace;

#define RACE_FILE	DATA_DIR "races"
#define RAEDIT( fun )		bool fun( CHAR_DATA *ch, char *argument )

extern struct race_type xRace;

const	struct	olc_comm_type	race_olc_comm_table	[]	=
{
 { "show",	NULL,				ed_olded,		raedit_show	},
#if !defined(FIRST_BOOT)
 { "name",	(void *) &xRace.name,		ed_line_string,		NULL		},
 { "pcrace",	(void *) &xRace.pc_race,	ed_bool,		NULL		},
 { "granted",	(void *) &xRace.granted,	ed_bool,		NULL		},
 { "act",	(void *) &xRace.act,		ed_flag_toggle,		act_flags	},
 { "aff",	(void *) &xRace.aff,		ed_flag_toggle,		affect_flags	},
 { "off",	(void *) &xRace.off,		ed_flag_toggle,		off_flags	},
 { "res",	(void *) &xRace.res,		ed_flag_toggle,		res_flags	},
 { "vuln",	(void *) &xRace.vuln,		ed_flag_toggle,		vuln_flags	},
 { "imm",	(void *) &xRace.imm,		ed_flag_toggle,		imm_flags	},
 { "form",	(void *) &xRace.form,		ed_flag_toggle,		form_flags	},
 { "part",	(void *) &xRace.parts,		ed_flag_toggle,		part_flags	},
 { "who",	(void *) &xRace.who_name,	ed_line_string,		NULL		},
 { "cresphere",	(void *) &xRace.sphere_points,		ed_number_s_pos,	NULL		},
 { "crestat",	(void *) &xRace.stat_points,		ed_number_s_pos,	NULL		},
 { "cremisc",	(void *) &xRace.misc_points,		ed_number_s_pos,	NULL		},
 { "points",	(void *) &xRace.points,		ed_number_s_pos,	NULL		},
 { "cmult",	NULL,				ed_olded,		raedit_cmult	},
 { "stats",	NULL,				ed_olded,		raedit_stats	},
 { "spheres",	NULL,				ed_olded,		raedit_spheres	},
 { "maxstats",	NULL,				ed_olded,		raedit_maxstats },
 { "skills",	NULL,				ed_olded,		raedit_skills	},
 { "size",	(void *) &xRace.size,		ed_shintposlookup,	size_lookup	},
/*
 { "new",	NULL,				ed_olded,		raedit_new	},
*/
#endif
 { "list",	NULL,				ed_olded,		raedit_list	},
 { "commands",	NULL,				ed_olded,		show_commands	},
 { "?",		NULL,				ed_olded,		show_help	},
 { "version",	NULL,				ed_olded,		show_version	},
 { NULL,	NULL,				NULL,			NULL		}
};

void    raedit (CHAR_DATA * ch, char *argument)
{
	struct race_type * pRace;

	EDIT_RACE (ch, pRace);

	if (ch->pcdata->security < 7)
	{
		send_to_char ("RAEdit : Insufficent security for editing skills.\n\r", ch);
		edit_done (ch);
		return;
	}

	if (!str_cmp (argument, "done"))
	{
		edit_done (ch);
		return;
	}

	if (!str_cmp (argument, "save"))
	{
		save_races();
		return;
	}

	if ( emptystring(argument) )
	{
		raedit_show (ch, argument);
		return;
	}

	/* Search Table and Dispatch Command. */
	if ( !procesar_comando_olc(ch, argument, race_olc_comm_table) )
		interpret(ch, argument);

	return;
}

void do_raedit(CHAR_DATA *ch, char *argument)
{
    struct race_type *pRace;
    int race;

    if ( IS_NPC(ch) || ch->desc == NULL )
    	return;

    if ( ch->pcdata->security < 7 )
    {
    	send_to_char( "RAEdit : Insufficient security to edit races.\n\r", ch );
    	return;
    }

    if ( IS_NULLSTR(argument) || (race = race_lookup(argument)) == 0 )
    {
    	send_to_char( "Syntax : RAEdit [race]\n\r", ch );
    	return;
    }

    pRace = &race_table[race];

    ch->desc->pEdit=(void *)pRace;
    ch->desc->editor= ED_RACE;

    return;
}

RAEDIT( raedit_show )
{
	struct race_type *pRace;
	char buf[MSL];
#if !defined(FIRST_BOOT)
	static const char *Stats[] = { "Str", "Int", "Wis", "Dex", "Con" };
	int i = 0;
#endif

	EDIT_RACE(ch,pRace);

	sprintf( buf,	"Name        : [%s]\n\r", pRace->name );
	send_to_char( buf, ch );
	sprintf( buf,   "Race PC     : [%s]\n\r", pRace->pc_race ? "yes" : "no");
	send_to_char( buf, ch );
	sprintf( buf,   "Granted     : [%s]\n\r", pRace->granted ? "yes" : "no");
	send_to_char( buf, ch );
	sprintf( buf,  	"Act         : [%s]\n\r", flag_string( act_flags, pRace->act) );
	send_to_char( buf, ch );
	sprintf( buf,  	"Aff         : [%s]\n\r", flag_string( affect_flags, pRace->aff) );
	send_to_char( buf, ch );
	sprintf( buf,  	"Off         : [%s]\n\r", flag_string( off_flags, pRace->off) );
	send_to_char( buf, ch );
	sprintf( buf,  	"Imm         : [%s]\n\r", flag_string( imm_flags, pRace->imm) );
	send_to_char( buf, ch );
	sprintf( buf,  	"Res         : [%s]\n\r", flag_string( res_flags, pRace->res) );
	send_to_char( buf, ch );
	sprintf( buf,  	"Vuln        : [%s]\n\r", flag_string( vuln_flags, pRace->vuln) );
	send_to_char( buf, ch );
	sprintf( buf,  	"Form        : [%s]\n\r", flag_string( form_flags, pRace->form) );
	send_to_char( buf, ch );
	sprintf( buf,  	"Parts       : [%s]\n\r", flag_string( part_flags, pRace->parts) );
	send_to_char( buf, ch );

#if !defined(FIRST_BOOT)

	sprintf( buf,       "Size        : [%-10.10s]\n\r", size_table[pRace->size].name);
	send_to_char( buf , ch);
	sprintf( buf,       "Points      : [%10d]\n\r", pRace->points);
	send_to_char( buf , ch);

	send_to_char( "\n\rCreation values\n\r", ch );
	sprintf( buf,       "Sphere Pnts : [%10d]\n\r", pRace->sphere_points);
	send_to_char( buf , ch);
	sprintf( buf,       "Stat   Pnts : [%10d]\n\r", pRace->stat_points);
	send_to_char( buf , ch);
	sprintf( buf,       "Misc   Pnts : [%10d]\n\r", pRace->misc_points);
	send_to_char( buf , ch);
	
	for (i = 0; i < MAX_SPHERE; i++) {
	  sprintf( buf, "%s:%4d ", capitalize(sphere_table[i].name), pRace->sphere[i]);
	  send_to_char(buf, ch);
	}
	send_to_char( "\n\r", ch );
	
	send_to_char( "\n\r", ch);
	send_to_char( "Name      CMu Exp       Name      CMu Exp       Name      CMu Exp\n\r", ch );
	for ( i = 0; i < MAX_CLASS; ++i )
	{
		sprintf( buf, "%-7.7s   %3d %4d(%3d)%s",
			capitalize(class_table[i].name),
			pRace->class_mult[i],
			race_exp_per_level(pRace->race_id,i,get_points(pRace->race_id,i)),
			get_points(pRace->race_id,i),
			i % 3 == 2 ? "\n\r" : " " );
		send_to_char( buf, ch );
	}
	if ( i % 3 )
	  send_to_char( "\n\r", ch );

	send_to_char( "\n\r", ch );

	for ( i = 0; i < MAX_STATS; ++i )
	{
		sprintf( buf, "%s:%2d(%2d) ", Stats[i], pRace->stats[i], pRace->max_stats[i] );
		send_to_char( buf, ch );
	}
	send_to_char( "\n\r", ch );

	for ( i = 0; i < 5; ++i )
		if ( !IS_NULLSTR(pRace->skills[i]) )
		{
			sprintf( buf, "%2d. %s\n\r",
				i,
				pRace->skills[i] );
			send_to_char( buf, ch );
		}
#endif

	return FALSE;
}

RAEDIT( raedit_list )
{
	int i;
	char buf[MSL];
	BUFFER *fBuf;
	
	fBuf = new_buf();
	
	for ( i = 0; race_table[i].name; i++ )
	{
		sprintf( buf, "%2d %c {W%-33.33s{p", i,
		race_table[i].pc_race ? '+' : '-',
		race_table[i].name );
		if ( i % 2 == 1 )
			strcat( buf, "\n\r" );
		else
			strcat( buf, " " );
		add_buf( fBuf, buf );
	}

	if ( i % 2 == 1 )
		add_buf( fBuf, "\n\r" );

	page_to_char( buf_string(fBuf), ch );
	free_buf( fBuf );
	
	return FALSE;
}

#if !defined(FIRST_BOOT)
RAEDIT( raedit_new )
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *tch;
	struct race_type *new_table;
	int maxRace;
	
	if ( IS_NULLSTR(argument) )
	{
		send_to_char( "Syntax : new [name-of-the-new-race]\n\r", ch );
		return FALSE;
	}

	if (race_lookup(argument) != 0)
	{
		send_to_char ("A race with this name already exists!\n\r",ch);
		return FALSE;
	}

	for ( d = descriptor_list; d; d = d->next )
	{
		if ( d->connected != CON_PLAYING || (tch = CH(d)) == NULL || tch->desc == NULL )
			continue;

		if ( tch->desc->editor == ED_RACE )
		  	edit_done(ch);
	}

	/* reallocate the table */

	for ( maxRace = 0; race_table[maxRace].name; maxRace++ )
		;

	maxRace++;
	new_table = realloc (race_table, sizeof(struct race_type) * (maxRace + 1));

	if (!new_table) /* realloc failed */
	{
		send_to_char ("REallocation failed. Prepare for impact.\n\r",ch);
		return FALSE;
	}

	race_table = new_table;

	race_table[maxRace-1].name		= str_dup ("unique");
	race_table[maxRace-1].pc_race		= FALSE;
	race_table[maxRace-1].granted		= FALSE;
	race_table[maxRace-1].act		= 0;
	race_table[maxRace-1].aff		= 0;
	race_table[maxRace-1].off		= 0;
	race_table[maxRace-1].imm		= 0;
	race_table[maxRace-1].res		= 0;
	race_table[maxRace-1].vuln		= 0;
	race_table[maxRace-1].form		= 0;
	race_table[maxRace-1].parts		= 0;

	free_string(race_table[maxRace-2].name);

	race_table[maxRace-2].name		= str_dup (argument);
	race_table[maxRace-2].pc_race		= FALSE;
	race_table[maxRace-2].granted		= FALSE;
	race_table[maxRace-2].act		= 0;
	race_table[maxRace-2].aff		= 0;
	race_table[maxRace-2].off		= 0;
	race_table[maxRace-2].imm		= 0;
	race_table[maxRace-2].res		= 0;
	race_table[maxRace-2].vuln		= 0;
	race_table[maxRace-2].form		= 0;
	race_table[maxRace-2].parts		= 0;

	race_table[maxRace].name		= NULL;

	ch->desc->editor	= ED_RACE;
	ch->desc->pEdit		= (void *) &race_table[maxRace-2];

	send_to_char ("New race created.\n\r",ch);
	return TRUE;
}

RAEDIT( raedit_cmult )
{
	struct race_type * race;
	int mult,vclase;
	char clase[MIL];

	EDIT_RACE(ch,race);

	if ( IS_NULLSTR(argument) )
	{
		send_to_char( "Syntax : cmult [class] [multiplier]\n\r", ch );
		return FALSE;
	}

	argument = one_argument( argument, clase );

	if ( (vclase = class_lookup( clase )) == -1 )
	{
		send_to_char( "RAEdit : Class exists.\n\r", ch );
		return FALSE;
	}

	if ( !is_number(argument) )
	{
		send_to_char( "RAEdit : Invalid argument.\n\r", ch );
		return FALSE;
	}

	mult = atoi(argument);

	if ( mult < 0 || mult > 200 )
	{
		send_to_char( "RAEdit : Multiplier invalid.\n\r", ch );
		return FALSE;
	}

	race->class_mult[vclase] = mult;
	send_to_char( "Ok.\n\r", ch );
	return TRUE;
}

RAEDIT( raedit_spheres )
{
	struct race_type *race;
	int vsphere, valor;
	char sphere[MIL];
	
	EDIT_RACE(ch,race);
	
	if ( IS_NULLSTR(argument) ) {
	  send_to_char( "Syntax : spheres [sphere] [value]\n\r", ch );
	  return FALSE;
	}

	argument = one_argument( argument, sphere );
	
	//vsphere = flag_value( sphere_table, sphere );
	if (!str_prefix(sphere, "air"))
	  vsphere = SPHERE_AIR;
	else if (!str_prefix(sphere, "earth"))
	  vsphere = SPHERE_EARTH;
	else if (!str_prefix(sphere, "fire"))
	  vsphere = SPHERE_FIRE;
	else if (!str_prefix(sphere, "spirit"))
	  vsphere = SPHERE_SPIRIT;
	else if (!str_prefix(sphere, "water"))
	  vsphere = SPHERE_WATER;
	else 
	  vsphere = NO_FLAG;
	
	if ( vsphere == NO_FLAG ) {
	  send_to_char( "RAEdit : Sphere invalid.\n\r", ch );
	  return FALSE;
	}
	
	if ( !is_number(argument) ) {
	  send_to_char( "RAEdit : Value must be a number.\n\r", ch );
	  return FALSE;
	}

	valor = atoi(argument);

	if ( valor < 1 || valor >  MAX_SPHERE_VALUE_PC) {
	  send_to_char( "RAEdit : Invalid Number.\n\r", ch );
	  return FALSE;
	}

	race->sphere[vsphere] = valor;
	send_to_char( "Ok.\n\r", ch );
	return TRUE;
}

RAEDIT( raedit_stats )
{
	struct race_type *race;
	int vstat, valor;
	char stat[MIL];

	EDIT_RACE(ch,race);

	if ( IS_NULLSTR(argument) )
	{
		send_to_char( "Syntax : stats [stat] [value]\n\r", ch );
		return FALSE;
	}

	argument = one_argument( argument, stat );

	vstat = flag_value( stat_table, stat );

	if ( vstat == NO_FLAG )
	{
		send_to_char( "RAEdit : Stat invalid.\n\r", ch );
		return FALSE;
	}

	if ( !is_number(argument) )
	{
		send_to_char( "RAEdit : Value must be a number.\n\r", ch );
		return FALSE;
	}

	valor = atoi(argument);

	if ( valor < 3 || valor > 25 )
	{
		send_to_char( "RAEdit : Invalid Number.\n\r", ch );
		return FALSE;
	}

	race->stats[vstat] = valor;
	send_to_char( "Ok.\n\r", ch );
	return TRUE;
}

RAEDIT( raedit_maxstats )
{
	struct race_type *race;
	int vstat, valor;
	char stat[MIL];

	EDIT_RACE(ch,race);

	if ( IS_NULLSTR(argument) )
	{
		send_to_char( "Syntax : maxstats [stat] [value]\n\r", ch );
		return FALSE;
	}

	argument = one_argument( argument, stat );

	vstat = flag_value( stat_table, stat );

	if ( vstat == -1 )
	{
		send_to_char( "RAEdit : Stat invalid.\n\r", ch );
		return FALSE;
	}

	if ( !is_number(argument) )
	{
		send_to_char( "RAEdit : Value must be a number.\n\r", ch );
		return FALSE;
	}

	valor = atoi(argument);

	if ( valor < 3 || valor > 25 )
	{
		send_to_char( "RAEdit : Invalid value.\n\r", ch );
		return FALSE;
	}

	race->max_stats[vstat] = valor;
	send_to_char( "Ok.\n\r", ch );
	return TRUE;
}

RAEDIT( raedit_skills )
{
	struct race_type * race;
	int sk, num;
	char snum[MIL];

	EDIT_RACE(ch, race);

	if (IS_NULLSTR(argument))
	{
		send_to_char( "Syntax : skills [number] [skill]\n\r", ch );
		return FALSE;
	}

	argument = one_argument( argument, snum );

	if ( !is_number(snum) )
	{
		send_to_char( "RAEdit : Invalid argument.\n\r", ch );
		return FALSE;
	}
	
	num = atoi( snum );

	if ( num < 0 || num > 4 )
	{
		send_to_char( "RAEdit : Number must be between 0 and 4.\n\r", ch );
		return FALSE;
	}

	sk = skill_lookup( argument );

	if ( sk == -1 )
	{
		send_to_char( "RAEdit : Argument is not a skill.\n\r", ch );
		return FALSE;
	}

	free_string(race->skills[num]);
	race->skills[num] = str_dup( argument );
	send_to_char( "Ok.\n\r", ch );
	return TRUE;
}

#endif
