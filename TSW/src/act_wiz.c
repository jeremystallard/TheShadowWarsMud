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
 ***************************************************************************/

/***************************************************************************
*  ROM 2.4 is copyright 1993-1998 Russ Taylor            *
*  ROM has been brought to you by the ROM consortium        *
*      Russ Taylor (rtaylor@hypercube.org)               *
*      Gabrielle Taylor (gtaylor@hypercube.org)          *
*      Brian Moore (zump@rom.org)                  *
*  By using this code, you have agreed to follow the terms of the    *
*  ROM license, in the file Rom24/doc/rom.license           *
***************************************************************************/
#include <dirent.h>
#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "signal.h"
#include "unistd.h"  /* for copyover execl */
#include "olc.h"

/* comm.c */
void list_background_table args( (CHAR_DATA *ch, const struct background_type *flag_table) );

/* board.c */
void do_nautopost args((CHAR_DATA *ch, char *argument));

/*
 * Local functions.
 */
ROOM_INDEX_DATA * find_location  args( ( CHAR_DATA *ch, char *arg ) );
extern bool write_to_descriptor  args( ( int desc, char *txt, int length ) );
const char * name_expand (CHAR_DATA *ch);

/* new warmboot handeling */
void    check_warmboot          args( ( void ) );
void    check_shutdown          args( ( void ) );

bool iswarmboot        = FALSE;
bool iscopyin          = FALSE;
int pulse_warmboot     = -1;
bool isshutdown        = FALSE;
int pulse_shutdown     = -1;
CHAR_DATA *warmboot_pc = NULL;

char warmbootMsg[MAX_STRING_LENGTH];

time_t last_restore = 0;

void do_wiznet( CHAR_DATA *ch, char *argument )
{
  int flag;
  char buf[MAX_STRING_LENGTH];
  int col=0;

  /* Show wiznet options - just like channel command */
  if ( argument[0] == '\0' ) {
    send_to_char("\n                          {WWELCOME TO WIZNET\n\r\n", ch);
    send_to_char("Option         Status   Option         Status   Option         Status\r\n",ch);
    send_to_char("---------------------   ---------------------   ---------------------{x\r\n",ch);
    /* list of all wiznet options */
    buf[0] = '\0';
    
    for (flag = 0; wiznet_table[flag].name != NULL; flag++) {
      if (wiznet_table[flag].level <= get_trust(ch)) {
   sprintf( buf, "%-14s %s\t", wiznet_table[flag].name,
       IS_SET(ch->wiznet,wiznet_table[flag].flag) ? "{RON{x" : "{gOFF{x" );
   send_to_char(buf, ch);   
   col++;
   if (col==3) {
     send_to_char("\r\n",ch);
     col=0;
   }
      }
    }
    /* To avoid color bleeding */
    send_to_char("{x\n",ch);
    return;
  }    
  
  if (!str_prefix(argument,"on")) {     
    send_to_char("{VWelcome to Wiznet!{x\r\n",ch);
    SET_BIT(ch->wiznet,WIZ_ON);
    return;
  }
  
  if (!str_prefix(argument,"off")) {
    send_to_char("{VSigning off of Wiznet.{x\r\n",ch);
    REMOVE_BIT(ch->wiznet,WIZ_ON);
    return;
  }
  
  flag = wiznet_lookup(argument);
  
  if (flag == -1 || get_trust(ch) < wiznet_table[flag].level) {
    send_to_char("{VNo such option.{x\r\n",ch);
    return;
  }
  
  if (IS_SET(ch->wiznet,wiznet_table[flag].flag)) {
    sprintf(buf,"{VYou will no longer see %s on wiznet.{x\r\n",
       wiznet_table[flag].name);
    send_to_char(buf,ch);
    REMOVE_BIT(ch->wiznet,wiznet_table[flag].flag);
    return;
  }
  else  {
    sprintf(buf,"{VYou will now see %s on wiznet.{x\r\n",
       wiznet_table[flag].name);
    send_to_char(buf,ch);
    SET_BIT(ch->wiznet,wiznet_table[flag].flag);
    return;
  }
}

void wiznet(char *string, CHAR_DATA *ch, OBJ_DATA *obj, long flag, long flag_skip, int min_level)
{
  DESCRIPTOR_DATA *d;
  char buf[MSL];
  
  for ( d = descriptor_list; d != NULL; d = d->next ) {
    if (d->connected == CON_PLAYING
      &&  IS_IMMORTAL(d->character) 
      &&  IS_SET(d->character->wiznet,WIZ_ON) 
      &&  (!flag || IS_SET(d->character->wiznet,flag))
      &&  (!flag_skip || !IS_SET(d->character->wiznet,flag_skip))
      &&  get_trust(d->character) >= min_level
      &&  d->character != ch) {

    if (IS_SET(d->character->wiznet,WIZ_PREFIX)) {
      sprintf(buf, "{W[ {Y%s{W ]:{x ",sec2strtime(current_time));
      send_to_char(buf, d->character );
    }
    else
      send_to_char( "{x", d->character );

    act_new(string,d->character,obj,ch,TO_CHAR,POS_DEAD);
    send_to_char( "{x", d->character );
    }
  }
  return;
}

void do_grab(CHAR_DATA * ch, char *argument)
{
   OBJ_DATA *obj;
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];

   argument = one_argument(argument, arg);
   argument = one_argument(argument, arg1);

   if (arg[0] == '\0')
   {
      send_to_char("Syntax: grab <objectname> <playername>.\r\n", ch);
      return;
   }

   if (arg1[0] == '\0')
   {
      send_to_char("Take it from whom?\r\n", ch);
      return;
   }

   if (!(victim = get_char_anywhere(ch, arg1)))
   {
      send_to_char("They aren't here.\r\n", ch);
      return;
   }

   if (get_trust(ch) < get_trust(victim))
   {
      send_to_char("You can't take things from people higher than you. They will kill you!", ch);
      return;
   }

   for (obj = victim->carrying; obj != NULL; obj = obj->next_content)
      if (is_name(arg, obj->name))
      {
         if (obj->wear_loc > WEAR_NONE)
            unequip_char(victim, obj);
         obj_from_char(obj);
         obj_to_char(obj, ch);
         act("You remove $N's $p and take it.", ch, obj, victim, TO_CHAR);
         return;
      }

   act("$N does not seem to have that.", ch, obj, victim, TO_CHAR);
   return;
}

/* equips a character */
void do_outfit ( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    int i,sn,vnum;

    if (ch->level > 5 || IS_NPC(ch))
    {
   send_to_char("Find it yourself!\r\n",ch);
   return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) == NULL )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0 );
   obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_LIGHT );
    }
 
    if ( ( obj = get_eq_char( ch, WEAR_BODY ) ) == NULL )
    {
   obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0 );
   obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_BODY );
    }

    /* do the weapon thing */
    if ((obj = get_eq_char(ch,WEAR_WIELD)) == NULL)
    {
      sn = 0; 
      vnum = OBJ_VNUM_SCHOOL_SWORD; /* just in case! */

      for (i = 0; weapon_table[i].name != NULL; i++)
      {
       if (ch->pcdata->learned[sn] < 
      ch->pcdata->learned[*weapon_table[i].gsn])
       {
         sn = *weapon_table[i].gsn;
         vnum = weapon_table[i].vnum;
       }
      }

      obj = create_object(get_obj_index(vnum),0);
      obj_to_char(obj,ch);
      equip_char(ch,obj,WEAR_WIELD);
    }

    if (((obj = get_eq_char(ch,WEAR_WIELD)) == NULL 
    ||   !IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS)) 
    &&  (obj = get_eq_char( ch, WEAR_SHIELD ) ) == NULL 
    && !IS_SET(ch->flaws, FLAW_ONEARM))
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0 );
   obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WEAR_SHIELD );
    }

    send_to_char("You have been equipped by the Creator.\r\n",ch);
}

     
/* RT nochannels command, for those spammers */
void do_nochannels( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
 
    one_argument( argument, arg );
 
    if ( arg[0] == '\0' )
    {
        send_to_char( "Nochannel whom?", ch );
        return;
    }
 
    if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
 
    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        send_to_char( "You failed.\r\n", ch );
        return;
    }
 
    if ( IS_SET(victim->comm, COMM_NOCHANNELS) )
    {
        REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char( "The gods have restored your channel priviliges.\r\n", 
            victim );
        send_to_char( "NOCHANNELS removed.\r\n", ch );
   sprintf(buf,"$N restores channels to %s",victim->name);
   wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char( "The gods have revoked your channel priviliges.\r\n", 
             victim );
        send_to_char( "NOCHANNELS set.\r\n", ch );
   sprintf(buf,"$N revokes %s's channels.",victim->name);
   wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
 
    return;
}


void do_smote(CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *vch;
  char *letter,*name;
  char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
  int matches = 0;
  
  if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
    send_to_char( "You can't show your emotions.\r\n", ch );
    return;
  }
  
  if ( argument[0] == '\0' ) {
    send_to_char( "Emote what?\r\n", ch );
    return;
  }
  
  if (strstr(argument,ch->name) == NULL) {
    send_to_char("You must include your name in an smote.\r\n",ch);
    return;
  }
  
  send_to_char(emote_parse(ch,argument),ch);
  send_to_char("\r\n",ch);
  
  for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
    if (vch->desc == NULL || vch == ch)
	 continue;
    
    if ((letter = strstr(argument,vch->name)) == NULL) {
	 send_to_char(emote_parse(vch,argument),vch);
	 send_to_char("\r\n",vch);
	 continue;
    }
    
    strcpy(temp,argument);
    temp[strlen(argument) - strlen(letter)] = '\0';
    last[0] = '\0';
    name = vch->name;
    
    for (; *letter != '\0'; letter++) {
	 if (*letter == '\'' && matches == strlen(vch->name)) {
	   strcat(temp,"r");
	   continue;
	 }
	 
	 if (*letter == 's' && matches == strlen(vch->name)) {
	   matches = 0;
	   continue;
	 }
	 
	 if (matches == strlen(vch->name)) {
	   matches = 0;
	 }
	 
	 if (*letter == *name) {
	   matches++;
	   name++;
	   if (matches == strlen(vch->name)) {
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
    
    send_to_char(emote_parse(vch,temp),vch);
    send_to_char("\r\n",vch);
    reward_rp (ch);
  }
  
  return;
}

void do_rpreset(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;
    char arg1[MAX_INPUT_LENGTH];
    
    argument = one_argument (argument, arg1);
    if (IS_NULLSTR(arg1))
    {
	send_to_char ("Syntax:\n\r", ch);
	send_to_char ("rpreset <player>\n\r", ch);
	return;
    }
    if ((vch = get_char_anywhere (ch, arg1)) == NULL)
    {
	send_to_char ("They aren't here.\n\r", ch);
	return;
    }
    if (IS_NPC (vch))
    {
	send_to_char ("They're an NPC, they can't RP anyway.\n\r", ch);
	return;
    }
    vch->pcdata->rprewardtimer = 0;
    send_to_char ("You have reset their RP timer to 0.  They will have to leave and re-enter RP to start geting rewards again.\n\r", ch);
    save_char_obj (vch, FALSE);
    return;
}

char *colorstrem(char *argument)
{
  static char target[512];
  int i=0;
  int cnt=0;

  if (argument == NULL || argument[0] == '\0')
    return 0;
  
  memset(target, 0x00, sizeof(target));
  
  for (i=0; i < strlen(argument); i++) {
    if (argument[i] == '{')
      continue;
    else if (argument[i-1] == '{')
      continue;
    else
      target[cnt] = argument[i];
    cnt++;
  }
  return (target);
}

void do_bamfin( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( !IS_NPC(ch) )
    {
   smash_tilde( argument );

   if (argument[0] == '\0')
   {
       sprintf(buf,"Your poofin is %s\r\n",ch->pcdata->bamfin);
       send_to_char(buf,ch);
       return;
   }

   if ( strstr(colorstrem(argument),ch->name) == NULL)
   {
       send_to_char("You must include your name.\r\n",ch);
       return;
   }
        
   free_string( ch->pcdata->bamfin );
   ch->pcdata->bamfin = str_dup( argument );

        sprintf(buf,"Your poofin is now %s\r\n",ch->pcdata->bamfin);
        send_to_char(buf,ch);
    }
    return;
}

void do_bamfout( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
 
    if ( !IS_NPC(ch) )
    {
        smash_tilde( argument );
 
        if (argument[0] == '\0')
        {
            sprintf(buf,"Your poofout is %s\r\n",ch->pcdata->bamfout);
            send_to_char(buf,ch);
            return;
        }
 
        if ( strstr(colorstrem(argument),ch->name) == NULL)
        {
            send_to_char("You must include your name.\r\n",ch);
            return;
        }
 
        free_string( ch->pcdata->bamfout );
        ch->pcdata->bamfout = str_dup( argument );
 
        sprintf(buf,"Your poofout is now %s\r\n",ch->pcdata->bamfout);
        send_to_char(buf,ch);
    }
    return;
}



void do_deny( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
   send_to_char( "Deny whom?\r\n", ch );
   return;
    }

    if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
    {
   send_to_char( "They aren't here.\r\n", ch );
   return;
    }

    if ( IS_NPC(victim) )
    {
   send_to_char( "Not on NPC's.\r\n", ch );
   return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
   send_to_char( "You failed.\r\n", ch );
   return;
    }

    SET_BIT(victim->act, PLR_DENY);
    send_to_char( "You are denied access!\r\n", victim );
    sprintf(buf,"$N denies access to %s",victim->name);
    wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    send_to_char( "OK.\r\n", ch );
    save_char_obj(victim, FALSE);
    stop_fighting(victim,TRUE);
    do_function(victim, &do_quit, "" );

    return;
}



void do_timeout( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int nNumberOfDays = 0;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
   	send_to_char( "Syntax: timeout <player> [<# of days>]\r\n", ch );
   	return;
    }

    if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
    {
   	send_to_char( "They aren't here.\r\n", ch );
   	return;
    }

    if ( IS_NPC(victim) )
    {
   	send_to_char( "Not on NPC's.\r\n", ch );
   	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
   	send_to_char( "You failed.\r\n", ch );
   	return;
    }

    argument = one_argument(argument, arg2);
    if (arg2[0] == '\0')  //no arguments = default 
    {
	nNumberOfDays = 2;
    }
    else
    {
	if (!is_number(arg2))
	{
		send_to_char("The second argument must be the number of days to put in timeout\r\n",ch);
		return;
	}
	else
	{
		nNumberOfDays = atoi(arg2);	
	}
    }


    victim->pcdata->timeoutstamp = current_time + (nNumberOfDays * 24 /*hours a day */ * 60 /*minutes in an hour*/ * 60 /*seconds in a minute */);

    sprintf(buf,"You are placed in timeout until %s!\r\n",(char *)ctime(&victim->pcdata->timeoutstamp) );
    send_to_char( buf, victim );
    sprintf(buf,"$N places %s in %d days of timeout",victim->name, nNumberOfDays);
    wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    send_to_char( "OK.\r\n", ch );
    save_char_obj(victim, FALSE);
    stop_fighting(victim,TRUE);
    do_function(victim, &do_quit, "" );

    return;
}



void do_disconnect( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
   send_to_char( "Disconnect whom?\r\n", ch );
   return;
    }

    if (is_number(arg))
    {
   int desc;

   desc = atoi(arg);
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
            if ( d->descriptor == desc )
            {
               close_socket( d );
               send_to_char( "Ok.\r\n", ch );
               return;
            }
   }
    }

    if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
    {
   send_to_char( "They aren't here.\r\n", ch );
   return;
    }

    if ( victim->desc == NULL )
    {
   act( "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
   return;
    }

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
   if ( d == victim->desc )
   {
       close_socket( d );
       send_to_char( "Ok.\r\n", ch );
       return;
   }
    }

    bug( "Do_disconnect: desc not found.", 0 );
    send_to_char( "Descriptor not found!\r\n", ch );
    return;
}

void do_plevel( CHAR_DATA *ch, char *argument )
{
	g_extraplevelFlag = !g_extraplevelFlag;
	char buffer[256];
	sprintf(buffer,"Bonus Plevelling is now: %s\r\n", g_extraplevelFlag ? "ON" : "OFF");
	send_to_char( buffer, ch);
	return;
}


void do_pardon( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
   send_to_char( "Syntax: pardon <character> <killer|thief>.\r\n", ch );
   return;
    }

    if ( ( victim = get_char_anywhere( ch, arg1 ) ) == NULL )
    {
   send_to_char( "They aren't here.\r\n", ch );
   return;
    }

    if ( IS_NPC(victim) )
    {
   send_to_char( "Not on NPC's.\r\n", ch );
   return;
    }

    if ( !str_cmp( arg2, "killer" ) )
    {
   if ( IS_SET(victim->act, PLR_KILLER) )
   {
       REMOVE_BIT( victim->act, PLR_KILLER );
       send_to_char( "Killer flag removed.\r\n", ch );
       send_to_char( "You are no longer a KILLER.\r\n", victim );
   }
   return;
    }

    if ( !str_cmp( arg2, "thief" ) )
    {
   if ( IS_SET(victim->act, PLR_THIEF) )
   {
       REMOVE_BIT( victim->act, PLR_THIEF );
       send_to_char( "Thief flag removed.\r\n", ch );
       send_to_char( "You are no longer a THIEF.\r\n", victim );
   }
   return;
    }

    send_to_char( "Syntax: pardon <character> <killer|thief>.\r\n", ch );
    return;
}



void do_echo( CHAR_DATA *ch, char *argument )
{
  DESCRIPTOR_DATA *d;
  
  if ( argument[0] == '\0' ) {
    send_to_char( "Global echo what?\r\n", ch );
    return;
  }
    
  for ( d = descriptor_list; d; d = d->next ) {
    if ( d->connected == CON_PLAYING ) {
	 if (get_trust(d->character) >= get_trust(ch))
	   send_to_char( "[{YGlobal echo{x]: ",d->character);
	 send_to_char( argument, d->character );
	 send_to_char( "\r\n",   d->character );
    }
    reward_rp (ch);
  }
  return;
}



void do_recho( CHAR_DATA *ch, char *argument )
{
  DESCRIPTOR_DATA *d;
  
  if ( argument[0] == '\0' ) {
    send_to_char( "Room echo what?\r\n", ch );
    
    return;
  }

  for ( d = descriptor_list; d; d = d->next ) {
    if ( d->connected == CON_PLAYING && d->character->in_room == ch->in_room && IS_SAME_WORLD(d->character, ch)) {
	 if (get_trust(d->character) >= get_trust(ch))
	   send_to_char( "[{YRoom echo{x]: ",d->character);
	 send_to_char( emote_parse(ch, argument), d->character );
	 send_to_char( "\r\n",   d->character );
    }
  }
  
  return;
}

void do_zecho(CHAR_DATA *ch, char *argument)
{
  DESCRIPTOR_DATA *d;
  
  if (!is_leader(ch) && !IS_IMMORTAL(ch))
  {
	send_to_char("You do not have permission to use area echos\n\r",ch);
	return;
  }
  if (argument[0] == '\0') {
    send_to_char("Zone echo what?\r\n",ch);
    return;
  }

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected == CON_PLAYING
	   &&  d->character->in_room != NULL && ch->in_room != NULL
	   &&  d->character->in_room->area == ch->in_room->area) {
	 if (get_trust(d->character) >= get_trust(ch))
	   send_to_char("[{YArea echo{x]: ",d->character);
	 send_to_char(argument,d->character);
	 send_to_char("\r\n",d->character);
    }
  }
  reward_rp (ch);
}

void do_pecho( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  
  argument = one_argument(argument, arg);
  


  if ( argument[0] == '\0' || arg[0] == '\0' ) {
    send_to_char("Personal echo what?\r\n", ch); 
    return;
  }

  if ( !IS_IMMORTAL(ch) ) {
	if ( (victim = get_char_room(ch,arg) ) == NULL) {
	   send_to_char("They aren't here!\r\n",ch);
	   return;
        }
  }
  else
  if  ( (victim = get_char_anywhere(ch, arg) ) == NULL ) {
    send_to_char("Target not found.\r\n",ch);
    return;
  }
  
  if ((!IS_IMMORTAL(ch) && !IS_FORSAKEN(ch)) &&  (victim != ch)) {
	send_to_char("You can't do that.\r\n",ch);
	return;
  }
  if (get_trust(victim) >= get_trust(ch) && get_trust(ch) != MAX_LEVEL && ch != victim)
    send_to_char( "[{YPersonal echo{x]: ",victim);

  send_to_char(argument,victim);
  send_to_char("\r\n",victim);
  if (ch != victim) {
     send_to_char( "[{YPersonal echo{x]: ",ch);
     send_to_char(argument,ch);
     send_to_char("\r\n",ch);
  }
   if (!IS_NPC(victim) && (IS_RP(victim))) {
	sprintf(log_buf,"%s: pecho to %s - %s", ch->name, victim->name, argument);
	log_rp_string( victim,log_buf );
   }
}


ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    if ( is_number(arg))
       return get_room_index( atoi( arg ) );

    if (IS_IMMORTAL(ch))
    {
    	if ( ( victim = get_char_area( ch, arg ) ) != NULL )
    	{
		if (IS_NPC(victim))
		{
	   	if (!IS_IMMORTAL(ch) && (ch->pcdata->questmob == victim->pIndexData->vnum ||  IS_SET(victim->act, ACT_TRAIN)) || strstr(victim->name,"quest") != NULL)
	   	{
			return NULL;
	   	}
        	}
		else
		{
			return victim->in_room;
		}
    	}
    }
    if (IS_IMMORTAL(ch))
    {
	victim = get_char_anywhere(ch, arg);
    }
    else
    {
	victim = get_char_same_world(ch, arg);
    }

    if (victim != NULL)
    {
	if (IS_NPC(victim))
	{
		if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && 
			((ch->pcdata->questmob == victim->pIndexData->vnum) ||  (IS_SET(victim->act, ACT_TRAIN)) || strstr(victim->name,"quest") != NULL))
		{
			return NULL;
		}
		else
		{
			return victim->in_room;
		}
	}
	else
	{
		return victim->in_room;
	}
    }

   
    if ( ( obj = get_obj_world( ch, arg ) ) != NULL )
    {
 	if ((!IS_IMMORTAL(ch)) && (obj->pIndexData->vnum == OBJ_VNUM_QUEST || strstr(obj->name,"keeper") || strstr(obj->name,"quest")))
        {
            return NULL;
 	}
   	else
   	{
	   return obj->in_room;
	}
   }    
   return NULL;
}



void do_transfer( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    OBJ_DATA  * obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
   send_to_char( "Transfer whom (and where)?\r\n", ch );
   return;
    }

    if ( !str_cmp( arg1, "all" ) )
    {
   for ( d = descriptor_list; d != NULL; d = d->next )
   {
       if ( d->connected == CON_PLAYING
       &&   d->character != ch
       &&   d->character->in_room != NULL
       &&   can_see( ch, d->character ) )
       {
      char buf[MAX_STRING_LENGTH];
      sprintf( buf, "%s %s", d->character->name, arg2 );
      do_function(ch, &do_transfer, buf );
       }
   }
   return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' )
    {
   location = ch->in_room;
        obj = ch->in_obj;
    }
    else
    {
   if ( ( location = find_location( ch, arg2 ) ) == NULL )
   {
       send_to_char( "No such location.\r\n", ch );
       return;
   }

   if ( !is_room_owner(ch,location) && room_is_private( location ) 
   &&  get_trust(ch) < MAX_LEVEL)
   {
       send_to_char( "That room is private right now.\r\n", ch );
       return;
   }
        /* Transfering someone to a container */
        obj = get_obj_world(ch,arg2);
    }

    if ( ( victim = get_char_anywhere( ch, arg1 ) ) == NULL )
    {
   send_to_char( "They aren't here.\r\n", ch );
   return;
    }

    if ( victim->in_room == NULL )
    {
   send_to_char( "They are in limbo.\r\n", ch );
   return;
    }

    if ( victim->fighting != NULL )
   stop_fighting( victim, TRUE );
    act( "$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, location );

   if ((obj != NULL) && (!put_char_in_obj(victim, obj)))
   {
      send_to_char("Trans to current location failed!  Victim dropped in room.\r\n", ch);
      return;
   }

   if( victim->mount != NULL ) {
	char_from_room( victim->mount );
	char_to_room( victim->mount, location );
	do_mount(victim, victim->mount->name);
	send_to_char("Your rider is being transferred, and so are you.\r\n", victim->mount );
   }

   if (ch->pet != NULL) {
    	char_from_room (ch->pet);
	char_to_room (ch->pet, location);
   }
   
    act( "$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM );
    if ( ch != victim )
	 act( "$n has transferred you.", ch, NULL, victim, TO_VICT );
    do_function(victim, &do_look, "auto" );
    send_to_char( "Ok.\r\n", ch );
}



void do_at( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    OBJ_DATA *on;
    CHAR_DATA *wch;
    OBJ_DATA *temp; 
    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
   send_to_char( "At where what?\r\n", ch );
   return;
    }

    if ( ( location = find_location( ch, arg ) ) == NULL )
    {
   send_to_char( "No such location.\r\n", ch );
   return;
    }

    if (!is_room_owner(ch,location) && room_is_private( location ) 
    &&  get_trust(ch) < MAX_LEVEL)
    {
   send_to_char( "That room is private right now.\r\n", ch );
   return;
    }

    temp = ch->in_obj;
    original = ch->in_room;
   if ( ch->in_obj != NULL )
     char_from_obj( ch );

    on = ch->on;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
   if ( wch == ch )
   {
       char_from_room( ch );
       char_to_room( ch, original );
            if ( temp != NULL )
              put_char_in_obj( ch, temp );
       ch->on = on;
       break;
   }
    }

    return;
}



void do_goto( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    CHAR_DATA *victim=NULL;
    int count = 0;
    bool inobj = FALSE;
    AREA_DATA *pArea;

    if ( argument[0] == '\0' )
    {
   send_to_char( "Goto where?\r\n", ch );
   return;
    }


    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
        send_to_char( "No such location.\r\n", ch );
        return;
    }
    
   pArea = location->area;

   if (!IS_BUILDER (ch, pArea))
   {
		send_to_char ("Goto:  Insufficient security to goto that room.\n\r", ch);
		return;
   }

   if ((location != NULL)
       && ((victim = get_char_anywhere(ch, argument)) != NULL)
       && (victim->in_obj != NULL))
      inobj = TRUE;

    count = 0;
    for ( rch = location->people; rch != NULL; rch = rch->next_in_room )
        count++;

    if (!is_room_owner(ch,location) && room_is_private(location) 
    &&  (count > 1 || get_trust(ch) < MAX_LEVEL))
    {
   send_to_char( "That room is private right now.\r\n", ch );
   return;
    }

    if ( ch->fighting != NULL )
   stop_fighting( ch, TRUE );

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
   if (get_trust(rch) >= ch->invis_level)
   {
       if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
      act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
       else
      act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
   }
    }

    char_from_room( ch );
    char_to_room( ch, location );

    if( inobj && !put_char_in_obj( ch, victim->in_obj ) )
      send_to_char("Goto failed to get you inside the object!\r\n", ch);


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
            else
                act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }

    do_function(ch, &do_look, "auto" );
    return;
}

void do_violate( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
 
    if ( argument[0] == '\0' )
    {
        send_to_char( "Goto where?\r\n", ch );
        return;
    }
 
    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
        send_to_char( "No such location.\r\n", ch );
        return;
    }

    if (!room_is_private( location ))
    {
        send_to_char( "That room isn't private, use goto.\r\n", ch );
        return;
    }
 
    if ( ch->fighting != NULL )
        stop_fighting( ch, TRUE );
 
    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
            else
                act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }
 
    char_from_room( ch );
    char_to_room( ch, location );
 
 
    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
            else
                act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }
 
    do_function(ch, &do_look, "auto" );
    return;
}

/* RT to replace the 3 stat commands */

void do_stat ( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char *string;
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *location;
   CHAR_DATA *victim;

   string = one_argument(argument, arg);
   if ( arg[0] == '\0')
   {
   send_to_char("Syntax:\r\n",ch);
   send_to_char("  stat <name>\r\n",ch);
   send_to_char("  stat obj <name>\r\n",ch);
   send_to_char("  stat mob <name>\r\n",ch);
   send_to_char("  stat char <name>\r\n", ch);
   send_to_char("  stat char <name> descs\r\n", ch);
   send_to_char("  stat room <number>\r\n",ch);
   return;
   }

   if (!str_cmp(arg,"room"))
   {
   do_function(ch, &do_rstat, string);
   return;
   }
  
   if (!str_cmp(arg,"obj"))
   {
   do_function(ch, &do_ostat, string);
   return;
   }

   if(!str_cmp(arg,"char")  || !str_cmp(arg,"mob"))
   {
   do_function(ch, &do_mstat, string);
   return;
   }
   
   /* do it the old way */

   obj = get_obj_world(ch,argument);
   if (obj != NULL)
   {
     do_function(ch, &do_ostat, argument);
     return;
   }

  victim = get_char_anywhere(ch,argument);
  if (victim != NULL)
  {
    do_function(ch, &do_mstat, argument);
    return;
  }

  location = find_location(ch,argument);
  if (location != NULL)
  {
    do_function(ch, &do_rstat, argument);
    return;
  }

  send_to_char("Nothing by that name found anywhere.\r\n",ch);
}

