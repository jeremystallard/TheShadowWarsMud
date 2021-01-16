/***************************************************************************
*       ROM 2.4 is copyright 1993-1995 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@pacinfo.com)                              *
*           Gabrielle Taylor (gtaylor@pacinfo.com)                         *
*           Brian Moore (rom@rom.efn.org)                                  *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

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
 **************************************************************************/
 
/***************************************************************************
 * This file include code special dedicated to minion handeling            *
 * TSW is copyright -2003 Swordfish and Zandor                             *
 **************************************************************************/
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"

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

int compare_mranks(const void *v1, const void *v2)
{
  return (*(CHAR_DATA**)v1)->mrank - (*(CHAR_DATA**)v2)->mrank;
}

void do_minion( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  
  argument = one_argument( argument, arg1 );

  if ( arg1[0] == '\0' ) {
    send_to_char("Syntax:\n\r  minion <char> \n\r\n\r",ch);
    return;
  }

  if ( ( victim = get_realname_char_world( ch, arg1) ) == NULL ) {
     send_to_char( "They aren't playing.\n\r", ch );
     return;
  }

/* OLD INTROCODE
  if (IS_IMMORTAL(ch)) {
    if ( ( victim = get_char_room( ch, arg1) ) == NULL ) {
      send_to_char( "They aren't here.\n\r", ch );
      return;
     }
  }
  else {
    if ( ( victim = get_introname_char_room( ch, arg1) ) == NULL ) {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }
  }
*/
  
  if IS_NPC(victim) {
    send_to_char("Mobiles can't be minions!\n\r",ch);
    return;
  }

  if (ch != victim) {

    /* Is char in minion him self ? */
    if (ch->minion == 0) {
	 send_to_char("You are not in a minion!\n\r", ch);
	 return;
    }

    sprintf(buf, "You offer to %s has been made.\n\r", PERS_NAME(victim, ch));
    send_to_char(buf, ch);
        
    victim->pcdata->minion_offer     = ch->minion;
    victim->pcdata->minion_requestor = ch->id;

    sprintf(buf, "%s has asked you to be a minion.\n\r"
		  "Type maccept to be a minion of %s.\n\r"
		  "Type mrefuse to clear the request.\n\r", PERS(ch, victim) , PERS_NAME(ch, victim));
    send_to_char(buf, victim);
    
  }  
  else  {

    if (ch->minion != 0) {
	 send_to_char("You are already in a minion!\n\r", ch);
	 return;
    }

    send_to_char("You have minioned yourself!\n\r", ch);

    ch->minion                   = ch->id;
    ch->pcdata->minion_requestor = ch->id;              
    ch->pcdata->minioned_by      = ch->id;
    ch->mrank                    = 0;

    free_string(ch->mname);
    sprintf(buf, "%s's minion", ch->name);
    ch->mname                    = str_dup(buf);
  }
  
  return;
}

void do_maccept( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *vch;
  char buf[MAX_INPUT_LENGTH];
  
  if (IS_NPC(ch))
    return;
  
  if (ch->pcdata->minion_offer == 0) 
    return;

  if ((vch = get_charId_world(ch, ch->pcdata->minion_requestor)) == NULL) {
    send_to_char("You and your requestor needs to both be present to fulfill this agreement.\n\r", ch);
    return;
  }
  
  if (argument[0] != '\0') {
    sprintf(buf, "Type maccept to become a minion of %s.\n\r", PERS(vch, ch));
    send_to_char(buf, ch);
    return;
  }

  ch->minion               = ch->pcdata->minion_offer;
  ch->mrank                = MAX_RANK-1;
  ch->pcdata->minion_offer = 0;
  ch->pcdata->minioned_by  = ch->pcdata->minion_requestor;

  free_string(ch->mname);  
  sprintf(buf, "%s", vch->mname);
  ch->mname               = str_dup(buf);

  sprintf(buf, "You have become a minion of %s.\n\r", PERS(vch, ch));
  send_to_char(buf, ch );
  
  sprintf(buf, "You have notified %s that you have became a minion.\n\r", PERS(vch, ch));
  send_to_char(buf, ch);
  
  sprintf(buf, "%s has accepted your offer and joined your minion.\n\r", PERS(ch, vch));
  send_to_char(buf, vch);  
}
  
  
void do_mrefuse( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *vch;
  char buf[MAX_INPUT_LENGTH];
  
  if (IS_NPC(ch))
    return;
  
  if (ch->pcdata->minion_offer == 0) 
    return;
  
  if (argument[0] != '\0') {
    if ((vch = get_charId_world(ch, ch->pcdata->minion_requestor)) != NULL) {
	 sprintf(buf, "Just type mrefuse to notify %s you don't want to be a minion.\n\r", PERS(vch, ch));
	 send_to_char(buf, ch);
    }
    else {
	 send_to_char("Just type mrefuse to notify your requestor you don't want to be a minion.\n\r", ch);
    }
    return;
  }
  
  if ((vch = get_charId_world(ch, ch->pcdata->minion_requestor)) != NULL) {
    sprintf(buf, "You refused to be a minion of %s.\n\r", PERS(vch, ch));
    send_to_char(buf, ch);
  }
  else {
    send_to_char("You refused to be a minion of your requestor.\n\r", ch);
  }

  if (vch != NULL) {
    sprintf(buf, "You have notified %s that you will not join the minion.\n\r", PERS(vch, ch));
    send_to_char(buf, ch);

    sprintf(buf, "%s has refused your offer to join the minion.\n\r", PERS(ch, vch));
    send_to_char(buf, vch);
  }
  
  
  ch->pcdata->minion_offer     = 0;      
  ch->pcdata->minion_requestor = 0;

  return;
}

