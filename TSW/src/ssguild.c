/***************************************************************************
*       ROM 2.4 is copyright 1993-1995 Russ Taylor			          *
*       ROM has been brought to you by the ROM consortium		          *
*           Russ Taylor (rtaylor@pacinfo.com)				          *
*           Gabrielle Taylor (gtaylor@pacinfo.com)			          *
*           Brian Moore (rom@rom.efn.org)				               *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	     *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									                              *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	     *
 *  Chastain, Michael Quan, and Mitchell Tse.				          *
 *									                              *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	     *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						                    *
 *									                              *
 *  Much time and thought has gone into this software and you are	     *
 *  benefitting.  We hope that you share your changes too.  What goes	     *
 *  around, comes around.						                    *
 ***************************************************************************/
#include <dirent.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                \
                if ( !str_cmp( word, literal ) )    \
                {                                   \
                    field  = value;                 \
                    fMatch = TRUE;                  \
                    break;                          \
                                }

char *ssguild_bit_name( int ssguild_flags )
{
  static char buf[512];
  
  buf[0] = '\0';
  if ( ssguild_flags & SSGUILD_INDEPENDENT	) 
    strcat( buf, " independent"	);

  if ( ssguild_flags & SSGUILD_CHANGED	) 
    strcat( buf, " changed"	);

  if ( ssguild_flags & SSGUILD_DELETED	) 
    strcat( buf, " deleted"	);

  if ( ssguild_flags & SSGUILD_WOLF		) 
    strcat( buf, " wolfkin"	);

  if ( ssguild_flags & SSGUILD_IMMORTAL	) 
    strcat( buf, " immortal"	);
  
  return ( buf[0] != '\0' ) ? buf+1 : "none";
}

void load_ssguilds(void)
{
  FILE *fp;
  char buf[MAX_STRING_LENGTH];
  char logbuf[256];
  char *string;
  int count = 0;
  int i;
  bool fMatch = FALSE;
  
  for (i=0; i < MAX_CLAN; i++) {
    ssguild_table[i].name = "";
    ssguild_table[i].who_name = "";
    ssguild_table[i].room[0]= 0;
    ssguild_table[i].room[1]= 0;
    ssguild_table[i].room[2]= 0;
    ssguild_table[i].rank[0].rankname = "";
    ssguild_table[i].rank[0].skillname = "";
    ssguild_table[i].ml[0] = 0;
    ssguild_table[i].ml[1] = 0;
    ssguild_table[i].ml[2] = 0;
    ssguild_table[i].ml[3] = 0;
    ssguild_table[i].flags = 0;
  }
    
  sprintf(buf, "%sssguild.dat", DATA_DIR);

  sprintf(logbuf, "Loading SSGuild data file %s", buf);
  log_string(logbuf);
  

  if ((fp = fopen(buf, "r")) == NULL) {
    log_string("Error: ssguild.dat file not found!");
    exit(1);
  }
  for (;;) {
    string = feof(fp) ? "End" : fread_word(fp);
    
    if (!str_cmp(string, "End"))
	 break;
    
    switch (UPPER(string[0])) {
    case 'F':
	 ssguild_table[count].flags  = fread_flag( fp );
	 fMatch = TRUE;
	 break;
	 
    case 'G':
	 count++;
	 ssguild_table[count].name = fread_string(fp);
	 fMatch = TRUE;
	 break;

    case 'R':
	 if (!str_cmp(string, "Rooms")) {
	   ssguild_table[count].room[0] = fread_number(fp);	/* hall   */
	   ssguild_table[count].room[1] = fread_number(fp);	/* morgue */
	   ssguild_table[count].room[2] = fread_number(fp);	/* temple */
	   fMatch = TRUE;
	 } 
	 else if (!str_cmp(string, "Rank")) {
	   i = fread_number(fp);
	   ssguild_table[count].rank[i - 1].rankname = fread_string(fp);
	   fMatch = TRUE;
	 }
	 break;

    case 'S':
	 i = fread_number(fp);
	 ssguild_table[count].rank[i - 1].skillname = fread_string(fp);
	 fMatch = TRUE;
	 break;
	 
    case 'M':
	 ssguild_table[count].ml[0] = fread_number(fp);
	 ssguild_table[count].ml[1] = fread_number(fp);
	 ssguild_table[count].ml[2] = fread_number(fp);
	 ssguild_table[count].ml[3] = fread_number(fp);
	 fMatch = TRUE;
	 break;
	 
    case 'W':
	 ssguild_table[count].who_name = fread_string(fp);
	 fMatch = TRUE;
	 break;
	 
    }			/* end of switch */
    
  }				/* end of while (!feof) */
  
  if (!fMatch) {
    bug("Fread_ssguilds: no match.", 0);
    fread_to_eol(fp);
  }
  fclose(fp);

  sprintf(logbuf, "Loaded %d SSGuilds successfully.", count);
  log_string(logbuf);

  return;
} /* end: load_ssguilds */

bool is_ssguild_leader(CHAR_DATA * ch)
{
     //return IS_SET(ch->act, PLR_MORTAL_LEADER) ? 1 : 0;
  if (ch->ssguild_rank <= 2)
    return TRUE;
  else
    return FALSE;
  
  return FALSE;
}

bool can_ssguild(CHAR_DATA * ch)
{
  /* ok if ch is a IMMORTAL or higher */
  /*if (ch->level >= IMMORTAL || ch->trust >= IMMORTAL) */
  if (IS_IMMORTAL(ch))
    return TRUE;
  
  /* not ok if ch is not ssguilded or is not a mortal leader */
  if (ch->ssguild == 0 || !is_ssguild_leader(ch))
    return FALSE;

  return ssguild_table[ch->ssguild].ml[0];
} /* end: can_ssguild */