void do_rstat( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    int door;

    one_argument( argument, arg );
    location = ( arg[0] == '\0' ) ? ch->in_room : find_location( ch, arg );
    if ( location == NULL )
    {
   send_to_char( "No such location.\r\n", ch );
   return;
    }

    if (!is_room_owner(ch,location) && ch->in_room != location 
    &&  room_is_private( location ) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
   send_to_char( "That room is private right now.\r\n", ch );
   return;
    }

    sprintf( buf, "Name: '%s'\r\nArea: '%s'\r\n",
   location->name,
   location->area->name );
    send_to_char( buf, ch );

    sprintf( buf,
   "Vnum: %d  Sector: %d  Light: %d  Healing: %d  Endurance: %d\r\n",
   location->vnum,
   location->sector_type,
   location->light,
   location->heal_rate,
   location->endurance_rate );
    send_to_char( buf, ch );

    sprintf( buf,
   "Room flags: %d.\r\nDescription:\r\n%s",
   location->room_flags,
   location->description );
    send_to_char( buf, ch );

    if ( location->extra_descr != NULL )
    {
   EXTRA_DESCR_DATA *ed;

   send_to_char( "Extra description keywords: '", ch );
   for ( ed = location->extra_descr; ed; ed = ed->next )
   {
       send_to_char( ed->keyword, ch );
       if ( ed->next != NULL )
      send_to_char( " ", ch );
   }
   send_to_char( "'.\r\n", ch );
    }

    send_to_char( "Characters:", ch );
    for ( rch = location->people; rch; rch = rch->next_in_room )
    {
   if (can_see(ch,rch))
        {
       send_to_char( " ", ch );
       one_argument( rch->name, buf );
       send_to_char( buf, ch );
   }
    }

    send_to_char( ".\r\nObjects:   ", ch );
    for ( obj = location->contents; obj; obj = obj->next_content )
    {
   send_to_char( " ", ch );
   one_argument( obj->name, buf );
   send_to_char( buf, ch );
    }
    send_to_char( ".\r\n", ch );

    for ( door = 0; door <= 5; door++ )
    {
   EXIT_DATA *pexit;

   if ( ( pexit = location->exit[door] ) != NULL )
   {
       sprintf( buf,
      "Door: %d.  To: %d.  Key: %d.  Exit flags: %ld.\r\nKeyword: '%s'.  Description: %s",

      door,
      (pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
         pexit->key,
         pexit->exit_info,
         pexit->keyword,
         pexit->description[0] != '\0'
          ? pexit->description : "(none).\r\n" );
       send_to_char( buf, ch );
   }
    }

    return;
}



void do_ostat( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
   send_to_char( "Stat what?\r\n", ch );
   return;
    }

    if ( ( obj = get_obj_world( ch, argument ) ) == NULL )
    {
   send_to_char( "Nothing like that in the web of the Wheel.\r\n", ch);
   return;
    }

    sprintf( buf, "Name(s): %s\r\n",
   obj->name );
    send_to_char( buf, ch );

    sprintf( buf, "Vnum: %d  Format: %s  Type: %s  Resets: %d\r\n",
   obj->pIndexData->vnum, obj->pIndexData->new_format ? "new" : "old",
   item_name(obj->item_type), obj->pIndexData->reset_num );
    send_to_char( buf, ch );

    sprintf( buf, "Short description: %s\r\nLong description: %s\r\n",
   obj->short_descr, obj->description );
    send_to_char( buf, ch );

    sprintf( buf, "Wear bits: %s\r\nExtra bits: %s\r\n",
   wear_bit_name(obj->wear_flags), extra_bit_name( obj->extra_flags ) );
    send_to_char( buf, ch );

    sprintf( buf, "Number: %d/%d  Weight: %d/%d/%d (10th pounds)\r\n",
   1,           get_obj_number( obj ),
   obj->weight, get_obj_weight( obj ),get_true_weight(obj) );
    send_to_char( buf, ch );

    sprintf( buf, "Level: %d  Cost: %d  Condition: %d  Timer: %d\r\n",
   obj->level, obj->cost, obj->condition, obj->timer );
    send_to_char( buf, ch );

    sprintf( buf,
   "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\r\n",
   obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
   obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
   obj->carried_by == NULL    ? "(none)" : 
       can_see(ch,obj->carried_by) ? obj->carried_by->name
               : "someone",
   obj->wear_loc );
    send_to_char( buf, ch );
    
   if (obj->owner != NULL && obj->owner[0] != '\0')
   {
	sprintf( buf, "Owner: %s\r\n",obj->owner); 
	send_to_char(buf, ch);
   }
    sprintf( buf, "Values: %d %d %d %d %d\r\n",
   obj->value[0], obj->value[1], obj->value[2], obj->value[3],
   obj->value[4] );
    send_to_char( buf, ch );
    
   
    /* now give out vital statistics as per identify */
    
    switch ( obj->item_type )
    {
      case ITEM_SCROLL: 
      case ITEM_POTION:
      case ITEM_PILL:
       sprintf( buf, "Level %d spells of:", obj->value[0] );
       send_to_char( buf, ch );

       if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
       {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[1]].name, ch );
         send_to_char( "'", ch );
       }

       if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
       {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[2]].name, ch );
         send_to_char( "'", ch );
       }

       if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
       {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[3]].name, ch );
         send_to_char( "'", ch );
       }

       if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
       {
      send_to_char(" '",ch);
      send_to_char(skill_table[obj->value[4]].name,ch);
      send_to_char("'",ch);
       }

       send_to_char( ".\r\n", ch );
   break;

      case ITEM_WAND: 
      case ITEM_STAFF: 
       sprintf( buf, "Has %d(%d) charges of level %d",
         obj->value[1], obj->value[2], obj->value[0] );
       send_to_char( buf, ch );
      
       if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
       {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[3]].name, ch );
         send_to_char( "'", ch );
       }

       send_to_char( ".\r\n", ch );
   break;

   case ITEM_DRINK_CON:
       sprintf(buf,"It holds %s-colored %s.\r\n",
      liq_table[obj->value[2]].liq_color,
      liq_table[obj->value[2]].liq_name);
       send_to_char(buf,ch);
       break;
      
      
      case ITEM_WEAPON:
       send_to_char("Weapon type is ",ch);
       switch (obj->value[0])
       {
       case(WEAPON_EXOTIC): 
       send_to_char("exotic\r\n",ch);
       break;
       case(WEAPON_SWORD): 
       send_to_char("sword\r\n",ch);
       break;  
       case(WEAPON_DAGGER): 
       send_to_char("dagger\r\n",ch);
       break;
       case(WEAPON_SPEAR):
       send_to_char("spear\r\n",ch);
       break;
       case(WEAPON_STAFF):
       send_to_char("staff\r\n", ch);
       break;
       case(WEAPON_MACE): 
       send_to_char("mace/club\r\n",ch);  
       break;
       case(WEAPON_AXE): 
       send_to_char("axe\r\n",ch);  
       break;
       case(WEAPON_FLAIL): 
       send_to_char("flail\r\n",ch);
       break;
       case(WEAPON_WHIP): 
       send_to_char("whip\r\n",ch);
       break;
       case(WEAPON_POLEARM): 
       send_to_char("polearm\r\n",ch);
       break;
       case(WEAPON_LANCE): 
       send_to_char("lance\r\n",ch);
       break;
       case(WEAPON_BOW): 
       send_to_char("bow\r\n",ch);
       break;
       case(WEAPON_ARROW): 
       send_to_char("arrow\r\n",ch);
       break;
       default: 
       send_to_char("unknown\r\n",ch);
       break;
       }
       if (obj->pIndexData->new_format)
         sprintf(buf,"Damage is %dd%d (average %d)\r\n",
          obj->value[1],obj->value[2],
          (1 + obj->value[2]) * obj->value[1] / 2);
       else
         sprintf( buf, "Damage is %d to %d (average %d)\r\n",
             obj->value[1], obj->value[2],
             ( obj->value[1] + obj->value[2] ) / 2 );
       send_to_char( buf, ch );

       sprintf(buf,"Damage noun is %s.\r\n",
      (obj->value[3] > 0 && obj->value[3] < MAX_DAMAGE_MESSAGE) ?
          attack_table[obj->value[3]].noun : "undefined");
       send_to_char(buf,ch);
       
       if (obj->value[4])  /* weapon flags */
       {
           sprintf(buf,"Weapons flags: %s\r\n",
          weapon_bit_name(obj->value[4]));
           send_to_char(buf,ch);
            }
   break;

      case ITEM_ARMOR:
       sprintf( buf, 
       "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\r\n",
           obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
       send_to_char( buf, ch );
   break;

        case ITEM_CONTAINER:
            sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s\r\n",
                obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
            send_to_char(buf,ch);
            if IS_SET( obj->value[1], CONT_ENTERABLE ) 
            {
               if (obj->who_in != NULL)
               {
                  CHAR_IN_DATA *who_in;
                  strcat(buf1, "Who's Inside: ");
         
                  who_in = obj->who_in;
                  for (who_in = obj->who_in; who_in; who_in = who_in->next)
                  {
                     sprintf(buf, "%s ", who_in->ch->name);
                     strcat(buf1, buf);
                  }
                  strcat(buf1, "\r\n");
               }
               else
                  strcat(buf1, "Who's Inside: No one.\r\n");
            }

            if (obj->value[4] != 100)
            {
                sprintf(buf,"Weight multiplier: %d%%\r\n",
          obj->value[4]);
                send_to_char(buf,ch);
            }
        break;
    }


    if ( obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL )
    {
   EXTRA_DESCR_DATA *ed;

   send_to_char( "Extra description keywords: '", ch );

   for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
   {
       send_to_char( ed->keyword, ch );
       if ( ed->next != NULL )
         send_to_char( " ", ch );
   }

   for ( ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next )
   {
       send_to_char( ed->keyword, ch );
       if ( ed->next != NULL )
      send_to_char( " ", ch );
   }

   send_to_char( "'\r\n", ch );
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
   sprintf( buf, "Affects %s by %d, level %d",
       affect_loc_name( paf->location ), paf->modifier,paf->level );
   send_to_char(buf,ch);
   if ( paf->duration > -1)
       sprintf(buf,", %d hours.\r\n",paf->duration);
   else
       sprintf(buf,".\r\n");
   send_to_char( buf, ch );
   if (paf->bitvector)
   {
       switch(paf->where)
       {
      case TO_AFFECTS:
          sprintf(buf,"Adds %s affect.\n",
         affect_bit_name(paf->bitvector));
          break;
                case TO_WEAPON:
                    sprintf(buf,"Adds %s weapon flags.\n",
                        weapon_bit_name(paf->bitvector));
          break;
      case TO_OBJECT:
          sprintf(buf,"Adds %s object flag.\n",
         extra_bit_name(paf->bitvector));
          break;
      case TO_IMMUNE:
          sprintf(buf,"Adds immunity to %s.\n",
         imm_bit_name(paf->bitvector));
          break;
      case TO_RESIST:
          sprintf(buf,"Adds resistance to %s.\r\n",
         imm_bit_name(paf->bitvector));
          break;
      case TO_VULN:
          sprintf(buf,"Adds vulnerability to %s.\r\n",
         imm_bit_name(paf->bitvector));
          break;
      default:
          sprintf(buf,"Unknown bit %d: %d\r\n",
         paf->where,paf->bitvector);
          break;
       }
       send_to_char(buf,ch);
   }
    }

    if (!obj->enchanted)
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
    {
   sprintf( buf, "Affects %s by %d, level %d.\r\n",
       affect_loc_name( paf->location ), paf->modifier,paf->level );
   send_to_char( buf, ch );
        if (paf->bitvector)
        {
            switch(paf->where)
            {
                case TO_AFFECTS:
                    sprintf(buf,"Adds %s affect.\n",
                        affect_bit_name(paf->bitvector));
                    break;
                case TO_OBJECT:
                    sprintf(buf,"Adds %s object flag.\n",
                        extra_bit_name(paf->bitvector));
                    break;
                case TO_IMMUNE:
                    sprintf(buf,"Adds immunity to %s.\n",
                        imm_bit_name(paf->bitvector));
                    break;
                case TO_RESIST:
                    sprintf(buf,"Adds resistance to %s.\r\n",
                        imm_bit_name(paf->bitvector));
                    break;
                case TO_VULN:
                    sprintf(buf,"Adds vulnerability to %s.\r\n",
                        imm_bit_name(paf->bitvector));
                    break;
                default:
                    sprintf(buf,"Unknown bit %d: %d\r\n",
                        paf->where,paf->bitvector);
                    break;
            }
            send_to_char(buf,ch);
        }
    }

    return;
}



void do_mstat( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  char LAcol[4];
  char LLcol[4];
  char HEcol[4];
  char BDcol[4];
  char RAcol[4];
  char RLcol[4];
  bool has_guildinfo = FALSE;
  AFFECT_DATA *paf;
  CHAR_DATA *victim;
  int fillers=0;
  char *strtime;
  
  argument = one_argument( argument, arg );
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Stat whom?\r\n", ch );
    return;
  }
  
  if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if (!IS_NPC(victim) && victim->level > ch->level ) {
    send_to_char( "Stating people higher level than you is bad.\r\n", ch);
    return;
  }

  // Show descriptions only
  // 1) normal description
  // 2) hood description
  // 3) veil description
  // 4) wolf description
  // 5) wound description
  // 6) aura description
  
  if  (!str_cmp(argument, "descs") && !IS_NPC(victim)) {
    send_to_char("--------------------------------------------------------------------------\r\n", ch);
    sprintf(buf, "Name: %s ({y%s{x)\r\n",victim->name, capitalize(class_table[victim->class].name));
    send_to_char(buf, ch);
    send_to_char("--------------------------------------------------------------------------\r\n", ch);

    if (!IS_NULLSTR(victim->description)) {
	 send_to_char("[{gNormal Desc{x]:\r\n", ch);
	 send_to_char(victim->description, ch);
	 send_to_char("--------------------------------------------------------------------------\r\n", ch);
    }
    if (!IS_NULLSTR(victim->hood_description)) {
	 send_to_char("[{yHood Desc{x]:\r\n", ch);
	 send_to_char(victim->hood_description, ch);
	 send_to_char("--------------------------------------------------------------------------\r\n", ch);
    }
    if (!IS_NULLSTR(victim->veil_description)) {
	 send_to_char("[{yVeil Desc{x]:\r\n", ch);
	 send_to_char(victim->veil_description, ch);
	 send_to_char("--------------------------------------------------------------------------\r\n", ch);
    }    
    if (!IS_NULLSTR(victim->wolf_description)) {
	 send_to_char("[{YWolf Desc{x]:\r\n", ch);
	 send_to_char(victim->wolf_description, ch);
	 send_to_char("--------------------------------------------------------------------------\r\n", ch);
    }
    if (!IS_NULLSTR(victim->wound_description)) {
	 send_to_char("[{RWound Desc{x]:\r\n", ch);
	 send_to_char(victim->wound_description, ch);
	 send_to_char("--------------------------------------------------------------------------\r\n", ch);
    }
    if (!IS_NULLSTR(victim->aura_description)) {
	 send_to_char("[{mAura Desc{x]:\r\n", ch);
	 send_to_char(victim->aura_description, ch);
	 send_to_char("--------------------------------------------------------------------------\r\n", ch);
    }
    return;
  }

  send_to_char("--------------------------------------------------------------------------\r\n", ch);
  sprintf(buf2, "Vnum: %d", IS_NPC(victim) ? victim->pIndexData->vnum : 0);  
  sprintf(buf3, "Name: %s <%s> ({y%s{x) %s",victim->name,
		IS_NULLSTR(victim->real_name) ? "n/a" : victim->real_name,
		IS_NPC(victim) ? "Mobile" : capitalize(class_table[victim->class].name),
		IS_TRUSTED(ch,IMPLEMENTOR)? (IS_FORSAKEN(victim) ? "[{RForsaken{x]" : "") : "");
  fillers = (75 - colorstrlen(buf2) - colorstrlen(buf3) -1);
  
  sprintf(buf, "%s%*s%s\r\n", buf3, fillers, "", buf2);
  send_to_char( buf, ch );


  
  if (!IS_NPC(victim)) {
    sprintf(buf2, "Room: %d", victim->in_room == NULL ? 0 : victim->in_room->vnum);
    if (IS_TRUSTED(ch,IMPLEMENTOR))
       sprintf(buf3, "Email: %s", victim->pcdata->email);
    else
       sprintf(buf3,"Email: Hidden");
    fillers = (75 - strlen(buf2) - strlen(buf3) -1 );
    sprintf(buf, "%s%*s%s\r\n", buf3, fillers, "", buf2);
    send_to_char(buf, ch);

    sprintf(buf, "Appearance: %s\r\n", 
        victim->pcdata->appearance ? victim->pcdata->appearance : "(none)");
    send_to_char(buf, ch);
    
    sprintf(buf, "ICtitle: %s\r\n",
            !IS_NULLSTR(victim->pcdata->ictitle) ? victim->pcdata->ictitle : "(none)");
    send_to_char(buf, ch);
    
    sprintf(buf, "Hood Appearance: %s\r\n", 
            !IS_NULLSTR(victim->pcdata->hood_appearance) ? victim->pcdata->hood_appearance : "(none)");
    send_to_char(buf, ch);
    
    if (IS_AIEL(victim) || IS_TARABONER(victim)) {
       sprintf(buf, "Veil Appearance: %s\r\n", 
               !IS_NULLSTR(victim->pcdata->veil_appearance) ? victim->pcdata->veil_appearance : "(none)");
       send_to_char(buf, ch);
    }    

    if (!IS_NULLSTR(victim->pcdata->dreaming_appearance)) {
	sprintf(buf, "Dreamworld Appearance: %s\r\n",victim->pcdata->dreaming_appearance);
     }
    
    if (IS_WOLFKIN(victim)) {
       sprintf(buf, "Wolf Appearance: %s\r\n",
    	       !IS_NULLSTR(victim->pcdata->wolf_appearance) ? victim->pcdata->wolf_appearance : "(none)");
       send_to_char(buf, ch);
    }
  }

  if (IS_NPC(victim)) {
    sprintf(buf2, "Room: %d", victim->in_room == NULL ? 0 : victim->in_room->vnum);
    fillers = (75 - strlen(buf2) -1);
    sprintf(buf, "%*s%s\r\n", fillers, "", buf2);
    send_to_char( buf, ch );
  }

  send_to_char("--------------------[ Guild / Minion / Darkfriend ]-----------------------\r\n", ch);

  if (is_clan(victim)) {
    has_guildinfo = TRUE;
    sprintf( buf, "Guild:  %17s  Rank: %2d  Title: %s{x\r\n",
		   player_clan(victim), 
		   victim->rank+1, 
		   victim->gtitle ? victim->gtitle : player_rank(victim));
    send_to_char(buf, ch);
  }

  if (is_oguild(victim)) {
    has_guildinfo = TRUE;
    sprintf( buf, "OGuild:  %17s  Rank: %2d  Title: %s{x\r\n",
		   player_oguild(victim), 
		   victim->oguild_rank+1, 
		   victim->oguild_title ? victim->oguild_title : player_oguild_rank(victim));
    send_to_char(buf, ch);
  }

  
  if (is_sguild(victim)) {
    has_guildinfo = TRUE;
    sprintf( buf, "SGuild: %17s  Rank: %2d  Title: %s{x\r\n",
		   player_sguild(victim), 
		   victim->sguild_rank+1, 
		   victim->sguild_title ? victim->sguild_title : player_sguild_rank(victim));
    send_to_char(buf, ch);
  }

  if (is_ssguild(victim)) {
    has_guildinfo = TRUE;
    sprintf( buf, "SSGuild: %16s  Rank: %2d  Title: %s{x\r\n",
		   player_ssguild(victim), 
		   victim->ssguild_rank+1, 
		   victim->ssguild_title ? victim->ssguild_title : player_ssguild_rank(victim));
    send_to_char(buf, ch);
  }
  
  if (victim->minion != 0) {
    has_guildinfo = TRUE;
    sprintf(buf, "Minion: %17s  Rank: %2d  Title: %s{x\r\n",
		  victim->mname,
		  victim->mrank+1,
		  victim->mtitle ? victim->mtitle : "Unassigned");
    send_to_char(buf, ch);
  }
  
  if (!IS_NPC(victim) && !IS_NULLSTR(victim->pcdata->df_name)) {
    has_guildinfo = TRUE;
    sprintf(buf, "DF:     %17s{x  Rank: %2d\r\n",
		  victim->pcdata->df_name,
		  victim->pcdata->df_level);
    send_to_char(buf, ch);
  }

  if (!has_guildinfo) {
    send_to_char("No Guild/Minion/DF info found.\r\n", ch);
  }

  send_to_char("--------------------------------------------------------------------------\r\n", ch);

  sprintf( buf, "Format: %s  Race: %s  Group: %d  Sex: %s\r\n",
       IS_NPC(victim) ? victim->pIndexData->new_format ? "New" : "Old" : "PC",
       capitalize(race_table[victim->race].name),
       IS_NPC(victim) ? victim->group : 0, sex_table[victim->sex].name);
  send_to_char( buf, ch );

  
  if (IS_NPC(victim)) {
    sprintf(buf,"Count: %d  Killed: %d\r\n",
        victim->pIndexData->count,victim->pIndexData->killed);
    send_to_char(buf,ch);
  }

  if (IS_TRUSTED(ch,IMPLEMENTOR)&& !IS_NPC(victim))
  {
     sprintf( buf, 
       "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\r\n",
       victim->perm_stat[STAT_STR],
       get_curr_stat(victim,STAT_STR),
       victim->perm_stat[STAT_INT],
       get_curr_stat(victim,STAT_INT),
       victim->perm_stat[STAT_WIS],
       get_curr_stat(victim,STAT_WIS),
       victim->perm_stat[STAT_DEX],
       get_curr_stat(victim,STAT_DEX),
       victim->perm_stat[STAT_CON],
       get_curr_stat(victim,STAT_CON) );
     send_to_char( buf, ch );
  }
  
  if (!IS_NPC(victim)) {
     sprintf( buf, "C-kill: %d  PC-kill: %d\r\n", victim->pcdata->clan_kill_cnt, victim->pcdata->pc_kill_cnt);
     send_to_char( buf, ch );
  }
  
  if (IS_TRUSTED(ch,IMPLEMENTOR)&& !IS_NPC(victim))
  {
     sprintf( buf, "Hp: %d/%d  En: %d/%d  Trains: %d\r\n",
       victim->hit, victim->max_hit,
       victim->endurance, victim->max_endurance,
       victim->train);
     send_to_char( buf, ch );
  

    sprintf(LAcol, "%s", get_hit_loc_col(ch, victim, LOC_LA));
    sprintf(LLcol, "%s", get_hit_loc_col(ch, victim, LOC_LL));
    sprintf(HEcol, "%s", get_hit_loc_col(ch, victim, LOC_HE));
    sprintf(BDcol, "%s", get_hit_loc_col(ch, victim, LOC_BD));
    sprintf(RAcol, "%s", get_hit_loc_col(ch, victim, LOC_RA));
    sprintf(RLcol, "%s", get_hit_loc_col(ch, victim, LOC_RL));
    
    sprintf( buf, "%sLA{x: %d/%d %sLL{x: %d/%d %sHE{x: %d/%d %sBD{x: %d/%d %sRA{x: %d/%d %sRL{x: %d/%d\r\n",
		 LAcol, victim->hit_loc[LOC_LA], get_max_hit_loc(victim, LOC_LA),
		 LLcol, victim->hit_loc[LOC_LL], get_max_hit_loc(victim, LOC_LL),
		 HEcol, victim->hit_loc[LOC_HE], get_max_hit_loc(victim, LOC_HE),
		 BDcol, victim->hit_loc[LOC_BD], get_max_hit_loc(victim, LOC_BD),
		 RAcol, victim->hit_loc[LOC_RA], get_max_hit_loc(victim, LOC_RA),
		 RLcol, victim->hit_loc[LOC_RL], get_max_hit_loc(victim, LOC_RL));
    send_to_char( buf, ch);
  }
  
  sprintf( buf,
		 "Lv: %d (%d)  eLv: %d  Exp: %d (%s%ld exp to level{x)\r\n",
		 victim->level, get_level(victim),
		 !IS_NPC(victim) ? victim->pcdata->extended_level : 0,
		 victim->exp,
		 (exp_next_level(victim) - victim->exp) < 0 ? "{g" : "{x",
		 exp_next_level(victim) - victim->exp);
  send_to_char( buf, ch );

  sprintf(buf,
     "{YGold{x: %ld  {WSilver{x: %ld  {YGold{x Bank: %ld  {WSilver{x Bank: %ld\n\r",
     victim->gold, 
     victim->silver, 
     IS_NPC(victim) ? 0 : victim->pcdata->gold_bank,
     IS_NPC(victim) ? 0 : victim->pcdata->silver_bank);
  send_to_char(buf, ch);
  
  if (IS_TRUSTED(ch,IMPLEMENTOR)&& !IS_NPC(victim))
  {
     sprintf(buf,"{WAir{x: %d/%d {yEarth{x: %d/%d {RFire{x: %d/%d {YSpirit{x: %d/%d {BWater{x: %d/%d\r\n",
      victim->perm_sphere[SPHERE_AIR], victim->cre_sphere[SPHERE_AIR],
      victim->perm_sphere[SPHERE_EARTH], victim->cre_sphere[SPHERE_EARTH],
      victim->perm_sphere[SPHERE_FIRE], victim->cre_sphere[SPHERE_FIRE],
      victim->perm_sphere[SPHERE_SPIRIT], victim->cre_sphere[SPHERE_SPIRIT],
      victim->perm_sphere[SPHERE_WATER], victim->cre_sphere[SPHERE_WATER]);
     send_to_char(buf,ch);
  
     sprintf(buf, "OP: %ld/%ld  Autochannel: %d%%  Burnout: %d/%d  Main Sphere: %s\r\n",
      victim->holding, get_curr_op(victim), 
      victim->autoholding,
      victim->burnout, victim->max_burnout,      
      victim->main_sphere != -1 ? sphere_table[victim->main_sphere].name : "none");
     send_to_char(buf,ch);
  
  
    sprintf(buf, "Merits/Flaws/Talents: %s/%s/%s\r\n", 
      background_flag_string(merit_table, victim->merits),
      background_flag_string(flaw_table, victim->flaws),
      background_flag_string(talent_table, victim->talents));
    send_to_char(buf, ch);
  }

  if (IS_TRUSTED(ch,IMPLEMENTOR)&& !IS_NPC(victim))
  {
    sprintf(buf, "IC: %s\n\r", flag_string( ic_flags, victim->ic_flags));
    send_to_char(buf, ch);
  }
  
  sprintf(buf,"Armor: pierce: %d  bash: %d  slash: %d  magic: %d\r\n",
      GET_AC(victim,AC_PIERCE), GET_AC(victim,AC_BASH),
      GET_AC(victim,AC_SLASH),  GET_AC(victim,AC_EXOTIC));
  send_to_char(buf,ch);
  
  sprintf( buf, 
       "Hit: %d  Dam: %d  Saves: %d  Size: %s  Position: %s  Wimpy: %d\r\n",
       GET_HITROLL(victim), GET_DAMROLL(victim), victim->saving_throw,
       size_table[victim->size].name, position_table[victim->position].name,
       victim->wimpy );
  send_to_char( buf, ch );
  
  if (IS_NPC(victim) && victim->pIndexData->new_format) {
    sprintf(buf, "Damage: %dd%d  Message:  %s\r\n",
        victim->damage[DICE_NUMBER],victim->damage[DICE_TYPE],
        attack_table[victim->dam_type].noun);
    send_to_char(buf,ch);
  }
  sprintf( buf, "Fighting: %s\r\n",
       victim->fighting ? victim->fighting->name : "(none)" );
  send_to_char( buf, ch );
  
  if ( !IS_NPC(victim) ) {
    sprintf( buf,
         "Thirst: %d  Hunger: %d  Full: %d  Drunk: %d\r\n",
         victim->pcdata->condition[COND_THIRST],
         victim->pcdata->condition[COND_HUNGER],
         victim->pcdata->condition[COND_FULL],
         victim->pcdata->condition[COND_DRUNK] );
    send_to_char( buf, ch );
  }

  sprintf( buf, "Carry number: %d  Carry weight: %ld\r\n",
       victim->carry_number, get_carry_weight(victim) / 10 );
  send_to_char( buf, ch );

  
  if (!IS_NPC(victim)) {
    sprintf( buf, 
         "Age: %d  Played: %d  Last Level: %d  Timer: %d\r\n",
         get_age(victim), 
         (int) (victim->played + current_time - victim->logon) / 3600, 
         victim->pcdata->last_level, 
         victim->timer );
    send_to_char( buf, ch );
  }

  if (!IS_NPC(victim)) {
    sprintf(buf,"Roleplayed: %d Insanity Points: %02d\r\n",
	(int) (victim->roleplayed / 3600),
        victim->insanity_points);
    send_to_char(buf,ch);

  }

  if (!IS_NPC(victim) && victim->pcdata->last_backup) {
    strtime = ctime (&current_time);
    strtime[strlen(strtime)-1] = '\0';
    sprintf( buf, "Backup: %s\r\n", strtime);
    send_to_char( buf, ch);
  }
  
  sprintf(buf, "Act: %s\r\n",act_bit_name(victim->act));
  send_to_char(buf,ch);

  if (IS_NPC(victim) && IS_SET(victim->act, ACT_GAIN)) {
    sprintf(buf, "Gain: %s\r\n", flag_string( gain_flags, victim->gain_flags ));
    send_to_char(buf, ch);
  }

  if (IS_NPC(victim) && IS_SET(victim->act, ACT_TRAIN)) {
    sprintf(buf, "Train: %s  Train Level: %d\r\n", flag_string( train_flags, victim->train_flags ),
        victim->train_level);
    send_to_char(buf, ch);
  }
  
  if (victim->comm) {
    sprintf(buf,"Comm: %s\r\n",comm_bit_name(victim->comm));
    send_to_char(buf,ch);
  }
  
  if (IS_NPC(victim) && victim->off_flags) {
    sprintf(buf, "Offense: %s\r\n",off_bit_name(victim->off_flags));
    send_to_char(buf,ch);
  }
  
  if (victim->imm_flags) {
    sprintf(buf, "Immune: %s\r\n",imm_bit_name(victim->imm_flags));
    send_to_char(buf,ch);
  }
  
  if (victim->res_flags) {
    sprintf(buf, "Resist: %s\r\n", imm_bit_name(victim->res_flags));
    send_to_char(buf,ch);
  }
  
  if (victim->vuln_flags) {
    sprintf(buf, "Vulnerable: %s\r\n", imm_bit_name(victim->vuln_flags));
    send_to_char(buf,ch);
  }
  
  sprintf(buf, "Form: %s\r\nParts: %s\r\n", 
      form_bit_name(victim->form), part_bit_name(victim->parts));
  send_to_char(buf,ch);
  
  if (victim->affected_by) {
    sprintf(buf, "Affected by %s\r\n", 
        affect_bit_name(victim->affected_by));
    send_to_char(buf,ch);
  }
  
  sprintf( buf, "Master: %s  Leader: %s  Pet: %s   Mount: %s Riding: %s\r\n",
       victim->master      ? victim->master->name   : "(none)",
       victim->leader      ? victim->leader->name   : "(none)",
       victim->pet       ? victim->pet->name      : "(none)",
       victim->mount       ? victim->mount->name    : "(none)",
       victim->riding      ? "yes"                  : "no" );
  send_to_char( buf, ch );
  
  if (!IS_NPC(victim)) {
    sprintf( buf, "Security: %d.\r\n", victim->pcdata->security );   /* OLC */
    send_to_char( buf, ch );              /* OLC */
  }
  
  sprintf( buf, "Short description: %s\r\nLong  description: %s",
       victim->short_descr[0] != '\0' ? victim->short_descr : "(none)",
       victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\r\n" );
  send_to_char( buf, ch );
  
  if ( IS_NPC(victim) && victim->spec_fun != 0 ) {
    sprintf(buf,"Mobile has special procedure %s.\r\n",
        spec_name(victim->spec_fun));
    send_to_char(buf,ch);
  }
  
  for ( paf = victim->affected; paf != NULL; paf = paf->next ) {

    if (skill_table[paf->type].spell_fun == spell_null)
    sprintf(buf, "Skill : ");
    else
    sprintf(buf, "Weave : ");
    
    sprintf(buf2,
        "'%s' modifies %s by %d for %d hours with bits %s, level %d.\r\n",
        skill_table[(int) paf->type].name,
        affect_loc_name( paf->location ),
        paf->modifier,
        paf->duration,
        affect_bit_name( paf->bitvector ),
        paf->level
        );
    
    strcat(buf, buf2);
    send_to_char( buf, ch );
  }

  if (!IS_NPC(victim)) {
    
    // Web vote
    if (victim->pcdata->last_web_vote) {
	 send_to_char("--------------------------[ Webvote info ]--------------------------------\r\n", ch);
	 sprintf(buf, "Last: %s  Next: %s  Window: %s\r\n",
		    sec2str(victim->pcdata->last_web_vote),
		    ((victim->pcdata->last_web_vote+43200) <= current_time) ? "now" : sec2str(victim->pcdata->last_web_vote+43200),
		    ((victim->pcdata->last_web_vote+3600) - current_time) >= 0 ? "open" : "closed");
	 send_to_char(buf, ch);
    }

    send_to_char("---------------------------[ Quest info ]---------------------------------\r\n", ch);
    sprintf(buf,"QP: %ld QPA: %ld QPN: %d\r\n",victim->pcdata->quest_curr, victim->pcdata->quest_accum, victim->pcdata->nextquest);
    send_to_char(buf,ch);
    if (IS_SET(victim->act,PLR_QUESTING)) {
	 sprintf(buf, "QGiver: %s QMob: %d QObj: %d QTime: %d\r\n", !IS_NULLSTR(victim->pcdata->questgiver->short_descr) ? victim->pcdata->questgiver->short_descr : "(null)", victim->pcdata->questmob, victim->pcdata->questobj, victim->pcdata->countdown);
	 send_to_char(buf,ch);
    }

    if (!IS_NPC(victim) && (victim->pcdata->next_bmtrain || victim->pcdata->next_sdtrain))
	 send_to_char("---------------------[ Master training status ]---------------------------\r\n", ch);
    else
	 send_to_char("--------------------------------------------------------------------------\r\n", ch);
  }
  else
    send_to_char("--------------------------------------------------------------------------\r\n", ch);
  
  if (!IS_NPC(victim) && victim->pcdata->learned[gsn_blademaster] > 0) {
    if (victim->pcdata->next_bmtrain) {
	 if (victim->pcdata->next_bmtrain > current_time)
	   sprintf(buf, "BMtrain: {R%s{x", (char *) ctime(&victim->pcdata->next_bmtrain));
	 else
	   sprintf(buf, "BMtrain: {gNow!{x\r\n");
	 send_to_char(buf, ch);
	 send_to_char("--------------------------------------------------------------------------\r\n", ch);
    }
  }
  
  if (!IS_NPC(victim) && victim->pcdata->learned[gsn_speardancer] > 0) {
     if (victim->pcdata->next_sdtrain) {
        if (victim->pcdata->next_sdtrain > current_time)
	  sprintf(buf, "SDtrain: {R%s{x", (char *) ctime(&victim->pcdata->next_sdtrain));
	else
	  sprintf(buf, "SDtrain: {gNow!{x\r\n");
	send_to_char(buf, ch);
	send_to_char("--------------------------------------------------------------------------\r\n", ch);
     }
  }  

  if (!IS_NPC(victim) && victim->pcdata->learned[gsn_duelling] > 0) {
     if (victim->pcdata->next_dutrain) {
        if (victim->pcdata->next_dutrain > current_time)
	  sprintf(buf, "DUtrain: {R%s{x", (char *) ctime(&victim->pcdata->next_dutrain));
	else
	  sprintf(buf, "DUtrain: {gNow!{x\r\n");
	send_to_char(buf, ch);
	send_to_char("--------------------------------------------------------------------------\r\n", ch);
     }
  }  

    
  return;
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;

    string = one_argument(argument,arg);
 
    if (arg[0] == '\0')
    {
   send_to_char("Syntax:\r\n",ch);
   send_to_char("  vnum obj <name>\r\n",ch);
   send_to_char("  vnum mob <name>\r\n",ch);
   send_to_char("  vnum skill <skill or spell>\r\n",ch);
   return;
    }

    if (!str_cmp(arg,"obj"))
    {
   do_function(ch, &do_ofind, string);
   return;
    }

    if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
    { 
   do_function(ch, &do_mfind, string);
   return;
    }

    if (!str_cmp(arg,"skill") || !str_cmp(arg,"spell"))
    {
   do_function (ch, &do_slookup, string);
   return;
    }
    /* do both */
    do_function(ch, &do_mfind, argument);
    do_function(ch, &do_ofind, argument);
}


void do_mfind( CHAR_DATA *ch, char *argument )
{
    extern int top_mob_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
   send_to_char( "Find whom?\r\n", ch );
   return;
    }

    fAll = FALSE; /* !str_cmp( arg, "all" ); */
    found   = FALSE;
    nMatch  = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_mob_index; vnum++ )
    {
   if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
   {
       nMatch++;
       if ( fAll || is_name( argument, pMobIndex->player_name ) )
       {
	if ((get_trust(ch) >= MAX_LEVEL -4) || (IS_BUILDER (ch, pMobIndex->area)))
        {
            found = TRUE;
            sprintf( buf, "[%5d] %s\r\n",
            pMobIndex->vnum, pMobIndex->short_descr );
      	    send_to_char( buf, ch );
        }
       }
   }
    }

    if ( !found )
   send_to_char( "No mobiles by that name.\r\n", ch );

    return;
}



void do_ofind( CHAR_DATA *ch, char *argument )
{
    extern int top_obj_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
   send_to_char( "Find what?\r\n", ch );
   return;
    }

    fAll = FALSE; /* !str_cmp( arg, "all" ); */
    found   = FALSE;
    nMatch  = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
    {
   if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
   {
       nMatch++;
       if ( fAll || is_name( argument, pObjIndex->name ) )
       {
	if ((get_trust(ch) >= MAX_LEVEL - 4) || (IS_BUILDER (ch, pObjIndex->area)))
        {
          found = TRUE;
          sprintf( buf, "[%5d] %s\r\n",
          pObjIndex->vnum, pObjIndex->short_descr );
          send_to_char( buf, ch );
        }
       }
   }
    }

    if ( !found )
   send_to_char( "No objects by that name.\r\n", ch );

    return;
}


void do_owhere(CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = 50;

    buffer = new_buf();

    if (argument[0] == '\0')
    {
   send_to_char("Find what?\r\n",ch);
   return;
    }
 
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( !can_see_obj( ch, obj ) || !is_name( argument, obj->name )
        ||   ch->level < obj->level)
            continue;
 
        found = TRUE;
        number++;
 
        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
            ;
 
        if ( in_obj->carried_by != NULL && can_see(ch,in_obj->carried_by)
   &&   in_obj->carried_by->in_room != NULL)
            sprintf( buf, "%3d) %s is carried by %s [Room %d]\r\n",
                number, obj->short_descr,PERS(in_obj->carried_by, ch),
      in_obj->carried_by->in_room->vnum );
        else if (in_obj->in_room != NULL && can_see_room(ch,in_obj->in_room))
            sprintf( buf, "%3d) %s is in %s [Room %d]\r\n",
                number, obj->short_descr,in_obj->in_room->name, 
         in_obj->in_room->vnum);
   else
            sprintf( buf, "%3d) %s is somewhere\r\n",number, obj->short_descr);
 
        buf[0] = UPPER(buf[0]);
        add_buf(buffer,buf);
 
        if (number >= max_found)
            break;
    }
 
    if ( !found )
        send_to_char( "Nothing like that in heaven or earth.\r\n", ch );
    else
        page_to_char(buf_string(buffer),ch);

    free_buf(buffer);
}