void set_mtitle( CHAR_DATA *ch, char *mtitle )
{
  // validate
  if (IS_NULLSTR(mtitle))
    return;
  
  free_string( ch->mtitle );
  ch->mtitle = str_dup(mtitle);
  return;
}
bool can_mpromote(CHAR_DATA * ch)
{
  /* ok if ch is a IMMORTAL */
  if (IS_IMMORTAL(ch))
    return TRUE;

  if (ch->mrank < 2)
    return TRUE;
  
  return FALSE;
}

void reset_mstatus(CHAR_DATA *ch)
{
  if (IS_NPC(ch))
     return;

  ch->pcdata->minion_requestor = 0;
  ch->minion = 0;

  free_string(ch->mname);
  ch->mname = NULL;  

  free_string(ch->mtitle);
  ch->mtitle = NULL;
}

void do_mbanish( CHAR_DATA *ch, char *argument )
{
  
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];    
  CHAR_DATA *vch;
  CHAR_DATA *rch;
  
  argument = one_argument( argument, arg1 );
  
  if ( arg1[0] == '\0') {
    send_to_char("Syntax: mbanish <char>\n\r", ch);
    return;
  }

  if ( ( vch = get_char_world( ch, arg1)) == NULL ) {
     send_to_char( "They aren't here.\n\r", ch );
     return;  	
  }

/*  OLD INTRO CODE
  if (IS_IMMORTAL(ch)) {
    if ( ( vch = get_char_world( ch, arg1)) == NULL ) {
      send_to_char( "They aren't here.\n\r", ch );
      return;  	
    }
  }
  else {
    if ( ( vch = get_introname_char_world( ch, arg1)) == NULL ) {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }
  }
*/

  if IS_NPC(vch) {
    send_to_char("Mobiles can't be minioned or banished!\n\r",ch);
    return;
  }    
  
  /* Get rid of people that are trying to banish themselves */
  if (!IS_IMMORTAL(ch) && ch->minion != vch->minion) {
    send_to_char("That person isn't in your minion!\n\r",ch);
    return;
  }

  if (!can_mpromote(ch) && ch != vch) {
    send_to_char("You can't banish from the minion with your rank.\n\r", ch);
    return;
  }

  if ((rch = get_charId_world(ch, vch->pcdata->minion_requestor)) != NULL) {  
    sprintf(buf,"%s banished %s from the minonship of %s.", ch->name, vch->name, rch->name);
    log_string(buf);
  }
  else {
    sprintf(buf, "%s banished %s from his minionship.\n\r", ch->name, vch->name);
    log_string(buf);
  }
  
  sprintf(buf, "You are no longer a member of %s!!\n\r", vch->mname);
  send_to_char(buf, vch);


  sprintf(buf,"%s has banished you from your master\n\r",PERS(ch, vch));
  send_to_char(buf,vch);

  sprintf(buf, "You have banished %s from the minion.\n\r", PERS(vch, ch));
  send_to_char(buf, ch);
    
  reset_mstatus(vch);
 
  return;
}

/*
 * Show current playing and wizible members of a Minion
 */
