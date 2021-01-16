/***************************************************************************
 * This file include code special dedicated to guild handeling             *
 * TSW is copyright -2003 Swordfish and Zandor                             *
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

bool is_oguild(CHAR_DATA * ch)
{
  return ch->oguild;
}

bool is_same_oguild(CHAR_DATA * ch, CHAR_DATA * victim)
{
  if (IS_SET(clan_table[ch->oguild].flags,GUILD_INDEPENDENT))
    return FALSE;
  else
    return (ch->oguild == victim->oguild);
}

char *player_oguild_rank(CHAR_DATA * ch)
{
  if (ch->oguild == 0)
    return '\0';
  return clan_table[ch->oguild].rank[ch->oguild_rank].rankname;
}

char *player_oguild(CHAR_DATA * ch)
{
  if (ch->oguild == 0)
    return '\0';
  return clan_table[ch->oguild].name;
}

void reset_oguild_status(CHAR_DATA *ch)
{
  ch->oguild                   = 0;
  ch->oguild_rank              = 0;
  ch->pcdata->oguild_offer     = 0;
  ch->pcdata->oguild_requestor = 0;
  ch->pcdata->oguilded_by      = 0;
  
  free_string(ch->oguild_title);
  ch->oguild_title = NULL;

  ch->oguild_invis = FALSE;
  ch->oguild_mute  = FALSE;
  
  if (IS_SET(ch->act, PLR_MORTAL_LEADER))
    REMOVE_BIT(ch->act, PLR_MORTAL_LEADER);
}

void set_oguild_title( CHAR_DATA *ch, char *oguild_title )
{
  // validate
  if (IS_NULLSTR(oguild_title))
    return;
  
  free_string( ch->oguild_title );
  ch->oguild_title = str_dup(oguild_title);
  return;
}

void do_oguild_promote(CHAR_DATA * ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  int cnt;
//  int sn = 0;
  
  argument = one_argument(argument, arg1);
  
  if (!can_promote(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (arg1[0] == '\0' || argument[0] == '\0') {

    send_to_char("Syntax: opromote <who> <rank #>\n\r", ch);
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
  
  if (!is_oguild(victim)) {
    send_to_char("They are not a member of any guilds!\n\r", ch);
    return;
  }

  if (ch->clan != victim->oguild && !IS_IMMORTAL(ch)) {
//  if (!is_same_oguild(ch, victim) && !IS_IMMORTAL(ch)) {
    send_to_char("They are a member of a oguild different than your guild!\n\r", ch);
    return;
  }
  
  if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim)) {
    send_to_char("You can't opromote Immortals.\n\r", ch);
    return;
  }

  if (!str_prefix(argument, "leader") && ch != victim) {
     send_to_char("You can't make a oguilded member a leader of the guild.\n\r", ch);
     return;	
  }
    
  cnt = atoi(argument) - 1;
  if (cnt < 0 ||
	 cnt > MAX_RANK -1 ||
	 clan_table[victim->oguild].rank[cnt].rankname == NULL) {
    send_to_char("That rank does not exist!\n\r", ch);
    return;
  }
  
  if (cnt > victim->oguild_rank && ((ch == victim) & (!IS_IMMORTAL(ch)))) {
    send_to_char("Heh. I dont think so...\n\r", ch);
    return;
  }
  
  if (cnt <= 2) {
    send_to_char("Oguilded members can't be promoted past rank 3.\n\r", ch);
    return;	
  }
    
  if (cnt < victim->oguild_rank) {
//    int i;
    
    sprintf(buf, "You have been promoted to %s!\n\r",
		  clan_table[victim->oguild].rank[cnt].rankname);
    send_to_char(buf, victim);
    
    sprintf(buf, "%s has been promoted to %s!\n\r",
		  PERS_NAME(victim,ch), clan_table[victim->oguild].rank[cnt].rankname);
    send_to_char(buf, ch);
    
    set_oguild_title(victim, clan_table[victim->oguild].rank[cnt].rankname);
  }   
  else if (cnt > victim->oguild_rank) {
    if (IS_SET(victim->act, PLR_MORTAL_LEADER))
	 REMOVE_BIT(victim->act, PLR_MORTAL_LEADER);
    
    sprintf(buf, "You have been demoted to %s!\n\r",
		  clan_table[victim->oguild].rank[cnt].rankname);

    set_oguild_title(victim, clan_table[victim->oguild].rank[cnt].rankname);
    
    send_to_char(buf, victim);

    sprintf(buf, "%s has been demoted to %s!\n\r",
		  PERS_NAME(victim, ch), clan_table[victim->oguild].rank[cnt].rankname);    
    send_to_char(buf, ch);
    
    // If ginvis, strip it and tell.
    if (victim->oguild_invis) {
  	ch->oguild_invis = FALSE;	
  	send_to_char("You are now visible to your oguild.\n\r", victim);    	
    }
  }				/* else no change */

  victim->oguild_rank = cnt;

  // Save info
  save_char_obj(victim, FALSE);

  return;
}