void do_mwhere( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    CHAR_DATA *victim;
    bool found;
    int count = 0;

    if ( argument[0] == '\0' )
    {
   DESCRIPTOR_DATA *d;

   /* show characters logged */

   buffer = new_buf();
   for (d = descriptor_list; d != NULL; d = d->next)
   {
       if (d->character != NULL && d->connected == CON_PLAYING
       &&  d->character->in_room != NULL && can_see(ch,d->character)
       &&  can_see_room(ch,d->character->in_room))
       {
      victim = d->character;
      count++;
      if (d->original != NULL)
          sprintf(buf,"%3d) %s (in the body of %s) is in %s [%d]\r\n",
         count, d->original->name,victim->short_descr,
         victim->in_room->name,victim->in_room->vnum);
      else
          sprintf(buf,"%3d) %s is in %s [%d]\r\n",
         count, victim->name,victim->in_room->name,
         victim->in_room->vnum);
      add_buf(buffer,buf);
       }
   }

        page_to_char(buf_string(buffer),ch);
   free_buf(buffer);
   return;
    }

    found = FALSE;
    buffer = new_buf();
    for ( victim = char_list; victim != NULL; victim = victim->next )
    {
   if ( victim->in_room != NULL
   &&   is_name( argument, victim->name ) 
   &&   victim->invis_level < get_trust(ch)
   &&   victim->incog_level < get_trust(ch) )
   {
       found = TRUE;
       count++;
       sprintf( buf, "%3d) [%5d] %-28s [%5d] %s\r\n", count,
      IS_NPC(victim) ? victim->pIndexData->vnum : 0,
      IS_NPC(victim) ? victim->short_descr : victim->name,
      victim->in_room->vnum,
      victim->in_room->name );
       add_buf(buffer,buf);
   }
    }

    if ( !found )
   act( "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
    else
      page_to_char(buf_string(buffer),ch);

    free_buf(buffer);

    return;
}



void do_reboo( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to REBOOT, spell it out.\r\n", ch );
    return;
}



void do_reboot( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    CHAR_DATA *vch;

    if (ch->invis_level < LEVEL_HERO)
    {
      sprintf( buf, "Reboot by %s.", ch->name );
      do_function(ch, &do_echo, buf );
    }

    save_polls  ( ch, "" );         /* autosave polls */

    merc_down = TRUE;
    for ( d = descriptor_list; d != NULL; d = d_next )
    {
   d_next = d->next;
   vch = d->original ? d->original : d->character;
   if (vch != NULL)
       save_char_obj(vch, FALSE);
      close_socket(d);
    }
    
    return;
}

void do_shutdow( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SHUTDOWN, spell it out.\r\n", ch );
    return;
}

void do_initshutdown(CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim;

  argument = one_argument(argument, arg1);

  if (iswarmboot) {
    send_to_char("There is a warmboot in progress already!\r\n", ch);
    return;
  }

  if (arg1[0] == '\0') {
    send_to_char("Syntax:  Shutdown Now\r\n"
			  "         Shutdown Stop\r\n"
			  "         Shutdown <# minutes> [<message>]\r\n", ch);
    return;
  }

  if (argument[0] == '\0') {
    warmbootMsg[0] = '\0';
  }
  else {
    sprintf(warmbootMsg, "%s\r\n", argument);
  }

  if (!str_cmp (arg1, "now")) {
    do_shutdown(ch, NULL);
    return;
  }

  if (!str_cmp (arg1, "stop")) {
    if (pulse_shutdown >= 0 && isshutdown) {
	 pulse_shutdown = -1;
	 isshutdown = FALSE;
	 warmboot_pc = NULL;
	 warmbootMsg[0] = '\0';
	 for ( d = descriptor_list; d; d = d->next ) {
	   if (d->character != NULL && d->connected == CON_PLAYING) {
		victim = d->character;
		send_to_char("Shutdown has been cancelled...\r\n", victim);
	   }
	 }
	 return;
    }
    else {
	 send_to_char("There is no shutdown countdown in progress.\r\n",ch);
	 return;
    }
  }
  
  if (is_number(arg1)) {
    if (atoi(arg1) < 1 || atoi(arg1) > 10) {
	 send_to_char("Time range is between 1 and 10 minutes.\r\n", ch);
	 return;
    }
    else {
	 pulse_shutdown = (atoi(arg1)*60);
	 isshutdown = TRUE;
	 warmboot_pc = ch;
	 check_shutdown( );
	 return;
    }
  }   
  else {
    send_to_char("Syntax:  Shutdown Now\r\n"
			  "         Shutdown Stop\r\n"
			  "         Shutdown <# minutes> [<message>]\r\n", ch);
    return; 
  }
  
  return;
}
  

void do_shutdown( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  extern bool merc_down;
  DESCRIPTOR_DATA *d,*d_next;
  CHAR_DATA *vch;
  
  if (ch->invis_level < LEVEL_HERO)
    sprintf( buf, "Shutdown by %s.", ch->name );
  else
    sprintf( buf, "Shutdown by someone.");
  
  append_file( ch, SHUTDOWN_FILE, buf );
  strcat( buf, "\r\n" );
  strcat( buf, "Saving...\r\n");
  
  if (ch->invis_level < LEVEL_HERO) {
    do_function(ch, &do_echo, buf );
  }
  
  save_polls  ( ch, "" );         /* autosave polls */

  merc_down = TRUE;
  for ( d = descriptor_list; d != NULL; d = d_next) {
    d_next = d->next;
    vch = d->original ? d->original : d->character;
    if (vch != NULL)
	 save_char_obj(vch, FALSE);
    close_socket(d);
  }
  return;
}

void do_protect( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;

    if (argument[0] == '\0')
    {
   send_to_char("Protect whom from snooping?\r\n",ch);
   return;
    }

    if ((victim = get_char_anywhere(ch,argument)) == NULL)
    {
   send_to_char("You can't find them.\r\n",ch);
   return;
    }

    if (IS_SET(victim->comm,COMM_SNOOP_PROOF))
    {
   act_new("$N is no longer snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
   send_to_char("Your snoop-proofing was just removed.\r\n",victim);
   REMOVE_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
    else
    {
   act_new("$N is now snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
   send_to_char("You are now immune to snooping.\r\n",victim);
   SET_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
}
  


void do_snoop( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
   send_to_char( "Snoop whom?\r\n", ch );
   return;
    }

    if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
    {
   send_to_char( "They aren't here.\r\n", ch );
   return;
    }

    if ( victim->desc == NULL )
    {
   send_to_char( "No descriptor to snoop.\r\n", ch );
   return;
    }

    if ( victim == ch )
    {
   send_to_char( "Cancelling all snoops.\r\n", ch );
   wiznet("$N stops being such a snoop.",
      ch,NULL,WIZ_SNOOPS,WIZ_SECURE,get_trust(ch));
   for ( d = descriptor_list; d != NULL; d = d->next )
   {
       if ( d->snoop_by == ch->desc )
      d->snoop_by = NULL;
   }
   return;
    }

    if ( victim->desc->snoop_by != NULL )
    {
   send_to_char( "Busy already.\r\n", ch );
   return;
    }

    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room 
    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\r\n",ch);
        return;
    }

    if ( IS_SET(victim->in_room->room_flags, ROOM_IMP_ONLY) )
    {
   send_to_char("The Victim is in an IMP_ONLY room.\r\n",ch);
   return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) 
    ||   IS_SET(victim->comm,COMM_SNOOP_PROOF))
    {
   send_to_char( "You failed.\r\n", ch );
   return;
    }

    if ( ch->desc != NULL )
    {
   for ( d = ch->desc->snoop_by; d != NULL; d = d->snoop_by )
   {
       if ( d->character == victim || d->original == victim )
       {
      send_to_char( "No snoop loops.\r\n", ch );
      return;
       }
   }
    }

    victim->desc->snoop_by = ch->desc;
    sprintf(buf,"$N starts snooping on %s",
   (IS_NPC(ch) ? victim->short_descr : victim->name));
    wiznet(buf,ch,NULL,WIZ_SNOOPS,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\r\n", ch );
    return;
}



void do_switch( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
   send_to_char( "Switch into whom?\r\n", ch );
   return;
    }

    if ( ch->desc == NULL )
   return;
    
    if ( ch->desc->original != NULL )
    {
   send_to_char( "You are already switched.\r\n", ch );
   return;
    }

    if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
    {
   send_to_char( "They aren't here.\r\n", ch );
   return;
    }

    if ( victim == ch )
    {
   send_to_char( "Ok.\r\n", ch );
   return;
    }

    if (!IS_NPC(victim))
    {
   send_to_char("You can only switch into mobiles.\r\n",ch);
   return;
    }

    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room 
    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
   send_to_char("That character is in a private room.\r\n",ch);
   return;
    }

    if ( victim->desc != NULL )
    {
   send_to_char( "Character in use.\r\n", ch );
   return;
    }

    sprintf(buf,"$N switches into %s",victim->short_descr);
    wiznet(buf,ch,NULL,WIZ_SWITCHES,WIZ_SECURE,get_trust(ch));

    ch->desc->character = victim;
    ch->desc->original  = ch;
    victim->desc        = ch->desc;
    ch->desc            = NULL;
    /* change communications to match */
    if (ch->prompt != NULL)
        victim->prompt = str_dup(ch->prompt);
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    send_to_char( "Ok.\r\n", victim );
    return;
}



void do_return( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( ch->desc == NULL )
   return;

    if ( ch->desc->original == NULL )
    {
   send_to_char( "You aren't switched.\r\n", ch );
   return;
    }

    send_to_char( 
"You return to your original body. Type replay to see any missed tells.\r\n", 
   ch );
    if (ch->prompt != NULL)
    {
   free_string(ch->prompt);
   ch->prompt = NULL;
    }

    sprintf(buf,"$N returns from %s.",ch->short_descr);
    wiznet(buf,ch->desc->original,0,WIZ_SWITCHES,WIZ_SECURE,get_trust(ch));
    ch->desc->character       = ch->desc->original;
    ch->desc->original        = NULL;
    ch->desc->character->desc = ch->desc; 
    ch->desc                  = NULL;
    return;
}

/* trust levels for load and clone */
bool obj_check (CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (IS_TRUSTED(ch,GOD)
   || (IS_TRUSTED(ch,IMMORTAL) && obj->level <= 20 && obj->cost <= 1000)
   || (IS_TRUSTED(ch,DEMI)     && obj->level <= 10 && obj->cost <= 500)
   || (IS_TRUSTED(ch,ANGEL)    && obj->level <=  5 && obj->cost <= 250)
   || (IS_TRUSTED(ch,AVATAR)   && obj->level ==  0 && obj->cost <= 100))
   return TRUE;
    else
   return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone)
{
    OBJ_DATA *c_obj, *t_obj;


    for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
    {
   //if (obj_check(ch,c_obj))
   {
       t_obj = create_object(c_obj->pIndexData,0);
       clone_object(c_obj,t_obj);
       obj_to_obj(t_obj,clone);
       recursive_clone(ch,c_obj,t_obj);
   }
    }
}

/* command that is similar to load */
void do_clone(CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char *rest;
    CHAR_DATA *mob;
    OBJ_DATA  *obj;

    rest = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
   send_to_char("Clone what?\r\n",ch);
   return;
    }

    if (!str_prefix(arg,"object"))
    {
   mob = NULL;
   obj = get_obj_here(ch,rest);
   if (obj == NULL)
   {
       send_to_char("You don't see that here.\r\n",ch);
       return;
   }
    }
    else if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
    {
   obj = NULL;
   mob = get_char_room(ch,rest);
   if (mob == NULL)
   {
       send_to_char("You don't see that here.\r\n",ch);
       return;
   }
    }
    else /* find both */
    {
   mob = get_char_room(ch,argument);
   obj = get_obj_here(ch,argument);
   if (mob == NULL && obj == NULL)
   {
       send_to_char("You don't see that here.\r\n",ch);
       return;
   }
    }

    /* clone an object */
    if (obj != NULL)
    {
   OBJ_DATA *clone;

   /*if (!obj_check(ch,obj))
   {
       send_to_char(
      "Your powers are not great enough for such a task.\r\n",ch);
       return;
   }
   */

   clone = create_object(obj->pIndexData,0); 
   clone_object(obj,clone);
   if (obj->carried_by != NULL)
       obj_to_char(clone,ch);
   else
           if (ch->in_obj)
              obj_to_obj(clone, ch->in_obj);
           else
         obj_to_room(clone,ch->in_room);
   recursive_clone(ch,obj,clone);

        if (ch->in_obj)
      act("$n has created $p.",ch,clone,NULL,TO_CONT);
        else 
      act("$n has created $p.",ch,clone,NULL,TO_ROOM);
   act("You clone $p.",ch,clone,NULL,TO_CHAR);
   wiznet("$N clones $p.",ch,clone,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
   return;
    }
    else if (mob != NULL)
    {
   CHAR_DATA *clone;
   OBJ_DATA *new_obj;
   char buf[MAX_STRING_LENGTH];

   if (!IS_NPC(mob))
   {
       send_to_char("You can only clone mobiles.\r\n",ch);
       return;
   }

  /*
   if ((mob->level > 20 && !IS_TRUSTED(ch,GOD))
   ||  (mob->level > 10 && !IS_TRUSTED(ch,IMMORTAL))
   ||  (mob->level >  5 && !IS_TRUSTED(ch,DEMI))
   ||  (mob->level >  0 && !IS_TRUSTED(ch,ANGEL))
   ||  !IS_TRUSTED(ch,AVATAR))
   {
       send_to_char(
      "Your powers are not great enough for such a task.\r\n",ch);
       return;
   }
*/

   clone = create_mobile(mob->pIndexData);
   clone_mobile(mob,clone); 
   
   for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
   {
      // if (obj_check(ch,obj))
       {
      new_obj = create_object(obj->pIndexData,0);
      clone_object(obj,new_obj);
      recursive_clone(ch,obj,new_obj);
      obj_to_char(new_obj,clone);
      new_obj->wear_loc = obj->wear_loc;
       }
   }
   char_to_room(clone,ch->in_room);
        act("$n has created $N.",ch,NULL,clone,TO_ROOM);
        act("You clone $N.",ch,NULL,clone,TO_CHAR);
   sprintf(buf,"$N clones %s.",clone->short_descr);
   wiznet(buf,ch,NULL,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
        return;
    }
}

/* RT to replace the two load commands */

void do_load(CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];

   argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
   send_to_char("Syntax:\r\n",ch);
   send_to_char("  load mob <vnum>\r\n",ch);
   send_to_char("  load obj <vnum> <level>\r\n",ch);
   return;
    }

    if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
    {
   do_function(ch, &do_mload, argument);
   return;
    }

   if (!str_cmp(arg,"obj")) {
      do_function(ch, &do_oload, argument);
      return;
   }
    
    /* echo syntax */
    do_function(ch, &do_load, "");
}


void do_mload( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    
    one_argument( argument, arg );

    if ( arg[0] == '\0' || !is_number(arg) )
    {
   send_to_char( "Syntax: load mob <vnum>.\r\n", ch );
   return;
    }

    if ( ( pMobIndex = get_mob_index( atoi( arg ) ) ) == NULL )
    {
   send_to_char( "No mob has that vnum.\r\n", ch );
   return;
    }

    victim = create_mobile( pMobIndex );
    char_to_room( victim, ch->in_room );
    act( "$n has created $N!", ch, NULL, victim, TO_ROOM );
    sprintf(buf,"$N loads %s.",victim->short_descr);
    wiznet(buf,ch,NULL,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\r\n", ch );
    return;
}



void do_oload( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    int level;
    
    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || !is_number(arg1))
    {
   send_to_char( "Syntax: load obj <vnum> <level>.\r\n", ch );
   return;
    }
    
    level = get_trust(ch); /* default */
  
    if ( arg2[0] != '\0')  /* load with a level */
    {
   if (!is_number(arg2))
        {
     send_to_char( "Syntax: oload <vnum> <level>.\r\n", ch );
     return;
   }
        level = atoi(arg2);
        if (level < 0 || level > get_trust(ch))
   {
     send_to_char( "Level must be be between 0 and your level.\r\n",ch);
     return;
   }
    }

    if ( ( pObjIndex = get_obj_index( atoi( arg1 ) ) ) == NULL )
    {
   send_to_char( "No object has that vnum.\r\n", ch );
   return;
    }

    obj = create_object( pObjIndex, level );
    
    if ( CAN_WEAR(obj, ITEM_TAKE) )
   obj_to_char( obj, ch );
    else
       if (!ch->in_obj)
       {
      obj_to_room( obj, ch->in_room );
           act( "$n has created $p!", ch, obj, NULL, TO_ROOM );
       }
       else
       {
      obj_to_obj( obj, ch->in_obj );
           act( "$n has created $p!", ch, obj, NULL, TO_CONT );
       }
     
    wiznet("$N loads $p.",ch,obj,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\r\n", ch );

    return;
}



void do_purge( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    DESCRIPTOR_DATA *d;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
   /* 'purge' */
   CHAR_DATA *vnext;
   OBJ_DATA  *obj_next;

   for ( victim = ch->in_room->people; victim != NULL; victim = vnext )
   {
       vnext = victim->next_in_room;
       if ( IS_NPC(victim) && !IS_SET(victim->act,ACT_NOPURGE) 
       &&   victim != ch /* safety precaution */ )
      extract_char( victim, TRUE, FALSE);
   }

   for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
   {
       obj_next = obj->next_content;
       if (!IS_OBJ_STAT(obj,ITEM_NOPURGE))
         extract_obj( obj );
   }

   act( "$n purges the room!", ch, NULL, NULL, TO_ROOM);
   send_to_char( "Ok.\r\n", ch );
   return;
    }

    if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
    {
   send_to_char( "They aren't here.\r\n", ch );
   return;
    }

    if ( !IS_NPC(victim) )
    {

   if (ch == victim)
   {
     send_to_char("Ho ho ho.\r\n",ch);
     return;
   }

   if (get_trust(ch) <= get_trust(victim))
   {
     send_to_char("Maybe that wasn't a good idea...\r\n",ch);
     sprintf(buf,"%s tried to purge you!\r\n",ch->name);
     send_to_char(buf,victim);
     return;
   }

   act("$n disintegrates $N.",ch,0,victim,TO_NOTVICT);

      if (victim->level > 1)
       save_char_obj( victim, FALSE );
      d = victim->desc;
      extract_char( victim, TRUE, FALSE );
      if ( d != NULL )
          close_socket( d );

   return;
    }

    act( "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
    extract_char( victim, TRUE, FALSE );
    return;
}



void do_advance( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int level;
  int iLevel;
  
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  
  if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) ) {
    send_to_char( "Syntax: advance <char> <level>.\r\n", ch );
    return;
  }
  
  if ( ( victim = get_char_anywhere( ch, arg1 ) ) == NULL ) {
    send_to_char( "That player is not here.\r\n", ch);
    return;
  }

  if ( IS_NPC(victim) ) {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
  }
  
  if ( ( level = atoi( arg2 ) ) < 1 || level > ch->level ) {
    sprintf(buf,"Level must be 1 to %d.\r\n", ch->level);
    send_to_char(buf, ch);
    return;
  }

  if ( level > get_trust( ch ) ) {
    send_to_char( "Limited to your trust level.\r\n", ch );
    return;
  }

  /*
   * Lower level:
   *   Reset to level 1.
   *   Then raise again.
   *   Currently, an imp can lower another imp.
   *   -- Swiftest
   */
  if ( level <= victim->level ) {
    send_to_char( "Lowering a player's level!\r\n", ch );
    send_to_char( "{R**** OOOOHHHHHHHHHH  NNNNOOOO ****{x\r\n", victim );
    victim->level         = 1;
    victim->exp           = 0;
    victim->max_hit       = 100;
    victim->max_endurance = 100;
    victim->hit           = victim->max_hit;
    victim->endurance     = victim->max_endurance;
    advance_level( victim, TRUE , FALSE);
    if (victim->level < LEVEL_HERO-1 && !IS_NPC(victim))
	 victim->pcdata->extended_level = 0;
  }
  else {
    send_to_char( "Raising a player's level!\r\n", ch );
    send_to_char( "{Y**** OOOOHHHHHHHHHH  YYYYEEEESSS ****{x\r\n", victim );
  }

  for ( iLevel = victim->level ; iLevel < level; iLevel++ ) {
    victim->level += 1;
    advance_level( victim,TRUE, FALSE);
  }
  sprintf(buf,"You are now level %d.\r\n",victim->level);
  send_to_char(buf,victim);
  victim->exp   = 0;
  victim->trust = 0;

  if (victim->level == LEVEL_HERO)
  {
   set_imminfo(victim,"HERO");
  }
  /*
  if ((victim->level > LEVEL_HERO) && (victim->level < (LEVEL_ADMIN - 1)))
  {
   set_imminfo(victim,"BUILDER");
  }
  */
  save_char_obj(victim, FALSE);
  return;
}



void do_trust( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int level;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
   send_to_char( "Syntax: trust <char> <level>.\r\n", ch );
   return;
    }

    if ( ( victim = get_char_anywhere( ch, arg1 ) ) == NULL )
    {
   send_to_char( "That player is not here.\r\n", ch);
   return;
    }

    if ( ( level = atoi( arg2 ) ) < 0 || level > MAX_LEVEL )
    {
   sprintf(buf, "Level must be 0 (reset) or 1 to %d.\r\n",MAX_LEVEL);
   send_to_char(buf, ch);
   return;
    }

    if ( level > get_trust( ch ) )
    {
   send_to_char( "Limited to your trust.\r\n", ch );
   return;
    }

    victim->trust = level;
    return;
}

void do_unrestore (CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  CHAR_DATA *vch;
  DESCRIPTOR_DATA *d;

  argument = one_argument( argument, arg );

  if (!IS_IMMORTAL(ch)) {
     send_to_char("Huh?\n\r", ch);
     return;
  }

  if (IS_NULLSTR(arg)) {
     send_to_char("See 'help unrestore' for syntax.\n\r	", ch);
     return;
  }
  
  // All PCs
  // ===========
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg,"all")) {
    /* cure all mobiles */

    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL)
	 act_old("$n has unrestored all PCs in the game.", ch, NULL, victim, TO_VICT,POS_SLEEPING );
    }

    for ( victim = char_list; victim != NULL; victim = victim->next) {

	 if (IS_NPC(victim))
	   continue;

	 if (IS_IMMORTAL(victim))
	   continue;
	 
	 victim->hit    = 0;
	 victim->endurance = 0;
	 
	 victim->hit_loc[LOC_LA] = 0;
	 victim->hit_loc[LOC_LL] = 0;
	 victim->hit_loc[LOC_HE] = 0;
	 victim->hit_loc[LOC_BD] = 0;
	 victim->hit_loc[LOC_RA] = 0;
	 victim->hit_loc[LOC_RL] = 0;

         if (!IS_NPC(victim)) {
            if (victim->pcdata->condition[COND_HUNGER] != -1)
              victim->pcdata->condition[COND_HUNGER] = 0;
            if (victim->pcdata->condition[COND_THIRST] != -1)
              victim->pcdata->condition[COND_THIRST] = 0;
            if (victim->pcdata->condition[COND_FULL] != -1)
              victim->pcdata->condition[COND_FULL] = 0;
         }

	 
	 update_pos( victim);
    }
    send_to_char("All PCs unrestored.\r\n", ch);
    return;
  }
  
  if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  /* Tell all about the dreadfull happening */
  for (d = descriptor_list; d != NULL; d = d->next) {
    vch = d->character;
    
    if (vch == NULL || IS_NPC(vch))
	 continue;
    
    if (vch->in_room != NULL)
	 act_old("$n has unrestored $N in the game.", ch, NULL, vch, TO_VICT,POS_SLEEPING );
  }
  
  //act( "$n has unrestored just you.", ch, NULL, victim, TO_VICT );
  
  victim->hit  = 0;
  victim->endurance = 0;
  
  victim->hit_loc[LOC_LA] = 0;
  victim->hit_loc[LOC_LL] = 0;
  victim->hit_loc[LOC_HE] = 0;
  victim->hit_loc[LOC_BD] = 0;
  victim->hit_loc[LOC_RA] = 0;
  victim->hit_loc[LOC_RL] = 0;
  
  if (!IS_NPC(victim)) {
     if (victim->pcdata->condition[COND_HUNGER] != -1)
       victim->pcdata->condition[COND_HUNGER] = 0;
     if (victim->pcdata->condition[COND_THIRST] != -1)
       victim->pcdata->condition[COND_THIRST] = 0;
     if (victim->pcdata->condition[COND_FULL] != -1)
       victim->pcdata->condition[COND_FULL] = 0;
  }
  
  update_pos( victim );
  
  sprintf(buf,"$N unrestored just %s", IS_NPC(victim) ? victim->short_descr : victim->name);
  wiznet(buf,ch,NULL,WIZ_RESTORE,WIZ_SECURE,get_trust(ch));
  
  send_to_char( "Ok.\r\n", ch );
  return;	
}

