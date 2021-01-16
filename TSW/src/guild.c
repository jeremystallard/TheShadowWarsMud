/***************************************************************************
 * This file include code special dedicated to guild handeling             *
 * TSW is copyright -2005 Swordfish and Zandor                             *
 **************************************************************************/
#include <dirent.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "olc.h"
#include "interp.h"
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

void show_flag_cmds( CHAR_DATA *ch, const struct flag_type *flag_table );

char *guild_bit_name( int guild_flags )
{
  static char buf[512];
  
  buf[0] = '\0';
  if ( guild_flags & GUILD_INDEPENDENT	) 
    strcat( buf, " independent"	);

  if ( guild_flags & GUILD_CHANGED	) 
    strcat( buf, " changed"	);

  if ( guild_flags & GUILD_DELETED	) 
    strcat( buf, " deleted"	);

  if ( guild_flags & GUILD_WOLF		) 
    strcat( buf, " wolfkin"	);

  if ( guild_flags & GUILD_IMMORTAL	) 
    strcat( buf, " immortal"	);
  
  return ( buf[0] != '\0' ) ? buf+1 : "none";
}

void load_guilds(void)
{
  FILE *fp;
  char buf[MAX_STRING_LENGTH];
  char logbuf[256];
  char *string;
  int count = 0;
  int i;
  bool fMatch = FALSE;
  
  for (i=0; i < MAX_CLAN; i++) {
    clan_table[i].name = "";
    clan_table[i].who_name = "";
    clan_table[i].room[0]= 0;
    clan_table[i].room[1]= 0;
    clan_table[i].room[2]= 0;
    clan_table[i].rank[0].rankname = "";
    clan_table[i].rank[0].skillname = "";
    clan_table[i].ml[0] = 0;
    clan_table[i].ml[1] = 0;
    clan_table[i].ml[2] = 0;
    clan_table[i].ml[3] = 0;
    clan_table[i].flags = 0;
  }
    
  sprintf(buf, "%sguild.dat", DATA_DIR);

  sprintf(logbuf, "Loading guild data file %s", buf);
  log_string(logbuf);

  if ((fp = fopen(buf, "r")) == NULL) {
    log_string("Error: guild.dat file not found!");
    exit(1);
  }
  for (;;) {
    string = feof(fp) ? "End" : fread_word(fp);
    
    if (!str_cmp(string, "End"))
	 break;
    
    switch (UPPER(string[0])) {
    case 'F':
	 clan_table[count].flags  = fread_flag( fp );
	 fMatch = TRUE;
	 break;
	 
    case 'G':
	 count++;
	 clan_table[count].name = fread_string(fp);
	 fMatch = TRUE;
	 break;

    case 'R':
	 if (!str_cmp(string, "Rooms")) {
	   clan_table[count].room[0] = fread_number(fp);	/* hall   */
	   clan_table[count].room[1] = fread_number(fp);	/* morgue */
	   clan_table[count].room[2] = fread_number(fp);	/* temple */
	   fMatch = TRUE;
	 } 
	 else if (!str_cmp(string, "Rank")) {
	   i = fread_number(fp);
	   clan_table[count].rank[i - 1].rankname = fread_string(fp);
	   fMatch = TRUE;
	 }
	 break;

    case 'S':
	 i = fread_number(fp);
	 clan_table[count].rank[i - 1].skillname = fread_string(fp);
	 fMatch = TRUE;
	 break;
	 
    case 'M':
	 clan_table[count].ml[0] = fread_number(fp);
	 clan_table[count].ml[1] = fread_number(fp);
	 clan_table[count].ml[2] = fread_number(fp);
	 clan_table[count].ml[3] = fread_number(fp);
	 fMatch = TRUE;
	 break;
	 
    case 'W':
	 clan_table[count].who_name = fread_string(fp);
	 fMatch = TRUE;
	 break;
	 
    }			/* end of switch */
    
  }				/* end of while (!feof) */
  
  if (!fMatch) {
    bug("Fread_guilds: no match.", 0);
    fread_to_eol(fp);
  }
  fclose(fp);

  sprintf(logbuf, "Loaded %d guilds successfully.", count);
  log_string(logbuf);

  return;
} /* end: load_guilds */

bool is_leader(CHAR_DATA * ch)
{
     return IS_SET(ch->act, PLR_MORTAL_LEADER) ? 1 : 0;
}

bool can_guild(CHAR_DATA * ch)
{
  /* ok if ch is a IMMORTAL or higher */
  /*if (ch->level >= IMMORTAL || ch->trust >= IMMORTAL) */
  if (IS_IMMORTAL(ch))
    return TRUE;
  
  /* not ok if ch is not guilded or is not a mortal leader */
  if (ch->clan == 0 || !is_leader(ch))
    return FALSE;

  return clan_table[ch->clan].ml[0];
} /* end: can_guild */


bool can_deguild(CHAR_DATA * ch)
{
  /* ok if ch is a SUPREME or higher */
  /* if (ch->level >= SUPREME || ch->trust >= SUPREME)*/
  if (IS_IMMORTAL(ch))
    return TRUE;
  
  /* not ok if ch is not guilded or is not a mortal leader */
  if (ch->clan == 0 || !is_leader(ch))
    return FALSE;
  
  return clan_table[ch->clan].ml[1];
} /* end: can_deguild */


bool can_promote(CHAR_DATA * ch)
{
  /* ok if ch is a IMMORTAL */
  if (IS_IMMORTAL(ch))
    return TRUE;
  
  /* not ok if ch is not guilded or is not a mortal leader */
  if (ch->clan == 0 || !is_leader(ch))
    return FALSE;
  
  /* is a mortal leader, but do they have the right? */
  return clan_table[ch->clan].ml[2];
} /* end: can_promote */


bool can_demote(CHAR_DATA * ch)
{
  /* ok if ch is a IMMORTAL */
  if (IS_IMMORTAL(ch))
    return TRUE;
  
  /* not ok if ch is not guilded or is not a mortal leader */
  if (ch->clan == 0 || !is_leader(ch))
    return FALSE;
  
  return clan_table[ch->clan].ml[3];
} /* end: can_demote */


bool is_clan(CHAR_DATA * ch)
{
  return ch->clan;
} /* end: is_clan */


bool is_same_clan(CHAR_DATA * ch, CHAR_DATA * victim)
{
  if (IS_SET(clan_table[ch->clan].flags,GUILD_INDEPENDENT))
    return FALSE;
  else
    return (ch->clan == victim->clan);
} /* end: is_same_clan */


