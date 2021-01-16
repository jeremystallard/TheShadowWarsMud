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

#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "olc.h"

int		total_polls	= 0;
POLL_DATA	*poll_first;

/* local functions */
void		poll_vote args( ( CHAR_DATA *ch, POLL_DATA *poll, int choice ) );
void		display_results	args( ( CHAR_DATA *ch, POLL_DATA *poll ) );
bool		can_edit_poll args( ( CHAR_DATA *ch, POLL_DATA *poll ) );
bool		has_voted args( ( CHAR_DATA *ch, POLL_DATA *poll ) );
long		get_raw_time args( ( char *date ) );
int		days_left args( ( POLL_DATA *poll ) );
POLL_DATA *	new_poll args( ( void ) );
POLL_DATA *	get_poll_data args( ( int num ) ) ;
void		poll_compact args( ( POLL_DATA *poll ) );
char	*	format_output args( ( char *string, int spaces ) );

/*
 * poll_vote - actual voting function
 * processes the players vote to see if it is valid
 * and to actually apply the vote
 */
void poll_vote ( CHAR_DATA *ch, POLL_DATA *poll, int choice )
{
    char list[MSL];

    if ( poll != NULL )
    {
	if ( poll->open > current_time || poll->close < current_time )
	{
	    send_to_char( "That poll is not currently open for voting.\n\r", ch );
	    return;
	}

	if ( has_voted( ch, poll ) )
	{
	    send_to_char( "You have already voted on this poll.\n\r", ch );
	    return;
	}

	if ( choice <= 0 || choice > MAX_CHOICE
	|| IS_NULLSTR(poll->choice[choice-1]) )
	{
	    send_to_char( "That is not a valid choice to cast your vote on.\n\r", ch );
	    return;
	}

	poll->votes[choice-1]++; // add vote to poll data record.

	
    // add poll unique id to pfile to ensure the player can't vote again
	sprintf( list, "%s%s%d", ch->pcdata->polls,
	    !IS_NULLSTR(ch->pcdata->polls) ? "|" : "", poll->id );
	free_string( ch->pcdata->polls );
	ch->pcdata->polls = str_dup( list );

	send_to_char( "Thank you for your vote.\n\r",ch);

	if (IS_IMMORTAL(ch))
	   display_results( ch, poll );
    }

    return;
}

/*
 * do_poll - main poll function
 * used to pass all poll related requests to sub functions.
 */
