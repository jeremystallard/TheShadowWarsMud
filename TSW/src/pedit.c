/* The following code is based on ILAB OLC by Jason Dinkel */
/* Mobprogram code by Lordrom for Nevermore Mud */

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
#include "recycle.h"

#define MPEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)

extern MPROG_CODE xProg;

DECLARE_OLC_FUN( pedit_create );
DECLARE_OLC_FUN( pedit_show   );

const	struct	olc_comm_type	prog_olc_comm_table	[]	=
{
	{	"create",	NULL,			ed_olded,	pedit_create	},
	{	"code",		&xProg.code,		ed_desc,	NULL		},
	{	"show",		NULL,			ed_olded,	pedit_show	},
	{ 	"commands",	NULL,			ed_olded,	show_commands	},
	{ 	"?",		NULL,			ed_olded,	show_help	},
	{ 	"version",	NULL,			ed_olded,	show_version	},
	{ 	NULL,		NULL,			NULL,		NULL		}
};

void pedit( CHAR_DATA *ch, char *argument)
{
    if (ch->pcdata->security < MIN_PEDIT_SECURITY)
    {
	send_to_char("MPEdit : Insufficient security to modify program.\n\r",ch);
	edit_done(ch);
	return;
    }

    if ( emptystring(argument) )
    {
        pedit_show(ch, argument);
        return;
    }

    if (!str_cmp(argument, "done") )
    {
        edit_done(ch);
        return;
    }

    if ( !procesar_comando_olc(ch, argument, prog_olc_comm_table) )
	interpret(ch, argument);

    return;
}

void do_pedit(CHAR_DATA *ch, char *argument)
{
    MPROG_CODE *pMcode;
    char command[MAX_INPUT_LENGTH];

    if ( IS_NPC(ch) )
	return;

    if ( ch->pcdata->security < MIN_PEDIT_SECURITY )
    {
    	send_to_char( "PEdit : Insufficint security to edit program.\n\r", ch );
    	return;
    }

    argument = one_argument( argument, command);

    if(is_number(command) )
    {
	AREA_DATA * pArea;

	if ( (pArea = get_vnum_area(atoi(command))) == NULL )
	{
		send_to_char( "PEdit : Vnum not assigned to the area.\n\r", ch);
		return;
	}
	if ( !IS_BUILDER(ch, pArea))
	{
		send_to_char( "PEdit : Insufficient security to change the area.\n\r", ch);
		return;
	}
	if (! (pMcode = get_mprog_index( atoi(command) ) ) )
	{
		send_to_char("PEdit : Vnum doesn't exist.\n\r",ch);
		return;
	}
	ch->desc->pEdit		= (void *)pMcode;
	ch->desc->editor	= ED_PROG;
	return;
    }

    if (!str_cmp(command, "create" ) )
    {
        if (argument[0] == '\0')
        {
           send_to_char("Syntax: edit code create [vnum]\n\r",ch);
           return;
        }

	pedit_create(ch, argument);
	return;
    }

    send_to_char( "Syntax : pedit [vnum]\n\r", ch );
    send_to_char( "           pedit create [vnum]\n\r", ch );

    return;
}

MPEDIT (pedit_create)
{
    MPROG_CODE *pMcode;
    AREA_DATA *pArea;
    int value;

    value = atoi(argument);

    if (argument[0] == '\0' || value == 0)
    {
        send_to_char("Syntax: pedit create [vnum]\n\r",ch);
        return FALSE;
    }

    if (get_mprog_index(value) )
    {
        send_to_char("PEdit: Vnum already exists.\n\r",ch);
        return FALSE;
    }

    if ( (pArea = get_vnum_area( value )) == NULL )
    {
    	send_to_char("PEdit: Vnum not assigned to this area.\n\r", ch);
    	return FALSE;
    }

    if ( !IS_BUILDER(ch, pArea) )
    {
    	send_to_char( "PEdit: Vnum of an area that you can't edit.\n\r", ch );
    	return FALSE;
    }

    if (ch->pcdata->security < MIN_PEDIT_SECURITY)
    {
        send_to_char("PEdit: Insufficient security to create MobProgs.\n\r", ch);
        return FALSE;
    }

    pMcode		= new_mpcode();
    pMcode->vnum	= value;
    pMcode->next	= mprog_list;
    pMcode->changed	= TRUE;
    mprog_list		= pMcode;

    set_editor(ch->desc,ED_PROG,pMcode);

    send_to_char("Program created.\n\r",ch);

    return TRUE;
}

MPEDIT(pedit_show)
{
    MPROG_CODE *pMcode;
    char buf[MAX_STRING_LENGTH];


    EDIT_PROG(ch,pMcode);

    sprintf(buf,
           "Vnum:       [%d]\n\r"
           "Code:\n\r%s\n\r",
           pMcode->vnum, pMcode->code);
    send_to_char(buf, ch);

    return FALSE;
}

void do_mplist( CHAR_DATA *ch, char *argument )
{
    int count;
    bool fAll = FALSE;
    MPROG_CODE *mprg;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    buffer=new_buf();

    if (IS_IMMORTAL(ch) && !str_cmp(argument,"all"))
    	fAll = TRUE;

    for (count =1, mprg= mprog_list; mprg !=NULL; mprg = mprg->next)
    {
	if (fAll
	|| (mprg->vnum >= ch->in_room->area->min_vnum
	&&  mprg->vnum <= ch->in_room->area->max_vnum))
	{
		sprintf(buf, "[%3d] %5d ", count, mprg->vnum );
		add_buf(buffer, buf);
		if ( count % 2 == 0 )
			add_buf( buffer, "\n\r" );
		count++;
	}
    }

    if ( count % 2 == 0 )
    	add_buf( buffer, "\n\r" );

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);

    return;
}