void do_restore( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  bool norestore=FALSE;
  char norestorebuf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  CHAR_DATA *vch;
  DESCRIPTOR_DATA *d;
  int clan;
  int sex=0;

  norestorebuf[0] = '\0';
  
  argument = one_argument( argument, arg );
  
  // All in the room
  // ===============
  if (arg[0] == '\0' || !str_cmp(arg,"room")) {
    /* cure room */
    
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
//	 affect_strip(vch,gsn_plague);
	 affect_strip(vch,gsn_poison);
	 affect_strip(vch,gsn_blindness);
	 affect_strip(vch,gsn_sleep);
	 affect_strip(vch,gsn_curse);
	 affect_strip(vch,gsn_sap);
	 affect_strip(vch,gsn_blademaster); //blinded by masterforms
	 affect_strip(vch,gsn_dirt); //blinded by dirt kick

      if (!IS_NPC(vch)) {	 
	 if (vch->pcdata->condition[COND_HUNGER] != -1)
             vch->pcdata->condition[COND_HUNGER] = 100;
	 if (vch->pcdata->condition[COND_THIRST] != -1)
            vch->pcdata->condition[COND_THIRST] = 100;
	 if (vch->pcdata->condition[COND_FULL] != -1)
            vch->pcdata->condition[COND_FULL] = 100;
     }
	 vch->hit    = vch->max_hit;
	 vch->endurance = vch->max_endurance;
	 
	 vch->hit_loc[LOC_LA] = get_max_hit_loc(vch, LOC_LA);
	 vch->hit_loc[LOC_LL] = get_max_hit_loc(vch, LOC_LL);
	 vch->hit_loc[LOC_HE] = get_max_hit_loc(vch, LOC_HE);
	 vch->hit_loc[LOC_BD] = get_max_hit_loc(vch, LOC_BD);
	 vch->hit_loc[LOC_RA] = get_max_hit_loc(vch, LOC_RA);
	 vch->hit_loc[LOC_RL] = get_max_hit_loc(vch, LOC_RL);

	 update_pos( vch);
	 act("$n has restored all PCs in the room.",ch,NULL,vch,TO_VICT);
    }
    
    sprintf(buf,"$N restored all PCs in the room %d.",ch->in_room->vnum);
    wiznet(buf,ch,NULL,WIZ_RESTORE,WIZ_SECURE,get_trust(ch));
    
    send_to_char("Room restored.\r\n",ch);
    return;
    
  }

  if (!str_cmp(arg,"set") && (get_trust(ch) >= MAX_LEVEL - 6))
  {
	ch->pcdata->restoremessage = str_dup(argument);
	send_to_char("Set.\r\n", ch);
	return;
  }

  // All PCs
  // ===========  
  if ( get_trust(ch) >=  MAX_LEVEL - 6 && ((!str_cmp(arg,"all")) || (!str_cmp(arg,"imm")))) 
  {

    // To avoid restore spam
    if ((last_restore + 300) > current_time) {
	 if (!str_cmp(argument, "now"))
	   send_to_char("Forced restore overruled time check...\r\n", ch);
	 else {
	   send_to_char("There already was a restore less than 5 minutes ago!\r\n", ch);
	   return;
	 }
    }

    last_restore = current_time;

    // If no restore, prebuild string
    if (!IS_NULLSTR(argument) && argument[0] == '!') 
    {
	 for (d = descriptor_list; d!= NULL; d = d->next) 
         {
	   victim = d->character;	   
	   if (victim == NULL || IS_NPC(victim))
		continue;

	   if (strstr(argument, victim->name) != NULL ) 
           {
		if (!norestore) 
                {
		  sprintf(norestorebuf, "%s", victim->name);
		  norestore = TRUE;
		}
		else 
                {
		  strcat(norestorebuf, ", ");
		  strcat(norestorebuf, victim->name);
		}
	   }
	 }
    }

    for (d = descriptor_list; d != NULL; d = d->next) {
	 norestore = FALSE;
	 victim    = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (strstr(argument, victim->name) != NULL)
	   norestore = TRUE;
	 
	 if (!norestore) {
	   //affect_strip(victim,gsn_plague);
	   affect_strip(victim,gsn_poison);
	   affect_strip(victim,gsn_blindness);
	   affect_strip(victim,gsn_sleep);
	   affect_strip(victim,gsn_curse);
	   affect_strip(victim,gsn_sap);
	   affect_strip(victim,gsn_blademaster); //blinded by masterforms
	   affect_strip(victim,gsn_dirt); //blinded by dirt kick
	   
	   victim->hit    = victim->max_hit;
	   victim->endurance = victim->max_endurance;
	   
	   victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
	   victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
	   victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
	   victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
	   victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
	   victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
	   
	   if (!IS_NPC(victim)) {
		if (victim->pcdata->condition[COND_HUNGER] != -1)
		  victim->pcdata->condition[COND_HUNGER] = 100;
		if (victim->pcdata->condition[COND_THIRST] != -1)
		  victim->pcdata->condition[COND_THIRST] = 100;
		if (victim->pcdata->condition[COND_FULL] != -1)
		  victim->pcdata->condition[COND_FULL] = 100;
        }
	 
	   update_pos( victim);
	 }

	 if (victim->in_room != NULL) {
	   /* act("$n has restored all PCs in the game.",ch,NULL,victim,TO_VICT); */
           if (!str_cmp(arg,"imm") && !IS_NULLSTR(ch->pcdata->restoremessage))
           {
		   act_old(ch->pcdata->restoremessage, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
           }
           else
           {
	      if (!IS_NULLSTR(norestorebuf)) {
		   sprintf(buf, "$n has restored {gall{x PCs (except for: {R%s{x) in the game.", norestorebuf);
		   act_old(buf, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	      }
	      else {
		   act_old("$n has restored {gall{x PCs in the game.", ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	      }
           }
	 }
    }

    if (!IS_NULLSTR(norestorebuf)) {
	 sprintf(buf, "All active PCs restore (except for: %s).\r\n", norestorebuf);
	 send_to_char(buf, ch);
    }
    else {
	 send_to_char("All active PCs restored.\r\n",ch);
    }

    return;
  }

  // All mobiles
  // ===========
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg,"mob")) {
    /* cure all mobiles */

    for ( victim = char_list; victim != NULL; victim = victim->next) {
	 if (!IS_NPC(victim))
	   continue;
	 
	 //affect_strip(victim,gsn_plague);
	 affect_strip(victim,gsn_poison);
	 affect_strip(victim,gsn_blindness);
	 affect_strip(victim,gsn_sleep);
	 affect_strip(victim,gsn_curse);
	 affect_strip(victim,gsn_sap);
	 affect_strip(victim,gsn_blademaster); //blinded by masterforms
	 affect_strip(victim,gsn_dirt); //blinded by dirt kick

	 victim->hit    = victim->max_hit;
	 victim->endurance = victim->max_endurance;
	 
	 victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
	 victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
	 victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
	 victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
	 victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
	 victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
	 
	 update_pos( victim);
    }

    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL)
	   act_old("{DThe Dark One has restored all MOBs in the game.{x", ch, NULL, d->character, TO_VICT,POS_SLEEPING );
    }

    send_to_char("All Mobiles restored.\r\n", ch);
    return;
  }
  
  // All in a certain Guild
  // =====================
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg,"guild")) {
    
    if ((clan = clan_lookup(argument)) == 0) {
       send_to_char("No such guild exists.\r\n", ch);
       return;
    }

    for ( victim = char_list; victim != NULL; victim = victim->next) {
         if (!is_clan(victim))
           continue;
         if (victim->clan != clan)
           continue;
         if (IS_NPC(victim))
           continue;
	 
	 //affect_strip(victim,gsn_plague);
	 affect_strip(victim,gsn_poison);
	 affect_strip(victim,gsn_blindness);
	 affect_strip(victim,gsn_sleep);
	 affect_strip(victim,gsn_curse);
	 affect_strip(victim,gsn_sap);
	 affect_strip(victim,gsn_blademaster); //blinded by masterforms
	 affect_strip(victim,gsn_dirt); //blinded by dirt kick

	 victim->hit    = victim->max_hit;
	 victim->endurance = victim->max_endurance;
	 
	 victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
	 victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
	 victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
	 victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
	 victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
	 victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
	 
	 update_pos( victim);
    }

    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL) {
	   sprintf(buf, "$n has restored all PCs in the %s {xguild.", clan_table[clan].who_name);
	   act_old(buf, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	}
    }

    sprintf(buf, "All PCs in %s guild restored.\r\n", clan_table[clan].who_name);
    send_to_char(buf, ch);
    return;
  } 
  

  // All that start with char
  // ========================
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg,"start")) {
    
    for ( victim = char_list; victim != NULL; victim = victim->next) {
         if (IS_NPC(victim))
           continue;
         if (victim->name[0] != UPPER(argument[0]))
           continue;
	 
	 //affect_strip(victim,gsn_plague);
	 affect_strip(victim,gsn_poison);
	 affect_strip(victim,gsn_blindness);
	 affect_strip(victim,gsn_sleep);
	 affect_strip(victim,gsn_curse);
	 affect_strip(victim,gsn_sap);
	 affect_strip(victim,gsn_blademaster); //blinded by masterforms
	 affect_strip(victim,gsn_dirt); //blinded by dirt kick

	 victim->hit    = victim->max_hit;
	 victim->endurance = victim->max_endurance;
	 
	 victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
	 victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
	 victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
	 victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
	 victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
	 victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
	 
	 update_pos( victim);
    }

    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL) {
	   sprintf(buf, "$n has restored all PCs that start with a %c.", UPPER(argument[0]));
	   act_old(buf, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	}
    }

    sprintf(buf, "All PCs that start with a %c restored.\r\n", UPPER(argument[0]));
    send_to_char(buf, ch);
    return;
  }  

  // All that end with char
  // ========================
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg,"end")) {
    
    for ( victim = char_list; victim != NULL; victim = victim->next) {
         if (IS_NPC(victim))
           continue;
         if (victim->name[strlen(victim->name)-1] != LOWER(argument[0]))
           continue;
	 
	 //affect_strip(victim,gsn_plague);
	 affect_strip(victim,gsn_poison);
	 affect_strip(victim,gsn_blindness);
	 affect_strip(victim,gsn_sleep);
	 affect_strip(victim,gsn_curse);
	 affect_strip(victim,gsn_sap);
	 affect_strip(victim,gsn_blademaster); //blinded by masterforms
	 affect_strip(victim,gsn_dirt); //blinded by dirt kick

	 victim->hit    = victim->max_hit;
	 victim->endurance = victim->max_endurance;
	 
	 victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
	 victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
	 victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
	 victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
	 victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
	 victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
	 
	 update_pos( victim);
    }

    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL) {
	   sprintf(buf, "$n has restored all PCs that end with a %c.", UPPER(argument[0]));
	   act_old(buf, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	}
    }

    sprintf(buf, "All PCs that end with a %c restored.\r\n", UPPER(argument[0]));
    send_to_char(buf, ch);
    return;
  }

  // All that with sex...?
  // ========================
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg,"sex")) {
    
    if (!str_cmp(argument, "male"))
       sex = SEX_MALE;
    else if (!str_cmp(argument, "female"))
       sex = SEX_FEMALE;
    else {
       send_to_char("Please spesify male or female only.\r\n", ch);
       return;
    }
    
    for ( victim = char_list; victim != NULL; victim = victim->next) {
         if (IS_NPC(victim))
           continue;
         if (victim->sex != sex)
           continue;
	 
	 //affect_strip(victim,gsn_plague);
	 affect_strip(victim,gsn_poison);
	 affect_strip(victim,gsn_blindness);
	 affect_strip(victim,gsn_sleep);
	 affect_strip(victim,gsn_curse);
	 affect_strip(victim,gsn_sap);
	 affect_strip(victim,gsn_blademaster); //blinded by masterforms
	 affect_strip(victim,gsn_dirt); //blinded by dirt kick

	 victim->hit    = victim->max_hit;
	 victim->endurance = victim->max_endurance;
	 
	 victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
	 victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
	 victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
	 victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
	 victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
	 victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
	 
	 update_pos( victim);
    }

    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL) {
	   sprintf(buf, "$n has restored all %s PCs .", argument);
	   act_old(buf, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	}
    }

    sprintf(buf, "All %s PCs restored.\r\n", argument);
    send_to_char(buf, ch);
    return;
  }  

  // All that has set a title
  // ========================
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg,"title")) {
    
    for ( victim = char_list; victim != NULL; victim = victim->next) {
         if (IS_NPC(victim))
           continue;

         if (!str_cmp(victim->pcdata->title, " probably wants to change his title.") ||
             !str_cmp(victim->pcdata->title, " probably wants to change her title."))
           continue;
 	 
	 //affect_strip(victim,gsn_plague);
	 affect_strip(victim,gsn_poison);
	 affect_strip(victim,gsn_blindness);
	 affect_strip(victim,gsn_sleep);
	 affect_strip(victim,gsn_curse);
	 affect_strip(victim,gsn_sap);
	 affect_strip(victim,gsn_blademaster); //blinded by masterforms
	 affect_strip(victim,gsn_dirt); //blinded by dirt kick

	 victim->hit    = victim->max_hit;
	 victim->endurance = victim->max_endurance;
	 
	 victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
	 victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
	 victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
	 victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
	 victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
	 victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
	 
	 update_pos( victim);
    }

    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL) {
	   sprintf(buf, "$n has restored all PCs that has set their title.");
	   act_old(buf, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	}
    }

    sprintf(buf, "All PCs that has set their title restored.\r\n");
    send_to_char(buf, ch);
    return;
  }   


  // Random!!!
  // ========================
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg,"random")) {

    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL) {
	   sprintf(buf, "$n has restored a set of random PCs.");
	   act_old(buf, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	 }
    }
    
    sprintf(buf, "Random PCs restored.\r\n");
    send_to_char(buf, ch);
    
    for ( victim = char_list; victim != NULL; victim = victim->next) {
	 if (IS_NPC(victim))
	   continue;

	 if (number_percent() < 78) {
	   continue;
	 }
	 else {
	   send_to_char("You have been randomly restored!!!\n\r", victim);
	   sprintf(buf, "%s was randomly restored.\n\r", victim->name);
	   send_to_char(buf, ch);
	 }
 	 
	 //affect_strip(victim,gsn_plague);
	 affect_strip(victim,gsn_poison);
	 affect_strip(victim,gsn_blindness);
	 affect_strip(victim,gsn_sleep);
	 affect_strip(victim,gsn_curse);
	 affect_strip(victim,gsn_sap);
	 affect_strip(victim,gsn_blademaster); //blinded by masterforms
	 affect_strip(victim,gsn_dirt); //blinded by dirt kick

	 victim->hit       = number_range(victim->hit, victim->max_hit);
	 victim->endurance = number_range(victim->endurance, victim->max_endurance);
	 
	 victim->hit_loc[LOC_LA] = number_range(victim->hit_loc[LOC_LA], get_max_hit_loc(victim, LOC_LA));
	 victim->hit_loc[LOC_LL] = number_range(victim->hit_loc[LOC_LL],get_max_hit_loc(victim, LOC_LL));
	 victim->hit_loc[LOC_HE] = number_range(victim->hit_loc[LOC_HE],get_max_hit_loc(victim, LOC_HE));
	 victim->hit_loc[LOC_BD] = number_range(victim->hit_loc[LOC_BD],get_max_hit_loc(victim, LOC_BD));
	 victim->hit_loc[LOC_RA] = number_range(victim->hit_loc[LOC_RA],get_max_hit_loc(victim, LOC_RA));
	 victim->hit_loc[LOC_RL] = number_range(victim->hit_loc[LOC_RL],get_max_hit_loc(victim, LOC_RL));
	 
	 update_pos( victim);
    }

    return;
  }
  

  // All that is not in RP mode
  // ========================
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg,"rp")) {
    
    for ( victim = char_list; victim != NULL; victim = victim->next) {
	 if (IS_NPC(victim))
	   continue;	 
	 
	 if (IS_RP(victim))
	   continue;
	 
	 //affect_strip(victim,gsn_plague);
	 affect_strip(victim,gsn_poison);
	 affect_strip(victim,gsn_blindness);
	 affect_strip(victim,gsn_sleep);
	 affect_strip(victim,gsn_curse);
	 affect_strip(victim,gsn_sap);
	 affect_strip(victim,gsn_blademaster); //blinded by masterforms
	 affect_strip(victim,gsn_dirt); //blinded by dirt kick
	 
	 victim->hit    = victim->max_hit;
	 victim->endurance = victim->max_endurance;
	 
	 victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
	 victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
	 victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
	 victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
	 victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
	 victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
	 
	 update_pos( victim);
    }
    
    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL) {
	   sprintf(buf, "$n has restored all PCs that are not in RP mode.");
	   act_old(buf, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	}
    }

    sprintf(buf, "All PCs that are not in RP mode restored.\r\n");
    send_to_char(buf, ch);
    return;
  }

  // All that are not whoinv
  // ========================
  if (get_trust(ch) >=  MAX_LEVEL - 6 && !str_cmp(arg, "whoinvis")) {
    
    for ( victim = char_list; victim != NULL; victim = victim->next) {
	 if (IS_NPC(victim))
	   continue;	 
	 
	 if (victim->incog_level > 0)
	   continue;
	 
	 //affect_strip(victim,gsn_plague);
	 affect_strip(victim,gsn_poison);
	 affect_strip(victim,gsn_blindness);
	 affect_strip(victim,gsn_sleep);
	 affect_strip(victim,gsn_curse);
	 affect_strip(victim,gsn_sap);
	 affect_strip(victim,gsn_blademaster); //blinded by masterforms
	 affect_strip(victim,gsn_dirt); //blinded by dirt kick
	 
	 victim->hit    = victim->max_hit;
	 victim->endurance = victim->max_endurance;
	 
	 victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
	 victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
	 victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
	 victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
	 victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
	 victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
	 
	 update_pos( victim);
    }
    
    /* Tell all about the dreadfull happening */
    for (d = descriptor_list; d != NULL; d = d->next) {
	 victim = d->character;
	 
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 if (victim->in_room != NULL) {
	   sprintf(buf, "$n has restored all PCs that are not whoinvis.");
	   act_old(buf, ch, NULL, d->character, TO_VICT,POS_SLEEPING );
	}
    }

    sprintf(buf, "All PCs that are not whoinvis restored.\r\n");
    send_to_char(buf, ch);
    return;
  }

  
  if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }
  
  //affect_strip(victim,gsn_plague);
  affect_strip(victim,gsn_poison);
  affect_strip(victim,gsn_blindness);
  affect_strip(victim,gsn_sleep);
  affect_strip(victim,gsn_curse);
  affect_strip(victim,gsn_sap);
  affect_strip(victim,gsn_blademaster); //blinded by masterforms
  affect_strip(victim,gsn_dirt); //blinded by dirt kick
  
  victim->hit  = victim->max_hit;
  victim->endurance = victim->max_endurance;
  
  victim->hit_loc[LOC_LA] = get_max_hit_loc(victim, LOC_LA);
  victim->hit_loc[LOC_LL] = get_max_hit_loc(victim, LOC_LL);
  victim->hit_loc[LOC_HE] = get_max_hit_loc(victim, LOC_HE);
  victim->hit_loc[LOC_BD] = get_max_hit_loc(victim, LOC_BD);
  victim->hit_loc[LOC_RA] = get_max_hit_loc(victim, LOC_RA);
  victim->hit_loc[LOC_RL] = get_max_hit_loc(victim, LOC_RL);
  
  if (!IS_NPC(victim)) {
     if (victim->pcdata->condition[COND_HUNGER] != -1)
       victim->pcdata->condition[COND_HUNGER] = 100;
     if (victim->pcdata->condition[COND_THIRST] != -1)
       victim->pcdata->condition[COND_THIRST] = 100;
     if (victim->pcdata->condition[COND_FULL] != -1)
       victim->pcdata->condition[COND_FULL] = 100;
  }
  
  update_pos( victim );
  act( "$n has restored just you.", ch, NULL, victim, TO_VICT );
  sprintf(buf,"$N restored just %s",
		IS_NPC(victim) ? victim->short_descr : victim->name);
  wiznet(buf,ch,NULL,WIZ_RESTORE,WIZ_SECURE,get_trust(ch));
  send_to_char( "Ok.\r\n", ch );
  return;
}
   
void do_freeze( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
   send_to_char( "Freeze whom?\r\n", ch );
   return;
    }

    if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
    {
   send_to_char( "They aren't here.\r\n", ch );
   return;
    }

    if ( IS_NPC(victim) )
    {
   send_to_char( "Not on NPC's.\r\n", ch );
   return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
   send_to_char( "You failed.\r\n", ch );
   return;
    }

    if ( IS_SET(victim->act, PLR_FREEZE) )
    {
   REMOVE_BIT(victim->act, PLR_FREEZE);
   send_to_char( "You can play again.\r\n", victim );
   send_to_char( "FREEZE removed.\r\n", ch );
   sprintf(buf,"$N thaws %s.",victim->name);
   wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
   SET_BIT(victim->act, PLR_FREEZE);
   send_to_char( "You can't do ANYthing!\r\n", victim );
   send_to_char( "FREEZE set.\r\n", ch );
   sprintf(buf,"$N puts %s in the deep freeze.",victim->name);
   wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    save_char_obj( victim, FALSE );

    return;
}



void do_log( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Log whom?\r\n", ch );
    return;
  }
  
  if ( !str_cmp( arg, "all" ) ) {
    if ( fLogAll ) {
	 fLogAll = FALSE;
	 send_to_char( "Log ALL off.\r\n", ch );
    }
    else {
	 fLogAll = TRUE;
	 send_to_char( "Log ALL on.\r\n", ch );
    }
    return;
  }

  if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }
  
  if ( IS_NPC(victim) ) {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
  }
  
  /*
   * No level check, gods can log anyone.
   */
  if ( IS_SET(victim->act, PLR_LOG) ) {
    REMOVE_BIT(victim->act, PLR_LOG);
    send_to_char( "LOG removed.\r\n", ch );
  }
  else {
    SET_BIT(victim->act, PLR_LOG);
    send_to_char( "LOG set.\r\n", ch );
  }
  
  return;
}

void do_noemote( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Noemote whom?\r\n", ch );
    return;
  }
  
  if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }
  
  
  if ( get_trust( victim ) >= get_trust( ch ) ) {
    send_to_char( "You failed.\r\n", ch );
    return;
  }
  
  if ( IS_SET(victim->comm, COMM_NOEMOTE) ) {
    REMOVE_BIT(victim->comm, COMM_NOEMOTE);
    send_to_char( "You can emote again.\r\n", victim );
    send_to_char( "NOEMOTE removed.\r\n", ch );
    sprintf(buf,"$N restores emotes to %s.",victim->name);
    wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
  }
  else {
    SET_BIT(victim->comm, COMM_NOEMOTE);
    send_to_char( "You can't emote!\r\n", victim );
    send_to_char( "NOEMOTE set.\r\n", ch );
    sprintf(buf,"$N revokes %s's emotes.",victim->name);
    wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
  }
  
  return;
}

void do_noshout( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Noshout whom?\r\n",ch);
    return;
  }
  
  if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }
  
  if ( IS_NPC(victim) ) {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
  }
  
  if ( get_trust( victim ) >= get_trust( ch ) ) {
    send_to_char( "You failed.\r\n", ch );
    return;
  }
  
  if ( IS_SET(victim->comm, COMM_NOSHOUT) ) {
    REMOVE_BIT(victim->comm, COMM_NOSHOUT);
    send_to_char( "You can shout again.\r\n", victim );
    send_to_char( "NOSHOUT removed.\r\n", ch );
    sprintf(buf,"$N restores shouts to %s.",victim->name);
    wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
  }
  else {
    SET_BIT(victim->comm, COMM_NOSHOUT);
    send_to_char( "You can't shout!\r\n", victim );
    send_to_char( "NOSHOUT set.\r\n", ch );
    sprintf(buf,"$N revokes %s's shouts.",victim->name);
    wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
  }
  
  return;
}

void do_notell( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
   send_to_char( "Notell whom?", ch );
   return;
    }

    if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
    {
   send_to_char( "They aren't here.\r\n", ch );
   return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
   send_to_char( "You failed.\r\n", ch );
   return;
    }

    if ( IS_SET(victim->comm, COMM_NOTELL) )
    {
   REMOVE_BIT(victim->comm, COMM_NOTELL);
   send_to_char( "You can tell again.\r\n", victim );
   send_to_char( "NOTELL removed.\r\n", ch );
   sprintf(buf,"$N restores tells to %s.",victim->name);
   wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
   SET_BIT(victim->comm, COMM_NOTELL);
   send_to_char( "You can't tell!\r\n", victim );
   send_to_char( "NOTELL set.\r\n", ch );
   sprintf(buf,"$N revokes %s's tells.",victim->name);
   wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    return;
}



void do_peace( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *rch;

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
   if ( rch->fighting != NULL )
       stop_fighting( rch, TRUE );
   if (IS_NPC(rch) && IS_SET(rch->act,ACT_AGGRESSIVE))
       REMOVE_BIT(rch->act,ACT_AGGRESSIVE);
    }

    send_to_char( "Ok.\r\n", ch );
    return;
}

void do_wizlock( CHAR_DATA *ch, char *argument )
{
    extern bool wizlock;
    wizlock = !wizlock;

    if ( wizlock )
    {
   wiznet("$N has wizlocked the game.",ch,NULL,0,0,0);
   send_to_char( "Game wizlocked.\r\n", ch );
    }
    else
    {
   wiznet("$N removes wizlock.",ch,NULL,0,0,0);
   send_to_char( "Game un-wizlocked.\r\n", ch );
    }

    return;
}

/* RT anti-newbie code */

void do_newlock( CHAR_DATA *ch, char *argument )
{
    extern bool newlock;
    newlock = !newlock;
 
    if ( newlock )
    {
   wiznet("$N locks out new characters.",ch,NULL,0,0,0);
        send_to_char( "New characters have been locked out.\r\n", ch );
    }
    else
    {
   wiznet("$N allows new characters back in.",ch,NULL,0,0,0);
        send_to_char( "Newlock removed.\r\n", ch );
    }
 
    return;
}


void do_slookup( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int sn;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
   send_to_char( "Lookup which skill or spell?\r\n", ch );
   return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
   for ( sn = 0; sn < MAX_SKILL; sn++ )
   {
       if ( skill_table[sn].name == NULL )
      break;
       sprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\r\n",
      sn, skill_table[sn].slot, skill_table[sn].name );
       send_to_char( buf, ch );
   }
    }
    else
    {
   if ( ( sn = skill_lookup( arg ) ) < 0 )
   {
       send_to_char( "No such skill or spell.\r\n", ch );
       return;
   }

   sprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\r\n",
       sn, skill_table[sn].slot, skill_table[sn].name );
   send_to_char( buf, ch );
    }

    return;
}

/* RT set replaces sset, mset, oset, and rset */

void do_set( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  
  argument = one_argument(argument,arg);
  
  if (arg[0] == '\0') {
    send_to_char("Syntax:\r\n",ch);
    send_to_char("  set char  <name> <field> <value>\r\n",ch);
    send_to_char("  set mob   <name> <field> <value>\r\n",ch);
    send_to_char("  set obj   <name> <field> <value>\r\n",ch);
    send_to_char("  set room  <room> <field> <value>\r\n",ch);
    send_to_char("  set skill <name> <spell or skill> <value>\r\n",ch);
    return;
  }

  if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character")) {
    do_function(ch, &do_mset, argument);
    return;
  }

  if (!str_prefix(arg,"skill") || !str_prefix(arg,"spell")) {
    do_function(ch, &do_sset, argument);
    return;
  }

  if (!str_prefix(arg,"object")) {
    do_function(ch, &do_oset, argument);
    return;
  }

  if (!str_prefix(arg,"room")) {
    do_function(ch, &do_rset, argument);
    return;
  }

  /* echo syntax */
  do_function(ch, &do_set, "");
}


