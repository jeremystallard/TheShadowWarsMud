#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "interp.h"
#include "tables.h"
#include "olc.h"
#include "lookup.h"
#include "recycle.h"

#define HEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)

extern HELP_AREA * had_list;

const struct olc_cmd_type hedit_table[] =
{
/*	{	command		function	}, */

	{	"keyword",	hedit_keyword	},
	{	"text",		hedit_text	},
	{	"new",		hedit_new	},
	{	"level",	hedit_level	},
	{	"commands",	show_cmd_commands	},
	{	"delete",	hedit_delete	},
	{	"list",		hedit_list	},
	{	"show",		hedit_show	},
	{	"?",		show_help	},
	{	NULL,		0		}
};

HELP_AREA * get_help_area( HELP_DATA *help )
{
	HELP_AREA * temp;
	HELP_DATA * thelp;

	for ( temp = had_list; temp; temp = temp->next )
		for ( thelp = temp->first; thelp; thelp = thelp->next_area )
			if ( thelp == help )
				return temp;

	return NULL;
}

HEDIT(hedit_show)
{
	HELP_DATA * help;
	char buf[MSL*2];

	EDIT_HELP(ch, help);

	sprintf( buf, "Keyword : [%s]\n\r"
		      "Level   : [%d]\n\r"
		      "Text    :\n\r"
		      "%s-FIN-\n\r",
		      help->keyword,
		      help->level,
		      help->text );
	send_to_char( buf, ch );

	return FALSE;
}

HEDIT(hedit_level)
{
	HELP_DATA *help;
	int lev;

	EDIT_HELP(ch, help);

	if ( IS_NULLSTR(argument) || !is_number(argument) )
	{
		send_to_char( "Syntax : level [-1..MAX_LEVEL]\n\r", ch );
		return FALSE;
	}

	lev = atoi(argument);

	if ( lev < -1 || lev > MAX_LEVEL )
	{
		printf_to_char( ch, "HEdit : levels between -1 and %d only.\n\r", MAX_LEVEL );
		return FALSE;
	}

	help->level = lev;
	send_to_char( "Ok.\n\r", ch );
	return TRUE;
}

HEDIT(hedit_keyword)
{
	HELP_DATA *help;

	EDIT_HELP(ch, help);

	if ( IS_NULLSTR(argument) )
	{
		send_to_char( "Syntax : keyword [keywords]\n\r", ch );
		return FALSE;
	}

	free_string(help->keyword);
	help->keyword = str_dup(argument);

	send_to_char( "Ok.\n\r", ch );
	return TRUE;
}

HEDIT(hedit_new)
{
	char arg[MIL], fullarg[MIL];
	HELP_AREA *had;
	HELP_DATA *help;
	extern HELP_DATA *help_last;

	if ( IS_NULLSTR(argument) )
	{
		send_to_char( "Syntax : new [name]\n\r", ch );
		send_to_char( "           new [area] [name]\n\r", ch );
		return FALSE;
	}

	strcpy( fullarg, argument );
	argument = one_argument( argument, arg );

	if ( !(had = had_lookup(arg)) )
	{
		had = ch->in_room->area->helps;
		argument = fullarg;
	}

	if ( help_lookup(argument) )
	{
		send_to_char( "HEdit : help already exists.\n\r", ch );
		return FALSE;
	}

	if (!had) /* el area no tiene helps */
	{
		had		= new_had();
		had->filename	= str_dup(ch->in_room->area->file_name);
		had->area	= ch->in_room->area;
		had->first	= NULL;
		had->last	= NULL;
		had->changed	= TRUE;
		had->next	= had_list;
		had_list	= had;
		ch->in_room->area->helps = had;
		SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
	}

	help		= new_help();
	help->level	= 0;
	help->keyword	= str_dup(argument);
	help->text	= str_dup( "" );

	if (help_last)
		help_last->next	= help;

	if (help_first == NULL)
		help_first = help;

	help_last	= help;
	help->next	= NULL;

	if ( !had->first )
		had->first	= help;
	if ( !had->last )
		had->last	= help;

	had->last->next_area	= help;
	had->last		= help;
	help->next_area		= NULL;

	ch->desc->pEdit		= (HELP_DATA *) help;
	ch->desc->editor	= ED_HELP;

	send_to_char( "Ok.\n\r", ch );
	return FALSE;
}

HEDIT( hedit_text )
{
	HELP_DATA *help;

	EDIT_HELP(ch, help);

	if ( !IS_NULLSTR(argument) )
	{
		send_to_char( "Syntax : text\n\r", ch );
		return FALSE;
	}

	string_append( ch, &help->text );

	return TRUE;
}