void do_minionlist( CHAR_DATA *ch, char *argument )
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

  if (ch->minion == 0) {
    send_to_char("You aren't in a minion.\n\r", ch);
    return;
  }

  output = new_buf();

  for (d = descriptor_list; d != NULL; d = d->next) {    
    if (d->connected != CON_PLAYING || !can_see_channel(ch,d->character))
	 continue;
    
    wch = ( d->original != NULL ) ? d->original : d->character;

    if (!can_see_channel(ch,wch))
    	 continue;

    if (wch->minion != ch->minion)
	 continue;

     /* Whoinvis */
     if ((wch->incog_level > 0) && (wch != ch) && !IS_IMMORTAL(ch))
        continue;

    pcs[cnt++] = wch;
  }

  /* Sort PC array based on name first */
  qsort (pcs, cnt, sizeof(wch), compare_char_names);

  /* Then we sort based on rank */
  qsort (pcs, cnt, sizeof(wch), compare_mranks);

  for (i=0; i < cnt; i++) {
    fillers = (16 - colorstrlen(pcs[i]->mname)-1);
    fillers2 = (16 - colorstrlen(PERS_OLD(pcs[i], ch))-1);

    sprintf(buf, "[%s%*s (%d)] %s%*s {x(%s{x)\n\r", pcs[i]->mname, 
		  fillers, "",
		  pcs[i]->mrank+1, 
		  PERS_OLD(pcs[i], ch),
		  fillers2, "",
		  pcs[i]->mtitle ? pcs[i]->mtitle : "Unassigned");
    add_buf(output,buf);
  }
  
  sprintf(buf, "\nMinions found: {y%d{x.\n\r", cnt);
  add_buf(output,buf);

  page_to_char(buf_string(output),ch);
  free_buf(output);
}

/*
 * Set a members minion title if leader
 */