void do_poll ( CHAR_DATA *ch, char *argument )
{
    POLL_DATA *poll = NULL;
    char buf[MSL];
    char arg1[MIL];
    char arg2[MIL];
    char arg3[MIL];
    int selected, choice;

    if ( IS_NPC(ch) )
    {
	send_to_char("Mobs can't vote.\n\r", ch);
	return;
    }

    // show the list of polls, or how many new polls are available
    if ( argument[0] == '\0' || !str_cmp( argument, "new" ) )
    {
	POLL_DATA *loop;
	char left[MSL];
	int poll_num = 0;
	int new_count = 0;
	bool found = FALSE;

        for ( loop = poll_first; loop != NULL; loop = loop->next )
        {
	    left[0] = '\0';
	    poll_num++;

	    if ( IS_SET( loop->flags, POLL_PUBLISHED )
	    && !IS_SET( loop->flags, POLL_HIDDEN )
	    && !IS_SET( loop->flags, POLL_DELETE ) )
	    {
		/*
		if ( !str_prefix( loop->to, "implementors" ) && get_trust(ch) < CREATOR )
		    continue;
		else if ( !str_prefix( loop->to, "immortals" ) && get_trust(ch) < LEVEL_IMMORTAL )
		    continue;
		*/
		if (!is_poll_to(ch,loop))
		    continue;

		if ( !str_cmp( argument, "new" ) && has_voted(ch, loop) )
		    continue;
		else if ( !str_cmp( argument, "new" ) )
		{
		    new_count++;
		    found = TRUE;
		    continue;
		}

	        if ( !found )
		    send_to_char( "Available Polls:\n\r", ch );

		if ( days_left(loop) > 0 && !has_voted(ch, loop) )
		    sprintf( left, " (%d days left)", days_left(loop) );

	        sprintf( buf, "%3d%s) %s%s\n\r", poll_num,
		    !has_voted(ch, loop) ? "N" : " ", format_output(loop->topic, 6),
			left[0] != '\0' ? left : "" );
	        send_to_char( buf, ch );
	        found = TRUE;
	    }
        }

	if ( !found )
	    send_to_char("There are no new polls available to vote on.\n\r", ch);
	else if ( new_count > 0 )
	{
	    sprintf( buf, "{YThere %s {R%d {Ynew poll%s to vote on.{x\n\r",
		new_count > 1 ? "are" : "is", new_count,
		new_count > 1 ? "s" : "" );
	    send_to_char( buf, ch );
	}

	return;
    }

    /*
     * Removing this will void the license agreement.
	 * Be kind and give credit where credit is due.
	 */
    if ( !str_prefix( argument, "credits" ) )
    {
	send_to_char( POLL_CREDITS, ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    // check for valid syntax
    if ( !is_number(arg1) || ( arg3[0] != '\0' && !is_number(arg3) ) )
    {
	send_to_char( "Syntax: poll [poll #] vote [choice #]\n\r", ch );
	send_to_char( "        poll [poll #]\n\r", ch );
	send_to_char( "        poll [poll #] results\n\r", ch );
	return;
    }

    selected = atoi(arg1);
    choice = atoi(arg3);

    // get the actual poll data
    if ( ( poll = get_poll_data(selected) ) == NULL
    || !IS_SET(poll->flags, POLL_PUBLISHED)
    || IS_SET(poll->flags, POLL_DELETE)
    || IS_SET(poll->flags, POLL_HIDDEN) )
    {
	send_to_char( "That is an invalid poll selection.\n\r", ch );
	return;
    }

/*
    if ( ( !str_prefix( poll->to, "implementors" ) && get_trust(ch) < CREATOR )
    || ( !str_prefix( poll->to, "immortals" ) && get_trust(ch) < LEVEL_IMMORTAL ) )
*/
    if (!is_poll_to(ch,poll))
    {
	send_to_char( "That is an invalid poll selection.\n\r", ch );   
	return;
    }

    if ( arg2[0] != '\0' && !str_prefix( arg2, "results" ) )
    {
	if (IS_IMMORTAL(ch) || current_time > poll->close)
        {
		display_results( ch, poll );
	}
	else
	{
		send_to_char("The results of the poll will be revealed when the poll closes.\n\r",ch);
	}
    }
    else if ( arg2[0] != '\0' && !str_prefix( arg2, "vote" ) )
	poll_vote( ch, poll, choice );
    else
    {
	sprintf( buf, "[%3d] %s\n\r", selected, format_output(poll->topic, 6) );
	send_to_char( buf, ch );
	send_to_char( "\n\rAvailable voting choices are:\n\r", ch );

	for ( choice = 0; choice < MAX_CHOICE; choice++ )
	{
	    if ( !IS_NULLSTR(poll->choice[choice]) )
	    {
		sprintf( buf, "%2d) %s\n\r",
		    choice + 1, format_output(poll->choice[choice], 4) );
		send_to_char( buf, ch );
	    }
	}

	send_to_char( "\n\rVoting Syntax: poll <poll #> vote <choice #>\n\r", ch );
    }

    return;
}

/*
 * display_results - shows the player the results of the
 * selected poll, including % of votes cast.
 */
void display_results ( CHAR_DATA *ch, POLL_DATA *poll )
{
    char buf[MSL];
    int count, len;
    int votes = 0;

    if ( poll == NULL || poll->topic == NULL )
    {
	send_to_char( "No such poll.\n\r", ch );
	return;
    }

    for ( count = 0; count < MAX_CHOICE; count++ )
	votes += poll->votes[count];

    sprintf( buf, "Results for poll: %s\n\r\n\r", format_output(poll->topic, 18) );
    send_to_char( buf, ch );

    sprintf( buf, "[ Choice ]-----------------------------------------------------[  Votes  ]\n\r" );
    send_to_char( buf, ch );

    for ( count = 0; count < MAX_CHOICE; count++ )
    {
	if ( !IS_NULLSTR(poll->choice[count]) )
	{
	    if ( strlen(poll->choice[count]) > 57 )
		len = 54;
	    else
		len = 57;

	    sprintf( buf, "%2d) %-*.*s%s  %4d (%2d%%)\n\r",
		count + 1, len, len, poll->choice[count], len == 54 ? "..." : "",
		poll->votes[count], votes > 0 ? (poll->votes[count]*100)/votes : 0 );
	    send_to_char( buf, ch );
	}
    }

    sprintf( buf, "\n\rTotal Votes: %d\n\r", votes );
    send_to_char( buf, ch );

    return;
}

/*
 * load_polls - gets the poll data from the 'polls.dat'
 * file in the DATA_DIR.
 */
void load_polls( void )
{
    FILE *fp;
    POLL_DATA *poll = NULL;
    char buf[MSL];
    char *string;
    int choice, vote;
    bool fMatch = FALSE;
    
    sprintf( buf, "%s%s", DATA_DIR, POLL_FILE );
    
    if ( ( fp = fopen( buf, "r" ) ) == NULL )
    {
	sprintf( buf, "Error: %s file not found!", POLL_FILE );
        log_string(buf);
        return;
    }

    poll_first	= NULL;
    top_poll	= 0;
    choice	= 0;
    vote	= 0;

    for (;;)
    {
	string = feof(fp) ? "End" : fread_word(fp);
    
	if ( !str_cmp( string, "End" ) )
            break;
    
	if ( !str_cmp( string, "Topic" ) )
	{
	    POLL_DATA *loop;

	    poll	= new_poll( );
	    choice	= 0;
	    top_poll++;

	    if ( poll_first == NULL )
		poll_first = poll;

	    for ( loop = poll_first; loop != NULL; loop = loop->next )
	    {
		if ( loop == poll )
		   break;

		if ( loop->next == NULL )
		   loop->next = poll;
	    }
	}

	switch ( UPPER(string[0]) )
	{
            case 'T':
		KEY("Topic",	poll->topic,		fread_string(fp) );
		KEY("To",	poll->to,		fread_string(fp) );
		break;
    
            case 'C':
		KEY("Creator",	poll->creator,		fread_string(fp) );
		KEY("Created",	poll->created,		fread_number(fp) );
		KEY("Close",	poll->close,		fread_number(fp) );

		if ( !str_cmp( string, "Ch" ) )
		{
                    if ( choice < MAX_CHOICE )
		    {
                        poll->choice[choice++] = str_dup( fread_string(fp) );
                        fMatch = TRUE;
		    }
		}
		break;

	    case 'I':
		KEY("Id",	poll->id,		fread_number(fp) );
		break;

	    case 'F':
		KEY("Flags",	poll->flags,		fread_flag(fp) );
		break;

	    case 'O':
		KEY("Open",	poll->open,		fread_number(fp) );
		break;

	    case 'V':
		if ( !str_cmp( string, "Votes" ) )
		{
		    for( vote = 0; vote < MAX_CHOICE; vote++ )
			poll->votes[vote]	= fread_number(fp);

		    fMatch = TRUE;
		}
		break;
	}
    }
                    
    if ( !fMatch && poll_first != NULL )
    {
	bug( "load_polls: no match", 0 );
	fread_to_eol(fp);
    }
                 
    fclose(fp);

    return;
}

/*
 * save_polls - used to write the poll data to the 'polls.dat'
 * file in the DATA_DIR. Also calls the compact function to
 * ensure that no 'blank' choices are left in between other
 * choices. Blanks can cause incorrect totals.
 */
void save_polls( CHAR_DATA *ch, char *argument )
{
    FILE *fp;
    POLL_DATA *poll;
    char buf[MIL];
    int i;

    sprintf( buf, "%s%s", DATA_DIR, POLL_FILE );

    if ( !( fp = fopen( buf, "w+" ) ) )
    {
        bug("Open polls: fopen", 0);
        return;
    }

    for ( poll = poll_first; poll != NULL; poll = poll->next )
    {
        if ( poll->topic == NULL || IS_SET( poll->flags, POLL_DELETE ) )
            continue;

	if ( IS_SET( poll->flags, POLL_CHANGED ) )
	    REMOVE_BIT( poll->flags, POLL_CHANGED );
    
	poll_compact(poll);

        fprintf( fp, "\nTopic %s~\n", ( poll->topic == NULL ) ? "" : poll->topic );
	fprintf( fp, "To %s~\n", ( poll->to == NULL ) ? "" : poll->to );
	fprintf( fp, "Creator %s~\n", ( poll->creator == NULL ) ? "" : poll->creator );
	fprintf( fp, "Open %ld\n", poll->open );
	fprintf( fp, "Close %ld\n", poll->close );
	fprintf( fp, "Created %ld\n", poll->created );
	fprintf( fp, "Flags %s\n", print_flags(poll->flags) );
	fprintf( fp, "Id %d\n", poll->id );

        fprintf( fp, "Votes" );
        for ( i = 0; i < MAX_CHOICE; i++ )
            fprintf( fp, " %3d", poll->votes[i] );
        fprintf( fp, "\n" );
        
        i = 0;
        while ( i < MAX_CHOICE )
        {
	    if ( !IS_NULLSTR(poll->choice[i]) )
                fprintf( fp, "Ch %s~\n", poll->choice[i] );

	    i++;
        }
    }
  
    fprintf( fp, "\nEnd\n" );
         
    fclose( fp );
    if ( ch )
        send_to_char( "Polls file written.\n\r", ch );
        
    return;
}

POLL_DATA * poll_free;
      
/*
 * new_poll - create and allocate memory for a new
 * poll in memory. full poll data initialization
 * is done here.
 */
POLL_DATA * new_poll ( void )
{
    POLL_DATA * poll;
    int i;
              
    if ( poll_free )
    {  
        poll		= poll_free;
        poll_free	= poll_free->next;
    }
    else
        poll		= alloc_perm( sizeof( *poll ) );
 
    VALIDATE(poll);
    poll->next		= NULL;
    poll->topic		= NULL;
    poll->to		= str_dup( "" );
    poll->creator	= str_dup( "" );
    poll->open		= 0;
    poll->close		= 0;
    poll->created	= 0;
    poll->flags		= 0;
    poll->id		= 0;

    for ( i = 0; i < MAX_CHOICE; i++ )
    {
        poll->choice[i]	= str_dup( "" );
        poll->votes[i]	= 0;
    }
        
    return poll;
}

/*
 * do_polledit - OLC style editor for poll data
 * I'm not going to go into each fucntion here,
 * read the PEDIT help in the text file.
 */
void do_polledit(CHAR_DATA *ch, char *argument)
{
    POLL_DATA *poll;
    char arg[MIL];
    int num = 0;

    if ( IS_NPC(ch) )
        return;
    
    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax : PEdit [poll #]\n\r", ch );
        return;
    }

    if (ch->pcdata->security < 9)
    {
        send_to_char( "PEdit : Insufficient security to edit polls.\n\r", ch );
        return;
    }
    
    argument = one_argument( argument, arg );
    
    if ( !str_cmp( arg, "create" ) )
    {
        if ( ch->pcdata->security < 9 )
        {
          send_to_char( "PEdit : Insuffecient security to create new polls.\n\r",ch);
          return;
        }
    
        if ( argument[0] == '\0' )
        {
          send_to_char( "PEdit: Please specify a topic for the poll.\n\r",ch );
          return;
        }

        polledit_create( ch, argument );
        ch->desc->editor = ED_POLL;
        return;
    }
    else if ( !str_cmp( arg, "list" ) )
    {
	polledit_list( ch, "" );
	return;
    }
    else
    {
        if ( is_number( arg ) )
            num = atoi( arg );
           
        if ( num <= 0 || num > top_poll
	|| ( poll = get_poll_data( num ) ) == NULL )
        {
            send_to_char("PEdit: That poll does not exist.\n\r", ch);
            return;
        }
        
        if ( poll->topic == NULL || poll->topic[0] == '\0' )
        {
            send_to_char( "PEdit: That poll isn't defined yet.\n\r", ch );
            return;
        }
    }     
        
    ch->desc->pEdit = poll;
    ch->desc->editor = ED_POLL;
    return;
}

void polledit( CHAR_DATA *ch, char *argument )
{
    POLL_DATA *poll;
    char arg[MIL];
    char command[MIL];
    int cmd;
            
    strcpy(arg, argument);
    argument = one_argument( argument, command );
     
    EDIT_POLL(ch, poll);
    if (ch->pcdata->security < 9)
    {
        send_to_char("GrEdit: Insufficient security to edit groups.\n\r",ch);
        edit_done(ch);
        return;
    }
    
    if (command[0] == '\0')
    {
        polledit_show(ch, argument);
        return;
    }
    
    if ( !str_cmp(command, "done") )
    {
	if ( IS_SET(poll->flags, POLL_CHANGED) )
	    save_polls( ch, "" );

        edit_done(ch);
        return;
    }

    if ( !str_cmp(command, "help") && argument[0] == '\0' )
    {
	do_help( ch, "polledit" );
	return;
    }

    for (cmd = 0; polledit_table[cmd].name != NULL; cmd++)
    {
        if ( !str_prefix(command, polledit_table[cmd].name) )
        {
	   bool changed;

           changed = (*polledit_table[cmd].olc_fun) (ch, argument);

	   if ( changed )
		SET_BIT(poll->flags, POLL_CHANGED);

           return;
        }
    }
    
    interpret(ch, arg);
    return;
}

PEDIT( polledit_create )
{
    POLL_DATA *poll, *loop;
         
    if ( argument[0] == '\0' )
    {
        send_to_char("Syntax: polledit create [topic]\n\r",ch);
        return FALSE;
    }
     
    for ( loop = poll_first; loop != NULL; loop = loop->next )
    {
        if ( !str_cmp( loop->topic, argument ) )
        {
            send_to_char("There is already a poll with that topic.\n\r", ch );
            return FALSE;
        }
    }

    poll = new_poll();
    top_poll++;
        
    if ( poll_first == NULL )
         poll_first = poll;

    for ( loop = poll_first; loop != NULL; loop = loop->next )
    {
        if ( loop == poll )
            break;

        if ( loop->next == NULL )
	{
	    loop->next = poll;
	    poll->id = loop->id + 1;
	}
    }

    free_string( poll->topic );
    poll->topic			= str_dup( argument );
    free_string( poll->creator );
    poll->creator		= str_dup( ch->name );
    poll->created		= current_time;

    ch->desc->pEdit             = (void *)poll;

    send_to_char("New Poll Created.\n\r",ch);
    return TRUE;
}

PEDIT( polledit_show )
{
    POLL_DATA *poll;
    char buf[MSL];
    int i;

    EDIT_POLL(ch,poll);

    sprintf( buf, "Topic: %s [%d]%s\n\r",
        format_output(poll->topic, 7), poll->id,
	IS_SET(poll->flags, POLL_DELETE) ? " (DEL)" : "" );
    send_to_char( buf, ch );

    sprintf( buf, "Created by: %s on %.24s MST\n\r",
	poll->creator, ctime(&poll->created) );
    send_to_char( buf, ch );

    sprintf( buf, "To: %s\n\r",
	IS_NULLSTR(poll->to) ? "(none)" : poll->to );
    send_to_char( buf, ch );

    sprintf( buf, "Poll Opens:  %.24s MST\n\r",
	ctime(&poll->open) );
    send_to_char( buf, ch );

    sprintf( buf, "Poll Closes: %.24s MST\n\r",
	ctime(&poll->close) );
    send_to_char( buf, ch );

    sprintf( buf, "Published: %s\n\r",
	IS_SET(poll->flags, POLL_PUBLISHED) ? "Yes" : "No" );
    send_to_char( buf, ch );

    send_to_char( "\n\rPoll Choices:\n\r", ch );

    for ( i = 0; i < MAX_CHOICE; i++ )
    {
	sprintf( buf, "%2d) %s\n\r",
	    (i+1), IS_NULLSTR(poll->choice[i]) ? "(none)" :
	    format_output(poll->choice[i], 4) );
	send_to_char( buf, ch );
    }

    return FALSE;
}

PEDIT( polledit_topic )
{
    POLL_DATA *poll;
    EDIT_POLL(ch,poll);

    if ( !can_edit_poll( ch, poll ) )
	return FALSE;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Syntax: topic [new topic]\n\r", ch );
	return FALSE;
    }

    free_string( poll->topic );
    poll->topic = str_dup( argument );
    send_to_char( "Topic set.\n\r", ch );

    return FALSE;
}

PEDIT( polledit_to )
{
    POLL_DATA *poll;
    EDIT_POLL(ch,poll);

    if ( !can_edit_poll( ch, poll ) )
        return FALSE;

    if ( argument[0] == '\0' )
    {
	send_to_char( "PEdit: to <all|immortals|implementors>\n\r", ch );
	send_to_char( "       to clear\n\r", ch );
	return FALSE;
    }

    if ( !str_cmp( argument, "clear" ) )
    {
	free_string( poll->to );
	poll->to = str_dup( "" );
	send_to_char( "To string cleared.\n\r", ch );
	return TRUE;
    }
    else if ( str_cmp( argument, "all" )
    && str_prefix( argument, "implementors" )
    && str_prefix( argument, "immortals" ) 
    && str_prefix( argument, "whitetower"  )
    && str_prefix( argument, "aessedai" ) 
    && str_prefix( argument, "initiates" ) 
    && str_prefix( argument, "borderguard"  )
    && str_prefix( argument, "children" ) 
    && str_prefix( argument, "lionthrone" ) 
    && str_prefix( argument, "theempire" ) )
    {
	send_to_char( "PEdit: Invalid TO string.\n\r", ch );
	return FALSE;
    }

    free_string( poll->to );
    poll->to = str_dup( argument );
    send_to_char( "To string set.\n\r", ch );
    return TRUE;
}

PEDIT( polledit_open )
{
    POLL_DATA *poll;
    char date[MIL];
    long raw_time = 0;

    EDIT_POLL(ch,poll);

    if ( !can_edit_poll( ch, poll ) )
        return FALSE;

    one_argument( argument, date );

    if ( date[0] == '\0' || strlen(date) < 10
    || !strstr( date, "/" ) )
    {
	send_to_char( "PEdit: open [opening date] (MM/DD/YYYY)\n\r", ch );
	return FALSE;
    }

    if ( ( raw_time = get_raw_time(date) ) <= 0 )
    {
	/* real max date is 01/18/2038 */
	send_to_char( "Invalid date. Date must be between 01/01/1970 and 01/01/2038.\n\r", ch );
	return FALSE;
    }

    poll->open = raw_time;
    send_to_char( "Open date set.\n\r", ch );

    if ( poll->close < poll->open )
    {
	poll->close = raw_time + 86400;	/* open time plus one day */
	send_to_char( "Close date updated.\n\r", ch );
    }

    return TRUE;
}

PEDIT( polledit_close )
{
    POLL_DATA *poll;
    char date[MIL];
    long raw_time = 0;
    
    EDIT_POLL(ch,poll);
    
    if ( !can_edit_poll( ch, poll ) )
        return FALSE;
 
    one_argument( argument, date );

    if ( date[0] == '\0' || strlen(date) < 10
    || !strstr( date, "/" ) )
    {
        send_to_char( "PEdit: close [close date] (MM/DD/YYYY)\n\r", ch );
        return FALSE;
    }
    
    if ( ( raw_time = get_raw_time(date) ) <= 0 )
    {
	/* real max date is 01/18/2038 */
        send_to_char( "Invalid date. Date must be between 01/01/1970 and 01/01/2038.\n\r", ch );
        return FALSE;
    }
    
    poll->close = raw_time;
    send_to_char( "Close date set.\n\r", ch );
    
    if ( poll->open > poll->close )
    {
        poll->open = raw_time - 86400; /* close time minus one day */
        send_to_char( "Open date updated.\n\r", ch );
    }

    return TRUE;
}

PEDIT( polledit_reopen )
{
    POLL_DATA *poll;  
    char date[MIL];
    long raw_time = 0;
    
    EDIT_POLL(ch,poll);
        
    one_argument( argument, date );
    
    if ( date[0] == '\0' || strlen(date) < 10 
    || !strstr( date, "/" ) )
    {
        send_to_char( "PEdit: reopen [new close date] (MM/DD/YYYY)\n\r", ch );
        return FALSE; 
    }
    
    if ( ( raw_time = get_raw_time(date) ) <= 0 )
    {
        /* real max date is 01/18/2038 */
        send_to_char( "Invalid date. Date must be between 01/01/1970 and 01/01/2038.\n\r", ch );
        return FALSE;
    }
    
    if ( raw_time < current_time || raw_time < poll->close )
    {
	send_to_char( "Invalid date. Date must be later than original close date.\n\r", ch );
	return FALSE;
    }

    poll->close = raw_time;
    send_to_char( "Close date set. Poll re-opened for voting.\n\r", ch );
    
    return TRUE;
}


PEDIT( polledit_choice )
{
    POLL_DATA *poll;
    char buf[MSL];
    char arg1[MIL];
    int num;

    EDIT_POLL(ch,poll);

    if ( !can_edit_poll( ch, poll ) )
        return FALSE;

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' || !is_number(arg1) )
    {
	send_to_char( "PEdit: choice [choice #] [text]\n\r", ch );
	send_to_char( "       choice [choice #] clear\n\r", ch );
	return FALSE;
    }

    num = atoi(arg1);

    if ( num <= 0 || num > MAX_CHOICE )
    {
	send_to_char( "Invalid choice. Please use 1 - 10.\n\r", ch );
	return FALSE;
    }

    free_string( poll->choice[num-1] );

    if ( argument[0] == '\0' || !str_cmp( argument, "clear" ) )
    {
	poll->choice[num-1] = str_dup( "" );
	sprintf( buf, "Choice %d cleared.\n\r", num );
	send_to_char( buf, ch );
	return TRUE;
    }

    poll->choice[num-1] = str_dup( argument );
    sprintf( buf, "Choice %d set: \"%s\"\n\r", num, argument );
    send_to_char( buf, ch );
    return TRUE;
}

PEDIT( polledit_publish )
{
    POLL_DATA *poll;
    EDIT_POLL(ch,poll);

    if ( IS_SET(poll->flags, POLL_DELETE) )
    {
	send_to_char( "You can't published a poll that is flagged for deletion.\n\r", ch );
	return FALSE;
    }

    if ( IS_SET(poll->flags, POLL_PUBLISHED)
    && poll->open < current_time
    && get_trust(ch) < MAX_LEVEL )
    {
	send_to_char( "You can not un-publish or modify a poll"
	    " after it has opened for voting.\n\r", ch );
	return FALSE;
    }

    if ( !IS_SET(poll->flags, POLL_PUBLISHED)
    && poll->close < current_time )
    {
	send_to_char( "You can't publish a poll in the past.\n\r", ch );
	return FALSE;
    }

    if ( IS_SET(poll->flags, POLL_PUBLISHED) )
	send_to_char( "Poll is no longer published.\n\r", ch );
    else
	send_to_char( "Poll is now published.\n\r", ch );

    TOGGLE_BIT(poll->flags, POLL_PUBLISHED);
    return TRUE;
}

PEDIT( polledit_delete )
{
    POLL_DATA *poll;
    EDIT_POLL(ch,poll);
    
    if ( IS_SET(poll->flags, POLL_PUBLISHED) )
    {
        send_to_char( "You can not delete a published poll.\n\r", ch );
        return FALSE;
    }
 
    if ( IS_SET(poll->flags, POLL_DELETE) )
        send_to_char( "Poll is no longer flagged for deletion.\n\r", ch );
    else
        send_to_char( "Poll is now flagged for deletion.\n\r", ch );
    
    TOGGLE_BIT(poll->flags, POLL_DELETE);
    return TRUE;
}

PEDIT( polledit_list )
{
    POLL_DATA *loop;
    char buf[MSL];
    int count = 0;

    for ( loop = poll_first; loop != NULL; loop = loop->next )
    {
	sprintf( buf, "%3d) %s%s\n\r", ++count,
	    format_output(loop->topic, 5),
	    IS_SET(loop->flags, POLL_DELETE) ? " (DEL)" : "" );
	send_to_char( buf, ch );
    }

    if ( count > 0 )
	sprintf( buf, "\n\rTotal Polls: %d\n\r", count );
    else
	sprintf( buf, "There are currently no polls defined.\n\r" );
    send_to_char( buf, ch );

    return FALSE;
}

PEDIT( polledit_credits )
{
    send_to_char( POLL_CREDITS, ch );
    return FALSE;
}

/*
 * get_poll_data - gets the poll requested by the player
 * retrieves the data out of the list of the currently loaded
 * polls. The 'num' passed here is NOT the unique id, it is
 * the number of the poll as it is loaded in memory.
 */
POLL_DATA *get_poll_data( int num )
{
    POLL_DATA *poll;
    int count = 0;

    for ( poll = poll_first; poll != NULL; poll = poll->next )
    {
	if ( ++count == num )
	    return poll;
    }

    return NULL;
}

/*
 * can_edit_poll - checks to see if the poll is allowed
 * to be edited. Published, flagged for deletion, and
 * polls that have been voted on may NOT be changed.
 */
bool can_edit_poll( CHAR_DATA *ch, POLL_DATA *poll )
{
    if ( IS_SET(poll->flags, POLL_DELETE) )
    {
        send_to_char( "You can't change a poll that is flagged for deletion.\n\r", ch );
        return FALSE;
    }
    else if ( IS_SET(poll->flags, POLL_PUBLISHED) )
    {
	send_to_char( "You can't change a poll that has been published.\n\r", ch );
	return FALSE;
    }
    else
    {
	int i;

	for ( i = 0; i < MAX_CHOICE; i++ )
	{
	    if ( poll->votes[i] > 0 )
	    {
		send_to_char( "You can't change a poll that has been voted on.\n\r", ch );
		return FALSE;
	    }
	}
    }

    return TRUE;
}

/*
 * get_raw_time - date conversion function.
 * converts MM/DD/YYYY to a 'struct tm' format,
 * then to a long integer for storing.
 */
long get_raw_time( char *date )
{
    char num[MIL] = { '\0' };
    char *ptr;
    int i;
    struct tm time;
    
    ptr = date; 
    i = 0;
    
    for ( ; *ptr != '/'; ptr++ )
        num[i++] = *ptr;
        
    time.tm_mon = atoi(num) - 1;
    ptr++;
    i = 0;
     
    for ( ; *ptr != '/'; ptr++ )
        num[i++] = *ptr;
     
    time.tm_mday = atoi(num);
    ptr++;
    i = 0;
    
    for ( ; *ptr != '\0'; ptr++ )
        num[i++] = *ptr;
        
    time.tm_year = atoi(num) - 1900;
    
    time.tm_sec = 0;
    time.tm_min = 0;
    time.tm_hour = 0;
    time.tm_isdst = 0;

    return mktime( &time );
}

/*
 * has_voted - checks the pfile to see if the unique id
 * has already been voted on. the '|' used in this string
 * is NOT a bitwise function. it is used as a delimiter,
 * much like a comma.
 */
bool has_voted( CHAR_DATA *ch, POLL_DATA *poll )
{
    char id[MIL] = { '\0' };
    char *list;		// list of polls the character has voted on
    int i = 0;

    if ( poll->open > current_time || poll->close < current_time )
	return TRUE;

    for ( list = ch->pcdata->polls; *list != '\0'; list++ )
    {
	if ( *list == '|' )
	{
	    id[0] = '\0';
	    i = 0;
	    continue;
	}

	id[i++] = *list;

	if ( *(list+1) == '|' || *(list+1) == '\0' )
	{
	    if ( atoi(id) == poll->id )
		return TRUE;
	}
    }

    return FALSE;
}

/*
 * days_left - computes the numbers of days left
 * for players to vote on a specific poll. This
 * is used soley for display purposes.
 */
int days_left( POLL_DATA *poll )
{
    int days;
    int one_day = 86400;

    if ( poll == NULL || current_time > poll->close )
	return 0;

    days = ( poll->close - current_time ) / one_day;

    return UMAX(1, days);
}

/*
 * poll_compact - used to remove the 'blank' choices
 * from a poll to ensure no errors occur in saving
 * or loading. called in save_polls.
 */
void poll_compact( POLL_DATA *poll )
{
    int i;
    bool found = TRUE;

    while ( found )
    {
	found = FALSE;

        for ( i = 1; i < MAX_CHOICE; i++ )
        {
	    if ( !IS_NULLSTR(poll->choice[i])
	    && IS_NULLSTR(poll->choice[i-1]) )
	    {
		free_string(poll->choice[i-1]);
		poll->choice[i-1] = str_dup(poll->choice[i]);
		free_string(poll->choice[i]);
		poll->choice[i] = str_dup( "" );
		found = TRUE;
	    }
	}
    }

    return;
}

/*
 * format_output - output string formatter
 * used to format a string before it is placed into
 * a 'buf' for output to a character. the 'spaces' is
 * the number of characters that should be reserved
 * for the original text.
 * ie: "Topic: " is 7 characters.
 * syntax: format_output( string, 7 )
 */
char *format_output( char *string, int spaces )
{
    char buf[MSL] = { '\0' };
    char spc[MSL];
    char *ptr;                  // temp string for looping
    bool hyphen = FALSE;        // TRUE means breaking in the middle of a word, need a hyphen
    bool first = TRUE;          // first time adding to buf, FALSE means add 'spaces'
    int strLength;              // max characters before line wrap
    int pos = 0;                // position in string
    int loc, i;                 // location of last space and counter
         
    sprintf( spc, "\n\r%*c", spaces, ' ' );
    ptr = string;
         
    /* keep looping through until the enter string is copied */
    while ( *ptr != '\0' )
    {
        /* set goalLength to maximum acceptable line length */
        strLength = 74 - spaces;
    
        /* reset tracking */
        loc = 0;
        hyphen = FALSE;
    
        /* loop through up to goalLength characters, or until
         * there are no characters left in the string
         */
        for ( i = 0; i < strLength; i++ )
        {
            /* skip spaces at the beginning of a line */
            if ( i == 0 && *ptr == ' ' )
	    {
                i--;
                ptr++;
                continue;   
            }   

/* section commented. color code parsing.
 * section can be re-enabled if modified
 * mud specific color codes, such as '^'
 * or '{'. 

            // ignore colour codes
            if ( *(ptr+i) == '^' )
            {
                strLength++;
        
                // make sure it's actually a colour code, not an actual ^
                if ( *(ptr+i+1) != '^' )
                    strLength++;
            }
 */ 

            /* keep track of where the last space is */
            else if ( *(ptr+i) == ' ' )
                loc = i;
        
            /* check for end of string */
            else if ( *(ptr+i) == '\0' )
            {
                loc = i;
                break;
            }
        }

        /* no spaces in strLength characters?! */
        if ( loc == 0 && *(ptr+i) != '\0' )
        {
            /* mark for a hyphen */
            loc = strLength - 2;
            hyphen = TRUE;
        }
            
        /* add begining spaces */
        if ( first )
            first = FALSE;
        else 
        {
            strcat( buf, spc );
            pos += spaces + 2;
        }
         
        /* copy up to the last space (or newline) into the buffer */
        strncpy( &buf[pos], ptr, loc );
            
        /* move the position ahead by the number of chars added */
        pos += loc;
        
        /* move the pointer ahead */
        ptr += loc;
        
        /* Add the line break */
        if ( hyphen )          // break in the middle of a line
            buf[pos++] = '-';  // add a hyphen
    }
         
    return str_dup(buf);
}

/* find out if a note is a Immortal note for THIS char */
bool is_imm_poll_to (CHAR_DATA *ch, POLL_DATA *poll)
{
  // Is Imm?
  if (!IS_IMMORTAL(ch))
    return FALSE;

  if (IS_IMMORTAL(ch) &&
	 ( is_full_name ("admin", poll->to) ||
	   is_full_name ("imm", poll->to) ||
	   is_full_name ("imms", poll->to) ||
	   is_full_name ("immortal", poll->to) ||
	   is_full_name ("builder", poll->to) ||
	   is_full_name ("builders", poll->to) ||
	   is_full_name ("god", poll->to) ||
	   is_full_name ("gods", poll->to) ||
	   is_full_name ("immortals", poll->to) ||
	   is_full_name ("guildimm", poll->to) ||
	   is_full_name ("guildadmin", poll->to)))
    return TRUE;
  
  if ((get_trust(ch) == MAX_LEVEL) && 
	 ( is_full_name ("imp", poll->to) ||
	   is_full_name ("imps", poll->to) ||
	   is_full_name ("implementor", poll->to) ||
	   is_full_name ("implementors", poll->to)))
    return TRUE;  

  return FALSE;
}

/* find out if a note is a guild note for THIS char */
bool is_guild_poll_to (CHAR_DATA *ch, POLL_DATA *poll)
{
  char guild_leader[MAX_STRING_LENGTH];

  memset(guild_leader, 0x00, sizeof(guild_leader));

  /* In a guild ? */
  if (!is_clan(ch))
    return FALSE;

  /* My guild ? */
  if (is_full_name (player_clan(ch), poll->to))
    return TRUE;

  /* Oguild? */
  if (is_oguild(ch) && is_full_name (player_oguild(ch), poll->to))
    return TRUE;

  /* Guild leader? */
  if (can_guild(ch)) 
     sprintf(guild_leader, "%sLeader", player_clan(ch));
     
    if (can_guild(ch) && (is_full_name("guildleader", poll->to) ||
                          is_full_name("guildleaders", poll->to)||
                          is_full_name(guild_leader, poll->creator)))
      return TRUE;
  
  return FALSE;
}

/* find out if a note is a SGuild note for THIS char */
bool is_sguild_poll_to (CHAR_DATA *ch, POLL_DATA *poll)
{
  char sguild_leader[MAX_STRING_LENGTH];

  memset(sguild_leader, 0x00, sizeof(sguild_leader));

  /* In a guild ? */
  if (!is_sguild(ch))
    return FALSE;

  /* My guild ? */
  if (is_full_name (player_sguild(ch), poll->to))
    return TRUE;

  /* Guild leader? */
  if (can_sguild(ch)) 
     sprintf(sguild_leader, "%sLeader", player_sguild(ch));
     
    if (can_sguild(ch) && (is_full_name("sguildleader", poll->to) ||
                          is_full_name("sguildleaders", poll->to)||
                          is_full_name(sguild_leader, poll->creator)))
      return TRUE;
    
    return FALSE;
}

/* find out if a note is a SSGuild note for THIS char */
bool is_ssguild_poll_to (CHAR_DATA *ch, POLL_DATA *poll)
{
  char ssguild_leader[MAX_STRING_LENGTH];
  
  memset(ssguild_leader, 0x00, sizeof(ssguild_leader));
  
  /* In a guild ? */
  if (!is_ssguild(ch))
    return FALSE;
  
  /* My guild ? */
  if (is_full_name (player_ssguild(ch), poll->to))
    return TRUE;
  
  /* Guild leader? */
  if (can_ssguild(ch)) 
    sprintf(ssguild_leader, "%sLeader", player_ssguild(ch));
  
  if (can_ssguild(ch) && (is_full_name("ssguildleader", poll->to) ||
                          is_full_name("ssguildleaders", poll->to)||
                          is_full_name(ssguild_leader, poll->creator)))
    return TRUE;
  
  return FALSE;
}

bool is_minion_poll_to(CHAR_DATA *ch, POLL_DATA *poll)
{
  char minion_name[MSL];
  char buf[MSL];
  char *ptr=NULL;

  buf[0] = '\0';
  minion_name[0] = '\0';
  
  if (ch->minion == 0 || IS_NULLSTR(ch->mname))
    return FALSE;
  
  // My minion?
  strcpy(buf, ch->mname);    
  ptr = strtok(buf, "'");
  
  if (ptr == NULL)
    return FALSE;
  
  sprintf(minion_name, "@%s",  ptr);
  
  if (is_full_name(minion_name, poll->to))
    return TRUE;    

  return FALSE;
}

/* find out if a note is a persoanal note to THIS char */
bool is_personal_poll_to (CHAR_DATA *ch, POLL_DATA *poll)
{
  if (!str_cmp (ch->name, poll->creator))
    return TRUE;

  if (is_full_name (ch->name, poll->to))
    return TRUE;

  return FALSE;
}

/* Returns TRUE if the specified note is address to ch */
bool is_poll_to (CHAR_DATA *ch, POLL_DATA * poll)
{
  char guild_leader[MAX_STRING_LENGTH];
  char sguild_leader[MAX_STRING_LENGTH];
  char ssguild_leader[MAX_STRING_LENGTH];
  char buf[MSL];
  char minion_name[MSL];

  guild_leader[0] = '\0';
  sguild_leader[0] = '\0';
  ssguild_leader[0] = '\0';
  buf[0] = '\0';
  minion_name[0] = '\0';

    //memset(guild_leader, 0x00, sizeof(guild_leader));
    //memset(sguild_leader, 0x00, sizeof(sguild_leader));
    //memset(ssguild_leader, 0x00, sizeof(ssguild_leader));
  
  /* All */
  if (is_full_name ("all", poll->to))
    return TRUE;
  
  if (IS_IMMORTAL(ch) && 
	 ( is_full_name ("admin", poll->to) ||
	   is_full_name ("imm", poll->to) ||
	   is_full_name ("imms", poll->to) ||
	   is_full_name ("immortal", poll->to) ||
	   is_full_name ("builder", poll->to) ||
	   is_full_name ("builders", poll->to) ||
	   is_full_name ("god", poll->to) ||
	   is_full_name ("gods", poll->to) ||
	   is_full_name ("immortals", poll->to) ||
	   is_full_name ("guildimm", poll->to) ||
	   is_full_name ("guildadmin", poll->to)))
    return TRUE;
  
  if ((get_trust(ch) == MAX_LEVEL) && 
	 ( is_full_name ("imp", poll->to) ||
	   is_full_name ("imps", poll->to) ||
	   is_full_name ("implementor", poll->to) ||
	   is_full_name ("implementors", poll->to)))
    return TRUE;
  
  if (IS_CODER(ch) && is_full_name("coder", poll->to))
    return TRUE;
  
  /* Personal notes */
  if (is_full_name (ch->name, poll->to))
    return TRUE;
  
  /* Guild notes */
  if ((is_clan(ch) && is_full_name (player_clan(ch), poll->to)))
    return TRUE;

  /* Oguild guild notes*/
  if ((is_oguild(ch) && is_full_name (player_oguild(ch), poll->to)))
    return TRUE;
  
  /* Guild leaders */
  if (is_clan(ch) && can_guild(ch)) {
    sprintf(guild_leader, "%sLeader", player_clan(ch));
    if (is_full_name("guildleader", poll->to)  || 
	   is_full_name("guildleaders", poll->to) ||
	   is_full_name(guild_leader, poll->creator)) {
	 return TRUE;    	
    }
  }
  
  /* SGuild notes */
  if ((is_sguild(ch) && is_full_name (player_sguild(ch), poll->to)))
    return TRUE;
  
  /* SGuild leaders */
  if (is_sguild(ch) && can_sguild(ch)) {
    sprintf(sguild_leader, "%sLeader", player_sguild(ch));
    if (is_full_name("sguildleader", poll->to)  || 
	   is_full_name("sguildleaders", poll->to) ||
	   is_full_name(sguild_leader, poll->creator)) {
	 return TRUE;
    }
  }

  /* SSGuild notes */
  if ((is_ssguild(ch) && is_full_name (player_ssguild(ch), poll->to)))
    return TRUE;
  
  /* SSGuild leaders */
  if (is_ssguild(ch) && can_ssguild(ch)) {
    sprintf(ssguild_leader, "%sLeader", player_ssguild(ch));
    if (is_full_name("ssguildleader", poll->to)  || 
	   is_full_name("ssguildleaders", poll->to) ||
	   is_full_name(ssguild_leader, poll->creator)) {
	 return TRUE;
    }
  }

  /* Minion */
  if (ch->minion != 0 && !IS_NULLSTR(ch->mname)) {
    char *ptr=NULL;
    
    strcpy(buf, ch->mname);    
    ptr = strtok(buf, "'");
    
    if (ptr == NULL)
	 return FALSE;
    
    sprintf(minion_name, "@%s",  ptr);
    
    if (is_full_name(minion_name, poll->to))
	 return TRUE;
  }
  
  /* Wolfkin */
  if (IS_WOLFKIN(ch) && is_full_name ("wolfkin", poll->to))
    return TRUE;

  // Newbie Helpers
  if ((IS_IMMORTAL(ch) || IS_NEWBIEHELPER(ch)) &&
	 (is_full_name ("newbiehelper", poll->to) ||
	  is_full_name ("newbiehelpers", poll->to))) {
    return TRUE;
  }

  // Warders
  if (IS_WARDER(ch) && (is_full_name("warder", poll->to) ||
				    is_full_name("warders", poll->to))) {
    return TRUE;
  }
  
  /* Allow a note to e.g. 40 to send to characters level 40 and above */		
  if (is_number(poll->to) && get_trust(ch) >= atoi(poll->to))
    return TRUE;

  /* to darkfriends */
  if ((ch->pcdata->df_level >= 0) && (is_full_name("darkfriend",poll->to) || is_full_name("darkfriends",poll->to)))
  {
    return TRUE;
  }
  
  return FALSE;
}