bool can_dessguild(CHAR_DATA * ch)
{
  /* ok if ch is a SUPREME or higher */
  /* if (ch->level >= SUPREME || ch->trust >= SUPREME)*/
  if (IS_IMMORTAL(ch))
    return TRUE;
  
  /* not ok if ch is not ssguilded or is not a mortal leader */
  if (ch->ssguild == 0 || !is_ssguild_leader(ch))
    return FALSE;
  
  return ssguild_table[ch->ssguild].ml[1];
} /* end: can_dessguild */


bool can_ssguild_promote(CHAR_DATA * ch)
{
  /* ok if ch is a IMMORTAL */
  if (IS_IMMORTAL(ch))
    return TRUE;
  
  /* not ok if ch is not ssguilded or is not a mortal leader */
  if (ch->ssguild == 0 || !is_ssguild_leader(ch))
    return FALSE;
  
  /* is a mortal leader, but do they have the right? */
  return ssguild_table[ch->ssguild].ml[2];
} /* end: can_promote */


bool can_ssguild_demote(CHAR_DATA * ch)
{
  /* ok if ch is a IMMORTAL */
  if (IS_IMMORTAL(ch))
    return TRUE;
  
  /* not ok if ch is not ssguilded or is not a mortal leader */
  if (ch->ssguild == 0 || !is_ssguild_leader(ch))
    return FALSE;
  
  return ssguild_table[ch->ssguild].ml[3];
} /* end: can_ssguild_demote */


bool is_ssguild(CHAR_DATA * ch)
{
  return ch->ssguild;
} /* end: is_ssguild */


bool is_same_ssguild(CHAR_DATA * ch, CHAR_DATA * victim)
{
  if (IS_SET(ssguild_table[ch->ssguild].flags,SSGUILD_INDEPENDENT))
    return FALSE;
  else
    return (ch->ssguild == victim->ssguild);
} /* end: is_same_ssguild */


int ssguild_lookup(const char *name)
{
  int ssguild;
  
  for (ssguild = 0; ssguild < MAX_CLAN; ssguild++) {
    if (!str_prefix(name, ssguild_table[ssguild].name))
	 return ssguild;
  }
  
  return 0;
} /* end: ssguild_lookup */


char *player_ssguild_rank(CHAR_DATA * ch)
{
  if (ch->ssguild == 0)
    return '\0';
  return ssguild_table[ch->ssguild].rank[ch->ssguild_rank].rankname;
} /* end: player_rank */


char *player_ssguild(CHAR_DATA * ch)
{
  if (ch->ssguild == 0)
    return '\0';
  return ssguild_table[ch->ssguild].name;
} /* end: player_ssguild */

void reset_ssguild_status(CHAR_DATA *ch)
{
  ch->ssguild                   = 0;
  ch->ssguild_rank              = 0;
  ch->pcdata->ssguild_offer     = 0;
  ch->pcdata->ssguild_requestor = 0;
  ch->pcdata->ssguilded_by      = 0;
  
  free_string(ch->ssguild_title);
  ch->ssguild_title = NULL;
  
  ch->ssguild_invis = FALSE;
}

void set_ssguild_title( CHAR_DATA *ch, char *ssguild_title )
{
  // validate
  if (IS_NULLSTR(ssguild_title))
    return;
  
  free_string( ch->ssguild_title );
  ch->ssguild_title = str_dup(ssguild_title);
  return;
}

