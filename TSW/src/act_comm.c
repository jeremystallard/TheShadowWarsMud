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
*	ROM 2.4 is copyright 1993-1998 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@hypercube.org)				   *
*	    Gabrielle Taylor (gtaylor@hypercube.org)			   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "vt.h"
#include "screen.h"
#include "tables.h"
#include <errno.h>

/* 
 * Channel History made global so everyone can access it
 */
HISTORY_DATA chat_history[HISTSIZE];
HISTORY_DATA df_history[HISTSIZE];
HISTORY_DATA game_history[HISTSIZE];
HISTORY_DATA guildleader_history[HISTSIZE];
HISTORY_DATA imm_history[HISTSIZE];
HISTORY_DATA pray_history[PRAYHISTSIZE];
HISTORY_DATA gossip_history[HISTSIZE];
HISTORY_DATA newbie_history[HISTSIZE];
HISTORY_DATA wkt_history[HISTSIZE];
HISTORY_DATA shadow_history[HISTSIZE];

HISTORY_DATA guild_history[MAX_CLAN][HISTSIZE];
HISTORY_DATA oguild_history[MAX_CLAN][HISTSIZE];
HISTORY_DATA sguild_history[MAX_CLAN][HISTSIZE];
HISTORY_DATA ssguild_history[MAX_CLAN][HISTSIZE];
HISTORY_DATA race_history[60][HISTSIZE];

void do_addchannelhistory(CHAR_DATA *ch, HISTORY_DATA *history, char * argument, char *use_name);
void one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt, int dual, long target );
void censor_speech( CHAR_DATA *ch, char *text );
bool is_leader(CHAR_DATA * ch);
bool is_sguild_leader(CHAR_DATA * ch);
bool check_parse_name (char* name);  /* comm.c */

bool is_ignoring(CHAR_DATA *ch, CHAR_DATA *victim)
{
  int pos;
  CHAR_DATA *rch;

  if (ch->desc == NULL)
    rch = ch;
  else
    rch = ch->desc->original ? ch->desc->original : ch;
 
  for (pos = 0; pos < MAX_IGNORE; pos++) {
    if (rch->pcdata->ignore[pos] == NULL)
      break;

    if (!str_cmp(rch->pcdata->ignore[pos], victim->name))
      return TRUE;
  }

  return FALSE;
}

void do_ignore(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *victim, *rch;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int pos=0;
  int ignored=0;

  one_argument(argument, arg);

  if (ch->desc == NULL)
    rch = ch;
  else
    rch = ch->desc->original ? ch->desc->original : ch;
 
  if (IS_NPC(rch))
    return;

  if (arg[0] == '\0') {
    send_to_char("Who do you want to ignore?\n\r", ch);
    return;
  }
  
  if (!str_cmp(arg, "list")) {
     send_to_char("List of players you ignore:\n\r", ch);     
    for (pos = 0; pos < MAX_IGNORE; pos++) {
       if (rch->pcdata->ignore[pos] != NULL) {
       	ignored++;
    	sprintf(buf, "[%d] %s\n\r", ignored, ch->pcdata->ignore[pos]);
    	send_to_char(buf, ch);       
       }
    }
    if (ignored < 1)
      send_to_char("None.\n\r", ch);
    return;     
  }

  if ((victim = get_char_world(rch, argument)) == NULL) {
    send_to_char("They aren't here.\n\r", ch);
    return;
  }

  if (IS_NPC(victim)) {
    send_to_char("Ignore a mob?  I don't think so.\n\r", ch);
    return;
  }

  if (IS_IMMORTAL(victim)) {
    send_to_char("You can't ignore the immortals.\n\r", ch);
    return;	
  }

  if (ch == victim) {
    send_to_char("I don't think you really want to ignore yourself.\n\r", ch);
    return;
  }

  for (pos = 0; pos < MAX_IGNORE; pos++) {
    if (rch->pcdata->ignore[pos] == NULL)
      break;

    if (!str_cmp(arg, rch->pcdata->ignore[pos])) {
      free_string(rch->pcdata->ignore[pos]);
      rch->pcdata->ignore[pos] = NULL;
      sprintf(buf, "You stop ignoring %s.\n\r", victim->name);
      send_to_char(buf, ch);
      //sprintf(buf, "%s stops ignoring you.\n\r", ch->name);
      //send_to_char(buf, victim);
      return;
    }
  }
 
  if (pos >= MAX_IGNORE) {
    send_to_char("You can't ignore anymore people.\n\r", ch);
    return;
  }
 
  rch->pcdata->ignore[pos] = str_dup(arg);
  sprintf(buf, "You now ignore %s.\n\r", victim->name);
  send_to_char(buf, ch);
//  sprintf(buf, "%s ignores you.\n\r", ch->name);
//  send_to_char(buf, victim);
  return;
}

/* Replace '{x' with the code */
char *colorcontinue(char code, char *argument)
{
  static char target[MAX_STRING_LENGTH];
  int i=0;
  
  if (argument == NULL || argument[0] == '\0')
    return 0;
  
  if (code == '\0')
    return argument;
  
  memset(target, 0x00, sizeof(target));
  
  /* Replace all terminating color codes with first code */
  for (i=0; i < strlen(argument); i++) {
    if (argument[i-1] == '{' && UPPER(argument[i]) == 'X')  {
	 target[i] = code;
    }
    else {
	 target[i] = argument[i];
    }    
  }
  
  return target;
}

/* RT code to delete yourself */

void do_delet( CHAR_DATA *ch, char *argument)
{
    send_to_char("You must type the full command to delete yourself.\n\r",ch);
}

void do_delete( CHAR_DATA *ch, char *argument)
{
  char strsave[MAX_INPUT_LENGTH];
  
  if (IS_NPC(ch))
    return;

  if (IS_DISGUISED(ch)) {
    send_to_char("Use the options to disguise to delete a disguise.\n\r", ch);
    return;
  }

  
  if (ch->pcdata->confirm_delete) {
    if (argument[0] != '\0') {
	 send_to_char("Delete status removed.\n\r",ch);
	 ch->pcdata->confirm_delete = FALSE;
	 return;
    }
    else {
	 sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( ch->name ) );
	 wiznet("$N has {RDELETED{x $Mself.",ch,NULL,0,0,0);
	 stop_fighting(ch,TRUE);
	 do_function(ch, &do_quit, "");
	 unlink(strsave);
	 return;
    }
  }
  
  if (argument[0] == '\0') {
    send_to_char("Syntax is: delete <password>.\n\r",ch);
    return;
  }

  if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd )) 
  {
    send_to_char("Wrong password.\n\r",ch);
    return;
  }
  
  send_to_char("Type delete again to confirm this command.\n\r",ch);
  send_to_char("{RWARNING{x: this command is irreversible.\n\r",ch);
  send_to_char("Typing delete with an argument will undo delete status.\n\r", ch);

  ch->pcdata->confirm_delete = TRUE;
  wiznet("$N is contemplating deletion.",ch,NULL,0,0,get_trust(ch));
}
	    

/* RT code to display channel status */

void do_channels( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    send_to_char("{WChannel        Status\n\r",ch);
    send_to_char("---------------------\n\r",ch);
 
    send_to_char("{WChat{x           ",ch);
    if (!IS_SET(ch->comm,COMM_NOCHAT))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("{gGame{x           ",ch);
    if (!IS_SET(ch->comm,COMM_NOGAME))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    if (is_clan(ch)) {
	 send_to_char("{8Guild{x          ", ch);
	 if (!IS_SET(ch->comm,COMM_NOCLAN))
	   send_to_char("ON\n\r",ch);
	 else
	   send_to_char("OFF\n\r",ch);
    }
    
    if (is_oguild(ch)) {
	 send_to_char("{NOGuild{x         ", ch);
	 if (!IS_SET(ch->comm2,COMM2_NOOGUILD))
	   send_to_char("ON\n\r",ch);
	 else
	   send_to_char("OFF\n\r",ch);    	    	
    }

    if (is_sguild(ch)) {
	 send_to_char("{SSubguild{x       ", ch);
	 if (!IS_SET(ch->comm,COMM_NOSGUILD))
	   send_to_char("ON\n\r",ch);
	 else
	   send_to_char("OFF\n\r",ch);
    }

    if (is_ssguild(ch)) {
	 send_to_char("{USSubguild{x      ", ch);
	 if (!IS_SET(ch->comm,COMM_NOSSGUILD))
	   send_to_char("ON\n\r",ch);
	 else
	   send_to_char("OFF\n\r",ch);
    }    

    if (IS_NEWBIE(ch) || IS_IMMORTAL(ch)) {
      send_to_char("{MNewbie{x         ",ch);
      if(!IS_SET(ch->comm,COMM_NONEWBIE))
        send_to_char("ON\n\r",ch);
      else
        send_to_char("OFF\n\r",ch);
    }

    send_to_char("{MHint{x           ",ch);
    if(!IS_SET(ch->comm,COMM_NOHINT))
	 send_to_char("ON\n\r",ch);
    else
	 send_to_char("OFF\n\r",ch);
    
    if (IS_IMMORTAL(ch)) {
	 send_to_char("{RForetelling{x    ",ch);
	 if(!IS_SET(ch->comm,COMM_FORETELLING))
        send_to_char("ON\n\r",ch);
      else
        send_to_char("OFF\n\r",ch);
    }

    send_to_char("{WPray{x           ",ch);
    send_to_char("ON\n\r",ch);

    send_to_char("{dGossip{x         ",ch);
    if (!IS_SET(ch->comm,COMM_NOGOSSIP))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("{aAuction{x        ",ch);
    if (!IS_SET(ch->comm,COMM_NOAUCTION))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("{eMusic{x          ",ch);
    if (!IS_SET(ch->comm,COMM_NOMUSIC))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    if (IS_IMMORTAL(ch))
    {
      send_to_char("{rImmortal{x       ",ch);
      if(!IS_SET(ch->comm,COMM_NOWIZ))
        send_to_char("ON\n\r",ch);
      else
        send_to_char("OFF\n\r",ch);
    }

    send_to_char("Race talk      ",ch);
    if (!IS_SET(ch->comm, COMM_NORACE))
	send_to_char("ON\n\r",ch);
    else
	send_to_char("OFF\n\r",ch);
    
    if (IS_WOLFKIN(ch)) {
	 send_to_char("Wolfkin talk   ", ch);
	 if (!IS_SET(ch->comm, COMM_NOWKT))
	   send_to_char("ON\n\r",ch);
	 else
	   send_to_char("OFF\n\r",ch);
    }
    
    if (IS_FORSAKEN(ch) || IS_IMMORTAL(ch) ||
       (ch->clan == clan_lookup("Shadowspawn") && is_leader(ch)) ||
       (ch->race == race_lookup("trolloc") && ch->sguild_rank == 0)) {
	 send_to_char("{DShadow talk{x    ", ch);
	 if (!IS_SET(ch->comm, COMM_NOSHADOW))
	   send_to_char("ON\n\r",ch);
	 else
	   send_to_char("OFF\n\r",ch);       	
    }

    send_to_char("{kTells{x          ",ch);
    if (!IS_SET(ch->comm,COMM_DEAF))
	send_to_char("ON\n\r",ch);
    else
	send_to_char("OFF\n\r",ch);

    send_to_char("{WQuiet mode{x     ",ch);
    if (IS_SET(ch->comm,COMM_QUIET))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    if (IS_SET(ch->comm,COMM_AFK))
	send_to_char("You are AFK.\n\r",ch);

   /* Separate channels and other info */
   send_to_char("\n\r",ch);   

    if (IS_SET(ch->comm,COMM_SNOOP_PROOF))
	send_to_char("You are immune to snooping.\n\r",ch);
   
    if (ch->lines != PAGELEN)
    {
	if (ch->lines)
	{
	    sprintf(buf,"You display %d lines of scroll.\n\r",ch->lines+2);
	    send_to_char(buf,ch);
 	}
	else
	    send_to_char("Scroll buffering is off.\n\r",ch);
    }

    if (ch->prompt != NULL)
    {
	sprintf(buf,"Your current prompt is: %s\n\r",ch->prompt);
	send_to_char(buf,ch);
    }

    if (IS_SET(ch->comm,COMM_NOTELL))
      send_to_char("You cannot use tell.\n\r",ch);
    
    if (IS_SET(ch->comm,COMM_NOCHANNELS))
	 send_to_char("You cannot use channels.\n\r",ch);
    
    if (IS_SET(ch->comm,COMM_NOEMOTE))
      send_to_char("You cannot show emotions.\n\r",ch);    
}

/* RT deaf blocks out all shouts */

void do_deaf( CHAR_DATA *ch, char *argument)
{
    
   if (IS_SET(ch->comm,COMM_DEAF))
   {
     send_to_char("You can now hear tells again.\n\r",ch);
     REMOVE_BIT(ch->comm,COMM_DEAF);
   }
   else 
   {
     send_to_char("From now on, you won't hear tells.\n\r",ch);
     SET_BIT(ch->comm,COMM_DEAF);
   }
}

/* RT quiet blocks out all communication */

void do_quiet ( CHAR_DATA *ch, char * argument)
{
    if (IS_SET(ch->comm,COMM_QUIET))
    {
      send_to_char("Quiet mode removed.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_QUIET);
    }
   else
   {
     send_to_char("From now on, you will only hear says and emotes.\n\r",ch);
     SET_BIT(ch->comm,COMM_QUIET);
   }
}

/* afk command */

void do_afk ( CHAR_DATA *ch, char * argument)
{
  char buf[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
    return;
  
  if (IS_SET(ch->comm,COMM_AFK)) {
    act( "{x$n removes the AFK sticker from $s head", ch, NULL, NULL, TO_ROOM );
    send_to_char("AFK mode removed. Type '{Wtellh{x' to see tells.\n\r",ch);
    REMOVE_BIT(ch->comm,COMM_AFK);

    free_string(ch->pcdata->afkmsg);
    ch->pcdata->afkmsg = str_dup( "" );
  }
  else {
    send_to_char("You are now in AFK mode.\n\r",ch);
    SET_BIT(ch->comm,COMM_AFK);

    // AFK msg
    if (!IS_NULLSTR(argument)) {
	 if (colorstrlen(argument) > 45 ) {
	   send_to_char("AFK message must be less than 45 characters long (not including color codes).\n\r", ch);
	   return;
	 }

	 if ( argument[0] != '.' && argument[0] != ',' && argument[0] != '!' && argument[0] != '?' ) {
	   sprintf(buf, " %s", argument);
	 }
	 else {
	   sprintf(buf, "%s", argument);
	 }
	 
	 free_string(ch->pcdata->afkmsg);
	 ch->pcdata->afkmsg = str_dup(buf);
    }

    act( "{x$n puts an AFK sticker on $s head", ch, NULL, NULL, TO_ROOM );
  }

  return;
}

void do_replay (CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    {
	send_to_char("You can't replay.\n\r",ch);
	return;
    }

    if (buf_string(ch->pcdata->buffer)[0] == '\0')
    {
	send_to_char("You have no tells to replay.\n\r",ch);
	return;
    }

    page_to_char(buf_string(ch->pcdata->buffer),ch);
    clear_buf(ch->pcdata->buffer);
}

/* RT auction rewritten in ROM style */
void do_auction( CHAR_DATA *ch, char *argument )
{
    /* char buf[MAX_STRING_LENGTH]; */
    char cemotebuf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOAUCTION))
      {
	send_to_char("{aAuction channel is now ON.{x\n\r",ch);
	REMOVE_BIT(ch->comm,COMM_NOAUCTION);
      }
      else
      {
	send_to_char("{aAuction channel is now OFF.{x\n\r",ch);
	SET_BIT(ch->comm,COMM_NOAUCTION);
      }
    }
    else  /* auction message sent, turn auction on if it is off */
    {
	if (IS_SET(ch->comm,COMM_QUIET))
	{
	  send_to_char("You must turn off quiet mode first.\n\r",ch);
	  return;
	}

	if (IS_SET(ch->comm,COMM_NOCHANNELS))
	{
	  send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
	  return;
	}

	REMOVE_BIT(ch->comm,COMM_NOAUCTION);
	
	if (!IS_EMOTE(argument))
	  act_old("{w[{aAuction{w]{W: {a$n {x'{a$t{x'",ch,argument,NULL,TO_CHAR,POS_SLEEPING);
	else {  
	  strcpy(cemotebuf,&argument[1]);
	  cemotebuf[strlen(cemotebuf)-1] = '\0';
	  act_old("{w[{aAuction{w]{W: {w'{a$n {a$t{x'",ch,cemotebuf,NULL,TO_CHAR,POS_SLEEPING);
	}
	
	for ( d = descriptor_list; d != NULL; d = d->next )
	  {
	    CHAR_DATA *victim;
	    
	    victim = d->original ? d->original : d->character;
	    
	    if ( d->connected == CON_PLAYING &&
		 d->character != ch &&
		 !IS_SET(victim->comm,COMM_NOAUCTION) &&
		 !IS_SET(victim->comm,COMM_QUIET) )
	      {
		
		if (!IS_EMOTE(argument)){   
		  act_old( "{w[{aAuction{w]{W: {a$n {x'{a$t{x'", 
			   ch,argument, d->character, TO_VICT,POS_SLEEPING ); 
		}
		else {  
		  act_old( "{w[{aAuction{w]{W: {w'{a$n {a$t{x'", 
			   ch,cemotebuf, d->character, TO_VICT,POS_SLEEPING ); 
		}	  
	      }
	  }
    }
}

/* RT chat replaced with ROM gossip */
void do_gossip( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  
  if (argument[0] == '\0' ) {
    if (IS_SET(ch->comm,COMM_NOGOSSIP)) {
      send_to_char("Gossip channel is now ON.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_NOGOSSIP);
    }
    else {
      send_to_char("Gossip channel is now OFF.\n\r",ch);
      SET_BIT(ch->comm,COMM_NOGOSSIP);
    }
  }
  else {  /* gossip message sent, turn gossip on if it isn't already */
    if (IS_SET(ch->comm,COMM_QUIET)) {
      send_to_char("You must turn off quiet mode first.\n\r",ch);
      return;
    }
    
    if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
      send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
      return;
    }
    
    REMOVE_BIT(ch->comm,COMM_NOGOSSIP);
    if (IS_AFFECTED(ch,AFF_GAGGED))
    {
      send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
      return;
    }

    
    buf[0] = '\0';
    sprintf(buf,"{u$N {ugossips{W: {u%s{x",argument);
    wiznet(buf,ch,NULL,WIZ_ON,0,0);
    
    act_old("{dWord spreads through the land that $t{x", ch,argument,NULL,TO_CHAR,POS_SLEEPING);
    
    do_addchannelhistory(ch,gossip_history,argument, NULL);
    for ( d = descriptor_list; d != NULL; d = d->next ) {
      CHAR_DATA *victim;
      
      victim = d->original ? d->original : d->character;
      
      if ( d->connected == CON_PLAYING &&
	   d->character != ch &&
	   !IS_SET(victim->comm,COMM_NOGOSSIP) &&
	   !IS_SET(victim->comm,COMM_QUIET) ) {
	
	act_old("{dWord spreads through the land that $t{x", 
		ch,argument,d->character,TO_VICT,POS_SLEEPING);
      }
    }
  }
}