int clan_lookup(const char *name)
{
  int clan;
  
  for (clan = 0; clan < MAX_CLAN; clan++) {
    if (!str_prefix(name, clan_table[clan].name))
	 return clan;
  }
  
  return 0;
} /* end: clan_lookup */


char *player_rank(CHAR_DATA * ch)
{
  if (ch->clan == 0)
    return '\0';
  return clan_table[ch->clan].rank[ch->rank].rankname;
} /* end: player_rank */


char *player_clan(CHAR_DATA * ch)
{
  if (ch->clan == 0)
    return '\0';
  return clan_table[ch->clan].name;
} /* end: player_clan */

void reset_gstatus(CHAR_DATA *ch)
{
  ch->clan                    = 0;
  ch->rank                    = 0;
  ch->pcdata->guild_offer     = 0;
  ch->pcdata->guild_requestor = 0;
  ch->pcdata->guilded_by      = 0;
  
  free_string(ch->gtitle);
  ch->gtitle = NULL;

  ch->ginvis = FALSE;
  ch->gmute  = FALSE;

  if (IS_SET(ch->act, PLR_MORTAL_LEADER))
     REMOVE_BIT(ch->act, PLR_MORTAL_LEADER);
}

void set_gtitle( CHAR_DATA *ch, char *gtitle )
{
  // validate
  if (IS_NULLSTR(gtitle))
    return;
  
  free_string( ch->gtitle );
  ch->gtitle = str_dup(gtitle);
  return;
}