void do_ssguild_promote(CHAR_DATA * ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  int cnt;
  int sn = 0;
  
  argument = one_argument(argument, arg1);
  
  if (!can_ssguild_promote(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (arg1[0] == '\0' || argument[0] == '\0') {

    send_to_char("Syntax: spromote <who> <rank #>\n\r", ch);
    send_to_char("Where rank is one of the following:\n\r", ch);
    
    for (cnt = 0; cnt < MAX_RANK; cnt++) {
	 sprintf(buf, "%2d] %s\n\r", cnt + 1,
		    is_ssguild(ch) ? ssguild_table[ch->ssguild].rank[cnt].rankname : "(None)");
	 send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);
    return;
  }				/* end syntax */

  if ( ( victim = get_realname_char_world( ch, arg1)) == NULL ) {
     send_to_char( "They aren't playing.\n\r", ch );
     return;
  }

  
/* OLD INTRO CODE  
  if (IS_IMMORTAL(ch)) {
    if ( ( victim = get_realname_char_world( ch, arg1)) == NULL ) {
	 send_to_char( "They aren't playing.\n\r", ch );
	 return;
    }
  }
  else {
    if ((victim = get_introname_char_world(ch, arg1)) == NULL) {
	 send_to_char("They aren't playing.\n\r", ch);
	 return;
    }
  }
*/
  
  if (!is_ssguild(victim)) {
    send_to_char("They are not a member of any SSGuilds!\n\r", ch);
    return;
  }
  
  if (!is_same_ssguild(ch, victim) && !IS_IMMORTAL(ch)) {
    send_to_char("They are a member of a SSGuild different than yours!\n\r", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim)) {
    send_to_char("You can't promote Immortals.\n\r", ch);
    return;
  }

  if (!str_prefix(argument, "leader") && ch != victim) {
//    SET_BIT(victim->act, PLR_MORTAL_LEADER);
    send_to_char("They are now a leader of the SSGuild.\n\r", ch);
    send_to_char("You have just been promoted to a leader of your SSGuild!\n\r", victim);
    victim->ssguild_rank = 2;

    // Save info
    save_char_obj(victim, FALSE);
    
    return;
  }
  
  cnt = atoi(argument) - 1;
  if (cnt < 0 ||
	 cnt > MAX_RANK -1 ||
	 ssguild_table[victim->ssguild].rank[cnt].rankname == NULL) {
    send_to_char("That rank does not exist!", ch);
    return;
  }
  if (cnt > victim->ssguild_rank && ((ch == victim) && (!IS_IMMORTAL(ch)))) {
    send_to_char("Heh. I dont think so...", ch);
    return;
  }
  
  if (cnt < victim->ssguild_rank) {
    int i;
    
    sprintf(buf, "You have been promoted to %s!\n\r",
		  ssguild_table[victim->ssguild].rank[cnt].rankname);
    send_to_char(buf, victim);
    
    sprintf(buf, "%s has been promoted to %s!\n\r",
		  PERS_NAME(victim,ch), ssguild_table[victim->ssguild].rank[cnt].rankname);
    send_to_char(buf, ch);
    
    set_ssguild_title(victim, ssguild_table[victim->ssguild].rank[cnt].rankname);
    
    for (i = victim->ssguild_rank; i >= cnt; i--)
	 if (ssguild_table[victim->ssguild].rank[i].skillname != NULL) {
	   sn = skill_lookup(ssguild_table[victim->ssguild].rank[i].skillname);
	   if (sn < 0) {
		sprintf(buf, "Bug: Add skill [%s] is not a valid skill",
			   ssguild_table[victim->ssguild].rank[cnt].skillname);
		log_string(buf);
		} 
	   else if (!victim->pcdata->learned[sn]) {
		victim->pcdata->learned[sn] = 20 + (victim->level / 4);
		sprintf(buf, "You have been SSGuild granted '%s' as a result of your promotion!\n\r",
			   ssguild_table[victim->ssguild].rank[i].skillname);
		send_to_char(buf, victim);
	   }
	 }
  } 
  
  else if (cnt > victim->ssguild_rank) {
//    if (IS_SET(victim->act, PLR_MORTAL_LEADER))
//	 REMOVE_BIT(victim->act, PLR_MORTAL_LEADER);
    
    sprintf(buf, "You have been SSGuild demoted to %s!\n\r",
		  ssguild_table[victim->ssguild].rank[cnt].rankname);

    set_ssguild_title(victim, ssguild_table[victim->ssguild].rank[cnt].rankname);
    
    send_to_char(buf, victim);
    
    sprintf(buf, "%s has been SSGuild demoted to %s!\n\r",
		  PERS_NAME(victim, ch), ssguild_table[victim->ssguild].rank[cnt].rankname);    
    send_to_char(buf, ch);
    
    // If ssginvis, strip it and tell.
    if (victim->ssguild_invis) {
  	ch->ssguild_invis = FALSE;
  	send_to_char("You are now visible to your SSGuild.\n\r", victim);    	
    }    
        
    /*
	* ---------------------------------------------------------------
	* Note: I dont think it would be fair here to take away any skills
	* the victim may have earned at a higher rank. It makes no RP sense
	* to do so and only hurts the player (loss of practices etc). Imms
	* may want to keep an eye on this, as we dont want players jumping
	* ssguilds just to gain new skills.
	* -------------------------------------------------------------- 
	*/
  }				/* else no change */

  victim->ssguild_rank = cnt;

  // Save info
  save_char_obj(victim, FALSE);
  
  return;
} /* end: do_promote */


/*
 * Immortal ssguild command for joining/banis
 */
void do_ssguild(CHAR_DATA * ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  int ssguild;
  
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  
  if (!can_ssguild(ch) || !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (arg1[0] == '\0' || arg2[0] == '\0') {
    send_to_char("Syntax: ssguild <char> <ssguild name>\n\r", ch);
    return;
  }

/*  if ((victim = get_char_world(ch, arg1)) == NULL) { */
  if ((victim = get_realname_char_world(ch, arg1)) == NULL) { 
    send_to_char("They aren't playing.\n\r", ch);
    return;
  }

  /** thanks to Zanthras for the bug fix here...*/
  if (is_ssguild(victim) && !is_same_ssguild(ch, victim) &&
	 ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("They are a member of a SSGuild other than your own.\n\r", ch);
    return;
  }

  if (!str_prefix(arg2, "none")) {
    
    if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim)) {
	 send_to_char("You can not set SSGuild status on the Immortals!\n\r", ch);
	 return;
    }
    
    send_to_char("They are no longer a member of any SSGuild.\n\r", ch);
    send_to_char("You are no longer a member of any SSGuild!\n\r", victim);

    reset_ssguild_status(victim);

//    if (IS_SET(victim->act, PLR_MORTAL_LEADER))
//	 REMOVE_BIT(victim->act, PLR_MORTAL_LEADER);
    
    return;
  }
  
  if ((ssguild = ssguild_lookup(arg2)) == 0) {
    send_to_char("No such SSGuild exists.\n\r", ch);
    return;
  }
  
  sprintf(buf, "They are now %s of the %s.\n\r",
		ssguild_table[ssguild].rank[MAX_RANK-1].rankname, ssguild_table[ssguild].name);
  send_to_char(buf, ch);
  
  sprintf(buf, "You are now %s of the %s.\n\r",
		ssguild_table[ssguild].rank[MAX_RANK-1].rankname, ssguild_table[ssguild].name);
  send_to_char(buf, victim);
  
  victim->ssguild = ssguild;
  victim->ssguild_rank = MAX_RANK-1;		/* lowest, default */
} /* end: do_ssguild */

/*
 * Set a members ssguild title if leader
 */
void do_ssguildtitle( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;

  smash_tilde( argument );
  argument = one_argument(argument, arg1);
  strcpy(arg2, argument);
  
  if (!can_ssguild(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if ( (arg1[0] == '\0') || (arg2[0] == '\0')) {
    send_to_char("Syntax: sgtitle <char> <title>\n\r", ch);
    return;
  }

  if ( ( victim = get_realname_char_world( ch, arg1)) == NULL ) {
   send_to_char( "They aren't playing.\n\r", ch );
   return;
  }  
  
/* OLD INTRO CODE  
  if (IS_IMMORTAL(ch)) {
    if ( ( victim = get_realname_char_world( ch, arg1)) == NULL ) {
	 send_to_char( "They aren't playing.\n\r", ch );
	 return;
    }
  }
  else {
    if ((victim = get_introname_char_world(ch, arg1)) == NULL) {
	 send_to_char("They aren't playing.\n\r", ch);
	 return;
    }
  }
*/

  if (is_ssguild(victim) && !is_same_ssguild(ch, victim) &&
	 ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("They are a member of a SSGuild other than your own.\n\r", ch);
    return;
  }

  if (ch != victim && IS_IMMORTAL(victim)) {
     send_to_char("You can't set SSGuild titles on immortals.\n\r", ch);
     return;
  }

  set_ssguild_title(victim, arg2);

  sprintf(buf, "You set %s's SSGuild title to %s.\n\r", PERS_NAME(victim, ch), victim->ssguild_title);
  send_to_char(buf, ch);
  
  sprintf(buf, "%s has set your SSGuild title to %s.\n\r", PERS_NAME(ch, victim), victim->ssguild_title);
  send_to_char(buf, victim);

  // Save info
  save_char_obj(victim, FALSE);
  
  return;
}

int compare_ssguild_ranks(const void *v1, const void *v2)
{
  return (*(CHAR_DATA**)v1)->ssguild_rank - (*(CHAR_DATA**)v2)->ssguild_rank;
}

/*
 * Show current playing and wizible members of a Ssguild
 */
void do_ssguildlist( CHAR_DATA *ch, char *argument )
{
  BUFFER *output;
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;  
  CHAR_DATA *wch;    
  CHAR_DATA *pcs[MAX_PC_ONLINE];
  int cnt = 0;
  int i   = 0;
  int fillers=0;
  int fillers2=0;

  if (!is_ssguild(ch)) {
    send_to_char("You aren't in a SSGuild.\n\r",ch);
    return;
  }
  
  output = new_buf();
  
  for (d = descriptor_list; d != NULL; d = d->next) {
    
    if (d->connected != CON_PLAYING || !can_see_channel(ch,d->character))
	 continue;
    
    wch = ( d->original != NULL ) ? d->original : d->character;
    
    if (!can_see_channel(ch,wch))
	 continue;
    
    if (wch->ssguild != ch->ssguild)
	 continue;

    // Only ssguild leaders can see other ginvis members
    if ( (wch->ssguild_invis == TRUE) && (wch != ch) && !can_ssguild(ch) )
    	continue;

    // If immortal and ginvis, then none can see you
    if (wch->ssguild_invis == TRUE && IS_IMMORTAL(wch) && (wch != ch))
	 continue;
    
    pcs[cnt++] = wch;
  }

  /* Sort PC array based on name first */
  qsort (pcs, cnt, sizeof(wch), compare_char_names);

  /* Then we sort based on rank */
  qsort (pcs, cnt, sizeof(wch), compare_ssguild_ranks);
  
  for (i=0; i < cnt; i++) {
    fillers = (16 - colorstrlen(ssguild_table[pcs[i]->ssguild].who_name)-1);
    fillers2 = (16 - colorstrlen(PERS_OLD(pcs[i], ch))-1);
    sprintf(buf, "[%s%*s (%d)] %s%*s (%s) %s%s\n\r", ssguild_table[pcs[i]->ssguild].who_name, 
		  fillers, "",
		  pcs[i]->ssguild_rank+1, 
		  PERS_OLD(pcs[i], ch),
		  fillers2, "",
		  pcs[i]->ssguild_title ? pcs[i]->ssguild_title : player_ssguild_rank(pcs[i]),
		  is_ssguild_leader(pcs[i]) ? "({Yssgl{x)" : "",
		  pcs[i]->ssguild_invis ? "({Yssgi{x)" : "");
    add_buf(output,buf);
  }

  sprintf(buf, "\nMembers found: {y%d{x.\n\r", cnt);
  add_buf(output,buf);
  
  page_to_char(buf_string(output),ch);
  free_buf(output);
}

/*
 * List all awailable ssguilds
 */
void do_ssguildslist( CHAR_DATA *ch, char *argument )
{
  char buf[MIL];
  BUFFER *buffer;
  int i;

  buffer = new_buf();
  
  sprintf(buf, "Available SSGuilds:\n\r"
		"--------------------\n\r");
  add_buf(buffer, buf);
  
  for (i=1; i <= MAX_CLAN; i++) {
    if (ssguild_table[i].name != NULL && ssguild_table[i].name[0] != '\0') {
	 sprintf(buf,"[%d] %-16s\n\r", i, ssguild_table[i].who_name);
      add_buf(buffer, buf);
    }
  }
  
  page_to_char( buf_string(buffer), ch );
  free_buf(buffer);
  return;
}

/*
 * Offer a PC to join a ssguild
 */
void do_ssguild_join( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  int ssguild;
  CHAR_DATA *victim;
  
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  
  /* Get rid of people that can't ssguild */    
  if (!can_ssguild(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
    send_to_char("Syntax:\n\r  ssjoin <char> <ssguild name>\n\r"
			  "Use the '{Wssguildslist{x' command to see awailable SSGuilds.\n\r"
			  ,ch);
    return;
  }
  
  if (strcasecmp(arg2, "none") == 0) {
    send_to_char("Try the 'sbanish' command.\n\r",ch);
    return;
  }

  if (IS_IMMORTAL(ch)) {
     if ((victim = get_realname_char_world(ch, arg1)) == NULL) {
        send_to_char("They aren't playing.\n\r", ch );
        return;
     }     
  }
  else {
     if ((victim = get_realname_char_world(ch, arg1)) == NULL) 
     {
        send_to_char( "They aren't in this room or they have not introduced themself to you.\n\r", ch );
	return;
     }
  }

/* OLD INTRO CODE
  if (IS_IMMORTAL(ch)) {
    if ( ( victim = get_realname_char_world( ch, arg1)) == NULL ) {
	 send_to_char( "They aren't playing.\n\r", ch );
	 return;
    }
  }
  else {
    if ( ( victim = get_introname_char_room( ch, arg1)) == NULL ) {
	 send_to_char( "They aren't in this room or they have not introduced themself to you.\n\r", ch );
	 return;
    }
  }
  */

  /* Get rid of people that are trying to ssguild themselves */
  if (ch == victim && !IS_IMMORTAL(ch)) {
    send_to_char("You can't join your self to a SSGuild!\n\r", ch);
    return;
  }

  if IS_NPC(victim) {
    send_to_char("Mobiles can't be SSGuilded!\n\r",ch);
    return;
  }

/*
  if (!is_clan(victim)) {
    send_to_char("They need to be a member of the same guild as you.\n\r", ch);
    return;
  }
  else if (!is_same_clan(ch, victim)) {
    send_to_char("They need to be a member of the same guild as you.\n\r", ch);
    return;
  }
*/

  if (is_ssguild(victim) && !is_same_ssguild(ch, victim) &&
	 !IS_IMMORTAL(ch)) {
    send_to_char("They are a member of a SSGuild other than your own.\n\r", ch);
    return;
  }

  if ((ssguild = ssguild_lookup(arg2)) == 0) {
    send_to_char("No such SSGuild exists.\n\r", ch);
    return;
  }
  
  if (ch->ssguild != ssguild) {
     send_to_char("You can't ssjoin to another ssguild than the one you are in.\n\r", ch);
     return;	
  }    

  /* Offer */
  sprintf(buf, "Your offer to %s has been made.\n\r", PERS_NAME(victim, ch));
  send_to_char(buf, ch);

  victim->pcdata->ssguild_offer     = ssguild;
  victim->pcdata->ssguild_requestor = ch->id;

  sprintf(buf,"%s has offered you a position in the SSGuild: [%s].\n\r",
		PERS(ch, victim), ssguild_table[ssguild].name);
  send_to_char(buf, victim);
  send_to_char("Type {yssaccept{x to join.\n\rType {yssrefuse{x to decline offer.\n\r", victim);
  
  return;
}

/*
 * Option to accept a given offer to join a Ssguild
 */
void do_ssguild_accept( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *vch;
  char buf[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  if (ch->pcdata->ssguild_offer == 0) 
    return;

  if (argument[0] != '\0') {
    sprintf(buf, "Type accept to join the SSGuild: [%s].\n\r",
		  ssguild_table[ch->pcdata->ssguild_offer].name);
    send_to_char(buf, ch);
    return;
  }

  ch->ssguild               = ch->pcdata->ssguild_offer;
  ch->ssguild_rank          = MAX_RANK-1;
  ch->pcdata->ssguild_offer = 0;
  ch->pcdata->ssguilded_by  = ch->pcdata->ssguild_requestor;

  sprintf(buf, "You have joined the SSGuild: [%s].\n\r", ssguild_table[ch->ssguild].name);
  send_to_char(buf, ch);
  
  if ((vch = get_charId_world(ch, ch->pcdata->ssguilded_by)) != NULL) {
    sprintf(buf, "You have notifed %s that you have joined the SSGuild: [%s].\n\r",
		  PERS(vch, ch), ssguild_table[ch->ssguild].name);
    send_to_char(buf, ch);
    
    sprintf(buf, "%s has accepted your offer and join the SSGuild: [%s].\n\r",
		  PERS_NAME(ch, vch), ssguild_table[ch->ssguild].name);
    send_to_char(buf, vch);
  }

  // Save info
  save_char_obj( ch, FALSE);
}

/*
 * Option to refuse to join a offered Ssguild
 */
void do_ssguild_refuse( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *vch;
  char buf[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  if (ch->pcdata->ssguild_offer == 0) 
    return;

  if (argument[0] != '\0') {
    send_to_char("Just type 'srefuse' to decline the offer to join.\n\r", ch);
    return;
  }
  
  sprintf(buf, "You refused to join the SSGuild: [%s].\n\r",
		ssguild_table[ch->pcdata->ssguild_offer].name);
  send_to_char(buf, ch);


  if ((vch = get_charId_world(ch, ch->pcdata->ssguild_requestor)) != NULL) {
    sprintf(buf, "You have notifed %s that you will not join the SSGuild: [%s].\n\r",
		  PERS(vch, ch), ssguild_table[ch->pcdata->ssguild_offer].name);
    send_to_char(buf, ch);

    sprintf(buf, "%s has refused your offer to join the SSGuild: [%s].\n\r",
		  PERS_NAME(ch, vch),  ssguild_table[ch->pcdata->ssguild_offer].name);
    send_to_char(buf, vch);

    ch->pcdata->ssguild_offer     = 0;      
    ch->pcdata->ssguild_requestor = 0;
  }

  // Save info
  save_char_obj(ch, FALSE);

  return;
}

// Offline banish
void do_ssguild_obanish( CHAR_DATA *ch, char *argument )
{ 
  CHAR_DATA *vch=NULL;
  DESCRIPTOR_DATA *d=NULL;
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  char note_buf1[MAX_STRING_LENGTH];
  char note_buf2[MAX_STRING_LENGTH];
  char note_to[MAX_STRING_LENGTH];
  bool found=FALSE;
  bool isDisguise=FALSE;
  bool setFtime=FALSE;
  char filename[MAX_STRING_LENGTH];
  struct stat s_buf;
  struct utimbuf ut_buf;
    
  argument = one_argument(argument, arg1);

  arg1[0] = UPPER(arg1[0]);

  // Do the player have the rights to banish ?
  if (!can_dessguild(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  // Syntax
  if ( arg1[0] == '\0') {
    send_to_char("Syntax: ssobanish <char> [reason]. \n\r",ch);
    return;
  }

  // Check that not self banish
  if (!str_cmp(ch->name, arg1)) {
    send_to_char("You can't ssobanish your self. Please see one of the immortals.\n\r", ch);
    return;
  }

  // Make sure char not already loaded!
  if ( (vch = get_char_world( ch, arg1)) != NULL) {
    sprintf(buf, "%s is already connected. Please use normal sbanish.\n\r", arg1);
    send_to_char(buf, ch);
    return;
  }

  // Try load a char object, but don't insert into normal descriptor
  // List....
  // Try normal player dir first
  d = new_descriptor();
  found = load_char_obj( d, arg1, FALSE );

  // If not found, try disguise dir
  if (found != TRUE) {
    found = load_char_obj( d, arg1, TRUE );
    if (found)
	 isDisguise=TRUE;
  }
  
  // If not found in either... alas
  if (!found) {
    send_to_char("No such player found offline.\n\r", ch);
    return;
  }
  
  // Can banish him/her/it?
  if (is_ssguild(d->character) && !is_same_ssguild(ch, d->character) &&
	 ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("You can't ssobanish that person!.\n\r", ch);
    return;
  }
  
  // Banish
  reset_ssguild_status(d->character);

  // Info
  sprintf(buf, "You have banished %s from the %s{x SSGuild.\n\r", arg1, ssguild_table[ch->ssguild].who_name);
  send_to_char(buf,ch);
  sprintf(buf, "A note has been left to %s with information about the banish.\n\r", arg1);
  send_to_char(buf, ch);

  //Send the banished person a note
  sprintf(note_buf1,
		"%s{x,\n\n\r"
		"You have been banished from the %s SSGuild with the following reason:\n\n\r"
		"o %s\n\n\r"
		"Signed by the hand of %s{x,\n\r"
		"%s.\n\r",
		arg1,
		ssguild_table[ch->ssguild].who_name,
		IS_NULLSTR(argument) ? "No reason given." : argument,
		ch->name,
		ssguild_table[ch->ssguild].who_name);
		//ch->in_room->area->name);

  sprintf(note_to, "%s %sLeader", arg1, player_ssguild(ch));
  
  sprintf(note_buf2, "You have been banished from the %s SGuild.", ssguild_table[ch->ssguild].who_name);
  
  make_note("Guild", ch->name, note_to, note_buf2, 56, note_buf1);

  // Try to keep old time on file
  // If stat fails, we just go with new mod time
  if (isDisguise)
    sprintf(filename, "%s%s", PLAYER_DISGUISE_DIR, arg1);
  else
    sprintf(filename, "%s%s", PLAYER_DIR, arg1);
  if (stat (filename, &s_buf) != -1) {
    setFtime=TRUE;
  }
  
  // Save info
  save_char_obj(d->character, FALSE);
  free_char(d->character);
  free_descriptor(d);

  if (setFtime) {
    ut_buf.actime = s_buf.st_atime;
    ut_buf.modtime = s_buf.st_mtime;
    utime(filename, &ut_buf);
  }
  
  // Done
  return;  
}

void do_ssguild_banish( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *vch;

  argument = one_argument(argument, arg1);

  /* Do the player have the rights to banish ? */
  if (!can_dessguild(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if ( arg1[0] == '\0') {
    send_to_char("Syntax: sbanish <char>. \n\r",ch);
    return;
  }

  if ( (vch = get_char_world( ch, arg1)) == NULL) {
  /* if ( (vch = get_introname_char_world( ch, arg1)) == NULL) { */
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }

  /* Check mobiles */
  if IS_NPC(vch) {
    send_to_char("Mobiles can't be SSGuilded or sbanished!\n\r",ch);
    return;
  }

  /* The self check */
  if ((ch == vch) && !IS_IMMORTAL(ch)) {
    send_to_char("You can't sbanish your self. Please see one of the immortals.\n\r", ch);
    return;
  }

  if (is_ssguild(vch) && !is_same_ssguild(ch, vch) &&
	 ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("You can't sbanish that person!.\n\r", ch);
    return;
  }

  /* Banish */
  reset_ssguild_status(vch);
  
  sprintf(buf,"%s has banished you from your SSGuild.\n\r",PERS_NAME(ch, vch));
  send_to_char(buf,vch);
  
  sprintf(buf, "You have sbanished %s.\n\r",PERS_NAME(vch, ch));
  send_to_char(buf,ch);

  // Save info
  save_char_obj(vch, FALSE);
  
  /* Done */
  return;
}

void do_ssguildinvis( CHAR_DATA *ch, char *argument )
{

  if (!can_ssguild(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (ch->ssguild_invis != TRUE) {
  	ch->ssguild_invis = TRUE;
  	send_to_char("You are now SSGuild invis.\n\r", ch);
  	return;
  }
  else {
    ch->ssguild_invis = FALSE;	
    send_to_char("You are now visible to your SSGuild.\n\r", ch);
    return;
  }
  
  return;
}

typedef struct ssguild_member_data {
  char player_name[64];
  int  player_level;
  char ssguild_name[128];
  char ssguild_title[512];
  time_t last_on_time;
  char last_on_time_str[64];
  int  ssguild_rank;
}ssguild_member_data;

#define MAX_SSGUILD_MEBER_DATA 100

struct ssguild_member_data gd[MAX_SSGUILD_MEBER_DATA];

void init_ssguild_member_data()
{
  int i;
  
  for (i=0; i < MAX_SSGUILD_MEBER_DATA; i++) {
    gd[i].player_name[0]      = '\0';
    gd[i].player_level        = 0;
    gd[i].ssguild_name[0]     = '\0';
    gd[i].ssguild_title[0]    = '\0';
    gd[i].last_on_time        = 0;
    gd[i].last_on_time_str[0] = '\0';
    gd[i].ssguild_rank        = 0;
  }
  
  return;
}

int find_free_ssguild_member_data_slot()
{
  int i;
  
  for (i=0; i < MAX_SSGUILD_MEBER_DATA; i++) {
    if (IS_NULLSTR(gd[i].player_name))
	 return i;
  }

  return -1;
}

bool insert_new_ssguild_member_data(struct ssguild_member_data new)
{
  int i = find_free_ssguild_member_data_slot();
  
  if (i < 0 || i > MAX_SSGUILD_MEBER_DATA)
    return FALSE;
  
  strcpy(gd[i].player_name, new.player_name);
  strcpy(gd[i].ssguild_name, new.ssguild_name);
  strcpy(gd[i].ssguild_title, new.ssguild_title);
  strcpy(gd[i].last_on_time_str, new.last_on_time_str);

  gd[i].player_level = new.player_level;
  gd[i].last_on_time = new.last_on_time;
  gd[i].ssguild_rank = new.ssguild_rank;
  
  return TRUE;
}

int load_ssguild_member_data(CHAR_DATA *ch, char *path)
{
  char *word=NULL;
  FILE *fp=NULL;
  char fname[256];
  struct stat sb;
  CHAR_DATA *victim=NULL;
  struct dirent **Dir;
  int i=0;
  int n=0;
  bool find_more = TRUE;
  bool isSsguild = FALSE;
  int mCnt=0;
  bool fMatch;
  char *ptr=NULL;
  char *strtime=NULL;

  struct ssguild_member_data new;
  
  n = scandir(path, &Dir, 0, alphasort);

  if (n < 0)
    return n;

  new.player_name[0]   = '\0';
  new.ssguild_name[0]  = '\0';
  new.ssguild_title[0] = '\0';
  new.player_level     = 0;
  new.ssguild_rank     = 0;
  new.last_on_time     = 0;
  new.last_on_time_str[0] = '\0';

  for (i=0; i<n; i++) {
    sprintf(fname, "%s%s", path, Dir[i]->d_name);

    if (Dir[i]->d_name[0] >= 'A' && Dir[i]->d_name[0] <= 'Z' && Dir[i]->d_name[0] != '.') {
	 if (( fp = fopen( fname, "r" )) == NULL )
	   continue;
	 
	 stat(fname,&sb);
	 
	 // Don't process directories
	 if ((sb.st_mode & S_IFDIR) != 0)
	   continue;
	 
	 sprintf(new.player_name, "%s", Dir[i]->d_name);

	 for ( ;find_more != FALSE; ) {

	   word = feof(fp) ? "END" : fread_word(fp);
	   
	   switch (UPPER(word[0])) {
	   case 'D':
	    	if (!strcmp(word, "Desc")) {
		  ptr = fread_string(fp);
		  if (ptr != NULL)
		    free_string(ptr);
		}		
		break;	   	
	   case 'E':
		if (!strcmp(word, "ELevl")) {
		  new.player_level += fread_number(fp);
		}
		break;
	   case 'L':
		KEY( "Levl", new.player_level, fread_number(fp));
		break;
	   case 'P':
		if (!strcmp(word, "Plyd")) 
		  find_more = FALSE;
		break;		
	   case 'S':
		if (!strcmp(word, "SSguild")) {
		  ptr = fread_string(fp);
		  if (!IS_NULLSTR(ptr)) {
		    sprintf(new.ssguild_name, "%s", ptr);
		  }
		  if (ptr != NULL)
		    free_string(ptr);
		  
		  if (ch->ssguild != ssguild_lookup(new.ssguild_name))
		    find_more = FALSE;
		  else
		    isSsguild = TRUE;
		  
		  break;
		}
		
		if (!strcmp(word, "SSrank")) {
		  new.ssguild_rank = fread_number(fp);
		  break;
		}
		
		if (!strcmp(word, "SStitle")) {
		  ptr = fread_string(fp);
		  if (!IS_NULLSTR(ptr)) {
		    sprintf(new.ssguild_title, "%s", ptr);
		  }
		  if (ptr != NULL)
		    free_string(ptr);
		  
		  break;
		}
		
		break;
	   }
	 }
	 
	 fclose(fp);
	 free(Dir[i]);
	 find_more = TRUE;
	 
	 if (isSsguild && ch->ssguild == ssguild_lookup(new.ssguild_name)) {
	   mCnt++;
	   
	   /* Is Victim online ? */
	   if ((victim = get_char_world(ch, new.player_name)) != NULL && !IS_NPC(victim)) {
		new.player_level = get_level(victim);		
		sprintf(new.ssguild_name, "%s", player_ssguild(victim));
		sprintf(new.ssguild_title, "%s", !IS_NULLSTR(victim->ssguild_title) ? victim->ssguild_title : ssguild_table[ssguild_lookup(new.ssguild_name)].rank[victim->ssguild_rank].rankname);
		sprintf(new.last_on_time_str, "Online");
		new.last_on_time = time(NULL);
		new.ssguild_rank = victim->ssguild_rank;		
	   }
	   else {
		new.last_on_time = sb.st_mtime;
		strtime = ctime(&new.last_on_time);
		strtime[strlen(strtime)-1] = '\0';
		sprintf(new.last_on_time_str, "%s", strtime);
	   }
	   
	   // Insert new ssguild member
	   insert_new_ssguild_member_data(new); 
	   
	   // reset used variables
	   isSsguild = FALSE;
	   new.player_name[0]   = '\0';
	   new.ssguild_name[0]  = '\0';
	   new.ssguild_title[0] = '\0';
	   new.player_level     = 0;
	   new.ssguild_rank     = 0;
	   new.last_on_time     = 0;
	   new.last_on_time_str[0] = '\0';
	   strtime = NULL;
	   ptr     = NULL;
	 }	 
    }
  }  
  
  free(Dir);
  return mCnt;
}

void print_ssguild_member_data_to_char(CHAR_DATA *ch)
{
  BUFFER *pbuf;
  char buffer[MSL];
  int i=0;
  int mCnt=0;

  pbuf = new_buf();
  
  send_to_char("{C                                        SSGUILD MEMBERS                                        {x\n\r",ch);
  send_to_char("{RName           SSGuild                     Rank Lvl  Last on                   Title           {x\n\r",ch);
  send_to_char("{R============================================================================================={x\n\r",ch);

  for (i=0; i < MAX_SSGUILD_MEBER_DATA; i++) {
    if (!IS_NULLSTR(gd[i].player_name)) {
	 mCnt++;
	 if (IS_IMMORTAL(ch)) {
	   sprintf(buffer, "{B[{c%-12s{B][{c%-24s{B][{c%3d{B][{c%3d{B][{c%24s{B][{c%s{x\n\r", 
			 gd[i].player_name, 
			 gd[i].ssguild_name,
			 gd[i].ssguild_rank+1,
			 gd[i].player_level,
			 gd[i].last_on_time_str,
			 !IS_NULLSTR(gd[i].ssguild_title) ? gd[i].ssguild_title : "n/a");
	 }
	 else {
	   sprintf(buffer, "{B[{c%-12s{B][{c%-24s{B][{c%3d{B][{c%3s{B][{c%24s{B][{c%s{x\n\r", 
			 gd[i].player_name, 
			 gd[i].ssguild_name,
			 gd[i].ssguild_rank+1,
			 "xxx",
			 gd[i].last_on_time_str,
			 !IS_NULLSTR(gd[i].ssguild_title) ? gd[i].ssguild_title : "n/a");
	 }
	 add_buf(pbuf, buffer);
    }
  }

  sprintf( buffer, "\n\r{cMembers found{C: {Y%d{x\n\r", mCnt );
  add_buf(pbuf, buffer);
  page_to_char(buf_string(pbuf), ch);
  free_buf(pbuf);  
  return;
}

int compare_ssguild_member_ranks(const void *v1, const void *v2)
{
  return (*(struct ssguild_member_data*)v1).ssguild_rank - (*(struct ssguild_member_data*)v2).ssguild_rank;
}

int compare_ssguild_member_names(const void *v1, const void *v2)
{
  return strcmp((*(struct ssguild_member_data*)v1).player_name, (*(struct ssguild_member_data*)v2).player_name);
}

int compare_ssguild_member_logons(const void *v1, const void *v2)
{
  return (*(struct ssguild_member_data*)v1).last_on_time - (*(struct ssguild_member_data*)v2).last_on_time;
}

void do_ssgmembers( CHAR_DATA *ch, char *argument )
{
  int members=0;

  if (!can_ssguild(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("You are not a leader of your sguild!\n\r", ch);
    return;
  }
  
  // Argument
  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: ssgmembers name\n", ch);
    send_to_char("        ssgmembers rank\n", ch);
    send_to_char("        ssgmembers logon\n", ch);
    return;
  }
  
  // Inity Array
  init_ssguild_member_data();
  
  // Fill array from pfiles that match this chars guid
  members  = load_ssguild_member_data(ch, PLAYER_DIR);
  members += load_ssguild_member_data(ch, PLAYER_DISGUISE_DIR);
  
  if (members <= 0) {
    send_to_char("No players found!\n\r", ch);
    return;	
  }
  
  if (!str_cmp(argument, "name")) {
    qsort (gd, members, sizeof(struct ssguild_member_data), compare_ssguild_member_names);
  }
  else if(!str_cmp(argument, "rank")) {
    qsort (gd, members, sizeof(struct ssguild_member_data), compare_ssguild_member_ranks);
  }
  else if(!str_cmp(argument, "logon")) {
    qsort (gd, members, sizeof(struct ssguild_member_data), compare_ssguild_member_logons);
  }
  else {
    send_to_char("Syntax: ssgmembers name\n", ch);
    send_to_char("        ssgmembers rank\n", ch);
    send_to_char("        ssgmembers logon\n", ch);
    return;
  }

  // Print result to screen
  print_ssguild_member_data_to_char(ch);
  
  return;
}