/* RT music channel */
void do_music( CHAR_DATA *ch, char *argument )
{
  DESCRIPTOR_DATA *d;
  char cemotebuf[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  
  if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOMUSIC))
      {
        send_to_char("Music channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOMUSIC);
      }
      else
      {
        send_to_char("Music channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOMUSIC);
      }
    }
    else  /* music sent, turn music on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
	}
 
        REMOVE_BIT(ch->comm,COMM_NOMUSIC);

	   // Make sure no cursing etc. on public channels
	   censor_speech(ch, argument);
	   
	if (!IS_EMOTE(argument))
        { 
          act_old("{w[{eMusic{w]{W: {x$n '{e$t{x'",ch,colorcontinue('e',argument),NULL,TO_CHAR,POS_SLEEPING);
	  sprintf(buffer,"Music: {G%s '%s{x'",ch->name,colorcontinue('G', argument));

        }
	else
	  {  strcpy(cemotebuf,&argument[1]);
	  cemotebuf[strlen(cemotebuf)-1] = '\0';
	  act_old("{w[{eMusic{w]{W: {w'{e$n {e$t{x'",ch,colorcontinue('e', cemotebuf),NULL,TO_CHAR,POS_SLEEPING);
	  sprintf(buffer,"Music: {G%s '%s{x'",ch->name,colorcontinue('G', cemotebuf));
	  }

      log_comm_string("music",buffer);


      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        CHAR_DATA *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOMUSIC) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
          if (!IS_EMOTE(argument)){   
	    act_old( "{w[{eMusic{w]{W: {x$n {w'{e$t{x'", 
		     ch,colorcontinue('e',argument), d->character, TO_VICT,POS_SLEEPING ); 
	  }
          else {  
	    act_old( "{w[{eMusic{w]{W: {w'{e$n {e$t{x'", 
		     ch,colorcontinue('e',cemotebuf), d->character, TO_VICT,POS_SLEEPING ); 
          }



        }
      }
    }
}

/* Wolfkin channels */
void do_wkt( CHAR_DATA *ch, char *argument )
{
  DESCRIPTOR_DATA *d;
  char buf[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  bool found = FALSE;

  static char * const brother_sister [] = { "Kind   ",  "Brother", "Sister " };
 
  if (!IS_WOLFKIN(ch) && IS_SET(ch->talents, TALENT_WOLFKIN)) {
   send_to_char("You are too young to be able to communicate with other wolves!\n\r", ch);
   return;
  } 
  
  if (!IS_WOLFKIN(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (ch->wkname[0] == '\0') {
    send_to_char("You need to give your self a wolfkin name first!\n\r", ch);
    return;
  }

  if (IS_SET(ch->comm,COMM_QUIET)) {
     send_to_char("You must turn off quiet mode first.\n\r",ch);
     return;
  }

  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm,COMM_NOWKT)) {
	 send_to_char("Wolfkin channel is now ON\n\r",ch);
	 REMOVE_BIT(ch->comm,COMM_NOWKT);
    }
    else {
	 send_to_char("Wolfkin channel is now OFF\n\r",ch);
	 SET_BIT(ch->comm,COMM_NOWKT);
    } 
    return;
  }
  
  REMOVE_BIT(ch->comm,COMM_NOWKT);
    
  if (!IS_EMOTE(argument)) {
    sprintf(buf, "{Y[{6Wolf %s{Y]{W:{x %s {x'{6%s{x'\n\r", brother_sister[URANGE(0,ch->sex, 2)], ch->wkname, colorcontinue('6',argument));
    send_to_char(buf, ch);
  }
  else {   
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf(buf, "{Y[{6Wolf %s{Y]{W:{x '{6%s {6%s{x'\n\r", brother_sister[URANGE(0,ch->sex, 2)], ch->wkname,  colorcontinue('6',cemotebuf));
    send_to_char(buf, ch);
  }
    
  do_addchannelhistory(ch,wkt_history,argument, ch->wkname);
  for ( d = descriptor_list; d != NULL; d = d->next ) {
     if (d->connected == CON_PLAYING && 
         IS_WOLFKIN(d->character) &&
         //d->character->in_room->area == ch->in_room->area &&
	      d->character != ch &&
	      !IS_SET(d->character->comm,COMM_NOWKT) ){
	     if (!IS_EMOTE(argument)) {
	        found = TRUE;
	        sprintf(buf, "{Y[{6Wolf %s{Y]{W:{x %s {x'{6%s{x'\n\r", can_see(d->character, ch) ? brother_sister[URANGE(0,ch->sex, 2)] : "soule", can_see_channel(d->character, ch) ? ch->wkname : "Someone",  colorcontinue('6',argument));
	        send_to_char(buf, d->character);
	     }
	     else {
	        found = TRUE;
	        sprintf(buf, "{Y[{6Wolf %s{Y]{W:{x '{6%s {6%s{x'\n\r", can_see(d->character, ch) ? brother_sister[URANGE(0,ch->sex, 2)] : "soule", can_see_channel(d->character, ch) ? ch->wkname : "Someone",  colorcontinue('6',cemotebuf));
	        send_to_char(buf, d->character);
	     }
     }
  } 
  
  if (!found) {
     send_to_char("You don't sense anyone else out there right now.\n\r", ch); 
  }
  
  return;
}

void do_wkth(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  int entry=FALSE;
  
  if (!IS_WOLFKIN(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  /*
   *  Do channel history
   */  
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (wkt_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  wkt_history[i].wizinvis_level) ||
		  ( (wkt_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(wkt_history[i].line_data)) { 
		  sprintf (line_detail,"{Y[{6Wolfkind{y %s{Y]{W: {wSomeone '{6%s{x'\n\r",   
				 sec2strtime(wkt_history[i].when),
				 wkt_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&wkt_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{Y[{6Wolfkind{y %s{Y]{W: '{6Someone {6%s{x'\n\r",sec2strtime(wkt_history[i].when),cemotebuf);
		}
	   }
	   else {
		if (!IS_EMOTE(wkt_history[i].line_data)) { 
		  sprintf (line_detail,"{Y[{6Wolfkind{y %s{Y]{W: {w%s '{6%s{x'\n\r",   
				 sec2strtime(wkt_history[i].when),
				 wkt_history[i].player_name, 
				 wkt_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&wkt_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{Y[{6Wolfkind{y %s{Y]{W: '{6%s {6%s{x'\n\r",
				 sec2strtime(wkt_history[i].when),
				 wkt_history[i].player_name, cemotebuf);
		}
	   }
	   send_to_char(line_detail,ch);
	 }
    }
  }
  if (!entry)
    send_to_char("No history found.\n\r", ch);
}

/* Shadow channels */
void do_shadowtalk( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  
  if (ch->clan != clan_lookup("Shadowspawn") && !IS_FORSAKEN(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  // If not Forsaken, check that either is leader in shadowspawn guild
  // or clan leader of sguild
  if (!IS_FORSAKEN(ch) && !IS_IMMORTAL(ch)) {
    if (ch->race == race_lookup("trolloc") && ch->sguild_rank != 0) {
       send_to_char("{DYou are not a clan leader!{x\n\r", ch);
       return;
    }

    if (ch->race != race_lookup("trolloc") && !is_leader(ch)) {
       send_to_char("{DYou are not one of the leaders of the shadowspawn!{x\n\r", ch);
       return;    	
    }
  }
  
  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm,COMM_NOSHADOW)) {
	 send_to_char("Shadow channel is now ON\n\r",ch);
	 REMOVE_BIT(ch->comm,COMM_NOSHADOW);
    }
    else {
	 send_to_char("Shadow channel is now OFF\n\r",ch);
	 SET_BIT(ch->comm,COMM_NOSHADOW);
    }
    return;
  }

  if (IS_SET(ch->comm,COMM_QUIET)) {
     send_to_char("You must turn off quiet mode first.\n\r",ch);
     return;
  }
  
  if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }

  if (IS_AFFECTED(ch,AFF_GAGGED)) {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }

    if (IS_SET(ch->comm,COMM_NOSHADOW)) {
	 send_to_char("Turn Shadow channel on to use it.\n\r",ch);
	return;
    }
  
  if (!IS_EMOTE(argument))
    sprintf(buf, "{x[{DShadow{x]{W:{x %s '{D%s{x'\n\r", COLORNAME(ch), colorcontinue('D',argument));
  else {   
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf(buf, "{x[{DShadow{x]{W:{x '{r%s {D%s{x'\n\r", COLORNAME(ch), colorcontinue('D',cemotebuf));
  }

  do_addchannelhistory(ch,shadow_history,argument, NULL);

  for ( d = descriptor_list; d != NULL; d = d->next ) {
    if ( d->connected == CON_PLAYING &&
         !IS_SET(d->character->comm,COMM_NOSHADOW) &&
         !IS_SET(d->character->comm,COMM_QUIET) ) {         
         if (IS_FORSAKEN(d->character) || 
             IS_IMMORTAL(d->character) ||
             (d->character->clan == clan_lookup("Shadowspawn") && is_leader(d->character)) ||
             (d->character->race == race_lookup("trolloc") && d->character->sguild_rank == 0)) {                  	
	    if (!IS_EMOTE(argument)) {
	    	sprintf(buf, "{x[{DShadow{x]{W:{x %s '{D%s{x'\n\r", PERS_OLD(ch, d->character), colorcontinue('D',argument));
                send_to_char(buf, d->character);
	    }
	    else {
	    	sprintf(buf, "{x[{DShadow{x]{W:{x '{D%s {D%s{x'\n\r", PERS_OLD(ch, d->character), colorcontinue('D',cemotebuf));
                send_to_char(buf, d->character);
	   }
        }
     }
   }  
  return;
}

void do_shadowtalkh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  int entry=FALSE;
  
  if (ch->clan != clan_lookup("Shadowspawn") && !IS_FORSAKEN(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  // If not Forsaken, check that either is leader in shadowspawn guild
  // or clan leader of sguild
  if (!IS_FORSAKEN(ch) && !IS_IMMORTAL(ch)) {
    if (ch->race == race_lookup("trolloc") && ch->sguild_rank != 0) {
	 send_to_char("{DYou are not a clan leader!{x\n\r", ch);
	 return;
    }
    
    if (ch->race != race_lookup("trolloc") && !is_leader(ch)) {
	 send_to_char("{DYou are not one of the leaders of the shadowspawn!{x\n\r", ch);
	 return;    	
    }
  }
  
  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (shadow_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  shadow_history[i].wizinvis_level) ||
		  ( (shadow_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(shadow_history[i].line_data)) { 
		  sprintf (line_detail,"{x[{DShadow{y %s{x]{W:{x Someone '{D%s{x'\n\r",   
				 sec2strtime(shadow_history[i].when),
				 shadow_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&shadow_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{x[{DShadow{x %s]{W:{x '{DSomeone {D%s{x'\n\r",sec2strtime(shadow_history[i].when),cemotebuf);
		  
		}
	   }
	   else {
		if (!IS_EMOTE(shadow_history[i].line_data)) { 
		  sprintf (line_detail,"{x[{DShadow{y %s{x]{W:{x %s '{D%s{x'\n\r",   
				 sec2strtime(shadow_history[i].when),
				 shadow_history[i].player_name, 
				 shadow_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&shadow_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{x[{DShadow{y %s{x]{W:{x '{D%s {D%s{x'\n\r",
				 sec2strtime(shadow_history[i].when),
				 shadow_history[i].player_name, cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

/* Minion channels */
void do_miniontalk( CHAR_DATA *ch, char *argument )
{
  char buf2[MAX_STRING_LENGTH+10];
  char buf[MAX_STRING_LENGTH];
  char category[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  
  if (ch->minion == 0) {
    send_to_char("You aren't in a minion.\n\r",ch);
    return;
  }

  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm,COMM_NOMINION)) {
	 send_to_char("Minion channel is now ON\n\r",ch);
	 REMOVE_BIT(ch->comm,COMM_NOMINION);
    }
    else {
	 send_to_char("Minion channel is now OFF\n\r",ch);
	 SET_BIT(ch->comm,COMM_NOMINION);
    }
    return;
  }
  
  if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }
  if (IS_AFFECTED(ch,AFF_GAGGED))
  {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }

    if (IS_SET(ch->comm,COMM_NOMINION)) {
	 send_to_char("Turn Minion channel on to use it.\n\r",ch);
	return;
    }
  
  if (!IS_EMOTE(argument))
  {
    sprintf(buf, "[%s](%s)  %s '{h%s{x'\n\r", ch->mname,
		  ch->mtitle ? ch->mtitle : "Unassigned" , COLORNAME(ch),  colorcontinue('h',argument));
  }
  else {   
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf( buf, "[%s](%s)  '{h%s {h%s{x'\n\r", ch->mname, 
		   ch->gtitle ? ch->gtitle : "Unassigned", COLORNAME(ch), colorcontinue('h',cemotebuf));
  }

   sprintf(buf2,"Wiznet: %s",buf);
   wiznet(buf2,ch,NULL,WIZ_MINION,0,0);

   sprintf(category,"minion/%s",ch->mname);
   int i = 0;
   for (i = 0; i < strlen(category); i++)
   {
	if (category[i] == ' ' || category[i] == '\'')
           category[i] = '_';
   } 

   log_comm_string(category,buf);

  for ( d = descriptor_list; d != NULL; d = d->next ) {
    if ( d->connected == CON_PLAYING &&
	    (ch->minion == d->character->minion || (IS_IMMORTAL(d->character) && IS_SET(d->character->wiznet, WIZ_SPAM))) &&
	    !IS_SET(d->character->comm,COMM_NOMINION) &&
	    !IS_SET(d->character->comm,COMM_QUIET) ) {
	 if (!IS_EMOTE(argument)) {
	   sprintf(buf, "[%s](%s)  %s '{h%s{x'\n\r", ch->mname, 
			 can_see_channel(d->character, ch) ? ch->mtitle ? ch->mtitle : "Unassigned" : "concealed",
			 PERS_OLD(ch, d->character), colorcontinue('h',argument));
	   send_to_char(buf, d->character);
	 }
	 else {
	   sprintf(buf, "[%s](%s)  '{h%s {h%s{x'\n\r", ch->mname, 
			 can_see_channel(d->character, ch) ? ch->mtitle ? ch->mtitle : "Unassigned": "concealed",
			 PERS_OLD(ch, d->character), colorcontinue('h',cemotebuf));
	   send_to_char(buf, d->character);
	 }
	 do_addchannelhistory(ch,d->character->pcdata->minion_history,argument, NULL);
    }
  }  
  return;
}

void do_miniontalkh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  /* char cemotebuf[MAX_STRING_LENGTH]; */
  int entry=FALSE;
  
  line_detail[0] = '\0';

  if (ch->minion == 0) {
    send_to_char("You aren't in a minion.\n\r",ch);
    return;
  }
  
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (ch->pcdata->minion_history[i].player_name) {
	   entry=TRUE;
	   sprintf (line_detail,"[%s{y %s{x]{W:{x %s '{h%s{x'\n\r",
	                  ch->mname,
			  sec2strtime(ch->pcdata->minion_history[i].when),
			  ch->pcdata->minion_history[i].player_name, 
			  ch->pcdata->minion_history[i].line_data);
	   
	 }
    }
    send_to_char(line_detail,ch);
    line_detail[0] = '\0';
  }
  
  if (!entry)
    send_to_char("No history found.\n\r", ch);
}

/* Minion Immortal channels */
void do_minionimmtalk( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg1[MSL];
  char cemotebuf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim=NULL;
  
  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg1);

  if (IS_NULLSTR(arg1)) {
    send_to_char("Syntax: mit <victim> <text>\n\r", ch);
    return;
  }

  if (!(victim = get_char_world(ch, arg1))) {
    send_to_char("They aren't here.\n\r", ch);
    return;
  }
  
  if (victim->minion == 0) {
    send_to_char("They aren't in a minion.\n\r",ch);
    return;
  }
  
  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm,COMM_NOMINION)) {
	 send_to_char("Minion channel is now ON\n\r",ch);
	 REMOVE_BIT(ch->comm,COMM_NOMINION);
    }
    else {
	 send_to_char("Minion channel is now OFF\n\r",ch);
	 SET_BIT(ch->comm,COMM_NOMINION);
    }
    return;
  }

  if (IS_SET(ch->comm,COMM_QUIET)) {
     send_to_char("You must turn off quiet mode first.\n\r",ch);
     return;
  }
  
  if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_GAGGED)) {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }
  
  if (IS_SET(ch->comm,COMM_NOMINION)) {
    send_to_char("Turn Minion channel on to use it.\n\r",ch);
    return;
  }
   
  if (!IS_EMOTE(argument))
    sprintf(buf, "[%s](%s)  %s '{h%s{x'\n\r", victim->mname,
		  "Immortal" , COLORNAME(ch),  colorcontinue('h',argument));
  else {   
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf( buf, "[%s](%s)  '{h%s {h%s{x'\n\r", victim->mname, 
		   "Immortal", COLORNAME(ch), colorcontinue('h',cemotebuf));
  }
  
  for ( d = descriptor_list; d != NULL; d = d->next ) {
    if ( d->connected == CON_PLAYING &&
	    (victim->minion == d->character->minion || ch == d->character || (IS_IMMORTAL(d->character) && IS_SET(d->character->wiznet, WIZ_SPAM))) &&
	    !IS_SET(d->character->comm,COMM_NOMINION) &&
	    !IS_SET(d->character->comm,COMM_QUIET) ) {
	 if (!IS_EMOTE(argument)) {
	   sprintf(buf, "[%s](%s)  %s '{h%s{x'\n\r", victim->mname, 
			 can_see_channel(d->character, ch) ? "Immortal" : "concealed",
			 PERS_OLD(ch, d->character), colorcontinue('h',argument));
	   send_to_char(buf, d->character);
	 }
	 else {
	   sprintf(buf, "[%s](%s)  '{h%s {h%s{x'\n\r", victim->mname, 
			 can_see_channel(d->character, ch) ? "Immortal": "concealed",
			 PERS_OLD(ch, d->character), colorcontinue('h',cemotebuf));
	   send_to_char(buf, d->character);
	 }
	 do_addchannelhistory(ch,d->character->pcdata->minion_history,argument, NULL);
    }
  }  
  return;
}

/* Guild channels */
void do_guildtalk( CHAR_DATA *ch, char *argument )
{
  char buf2[MAX_STRING_LENGTH+10];
  char buf[MAX_STRING_LENGTH];
  char category[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  
  if (!is_clan(ch)) {
    send_to_char("You aren't in a guild.\n\r",ch);
    return;
  }
  
  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm,COMM_NOCLAN)) {
	 send_to_char("Guild channel is now ON\n\r",ch);
	 REMOVE_BIT(ch->comm,COMM_NOCLAN);
    }
    else {
	 send_to_char("Guild channel is now OFF\n\r",ch);
	 SET_BIT(ch->comm,COMM_NOCLAN);
    }
    return;
  }

  if (IS_SET(ch->comm,COMM_QUIET)) {
     send_to_char("You must turn off quiet mode first.\n\r",ch);
     return;
  }
  
  if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_GAGGED)) {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }
  
  if (IS_SET(ch->comm,COMM_NOCLAN)) {
    send_to_char("Turn Guild channel on to use it.\n\r",ch);
    return;
  }
  
  if (ch->gmute) {
    send_to_char("The guild leaders have revoked your guild channel priviliges.\n\r", ch);
    return;
  }
  
  // Make sure no cursing etc. on public channels
  censor_speech(ch, argument);
  
  if (!IS_EMOTE(argument))
    sprintf(buf, "{w[%s{w](%s) %s '{8%s{x'\n\r", clan_table[ch->clan].who_name,
		  ch->gtitle ? ch->gtitle : player_rank(ch), COLORNAME(ch), colorcontinue('8', argument));
  else {   
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf( buf, "{w[%s](%s)  '{8%s {8%s{x'\n\r", clan_table[ch->clan].who_name, 
		   ch->gtitle ? ch->gtitle : player_rank(ch), COLORNAME(ch), colorcontinue('8', cemotebuf));
  }

  sprintf(buf2,"Wiznet: %s",buf);
  wiznet(buf2,ch,NULL,WIZ_GUILD,0,0);

  do_addchannelhistory(ch, guild_history[ch->clan],argument, NULL);
  do_addchannelhistory(ch, oguild_history[ch->clan],argument, NULL);

   sprintf(category,"guild/%s",clan_table[ch->clan].name);
   int i = 0;
   for (i = 0; i < strlen(category); i++)
   {
	if (category[i] == ' ' || category[i] == '\'')
           category[i] = '_';
   } 
  log_comm_string(category,buf);

  for ( d = descriptor_list; d != NULL; d = d->next ) {
    if ( d->connected == CON_PLAYING &&
	    (is_same_clan(ch,d->character) || ch->clan == d->character->oguild) &&	    
	    !IS_SET(d->character->comm,COMM_NOCLAN) &&
	    !IS_SET(d->character->comm2,COMM2_NOOGUILD) &&	    
	    !IS_SET(d->character->comm,COMM_QUIET) ) {
	 if (d->character->oguild == ch->clan) {
	    if (!IS_EMOTE(argument)) {
	      sprintf(buf, "{w[%s](%s)  %s '{N%s{x'\n\r", clan_table[ch->clan].who_name, 
		   	 can_see_channel(d->character, ch) ? ch->gtitle ? ch->gtitle : player_rank(ch) : "concealed",
		   	 PERS_OLD(ch, d->character), colorcontinue('N', argument));
	      send_to_char(buf, d->character);
	    }
	    else {
	      sprintf(buf, "{w[%s](%s)  '{N%s {N%s{x'\n\r", clan_table[ch->clan].who_name, 
		   	 can_see_channel(d->character, ch) ? ch->gtitle ? ch->gtitle : player_rank(ch) : "concealed",
		   	 PERS_OLD(ch, d->character), colorcontinue('N', cemotebuf));
	      send_to_char(buf, d->character);
	    }
	 	
	 }
	 else {
	    if (!IS_EMOTE(argument)) {
	      sprintf(buf, "{w[%s](%s)  %s '{8%s{x'\n\r", clan_table[ch->clan].who_name, 
		   	 can_see_channel(d->character, ch) ? ch->gtitle ? ch->gtitle : player_rank(ch) : "concealed",
		   	 PERS_OLD(ch, d->character), colorcontinue('8', argument));
	      send_to_char(buf, d->character);
	    }
	    else {
	      sprintf(buf, "{w[%s](%s)  '{8%s {8%s{x'\n\r", clan_table[ch->clan].who_name, 
		   	 can_see_channel(d->character, ch) ? ch->gtitle ? ch->gtitle : player_rank(ch) : "concealed",
		   	 PERS_OLD(ch, d->character), colorcontinue('8', cemotebuf));
	      send_to_char(buf, d->character);
	    }	    
	}
    }
  }
  
  return;
}

void do_guildtalkh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  /* int first=TRUE; */
  int entry=FALSE;

  if (!is_clan(ch)) {
    send_to_char("You aren't in a guild.\n\r",ch);
    return;
  }

  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (guild_history[ch->clan][i].player_name){
	   entry=TRUE;
	   if ((get_trust(ch) <  guild_history[ch->clan][i].wizinvis_level) ||
		  ( (guild_history[ch->clan][i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(guild_history[ch->clan][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: Someone '{c%s{x'\n\r",
				 clan_table[ch->clan].who_name,
				 sec2strtime(guild_history[ch->clan][i].when),
				 guild_history[ch->clan][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&guild_history[ch->clan][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{8Someone {8%s{x'\n\r",
				 clan_table[ch->clan].who_name,
				 sec2strtime(guild_history[ch->clan][i].when),
				 cemotebuf);
		}
	   }
	   else {
		if (!IS_EMOTE(guild_history[ch->clan][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: %s '{8%s{x'\n\r",
				 clan_table[ch->clan].who_name,
				 sec2strtime(guild_history[ch->clan][i].when),
				 guild_history[ch->clan][i].player_name, 
				 guild_history[ch->clan][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&guild_history[ch->clan][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{8%s {8%s{x'\n\r",
				 clan_table[ch->clan].who_name,
				 sec2strtime(guild_history[ch->clan][i].when),
				 guild_history[ch->clan][i].player_name, 
				 cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

/* OGuild channels */
void do_oguildtalk( CHAR_DATA *ch, char *argument )
{
  char category[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH+10];
  char cemotebuf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  
  if (!is_oguild(ch)) {
    send_to_char("You aren't in a oguild.\n\r",ch);
    return;
  }
  
  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm2,COMM2_NOOGUILD)) {
	 send_to_char("OGuild channel is now ON\n\r",ch);
	 REMOVE_BIT(ch->comm2,COMM2_NOOGUILD);
    }
    else {
	 send_to_char("OGuild channel is now OFF\n\r",ch);
	 SET_BIT(ch->comm2,COMM2_NOOGUILD);
    }
    return;
  }
  
  if (IS_SET(ch->comm,COMM_QUIET)) {
    send_to_char("You must turn off quiet mode first.\n\r",ch);
    return;
  }
  
  if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_GAGGED)) {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }
  
  if (IS_SET(ch->comm2,COMM2_NOOGUILD)) {
    send_to_char("Turn OGuild channel on to use it.\n\r",ch);
    return;
  }
  
  if (ch->oguild_mute) {
    send_to_char("The guild leaders have revoked your oguild channel priviliges.\n\r", ch);
    return;
  }
  
  // Make sure no cursing etc. on public channels
  censor_speech(ch, argument);
  
  if (!IS_EMOTE(argument))
    sprintf(buf, "{w[%s{w](%s) %s '{N%s{x'\n\r", clan_table[ch->oguild].who_name,
		  ch->oguild_title ? ch->oguild_title : player_oguild_rank(ch), COLORNAME(ch), colorcontinue('8', argument));
  else {   
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf( buf, "{w[%s](%s)  '{N%s {N%s{x'\n\r", clan_table[ch->oguild].who_name, 
		   ch->oguild_title ? ch->oguild_title : player_oguild_rank(ch), COLORNAME(ch), colorcontinue('8', cemotebuf));
  }

  sprintf(buf2,"Wiznet: %s",buf);
  wiznet(buf2,ch,NULL,WIZ_GUILD,0,get_trust(ch));
  do_addchannelhistory(ch, oguild_history[ch->oguild],argument, NULL);
  do_addchannelhistory(ch, guild_history[ch->oguild],argument, NULL);
  
   sprintf(category,"guild/%s",clan_table[ch->oguild].name);
   int i = 0;
   for (i = 0; i < strlen(category); i++)
   {
	if (category[i] == ' ' || category[i] == '\'')
           category[i] = '_';
   } 
  log_comm_string(category,buf);

  for ( d = descriptor_list; d != NULL; d = d->next ) {
    if ( d->connected == CON_PLAYING &&
	    (is_same_oguild(ch,d->character) || ch->oguild == d->character->clan) &&	    
	    !IS_SET(d->character->comm2,COMM2_NOOGUILD) &&
	    !IS_SET(d->character->comm,COMM_NOCLAN) &&
	    !IS_SET(d->character->comm,COMM_QUIET) ) {
	 if (d->character->clan == ch->oguild) {
	    if (!IS_EMOTE(argument)) {	 
	      sprintf(buf, "{w[%s](%s)  %s '{8%s{x'\n\r", clan_table[ch->oguild].who_name, 
		   	 can_see_channel(d->character, ch) ? ch->oguild_title ? ch->oguild_title : player_oguild_rank(ch) : "concealed",
		   	 PERS_OLD(ch, d->character), colorcontinue('8', argument));
	      send_to_char(buf, d->character);
	    }
	    else {
	      sprintf(buf, "{w[%s](%s)  '{8%s {8%s{x'\n\r", clan_table[ch->oguild].who_name, 
		   	 can_see_channel(d->character, ch) ? ch->oguild_title ? ch->oguild_title : player_oguild_rank(ch) : "concealed",
		   	 PERS_OLD(ch, d->character), colorcontinue('8', cemotebuf));
	      send_to_char(buf, d->character);
	    }	 	
	}
	else {	 
	    if (!IS_EMOTE(argument)) {	 
	      sprintf(buf, "{w[%s](%s)  %s '{N%s{x'\n\r", clan_table[ch->oguild].who_name, 
		   	 can_see_channel(d->character, ch) ? ch->oguild_title ? ch->oguild_title : player_oguild_rank(ch) : "concealed",
		   	 PERS_OLD(ch, d->character), colorcontinue('N', argument));
	      send_to_char(buf, d->character);
	    }
	    else {
	      sprintf(buf, "{w[%s](%s)  '{N%s {N%s{x'\n\r", clan_table[ch->oguild].who_name, 
		   	 can_see_channel(d->character, ch) ? ch->oguild_title ? ch->oguild_title : player_oguild_rank(ch) : "concealed",
		   	 PERS_OLD(ch, d->character), colorcontinue('N', cemotebuf));
	      send_to_char(buf, d->character);
	    }
	}
    }
  }
  
  return;
}

void do_oguildtalkh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  /* int first=TRUE; */
  int entry=FALSE;
  
  if (!is_oguild(ch)) {
    send_to_char("You aren't in a oguild.\n\r",ch);
    return;
  }
  
  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (oguild_history[ch->oguild][i].player_name){
	   entry=TRUE;
	   if ((get_trust(ch) <  oguild_history[ch->oguild][i].wizinvis_level) ||
		  ( (oguild_history[ch->oguild][i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(oguild_history[ch->oguild][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: Someone '{c%s{x'\n\r",
				 clan_table[ch->oguild].who_name,
				 sec2strtime(oguild_history[ch->oguild][i].when),
				 oguild_history[ch->oguild][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&oguild_history[ch->oguild][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{NSomeone {N%s{x'\n\r",
				 clan_table[ch->oguild].who_name,
				 sec2strtime(oguild_history[ch->oguild][i].when),
				 cemotebuf);
		}
	   }
	   else {
		if (!IS_EMOTE(oguild_history[ch->oguild][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: %s '{N%s{x'\n\r",
				 clan_table[ch->oguild].who_name,
				 sec2strtime(oguild_history[ch->oguild][i].when),
				 oguild_history[ch->oguild][i].player_name, 
				 oguild_history[ch->oguild][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&oguild_history[ch->oguild][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{N%s {N%s{x'\n\r",
				 clan_table[ch->oguild].who_name,
				 sec2strtime(oguild_history[ch->oguild][i].when),
				 oguild_history[ch->oguild][i].player_name, 
				 cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

/* Subguild channels */
void do_sguildtalk( CHAR_DATA *ch, char *argument )
{
  char category[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH+10];
  char cemotebuf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  
  if (!is_sguild(ch)) {
    send_to_char("You aren't in a SGuild.\n\r",ch);
    return;
  }

  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm, COMM_NOSGUILD)) {
	 send_to_char("SGuild channel is now ON\n\r",ch);
	 REMOVE_BIT(ch->comm,COMM_NOSGUILD);
    }
    else {
	 send_to_char("SGuild channel is now OFF\n\r",ch);
	 SET_BIT(ch->comm,COMM_NOSGUILD);
    }
    return;
  }

  if (IS_SET(ch->comm,COMM_QUIET)) {
     send_to_char("You must turn off quiet mode first.\n\r",ch);
     return;
  }
  
  if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_GAGGED)) {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }
  
  REMOVE_BIT(ch->comm,COMM_NOSGUILD);
  
  // Make sure no cursing etc. on public channels
  censor_speech(ch, argument);
  
  if (!IS_EMOTE(argument))
    sprintf(buf, "{w[%s{w](%s) %s '{S%s{x'\n\r", sguild_table[ch->sguild].who_name,
		  ch->sguild_title ? ch->sguild_title : player_sguild_rank(ch), COLORNAME(ch), colorcontinue('S', argument));
  else {   
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf( buf, "{w[%s](%s)  '{S%s {S%s{x'\n\r", sguild_table[ch->sguild].who_name, 
		   ch->sguild_title ? ch->sguild_title : player_sguild_rank(ch), COLORNAME(ch), colorcontinue('S', cemotebuf));
  }

  sprintf(buf2,"Wiznet: %s",buf);
  wiznet(buf2,ch,NULL,WIZ_SUBGUILD,0,0);
  do_addchannelhistory(ch, sguild_history[ch->sguild],argument, NULL);

   sprintf(category,"guild/%s",sguild_table[ch->sguild].name);
   int i = 0;
   for (i = 0; i < strlen(category); i++)
   {
	if (category[i] == ' ' || category[i] == '\'')
           category[i] = '_';
   } 
  log_comm_string(category,buf);
  for ( d = descriptor_list; d != NULL; d = d->next ) {
    if ( d->connected == CON_PLAYING &&
	    is_same_sguild(ch,d->character) &&
	    !IS_SET(d->character->comm,COMM_NOSGUILD) &&
	    !IS_SET(d->character->comm,COMM_QUIET) ) {
	 if (!IS_EMOTE(argument)) {
	   sprintf(buf, "{w[%s](%s)  %s '{S%s{x'\n\r", sguild_table[ch->sguild].who_name, 
			 can_see_channel(d->character, ch) ? ch->sguild_title ? ch->sguild_title : player_sguild_rank(ch) : "concealed",
			 PERS_OLD(ch, d->character), colorcontinue('S', argument));
	   send_to_char(buf, d->character);
	 }
	 else {
	   sprintf(buf, "{w[%s](%s)  '{S%s {S%s{x'\n\r", sguild_table[ch->sguild].who_name, 
			 can_see_channel(d->character, ch) ? ch->sguild_title ? ch->sguild_title : player_sguild_rank(ch) : "concealed",
			 PERS_OLD(ch, d->character), colorcontinue('S', cemotebuf));
	   send_to_char(buf, d->character);
	 }
    }
  }
  
  return;
}

void do_sguildtalkh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  /* int first=TRUE; */
  int entry=FALSE;

  if (!is_sguild(ch)) {
    send_to_char("You aren't in a SGuild.\n\r",ch);
    return;
  }
  
  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (sguild_history[ch->sguild][i].player_name){
	   entry=TRUE;
	   if ((get_trust(ch) <  sguild_history[ch->sguild][i].wizinvis_level) ||
		  ( (sguild_history[ch->sguild][i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(sguild_history[ch->sguild][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: Someone '{c%s{x'\n\r",
				 sguild_table[ch->sguild].who_name,
				 sec2strtime(sguild_history[ch->sguild][i].when),
				 sguild_history[ch->sguild][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&sguild_history[ch->sguild][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{SSomeone {S%s{x'\n\r",
				 sguild_table[ch->sguild].who_name,
				 sec2strtime(sguild_history[ch->clan][i].when),
				 cemotebuf);
		}
	   }
	   else {
		if (!IS_EMOTE(sguild_history[ch->sguild][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: %s '{S%s{x'\n\r",
				 sguild_table[ch->sguild].who_name,
				 sec2strtime(sguild_history[ch->sguild][i].when),
				 sguild_history[ch->sguild][i].player_name, 
				 sguild_history[ch->sguild][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&sguild_history[ch->sguild][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{S%s {S%s{x'\n\r",
				 sguild_table[ch->sguild].who_name,
				 sec2strtime(sguild_history[ch->sguild][i].when),
				 sguild_history[ch->sguild][i].player_name, 
				 cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

/* Subguild channels */
void do_ssguildtalk( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char category[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH+10];
  char cemotebuf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  
  if (!is_ssguild(ch)) {
    send_to_char("You aren't in a SSGuild.\n\r",ch);
    return;
  }

  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm, COMM_NOSSGUILD)) {
	 send_to_char("SSGuild channel is now ON\n\r",ch);
	 REMOVE_BIT(ch->comm,COMM_NOSSGUILD);
    }
    else {
	 send_to_char("SSGuild channel is now OFF\n\r",ch);
	 SET_BIT(ch->comm,COMM_NOSSGUILD);
    }
    return;
  }

  if (IS_SET(ch->comm,COMM_QUIET)) {
     send_to_char("You must turn off quiet mode first.\n\r",ch);
     return;
  }
  
  if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_GAGGED)) {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }
  
  REMOVE_BIT(ch->comm,COMM_NOSSGUILD);
  
  // Make sure no cursing etc. on public channels
  censor_speech(ch, argument);
  
  if (!IS_EMOTE(argument))
    sprintf(buf, "{w[%s{w](%s) %s '{U%s{x'\n\r", ssguild_table[ch->ssguild].who_name,
		  ch->ssguild_title ? ch->ssguild_title : player_ssguild_rank(ch), COLORNAME(ch), colorcontinue('U', argument));
  else {   
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf( buf, "{w[%s](%s)  '{U%s {U%s{x'\n\r", ssguild_table[ch->ssguild].who_name, 
		   ch->ssguild_title ? ch->ssguild_title : player_ssguild_rank(ch), COLORNAME(ch), colorcontinue('U', cemotebuf));
  }
  sprintf(buf2,"Wiznet: %s",buf);
  wiznet(buf2,ch,NULL,WIZ_SUBSUBGUILD,0,0);
  do_addchannelhistory(ch, ssguild_history[ch->ssguild],argument, NULL);
   sprintf(category,"guild/%s",ssguild_table[ch->ssguild].name);
   int i = 0;
   for (i = 0; i < strlen(category); i++)
   {
	if (category[i] == ' ' || category[i] == '\'')
           category[i] = '_';
   } 
  log_comm_string(category,buf);

  for ( d = descriptor_list; d != NULL; d = d->next ) {
    if ( d->connected == CON_PLAYING &&
	    is_same_ssguild(ch,d->character) &&
	    !IS_SET(d->character->comm,COMM_NOSSGUILD) &&
	    !IS_SET(d->character->comm,COMM_QUIET) ) {
	 if (!IS_EMOTE(argument)) {
	   sprintf(buf, "{w[%s](%s)  %s '{U%s{x'\n\r", ssguild_table[ch->ssguild].who_name, 
			 can_see_channel(d->character, ch) ? ch->ssguild_title ? ch->ssguild_title : player_ssguild_rank(ch) : "concealed",
			 PERS_OLD(ch, d->character), colorcontinue('U', argument));
	   send_to_char(buf, d->character);
	 }
	 else {
	   sprintf(buf, "{w[%s](%s)  '{U%s {U%s{x'\n\r", ssguild_table[ch->ssguild].who_name, 
			 can_see_channel(d->character, ch) ? ch->ssguild_title ? ch->ssguild_title : player_ssguild_rank(ch) : "concealed",
			 PERS_OLD(ch, d->character), colorcontinue('U', cemotebuf));
	   send_to_char(buf, d->character);
	 }
    }
  }
  
  return;
}

void do_ssguildtalkh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  /* int first=TRUE; */
  int entry=FALSE;

  if (!is_ssguild(ch)) {
    send_to_char("You aren't in a SSGuild.\n\r",ch);
    return;
  }
  
  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (ssguild_history[ch->ssguild][i].player_name){
	   entry=TRUE;
	   if ((get_trust(ch) <  ssguild_history[ch->ssguild][i].wizinvis_level) ||
		  ( (ssguild_history[ch->ssguild][i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(ssguild_history[ch->ssguild][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: Someone '{U%s{x'\n\r",
				 ssguild_table[ch->ssguild].who_name,
				 sec2strtime(ssguild_history[ch->ssguild][i].when),
				 ssguild_history[ch->ssguild][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&ssguild_history[ch->ssguild][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{USomeone {U%s{x'\n\r",
				 ssguild_table[ch->ssguild].who_name,
				 sec2strtime(ssguild_history[ch->ssguild][i].when),
				 cemotebuf);
		}
	   }
	   else {
		if (!IS_EMOTE(ssguild_history[ch->ssguild][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: %s '{U%s{x'\n\r",
				 ssguild_table[ch->ssguild].who_name,
				 sec2strtime(ssguild_history[ch->ssguild][i].when),
				 ssguild_history[ch->ssguild][i].player_name, 
				 ssguild_history[ch->ssguild][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&ssguild_history[ch->ssguild][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{U%s {U%s{x'\n\r",
				 ssguild_table[ch->ssguild].who_name,
				 sec2strtime(ssguild_history[ch->ssguild][i].when),
				 ssguild_history[ch->ssguild][i].player_name, 
				 cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

void do_foretelling(  CHAR_DATA *ch, char *argument )
{
  static char * const his_her [] = { "its", "his", "her" };

  DESCRIPTOR_DATA *d;
  char buf[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  
  /* Only immortals can use this channel */
  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (argument[0] == '\0' ) {
    if (IS_SET(ch->comm,COMM_FORETELLING)) {
	 send_to_char("Foretelling channel is now ON.\n\r", ch);
	 REMOVE_BIT(ch->comm,COMM_FORETELLING);
    }
    else {
	 send_to_char("Foretelling channel is now OFF.\n\r", ch);
	 SET_BIT(ch->comm,COMM_FORETELLING);
    }
  }
  else {
    if (IS_SET(ch->comm,COMM_QUIET)) {
      send_to_char("You must turn off quiet mode first.\n\r",ch);
      return;
    }
    
    REMOVE_BIT(ch->comm,COMM_FORETELLING);

    if (!IS_EMOTE(argument))
	 act_old("{W[{RForetelling{W]{W:{x $n {x'{y$t{x'",ch,colorcontinue('y',argument),NULL,TO_CHAR,POS_DEAD);
    else {
	 strcpy(cemotebuf,&argument[1]);
	 cemotebuf[strlen(cemotebuf)-1] = '\0';
	 act_old("{W[{RForetelling{W]{x({YP{x){W:{x '$n {y$t{x'",ch,colorcontinue('y',cemotebuf),NULL,TO_CHAR,POS_DEAD);
    }
    
    for ( d = descriptor_list; d != NULL; d = d->next ) {
	 if( d->connected == CON_PLAYING && 
		IS_IMMORTAL(d->character) && 
		!IS_SET(d->character->comm,COMM_FORETELLING) ) {
	   if (!IS_EMOTE(argument))
		act_old("{W[{RForetelling{W]{W:{x $n '{y$t{x'",ch,colorcontinue('y',argument),d->character,TO_VICT,POS_DEAD);
	   else {
		act_old("{W[{RForetelling{W]{x({YP{x){W:{x '$n {y$t{x'",ch,colorcontinue('y',cemotebuf), d->character,TO_VICT,POS_DEAD);
	   }
	 }
	 if ( d->connected == CON_PLAYING 
		 && d->character->position != POS_SLEEPING
		 && !IS_SET(d->character->comm, COMM_NOEMOTE)
		 && IS_SET(d->character->talents, TALENT_FORETELLING)
		 && !IS_IMMORTAL(d->character)
		 && IS_EMOTE(argument)) {
	   wiznet("$N has received the foretelling.",d->character,NULL,WIZ_ON,0,0);
	   send_to_char("{WYou feel a sudden void envolve around you.{x\n\r", d->character);
	   sprintf(buf, "fingers tighten into a fist and %s eyes open wide"
			 ", '{7This I Foretell and swear under the Light that I can say no clearer. %s{x'.",
			 his_her[URANGE(0, d->character->sex, 2)], cemotebuf);
	   do_function (d->character, &do_emote, buf);
	   /* send_to_char("Your body trembles and you pant for a quick moment to gain control of your self.\n\r", d->character); */
	 }
    }
  }
}



void do_immtalk( CHAR_DATA *ch, char *argument )
{
  DESCRIPTOR_DATA *d;
  char cemotebuf[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  
  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm,COMM_NOWIZ)) {
	 send_to_char("Immortal channel is now ON\n\r",ch);
	 REMOVE_BIT(ch->comm,COMM_NOWIZ);
    }
    else {
	 send_to_char("Immortal channel is now OFF\n\r",ch);
	 SET_BIT(ch->comm,COMM_NOWIZ);
    } 
    return;
  }
  
  REMOVE_BIT(ch->comm,COMM_NOWIZ);
    
  if (!IS_EMOTE(argument))
  {
    act_old("{w[{iImmortal{w]{W:{x $n {x'{i$t{x'",ch,colorcontinue('i',argument),NULL,TO_CHAR,POS_DEAD);
    sprintf(buffer,"Immortal: {W%s '%s{x'",ch->name,colorcontinue('W', argument));
  }
  else {   
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    act_old("{w[{iImmortal{w]{W:{x '$n {i$t{x'",ch,colorcontinue('i',cemotebuf),NULL,TO_CHAR,POS_DEAD);
    sprintf(buffer,"Immortal: {W%s '%s{x'",ch->name,colorcontinue('W', cemotebuf));
  }
    
  log_comm_string("immortal",buffer);
  do_addchannelhistory(ch,imm_history,argument, NULL);
  for ( d = descriptor_list; d != NULL; d = d->next ) {
    if ( d->connected == CON_PLAYING && 
	    IS_HERO(d->character) && 
	    !IS_SET(d->character->comm,COMM_NOWIZ) ){
	 if (!IS_EMOTE(argument))
	   act_old("{w[{iImmortal{w]{W:{x $n '{i$t{x'",ch,colorcontinue('c',argument),d->character,TO_VICT,POS_DEAD);
	 else {
	   act_old("{w[{iImmortal{w]{W:{x '$n {i$t{x'",ch,colorcontinue('c',cemotebuf),d->character,TO_VICT,POS_DEAD);
	 }
    }
  }
  return;
}

void do_pray (CHAR_DATA *ch, char *argument )
{
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];
  char mort[MAX_INPUT_LENGTH];
  
  if (IS_IMMORTAL(ch)) {    
    argument = one_argument( argument, mort );

    if ( mort[0] == '\0' ) {
      send_to_char("Syntax: Pray <Mortal> <Answer string>\n\r", ch);
      return;
    }
    
    if (( victim = get_char_world( ch, mort )) == NULL ) {
      send_to_char( "They are not here.\n\r", ch );
      return;
    }

    if (IS_IMMORTAL(victim)) {
      send_to_char("Please direct the pray to a mortal\n\r", ch);
      return;
    }
    
    if ( argument[0] == '\0' ) {
      send_to_char( "Please included the answer string to your pray?\n\r", ch );
      return;
    }

    /* Add immortal pray to channel history */
    sprintf(buf, "{x({Y->{x %s){W %s", !IS_NPC(victim) ? victim->name : "mob", argument);
    do_addchannelhistory(ch,pray_history,buf, NULL);
    
    buf[0] = '\0';
    sprintf (buf, "{w[{WPray{w]{W:{x A voice from the heavens booms inside your head{W: %s{x\n\r",
	     argument);
    /* act(buf, victim, NULL, argument, TO_CHAR);*/
    send_to_char(buf, victim);

    buf[0] = '\0';
    sprintf (buf, "{w[{WPray{w]{W:{x %s reply %s's pray{W: %s{x\n\r",
		   COLORNAME(ch), COLORNAME(victim), argument);

    for ( d = descriptor_list; d != NULL; d = d->next ) {
      if ( d->connected == CON_PLAYING && 
	   IS_IMMORTAL(d->character)) {
	/* act_old(buf,ch,argument,d->character,TO_VICT,POS_SLEEPING);*/
	send_to_char(buf, d->character);
      }
    }
    return;    
  }
  else {
    buf[0] = '\0';

    if ( argument[0] == '\0' ) {
      send_to_char( "What would you like to pray to the Immortals ?\n\r", ch );
      return;
    }

    sprintf(buf, "{w[{WPray{w]{W:{x $n{W: $t{x");
    act_old(buf,ch,argument,NULL,TO_CHAR,POS_DEAD);
    
    do_addchannelhistory(ch,pray_history,argument, NULL);
    for ( d = descriptor_list; d != NULL; d = d->next ) {
      if ( d->connected == CON_PLAYING && 
	   IS_IMMORTAL(d->character)) {
	act_new(buf,ch,argument,d->character,TO_VICT,POS_DEAD);
      }
    }
  }
  return;
}

void do_voice( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];

  argument = one_argument( argument, arg );
  
  // Only for PCs
  if (IS_NPC(ch))
    return;

  if (IS_NULLSTR(arg)) {
    send_to_char("Syntax: voice say <voice string>           / [reset]\n\r", ch);
    send_to_char("Syntax: voice ask <voice string>           / [reset]\n\r", ch);
    send_to_char("Syntax: voice exclaim <voice string>       / [reset]\n\r", ch);
    send_to_char("Syntax: voice battlecry <battlecry string> / [reset]\n\r", ch);
    send_to_char("Syntax: voice show\n\n\r", ch);
    send_to_char("See '{Whelp pc_voice{x' for more help.\n\r", ch);
    return;
  }
  
  // Say voice
  if (!str_cmp(arg, "say")) {
    if (IS_NULLSTR(argument)) {
	 sprintf(buf, "Your current say voice is set to: %s\n\r", IS_NULLSTR(ch->pcdata->say_voice) ? "default" : ch->pcdata->say_voice);
	 send_to_char(buf, ch);
	 return;
    }
    
    // Set it
    if (!IS_NULLSTR(argument)) {
	 if (!str_cmp(argument, "reset")) {
	   free_string( ch->pcdata->say_voice );
	   ch->pcdata->say_voice = NULL;
	   send_to_char("Say voice reset to default.\n\r", ch);
	   return;
	 }
	 else if (strstr(argument, "$n") == NULL) { // ||
//			strstr(argument, "says") == NULL) {
	   send_to_char("Required keywords '$n' or 'says' are missing.\n\r", ch);
	   return;
	 }
	 else if (colorstrlen(argument) < 5 ||
			colorstrlen(argument) > 80) {
	   send_to_char("Voice string must be between 5 and 80 characters long.\n\r", ch);
	   return;
	 }
	 else {	   
	   free_string( ch->pcdata->say_voice );
	   ch->pcdata->say_voice = str_dup( argument );
	   sprintf(buf, "You set your say voice to: %s\n\r", ch->pcdata->say_voice);
	   send_to_char(buf, ch);
	   return;
	 }
    }
  }

  // Ask voice
  else if (!str_cmp(arg, "ask")) {
    if (IS_NULLSTR(argument)) {
	 sprintf(buf, "Your current ask voice is set to: %s\n\r", IS_NULLSTR(ch->pcdata->ask_voice) ? "default" : ch->pcdata->ask_voice);
	 send_to_char(buf, ch);
	 return;
    }

    // Set it
    if (!IS_NULLSTR(argument)) {
	 if (!str_cmp(argument, "reset")) {
	   free_string( ch->pcdata->ask_voice );
	   ch->pcdata->ask_voice = NULL;
	   send_to_char("Ask voice reset to default.\n\r", ch);
	   return;
	 }
	 else if (strstr(argument, "$n") == NULL ) { // ||
//			strstr(argument, "asks") == NULL) {
	   send_to_char("Required keywords '$n' or 'asks' are missing.\n\r", ch);
	   return;
	 }
	 else if (strlen(argument) < 5 ||
			strlen(argument) > 80) {
	   send_to_char("Voice string must be between 5 and 80 characters long.\n\r", ch);
	   return;
	 }
	 else {
	   free_string( ch->pcdata->ask_voice );
	   ch->pcdata->ask_voice = str_dup( argument );
	   sprintf(buf, "You set your ask voice to: %s\n\r", ch->pcdata->ask_voice);
	   send_to_char(buf, ch);
	   return;
	 }
    }    
  }
  
  // Exclaim voice
  else if (!str_cmp(arg, "exclaim")) {
    if (IS_NULLSTR(argument)) {
	 sprintf(buf, "Your current exclaim voice is set to: %s\n\r", IS_NULLSTR(ch->pcdata->exclaim_voice) ? "default" : ch->pcdata->exclaim_voice);
	 send_to_char(buf, ch);
	 return;
    }

    // Set it
    if (!IS_NULLSTR(argument)) {
	 if (!str_cmp(argument, "reset")) {
	   free_string( ch->pcdata->exclaim_voice );
	   ch->pcdata->exclaim_voice = NULL;
	   send_to_char("Exclaim voice reset to default.\n\r", ch);
	   return;
	 }
	 else if (strstr(argument, "$n") == NULL ) { //||
//		strstr(argument, "exclaims") == NULL) {
	   send_to_char("Required keywords '$n' or 'exclaims' are missing.\n\r", ch);
	   return;
	 }
	 else if (strlen(argument) < 5 ||
			strlen(argument) > 80) {
	   send_to_char("Voice string must be between 5 and 80 characters long.\n\r", ch);
	   return;
	 }
	 else {
	   free_string( ch->pcdata->exclaim_voice );
	   ch->pcdata->exclaim_voice = str_dup( argument );
	   sprintf(buf, "You set your exclaim voice to: %s\n\r", ch->pcdata->exclaim_voice);
	   send_to_char(buf, ch);
	   return;
	 }
    }
  }

  else if (!str_cmp(arg, "battlecry")) {
    if (IS_NULLSTR(argument)) {
	 sprintf(buf, "Your current battlecry is: %s\n\r", IS_NULLSTR(ch->pcdata->battlecry_voice) ? "default" : ch->pcdata->battlecry_voice);
	 send_to_char(buf, ch);
	 return;
    }

    // Set it
    if (!IS_NULLSTR(argument)) {
	 if (!str_cmp(argument, "reset")) {
	   free_string( ch->pcdata->battlecry_voice );
	   ch->pcdata->battlecry_voice = NULL;
	   send_to_char("Battlecry reset to default.\n\r", ch);
	   return;
	 }
	 else if (strstr(argument, "$n") == NULL ) {
	   send_to_char("Required keywords '$n' is missing.\n\r", ch);
	   return;
	 }
	 else if (strlen(argument) < 5 ||
			strlen(argument) > 80) {
	   send_to_char("Battlecry string must be between 5 and 80 characters long.\n\r", ch);
	   return;
	 }
	 else {
	   free_string( ch->pcdata->battlecry_voice );
	   ch->pcdata->battlecry_voice = str_dup( argument );
	   sprintf(buf, "You set your battlecry to: %s\n\r", ch->pcdata->battlecry_voice);
	   send_to_char(buf, ch);
	   return;
	 }
    }
  }
  

  // Show all and their config
  else if (!str_cmp(arg, "show")) {
    sprintf(buf, "Your current say voice is set to: %s\n\r", IS_NULLSTR(ch->pcdata->say_voice) ? "default" : ch->pcdata->say_voice);
    send_to_char(buf, ch);
    sprintf(buf, "Your current ask voice is set to: %s\n\r", IS_NULLSTR(ch->pcdata->ask_voice) ? "default" : ch->pcdata->ask_voice);
    send_to_char(buf, ch);
    sprintf(buf, "Your current exclaim voice is set to: %s\n\r", IS_NULLSTR(ch->pcdata->exclaim_voice) ? "default" : ch->pcdata->exclaim_voice);    
    send_to_char(buf, ch);
    sprintf(buf, "Your current battlecry is set to: %s\n\r", IS_NULLSTR(ch->pcdata->battlecry_voice) ? "default" : ch->pcdata->battlecry_voice);    
    send_to_char(buf, ch);
    return;
  }


  // If we are here, show syntax again
  send_to_char("Syntax: voice say <voice string>           / [reset]\n\r", ch);
  send_to_char("Syntax: voice ask <voice string>           / [reset]\n\r", ch);
  send_to_char("Syntax: voice exclaim <voice string>       / [reset]\n\r", ch);
  send_to_char("Syntax: voice battlecry <battlecry string> / [reset]\n\r", ch);
  send_to_char("Syntax: voice show\n\n\r", ch);
  send_to_char("See '{Whelp pc_voice{x' for more help.\n\r", ch);
  return;
}

void do_say( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];

  if (IS_WOLFSHAPE(ch)) {
    send_to_char("You are unable to speak while in wolf shape.\n\r", ch);
    return;
  }

  if ( argument[0] == '\0' ) {
    send_to_char( "Say what?\n\r", ch );
    return;
  }

  if (IS_AFFECTED(ch,AFF_GAGGED)) {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }

  if (argument[strlen(argument)-1] == '?') {
    if (!IS_NPC(ch) && !IS_NULLSTR(ch->pcdata->ask_voice)) {
	 sprintf(buf, "%s '{7$T{x'", ch->pcdata->ask_voice);
	 act(buf, ch, NULL, colorcontinue('7',argument), TO_ROOM );
	 act(buf, ch, NULL, colorcontinue('7',argument), TO_CHAR );
    }
    else {
	 act( "{x$n asks '{7$T{x'", ch, NULL, colorcontinue('7',argument), TO_ROOM );
	 act( "{xYou ask '{7$T{x'", ch, NULL, colorcontinue('7',argument), TO_CHAR );  	
    }
  }
  else if (argument[strlen(argument)-1] == '!') {
    if (!IS_NPC(ch) && !IS_NULLSTR(ch->pcdata->exclaim_voice)) {
	 sprintf(buf, "%s '{7$T{x'", ch->pcdata->exclaim_voice);
	 act(buf, ch, NULL, colorcontinue('7',argument), TO_ROOM );
	 act(buf, ch, NULL, colorcontinue('7',argument), TO_CHAR );
    }
    else {
	 act( "{x$n exclaims '{7$T{x'", ch, NULL, colorcontinue('7',argument), TO_ROOM );
	 act( "{xYou exclaim '{7$T{x'", ch, NULL, colorcontinue('7',argument), TO_CHAR );  	
    }
  }
  else {
    if (!IS_NPC(ch) && !IS_NULLSTR(ch->pcdata->say_voice)) {
	 sprintf(buf, "%s '{7$T{x'", ch->pcdata->say_voice);
	 act(buf, ch, NULL, colorcontinue('7',argument), TO_ROOM );
	 act(buf, ch, NULL, colorcontinue('7',argument), TO_CHAR );
    }
    else {
	 act( "{x$n says '{7$T{x'", ch, NULL, colorcontinue('7',argument), TO_ROOM );
	 act( "{xYou say '{7$T{x'", ch, NULL, colorcontinue('7',argument), TO_CHAR );
    }
  }
  
  if ( !IS_NPC(ch) ) {
    CHAR_DATA *mob, *mob_next;
    for ( mob = ch->in_room->people; mob != NULL; mob = mob_next ) {
	 mob_next = mob->next_in_room;
	 if (mob->world != ch->world)
	   continue;
	 if ( IS_NPC(mob) && HAS_TRIGGER( mob, TRIG_SPEECH )
		 &&   mob->position == mob->pIndexData->default_pos )
	   mp_act_trigger( argument, mob, ch, NULL, NULL, TRIG_SPEECH );
    }
  }
  if (IS_RP(ch))
      reward_rp (ch);
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
             handle_wolfkin_insanity(ch);
     }
     else
     {
        handle_mc_insanity(ch);
     }
  }
  return;
}

void do_sayto( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char buffer[MAX_INPUT_LENGTH * 2];

    one_argument( argument, arg );

    if (IS_WOLFSHAPE(ch)) {
	 send_to_char("You are unable to speak while in wolf shape.\n\r", ch);
	 return;
    }

    if ( arg[0] == '\0' ) {
	 send_to_char( "Say What to whom?\n\r", ch );
	 return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg );

    if ( argument[0] == '\0' )
    {
	send_to_char( "Say what to them?\n\r", ch );
	return;
    }

    if (argument[strlen(argument)-1] == '?') {
	 if (!IS_NPC(ch) && !IS_NULLSTR(ch->pcdata->ask_voice)) {
	   sprintf(buffer, "%s of $N, '{7%s{x'", ch->pcdata->ask_voice, colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_NOTVICT );
	   act( buffer,  ch, NULL, victim, TO_CHAR );
	   sprintf(buffer, "%s of you, '{7%s{x'", ch->pcdata->ask_voice, colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_VICT );
	 }
	 else {
	   sprintf(buffer,"$n asks of $N, '{7%s'{x",colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_NOTVICT );
	   sprintf(buffer,"$n asks of you, '{7%s'{x",colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_VICT );
	   sprintf(buffer,"You ask of $N, '{7%s{x",colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_CHAR );   	
	 }
    }
    else if (argument[strlen(argument)-1] == '!') {
	 if (!IS_NPC(ch) && !IS_NULLSTR(ch->pcdata->exclaim_voice)) {
	   sprintf(buffer, "%s to $N, '{7%s{x'", ch->pcdata->exclaim_voice, colorcontinue('7',argument));
	   act(buffer, ch, NULL, victim, TO_NOTVICT );
	   act(buffer, ch, NULL, victim, TO_CHAR );
	   sprintf(buffer, "%s to you, '{7%s{x'", ch->pcdata->exclaim_voice, colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_VICT );
	 }
	 else {
	   sprintf(buffer,"$n exclaims to $N, '{7%s'{x",colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_NOTVICT );
	   sprintf(buffer,"$n exclaims to you, '{7%s'{x",colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_VICT );
	   sprintf(buffer,"You exclaim to $N, '{7%s{x",colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_CHAR );
	 }
    }
    else {
	 if (!IS_NPC(ch) && !IS_NULLSTR(ch->pcdata->say_voice)) {
	   sprintf(buffer, "%s to $N, '{7%s{x'", ch->pcdata->say_voice, colorcontinue('7',argument));
	   act(buffer, ch, NULL, victim, TO_NOTVICT );
	   act(buffer, ch, NULL, victim, TO_CHAR );
	   sprintf(buffer, "%s to you, '{7%s{x'", ch->pcdata->say_voice, colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_VICT );		
	 }
	 else {
	   sprintf(buffer,"$n says to $N, '{7%s'{x",colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_NOTVICT );
	   sprintf(buffer,"$n says to you, '{7%s'{x",colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_VICT );
	   sprintf(buffer,"You say to $N, '{7%s{x",colorcontinue('7',argument));
	   act( buffer,  ch, NULL, victim, TO_CHAR );
	 }
    }

	

    if ( !IS_NPC(ch) )
    {
	CHAR_DATA *mob, *mob_next;
	for ( mob = ch->in_room->people; mob != NULL; mob = mob_next )
	{
	    mob_next = mob->next_in_room;
	    if ( IS_NPC(mob) && HAS_TRIGGER( mob, TRIG_SPEECH )
	    &&   mob->position == mob->pIndexData->default_pos )
		mp_act_trigger( argument, mob, ch, NULL, NULL, TRIG_SPEECH );
	}
    }
    if (IS_RP(ch))
    	reward_rp (ch);
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
             handle_wolfkin_insanity(ch);
     }
     else
     {
        handle_mc_insanity(ch);
     }
  }
    return;
}

void do_osay( CHAR_DATA *ch, char *argument )
{
    if ( argument[0] == '\0' )
    {
	send_to_char( "OSay what?\n\r", ch );
	return;
    }

    act( "{x$n {xsays, speaking OOCly, '{9$T{x'", ch, NULL, colorcontinue('G',argument), TO_ROOM );
    act( "{xYou {xsay, speaking OOCly, '{9$T{x'", ch, NULL, colorcontinue('G',argument), TO_CHAR );

    return;
}

void do_hint( CHAR_DATA *ch, char *argument)
{
  if(IS_NPC(ch))
    return;
  
  if(IS_SET(ch->comm,COMM_NOHINT)) {
    REMOVE_BIT(ch->comm, COMM_NOHINT);
    send_to_char("Hint channel is now ON.\n\r",ch);
    return;
  }
  else {
    SET_BIT(ch->comm, COMM_NOHINT);
    send_to_char("Hint channel is now OFF.\n\r",ch);
    return;
  }
}

bool is_group_tell(char *name_list)
{
  int names=0;
  int i=0;
  
  for (i=0; i < strlen(name_list); i++) {
    if (name_list[i] == ',')
	 names++;
  }
  
  if (names > 0)
    return TRUE;
  else
    return FALSE;

  return FALSE;
}

// New tell for TSW
// Support group tell. 
// Example: 'tell name1,name2,name3 Hello, this is a group tell!'
void do_tell( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char group_buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  int i;
  bool group_tell=FALSE;
  char *ptr;
  bool intro = FALSE;

  if ( IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm,COMM_DEAF)) {
    send_to_char( "Your message didn't get through.\n\r", ch );
    return;
  }
  
  if ( IS_SET(ch->comm, COMM_QUIET) ) {
    send_to_char( "You must turn off quiet mode first.\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->comm,COMM_DEAF)) {
    send_to_char("You must turn off deaf mode first.\n\r",ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_GAGGED)) {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }
  
  argument = one_argument( argument, arg );
  
  if ( IS_NULLSTR(arg) || IS_NULLSTR(argument)) {
    send_to_char( "Tell whom what?\n\r", ch );
    return;
  }
  
  // Is it a group tell?
  group_tell = is_group_tell(arg);

  // Fix group buffer
  if (group_tell) {
    sprintf(group_buf, "%s", arg);
    group_buf[0] = UPPER(group_buf[0]);
    
    for (i=0; i<strlen(group_buf);i++)
	 if (group_buf[i-1] == ',')
	   group_buf[i] = UPPER(group_buf[i]);
  }

  // Get first name
  ptr = strtok(arg, ",");
  
  // As long as more names, keep sending
  while (ptr != NULL) 
  {
    
    // Are they here?
//    if (( victim = get_realname_char_world( ch, ptr )) == NULL
    if (( victim = get_realname_char_world( ch, ptr )) == NULL
	   || IS_NPC(victim)) 
    {
         if (IS_INTRONAME(ch, arg)) 
         {
	    log_string("Have IntroName");
             if (((victim = get_char_world(ch, arg) ) == NULL)
                || IS_NPC(victim)) 
             {
	    	sprintf(buf, "%s isn't here.\n\r", capitalize(ptr));
	    	send_to_char(buf, ch);
	    	ptr = strtok(NULL, ",");
	    	continue;
             }
	     else
             {
    	       intro = TRUE; 
             }
	}
	else {
	    	sprintf(buf, "%s isn't here.\n\r", capitalize(ptr));
		log_string(buf);
	    	send_to_char(buf, ch);
	    	ptr = strtok(NULL, ",");
	    	continue;
	}
    }

    if ((IS_IMMORTAL(victim) || IS_FORSAKEN(victim) || IS_DR(victim)) &&
        (victim->incog_level > 0 )  &&
        ( !IS_DISGUISED(victim)) &&
	( !IS_IMMORTAL(ch)))
    {
	    	sprintf(buf, "%s isn't here.\n\r", capitalize(ptr));
	    	send_to_char(buf, ch);
	    	ptr = strtok(NULL, ",");
	        sprintf(buf,"{x%s tried to tell you '{k%s{x'\n\r",ch->name,argument);
	        buf[0] = UPPER(buf[0]);
		send_to_char(buf,victim);
		victim = NULL;
	    	continue;
    }
    
    // Link dead?
    if ( victim->desc == NULL && !IS_NPC(victim)) {
	 act_old("$N seems to have misplaced $S link...try again later.", ch,NULL,victim,TO_CHAR, POS_DEAD);
	 
	 if (group_tell)
	   sprintf(buf,"{x%s tells the group ( %s ) '{k%s{x'\n\r",ch->name, group_buf, colorcontinue('g',argument));
	 else
	   sprintf(buf,"{x%s tells you '{k%s{x'\n\r",ch->name,colorcontinue('g',argument));
	 buf[0] = UPPER(buf[0]);
	 add_buf(victim->pcdata->buffer,buf);
	 ptr = strtok(NULL, ",");
	 continue;
    }
    
    if ( !(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && victim->position < POS_SLEEPING) {
	 act_old("$E can't hear you.", ch, 0, victim, TO_CHAR, POS_DEAD);
	 ptr = strtok(NULL, ",");
	 continue;
    }
    
    if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF))
	   && !IS_IMMORTAL(ch)) {
	 act_old("$E is not receiving tells.", ch, 0, victim, TO_CHAR, POS_DEAD);
	 ptr = strtok(NULL, ",");
	 continue;
    }
    
    // Ignored?
    if (is_ignoring(victim, ch)) {
	 act_old("$N is not receiving tells right now.", ch,NULL,victim,TO_CHAR,POS_DEAD);
	 ptr = strtok(NULL, ",");
	 continue;
    }
    
    // AFK?
    if (IS_SET(victim->comm,COMM_AFK)) {
	 if (IS_NPC(victim)) {
	   act_old("$E is AFK, and not receiving tells.",ch,NULL,victim,TO_CHAR, POS_DEAD);
	   ptr = strtok(NULL, ",");
	   continue;
	 }
	 
	 act_old("$E is AFK, but your tell will go through when $E returns.",ch,NULL,victim,TO_CHAR, POS_DEAD);
	 if (!IS_NULLSTR(victim->pcdata->afkmsg)) {
	   sprintf(buf, "[{YAFK msg{x]: %s\n\r", victim->pcdata->afkmsg);
	   send_to_char(buf, ch);
	 }
	 
	 if (group_tell)
	   sprintf(buf,"{x%s tells the group ( %s ) '{k%s{x'\n\r",ch->name, group_buf, argument);
	 else
	   sprintf(buf,"{x%s tells you '{k%s{x'\n\r",ch->name,argument);
	 
	 buf[0] = UPPER(buf[0]);
	 add_buf(victim->pcdata->buffer,buf);
    }
    
 
    if (group_tell) {
	 sprintf(buf, "{x$n tells the group ( %s ) '{k$t{x'", group_buf);
	 act_old(buf, ch,colorcontinue('g',argument),victim,TO_VICT,POS_DEAD);
    }
    else
      if (intro) { 
	log_string("Sending tell to intro");
        act_w_intro("{xYou tell $N '{g$t{x'", ch, argument, victim, TO_CHAR, POS_DEAD); 
        sprintf(buf, "{x$n tells you (%s) '{g$t{x'", PERS_NAME(victim, ch));
        act_w_intro(buf,ch,argument,victim,TO_VICT,POS_DEAD);
    	victim->replyWintro = TRUE;
      } 
      else 
      {
    	 act_old("{xYou tell $N '{k$t{x'", ch, colorcontinue('g',argument), victim, TO_CHAR, POS_DEAD);
	 act_old("{x$n tells you '{k$t{x'",ch,colorcontinue('g',argument),victim,TO_VICT,POS_DEAD);
    	victim->replyWintro = FALSE;
      } 
    victim->reply	  = ch;
    
    if ( !IS_NPC(ch) && IS_NPC(victim) && HAS_TRIGGER(victim,TRIG_SPEECH) )
	 mp_act_trigger( argument, victim, ch, NULL, NULL, TRIG_SPEECH );
    
    /*
	* Do Tell History: Zandor 11-23-96
	*/
    if (!IS_NPC(victim)) {
	 for (i=0;i<HISTSIZE-1;i++) {
	   if (victim->pcdata->tell_history[i+1].player_name) {
		if (victim->pcdata->tell_history[1].player_name) {   
		  free_string(victim->pcdata->tell_history[i].player_name);
		  free_string(victim->pcdata->tell_history[i].line_data);
		} 
		victim->pcdata->tell_history[i].player_name = 
		  str_dup(victim->pcdata->tell_history[i+1].player_name);
		victim->pcdata->tell_history[i].line_data = 
		  str_dup(victim->pcdata->tell_history[i+1].line_data);
		victim->pcdata->tell_history[i].wizinvis_level = 
		  victim->pcdata->tell_history[i+1].wizinvis_level;
		victim->pcdata->tell_history[i].invis_flag = 
		  victim->pcdata->tell_history[i+1].invis_flag;
		victim->pcdata->tell_history[i].when =
		  victim->pcdata->tell_history[i+1].when;
		victim->pcdata->tell_history[i].sent_comm =
				  victim->pcdata->tell_history[i+1].sent_comm;
	   };
	 };
	 if (victim->pcdata->tell_history[HISTSIZE-1].player_name) {   
	   free_string(victim->pcdata->tell_history[HISTSIZE-1].player_name);
	   free_string(victim->pcdata->tell_history[HISTSIZE-1].line_data);
	 } 
	 victim->pcdata->tell_history[HISTSIZE-1].line_data = str_dup(argument);
	 victim->pcdata->tell_history[HISTSIZE-1].player_name = 
	   str_dup(COLORNAME(ch));
	 if (IS_AFFECTED(ch,AFF_INVISIBLE)) {
	   victim->pcdata->tell_history[HISTSIZE-1].invis_flag = 1;
	 }
	 else
	   victim->pcdata->tell_history[HISTSIZE-1].invis_flag = 0;
	 
	 if ((!IS_NPC(ch)) && (ch->desc != NULL))
	   victim->pcdata->tell_history[HISTSIZE-1].wizinvis_level = ch->incog_level;
	 
	 victim->pcdata->tell_history[HISTSIZE-1].when = current_time;
	 victim->pcdata->tell_history[HISTSIZE-1].sent_comm = FALSE;
    };

    /*
    * Add tell history for tells YOU send
    * Modified history_data structure in merc.h for this
    * - Caldazar 2009-11-12
    */
        if (!IS_NPC(ch)) {
    	 for (i=0;i<HISTSIZE-1;i++) {
    	   if (ch->pcdata->tell_history[i+1].player_name) {
    		if (ch->pcdata->tell_history[1].player_name) {
    		  free_string(ch->pcdata->tell_history[i].player_name);
    		  free_string(ch->pcdata->tell_history[i].line_data);
    		}
    		ch->pcdata->tell_history[i].player_name =
    		  str_dup(ch->pcdata->tell_history[i+1].player_name);
    		ch->pcdata->tell_history[i].line_data =
    		  str_dup(ch->pcdata->tell_history[i+1].line_data);
    		ch->pcdata->tell_history[i].wizinvis_level =
    				ch->pcdata->tell_history[i+1].wizinvis_level;
    		ch->pcdata->tell_history[i].invis_flag =
    				ch->pcdata->tell_history[i+1].invis_flag;
    		ch->pcdata->tell_history[i].when =
    				ch->pcdata->tell_history[i+1].when;
    		ch->pcdata->tell_history[i].sent_comm = ch->pcdata->tell_history[i+1].sent_comm;
    	   };
    	 };
    	 if (ch->pcdata->tell_history[HISTSIZE-1].player_name) {
    	   free_string(ch->pcdata->tell_history[HISTSIZE-1].player_name);
    	   free_string(ch->pcdata->tell_history[HISTSIZE-1].line_data);
    	 }
    	 ch->pcdata->tell_history[HISTSIZE-1].line_data = str_dup(argument);
    	 ch->pcdata->tell_history[HISTSIZE-1].player_name = str_dup(COLORNAME(victim));
    	 if (IS_AFFECTED(victim,AFF_INVISIBLE)) {
    		 ch->pcdata->tell_history[HISTSIZE-1].invis_flag = 1;
    	 }
    	 else
    		 ch->pcdata->tell_history[HISTSIZE-1].invis_flag = 0;

    	 if ((!IS_NPC(victim)) && (victim->desc != NULL))
    		 ch->pcdata->tell_history[HISTSIZE-1].wizinvis_level = victim->incog_level;

    	 ch->pcdata->tell_history[HISTSIZE-1].when = current_time;
    	 ch->pcdata->tell_history[HISTSIZE-1].sent_comm = TRUE;
        };
        ptr = strtok(NULL, ",");

  }
    if (group_tell) {
         sprintf(buf, "{xYou tell the group ( %s ) '{k$t{x'", group_buf);
         act_w_intro(buf, ch, argument, victim, TO_CHAR, POS_DEAD); 
    }

  return;
}

void reward_rp (CHAR_DATA *ch)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    char rpbuf [MAX_STRING_LENGTH];
    int i = 0;
    int peoplecounter = 0;

    if ((IS_NPC (ch)) || (IS_IMMORTAL(ch)))
	return;
	
	if (!IS_RP(ch))
		return;

    /*  Make sure there is another person in the room so that they aren't rping with themselves */
    vch = ch->in_room->people;
    while (vch)
    {
		vch_next = vch->next_in_room;
		if (vch == ch)
		{
		     vch = vch_next;	
		     continue;
		}
		if (!IS_RP(vch))
		{
		     vch = vch_next;	
		     continue;
		}
	   	peoplecounter++; 
		vch = vch_next;	
    }
    
    if (peoplecounter == 0)
    {
	char buf[256];
        sprintf(buf, "%s may be roleplaying alone",ch->name);
        wiznet(buf, ch, NULL, WIZ_ROLEPLAY, 0, get_trust(ch));
	return;
    }


    //Calculate reward
    for (vch = char_list; vch != NULL; vch = vch_next)
    {
		vch_next = vch->next;
		if ((!IS_NPC (vch)) && (!IS_IMMORTAL(vch)) && (ch != vch) && (IS_RP(vch)))
		{
		    ch->pcdata->rpbonus += 1;
		    save_char_obj (ch, FALSE);
		}
    }
    if (ch->pcdata->rprewardtimer <= current_time)
    {
		if (ch->pcdata->rpbonus >= 5)
		{
		    i = 250;
		    if (ch->pcdata->rpbonus >= 30)
				i += ch->pcdata->rpbonus;
		    if (i > 350)
				i = 350;
		    i *= 3;
  		    if (reward_multiplier > 0)
  		    {
        		    if (current_time <= reward_time)
        		    {
                		    i=i*reward_multiplier;
                		    send_to_char("{WB{RO{WN{RU{WS XP {RR{WE{RW{WA{RR{WD!!!{x\r\n",ch);
        		    }
        		    else {
                		    reward_multiplier = 0;
        		    }
  		    }
		    sprintf (rpbuf, "{CYou have been awarded %d experience for RPing!{x", i);
		    send_to_char (rpbuf, ch);
		    gain_exp (ch, i);
		}
	    ch->pcdata->rprewardtimer = (current_time + 750 + (number_range (0, 600)));
	    char buff[256];
	    sprintf(buff,"RewardRP: %s : RPBonus: %d  : NextReward: %s",ch->name,ch->pcdata->rpbonus,(char *)ctime(&ch->pcdata->rprewardtimer));
	    if (ch->pcdata->rpbonus >= 5) {
	       ch->pcdata->rpbonus = 0;
	    }

            wiznet(buff, ch, NULL, WIZ_ROLEPLAY, 0, get_trust(ch));
    }
    
    return;
}



void do_tell_OLD( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  bool intro;
  int i;
  //  bool intro=FALSE;
  
  if ( IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm,COMM_DEAF)) {
    send_to_char( "Your message didn't get through.\n\r", ch );
    return;
  }
  
  if ( IS_SET(ch->comm, COMM_QUIET) ) {
    send_to_char( "You must turn off quiet mode first.\n\r", ch);
    return;
  }

  if (IS_SET(ch->comm,COMM_DEAF)) {
    send_to_char("You must turn off deaf mode first.\n\r",ch);
    return;
  }

  if (IS_AFFECTED(ch,AFF_GAGGED))
  {
    send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
    return;
  }

  argument = one_argument( argument, arg );
  
  if ( arg[0] == '\0' || argument[0] == '\0' ) {
    send_to_char( "Tell whom what?\n\r", ch );
    return;
  }
  
  /*
   * Can tell to PC's anywhere, but NPC's only in same room.
   * -- Furey
   */
  
  /* Old way when intro was unknown */
  if ( ( victim = get_realname_char_world( ch, arg ) ) == NULL
	  || ( IS_NPC(victim) && victim->in_room != ch->in_room ) ) {
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }
  
   if (IS_INTRONAME(ch, arg)) {
     if ((victim = get_introname_char_world(ch, arg) ) == NULL
 	   || ( IS_NPC(victim) && victim->in_room != ch->in_room ) ) { 
 	 send_to_char( "They aren't here.\n\r", ch ); 
 	 return;
    } 
    intro = TRUE; 
  }
  else { 
    if ((victim = get_realname_char_world(ch, arg) ) == NULL
	   || ( IS_NPC(victim) && victim->in_room != ch->in_room ) ) { 
	 send_to_char( "They aren't here.\n\r", ch ); 
	 return; 
    } 
    intro = FALSE; 
  } 
  
  if ( victim->desc == NULL && !IS_NPC(victim)) {
    /* act("$N seems to have misplaced $S link...try again later.", ch,NULL,victim,TO_CHAR); */
    act_old("$N seems to have misplaced $S link...try again later.", ch,NULL,victim,TO_CHAR, POS_DEAD);
    
/*     if (intro) */
/* 	 sprintf(buf,"{x%s tells you '{g%s{x'\n\r",PERS_NAME(ch,victim),argument); */
/*     else */
	 sprintf(buf,"{x%s tells you '{k%s{x'\n\r",ch->name,colorcontinue('g',argument));

    buf[0] = UPPER(buf[0]);
    add_buf(victim->pcdata->buffer,buf);
    return;
  }

  if ( !(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && victim->position < POS_SLEEPING) { /* && !IS_AWAKE(victim) ) { */
    /* act( "$E can't hear you.", ch, 0, victim, TO_CHAR ); */
    act_old("$E can't hear you.", ch, 0, victim, TO_CHAR, POS_DEAD);
    return;
  }
  
  if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF))
	 && !IS_IMMORTAL(ch)) {
    /* act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR ); */
    act_old("$E is not receiving tells.", ch, 0, victim, TO_CHAR, POS_DEAD);
    return;
  }

  // Ignored?
  if (is_ignoring(victim, ch)) {
    act_old("$N is not receiving tells right now.", ch,NULL,victim,TO_CHAR,POS_DEAD);
    return;
  }
  
  if (IS_SET(victim->comm,COMM_AFK)) {
    if (IS_NPC(victim)) {
	 /* act("$E is AFK, and not receiving tells.",ch,NULL,victim,TO_CHAR); */
	 act_old("$E is AFK, and not receiving tells.",ch,NULL,victim,TO_CHAR, POS_DEAD);
	 return;
    }
    
    /* act("$E is AFK, but your tell will go through when $E returns.",ch,NULL,victim,TO_CHAR); */
    act_old("$E is AFK, but your tell will go through when $E returns.",ch,NULL,victim,TO_CHAR, POS_DEAD);
    if (!IS_NULLSTR(victim->pcdata->afkmsg)) {
	 sprintf(buf, "[{YAFK msg{x]: %s\n\r", victim->pcdata->afkmsg);
	 send_to_char(buf, ch);
    }

/*     if (intro) */
/* 	 sprintf(buf,"{x%s tells you '{g%s{x'\n\r",PERS_NAME(ch,victim),argument); */
/*     else */
    sprintf(buf,"{x%s tells you '{k%s{x'\n\r",ch->name,argument);
    
    buf[0] = UPPER(buf[0]);
    add_buf(victim->pcdata->buffer,buf);
    /* return; */ /* Do this if you don't want tell to go through when afk */
  }
  
/*   if (intro) { */
/*     act_w_intro("{xYou tell $N '{g$t{x'", ch, argument, victim, TO_CHAR, POS_DEAD); */
/*     sprintf(buf, "{x$n tells you (%s) '{g$t{x'", PERS_NAME(victim, ch)); */
/*     act_w_intro(buf,ch,argument,victim,TO_VICT,POS_DEAD); */
/*   } */
/*   else { */
    act_old("{xYou tell $N '{k$t{x'", ch, colorcontinue('g',argument), victim, TO_CHAR, POS_DEAD);
    act_old("{x$n tells you '{k$t{x'",ch,colorcontinue('g',argument),victim,TO_VICT,POS_DEAD);
/*   } */
  victim->reply	= ch;

/*   if (intro) */
/*     victim->replyWintro = TRUE; */
/*   else */
    victim->replyWintro = FALSE;
	 
  
  if ( !IS_NPC(ch) && IS_NPC(victim) && HAS_TRIGGER(victim,TRIG_SPEECH) )
    mp_act_trigger( argument, victim, ch, NULL, NULL, TRIG_SPEECH );
  /*
   * Do Tell History: Zandor 11-23-96
   */
  if (!IS_NPC(victim)) {
    for (i=0;i<HISTSIZE-1;i++) {
	 if (victim->pcdata->tell_history[i+1].player_name) {
	   if (victim->pcdata->tell_history[1].player_name)
		{   free_string(victim->pcdata->tell_history[i].player_name);
		free_string(victim->pcdata->tell_history[i].line_data);
		} 
	   victim->pcdata->tell_history[i].player_name = 
		str_dup(victim->pcdata->tell_history[i+1].player_name);
	   victim->pcdata->tell_history[i].line_data = 
		str_dup(victim->pcdata->tell_history[i+1].line_data);
	   victim->pcdata->tell_history[i].wizinvis_level = 
		victim->pcdata->tell_history[i+1].wizinvis_level;
	   victim->pcdata->tell_history[i].invis_flag = 
		victim->pcdata->tell_history[i+1].invis_flag;
	 };
    };
    if (victim->pcdata->tell_history[HISTSIZE-1].player_name)
      {   free_string(victim->pcdata->tell_history[HISTSIZE-1].player_name);
	 free_string(victim->pcdata->tell_history[HISTSIZE-1].line_data);
      } 
    victim->pcdata->tell_history[HISTSIZE-1].line_data = str_dup(argument);
    victim->pcdata->tell_history[HISTSIZE-1].player_name = 
	 str_dup(COLORNAME(ch));
    if (IS_AFFECTED(ch,AFF_INVISIBLE))
      {
	   victim->pcdata->tell_history[HISTSIZE-1].invis_flag = 1;
      }
    else
	 victim->pcdata->tell_history[HISTSIZE-1].invis_flag = 0;
    
    if ((!IS_NPC(ch)) && (ch->desc != NULL))
	 victim->pcdata->tell_history[HISTSIZE-1].wizinvis_level = ch->incog_level;
  };
  return;
}

void do_beep( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  bool intro=FALSE;
  
  if ( IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm,COMM_DEAF)) {
    send_to_char( "Your message didn't get through.\n\r", ch );
    return;
  }
  
  if ( IS_SET(ch->comm, COMM_QUIET) ) {
    send_to_char( "You must turn off quiet mode first.\n\r", ch);
    return;
  }

  if (IS_SET(ch->comm,COMM_DEAF)) {
    send_to_char("You must turn off deaf mode first.\n\r",ch);
    return;
  }

  argument = one_argument( argument, arg );
  
  if (IS_INTRONAME(ch, arg)) {
    if ((victim = get_introname_char_world(ch, arg) ) == NULL
	   || ( IS_NPC(victim) && victim->in_room != ch->in_room ) ) {
	 send_to_char( "They aren't here.\n\r", ch );
	 return;
    }
    intro = TRUE;
  }
  else {
    if ((victim = get_realname_char_world(ch, arg) ) == NULL
	   || ( IS_NPC(victim) && victim->in_room != ch->in_room ) ) {
	 send_to_char( "They aren't here.\n\r", ch );
	 return;
    }
    intro = FALSE;
  }
  
  if ( victim->desc == NULL && !IS_NPC(victim)) {
    act("$N seems to have misplaced $S link...try again later.",
	   ch,NULL,victim,TO_CHAR);
    
    if (intro)
	 sprintf(buf,"{x%s has beeped you '{x'\n\r",PERS_NAME(ch,victim));
    else
	 sprintf(buf,"{x%s has beeped you '{x'\n\r",victim->name);

    buf[0] = UPPER(buf[0]);
    add_buf(victim->pcdata->buffer,buf);
    return;
  }

  if ( !(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(victim) ) {
    act( "$E can't hear you.", ch, 0, victim, TO_CHAR );
    return;
  }
  
  if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF))
	 && !IS_IMMORTAL(ch)) {
    act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR );
    return;
  }
  
  if (IS_SET(victim->comm,COMM_AFK)) {
    if (IS_NPC(victim)) {
	 act("$E is AFK, and not receiving tells.",ch,NULL,victim,TO_CHAR);
	 return;
    }
    
    act("$E is AFK, but your tell will go through when $E returns.",
	   ch,NULL,victim,TO_CHAR);

    if (intro)
	 sprintf(buf,"{x%s has beeped you '{x'\n\r",PERS_NAME(ch,victim));
    else
	 sprintf(buf,"{x%s has beeped you '{x'\n\r",victim->name);
    
    buf[0] = UPPER(buf[0]);
    add_buf(victim->pcdata->buffer,buf);
    /* return; */ /* Do this if you don't want tell to go through when afk */
  }
  
  if (intro) {
    act_w_intro("{xYou beep $N{x", ch, "", victim, TO_CHAR, POS_DEAD);
    sprintf(buf, "{x%s beeps you {x", PERS_NAME(victim, ch));
    act_w_intro(buf,ch,argument,victim,TO_VICT,POS_DEAD);
  }
  else {
    act_old("{xYou beep $N'{x", ch, "", victim, TO_CHAR, POS_DEAD);
    act_old("{x$n beeps you{x",ch,"",victim,TO_VICT,POS_DEAD);
  }
  victim->reply	= ch;

  if (intro)
    victim->replyWintro = TRUE;
  else
    victim->replyWintro = FALSE;
	 
  
  if ( !IS_NPC(ch) && IS_NPC(victim) && HAS_TRIGGER(victim,TRIG_SPEECH) )
    mp_act_trigger( argument, victim, ch, NULL, NULL, TRIG_SPEECH );
  return;
}


void do_reply( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];
  int i=0;
  
  if ( IS_SET(ch->comm, COMM_NOTELL) ) {
    send_to_char( "Your message didn't get through.\n\r", ch );
    return;
  }
  
  if ( ( victim = ch->reply ) == NULL ) {
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Reply what?\n\r", ch);
    return;
  }

  if ( victim->desc == NULL && !IS_NPC(victim)) {
    act_old("$N seems to have misplaced $S link...try again later.", ch,NULL,victim,TO_CHAR, POS_DEAD);
    sprintf(buf,"{x%s replies, '{l%s{x'\n\r",PERS_NAME(ch,victim),argument);
    buf[0] = UPPER(buf[0]);
    add_buf(victim->pcdata->buffer,buf);
    return;
  }

  // Ignored?
  if (is_ignoring(victim, ch)) {
    act_old("$N is not receiving tells right now.", ch,NULL,victim,TO_CHAR,POS_DEAD);
    return;
  }
  if ( !IS_IMMORTAL(ch) && victim->position < POS_SLEEPING) { /* && !IS_AWAKE(victim) ) { */
    /* act( "$E can't hear you.", ch, 0, victim, TO_CHAR ); */
    act_old( "$E can't hear you.", ch, 0, victim, TO_CHAR, POS_DEAD );
    return;
  }
  
  if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF))
	 &&  !IS_IMMORTAL(ch) && !IS_IMMORTAL(victim)) {
    act_new( "$E is not receiving tells.", ch, 0, victim, TO_CHAR,POS_DEAD);
    return;
  }

  // Ignored?
  if (is_ignoring(victim, ch)) {
    act_old("$N is not receiving tells right now.", ch,NULL,victim,TO_CHAR,POS_DEAD);
    return;
  }
  
  if (!IS_IMMORTAL(victim) && victim->position < POS_SLEEPING) { /* && !IS_AWAKE(ch)) { */
    send_to_char( "In your dreams, or what?\n\r", ch );
    return;
  }
  
  if (IS_SET(victim->comm,COMM_AFK)) {
    if (IS_NPC(victim)) {
	 act_new("$E is AFK, and not receiving tells.", ch,NULL,victim,TO_CHAR,POS_DEAD);
	 return;
    }
 
    act_new("$E is AFK, but your tell will go through when $E returns.", ch,NULL,victim,TO_CHAR,POS_DEAD);
    if (!IS_NULLSTR(victim->pcdata->afkmsg)) {
	 sprintf(buf, "[{YAFK msg{x]: %s\n\r", victim->pcdata->afkmsg);
	 send_to_char(buf, ch);
    }

    sprintf(buf,"{x%s replies, '{l%s{x'\n\r",PERS_NAME(ch,victim),argument);
    buf[0] = UPPER(buf[0]);
    add_buf(victim->pcdata->buffer,buf);
    //return;
  }

  if (ch->replyWintro) {  
    sprintf(buf, "{xYou (%s) reply $N '{g$t{x'", PERS_NAME(ch, victim));
    act_w_intro(buf, ch,argument,victim,TO_CHAR,POS_DEAD);
    act_w_intro("{x$n replies, '{l$t{x'",ch,argument,victim,TO_VICT,POS_DEAD);
  }
  else {
    act_old("{xYou reply to $N '{l$t{x'",ch,argument,victim,TO_CHAR,POS_DEAD);
    act_old("{x$n replies, '{l$t{x'",ch,argument,victim,TO_VICT,POS_DEAD);
  }
  victim->reply	= ch;

  if (!IS_NPC(victim)) {
    for (i=0;i<HISTSIZE-1;i++) {
	 if (victim->pcdata->tell_history[i+1].player_name) {
	   if (victim->pcdata->tell_history[1].player_name) {   
		free_string(victim->pcdata->tell_history[i].player_name);
		free_string(victim->pcdata->tell_history[i].line_data);
	   } 
	   victim->pcdata->tell_history[i].player_name = 
		str_dup(victim->pcdata->tell_history[i+1].player_name);
	   victim->pcdata->tell_history[i].line_data = 
		str_dup(victim->pcdata->tell_history[i+1].line_data);
	   victim->pcdata->tell_history[i].wizinvis_level = 
		victim->pcdata->tell_history[i+1].wizinvis_level;
	   victim->pcdata->tell_history[i].invis_flag = 
		victim->pcdata->tell_history[i+1].invis_flag;
	   victim->pcdata->tell_history[i].when =
		victim->pcdata->tell_history[i+1].when;
           victim->pcdata->tell_history[i].sent_comm = victim->pcdata->tell_history[i+1].sent_comm;
	 };
    };
    if (victim->pcdata->tell_history[HISTSIZE-1].player_name) {   
	 free_string(victim->pcdata->tell_history[HISTSIZE-1].player_name);
	 free_string(victim->pcdata->tell_history[HISTSIZE-1].line_data);
    } 
    victim->pcdata->tell_history[HISTSIZE-1].line_data = str_dup(argument);
    if (ch->replyWintro)
	 victim->pcdata->tell_history[HISTSIZE-1].player_name =
	   PERS_NAME(ch, victim);
    else
	 victim->pcdata->tell_history[HISTSIZE-1].player_name = 
	   str_dup(COLORNAME(ch));
    if (IS_AFFECTED(ch,AFF_INVISIBLE))
      {
	   victim->pcdata->tell_history[HISTSIZE-1].invis_flag = 1;
      }
    else
	 victim->pcdata->tell_history[HISTSIZE-1].invis_flag = 0;
    
    if ((!IS_NPC(ch)) && (ch->desc != NULL))
	 victim->pcdata->tell_history[HISTSIZE-1].wizinvis_level = ch->incog_level;

    victim->pcdata->tell_history[HISTSIZE-1].when = current_time;
    victim->pcdata->tell_history[i].sent_comm = FALSE;
  };
    /*
    * Add tell history for tells YOU send
    * Modified history_data structure in merc.h for this
    * - Caldazar 2009-11-12
    */
        if (!IS_NPC(ch)) {
         for (i=0;i<HISTSIZE-1;i++) {
           if (ch->pcdata->tell_history[i+1].player_name) {
                if (ch->pcdata->tell_history[1].player_name) {
                  free_string(ch->pcdata->tell_history[i].player_name);
                  free_string(ch->pcdata->tell_history[i].line_data);
                }
                ch->pcdata->tell_history[i].player_name =
                  str_dup(ch->pcdata->tell_history[i+1].player_name);
                ch->pcdata->tell_history[i].line_data =
                  str_dup(ch->pcdata->tell_history[i+1].line_data);
                ch->pcdata->tell_history[i].wizinvis_level =
                                ch->pcdata->tell_history[i+1].wizinvis_level;
                ch->pcdata->tell_history[i].invis_flag =
                                ch->pcdata->tell_history[i+1].invis_flag;
                ch->pcdata->tell_history[i].when =
                                ch->pcdata->tell_history[i+1].when;
                ch->pcdata->tell_history[i].sent_comm = ch->pcdata->tell_history[i+1].sent_comm;
           };
         };
         if (ch->pcdata->tell_history[HISTSIZE-1].player_name) {
           free_string(ch->pcdata->tell_history[HISTSIZE-1].player_name);
           free_string(ch->pcdata->tell_history[HISTSIZE-1].line_data);
         }
         ch->pcdata->tell_history[HISTSIZE-1].line_data = str_dup(argument);
         ch->pcdata->tell_history[HISTSIZE-1].player_name = str_dup(COLORNAME(victim));
         if (IS_AFFECTED(victim,AFF_INVISIBLE)) {
                 ch->pcdata->tell_history[HISTSIZE-1].invis_flag = 1;
         }
         else
                 ch->pcdata->tell_history[HISTSIZE-1].invis_flag = 0;

         if ((!IS_NPC(victim)) && (victim->desc != NULL))
                 ch->pcdata->tell_history[HISTSIZE-1].wizinvis_level = victim->incog_level;

         ch->pcdata->tell_history[HISTSIZE-1].when = current_time;
         ch->pcdata->tell_history[HISTSIZE-1].sent_comm = TRUE;
        };

  
  return;
}



void do_yell( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;

    if ( IS_SET(ch->comm, COMM_NOSHOUT) ) {
        send_to_char( "You can't yell.\n\r", ch );
        return;
    }
    
    if ( IS_SET(ch->comm, COMM_QUIET) ) {
       send_to_char( "You must turn off quiet mode first.\n\r", ch);
       return;
    }    
 
    if ( argument[0] == '\0' ) {
	send_to_char( "Yell what?\n\r", ch );
	return;
    }


    act("You yell '{7$t{x!'.",ch,argument,NULL,TO_CHAR);
    for ( d = descriptor_list; d != NULL; d = d->next ) {
	if ( d->connected == CON_PLAYING
	&&   d->character != ch
	&&   d->character->in_room != NULL
	&&   d->character->in_room->area == ch->in_room->area 
        &&   !IS_SET(d->character->comm,COMM_QUIET) )
	{
	    act("$n yells '{7$t{x!'.",ch,argument,d->character,TO_VICT);
	}
    }

    return;
}

char *emote_parse(CHAR_DATA * ch,char *argument )
{
  static char target[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int i=0;
  int flag=FALSE;

  /* Reset target each time before use */
  memset(target, 0x00, sizeof(target));

  for (i=0; i < strlen(argument); i++) {
    if (argument[i] == '"' && flag == FALSE) {
      flag = TRUE;
      sprintf(buf, "%c", argument[i]);
      strcat(target, buf);
      strcat(target, "{7");
      continue;
    }
    else if (argument[i] == '"' && flag == TRUE) {
      flag = FALSE;
      strcat(target, "{x");
      sprintf(buf, "%c", argument[i]);
      strcat(target, buf);
      continue;
    }
    else {
      if (IS_AFFECTED(ch,AFF_GAGGED) && (flag == TRUE))
      {
	      strcat(target,"gr");
      }
      else
      {
         sprintf(buf, "%c", argument[i]);
         strcat(target, buf);
      }
    }
  }
  if (flag == TRUE) {
    strcat(target, "{x");
    sprintf(buf, "%c", '"');
    strcat(target, buf);
  }
  return(target);
}

// Emote as someone else.
void do_semote( CHAR_DATA *ch,  char *argument )
{
  DESCRIPTOR_DATA *d;
  char buf[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  char *ptr= str_dup( argument );

  // Only PCs
  if (IS_NPC(ch))
    return;
  
  if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
    send_to_char( "You can't show your emotions.\n\r", ch );
    return;
  }
  
  argument = one_argument( argument, arg );
  
  if (IS_NULLSTR(arg)) {
    send_to_char("Syntax: semote as <emote as name>\n\r", ch);
    send_to_char("Syntax: semote <emote string>\n\r", ch);
    return;
  }

  if (!str_cmp(arg, "as")) { 

    if (IS_NULLSTR(argument)) {
	 send_to_char("Syntax: semote as <emote as name>\n\r", ch);
	 send_to_char("Syntax: semote <emote string>\n\r", ch);
	 return;
    }

    if (!check_parse_name(colorstrem(argument))) {
	 send_to_char("Illegal semote as name, try another name.\n\r", ch);
	 return; 
    }

    if (IS_NULLSTR(ch->pcdata->semote)) {
	 ch->pcdata->semote = str_dup(argument);
	 sprintf(buf, "Semote as set to <%s>\n\r", ch->pcdata->semote);
	 send_to_char(buf, ch);
	 return;
    }
    else {
	 free_string(ch->pcdata->semote);
	 ch->pcdata->semote = str_dup(argument);
	 sprintf(buf, "Semote as changed to <%s>\n\r", ch->pcdata->semote);
	 send_to_char(buf, ch);
	 return;
    }
  }

  if (IS_NULLSTR(ch->pcdata->semote)) {
    send_to_char("You need to set a emote as name before you can use semote.\n\r", ch);
    send_to_char("Example: semote as Bob\n\r", ch);
    return;
  }

  if (strstr(ptr, "$n") == NULL) {
    send_to_char("Required keyword '$n' is missing in the semote string.\n\r", ch);
    return;
  }
  
  ptr = string_replace(ptr, "$n", ch->pcdata->semote);
  
  for ( d = descriptor_list; d; d = d->next ) {
    if ( d->connected == CON_PLAYING && d->character->in_room == ch->in_room && IS_SAME_WORLD(d->character, ch)) {
	 if (get_trust(d->character) >= get_trust(ch))
	   send_to_char( "[{YSemote{x]: ",d->character);
	 send_to_char( emote_parse(ch, ptr), d->character );
	 send_to_char( "\n\r",   d->character );
    }
  }
  reward_rp (ch); 
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
             handle_wolfkin_insanity(ch);
     }
     else
     {
        handle_mc_insanity(ch);
     }
  }
  free_string(ptr);
  return;
  
}
 
// weather emote for Cloud dancers
void do_wemote(  CHAR_DATA *ch, char *argument )
{
  char buf[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  int sn=0;
  int endurance=0;
  bool area_weather=FALSE;

  // Only PCs
  if (IS_NPC(ch))
    return;
  
  if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
    send_to_char( "You can't show your emotions.\n\r", ch );
    return;
  }
  
  // Has talent?
  if (!IS_SET(ch->talents, TALENT_CLOUDDANCING) && !IS_IMMORTAL(ch)) {
    send_to_char("You don't know how to affect or shift the local weather.\n\r", ch);
    return;
  }
  
  sn = skill_lookup("control weather");
  
  if (ch->pcdata->learned[sn] < 1) {
    send_to_char("You don't even know how to control the weather.\n\r", ch);
    return;
  }

  // Only strong in the weave can do area emotes
  if (ch->pcdata->learned[sn] >= 85)
    area_weather=TRUE;
  
  endurance = skill_table[sn].min_endurance;
  
  if (ch->endurance < endurance) {
    send_to_char("You are too tired to concentrate any more!\n\r", ch);
    return;	
  }
  
  argument = one_argument( argument, arg );

  // Syntax
  if (IS_NULLSTR(arg) || IS_NULLSTR(argument)) {
    send_to_char("Syntax: wemote local <emote string>\n\r", ch);
    send_to_char("Syntax: wemote area  <emote string>\n\r", ch);
    return;
  }
  
  // Need to be outside
  if (!IS_OUTSIDE(ch)) {
    send_to_char("You need to be outside to do this.\n\r", ch);
    return;
  }
  
  // Need to channel for this. It's a weave in emote form.
  if (!IS_AFFECTED(ch, AFF_CHANNELING)) {
    do_function(ch, &do_channel, "");
    if (!IS_AFFECTED(ch ,AFF_CHANNELING))
	 return;
  }
  
  // Chance to fail, chance to success
  if (number_percent() > (ch->pcdata->learned[sn]/2)) {
    send_to_char("You reach into the sky, but fails to control it.\n\r", ch);    
    ch->endurance -= endurance/2;
    return;
  }
  else {
    sprintf(buf, "You reach into the sky by combining flow of %s to control the weather.\n\r", (char *)flow_text(sn, ch));
    send_to_char(buf, ch);
  }
  
  // For local weather emotes, $n is required.
  if (!str_cmp(arg, "local")) {  
    if (strstr(argument, "$n") == NULL) {
	 send_to_char("Required keyword '$n' is missing in the weather emote string.\n\r", ch);
	 send_to_char("example: wemote local Snowflakes swirl about $n, leaving them untouched.\n\r", ch);
	 return;
    }
  
    MOBtrigger = FALSE;
    act( argument, ch, NULL, NULL, TO_ROOM );
    act( argument, ch, NULL, NULL, TO_CHAR );
    reward_rp (ch);
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
             handle_wolfkin_insanity(ch);
     }
     else
     {
        handle_mc_insanity(ch);
     }
  }
    MOBtrigger = TRUE;
    ch->endurance -= endurance;
    WAIT_STATE(ch,skill_table[sn].beats);

    return;
  }
  // For area weather emotes, "weather" keyword is required
  else if (!str_cmp(arg, "area")) {

    if (!area_weather) {
	 send_to_char("You are not strong enough in your talent to affect the weather in such a large scale.\n\r", ch);
	 return;
    }

    if (strstr(argument, "weather") == NULL) {
	 send_to_char("Required keyword 'weather' is missing in the weather emote string.\n\r", ch);
	 send_to_char("example: wemote area The weather suddenly shift and rain drops star to fall from the sky.\n\r", ch);
	 return;
    }
    
    for ( vch = char_list; vch != NULL; vch = vch_next ) {
	 vch_next	= vch->next;
	 if (vch->in_room == NULL)
	   continue;
	 if (ch == vch)
	   continue;
	 if (!IS_SAME_WORLD(vch, ch))
	   continue;
	 if (vch->in_room->area == ch->in_room->area && IS_OUTSIDE(vch) && IS_AWAKE(vch)) {
	   send_to_char(argument, vch);
	   send_to_char("\n\r", vch);	  
	 }
    }
    reward_rp (ch);
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
             handle_wolfkin_insanity(ch);
     }
     else
     {
        handle_mc_insanity(ch);
     }
  }
    ch->endurance -= endurance;
    return;
  }
  
  send_to_char("Syntax: wemote local <emote string>\n\r", ch);
  send_to_char("Syntax: wemote area  <emote string>\n\r", ch);
  return;  
}

void do_emote( CHAR_DATA *ch, char *argument )
{
  if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
    send_to_char( "You can't show your emotions.\n\r", ch );
    return;
  }
  
  if ( argument[0] == '\0' ) {
    send_to_char( "Emote what?\n\r", ch );
    return;
  }
  
  MOBtrigger = FALSE;
  act( "$n $T", ch, NULL, emote_parse(ch,argument), TO_ROOM );
  act( "$n $T", ch, NULL, emote_parse(ch,argument), TO_CHAR );
  reward_rp (ch);
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
	     handle_wolfkin_insanity(ch);	
     }
     else
     {
	handle_mc_insanity(ch);	
     }
  }
  MOBtrigger = TRUE;
  return;
}

void do_pemote( CHAR_DATA *ch, char *argument )
{
    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }
 
    if ( argument[0] == '\0' )
    {
        send_to_char( "pemote what?\n\r", ch );
        return;
    }
 
    MOBtrigger = FALSE;
    act( "$n's $T", ch, NULL, emote_parse(ch,argument), TO_ROOM );
    act( "$n's $T", ch, NULL, emote_parse(ch,argument), TO_CHAR );
    reward_rp (ch);
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
             handle_wolfkin_insanity(ch);
     }
     else
     {
        handle_mc_insanity(ch);
     }
  }
    MOBtrigger = TRUE;
    return;
}

void do_emoteto( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char arg[MAX_INPUT_LENGTH];
  char buffer[MAX_INPUT_LENGTH];
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Do what to whom?\n\r", ch );
    return;
  }
  
  if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }

  argument = one_argument( argument, arg );
  
  if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
    send_to_char( "You can't show your emotions.\n\r", ch );
    return;
  }
  
  if ( argument[0] == '\0' ) {
    send_to_char( "Emote what?\n\r", ch );
    return;
  }
  
  MOBtrigger = FALSE;
  
  sprintf(buffer,"$n %s",emote_parse(ch,argument));
  act( buffer, ch, NULL, victim, TO_NOTVICT );
  act( buffer, ch, NULL, victim, TO_VICT );

  sprintf(buffer,"You %s",emote_parse(ch,argument));
  act( buffer, ch, NULL, victim, TO_CHAR );
  reward_rp (ch);
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
             handle_wolfkin_insanity(ch);
     }
     else
     {
        handle_mc_insanity(ch);
     }
  }
  MOBtrigger = TRUE;
  return;
}


void do_pmote( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    char *letter,*name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }
 
    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }
 
    act( "$n $t", ch, argument, NULL, TO_CHAR );

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
	if (vch->desc == NULL || vch == ch)
	    continue;

	if ((letter = strstr(argument,vch->name)) == NULL)
	{
	    MOBtrigger = FALSE;
	    act("$N $t",vch,argument,ch,TO_CHAR);
	    MOBtrigger = TRUE;
	    continue;
	}

	strcpy(temp,argument);
	temp[strlen(argument) - strlen(letter)] = '\0';
   	last[0] = '\0';
 	name = vch->name;
	
	for (; *letter != '\0'; letter++)
	{ 
	    if (*letter == '\'' && matches == strlen(vch->name))
	    {
		strcat(temp,"r");
		continue;
	    }

	    if (*letter == 's' && matches == strlen(vch->name))
	    {
		matches = 0;
		continue;
	    }
	    
 	    if (matches == strlen(vch->name))
	    {
		matches = 0;
	    }

	    if (*letter == *name)
	    {
		matches++;
		name++;
		if (matches == strlen(vch->name))
		{
		    strcat(temp,"you");
		    last[0] = '\0';
		    name = vch->name;
		    continue;
		}
		strncat(last,letter,1);
		continue;
	    }

	    matches = 0;
	    strcat(temp,last);
	    strncat(temp,letter,1);
	    last[0] = '\0';
	    name = vch->name;
	}

	MOBtrigger = FALSE;
	act("$N $t",vch,temp,ch,TO_CHAR);
	reward_rp (ch);
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
             handle_wolfkin_insanity(ch);
     }
     else
     {
        handle_mc_insanity(ch);
     }
  }
	MOBtrigger = TRUE;
    }
	
    return;
}


/*
 * All the posing stuff.
 */
struct	pose_table_type
{
    char *	message[2*MAX_CLASS];
};

const	struct	pose_table_type	pose_table	[]	=
{
    {
	{
	    "You sizzle with energy.",
	    "$n sizzles with energy.",
	    "You perform a small card trick.",
	    "$n performs a small card trick.",
	    "You show your bulging muscles.",
	    "$n shows $s bulging muscles."
	}
    },

    {
	{
	    "You turn into a butterfly, then return to your normal shape.",
	    "$n turns into a butterfly, then returns to $s normal shape.",
	    "You wiggle your ears alternately.",
	    "$n wiggles $s ears alternately.",
	    "You crack nuts between your fingers.",
	    "$n cracks nuts between $s fingers."
	}
    },

    {
	{
	    "Blue sparks fly from your fingers.",
	    "Blue sparks fly from $n's fingers.",
	    "You nimbly tie yourself into a knot.",
	    "$n nimbly ties $mself into a knot.",
	    "You grizzle your teeth and look mean.",
	    "$n grizzles $s teeth and looks mean."
	}
    },

    {
	{
	    "Little red lights dance in your eyes.",
	    "Little red lights dance in $n's eyes.",
	    "You juggle with daggers, apples, and eyeballs.",
	    "$n juggles with daggers, apples, and eyeballs.",
	    "You hit your head, and your eyes roll.",
	    "$n hits $s head, and $s eyes roll."
	}
    },

    {
	{
	    "A slimy green monster appears before you and bows.",
	    "A slimy green monster appears before $n and bows.",
	    "You steal the underwear off every person in the room.",
	    "Your underwear is gone!  $n stole it!",
	    "Crunch, crunch -- you munch a bottle.",
	    "Crunch, crunch -- $n munches a bottle."
	}
    },

    {
	{
	    "You turn everybody into a little pink elephant.",
	    "You are turned into a little pink elephant by $n.",
	    "The dice roll ... and you win again.",
	    "The dice roll ... and $n wins again.",
	    "... 98, 99, 100 ... you do pushups.",
	    "... 98, 99, 100 ... $n does pushups."
	}
    },

    {
	{
	    "A small ball of light dances on your fingertips.",
	    "A small ball of light dances on $n's fingertips.",
	    "You count the money in everyone's pockets.",
	    "Check your money, $n is counting it.",
	    "Arnold Schwarzenegger admires your physique.",
	    "Arnold Schwarzenegger admires $n's physique."
	}
    },

    {
	{
	    "Smoke and fumes leak from your nostrils.",
	    "Smoke and fumes leak from $n's nostrils.",
	    "You balance a pocket knife on your tongue.",
	    "$n balances a pocket knife on your tongue.",
	    "Watch your feet, you are juggling granite boulders.",
	    "Watch your feet, $n is juggling granite boulders."
	}
    },

    {
	{
	    "The light flickers as you rap in magical languages.",
	    "The light flickers as $n raps in magical languages.",
	    "You produce a coin from everyone's ear.",
	    "$n produces a coin from your ear.",
	    "Oomph!  You squeeze water out of a granite boulder.",
	    "Oomph!  $n squeezes water out of a granite boulder."
	}
    },

    {
	{
	    "Your head disappears.",
	    "$n's head disappears.",
	    "You step behind your shadow.",
	    "$n steps behind $s shadow.",
	    "You pick your teeth with a spear.",
	    "$n picks $s teeth with a spear."
	}
    },

    {
	{
	    "A fire elemental singes your hair.",
	    "A fire elemental singes $n's hair.",
	    "Your eyes dance with greed.",
	    "$n's eyes dance with greed.",
	    "Everyone is swept off their foot by your hug.",
	    "You are swept off your feet by $n's hug."
	}
    },

    {
	{
	    "The sky changes color to match your eyes.",
	    "The sky changes color to match $n's eyes.",
	    "You deftly steal everyone's weapon.",
	    "$n deftly steals your weapon.",
	    "Your karate chop splits a tree.",
	    "$n's karate chop splits a tree."
	}
    },

    {
	{
	    "The stones dance to your command.",
	    "The stones dance to $n's command.",
	    "The Grey Mouser buys you a beer.",
	    "The Grey Mouser buys $n a beer.",
	    "A strap of your armor breaks over your mighty thews.",
	    "A strap of $n's armor breaks over $s mighty thews."
	}
    },

    {
	{
	    "The heavens and grass change colour as you smile.",
	    "The heavens and grass change colour as $n smiles.",
	    "Everyone's pocket explodes with your fireworks.",
	    "Your pocket explodes with $n's fireworks.",
	    "A boulder cracks at your frown.",
	    "A boulder cracks at $n's frown."
	}
    },

    {
	{
	    "Everyone's clothes are transparent, and you are laughing.",
	    "Your clothes are transparent, and $n is laughing.",
	    "Everyone discovers your dagger a centimeter from their eye.",
	    "You discover $n's dagger a centimeter from your eye.",
	    "Mercenaries arrive to do your bidding.",
	    "Mercenaries arrive to do $n's bidding."
	}
    },

    {
	{
	    "A black hole swallows you.",
	    "A black hole swallows $n.",
	    "Where did you go?",
	    "Where did $n go?",
	    "Four matched Percherons bring in your chariot.",
	    "Four matched Percherons bring in $n's chariot."
	}
    },

    {
	{
	    "The world shimmers in time with your whistling.",
	    "The world shimmers in time with $n's whistling.",
	    "Click.",
	    "Click.",
	    "Atlas asks you to relieve him.",
	    "Atlas asks $n to relieve him."
	}
    }
};



void do_pose( CHAR_DATA *ch, char *argument )
{
    int level;
    int pose;

    if ( IS_NPC(ch) )
	return;

    level = UMIN( ch->level, sizeof(pose_table) / sizeof(pose_table[0]) - 1 );
    pose  = number_range(0, level);

    act( pose_table[pose].message[2*ch->class+0], ch, NULL, NULL, TO_CHAR );
    act( pose_table[pose].message[2*ch->class+1], ch, NULL, NULL, TO_ROOM );

    return;
}



void do_bug( CHAR_DATA *ch, char *argument )
{
    append_file( ch, BUG_FILE, argument );
    send_to_char( "Bug logged.\n\r", ch );
    return;
}

void do_typo( CHAR_DATA *ch, char *argument )
{
    append_file( ch, TYPO_FILE, argument );
    send_to_char( "Typo logged.\n\r", ch );
    return;
}

void do_rent( CHAR_DATA *ch, char *argument )
{
    send_to_char( "There is no rent here.  Just save and quit.\n\r", ch );
    return;
}


void do_qui( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to QUIT, you have to spell it out.\n\r", ch );
    return;
}



void do_quit( CHAR_DATA *ch, char *argument )
{
  DESCRIPTOR_DATA *d,*d_next;
  int id;
  char buf[MSL];
  
  if ( IS_NPC(ch) )
    return;

  if ( ch->position == POS_FIGHTING ) {
    send_to_char( "No way! You are fighting.\n\r", ch );
    return;
  }
  
  if ( ch->position  < POS_STUNNED  ) {
    send_to_char( "You're not DEAD yet.\n\r", ch );
    return;
  }

  if (ch->pcdata->next_quit > current_time) {
    sprintf(buf, "You have recently been to a fight with a PC and need to be in the game for %ld more seconds before you can quit.\n\r", (ch->pcdata->next_quit - current_time) < 0 ? 0 : (ch->pcdata->next_quit -  current_time));
    send_to_char(buf, ch);
    return;
  }
  
  // Get out of the link on exit
  // ---------------------------
  if (IS_CHANNELING(ch) && IS_LINKED(ch)) {
    do_unlink_char(ch->pIsLinkedBy,ch);
    do_unlink(ch, "");
  }
  
  // Unchannel when you log off
  // ---------------------------
  if (IS_CHANNELING(ch)) {
    do_function(ch, &do_unchannel, "" );
  }
  
  // If any sustained weaves on char, remove them
  // ---------------------------
  remove_sustained_weaves(ch);
  
  send_to_char( 
			"\nThank you for playing on the {DShadow {rWars{x.\n\r",ch);
  act( "$n has left the game.", ch, NULL, NULL, TO_ROOM );
  sprintf( log_buf, "%s has quit.", ch->name );
  log_string( log_buf );
  
  buf[0] = '\0';
  sprintf(buf,"$N {x(Level: %d) has logged off.", get_level(ch));
  wiznet(buf,ch,NULL,WIZ_LOGINS,0,get_trust(ch));
  do_bondnotify(ch,"leaving");

  /*
   * After extract_char the ch is no longer valid!
   */
  save_char_obj( ch, FALSE);
  
  /* Free note that might be there somehow */
  if (ch->pcdata->in_progress)
    free_note (ch->pcdata->in_progress);
  
  id = ch->id;
  d = ch->desc;
  extract_char( ch, TRUE, FALSE );
  if ( d != NULL )
    close_socket( d );
  
  /* toast evil cheating bastards */
  for (d = descriptor_list; d != NULL; d = d_next) {
    CHAR_DATA *tch;
    
    d_next = d->next;
    tch = d->original ? d->original : d->character;
    if (tch && tch->id == id) {
	 extract_char( tch, TRUE, FALSE );
	 close_socket(d);
    } 
  }
  return;
}

void do_save( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
	return;

    if ( IS_FACELESS(ch))
    {
	send_to_char("Your character is a temporary character.  You are not allowed to save it\n\r",ch);
	return;
    }

    save_char_obj( ch, FALSE);
    send_to_char("Saving...\n\r", ch);
/*
    send_to_char("Saving. Remember that ROM has automatic saving now.\n\r", ch);
    WAIT_STATE(ch,4 * PULSE_VIOLENCE);
*/
    return;
}



void do_follow( CHAR_DATA *ch, char *argument )
{
  /* RT changed to allow unlimited following and follow the NOFOLLOW rules */
  char buf[MSL];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Follow whom?\n\r", ch );
    return;
  }

  if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }
  
  if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL ) {
    act( "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
    return;
  }

  if ( victim == ch ) {
    if ( ch->master == NULL ) {
	 send_to_char( "You already follow yourself.\n\r", ch );
	 return;
    }
    stop_follower(ch);
    return;
  }
  
  if (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOFOLLOW) && !IS_IMMORTAL(ch)) {
    act("$N doesn't seem to want any followers.",ch,NULL,victim, TO_CHAR);
    return;
  }
  
  REMOVE_BIT(ch->act,PLR_NOFOLLOW);
  
  if ( ch->master != NULL )
    stop_follower( ch );
  
  
  /* Supplize!!! */
  if (IS_NPC(victim) && victim->spec_fun == spec_lookup( "spec_blademaster" )) {
    act("$N grip around the hilt of the sword as he says '{7It's time to practice the forms..{x'",ch,NULL,victim,TO_CHAR);
    one_hit( victim, ch, gsn_blademaster, FALSE, 0);
  }

  /* Supplize!!! */
  if (IS_NPC(victim) && victim->spec_fun == spec_lookup( "spec_duelling" )) {
    act("$N draws the daggers and says '{7It's time to practice the forms..{x'",ch,NULL,victim,TO_CHAR);
    one_hit( victim, ch, gsn_duelling, FALSE, 0);
  }

  /* Santa handeling, if object exists */
  if (IS_NPC(victim) && victim->spec_fun == spec_lookup( "spec_santa" )) {
    OBJ_DATA *obj=NULL;
    OBJ_DATA *obj2=NULL;
    OBJ_DATA *token=NULL;
    ROOM_INDEX_DATA *room=NULL;
    OBJ_INDEX_DATA *index=NULL;
    OBJ_INDEX_DATA *index_obj2=NULL;
    OBJ_INDEX_DATA *index_token=NULL;
    bool withToken=FALSE;
    DESCRIPTOR_DATA *d;
    
    index       = get_obj_index(9929);
    index_obj2  = get_obj_index(number_range(9930,9932));
    index_token = get_obj_index(9957);
    
    if (index != NULL && index_obj2 != NULL) {
	 
	 obj   = create_object(index, 0);
	 obj2 = create_object( index_obj2, 0 );
	 
	 if (number_percent() <= 25 && index_token != NULL) {
	   token = create_object(index_token, 0);
	   withToken=TRUE;
	 }

	 act("$N smile at you as he says, '{7Come sit on Santas knee, child.{x", ch, NULL, victim, TO_CHAR);
	 act("Suddenly bell sounds over the sky as $N's sledge and all the deers arrive. Rudolf nose {Rglow{x with an intense light. $N pat Rudolf as he take out a $p and give you!{x", ch, obj, victim, TO_CHAR);
	 act("Suddenly bell sounds over the sky as $N's sledge and all the deers arrive. Rudolf nose {Rglow{x with an intense light. $N pat Rudolf as he take out $p and give $n.{x", ch, obj, victim, TO_ROOM);            
	 obj_to_obj(obj2, obj);
	 
	 if (withToken) {
	   obj_to_obj(token, obj);
	 }
	 
	 obj_to_char(obj, ch);
	 
	 // Log it
	 if (withToken) {
	   sprintf(buf, "$N found Santa at room %d and got $p with %s and a partial Token.", victim->in_room->vnum, obj2->short_descr);
	   wiznet(buf, ch, obj, WIZ_LEVELS, 0, get_trust(ch));
	   sprintf(buf, "%s found Santa at room %d and got %s with %s and a partial Token.", ch->name, victim->in_room->vnum, obj->short_descr, obj2->short_descr);
	   log_string(buf);
	 }
	 else {
	   sprintf(buf, "$N found Santa at room %d and got $p with %s.", victim->in_room->vnum, obj2->short_descr);
	   wiznet(buf, ch, obj, WIZ_LEVELS, 0, get_trust(ch));
	   sprintf(buf, "%s found Santa at room %d and got %s with %s.", ch->name, victim->in_room->vnum, obj->short_descr, obj2->short_descr);
	   log_string(buf);
	 }
	 
	 room = get_random_room(victim, FALSE);
	 if (room != NULL) {
	   act("$N climb up into the sledge. A round and well tuned, '{7Ho Ho Ho{x' chime through the sky as the sledge shoot up into the sky.",ch,NULL,victim,TO_ROOM);
	   char_from_room(victim);
	   char_to_room(victim, room);

	   for ( d = descriptor_list; d; d = d->next )
		if ( d->connected == CON_PLAYING )
		  send_to_char("Through the sky a round and well tuned '{RHo {WHo {RHo{W...{x' chime.\n", d->character);
	 }
	 return;
    }
  }
  
  add_follower( ch, victim );
  return;
}


void add_follower( CHAR_DATA *ch, CHAR_DATA *master )
{
    if ( ch->master != NULL )
    {
	bug( "Add_follower: non-null master.", 0 );
	return;
    }

    ch->master        = master;
    ch->leader        = NULL;

    if ( can_see( master, ch ) )
	act( "$n now follows you.", ch, NULL, master, TO_VICT );

    act( "You now follow $N.",  ch, NULL, master, TO_CHAR );

    return;
}



void stop_follower( CHAR_DATA *ch )
{
  if ( ch->master == NULL ) {
    bug( "Stop_follower: null master.", 0 );
    return;
  }
  

/*
  if ( IS_AFFECTED(ch, AFF_CHARM) ) {
    REMOVE_BIT( ch->affected_by, AFF_CHARM );
    affect_strip( ch, gsn_charm_person );
  }
*/
  
  if ( can_see( ch->master, ch ) && ch->in_room != NULL) {
    act( "$n stops following you.",     ch, NULL, ch->master, TO_VICT    );
    act( "You stop following $N.",      ch, NULL, ch->master, TO_CHAR    );
  }
  
  if (ch->master->pet == ch) {
    ch->master->pet = NULL;
  }
	  
  if ((ch == ch->master->mount) && (ch->master == ch->mount)
	 && IS_SET(ch->act, ACT_RIDEABLE)) {
    
    ch->mount = NULL;
    ch->riding = FALSE;
    ch->master->mount = NULL;
    ch->master->riding = FALSE;
    
  }
    	
  ch->master = NULL;
  ch->leader = NULL;
  return;
}

/* nukes charmed monsters and pets */
void nuke_pets( CHAR_DATA *ch )
{    
  CHAR_DATA *pet;
  
  if (IS_NPC(ch))
    return;
  
  if ((pet = ch->pet) != NULL) {
    stop_follower(pet);
    if (pet->in_room != NULL)
	 act("$N slowly fades away.",ch,NULL,pet,TO_NOTVICT);
    extract_char(pet,TRUE,FALSE);
  }
  ch->pet = NULL;

  if ( ch->mount && (ch->mount->in_room == ch->in_room || ch->mount->in_room==NULL) ) {
    pet = ch->mount;
    if (MOUNTED(ch))
	 do_dismount(ch, "");
    if (pet->in_room != NULL)
	 act("$N slowly fades away.",ch,NULL,pet,TO_NOTVICT);
    else
	 log_string("void nuke_pets: Extracting null pet");
    ch->mount = NULL;
    ch->riding = FALSE;            
    extract_char(pet, TRUE, FALSE);
  }
  else if (ch->mount) {
    pet = ch->mount;
    if (pet->in_room != NULL)
       act("$N slowly fades away.",ch,NULL,pet,TO_NOTVICT);
      	
    ch->mount->mount = NULL;
    ch->mount->riding = FALSE;
    ch->mount = NULL;
    ch->riding = FALSE;
    extract_char(pet, TRUE, FALSE);
  }
  ch->mount = NULL;
  
  return;
}



void die_follower( CHAR_DATA *ch )
{
  CHAR_DATA *fch;

  if ( ch->master != NULL ) {
    if (ch->master->pet == ch)
	 ch->master->pet = NULL;
    stop_follower( ch );
  }
  
  ch->leader = NULL;
  
  for ( fch = char_list; fch != NULL; fch = fch->next ) {
    if ( fch->master == ch ) {
	 stop_follower( fch );
    }
    if ( fch->leader == ch )
	 fch->leader = fch;
  }
  
  return;
}



void do_order( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  CHAR_DATA *och;
  CHAR_DATA *och_next;
  bool found;
  bool fAll;
  
  argument = one_argument( argument, arg );
  one_argument(argument,arg2);
  
  if (!str_cmp(arg2,"delete") || !str_cmp(arg2,"mob") /*|| (!str_cmp(arg2,"remove") && !IS_IMMORTAL(ch))*/) {
    send_to_char("That will {RNOT{x be done! This has been logged.\n\r",ch);
    sprintf(buf, "%s tried to order %s %s!", !IS_NPC(ch) ? ch->name : ch->short_descr, !IS_NULLSTR(arg) ? arg : "(null)", !IS_NULLSTR(arg2) ? arg2 : "(null)");
    log_string(buf);
    return;
  }

  if ( arg[0] == '\0' || argument[0] == '\0' ) {
    send_to_char( "Order whom to do what?\n\r", ch );
    return;
  }
  
  if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
    send_to_char( "You feel like taking, not giving, orders.\n\r", ch );
    return;
  }
  
  if ( !str_cmp( arg, "all" ) ) {
    fAll   = TRUE;
    victim = NULL;
  }
  else {
    fAll   = FALSE;
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
	 send_to_char( "They aren't here.\n\r", ch );
	 return;
    }
    
    if ( victim == ch ) {
	 send_to_char( "Aye aye, right away!\n\r", ch );
	 return;
    }
    
    // orders not allowed on PCs
    /*
    if (!IS_NPC(victim) && !str_cmp(arg2,"remove")) {
	 send_to_char("That will {RNOT{x be done! This has been logged.\n\r",ch);
	 sprintf(buf, "%s tried to order %s %s!", !IS_NPC(ch) ? ch->name : ch->short_descr, !IS_NULLSTR(arg) ? arg : "(null)", !IS_NULLSTR(arg2) ? arg2 : "(null)");
	 log_string(buf);
	 return;
    }	
    */
    
    if (IS_AFFECTED(victim,AFF_CHARM) && victim->master == NULL) {
	 AFFECT_DATA *paf;
	 AFFECT_DATA *paf_next;
	 
	 for (paf = victim->affected; paf != NULL; paf = paf_next) {
	   paf_next = paf->next;
	   if (paf->bitvector == AFF_CHARM && paf->casterId == ch->id) {
		victim->master = ch;
		add_follower( victim, ch );
		victim->leader = ch;
	   }
	 }    
    }
    else  {
	 //send_to_char("They aren't charmed\r\n",ch);
    }
    
    if (!IS_NPC(victim) && !str_cmp(victim->pcdata->bondedby,ch->name) /*&& victim->pcdata->bondedbysex == SEX_MALE*/)
    {
	 found = TRUE;
    }
    else
    {
    if (!is_guild_guard(victim, ch)) {
	 if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch 
		||  (IS_IMMORTAL(victim) && victim->trust >= ch->trust) )
 	 {
	   send_to_char( "Do it yourself!\n\r", ch );
	   return;
	 }
    }
  }
  }
  
  found = FALSE;
  for ( och = ch->in_room->people; och != NULL; och = och_next ) {
    och_next = och->next_in_room;
    
    if ((is_guild_guard(och, ch) || 
        (IS_AFFECTED(och, AFF_CHARM) && och->master == ch)
	|| (!IS_NPC(och) && !str_cmp(och->pcdata->bondedby,ch->name) && och->pcdata->bondedbysex == SEX_MALE))
	&& ( fAll || och == victim ) ) {
      
	 // orders not allowed on PCs
	 /*
	 if (!IS_NPC(och) && !str_cmp(arg2,"remove")) {
	   send_to_char("That will {RNOT{x be done! This has been logged.\n\r",ch);
	   sprintf(buf, "%s tried to order %s %s!", !IS_NPC(ch) ? ch->name : ch->short_descr, !IS_NULLSTR(arg) ? arg : "(null)", !IS_NULLSTR(arg2) ? arg2 : "(null)");
	   log_string(buf);
	   return;
	 }			
	 */
	 
	 found = TRUE;
	 sprintf( buf, "$n orders you to '%s'.", argument );
	 act( buf, ch, NULL, och, TO_VICT );
	 interpret( och, argument );
    }
  }
  
  if ( found ) {
    WAIT_STATE(ch,PULSE_VIOLENCE);
    send_to_char( "Ok.\n\r", ch );
  }
  else
    send_to_char( "You have no followers here.\n\r", ch );
  return;
}


void do_group( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	CHAR_DATA *gch;
	CHAR_DATA *leader;

	leader = (ch->leader != NULL) ? ch->leader : ch;
	sprintf( buf, "%s's group:\n\r", PERS(leader, ch));
	send_to_char( buf, ch );

	for ( gch = char_list; gch != NULL; gch = gch->next ) {
	  if ( is_same_group( gch, ch ) ) {
	    sprintf (buf, "[Hp: {R%3d{x%c End: {G%3d{x%c{W]{x %s\n\r",
			   ((100 * gch->hit ) / gch->max_hit),
			   '%', ((100 * gch->endurance) / gch->max_endurance),
			   '%', PERS(gch, ch));
	    send_to_char( buf, ch );
	  }
	}
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( ch->master != NULL || ( ch->leader != NULL && ch->leader != ch ) )
    {
	send_to_char( "But you are following someone else!\n\r", ch );
	return;
    }

    if ( victim->master != ch && ch != victim )
    {
	act_new("$N isn't following you.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
	return;
    }
    
    if (IS_AFFECTED(victim,AFF_CHARM))
    {
        send_to_char("You can't remove charmed mobs from your group.\n\r",ch);
        return;
    }
    
    if (IS_AFFECTED(ch,AFF_CHARM))
    {
    	act_new("You like your master too much to leave $m!",
	    ch,NULL,victim,TO_VICT,POS_SLEEPING);
    	return;
    }

    if ( is_same_group( victim, ch ) && ch != victim )
    {
	victim->leader = NULL;
	act_new("$n removes $N from $s group.",
	    ch,NULL,victim,TO_NOTVICT,POS_RESTING);
	act_new("$n removes you from $s group.",
	    ch,NULL,victim,TO_VICT,POS_SLEEPING);
	act_new("You remove $N from your group.",
	    ch,NULL,victim,TO_CHAR,POS_SLEEPING);
	return;
    }

    victim->leader = ch;
    if (!IS_IMMORTAL(ch) && !IS_IMMORTAL(victim))
    	act_new("$N joins $n's group.",ch,NULL,victim,TO_NOTVICT,POS_RESTING);
    act_new("You join $n's group.",ch,NULL,victim,TO_VICT,POS_SLEEPING);
    act_new("$N joins your group.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
    return;
}

/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount_gold = 0, amount_silver = 0;
    int share_gold, share_silver;
    int extra_gold, extra_silver;

    argument = one_argument( argument, arg1 );
	       one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Split how much?\n\r", ch );
	return;
    }
    
    amount_silver = atoi( arg1 );

    if (arg2[0] != '\0')
	amount_gold = atoi(arg2);

    if ( amount_gold < 0 || amount_silver < 0)
    {
	send_to_char( "Your group wouldn't like that.\n\r", ch );
	return;
    }

    if ( amount_gold == 0 && amount_silver == 0 )
    {
	send_to_char( "You hand out zero coins, but no one notices.\n\r", ch );
	return;
    }

    if ( ch->gold <  amount_gold || ch->silver < amount_silver)
    {
	send_to_char( "You don't have that much to split.\n\r", ch );
	return;
    }
  
    members = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
	if ( is_same_group( gch, ch ) && !IS_AFFECTED(gch,AFF_CHARM))
	    members++;
    }

    if ( members < 2 )
    {
	send_to_char( "Just keep it all.\n\r", ch );
	return;
    }
	    
    share_silver = amount_silver / members;
    extra_silver = amount_silver % members;

    share_gold   = amount_gold / members;
    extra_gold   = amount_gold % members;

    if ( share_gold == 0 && share_silver == 0 )
    {
	send_to_char( "Don't even bother, cheapskate.\n\r", ch );
	return;
    }

    ch->silver	-= amount_silver;
    ch->silver	+= share_silver + extra_silver;
    ch->gold 	-= amount_gold;
    ch->gold 	+= share_gold + extra_gold;

    if (share_silver > 0)
    {
	sprintf(buf,
	    "You split %d silver coins. Your share is %d silver.\n\r",
 	    amount_silver,share_silver + extra_silver);
	send_to_char(buf,ch);
    }

    if (share_gold > 0)
    {
	sprintf(buf,
	    "You split %d gold coins. Your share is %d gold.\n\r",
	     amount_gold,share_gold + extra_gold);
	send_to_char(buf,ch);
    }

    if (share_gold == 0)
    {
	sprintf(buf,"$n splits %d silver coins. Your share is %d silver.",
		amount_silver,share_silver);
    }
    else if (share_silver == 0)
    {
	sprintf(buf,"$n splits %d gold coins. Your share is %d gold.",
		amount_gold,share_gold);
    }
    else
    {
	sprintf(buf,
"$n splits %d silver and %d gold coins, giving you %d silver and %d gold.\n\r",
	 amount_silver,amount_gold,share_silver,share_gold);
    }

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
	if ( gch != ch && is_same_group(gch,ch) && !IS_AFFECTED(gch,AFF_CHARM))
	{
	    act( buf, ch, NULL, gch, TO_VICT );
	    gch->gold += share_gold;
	    gch->silver += share_silver;
	}
    }

    return;
}

void do_gtell( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *gch;
  
  if ( argument[0] == '\0' ) {
    send_to_char( "Tell your group what?\n\r", ch );
    return;
  }
  
  if ( IS_SET( ch->comm, COMM_NOTELL ) ) {
    send_to_char( "Your message didn't get through!\n\r", ch );
    return;
  }
  
  if (IS_SET(ch->comm,COMM_QUIET)) {
     send_to_char("You must turn off quiet mode first.\n\r",ch);
     return;
  }
  
  act_new("$n tells the group '{n$t{x'",ch,argument,NULL,TO_CHAR,POS_SLEEPING);
  for ( gch = char_list; gch != NULL; gch = gch->next ) {
    if (is_same_group( gch, ch )) {
	 act_new("$n tells the group '{n$t{x'",ch,argument,gch,TO_VICT,POS_SLEEPING);
	 if (!IS_NPC(gch))
	   do_addchannelhistory(ch,gch->pcdata->group_history,argument, NULL);
    }
  }
  
  return;
}

void do_gtellh( CHAR_DATA *ch, char *argument )
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  /* int first=TRUE; */
  int entry=FALSE;

  /*
   *  Do Group Tell history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (ch->pcdata->group_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  ch->pcdata->group_history[i].wizinvis_level) ||
		  ((ch->pcdata->group_history[i].invis_flag == 1) && 
		   ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
		    (IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		sprintf (line_detail,"[ {nGroup {y%s{x]{W:{x Someone {n'%s'{x\n\r",        
			    sec2strtime(ch->pcdata->group_history[i].when),
			    ch->pcdata->group_history[i].line_data);
	   }
	   else {
		sprintf(line_detail,"[ {nGroup {y%s{x]{W:{x %s {n'%s'{x\n\r",
			   sec2strtime(ch->pcdata->group_history[i].when),
			   ch->pcdata->group_history[i].player_name,
			   ch->pcdata->group_history[i].line_data);
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch )
{

    if ( ach == NULL || bch == NULL)
	return FALSE;

    if ( ach == bch)
	return TRUE; 

    if ( ach->leader != NULL ) ach = ach->leader;
    if ( bch->leader != NULL ) bch = bch->leader;
    return ach == bch;
}

/*
 * ColoUr setting and unsetting, way cool, Ant Oct 94
 *        revised to include config colour, Ant Feb 95
 */
void do_colour( CHAR_DATA *ch, char *argument )
{
    char 	arg[ MAX_STRING_LENGTH ];

    if( IS_NPC( ch ) )
    {
	send_to_char_bw( "ANSI color is not on.\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg );

    if( !*arg )
    {
	if( !IS_SET( ch->act, PLR_COLOUR ) )
	{
	    SET_BIT( ch->act, PLR_COLOUR );
	    send_to_char( "ANSI color is now turned on.\n\r"
		"Further syntax:\n\r   color {c<{xfield{c> <{xcolor{c>{x\n\r"
		"   color {c<{xfield{c>{x {cbeep{x|{cnobeep{x\n\r"
		"Type help {ccolor{x and {ccolor2{x for details.\n\r"
		"Color is brought to you by Lope, ant@solace.mh.se.\n\r", ch );
	}
	else
	{
	    send_to_char_bw( "ANSI color is now turned off.\n\r", ch );
	    REMOVE_BIT( ch->act, PLR_COLOUR );
	}
	return;
    }

    if( !str_cmp( arg, "default" ) )
    {
	default_colour( ch );
	send_to_char_bw( "ANSI Color setting set to default values.\n\r", ch );
	return;
    }

    if( !str_cmp( arg, "list" ) )
    {
	do_help( ch, "color list" );
	return;
    }

    if( !str_cmp( arg, "all" ) )
    {
	all_colour( ch, argument );
	return;
    }

    /*
     * Yes, I know this is ugly and unnessessary repetition, but its old
     * and I can't justify the time to make it pretty. -Lope
     */
    if( !str_cmp( arg, "auction" ) )
    {
	ALTER_COLOUR( auction )
    }
    else if( !str_cmp( arg, "gossip" ) )
    {
	ALTER_COLOUR( gossip )
    }
    else if( !str_cmp( arg, "music" ) )
    {
	ALTER_COLOUR( music )
    }
    else if( !str_cmp( arg, "chat" ) )
    {
	ALTER_COLOUR( chat )
    }
    else if( !str_cmp( arg, "minion" ) )
    {
	ALTER_COLOUR( minion )
    }
    else if( !str_cmp( arg, "immtalk" ) )
    {
	ALTER_COLOUR( immtalk )
    }
    else if( !str_cmp( arg, "game" ) )
    {
	ALTER_COLOUR( game )
    }
    else if( !str_cmp( arg, "tell" ) )
    {
	ALTER_COLOUR( tell )
    }
    else if( !str_cmp( arg, "reply" ) )
    {
	ALTER_COLOUR( reply )
    }
    else if( !str_cmp( arg, "gtell" ) )
    {
	ALTER_COLOUR( gtell )
    }
    else if( !str_cmp( arg, "room_exits" ) )
    {
	ALTER_COLOUR( room_exits )
    }
    else if( !str_cmp( arg, "room_things" ) )
    {
	ALTER_COLOUR( room_things )
    }
    else if( !str_cmp( arg, "prompt" ) )
    {
	ALTER_COLOUR( prompt )
    }
    else if( !str_cmp( arg, "room_people" ) )
    {
	ALTER_COLOUR( room_people )
    }
    else if( !str_cmp( arg, "room" ) )
    {
	ALTER_COLOUR( room )
    }
    else if( !str_cmp( arg, "bondtalk" ) )
    {
	ALTER_COLOUR( bondtalk )
    }
    else if( !str_cmp( arg, "channel_name" ) )
    {
	ALTER_COLOUR( channel_name )
    }
    else if( !str_cmp( arg, "wiznet" ) )
    {
	ALTER_COLOUR( wiznet )
    }
    else if( !str_cmp( arg, "fight_death" ) )
    {
	ALTER_COLOUR( fight_death )
    }
    else if( !str_cmp( arg, "fight_yhit" ) )
    {
	ALTER_COLOUR( fight_yhit )
    }
    else if( !str_cmp( arg, "fight_ohit" ) )
    {
	ALTER_COLOUR( fight_ohit )
    }
    else if( !str_cmp( arg, "fight_thit" ) )
    {
	ALTER_COLOUR( fight_thit )
    }
    else if( !str_cmp( arg, "fight_skill" ) )
    {
	ALTER_COLOUR( fight_skill )
    }
    else if( !str_cmp( arg, "say" ) )
    {
	ALTER_COLOUR( sayt )
    }
    else if( !str_cmp( arg, "osay" ) )
    {
	ALTER_COLOUR( osay )
    }
    else if( !str_cmp( arg, "wolfkin_talk" ) )
    {
	ALTER_COLOUR( wolfkin_talk )
    }
    else if( !str_cmp( arg, "guild_talk" ) )
    {
	ALTER_COLOUR( guild_talk )
    }
    else if( !str_cmp( arg, "race_talk" ) )
    {
	ALTER_COLOUR( race_talk )
    }
    else if( !str_cmp( arg, "df_talk" ) )
    {
	ALTER_COLOUR( df_talk )
    }
    else if( !str_cmp( arg, "newbie" ) )
    {
	ALTER_COLOUR( newbie )
    }
    else if( !str_cmp( arg, "sguild_talk") ) {
	 ALTER_COLOUR( sguild_talk )
    }
    else if( !str_cmp( arg, "ssguild_talk") ) {
	 ALTER_COLOUR( ssguild_talk )
    }
    else if( !str_cmp( arg, "leader_talk") ) {
	 ALTER_COLOUR( leader_talk )
    }
    else if( !str_cmp( arg, "oguild_talk") ) {
	 ALTER_COLOUR( oguild_talk )
    }    
    else
    {
	send_to_char_bw( "Unrecognised Colour Parameter Not Set.\n\r", ch );
	return;
    }

    send_to_char_bw( "New Colour Parameter Set.\n\r", ch );
    return;
}

void do_clear(CHAR_DATA *ch, char *argument)
{
	if ( !str_cmp( argument, "reset" ) )
//		send_to_char(VT_SETWIN_CLEAR VT_HOMECLR, ch);
		send_to_char(VT_CLENSEQ, ch);
	else
		send_to_char(VT_CLEAR_SCREEN, ch);

	if (ch->desc && ch->desc->screenmap)
		InitScreenMap(ch->desc);

	return;
}

void do_clearcommand(CHAR_DATA *ch, char *argument)
{
	//The work for this command has already been done at this point. just show them that it was called
	send_to_char("Command Stack cleared.\r\n",ch);
	return;
}

void do_olcx(CHAR_DATA *ch, char *argument)
{
	TOGGLE_BIT(ch->comm, COMM_OLCX);
	if (IS_SET(ch->comm, COMM_OLCX))
		send_to_char("VT100/OLC extensions activated.\n\r", ch );
	else
		send_to_char("VT100/OLC extensions deactivated.\n\r", ch );
}

void do_racelist( CHAR_DATA *ch, char *argument )
{
  BUFFER *output;
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;

  CHAR_DATA *wch;    
  CHAR_DATA *pcs[MAX_PC_ONLINE];
  int cnt = 0;
  int i   = 0;

  output = new_buf();
  
  for (d = descriptor_list; d != NULL; d = d->next) {  
    if (d->connected != CON_PLAYING || !can_see_channel(ch,d->character))
	 continue;
    
    wch = ( d->original != NULL ) ? d->original : d->character;
    
    if (!can_see_channel(ch,wch))
	 continue;
    
     /* Whoinvis */
     if ((wch->incog_level > 0) && (wch != ch) && !IS_IMMORTAL(ch))
        continue;

    /* Immortals don't turn up on racelist */
    if (IS_IMMORTAL(wch))
	 continue;
    
    if (wch->race != ch->race)
	 continue;

    pcs[cnt++] = wch;
  }
  
  /* Sort PC's array */
  qsort (pcs, cnt, sizeof(wch), compare_char_names);
  
  for (i=0; i < cnt; i++) {
    sprintf(buf, "[%-16s ]  %s\n", race_table[ch->race].who_name, COLORNAME(pcs[i]));
    add_buf(output,buf);
  }

  sprintf(buf, "\nMembers found: {y%d{x.\n\r", cnt);
  add_buf(output,buf);
  
  page_to_char(buf_string(output),ch);
  free_buf(output);
}

void do_racetalk( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  
  if (IS_NPC(ch))
    return;
  
  if (argument[0] == '\0') {
    if (IS_SET(ch->comm, COMM_NORACE)) {
	 send_to_char("Race channel is now ON.\n\r", ch);
	 REMOVE_BIT(ch->comm, COMM_NORACE);
    }
    else {
	 send_to_char("Race channel is now OFF.\n\r", ch);
	 SET_BIT(ch->comm, COMM_NORACE);
    }
  }
  else {
    if (IS_SET(ch->comm, COMM_QUIET)) {
	 send_to_char("You must turn off quiet mode first.\n\r", ch);
	 return;
    }
    
    if (IS_SET(ch->comm, COMM_NOCHANNELS)) {
	 send_to_char("The gods have revoked your channels priviliges.\n\r", ch);
	 return;
    }
    
    REMOVE_BIT(ch->comm, COMM_NORACE);
    
    if (!IS_EMOTE(argument)) {   
	 sprintf(buf, "{w[{v %s {w]{W: {w$n {w'{v$t{w'{x", race_table[ch->race].who_name);
	 act_old(buf, ch, colorcontinue('v',argument), NULL, TO_CHAR, POS_DEAD);
    }
    else {  
	 strcpy(cemotebuf,&argument[1]);
	 cemotebuf[strlen(cemotebuf)-1] = '\0';
	 sprintf(buf,"{w[ {v%s {w]{W:{x '{v$n {v$t{x'", race_table[ch->race].who_name);
	 act_old(buf, ch, colorcontinue('v',cemotebuf), NULL, TO_CHAR, POS_DEAD);
    }
    
    do_addchannelhistory(ch, race_history[ch->race],argument, NULL);

    for (d = descriptor_list; d != NULL; d = d->next) {
	 CHAR_DATA *victim;

	 victim = d->original ? d->original : d->character;

	 if (d->connected == CON_PLAYING
		&&	victim != ch
		&&	victim->race == ch->race
		&&  !IS_NPC(victim) && !IS_SET(victim->comm, COMM_NORACE)
		&&	!IS_SET(victim->comm, COMM_QUIET)) {
	   if (!IS_EMOTE(argument))
		act_old(buf, ch, colorcontinue('v',argument), d->character, TO_VICT,POS_SLEEPING);
	   else
		act_old(buf, ch, colorcontinue('v',cemotebuf), d->character, TO_VICT,POS_SLEEPING);
	 }
    }
  }
}

void do_racetalkh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  /* int first=TRUE; */
  int entry=FALSE;

  if (IS_NPC(ch))
     return;

  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (race_history[ch->race][i].player_name){
	   entry=TRUE;
	   if ((get_trust(ch) <  race_history[ch->race][i].wizinvis_level) ||
		  ( (race_history[ch->race][i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(race_history[ch->race][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: Someone '{c%s{x'\n\r",
				 race_table[ch->race].who_name,
				 sec2strtime(race_history[ch->race][i].when),
				 race_history[ch->race][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&race_history[ch->race][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{8Someone {8%s{x'\n\r",
				 race_table[ch->race].who_name,
				 sec2strtime(race_history[ch->race][i].when),
				 cemotebuf);
		}
	   }
	   else {
		if (!IS_EMOTE(race_history[ch->race][i].line_data)) { 
		  sprintf (line_detail,"[%s {y%s{x]: %s '{8%s{x'\n\r",
				 race_table[ch->race].who_name,
				 sec2strtime(race_history[ch->race][i].when),
				 race_history[ch->race][i].player_name, 
				 race_history[ch->race][i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&race_history[ch->race][i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"[%s {y%s{x]: '{8%s {8%s{x'\n\r",
				 race_table[ch->race].who_name,
				 sec2strtime(race_history[ch->race][i].when),
				 race_history[ch->race][i].player_name, 
				 cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

void do_bounty( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
	
    
        if (!str_cmp(arg1,"list")) {
           do_bountylist(ch,arg1);
           return;
        }
        
	if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
	     send_to_char( "Place a bounty on who's head?\n\rSyntax: Bounty <victim> <amount>\n\r", ch );
             return;
        }
	
        if ( ( victim = get_char_world( ch, arg1 ) ) == NULL) {
  	   send_to_char( "They are currently not logged in!", ch );
	   return;
        }
  
	if (ch == victim) {
	send_to_char( "You can't place a bounty on yourself!",ch);
	return;
	}
	
      if (IS_NPC(victim)) {
	send_to_char( "You cannot put a bounty on NPCs!", ch );
	return;
      }

	if ( is_number( arg2 ) ) {
	int amount;
	amount   = atoi(arg2);
	if (amount <= 0) {
	   send_to_char("Don't even think about it!",ch);
           return;
        }
        
        if (ch->gold < amount) {
		send_to_char( "You don't have that much gold!", ch );
		return;
        }
	ch->gold -= amount;
	victim->pcdata->bounty +=amount;
	sprintf( buf, "You have placed a %d gold bounty on %s{g.\n\r%s now has a bounty of %d gold.",
	amount,victim->name,victim->name,victim->pcdata->bounty );
	send_to_char(buf,ch);

        sprintf( buf, "%s has placed a %d gold bounty on you.\n\rYou are now worth %d gold dead.",ch->name,amount,victim->pcdata->bounty);
	send_to_char(buf,victim);

	return;
	}
}

void do_bountylist( CHAR_DATA *ch, char *argument )
{
    BUFFER *output;
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found = FALSE;

    output = new_buf();

    sprintf(buf, "The following players have a {rBounty{x for their head:\n\n\r");
    add_buf(output, buf);
    sprintf(buf, "{D[{W  Amount  {D] [{W Players:{x\n\r");
    add_buf(output, buf);

    for (d = descriptor_list; d != NULL; d = d->next) {
   	CHAR_DATA *wch;

 	   if (d->connected != CON_PLAYING || !can_see_channel(ch,d->character))
	      continue;
	
	   wch = ( d->original != NULL ) ? d->original : d->character;

 	   if (!can_see_channel(ch,wch))
	      continue;
	      
          /* Whoinvis */
          if ((wch->incog_level > 0) && (wch != ch) && !IS_IMMORTAL(ch))
              continue;	      

	   if (wch->pcdata->bounty) {
	      found = TRUE;
	        
	       /* a little formatting */
         sprintf(buf, "{D[{x %8d {D] [{x %s%s\n\r",
             wch->pcdata->bounty,
             COLORNAME(wch), 
             IS_NPC(wch) ? "" : wch->pcdata->title);
	      add_buf(output,buf);
	   }
    }

    if (!found) {
	   send_to_char("No one with a bounty is playing.\n\r",ch);
	   return;
    }

    page_to_char(buf_string(output),ch);
    free_buf(output);
}

void do_chat( CHAR_DATA *ch, char *argument )
{
    char cemotebuf[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOCHAT))
      {
        send_to_char("Chat channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOCHAT);
      }
      else
      {
        send_to_char("Chat channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOCHAT);
      }
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
		
	   }

      if (IS_SET(ch->comm,COMM_NOCHAT))
      {
	send_to_char("You must turn on chat before using it.\n\r",ch);
	return;
      }
	   
      if (IS_AFFECTED(ch,AFF_GAGGED))
      {
        send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
        return;
      }

	 // Make sure no cursing etc. on public channels
	 censor_speech(ch, argument);

      if (!IS_EMOTE(argument))
      {
	   act_old("{w[{fChat{w]{W: {w$n '{f$t{x'",ch,colorcontinue('f', argument),NULL,TO_CHAR,POS_SLEEPING);
	   sprintf(buffer,"Chat: {C%s '%s{x'",ch->name,colorcontinue('C', argument));
      }
      else
	   {  strcpy(cemotebuf,&argument[1]);
	   cemotebuf[strlen(cemotebuf)-1] = '\0';
	   act_old("{w[{fChat{w]{W: {w'{f$n {f$t{x'",ch,colorcontinue('f', cemotebuf),NULL,TO_CHAR,POS_SLEEPING);
	   sprintf(buffer,"Chat: {C%s '%s{x'",ch->name,colorcontinue('C', cemotebuf));
	   }
      
      log_comm_string("chat",buffer);

      do_addchannelhistory(ch,chat_history,argument, NULL);
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        CHAR_DATA *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOCHAT) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
          if (!IS_EMOTE(argument))
          {   act_old( "{w[{fChat{w]{W: {w$n '{f$t{x'", 
		   ch,colorcontinue('f', argument), d->character, TO_VICT,POS_SLEEPING ); 
          }
          else
		  {  
		    act_old( "{w[{fChat{w]{W: {w'{w$n {f$t{x'", 
				   ch,colorcontinue('f', cemotebuf), d->character, TO_VICT,POS_SLEEPING );
          }

        }
      }
    }
}

void do_glt( CHAR_DATA *ch, char *argument )
{
    char cemotebuf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    if ((!is_leader(ch)) && (!IS_IMMORTAL(ch)))
    {
	send_to_char("Huh?\n\r",ch);
        return;
    }
 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOGUILDLEADER))
      {
        send_to_char("Guild Leader channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOGUILDLEADER);
      }
      else
      {
        send_to_char("Guild Leader channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOGUILDLEADER);
      }
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
		
	   }

	   REMOVE_BIT(ch->comm,COMM_NOGUILDLEADER);
	   
      if (IS_AFFECTED(ch,AFF_GAGGED))
      {
        send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
        return;
      }

	 // Make sure no cursing etc. on public channels
	 censor_speech(ch, argument);

      if (!IS_EMOTE(argument))
	   act_old("{w[{LGUILDLEADER{w]{W: {w$n '{L$t{x'",ch,colorcontinue('L', argument),NULL,TO_CHAR,POS_SLEEPING);
      else
	   {  strcpy(cemotebuf,&argument[1]);
	   cemotebuf[strlen(cemotebuf)-1] = '\0';
	   act_old("{w[{LGUILDLEADER{w]{W: {w'{L$n {L$t{x'",ch,colorcontinue('L', cemotebuf),NULL,TO_CHAR,POS_SLEEPING);
	   }
      

      do_addchannelhistory(ch,guildleader_history,argument, NULL);
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        CHAR_DATA *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             is_leader(victim) &&
             !IS_SET(victim->comm,COMM_NOGUILDLEADER) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
          if (!IS_EMOTE(argument))
          {   act_old( "{w[{LGUILDLEADER{w]{W: {w$n '{L$t{x'", 
		   ch,colorcontinue('L', argument), d->character, TO_VICT,POS_SLEEPING ); 
          }
          else
		  {  
		    act_old( "{w[{LGUILDLEADER{w]{W: {w'{w$n {L$t{x'", 
				   ch,colorcontinue('L', cemotebuf), d->character, TO_VICT,POS_SLEEPING );
          }

        }
      }
    }
}

void do_dftalk( CHAR_DATA *ch, char *argument )
{
    char cemotebuf[MAX_STRING_LENGTH];
    char buff[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (ch->pcdata->df_level < 0)
    {
        send_to_char("You aren't a friend of the dark.\n\r",ch);
	return;
    }
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NODFT))
      {
        send_to_char("Darkfriend channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NODFT);
      }
      else
      {
        send_to_char("Darkfriend channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NODFT);
      }
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
 
       	}

      REMOVE_BIT(ch->comm,COMM_NODFT);
	   
      if (!IS_EMOTE(argument))
      {
	   sprintf(buff,"{w[{RDarkfriend{w] {R(%d){W: {w%s '{X%s{x'\n\r",ch->pcdata->df_level,ch->pcdata->df_name,argument);
      }
      else
	   {  strcpy(cemotebuf,&argument[1]);
	      cemotebuf[strlen(cemotebuf)-1] = '\0';
	      sprintf(buff,"{w[{RDarkfriend{w] {R(%d){W: {w'{w%s {X%s{x'\n\r",ch->pcdata->df_level,ch->pcdata->df_name,cemotebuf);
	   }
      

      if (!IS_NPC(ch)) {
         do_addchannelhistory(ch,df_history,argument, ch->pcdata->df_name);
      }
      else {
         do_addchannelhistory(ch,df_history,argument, NULL);
      }
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        CHAR_DATA *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             !IS_SET(victim->comm,COMM_NODFT) &&
             !IS_SET(victim->comm,COMM_QUIET) &&
	     victim->pcdata->df_level >= 0)
        {
	     send_to_char(buff,victim);	
        }
      }
    }
}

void do_gamec( CHAR_DATA *ch, char *argument )
{
    char cemotebuf[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOGAME))
      {
        send_to_char("Game channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOGAME);
      }
      else
      {
        send_to_char("Game channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOGAME);
      }
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET)) {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
 
       	}

      if (IS_SET(ch->comm,COMM_NOGAME))
      {
        send_to_char("Turn the GAME channel on before using it.\n\r",ch);
	return;
      }
      if (IS_AFFECTED(ch,AFF_GAGGED))
      {
        send_to_char( "Something prevents you from speaking. It feels as though you've been gagged.\n\r", ch );
        return;
      }

	 // Make sure no cursing etc. on public channels
	 censor_speech(ch, argument);

      if (!IS_EMOTE(argument))
      {
          act_old("{w[{jGame{w]{W: {w$n {w'{j$t{x'",ch, colorcontinue('j', argument),NULL,TO_CHAR,POS_SLEEPING);
	  sprintf(buffer,"Game: {R%s '%s{x'",ch->name,colorcontinue('R', argument));
      }
      else
      {  strcpy(cemotebuf,&argument[1]);
         cemotebuf[strlen(cemotebuf)-1] = '\0';
         act_old("{w[{jGame{w]{W: '{w$n {j$t{x'",ch,colorcontinue('j', cemotebuf),NULL,TO_CHAR,POS_SLEEPING);
	 sprintf(buffer,"Game: {R%s '%s{x'",ch->name,colorcontinue('R', cemotebuf));
		 
      }
      

      log_comm_string("game",buffer);
      do_addchannelhistory(ch,game_history,argument, NULL);
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        CHAR_DATA *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOGAME) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
          if (!IS_EMOTE(argument))
          {   act_old( "{w[{jGame{w]{W: {w$n '{j$t{x'", 
		   ch,colorcontinue('j', argument), d->character, TO_VICT,POS_SLEEPING ); 
          }
          else
          {  
              act_old( "{w[{jGame{w]{W: '{w$n {j$t{x'", 
		   ch,colorcontinue('j', cemotebuf), d->character, TO_VICT,POS_SLEEPING ); 
          }

        }
      }
    }
}

void do_newbie( CHAR_DATA *ch, char *argument )
{
    char cemotebuf[MAX_STRING_LENGTH];
    char buffer[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (!IS_NPC(ch) && !IS_NEWBIE(ch) && !IS_IMMORTAL(ch) && !IS_NEWBIEHELPER(ch) ) {
       send_to_char("You can't use this channel anymore. Use the Game channel for questions related to the game.\n\r", ch);
       return;
    }

    if (argument[0] == '\0' ) 
    {
       if (IS_SET(ch->comm,COMM_NONEWBIE)) {
         send_to_char("Newbie channel is now ON.\n\r",ch);
         REMOVE_BIT(ch->comm,COMM_NONEWBIE);
       }
       else {
         send_to_char("Newbie channel is now OFF.\n\r",ch);
         SET_BIT(ch->comm,COMM_NONEWBIE);
       }
       return;
     }
     else 
     {
         if (IS_SET(ch->comm,COMM_QUIET)) {
           send_to_char("You must turn off quiet mode first.\n\r",ch);
           return;
         }
     }
   
/* Allow unvalidated players to use this channel
        if (IS_SET(ch->comm,COMM_NOCHANNELS)) {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
        }
*/

	 // Make sure no cursing etc. on public channels
      REMOVE_BIT(ch->comm,COMM_NONEWBIE);
      if (!IS_NPC(ch))
	 censor_speech(ch, argument);
	   
      if (!IS_EMOTE(argument))
      {
         if (IS_NEWBIEHELPER(ch)) {
          act_old("{w[{zNewbie{w]{W: {x({YNH{x) $n '{z$t{x'",ch,colorcontinue('z', argument),NULL,TO_CHAR,POS_SLEEPING);
         }
         else {
          act_old("{w[{zNewbie{w]{W: {x$n '{z$t{x'",ch,colorcontinue('z', argument),NULL,TO_CHAR,POS_SLEEPING);
        }
	sprintf(buffer,"Newbie: {Y%s '%s{x'",ch->name,colorcontinue('Y', argument));
      }
      else {  
         strcpy(cemotebuf,&argument[1]);
         cemotebuf[strlen(cemotebuf)-1] = '\0';
         if (IS_NEWBIEHELPER(ch)) {
          act_old("{w[{zNewbie{w]{W: {x({YNH{x) {w'{z$n {z$t{x'",ch,colorcontinue('z', cemotebuf),NULL,TO_CHAR,POS_SLEEPING);
         }
         else {
          act_old("{w[{zNewbie{w]{W: {w'{z$n {z$t{x'",ch,colorcontinue('z', cemotebuf),NULL,TO_CHAR,POS_SLEEPING);
        }
	sprintf(buffer,"<Newbie: {Y%s '%s{x'",ch->name,colorcontinue('Y', cemotebuf));
      }
      

      log_comm_string("newbie",buffer);
      do_addchannelhistory(ch,newbie_history,argument, NULL);
      for ( d = descriptor_list; d != NULL; d = d->next ) {
        CHAR_DATA *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NONEWBIE) &&
             !IS_SET(victim->comm,COMM_QUIET) &&
             (IS_NEWBIE(victim) || IS_IMMORTAL(victim) || IS_NEWBIEHELPER(victim))) {
          if (!IS_EMOTE(argument)) {
             if (IS_NEWBIEHELPER(ch)) {
                act_old( "{w[{zNewbie{w]{W: {x({YNH{x) {x$n {w'{z$t{x'", 
		         ch,colorcontinue('z', argument), d->character, TO_VICT,POS_SLEEPING ); 
             }
             else {            
               act_old( "{w[{zNewbie{w]{W: {x$n {w'{z$t{x'", 
		         ch,colorcontinue('z', argument), d->character, TO_VICT,POS_SLEEPING ); 
             }
          }
          else { 
            if (IS_NEWBIEHELPER(ch)) {
             act_old( "{w[{zNewbie{w]{W: {x({YNH{x) {w'{z$n {z$t{x'", 
		       ch,colorcontinue('z', cemotebuf), d->character, TO_VICT,POS_SLEEPING );             	
            } 
            else {
             act_old( "{w[{zNewbie{w]{W: {w'{z$n {z$t{x'", 
		       ch,colorcontinue('z', cemotebuf), d->character, TO_VICT,POS_SLEEPING ); 
	    }
          }

        }
      }
}

void do_tellh(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *och;
  CHAR_DATA *och_next;
  int i;
  char line_detail[MAX_STRING_LENGTH];
  /* int first=TRUE; */
  int entry=FALSE;
  
  /*
   *  Do Tell history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (ch->pcdata->tell_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  ch->pcdata->tell_history[i].wizinvis_level) ||
		  ( (ch->pcdata->tell_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		   if(ch->pcdata->tell_history[i].sent_comm){
			   sprintf (line_detail,"You told Someone at {y%s {k'%s'{p\n\r",
					    sec2strtime(ch->pcdata->tell_history[i].when),
					    ch->pcdata->tell_history[i].line_data);
		   } else {
			   sprintf (line_detail,"Someone told you at {y%s {k'%s'{p\n\r",
			   			sec2strtime(ch->pcdata->tell_history[i].when),
			   			ch->pcdata->tell_history[i].line_data);
		   }
	   }
	   else {
		   if(ch->pcdata->tell_history[i].sent_comm){
			   sprintf(line_detail,"You told %s at {y%s {k'%s'{p\n\r",
					   ch->pcdata->tell_history[i].player_name,
					   sec2strtime(ch->pcdata->tell_history[i].when),
					   ch->pcdata->tell_history[i].line_data);
		   } else {
			   sprintf(line_detail,"%s told you at {y%s {k'%s'{p\n\r",
					   ch->pcdata->tell_history[i].player_name,
			   		   sec2strtime(ch->pcdata->tell_history[i].when),
			   		   ch->pcdata->tell_history[i].line_data);
		   }
	    };
	    send_to_char(line_detail,ch);
	 };     
    };
  };

  if (!entry)
      send_to_char("No history found.\n\r", ch);
};


void do_addchannelhistory(CHAR_DATA *ch, HISTORY_DATA *history, char * argument, char *use_name)
{
  int i;
  
  for (i=0;i<HISTSIZE-1;i++) {
    if (history[i+1].player_name) {
	 if (history[1].player_name) {   
	   free_string(history[i].player_name);
	   free_string(history[i].line_data);	   
	 }
	    
	 history[i].player_name    = str_dup (history[i+1].player_name);
	 history[i].line_data      = str_dup (history[i+1].line_data);
	 history[i].wizinvis_level = history[i+1].wizinvis_level;
	 history[i].invis_flag     = history[i+1].invis_flag;	 
	 history[i].when           = history[i+1].when;
    }
  }
  
  if (history[HISTSIZE-1].player_name) {   
    free_string(history[HISTSIZE-1].player_name);
    free_string(history[HISTSIZE-1].line_data);
  }
  
  history[HISTSIZE-1].line_data   = str_dup(argument);
  
  if (!IS_NULLSTR(use_name))
     history[HISTSIZE-1].player_name = str_dup(use_name);
  else
     history[HISTSIZE-1].player_name = str_dup(COLORNAME(ch));

  if (IS_AFFECTED(ch,AFF_INVISIBLE)) {
    history[HISTSIZE-1].invis_flag = 1;
  }
  else
    history[HISTSIZE-1].invis_flag = 0;
  /* history[HISTSIZE-1].wizinvis_level = ch->incog_level; */
  history[HISTSIZE-1].wizinvis_level = ch->invis_level;

  // Keep track of time
  history[HISTSIZE-1].when = current_time;

}

void do_chath(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  int entry=FALSE;
  
  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (chat_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  chat_history[i].wizinvis_level) ||
		  ( (chat_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {

		if (!IS_EMOTE(chat_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{fChat{y %s{w]{W: {wSomeone '{f%s{x'\n\r", sec2strtime(chat_history[i].when), chat_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&chat_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{fChat{y %s{w]{W: '{fSomeone {f%s{x'\n\r", sec2strtime(chat_history[i].when),cemotebuf);
		}
	   }
	   else {
		if (!IS_EMOTE(chat_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{fChat{y %s{w]{W: {w%s '{f%s{x'\n\r",   
				 sec2strtime(chat_history[i].when),
				 chat_history[i].player_name, 
				 chat_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&chat_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{fChat{y %s{w]{W: '{f%s {f%s{x'\n\r",
				 sec2strtime(chat_history[i].when),
				 chat_history[i].player_name, cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
 };

void do_dftalkh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  int entry=FALSE;
  
    if (ch->pcdata->df_level < 0)
    {
        send_to_char("You aren't a friend of the dark.\n\r",ch);
	return;
    }
  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (df_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  df_history[i].wizinvis_level) ||
		  ( (df_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {

		if (!IS_EMOTE(df_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{RDarkfriend{y %s{w]{W: {wSomeone '{X%s{x'\n\r", sec2strtime(df_history[i].when), df_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&df_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{RDarkfriend{y %s{w]{W: '{XSomeone {X%s{x'\n\r", sec2strtime(df_history[i].when),cemotebuf);
		}
	   }
	   else {
		if (!IS_EMOTE(df_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{RDarkfriend{y %s{w]{W: {w%s '{X%s{x'\n\r",   
				 sec2strtime(df_history[i].when),
				 df_history[i].player_name, 
				 df_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&df_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{RDarkfriend{X %s{w]{W: '{X%s {X%s{x'\n\r",
				 sec2strtime(df_history[i].when),
				 df_history[i].player_name, cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
 };

void do_glth(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  int entry=FALSE;
  
  /*
   *  Do channel history
   */
  if ((!is_leader(ch)) && (!IS_IMMORTAL(ch))) {
    send_to_char("Huh?\n\r",ch);
    return;
  }
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (guildleader_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  guildleader_history[i].wizinvis_level) ||
		  ( (guildleader_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(guildleader_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{fGUILDLEADER{y %s{w]{W: {wSomeone '{f%s{x'\n\r",   
				 sec2strtime(guildleader_history[i].when),
				 guildleader_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&guildleader_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{fGUILDLEADER{y %s{w]{W: '{fSomeone {f%s{x'\n\r",sec2strtime(guildleader_history[i].when), cemotebuf);		  
		}
	   }
	   else {
		if (!IS_EMOTE(guildleader_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{fGUILDLEADER{y %s{w]{W: {w%s '{f%s{x'\n\r",   
				 sec2strtime(guildleader_history[i].when),
				 guildleader_history[i].player_name, 
				 guildleader_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&guildleader_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{fGUILDLEADER{y %s{w]{W: '{f%s {f%s{x'\n\r",
				 sec2strtime(guildleader_history[i].when),
				 guildleader_history[i].player_name, cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

void do_gamech(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  int entry=FALSE;
  
  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (game_history[i].player_name) {
	   entry=TRUE;

	   if ((get_trust(ch) <  game_history[i].wizinvis_level) ||
		  ( (game_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {

		if (!IS_EMOTE(game_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{jGame{y %s{w]{W: {wSomeone '{j%s{x'\n\r", sec2strtime(game_history[i].when), game_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&game_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{jGame{y %s{w]{W: '{jSomeone {j%s{x'\n\r", sec2strtime(game_history[i].when), cemotebuf);		  
		}
	   }
	   else {
		if (!IS_EMOTE(game_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{jGame{y %s{w]{W: {w%s '{j%s{x'\n\r",   
				 sec2strtime(game_history[i].when),
				 game_history[i].player_name, 
				 game_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&game_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{jGame{y %s{w]{W: '{j%s {j%s{x'\n\r",
				 sec2strtime(game_history[i].when),
				 game_history[i].player_name, cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

void do_prayh (CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  /* char cemotebuf[MAX_STRING_LENGTH]; */
  int entry=FALSE;
  
  line_detail[0] = '\0';
  
  for (i=0;i<PRAYHISTSIZE;i++) {
    if (ch->pcdata) {
	 if (pray_history[i].player_name) {
	   entry=TRUE;
	   sprintf (line_detail,"{w[{WPray{y %s{w]{W:{x (%s){W: %s{x\n\r",   
			  sec2strtime(pray_history[i].when),
			  pray_history[i].player_name, 
			  pray_history[i].line_data);
	   
	 }
    }
    send_to_char(line_detail,ch);
    line_detail[0] = '\0';
  }
  
  if (!entry)
    send_to_char("No history found.\n\r", ch);    
}

void do_immh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  int entry=FALSE;
  
  /*
   *  Do channel history
   */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (imm_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  imm_history[i].wizinvis_level) ||
		  ( (imm_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(imm_history[i].line_data)) { 
		  sprintf (line_detail,"{c[{iImmortal{y %s{c]{W:{x Someone '{i%s{x'\n\r",   
				 sec2strtime(imm_history[i].when),
				 imm_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&imm_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{c[{iImmortal{y %s{c]{W:{x '{iSomeone {i%s{x'\n\r",sec2strtime(imm_history[i].when),cemotebuf);		  
		}
	   }
	   else {
		if (!IS_EMOTE(imm_history[i].line_data)) { 
		  sprintf (line_detail,"{c[{iImmortal{y %s{c]{W:{x %s '{i%s{x'\n\r",   
				 sec2strtime(imm_history[i].when),
				 imm_history[i].player_name, 
				 imm_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&imm_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{c[{iImmortal{y %s{c]{W:{x '{i%s {i%s{x'\n\r",
				 sec2strtime(imm_history[i].when),
				 imm_history[i].player_name, cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
   };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
 };

void do_gossiph(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  int entry=FALSE;
  
  /*
   *  Do channel history
     */
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (gossip_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  gossip_history[i].wizinvis_level) ||
	       ( (gossip_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(gossip_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{mGossip{y %s{x]{W: {wSomeone '{M%s{x'\n\r",   
				 sec2strtime(gossip_history[i].when),
				 gossip_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&gossip_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{mGossip{y %s{x]{W: '{MSomeone {M%s{x'\n\r",sec2strtime(gossip_history[i].when),cemotebuf);
		}
	   }
	   else {
		if (!IS_EMOTE(gossip_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{mGossip{y %s{x]{W: {w%s '{M%s{x'\n\r",   
				 sec2strtime(gossip_history[i].when),
				 gossip_history[i].player_name, 
				 gossip_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&gossip_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{mGossip{y %s{x]{W: '{M%s {M%s{x'\n\r",
				 sec2strtime(gossip_history[i].when),
				 gossip_history[i].player_name, cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

void do_newbieh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  int entry=FALSE;
  
  /*
   *  Do channel history
   */
  
  if (!IS_NEWBIE(ch) && !IS_IMMORTAL(ch) && !IS_NEWBIEHELPER(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (newbie_history[i].player_name) {
	   entry=TRUE;
	   if ((get_trust(ch) <  newbie_history[i].wizinvis_level) ||
		  ( (newbie_history[i].invis_flag == 1) && 
		    ((IS_AFFECTED(ch, AFF_DETECT_INVIS)) ||
			(IS_SET(ch->act,PLR_HOLYLIGHT))))) {
		if (!IS_EMOTE(newbie_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{zNewbie{y %s{w]{W: {wSomeone '{z%s{x'\n\r",   
				 sec2strtime(newbie_history[i].when),
				 newbie_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&newbie_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{zNewbie{y %s{w]{W: '{zSomeone {z%s{x'\n\r",sec2strtime(newbie_history[i].when),cemotebuf);		  
		}
	   }
	   else {
		if (!IS_EMOTE(newbie_history[i].line_data)) { 
		  sprintf (line_detail,"{w[{zNewbie{y %s{w]{W: {w%s '{z%s{x'\n\r",   
				 sec2strtime(newbie_history[i].when),
				 newbie_history[i].player_name, 
				 newbie_history[i].line_data);
		}
		else {  
		  strcpy(cemotebuf,&newbie_history[i].line_data[1]);
		  cemotebuf[strlen(cemotebuf)-1] = '\0';
		  sprintf( line_detail,"{w[{zNewbie{y %s{w]{W: '{z%s {z%s{x'\n\r",
				 sec2strtime(newbie_history[i].when),
				 newbie_history[i].player_name, cemotebuf);
		}
	   };
	   send_to_char(line_detail,ch);
	 };     
    };
  };
  if (!entry)
    send_to_char("No history found.\n\r", ch);
};

void do_bondwhere( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *bch;
  for ( bch = char_list; bch != NULL; bch = bch->next ) {
    if (!IS_NPC(bch)) {
	 if ( (is_bonded_to( ch, bch ) && (ch != bch)) )
	   do_bondfind( ch, bch->name );
    }
  }  
}

bool is_bonded_to( CHAR_DATA *ach, CHAR_DATA *bch )
{
   if (ach == bch)  //One can always see ones own mind
   {
	return TRUE;
   };


   if (ach->pcdata->bondedby[0] != '\0' )             //Sender is bonded
   {
      if (!str_cmp(ach->pcdata->bondedby, bch->name) )  //A bonded by B 
      {
	return TRUE; 
      }
      if (bch->pcdata->bondedby[0] != '\0') //Sender is bonded, receiver is possible
      {
	if (!str_cmp(ach->pcdata->bondedby, bch->pcdata->bondedby)) //Bonded by same person
        {
	   return TRUE;
        };
      }
   }
   if (bch->pcdata->bondedby[0] != '\0')  //Possible recipient
   {
       if (!str_cmp(bch->pcdata->bondedby, ach->name) )  //bonded by  person
       {
           return TRUE;
       }
   }
   return FALSE;
}

void do_bondfind( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  int direction;
  bool fArea;

  one_argument( argument, arg );

  /* only imps can hunt to different areas */
  fArea = ( get_trust(ch) < MAX_LEVEL );

  if( fArea )
    victim = get_char_area( ch, arg);
  else
    victim = get_char_world( ch, arg);

  if( victim == NULL )
    {
      send_to_char("No-one around by that name.\n\r", ch );
      return;
    }

  if( ch->in_room == victim->in_room )
    {
      act( "$N is here!", ch, NULL, victim, TO_CHAR );
      return;
    }

  direction = find_path( ch->in_room->vnum, victim->in_room->vnum,
			ch, -40000, fArea );

  if( direction == -1 )
    {
      act( "You couldn't find a path to $N from here.",
	  ch, NULL, victim, TO_CHAR );
      return;
    }

  if( direction < 0 || direction > 9 )
    {
      send_to_char( "Hmm... Something seems to be wrong.\n\r", ch );
      return;
    }

  /*
   * Display the results of the search.
   */
  sprintf( buf, "$N is %s from here.", dir_name[direction] );
  act( buf, ch, NULL, victim, TO_CHAR );

  return;
}

void do_bondtell( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  CHAR_DATA *bch;
  
  if ( argument[0] == '\0' ) {
    send_to_char( "Tell your bonded what?\n\r", ch );
    return;
  }
  
  if ( IS_SET( ch->comm, COMM_NOTELL ) ) {
    send_to_char( "Your message didn't get through!\n\r", ch );
    return;
  }
  
  if (IS_SET(ch->comm,COMM_QUIET)) {
     send_to_char("You must turn off quiet mode first.\n\r",ch);
     return;
  }  

//    if (IS_DRUNK(ch)) drunk_speech(argument);

  /*
   * Note use of send_to_char, so gtell works on sleepers.
   */
  if (!IS_EMOTE(argument)) {
    sprintf( buf, "{D[{WBOND{D]{W: {s%s {x'{s%s{x'.\n\r",COLORNAME(ch), colorcontinue('s', argument) );
  }
  else {  
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf( buf, "{D[{WBOND{D]{W: {s%s {s%s{x\n\r",COLORNAME(ch), colorcontinue('s', cemotebuf));
  }
  
  for ( bch = char_list; bch != NULL; bch = bch->next ) {
    if (!IS_NPC(bch)) {
	 if ( is_bonded_to( ch, bch ) ) {
	   send_to_char( buf, bch );
	   do_addchannelhistory(ch,bch->pcdata->bond_history,argument, NULL);
	 }
    }
  }
  
  return;
}

void do_directedbondtell( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char cemotebuf[MAX_STRING_LENGTH];
  CHAR_DATA *bch;
  char arg[MAX_INPUT_LENGTH];

  argument = one_argument(argument, arg);
    
  if ( argument[0] == '\0' ) {
    send_to_char( "Tell your bonded what?\n\r", ch );
    return;
  }
  
  if ( IS_SET( ch->comm, COMM_NOTELL ) ) {
    send_to_char( "Your message didn't get through!\n\r", ch );
    return;
  }
  
  if (IS_SET(ch->comm,COMM_QUIET)) {
     send_to_char("You must turn off quiet mode first.\n\r",ch);
     return;
  }  

  if ((bch = get_char_anywhere(ch,arg)) == NULL)
  {
	send_to_char("They aren't here.\r\n",ch);
	return;
  }

  if (IS_NPC(bch))
  {
	send_to_char("Not to a mob.\r\n",ch);
	return;
  }

  if (!is_bonded_to(ch,bch))
  {
	send_to_char("You aren't bonded to them.\r\n",ch);
	return;
  }
  if ( bch->desc == NULL && !IS_NPC(bch)) {
         act_old("$N seems to have misplaced $S link...try again later.", ch,NULL,bch,TO_CHAR, POS_DEAD);
 	 return;
  }

  /*
   * Note use of send_to_char, so gtell works on sleepers.
   */
  if (!IS_EMOTE(argument)) {
    sprintf( buf, "{D[{WBOND{D]{W: {s%s {x'{s%s{x'.\n\r",COLORNAME(ch), colorcontinue('s', argument) );
  }
  else {  
    strcpy(cemotebuf,&argument[1]);
    cemotebuf[strlen(cemotebuf)-1] = '\0';
    sprintf( buf, "{D[{WBOND{D]{W: {s%s {s%s{x\n\r",COLORNAME(ch), colorcontinue('s', cemotebuf));
  }
  
  send_to_char( buf, ch );
  do_addchannelhistory(ch,ch->pcdata->bond_history,argument, NULL);
  send_to_char( buf, bch );
  do_addchannelhistory(ch,bch->pcdata->bond_history,argument, NULL);
  
  return;
}

void do_bondtellh(CHAR_DATA *ch, char *argument)
{
  int i;
  char line_detail[MAX_STRING_LENGTH];
  /* char cemotebuf[MAX_STRING_LENGTH]; */
  int entry=FALSE;
  
  line_detail[0] = '\0';
  
  for (i=0;i<HISTSIZE;i++) {
    if (ch->pcdata) {
	 if (ch->pcdata->bond_history[i].player_name) {
	   entry=TRUE;
	   sprintf (line_detail,"{D[{WBOND{y %s{D]{W:{x (%s){W: %s{x\n\r",   
			  sec2strtime(ch->pcdata->bond_history[i].when),
			  ch->pcdata->bond_history[i].player_name, 
			  ch->pcdata->bond_history[i].line_data);
	   
	 }
    }
    send_to_char(line_detail,ch);
    line_detail[0] = '\0';
  }
  
  if (!entry)
    send_to_char("No history found.\n\r", ch);
}

void do_bondnotify( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *bch;
  
  if ( argument[0] == '\0' ) {
    return;
  }
  
  /*
   * Note use of send_to_char, so gtell works on sleepers.
   */
  if (!str_cmp(argument,"arriving") ) {   
    sprintf( buf, "{WYou feel your bonded step into the pattern.{p\n\r" );
  }
  else if (!str_cmp(argument, "subdued")) {
     sprintf(buf, "{RYou feel your bonded has been subdued!{x\n\r");
  }
  else {
    sprintf( buf, "{WYou feel your bonded leave the pattern.{p\n\r");
  }

  for ( bch = char_list; bch != NULL; bch = bch->next ) {
    if (!IS_NPC(bch)) {
	 if ( is_bonded_to( ch, bch ) && (ch != bch) )
	   send_to_char( buf, bch );
    }
  }
  
  return;
}

void do_profile(CHAR_DATA * ch, char *argument)
{
  PROFILE_DATA *prof;
  BUFFER *output;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char read[MAX_INPUT_LENGTH];
  char fname[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char *ptr, *text;
  FILE *fp;
  bool find_more = TRUE;
  bool found=FALSE;
  
  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg2);
  
  if (arg[0] == '\0') {
    send_to_char("Syntax: profile <player> [full/search] [search string]\n\r", ch);
    return;
  }
  
  sprintf( fname, "%s%s", PLAYER_DIR, capitalize( arg ) );
    
  
  if ( ( fp = fopen( fname, "r" ) ) == NULL ) {
    send_to_char( "That player does not exist.\n\r", ch );
    return;
  }

  /* Print all elements in the pfile: Atwain */
  if (!str_cmp(arg2, "full") && (get_trust(ch) >= IMPLEMENTOR)) {
    output = new_buf();
    while(fgets(read, 80, fp)) {
	 add_buf(output, read);
    }
    page_to_char( buf_string(output), ch );
    free_buf(output);
    fclose(fp);
    return;
  }

  /* Loose pfile search with str_prefix */
  if (!str_cmp(arg2, "search")) {
    if (argument[0] == '\0') {
	 send_to_char("What do you want to search the pfile for ?\n\r", ch);
	 fclose(fp);
	 return;
    }
    output = new_buf();
    while(fgets(read, 80, fp)) {
	 if (!str_prefix(argument, read)) {
	   found = TRUE;
	   add_buf(output, read);
	 }
    }
    if (!found)
	 add_buf(output, "No match found in pfile.\n\r");

    page_to_char( buf_string(output), ch );
    free_buf(output);
    fclose(fp);
    return;
  }
  
  prof = (PROFILE_DATA *) alloc_mem( sizeof( PROFILE_DATA ) );
  bzero( prof, sizeof( PROFILE_DATA ) );
  
  read[0] = 0;
  
  while( find_more && (fgets( read, 80, fp  ) != NULL) )  {
    int len;
    len = strlen( read );
    if( read[len-2] == '~' )
	 read[len-2] = '\0';
    
    bzero( buf, MAX_INPUT_LENGTH );
    ptr = read; text = buf;
    while( *ptr != ' ' )   /* Grab the keyword only */
	 *text++ = *ptr++ ;
    
    /*
	* The extra checks are added to ensure the entire pfile
	* isnt read (its huge usually).  This allows you to add
	* more profile stuff easily as well.  THe structure is
	* the same as in reading a pfile.
	*/ 
    switch( buf[0] ) {
    default:  continue;
    case 'A':
	 if( !strcmp( buf, "Alias" ) ) /* Done looking */ {
	   find_more = FALSE;
	   break; 
	 }

    case 'C':
	 if( !strcmp( buf, "Colors" ) ) /* Done looking */ {
	   find_more = FALSE;
	   break; 
	 }
	 if( !strcmp( buf, "Clan" ) ) {
	   sprintf( prof->guild, "Guild: %s\n\r", read );
	   break;
	 }
	 break;

    case 'E':
	 if( !strcmp( buf, "Email" ) && (get_trust(ch) == IMPLEMENTOR)) {
	   sprintf( prof->email, "%s\n\r", read );
	   break;
	 }
	 
	 if( !strcmp( buf, "ELevl" ) ) {
	   sprintf( prof->elevel,"%s\n\r", read );
	   break;
	 }
	 
	 if( !strcmp( buf, "End" ) ) /* Done looking */ {
	   find_more = FALSE;
	   break; 
	 }
	 break;
	 
    case 'G':
	 if( !strcmp( buf, "Gtitle" ) ) /* Done looking */ {
	   sprintf( prof->guildtitle,"%s\n\r", read); 
	   break; 
	 }
    case 'I':
	 if( !strcmp( buf, "Insanity" ) ) /* Done looking */ {
	   sprintf( prof->insanity,"%s\n\r", read); 
	   break; 
	 }
	 break;
    case 'L':
	 if( !strcmp( buf, "Levl" ) ) {
	   sprintf( prof->level,"%s", read );
	   break;
	 }
	 
	 if( !strcmp( buf, "Lastsite" )  && (get_trust(ch) >= IMPLEMENTOR)) {
	   sprintf( prof->lastsite, "%s\n\r", read );
	   break;
	 }
	 
	 if( !strcmp( buf, "Lastlog" ) ) {
	   sprintf( prof->lastlog, "%s\n\r", read );
	   break;
	 }
	 break;

	 
	 /* This will also read names on objects in inventory - Atwain */
	 /* If file is found we already have name in arg */
    case 'N':
	 if( !strcmp( buf, "Name" ) ) {
	   sprintf( prof->name,"%s\n\r", read );
	   break;
	 }
	 break;

    case 'P':
	 if( !strcmp( buf, "Plan" ) ) {
	   sprintf( prof->plan, "%s\n\r", read );
	   break;
	 }
	 if( !strcmp( buf, "Plyd" ) ) {
           float act_hours = 0.0;
	   char buf2[128];
	   strcpy(buf2,&read[4]);
	   act_hours = atof(buf2)/3600;
	   sprintf( prof->hoursplayed, "Played: %f\n\r", act_hours );
	   break;
	 }
	 break;
	 
    case 'R':
	 if( !strcmp( buf, "Race" ) ) {
	   sprintf( prof->race, "%s\n\r",read);
	   break;
	 }
	 if( !strcmp( buf, "Rank" ) ) {
	   sprintf( prof->guildlevel, "Guild Level: %s\n\r",read);
	   break;
	 }
	 if( !strcmp( buf, "RolePlayed" ) ) {

           int rptime = 0;
     	   int hours = 0;
           int minutes = 0;
           int seconds = 0;
	   char buf2[128];
	   strcpy(buf2,&read[10]);
	   rptime = atoi(buf2);
           hours = rptime / 3600;
           minutes = (rptime - (hours * 3600) ) / 60;
           seconds = rptime - (hours * 3600) - (minutes * 60);
           sprintf(prof->hoursplayed, "RolePlayed : %02d:%02d:%02d\n\r", hours,minutes,seconds);

	   break;
	 }
	 break; 
	 
    case 'S':
	 if( !strcmp( buf, "Sex" ) ) {
	   int sex;
	   while( *ptr == ' ' ) ptr++; 
	   sex = atoi( ptr );
	   sprintf( prof->sex, "%s: %s\n\r", buf,(char *)sex_table[sex].name );
	   break;
	 }

	 if( !strcmp( buf, "Skill" ) ) /* Done looking */ {
	   find_more = FALSE;
	   break; 
	 }
	 if( !strcmp( buf, "Sphere" ) ) {
	   sprintf( prof->spheres, "%s\n\r",read);
	   break;
	 }
	 break;
	 
    }
    bzero( read, MAX_INPUT_LENGTH );
  }
  
  sprintf(prof->name, "Name: %s\n\r", capitalize( arg ));
  
  strcpy( buf, "\n{WProfile Information{x:\n\r" );
  strcat( buf, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r" );
  strcat( buf, prof->name     );
  strcat( buf, prof->sex      );
  strcat( buf, prof->race     );
  strcat( buf, prof->level    );
  strcat( buf, prof->elevel   );
  if ( get_trust( ch ) > HERO )
    strcat( buf, prof->lastsite );
  if ( get_trust(ch) > HERO )
    strcat( buf, prof->lastlog  );
  if ( get_trust(ch) > HERO )
    strcat( buf, prof->hoursplayed  );
  if ( get_trust(ch) > HERO )
    strcat( buf, prof->hoursroleplayed  );
  if ( get_trust(ch) > HERO )
    strcat( buf, prof->insanity  );
  strcat( buf, prof->email    );
  strcat( buf, prof->plan     );
  if ( get_trust(ch) > HERO )
    strcat( buf, prof->spheres  );
  strcat( buf, prof->guild    );
  strcat( buf, prof->guildtitle );
  strcat( buf, prof->guildlevel );
  strcat( buf, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r" );
  
  send_to_char(buf, ch);
  
  free_mem( prof, sizeof( PROFILE_DATA ) );
  fclose(fp);
}

void do_mymail( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  
  if (IS_NPC(ch))
     return;
  
  if (ch->pcdata->email[0]=='\0') {
    send_to_char("You have not provided any Email address.\n\r", ch);
    return;
  }
  
  if (IS_NULLSTR(argument)) {  
     sprintf(buf, "You have provided TSW with the following Email address: [%s]\n\r", ch->pcdata->email);
     send_to_char(buf, ch);
     return;
  }
  else {
     if (!(!str_cmp(argument,"none") || strstr(argument,"@"))) {
     	send_to_char("Error in Email address. Must include '{W@{x' or set to '{Wnone{x'\r\n", ch);
        return;
     }
     else {
       free_string(ch->pcdata->email);
       ch->pcdata->email = str_dup(argument);
       sprintf(buf, "Email address set to <%s>\n\r", ch->pcdata->email);
       send_to_char(buf, ch);
       return;
     }
  }
  return;     
}

void do_mutter(CHAR_DATA *ch, char *argument)
{
  static char * const him_her [] = { "it",  "him", "her" };
  char         word[MAX_INPUT_LENGTH];
  CHAR_DATA   *victim;
  CHAR_DATA   *vch;  
  CHAR_DATA   *vch_next;
  char         buf[MAX_INPUT_LENGTH];
  char         message[MAX_STRING_LENGTH];
  char         muffled[MAX_STRING_LENGTH];
  char         wkmuffled[MAX_STRING_LENGTH];
  char         *unmuffled;
  bool         lastMufd = TRUE;
  bool         lastwkMufd = TRUE;
  int          chance=0;
  int          wkchance=0;
  
  
  if (IS_NULLSTR(argument)) {
    send_to_char ("Usage: mutter <player> <message>\n\r", ch);
    return;
  }
  
  /* find target */
  argument = one_argument (argument, word);
  
  if ((victim = get_char_room (ch, word)) == NULL) {
    send_to_char ("They aren't here.\n\r", ch);
    return;
  }
  
  if (victim->position <= POS_SLEEPING) {
    send_to_char ("Perhaps now is not the most opportune time, they're not awake.\n\r", ch);
    return;
  }
  
  /* we need to save original message */
  unmuffled = argument;
  muffled[0] = '\0';
  wkmuffled[0] = '\0';
  
  while (*argument != '\0') {
    chance = number_range (1, 100);
    wkchance = number_range(1,40);
    
    argument = first_arg (argument, word, FALSE);
    
    /* chance of word being unheard */
    if (chance > 25) {
	 if (chance > 80)
	   strcat (muffled, "...");
	 else if (chance > 50)
	   strcat (muffled, "..");
	 else
	   strcat (muffled, ".");
	 lastMufd = TRUE;
    }
    else {
	 sprintf (buf, "%s%s", lastMufd ? "" : " ", word);
	 strcat (muffled, buf);
	 lastMufd = FALSE;
    }

    if (wkchance > 25) {
	 if (wkchance > 80)
	   strcat (wkmuffled, "...");
	 else if (wkchance > 50)
	   strcat (wkmuffled, "..");
	 else
	   strcat (wkmuffled, ".");
	 lastwkMufd = TRUE;
    }
    else {
	 sprintf (buf, "%s%s", lastwkMufd ? "" : " ", word);
	 strcat (wkmuffled, buf);
	 lastwkMufd = FALSE;
    }    
  }
  
  /* send it out as appropriate     */
  /* IF Wolfkin, send out wkmuffled */
  /* ELSE, send out normal muffled  */
  if (ch == victim) {
    sprintf (message, "You mutter to your self, '{7%s{x'", unmuffled);
    act_new (message, ch, NULL, victim, TO_CHAR, POS_RESTING);

    /* Wolfkin check */
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
	 vch_next	= vch->next_in_room;
	 if (IS_NPC(vch))
	   continue;
	 if (vch == ch)
	   continue;
	 if (vch == victim)
	   continue;
	 if (IS_WOLFKIN(vch)) {
	   sprintf (message, "%s mutters to %sself, '{7%s{x'", PERS(ch, vch), him_her[URANGE(0, ch->sex, 2)], wkmuffled);
	   act_new (message, vch, NULL, victim, TO_CHAR, POS_RESTING);
	 }
	 else {
	   sprintf (message, "%s mutters to %sself, '{7%s{x'",PERS(ch, vch), him_her[URANGE(0, ch->sex, 2)], muffled);
	   act_new (message, vch, NULL, victim, TO_CHAR, POS_RESTING);
	 }
    }
    
    // Old before WK better hearing
    //sprintf (message, "$n mutters to $mself, '{7%s{x'", muffled);
    //act_new (message, ch, NULL, victim, TO_NOTVICT, POS_RESTING);
  }
  else {
    sprintf (message, "You mutter to $N, '{7%s{x'", unmuffled);
    act_new (message, ch, NULL, victim, TO_CHAR, POS_RESTING);
    
    sprintf (message, "$n mutters to you, '{7%s{x'", unmuffled);
    act_new (message, ch, NULL, victim, TO_VICT, POS_RESTING);

    /* Wolfkin check */
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next ) {
	 vch_next	= vch->next_in_room;
	 if (IS_NPC(vch))
	   continue;
	 if (vch == ch)
	   continue;
	 if (vch == victim)
	   continue;
	 if (IS_WOLFKIN(vch)) {
	   sprintf (message, "%s mutters to",PERS(ch,vch));
 	   sprintf (message, "%s %s,{7%s{x'",message, PERS(victim,vch),wkmuffled);
	   //sprintf (message, "%s mutters to %s, '{7%s{x'", PERS(ch, vch), PERS(victim,vch),wkmuffled);
	   //sprintf (message, "$n mutters to $N, '{7%s{x'", wkmuffled);
	   act_new (message, vch, NULL, victim, TO_CHAR, POS_RESTING);
	 }
	 else {
	   sprintf (message, "%s mutters to",PERS(ch,vch));
 	   sprintf (message, "%s %s,{7%s{x'",message, PERS(victim,vch),muffled);
	   //sprintf (message, "%s mutters to %s, '{7%s{x'", PERS(ch, vch), PERS(victim,vch),muffled);
	   //sprintf (message, "$n mutters to $N, '{7%s{x'", muffled);
	   act_new (message, vch, NULL, victim, TO_CHAR, POS_RESTING);
	 }
    }

    // Old before WK better hearing
    //sprintf (message, "$n mutters to $N, '{7%s{x'", muffled);
    //act_new (message, ch, NULL, victim, TO_NOTVICT, POS_RESTING);
  }
  
  return;
}

/* 
 * My wonderful censoring routine... don't try to understand the algorithm,
 * it will give you a headache.  - Zork 7/27/95 
 */
void censor_speech( CHAR_DATA *ch, char *text )
{  

   //for now, disable the censorship
   //return;
  char *lower(char *s) {
    static char c[1000];
    int i=0;    
    
    strcpy(c, s);
    while (c[i]) {
      c[i] = LOWER(c[i]);
      i++;
    }
    return(c);
  }
  
#define NUM_BAD_WORDS 12
  
  /* I suppose this could be called a politically correct routine... hehe */
  
  const char *bad_words[NUM_BAD_WORDS * 2] = {
    /* bad word           translation */
    "fuck",             "squack",
    "shit",             "shoot",
    "dick",             "duck",
    "bitch",            "witch",
    "cunt",             "trolloc",		/* 5 */
    "whore",            "madame",
    "bastard",          "illegitimate son",
    "asshole",          "nice person",
    /*    "ass",              "donkey",   took out ass until have time
		to fix it triggering other words */
    "god damn",         "gosh darn",    /* 10 */
    "damn",             "darn",
    "cock",             "stanly",
    "slut",             "madame"
  };
  
  char *a;
  char b[1000];
  char c[1000];
  int diff, curr;
  bool found = FALSE;
  a = text;
  
  for(curr = 0; curr < (NUM_BAD_WORDS * 2); curr += 2) {
    b[0] = '\0';
    if (strstr(lower(a), bad_words[curr])) {
      found = TRUE;
      while (strstr(lower(a), bad_words[curr])) {
        diff = strlen(a) - strlen((char *)strstr(lower(a), bad_words[curr]));
        strncat(b, a, diff);
        strcat(b, bad_words[curr+1]);
        a += diff + strlen(bad_words[curr]);
      }
    }
    strcat(b, a);
    a = strcpy(c, b);
  }
  
  if (found) {
    char buf[MSL];
    int fee=500; /* 500 silver fined */    
    sprintf(buf, "{RYou are fined {W%d S{xi{Wlver{R for use of inappropriate language on a public channel{x.\n\r", fee);
    send_to_char(buf, ch);
    if (ch->silver > fee)
      ch->silver -= fee;
    else if (!IS_NPC(ch) && ch->pcdata->silver_bank > fee)
	 ch->pcdata->silver_bank -= fee;
    else if (ch->gold*100 > fee)
      ch->gold -= 1;
    else if (!IS_NPC(ch) && ch->pcdata->gold_bank*100 > fee)
	 ch->pcdata->gold_bank -= 1;
    else
      ch->silver = 0;
  }
  
  strcpy(text,a);
}

void do_whochannels(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *pcs[MAX_PC_ONLINE]; // 150 PC's
  BUFFER *output;
  CHAR_DATA *wch;
  int cnt=0;
  int i=0;
  int fillers=0;
  
  output = new_buf();
  
  /* Count PCs online and init pcs array to point to wch */
  for ( wch = char_list; wch != NULL; wch = wch->next ) {

    // No NPCs
    if (IS_NPC(wch)) 
	 continue;

    // No Imms
    //if (IS_IMMORTAL(wch))
    // continue;

    pcs[cnt++] = wch;
  }
  
  /* sort PCS array */  
  qsort (pcs, cnt, sizeof(wch), compare_char_names);

  sprintf(buf, "%-16s      {fChat    {jGame    {8Guild   {NOGuild  {SSGuild  {USSGuild {zNewbie  {dGossip  {aAuction {eMusic   {vRace    {kTells    {RQuiet{x\n\r",
		"{yPlayer{x:");
  add_buf(output, buf);
  
  sprintf(buf, "%-16s  %-7s %-7s %-7s %-7s %-7s %-7s %-7s %-7s %-7s %-7s %-7s %-7s %-7s\n\r",
		"-------", "------", "------", "------","------", "------", "------", "------", "------", "------", "------", "------", "------", "------");
  add_buf(output, buf);
    
  for (i=0; i < cnt; i++) {

    if (get_trust(ch) < pcs[i]->invis_level)
	 continue;

    if ((pcs[i]->incog_level > 0) && (pcs[i] != ch) && !IS_IMMORTAL(ch))
        continue;

    fillers = (16 - colorstrlen(COLORNAME(pcs[i])));
    
    sprintf(buf, "%s%*s    %-7s{x   %-7s{x   %-7s{x   %-7s{x   %-7s{x   %-7s{x   %-7s{x   %-7s{x   %-7s{x   %-7s{x   %-7s{x   %-7s{x   %-7s{x\n\r",
		  COLORNAME(pcs[i]),
		  fillers, "",
		  !IS_SET(pcs[i]->comm,COMM_NOCHAT)   ?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_NOGAME)   ?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_NOCLAN)   ?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm2,COMM2_NOOGUILD) ?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_NOSGUILD) ?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_NOSSGUILD)?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_NONEWBIE) ?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_NOGOSSIP) ?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_NOAUCTION)?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_NOMUSIC)  ?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_NORACE)   ?  "{gOn" : "{ROff",
		  !IS_SET(pcs[i]->comm,COMM_DEAF)     ?  "{gOn" : "{ROff",
		  IS_SET(pcs[i]->comm,COMM_QUIET)     ?  "{gOn" : "{ROff");		 
    add_buf(output, buf);		  
  }

  sprintf(buf, "\n\rListed players: {y%d{x.\n\r", cnt);
  add_buf(output, buf);
  
  page_to_char( buf_string(output), ch );
  free_buf(output);
  return;	
}

void do_emweave( CHAR_DATA *ch, char *argument )
{

  char buffer[MAX_STRING_LENGTH];
  if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
    send_to_char( "You can't do that yet.\n\r", ch );
    return;
  }
  
  if ( argument[0] == '\0' ) {
    send_to_char( "Emweave what?\n\r", ch );
    return;
  }
  
  if (ch->class != CLASS_CHANNELER)
  {
    send_to_char("You aren't a channeller!\r\n",ch);
    return;
  }
  
  CHAR_DATA * pVictim = ch->in_room->people;
  act( "You $T", ch, NULL, emote_parse(ch,argument), TO_CHAR );
  sprintf(buffer,"$n %s",emote_parse(ch,argument));
  reward_rp (ch);
  if (IS_RP(ch) && ch->pcdata->forceinsanity)
  {
     ch->pcdata->forceinsanity = 0;
     ch->insanity_points++;
     char buf[256];
     sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
     sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
     log_string(buf);
     if (IS_WOLFKIN(ch))
     {
             handle_wolfkin_insanity(ch);
     }
     else
     {
        handle_mc_insanity(ch);
     }
  }
  while (pVictim)
  {
     if (pVictim->class == CLASS_CHANNELER && pVictim->sex == ch->sex)
     {
        MOBtrigger = FALSE;
        act( buffer, ch, NULL, pVictim, TO_VICT );
        MOBtrigger = TRUE;
     }
     else if (!IS_NPC(pVictim) && pVictim->sex == SEX_MALE && ch->sex == SEX_FEMALE && pVictim->class == CLASS_CHANNELER)
     {
	send_to_char("Your skin tingles slightly as a woman nearby channels.\r\n",pVictim);
     }
     
     pVictim = pVictim->next_in_room;
  }
  return;
}

/*
 * Writes a string to the Communications log.
 *  
 l*/
void log_comm_string( char * category, const char *str )
{
  char      *strtime;
  struct tm log_time;
  FILE      *log_fp;
  char      log_fname[MAX_STRING_LENGTH];
  char      log_date[9];
  char      buffer[MAX_STRING_LENGTH * 3];
  char      *point;
  char      *bufptr = buffer;

  strtime                    = ctime( &current_time );
  strtime[strlen(strtime)-1] = '\0';

  log_time = *localtime((time_t *)&current_time);

  sprintf(log_date, "%04d%02d%02d", log_time.tm_year+1900, log_time.tm_mon+1, log_time.tm_mday);

  sprintf(log_fname, "%s%s/%s.html", LOG_DIR, category, log_date);

  for( point = str ; *point ; point++ ) {
     if( *point == '{' ) {
        point++;
        continue;
     }
     *bufptr = *point;
     *++bufptr = '\0';
  }
  *bufptr = '\0';

 // colorize for web disabled. Plan text is easier to read
 // strcpy(buffer,color2web((char *)str));

  if ((log_fp = fopen(log_fname, "a+w")) == NULL) {
        fprintf( stderr, "%s :: Internal error: Error opening log file <%s> [%s]\n", strtime, log_fname, strerror(errno));
	sprintf(log_fname,"mkdir -p %s%s", LOG_DIR, category);
	system(log_fname);
        return;
  }
  else 
  {
     fprintf(log_fp, "%s :: %s<br>\n", strtime, buffer);
     fclose(log_fp);
  }
  return;
}