void do_miniontitle( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;

  smash_tilde( argument );
  argument = one_argument(argument, arg1);
  strcpy(arg2, argument);

  if (ch->minion == 0) {
    send_to_char("You aren't in a minion.\n\r", ch);
    return;
  }
  
  if (!can_mpromote(ch)) {
    send_to_char("You can't set a minion title with your rank.\n\r", ch);
    return;
  }
  
  if ( (arg1[0] == '\0') || (arg2[0] == '\0')) {
    send_to_char("Syntax: mtitle <char> <title>\n\r", ch);
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

  if (victim->minion != ch->minion) {
    sprintf(buf, "They are not a member of %s.\n\r", ch->mname);
    send_to_char(buf, ch);
    return;
  }

  /*
  if (IS_IMMORTAL(victim) && !IS_IMMORTAL(ch)) {
     send_to_char("You can't set minion titles on immortals.\n\r", ch);
     return;
  }
  */

  
  set_mtitle(victim, arg2);

  sprintf(buf, "You set %s's minion title to %s.\n\r", PERS_NAME(victim, ch), victim->mtitle);
  send_to_char(buf, ch);
  
  sprintf(buf, "%s has set your minion title to %s.\n\r", PERS_NAME(ch, victim), victim->mtitle);
  send_to_char(buf, victim);
  
  return;
}

void do_mpromote(CHAR_DATA * ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  int cnt;

  argument = one_argument(argument, arg1);

  if (ch->minion == 0) {
    send_to_char("You aren't in a minion.\n\r", ch);
    return;
  }

  if (!can_mpromote(ch)) {
    send_to_char("You can't mpromote with your rank.\n\r", ch);
    return;
  }

  if (arg1[0] == '\0' || argument[0] == '\0') {
    send_to_char("Syntax: promote <who> <rank #>\n\r", ch);
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

  if (victim->minion != ch->minion) {
    sprintf(buf, "They are not a member of %s!\n\r", ch->mname);
    send_to_char(buf, ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim)) {
    send_to_char("You can't promote Immortals.\n\r", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && victim == ch) {
    send_to_char("You can't promote your self. You are the leader.\n\r", ch);
    return;
  }

  cnt = atoi(argument) - 1;

  if (cnt < 0 || cnt > MAX_RANK -1 ) {
    sprintf(buf, "Rank needs to be between 1 and %d.\n\r", MAX_RANK);
    send_to_char(buf, ch);
    return;
  }
  
  if (cnt < victim->mrank) {
    sprintf(buf, "You have been promoted to rank %d in %s.\n\r", cnt+1, victim->mname);
    send_to_char(buf, victim);

    sprintf(buf, "%s has been promoted to rank %d in %s!\n\r", PERS_NAME(victim, ch), cnt+1, victim->mname);
    send_to_char(buf, ch);
  }
  else {
    sprintf(buf, "You have been demoted to rank %d in %s.\n\r", cnt+1, victim->mname);
    send_to_char(buf, victim);
    
    sprintf(buf, "%s has been demoted to rank %d in %s!\n\r", PERS_NAME(victim, ch), cnt+1, victim->mname);
    send_to_char(buf, ch);
  }
  
  victim->mrank = cnt;
  return;
}


typedef struct minion_member_data {
  char player_name[64];
  int  player_level;
  long minion;
  char minion_name[128];
  char minion_title[512];
  time_t last_on_time;
  char last_on_time_str[64];
  int  minion_rank;  
}minion_member_data;

#define MAX_MINION_MEBER_DATA 100

struct minion_member_data gd[MAX_MINION_MEBER_DATA];

void init_minion_member_data()
{
  int i;

  for (i=0; i < MAX_MINION_MEBER_DATA; i++) {
    gd[i].player_name[0]      = '\0';
    gd[i].player_level        = 0;
    gd[i].minion              = 0;
    gd[i].minion_name[0]      = '\0';
    gd[i].minion_title[0]     = '\0';
    gd[i].last_on_time        = 0;
    gd[i].last_on_time_str[0] = '\0';
    gd[i].minion_rank         = 0;
  }
  
  return;
}

int find_free_minion_member_data_slot()
{
  int i;
  
  for (i=0; i < MAX_MINION_MEBER_DATA; i++) {
    if (IS_NULLSTR(gd[i].player_name))
	 return i;
  }
  
  return -1;
}

bool insert_new_minion_member_data(struct minion_member_data new)
{
  int i = find_free_minion_member_data_slot();
  
  if (i < 0 || i > MAX_MINION_MEBER_DATA)
    return FALSE;
  
  strcpy(gd[i].player_name, new.player_name);
  strcpy(gd[i].minion_name, new.minion_name);
  strcpy(gd[i].minion_title, new.minion_title);
  strcpy(gd[i].last_on_time_str, new.last_on_time_str);

  gd[i].player_level = new.player_level;
  gd[i].minion       = new.minion;
  gd[i].last_on_time = new.last_on_time;
  gd[i].minion_rank  = new.minion_rank;

  return TRUE;
}

int load_minion_member_data(CHAR_DATA *ch, char *path)
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
  bool isMinion = FALSE;
  int mCnt=0;
  bool fMatch;
  char *ptr=NULL;
  char *strtime=NULL;

  struct minion_member_data new;
  
  n = scandir(path, &Dir, 0, alphasort);

  if (n < 0)
    return n;

  new.minion          = 0;
  new.player_name[0]  = '\0';
  new.minion_name[0]  = '\0';
  new.minion_title[0] = '\0';
  new.player_level    = 0;
  new.minion_rank     = 0;
  new.last_on_time   = 0;
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
		  ptr = fread_string( fp );
		  if (ptr != NULL) {
		    free_string(ptr);
		  }
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
	   case 'M':
		if (!strcmp(word, "Minion")) {
		  new.minion = fread_number(fp);
		  
		  if (ch->minion != new.minion)
		    find_more = FALSE;		  
		  else
		    isMinion = TRUE;
		  break;
		}
		
		if (!strcmp(word, "Mname")) {
		  ptr = fread_string(fp);
		  if (!IS_NULLSTR(ptr)) {
		    sprintf(new.minion_name, "%s", ptr);
		  }
		  if (ptr != NULL)
		    free_string(ptr);
		  break;
		}
		
		if (!strcmp(word, "Mtitle")) {
		  ptr = fread_string(fp);
		  if (!IS_NULLSTR(ptr)) {
		    sprintf(new.minion_title, "%s", ptr);
		  }
		  if (ptr != NULL)
		    free_string(ptr);
		  break;
		}
		
		if (!strcmp(word, "Mrank")) {		  
		  new.minion_rank = fread_number(fp);
		  break;
		}
		
		break;
	   case 'P':
		if (!strcmp(word, "Plyd")) 
		  find_more = FALSE;
		break;		
	   }
	 }
	 
	 fclose(fp);
	 free(Dir[i]);
	 find_more = TRUE;
	 
	 if (isMinion && ch->minion == new.minion) {
	   mCnt++;
	   
	   /* Is Victim online ? */
	   if ((victim = get_char_world(ch, new.player_name)) != NULL && !IS_NPC(victim)) {
		new.player_level = get_level(victim);		
		sprintf(new.minion_name, "%s", victim->mname);
		sprintf(new.minion_title, "%s", !IS_NULLSTR(victim->mtitle) ? victim->mtitle : "Unassigned");
		sprintf(new.last_on_time_str, "Online");
		new.last_on_time = time(NULL);
		new.minion_rank = victim->mrank;		
	   }
	   else {
		new.last_on_time = sb.st_mtime;
		strtime = ctime(&new.last_on_time);
		strtime[strlen(strtime)-1] = '\0';
		sprintf(new.last_on_time_str, "%s", strtime);
	   }
	   
	   // Insert new minion member
	   insert_new_minion_member_data(new); 
	   
	   // reset used variables
	   isMinion = FALSE;
	   new.minion          = 0;
	   new.player_name[0]  = '\0';
	   new.minion_name[0]  = '\0';
	   new.minion_title[0] = '\0';
	   new.player_level    = 0;
	   new.minion_rank     = 0;
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

void print_minion_member_data_to_char(CHAR_DATA *ch)
{
  BUFFER *pbuf;
  char buffer[MSL];
  int i=0;
  int mCnt=0;

  pbuf = new_buf();
  
  send_to_char("{C                                        MINION MEMBERS                                        {x\n\r",ch);
  send_to_char("{RName           Minion                     Rank Lvl  Last on                   Title           {x\n\r",ch);
  send_to_char("{R============================================================================================={x\n\r",ch);

  for (i=0; i < MAX_MINION_MEBER_DATA; i++) {
    if (!IS_NULLSTR(gd[i].player_name)) {
	 mCnt++;
	 if (IS_IMMORTAL(ch)) {
	   sprintf(buffer, "{B[{c%-12s{B][{c%-24s{B][{c%3d{B][{c%3d{B][{c%24s{B][{c%s{x\n\r", 
			 gd[i].player_name, 
			 gd[i].minion_name,
			 gd[i].minion_rank+1,
			 gd[i].player_level,
			 gd[i].last_on_time_str,
			 !IS_NULLSTR(gd[i].minion_title) ? gd[i].minion_title : "n/a");
	 }
	 else {
	   sprintf(buffer, "{B[{c%-12s{B][{c%-24s{B][{c%3d{B][{c%3s{B][{c%24s{B][{c%s{x\n\r", 
			 gd[i].player_name, 
			 gd[i].minion_name,
			 gd[i].minion_rank+1,
			 "xxx",
			 gd[i].last_on_time_str,
			 !IS_NULLSTR(gd[i].minion_title) ? gd[i].minion_title : "n/a");
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

int compare_minion_member_ranks(const void *v1, const void *v2)
{
  return (*(struct minion_member_data*)v1).minion_rank - (*(struct minion_member_data*)v2).minion_rank;
}

int compare_minion_member_names(const void *v1, const void *v2)
{
  return strcmp((*(struct minion_member_data*)v1).player_name, (*(struct minion_member_data*)v2).player_name);
}

int compare_minion_member_logons(const void *v1, const void *v2)
{
  return (*(struct minion_member_data*)v1).last_on_time - (*(struct minion_member_data*)v2).last_on_time;
}

void do_mmembers( CHAR_DATA *ch, char *argument )
{
  int members=0;

  if (!can_mpromote(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("You are not a leader of your minion!\n\r", ch);
    return;
  }
  
  // Argument
  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: mmembers name\n", ch);
    send_to_char("        mmembers rank\n", ch);
    send_to_char("        mmembers logon\n", ch);
    return;
  }
  
  // Inity Array
  init_minion_member_data();
  
  // Fill array from pfiles that match this chars guid
  members  = load_minion_member_data(ch, PLAYER_DIR);
  members += load_minion_member_data(ch, PLAYER_DISGUISE_DIR);
  
  if (members <= 0) {
    send_to_char("No players found!\n\r", ch);
    return;	
  }
  
  if (!str_cmp(argument, "name")) {
    qsort (gd, members, sizeof(struct minion_member_data), compare_minion_member_names);
  }
  else if(!str_cmp(argument, "rank")) {
    qsort (gd, members, sizeof(struct minion_member_data), compare_minion_member_ranks);
  }
  else if(!str_cmp(argument, "logon")) {
    qsort (gd, members, sizeof(struct minion_member_data), compare_minion_member_logons);
  }
  else {
    send_to_char("Syntax: mmembers name\n", ch);
    send_to_char("        mmembers rank\n", ch);
    send_to_char("        mmembers logon\n", ch);
    return;
  }

  // Print result to screen
  print_minion_member_data_to_char(ch);
  
  return;
}