void do_promote(CHAR_DATA * ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  int cnt;
  int sn = 0;
  
  argument = one_argument(argument, arg1);
  
  if (!can_promote(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (arg1[0] == '\0' || argument[0] == '\0') {

    send_to_char("Syntax: promote <who> <rank #>\n\r", ch);
    send_to_char("Where rank is one of the following:\n\r", ch);
    
    for (cnt = 0; cnt < MAX_RANK; cnt++) {
	 sprintf(buf, "%2d] %s\n\r", cnt + 1,
		    is_clan(ch) ? clan_table[ch->clan].rank[cnt].rankname : "(None)");
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
  
  if (!is_clan(victim)) {
    send_to_char("They are not a member of any guilds!\n\r", ch);
    return;
  }
  
  if (!is_same_clan(ch, victim) && !IS_IMMORTAL(ch)) {
    send_to_char("They are a member of a guild different than yours!\n\r", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim)) {
    send_to_char("You can't promote Immortals.\n\r", ch);
    return;
  }

  if (!str_prefix(argument, "leader") && ch != victim) {
    SET_BIT(victim->act, PLR_MORTAL_LEADER);
    send_to_char("They are now a mortal leader.\n\r", ch);
    send_to_char("You have just been promoted to a leader of your guild!\n\r", victim);
    victim->rank = 2;

    // Save info
    save_char_obj(victim, FALSE);

    return;
  }
  
  cnt = atoi(argument) - 1;
  if (cnt < 0 ||
	 cnt > MAX_RANK -1 ||
	 clan_table[victim->clan].rank[cnt].rankname == NULL) {
    send_to_char("That rank does not exist!", ch);
    return;
  }
  if (cnt > victim->rank && ((ch == victim) & (!IS_IMMORTAL(ch)))) {
    send_to_char("Heh. I dont think so...", ch);
    return;
  }
  
  if (cnt < victim->rank) {
    int i;
    
    sprintf(buf, "You have been promoted to %s!\n\r",
		  clan_table[victim->clan].rank[cnt].rankname);
    send_to_char(buf, victim);
    
    sprintf(buf, "%s has been promoted to %s!\n\r",
		  PERS_NAME(victim,ch), clan_table[victim->clan].rank[cnt].rankname);
    send_to_char(buf, ch);
    
    set_gtitle(victim, clan_table[victim->clan].rank[cnt].rankname);
    
    for (i = victim->rank; i >= cnt; i--)
	 if (clan_table[victim->clan].rank[i].skillname != NULL) {
	   sn = skill_lookup(clan_table[victim->clan].rank[i].skillname);
	   if (sn < 0) {
		sprintf(buf, "Bug: Add skill [%s] is not a valid skill",
			   clan_table[victim->clan].rank[cnt].skillname);
		log_string(buf);
		} 
	   else if (!victim->pcdata->learned[sn]) {
		victim->pcdata->learned[sn] = 20 + (victim->level / 4);
		sprintf(buf, "You have been guild granted '%s' as a result of your promotion!\n\r",
			   clan_table[victim->clan].rank[i].skillname);
		send_to_char(buf, victim);
	   }
	 }
  } 
  
  else if (cnt > victim->rank) {
    if (IS_SET(victim->act, PLR_MORTAL_LEADER))
	 REMOVE_BIT(victim->act, PLR_MORTAL_LEADER);
    
    sprintf(buf, "You have been demoted to %s!\n\r",
		  clan_table[victim->clan].rank[cnt].rankname);

    set_gtitle(victim, clan_table[victim->clan].rank[cnt].rankname);
    
    send_to_char(buf, victim);

    sprintf(buf, "%s has been demoted to %s!\n\r",
		  PERS_NAME(victim, ch), clan_table[victim->clan].rank[cnt].rankname);    
    send_to_char(buf, ch);
    
    // If ginvis, strip it and tell.
    if (victim->ginvis) {
  	ch->ginvis = FALSE;	
  	send_to_char("You are now visible to your guild.\n\r", victim);    	
    }
    
    /*
	* ---------------------------------------------------------------
	* Note: I dont think it would be fair here to take away any skills
	* the victim may have earned at a higher rank. It makes no RP sense
	* to do so and only hurts the player (loss of practices etc). Imms
	* may want to keep an eye on this, as we dont want players jumping
	* guilds just to gain new skills.
	* -------------------------------------------------------------- 
	*/
  }				/* else no change */

  victim->rank = cnt;

  // Save info
  save_char_obj(victim, FALSE);

  return;
} /* end: do_promote */


/*
 * Immortal guild command for joining/banis
 */
void do_guild(CHAR_DATA * ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  int clan;
  
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  
  if (!can_guild(ch) || !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (arg1[0] == '\0' || arg2[0] == '\0') {
    send_to_char("Syntax: guild <char> <guild name>\n\r", ch);
    return;
  }

/*  if ((victim = get_char_world(ch, arg1)) == NULL) { */
  if ((victim = get_realname_char_world(ch, arg1)) == NULL) { 
    send_to_char("They aren't playing.\n\r", ch);
    return;
  }

  /** thanks to Zanthras for the bug fix here...*/
  if (is_clan(victim) && !is_same_clan(ch, victim) &&
	 ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("They are a member of a guild other than your own.\n\r", ch);
    return;
  }

  if (!str_prefix(arg2, "none")) {
    
    if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim)) {
	 send_to_char("You can not set guild status on the Immortals!\n\r", ch);
	 return;
    }
    
    send_to_char("They are no longer a member of any guild.\n\r", ch);
    send_to_char("You are no longer a member of any guild!\n\r", victim);

    reset_gstatus(victim);

    if (IS_SET(victim->act, PLR_MORTAL_LEADER))
	 REMOVE_BIT(victim->act, PLR_MORTAL_LEADER);
    
    return;
  }
  
  if ((clan = clan_lookup(arg2)) == 0) {
    send_to_char("No such guild exists.\n\r", ch);
    return;
  }
  
  sprintf(buf, "They are now %s of the %s.\n\r",
		clan_table[clan].rank[MAX_RANK-1].rankname, clan_table[clan].name);
  send_to_char(buf, ch);
  
  sprintf(buf, "You are now %s of the %s.\n\r",
		clan_table[clan].rank[MAX_RANK-1].rankname, clan_table[clan].name);
  send_to_char(buf, victim);
  
  victim->clan = clan;
  victim->rank = MAX_RANK-1;		/* lowest, default */
} /* end: do_guild */

/*
 * Set a members guild title if leader
 */
void do_guildtitle( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;

  smash_tilde( argument );
  argument = one_argument(argument, arg1);
  strcpy(arg2, argument);
  
  if (!can_guild(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if ( (arg1[0] == '\0') || (arg2[0] == '\0')) {
    send_to_char("Syntax: gtitle <char> <title>\n\r", ch);
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

  if (is_clan(victim) && !is_same_clan(ch, victim) &&
	 ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("They are a member of a guild other than your own.\n\r", ch);
    return;
  }

  if (ch != victim && IS_IMMORTAL(victim)) {
     send_to_char("You can't set guild titles on immortals.\n\r", ch);
     return;
  }

  set_gtitle(victim, arg2);

  sprintf(buf, "You set %s's guild title to %s.\n\r", PERS_NAME(victim, ch), victim->gtitle);
  send_to_char(buf, ch);
  
  sprintf(buf, "%s has set your guild title to %s.\n\r", PERS_NAME(ch, victim), victim->gtitle);
  send_to_char(buf, victim);

  // Save info
  save_char_obj(victim, FALSE);
  
  return;
}

int cnt_guild_guards(CHAR_DATA *ch)
{
  CHAR_DATA *wch=NULL;
  int cnt=0;
  
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {
    if (!IS_NPC(wch))
	 continue;
    if (wch->guild_guard == ch->clan)
	 cnt++;
  }
  
  return cnt;
}

int compare_ranks(const void *v1, const void *v2)
{
  return (*(CHAR_DATA**)v1)->rank - (*(CHAR_DATA**)v2)->rank;
}

/*
 * Show current playing and wizible members of a Guild
 */
void do_guildlist( CHAR_DATA *ch, char *argument )
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

  if (!is_clan(ch)) {
    send_to_char("You aren't in a guild.\n\r",ch);
    return;
  }

  if (ch->rank == 9 && !IS_IMMORTAL(ch))
  {
	send_to_char("You are flagged at glevel 9 and do not yet have this privilege.\n\r",ch);
        return;
  }
  
  output = new_buf();
  
  for (d = descriptor_list; d != NULL; d = d->next) {
    
    if (d->connected != CON_PLAYING || !can_see_channel(ch,d->character))
	 continue;
    
    wch = ( d->original != NULL ) ? d->original : d->character;
    
    if (!can_see_channel(ch,wch))
	 continue;
    
    if (wch->clan != ch->clan && wch->oguild != ch->clan)
	 continue;

    // Only guild leaders can see other ginvis members
    if ( (wch->ginvis == TRUE) && (wch != ch) && !can_guild(ch) )
    	continue;

    // If immortal and ginvis, then none can see you
    if (wch->ginvis == TRUE && IS_IMMORTAL(wch) && (wch != ch))
	 continue;

    wch->trank = wch->oguild == ch->clan ? wch->oguild_rank : wch->rank;

    pcs[cnt++] = wch;
  }

  /* Sort PC array based on name first */
  qsort (pcs, cnt, sizeof(wch), compare_char_names);

  /* Then we sort based on rank */
  qsort (pcs, cnt, sizeof(wch), compare_cross_ranks);
  
  for (i=0; i < cnt; i++) {
    if (pcs[i]->oguild == ch->clan) {
	 fillers = (16 - colorstrlen(clan_table[pcs[i]->oguild].who_name)-1);
	 fillers2 = (16 - colorstrlen(PERS_OLD(pcs[i], ch))-1);
	 sprintf(buf, "[%s%*s (%d)] %s%*s (%s) %s%s%s\n\r", clan_table[pcs[i]->oguild].who_name, 
		    fillers, "",
		    pcs[i]->oguild_rank+1, 
		    PERS_OLD(pcs[i], ch),
		    fillers2, "",
		    pcs[i]->oguild_title ? pcs[i]->oguild_title : player_oguild_rank(pcs[i]),
		    "({Bo{x)",
		    pcs[i]->oguild_invis ? "({Ygi{x)" : "",
		    pcs[i]->oguild_mute  ? "({Rgm{x)" : "");
	 add_buf(output,buf);
    }
    else {
	 fillers = (16 - colorstrlen(clan_table[pcs[i]->clan].who_name)-1);
	 fillers2 = (16 - colorstrlen(PERS_OLD(pcs[i], ch))-1);
	 sprintf(buf, "[%s%*s (%d)] %s%*s (%s) %s%s%s\n\r", clan_table[pcs[i]->clan].who_name, 
		    fillers, "",
		    pcs[i]->rank+1, 
		    PERS_OLD(pcs[i], ch),
		    fillers2, "",
		    pcs[i]->gtitle ? pcs[i]->gtitle : player_rank(pcs[i]),
		    is_leader(pcs[i]) ? "({Ygl{x)" : "",
		    pcs[i]->ginvis ? "({Ygi{x)" : "",
		    pcs[i]->gmute  ? "({Rgm{x)" : "");
	 add_buf(output,buf);
    }
  }

  sprintf(buf, "\nMembers found: {y%d{x.  Guild guards: {y%d{x.\n\r", cnt,  cnt_guild_guards(ch));
  add_buf(output,buf);
  
  page_to_char(buf_string(output),ch);
  free_buf(output);
}

/*
 * List all awailable guilds
 */
void do_guildslist( CHAR_DATA *ch, char *argument )
{
  char buf[MIL];
  BUFFER *buffer;
  int i;

  buffer = new_buf();
  
  sprintf(buf, "Available Guilds:\n\r"
		"----------------\n\r");
  add_buf(buffer, buf);
  
  for (i=1; i <= MAX_CLAN; i++) {
    if (clan_table[i].name != NULL && clan_table[i].name[0] != '\0') {
	 sprintf(buf,"[%d] %-16s\n\r", i, clan_table[i].who_name);
      add_buf(buffer, buf);
    }
  }
  
  page_to_char( buf_string(buffer), ch );
  free_buf(buffer);
  return;
}

/*
 * Offer a PC to join a guild
 */
void do_join( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  int clan;
  CHAR_DATA *victim;
  
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  
  /* Get rid of people that can't guild */    
  if (!can_guild(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
    send_to_char("Syntax:\n\r  join <char> <guild name>\n\r"
			  "Use the '{Wguildslist{x' command to see awailable guilds.\n\r"
			  ,ch);
    return;
  }
  
  if (strcasecmp(arg2, "none") == 0) {
    send_to_char("Try the banish command.\n\r",ch);
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

  if (!victim)
  {
	send_to_char("They aren't here.\r\n",ch);
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
    if ( ( victim = get_introname_char_room( ch, arg1)) == NULL ) {
	 send_to_char( "They aren't in this room or they have not introduced themself to you.\n\r", ch );
	 return;
    }
  }
  */

  /* Get rid of people that are trying to guild themselves */
  if (ch == victim && !IS_IMMORTAL(ch)) {
    send_to_char("You can't join your self to a guild!\n\r", ch);
    return;
  }

  if IS_NPC(victim) {
    send_to_char("Mobiles can't be guilded!\n\r",ch);
    return;
  }

  if (is_clan(victim) && !is_same_clan(ch, victim) &&
	 !IS_IMMORTAL(ch)) {
    send_to_char("They are a member of a guild other than your own.\n\r", ch);
    return;
  }

  if ((clan = clan_lookup(arg2)) == 0) {
    send_to_char("No such guild exists.\n\r", ch);
    return;
  }
  
  if (ch->clan != clan) {
     send_to_char("You can't join to another guild than the one you are in.\n\r", ch);
     return;	
  }  

  /* Offer */
  sprintf(buf, "Your offer to %s has been made.\n\r", PERS_NAME(victim, ch));
  send_to_char(buf, ch);

  victim->pcdata->guild_offer     = clan;
  victim->pcdata->guild_requestor = ch->id;

  sprintf(buf,"%s has offered you a position in the guild: [%s].\n\r",
		PERS(ch, victim), clan_table[clan].name);
  send_to_char(buf, victim);
  send_to_char("Type {yaccept{x to join.\n\rType {yrefuse{x to decline offer.\n\r", victim);
  
  return;
}

/*
 * Option to accept a given offer to join a Guild
 */
void do_accept( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *vch;
  char buf[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  if (ch->pcdata->guild_offer == 0) 
    return;

  if (argument[0] != '\0') {
    sprintf(buf, "Type accept to join the guild: [%s].\n\r",
		  clan_table[ch->pcdata->guild_offer].name);
    send_to_char(buf, ch);
    return;
  }

  ch->clan                = ch->pcdata->guild_offer;
  ch->rank                = MAX_RANK-1;
  ch->pcdata->guild_offer = 0;
  ch->pcdata->guilded_by  = ch->pcdata->guild_requestor;

  sprintf(buf, "You have joined the guild: [%s].\n\r", clan_table[ch->clan].name);
  send_to_char(buf, ch);
  
  if ((vch = get_charId_world(ch, ch->pcdata->guilded_by)) != NULL) {
    sprintf(buf, "You have notifed %s that you have joined the guild: [%s].\n\r",
		  PERS(vch, ch), clan_table[ch->clan].name);
    send_to_char(buf, ch);
    
    sprintf(buf, "%s has accepted your offer and join the guild: [%s].\n\r",
		  PERS_NAME(ch, vch), clan_table[ch->clan].name);
    send_to_char(buf, vch);
  }
  
  // Save info
  save_char_obj( ch, FALSE);
}

/*
 * Option to refuse to join a offered Guild
 */
void do_refuse( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *vch;
  char buf[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  if (ch->pcdata->guild_offer == 0) 
    return;

  if (argument[0] != '\0') {
    send_to_char("Just type refuse to decline the offer to join.\n\r", ch);
    return;
  }
  
  sprintf(buf, "You refused to join the guild: [%s].\n\r",
		clan_table[ch->pcdata->guild_offer].name);
  send_to_char(buf, ch);


  if ((vch = get_charId_world(ch, ch->pcdata->guild_requestor)) != NULL) {
    sprintf(buf, "You have notifed %s that you will not join the guild: [%s].\n\r",
		  PERS(vch, ch), clan_table[ch->pcdata->guild_offer].name);
    send_to_char(buf, ch);

    sprintf(buf, "%s has refused your offer to join the guild: [%s].\n\r",
		  PERS_NAME(ch, vch),  clan_table[ch->pcdata->guild_offer].name);
    send_to_char(buf, vch);

    ch->pcdata->guild_offer     = 0;      
    ch->pcdata->guild_requestor = 0;
  }

  // Save info
  save_char_obj( ch, FALSE);
  return;
}

// Offline banish
void do_obanish( CHAR_DATA *ch, char *argument )
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
  if (!can_deguild(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  // Syntax
  if ( arg1[0] == '\0') {
    send_to_char("Syntax: offbanish <char> [reason]. \n\r",ch);
    return;
  }

  // Check that not self banish
  if (!str_cmp(ch->name, arg1)) {
    send_to_char("You can't obanish your self. Please see one of the immortals.\n\r", ch);
    return;
  }

  // Make sure char not already loaded!
  if ( (vch = get_char_world( ch, arg1)) != NULL) {
    sprintf(buf, "%s is already connected. Please use normal banish.\n\r", arg1);
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
  if (is_clan(d->character) && !is_same_clan(ch, d->character) &&
	 ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("You can't banish that person!.\n\r", ch);
    return;
  }

  // Banish
  reset_gstatus(d->character);

  // Info
  sprintf(buf, "You have banished %s from the %s{x guild.\n\r", arg1, clan_table[ch->clan].who_name);
  send_to_char(buf,ch);
  sprintf(buf, "A note has been left to %s with information about the banish.\n\r", arg1);
  send_to_char(buf, ch);

  //Send the banished person a note
  sprintf(note_buf1,
		"%s{x,\n\n\r"
		"You have been banished from the %s guild with the following reason:\n\n\r"
		"o %s\n\n\r"
		"Signed by the hand of %s{x,\n\r"
		"%s.\n\r",
		arg1,
		clan_table[ch->clan].who_name,
		IS_NULLSTR(argument) ? "No reason given." : argument,
		ch->name,
		clan_table[ch->clan].who_name);
		//ch->in_room->area->name);

  sprintf(note_to, "%s %sLeader", arg1, player_clan(ch));
  
  sprintf(note_buf2, "You have been banished from the %s guild.", clan_table[ch->clan].name);
  
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

void do_banish( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *vch;

  argument = one_argument(argument, arg1);

  /* Do the player have the rights to banish ? */
  if (!can_deguild(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if ( arg1[0] == '\0') {
    send_to_char("Syntax: banish <char>. \n\r",ch);
    return;
  }

  if ( (vch = get_char_world( ch, arg1)) == NULL) {
  /* if ( (vch = get_introname_char_world( ch, arg1)) == NULL) { */
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }

  /* Check mobiles */
  if IS_NPC(vch) {
    send_to_char("Mobiles can't be guilded or banished!\n\r",ch);
    return;
  }

  /* The self check */
  if ((ch == vch) && !IS_IMMORTAL(ch)) {
    send_to_char("You can't banish your self. Please see one of the immortals.\n\r", ch);
    return;
  }

  if (is_clan(vch) && !is_same_clan(ch, vch) &&
	 ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("You can't banish that person!.\n\r", ch);
    return;
  }

  /* Banish */
  reset_gstatus(vch);
  
  sprintf(buf,"%s has banished you from your guild.\n\r",PERS_NAME(ch, vch));
  send_to_char(buf,vch);
  
  sprintf(buf, "You have banished %s.\n\r",PERS_NAME(vch, ch));
  send_to_char(buf,ch);

  // Save info
  save_char_obj(vch, FALSE);
    
  /* Done */
  return;
}

void do_guildinvis( CHAR_DATA *ch, char *argument )
{

  if (!can_guild(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (ch->ginvis != TRUE) {
  	ch->ginvis = TRUE;
  	send_to_char("You are now guild invis.\n\r", ch);
  	return;
  }
  else {
  	ch->ginvis = FALSE;	
  	send_to_char("You are now visible to your guild.\n\r", ch);
  	return;
  }
  
  return;
}

void do_guildmute( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim=NULL;
  
  if (!can_guild(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: guildmute <guild member>\n\r", ch);
    return;
  }
  
  if ((victim = get_realname_char_world( ch, argument)) == NULL ) {  
    send_to_char( "They aren't playing.\n\r", ch );
    return;
  }

  if (!is_clan(victim)) {
    send_to_char("They are not a member of your guild!\n\r", ch);
    return;
  }

  if (!is_same_clan(ch, victim) && !IS_IMMORTAL(ch)) {
    send_to_char("They are a member of a guild different than yours!\n\r", ch);
    return;
  }
  
  if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim)) {
    send_to_char("You can't mute Immortals.\n\r", ch);
    return;
  }
  
  if (victim->gmute != TRUE) {
    victim->gmute = TRUE;
    send_to_char("You have revoked their guild channel priviliges.\n\r", ch);
    act("$n has revoked your guild channel priviliges.", ch, NULL, victim, TO_VICT);
    return;
  }
  else {
    victim->gmute = FALSE;
    send_to_char("You have restored their guild channel priviliges.\n\r", ch);
    act("$n has restored your guild channel priviliges.", ch, NULL, victim, TO_VICT);
    return;
  }
  
  return;
}

typedef struct guild_member_data {
  char player_name[64];
  int  player_level;
  char guild_name[128];
  char guild_title[512];
  time_t last_on_time;
  char last_on_time_str[64];
  int  guild_rank;
}guild_member_data;

#define MAX_GUILD_MEBER_DATA 100

struct guild_member_data gd[MAX_GUILD_MEBER_DATA];

void init_guild_member_data()
{
  int i;
  
  for (i=0; i < MAX_GUILD_MEBER_DATA; i++) {
    gd[i].player_name[0]      = '\0';
    gd[i].player_level        = 0;
    gd[i].guild_name[0]       = '\0';
    gd[i].guild_title[0]      = '\0';
    gd[i].last_on_time        = 0;
    gd[i].last_on_time_str[0] = '\0';
    gd[i].guild_rank=0;
  }
  
  return;
}

int find_free_guild_member_data_slot()
{
  int i;
  
  for (i=0; i < MAX_GUILD_MEBER_DATA; i++) {
    if (IS_NULLSTR(gd[i].player_name))
	 return i;
  }
  
  return -1;
}

bool insert_new_guild_member_data(struct guild_member_data new)
{
  int i = find_free_guild_member_data_slot();
  
  if (i < 0 || i > MAX_GUILD_MEBER_DATA)
    return FALSE;
  
  strcpy(gd[i].player_name, new.player_name);
  strcpy(gd[i].guild_name, new.guild_name);
  strcpy(gd[i].guild_title, new.guild_title);
  strcpy(gd[i].last_on_time_str, new.last_on_time_str);

  gd[i].player_level = new.player_level;
  gd[i].last_on_time = new.last_on_time;
  gd[i].guild_rank   = new.guild_rank;
  
  return TRUE;
}

int load_guild_member_data(CHAR_DATA *ch, char *path)
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
  bool isGuild = FALSE;
  int mCnt=0;
  bool fMatch;
  char *ptr=NULL;
  char *strtime=NULL;
  
  struct guild_member_data new;
  
  n = scandir(path, &Dir, 0, alphasort);

  if (n < 0)
    return n;

  // Init tmp gmd
  new.player_name[0]      = '\0';
  new.player_level        = 0;
  new.guild_name[0]       = '\0';
  new.guild_title[0]      = '\0';
  new.last_on_time        = 0;
  new.last_on_time_str[0] = '\0';
  new.guild_rank          = 0;
  
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
	   case 'C':
		if (!strcmp(word, "Clan")) {
		  ptr = fread_string(fp);
		  if (!IS_NULLSTR(ptr)) {
		    sprintf(new.guild_name, "%s",  ptr);
		  }
		  if (ptr != NULL)
		    free_string(ptr);
		  
		  if (ch->clan != clan_lookup(new.guild_name))
		    find_more = FALSE;
		  else
		    isGuild = TRUE;
		}
		break;
	   case 'D':
		if (!strcmp(word, "Desc")) {
		  ptr = fread_string( fp );
		  if (ptr != NULL)
		    free_string(ptr);
		}
		break;
	   case 'E':
		if (!strcmp(word, "ELevl")) {
		  new.player_level += fread_number(fp);
		}
		break;
	   case 'G':
		if (!strcmp(word, "Gtitle")) {
		  ptr = fread_string(fp);
		  if (!IS_NULLSTR(ptr)) {
		    sprintf(new.guild_title, "%s", ptr);
		  }
		  if (ptr != NULL)
		    free_string(ptr);
		}
		break;
	   case 'L':
		KEY( "Levl", new.player_level, fread_number(fp));
		break;
	   case 'P':
		if (!strcmp(word, "Plyd")) 
		  find_more = FALSE;
		break;		
	   case 'R':
		KEY( "Rank", new.guild_rank, fread_number(fp));
		break;
	   }
	 }
	 
	 fclose(fp);
	 free(Dir[i]);
	 find_more = TRUE;
	 
	 if (isGuild && ch->clan == clan_lookup(new.guild_name)) {
	   mCnt++;
	   
	   /* Is Victim online ? */
	   if ((victim = get_char_world(ch, new.player_name)) != NULL && !IS_NPC(victim)) {
		new.player_level = get_level(victim);		
		sprintf(new.guild_name, "%s", player_clan(victim));
		sprintf(new.guild_title, "%s", !IS_NULLSTR(victim->gtitle) ? victim->gtitle : clan_table[clan_lookup(new.guild_name)].rank[victim->rank].rankname);
		sprintf(new.last_on_time_str, "Online");
		new.last_on_time = time(NULL);
		new.guild_rank = victim->rank;		
	   }
	   else {
		new.last_on_time = sb.st_mtime;
		strtime = ctime(&new.last_on_time);
		strtime[strlen(strtime)-1] = '\0';
		sprintf(new.last_on_time_str, "%s", strtime);		
	   }
	   
	   // Insert new guild member
	   insert_new_guild_member_data(new); 

	   // reset used variables
	   isGuild = FALSE;
	   new.player_name[0] = '\0';
	   new.guild_name[0]  = '\0';
	   new.guild_title[0] = '\0';
	   new.player_level   = 0;
	   new.guild_rank     = 0;
	   new.last_on_time   = 0;
	   new.last_on_time_str[0] = '\0';
	   strtime = NULL;
	   ptr     = NULL;
	 }	 
    }
  }  
  
  free(Dir);
  return mCnt;
}

void print_guild_member_data_to_char(CHAR_DATA *ch)
{
  BUFFER *pbuf;
  char buffer[MSL];
  int i=0;
  int mCnt=0;

  pbuf = new_buf();
  
  send_to_char("{C                                        GUILD MEMBERS                                        {x\n\r",ch);
  send_to_char("{RName           Guild                     Rank Lvl  Last on                   Title           {x\n\r",ch);
  send_to_char("{R============================================================================================={x\n\r",ch);

  for (i=0; i < MAX_GUILD_MEBER_DATA; i++) {
    if (!IS_NULLSTR(gd[i].player_name)) {
	 mCnt++;
	 if (IS_IMMORTAL(ch)) {
	   sprintf(buffer, "{B[{c%-12s{B][{c%-24s{B][{c%3d{B][{c%3d{B][{c%24s{B][{c%s{x\n\r", 
			 gd[i].player_name, 
			 gd[i].guild_name,
			 gd[i].guild_rank+1,
			 gd[i].player_level,
			 gd[i].last_on_time_str,
			 !IS_NULLSTR(gd[i].guild_title) ? gd[i].guild_title : "n/a");
	 }
	 else {
	   sprintf(buffer, "{B[{c%-12s{B][{c%-24s{B][{c%3d{B][{c%3s{B][{c%24s{B][{c%s{x\n\r", 
			 gd[i].player_name, 
			 gd[i].guild_name,
			 gd[i].guild_rank+1,
			 "xxx",
			 gd[i].last_on_time_str,
			 !IS_NULLSTR(gd[i].guild_title) ? gd[i].guild_title : "n/a");
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

int compare_guild_member_ranks(const void *v1, const void *v2)
{
  return (*(struct guild_member_data*)v1).guild_rank - (*(struct guild_member_data*)v2).guild_rank;
}

int compare_guild_member_names(const void *v1, const void *v2)
{
  return strcmp((*(struct guild_member_data*)v1).player_name, (*(struct guild_member_data*)v2).player_name);
}

int compare_guild_member_logons(const void *v1, const void *v2)
{
  return (*(struct guild_member_data*)v1).last_on_time - (*(struct guild_member_data*)v2).last_on_time;
}

void do_gmembers( CHAR_DATA *ch, char *argument )
{
  int members=0;

  if (!can_guild(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("You are not a leader of your guild!\n\r", ch);
    return;
  }
  
  // Argument
  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: gmembers name\n\r", ch);
    send_to_char("        gmembers rank\n\r", ch);
    send_to_char("        gmembers logon\n\r", ch);
    return;
  }
  
  // Inity Array
  init_guild_member_data();
  
  // Fill array from pfiles that match this chars guid
  members  = load_guild_member_data(ch, PLAYER_DIR);
  members += load_guild_member_data(ch, PLAYER_DISGUISE_DIR);
  
  if (members <= 0) {
    send_to_char("No players found!\n\r", ch);
    return;	
  }
  
  if (!str_cmp(argument, "name")) {
    qsort (gd, members, sizeof(struct guild_member_data), compare_guild_member_names);
  }
  else if(!str_cmp(argument, "rank")) {
    qsort (gd, members, sizeof(struct guild_member_data), compare_guild_member_ranks);
  }
  else if(!str_cmp(argument, "logon")) {
    qsort (gd, members, sizeof(struct guild_member_data), compare_guild_member_logons);
  }
  else {
    send_to_char("Syntax: gmembers name\n\r", ch);
    send_to_char("        gmembers rank\n\r", ch);
    send_to_char("        gmembers logon\n\r", ch);
    return;
  }

  // Print result to screen
  print_guild_member_data_to_char(ch);
  
  return;
}

void do_grecall( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  ROOM_INDEX_DATA *location=NULL;
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can recall.\n\r",ch);
    return;
  }

  if (!IS_SET(ch->act, ACT_PET) && !is_clan(ch)) {
    send_to_char("You aren't in a guild.\n\r",ch);
    return;
  }
  
  if (ch->fighting) {
    send_to_char("You are still fighting!\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)) {
     send_to_char("The gods has forsaken you!\n\r", ch);
     return;	
  }  
  
  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\n\r", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* Is blocking an exit? */
  if (IS_BLOCKING(ch)) {
    sprintf(buf, "You are still trying to block the %s entrance.\n\r", dir_name[ch->exit_block.direction]);
    send_to_char(buf, ch);
    return;
  }
  
  if (!IS_NPC(ch) && ch->pcdata->next_recall > current_time) {
       sprintf(buf, "You need to be in the room for %ld more seconds before you can recall.\n\r", (ch->pcdata->next_recall - current_time) < 0 ? 0 : (ch->pcdata->next_recall -  current_time));
       send_to_char(buf, ch);
       return;	
  }    
  
  act( "$n recalls!", ch, 0, 0, TO_ROOM );
  
  if (!IS_NPC(ch)) {
    if ((location = get_room_index(clan_table[ch->clan].room[0] )) == NULL) {
	 send_to_char("Your guild haven't set up a recall yet.\n\r", ch);
	 return;
    }
  }  
  // ######### PET RECALL #######
  else if (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)) {
    if ( ( location = get_room_index( ch->master->in_room->vnum ) ) == NULL ) {
	 send_to_char( "You are completely lost.\n\r", ch );
	 send_to_char( "Your pet is hosed.\n\r", ch->master );
	 return;
    }
  }
  
  if ( ch->in_room == location ) {
    send_to_char("You are already at your guilds recall!\n\r", ch);
    return;
  }
  
  if ( ( victim = ch->fighting ) != NULL ) {
    int lose,skill;
    
    skill = get_skill(ch,gsn_recall);
    
    if ( number_percent() < 80 * skill / 100 ) {
	 check_improve(ch,gsn_recall,FALSE,6);
	 WAIT_STATE( ch, 4 );
	 sprintf( buf, "You failed!\n\r");
	 send_to_char( buf, ch );
	 return;
    }
    
    lose = (ch->desc != NULL) ? 25 : 50;
    gain_exp( ch, 0 - lose );
    check_improve(ch,gsn_recall,TRUE,4);
    stop_fighting( ch, TRUE );
  }

  ch->endurance /= 2;
  act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
  char_from_room( ch );
  char_to_room( ch, location );
  act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
  do_function(ch, &do_look, "auto" );
  
  if (ch->pet != NULL && ch->pet->in_room == ch->in_room) {
    char_from_room(ch->pet);
    char_to_room(ch->pet,location);
  }
  else if (ch->mount != NULL && ch->mount->in_room == ch->in_room) {
    char_from_room(ch->mount);
    char_to_room(ch->mount,location);
    do_mount(ch, ch->mount->name);
  }
  
  return;
}


bool is_guild_guard(CHAR_DATA *guard, CHAR_DATA *ch)
{
  if (guard->guild_guard && guard->guild_guard == ch->clan && is_leader(ch))
    return TRUE;
  
  return FALSE;    
}

bool is_in_guild_group(CHAR_DATA *guard, CHAR_DATA *ch)
{
  CHAR_DATA *vch=NULL;

  // Not a guild guard?
  if (!guard->guild_guard)
    return FALSE;
  
  for (vch = ch->in_room->people; vch; vch = vch->next_in_room ) {
    if (is_same_group(ch, vch) && vch->clan == guard->guild_guard)
	 return TRUE;
  }
  
  return FALSE;
}

void do_guildguard(CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *guard=NULL;
  char arg1[MSL];
  char buf[MSL];
  int i=0;
  long pos = NO_FLAG;
  int num=0;
  
  argument = one_argument(argument, arg1);

  // Only PCs
  if (IS_NPC(ch))
    return;

  // Can use guildguards?
  if (!is_leader(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  // Arg
  if (IS_NULLSTR(arg1) || IS_NULLSTR(argument)) {
    send_to_char("Syntax: guildguard <name> <guildflag>\n\r", ch);
    send_to_char("Syntax: guildguard <name> status\n\r", ch);
    send_to_char("Syntax: guildguard <name> standdown\n\r", ch);
    return;
  }
  
  // Find mob
  if ((guard = get_char_room( ch, arg1)) == NULL) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }
  
  // Only MOB gguards
  if (!IS_NPC(guard)) {
    send_to_char( "Only MOBs can be guild guards.\n\r", ch);
    return;
  }
  
  // Is guild guard?
  if (!is_guild_guard(guard, ch)) {
    sprintf(buf, "%s isn't one of your guild guards.\n\r", guard->short_descr);
    send_to_char(buf, ch);
    return;
  }
  
  if (!str_cmp(argument, "standdown")) {
    guard->guild_guard = 0;
    act("You relieve $N from $S guild guard duty!", ch, NULL, guard, TO_CHAR);
    act("$n relieve $N from $S guild guard duty!", ch, NULL, guard, TO_NOTVICT);
    act("$N nod and trail off towards the barracks.", ch, NULL, guard, TO_CHAR);
    act("$N nod and trail off towards the barracks.", ch, NULL, guard, TO_ROOM);

    if (IS_SET(guard->act, ACT_SENTINEL))
	 REMOVE_BIT(guard->act, ACT_SENTINEL);
   
    //stop_follower(guard);
    //extract_char(guard, TRUE, FALSE);
    return;
  }
  else if (!str_cmp(argument, "status")) {
    sprintf(buf, "%s is set up with the following guild guard flags:\n\r", guard->short_descr);
    send_to_char(buf, ch);
    sprintf(buf, "%s\n\r", flag_string( guild_guard_flags, guard->guild_guard_flags ));
    send_to_char(buf, ch);
    return;
  }
  else if (!str_cmp(argument, "?")) {
    send_to_char("Available guild guard flags:\n\r", ch);
    show_flag_cmds(ch, guild_guard_flags);
  }
  else {
    for (i=0; guild_guard_flags[i].name != NULL; i++) {
	 if (!str_prefix(guild_guard_flags[i].name, argument)) {
	   pos = guild_guard_flags[i].bit;
	   num = i;
	 }
    }

    if (pos == NO_FLAG) {
	 send_to_char("That guild guard flag doesn't exist!\n\r", ch);
	 send_to_char("Use 'guildguard <name> ?' to see available flags.\n\r", ch);
	 return;
    }
    
    if (!IS_SET(guard->guild_guard_flags, pos)) {
	 SET_BIT(guard->guild_guard_flags, pos);
	 sprintf(buf, "Guild guard flag '%s' set on %s.\r\n", guild_guard_flags[num].name, guard->short_descr);
	 send_to_char(buf, ch);
    }
    else {
	 REMOVE_BIT(guard->guild_guard_flags, pos);
	 sprintf(buf, "Guild guard flag '%s' removed from %s.\r\n", guild_guard_flags[num].name, guard->short_descr);
	 send_to_char(buf, ch);
    }
  }
  
  return;	       
}

void do_sgrecall( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  ROOM_INDEX_DATA *location=NULL;
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can recall.\n\r",ch);
    return;
  }

  if (!IS_SET(ch->act, ACT_PET) && !is_sguild(ch)) {
    send_to_char("You aren't in an sguild.\n\r",ch);
    return;
  }
  
  if (ch->fighting) {
    send_to_char("You are still fighting!\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)) {
     send_to_char("The gods has forsaken you!\n\r", ch);
     return;	
  }  
  
  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\n\r", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* Is blocking an exit? */
  if (IS_BLOCKING(ch)) {
    sprintf(buf, "You are still trying to block the %s entrance.\n\r", dir_name[ch->exit_block.direction]);
    send_to_char(buf, ch);
    return;
  }
  
  if (!IS_NPC(ch) && ch->pcdata->next_recall > current_time) {
       sprintf(buf, "You need to be in the room for %ld more seconds before you can recall.\n\r", (ch->pcdata->next_recall - current_time) < 0 ? 0 : (ch->pcdata->next_recall -  current_time));
       send_to_char(buf, ch);
       return;	
  }    
  
  act( "$n recalls!", ch, 0, 0, TO_ROOM );
  
  if (!IS_NPC(ch)) {
    if ((location = get_room_index(sguild_table[ch->sguild].room[0] )) == NULL) {
	 send_to_char("Your guild haven't set up a recall yet.\n\r", ch);
	 return;
    }
  }  
  // ######### PET RECALL #######
  else if (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)) {
    if ( ( location = get_room_index( ch->master->in_room->vnum ) ) == NULL ) {
	 send_to_char( "You are completely lost.\n\r", ch );
	 send_to_char( "Your pet is hosed.\n\r", ch->master );
	 return;
    }
  }
  
  if ( ch->in_room == location ) {
    send_to_char("You are already at your guilds recall!\n\r", ch);
    return;
  }
  
  if ( ( victim = ch->fighting ) != NULL ) {
    int lose,skill;
    
    skill = get_skill(ch,gsn_recall);
    
    if ( number_percent() < 80 * skill / 100 ) {
	 check_improve(ch,gsn_recall,FALSE,6);
	 WAIT_STATE( ch, 4 );
	 sprintf( buf, "You failed!\n\r");
	 send_to_char( buf, ch );
	 return;
    }
    
    lose = (ch->desc != NULL) ? 25 : 50;
    gain_exp( ch, 0 - lose );
    check_improve(ch,gsn_recall,TRUE,4);
    stop_fighting( ch, TRUE );
  }

  ch->endurance /= 2;
  act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
  char_from_room( ch );
  char_to_room( ch, location );
  act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
  do_function(ch, &do_look, "auto" );
  
  if (ch->pet != NULL && ch->pet->in_room == ch->in_room) {
    char_from_room(ch->pet);
    char_to_room(ch->pet,location);
  }
  else if (ch->mount != NULL && ch->mount->in_room == ch->in_room) {
    char_from_room(ch->mount);
    char_to_room(ch->mount,location);
    do_mount(ch, ch->mount->name);
  }
  
  return;
}

void do_ssgrecall( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  ROOM_INDEX_DATA *location=NULL;
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can recall.\n\r",ch);
    return;
  }

  if (!IS_SET(ch->act, ACT_PET) && !is_ssguild(ch)) {
    send_to_char("You aren't in an ssguild.\n\r",ch);
    return;
  }
  
  if (ch->fighting) {
    send_to_char("You are still fighting!\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)) {
     send_to_char("The gods has forsaken you!\n\r", ch);
     return;	
  }  
  
  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\n\r", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* Is blocking an exit? */
  if (IS_BLOCKING(ch)) {
    sprintf(buf, "You are still trying to block the %s entrance.\n\r", dir_name[ch->exit_block.direction]);
    send_to_char(buf, ch);
    return;
  }
  
  if (!IS_NPC(ch) && ch->pcdata->next_recall > current_time) {
       sprintf(buf, "You need to be in the room for %ld more seconds before you can recall.\n\r", (ch->pcdata->next_recall - current_time) < 0 ? 0 : (ch->pcdata->next_recall -  current_time));
       send_to_char(buf, ch);
       return;	
  }    
  
  act( "$n recalls!", ch, 0, 0, TO_ROOM );
  
  if (!IS_NPC(ch)) {
    if ((location = get_room_index(ssguild_table[ch->ssguild].room[0] )) == NULL) {
	 send_to_char("Your guild haven't set up a recall yet.\n\r", ch);
	 return;
    }
  }  
  // ######### PET RECALL #######
  else if (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)) {
    if ( ( location = get_room_index( ch->master->in_room->vnum ) ) == NULL ) {
	 send_to_char( "You are completely lost.\n\r", ch );
	 send_to_char( "Your pet is hosed.\n\r", ch->master );
	 return;
    }
  }
  
  if ( ch->in_room == location ) {
    send_to_char("You are already at your guilds recall!\n\r", ch);
    return;
  }
  
  if ( ( victim = ch->fighting ) != NULL ) {
    int lose,skill;
    
    skill = get_skill(ch,gsn_recall);
    
    if ( number_percent() < 80 * skill / 100 ) {
	 check_improve(ch,gsn_recall,FALSE,6);
	 WAIT_STATE( ch, 4 );
	 sprintf( buf, "You failed!\n\r");
	 send_to_char( buf, ch );
	 return;
    }
    
    lose = (ch->desc != NULL) ? 25 : 50;
    gain_exp( ch, 0 - lose );
    check_improve(ch,gsn_recall,TRUE,4);
    stop_fighting( ch, TRUE );
  }

  ch->endurance /= 2;
  act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
  char_from_room( ch );
  char_to_room( ch, location );
  act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
  do_function(ch, &do_look, "auto" );
  
  if (ch->pet != NULL && ch->pet->in_room == ch->in_room) {
    char_from_room(ch->pet);
    char_to_room(ch->pet,location);
  }
  else if (ch->mount != NULL && ch->mount->in_room == ch->in_room) {
    char_from_room(ch->mount);
    char_to_room(ch->mount,location);
    do_mount(ch, ch->mount->name);
  }
  
  return;
}

