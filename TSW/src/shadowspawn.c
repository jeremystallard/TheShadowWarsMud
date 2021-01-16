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
 * This file include code special dedicated to Shadowspawn                 *
 * TSW is copyright -2003 Swordfish and Zandor                             *
 **************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "magic.h"

#define FADE_NAME_FILE        "fadename.dat"

/**********************************************************************
*       Function      : is_fade_granted()
*       Author        : Swordfish
*       Description   : lookup in data file is name is a fade grant
*       Parameters    : 
*       Returns       : 
**********************************************************************/
bool is_fade_granted(CHAR_DATA *ch) 
{
  FILE *fp;
  char filename[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];

  sprintf(filename, "%s%s", DATA_DIR, FADE_NAME_FILE);
  
  if ((fp = fopen(filename, "r")) == NULL) {
    return FALSE;
  }
  
  while (fgets(buf, MAX_STRING_LENGTH, fp) != NULL) {
    
    // If line feed, discard and continue
    if (buf[0] == 0x0d || buf[0] == 0x0a)
	 continue;

    // If comment - handy if temporary disable a fade
    if (buf[0] == '#')
	 continue;    

    buf[strlen(buf)-1] = 0x00;

    if (!str_cmp(buf, ch->name)) {
	 fclose(fp);
	 return TRUE;
    }
  }
  
  fclose(fp);
  return FALSE;
}

/**********************************************************************
*       Function      : do_shadowtravel
*       Author        : Swordfish
*       Description   : Allow a fade to travel within the shadows
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_shadowtravel ( CHAR_DATA *ch, char *argument )
{
  int sn = 0;
  int endurance = 0;
  CHAR_DATA       *victim=NULL; // victim
  ROOM_INDEX_DATA *tlocation;   // To location
  AFFECT_DATA af;
  char toRoomKey[MAX_STRING_LENGTH];
  //char buf[MAX_STRING_LENGTH];

  /* Only Fades or Immortals */
  if (ch->race != race_lookup("fade") && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  sn = skill_lookup("shadowtravel");
  endurance = skill_table[sn].min_endurance;

  if (!IS_NPC(ch) && ch->pcdata->learned[sn] < 1) {
    send_to_char("You don't know how to shadowtravel yet.\n\r", ch);
    return;
  }

  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: shadowtravel <to room-key>\n\r", ch);
    if (IS_SPECIAL(ch))
    	send_to_char("        shadowtravel <to player>\n\r", ch);
    return;
  }

  if (ch->endurance < endurance) {
    send_to_char("You are too tired to concentrate and are unable to slip into the shadows.\n\r", ch);
    return;
  }

  /* To Player */
  if (IS_SPECIAL(ch))
  {
  	if ((victim = get_char_world(ch, argument)) != NULL) {
   	 
    		if (victim == ch) {
	 		send_to_char("You can't shadowtravel to your self!\n\r", ch);
	 		return;
    		}
  
    		if (IS_NPC(victim)) {
      			send_to_char("You can't shadowtravel to mobile!\n\r", ch);
	  		return;
    		}
    		if (!IS_RP(victim) && !IS_IMMORTAL(ch))
    		{
			send_to_char("They are not in RP Mode at the moment.\n\r",ch);
			return;
    		}

    		if (victim->in_room == ch->in_room || is_ward_set(ch->in_room, WARD_LIGHT) || is_ward_set(victim->in_room, WARD_LIGHT)) 
    		{
	 		send_to_char("You can't shadowtravel there right now.\n\r", ch);
	 		return;
    		}
		
    		if (IS_IMMORTAL(victim)) {
	 		send_to_char("You can't shadowtravel to immortals!\n\r", ch);
	 		wiznet("$N tried to shadowtravel to an immortal.",ch ,NULL,WIZ_PENALTIES,0,get_trust(ch));
	 		return;
    		}
    
    		send_to_char("\n{DYou slip into the shadows and {rtravel{D to your destination.{x\n\n\r", ch);
    		check_improve(ch, sn ,TRUE,3);

    		char_from_room(ch);
    		char_to_room(ch,victim->in_room);

    		if (!IS_AFFECTED(ch, AFF_HIDE)) {
	 		af.where     = TO_AFFECTS;
	 		af.casterId  = ch->id;
	 		af.type      = gsn_hide;
	 		if (!IS_NPC(ch)) {
	   			af.level     = ch->pcdata->learned[sn] + ch->level/2;
	   			af.duration  = ch->pcdata->learned[sn] + ch->level/2;
	 		}
	 		else {
	   			af.level     = ch->level;
	   			af.duration  = ch->level;
	 		}
	 		af.location  = APPLY_NONE;
	 		af.modifier  = 0;
	 		af.bitvector = AFF_HIDE;
	 		affect_to_char( ch, &af );
    		}
    		ch->endurance -= endurance;
    		do_function(ch, &do_look, "auto");
    		return;
  	}
   }

  /* If not to player, to room */
  sprintf(toRoomKey, "%d", key2vnum(ch, argument));
  if ((tlocation = find_location(ch, toRoomKey)) == NULL) {
    send_to_char("No such location.\n\r", ch);
    return;
  }

  if (tlocation == ch->in_room) {
    send_to_char("You are standing right here.\n\r", ch);
    return;
  }

  if (!is_gate_room(ch, tlocation)) {
    send_to_char("The shadows seems to be of no help there.\n\r", ch);
    return;
  }
  
  if (is_ward_set(ch->in_room, WARD_LIGHT) || is_ward_set(tlocation, WARD_LIGHT))
  {
       send_to_char("You can't shadowtravel there right now.\n\r", ch);
       return;
  }


  send_to_char("\n{DYou slip into the shadows and {rtravel{D to your destination.{x\n\n\r", ch);
  check_improve(ch, sn ,TRUE,3);

  char_from_room(ch);
  char_to_room(ch,tlocation);

  if (!IS_AFFECTED(ch, AFF_HIDE)) {
    af.where     = TO_AFFECTS;
    af.casterId  = ch->id;
    af.type      = gsn_hide;
    if (!IS_NPC(ch)) {
	 af.level     = ch->pcdata->learned[sn] + ch->level/2;
	 af.duration  = ch->pcdata->learned[sn] + ch->level/2;
    }
    else {
	 af.level     = ch->level;
	 af.duration  = ch->level;
    }
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_HIDE;
    affect_to_char( ch, &af );
  }

  ch->endurance -= endurance;
  
  do_function(ch, &do_look, "auto");
  
  return;
}

