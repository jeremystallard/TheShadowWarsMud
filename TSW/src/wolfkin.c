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
 * This file include code special dedicated to the Wolfkin talent          *
 * TSW is copyright -2003 Swordfish and Zandor                             *
 **************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"

bool check_parse_name (char* name);                 /* comm.c     */
char *emote_parse(CHAR_DATA *ch,char *argument );   /* act_comm.c */
void do_remove( CHAR_DATA *ch, char *argument );    /* act_obj.c  */

/* Compare two wolfkin by wkname */
int compare_wk_names(const void *v1, const void *v2)
{
   return strcmp(colorstrem((*(CHAR_DATA**)v1)->wkname), colorstrem((*(CHAR_DATA**)v2)->wkname));
}

/*
 * Show current playing and wizible wolfkin in your area
 */
void do_wkl ( CHAR_DATA *ch, char *argument )
{
  static char * const brother_sister [] = { "Kind   ",  "Brother", "Sister " };

  BUFFER *output;
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;  
  CHAR_DATA *wch;    
  CHAR_DATA *pcs[MAX_PC_ONLINE];
  int cnt = 0;
  int i   = 0;
  int fillers=0;

  if (!IS_WOLFKIN(ch) && IS_SET(ch->talents, TALENT_WOLFKIN)) {
   send_to_char("You are too young to be able to sense other wolves!\n\r", ch);
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

  output = new_buf();

  for (d = descriptor_list; d != NULL; d = d->next) {

    if (d->connected != CON_PLAYING || !can_see_channel(ch, d->character))
    continue;
    
    wch = ( d->original != NULL ) ? d->original : d->character;
    
    if (!can_see_channel(ch, wch))
    continue;

    if (!IS_WOLFKIN(wch))
    continue;

    /* Can only "sense" wolfs in same area */
/*    
    if (wch->in_room->area != ch->in_room->area)
    continue;
*/

    /* Only those who have given them self names */
    if (wch->wkname[0] == '\0')
    continue;
    
    pcs[cnt++] = wch;
  }

  /* sort array of characters based on their wkname */  
  qsort ( pcs, cnt, sizeof(wch), compare_wk_names);

 /* Build wolfkin list */
  for (i=0; i < cnt; i++) {
    fillers = (16 - colorstrlen(pcs[i]->wkname)-1);
    if (pcs[i] == ch)
	 sprintf(buf, "{c[{YWolf %s{c]{x %s%*s - %12s  {x(%s)\n\r", 
		    brother_sister[URANGE(0,pcs[i]->sex, 2)], 
		    pcs[i]->wkname,
		    fillers, "",
 		    (IS_IMMORTAL(ch) ? pcs[i]->name : ""),
		    "{Wyou{x");
    else     
	 sprintf(buf, "{c[{YWolf %s{c]{x %s%*s - %12s  {x(%s)\n\r", 
		    brother_sister[URANGE(0,pcs[i]->sex, 2)], 
		    pcs[i]->wkname,
		    fillers, "",
 		    (IS_IMMORTAL(ch) ? pcs[i]->name : ""),
		    pcs[i]->in_room->area == ch->in_room->area ? "{yclose by{x" : "{cin the distance{x");
    
    add_buf(output, buf);
  }
  
  sprintf(buf, "\nWolf souls sensed: {y%d{x.\n\r", cnt);
  add_buf(output,buf);
  
  page_to_char(buf_string(output),ch);
  free_buf(output);  
}

void do_wkname ( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];

  if (!IS_WOLFKIN(ch) && IS_SET(ch->talents, TALENT_WOLFKIN)) {
   send_to_char("You are too young to name your self amoung the wolves!\n\r", ch);
   return;
  }
  
  if (!IS_WOLFKIN(ch)) {
     send_to_char("Huh?\n\r", ch); 
     return;
  }

  if ( argument[0] == '\0' ) {
    send_to_char("Please spesify a wolfkin name for your self.\n\r", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && ch->wkname[0] != '\0') {
    send_to_char("You already have set a wolfkin name for your self.\n\r", ch);
    send_to_char("If you need to change your wolfkin name, see one of the immortals for assistance.\n\r", ch);
    return;
  }

  sprintf(buf, "%s", argument);  

  if (!check_parse_name(colorstrem(buf))) {
     send_to_char("Illegal wolf name, try another name.\n\r", ch);
     return; 
  }

  free_string( ch->wkname);
  ch->wkname = str_dup( buf );

  sprintf(buf, "You will be known as '%s' amoung your fellow wolfkin.\n\r", ch->wkname);
  send_to_char(buf, ch);

  return;
}

// Depending on the one enticing wolves
// set up the wolve accordingly
CHAR_DATA *update_wolf(CHAR_DATA *ch, CHAR_DATA *wolf, int number, int enticed, char *wolf_appearance)
{
  char buf[MSL];
  CHAR_DATA *_wolf=wolf;
  int level=0;
  int lowdam=0;
  int highdam=0;
  //int dammod=0;
  //int addition=0;
  int lowhit=0;
  int highhit=0;
  int hitmod=0;

  // If standard wolf is higher level than what update will be
  // use standard wolf
  if (wolf->level >= (get_level(ch)/2))
    return wolf;
  
  // Max entice level is 90
  if (get_level(ch) >= 200 && number <= 1 && enticed <= 0)
    level = 96;
  else if (get_level(ch) >= 200 && enticed >= 1)
    level = 86;
  else if (get_level(ch) >= 200)
    level = 91;
  else
    level = get_level(ch)/2;

  //Make DF WK suffer by loading uber-wolves who will turn and attack
  if (!IS_NPC(ch) && ch->pcdata->df_level >= 0 )
  {
	level = get_level(ch) * 2;
	_wolf->to_hunt = ch;
  }
  
  _wolf->level        = level-number;
  _wolf->hitroll      = (level-15) - (number*2);
  _wolf->damroll      = (level/2) - (number*2);

  //calculate hit dice
  lowhit = (level / 2) + 1;
  if (level % 2) {
    highhit = (level / 2) + 1;
  }
  else {
    highhit = (level / 2) + 2;
  }
  hitmod = (level * level);
  
  _wolf->max_hit      = dice(lowhit, (highhit-number))+(hitmod-number);
  _wolf->hit          = _wolf->max_hit;
  
  _wolf->hit_loc[LOC_LA] = get_max_hit_loc(_wolf, LOC_LA);
  _wolf->hit_loc[LOC_LL] = get_max_hit_loc(_wolf, LOC_LL);
  _wolf->hit_loc[LOC_HE] = get_max_hit_loc(_wolf, LOC_HE);
  _wolf->hit_loc[LOC_BD] = get_max_hit_loc(_wolf, LOC_BD);
  _wolf->hit_loc[LOC_RA] = get_max_hit_loc(_wolf, LOC_RA);
  _wolf->hit_loc[LOC_RL] = get_max_hit_loc(_wolf, LOC_RL);
  
  _wolf->max_endurance   = dice(level, level/2)+100;
  _wolf->endurance       = _wolf->max_endurance;
  
  _wolf->damage[DICE_NUMBER] = lowdam;
  _wolf->damage[DICE_TYPE]   = highdam;
  
  //Armor class
  if (level >= 45) {
    int newac = (level - 30);
    _wolf->armor[0] = newac;
    _wolf->armor[1] = newac;
    _wolf->armor[2] = newac;
    _wolf->armor[3] = newac;
  }
  else if ((level >= 0) && level <= 11) {
    int newac =  level - 11;
    _wolf->armor[0] = newac;
    _wolf->armor[1] = newac;
    _wolf->armor[2] = newac;
    _wolf->armor[3] = newac;
  }
  else if ((level >= 12) && (level <45)) {
    int newac = ((level / 2) - 4);
    _wolf->armor[0] = newac;
    _wolf->armor[1] = newac;
    _wolf->armor[2] = newac;
    _wolf->armor[3] = newac;
  }
  
  if (!IS_NULLSTR(wolf_appearance)) {
  	if (strstr(wolf_appearance, "wolf") != NULL) {
  	   sprintf(buf, "%s", wolf_appearance);
  	   buf[0] = LOWER(buf[0]);
  	   free_string(_wolf->short_descr);
  	   _wolf->short_descr = str_dup(buf);
  	   
  	   sprintf(buf, "%s is here.\n\r", wolf_appearance);
  	   buf[0] = UPPER(buf[0]);
  	   free_string(_wolf->long_descr);
  	   _wolf->long_descr = str_dup(buf);
  	}
  }
    
  return _wolf;
}

void do_entice (  CHAR_DATA *ch, char *argument )
{
  static char * const numbername [] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"};
  CHAR_DATA *pet;
  CHAR_DATA *npc;    
  CHAR_DATA *npc_next;
  char buf[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  int sn = 0;
  int endurance = 0;
  int wolfs = 1;
  int i = 0;
  int enticed_wolves = 0;
  int wanted_wolves = 0;

  if (!IS_WOLFKIN(ch) && IS_SET(ch->talents, TALENT_WOLFKIN)) {
   send_to_char("You are too young to be able to communicate with other wolves in such a way.\n\r", ch);
   return;
  }

  if (!IS_WOLFKIN(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (IS_WOLFSHAPE(ch)) {
    send_to_char("You can't entice any wolves while in wolf shape.\n\r", ch);
    return;
  }

  sn        = skill_lookup("entice");
  endurance = skill_table[sn].min_endurance;

  if (!IS_NPC(ch) && ch->pcdata->learned[sn] < 1) {
    send_to_char("You don't know how to entice yet.\n\r", ch);
    return;
  }

  if (!IS_SET(ch->in_room->room_flags, ROOM_VMAP)) {
    send_to_char("The city noise makes you unable to communicate with the other wolves in such a way.\n\r", ch);
    return;
  }
  

  if ( ch->master != NULL || ( ch->leader != NULL && ch->leader != ch )) {
     send_to_char( "You need to lead the group to be able to controll it!\n\r", ch );
     return;
  }
  
  /* Find out how many wolves already are enticed */
  for ( npc = char_list; npc != NULL; npc = npc_next ) {
    npc_next	= npc->next;
    if ( npc->in_room == NULL )
	    continue;
	 if (!IS_NPC(npc))
	    continue;
	 if (npc->pIndexData->vnum != MOB_VNUM_WOLF)
	    continue;
	 if (is_same_group( npc, ch ))
      enticed_wolves++;
   }

  if (!IS_NPC(ch))
	{  //wolfs = ch->pcdata->learned[sn]/25;
  
	if (ch->pcdata->learned[sn]>0 && ch->pcdata->learned[sn]<=99)
		{
			wolfs = 1;
		}
	if (ch->pcdata->learned[sn]>=100 && ch->pcdata->learned[sn]<=199)
		{
			wolfs = 2;
		}
	if (ch->pcdata->learned[sn]>=200 && ch->pcdata->learned[sn]<=299)
		{
			wolfs = 3;
		}
	if (ch->pcdata->learned[sn]>=300 && ch->pcdata->learned[sn]<=302)
		{
			wolfs = 4;
		}
	}
  /* Can only have skill/25 wolves at any time */
  if (enticed_wolves > 0) {
   if (enticed_wolves >= wolfs) {
    send_to_char("You are unable to entice any more wolves to assist you.\n\r", ch);
    return;
   }
   else {
     wolfs = wolfs - enticed_wolves; 
   }
  }

  argument = one_argument(argument, arg);

  if (!IS_NULLSTR(arg)) {
    if (!(wanted_wolves = atoi(arg))) {
	 send_to_char("Syntax: entice <amount of wolves> [<wolf appearance>]\n\r", ch);
	 return;
    }
    
    if (wanted_wolves <= 0 || wanted_wolves > wolfs) {
	 sprintf(buf, "You can only entice a max of %d wolves\n\r", wolfs);
	 send_to_char(buf, ch);
	 return;
    }
    
    wolfs = wanted_wolves;
  }
  
  if (!IS_NULLSTR(argument) && ch->pcdata->learned[sn] < 95) {
      send_to_char("You are not skilled enough to set wolf appearance yet.\n\r", ch);
      return;	
  }

  if (ch->endurance < endurance*wolfs) {
    send_to_char("You are too tired to concentrate or don't have enough endurance to focus!\n\r", ch);
    return;
  }
  
  if (!IS_NPC(ch)) {
    if (number_percent() > (ch->pcdata->learned[sn]/2)) {
	 send_to_char("You start to send images of your location out to the wolves, but fails.\n\r", ch);
	 ch->endurance -= (endurance*wolfs)/2;
	 return;
    }
  }
  
  if (wolfs <= 0) {
    send_to_char("You start to send images of your location out to the wolves, but you are not stroung enough yet to entice any wolfs.\n\r", ch);
    ch->endurance -= (endurance*wolfs)/2;
    return;
  }
  
  ch->endurance -= endurance*wolfs;;

  send_to_char("You start to send images of your location out to the wolves that are within reach.\n\r", ch);

  WAIT_STATE( ch, skill_table[sn].beats );

  sprintf(buf, "%s %s arrive and start to circle around you.", numbername[wolfs], wolfs > 1 ? "wolves" : "wolf");
  act(buf, ch, NULL, NULL, TO_CHAR);
  sprintf(buf, "%s %s arrive and start to circle around $n.", numbername[wolfs], wolfs > 1 ? "wolves" : "wolf");
  act(buf, ch, NULL, NULL, TO_ROOM);

  for (i = 1; i <= wolfs; i++) {    
    pet = create_mobile(get_mob_index(MOB_VNUM_WOLF));
    pet = update_wolf(ch, pet, wolfs, enticed_wolves, argument);
    pet->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
    char_to_room( pet, ch->in_room );
    if (!IS_NPC(ch) && ch->pcdata->df_level >= 0 )
    {
       sprintf(buf, "$N growls at $n and starts to attack.");
       act(buf, ch, NULL, pet, TO_ROOM);
       damage(pet,ch,number_range(2,2 + 2 * pet->size),gsn_bash, DAM_BASH,FALSE);
    }
    else
    {
    	SET_BIT(pet->affected_by, AFF_CHARM);
    	add_follower( pet, ch );
    	pet->leader = ch;
    }
  }  
}

void do_dismiss (  CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *npc;    
  CHAR_DATA *npc_next;
  char buf[MAX_INPUT_LENGTH];
  int sn               = 0;
  int enticed_wolves   = 0;
  int dismiss_wolves   = 0;
  int dismissed_wolves = 0;

  if (!IS_WOLFKIN(ch) && IS_SET(ch->talents, TALENT_WOLFKIN)) {
    send_to_char("You are too young to be able to communicate with other wolves in such a way.\n\r", ch);
    return;
  }
  
  if (!IS_WOLFKIN(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  sn = skill_lookup("entice");
  
  if (!IS_NPC(ch) && ch->pcdata->learned[sn] < 1) {
    send_to_char("You don't know how to entice yet.\n\r", ch);
    return;
  }

  /* Find out how many wolves are enticed            */
  for ( npc = char_list; npc != NULL; npc = npc_next ) {
    npc_next	= npc->next;
    if ( npc->in_room == NULL )
	 continue;
    if (!IS_NPC(npc))
	 continue;
    if (npc->pIndexData->vnum != MOB_VNUM_WOLF)
	 continue;
    if (is_same_group( npc, ch ))
      enticed_wolves++;
  }

  if (!IS_NULLSTR(argument)) {
    if (!(dismiss_wolves = atoi(argument))) {
	 send_to_char("Syntax: entice <amount of wolves>\n\r", ch);
	 return;
    }

    if (dismiss_wolves <= 0) {
	 send_to_char("You must give a number higher then 0.\n\r", ch);
	 return;
    }

    if (dismiss_wolves > enticed_wolves) {
	 sprintf(buf, "You have %d wolves enticed that could be dismissed.\n\r", enticed_wolves);
	 send_to_char(buf, ch);
	 return;
    }
    
    /* dismiss amount of wolves */
    for ( npc = char_list; npc != NULL; npc = npc_next ) {
	 npc_next	= npc->next;
	 if ( npc->in_room == NULL )
	   continue;
	 if (!IS_NPC(npc))
	   continue;
	 if (npc->pIndexData->vnum != MOB_VNUM_WOLF)
	   continue;
	 if (dismissed_wolves >= dismiss_wolves)
	   continue;
	 if (is_same_group( npc, ch )) {
	   dismissed_wolves++;
	   act("You look at $N for a moment and dismiss it from your entice.", ch, NULL, npc, TO_CHAR);		
	   stop_follower( npc );
	   act("$N leaves quietly.", ch, NULL, npc, TO_ROOM);
	   act("$N leaves quietly.", ch, NULL, npc, TO_CHAR);
	   extract_char(npc,TRUE,FALSE);
	 }
    }
  }
  else {
    send_to_char("Syntax: dismiss <amount of wolves>\n\r", ch);
    return;
  }

}

void do_wkemote( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_INPUT_LENGTH]; 
   CHAR_DATA *vch;    
   CHAR_DATA *vch_next;
   
/*   
   
   if (!IS_WOLFKIN(ch) && IS_SET(ch->talents, TALENT_WOLFKIN)) {
      send_to_char("You are too young to express your wolfkin emotions!\n\r", ch);
      return;
   }

   if (!IS_WOLFKIN(ch)) {
      send_to_char("Huh?\n\r", ch);
      return;
   }
  
*/  
   
   if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
      send_to_char( "You can't show your wolfkin emotions.\n\r", ch );
      return;
   }
 
   if ( argument[0] == '\0' ) {
      send_to_char( "Wkemote what?\n\r", ch );
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

  MOBtrigger = FALSE;

  act("({Ywkemote{x) $n $T", ch, NULL, emote_parse(ch,argument), TO_CHAR);
  reward_rp (ch);
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if ( vch->in_room == NULL )
	    continue;
    if (ch == vch)
	    continue;
	 if (!IS_WOLFKIN(vch))
	    continue;
    if ( vch->in_room == ch->in_room ) {
      sprintf(buf, "({Ywkemote{x) $n %s", emote_parse(ch,argument));
      act( buf, ch, NULL, vch, TO_VICT );
    }
  }

  MOBtrigger = TRUE;
  return;
}

void do_wkpemote( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_INPUT_LENGTH]; 
   CHAR_DATA *vch;    
   CHAR_DATA *vch_next;	
    
    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) ) {
        send_to_char( "You can't show your wolfkin emotions.\n\r", ch );
        return;
    }
 
    if ( argument[0] == '\0' ) {
        send_to_char( "wkpemote what?\n\r", ch );
        return;
    }

   /* If wrapped */
   if (IS_AFFECTED(ch, AFF_WRAPPED)) {
	  send_to_char("{WAn unseen force prevents you from moving!{x\n\r", ch);
	  act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
	  return;
   }
 
  MOBtrigger = FALSE;
  act( "({Ywkpemote{x) $n's $T", ch, NULL, emote_parse(ch,argument), TO_CHAR );
  reward_rp (ch);  
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if ( vch->in_room == NULL )
	    continue;
    if (ch == vch)
	    continue;
	 if (!IS_WOLFKIN(vch))
	    continue;
    if ( vch->in_room == ch->in_room ) {
      sprintf(buf, "({Ywkpemote{x) $n's %s", emote_parse(ch,argument));
      act( buf, ch, NULL, vch, TO_VICT );
    }
  }         
    MOBtrigger = TRUE;
    return;
}