void do_sset( CHAR_DATA *ch, char *argument )
{
  char arg1 [MAX_INPUT_LENGTH];
  char arg2 [MAX_INPUT_LENGTH];
  char arg3 [MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int value;
  int sn;
  bool fAll;
  bool fAll_skill;
  bool fAll_weave;
  
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  argument = one_argument( argument, arg3 );
  
  if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' ) {
    send_to_char( "Syntax:\r\n",ch);
    send_to_char( "  set skill <name> <weave or skill> <value>\r\n", ch);
    send_to_char( "  set skill <name> all <value>\r\n",ch);
    send_to_char( "  set skill <name> all_skill <value>\r\n", ch);
    send_to_char( "  set skill <name> all_weave <value>\r\n", ch);
    send_to_char("   (use the name of the skill, not the number)\r\n",ch);
    return;
  }

  if ( ( victim = get_char_anywhere( ch, arg1 ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if ( IS_NPC(victim) ) {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
  }

  fAll       = !str_cmp( arg2, "all" );
  fAll_skill = !str_cmp( arg2, "all_skill" );
  fAll_weave = !str_cmp( arg2, "all_weave" );
  sn   = 0;
  
  if ( !fAll && !fAll_skill && !fAll_weave && ( sn = skill_lookup( arg2 ) ) < 0 ) {
    send_to_char( "No such skill or weave.\r\n", ch );
    return;
  }
  
  /*
   * Snarf the value.
   */
  if ( !is_number( arg3 ) ) {
    send_to_char( "Value must be numeric.\r\n", ch );
    return;
  }

  value = atoi( arg3 );
  if ( value < -1 || value > 301 ) {
    send_to_char( "Value range is -1 to 301.\r\n", ch );
    return;
  }
  
  if ( fAll || fAll_skill || fAll_weave ) {
    for ( sn = 0; sn < MAX_SKILL; sn++ ) {
	 if ( skill_table[sn].name != NULL && 
		 skill_table[sn].restriction != RES_GRANTED &&
		 (skill_table[sn].rating[victim->class] > 0 || value == 0)) {
	   if (fAll)
		victim->pcdata->learned[sn] = value;
	   else if (fAll_skill && skill_table[sn].spell_fun == spell_null)
		victim->pcdata->learned[sn] = value;
	   else if (fAll_weave && skill_table[sn].spell_fun != spell_null)
		victim->pcdata->learned[sn] = value;
	 }
    }
  }
  else {
    victim->pcdata->learned[sn] = value;
  }

  // Feedback
  if (fAll)
    sprintf(buf, "All skill and weaves set to %d for %s.\r\n", value, victim->name);
  else if (fAll_skill)
    sprintf(buf, "All skills set to %d for %s.\r\n", value, victim->name);
  else if (fAll_weave)
    sprintf(buf, "All weaves set to %d for %s.\r\n", value, victim->name);
  else
    sprintf(buf, "%s set to %d for %s.\r\n", skill_table[sn].name, value, victim->name);
  
  send_to_char(buf, ch);
  
  return;
}


void do_mset( CHAR_DATA *ch, char *argument )
{
  char arg1 [MAX_INPUT_LENGTH];
  char arg2 [MAX_INPUT_LENGTH];
  char arg3 [MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int value;

  /* For flags (Merit/Flaw/Talent)*/
  long pos = NO_FLAG;
  int i=0;
  int num=0;

  smash_tilde( argument );
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  strcpy( arg3, argument );

  if (!str_cmp(arg2, "newbiehelper")) {
     sprintf(arg3, "1");	
  }
  if (!str_cmp(arg2, "forsaken")) {
    sprintf(arg3, "1");	
  }
  if (!str_cmp(arg2, "spark")) {
    sprintf(arg3, "1");	
  }
  
  if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' ) {
    send_to_char("Syntax:\r\n",ch);
    send_to_char( "  set char <name> <field> <value>\r\n",ch); 
    send_to_char( "  Field being one of:\r\n",        ch );
    send_to_char( "    email\r\n", ch);
    send_to_char( "    spheres air earth fire spirit water main bo mbo\r\n", ch);
    send_to_char( "    str int wis dex con sex class level\r\n",  ch );
    send_to_char( "    race group gold goldbank silver silverbank hp endurance\r\n",ch);
    send_to_char( "    train thirst hunger drunk full\r\n", ch );
    send_to_char( "    bounty security pkcount\r\n",       ch );
    send_to_char( "    merit, flaw, talent\r\n", ch);
    send_to_char( "    IC newbiehelper insanity\r\n", ch);
    return;
  }
  
  if ( ( victim = get_char_anywhere( ch, arg1 ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }
  
  /* clear zones for mobs */
  victim->zone = NULL;

  if ( !str_prefix (arg2, "spark"))
  {
     if (victim->pcdata->forcespark == 0)
     {
	send_to_char("Character will spark on next level\r\n",ch);
	victim->pcdata->forcespark = 1;
     }
     else
     {
	send_to_char("Spark on next level removed\r\n",ch);
	victim->pcdata->forcespark = 0;
     }
     return; 

  }
  /*
   * Snarf the value (which need not be numeric).
   */
  value = is_number( arg3 ) ? atoi( arg3 ) : -1;

  /*
   * Set something.
   */

  if (!str_cmp(arg2, "newbiehelper")) {
     if (!IS_SET(victim->act, PLR_IS_NEWBIEHELPER)) {
        SET_BIT(victim->act, PLR_IS_NEWBIEHELPER);
        sprintf(buf, "%s is now a newbiehelper.\r\n", victim->name);
        send_to_char(buf, ch);
        send_to_char("You have been made a newbiehelper by the immortals.\r\n", victim);
     }	
     else {
     	REMOVE_BIT(victim->act, PLR_IS_NEWBIEHELPER);
     	sprintf(buf, "%s is no longer a newbiehelper.\r\n", victim->name);
     	send_to_char(buf, ch);
     	send_to_char("The immortals has removed your newbiehelper status.\r\n", victim);
     }
     return;
  }
  
  // IC flags
  if (!str_cmp(arg2, "IC")) {
    if (!str_cmp(arg3, "?")) {
	 send_to_char("Available IC flags:\n\r", ch);
	 for (i=0; ic_flags[i].name != NULL; i++) {
	   sprintf(buf, "%s\n\r", ic_flags[i].name);
	   send_to_char(buf, ch);
	 }
	 return;
    }
    
    for (i=0; ic_flags[i].name != NULL; i++) {
	 if (!str_prefix(ic_flags[i].name, arg3)) {
	   pos = ic_flags[i].bit;
	   num = i;
	 }
    }
    
    if (pos == NO_FLAG) {
	 send_to_char("That IC flag doesn't exist!\n\r", ch);
	 send_to_char("Use 'set char <name> ic ?' to list available IC flags.\r\n", ch);
	 return;
    }
    else {
	 if (!IS_SET(victim->ic_flags, pos)) {
	   SET_BIT(victim->ic_flags, pos);
	   sprintf(buf, "IC flag '%s' set on %s.\r\n", ic_flags[num].name, victim->name);
	   send_to_char(buf, ch);
	 }
	 else {
	   REMOVE_BIT(victim->ic_flags, pos);
	   sprintf(buf, "IC flag '%s' removed from %s.\r\n", ic_flags[num].name, victim->name);
	   send_to_char(buf, ch);
	 }
	 return;
    }
  }

  if (!str_cmp(arg2, "merit") || !str_cmp(arg2, "merits")) {
    if (!str_cmp(arg3, "?")) {
	 send_to_char("Available Merits:\r\n", ch);
	 list_background_table(ch, merit_table);
	 return;
    }
    for (i=0; merit_table[i].name != NULL; i++) {
	 if (!str_prefix(merit_table[i].name, arg3)) {
	   pos = merit_table[i].bit;
	   num = i;
	 }
    }
    if (pos == NO_FLAG) {
	 send_to_char("That merit doesn't exist!\r\n", ch);
	 send_to_char("Use 'set char <name> merit ?' to list available merits.\r\n", ch);
	 return;
    }
    // check class
    else if ((merit_table[num].class != victim->class) && (merit_table[num].class != -1)) {
	 send_to_char("Merit not settable for this class.\r\n", ch);
	 send_to_char("Use 'set char <name> merit ?' to list available merits.\r\n", ch);
	 return;
    }
    // check race
    else if ((merit_table[num].race != NULL) && (strstr(merit_table[num].race, race_table[victim->race].name) == NULL)) {
	 send_to_char("Merit not settable for this race.\r\n", ch);
	 send_to_char("Use 'set char <name> merit ?' to list available merits.\r\n", ch);
	 return;
    }
    // check not race
    else if ((merit_table[num].nrace != NULL) && (strstr(merit_table[num].nrace, race_table[victim->race].name) != NULL)) {
	 send_to_char("Merit not allowed for this race.\r\n", ch);
	 send_to_char("Use 'set char <name> merit ?' to list available merits.\r\n", ch);
	 return;
    }
    else {
	 if (!IS_SET(victim->merits, pos)) {
	   SET_BIT(victim->merits, pos);
	   sprintf(buf, "Merit '%s' set on %s.\r\n", merit_table[num].name, victim->name);
	   send_to_char(buf, ch);
	 }
	 else {
	   REMOVE_BIT(victim->merits, pos);
	   sprintf(buf, "Merit '%s' removed from %s.\r\n", merit_table[num].name, victim->name);
	   send_to_char(buf, ch);
	 }
	 return;
    }
  }

  if (!str_cmp(arg2, "flaw") || !str_cmp(arg2, "flaws")) {
    if (!str_cmp(arg3, "?")) {
	 send_to_char("Available Flaws:\r\n", ch);
	 list_background_table(ch, flaw_table);
	 return;
    }
    for (i=0; flaw_table[i].name != NULL; i++) {
	 if (!str_prefix(flaw_table[i].name, arg3)) {
	   pos = flaw_table[i].bit;
	   num = i;
	 }
    }
    if (pos == NO_FLAG) {
	 send_to_char("That flaw doesn't exist!\r\n", ch);
	 send_to_char("Use 'set char <name> flaw ?' to list available flaws.\r\n", ch);
	 return;
    }
    // check class
    else if ((flaw_table[num].class != victim->class) && (flaw_table[num].class != -1)) {
	 send_to_char("Flaw not settable for this class.\r\n", ch);
	 send_to_char("Use 'set char <name> flaw ?' to list available flaws.\r\n", ch);
	 return;
    }
    // check race
    else if ((flaw_table[num].race != NULL) && (strstr(flaw_table[num].race, race_table[victim->race].name) == NULL)) {
	 send_to_char("Flaw not settable for this race.\r\n", ch);
	 send_to_char("Use 'set char <name> flaw ?' to list available flaws.\r\n", ch);
	 return;
    }
    // check not race
    else if ((flaw_table[num].nrace != NULL) && (strstr(flaw_table[num].nrace, race_table[victim->race].name) != NULL)) {
	 send_to_char("Flaw not allowed for this race.\r\n", ch);
	 send_to_char("Use 'set char <name> flaw ?' to list available flaws.\r\n", ch);
	 return;
    }
    else {
	 if (!IS_SET(victim->flaws, pos)) {
	   SET_BIT(victim->flaws, pos);
	   sprintf(buf, "Flaw '%s' set on %s.\r\n", flaw_table[num].name, victim->name);
	   send_to_char(buf, ch);
	 }
	 else {
	   REMOVE_BIT(victim->flaws, pos);
	   sprintf(buf, "Flaw '%s' removed from %s.\r\n", flaw_table[num].name, victim->name);
	   send_to_char(buf, ch);
	 }
	 return;
    }
  }

  if (!str_cmp(arg2, "talent") || !str_cmp(arg2, "talents")) {
    if (!str_cmp(arg3, "?")) {
	 send_to_char("Available Talents:\r\n", ch);
	 list_background_table(ch, talent_table);
	 return;
    }
    for (i=0; talent_table[i].name != NULL; i++) {
	 if (!str_prefix(talent_table[i].name, arg3)) {
	   pos = talent_table[i].bit;
	   num = i;
	 }
    }
    if (pos == NO_FLAG) {
	 send_to_char("That talent doesn't exist!\r\n", ch);
	 send_to_char("Use 'set char <name> talent ?' to list available talents.\r\n", ch);
	 return;
    }
    // check class
    else if ((talent_table[num].class != victim->class) && (talent_table[num].class != -1)) {
	 send_to_char("Talent not settable for this class.\r\n", ch);
	 send_to_char("Use 'set char <name> talent ?' to list available talents.\r\n", ch);
	 return;
    }
    // check race
    else if ((talent_table[num].race != NULL) && (strstr(talent_table[num].race, race_table[victim->race].name) == NULL)) {
	 send_to_char("Talent not settable for this race.\r\n", ch);
	 send_to_char("Use 'set char <name> talent ?' to list available talents.\r\n", ch);
	 return;
    }
    // check not race
    else if ((talent_table[num].nrace != NULL) && (strstr(talent_table[num].nrace, race_table[victim->race].name) != NULL)) {
	 send_to_char("Talent not allowed for this race.\r\n", ch);
	 send_to_char("Use 'set char <name> talent ?' to list available talents.\r\n", ch);
	 return;
    }
    else {
	 if (!IS_SET(victim->talents, pos)) {
	   SET_BIT(victim->talents, pos);
	   sprintf(buf, "Talent '%s' set on %s.\r\n", talent_table[num].name, victim->name);
	   send_to_char(buf, ch);
	 }
	 else {
	   REMOVE_BIT(victim->talents, pos);
	   sprintf(buf, "Talent '%s' removed from %s.\r\n", talent_table[num].name, victim->name);
	   send_to_char(buf, ch);
	 }
	 return;
    }
  }
  
  if ( !str_cmp( arg2, "email")) {
    if (arg3[0]=='\0' || !(!str_cmp(arg3,"none") || strstr(arg3,"@"))) {
    send_to_char("Error in Email address. Must include '{W@{x' or set to '{Wnone{x'\r\n", ch);
    return;
    }
    free_string(victim->pcdata->email);
    victim->pcdata->email = str_dup(arg3);
    send_to_char("Email address set.\r\n", ch);
    return;
  }

  if ( !str_cmp( arg2, "spheres")) {
    if (value < 0 || value > 32000 ) {
    sprintf(buf, "Sphere range is from 0 to 32,000.\r\n");
    send_to_char(buf, ch);
    return;
    }
    victim->perm_sphere[SPHERE_AIR] = value;
    victim->cre_sphere[SPHERE_AIR] = value;
    victim->perm_sphere[SPHERE_EARTH] = value;
    victim->cre_sphere[SPHERE_EARTH] = value;
    victim->perm_sphere[SPHERE_FIRE] = value;
    victim->cre_sphere[SPHERE_FIRE] = value;
    victim->perm_sphere[SPHERE_SPIRIT] = value;
    victim->cre_sphere[SPHERE_SPIRIT] = value;
    victim->perm_sphere[SPHERE_WATER] = value;
    victim->cre_sphere[SPHERE_WATER] = value;
    sprintf(buf, "%s's {WSpheres{x is set to %d.\r\n", victim->name, value);
    send_to_char(buf, ch);
    return;
  }
  
  if ( !str_cmp( arg2, "air")) {
    if (value < 0 || value > 32000 ) {
    sprintf(buf, "Air sphere range is from 0 to 32,000.\r\n");
    send_to_char(buf, ch);
    return;
    }
    victim->perm_sphere[SPHERE_AIR] = value;
    victim->cre_sphere[SPHERE_AIR] = value;
    sprintf(buf, "%s's {WAir{x sphere is set to %d.\r\n", victim->name, value);
    send_to_char(buf, ch);
    return;
  }
  
  if ( !str_cmp( arg2, "earth")) {
    if (value < 0 || value > 32000 ) {
    sprintf(buf, "Earth sphere range is from 0 to 32,000.\r\n");
    send_to_char(buf, ch);
    return;
    }
    victim->perm_sphere[SPHERE_EARTH] = value;
    victim->cre_sphere[SPHERE_EARTH] = value;
    sprintf(buf, "%s's {yEarth{x sphere is set to %d.\r\n", victim->name, value);
    send_to_char(buf, ch);
    return;
  }
  
  if ( !str_cmp( arg2, "fire")) {
    if (value < 0 || value > 32000 ) {
    sprintf(buf, "Fire sphere range is from 0 to 32,000.\r\n");
    send_to_char(buf, ch);
    return;
    }
    victim->perm_sphere[SPHERE_FIRE] = value;
    victim->cre_sphere[SPHERE_FIRE] = value;
    sprintf(buf, "%s's {RFire{x sphere is set to %d.\r\n", victim->name, value);
    send_to_char(buf, ch);
    return;
  }
  
  if ( !str_cmp( arg2, "spirit")) {
    if (value < 0 || value > 32000 ) {
    sprintf(buf, "Spirit sphere range is from 0 to 32,000.\r\n");
    send_to_char(buf, ch);
    return;
    }
    victim->perm_sphere[SPHERE_SPIRIT] = value;
    victim->cre_sphere[SPHERE_SPIRIT] = value;
    sprintf(buf, "%s's {YSpirit{x sphere is set to %d.\r\n", victim->name, value);
    send_to_char(buf, ch);
    return;
  }
  
  if ( !str_cmp( arg2, "water")) {
    if (value < 0 || value > 32000 ) {
    sprintf(buf, "Water sphere range is from 0 to 32,000.\r\n");
    send_to_char(buf, ch);
    return;
    }
    victim->perm_sphere[SPHERE_WATER] = value;
    victim->cre_sphere[SPHERE_WATER] = value;
    sprintf(buf, "%s's {BWater{x sphere is set to %d.\r\n", victim->name, value);
    send_to_char(buf, ch);
    return;
  }
  
  if ( !str_cmp( arg2, "main")) {
    int sphere = -1;
    if (arg3[0] == '\0') {
    send_to_char("Invalid choice, sphere not defined.\r\n", ch);
    return;
    }
    else if (!str_prefix (arg3, "air")) {
    sphere = SPHERE_AIR;
    }
    else if (!str_prefix (arg3, "earth")) {
    sphere = SPHERE_EARTH;
    }
    else if (!str_prefix (arg3, "fire")) {
    sphere = SPHERE_FIRE;
    }
    else if (!str_prefix (arg3, "spirit")) {
    sphere = SPHERE_SPIRIT;
    }
    else if (!str_prefix (arg3, "water")) {
    sphere = SPHERE_WATER;
    }
    else {
    send_to_char("Invalid choice, sphere non-existant.\r\n", ch);
    return;
    }
    victim->main_sphere = sphere;
    sprintf(buf, "%s's main sphere set to '%s'.\r\n", victim->name, sphere_table[sphere].name);
    send_to_char(buf, ch);
    return;
  }

  if (!str_cmp(arg2, "bo")) {
    victim->burnout = value;
    sprintf(buf, "%s's burnout value is set to %d.\r\n", victim->name, value);
    send_to_char(buf, ch);
    return;
  }
  
  if (!str_cmp(arg2, "mbo")) {
    victim->max_burnout = value;
    sprintf(buf, "%s's max burnout value is set to %d.\r\n", victim->name, value);
    send_to_char(buf, ch);
    return;
  }

  if ( !str_cmp( arg2, "str" ) ) {
    if ( value < 3 || value > get_max_train(victim,STAT_STR) ) {
    sprintf(buf, "Strength range is 3 to %d\r\n.",
          get_max_train(victim,STAT_STR));
    send_to_char(buf,ch);
    return;
    }

    victim->perm_stat[STAT_STR] = value;
    return;
  }


  if ( !str_cmp( arg2, "insanity" ) ) {
    if ( value < 0 || value > 50 ) {
    sprintf(buf, "Insanity range is 0 to 50\r\n.");
    send_to_char(buf,ch);
    return;
    }

    victim->insanity_points = value;
    return;
  }

  if ( !str_cmp( arg2, "bounty" )  ) {
    if (IS_NPC( victim ) ) {
    send_to_char( "Not on NPC's.\r\n", ch);
    return;
    }
    victim->pcdata->bounty = value;
    send_to_char("Bounty Set!\r\n",ch);
    return;
  };
  if ( !str_cmp( arg2, "security" ) ) {   /* OLC */
    if ( IS_NPC(ch) ) {
    send_to_char( "Si, claro.\r\n", ch );
    return;
    }
    
    if ( IS_NPC( victim ) ) {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
    }

    if ( value > ch->pcdata->security || value < 0 ) {
    if ( ch->pcdata->security != 0 ) {
      sprintf( buf, "Valid security is 0-%d.\r\n",
           ch->pcdata->security );
      send_to_char( buf, ch );
    }
    else {
      send_to_char( "Valid security is 0 only.\r\n", ch );
    }
    return;
    }
    victim->pcdata->security = value;
    return;
  }

  if ( !str_cmp( arg2, "int" ) ) {
    if ( value < 3 || value > get_max_train(victim,STAT_INT) ) {
    sprintf(buf,
          "Intelligence range is 3 to %d.\r\n",
          get_max_train(victim,STAT_INT));
    send_to_char(buf,ch);
    return;
    }
    
    victim->perm_stat[STAT_INT] = value;
    return;
  }
  
  if ( !str_cmp( arg2, "wis" ) ) {
    if ( value < 3 || value > get_max_train(victim,STAT_WIS) ) {
    sprintf(buf,
          "Wisdom range is 3 to %d.\r\n",get_max_train(victim,STAT_WIS));
    send_to_char( buf, ch );
    return;
    }

    victim->perm_stat[STAT_WIS] = value;
    return;
  }

  if ( !str_cmp( arg2, "dex" ) ) {
    if ( value < 3 || value > get_max_train(victim,STAT_DEX) ) {
    sprintf(buf,
          "Dexterity range is 3 to %d.\r\n",
          get_max_train(victim,STAT_DEX));
    send_to_char( buf, ch );
    return;
    }

    victim->perm_stat[STAT_DEX] = value;
    return;
  }
  
  if ( !str_cmp( arg2, "con" ) ) {
    if ( value < 3 || value > get_max_train(victim,STAT_CON) ) {
    sprintf(buf,
          "Constitution range is 3 to %d.\r\n",
          get_max_train(victim,STAT_CON));
    send_to_char( buf, ch );
    return;
    }
    
    victim->perm_stat[STAT_CON] = value;
    return;
  }
  
  if ( !str_prefix( arg2, "sex" ) ) {
    if ( value < 0 || value > 2 ) {
    send_to_char( "Sex range is 0 to 2.\r\n", ch );
    return;
    }
    victim->sex = value;
    if (!IS_NPC(victim))
    victim->pcdata->true_sex = value;
    return;
  }
  
  if ( !str_prefix( arg2, "class" ) ) {
    int class;
    
    if (IS_NPC(victim)) {
    send_to_char("Mobiles have no class.\r\n",ch);
    return;
    }
    
    class = class_lookup(arg3);
    if ( class == -1 ) {
    char buf[MAX_STRING_LENGTH];
    
    strcpy( buf, "Possible classes are: " );
    for ( class = 0; class < MAX_CLASS; class++ ) {
      if ( class > 0 )
      strcat( buf, " " );
      strcat( buf, class_table[class].name );
    }
    strcat( buf, ".\r\n" );
    
    send_to_char(buf,ch);
    return;
    }
    
    victim->class = class;
    return;
  }
  
  if ( !str_prefix( arg2, "level" ) ) {
    if ( !IS_NPC(victim) ) {
    send_to_char( "Not on PC's.\r\n", ch );
    return;
    }
    
    if ( value < 0 || value > MAX_LEVEL ) {
    sprintf(buf, "Level range is 0 to %d.\r\n", MAX_LEVEL);
    send_to_char(buf, ch);
    return;
    }
    victim->level = value;
    return;
  }
  
  if ( !str_prefix( arg2, "gold" ) ) {
    victim->gold = value;
    sprintf(buf, "%s's gold set to %ld\n\r", IS_NPC(victim) ? victim->short_descr : victim->name, victim->gold);
    send_to_char(buf, ch);
    return;
  }
  
  if ( !str_prefix( arg2, "goldbank" ) && !IS_NPC(victim)) {
    victim->pcdata->gold_bank = value;
    sprintf(buf, "%s's gold in bank set to %ld\n\r", IS_NPC(victim) ? victim->short_descr : victim->name, victim->pcdata->gold_bank);
    send_to_char(buf, ch);
    return;
  }
    
  if ( !str_prefix(arg2, "silver" ) ) {
    victim->silver = value;
    sprintf(buf, "%s's silver set to %ld\n\r", IS_NPC(victim) ? victim->short_descr : victim->name, victim->silver);
    send_to_char(buf, ch);  
    return;
  }
  
  if ( !str_prefix( arg2, "silverbank" ) && !IS_NPC(victim)) {
    victim->pcdata->silver_bank = value;
    sprintf(buf, "%s's silver in bank set to %ld\n\r", IS_NPC(victim) ? victim->short_descr : victim->name, victim->pcdata->silver_bank);
    send_to_char(buf, ch);
    return;
  }
    
  if ( !str_prefix( arg2, "hp" ) ) {
    if ( value < -10 || value > 30000 ) {
    send_to_char( "Hp range is -10 to 30,000 hit points.\r\n", ch );
    return;
    }
    victim->max_hit = value;
    if (!IS_NPC(victim))
    victim->pcdata->perm_hit = value;
    return;
  }
  
  if ( !str_prefix( arg2, "endurance" ) ) {
    if ( value < 0 || value > 30000 ) {
    send_to_char( "Endurance range is 0 to 30,000 endurance points.\r\n", ch );
    return;
    }
    victim->max_endurance = value;
    if (!IS_NPC(victim))
    victim->pcdata->perm_endurance = value;
    return;
  }
  
  if ( !str_prefix( arg2, "train" )) {
    if (value < 0 || value > 50 ) {
    send_to_char("Training session range is 0 to 50 sessions.\r\n",ch);
    return;
    }
    victim->train = value;
    return;
  }
  
  if ( !str_prefix( arg2, "thirst" ) ) {
    if ( IS_NPC(victim) ) {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
    }
    
    if ( value < -1 || value > 100 ) {
    send_to_char( "Thirst range is -1 to 100.\r\n", ch );
    return;
    }
    
    victim->pcdata->condition[COND_THIRST] = value;
    return;
  }
  
  if ( !str_prefix( arg2, "drunk" ) ) {
    if ( IS_NPC(victim) ) {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
    }
    
    if ( value < -1 || value > 100 ) {
    send_to_char( "Drunk range is -1 to 100.\r\n", ch );
    return;
    }
    
    victim->pcdata->condition[COND_DRUNK] = value;
    return;
  }
  
  if ( !str_prefix( arg2, "full" ) ) {
    if ( IS_NPC(victim) ) {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
    }
    
    if ( value < -1 || value > 100 ) {
    send_to_char( "Full range is -1 to 100.\r\n", ch );
    return;
    }
    
    victim->pcdata->condition[COND_FULL] = value;
    return;
  }
  
  if ( !str_prefix( arg2, "hunger" ) ) {
    if ( IS_NPC(victim) ) {
    send_to_char( "Not on NPC's.\r\n", ch );
    return;
    }
    
    if ( value < -1 || value > 100 ) {
    send_to_char( "Full range is -1 to 100.\r\n", ch );
    return;
    }
    
    victim->pcdata->condition[COND_HUNGER] = value;
    return;
  }
  
  if (!str_prefix( arg2, "race" ) ) {
    int race;
    
    race = race_lookup(arg3);
    
    if ( race == 0) {
    send_to_char("That is not a valid race.\r\n",ch);
    return;
    }
    
    if (!IS_NPC(victim) && !race_table[race].pc_race) {
    send_to_char("That is not a valid player race.\r\n",ch);
    return;
    }
    
    victim->race = race;
    return;
  }
  
  if (!str_prefix(arg2,"group")) {
    if (!IS_NPC(victim)) {
    send_to_char("Only on NPCs.\r\n",ch);
    return;
    }
    victim->group = value;
    return;
  }

  
  /*
   * Generate usage message.
   */
  do_function(ch, &do_mset, "" );
  return;
}

void do_string( CHAR_DATA *ch, char *argument )
{
    char type [MAX_INPUT_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    smash_tilde( argument );
    argument = one_argument( argument, type );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
   send_to_char("Syntax:\r\n",ch);
   send_to_char("  string char <name> <field> <string>\r\n",ch);
   send_to_char("    fields: name wkname short long desc title spec\r\n",ch);
   send_to_char("            wolfappearance dreamappearance\r\n", ch);
   send_to_char("  string obj  <name> <field> <string>\r\n",ch);
   send_to_char("    fields: name short long extended\r\n",ch);
   return;
    }
    
    if (!str_prefix(type,"character") || !str_prefix(type,"mobile"))
    {
      if ( ( victim = get_char_anywhere( ch, arg1 ) ) == NULL )
      {
       send_to_char( "They aren't here.\r\n", ch );
       return;
      }

   /* clear zone for mobs */
   victim->zone = NULL;

   /* string something */

      if ( !str_prefix( arg2, "name" ) )
      {
       if ( !IS_NPC(victim) )
       {
         send_to_char( "Not on PC's.\r\n", ch );
         return;
       }
       free_string( victim->name );
       victim->name = str_dup( arg3 );
       return;
      }

      if ( !str_prefix( arg2, "wkname" ) )
      {
       if (IS_NPC(victim) )
       {
         send_to_char( "Not on MOB's\r\n", ch );
         return;
       }
       free_string( victim->wkname );
       victim->wkname = str_dup( arg3 );
       return;
      }      
      
      if ( !str_prefix( arg2, "description" ) )
      {
          free_string(victim->description);
          victim->description = str_dup(arg3);
          return;
      }

      if ( !str_prefix( arg2, "short" ) )
      {
       free_string( victim->short_descr );
       victim->short_descr = str_dup( arg3 );
       return;
      }

      if ( !str_prefix( arg2, "long" ) )
      {
       free_string( victim->long_descr );
       strcat(arg3,"\r\n");
       victim->long_descr = str_dup( arg3 );
       return;
      }

      if ( !str_prefix( arg2, "title" ) )
      {
       if ( IS_NPC(victim) )
       {
         send_to_char( "Not on NPC's.\r\n", ch );
         return;
       }

       set_title( victim, arg3 );
       return;
      }

      if ( !str_prefix( arg2, "spec" ) )
      {
       if ( !IS_NPC(victim) )
       {
         send_to_char( "Not on PC's.\r\n", ch );
         return;
       }

       if ( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 )
       {
         send_to_char( "No such spec fun.\r\n", ch );
         return;
       }

       return;
      }
      
      if ( !str_prefix( arg2, "wolfappearance" ) )
      {
       if (IS_NPC(victim) )
       {
         send_to_char( "Not on MOB's\r\n", ch );
         return;
       }
       free_string( victim->pcdata->wolf_appearance );
       victim->pcdata->wolf_appearance = str_dup( arg3 );
       return;
      }            

      if ( !str_prefix( arg2, "dreamappearance" ) )
      {
       if (IS_NPC(victim) )
       {
         send_to_char( "Not on MOB's\r\n", ch );
         return;
       }
       free_string( victim->pcdata->dreaming_appearance );
       victim->pcdata->dreaming_appearance = str_dup( arg3 );
       return;
      }            
      
    }
    
    if (!str_prefix(type,"object"))
    {
      /* string an obj */
      
      if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
      {
       send_to_char( "Nothing like that in heaven or earth.\r\n", ch );
       return;
      }
      
        if ( !str_prefix( arg2, "name" ) )
      {
       free_string( obj->name );
       obj->name = str_dup( arg3 );
       return;
      }

      if ( !str_prefix( arg2, "short" ) )
      {
       free_string( obj->short_descr );
       obj->short_descr = str_dup( arg3 );
       return;
      }

      if ( !str_prefix( arg2, "long" ) )
      {
       free_string( obj->description );
       obj->description = str_dup( arg3 );
       return;
      }

      if ( !str_prefix( arg2, "ed" ) || !str_prefix( arg2, "extended"))
      {
       EXTRA_DESCR_DATA *ed;

       argument = one_argument( argument, arg3 );
       if ( argument == NULL )
       {
         send_to_char( "Syntax: oset <object> ed <keyword> <string>\r\n",
          ch );
         return;
       }

       strcat(argument,"\r\n");

       ed = new_extra_descr();

       ed->keyword      = str_dup( arg3     );
       ed->description  = str_dup( argument );
       ed->next      = obj->extra_descr;
       obj->extra_descr = ed;
       return;
      }
    }
    
      
    /* echo bad use message */
    do_function(ch, &do_string, "");
}



void do_oset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int value;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
   send_to_char("Syntax:\r\n",ch);
   send_to_char("  set obj <object> <field> <value>\r\n",ch);
   send_to_char("  Field being one of:\r\n",          ch );
   send_to_char("    value0 value1 value2 value3 value4 (v1-v4)\r\n",   ch );
   send_to_char("    extra wear level weight cost timer\r\n",     ch );
   return;
    }

    if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
    {
   send_to_char( "Nothing like that in heaven or earth.\r\n", ch );
   return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
    {
   obj->value[0] = UMIN(50,value);
   return;
    }

    if ( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
    {
   obj->value[1] = value;
   return;
    }

    if ( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
    {
   obj->value[2] = value;
   return;
    }

    if ( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
    {
   obj->value[3] = value;
   return;
    }

    if ( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
    {
   obj->value[4] = value;
   return;
    }

    if ( !str_prefix( arg2, "extra" ) )
    {
   obj->extra_flags = value;
   return;
    }

    if ( !str_prefix( arg2, "wear" ) )
    {
   obj->wear_flags = value;
   return;
    }

    if ( !str_prefix( arg2, "level" ) )
    {
   obj->level = value;
   return;
    }
   
    if ( !str_prefix( arg2, "weight" ) )
    {
   obj->weight = value;
   return;
    }

    if ( !str_prefix( arg2, "cost" ) )
    {
   obj->cost = value;
   return;
    }

    if ( !str_prefix( arg2, "timer" ) )
    {
   obj->timer = value;
   return;
    }
   
    /*
     * Generate usage message.
     */
    do_function(ch, &do_oset, "" );
    return;
}



void do_rset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    int value;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
   send_to_char( "Syntax:\r\n",ch);
   send_to_char( "  set room <location> <field> <value>\r\n",ch);
   send_to_char( "  Field being one of:\r\n",         ch );
   send_to_char( "    flags sector\r\n",           ch );
   return;
    }

    if ( ( location = find_location( ch, arg1 ) ) == NULL )
    {
   send_to_char( "No such location.\r\n", ch );
   return;
    }

    if (!is_room_owner(ch,location) && ch->in_room != location 
    &&  room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That room is private right now.\r\n",ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if ( !is_number( arg3 ) )
    {
   send_to_char( "Value must be numeric.\r\n", ch );
   return;
    }
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_prefix( arg2, "flags" ) )
    {
   location->room_flags = value;
   return;
    }

    if ( !str_prefix( arg2, "sector" ) )
    {
   location->sector_type   = value;
   return;
    }

    /*
     * Generate usage message.
     */
    do_function(ch, &do_rset, "");
    return;
}



/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
   send_to_char( "Force whom to do what?\r\n", ch );
   return;
    }

    one_argument(argument,arg2);
  
    if (!str_cmp(arg2,"delete") || !str_prefix(arg2,"mob"))
    {
   send_to_char("That will NOT be done.\r\n",ch);
   return;
    }

    sprintf( buf, "$n forces you to '%s'.", argument );

    if ( !str_cmp( arg, "all" ) )
    {
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;

   if (get_trust(ch) < MAX_LEVEL - 3)
   {
       send_to_char("Not at your level!\r\n",ch);
       return;
   }

   for ( vch = char_list; vch != NULL; vch = vch_next )
   {
       vch_next = vch->next;

       if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch ) )
       {
      act( buf, ch, NULL, vch, TO_VICT );
      interpret( vch, argument );
       }
   }
    }
    else if (!str_cmp(arg,"players"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;
 
        if (get_trust(ch) < MAX_LEVEL - 2)
        {
            send_to_char("Not at your level!\r\n",ch);
            return;
        }
 
        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;
 
            if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch ) 
       &&    vch->level < LEVEL_HERO)
            {
                act( buf, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else if (!str_cmp(arg,"gods"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;
 
        if (get_trust(ch) < MAX_LEVEL - 2)
        {
            send_to_char("Not at your level!\r\n",ch);
            return;
        }
 
        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;
 
            if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
            &&   vch->level >= LEVEL_HERO)
            {
                act( buf, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else
    {
   CHAR_DATA *victim;

   if ( ( victim = get_char_anywhere( ch, arg ) ) == NULL )
   {
       send_to_char( "They aren't here.\r\n", ch );
       return;
   }

   if ( victim == ch )
   {
       send_to_char( "Aye aye, right away!\r\n", ch );
       return;
   }

      if (!is_room_owner(ch,victim->in_room) 
   &&  ch->in_room != victim->in_room 
        &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
      {
            send_to_char("That character is in a private room.\r\n",ch);
            return;
        }

   if ( get_trust( victim ) >= get_trust( ch ) )
   {
       send_to_char( "Do it yourself!\r\n", ch );
       return;
   }

   if ( !IS_NPC(victim) && get_trust(ch) < MAX_LEVEL -3)
   {
       send_to_char("Not at your level!\r\n",ch);
       return;
   }

   act( buf, ch, NULL, victim, TO_VICT );
   interpret( victim, argument );
    }

    send_to_char( "Ok.\r\n", ch );
    return;
}



/*
 * New routines by Dionysos.
 */
void do_invis( CHAR_DATA *ch, char *argument )
{
    int level;
    char arg[MAX_STRING_LENGTH];

    /* RT code for taking a level argument */
    one_argument( argument, arg );

    if ( arg[0] == '\0' ) 
    /* take the default path */

      if ( ch->invis_level)
      {
     ch->invis_level = 0;
     act( "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
     send_to_char( "You slowly fade back into existence.\r\n", ch );
      }
      else
      {
     ch->invis_level = get_trust(ch);
     act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
     send_to_char( "You slowly vanish into thin air.\r\n", ch );
      }
    else
    /* do the level thing */
    {
      level = atoi(arg);
      if (level < 2 || level > get_trust(ch))
      {
   send_to_char("Invis level must be between 2 and your level.\r\n",ch);
        return;
      }
      else
      {
     ch->reply = NULL;
          ch->invis_level = level;
          act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You slowly vanish into thin air.\r\n", ch );
      }
    }

    return;
}


void do_incognito( CHAR_DATA *ch, char *argument )
{
    int level;
    char arg[MAX_STRING_LENGTH];
 
    /* RT code for taking a level argument */
    one_argument( argument, arg );
 
    if ( arg[0] == '\0' )
    /* take the default path */
 
      if ( ch->incog_level)
      {
          ch->incog_level = 0;
          act( "$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You are no longer cloaked.\r\n", ch );
      }
      else
      {
          ch->incog_level = get_trust(ch);
          act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You cloak your presence.\r\n", ch );
      }
    else
    /* do the level thing */
    {
      level = atoi(arg);
      if (level < 2 || level > get_trust(ch))
      {
        send_to_char("Incog level must be between 2 and your level.\r\n",ch);
        return;
      }
      else
      {
          ch->reply = NULL;
          ch->incog_level = level;
          act( "$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You cloak your presence.\r\n", ch );
      }
    }
 
    return;
}



void do_holylight( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
   return;

    if ( IS_SET(ch->act, PLR_HOLYLIGHT) )
    {
   REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
   send_to_char( "Holy light mode off.\r\n", ch );
    }
    else
    {
   SET_BIT(ch->act, PLR_HOLYLIGHT);
   send_to_char( "Holy light mode on.\r\n", ch );
    }

    return;
}

/* prefix command: it will put the string typed on each line typed */

void do_prefi (CHAR_DATA *ch, char *argument)
{
    send_to_char("You cannot abbreviate the prefix command.\r\n",ch);
    return;
}

void do_prefix (CHAR_DATA *ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
    {
   if (ch->prefix[0] == '\0')
   {
       send_to_char("You have no prefix to clear.\r\n",ch);
       return;
   }

   send_to_char("Prefix removed.\r\n",ch);
   free_string(ch->prefix);
   ch->prefix = str_dup("");
   return;
    }

    if (ch->prefix[0] != '\0')
    {
   sprintf(buf,"Prefix changed to %s.\r\n",argument);
   free_string(ch->prefix);
    }
    else
    {
   sprintf(buf,"Prefix set to %s.\r\n",argument);
    }

    ch->prefix = str_dup(argument);
}


void do_altcount( CHAR_DATA *ch, char *argument) {
    CHAR_DATA       *vch;
    DESCRIPTOR_DATA *d;
    DESCRIPTOR_DATA *d2;
    char            buf  [ MAX_STRING_LENGTH ];
    char            buf2 [ MAX_STRING_LENGTH ];
    int             count;
    int             alt_count;
    char *          st;
    char            s[100];
    char            idle[10];


    alt_count       = 0;
    count       = 0;
    buf[0]      = '\0';
    buf2[0]     = '\0';

    strcat( buf2, "\r\n[Num Connected_State Login@  Idl] Player Name  Host\r\n" );
    strcat( buf2, "--------------------------------------------------------------------------------\r\n");  
    for ( d = descriptor_list; d; d = d->next )
    {
        count++;
	for (d2 = d->next;d2;d2 = d2->next)
        {
	   if (!strcmp(d->host,d2->host)) {  //If they're from the same host
	      alt_count++;
	      if ( d2->character && can_see_channel( ch, d2->character ) )
      	      {
      		/* NB: You may need to edit the CON_ values */
                switch( d2->connected ) {
                case CON_PLAYING:              st = "    PLAYING    ";    break;
                case CON_GET_NAME:             st = "   Get Name    ";    break;
                case CON_GET_OLD_PASSWORD:     st = "Get Old Passwd ";    break;
                case CON_CONFIRM_NEW_NAME:     st = " Confirm Name  ";    break;
                case CON_GET_NEW_PASSWORD:     st = "Get New Passwd ";    break;
                case CON_CONFIRM_NEW_PASSWORD: st = "Confirm Passwd ";    break;
                case CON_GET_NEW_RACE:         st = "  Get New Race ";    break;
                case CON_GET_NEW_SEX:          st = "  Get New Sex  ";    break;
                case CON_GET_NEW_CLASS:        st = " Get New Class ";    break;
                case CON_PICK_WEAPON:       st = " Picking Weapon";      break;
                case CON_READ_IMOTD:     st = " Reading IMOTD ";    break;
                case CON_BREAK_CONNECT:     st = "   LINKDEAD    ";      break;
                case CON_READ_MOTD:            st = "  Reading MOTD ";    break;
                case CON_GET_EMAIL:            st = "   Get Email   ";    break;
                case CON_GET_APPEARANCE:       st = "Get Appearance ";    break;
                case CON_SELECTION_MENU:       st = "Selection Menu ";    break;
                case CON_GET_SPHERES:          st = "  Get Spheres  ";    break;
                case CON_GET_STATS:            st = "   Get Stats   ";    break;
                case CON_GET_MERITS:           st = "   Get Merits  ";    break;
                case CON_GET_FLAWS:            st = "   Get Flaws   ";    break;
                case CON_GET_TALENTS:          st = "  Get Talents  ";    break;
                default:                       st = "   !UNKNOWN!   ";    break;
              }
                     
           /* Format "login" value... */
           vch = d2->original ? d2->original : d2->character;
           strftime( s, 100, "%I:%M%p", localtime( &vch->logon ) );
           
           if ( vch->timer > 0 )
              sprintf( idle, "%3d", vch->timer );
           else
              sprintf( idle, "   " );
           
           sprintf( buf, "[%3d {y%s{x %7s %2s] %-12s {g%-56.56s{x\r\n",
              d2->descriptor,
              st,
              s,
              idle,
              (d2->original ) ? d2->original->name : ( d2->character )  ? d2->character->name : "(None!)",
              d2->host );
              
           strcat( buf2, buf );
         }
        } 
      }
    }

    sprintf( buf, "\r\n[%d] active user%s found.\r\n", count, count == 1 ? "" : "s" );
    strcat( buf2, buf );
    sprintf( buf, "\r\n[%d] duplicate user%s found.\r\n", alt_count, count == 1 ? "" : "s" );
    strcat( buf2, buf );
    send_to_char( buf2, ch );
    return;
}


/* Written by Stimpy, ported to rom2.4 by Silverhand 3/12
 *
 * Added the other COMM_ stuff that wasn't defined before 4/16 -Silverhand
 */
void do_sockets( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA       *vch;
    DESCRIPTOR_DATA *d;
    char            buf  [ MAX_STRING_LENGTH ];
    char            buf2 [ MAX_STRING_LENGTH ];
    int             count;
    char *          st;
    char            s[100];
    char            idle[10];


    count       = 0;
    buf[0]      = '\0';
    buf2[0]     = '\0';

    strcat( buf2, "\r\n[Num Connected_State Login@  Idl] Player Name  Host\r\n" );
    strcat( buf2, "--------------------------------------------------------------------------------\r\n");  
    for ( d = descriptor_list; d; d = d->next )
    {
        if ( d->character && can_see_channel( ch, d->character ) )
        {
      /* NB: You may need to edit the CON_ values */
      switch( d->connected ) {
      case CON_PLAYING:              st = "    PLAYING    ";    break;
      case CON_GET_NAME:             st = "   Get Name    ";    break;
      case CON_GET_OLD_PASSWORD:     st = "Get Old Passwd ";    break;
      case CON_CONFIRM_NEW_NAME:     st = " Confirm Name  ";    break;
      case CON_GET_NEW_PASSWORD:     st = "Get New Passwd ";    break;
      case CON_CONFIRM_NEW_PASSWORD: st = "Confirm Passwd ";    break;
      case CON_GET_NEW_RACE:         st = "  Get New Race ";    break;
      case CON_GET_NEW_SEX:          st = "  Get New Sex  ";    break;
      case CON_GET_NEW_CLASS:        st = " Get New Class ";    break;
      case CON_PICK_WEAPON:       st = " Picking Weapon";      break;
      case CON_READ_IMOTD:     st = " Reading IMOTD ";    break;
      case CON_BREAK_CONNECT:     st = "   LINKDEAD    ";      break;
      case CON_READ_MOTD:            st = "  Reading MOTD ";    break;
      case CON_GET_EMAIL:            st = "   Get Email   ";    break;
      case CON_GET_APPEARANCE:       st = "Get Appearance ";    break;
      case CON_SELECTION_MENU:       st = "Selection Menu ";    break;
      case CON_GET_SPHERES:          st = "  Get Spheres  ";    break;
      case CON_GET_STATS:            st = "   Get Stats   ";    break;
      case CON_GET_MERITS:           st = "   Get Merits  ";    break;
      case CON_GET_FLAWS:            st = "   Get Flaws   ";    break;
      case CON_GET_TALENTS:          st = "  Get Talents  ";    break;
      default:                       st = "   !UNKNOWN!   ";    break;
           }
           count++;
           
           /* Format "login" value... */
           vch = d->original ? d->original : d->character;
           strftime( s, 100, "%I:%M%p", localtime( &vch->logon ) );
           
           if ( vch->timer > 0 )
              sprintf( idle, "%3d", vch->timer );
           else
              sprintf( idle, "   " );
           
           sprintf( buf, "[%3d {y%s{x %7s %2s] %-12s {g%-56.56s{x\r\n",
              d->descriptor,
              st,
              s,
              idle,
              (d->original ) ? d->original->name : ( d->character )  ? d->character->name : "(None!)",
              d->host );
              
           strcat( buf2, buf );

        }
    }

    sprintf( buf, "\r\n[%d] active user%s found.\r\n", count, count == 1 ? "" : "s" );
    strcat( buf2, buf );
    send_to_char( buf2, ch );
    return;
}

void do_initwarmboot(CHAR_DATA *ch, char *argument)
{
  char arg1[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim;

  argument = one_argument(argument, arg1);

  if (isshutdown) {
    send_to_char("There is a shutdown in progress already!\r\n", ch);
    return;
  }
  
  if (arg1[0] == '\0') {
    send_to_char("Syntax:  Warmboot Now\r\n"
			  "         Warmboot Stop\r\n"
			  "         Warmboot <# minutes> [<message>]\r\n", ch);
    return;
  }
  
  if (argument[0] == '\0') {
    warmbootMsg[0] = '\0';
  }
  else {
    sprintf(warmbootMsg, "%s\r\n", argument);
  }

  if (!str_cmp (arg1, "now")) {
    do_warmboot(ch, NULL);
    return;
  }
  
  if (!str_cmp (arg1, "stop")) {
    if (pulse_warmboot >= 0 && iswarmboot) {
    pulse_warmboot = -1;
    iswarmboot = FALSE;
    warmboot_pc = NULL;
    warmbootMsg[0] = '\0';
    for ( d = descriptor_list; d; d = d->next ) {
      if (d->character != NULL && d->connected == CON_PLAYING) {
      victim = d->character;
      send_to_char("Warmboot has been cancelled...\r\n", victim);
      }
    }
    return;
    }
    else {
    send_to_char("There is no warmboot countdown in progress.\r\n",ch);
    return;
    }
  }
  
  if (is_number(arg1)) {
    if (atoi(arg1) < 1 || atoi(arg1) > 60) {
    send_to_char("Time range is between 1 and 60 minutes.\r\n", ch);
    return;
    }
    else {
    pulse_warmboot = (atoi(arg1)*60);
    iscopyin   = FALSE;
    iswarmboot = TRUE;
    warmboot_pc = ch;
    check_warmboot( );
    return;
    }
  }   
  else {
    send_to_char("Syntax:  Warmboot Now\r\n"
           "         Warmboot Stop\r\n"
           "         Warmboot <# minutes> [<message>]\r\n", ch);
    return; 
  }
  
  return;
}

void do_initcopyinboot(CHAR_DATA *ch, char *argument)
{
  char arg1[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim;

  argument = one_argument(argument, arg1);

  if (isshutdown) {
    send_to_char("There is a shutdown in progress already!\r\n", ch);
    return;
  }
  
  if (arg1[0] == '\0') {
    send_to_char("Syntax:  CopyIn Now\r\n"
			  "         CopyIn Stop\r\n"
			  "         CopyIn <# minutes> [<message>]\r\n", ch);
    return;
  }
  
  if (argument[0] == '\0') {
    warmbootMsg[0] = '\0';
  }
  else {
    sprintf(warmbootMsg, "%s\r\n", argument);
  }

  if (!str_cmp (arg1, "now")) {
    do_copyinboot(ch, NULL);
    return;
  }
  
  if (!str_cmp (arg1, "stop")) {
    if (pulse_warmboot >= 0 && iswarmboot) {
    pulse_warmboot = -1;
    iscopyin   = FALSE;
    iswarmboot = FALSE;
    warmboot_pc = NULL;
    warmbootMsg[0] = '\0';
    for ( d = descriptor_list; d; d = d->next ) {
      if (d->character != NULL && d->connected == CON_PLAYING) {
      victim = d->character;
      send_to_char("Warmboot has been cancelled...\r\n", victim);
      }
    }
    return;
    }
    else {
    send_to_char("There is no copyin countdown in progress.\r\n",ch);
    return;
    }
  }
  
  if (is_number(arg1)) {
    if (atoi(arg1) < 1 || atoi(arg1) > 60) {
    send_to_char("Time range is between 1 and 60 minutes.\r\n", ch);
    return;
    }
    else {
    pulse_warmboot = (atoi(arg1)*60);
    iscopyin   = TRUE;
    iswarmboot = TRUE;
    warmboot_pc = ch;
    check_warmboot( );
    return;
    }
  }   
  else {
    send_to_char("Syntax:  CopyIn Now\r\n"
           "         CopyIn Stop\r\n"
           "         CopyIn <# minutes> [<message>]\r\n", ch);
    return; 
  }
  
  return;
}
 
 /* This file holds the copyover data */
 #define COPYOVER_FILE "copyover.data"
 
 /* This is the executable file */
 #define EXE_FILE   "../src/tsw"
 
 
 /*  Copyover - Original idea: Fusion of MUD++
  *  Adapted to Diku by Erwin S. Andreasen, <erwin@pip.dknet.dk>
  *  http://pip.dknet.dk/~pip1773
  *  Changed into a ROM patch after seeing the 100th request for it :)
  */
void do_warmboot (CHAR_DATA *ch, char * argument)
{
  FILE *fp;
  DESCRIPTOR_DATA *d, *d_next;
  char buf [100], buf2[100];
  extern int port,control; /* db.c */
  char *pbuff;
  char buffer[ MAX_STRING_LENGTH*2 ];

  signal(SIGSEGV,SIG_IGN);
  fflush(NULL);
  
  fp = fopen (COPYOVER_FILE, "w");
   
  if (!fp) {
    if (ch) {  
    send_to_char ("Copyover file not writeable, aborted.\r\n",ch);
    }
    _logf ("Could not write to copyover file: %s", COPYOVER_FILE);
    perror ("do_copyover:fopen");
    return;
  }

  if (ch) {
    if (ch->invis_level >= LEVEL_HERO)
    sprintf (buf, "\r\nWarmboot by someone.\r\n");
    else
    sprintf (buf, "\r\nWarmboot by %s.\r\n", COLORNAME(ch));

  }
  else {
    sprintf (buf, "\r\nCrash recovery starting...\r\n");
  }
  save_polls  ( ch, "" );         /* autosave polls */

  /* For each playing descriptor, save its state */
  for (d = descriptor_list; d ; d = d_next) {
    CHAR_DATA * och = CH (d);
    d_next = d->next; /* We delete from the list , so need to save this */
    
    if (!d->character || d->connected > CON_PLAYING) { /* drop those logging on */
	 write_to_descriptor (d->descriptor, "\r\n"
					  "Sorry, we are rebooting. Come back in a few minutes.\r\n", 0);
	 close_socket (d); /* throw'em out */
    }
    else {
	 fprintf (fp, "%d %s %s\n", d->descriptor, och->name, d->host);
	 
#if 0        /* This is not necessary for ROM */
	 if (och->level == 1) {
	   write_to_descriptor (d->descriptor, "Since you are level one, and level one characters do not save, you gain a free level!\r\n", 0);
	   advance_level (och);
	   och->level++; /* Advance_level doesn't do that */
	 }
#endif         
	 if (IS_CHANNELING(och))
         {
	 	do_unchannel(och,"");
	 }
	 save_char_obj (och, FALSE);

	 /* Save notes in progress */
	 do_nautopost(och, "self");

  	 nuke_pets(och);
	 och->pet = NULL;

	 if (ch) {
	   pbuff = buffer;
	   colourconv( pbuff, buf, ch );  
	   write_to_descriptor (d->descriptor, buffer, 0);
	   write_to_descriptor (d->descriptor, "Saving...\r\n", 0);
	   write_to_descriptor (d->descriptor, "Warmboot starting...\r\n", 0); 
	 }
	 else {
	   write_to_descriptor (d->descriptor, buf, 0);
	 }
    }
  }
  
  /* Consider changing all saved areas here, if you use OLC */
  if (!iscopyin)
     do_asave(NULL, "");

  fprintf (fp, "-1\n");
  fclose (fp);
  
  /* Close reserve and other always-open files and release other resources */
  
  fclose (fpReserve);
  
  /* exec - descriptors are inherited */
  
  sprintf (buf, "%d", port);
  sprintf (buf2, "%d", control);
  execl (EXE_FILE, "tsw", buf, "copyover", buf2, (char *) NULL);
  
  /* Failed - sucessful exec will not return */
  
  perror ("do_warmboot: execl");
  send_to_char ("Warmboot terminated...\r\n",ch);
  
  /* Here you might want to reopen fpReserve */
  fpReserve = fopen (NULL_FILE, "r");
}

void do_copyinboot (CHAR_DATA *ch, char * argument)
{
  FILE *fp;
  DESCRIPTOR_DATA *d, *d_next;
  char buf [100], buf2[100];
  extern int port,control; /* db.c */
  char *pbuff;
  char buffer[ MAX_STRING_LENGTH*2 ];

  signal(SIGSEGV,SIG_IGN);
  fflush(NULL);
  
  fp = fopen (COPYOVER_FILE, "w");
   
  if (!fp) {
    if (ch) {  
    send_to_char ("Copyover file not writeable, aborted.\r\n",ch);
    }
    _logf ("Could not write to copyover file: %s", COPYOVER_FILE);
    perror ("do_copyover:fopen");
    return;
  }

  if (ch) {
    if (ch->invis_level >= LEVEL_HERO)
    sprintf (buf, "\r\nWarmboot by someone.\r\n");
    else
    sprintf (buf, "\r\nWarmboot by %s.\r\n", COLORNAME(ch));
  }
  else {
    sprintf (buf, "\r\nCrash recovery starting...\r\n");
  }

  /* For each playing descriptor, save its state */
  for (d = descriptor_list; d ; d = d_next) {
    CHAR_DATA * och = CH (d);
    d_next = d->next; /* We delete from the list , so need to save this */
    
    if (!d->character || d->connected > CON_PLAYING) { /* drop those logging on */
    write_to_descriptor (d->descriptor, "\r\n"
                 "Sorry, we are rebooting. Come back in a few minutes.\r\n", 0);
    close_socket (d); /* throw'em out */
    }
    else {
    fprintf (fp, "%d %s %s\n", d->descriptor, och->name, d->host);
    
#if 0        /* This is not necessary for ROM */
    if (och->level == 1) {
      write_to_descriptor (d->descriptor, "Since you are level one, and level one characters do not save, you gain a free level!\r\n", 0);
      advance_level (och);
      och->level++; /* Advance_level doesn't do that */
    }
#endif         
    if (IS_CHANNELING(och))
    {
	do_unchannel(och,"");
    }
    save_char_obj (och, FALSE);

    /* Save notes in progress */
    do_nautopost(och, "self");
    
    nuke_pets(och);
    och->pet = NULL;
    
    if (ch) {
      pbuff = buffer;
      colourconv( pbuff, buf, ch );  
      write_to_descriptor (d->descriptor, buffer, 0);
      write_to_descriptor (d->descriptor, "Saving...\r\n", 0);
      write_to_descriptor (d->descriptor, "Warmboot starting...\r\n", 0); 
    }
    else {
      write_to_descriptor (d->descriptor, buf, 0);
    }
    }
  }
  
 
  /* This rendition is intended to reboot and read in files that were manually
     copied over, so no world save is needed 
  */
  //do_asave(NULL, "");
  
  fprintf (fp, "-1\n");
  fclose (fp);
  
  /* Close reserve and other always-open files and release other resources */
  
  fclose (fpReserve);
  
  /* exec - descriptors are inherited */
  
  sprintf (buf, "%d", port);
  sprintf (buf2, "%d", control);
  execl (EXE_FILE, "tsw", buf, "copyover", buf2, (char *) NULL);
  
  /* Failed - sucessful exec will not return */
  
  perror ("do_warmboot: execl");
  send_to_char ("Warmboot terminated...\r\n",ch);
  
  /* Here you might want to reopen fpReserve */
  fpReserve = fopen (NULL_FILE, "r");
}

 void do_warmboot_OLD (CHAR_DATA *ch, char * argument)
 {
   FILE *fp;
   DESCRIPTOR_DATA *d, *d_next;
   char buf [100], buf2[100];
   extern int port,control; /* db.c */
   char *pbuff;
   char buffer[ MAX_STRING_LENGTH*2 ];
   
        signal(SIGSEGV,SIG_IGN);
        fflush(NULL);

   fp = fopen (COPYOVER_FILE, "w");
   
   if (!fp)
   {
             if (ch)
        {   send_to_char ("Copyover file not writeable, aborted.\r\n",ch);
             }
      _logf ("Could not write to copyover file: %s", COPYOVER_FILE);
      perror ("do_copyover:fopen");
      return;
   }
   
   /* Consider changing all saved areas here, if you use OLC */
   
   do_asave (NULL, "");  /*- autosave changed areas */

   if (ch->invis_level >= LEVEL_HERO)
     sprintf (buf, "\r\nWarmboot by someone...\r\n");
        else if (ch)
     sprintf (buf, "\r\nWarmboot by %s...\r\n", COLORNAME(ch));
        else
     sprintf (buf, "\r\nCrash recovery starting...\r\n");    
   
   /* For each playing descriptor, save its state */
   for (d = descriptor_list; d ; d = d_next)
   {
      CHAR_DATA * och = CH (d);
      d_next = d->next; /* We delete from the list , so need to save this */
      
      if (!d->character || d->connected > CON_PLAYING) /* drop those logging on */
      {
         write_to_descriptor (d->descriptor, "\r\nSorry, we are rebooting. Come back in a few minutes.\r\n", 0);
         close_socket (d); /* throw'em out */
      }
      else
      {
         fprintf (fp, "%d %s %s\n", d->descriptor, och->name, d->host);
 
 #if 0          /* This is not necessary for ROM */
         if (och->level == 1)
         {
            write_to_descriptor (d->descriptor, "Since you are level one, and level one characters do not save, you gain a free level!\r\n", 0);
            advance_level (och);
            och->level++; /* Advance_level doesn't do that */
         }
 #endif        
	 if (IS_CHANNELING(och))
         {
	 	do_unchannel(och,"");
	 }
         save_char_obj (och, FALSE);

         pbuff = buffer;
         colourconv( pbuff, buf, ch );
         
         write_to_descriptor (d->descriptor, buffer, 0);
      }
   }
   
   fprintf (fp, "-1\n");
   fclose (fp);
   
   /* Close reserve and other always-open files and release other resources */
   
   fclose (fpReserve);
   
   /* exec - descriptors are inherited */
   
   sprintf (buf, "%d", port);
   sprintf (buf2, "%d", control);
   execl (EXE_FILE, "tsw", buf, "warmboot", buf2, (char *) NULL);
 
   /* Failed - sucessful exec will not return */
   
   perror ("do_warmboot: execl");
   send_to_char ("Warmboot FAILED!\r\n",ch);
   
   /* Here you might want to reopen fpReserve */
   fpReserve = fopen (NULL_FILE, "r");
 }
 
 /* Recover from a copyover - load players */
 void copyover_recover ()
 {
   DESCRIPTOR_DATA *d;
   FILE *fp;
   char name [100];
   char host[MSL];
   int desc;
   bool fOld;
   
   _logf ("Copyover recovery initiated");
   
   fp = fopen (COPYOVER_FILE, "r");
   
   if (!fp) /* there are some descriptors open which will hang forever then ? */
   {
      perror ("copyover_recover:fopen");
      _logf ("Copyover file not found. Exitting.\r\n");
      exit (1);
   }
 
   unlink (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading   */
   
   for (;;)
   {
      fscanf (fp, "%d %s %s\n", &desc, name, host);
      if (desc == -1)
         break;
 
      /* Write something, and check if it goes error-free */      
      if (!write_to_descriptor (desc, "\r\nRestoring from recovery...\r\n",0))
      {
         close (desc); /* nope */
         continue;
      }
      
      d = new_descriptor();
      d->descriptor = desc;
      
      d->host = str_dup (host);
      d->next = descriptor_list;
      descriptor_list = d;
      d->connected = CON_COPYOVER_RECOVER; /* -15, so close_socket frees the char */
      
   
      /* Now, find the pfile */      
      fOld = load_char_obj (d, name, FALSE);

	 /* If not found in normal player dir, try disguise */
	 if (!fOld)
	   fOld = load_char_obj (d, name, TRUE);
      
      if (!fOld) /* Player file not found?! */
      {
         write_to_descriptor (desc, "\r\nSomehow, your character was lost in the warmboot. Sorry.\r\n", 0);
         close_socket (d);       
      }
      else /* ok! */
      {
         write_to_descriptor (desc, "\r\nRecovery complete.\r\n",0);
   
         /* Just In Case */
         if (!d->character->in_room)
            d->character->in_room = get_room_index (ROOM_VNUM_TEMPLE);
 
         /* Insert in the char_list */
         d->character->next = char_list;
         char_list = d->character;
 
         char_to_room (d->character, d->character->in_room);
         do_look (d->character, "auto");

         if (!IS_IMMORTAL(d->character))
           act ("$n materializes!", d->character, NULL, NULL, TO_ROOM);
         
         d->connected = CON_PLAYING;
 
         if (d->character->pet != NULL)
         {
             char_to_room(d->character->pet,d->character->in_room);
             act("$n materializes!.",d->character->pet,NULL,NULL,TO_ROOM);
         }
	    else if (d->character->mount != NULL) {
		 char_to_room(d->character->mount,d->character->in_room);
		 act("$n materializes!.",d->character->mount,NULL,NULL,TO_ROOM);
		 add_follower( d->character->mount, d->character);
		 do_mount(d->character, d->character->mount->name);
		 d->character->mount->leader = d->character;
	    }
      }
      
   }
    fclose (fp);
   
   
 }
 
 
void do_addlag(CHAR_DATA *ch, char *argument)
{

   CHAR_DATA *victim;
   char arg1[MAX_STRING_LENGTH];
   int x;

   argument = one_argument(argument, arg1);

   if (arg1[0] == '\0')
   {
      send_to_char("addlag to who?", ch);
      return;
   }

   if ((victim = get_char_anywhere(ch, arg1)) == NULL)
   {
      send_to_char("They're not here.", ch);
      return;
   }

   if ((x = atoi(argument)) <= 0)
   {
      send_to_char("syntax: addlag <player> <lag amount>.", ch);
      return;
   }

   if (x > 100)
   {
      send_to_char("There's a limit to cruel and unusual punishment", ch);
      return;
   }

   send_to_char("Somebody REALLY didn't like you", victim);
   WAIT_STATE(victim, x);
   send_to_char("Adding lag now...", ch);
   return;
}


/* Super-AT command:

FOR ALL <action>
FOR MORTALS <action>
FOR GODS <action>
FOR MOBS <action>
FOR EVERYWHERE <action>


Executes action several times, either on ALL players (not including yourself),
MORTALS (including trusted characters), GODS (characters with level higher than
L_HERO), MOBS (Not recommended) or every room (not recommended either!)

If you insert a # in the action, it will be replaced by the name of the target.

If # is a part of the action, the action will be executed for every target
in game. If there is no #, the action will be executed for every room containg
at least one target, but only once per room. # cannot be used with FOR EVERY-
WHERE. # can be anywhere in the action.

Example: 

FOR ALL SMILE -> you will only smile once in a room with 2 players.
FOR ALL TWIDDLE # -> In a room with A and B, you will twiddle A then B.

Destroying the characters this command acts upon MAY cause it to fail. Try to
avoid something like FOR MOBS PURGE (although it actually works at my MUD).

FOR MOBS TRANS 3054 (transfer ALL the mobs to Midgaard temple) does NOT work
though :)

The command works by transporting the character to each of the rooms with 
target in them. Private rooms are not violated.

*/

/* Expand the name of a character into a string that identifies THAT
   character within a room. E.g. the second 'guard' -> 2. guard
*/   
const char * name_expand (CHAR_DATA *ch)
{
   int count = 1;
   CHAR_DATA *rch;
   char name[MAX_INPUT_LENGTH]; /*  HOPEFULLY no mob has a name longer than THAT */

   static char outbuf[MAX_INPUT_LENGTH];  
   
   if (!IS_NPC(ch))
      return ch->name;
      
   one_argument (ch->name, name); /* copy the first word into name */
   
   if (!name[0]) /* weird mob .. no keywords */
   {
      strcpy (outbuf, ""); /* Do not return NULL, just an empty buffer */
      return outbuf;
   }
      
   for (rch = ch->in_room->people; rch && (rch != ch);rch = rch->next_in_room)
      if (is_name (name, rch->name))
         count++;
         

   sprintf (outbuf, "%d.%s", count, name);
   return outbuf;
}


void do_for (CHAR_DATA *ch, char *argument)
{
   char range[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   bool fGods = FALSE, fMortals = FALSE, fMobs = FALSE, fEverywhere = FALSE, found;
   ROOM_INDEX_DATA *room, *old_room;
   CHAR_DATA *p, *p_next;
   int i;
   
   argument = one_argument (argument, range);
   
   if (!range[0] || !argument[0]) /* invalid usage? */
   {
      do_help (ch, "for");
      return;
   }
   
   if (!str_prefix("quit", argument))
   {
      send_to_char ("Are you trying to crash the MUD or something?\r\n",ch);
      return;
   }
   
   
   if (!str_cmp (range, "all"))
   {
      fMortals = TRUE;
      fGods = TRUE;
   }
   else if (!str_cmp (range, "gods"))
      fGods = TRUE;
   else if (!str_cmp (range, "mortals"))
      fMortals = TRUE;
   else if (!str_cmp (range, "mobs"))
      fMobs = TRUE;
   else if (!str_cmp (range, "everywhere"))
      fEverywhere = TRUE;
   else
      do_help (ch, "for"); /* show syntax */

   /* do not allow # to make it easier */    
   if (fEverywhere && strchr (argument, '#'))
   {
      send_to_char ("Cannot use FOR EVERYWHERE with the # thingie.\r\n",ch);
      return;
   }
      
   if (strchr (argument, '#')) /* replace # ? */
   { 
      for (p = char_list; p ; p = p_next)
      {
         p_next = p->next; /* In case someone DOES try to AT MOBS SLAY # */
         found = FALSE;
         
         if (!(p->in_room) || room_is_private(p->in_room) || (p == ch))
            continue;
         
         if (IS_NPC(p) && fMobs)
            found = TRUE;
         else if (!IS_NPC(p) && p->level >= LEVEL_IMMORTAL && fGods)
            found = TRUE;
         else if (!IS_NPC(p) && p->level < LEVEL_IMMORTAL && fMortals)
            found = TRUE;

         /* It looks ugly to me.. but it works :) */           
         if (found) /* p is 'appropriate' */
         {
            char *pSource = argument; /* head of buffer to be parsed */
            char *pDest = buf; /* parse into this */
            
            while (*pSource)
            {
               if (*pSource == '#') /* Replace # with name of target */
               {
                  const char *namebuf = name_expand (p);
                  
                  if (namebuf) /* in case there is no mob name ?? */
                     while (*namebuf) /* copy name over */
                        *(pDest++) = *(namebuf++);

                  pSource++;
               }
               else
                  *(pDest++) = *(pSource++);
            } /* while */
            *pDest = '\0'; /* Terminate */
            
            /* Execute */
            old_room = ch->in_room;
            char_from_room (ch);
            char_to_room (ch,p->in_room);
            interpret (ch, buf);
            char_from_room (ch);
            char_to_room (ch,old_room);
            
         } /* if found */
      } /* for every char */
   }
   else /* just for every room with the appropriate people in it */
   {
      for (i = 0; i < MAX_KEY_HASH; i++) /* run through all the buckets */
         for (room = room_index_hash[i] ; room ; room = room->next)
         {
            found = FALSE;
            
            /* Anyone in here at all? */
            if (fEverywhere) /* Everywhere executes always */
               found = TRUE;
            else if (!room->people) /* Skip it if room is empty */
               continue;
               
               
            /* Check if there is anyone here of the requried type */
            /* Stop as soon as a match is found or there are no more ppl in room */
            for (p = room->people; p && !found; p = p->next_in_room)
            {

               if (p == ch) /* do not execute on oneself */
                  continue;
                  
               if (IS_NPC(p) && fMobs)
                  found = TRUE;
               else if (!IS_NPC(p) && (p->level >= LEVEL_IMMORTAL) && fGods)
                  found = TRUE;
               else if (!IS_NPC(p) && (p->level <= LEVEL_IMMORTAL) && fMortals)
                  found = TRUE;
            } /* for everyone inside the room */
                  
            if (found && !room_is_private(room)) /* Any of the required type here AND room not private? */
            {
               /* This may be ineffective. Consider moving character out of old_room
                  once at beginning of command then moving back at the end.
                  This however, is more safe?
               */
            
               old_room = ch->in_room;
               char_from_room (ch);
               char_to_room (ch, room);
               interpret (ch, argument);
               char_from_room (ch);
               char_to_room (ch, old_room);
            } /* if found */
         } /* for every room in a bucket */
   } /* if strchr */
} /* do_for */

/* get the 'short' name of an area (e.g. MIDGAARD, MIRROR etc. */
/* assumes that the filename saved in the AREA_DATA struct is something like midgaard.are */
char * area_name (AREA_DATA *pArea)
{
   static char buffer[64]; /* short filename */
   char  *period;

   assert (pArea != NULL);
   
   strncpy (buffer, pArea->file_name, 64); /* copy the filename */   
   period = strchr (buffer, '.'); /* find the period (midgaard.are) */
   if (period) /* if there was one */
      *period = '\0'; /* terminate the string there (midgaard) */
      
   return buffer; 
}

/* show a list of all used VNUMS */

#define MAX_SHOW_VNUM   330 /* show only 1 - 100*100 */
#define COLUMNS      5   /* number of columns */
#define MAX_ROW      ((MAX_SHOW_VNUM / COLUMNS)+1) /* rows */
#define HIGH_VNUM       330000

void do_vlist (CHAR_DATA *ch, char *argument)
{
   int i=0;
   int j=0;
   int vnum=0;
   ROOM_INDEX_DATA *room;
   MOB_INDEX_DATA *mob;
   OBJ_INDEX_DATA *obj;
   char buffer[MAX_ROW*10000]; /*should be plenty */
   char buf2 [MAX_ROW*10000];
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char arg3[MAX_INPUT_LENGTH];
   int from=0;
   int to=0;
   int counter=0;
   int diff=0;
   int flag=0;
   AREA_DATA *pArea;
   
   argument = one_argument(argument,arg);
   argument = one_argument(argument,arg2);
   argument = one_argument(argument,arg3);

   if (arg[0] == '\0') { 
     send_to_char("Syntax: vlist <all/obj/mob/room> <from> <to>\r\n",ch);
     send_to_char("        if no range is spesified, default is used\r\n",ch);
     return;
   }
   
   /* if no range, use default */
   if (arg2[0] == '\0')
     flag=1;
   
   if (!flag) {
     /* Assign from vnum and to vnum values */
     from = atoi(arg2);
     if (from == 0) { 
       to   = (atoi(arg3)/COLUMNS);
       if ((to - from) < 1)
         to = 1;
     }
     else {
       to   = (atoi(arg3));
       diff = (to - from)/COLUMNS;
       to = diff + from;
       if ((to - from) < 1)
         to = from + 1;
     }
   }
   else {
     from = 0;
     to = 10;
   }

   // Check access
   pArea = get_vnum_area(atoi(arg2));
   
   if (!pArea)
     return;
   
   if (!IS_BUILDER( ch, pArea )) {
      send_to_char( "You can only list vnums for the areas you have access to.\r\n", ch );
      return;
    }

   if ((atoi(arg3) - atoi(arg2)) > 500) {
     send_to_char("Please keep range within a delta of 500\r\n", ch);
     return;
   }

   if (!str_cmp(arg,"all")) {
     for (i = from; i < to; i++) {
       strcpy (buffer, ""); /* clear the buffer for this row */
       for (j = 0; j < COLUMNS; j++) /* for each column */ {
         if (from == 0)
      vnum = ((j*to) + i); /* find a vnum whih should be there */
         else
      vnum = from + counter;
         if (vnum < HIGH_VNUM) {
        obj = get_obj_index (vnum);
        mob = get_mob_index (vnum);
        room = get_room_index (vnum);
      sprintf (buf2, "%3d %s{g-{x%s{g-{x%s  ", vnum, 
          obj ? "{wOB{x" : "{G--{x",
          mob ? "{rMO{x" : "{G--{x",
          room ? "{yRO{x" : "{G--{x");
      /* something there or unused ? */
      strcat (buffer,buf2);
      counter ++;
         } 
       } /* for columns */
       send_to_char (buffer,ch);
       send_to_char ("\r\n",ch);
     } /* for rows */
     strcpy (buffer, "");
     sprintf (buffer, "\r\n%14s%s\r\n"," ",
         "{wOB{x = Object    {rMO{x = Mobile    {yRO{x = Room");
     send_to_char (buffer, ch);
     /* reset counter */
     counter = 0;
   }
   
   else if (!str_cmp(arg,"obj")) {
     for (i = from; i < to; i++) {
       strcpy (buffer, ""); /* clear the buffer for this row */
       for (j = 0; j < COLUMNS; j++) /* for each column */ {
         if (from==0)
      vnum = ((j*to) + i); /* find a vnum whih should be there */
         else
      vnum = from + counter;
         if (vnum < HIGH_VNUM) {
      obj = get_obj_index (vnum);
      sprintf (buf2, "%3d %-8.8s  ", vnum, 
          obj ? obj->name : "--------" ); 
      /* something there or unused ? */
      strcat (buffer,buf2);
      counter ++;
         } 
       } /* for columns */
       send_to_char (buffer,ch);
       send_to_char ("\r\n",ch);
     } /* for rows */
     /* reset counter */
     counter = 0;
   }
   
   else if (!str_cmp(arg,"mob")) {
     for (i = from; i < to; i++) {
       strcpy (buffer, ""); /* clear the buffer for this row */
       for (j = 0; j < COLUMNS; j++) /* for each column */ {
         if (from ==0)
      vnum = ((j*to) + i); /* find a vnum whih should be there */
         else
      vnum = from + counter;
         if (vnum < HIGH_VNUM) {
      mob = get_mob_index (vnum);
      sprintf (buf2, "%3d %-8.8s  ", vnum, 
          mob ? mob->player_name : "--------" ); 
      /* something there or unused ? */
      strcat (buffer,buf2);
      counter ++;
         } 
       } /* for columns */
       send_to_char (buffer,ch);
       send_to_char ("\r\n",ch);
     } /* for rows */
     /* reset counter */
     counter=0;     
   }

   else if (!str_cmp(arg,"room")) {
     for (i = from; i < to; i++) {
       strcpy (buffer, ""); /* clear the buffer for this row */
       for (j = 0; j < COLUMNS; j++) /* for each column */ {
         if (from == 0)
      vnum = ((j*to) + i); /* find a vnum whih should be there */
         else
      vnum = from + counter;
         if (vnum < HIGH_VNUM) {
      room = get_room_index (vnum);
      sprintf (buf2, "%3d %-8.8s  ", vnum, 
          room ? area_name(room->area) : "--------" ); 
      /* something there or unused ? */
      strcat (buffer,buf2);   
      counter ++;
         } 
       } /* for columns */
       send_to_char (buffer,ch);
       send_to_char ("\r\n",ch);
     } /* for rows */
     /* reset counter */
     counter=0;
   }

   else {
     /* No option hit, write the dang syntax once more */
     send_to_char("Syntax: vnum <all/obj/mob/room> <from> <to>\r\n",ch);
     return;
   }
}

/* show a list of all used AreaVNUMS */
/* By The Mage */
void do_fvlist (CHAR_DATA *ch, char *argument)
{
  int i,j;
//  char buffer[MAX_ROW*100]; /* should be plenty */
//  char buf2 [100];
  char arg[MAX_INPUT_LENGTH];
  char *string;

  string = one_argument(argument,arg);
 
  if (arg[0] == '\0')
    {
      send_to_char("Syntax:\n\r",ch);
      send_to_char("  fvlist obj\n\r",ch);
      send_to_char("  fvlist mob\n\r",ch);
      send_to_char("  fvlist room\n\r",ch);
      return;
    }
  j=1;
  if (!str_cmp(arg,"obj"))
    {
      printf_to_char(ch,"{WFree {C%s{W vnum listing for area {C%s{x\n\r",arg,
		     ch->in_room->area->name);
      printf_to_char(ch,"{Y=============================================================================={C\n\r");
      for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) {
	if (get_obj_index(i) == NULL) {
	  printf_to_char(ch,"%8d, ",i);
	  if (j == COLUMNS) {
	    send_to_char("\n\r",ch);
	    j=0;
	  }
	  j++;
	}
      }
      send_to_char("{x\n\r",ch);
      return;
    }

  if (!str_cmp(arg,"mob"))
    { 
      printf_to_char(ch,"{WFree {C%s {Wvnum listing for area {C%s{x\n\r",arg,
		     ch->in_room->area->name);
      printf_to_char(ch,"{Y=============================================================================={C\n\r");
      for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) {
	if (get_mob_index(i) == NULL) {
	  printf_to_char(ch,"%8d, ",i);
	  if (j == COLUMNS) {
	    send_to_char("\n\r",ch);
	    j=0;
	  } 
	  else j++;
	}
      }
      send_to_char("{x\n\r",ch);
      return;
    }
  if (!str_cmp(arg,"room"))
    { 
      printf_to_char(ch,"{WFree {C%s {Wvnum listing for area {C%s{x\n\r",arg,
		     ch->in_room->area->name);
      printf_to_char(ch,"{Y=============================================================================={C\n\r");
      for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) {
	if (get_room_index(i) == NULL) {
	  printf_to_char(ch,"%8d, ",i);
	  if (j == COLUMNS) {
	    send_to_char("\n\r",ch);
	    j=0;
	  }
	  else j++;
	}
      }
      send_to_char("{x\n\r",ch);
      return;
    }
  send_to_char("WHAT??? \n\r",ch);
  send_to_char("Syntax:\n\r",ch);
  send_to_char("  fvlist obj\n\r",ch);
  send_to_char("  fvlist mob\n\r",ch);
  send_to_char("  fvlist room\n\r",ch);
}


void do_remlink (CHAR_DATA *ch, char * argument)
{
  CHAR_DATA *wch;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  int match=FALSE;
  
  argument = one_argument(argument, arg);

  if (arg[0] == '\0') {
    send_to_char("Syntax: remlink <playername> \r\n", ch);
    send_to_char("Remlink remove a player that is link-dead from the character list\r\n", ch);
    return;
  }

  for ( wch = char_list; wch != NULL; wch = wch->next ) {

    /* Only want to process PC's */
    if (IS_NPC(wch))
      continue;
    
    if (!str_cmp(arg, wch->name) && (wch->desc == NULL)) {
      match=TRUE;
      sprintf(buf, "%s removed from character list.\r\n", wch->name);
      send_to_char(buf,ch);
      do_function(wch, &do_quit, "" );
      return;
    }
    
  }

  if (match == FALSE) {
    sprintf(buf, "%s is not link-dead or found in character list.\r\n", arg);
    send_to_char(buf, ch );
  }
}

void do_skillstat(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO + 1];
    int sn, level, min_lev = 1, max_lev = LEVEL_HERO;
    bool found = FALSE;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    if (IS_NPC(ch))
    return;

    if ( argument[0] == '\0' ) {
       send_to_char( "List skills for whom?\r\n", ch );
       return;
    }

    if ( ( victim = get_char_anywhere( ch, argument ) ) == NULL ) {
       send_to_char( "They aren't here.\r\n", ch );
       return;
    }

    if (IS_NPC(victim)) {
       send_to_char( "Use this for skills on players.\r\n", ch );
       return;
    }

    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
        skill_columns[level] = 0;
        skill_list[level][0] = '\0';
    }

    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL )
        break;

        if ((level = skill_table[sn].skill_level[victim->class]) < LEVEL_HERO + 1
        &&  level >= min_lev && level <= max_lev
        &&  (skill_table[sn].spell_fun == spell_null)
        &&  victim->pcdata->learned[sn] > 0)
        {
            found = TRUE;
            level = skill_table[sn].skill_level[victim->class];
            if (victim->level < level)
                sprintf(buf,"%-18s   n/a      ", skill_table[sn].name);
            else
                sprintf(buf,"%-18s  %3d (%s)  ",skill_table[sn].name,
                victim->pcdata->learned[sn],
                skill_table[sn].restriction == RES_NORMAL  ? "{gt{x" :
                skill_table[sn].restriction == RES_GRANTED ? "{Wg{x" :
                skill_table[sn].restriction == RES_MALE    ? "{Bm{x" :
                skill_table[sn].restriction == RES_FEMALE  ? "{rf{x" : "{rn{x");

            if (skill_list[level][0] == '\0')
                sprintf(skill_list[level],"\r\nLevel %2d: %s",level,buf);
            else /* append */
            {
                if ( ++skill_columns[level] % 2 == 0)
                    strcat(skill_list[level],"\r\n          ");
                    strcat(skill_list[level],buf);
            }
        }
    }

    /* return results */

    if (!found)
    {
        send_to_char("No skills found.\r\n",ch);
        return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
        if (skill_list[level][0] != '\0')
            add_buf(buffer,skill_list[level]);
            add_buf(buffer,"\r\n");
            page_to_char(buf_string(buffer),ch);
            free_buf(buffer);
}

void do_spellstat(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    char buff[100];
    char spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char spell_columns[LEVEL_HERO + 1];
    int sn, gn, col, level, min_lev = 1, max_lev = LEVEL_HERO, endurance;
    bool found = FALSE;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;


    if (IS_NPC(ch))
    return;

    if (argument[0] == '\0')
    {
        send_to_char( "List spells for whom?\r\n", ch );
        return;
    }

    if ((victim = get_char_anywhere( ch, argument )) == NULL)
    {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char( "Use this for skills on players.\r\n", ch );
        return;
    }

    /* groups */

    col = 0;

    for (gn = 0; gn < MAX_GROUP; gn++)
    {
        if (group_table[gn].name == NULL)
        break;
        if (victim->pcdata->group_known[gn])
        {
            sprintf(buff,"%-20s ",group_table[gn].name);
            send_to_char(buff,ch);
            if (++col % 3 == 0)
                send_to_char("\r\n",ch);
        }
    }
    if ( col % 3 != 0 )
    {
        send_to_char( "\r\n", ch );
        sprintf(buff,"Creation points: %d\r\n",victim->pcdata->points);
        send_to_char(buff,ch);
    }
    
    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
        spell_columns[level] = 0;
        spell_list[level][0] = '\0';
    }

    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL )
        break;

        if ((level = skill_table[sn].skill_level[victim->class]) < LEVEL_HERO
+ 1
        &&  level >= min_lev && level <= max_lev
        &&  skill_table[sn].spell_fun != spell_null
        &&  victim->pcdata->learned[sn] > 0)
        {
            found = TRUE;
            level = skill_table[sn].skill_level[victim->class];

            if (victim->level < level)
                sprintf(buf,"%-18s   n/a      ", skill_table[sn].name);
            else
            {
                endurance = UMAX(skill_table[sn].min_endurance,
                100/(2 + victim->level - level));
                sprintf(buf,"%-18s  %3d (%s)  ",skill_table[sn].name,
                        victim->pcdata->learned[sn],
                        skill_table[sn].restriction == RES_NORMAL  ? "{gt{x" :
                        skill_table[sn].restriction == RES_GRANTED ? "{Wg{x" :
                        skill_table[sn].restriction == RES_MALE    ? "{Bm{x" :
                        skill_table[sn].restriction == RES_FEMALE  ? "{rf{x" : "{rn{x");
            }

            if (spell_list[level][0] == '\0')
                sprintf(spell_list[level],"\r\nLevel %2d: %s",level,buf);
            else /* append */
            {
                if ( ++spell_columns[level] % 2 == 0)
                strcat(spell_list[level],"\r\n          ");
                strcat(spell_list[level],buf);
            }
        }
    }

    /* return results */

    if (!found)
    {
        send_to_char("No spells found.\r\n",ch);
        return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
        if (spell_list[level][0] != '\0')
            add_buf(buffer,spell_list[level]);
            add_buf(buffer,"\r\n");
            page_to_char(buf_string(buffer),ch);
            free_buf(buffer);
}

/*
 * do_rename renames a player to another name.
 * PCs only. Previous file is deleted, if it exists.
 * Char is then saved to new file.
 * New name is checked against std. checks, existing offline players and
 * online players. 
 * .gz files are checked for too, just in case.
 */

bool check_parse_name (char* name);  /* comm.c */

void do_rename (CHAR_DATA* ch, char* argument)
{
  char old_name[MAX_INPUT_LENGTH];
  char new_name[MAX_INPUT_LENGTH];
  char strsave [MAX_INPUT_LENGTH];
  
  CHAR_DATA* victim;
  FILE* file;
  
  argument = one_argument(argument, old_name); /* find new/old name */
  one_argument (argument, new_name);
  
  /* Trivial checks */
  if (!old_name[0]) {
    send_to_char ("Rename who?\r\n",ch);
    return;
  }
   
  victim = get_char_anywhere (ch, old_name);
  
  if (!victim) {
    send_to_char ("There is no such a person online.\r\n",ch);
    return;
  }
  
  if (IS_NPC(victim)) {   
    send_to_char ("You cannot use Rename on NPCs.\r\n",ch);
    return;
  }
  
  /* allow rename self new_name,but otherwise only lower level */ 
  if ( (victim != ch) && (get_trust (victim) >= get_trust (ch)) ) {
    send_to_char ("You failed.\r\n",ch);
    return;
  }
  
  if (!victim->desc || (victim->desc->connected != CON_PLAYING) ) {
    send_to_char ("This player has lost his link or is inside a pager or the like.\r\n",ch);
    return;
  }
  
  if (!new_name[0]) {
    send_to_char ("Rename to what new name?\r\n",ch);
    return;
  }
  
  /* Insert check for clan here!! */
  /*
    
    if (victim->clan) {
    send_to_char ("This player is member of a clan, remove him from there first.\r\n",ch);
    return;
    }
  */
   
  if (!check_parse_name(new_name)) {
    send_to_char ("The new name is illegal.\r\n",ch);
    return;
  }
  
  /* First, check if there is a player named that off-line */  
  sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( new_name ) );
  
  fclose (fpReserve); /* close the reserve file */
  file = fopen (strsave, "r"); /* attempt to to open pfile */
  if (file) {
    send_to_char ("A player with that name already exists!\r\n",ch);
    fclose (file);
    fpReserve = fopen( NULL_FILE, "r" ); /* is this really necessary these days? */
    return;    
  }
  fpReserve = fopen( NULL_FILE, "r" );  /* reopen the extra file */
  
  /* Check .gz file ! */
  sprintf( strsave, "%s%s.gz", PLAYER_DIR, capitalize( new_name ) );
  
  fclose (fpReserve); /* close the reserve file */
  file = fopen (strsave, "r"); /* attempt to to open pfile */
  if (file) {
    send_to_char ("A player with that name already exists in a compressed file!\r\n",ch);
    fclose (file);
    fpReserve = fopen( NULL_FILE, "r" ); 
    return;    
  }
  fpReserve = fopen( NULL_FILE, "r" );  /* reopen the extra file */
  
  /* check for playing level-1 non-saved */
  if (get_char_anywhere(ch,new_name)) { 
    send_to_char ("A player with the name you specified already exists!\r\n",ch);
    return;
  }
  
  /* Save the filename of the old name */
  sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( victim->name ) );
  
  /* Rename the character and save him to a new file */
  /* NOTE: Players who are level 1 do NOT get saved under a new name */
  
  free_string (victim->name);
  victim->name = str_dup (capitalize(new_name));
  
  save_char_obj (victim, FALSE);
  
  /* unlink the old file */
  unlink (strsave); /* unlink does return a value.. but we do not care */
  
  /* That's it! */
   
  send_to_char ("Character renamed.\r\n",ch);
  
  victim->position = POS_STANDING; /* I am laaazy */
  act ("$n has renamed you to $N!",ch,NULL,victim,TO_VICT);
  
} /* do_rename */

void do_olevel(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_INPUT_LENGTH];
  char level[MAX_INPUT_LENGTH];
  char name[MAX_INPUT_LENGTH];
  BUFFER *buffer;
  OBJ_DATA *obj;
  OBJ_DATA *in_obj;
  bool found;
  int number = 0, max_found;
  
  found = FALSE;
  number = 0;
  max_found = 500;
  
  buffer = new_buf();
  
  argument = one_argument(argument, level);
  if (level[0] == '\0') {
    send_to_char("Syntax: olevel <level>\r\n",ch);
    send_to_char("        olevel <level> <name>\r\n",ch);
    return;
  }
 
  argument = one_argument(argument, name);
  for ( obj = object_list; obj != NULL; obj = obj->next ) {
    if (obj->level == 0)
    continue;
    if ( obj->level != atoi(level) )
    continue;

    if ( name[0] != '\0' && !is_name(name, obj->name) )
    continue;

    found = TRUE;
    number++;
    
    for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj );

    if ((in_obj->carried_by != NULL) && (can_see(ch,in_obj->carried_by))
      && (in_obj->carried_by->in_room != NULL))
    sprintf( buf, "[%3d ] %s is carried by %s [Room %d]\r\n",
         number, obj->short_descr,PERS(in_obj->carried_by, ch),
         in_obj->carried_by->in_room->vnum );
    else if ((in_obj->in_room != NULL) && (can_see_room(ch,in_obj->in_room)))
    sprintf( buf, "[%3d ] %s is in %s [Room %d]\r\n",
         number, obj->short_descr,in_obj->in_room->name, 
         in_obj->in_room->vnum);
    else
    sprintf( buf, "[%3d ] %s is somewhere\r\n",number, obj->short_descr); 
    
    buf[0] = UPPER(buf[0]);
    add_buf(buffer,buf);
    
    if (number >= max_found)
    break;
  }
  
  if ( !found )
    send_to_char( "Nothing like that in the pattern.\r\n", ch );
  else
    page_to_char(buf_string(buffer),ch);
  
  free_buf(buffer);
}

void do_mlevel( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_INPUT_LENGTH];
  BUFFER *buffer;
  CHAR_DATA *victim;
  bool found;
  int count = 0;
  int fillers = 0;
  
  if ( argument[0] == '\0' ) {
    send_to_char("Syntax: mlevel <level>\r\n",ch);
    return;
  }
  
  found = FALSE;
  buffer = new_buf();
  for ( victim = char_list; victim != NULL; victim = victim->next ) {
    if ( victim->in_room != NULL &&   atoi(argument) == victim->level ) {
    found = TRUE;
    count++;
    if (IS_NPC(victim)) {
      fillers = (28 - colorstrlen(victim->short_descr));
      if (fillers <= 0)
      fillers = 0;
    }
    else {
      fillers = (28 - colorstrlen(victim->name));
      if (fillers <= 0)
      fillers = 0;
    }
    sprintf( buf, "[%3d ] [%5d] %s%*s [%5d] %s\r\n", count,
         IS_NPC(victim) ? victim->pIndexData->vnum : 0,
         IS_NPC(victim) ? victim->short_descr : victim->name,
         fillers, "",
         victim->in_room->vnum,
         victim->in_room->name );
    add_buf(buffer,buf);
    }
  }
  
    if ( !found )
    act( "You didn't find any mob of level $T.", ch, NULL, argument, TO_CHAR );
    else
    page_to_char(buf_string(buffer),ch);
    
    free_buf(buffer);
    
    return;
}