bool gaze(CHAR_DATA *ch, CHAR_DATA *victim)
{
  ROOM_INDEX_DATA *was_in;
  ROOM_INDEX_DATA *now_in;
  int attempt;
  
  was_in = victim->in_room;
  
  for ( attempt = 0; attempt < ch->level/10; attempt++ ) {
    EXIT_DATA *pexit;
    int door;
    
    door = number_door( );
    if ( ( pexit = was_in->exit[door] ) == 0
	    ||   pexit->u1.to_room == NULL
	    ||   IS_SET(pexit->exit_info, EX_CLOSED)
	    ||   number_range(0,victim->daze) != 0
	    || ( IS_NPC(victim)
		    &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
	 continue;
    
    
    act( "You turn your deadly gaze upon $N", ch, NULL, victim, TO_CHAR);
    act( "$n turn its deadly gaze upon you!", ch, NULL, victim, TO_VICT);
    move_char( victim, door, FALSE );
    
    if ( ( now_in = victim->in_room ) == was_in )
	 continue;
    
    victim->in_room = was_in;
    act( "$n has fled!", victim, NULL, NULL, TO_ROOM );
    victim->in_room = now_in;
    
    WAIT_STATE(victim, 5 * PULSE_VIOLENCE);
    
    return TRUE;
  }
  
  return FALSE;
}

/**********************************************************************
*       Function      : do_gaze
*       Author        : Swordfish
*       Description   : Allow a fade to turn it's scary gaze upon a foe
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_gaze ( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  
  /* Only Fades or Immortals */
  if (ch->race != race_lookup("fade") && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: gaze <character>\n\r", ch);
    return;
  }
  

  if ((victim = get_char_room(ch, argument)) != NULL) {

    if (victim == ch) {
	 send_to_char("You can't turn your gaze on your self!\n\r", ch);
	 return;
    }

    if (IS_IMMORTAL(victim)) {
	 send_to_char("You turned your gaze the wrong way!\n\r", ch);
	 act("Mr. Smith appears and kick the shit out of you!",  ch, NULL, victim, TO_CHAR );
	 act("Mr. Smith appears and kick the shit out of $n", ch, NULL, victim, TO_ROOM );
	 ch->hit		= -10;
	 ch->endurance	= -10;
	 raw_kill( ch );
    }
    
    if (IS_AFFECTED(victim, AFF_BLIND) || IS_AFFECTED(victim, AFF_BLINDFOLDED)) {
    	act("You turn your eyeless gaze toward $N but $E seems to be blinded.", ch, NULL, victim, TO_CHAR);
    	return;
    }

    if (!gaze(ch, victim)) {
	 // Msg's
	 act("You turn your eyeless gaze toward $N but $E grits $S teeth and remains unaffected.", ch, NULL, victim, TO_CHAR);
	 act("$n stares through your soul with $s eyeless gaze but your experience and confidence takes control of your fear.", ch, NULL, victim, TO_VICT);
    }
  }
  else 
    send_to_char( "They aren't here.\n\r", ch );
  
  return;
}

/**********************************************************************
*       Function      : do_taintblade
*       Author        : Swordfish
*       Description   : A fade can tain their blades
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_taintblade ( CHAR_DATA *ch, char *argument )
{
  /* TBD */
}