/*
 * Immortal guild command for joining/banis
 */
void do_oguild(CHAR_DATA * ch, char *argument)
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
    send_to_char("Syntax: oguild <char> <guild name>\n\r", ch);
    return;
  }

/*  if ((victim = get_char_world(ch, arg1)) == NULL) { */
  if ((victim = get_realname_char_world(ch, arg1)) == NULL) { 
    send_to_char("They aren't playing.\n\r", ch);
    return;
  }

  /** thanks to Zanthras for the bug fix here...*/
  if (is_oguild(victim) && !is_same_clan(ch, victim) &&
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

    reset_oguild_status(victim);

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
  
  victim->oguild = clan;
  victim->oguild_rank = MAX_RANK-1;		/* lowest, default */
} /* end: do_guild */

/*
 * Set a members guild title if leader
 */
void do_oguildtitle( CHAR_DATA *ch, char *argument )
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
    send_to_char("Syntax: ogtitle <char> <title>\n\r", ch);
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

  if (ch->clan != victim->oguild && ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("They are a member of a oguild other than your own guild.\n\r", ch);
    return;
  }
  
  if (ch != victim && IS_IMMORTAL(victim)) {
    send_to_char("You can't set oguild titles on immortals.\n\r", ch);
    return;
  }

  set_oguild_title(victim, arg2);

  sprintf(buf, "You set %s's oguild title to %s.\n\r", PERS_NAME(victim, ch), victim->oguild_title);
  send_to_char(buf, ch);
  
  sprintf(buf, "%s has set your oguild title to %s.\n\r", PERS_NAME(ch, victim), victim->oguild_title);
  send_to_char(buf, victim);
  
  // Save info
  save_char_obj(victim, FALSE);
  
  return;
}

int cnt_oguild_guards(CHAR_DATA *ch)
{
  CHAR_DATA *wch=NULL;
  int cnt=0;
  
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {
    if (!IS_NPC(wch))
	 continue;
    if (wch->guild_guard == ch->oguild)
	 cnt++;
  }
  
  return cnt;
}

int compare_cross_ranks(const void *v1, const void *v2)
{
    return (*(CHAR_DATA**)v1)->trank - (*(CHAR_DATA**)v2)->trank;
}

/*
 * Show current playing and wizible members of a Guild
 */