void do_validate( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  
  one_argument( argument, arg );

  if (!IS_IMMORTAL(ch) && !IS_NEWBIEHELPER(ch)) {
    send_to_char("Huh?\r\n", ch);
    return;
  }

  if ( arg[0] == '\0' ) {
    send_to_char( "Validate whom?\r\n", ch );
    return;
  }
  
  for ( d = descriptor_list; d != NULL; d = d->next ) {
    CHAR_DATA *victim;
    victim = d->original ? d->original : d->character;    
    
    if ( (victim!=NULL) && !str_cmp(argument,victim->name)) {
	 if (!IS_SET(victim->act, PLR_UNVALIDATED)) {
	   sprintf(buf, "%s is already validated.\r\n", victim->name);
	   send_to_char(buf, ch );
	   return;
	 }
	 REMOVE_BIT(victim->comm,COMM_NOEMOTE);
	 REMOVE_BIT(victim->comm,COMM_NOSHOUT);    
	 REMOVE_BIT(victim->comm,COMM_NOCHANNELS);    
	 REMOVE_BIT(victim->comm,COMM_NOTELL);    
	 /*  REMOVE_BIT(victim->comm,COMM_NOMAIL);*/
	 REMOVE_BIT(victim->act,PLR_UNVALIDATED);
	 
	 if (IS_NEWBIEHELPER(ch)) {
	    send_to_char("You have been validated by one of the Newbie Helpers, all channels are activated.\r\n", victim);

	    sprintf(buf, "Newbie Helper %s has validated %s.", ch->name, victim->name);
            wiznet(buf, ch, NULL, WIZ_NEWBIE, 0, get_trust(ch));
            sprintf(buf, "Newbie Helper %s has validated %s.", ch->name, victim->name);
            log_string(buf);
	 }
	 else {
	    send_to_char("You have been validated by the immortals, all channels are activated.\r\n", victim);
	 }
	 
	 send_to_char("You can now use the '{Wgraduate{x' command to graduate from newbie school.\r\n", victim);
	 act( "You have validated $N.", ch, NULL, victim, TO_CHAR );
	 if (victim->sex == 1)
	   set_title(victim,"probably wants to change his title.");
	 else
	   set_title(victim,"probably wants to change her title.");
	 return;
    }
  }
  
  /* bug( "do_validate: desc not found.", 0 ); */
  send_to_char( "Descriptor not found!\r\n", ch );
  return;
}