void do_wolfappearance(CHAR_DATA *ch, char *argument )
{  
  
  char buf[MAX_STRING_LENGTH];
  char wolf_app[MAX_STRING_LENGTH];
  
  /* Only PCs */
  if (IS_NPC(ch))
    return;
  
  /* Is WK ? */
  if (!IS_WOLFKIN(ch)) {
    send_to_char("Huh?\n\r", ch); 
    return;
  }
  
  if ( argument[0] == '\0' ) {
    send_to_char("Please spesify a wolf appearance for your self.\n\r", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && !IS_NULLSTR(ch->pcdata->wolf_appearance)) {
    send_to_char("You already have set a wolf appearance for your self.\n\r", ch);
    send_to_char("If you need to change your wolf appearance, see one of the immortals for assistance.\n\r", ch);
    return;
  }
  
  if (strstr(argument, "wolf") == NULL) {
    send_to_char("You need to have the keyword '{Wwolf{x' in your wolf appearance.\n\r", ch);
    return;
  }
  
  if (strlen(argument) > 35 || strlen(argument) < 5) {
    send_to_char("Wolf appearance must be between 5 and 35 characters long (no colors allowed).\n\r", ch);
    return;
  }
    
  free_string( ch->pcdata->wolf_appearance);
  smash_tilde( argument );
  sprintf(wolf_app, "%s", colorstrem(argument));
  ch->pcdata->wolf_appearance = str_dup( wolf_app);

  sprintf(buf, "You set your wolf appearance to: %s\n\r", ch->pcdata->wolf_appearance);
  send_to_char(buf, ch);

  sprintf(buf, "%s set wolf appearance to: %s", ch->name, ch->pcdata->wolf_appearance);
  wiznet(buf,ch,NULL,WIZ_ON,WIZ_SECURE,0);

  return;
}

void do_wolfdescription( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

  /* Only PCs */
  if (IS_NPC(ch))
    return;
  
  /* Is WK ? */
  if (!IS_WOLFKIN(ch)) {
    send_to_char("Huh?\n\r", ch); 
    return;
  }

    if ( argument[0] != '\0' ) {
      buf[0] = '\0';
      smash_tilde( argument );

      if (!str_cmp(argument, "write")) {
	string_append(ch, &ch->wolf_description);
	return;
      }
      
      if (argument[0] == '-') {
	int len;
	bool found = FALSE;
	
	if (ch->wolf_description == NULL || ch->wolf_description[0] == '\0') {
	  send_to_char("No lines left to remove.\n\r",ch);
	  return;
	}
	
	strcpy(buf,ch->wolf_description);
	
	for (len = strlen(buf); len > 0; len--) {
	  if (buf[len] == '\r') {
	    if (!found)  { /* back it up */
	      if (len > 0)
		len--;
	      found = TRUE;
	    }
	    else { /* found the second one */
	      buf[len + 1] = '\0';
	      free_string(ch->wolf_description);
	      ch->wolf_description = str_dup(buf);
	      send_to_char( "Your wolf description is:\n\r", ch );
	      send_to_char( ch->wolf_description ? ch->wolf_description : 
			    "(None).\n\r", ch );
	      return;
	    }
	  }
	}
	buf[0] = '\0';
	free_string(ch->wolf_description);
	ch->wolf_description = str_dup(buf);
	send_to_char("Wolf description cleared.\n\r",ch);
	return;
      }
      if ( argument[0] == '+' ) {
	if ( ch->wolf_description != NULL )
	  strcat( buf, ch->wolf_description );
	argument++;
	while ( isspace(*argument) )
	  argument++;
      }
      
      if ( strlen(buf) >= 1024) {
	send_to_char( "Wolf description too long.\n\r", ch );
	return;
      }
      
      strcat( buf, argument );
      strcat( buf, "\n\r" );
      free_string( ch->wolf_description );
      ch->wolf_description = str_dup( buf );
    }
    
    send_to_char( "Your wolf description is:\n\r", ch );
    send_to_char( ch->wolf_description ? ch->wolf_description : "(None).\n\r", ch );
    return;
}

/* Turn a player into wolf shape or back if toggled */
/* Used when a wolfkin enter TAR only!              */
void do_wolfshape( CHAR_DATA *ch, char *argument )
{
  if (IS_SET(ch->app,APP_WOLFSHAPE)) {
    if (IS_CODER(ch))
	 send_to_char("[ {YCoder {x]: Return to human shape.\n\r", ch);
    REMOVE_BIT(ch->app,APP_WOLFSHAPE);
  }
  else {
    if (IS_CODER(ch))
	 send_to_char("[ {YCoder {x]: Turn into Wolf shape.\n\r", ch);
    SET_BIT(ch->app,APP_WOLFSHAPE);
  }
}

void do_wkrename( CHAR_DATA *ch, char *argument)
{
  char old_name[MAX_INPUT_LENGTH];
  char new_name[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];

  CHAR_DATA* victim;

  argument = one_argument(argument, old_name); /* find new/old name */
  one_argument (argument, new_name);

  /* Trivial checks */
  if (!old_name[0]) {
    send_to_char ("Rename who?\r\n",ch);
    return;
  }

  victim = get_char_world (ch, old_name);

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

  free_string( victim->wkname);
  victim->wkname = str_dup( new_name );

  sprintf(buf, "You will be known as '%s' amoung your fellow wolfkin.\n\r", victim->wkname);
  send_to_char(buf, victim);
  sprintf(buf, "They will be known as '%s' amoung their fellow wolfkin.\n\r", victim->wkname);
  send_to_char(buf, ch);

  return;
}

void do_wolfform(CHAR_DATA *ch, char * argument) {

	if (IS_NPC(ch)) {
        	send_to_char("Not on a mob.\r\n",ch);
		return;
	}
	if (!IS_WOLFKIN(ch)) {
		send_to_char("You are not a wolfkin.\r\n",ch);
		return;
	}

	if (get_skill(ch,skill_lookup("wolfdream")) < 100) {
		send_to_char("You do not have the control to switch between forms yet.\r\n",ch);
		return;
	}

	TOGGLE_BIT(ch->act2, PLR2_WOLFFORM);
	if (IS_SET(ch->act2, PLR2_WOLFFORM)) {
		send_to_char("You will now show up in wolf form in Tel'aran'rhiod.\r\n",ch);
        }
	else {
		send_to_char("You will now show up in human form in Tel'aran'rhiod.\r\n",ch);
	}
	return;
}