void do_oguildlist( CHAR_DATA *ch, char *argument )
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

  if (!is_oguild(ch)) {
    send_to_char("You aren't in a oguild.\n\r",ch);
    return;
  }

  if (ch->oguild_rank == 9 && !IS_IMMORTAL(ch))
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
    
    if (wch->oguild != ch->oguild && wch->clan != ch->oguild)
	 continue;

    // Only guild leaders can see other ginvis members
    if ( (wch->oguild_invis == TRUE) && (wch != ch) && !can_guild(ch) )
    	continue;

    // Only guild leaders can see other ginvis members
    if ( (wch->ginvis == TRUE) && (wch != ch) && !can_guild(ch) && (ch->oguild == wch->clan) )
    	continue;

    // If immortal and ginvis, then none can see you
    if (wch->oguild_invis == TRUE && IS_IMMORTAL(wch) && (wch != ch))
	 continue;

    wch->trank = wch->clan == ch->oguild ? wch->rank : wch->oguild_rank;

    pcs[cnt++] = wch;    
  }

  /* Sort PC array based on name first */
  qsort (pcs, cnt, sizeof(wch), compare_char_names);

  /* Then we sort based on oguild rank */
  qsort (pcs, cnt, sizeof(wch), compare_cross_ranks);

  for (i=0; i < cnt; i++) {
    if (pcs[i]->clan == ch->oguild) {
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
    else {
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
  }
  
  sprintf(buf, "\nMembers found: {y%d{x.  Guild guards: {y%d{x.\n\r", cnt,  cnt_oguild_guards(ch));
  add_buf(output,buf);
  
  page_to_char(buf_string(output),ch);
  free_buf(output);
}

/*
 * Offer a PC to join a guild
 */
void do_ojoin( CHAR_DATA *ch, char *argument )
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
    send_to_char("Syntax:\n\r  ojoin <char> <guild name>\n\r"
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
    if (((victim = get_realname_char_world(ch, arg1)) == NULL) &&
	   victim->in_room != ch->in_room) {
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
  
  /* Get rid of people that are trying to guild themselves */
  if (ch == victim && !IS_IMMORTAL(ch)) {
    send_to_char("You can't join your self to a oguild!\n\r", ch);
    return;
  }

  if IS_NPC(victim) {
    send_to_char("Mobiles can't be oguilded!\n\r",ch);
    return;
  }

  if (!is_clan(victim)) {
    send_to_char("They need to be in a guild before they can be oguilded.\n\r", ch);
    return;
  }
  
  if (is_oguild(victim)) {
    send_to_char("They are a member of a oguild already.\n\r", ch);
    return;
  }


  if ((clan = clan_lookup(arg2)) == 0) {
    send_to_char("No such guild exists.\n\r", ch);
    return;
  }
  
  if (ch->clan != clan) {
     send_to_char("You can't ojoin to another guild than the one you are in.\n\r", ch);
     return;	
  }

  if (victim->clan == clan) {
    send_to_char("They already are a member of this guild!\n\r", ch);
    return;
  }

  /* Offer */
  sprintf(buf, "Your offer to %s has been made.\n\r", PERS_NAME(victim, ch));
  send_to_char(buf, ch);
  
  victim->pcdata->oguild_offer     = clan;
  victim->pcdata->oguild_requestor = ch->id;

  sprintf(buf,"%s has offered you a oguilded position in the guild: [%s].\n\r",
		PERS(ch, victim), clan_table[clan].name);
  send_to_char(buf, victim);
  send_to_char("Type {yoaccept{x to join.\n\rType {yorefuse{x to decline offer.\n\r", victim);
  
  return;
}

/*
 * Option to accept a given offer to join a Guild
 */
void do_oaccept( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *vch;
  char buf[MAX_INPUT_LENGTH];
  
  if (IS_NPC(ch))
    return;
  
  if (ch->pcdata->oguild_offer == 0) 
    return;
  
  if (argument[0] != '\0') {
    sprintf(buf, "Type oaccept to join the guild: [%s].\n\r",
		  clan_table[ch->pcdata->oguild_offer].name);
    send_to_char(buf, ch);
    return;
  }

  ch->oguild               = ch->pcdata->oguild_offer;
  ch->oguild_rank          = MAX_RANK-1;
  ch->pcdata->oguild_offer = 0;
  ch->pcdata->oguilded_by  = ch->pcdata->oguild_requestor;

  sprintf(buf, "You have joined the oguild: [%s].\n\r", clan_table[ch->oguild].name);
  send_to_char(buf, ch);
  
  if ((vch = get_charId_world(ch, ch->pcdata->oguilded_by)) != NULL) {
    sprintf(buf, "You have notifed %s that you have joined the oguild: [%s].\n\r",
		  PERS(vch, ch), clan_table[ch->oguild].name);
    send_to_char(buf, ch);
    
    sprintf(buf, "%s has accepted your offer and join the oguild: [%s].\n\r",
		  PERS_NAME(ch, vch), clan_table[ch->oguild].name);
    send_to_char(buf, vch);
  }
  
  // Save info
  save_char_obj( ch, FALSE);
}

/*
 * Option to refuse to join a offered Guild
 */
void do_orefuse( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *vch;
  char buf[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  if (ch->pcdata->oguild_offer == 0) 
    return;

  if (argument[0] != '\0') {
    send_to_char("Just type orefuse to decline the offer to join the oguild.\n\r", ch);
    return;
  }
  
  sprintf(buf, "You refused to join the oguild: [%s].\n\r",
		clan_table[ch->pcdata->oguild_offer].name);
  send_to_char(buf, ch);


  if ((vch = get_charId_world(ch, ch->pcdata->oguild_requestor)) != NULL) {
    sprintf(buf, "You have notifed %s that you will not join the oguild: [%s].\n\r",
		  PERS(vch, ch), clan_table[ch->pcdata->oguild_offer].name);
    send_to_char(buf, ch);

    sprintf(buf, "%s has refused your offer to join the oguild: [%s].\n\r",
		  PERS_NAME(ch, vch),  clan_table[ch->pcdata->oguild_offer].name);
    send_to_char(buf, vch);

    ch->pcdata->oguild_offer     = 0;      
    ch->pcdata->oguild_requestor = 0;
  }

  // Save info
  save_char_obj( ch, FALSE);
  return;
}

void do_oguild_banish( CHAR_DATA *ch, char *argument )
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
    send_to_char("Syntax: obanish <char>. \n\r",ch);
    return;
  }

  if ( (vch = get_char_world( ch, arg1)) == NULL) {
  /* if ( (vch = get_introname_char_world( ch, arg1)) == NULL) { */
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }

  /* Check mobiles */
  if IS_NPC(vch) {
    send_to_char("Mobiles can't be oguilded or obanished!\n\r",ch);
    return;
  }
  
  /* The self check */
  if ((ch == vch) && !IS_IMMORTAL(ch)) {
    send_to_char("You can't obanish your self. Please see one of the immortals.\n\r", ch);
    return;
  }

  if (ch->clan != vch->oguild && ((ch->level < SUPREME) & (ch->trust < SUPREME))) {
    send_to_char("You can't obanish that person!.\n\r", ch);
    return;
  }
  
  /* Banish */
  reset_oguild_status(vch);
  
  sprintf(buf,"%s has banished you from your oguild.\n\r",PERS_NAME(ch, vch));
  send_to_char(buf,vch);
  
  sprintf(buf, "You have banished %s from the oguild.\n\r",PERS_NAME(vch, ch));
  send_to_char(buf,ch);
  
  // Save info
  save_char_obj(vch, FALSE);
  
  /* Done */
  return;
}

void do_oguildinvis( CHAR_DATA *ch, char *argument )
{

  if (!can_guild(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (ch->oguild_invis != TRUE) {
  	ch->oguild_invis = TRUE;
  	send_to_char("You are now oguild invis.\n\r", ch);
  	return;
  }
  else {
  	ch->oguild_invis = FALSE;	
  	send_to_char("You are now visible to your oguild.\n\r", ch);
  	return;
  }
  
  return;
}

void do_oguildmute( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim=NULL;
  
  if (!can_guild(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: oguildmute <oguild member>\n\r", ch);
    return;
  }
  
  if ((victim = get_realname_char_world( ch, argument)) == NULL ) {  
    send_to_char( "They aren't playing.\n\r", ch );
    return;
  }

  if (!is_oguild(victim)) {
    send_to_char("They are not a member of your guild!\n\r", ch);
    return;
  }

  if (ch->clan != victim->oguild && !IS_IMMORTAL(ch)) {
    send_to_char("They are a member of a guild different than yours!\n\r", ch);
    return;
  }
  
  if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim)) {
    send_to_char("You can't mute Immortals.\n\r", ch);
    return;
  }
  
  if (victim->oguild_mute != TRUE) {
    victim->oguild_mute = TRUE;
    send_to_char("You have revoked their oguild channel priviliges.\n\r", ch);
    act("$n has revoked your oguild channel priviliges.", ch, NULL, victim, TO_VICT);
    return;
  }
  else {
    victim->oguild_mute = FALSE;
    send_to_char("You have restored their oguild channel priviliges.\n\r", ch);
    act("$n has restored your oguild channel priviliges.", ch, NULL, victim, TO_VICT);
    return;
  }
  
  return;
}

void do_ogrecall( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  ROOM_INDEX_DATA *location=NULL;
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can recall.\n\r",ch);
    return;
  }

  if (!IS_SET(ch->act, ACT_PET) && !is_oguild(ch)) {
    send_to_char("You aren't in a oguild.\n\r",ch);
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
    if ((location = get_room_index(clan_table[ch->oguild].room[0] )) == NULL) {
	 send_to_char("Your oguild haven't set up a recall yet.\n\r", ch);
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
    send_to_char("You are already at your oguilds recall!\n\r", ch);
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
  
  if (ch->pet != NULL) {
    char_from_room(ch->pet);
    char_to_room(ch->pet,location);
  }
  else if (ch->mount != NULL) {
    char_from_room(ch->mount);
    char_to_room(ch->mount,location);
    do_mount(ch, ch->mount->name);
  }
  
  return;
}