void do_grant(CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char arg4[MAX_INPUT_LENGTH];
  char g2v_buf[MAX_STRING_LENGTH]; /* Grant 2 Victim buffer */
  char g2c_buf[MAX_STRING_LENGTH]; /* Grant 2 Char   buffer */
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  int value;
  int isGrant=FALSE;
  int sn=0;

  /* merit / flaw / talent */
  long pos = NO_FLAG;
  int i=0;
  int num=0;

  smash_tilde( argument );
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  argument = one_argument(argument, arg3);
  one_argument(argument, arg4);
  
  if (IS_NULLSTR(arg1) || IS_NULLSTR(arg2) || IS_NULLSTR(arg3)) {
    send_to_char("Syntax:\r\n",ch);
    send_to_char("  grant <player> <field> <arg1> [<arg2>]\r\n", ch);
    send_to_char("  Field being one of:\r\n", ch);
    send_to_char("  hp end exp train \r\n", ch);
    send_to_char("  str int wis dex con \r\n", ch);
    send_to_char("  air earth fire spirit water\r\n", ch);
    send_to_char("  skill weave\r\n", ch);
    send_to_char("  merit flaw talent\r\n", ch);
    send_to_char("  qp qtime webvote rm spark\r\n", ch);
    return;
  }

  if ( ( victim = get_char_anywhere( ch, arg1 ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if(IS_NPC(victim)) {
    send_to_char("You can't grant NPCs.\r\n", ch);
    return;
  }
  
  if ((ch == victim) && (get_trust(ch) < MAX_LEVEL)) {
    send_to_char("You can't grant your self.\r\n", ch);
    return;
  }
  
  /* Build standard Grant message */
  sprintf(g2v_buf, "[ {BGrant{x ]{W:{x You have been granted ");
  sprintf(g2c_buf, "[ {BGrant{x ]{W:{x %s has been granted ", victim->name);

  /*--------------------------------------------------------------*/
  /* HP                                                           */
  if (!str_cmp(arg2, "hp")) {
    if (!is_number(arg3)) {
    send_to_char("Hit points needs to be set with a number.\r\n", ch);
    return;
    }

    value = atoi(arg3);
/*
    if (victim->max_hit + value < -10 || victim->max_hit + value > MAX_PC_HP ) {
    sprintf(buf, "Hp range is -10 to %d hit points.\r\n", MAX_PC_HP);
    send_to_char(buf, ch );
    return;
    }
*/
    
    isGrant=TRUE;
    victim->max_hit += value;
    victim->pcdata->perm_hit += value;
    
    sprintf(buf, "{W%d{x hit points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  /*--------------------------------------------------------------*/
  /* END                                                          */
  if (!str_cmp(arg2, "end")) {
    if (!is_number(arg3)) {
    send_to_char("Endurance points needs to be set with a number.\r\n", ch);
    return;
    }
    
    value = atoi(arg3);
    if (victim->max_endurance + value < 0 || victim->max_endurance + value > MAX_PC_END ) {
    sprintf(buf, "Endurance range is 0 to %d endurance points.\r\n", MAX_PC_END);
    send_to_char(buf, ch );
    return;
    }

    isGrant=TRUE;
    victim->max_endurance += value;
    victim->pcdata->perm_endurance += value;
    
    sprintf(buf, "{W%d{x endurance points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  
  /*--------------------------------------------------------------*/
  /* EXPERIANCE                                                   */
  if (!str_cmp(arg2, "exp")) {
    if (!is_number(arg3)) {
    send_to_char("Experience needs to be set with a number.\r\n", ch);
    return;
    }
    else {
    isGrant=TRUE;
    value = atoi(arg3);
    victim->exp += value;
    
    // Adjust possible to level or not
    if (victim->exp >= exp_next_level(victim) && !IS_SET(victim->act, PLR_CANLEVEL)) {
	 send_to_char("{WYou gain a l{re{Wv{re{Wl{r!!{x\r\n", victim );
	 send_to_char("Use the 'level' command to advance your level.\r\n", victim);
	 SET_BIT(victim->act, PLR_CANLEVEL);
    }
    else if (IS_SET(victim->act, PLR_CANLEVEL) && victim->exp < exp_next_level(victim)) {
	 REMOVE_BIT(victim->act, PLR_CANLEVEL);
    }
    
    sprintf(buf, "{W%d{x experience points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
    }
  }

  /*--------------------------------------------------------------*/  
  /* TRAINS                                                       */
  if (!str_cmp(arg2, "train")) {
    if (!is_number(arg3)) {
    send_to_char("Train needs to be set with a number.\r\n", ch);
    return;
    }
    else {
    isGrant=TRUE;
    value = atoi(arg3);
    victim->train += value;
    
    sprintf(buf, "{W%d{x trains.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
    }
  }

  /*--------------------------------------------------------------*/  
  /* AIR, EARTH, FIRE, SPIRIT, WATER                              */
  
  // AIR
  if (!str_cmp(arg2, "air")) {
    if (!is_number(arg3)) {
    send_to_char("Sphere values needs to be set with a number.\r\n", ch);
    return;
    }
        
    value = atoi(arg3);
    if (victim->perm_sphere[SPHERE_AIR] + value < 0 || 
      victim->perm_sphere[SPHERE_AIR] + value > MAX_SPHERE_VALUE_PC) {
    sprintf(buf, "Sphere value range is from 0 to %d.\r\n", MAX_SPHERE_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_sphere[SPHERE_AIR] += value;
    victim->cre_sphere[SPHERE_AIR] += value;
    sprintf(buf, "{W%d{x air sphere points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  // EARTH
  if (!str_cmp(arg2, "earth")) {
    if (!is_number(arg3)) {
    send_to_char("Sphere values needs to be set with a number.\r\n", ch);
    return;
    }
    
    value = atoi(arg3);
    if (victim->perm_sphere[SPHERE_EARTH] + value < 0 || 
      victim->perm_sphere[SPHERE_EARTH] + value > MAX_SPHERE_VALUE_PC) {
    sprintf(buf, "Sphere value range is from 0 to %d.\r\n", MAX_SPHERE_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_sphere[SPHERE_EARTH] += value;
    victim->cre_sphere[SPHERE_EARTH] += value;
    sprintf(buf, "{W%d{x earth sphere points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  } 

  // FIRE
  if (!str_cmp(arg2, "fire")) {
    if (!is_number(arg3)) {
    send_to_char("Sphere values needs to be set with a number.\r\n", ch);
    return;
    }
        
    value = atoi(arg3);
    if (victim->perm_sphere[SPHERE_FIRE] + value < 0 || 
      victim->perm_sphere[SPHERE_FIRE] + value > MAX_SPHERE_VALUE_PC) {
    sprintf(buf, "Sphere value range is from 0 to %d.\r\n", MAX_SPHERE_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_sphere[SPHERE_FIRE] += value;
    victim->cre_sphere[SPHERE_FIRE] += value;
    sprintf(buf, "{W%d{x fire sphere points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  // SPIRIT
  if (!str_cmp(arg2, "spirit")) {
    if (!is_number(arg3)) {
    send_to_char("Sphere values needs to be set with a number.\r\n", ch);
    return;
    }
        
    value = atoi(arg3);
    if (victim->perm_sphere[SPHERE_SPIRIT] + value < 0 || 
      victim->perm_sphere[SPHERE_SPIRIT] + value > MAX_SPHERE_VALUE_PC) {
    sprintf(buf, "Sphere value range is from 0 to %d.\r\n", MAX_SPHERE_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_sphere[SPHERE_SPIRIT] += value;
    victim->cre_sphere[SPHERE_SPIRIT] += value;
    sprintf(buf, "{W%d{x spirit sphere points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  // WATER
  if (!str_cmp(arg2, "water")) {
    if (!is_number(arg3)) {
    send_to_char("Sphere values needs to be set with a number.\r\n", ch);
    return;
    }
        
    value = atoi(arg3);
    if (victim->perm_sphere[SPHERE_WATER] + value < 0 || 
      victim->cre_sphere[SPHERE_WATER] + value > MAX_SPHERE_VALUE_PC) {
    sprintf(buf, "Sphere value range is from 0 to %d.\r\n", MAX_SPHERE_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_sphere[SPHERE_WATER] += value;
    victim->cre_sphere[SPHERE_WATER] += value;
    sprintf(buf, "{W%d{x water sphere points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  /*--------------------------------------------------------------*/  
  /* STR, INT, WIS, DEX, CON                                      */
  
  /* STR */
  if (!str_cmp(arg2, "str")) {
    if (!is_number(arg3)) {
    send_to_char("Stat values needs to be set with a number.\r\n", ch);
    return;
    }
        
    value = atoi(arg3);
    if (victim->perm_stat[STAT_STR] + value < 3 || 
      victim->perm_stat[STAT_STR] + value > MAX_STAT_VALUE_PC) {
    sprintf(buf, "Stat value range is from 3 to %d.\r\n", MAX_STAT_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_stat[STAT_STR] += value;
    sprintf(buf, "{W%d{x str stat points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }
  
  /* INT */
  if (!str_cmp(arg2, "int")) {
    if (!is_number(arg3)) {
    send_to_char("Stat values needs to be set with a number.\r\n", ch);
    return;
    }
        
    value = atoi(arg3);
    if (victim->perm_stat[STAT_INT] + value < 3 || 
      victim->perm_stat[STAT_INT] + value > MAX_STAT_VALUE_PC) {
    sprintf(buf, "Stat value range is from 3 to %d.\r\n", MAX_STAT_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_stat[STAT_INT] += value;
    sprintf(buf, "{W%d{x int stat points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }
  
  /* WIS */
  if (!str_cmp(arg2, "wis")) {
    if (!is_number(arg3)) {
    send_to_char("Stat values needs to be set with a number.\r\n", ch);
    return;
    }
        
    value = atoi(arg3);
    if (victim->perm_stat[STAT_WIS] + value < 3 || 
      victim->perm_stat[STAT_WIS] + value > MAX_STAT_VALUE_PC) {
    sprintf(buf, "Stat value range is from 3 to %d.\r\n", MAX_STAT_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_stat[STAT_WIS] += value;
    sprintf(buf, "{W%d{x wis stat points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  /* DEX */
  if (!str_cmp(arg2, "dex")) {
    if (!is_number(arg3)) {
    send_to_char("Stat values needs to be set with a number.\r\n", ch);
    return;
    }
        
    value = atoi(arg3);
    if (victim->perm_stat[STAT_DEX] + value < 3 || 
      victim->perm_stat[STAT_DEX] + value > MAX_STAT_VALUE_PC) {
    sprintf(buf, "Stat value range is from 3 to %d.\r\n", MAX_STAT_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_stat[STAT_DEX] += value;
    sprintf(buf, "{W%d{x dex stat points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  /* CON */
  if (!str_cmp(arg2, "con")) {
    if (!is_number(arg3)) {
    send_to_char("Stat values needs to be set with a number.\r\n", ch);
    return;
    }
        
    value = atoi(arg3);
    if (victim->perm_stat[STAT_CON] + value < 3 || 
      victim->perm_stat[STAT_CON] + value > MAX_STAT_VALUE_PC) {
    sprintf(buf, "Stat value range is from 3 to %d.\r\n", MAX_STAT_VALUE_PC);
    send_to_char(buf, ch);
    return;
    }
    
    isGrant=TRUE;
    victim->perm_stat[STAT_CON] += value;
    sprintf(buf, "{W%d{x con stat points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  /* --------------------------------------------------------------*/  
  /* Reset of WebVote                                              */
  if (!str_cmp(arg2, "webvote")) {
    if (!is_number(arg3)) {
	 send_to_char("webvote needs to be set with amount of seconds (1hour = 3600sec).\r\n", ch);
	 return;
    }

    value = atoi(arg3);

    if (value < 0 || value > 10800) {
	 send_to_char("webvote window time range is from 0 to 10800 seconds (3 hours).\r\n", ch);
	 return;
    }
    
    isGrant = TRUE;
    victim->pcdata->last_web_vote = (current_time + value - 3600);

    int hours=0, mins=0, secs=0;
    hours = (value / 3600) % 24;
    mins  = (value % 3600 ) / 60;
    secs  = (value % 60);
    sprintf(buf, "a open webvote window for [{y%02d{xh:{y%02d{xm:{y%02d{xs].\n\r", hours, mins, secs);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  /*--------------------------------------------------------------*/  
  /* Quest time (more or less internal for testing)               */
  if (!str_cmp(arg2, "qtime")) {
    if (!is_number(arg3)) {
	 send_to_char("Quest time needs to be set with a number.\r\n", ch);
	 return;
    }

    value = atoi(arg3);

    if (value < 0 || value > 30) {
	 send_to_char("Quest time range is from 0 to 30.\r\n", ch);
	 return;
    }
    
    isGrant = TRUE;
    victim->pcdata->nextquest = value;
    if (IS_SET(victim->act, PLR_QUESTING)) {
	 victim->pcdata->questgiver = NULL;
	 victim->pcdata->questgivervnum = 0;
	 victim->pcdata->countdown = 0;
	 victim->pcdata->questmob = 0;
	 victim->pcdata->questobj = 0;
    }
    sprintf(buf, "a reset in the quest timer to %d.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  /*--------------------------------------------------------------*/  
  /* Quest points                                                 */
  if (!str_cmp(arg2, "qp")) {
    if (!is_number(arg3)) {
	 send_to_char("Quest points needs to be set with a number.\r\n", ch);
	 return;
    }
    
    value = atoi(arg3);

    if (value < 0 || value > 32000) {
	 send_to_char("Quest points range is from 0 to 32000.\r\n", ch);
	 return;
    }
    
    isGrant = TRUE;
    victim->pcdata->quest_curr += value;
    victim->pcdata->quest_accum += value;
    
    sprintf(buf, "%d quest points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);    
  }

  /*--------------------------------------------------------------*/  
  /* Reward Multiplier points                                                 */
  if (!str_cmp(arg2, "rm")) {
    if (!is_number(arg3)) {
	 send_to_char("Reward Multiplier points needs to be set with a number.\r\n", ch);
	 return;
    }
    
    value = atoi(arg3);

    if (value < 0 || value > 20) {
	 send_to_char("More than 0, less than 20.\r\n", ch);
	 return;
    }
    
    isGrant = TRUE;
    victim->pcdata->reward_multiplier = value;
    victim->pcdata->reward_time = current_time + (24*60*60);
    
    sprintf(buf, "%d reward multiplier points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);    
  }
  

  /*--------------------------------------------------------------*/
  /* PK Counter */
  if (!str_cmp(arg2, "pkcount")) {
    if (!is_number(arg3)) {
         send_to_char("PK Counter points needs to be set with a number.\r\n", ch);
         return;
    }

    value = atoi(arg3);
    isGrant = TRUE;

    victim->pk_count = value;

    sprintf(buf, "%d PK Counter points.\r\n", value);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
  }

  /*--------------------------------------------------------------*/  
  /* SKILLS                                                       */
  if (!str_cmp(arg2, "skill")) {
    if ((sn = skill_lookup( arg3 )) < 0 || skill_table[sn].spell_fun != spell_null) {
    send_to_char("No such skill.\r\n", ch);
    return;
    }

    if (arg4[0] != '\0') {
    value = atoi(arg4);
    if (victim->pcdata->learned[sn] + value < 0 ||
      victim->pcdata->learned[sn] + value > 100) {
      send_to_char("Legal mortal skill range is from 0 to 100.\r\n", ch);
      return;
    }

    isGrant = TRUE;
    victim->pcdata->learned[sn] += value;
    sprintf(buf, "{W%d{x points in the '{W%s{x' skill.\r\n", value, skill_table[sn].name);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
    }
    else {
    if (victim->pcdata->learned[sn] > 0) {
      sprintf(buf, "%s already knows %s to %d.\r\n", victim->name, 
          skill_table[sn].name, victim->pcdata->learned[sn]);
      send_to_char(buf, ch);
      return;
    }
    
    isGrant=TRUE;
    victim->pcdata->learned[sn] = 1;
    sprintf(buf, "the '{W%s{x' skill.\r\n", skill_table[sn].name);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
    }
  }

  /*--------------------------------------------------------------*/  
  /* WEAVES                                                       */
  if (!str_cmp(arg2, "weave")) {
    if ((sn = skill_lookup( arg3 )) < 0 || skill_table[sn].spell_fun == spell_null) {
    send_to_char("No such weave.\r\n", ch);
    return;
    }

    if (arg4[0] != '\0') {
    value = atoi(arg4);
    if (victim->pcdata->learned[sn] + value < 0 ||
      victim->pcdata->learned[sn] + value > 100) {
      send_to_char("Legal mortal skill range is from 0 to 100.\r\n", ch);
      return;
    }
    
    isGrant = TRUE;
    victim->pcdata->learned[sn] += value;
    sprintf(buf, "{W%d{x points in the '{W%s{x' skill.\r\n", value, skill_table[sn].name);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
    }
    else {
    if (victim->pcdata->learned[sn] > 0) {
      sprintf(buf, "%s already knows %s to %d.\r\n", victim->name, 
          skill_table[sn].name, victim->pcdata->learned[sn]);
      send_to_char(buf, ch);
      return;
    }
    
    isGrant=TRUE;
    victim->pcdata->learned[sn] = 1;
    sprintf(buf, "the '{W%s{x' weave.\r\n", skill_table[sn].name);
    strcat(g2v_buf, buf);
    strcat(g2c_buf, buf);
    }
  }

  /*--------------------------------------------------------------*/  
  /* MERIT                                                        */  
  if (!str_cmp(arg2, "merit") || !str_cmp(arg2, "merits")) {
    for (i=0; merit_table[i].name != NULL; i++) {
      if (!str_prefix(merit_table[i].name, arg3)) {
        pos = merit_table[i].bit;
        num = i;
      }
    }
    
    if (pos == NO_FLAG) {
       send_to_char("That merit doesn't exist!\r\n", ch);
       return;
    }
    
    else {
       if (!IS_SET(victim->merits, pos)) {
         isGrant = TRUE;
         SET_BIT(victim->merits, pos);
         sprintf(buf, "the merit '{W%s{x'.\r\n", merit_table[num].name);
         strcat(g2v_buf, buf);
         strcat(g2c_buf, buf);
       }
       else {
         isGrant = TRUE;
         REMOVE_BIT(victim->merits, pos);
         sprintf(buf, "a removing of the merit '{W%s{x'.\r\n", merit_table[num].name);
         strcat(g2v_buf, buf);
         strcat(g2c_buf, buf);
      }
    }
  }

  /*--------------------------------------------------------------*/  
  /* FLAW                                                         */  
  if (!str_cmp(arg2, "flaw") || !str_cmp(arg2, "flaws")) {
    for (i=0; flaw_table[i].name != NULL; i++) {
      if (!str_prefix(flaw_table[i].name, arg3)) {
        pos = flaw_table[i].bit;
        num = i;
      }
    }
    
    if (pos == NO_FLAG) {
       send_to_char("That flaw doesn't exist!\r\n", ch);
       return;
    }
    
    else {
       if (!IS_SET(victim->flaws, pos)) {
         isGrant = TRUE;
         SET_BIT(victim->flaws, pos);
         sprintf(buf, "the flaw '{W%s{x'.\r\n", flaw_table[num].name);
         strcat(g2v_buf, buf);
         strcat(g2c_buf, buf);
       }
       else {
         isGrant = TRUE;
         REMOVE_BIT(victim->flaws, pos);
         sprintf(buf, "a removing of the flaw '{W%s{x'.\r\n", flaw_table[num].name);
         strcat(g2v_buf, buf);
         strcat(g2c_buf, buf);
      }
    }
  }

  /*--------------------------------------------------------------*/  
  /* TALENT                                                       */  
  if (!str_cmp(arg2, "talent") || !str_cmp(arg2, "talents")) {
    for (i=0; talent_table[i].name != NULL; i++) {
      if (!str_prefix(talent_table[i].name, arg3)) {
        pos = talent_table[i].bit;
        num = i;
      }
    }
    
    if (pos == NO_FLAG) {
       send_to_char("That talent doesn't exist!\r\n", ch);
       return;
    }
    
    else {
       if (!IS_SET(victim->talents, pos)) {
         isGrant = TRUE;
         SET_BIT(victim->talents, pos);
         sprintf(buf, "the talent '{W%s{x'.\r\n", talent_table[num].name);
         strcat(g2v_buf, buf);
         strcat(g2c_buf, buf);
       }
       else {
         isGrant = TRUE;
         REMOVE_BIT(victim->talents, pos);
         sprintf(buf, "a removing of the talent '{W%s{x'.\r\n", talent_table[num].name);
         strcat(g2v_buf, buf);
         strcat(g2c_buf, buf);
      }
    }
  }
    
  if (!isGrant) {
    send_to_char("That is not a valid field.\r\n", ch);
    return;
  }
  
  send_to_char(g2v_buf, victim);
  send_to_char(g2c_buf, ch);
  return;
}

void do_review( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;

  if (!IS_IMMORTAL(ch) && !IS_NEWBIEHELPER(ch)) {
    send_to_char("Huh?\r\n", ch);
    return;
  }

  if ( argument[0] == '\0' ) {
    send_to_char( "Review whom?\r\n", ch );
    return;
  }  

  if ( ( victim = get_char_anywhere( ch, argument ) ) == NULL ) {
    if (!IS_IMMORTAL(ch)) {
       send_to_char( "They aren't here.\r\n", ch );
    }
    else {
       send_to_char( "They aren't here. Use 'profile' command  for players not logged on.\r\n", ch );
    }
    return;
  }

  if (IS_NEWBIEHELPER(ch) && !IS_SET(victim->act, PLR_UNVALIDATED)) {
     sprintf(buf, "%s is already validated.\r\n", victim->name);
     send_to_char(buf, ch );
     return;
  }

  if (victim->level > ch->level ) {
    send_to_char( "Reviewing people higher levels than you is bad.\r\n", ch);
    return;
  }

  send_to_char("-------= Review =-------\r\n", ch);

  sprintf(buf, "Name       : %s\r\n", victim->name);
  send_to_char(buf, ch);

  if (IS_ADMIN(ch)) {
     sprintf(buf, "Created    : %s\r\n", ctime(&victim->id));
     send_to_char(buf,ch);

     sprintf(buf, "Class      : %s\r\n", capitalize(class_table[victim->class].name));
     send_to_char(buf, ch);
   
     sprintf(buf, "Level      : %d\r\n", victim->level);
     send_to_char(buf, ch);
     
     sprintf(buf, "Played     : %d\r\n", victim->played);
     send_to_char(buf, ch);
     
     int hours = 0;
     int minutes = 0;
     int seconds = 0;
     hours = victim->roleplayed / 3600;
     minutes = (victim->roleplayed - (hours * 3600) ) / 60;
     seconds = victim->roleplayed - (hours * 3600) - (minutes * 60);
     sprintf(buf, "RolePlayed : %02d:%02d:%02d\n\r", hours,minutes,seconds );
     send_to_char(buf, ch);
     
     if (victim->insanity_points >0)
     {
           sprintf(buf,"Insanity Points: %02d\n\r",victim->insanity_points);
           send_to_char(buf,ch);
     }

     sprintf(buf, "Email      : %s\r\n", victim->pcdata->email);
     send_to_char(buf, ch);
  }
  
  sprintf(buf, "Appearance : %s\r\n", victim->pcdata->appearance[0] != '\0' ? victim->pcdata->appearance : "none");
  send_to_char(buf, ch);

  if (IS_IMMORTAL(ch)) {  
     sprintf(buf, "ICtitle    : %s\r\n", victim->pcdata->ictitle[0] != '\0' ? victim->pcdata->ictitle : "none");
     send_to_char(buf, ch);
   
     if (is_clan(victim)) {
       sprintf( buf, "Guild      : %s\r\n",player_clan(victim));
       send_to_char(buf, ch);
       sprintf( buf, " + Rank    : %d\r\n", victim->rank+1);
       send_to_char(buf, ch);
       sprintf( buf, " + Title   : %s\r\n", victim->gtitle ? victim->gtitle : player_rank(victim));
       send_to_char(buf, ch);
     }

	if (is_oguild(victim)) {
	  sprintf( buf, "OGuild     : %s\r\n", player_oguild(victim));
       send_to_char(buf, ch);
       sprintf( buf, " + Rank    : %d\r\n", victim->oguild_rank+1);
       send_to_char(buf, ch);
       sprintf( buf, " + Title   : %s\r\n", victim->oguild_title ? victim->oguild_title : player_oguild_rank(victim));
       send_to_char(buf, ch);
	}

     if (is_sguild(victim)) {
       sprintf( buf, "SGuild     : %s\r\n",player_sguild(victim));
       send_to_char(buf, ch);
       sprintf( buf, " + Rank    : %d\r\n", victim->sguild_rank+1);
       send_to_char(buf, ch);
       sprintf( buf, " + Title   : %s\r\n", victim->sguild_title ? victim->sguild_title : player_sguild_rank(victim));
       send_to_char(buf, ch);
     }     

     if (is_ssguild(victim)) {
       sprintf( buf, "SSGuil     : %s\r\n",player_ssguild(victim));
       send_to_char(buf, ch);
       sprintf( buf, " + Rank    : %d\r\n", victim->ssguild_rank+1);
       send_to_char(buf, ch);
       sprintf( buf, " + Title   : %s\r\n", victim->ssguild_title ? victim->ssguild_title : player_ssguild_rank(victim));
       send_to_char(buf, ch);
     }
     
     if (victim->minion != 0) {
       sprintf(buf, "Minion     : %s\r\n", victim->mname);
       send_to_char(buf, ch);
       sprintf(buf, " + Rank    : %d\r\n", victim->mrank+1);
       send_to_char(buf, ch);
       sprintf(buf, " + Title   : %s\r\n", victim->mtitle ? victim->mtitle : "Unassigned");
       send_to_char(buf, ch);
     }
     
     if (!IS_NPC(victim) && !IS_NULLSTR(victim->pcdata->df_name)) {
       sprintf(buf, "Darkfriend : %s\r\n", victim->pcdata->df_name);
       send_to_char(buf, ch);
       sprintf(buf, " + Rank    : %d\r\n", victim->pcdata->df_level);
       send_to_char(buf, ch);
     }
     
     sprintf(buf, "Race       : %s\r\n", capitalize(race_table[victim->race].name));
     send_to_char(buf, ch);
   
     sprintf(buf, "Sex        : %s\r\n", sex_table[victim->sex].name);
     send_to_char(buf, ch);
  }
  
}

void do_setemail( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  //char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;

  argument = one_argument( argument, arg1 );

  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\r\n", ch);
    return;
  }

  if ( arg1[0] == '\0' ) {
    send_to_char( "Syntax: setemail <char> <email adr>\r\n", ch );
    return;
  }  

  if ( ( victim = get_char_anywhere( ch, arg1 ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if (victim->level > ch->level ) {
    send_to_char( "You can't set the email address for someone higher level.\r\n", ch);
    return;
  }
  	
  if (argument[0]=='\0' || !(!str_cmp(argument,"none") || strstr(argument,"@"))) {
     send_to_char("Error in Email address. Must include '{W@{x' or set to '{Wnone{x'\r\n", ch);
     return;
  }

  free_string(victim->pcdata->email);
  victim->pcdata->email = str_dup(argument);
  send_to_char("Email address set.\r\n", ch);
  return;
}

void do_setweather(CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  argument = one_argument( argument, arg1 );

  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\r\n", ch);
    return;
  }

   if (arg1[0] == '\0')
   {
      send_to_char("Syntax: setweather <0 - 80> [freeze/unfreeze]\r\n",ch);
      return;
   } 

   if (!is_number(arg1))
   {
      send_to_char("Syntax: setweather <0 - 80> [freeze/unfreeze]\r\n",ch);
      return;
   }

   int val = atoi(arg1);
   if (val < 0 || val > 80)
   {
      send_to_char("Syntax: setweather <0 - 80> [freeze/unfreeze]\r\n",ch);
      return;
   }

   weather_info.mmhg = 960 + val;
   if ( weather_info.mmhg <=  980 ) weather_info.sky = SKY_LIGHTNING;
   else if ( weather_info.mmhg <= 1000 ) weather_info.sky = SKY_RAINING;
   else if ( weather_info.mmhg <= 1020 ) weather_info.sky = SKY_CLOUDY;
   else                                  weather_info.sky = SKY_CLOUDLESS;



   if (argument[0] == '\0')
   {
      send_to_char("OK\r\n",ch);
      return;
   }
   else if (!str_prefix(argument,"freeze")) {     
	g_FreezeWeather = TRUE;
   }
   else if (!str_prefix(argument,"unfreeze")) {     
	g_FreezeWeather = FALSE;
   }
   send_to_char("OK\r\n",ch);

   return;
}
void do_sethome( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  //char buf[MAX_STRING_LENGTH];
  //CHAR_DATA *victim;

  argument = one_argument( argument, arg1 );

  if (!IS_IMMORTAL(ch)) {
    if ( !is_room_owner(ch,ch->in_room))
    { 
    	send_to_char("You don't own this room!\r\n", ch);
        return;
    }
  }

  ch->pcdata->home = ch->in_room->vnum;
  send_to_char("Home set to this room.\r\n",ch);
  return;
}

void do_home( CHAR_DATA *ch, char *argument )
{
  //char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  ROOM_INDEX_DATA *location;
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can go home.\r\n",ch);
    return;
  }

  if (ch->fighting) {
     send_to_char("You are still fighting!\r\n", ch);
     return;
  }

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }
  

  if ((location = get_room_index(ch->pcdata->home) )== NULL) {
       send_to_char( "You don't have a home.\r\n", ch );
       return;
   }

  if ( ch->in_room == location ) {
    send_to_char("You are already at your home!\r\n", ch);
    return;
  }
  

  if ( ( victim = ch->fighting ) != NULL ) {
    stop_fighting( ch, TRUE );
  }

  //act( "$n goes home!", ch, NULL, NULL, TO_ROOM );
  //act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
  char_from_room( ch );
  char_to_room( ch, location );
  //act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
  do_function(ch, &do_look, "auto" );
  
  if (ch->pet != NULL)
  {
    char_from_room(ch->pet);
    char_to_room(ch->pet,location);
  }
  else  if (ch->mount != NULL && (ch->in_room == ch->mount->in_room)) { 
    char_from_room(ch->mount);
    char_to_room(ch->mount,location);
    ch->riding = TRUE;
  }

  return;
}

void do_setidle(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;

  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\r\n", ch);
    return;
  }

  ch->timer = 4;
  
  send_to_char("You set your self to idle status.\r\n", ch);
  return;
}

void do_whoguild(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *pcs[MAX_PC_ONLINE]; // 150 PC's
  BUFFER *output;
  CHAR_DATA *wch;
  int cnt=0;
  int i=0;
  int fillers=0;
  
  // Imm only
  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\r\n", ch);
    return;
  }
  
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

  sprintf(buf, "%-16s      %-16s      %-16s      %-16s      %-16s      %-18s      %-16s\r\n",
		"{yPlayer{x:", "{8Guild{x:", "{NOGuild{x:", "{SSGuild{x:", "{USSGuild{x:", "{hMinion{x:", "{XDF{x:");
  add_buf(output, buf);
  
  sprintf(buf, "%-16s  %-16s  %-16s  %-16s  %-16s  %-18s  %-16s\r\n",
		"-------", "------", "------", "-------", "-------", "-------", "---");
  add_buf(output, buf);
    
  for (i=0; i < cnt; i++) {

    if (get_trust(ch) < pcs[i]->invis_level)
	 continue;

    fillers = (16 - colorstrlen(COLORNAME(pcs[i])));
    
    sprintf(buf, "%s%*s  %s%-16s{x  %s%-16s{x  %s%-16s{x  %s%-16s{x  %s%-18s{x  %s%-16s{x\r\n",
		  COLORNAME(pcs[i]),
		  fillers, "",
		  is_clan(pcs[i])                      ? "{x" : "{R",
		  is_clan(pcs[i]) ? player_clan(pcs[i])                          : "n/a",
		  is_oguild(pcs[i])                    ? "{x" : "{R",
		  is_oguild(pcs[i]) ? player_oguild(pcs[i])                      : "n/a",
		  is_sguild(pcs[i])                    ? "{x" : "{R",
		  is_sguild(pcs[i]) ? player_sguild(pcs[i])                      : "n/a",
		  is_ssguild(pcs[i])                   ? "{x" : "{R",
		  is_ssguild(pcs[i]) ? player_ssguild(pcs[i])                    : "n/a",
		  pcs[i]->minion != 0                  ? "{x" : "{R",
		  pcs[i]->minion != 0 ? pcs[i]->mname                            : "n/a",
		  !IS_NULLSTR(pcs[i]->pcdata->df_name) ? "{x" : "{R",
		  !IS_NULLSTR(pcs[i]->pcdata->df_name) ? pcs[i]->pcdata->df_name : "n/a");
    add_buf(output, buf);		  
  }

  sprintf(buf, "\r\nListed players: {y%d{x.\r\n", cnt);
  add_buf(output, buf);
  
  page_to_char( buf_string(output), ch );
  free_buf(output);
  return;
}

void do_plist(CHAR_DATA *ch, char *argument)
{
   struct stat sb;
   time_t last_on_time;
   char last_on_time_str[MSL];
   CHAR_DATA *victim;  
   bool isOnline = FALSE;
   time_t st_time;
   time_t en_time;
   
   BUFFER *pbuf;
   struct dirent **Dir;
   int n;
   int i;
   char fname[80];
   char buf[MSL];
   char buffer[MSL];
   int nMatch=0;

   char read[MAX_INPUT_LENGTH];
   char *ptr, *text;
   char name[MSL];
   int level=0;
   char race[MSL];
   char class[MSL];
   int  iclass=0;
   
   bool find_more = TRUE;
   FILE *fp;

  n = scandir(PLAYER_DIR, &Dir, 0, alphasort);

  if (n < 0) {
     send_to_char("No players found!\r\n", ch);
     return;	
  }

  st_time = time(NULL);
  
  pbuf = new_buf();

   send_to_char("{C                           PLAYER BASE                                 {x\r\n",ch);
   send_to_char("{R Name          lvl   Race         Class          Last on               {x\r\n",ch);
   send_to_char("{R======================================================================={x\r\n",ch);

   for (i=0; i<n; i++) {
	sprintf(fname, PLAYER_DIR "%s", Dir[i]->d_name); {

	  if (Dir[i]->d_name[0] >= 'A' && Dir[i]->d_name[0] <= 'Z' && Dir[i]->d_name[0] != '.') {
	    nMatch++;

	    if ( ( fp = fopen( fname, "r" ) ) == NULL )
		 continue;

	    /* Name not so hard to find.. it's the FNAME!! */
	    sprintf(name, "%s", Dir[i]->d_name);
	    
	    while( find_more && (fgets( read, 80, fp  ) != NULL) )  {
		 int len;
		 len = strlen( read );
		 if( read[len-2] == '~' )
		   read[len-2] = '\0';
		 
		 memset(buf, 0x00, sizeof(buf));
		 ptr = read; 
		 text = buf;
		 
		 while( *ptr != ' ' )   /* Grab the keyword only */
		   *text++ = *ptr++ ;		 

		 switch( buf[0] ) {
		 default:  
		   continue;
		   break;

		 case 'A':
		   if( !strcmp( buf, "Alias" ) )  {
			find_more = FALSE;
			break;
		   }
		   break;
		 case 'B': 
		   if ( !strcmp( buf, "Bounty" ) )  {
			find_more = FALSE;
		   }
		   break;
		 case 'C':
		   if (!strcmp(buf, "Cla") ) {
			while ( *ptr == ' ' ) ptr++;
			iclass = atoi(ptr);
			if (iclass >= MAX_CLASS)
			  sprintf(class, "%s", "!Bad class!");
			else
			  sprintf(class, "%s", capitalize(class_table[iclass].name));
			break;
		   }
		   
		   if( !strcmp( buf, "Colors" ) ) /* Done looking */ {
			find_more = FALSE;
			break; 
		   }
		   break;

		 case 'E':
		   if( !strcmp( buf, "End" ) ) /* Done looking */ {
			find_more = FALSE;
			break; 
		   }
		   break;

		 case 'L':
		   if( !strcmp( buf, "Levl" ) ) {
			while ( *ptr == ' ' ) ptr++; 
			level = atoi(ptr);
			break;
		   }
		   
		 case 'R':
		   if( !strcmp( buf, "Race" ) ) {
			while ( *ptr == ' ' ) ptr++;
			sprintf(race, "%s", capitalize(ptr));
			break;
		   }
		   break;
		 case 'P':
		   if( !strcmp( buf, "Plyd" ) ) {
			find_more = FALSE;
			break; 
		   }
		   break;
		 case 'S':
		   if( !strcmp( buf, "Sec" ) ) {
			find_more = FALSE;
			break; 
		   }
		   break;
		 }

		 memset(read, 0x00, sizeof(read));		 
	    }
	    fclose(fp);	    
	    free(Dir[i]);

	 /* Is Victim online ? */
	 if ((victim = get_char_anywhere(ch, name)) != NULL) {
	   isOnline = TRUE;
	 }
	 else {
	   isOnline = FALSE;
	   stat(fname,&sb);
	   last_on_time = (sb.st_mtime);
	   sprintf(last_on_time_str, "%s", (char *)ctime(&last_on_time));
	   last_on_time_str[strlen(last_on_time_str)-1] = '\0';
	 }

	    if (level > ch->level)
		 sprintf(buffer, "{B[{c%-12s{B][{c%3s{B][{c%-10s{B][{c%-12s{B][{c%24s{B]{x\r\n", name, "xxx", race, class, isOnline ? "Online" : last_on_time_str);
	    else
		 sprintf(buffer, "{B[{c%-12s{B][{c%3d{B][{c%-10s{B][{c%-12s{B][{c%24s{B]{x\r\n", name, level, race, class, isOnline ? "Online" : last_on_time_str);
	    add_buf(pbuf, buffer);
	  }
	}
	find_more = TRUE;
   }
      
   free(Dir);

   en_time = time(NULL);

   sprintf( buffer, "\r\n{cPlayers found{C: {Y%d{x\r\n", nMatch );
   add_buf(pbuf, buffer);
   if (IS_CODER(ch)) {
	sprintf( buffer, "\r\n[ {YCoder{x ]: Plist used time = %ldsec.\r\n", en_time - st_time);
	add_buf(pbuf, buffer);
   }
   page_to_char(buf_string(pbuf), ch);
   free_buf(pbuf);
   return;
}

// Find teacher, gainer and trainers
void do_twhere( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    CHAR_DATA *victim;
    CHAR_DATA *victim_next;
    bool found;
    int count = 0;
    int fillers = 0;

    found = FALSE;
    buffer = new_buf();

    for ( victim = char_list; victim != NULL; victim = victim_next ) {
      victim_next = victim->next;
      
      if (!IS_NPC(victim))
         continue;
         
      if (!IS_SET(victim->act,ACT_GAIN) &&
          !IS_SET(victim->act,ACT_TRAIN))
          continue;
          
      found = TRUE;
      count++;
      
      fillers = (28 - colorstrlen(victim->short_descr));
      if (fillers <= 0)
         fillers = 0;
      
      sprintf(buf, "{y%3d{x) [%5d] %s%*s [%5d] %s\r\n", count,
                   victim->pIndexData->vnum,
                   victim->short_descr,
                   fillers, "",
                   victim->in_room->vnum,
                   victim->in_room->name );
      add_buf(buffer,buf);
      
      if (IS_SET(victim->act,ACT_GAIN)) {
         sprintf(buf, "     {GGain flags:  [%s]{x\r\n", flag_string( gain_flags, victim->gain_flags ));
         add_buf(buffer,buf);
      }
      
      if (IS_SET(victim->act,ACT_TRAIN)) {      
         sprintf(buf, "     {RTrain flags: [%s] Tlevel: [%d]{x\r\n", flag_string( train_flags, victim->train_flags ), victim->train_level);
         add_buf(buffer,buf);
      }
      
      sprintf(buf, "\r\n");
      add_buf(buffer,buf);
    }
   

    if ( !found )
      send_to_char("No teachers found!\r\n", ch);
    else
      page_to_char(buf_string(buffer),ch);

    free_buf(buffer);
    
    return;
}

void do_scatter( CHAR_DATA *ch, char *argument )
{
  char buf[MSL];
  char arg[MSL];
  OBJ_DATA *obj=NULL;
  OBJ_DATA *obj_next=NULL;
  CHAR_DATA *mob=NULL;
  CHAR_DATA *mob_next=NULL;
  ROOM_INDEX_DATA *pRoomIndex=NULL;
  AREA_DATA *pArea=NULL;
  bool scatter_mob=FALSE;
  bool scatter_obj=FALSE;
  bool scatter2vmap=FALSE;
  bool scatter2area=FALSE;

  argument = one_argument(argument, arg);

  if (IS_NULLSTR(arg)) {
    send_to_char("Syntax: scatter <obj/mob> [vmap/area]\n\r", ch);
    return;	
  }
  
  if (!str_cmp(arg, "obj"))
     scatter_obj=TRUE;
  else if (!str_cmp(arg, "mob"))
     scatter_mob=TRUE;
  else  {
    send_to_char("Syntax: scatter <obj/mob> [vmap/area]\n\r", ch);
    return;	
  }
  
  if (!str_cmp(argument, "vmap"))
    scatter2vmap=TRUE;
    
  if (!str_cmp(argument, "area"))
    scatter2area=TRUE;
  

  if (scatter_obj) {
     if (scatter2area) {
     	pArea = ch->in_room->area;
        for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next ) {
           obj_next = obj->next_content;
                            
           pRoomIndex = get_room_index(number_range(pArea->min_vnum, pArea->max_vnum));
        
           while (pRoomIndex == NULL) {
              pRoomIndex = get_room_index(number_range(pArea->min_vnum, pArea->max_vnum));
           }
           act("$p vanishes in a puff of smoke!",ch,obj,NULL,TO_ROOM);
           sprintf(buf, "%s is scattered to room [%d] in area [%s]\n\r", obj->short_descr, pRoomIndex->vnum, pRoomIndex->area->name);
           send_to_char(buf, ch);
           obj_from_room(obj);
           obj_to_room(obj, pRoomIndex);
        }     	
     }
     else {
        for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next ) {
           obj_next = obj->next_content;
        
           pRoomIndex = get_random_room(ch, TRUE);
        
           while ( (pRoomIndex == NULL)
                 || (!str_cmp(pRoomIndex->area->file_name, "school.are"))
                 || (!str_cmp(pRoomIndex->area->file_name, "vmap.are") && !scatter2vmap)
	         || (!IS_SET(pRoomIndex->area->area_flags, AREA_OPEN))) 
           {
              pRoomIndex = get_random_room(ch, TRUE);
           }
           act("$p vanishes in a puff of smoke!",ch,obj,NULL,TO_ROOM);
           sprintf(buf, "%s is scattered to room [%d] in area [%s]\n\r", obj->short_descr, pRoomIndex->vnum, pRoomIndex->area->name);
           send_to_char(buf, ch);
           obj_from_room(obj);
           obj_to_room(obj, pRoomIndex);
        }
     }
  }
  
  if (scatter_mob) {
     if (scatter2area) {
     	pArea = ch->in_room->area;
        for ( mob = ch->in_room->people; mob != NULL; mob = mob_next ) {
           mob_next = mob->next_in_room;
           if (!IS_NPC(mob))
              continue;
	   if (mob->world != ch->world)
	      continue;
	   if (is_same_group(mob, ch))
	      continue;
	   if (mob->mount != NULL)
	      continue;
	      
	   pRoomIndex = get_room_index(number_range(pArea->min_vnum, pArea->max_vnum));
	      
	   while (pRoomIndex == NULL) {
              pRoomIndex = get_room_index(number_range(pArea->min_vnum, pArea->max_vnum));
           }  
	   
	   act("$N vanishes in a puff of smoke!",ch,NULL,mob,TO_ROOM);
           sprintf(buf, "%s is scattered to room [%d] in area [%s]\n\r", mob->short_descr, pRoomIndex->vnum, pRoomIndex->area->name);
           send_to_char(buf, ch);   
	   char_from_room(mob);
	   char_to_room(mob, pRoomIndex);   
	   
       }     	
     }
     else {
        for ( mob = ch->in_room->people; mob != NULL; mob = mob_next ) {
           mob_next = mob->next_in_room;
           if (!IS_NPC(mob))
              continue;
	   if (mob->world != ch->world)
	      continue;
	   if (is_same_group(mob, ch))
	      continue;
	   if (mob->mount != NULL)
	      continue;
	      
	   pRoomIndex = get_random_room(ch, TRUE);   
	      
	   while ( (pRoomIndex == NULL)
                 || (!str_cmp(pRoomIndex->area->file_name, "school.are"))
                 || (!str_cmp(pRoomIndex->area->file_name, "vmap.are") && !scatter2vmap)
	         || (!IS_SET(pRoomIndex->area->area_flags, AREA_OPEN))) 
           {
              pRoomIndex = get_random_room(ch, TRUE);
           }  
	   
	   act("$N vanishes in a puff of smoke!",ch,NULL,mob,TO_ROOM);
           sprintf(buf, "%s is scattered to room [%d] in area [%s]\n\r", mob->short_descr, pRoomIndex->vnum, pRoomIndex->area->name);
           send_to_char(buf, ch);   
	   char_from_room(mob);
	   char_to_room(mob, pRoomIndex);   
	   
       }
     }
  }  	
  
  return;
}

void do_masquerade( CHAR_DATA *ch, char *argument )
{  
  char buf[MSL];
  char tmp[MSL];
  char arg[MSL];
  extern MASQUERADE_TYPE masquerade;
  CHAR_DATA *victim=NULL;
  DESCRIPTOR_DATA *d=NULL;
  int tnum=0;
  int vnum=0;
  sh_int race=0;
  AREA_DATA *pArea=NULL;
  ROOM_INDEX_DATA *location;
  bool all=TRUE;

  argument = one_argument(argument, arg);

  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (IS_NULLSTR(arg)) {
    send_to_char("Syntax: masquerade <on/off>\n\r", ch);
    send_to_char("        masquerade town <area num>\n\r", ch);
    send_to_char("        masquerade room <vnum>\n\r", ch);
    send_to_char("        masquerade race <race>\n\r", ch);
    send_to_char("        masquerade info\n\r", ch);    
    send_to_char("        masquerade announce\n\r", ch);
    return;	
  }

  if (!str_cmp(arg, "on")) {
    if (masquerade.on) {
	 send_to_char("Masquerade is already on!\n\r", ch);
	 return;	 
    }
    else {
	 masquerade.on = TRUE;
	 send_to_char("Masquerade has been toggled on for all areas and races.\n\r", ch);
	 return;
    }
  }
  else if (!str_cmp(arg, "off")) {
    if (!masquerade.on) {
	 send_to_char("Masquerade is already off!\n\r", ch);
	 return;
    }
    else {
	 masquerade.on = FALSE;
	 masquerade.anum = 0;
	 masquerade.vnum = 0;
	 masquerade.race = 0;
	 send_to_char("Masquerade has been toggled off for all areas and races.\n\r", ch);
	 return;
    }
  }
  else if (!str_cmp(arg, "town")) {
    if (!(tnum = atoi(argument))) {
	 send_to_char("Syntax: masquerade town <area num>\n\r", ch);
	 return;
    }
    else {
	 for (pArea = area_first; pArea; pArea = pArea->next) {
	   if (pArea->vnum == tnum) {
		masquerade.anum = tnum;
		sprintf(buf, "Masquerade limited to area '%s' only.\n\r", pArea->name);
		send_to_char(buf, ch);
		return;
	   }	   
	 }
	 
	 send_to_char("Area not found!\n\r", ch);
	 return;
    }
  }
  else if (!str_cmp(arg, "room")) {
    if (!(vnum = atoi(argument))) {
	 send_to_char("Syntax: masquerade room <vnum>\n\r", ch);
	 return;
    }
    else {
	 if (( location = get_room_index( vnum ) ) == NULL ) {
	   send_to_char("Room not found!\n\r", ch);
	   return;
	 }
	 else {
	   masquerade.vnum = vnum;

	   if (masquerade.anum != 0) {
		send_to_char("Masquerade limit on area removed.\n\r", ch);
		masquerade.anum = 0;
	   }
	   
	   sprintf(buf, "Masquerade limited to room vnum %d - '%s'.\n\r", vnum, location->name);
	   send_to_char(buf, ch);	   
	   return;
	 }
    }
  }
  else if (!str_cmp(arg, "race")) {
    race =  race_lookup(argument);
    if (race == 0) {
	 send_to_char("Race not found!\n\r", ch);
	 return;
    }
    else {
	 masquerade.race = race;
	 sprintf(buf, "Masquerade limited to race '%s'.\n\r", race_table[race].who_name);
	 send_to_char(buf, ch);
	 return;
    }
  }
  else if (!str_cmp(arg, "info")) {

    if (!masquerade.on) {
	 send_to_char("Masquerade is off.\n\r", ch);
	 return;
    }
    
    sprintf(buf, "{mMasquerade info{W:{x\n\r"
		       "----------------\n\r");
    send_to_char(buf, ch);
    
    if (masquerade.anum != 0) {
	 for (pArea = area_first; pArea; pArea = pArea->next) {
	   if (pArea->vnum == masquerade.anum) {
		sprintf(buf, "Town : {y%s{x\n\r", pArea->name);
	   }
	 }
    }
    else
	 sprintf(buf, "Town : {gNot set{x.\n\r");
    send_to_char(buf, ch);


    if (masquerade.vnum != 0)
	 sprintf(buf, "Room : {y%d{x\n\r", masquerade.vnum != 0 ? masquerade.vnum : -1);
    else
	 sprintf(buf, "Room : {gNot set{x.\n\r");    
    send_to_char(buf, ch);
    
    sprintf(buf, "Race : %s\n\r", masquerade.race != 0 ? race_table[masquerade.race].who_name : "{gNot set{x.");
    send_to_char(buf, ch);

  }
  else if (!str_cmp(arg, "announce")) {
    
    if (!masquerade.on) {
	 send_to_char("Masquerade is off.\n\r", ch);
	 return;
    }

    sprintf(buf, "{W[{mMasquerade{W]:{x Started for ");

    if (masquerade.race != 0) {
	 sprintf(tmp, "race '{C%s{x'", race_table[masquerade.race].who_name);
	 strcat(buf, tmp);	   
	 all=FALSE;
    }
    
    if (masquerade.vnum != 0) {
	 location = get_room_index( masquerade.vnum );
	 sprintf(tmp, "%sroom '{y%s{x' (in area '%s')", all ? "" : " and ", location->name, location->area->name);
	 strcat(buf, tmp);	   
	 all=FALSE;
    }
    
    if (masquerade.anum != 0) {
	 for (pArea = area_first; pArea; pArea = pArea->next) {
	   if (pArea->vnum == masquerade.anum)
		break;
	 }
	 
	 sprintf(tmp, "%sarea '{y%s{x'", all ? "" : " and ", pArea->name);
	 strcat(buf, tmp);	   
	 all=FALSE;
    }

    if (all) {
	 strcat(buf, "{gall{x PCs.\n\r");	 
    }
    else {
	 strcat(buf, ".\n\r");
    }
    
    for (d = descriptor_list; d!= NULL; d = d->next) {
	 victim = d->character;	   
	 if (victim == NULL || IS_NPC(victim))
	   continue;
	 
	 send_to_char(buf, victim);
	 
	 if (can_use_masquerade(victim))
	   send_to_char("{W[{mMasquerade{W]:{x You may use the '{Wmaskapperance{x' and '{Wmask{x' commands now.\n\r", victim);
    }
  }
  else {
    send_to_char("Syntax: masquerade <on/off>\n\r", ch);
    send_to_char("        masquerade town <area num>\n\r", ch);
    send_to_char("        masquerade room <vnum>\n\r", ch);
    send_to_char("        masquerade race <race>\n\r", ch);
    send_to_char("        masquerade info\n\r", ch);    
    send_to_char("        masquerade announce\n\r", ch);
    return;	
  }
  
  
  return;
}

void do_forcespark( CHAR_DATA *ch, char *argument )
{
     CHAR_DATA * victim;

	
     if ((victim = get_char_anywhere(ch,argument)) != NULL)
     {

	if (IS_NPC(victim))
	{
		send_to_char("Only on PC's\r\n",ch);
		return;
	}
     	if (victim->pcdata->forcespark == 0)
     	{
		send_to_char("Character will spark on next level\r\n",ch);
		victim->pcdata->forcespark = 1;
     	}
     	else
     	{
		send_to_char("Spark on next level removed\r\n",ch);
		victim->pcdata->forcespark = 0;
     	}
     }
     else
     {
	send_to_char("That player was not found\r\n",ch);
     }

}
void do_forceinsanity( CHAR_DATA *ch, char *argument )
{
     CHAR_DATA * victim;


     if ((victim = get_char_anywhere(ch,argument)) != NULL)
     {

        if (IS_NPC(victim))
        {
                send_to_char("Only on PC's\r\n",ch);
                return;
        }
        if (victim->pcdata->forceinsanity == 0)
        {
                send_to_char("Character will gain an insanity point on the next IC Emote\r\n",ch);
                victim->pcdata->forceinsanity = 1;
        }
        else
        {
                send_to_char("Force Insanity  on next emote removed\r\n",ch);
                victim->pcdata->forceinsanity = 0;
        }
     }
     else
     {
        send_to_char("That player was not found\r\n",ch);
     }


}
void do_setRewardMultiplier( CHAR_DATA *ch, char *argument )
{
	int argval;
	char arg1[MAX_STRING_LENGTH];
	one_argument(argument,arg1);
        if (arg1[0] == '\0') 
	{
	  send_to_char("Syntax: setRewardMultiplier #\n\r",ch);
	  return;
	}

	if (!is_number(arg1))
	{
	  send_to_char("Syntax: setRewardMultiplier #\n\r",ch);
	  return;
	}

	argval = atoi(arg1);
	if ( argval < 0 || argval > 10)
	{
		send_to_char("Minimum is 0, Maximum is 10\n\r",ch);
		return;
	}

	reward_multiplier = argval;
	reward_time = current_time + (60*60*24);
	if (argval == 0) 
	{
		send_to_char("Reward Multiplier turned off.\n\r",ch);
	}
	else
	{
		char buffer[256];
		sprintf(buffer,"Reward Multipler set to %d for 24 hours.\r\n",argval);
		send_to_char(buffer,ch);
	}
	return;
}

void do_retire_imm(CHAR_DATA * ch, char * argument) {

	char buff[MAX_STRING_LENGTH];
	char arg1[MAX_STRING_LENGTH];

 	if (IS_NULLSTR(argument)) {
		send_to_char("Syntax: retireimm <password>\r\n\tThis command retires your immortal character from active duty.  Use this when you feel that you have done what you desire to do as an Immortal at The Shadow Wars, yet still want to keep your character pfile around for general chatting.  It is a way of stepping out nicely without any hard feelings either way.  You will not be asked why you did it, nor will you be asked to return to active duty.  Neither should you ask to return to active duty once you have retired.  The stipulation of not leading a guild will no longer apply, as you will no longer be actively participating in running the game.\r\n",ch);
		return;
        }

  	if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ))
  	{
    		send_to_char("Wrong password.\n\r",ch);
    		return;
  	}

	ch->level = LEVEL_IMMORTAL;
   	set_imminfo(ch,"RETIRED");
	do_wiznet(ch,"off");
	do_save(ch,"");

}