void hedit( CHAR_DATA *ch, char *argument)
{
    HELP_DATA * pHelp;
    HELP_AREA *had = NULL;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument( argument, command);


    EDIT_HELP(ch, pHelp);

    if (ch->pcdata->security < 1)
    {
        send_to_char("HEdit: Insufficient Security to edit helps.\n\r",ch);
	edit_done(ch);
	return;
    }

    had = get_help_area(pHelp);
    if (had == NULL)
    {
    	bugf( "hedit : had stops help %s NULL", pHelp->keyword );
    	edit_done(ch);
    	return;
    }

    if (command[0] == '\0')
    {
	hedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done") )
    {
        edit_done(ch);
        return;
    }

    for (cmd = 0; hedit_table[cmd].name != NULL; cmd++)
    {
	if (!str_prefix(command, hedit_table[cmd].name) )
	{
		if ((*hedit_table[cmd].olc_fun) (ch, argument))
			had->changed = TRUE;
		return;
	}
    }
    interpret(ch, arg);
    return;
}

void do_hedit(CHAR_DATA *ch, char *argument)
{
	HELP_DATA * pHelp;
        int cmd;
        char command[MAX_INPUT_LENGTH];
        char arg[MAX_INPUT_LENGTH];
        HELP_AREA *had = NULL;

	if ( IS_NPC(ch) )
		return;

        if ( IS_NULLSTR(argument) )
        {
    	    send_to_char( "Syntax  :  HEdit [help]\n\r", ch );
    	    send_to_char( "           HEdit new [help]\n\r", ch );
	    send_to_char( "           HEdit list all/area\n\r", ch);
    	    send_to_char( "           HEdit delete [help]\n\r", ch );
       	    return;
        }

       smash_tilde(argument);
       strcpy(arg, argument);
       argument = one_argument( argument, command);

        for (cmd = 0; hedit_table[cmd].name != NULL; cmd++)
        {
	    if (!str_prefix(command, hedit_table[cmd].name) )
	    {
		    if ((*hedit_table[cmd].olc_fun) (ch, argument))
			had->changed = TRUE;
		    return;
	    }
        }

	if ( (pHelp = help_lookup( arg )) == NULL )
	{
		send_to_char( "HEdit : Help doesn't exist.\n\r", ch );
		return;
	}

	ch->desc->pEdit		= (void *) pHelp;
	ch->desc->editor	= ED_HELP;

	return;
}

HEDIT(hedit_delete)
{
	HELP_DATA * pHelp, * temp;
	HELP_AREA * had;
	DESCRIPTOR_DATA *d;
	bool found = FALSE;

	EDIT_HELP(ch, pHelp);

	for ( d = descriptor_list; d; d = d->next )
		if ( d->editor == ED_HELP && pHelp == (HELP_DATA *) d->pEdit )
			edit_done(d->character);

	if (help_first == pHelp)
		help_first = help_first->next;
	else
	{
		for ( temp = help_first; temp; temp = temp->next )
			if ( temp->next == pHelp )
				break;

		if ( !temp )
		{
			bugf( "hedit_delete : help %s not found in help_first", pHelp->keyword );
			return FALSE;
		}

		temp->next = pHelp->next;
	}

	for ( had = had_list; had; had = had->next )
		if ( pHelp == had->first )
		{
			found = TRUE;
			had->first = had->first->next_area;
		}
		else
		{
			for ( temp = had->first; temp; temp = temp->next_area )
				if ( temp->next_area == pHelp )
					break;

			if ( temp )
			{
				temp->next_area = pHelp->next_area;
				found = TRUE;
				break;
			}
		}

	if ( !found )
	{
		bugf( "hedit_delete : help %s not found in had_list", pHelp->keyword );
		return FALSE;
	}

	free_help(pHelp);

	send_to_char( "Ok.\n\r", ch );
	return TRUE;
}

HEDIT(hedit_list)
{
	char buf[MIL];
	int cnt = 0;
	HELP_DATA *pHelp;
	BUFFER *buffer;

	EDIT_HELP(ch, pHelp);

	if ( !str_cmp( argument, "all" ) )
	{
		buffer = new_buf();

		for ( pHelp = help_first; pHelp; pHelp = pHelp->next )
		{
			sprintf( buf, "%3d. %-14.14s%s", cnt, pHelp->keyword,
				cnt % 4 == 3 ? "\n\r" : " " );
			add_buf( buffer, buf );
			cnt++;
		}

		if ( cnt % 4 )
			add_buf( buffer, "\n\r" );

		page_to_char( buf_string(buffer), ch );
		return FALSE;
	}

	if ( !str_cmp( argument, "area" ) )
	{
		if ( ch->in_room->area->helps == NULL )
		{
			send_to_char( "You can't put helps here.\n\r", ch );
			return FALSE;
		}

		buffer = new_buf();

		for ( pHelp = ch->in_room->area->helps->first; pHelp; pHelp = pHelp->next_area )
		{
			sprintf( buf, "%3d. %-14.14s%s", cnt, pHelp->keyword,
				cnt % 4 == 3 ? "\n\r" : " " );
			add_buf( buffer, buf );
			cnt++;
		}

		if ( cnt % 4 )
			add_buf( buffer, "\n\r" );

		page_to_char( buf_string(buffer), ch );
		return FALSE;
	}

	if ( IS_NULLSTR(argument) )
	{
		send_to_char( "Syntax : list all\n\r", ch );
		send_to_char( "           list area\n\r", ch );
		return FALSE;
	}

	return FALSE;
}

char *color2web(char *text)
{
  char *ptr = (char *)text;
  int i=0;
  int cnt=0;
  
  if (IS_NULLSTR(text))
    return NULL;
  
  for (i=0; i < strlen(ptr); i++) {
    if (ptr[i-1] == '{') {
	 switch (ptr[i]) {
	 case 'b':
	   ptr = string_replace(ptr, "{b", "</FONT><FONT COLOR=\"darkblue\">");
	   cnt++;
	   break;
	 case 'B':
	   ptr = string_replace(ptr, "{B", "</FONT><FONT COLOR=\"blue\">");
	   cnt++;
	   break;
	 case 'c':
	   ptr = string_replace(ptr, "{c", "</FONT><FONT COLOR=\"darkcyan\">");
	   cnt++;
	   break;
	 case 'C':
	   ptr = string_replace(ptr, "{C", "</FONT><FONT COLOR=\"cyan\">");
	   cnt++;
	   break;
	 case 'g':
	   ptr = string_replace(ptr, "{g", "</FONT><FONT COLOR=\"darkgreen\">");
	   cnt++;
	   break;
	 case 'G':
	   ptr = string_replace(ptr, "{G", "</FONT><FONT COLOR=\"green\">");
	   cnt++;
	   break;
	 case 'm':
	   ptr = string_replace(ptr, "{m", "</FONT><FONT COLOR=\"darkmagenta\">");
	   cnt++;
	   break;
	 case 'M':
	   ptr = string_replace(ptr, "{M", "</FONT><FONT COLOR=\"magenta\">");
	   cnt++;
	   break;
	 case 'r':
	   ptr = string_replace(ptr, "{r", "</FONT><FONT COLOR=\"darkred\">");
	   cnt++;
	   break;
	 case 'R':
	   ptr = string_replace(ptr, "{R", "</FONT><FONT COLOR=\"red\">");
	   cnt++;
	   break;
	 case 'w':
	   //ptr = string_replace(ptr, "{w", "</FONT><FONT COLOR=\"darkgrey\">");
           ptr = string_replace(ptr, "{w", "</FONT>");
	   cnt++;
	   break;
	 case 'W':
	   ptr = string_replace(ptr, "{W", "</FONT><FONT COLOR=\"white\">");
	   cnt++;
	   break;
	 case 'y':
	   ptr = string_replace(ptr, "{y", "</FONT><FONT COLOR=\"goldenrod\">");
	   cnt++;
	   break;
	 case 'Y':
	   ptr = string_replace(ptr, "{Y", "</FONT><FONT COLOR=\"yellow\">");
	   cnt++;
	   break;
	 case 'D':  
	   ptr = string_replace(ptr, "{D", "</FONT><FONT COLOR=\"darkgray\">");
	   cnt++;
	   break;
	 case 'x':
	   ptr = string_replace(ptr, "{x", "</FONT>");
	   cnt++;
	   break;
	 default:
	   break;
	 }
    }    
  }

  return ptr;
}

#define WEBHELP_DIR "../webhelp/"

void do_help2web(CHAR_DATA *ch, char *argument)
{
  char buf[MIL];
  char _keyword[MSL];
  int cnt = 0;
  int icnt=0;
  int acnt=0;
  int i=0;
  int level=0;
  HELP_DATA *pHelp;
  FILE *fp;
  FILE *ifp;
  char filename[MSL];
  char index_filename[MSL];
  char *ptr=NULL;

  fclose (fpReserve);
   

  // Copy all help entries to one dedicated help html file
  // Use first keyword
  for ( pHelp = help_first; pHelp; pHelp = pHelp->next ) {
    if (IS_NULLSTR(pHelp->keyword))
	 continue;

    cnt++;

    sprintf(_keyword, "%s", pHelp->keyword);
    ptr = strtok(_keyword, " ");
    if (ptr != NULL)
	 sprintf(filename, "%s%s.html", WEBHELP_DIR, ptr);
    else
	 sprintf(filename, "%s%s.html", WEBHELP_DIR, pHelp->keyword);
    
    fp =  fopen(filename, "w");
    
    fprintf(fp, "<BODY TEXT=\"darkgray\" BGCOLOR=\"black\">");
    fprintf(fp, "<FONT FACE=\"Courier\"><B>\n");
    fprintf(fp, "<PRE><H1>%s</H1>", pHelp->keyword);
    fprintf(fp, "%s\n", color2web(pHelp->text));  
    fprintf(fp, "</FONT></B></PRE>\n");
    fprintf(fp, "</BODY>\n");   
    fclose(fp);
  }

  // Make index file
  sprintf(index_filename, "%s%s.html", WEBHELP_DIR, "index");
  ifp = fopen(index_filename, "w");
  
  fprintf(ifp, "<BODY TEXT=\"darkgray\" BGCOLOR=\"black\" LINK=\"darkgray\">");
  fprintf(ifp, "<FONT FACE=\"Courier\"><B>\n");
  fprintf(ifp, "<PRE><H1>The Shadow <FONT COLOR=\"darkred\">Wars</FONT> Help files</H1>");
  fprintf(ifp, "Created with help2web Copyright &copy; 1995-2004 by The Shadow Wars\n");
  fprintf(ifp, "Last updated at <FONT COLOR=\"goldenrod\">%s</FONT>\n", (char *) ctime(&current_time));
  
  // Loop through, make Index for each starting char A-Z
  for (i=65; i < 91; i++) {
    if (acnt++ > 1)
	 fprintf(ifp, "\n");
    fprintf(ifp, "<H1><FONT COLOR=\"goldenrod\">%c</FONT></H1>\n", i);
    for ( pHelp = help_first; pHelp; pHelp = pHelp->next ) {
	 
	 // Only list mortal helps
	 level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;
	 if (level > 90)
	   continue;
	 
	 if (IS_NULLSTR(pHelp->keyword))
	   continue;
	 
	 sprintf(_keyword, "%s", pHelp->keyword);
	 if ((char)i == UPPER(_keyword[0])) {
	   icnt++;
	   ptr = strtok(_keyword, " ");
	   if (ptr != NULL)
		sprintf(filename, "%s%s.html", WEBHELP_DIR, ptr);
	   else
		sprintf(filename, "%s%s.html", WEBHELP_DIR, pHelp->keyword);
	   
	   sprintf(buf, "<a href=\"http://www.shadowwars.org/webhelp/%s\" STYLE=\"TEXT-DECORATION:NONE\">%-26.26s</a>%s", filename, _keyword, icnt % 4 == 3? "\n" : "  ");
	   fprintf(ifp, buf);
	 }	 
    }
    fprintf(ifp, "\n");
  }
  
  // Other help entries not found with A-Z
  icnt=0;
  fprintf(ifp, "\n");
  fprintf(ifp, "<H1><FONT COLOR=\"goldenrod\">Misc</FONT></H1>\n");
  for ( pHelp = help_first; pHelp; pHelp = pHelp->next ) {
    level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;
    if (level > 90)
	 continue;
    
    if (IS_NULLSTR(pHelp->keyword))
	 continue;
    
    sprintf(_keyword, "%s", pHelp->keyword);
    if (UPPER(_keyword[0]) < 65 || UPPER(_keyword[0]) > 91) {
	 icnt++;	 
	 ptr = strtok(_keyword, " ");
	 if (ptr != NULL)
	   sprintf(filename, "%s%s.html", WEBHELP_DIR, ptr);
	 else
	   sprintf(filename, "%s%s.html", WEBHELP_DIR, pHelp->keyword);
	 
	 sprintf(buf, "<a href=\"http://www.shadowwars.org/webhelp/%s\" STYLE=\"TEXT-DECORATION:NONE\">%-26.26s</a>%s", filename, pHelp->keyword, icnt % 4 == 3? "\n" : "  ");
	 fprintf(ifp, buf);
    }
  }	 
  
  fprintf(ifp, "</FONT></B></PRE>\n");
  fprintf(ifp, "</BODY>\n");   
  fclose(ifp);

  fpReserve = fopen( NULL_FILE, "r" );

  sprintf(buf, "%d help entries written to html files.\n\r", cnt);
  send_to_char(buf, ch);

  return;
}
