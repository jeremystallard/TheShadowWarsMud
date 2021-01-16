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
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "tables.h"

char *	const	dir_name	[]		=
{
/*  "north", "east", "south", "west", "up", "down" */
  "north", "east", "south", "west", "up", "down",
  "enter (n)", "enter (e)", "enter (s)", "enter (w)"
};

char *	const	enter_name	[]		=
{
  "enter (n)", "enter (e)", "enter (s)", "enter (w)"
};

const	sh_int	rev_dir		[]		=
{
/*    2, 3, 0, 1, 5, 4 */
  2, 3, 0, 1, 5, 4, 6, 7, 8, 9
};

const	sh_int	movement_loss	[SECT_MAX]	=
{
    1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6,
    2, /* SECT_WAYS          */
    6, /* SECT_ROCK_MOUNTAIN */
    10,/* SECT_SNOW_MOUNTAIN */
    0, /* SECT_ROAD          */
    2, /* SECT_ENTER         */
    6, /* SECT_SWAMP         */
    6, /* SECT_JUNGLE        */
    6, /* SECT_RUINS         */
   40, /* SECT_OCEAN         */
   10, /* SECT_RIVER         */
    6, /* SECT_SAND          */
   12, /* SECT_BLIGHT        */
    6, /* SECT_ISLAND        */
    6 /* SECT_LAKE          */         
};        
          
          
          
/*        
 * Local functions.
 */    
int	 find_door	args( ( CHAR_DATA *ch, char *arg ) );
bool	 has_key		args( ( CHAR_DATA *ch, int key ) );
void do_drag_obj (CHAR_DATA * ch, char * argument);
void do_drag_char (CHAR_DATA * ch, char * argument);
void drag_char( CHAR_DATA *ch, CHAR_DATA *victim, int door, bool follow );

void do_wolfshape( CHAR_DATA *ch, char *argument );

bool mount_success ( CHAR_DATA *ch, CHAR_DATA *mount, int canattack)
{
  int percent=0;
  int success=0;

  percent = number_percent() + (ch->level < mount->level ? 
            (mount->level - ch->level) * 3 : 
            (mount->level - ch->level) * 2);

  if (!ch->fighting)
    percent -= 15;
    
  if (ch->race == race_lookup("trolloc"))
    percent += 50;

  if (!IS_NPC(ch) && IS_DRUNK(ch)) {
    percent += get_skill(ch,gsn_riding) / 2;
    send_to_char("Due to your being under the influence, riding seems a bit harder...\n\r", ch);
  }

  success = percent - get_skill(ch,gsn_riding);

  if( success <= 0 ) { /* Success */
    check_improve(ch, gsn_riding, TRUE, 1);
    return TRUE;
  } else {
    check_improve(ch, gsn_riding, FALSE, 1);
    if ( success >= 10 && MOUNTED(ch) == mount) {
      act("You lose control and fall off of $N.", ch, NULL, mount, TO_CHAR);
      act("$n loses control and falls off of $N.", ch, NULL, mount, TO_ROOM);
      act("$n loses control and falls off of you.", ch, NULL, mount, TO_VICT);

      ch->riding = FALSE;
      mount->riding = FALSE;
      if (ch->position > POS_STUNNED) 
		ch->position=POS_SITTING;
    
    /*  if (ch->hit > 2) { */
        ch->hit -= 5;
        update_pos(ch);
      
    }
    if ( success >= 40 && canattack) {
      act("$N doesn't like the way you've been treating $M.", ch, NULL, mount, TO_CHAR);
      act("$N doesn't like the way $n has been treating $M.", ch, NULL, mount, TO_ROOM);
      act("You don't like the way $n has been treating you.", ch, NULL, mount, TO_VICT);

      act("$N snarls and attacks you!", ch, NULL, mount, TO_CHAR);
      act("$N snarls and attacks $n!", ch, NULL, mount, TO_ROOM);
      act("You snarl and attack $n!", ch, NULL, mount, TO_VICT);  

      damage(mount, ch, number_range(1, mount->level), gsn_kick,DAM_BASH,TRUE );

/*      multi_hit( mount, ch, TYPE_UNDEFINED ); */
    }
  }
  return FALSE;
}


bool check_obj_dodge( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int bonus )
{
    int chance;

    if ( !IS_AWAKE(victim) || MOUNTED(victim) )
	return FALSE;

    if ( IS_NPC(victim) )
         chance  = UMIN( 10, victim->level ); 
    else 
	{
          chance  = get_skill(victim,gsn_dodge) / 5;
          
    	  /* chance for high dex. */
          if (get_curr_stat(victim,STAT_DEX) > 20 )
          {
             chance += 2 * (get_curr_stat(victim,STAT_DEX) - 20);
          }

	 }

    chance -= (bonus);
    chance /= 2;
    if ( number_percent( ) >= chance )
        return FALSE;

    if (IS_NPC(victim))
    {
     act("You catch $p that had been shot to you.",ch,obj,victim,TO_VICT);
     act("$N catches $p that had been shot to $M.",ch,obj,victim,TO_CHAR);
     act("$n catches $p that had been shot to $m.",victim,obj,ch,TO_NOTVICT);
     obj_to_char(obj,victim);
     return TRUE;
    }
    else
    {
     act("You dodge $p that had been shot at you.",ch,obj,victim,TO_VICT);
     act("$N dodges $p that had been shot at $M.",ch,obj,victim,TO_CHAR);
     act("$n dodges $p that had been shot at $m.",victim,obj,ch,TO_NOTVICT);
     obj_to_room(obj,victim->in_room);
     check_improve(victim,gsn_dodge,TRUE,6);
    }
    return TRUE;
}

void move_char( CHAR_DATA *ch, int door, bool follow )
{
  CHAR_DATA *gguard=NULL;
  CHAR_DATA *fch;
  CHAR_DATA *fch_next;
  CHAR_DATA *mount;
  ROOM_INDEX_DATA *in_room;
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit_rev=NULL;
  EXIT_DATA *pexit;
  char buf[MSL];
  int chance=0;
  int percent_chance;

  CHAR_DATA *rch;  
  bool close_door = FALSE; /* Used in automatic open/close unlocked doors when you walk */

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\n\r", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* Is blocking an exit? */
  if (IS_BLOCKING(ch)) {
    sprintf(buf, "You are still trying to block the %s entrance.\n\r", dir_name[ch->exit_block.direction]);
    send_to_char(buf, ch);
    return;
  }
  
  if ((RIDDEN(ch) )&& (!IS_NPC(ch->mount)) && 
	(ch->mount->position >= POS_STANDING) &&
	(ch->mount->in_room == ch->in_room))  {
    move_char(ch->mount,door,follow);
    return;
  }
  
  if ( door < 0 || door > 9 ) {
    bug( "Do_move: bad door %d.", door );
    return;
  }
  
  if (IS_AFFECTED(ch, AFF_INVISIBLE) && !IS_SET(ch->talents,TALENT_ILLUSION))
  {
	affect_strip(ch, gsn_invis);
	REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
	send_to_char( "You step out from behind the invisibility weave.\r\n",ch);
        act( "$n steps out from where they were hiding.", ch, NULL, NULL, TO_ROOM);
  }
  /* if ( IS_AFFECTED( ch, AFF_HIDE ) && !IS_AFFECTED(ch, AFF_SNEAK)) { */
  if ( IS_AFFECTED( ch, AFF_HIDE )) {
    affect_strip ( ch, gsn_hide );
    REMOVE_BIT(ch->affected_by, AFF_HIDE);
    send_to_char( "You step out of the shadows.\n\r", ch );
    act( "$n steps out of the shadows.", ch, NULL, NULL, TO_ROOM);
  }

  if (ch->study) {
    ch->study = FALSE;
    ch->study_pulse = -1;
  }

  // Assassinate time is set on char if know assassinate
  if (!IS_NPC(ch) && ch->pcdata->learned[gsn_assassinate] > 0) {
  	ch->pcdata->next_assassinate = current_time + 15;
  }

  // Wrap timer is set on PCs (channeler) if they know wrap
  if (!IS_NPC(ch) && ch->class == CLASS_CHANNELER)
    ch->pcdata->next_wrap = current_time + 15;

  // Masquerade check update
  if (IS_MASQUERADED(ch) && !can_use_masquerade(ch)) {
    send_to_char("{mYou have left the masquerade area and remove the mask from your face{x.\n\r", ch);
    REMOVE_BIT(ch->app,APP_MASQUERADE);
  }
  
  /*
   * Exit trigger, if activated, bail out. Only PCs are triggered.
   */
  if ( !IS_NPC(ch) && mp_exit_trigger( ch, door ) )
    return;
  
  in_room = ch->in_room;
  if ( ( pexit   = in_room->exit[door] ) == NULL
	  ||   ( to_room = pexit->u1.to_room   ) == NULL 
	  ||	 !can_see_room(ch,pexit->u1.to_room)) {
    send_to_char( "Alas, you cannot go that way.\n\r", ch );
    return;
  }

  /* Fire wall? */
  if (IS_SET(pexit->exit_info, EX_FIREWALL) && !IS_GHOLAM(ch)) {
    if (number_chance(5)) {
	 act("You run into the {rwall of fire{x and take the pain as the {Rfire{x surrounds you!!!", ch, NULL, NULL, TO_CHAR);
	 act("$n runs into the {rwall of fire{x and take the pain as the {Rfire{x surrounds $m!!!", ch, NULL, NULL, TO_ROOM);
	 damage(ch, ch, UMAX(300, ch->hit/2), gsn_wof, DAM_FIRE, FALSE);
    }
    else {
	 act("You walk into a {rwall of fire{x and jump back as the {Rfire{x surrounds you!", ch, NULL, NULL, TO_CHAR);
	 act("$n walks into the {rwall of fire{x and jump back as the {Rfire{x surrounds $m!", ch, NULL, NULL, TO_ROOM);
	 damage(ch, ch, UMAX(100, ch->hit/2), gsn_wof, DAM_FIRE, FALSE);	   
	 return;
    }
  }

  /* Air wall? */
  if (IS_SET(pexit->exit_info, EX_AIRWALL) && !IS_GHOLAM(ch)) {
    act("You start to walk $T, but you are blocked by a {Cblue haze{x floating in the {Wair{x.", ch, NULL, door < 6 ? dir_name[door] : "out", TO_CHAR);
    act("$n starts to walk $T, but $e is blocked by a {Cblue haze{x floating in the {Wair{x.", ch, NULL, door < 6 ? dir_name[door] : "out", TO_ROOM);
    return;
  }

  /* Is the door barred? */
  if (IS_SET(pexit->exit_info, EX_BARRED) && !IS_GHOLAM(ch))
  {
	send_to_char("The door appears to be barred\n\r",ch);
	return;
  }

  /* Direction blocked by someone? */
  if (IS_SET(pexit->exit_info, EX_BLOCKED)) {
    fch = get_blocker(ch->in_room, door);
    if (fch != NULL) {
       if (!IS_GHOLAM(ch))
       {
          sprintf(buf, "$N blocks you from entering the %s entrance.", door < 6 ? dir_name[door] : "out");
          act(buf, ch, NULL, fch, TO_CHAR);
          sprintf(buf, "$N blocks $n from entering the %s entrance.", door < 6 ? dir_name[door] : "out");
          act(buf, ch, NULL, fch, TO_NOTVICT);
          act("You block $n from entering the room.", ch, NULL, fch, TO_VICT);
          return;
	}
	else
	{
	   sprintf(buf, "You slide past $N as he tries to block the %s entrance.",door < 6 ? dir_name[door] : "out");
           act(buf, ch, NULL, fch, TO_CHAR);
           sprintf(buf, "$n slips past $N's attempt to block $m from entering the %s entrance.", door < 6 ? dir_name[door] : "out");
          act(buf, ch, NULL, fch, TO_NOTVICT);
          act(buf, ch, NULL, fch, TO_NOTVICT);
          act("$n slips past you and enters the room.", ch, NULL, fch, TO_VICT);
	}
    }
  }

  // Is there a guild guard here?
  for (gguard = ch->in_room->people; gguard; gguard = gguard->next_in_room ) {
    if (IS_NPC(gguard) && 
	   gguard->guild_guard && 
	   gguard->guild_guard != ch->clan && 
	   (IS_SET(gguard->guild_guard_flags, GGUARD_BLOCK) || 
	    IS_SET(gguard->guild_guard_flags, GGUARD_REPORT_GUILD) ||
	    (IS_SET(gguard->guild_guard_flags, GGUARD_SHADOWSPAWN) && IS_SHADOWSPAWN(ch)) || 
	    (IS_SET(gguard->guild_guard_flags, GGUARD_RACE) && gguard->race != ch->race))) {
	 break;
    }
  }
  
  // Guild guard!?
  if (gguard != NULL && !IS_NPC(ch) && IS_RP(ch) && IS_SAME_WORLD(gguard, ch)) {

    // If in the group of someone that is int he guild, it void the gguard
    if (!is_in_guild_group(gguard, ch)) {

	 // BLOCK
	 if (IS_SET(gguard->guild_guard_flags, GGUARD_BLOCK) && !IS_GHOLAM(ch)) {
	   act("$N block you from leaving the room!", ch, NULL, gguard, TO_CHAR);
	   act("$N blocks $n from leaving the room!", ch, NULL, gguard, TO_NOTVICT);
	   act("$N says to you, '{7You don't belong here! Please wait until someone come and escort you.{x'", ch, NULL, gguard, TO_CHAR);
	   act("$N says to $n, '{7You don't belong here! Please wait until someone come and escort you.{x'", ch, NULL, gguard, TO_NOTVICT);
	   
	   // Report to guild channel?
	   if (IS_SET(gguard->guild_guard_flags, GGUARD_REPORT_GUILD)) {
		REMOVE_BIT(gguard->comm,COMM_NOCLAN);
		REMOVE_BIT(gguard->comm,COMM_NOCHANNELS);
		gguard->clan = gguard->guild_guard;
		free_string( gguard->gtitle );
		gguard->gtitle = str_dup("Guild Guard");
		sprintf(buf, "Intruder found at %s! Please come escort them out of here.", gguard->in_room->name);
		do_guildtalk(gguard, buf);
		gguard->clan = 0;
	   }
	   return;
	 }
	 // SS REPORT
	 else if (IS_SET(gguard->guild_guard_flags, GGUARD_REPORT_GUILD) && 
			IS_SET(gguard->guild_guard_flags, GGUARD_SHADOWSPAWN) &&
			IS_SHADOWSPAWN(ch)) {	
	   REMOVE_BIT(gguard->comm,COMM_NOCLAN);
	   REMOVE_BIT(gguard->comm,COMM_NOCHANNELS);
	   gguard->clan = gguard->guild_guard;
	   free_string( gguard->gtitle );
	   gguard->gtitle = str_dup("Guild Guard");
	   sprintf(buf, "Shadowspawn seen at %s! Alarm! Alarm!", gguard->in_room->name);
	   do_guildtalk(gguard, buf);
	   gguard->clan = 0;
	 }
	 
	 // RACE REPORT
	 else if (IS_SET(gguard->guild_guard_flags, GGUARD_REPORT_GUILD) && 
			IS_SET(gguard->guild_guard_flags, GGUARD_RACE) &&
			gguard->race != ch->race) {	
	   REMOVE_BIT(gguard->comm,COMM_NOCLAN);
	   REMOVE_BIT(gguard->comm,COMM_NOCHANNELS);
	   gguard->clan = gguard->guild_guard;
	   free_string( gguard->gtitle );
	   gguard->gtitle = str_dup("Guild Guard");
	   sprintf(buf, "%s seen at %s!", capitalize(race_table[ch->race].name), gguard->in_room->name);
	   do_guildtalk(gguard, buf);
	   gguard->clan = 0;
	 }
	 else if (IS_SET(gguard->guild_guard_flags, GGUARD_REPORT_GUILD)) {
	   REMOVE_BIT(gguard->comm,COMM_NOCLAN);
	   REMOVE_BIT(gguard->comm,COMM_NOCHANNELS);
	   gguard->clan = gguard->guild_guard;
	   free_string( gguard->gtitle );
	   gguard->gtitle = str_dup("Guild Guard");
	   sprintf(buf, "A stranger seen at %s!", gguard->in_room->name);
	   do_guildtalk(gguard, buf);
	   gguard->clan = 0;
	 }
    }
  }
  
  if (!follow) {
    if (IS_SET(pexit->exit_info, EX_CLOSED)
	   &&  (!IS_AFFECTED(ch, AFF_PASS_DOOR) || IS_SET(pexit->exit_info,EX_NOPASS))
	   &&   !IS_TRUSTED(ch,ANGEL)) {
	 if (IS_SET(pexit->exit_info, EX_LOCKED) && !IS_GHOLAM(ch)) {
	   act( "You bump into the $d with your head and realize it must be locked.", ch, NULL, pexit->keyword, TO_CHAR);
	   return;
	 }
	 else if (!IS_SET(pexit->exit_info,EX_LOCKED)){
	   close_door = TRUE;
	   REMOVE_BIT(pexit->exit_info, EX_CLOSED);
	   act( "$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	   act( "You open the $d.", ch, NULL, pexit->keyword, TO_CHAR);
	   
	   /* open the other side */
	   if ( ( to_room   = pexit->u1.to_room            ) != NULL
		   &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
		   &&   pexit_rev->u1.to_room == ch->in_room ) {
		
		REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
		for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
		  act( "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
	   }
	 }
	else if(IS_GHOLAM(ch))
	{
	   act( "$n slides under the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	   act( "You slide under the $d.", ch, NULL, pexit->keyword, TO_CHAR);
	}
    }
  }

  if ( IS_AFFECTED(ch, AFF_CHARM)
	  &&   ch->master != NULL
	  &&   in_room == ch->master->in_room 
	  &&   !IS_NPC(ch)) {
    send_to_char( "What?  And leave your beloved master?\n\r", ch );
    return;
  }
  
  if ((get_trust(ch) < MAX_LEVEL) && (!IS_GHOLAM(ch))) {
    if ( !is_room_owner(ch,to_room) && room_is_private( to_room ) ) {
	 send_to_char( "That room is private right now.\n\r", ch );
	 return;
    }
  } 
  
  if (MOUNTED(ch))  {
    if (MOUNTED(ch)->position < POS_FIGHTING)  {
	 send_to_char("Your mount must be standing.\n\r", ch);
	 return; 
    }
    if (!mount_success(ch, MOUNTED(ch), FALSE)) {
	 send_to_char("Your mount stubbornly refuses to go that way.\n\r", ch);
	 return;
    }
  }
  
  if ( !IS_NPC(ch) ) {
    //int iClass, iGuild;
    int endurance;
    
    /*
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ ) {
	 for ( iGuild = 0; iGuild < MAX_GUILD; iGuild ++)	{
	   if ( iClass != ch->class
		   &&   to_room->vnum == class_table[iClass].guild[iGuild] ) {
		send_to_char( "You aren't allowed in there.\n\r", ch );
		return;
	   }
	 }
    }
    */

    if ( in_room->sector_type == SECT_AIR
	    ||   to_room->sector_type == SECT_AIR ) {
	 if ( MOUNTED(ch) ) {
	   if( !IS_AFFECTED(MOUNTED(ch), AFF_FLYING) )  {
		send_to_char( "Your mount can't fly.\n\r", ch );
		return;
	   }
	 } 
	 else if ( !IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch)) {
	   send_to_char( "You can't fly.\n\r", ch );
	   return;
	 }
    }
    
    if (( in_room->sector_type == SECT_WATER_NOSWIM
		||    to_room->sector_type == SECT_WATER_NOSWIM )
	   &&    (MOUNTED(ch) && !IS_AFFECTED(MOUNTED(ch),AFF_FLYING)) ) {
	 send_to_char("You can't take your mount there.\n\r",ch);
	 return; 
    }  
    
    if (( in_room->sector_type == SECT_WATER_NOSWIM
		||    to_room->sector_type == SECT_WATER_NOSWIM 
		||    to_room->sector_type == SECT_LAKE
		||    to_room->sector_type == SECT_OCEAN
		||    to_room->sector_type == SECT_RIVER )
	   &&    (!MOUNTED(ch) && !IS_AFFECTED(ch,AFF_FLYING)) ) {
	 OBJ_DATA *obj;
	 bool found;
	 
	 /*
	  * Look for a boat.
	  */
	 found = FALSE;
	 
	 if (IS_IMMORTAL(ch))
	   found = TRUE;
	 
	 for ( obj = ch->carrying; obj != NULL; obj = obj->next_content ) {
	   if ( obj->item_type == ITEM_BOAT ) {
		found = TRUE;
		break;
	   }
	 }
	 if ( !found ) {
	   send_to_char( "You need a boat to go there.\n\r", ch );
	   return;
	 }
    }
    
    endurance = movement_loss[UMIN(SECT_MAX-1, in_room->sector_type)]
	 + movement_loss[UMIN(SECT_MAX-1, to_room->sector_type)]
	 ;

    if (IS_AIEL(ch))
	 endurance /= 2;
    
    if (!IS_SET(ch->merits, MERIT_HUGESIZE))
	 endurance /= 2;  /* i.e. the average */
    
    /* conditional effects */
    if (IS_AFFECTED(ch,AFF_FLYING) || IS_AFFECTED(ch,AFF_HASTE))
	 endurance /= 2;
    
    if (IS_AFFECTED(ch,AFF_SLOW))
	 endurance *= 2;
    
    if ( !MOUNTED(ch) && ch->endurance < endurance  && ch->position != POS_FIGHTING) {
	 send_to_char( "You are too exhausted.\n\r", ch );
	 return;
    }
    
    WAIT_STATE( ch, 1 );
    if (!MOUNTED(ch))	
	 ch->endurance -= endurance;
  }

  if ( !IS_AFFECTED(ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO) {
    if (MOUNTED(ch))
	 sprintf(buf,"$n leaves $T, riding on %s.",MOUNTED(ch)->short_descr );    
    else 
	 sprintf(buf,"$n leaves $T." );
    
    act( buf, ch, NULL, door < 6 ? dir_name[door] : "out", TO_ROOM );
  }	
  else  {
    if (ch->invis_level < LEVEL_HERO)  {
      if (MOUNTED(ch))
	   sprintf(buf,"$n sneaks %s, riding on %s.",door < 6 ? dir_name[door] : "out", MOUNTED(ch)->short_descr );    
      else 
	   sprintf(buf,"$n sneaks %s.", door < 6 ? dir_name[door] : "out" );
	 
      percent_chance = number_percent();
      in_room = ch->in_room;
      for ( fch = in_room->people; fch != NULL; fch = fch_next ) {
	   fch_next = fch->next_in_room;
	   
	   chance = 0;
	   chance = get_skill(fch,gsn_alertness) + (get_skill(fch,gsn_observation)/2);
	   chance -= number_percent() / 2;
	   chance -= get_skill(ch,gsn_sneak);
	   
	   if (IS_SET(ch->merits, MERIT_STEALTHY)) {
		chance -= 20;
	   }
	   
	   if (percent_chance < chance) {
		//act( buf, ch, NULL, door < 6 ? dir_name[door] : "out", TO_ROOM );
		act( buf, ch, NULL, fch, TO_VICT );
		check_improve(fch,gsn_alertness,TRUE,1);
		check_improve(fch,gsn_observation,TRUE,1);
	   }
	   else {
		check_improve(fch,gsn_alertness,FALSE,1);
		check_improve(fch,gsn_observation,FALSE,1);
	   }
      }
    }
  }
  
  mount = MOUNTED(ch);
  char_from_room( ch );
  char_to_room( ch, to_room );

  for ( fch = in_room->people; fch != NULL; fch = fch_next ) {
    fch_next = fch->next_in_room;
    if (in_room == to_room) 
       continue;
    if (!IS_SAME_WORLD(fch, ch))
	 continue;
    
    if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM) 
	    &&   fch->position < POS_STANDING)
	 do_function(fch, &do_stand, "");
   
    if ( fch->master == ch && fch->position == POS_STANDING 
	    &&   can_see_room(fch,to_room)) {
	 
	 if (IS_SET(ch->in_room->room_flags,ROOM_LAW)
		&&  (IS_NPC(fch) && IS_SET(fch->act,ACT_AGGRESSIVE))) {
	   act("You can't bring $N into the city.",
		  ch,NULL,fch,TO_CHAR);
	   act("You aren't allowed in the city.",
		  fch,NULL,NULL,TO_CHAR);
	   continue;
	 }	 
	 act( "You follow $N.", fch, NULL, ch, TO_CHAR );
	 if (IS_NPC(fch))
	   move_char( fch, door, TRUE );
	 else {
	   if (fch->desc != NULL)
		move_char( fch, door, TRUE );
	 }
   }
  }
  if ( !IS_AFFECTED(ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO) {
    if (mount)
	 act( "$n has arrived, riding $N.", ch, NULL, mount,TO_ROOM );
    else act( "$n has arrived.", ch, NULL, NULL, TO_ROOM );
  }
  else {
    if (ch->invis_level < LEVEL_HERO)  {
      int percent_chance = number_percent();
      in_room = ch->in_room;
      for ( fch = in_room->people; fch != NULL; fch = fch_next ) {
	   fch_next = fch->next_in_room;
	   chance = 0;
	   chance = get_skill(fch,gsn_alertness) + (get_skill(fch,gsn_observation)/2);
	   chance -= number_percent() / 2;
	   chance -= get_skill(ch,gsn_sneak);

	   if (IS_SET(ch->merits, MERIT_STEALTHY)) {
		chance -= 20;	
	   }
	   
	   if (percent_chance < chance) {
		act( "$n sneaks in.",ch,NULL,fch,TO_VICT);
		check_improve(fch,gsn_alertness,TRUE,1);
		check_improve(fch,gsn_observation,TRUE,1);
	   }
	   else {
		check_improve(fch,gsn_alertness,FALSE,1);
		check_improve(fch,gsn_observation,FALSE,1);
	   }
      }
    }
  }
  
  do_function(ch, &do_look, "auto" );
  
  // Recall timer
  if (!IS_NPC(ch)) {
  	ch->pcdata->next_recall = current_time + 60;
  }

  // Room warded against SS?
  if (IS_SHADOWSPAWN(ch) && is_ward_set(ch->in_room, WARD_SHADOWSPAWN)) {
    WARD_DATA *pWard;
    for (pWard = ch->in_room->wards; pWard != NULL; pWard = pWard->next) {
	 fch=NULL;
	 fch = get_charId_world(ch, pWard->casterId);
	 
	 if (fch != NULL) {
	   if (ch->in_room != fch->in_room) {
		send_to_char("A bird chippers and breaks the silence for a few moments.\n\r", fch);
		act("A bird chippers and breaks the silence for a few moments.", fch, NULL, NULL, TO_ROOM);
	   }
	   act("A bird chippers and breaks the silence for a few moments.", ch, NULL, NULL, TO_CHAR);	   
	   act("A bird chippers and breaks the silence for a few moments.", ch, NULL, NULL, TO_ROOM);	   
	 }
    }
  }

  /* IF STEDDING */
  if (IS_SET(ch->in_room->room_flags,ROOM_STEDDING) && IS_AFFECTED(ch, AFF_CHANNELING)) {
    send_to_char("\n{CYou feel a sudden chill as you walk into this room, then the True Source is no longer in reach.{x\n\r", ch);
    REMOVE_BIT(ch->affected_by, AFF_CHANNELING);
    ch->holding = 0;
    release_sustained_weaves(ch);
  }

  /* 
   * If someone is following the char, these triggers get activated
   * for the followers before the char, but it's safer this way...
   */
  if ( IS_NPC( ch ) && HAS_TRIGGER( ch, TRIG_ENTRY ) )
    mp_percent_trigger( ch, NULL, NULL, NULL, TRIG_ENTRY );
  if ( !IS_NPC( ch ) )
    mp_greet_trigger( ch );

  if (mount) {
    char_from_room( mount );
    char_to_room( mount, to_room);
    ch->riding = TRUE;
    mount->riding = TRUE;
  }

  /* If automatic open/close door and not already closed */
  if (close_door == TRUE && !follow && (pexit_rev != NULL)) {
    if (!IS_SET(pexit_rev->exit_info, EX_CLOSED)) {
	 close_door = FALSE;
	 SET_BIT(pexit_rev->exit_info, EX_CLOSED);
	 act( "$n closes the $d.", ch, NULL, pexit_rev->keyword, TO_ROOM );
	 act( "You close the $d.", ch, NULL, pexit_rev->keyword, TO_CHAR );
	 
	 /* close the other side */	 
	 SET_BIT( pexit->exit_info, EX_CLOSED );
	 for ( rch = in_room->people; rch != NULL; rch = rch->next_in_room )
	   act( "The $d closes.", rch, NULL, pexit->keyword, TO_CHAR );
    }
  }
  
  return;
}


void move_char_OLD( CHAR_DATA *ch, int door, bool follow )
{
  CHAR_DATA *fch;
  CHAR_DATA *fch_next;
  CHAR_DATA *mount;
  ROOM_INDEX_DATA *in_room;
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit;
  char buf[MAX_STRING_LENGTH];
  
  if (RIDDEN(ch) && !IS_NPC(ch->mount))  {
    move_char(ch->mount,door,follow);
    return;
  }
   
  if ( door < 0 || door > 9 ) {
    bug( "Do_move: bad door %d.", door );
    return;
  }

  if ( IS_AFFECTED( ch, AFF_HIDE )) { /* && !IS_AFFECTED(ch, AFF_SNEAK)) { */
    affect_strip ( ch, gsn_hide );
    REMOVE_BIT(ch->affected_by, AFF_HIDE);
    send_to_char( "You step out of the shadows.\n\r", ch );
    act( "$n steps out of the shadows.", ch, NULL, NULL, TO_ROOM);
  }

  if (ch->study) {
    ch->study = FALSE;
    ch->study_pulse = -1;
  }

    /*
     * Exit trigger, if activated, bail out. Only PCs are triggered.
     */
    if ( !IS_NPC(ch) && mp_exit_trigger( ch, door ) )
	return;

    in_room = ch->in_room;
    if ( ( pexit   = in_room->exit[door] ) == NULL
    ||   ( to_room = pexit->u1.to_room   ) == NULL 
    ||	 !can_see_room(ch,pexit->u1.to_room))
    {
	send_to_char( "Alas, you cannot go that way.\n\r", ch );
	return;
    }
	
    if (IS_SET(pexit->exit_info, EX_CLOSED)
    &&  (!IS_AFFECTED(ch, AFF_PASS_DOOR) || IS_SET(pexit->exit_info,EX_NOPASS))
    &&   !IS_TRUSTED(ch,ANGEL))
    {
	act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
	return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM)
    &&   ch->master != NULL
    &&   in_room == ch->master->in_room )
    {
	send_to_char( "What?  And leave your beloved master?\n\r", ch );
	return;
    }
	
	if (get_trust(ch) < MAX_LEVEL)
    {
    if ( !is_room_owner(ch,to_room) && room_is_private( to_room ) )
    {
	send_to_char( "That room is private right now.\n\r", ch );
	return;
    }
	} 
	
	if (MOUNTED(ch)) 
    {
        if (MOUNTED(ch)->position < POS_FIGHTING) 
        {
         send_to_char("Your mount must be standing.\n\r", ch);
         return; 
        }
        if (!mount_success(ch, MOUNTED(ch), FALSE)) 
        {
         send_to_char("Your mount stubbornly refuses to go that way.\n\r", ch);
         return;
        }
    }

    if ( !IS_NPC(ch) )
    {
	int iClass, iGuild;
	int endurance;

	for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
	{
	    for ( iGuild = 0; iGuild < MAX_GUILD; iGuild ++)	
	    {
	    	if ( iClass != ch->class
	    	&&   to_room->vnum == class_table[iClass].guild[iGuild] )
	    	{
		    send_to_char( "You aren't allowed in there.\n\r", ch );
		    return;
		}
	    }
	}

	if ( in_room->sector_type == SECT_AIR
	||   to_room->sector_type == SECT_AIR )
	{
	 if ( MOUNTED(ch) ) 
            {
                if( !IS_AFFECTED(MOUNTED(ch), AFF_FLYING) ) 
                {
                    send_to_char( "Your mount can't fly.\n\r", ch );
                    return;
                }
            } 
     else if ( !IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
	    {
		send_to_char( "You can't fly.\n\r", ch );
		return;
	    }
	}

		if (( in_room->sector_type == SECT_WATER_NOSWIM
	||    to_room->sector_type == SECT_WATER_NOSWIM )
	&&    (MOUNTED(ch) && !IS_AFFECTED(MOUNTED(ch),AFF_FLYING)) )
	{
	    send_to_char("You can't take your mount there.\n\r",ch);
	    return; 
        }  

	if (( in_room->sector_type == SECT_WATER_NOSWIM
	||    to_room->sector_type == SECT_WATER_NOSWIM 
        ||    to_room->sector_type == SECT_LAKE
        ||    to_room->sector_type == SECT_OCEAN
        ||    to_room->sector_type == SECT_RIVER )
  	&&    (!MOUNTED(ch) && !IS_AFFECTED(ch,AFF_FLYING)) )
	{
	    OBJ_DATA *obj;
	    bool found;

	    /*
	     * Look for a boat.
	     */
	    found = FALSE;

	    if (IS_IMMORTAL(ch))
		found = TRUE;

	    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
	    {
		if ( obj->item_type == ITEM_BOAT )
		{
		    found = TRUE;
		    break;
		}
	    }
	    if ( !found )
	    {
		send_to_char( "You need a boat to go there.\n\r", ch );
		return;
	    }
	}

	endurance = movement_loss[UMIN(SECT_MAX-1, in_room->sector_type)]
	     + movement_loss[UMIN(SECT_MAX-1, to_room->sector_type)]
	     ;

        endurance /= 2;  /* i.e. the average */


	/* conditional effects */
	if (IS_AFFECTED(ch,AFF_FLYING) || IS_AFFECTED(ch,AFF_HASTE))
	    endurance /= 2;

	if (IS_AFFECTED(ch,AFF_SLOW))
	    endurance *= 2;

	if ( !MOUNTED(ch) && ch->endurance < endurance )
	{
	    send_to_char( "You are too exhausted.\n\r", ch );
	    return;
	}

	WAIT_STATE( ch, 1 );
	if (!MOUNTED(ch))	
		ch->endurance -= endurance;
    }

    if ( !IS_AFFECTED(ch, AFF_SNEAK)
    &&   ch->invis_level < LEVEL_HERO)
    {
	if (MOUNTED(ch))
		sprintf(buf,"$n leaves $T, riding on %s.",
		MOUNTED(ch)->short_descr );
  	
	else 
	    sprintf(buf,"$n leaves $T." );
	  
  	act( buf, ch, NULL, door < 6 ? dir_name[door] : "out", TO_ROOM );

   }	

    mount = MOUNTED(ch);
    char_from_room( ch );
    char_to_room( ch, to_room );
    if ( !IS_AFFECTED(ch, AFF_SNEAK)
    &&   ch->invis_level < LEVEL_HERO)
	 {
		if (mount)
		    act( "$n has arrived, riding $N.", ch, NULL, mount,TO_ROOM );
		else act( "$n has arrived.", ch, NULL, NULL, TO_ROOM );
     }

    do_function(ch, &do_look, "auto" );

/* IF STEDDING */
    if (IS_SET(ch->in_room->room_flags,ROOM_STEDDING) && IS_AFFECTED(ch, AFF_CHANNELING)) {
	 send_to_char("\n{CYou feel a sudden chill as you walk into this room, then the True Source is no longer in reach.{x\n\r", ch);
	 REMOVE_BIT(ch->affected_by, AFF_CHANNELING);
	 ch->holding = 0;
	 release_sustained_weaves(ch);
    }

	if (mount)
	{
	 char_from_room( mount );
	 char_to_room( mount, to_room);
  	 ch->riding = TRUE;
  	 mount->riding = TRUE;
	}
    if (in_room == to_room) /* no circular follows */
	return;

    for ( fch = in_room->people; fch != NULL; fch = fch_next )
    {
	fch_next = fch->next_in_room;

	if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM) 
	&&   fch->position < POS_STANDING)
	    do_function(fch, &do_stand, "");

	if ( fch->master == ch && fch->position == POS_STANDING 
	&&   can_see_room(fch,to_room))
	{

	    if (IS_SET(ch->in_room->room_flags,ROOM_LAW)
	    &&  (IS_NPC(fch) && IS_SET(fch->act,ACT_AGGRESSIVE)))
	    {
		act("You can't bring $N into the city.",
		    ch,NULL,fch,TO_CHAR);
		act("You aren't allowed in the city.",
		    fch,NULL,NULL,TO_CHAR);
		continue;
	    }

	    act( "You follow $N.", fch, NULL, ch, TO_CHAR );
	    move_char( fch, door, TRUE );
	}
    }

    /* 
     * If someone is following the char, these triggers get activated
     * for the followers before the char, but it's safer this way...
     */
    if ( IS_NPC( ch ) && HAS_TRIGGER( ch, TRIG_ENTRY ) )
	mp_percent_trigger( ch, NULL, NULL, NULL, TRIG_ENTRY );
    if ( !IS_NPC( ch ) )
    	mp_greet_trigger( ch );

    return;
}



/** 
 * Direction commands are now found in vehicle.c
 */
/*
void do_north( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_NORTH, FALSE );
    return;
}



void do_east( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_EAST, FALSE );
    return;
}



void do_south( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_SOUTH, FALSE );
    return;
}



void do_west( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_WEST, FALSE );
    return;
}



void do_up( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_UP, FALSE );
    return;
}



void do_down( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_DOWN, FALSE );
    return;
}
*/

int find_exit( CHAR_DATA *ch, char *arg )
{
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    int door = -1;

    if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) ) door = DIR_NORTH;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east"  ) ) door = DIR_EAST;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) ) door = DIR_SOUTH;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west"  ) ) door = DIR_WEST;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up"    ) ) door = DIR_UP;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down"  ) ) door = DIR_DOWN;
   else if  (!str_cmp (arg,"enter n") || !str_cmp(arg,"enter north"))
        door = ENTER_NORTH;
    else if (!str_cmp (arg,"enter e") || !str_cmp(arg,"enter east"))
        door = ENTER_EAST;
    else if (!str_cmp (arg,"enter s") || !str_cmp(arg,"enter south"))
        door = ENTER_SOUTH;
    else if (!str_cmp (arg,"enter w") || !str_cmp(arg,"enter west"))
        door = ENTER_WEST;
    else if (!str_cmp(arg,"leave"))
    {
      /********
       * Leav Areas linked to Vmap
       **********/
      if (ch->in_room->inside_of)
      {
         if (!IS_SET(ch->in_room->inside_of->in_room->room_flags, ROOM_VMAP))
         {
           in_room = ch->in_room->inside_of->in_room;
           for (door=6; door <= 9; door++)
           {
                if (( pexit = in_room->exit[door]) != NULL
                       && (to_room = pexit->u1.to_room) != NULL)
                {
                  break;
                }
           }
         }
      }
      else  //Not inside of something
      {

  	/********
   	* Leav Areas linked to Vmap
   	**********/
  	if (!IS_SET(ch->in_room->room_flags, ROOM_VMAP)) 
        {
    	   in_room = ch->in_room;
    	   for (door=6; door <= 9; door++) 
           {
              if (( pexit = in_room->exit[door]) == NULL
                || (to_room = pexit->u1.to_room) == NULL) 
              {
           	   continue;
              }
	      return door;
           }
         }
      }
     }
     else //Not leave
     {
        if (ch->in_room->inside_of)
        {
           for (door = 0; door <= 9; door++)
           {
             if ((pexit = ch->in_room->inside_of->in_room->exit[door]) != NULL
                 && IS_SET (pexit->exit_info, EX_ISDOOR)
                 && pexit->keyword != NULL
                 && is_name (arg, pexit->keyword))
             {
                 return door;
             }
	   }
        }
        else
		return door;
      }
      if (door >= 0)
	return door;
      else
      {
         act ("I see no exit $T here.", ch, NULL, arg, TO_CHAR);
         return -1;
      }
    }
    

int find_door( CHAR_DATA *ch, char *arg )
{

    int door =  find_hack_door(ch,arg);
 
    EXIT_DATA *pexit;
    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
	//act( "I see no door $T here.", ch, NULL, arg, TO_CHAR );
	return -1;
    }

    if ( !IS_SET(pexit->exit_info, EX_ISDOOR) )
    {
	//send_to_char( "You can't do that.\n\r", ch );
	return -1;
    }

    return door;
}



void do_open( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int door;

   one_argument( argument, arg );

   if ( arg[0] == '\0' ) {
	   send_to_char( "Open what?\n\r", ch );
	   return;
   }
   

   if ( ( door = find_door( ch, arg ) ) >= 0 ) {
	    /* 'open door' */
      ROOM_INDEX_DATA *to_room;
	   EXIT_DATA *pexit;
	   EXIT_DATA *pexit_rev;

	   pexit = ch->in_room->exit[door];
	   if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	      { send_to_char( "It's already open.\n\r",      ch ); return; }
	   if (  IS_SET(pexit->exit_info, EX_LOCKED) )
	      { send_to_char( "It's locked.\n\r",            ch ); return; }

  	   /* Is the door barred? */
  	   if (IS_SET(pexit->exit_info, EX_BARRED))
  	   {
		   send_to_char("It won't budge.\n\r",ch);
		   return;
  	   }
	   REMOVE_BIT(pexit->exit_info, EX_CLOSED);
	   act( "$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM );
           act( "You open the $d.", ch, NULL, pexit->keyword, TO_CHAR );

	   /* open the other side */
	   if ( ( to_room   = pexit->u1.to_room            ) != NULL
	   &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
	   &&   pexit_rev->u1.to_room == ch->in_room ) {
	      CHAR_DATA *rch;

	      REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
	      for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
		   act( "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
	   }
	   return;
   }  
   if (( obj = get_obj_here( ch, arg )) != NULL ) {

 	   /* open portal */
	   if (obj->item_type == ITEM_PORTAL || obj->item_type == ITEM_VEHICLE) {
	      if (!IS_SET(obj->value[1], EX_ISDOOR)) {
		      send_to_char("You can't do that.\n\r",ch);
		      return;
	      }

	      if (!IS_SET(obj->value[1], EX_CLOSED)) {
		      send_to_char("It's already open.\n\r",ch);
		      return;
	      }

	      if (IS_SET(obj->value[1], EX_LOCKED)) {
		      send_to_char("It's locked.\n\r",ch);
		      return;
	      }

	      REMOVE_BIT(obj->value[1], EX_CLOSED);
	      act("You open $p.",ch,obj,NULL,TO_CHAR);
	      act("$n opens $p.",ch,obj,NULL,TO_ROOM);
	      return;
 	   }  

	   /* 'open object' */
	   if ( obj->item_type != ITEM_CONTAINER )
	      { send_to_char( "You can't open that.\n\r", ch ); return; }
	   if ( !IS_SET(obj->value[1], CONT_CLOSED) )
	      { send_to_char( "It's already open.\n\r",      ch ); return; }
	   if ( !IS_SET(obj->value[1], CONT_CLOSEABLE) )
	      { send_to_char( "You can't do that.\n\r",      ch ); return; }
	   if ( IS_SET(obj->value[1], CONT_LOCKED) )
	      { send_to_char( "It's locked.\n\r",            ch ); return; }

	   REMOVE_BIT(obj->value[1], CONT_CLOSED);
	   act("You open $p.",ch,obj,NULL,TO_CHAR);
	   act( "$n opens $p.", ch, obj, NULL, TO_ROOM );
	   return;
   }
   if (ch->in_room->inside_of && !str_cmp(arg,"door"))
   {
	if (!IS_SET(ch->in_room->inside_of->value[1],EX_ISDOOR)
	    ||  IS_SET(ch->in_room->inside_of->value[1],EX_NOCLOSE))
        {
		send_to_char("You can't do that.\r\n",ch);
		return;
	}
	if (IS_SET(ch->in_room->inside_of->value[1],EX_LOCKED)) 
        {
		send_to_char("It's locked.\r\r", ch);
		return;
        }
	if (!IS_SET(ch->in_room->inside_of->value[1],EX_CLOSED)) 
        {
		send_to_char("It's already open.\r\r", ch);
		return;
        }
        REMOVE_BIT(ch->in_room->inside_of->value[1], EX_CLOSED);
	act("You open $p.",ch,ch->in_room->inside_of,NULL,TO_CHAR);
	act("$n opens $p.",ch,ch->in_room->inside_of,NULL,TO_ROOM);
	return;
   }
   act( "I see no $T here.", ch, NULL, arg, TO_CHAR );
}



void do_close( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int door;

   one_argument( argument, arg );

   if ( arg[0] == '\0' ) {
	   send_to_char( "Close what?\n\r", ch );
	   return;
   }

   
  if ( ( door = find_door( ch, arg ) ) >= 0 ) {

     /* 'close door' */
     ROOM_INDEX_DATA *to_room;
     EXIT_DATA *pexit;
     EXIT_DATA *pexit_rev;

     pexit	= ch->in_room->exit[door];
     if ( IS_SET(pexit->exit_info, EX_CLOSED) )
      { send_to_char( "It's already closed.\n\r",    ch ); return; }

     SET_BIT(pexit->exit_info, EX_CLOSED);
     act( "$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM );
     send_to_char( "Ok.\n\r", ch );

     /* close the other side */
     if ( ( to_room   = pexit->u1.to_room            ) != NULL
     &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
     &&   pexit_rev->u1.to_room == ch->in_room ) {
        CHAR_DATA *rch;

        SET_BIT( pexit_rev->exit_info, EX_CLOSED );
        for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
      act( "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );
     }
     return;
   }        
   if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
	   /* portal stuff */
	   if (obj->item_type == ITEM_PORTAL || obj->item_type == ITEM_VEHICLE) {
         if (!IS_SET(obj->value[1],EX_ISDOOR)
	      ||   IS_SET(obj->value[1],EX_NOCLOSE)) {
	         send_to_char("You can't do that.\n\r",ch);
		      return;
         }

	      if (IS_SET(obj->value[1],EX_CLOSED)) {
		      send_to_char("It's already closed.\n\r",ch);
		      return;
         }

	      SET_BIT(obj->value[1],EX_CLOSED);
	      act("You close $p.",ch,obj,NULL,TO_CHAR);
	      act("$n closes $p.",ch,obj,NULL,TO_ROOM);
	      return;
      }

	   /* 'close object' */
	   if ( obj->item_type != ITEM_CONTAINER ) { 
	      send_to_char( "That's not a container.\n\r", ch ); 
	      return; 
	   }
	   if ( IS_SET(obj->value[1], CONT_CLOSED) )
	      { send_to_char( "It's already closed.\n\r",    ch ); return; }
	   if ( !IS_SET(obj->value[1], CONT_CLOSEABLE) )
	      { send_to_char( "You can't do that.\n\r",      ch ); return; }

	   SET_BIT(obj->value[1], CONT_CLOSED);
	   act("You close $p.",ch,obj,NULL,TO_CHAR);
	   act( "$n closes $p.", ch, obj, NULL, TO_ROOM );
	   return;
   }
   if (ch->in_room->inside_of && !str_cmp(arg,"door"))
   {
	if (!IS_SET(ch->in_room->inside_of->value[1],EX_ISDOOR)
	    ||  IS_SET(ch->in_room->inside_of->value[1],EX_NOCLOSE))
        {
		send_to_char("You can't do that.\r\n",ch);
		return;
	}
        if (IS_SET(ch->in_room->inside_of->value[1],EX_CLOSED))
        {
                send_to_char("It's already closed.\r\r", ch);
                return;
        }
        SET_BIT(ch->in_room->inside_of->value[1], EX_CLOSED);
        act("You close $p.",ch,ch->in_room->inside_of,NULL,TO_CHAR);
        act("$n closes $p.",ch,ch->in_room->inside_of,NULL,TO_ROOM);
        return;
   }

   act( "I see no $T here.", ch, NULL, arg, TO_CHAR );

}



bool has_key( CHAR_DATA *ch, int key )
{
  OBJ_DATA *obj;
  OBJ_DATA *container_obj;
  
  for ( obj = ch->carrying; obj != NULL; obj = obj->next_content ) {
    if (obj->item_type == ITEM_CONTAINER) {
	 for (container_obj = obj->contains; container_obj != NULL; container_obj = container_obj->next_content) {
	   if ( container_obj->pIndexData->vnum == key )
		return TRUE;
	 }
    }
    else {
	 if ( obj->pIndexData->vnum == key )
	   return TRUE;
    }
  }
  
  return FALSE;
}



void do_lock( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Lock what?\n\r", ch );
	return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
	/* 'lock door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit	= ch->in_room->exit[door];
	if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	    { send_to_char( "It's not closed.\n\r",        ch ); return; }
	if ( pexit->key < 0 )
	    { send_to_char( "It can't be locked.\n\r",     ch ); return; }
	if ( !has_key( ch, pexit->key) )
	    { send_to_char( "You lack the key.\n\r",       ch ); return; }
	if ( IS_SET(pexit->exit_info, EX_LOCKED) )
	    { send_to_char( "It's already locked.\n\r",    ch ); return; }

	SET_BIT(pexit->exit_info, EX_LOCKED);
	send_to_char( "*Click*\n\r", ch );
	act( "$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM );

	/* lock the other side */
	if ( ( to_room   = pexit->u1.to_room            ) != NULL
	&&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
	&&   pexit_rev->u1.to_room == ch->in_room )
	{
	    SET_BIT( pexit_rev->exit_info, EX_LOCKED );
	}
	return;
    }

    obj = get_obj_here(ch,arg);
    if (!obj && !str_cmp(arg,"door") && ch->in_room->inside_of)
    {
	obj = ch->in_room->inside_of;
    }
    if  ( obj != NULL )
    {
	/* portal stuff */
	if (obj->item_type == ITEM_PORTAL || obj->item_type == ITEM_VEHICLE)
	{
	    if (!IS_SET(obj->value[1],EX_ISDOOR)
	    ||  IS_SET(obj->value[1],EX_NOCLOSE))
	    {
		send_to_char("You can't do that.\n\r",ch);
		return;
	    }
	    if (!IS_SET(obj->value[1],EX_CLOSED))
	    {
		send_to_char("It's not closed.\n\r",ch);
	 	return;
	    }

	    if (obj->value[4] < 0 || IS_SET(obj->value[1],EX_NOLOCK))
	    {
		send_to_char("It can't be locked.\n\r",ch);
		return;
	    }

	    if (!has_key(ch,obj->value[4]))
	    {
		send_to_char("You lack the key.\n\r",ch);
		return;
	    }

	    if (IS_SET(obj->value[1],EX_LOCKED))
	    {
		send_to_char("It's already locked.\n\r",ch);
		return;
	    }

	    SET_BIT(obj->value[1],EX_LOCKED);
	    act("You lock $p.",ch,obj,NULL,TO_CHAR);
	    act("$n locks $p.",ch,obj,NULL,TO_ROOM);
	    return;
	}

	/* 'lock object' */
	if ( obj->item_type != ITEM_CONTAINER )
	    { send_to_char( "That's not a container.\n\r", ch ); return; }
	if ( !IS_SET(obj->value[1], CONT_CLOSED) )
	    { send_to_char( "It's not closed.\n\r",        ch ); return; }
	if ( obj->value[2] < 0 )
	    { send_to_char( "It can't be locked.\n\r",     ch ); return; }
	if ( !has_key( ch, obj->value[2] ) )
	    { send_to_char( "You lack the key.\n\r",       ch ); return; }
	if ( IS_SET(obj->value[1], CONT_LOCKED) )
	    { send_to_char( "It's already locked.\n\r",    ch ); return; }

	SET_BIT(obj->value[1], CONT_LOCKED);
	act("You lock $p.",ch,obj,NULL,TO_CHAR);
	act( "$n locks $p.", ch, obj, NULL, TO_ROOM );
	return;
    }


    return;
}

void do_bar( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Bar what?\n\r", ch );
	return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
	/* 'lock door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit	= ch->in_room->exit[door];
	if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	    { send_to_char( "It's not closed.\n\r",        ch ); return; }
	if ( IS_SET(pexit->exit_info, EX_BARRED) )
	    { send_to_char( "It's already barred.\n\r",    ch ); return; }

        if (!IS_SET(ch->in_room->room_flags, ROOM_PRIVATE) && !is_room_owner(ch,ch->in_room))
        {
	   send_to_char("There is no bar on this side of the door.\n\r",ch);
	   return;
        }
	SET_BIT(pexit->exit_info, EX_BARRED);
	send_to_char( "*The bar slides into place*\n\r", ch );
	act( "$n bars the $d.", ch, NULL, pexit->keyword, TO_ROOM );

	/* bar the other side */
	if ( ( to_room   = pexit->u1.to_room            ) != NULL
	&&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
	&&   pexit_rev->u1.to_room == ch->in_room )
	{
	    SET_BIT( pexit_rev->exit_info, EX_BARRED );
	}
    }

    return;
}

void do_unbar( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Bar what?\n\r", ch );
	return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
	/* 'lock door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit	= ch->in_room->exit[door];
	if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	    { send_to_char( "It's not closed.\n\r",        ch ); return; }
	if ( !IS_SET(pexit->exit_info, EX_BARRED) )
	    { send_to_char( "It's not barred.\n\r",    ch ); return; }

/*
        if (!IS_SET(ch->in_room->room_flags, ROOM_PRIVATE))
        {
	   send_to_char("There is no bar on this side of the door.\n\r",ch);
	   return;
        }
*/
	REMOVE_BIT(pexit->exit_info, EX_BARRED);
	send_to_char( "*The bar slides out of place*\n\r", ch );
	act( "$n unbars the $d.", ch, NULL, pexit->keyword, TO_ROOM );

	/* bar the other side */
	if ( ( to_room   = pexit->u1.to_room            ) != NULL
	&&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
	&&   pexit_rev->u1.to_room == ch->in_room )
	{
	    REMOVE_BIT( pexit_rev->exit_info, EX_BARRED );
	}
    }

    return;
}

void do_unlock( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  int door;
  
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Unlock what?\n\r", ch );
    return;
  }

  if ( ( door = find_door( ch, arg ) ) >= 0 ) {
    /* 'unlock door' */
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    EXIT_DATA *pexit_rev;
    
    pexit = ch->in_room->exit[door];
    if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	 { send_to_char( "It's not closed.\n\r",        ch ); return; }
    if ( pexit->key < 0 )
	 { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
    if ( !has_key( ch, pexit->key) )
	 { send_to_char( "You lack the key.\n\r",       ch ); return; }
    if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
	 { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
    
    REMOVE_BIT(pexit->exit_info, EX_LOCKED);
    send_to_char( "*Click*\n\r", ch );
    act( "$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
    
    /* unlock the other side */
    if ( ( to_room   = pexit->u1.to_room            ) != NULL
	    &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
	    &&   pexit_rev->u1.to_room == ch->in_room ) {
	 REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
    }
    return;
  }
  
  obj = get_obj_here( ch, arg );
  if (!obj && ch->in_room->inside_of && !str_cmp(arg,"door"))
  {
	obj = ch->in_room->inside_of;
  }
  if ( obj != NULL ) {
    /* portal stuff */
    if (obj->item_type == ITEM_PORTAL || obj->item_type == ITEM_VEHICLE) {

	 if (!IS_SET(obj->value[1],EX_ISDOOR)) {
	   send_to_char("You can't do that.\n\r",ch);
	   return;
	 }
	 
	 if (!IS_SET(obj->value[1],EX_CLOSED)) {
	   send_to_char("It's not closed.\n\r",ch);
	   return;
	 }
	 
	 if (obj->value[4] < 0) {
	   send_to_char("It can't be unlocked.\n\r",ch);
	   return;
	 }
	 
	 if (!has_key(ch,obj->value[4])) {
	   send_to_char("You lack the key.\n\r",ch);
	   return;
	 }

	 if (!IS_SET(obj->value[1],EX_LOCKED)) {
	   send_to_char("It's already unlocked.\n\r",ch);
	   return;
	 }

	 REMOVE_BIT(obj->value[1],EX_LOCKED);
	 act("You unlock $p.",ch,obj,NULL,TO_CHAR);
	 act("$n unlocks $p.",ch,obj,NULL,TO_ROOM);
	 return;
    }
    
    /* 'unlock object' */
    if ( obj->item_type != ITEM_CONTAINER )
	 { send_to_char( "That's not a container.\n\r", ch ); return; }
    if ( !IS_SET(obj->value[1], CONT_CLOSED) )
	 { send_to_char( "It's not closed.\n\r",        ch ); return; }
    if ( obj->value[2] < 0 )
	 { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
    if ( !has_key( ch, obj->value[2] ) )
	 { send_to_char( "You lack the key.\n\r",       ch ); return; }
    if ( !IS_SET(obj->value[1], CONT_LOCKED) )
	 { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
    
    REMOVE_BIT(obj->value[1], CONT_LOCKED);
    act("You unlock $p.",ch,obj,NULL,TO_CHAR);
    act( "$n unlocks $p.", ch, obj, NULL, TO_ROOM );
    return;
  }
  
  return;
}



void do_pick( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Pick what?\n\r", ch );
	return;
    }

	if (get_skill(ch,gsn_pick_lock)<=0)
	{
	send_to_char( "You don't know how to do that.\n\r", ch );
	return;
	}
	
	if (MOUNTED(ch)) 
	{
    send_to_char("You can't pick locks while mounted.\n\r", ch);
     return;
	}

    WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );

    /* look for guards */
    for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
    {
	if ( IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level )
	{
	    act( "$N is standing too close to the lock.",
		ch, NULL, gch, TO_CHAR );
	    return;
	}
    }

    
    if ( !IS_NPC(ch) && number_percent( ) > get_skill(ch,gsn_pick_lock))
    {
	send_to_char( "You failed.\n\r", ch);
	check_improve(ch,gsn_pick_lock,FALSE,2);
	return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
	/* portal stuff */
	if (obj->item_type == ITEM_PORTAL || obj->item_type == ITEM_VEHICLE)
	{
	    if (!IS_SET(obj->value[1],EX_ISDOOR))
	    {	
		send_to_char("You can't do that.\n\r",ch);
		return;
	    }

	    if (!IS_SET(obj->value[1],EX_CLOSED))
	    {
		send_to_char("It's not closed.\n\r",ch);
		return;
	    }

	    if (obj->value[4] < 0)
	    {
		send_to_char("It can't be unlocked.\n\r",ch);
		return;
	    }

	    if (IS_SET(obj->value[1],EX_PICKPROOF))
	    {
		send_to_char("This lock is too complex.\n\r",ch);
		return;
	    }

	    if (IS_SET(obj->value[1],EX_BARRED))
	    {
		send_to_char("There is more to opening the door than the lock.\n\r",ch);
		return;
	    }

    	    int modifier = 0;
            if (IS_SET(obj->value[1], EX_EASY))
	    {
		modifier = 0;
	    }
            if (IS_SET(obj->value[1], EX_HARD))
	    {
		modifier = 50;
	    }
            if (IS_SET(obj->value[1], EX_INFURIATING))
	    {
		modifier = 90;
	    }
    	    if (number_percent( ) > (get_skill(ch,gsn_pick_lock) - modifier))
            {
		send_to_char("You failed.\n\r",ch);
		check_improve(ch,gsn_pick_lock,FALSE,2);
		return;
	    }
	  
	    REMOVE_BIT(obj->value[1],EX_LOCKED);
	    act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR);
	    act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM);
	    check_improve(ch,gsn_pick_lock,TRUE,2);
	    return;
	}

	    


	
	/* 'pick object' */
	if ( obj->item_type != ITEM_CONTAINER )
	    { send_to_char( "That's not a container.\n\r", ch ); return; }
	if ( !IS_SET(obj->value[1], CONT_CLOSED) )
	    { send_to_char( "It's not closed.\n\r",        ch ); return; }
	if ( obj->value[2] < 0 )
	    { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
	if ( !IS_SET(obj->value[1], CONT_LOCKED) )
	    { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
	if ( IS_SET(obj->value[1], CONT_PICKPROOF) )
	    { send_to_char( "This lock is too complex.\n\r",             ch ); return; }

	REMOVE_BIT(obj->value[1], CONT_LOCKED);
        act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR);
        act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM);
	check_improve(ch,gsn_pick_lock,TRUE,2);
	return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
	/* 'pick door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit = ch->in_room->exit[door];
	if ( !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
	    { send_to_char( "It's not closed.\n\r",        ch ); return; }
	if ( pexit->key < 0 && !IS_IMMORTAL(ch))
	    { send_to_char( "It can't be picked.\n\r",     ch ); return; }
	if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
	    { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
	if ( IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
	    { send_to_char( "This lock is too complex.\n\r",  ch ); return; }
	if ( IS_SET(pexit->exit_info, EX_BARRED) && !IS_IMMORTAL(ch))
	    { send_to_char( "You failed.\n\r",             ch ); return; }


        int modifier = 0;
        if (IS_SET(pexit->exit_info, EX_EASY))
        {
            modifier = 0;
        }
        if (IS_SET(pexit->exit_info, EX_HARD))
        {
            modifier = 50;
        }
        if (IS_SET(pexit->exit_info, EX_INFURIATING))
        {
            modifier = 90;
        }
        if (number_percent( ) > (get_skill(ch,gsn_pick_lock) - modifier))
        {
            send_to_char("You failed.\n\r",ch);
            return;
        }

	REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	send_to_char( "*Click*\n\r", ch );
	act( "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	check_improve(ch,gsn_pick_lock,TRUE,2);

	/* pick the other side */
	if ( ( to_room   = pexit->u1.to_room            ) != NULL
	&&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
	&&   pexit_rev->u1.to_room == ch->in_room )
	{
	    REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
	}
    }

    return;
}




void do_stand( CHAR_DATA *ch, char *argument )
{
  OBJ_DATA *obj = NULL;
  
  if (argument[0] != '\0') {
    if (ch->position == POS_FIGHTING) {
	 send_to_char("Maybe you should finish fighting first?\n\r",ch);
	 return;
    }

    //obj = get_obj_list(ch,argument,ch->in_room->contents);
    obj = get_obj_here(ch,argument);
    if (obj == NULL) {
	 send_to_char("You don't see that here.\n\r",ch);
	 return;
    }

    if (obj->item_type != ITEM_FURNITURE
	   ||  (!IS_SET(obj->value[2],STAND_AT)
		   &&   !IS_SET(obj->value[2],STAND_ON)
		   &&   !IS_SET(obj->value[2],STAND_IN))) {
	 send_to_char("You can't seem to find a place to stand.\n\r",ch);
	 return;
    }

    if (ch->on != obj && count_users(obj) >= obj->value[0]) {
	 act_new("There's no room to stand on $p.",
		    ch,obj,NULL,TO_CHAR,POS_DEAD);
	 return;
    }
    ch->on = obj;
  }
  
  switch ( ch->position ) {
  case POS_SLEEPING:
    if ( IS_AFFECTED(ch, AFF_SLEEP) || IS_AFFECTED(ch,AFF_SAP) ) { 
	 send_to_char( "You can't wake up!\n\r", ch ); 
	 return; 
    }
    
    if (obj == NULL) {
	 send_to_char( "You wake and stand up.\n\r", ch );
	 act( "$n wakes and stands up.", ch, NULL, NULL, TO_ROOM );
	 ch->on = NULL;
    }
    else if (IS_SET(obj->value[2],STAND_AT)) {
	 act_new("You wake and stand at $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
	 act("$n wakes and stands at $p.",ch,obj,NULL,TO_ROOM);
    }
    else if (IS_SET(obj->value[2],STAND_ON)) {
	 act_new("You wake and stand on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
	 act("$n wakes and stands on $p.",ch,obj,NULL,TO_ROOM);
    }
    else  {
	 act_new("You wake and stand in $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
	 act("$n wakes and stands in $p.",ch,obj,NULL,TO_ROOM);
    }
    ch->position = POS_STANDING;
    do_function(ch, &do_look, "auto");
    break;
    
  case POS_RESTING: 
  case POS_SITTING:
    if (obj == NULL) {
	 send_to_char( "You stand up.\n\r", ch );
	 act( "$n stands up.", ch, NULL, NULL, TO_ROOM );
	 ch->on = NULL;
    }
    else if (IS_SET(obj->value[2],STAND_AT)) {
	 act("You stand at $p.",ch,obj,NULL,TO_CHAR);
	 act("$n stands at $p.",ch,obj,NULL,TO_ROOM);
    }
    else if (IS_SET(obj->value[2],STAND_ON)) {
	 act("You stand on $p.",ch,obj,NULL,TO_CHAR);
	 act("$n stands on $p.",ch,obj,NULL,TO_ROOM);
    }
    else {
	 act("You stand in $p.",ch,obj,NULL,TO_CHAR);
	 act("$n stands on $p.",ch,obj,NULL,TO_ROOM);
    }
    ch->position = POS_STANDING;
    break;
    
  case POS_STANDING:
    send_to_char( "You are already standing.\n\r", ch );
    break;
    
  case POS_FIGHTING:
    send_to_char( "You are already fighting!\n\r", ch );
    break;
  }
  
  return;
}



void do_rest( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj = NULL;

    if (ch->position == POS_FIGHTING)
    {
	send_to_char("You are already fighting!\n\r",ch);
	return;
    }
	if (MOUNTED(ch)) 
    {
        send_to_char("You can't rest while mounted.\n\r", ch);
        return;
    }
    if (RIDDEN(ch)) 
    {
        send_to_char("You can't rest while being ridden.\n\r", ch);
        return;
    }
	if ( IS_AFFECTED(ch, AFF_SLEEP) )
    { 
		send_to_char( "You're sleeping already.\n\r", ch ); 
		return; 
	}

	/* okay, now that we know we can rest, find an object to rest on */
    if (argument[0] != '\0')
    {
	//obj = get_obj_list(ch,argument,ch->in_room->contents);
        obj = get_obj_here(ch,argument);
	if (obj == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }
    else obj = ch->on;

    if (obj != NULL)
    {
        if (obj->item_type != ITEM_FURNITURE
    	||  (!IS_SET(obj->value[2],REST_ON)
    	&&   !IS_SET(obj->value[2],REST_IN)
    	&&   !IS_SET(obj->value[2],REST_AT)))
    	{
	    send_to_char("You can't rest on that.\n\r",ch);
	    return;
    	}

        if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
        {
	    act_new("There's no more room on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
	    return;
    	}
	
	ch->on = obj;
    }

    switch ( ch->position )
    {
    case POS_SLEEPING:
	if ( IS_AFFECTED(ch, AFF_SLEEP) || IS_AFFECTED(ch,AFF_SAP) )
	{
	    send_to_char("You can't wake up!\n\r",ch);
	    return;
	}

	if (obj == NULL)
	{
	    send_to_char( "You wake up and start resting.\n\r", ch );
	    act ("$n wakes up and starts resting.",ch,NULL,NULL,TO_ROOM);
	}
	else if (IS_SET(obj->value[2],REST_AT))
	{
	    act_new("You wake up and rest at $p.",
		    ch,obj,NULL,TO_CHAR,POS_SLEEPING);
	    act("$n wakes up and rests at $p.",ch,obj,NULL,TO_ROOM);
	}
        else if (IS_SET(obj->value[2],REST_ON))
        {
            act_new("You wake up and rest on $p.",
                    ch,obj,NULL,TO_CHAR,POS_SLEEPING);
            act("$n wakes up and rests on $p.",ch,obj,NULL,TO_ROOM);
        }
        else
        {
            act_new("You wake up and rest in $p.",
                    ch,obj,NULL,TO_CHAR,POS_SLEEPING);
            act("$n wakes up and rests in $p.",ch,obj,NULL,TO_ROOM);
        }
	ch->position = POS_RESTING;
	break;

    case POS_RESTING:
	send_to_char( "You are already resting.\n\r", ch );
	break;

    case POS_STANDING:
	if (obj == NULL)
	{
	    send_to_char( "You rest.\n\r", ch );
	    act( "$n sits down and rests.", ch, NULL, NULL, TO_ROOM );
	}
        else if (IS_SET(obj->value[2],REST_AT))
        {
	    act("You sit down at $p and rest.",ch,obj,NULL,TO_CHAR);
	    act("$n sits down at $p and rests.",ch,obj,NULL,TO_ROOM);
        }
        else if (IS_SET(obj->value[2],REST_ON))
        {
	    act("You sit on $p and rest.",ch,obj,NULL,TO_CHAR);
	    act("$n sits on $p and rests.",ch,obj,NULL,TO_ROOM);
        }
        else
        {
	    act("You rest in $p.",ch,obj,NULL,TO_CHAR);
	    act("$n rests in $p.",ch,obj,NULL,TO_ROOM);
        }
	ch->position = POS_RESTING;
	break;

    case POS_SITTING:
	if (obj == NULL)
	{
	    send_to_char("You rest.\n\r",ch);
	    act("$n rests.",ch,NULL,NULL,TO_ROOM);
	}
        else if (IS_SET(obj->value[2],REST_AT))
        {
	    act("You rest at $p.",ch,obj,NULL,TO_CHAR);
	    act("$n rests at $p.",ch,obj,NULL,TO_ROOM);
        }
        else if (IS_SET(obj->value[2],REST_ON))
        {
	    act("You rest on $p.",ch,obj,NULL,TO_CHAR);
	    act("$n rests on $p.",ch,obj,NULL,TO_ROOM);
        }
        else
        {
	    act("You rest in $p.",ch,obj,NULL,TO_CHAR);
	    act("$n rests in $p.",ch,obj,NULL,TO_ROOM);
	}
	ch->position = POS_RESTING;
	break;
    }


    return;
}


void do_sit (CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj = NULL;

    if (ch->position == POS_FIGHTING)
    {
	send_to_char("Maybe you should finish this fight first?\n\r",ch);
	return;
    }
	
	if (MOUNTED(ch)) 
    {
        send_to_char("You can't sit while mounted.\n\r", ch);
        return;
    }
    if (RIDDEN(ch)) 
    {
        send_to_char("You can't sit while being ridden.\n\r", ch);
        return;
    }

    if ( IS_AFFECTED(ch, AFF_SLEEP) )
    { 
		send_to_char( "You are already sleeping.\n\r", ch ); 
		return; 
	}

    /* okay, now that we know we can sit, find an object to sit on */
    if (argument[0] != '\0')
    {
	//	obj = get_obj_list(ch,argument,ch->in_room->contents);
        obj = get_obj_here(ch,argument);
	if (obj == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }
    else obj = ch->on;

    if (obj != NULL)                                                              
    {
	if (obj->item_type != ITEM_FURNITURE
	||  (!IS_SET(obj->value[2],SIT_ON)
	&&   !IS_SET(obj->value[2],SIT_IN)
	&&   !IS_SET(obj->value[2],SIT_AT)))
	{
	    send_to_char("You can't sit on that.\n\r",ch);
	    return;
	}

	if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
	{
	    act_new("There's no more room on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
	    return;
	}

	ch->on = obj;
    }
    switch (ch->position)
    {
	case POS_SLEEPING:
	    if ( IS_AFFECTED(ch, AFF_SLEEP) || IS_AFFECTED(ch,AFF_SAP) )
	    {
		send_to_char("You can't wake up!\n\r",ch);
		return;
	    }

            if (obj == NULL)
            {
            	send_to_char( "You wake and sit up.\n\r", ch );
            	act( "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM );
            }
            else if (IS_SET(obj->value[2],SIT_AT))
            {
            	act_new("You wake and sit at $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
            	act("$n wakes and sits at $p.",ch,obj,NULL,TO_ROOM);
            }
            else if (IS_SET(obj->value[2],SIT_ON))
            {
            	act_new("You wake and sit on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
            	act("$n wakes and sits at $p.",ch,obj,NULL,TO_ROOM);
            }
            else
            {
            	act_new("You wake and sit in $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
            	act("$n wakes and sits in $p.",ch,obj,NULL,TO_ROOM);
            }

	    ch->position = POS_SITTING;
	    break;
	case POS_RESTING:
	    if (obj == NULL)
		send_to_char("You stop resting.\n\r",ch);
	    else if (IS_SET(obj->value[2],SIT_AT))
	    {
		act("You sit at $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits at $p.",ch,obj,NULL,TO_ROOM);
	    }

	    else if (IS_SET(obj->value[2],SIT_ON))
	    {
		act("You sit on $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits on $p.",ch,obj,NULL,TO_ROOM);
	    }
	    ch->position = POS_SITTING;
	    break;
	case POS_SITTING:
	    send_to_char("You are already sitting down.\n\r",ch);
	    break;
	case POS_STANDING:
	    if (obj == NULL)
    	    {
		send_to_char("You sit down.\n\r",ch);
    	        act("$n sits down on the ground.",ch,NULL,NULL,TO_ROOM);
	    }
	    else if (IS_SET(obj->value[2],SIT_AT))
	    {
		act("You sit down at $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits down at $p.",ch,obj,NULL,TO_ROOM);
	    }
	    else if (IS_SET(obj->value[2],SIT_ON))
	    {
		act("You sit on $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits on $p.",ch,obj,NULL,TO_ROOM);
	    }
	    else
	    {
		act("You sit down in $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits down in $p.",ch,obj,NULL,TO_ROOM);
	    }
    	    ch->position = POS_SITTING;
    	    break;
    }
    return;
}


void do_sleep( CHAR_DATA *ch, char *argument )
{
  OBJ_DATA *obj = NULL;
  
  if (MOUNTED(ch))  {
    send_to_char("You can't sleep while mounted.\n\r", ch);
    return;
  }

  if (RIDDEN(ch)) {
    send_to_char("You can't sleep while being ridden.\n\r", ch);
    return;
  }

  if (ch->bIsLinked) {
    send_to_char("You can't sleep while linked.\n\r", ch);
    return;
  }

  switch ( ch->position ) {
  case POS_SLEEPING:
    send_to_char( "You are already sleeping.\n\r", ch );
    break;
    
  case POS_RESTING:
  case POS_SITTING:
  case POS_STANDING: 
    if (argument[0] == '\0' && ch->on == NULL) {
	 send_to_char( "You go to sleep.\n\r", ch );
         if (IS_AFFECTED(ch, AFF_CHANNELING))
	    do_function(ch, &do_unchannel, "");
	 act( "$n goes to sleep.", ch, NULL, NULL, TO_ROOM );
	 ch->position = POS_SLEEPING;
    }
    else {  /* find an object and sleep on it */
	 if (argument[0] == '\0')
	   obj = ch->on;
	 else
	   obj = get_obj_here(ch,argument);
	 //	obj = get_obj_list( ch, argument,  ch->in_room->contents );
	 
	 if (obj == NULL) {
	   send_to_char("You don't see that here.\n\r",ch);
	   return;
	 }
	 if (obj->item_type != ITEM_FURNITURE
		||  (!IS_SET(obj->value[2],SLEEP_ON) 
			&&   !IS_SET(obj->value[2],SLEEP_IN)
			&&	 !IS_SET(obj->value[2],SLEEP_AT))) {
	   send_to_char("You can't sleep on that!\n\r",ch);
	   return;
	 }
	 
	 if (ch->on != obj && count_users(obj) >= obj->value[0]) {
	   act_new("There is no room on $p for you.",
			 ch,obj,NULL,TO_CHAR,POS_DEAD);
	   return;
	 }
	 
	 ch->on = obj;
	 if (IS_SET(obj->value[2],SLEEP_AT)) {
	   act("You go to sleep at $p.",ch,obj,NULL,TO_CHAR);
	   act("$n goes to sleep at $p.",ch,obj,NULL,TO_ROOM);
	 }
	 else if (IS_SET(obj->value[2],SLEEP_ON)) {
	   act("You go to sleep on $p.",ch,obj,NULL,TO_CHAR);
	   act("$n goes to sleep on $p.",ch,obj,NULL,TO_ROOM);
	 }
	 else {
	   act("You go to sleep in $p.",ch,obj,NULL,TO_CHAR);
	   act("$n goes to sleep in $p.",ch,obj,NULL,TO_ROOM);
	 }
         if (IS_AFFECTED(ch, AFF_CHANNELING))
	    do_function(ch, &do_unchannel, "");
	 ch->position = POS_SLEEPING;
    }
    break;
    
  case POS_FIGHTING:
    send_to_char( "You are already fighting!\n\r", ch );
    break;
  }
  
  return;
}



void do_wake( CHAR_DATA *ch, char *argument )
{
  ROOM_INDEX_DATA *location;
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;

  one_argument( argument, arg );

    if ( arg[0] == '\0' ) {

    	if ( IS_AFFECTED(ch, AFF_SLEEP) || IS_AFFECTED(ch,AFF_SAP) ) { 
	 	act( "You can't wake up!",   ch, NULL, NULL, TO_CHAR );  
	 	return; 
    	}
    
	 if (IS_DREAMING(ch)) {
  	   act("$n fades out of view.", ch,NULL,NULL,TO_ROOM);
	   TOGGLE_BIT(ch->world, WORLD_NORMAL);
	   TOGGLE_BIT(ch->world, WORLD_TAR_DREAM);

	   
	   location = get_room_index(ch->world_vnum);
	   char_from_room( ch );
	   char_to_room( ch, location );

	   if (IS_WOLFSHAPE(ch))
		do_wolfshape(ch, "");
	   
	   send_to_char( "You return to the real world.\n\r", ch );
	   act( "$n wakes and stands up.", ch, NULL, NULL, TO_ROOM );
	   ch->position = POS_STANDING;
	   do_function(ch, &do_look, "auto");
	   return;
	 }
	 else {
	   do_function(ch, &do_stand, "");
	   return; 
	 }
    }

    if ( !IS_AWAKE(ch) ) { 
	 send_to_char( "You are asleep yourself!\n\r",       ch ); 
	 return; 
    }

    if (str_cmp(arg,"all") == 0 || str_cmp(arg,"group") == 0)  //If the argument = "all"
     {
	for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room) 
	  {
	     if (IS_NPC(victim)) //don't wake mobs
	       continue;
	     if (victim == ch) //don't try and wake yourself
	       continue;
	     if (str_cmp(arg,"group") == 0 && !is_same_group(ch,victim))
	       continue;
	     if ( IS_AWAKE(victim) ) 
	       {	     	  
		           act( "$N is already awake.", ch, NULL, victim, TO_CHAR );
		           return;
	       }
	     
	      if ( IS_AFFECTED(victim, AFF_SLEEP) || IS_AFFECTED(victim,AFF_SAP) ) 
	       {	  
		           act( "You can't wake $M!",   ch, NULL, victim, TO_CHAR );
		           return;
	       }
	     
	     
	       act_new( "$n wakes you.", ch, NULL, victim, TO_VICT,POS_SLEEPING );
	       do_function(victim, &do_stand, "");
	  }
	return;
     }
   
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) { 
	 send_to_char( "They aren't here.\n\r",              ch ); 
	 return; 
    }

    if ( IS_AWAKE(victim) ) { 
	 act( "$N is already awake.", ch, NULL, victim, TO_CHAR ); 
	 return; 
    }

    if ( IS_AFFECTED(victim, AFF_SLEEP) || IS_AFFECTED(victim,AFF_SAP) ) { 
	 act( "You can't wake $M!",   ch, NULL, victim, TO_CHAR );  
	 return; 
    }
    
    act_new( "$n wakes you.", ch, NULL, victim, TO_VICT,POS_SLEEPING );
    do_function(victim, &do_stand, "");
    return;    
}


void do_sneak( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
	
	  if (MOUNTED(ch)) 
    {
        send_to_char("You can't sneak while mounted.\n\r", ch);
        return;
    }
    send_to_char( "You attempt to move silently.\n\r", ch );
    affect_strip( ch, gsn_sneak );

    if (IS_AFFECTED(ch,AFF_SNEAK))
	return;

    if (is_ward_set(ch->in_room, WARD_LIGHT)) {
	send_to_char("There is no place to be sneaky here.\n\r",ch);
	return;
    }


    if ( number_percent( ) < get_skill(ch,gsn_sneak))
    {
	check_improve(ch,gsn_sneak,TRUE,3);
	af.where     = TO_AFFECTS;
	af.casterId  = ch->id;
	af.type      = gsn_sneak;
	af.level     = ch->level; 
	af.duration  = ch->level;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_SNEAK;
	affect_to_char( ch, &af );
    }
    else
	check_improve(ch,gsn_sneak,FALSE,3);

    return;
}



void do_hide( CHAR_DATA *ch, char *argument )
{
  AFFECT_DATA af;
  char buf[MSL];

  if (MOUNTED(ch))  {
    send_to_char("You can't hide while mounted.\n\r", ch);
    return;
  }

  if (RIDDEN(ch))  {
    send_to_char("You can't hide while being ridden.\n\r", ch);
    return;
  }

  if ( IS_AFFECTED(ch, AFF_HIDE) ) {	
    /* REMOVE_BIT(ch->affected_by, AFF_HIDE); */
    send_to_char("You already are hidden from most peoples view.\n\r", ch);
    return;
  }
    if (is_ward_set(ch->in_room, WARD_LIGHT)) {
	send_to_char("There is no place to hide here.\n\r",ch);
	return;
    }
  
  if (!IS_NPC(ch) && ch->pcdata->next_hide > current_time) {
    sprintf(buf, "The {Gglowing{x dust still reveals you for %ld more seconds!\n\r", (ch->pcdata->next_hide - current_time) < 0 ? 0 : (ch->pcdata->next_hide -  current_time));
    send_to_char(buf, ch);
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

  if ( number_percent( ) < get_skill(ch,gsn_hide)) {
    send_to_char("You slip into the shadows and hide your presence.\n\r", ch);
    check_improve(ch,gsn_hide,TRUE,3);

    af.where     = TO_AFFECTS;
    af.casterId  = ch->id;
    af.type      = gsn_hide;
    af.level     = ch->level; 
    af.duration  = ch->level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_HIDE;
    affect_to_char( ch, &af );
    
    /*
    send_to_char("You find an excellent place to hide!\n\r",ch);
    SET_BIT(ch->affected_by, AFF_HIDE);
    check_improve(ch,gsn_hide,TRUE,3);
    */
    
  }
  else {
    send_to_char("You can't seem to find a place to hide.\n\r",ch);
    check_improve(ch,gsn_hide,FALSE,3);
  }
  
  return;
}



/*
 * Contributed by Alander.
 */
void do_visible( CHAR_DATA *ch, char *argument )
{
  if (IS_SET(ch->affected_by, AFF_HIDE)) {
    affect_strip ( ch, gsn_hide );
    REMOVE_BIT   ( ch->affected_by, AFF_HIDE		);
    send_to_char("You step out of the shadows.\n\r",ch);
    if (ch->race == race_lookup("fade"))
	 act("$n step out of the shadows.", ch, NULL, NULL, TO_ROOM);
    else
	 act("$n comes out of hiding.", ch, NULL, NULL, TO_ROOM);
  }

  if (IS_SET(ch->affected_by, AFF_INVISIBLE)) {
    affect_strip ( ch, gsn_mass_invis			);
    affect_strip ( ch, gsn_invis			);
    REMOVE_BIT   ( ch->affected_by, AFF_INVISIBLE	);
    send_to_char("You fade into existence.\n\r", ch);
    act("$n fades into existence.", ch, NULL, NULL, TO_ROOM);
  }

  if (IS_SET(ch->affected_by, AFF_SNEAK)) {
    affect_strip ( ch, gsn_sneak			);
    REMOVE_BIT   ( ch->affected_by, AFF_SNEAK		);
    send_to_char("You make your presence known.\n\r", ch);
    act("$n makes $s presence know.", ch, NULL, NULL, TO_ROOM);
  }

  if (IS_SET(ch->affected_by, AFF_CAMOUFLAGE)) {
    affect_strip ( ch, gsn_camouflage			);
    REMOVE_BIT   ( ch->affected_by, AFF_CAMOUFLAGE		);
    send_to_char("You step out from under the cover of leaves.\n\r", ch);
    act("$n steps out from under the cover of leaves.", ch, NULL, NULL, TO_ROOM);
  }
  
  return;
}

void do_graduate(CHAR_DATA *ch, char *argument)
{
  ROOM_INDEX_DATA *location;

  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can graduate.\n\r",ch);
    return;
  }

/*
  if (IS_IMMORTAL(ch)) {
    send_to_char("Get a grip! Immortals don't need to graduate, or perhaps this one does?\n\r", ch);
    return;
  }
*/
  
  if (IS_SET(ch->act, PLR_UNVALIDATED)) {
    send_to_char("You need to be validated before you can graduate from Newbie School.\n\r", ch);
    return;
  }

  if (IS_SET(ch->act, PLR_GRADUATED)) {
    send_to_char("You have already graduated.\n\r", ch);
    return;
  }

  if (( location = get_room_index( ROOM_VNUM_RECALL )) == NULL) {
    send_to_char( "You can't graduate right now.\nPlease see one of the Admin to graduate.\n\r", ch );
    return;
  }
  
  if (ch->position == POS_FIGHTING ) {
    send_to_char( "Maybe you should stop fighting first?\n\r", ch);
    return;
  }

  /* Graduate */
  SET_BIT(ch->act, PLR_GRADUATED);

  /* Once you graduate, conditions starts to kick in */
/*
  ch->pcdata->condition[COND_THIRST]	= 48; 
  ch->pcdata->condition[COND_FULL]	     = 48;
  ch->pcdata->condition[COND_HUNGER]	= 48;
*/

  act( "$n graduate from newbie school.", ch, NULL, NULL, TO_ROOM );
  send_to_char("\nCongratulation! You graduate from Newbie School.\n\r", ch);
  send_to_char("You may now start to explore the realms of the {DShadow {rWars{x.\n\r", ch);
  send_to_char("Please try to have fun.\n\n\r", ch);

  char_from_room( ch );
  char_to_room( ch, location );
  act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
  do_function(ch, &do_look, "auto" );

  wiznet("$N has graduated from newbie school.",ch,NULL,WIZ_NEWBIE,0,0);
  
  return;
}

void do_recall( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  ROOM_INDEX_DATA *location=NULL;
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can recall.\n\r",ch);
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
  
  one_argument(argument,arg);

  act( "$n recalls!", ch, 0, 0, TO_ROOM );
  
  if (!IS_NPC(ch)) {
    if (IS_SET(ch->act, PLR_GRADUATED)) 
    {
	 if (!strcmp("",argument))
	 {
		location = get_room_index( ROOM_VNUM_RECALL );
         }
	 else
	 if (!str_prefix(argument,"mayene"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_MAYENE );
         }
	 else
	 if (!str_prefix(argument,"tear"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_TEAR );
         }
	 else
	 if (!str_prefix(argument,"shaido"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_SHAIDO );
         }
	 else
	 if (!str_prefix(argument,"caemlyn"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_CAEMLYN );
         }
	 else
	 if (!str_prefix(argument,"faldara"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_FALDARA );
         }
	 else
	 if (!str_prefix(argument,"maradon"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_MARADON );
         }
	 else
	 if (!str_prefix(argument,"osenrein"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_OSENREIN );
         }
	 else
	 if (!str_prefix(argument,"falme"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_FALME );
         }
	 else
	 if (!str_prefix(argument,"cairhien"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_CAIRHIEN );
         }
	 else
	 if (!str_prefix(argument,"malden"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_MALDEN );
         }
	 else
	 if (!str_prefix(argument,"sholarbella"))
	 {
		location = get_room_index( ROOM_VNUM_RECALL_SHOLARBELLA );
         }
	 else {
		location = find_city(ch,argument);
         }
	 if ( ( location == NULL )) {
	   send_to_char( "I'm sorry, but you can't jump to there.\n\r", ch );
	   return;
	 }
    }
    else {
	 if ( ( location = get_room_index( ROOM_VNUM_TEMPLE ) ) == NULL ) {
	   send_to_char( "You are completely lost.\n\r", ch );
	   return;
	 }
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
    send_to_char("You are already at your recall!\n\r", ch);
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
  else if (ch->mount != NULL && (ch->in_room == ch->mount->in_room)) {
    char_from_room(ch->mount);
    char_to_room(ch->mount,location);
    do_mount(ch, ch->mount->name);
  }
  
  return;
}

void do_iclocrecall( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  ROOM_INDEX_DATA *location=NULL;


  one_argument(argument,arg);

  if (!strcmp(arg,"set")) {
    if ( !is_room_owner(ch,ch->in_room) && str_cmp(ch->in_room->owner,""))
    {
	send_to_char("You can not set your iclocrecall to this room.\r\n",ch);
	return;
    }
    ch->pcdata->iclocrecall = ch->in_room->vnum;
    send_to_char("ICLocRecall set to this room.\n\r",ch);
    return;
  }
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can recall.\n\r",ch);
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
    if ((location = get_room_index(ch->pcdata->iclocrecall)) == NULL) {
	 if (IS_SET(ch->act, PLR_GRADUATED)) {
	   if ( ( location = get_room_index( ROOM_VNUM_RECALL ) ) == NULL ) {
		send_to_char( "You are completely lost.\n\r", ch );
		return;
	   }
	 }
	 else {
	   if ( ( location = get_room_index( ch->pcdata->iclocrecall ) ) == NULL ) {
		send_to_char( "You are completely lost.\n\r", ch );
		return;
	   }
	 }
    }
  }
  // ######### PET RECALL #######
  else if (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)) {
    //if ( ( location = get_room_index( ch->master->pcdata->iclocrecall ) ) == NULL ) {
    if ( ( location = get_room_index( ch->master->in_room->vnum ) ) == NULL ) {
	 send_to_char( "You are completely lost.\n\r", ch );
	 return;
    }
  }
  
  if ( ch->in_room == location ) {
    send_to_char("You are already at your recall!\n\r", ch);
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
  else if (ch->mount != NULL && (ch->in_room == ch->mount->in_room)) {
    char_from_room(ch->mount);
    char_to_room(ch->mount,location);
    do_mount(ch, ch->mount->name);
  }
  
  return;
}

void do_school( CHAR_DATA *ch, char *argument )
{
  ROOM_INDEX_DATA *location;
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can go to school.\n\r",ch);
    return;
  }

  if (ch->fighting) {
     send_to_char("You are still fighting!\n\r", ch);
     return;
  }

  if ((IS_SET(ch->act, PLR_GRADUATED)) && (!IS_NEWBIEHELPER(ch))) {  
     send_to_char("You can't go to school, you have already graduated.\n\r", ch);
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

  act( "$n heads to school!", ch, 0, 0, TO_ROOM );
  if ((location = get_room_index(ROOM_VNUM_SCHOOL)) == NULL) {
    if ((IS_SET(ch->act, PLR_GRADUATED)) && (!IS_NEWBIEHELPER(ch)) ) {
	   if ( ( location = get_room_index( ROOM_VNUM_RECALL ) ) == NULL ) {
	   send_to_char( "You are completely lost.\n\r", ch );
	   return;
	 }
    }
    else {
	 if ( ( location = get_room_index( ROOM_VNUM_SCHOOL ) ) == NULL ) {
	 //if ( ( location = get_room_index( ROOM_VNUM_RECALL ) ) == NULL ) {
	   send_to_char( "You are completely lost.\n\r", ch );
	   return;
	 }
    }
  }

  if ( ch->in_room == location ) {
    send_to_char("You are already in school!\n\r", ch);
    return;
  }
  
  act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
  char_from_room( ch );
  char_to_room( ch, location );
  act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
  do_function(ch, &do_look, "auto" );
  
  if (ch->pet != NULL)
    do_function(ch->pet, &do_school, "");
  
  return;
}



void do_train_OLD( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    sh_int stat = - 1;
    char *pOutput = NULL;
    int cost;

    if ( IS_NPC(ch) )
	return;

    /*
     * Check for trainer.
     */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
	if ( IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN) )
	    break;
    }

    if ( mob == NULL )
    {
	send_to_char( "You can't do that here.\n\r", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	sprintf( buf, "You have %d training sessions.\n\r", ch->train );
	send_to_char( buf, ch );
	argument = "foo";
    }

    cost = 1;

    if ( !str_cmp( argument, "str" ) )
    {
	if ( class_table[ch->class].attr_prime == STAT_STR )
	    cost    = 1;
	stat        = STAT_STR;
	pOutput     = "strength";
    }

    else if ( !str_cmp( argument, "int" ) )
    {
	if ( class_table[ch->class].attr_prime == STAT_INT )
	    cost    = 1;
	stat	    = STAT_INT;
	pOutput     = "intelligence";
    }

    else if ( !str_cmp( argument, "wis" ) )
    {
	if ( class_table[ch->class].attr_prime == STAT_WIS )
	    cost    = 1;
	stat	    = STAT_WIS;
	pOutput     = "wisdom";
    }

    else if ( !str_cmp( argument, "dex" ) )
    {
	if ( class_table[ch->class].attr_prime == STAT_DEX )
	    cost    = 1;
	stat  	    = STAT_DEX;
	pOutput     = "dexterity";
    }

    else if ( !str_cmp( argument, "con" ) )
    {
	if ( class_table[ch->class].attr_prime == STAT_CON )
	    cost    = 1;
	stat	    = STAT_CON;
	pOutput     = "constitution";
    }

    else if ( !str_cmp(argument, "hp" ) )
	cost = 1;

    else if ( !str_cmp(argument, "endurance" ) )
	cost = 1;

    else
    {
	strcpy( buf, "You can train:" );
	if ( ch->perm_stat[STAT_STR] < get_max_train(ch,STAT_STR)) 
	    strcat( buf, " str" );
	if ( ch->perm_stat[STAT_INT] < get_max_train(ch,STAT_INT))  
	    strcat( buf, " int" );
	if ( ch->perm_stat[STAT_WIS] < get_max_train(ch,STAT_WIS)) 
	    strcat( buf, " wis" );
	if ( ch->perm_stat[STAT_DEX] < get_max_train(ch,STAT_DEX))  
	    strcat( buf, " dex" );
	if ( ch->perm_stat[STAT_CON] < get_max_train(ch,STAT_CON))  
	    strcat( buf, " con" );
	strcat( buf, " hp endurance");

	if ( buf[strlen(buf)-1] != ':' )
	{
	    strcat( buf, ".\n\r" );
	    send_to_char( buf, ch );
	}
	else
	{
	    /*
	     * This message dedicated to Jordan ... you big stud!
	     */
	    act( "You have nothing left to train, you $T!",
		ch, NULL,
		ch->sex == SEX_MALE   ? "big stud" :
		ch->sex == SEX_FEMALE ? "hot babe" :
					"wild thing",
		TO_CHAR );
	}

	return;
    }

    if (!str_cmp("hp",argument))
    {
    	if ( cost > ch->train )
    	{
       	    send_to_char( "You don't have enough training sessions.\n\r", ch );
            return;
        }
 
	ch->train -= cost;
        ch->pcdata->perm_hit += 10;
        ch->max_hit += 10;
        ch->hit +=10;
        act( "Your durability increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's durability increases!",ch,NULL,NULL,TO_ROOM);
        return;
    }
 
    if (!str_cmp("endurance",argument))
    {
        if ( cost > ch->train )
        {
            send_to_char( "You don't have enough training sessions.\n\r", ch );
            return;
        }

	ch->train -= cost;
        ch->pcdata->perm_endurance += 10;
        ch->max_endurance += 10;
        ch->endurance += 10;
        act( "Your power increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's power increases!",ch,NULL,NULL,TO_ROOM);
        return;
    }

    if ( ch->perm_stat[stat]  >= get_max_train(ch,stat) )
    {
	act( "Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR );
	return;
    }

    if ( cost > ch->train )
    {
	send_to_char( "You don't have enough training sessions.\n\r", ch );
	return;
    }

    ch->train		-= cost;
  
    ch->perm_stat[stat]		+= 1;
    act( "Your $T increases!", ch, NULL, pOutput, TO_CHAR );
    act( "$n's $T increases!", ch, NULL, pOutput, TO_ROOM );
    return;
}

void do_camouflage( CHAR_DATA *ch, char *argument )
{

    if (MOUNTED(ch)) 
    {
        send_to_char("You can't camouflage while mounted.\n\r", ch);
        return;
    }
    if (RIDDEN(ch)) 
    {
        send_to_char("You can't camouflage while being ridden.\n\r", ch);
        return;
    }

    if ( IS_NPC(ch) || 
         ch->level < skill_table[gsn_camouflage].skill_level[ch->class] )
      {
		send_to_char("You don't know how to camouflage yourself.\n\r",ch);
		return;
      }

    if (ch->in_room->sector_type != SECT_FOREST &&
	ch->in_room->sector_type != SECT_HILLS  &&
	ch->in_room->sector_type != SECT_MOUNTAIN &&
	ch->in_room->sector_type != SECT_DESERT)
      {
		send_to_char("There is no cover here.\n\r",ch);
		act("$n tries to camouflage $mself against the lone leaf on the ground.",ch,NULL,NULL,TO_ROOM);
		return;
      }
    send_to_char( "You attempt to camouflage yourself.\n\r", ch );
    WAIT_STATE( ch, skill_table[gsn_camouflage].beats );

    if ( IS_AFFECTED(ch, AFF_CAMOUFLAGE) )
	REMOVE_BIT(ch->affected_by, AFF_CAMOUFLAGE);


    if ( IS_NPC(ch) || 
	number_percent( ) < get_skill(ch,gsn_camouflage) )
    {
	send_to_char ("You find an excellent cover.\n\r",ch);
		SET_BIT(ch->affected_by, AFF_CAMOUFLAGE);
	check_improve(ch,gsn_camouflage,TRUE,1);
    }
    else
	{
		send_to_char("You can't find any cover.\n\r",ch);
		check_improve(ch,gsn_camouflage,FALSE,1);
	}
    
	return;
}

void do_bash_door( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int chance=0;
    int damage_bash,door;

    one_argument(argument,arg);
 
    if ( get_skill(ch,gsn_bash_door) == 0
    ||	 (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BASH))
    ||	 (!IS_NPC(ch)
    &&	  ch->level < skill_table[gsn_bash_door].skill_level[ch->class]))
    {	
	send_to_char("Bashing? What's that?\n\r",ch);
	return;
    }
 
    if (MOUNTED(ch)) 
    {
        send_to_char("You can't bash doors while mounted.\n\r", ch);
        return;
    }
    if (RIDDEN(ch)) 
    {
        send_to_char("You can't bash doors while being ridden.\n\r", ch);
        return;
    }

    if (arg[0] == '\0')
    {
    send_to_char("Bash which door or direction.\n\r",ch);
    return;
    }

    if (ch->fighting)
    {	
	send_to_char("Wait until the fight finishes.\n\r",ch);
	return;
    }

    /* look for guards */
    for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
    {
	if ( IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level )
	{
	    act( "$N is standing too close to the door.",
		ch, NULL, gch, TO_CHAR );
	    return;
	}
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
	/* 'bash door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit;
	EXIT_DATA *pexit_rev;

	pexit = ch->in_room->exit[door];
	if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
	    { send_to_char( "It's already open.\n\r",      ch ); return; }
	if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
	    { send_to_char( "Just try to open it.\n\r",     ch ); return; }
	if (IS_SET(pexit->exit_info, EX_NOBASH)) {
	  send_to_char("This door looks to be impossible to bash!\n\r", ch);
	  return;
	}	  
	if (IS_SET(pexit->exit_info, EX_FIREWALL)) {
	  send_to_char("You suddenly realise that it hides a wall of fire!!\n\r", ch);
	  damage(ch, ch, UMAX(100, ch->hit/2), gsn_wof, DAM_FIRE, FALSE);
	  return;
	}
	if ( IS_SET(pexit->exit_info, EX_NOPASS) )
	  { send_to_char( "A mystical shield protects the exit.\n\r",ch ); 
	  return; }

    /* modifiers */

    /* size  and weight */
    chance += get_carry_weight(ch) / 100;

    chance += (ch->size - 2) * 20;

    /* stats */
    chance += get_curr_stat(ch,STAT_STR);

    if (IS_AFFECTED(ch,AFF_FLYING))
	chance -= 10;

    /* level 
    chance += ch->level / 10;
    */

    chance += (get_skill(ch,gsn_bash_door) - 90);

    act("You slam into $d, and try to break $d!",
		ch,NULL,pexit->keyword,TO_CHAR);
    act("You slam into $d, and try to break $d!",
		ch,NULL,pexit->keyword,TO_ROOM);

    if ( IS_SET(ch->in_room->room_flags, ROOM_DARK) )
		chance /= 2;

    if ( IS_SET(pexit->exit_info,EX_BARRED))
    {
	chance = chance / 4;
    }
    /* now the attack */
    if (number_percent() < chance )
    {
    
	check_improve(ch,gsn_bash_door,TRUE,1);


	REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	REMOVE_BIT(pexit->exit_info, EX_CLOSED);
	act( "$n bashes the the $d and breaks the lock.", ch, NULL, 
		pexit->keyword, TO_ROOM );
	send_to_char( "You succeeded to open the door.\n\r", ch );

	/* open the other side */
	if ( ( to_room   = pexit->u1.to_room            ) != NULL
	&&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
	&&   pexit_rev->u1.to_room == ch->in_room )
	{
	    CHAR_DATA *rch;

	    REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
	    REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
	    for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
		act( "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
	}


	WAIT_STATE(ch,skill_table[gsn_bash_door].beats);
	
    }
    else
    {
	act("You fall flat on your face!",
	    ch,NULL,NULL,TO_CHAR);
	act("$n falls flat on $s face.",
	    ch,NULL,NULL,TO_ROOM);
	check_improve(ch,gsn_bash_door,FALSE,1);
	ch->position = POS_RESTING;
	WAIT_STATE(ch,skill_table[gsn_bash_door].beats * 3/2); 
	damage_bash = ch->damroll + number_range(4,4 + 4* ch->size + chance/5);
	damage(ch,ch,damage_bash,gsn_bash_door, DAM_BASH, TRUE);
    }
    return;
    }
  return;
}

void do_push( CHAR_DATA *ch, char *argument )
{
  char buf  [MAX_STRING_LENGTH];
  char arg1 [MAX_INPUT_LENGTH];
  char arg2 [MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int percent;
  int door=0;
  
  // portal stuff
  OBJ_DATA *portal=NULL;
  ROOM_INDEX_DATA *location=NULL;
  ROOM_INDEX_DATA *old_room;
  
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  
  if ( arg1[0] == '\0' || arg2[0] == '\0') {
    send_to_char( "Push whom to what diretion?\n\r", ch );
    return;
  }

  if (MOUNTED(ch))  {
    send_to_char("You can't push while mounted.\n\r", ch);    
    return;
  }

  if (RIDDEN(ch))  {
    send_to_char("You can't push while being ridden.\n\r", ch);    
    return;
  }

  if ( IS_NPC(ch) && IS_SET(ch->affected_by, AFF_CHARM) && (ch->master != NULL)) {
    send_to_char( "You are to dazed to push anyone.\n\r", ch);
    return;
  }
  
  if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }
  
  if (!IS_NPC(victim) && victim->desc == NULL) {
    send_to_char("You can't do that.\n\r", ch);
    return;
  }
  
  if ( victim == ch )     {
    send_to_char( "That's pointless.\n\r", ch );    
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
  
  if (is_safe(ch,victim))
    return;

  if (victim->position == POS_FIGHTING) {
    send_to_char("Wait till the end of fight.\n\r",ch);
    return;
  }
  
  if (victim->position != POS_STANDING) {
    send_to_char("You can't push someone not standing.\n\r",ch);
    return;
  }
  
  portal = get_obj_list( ch, arg2,  ch->in_room->contents );
  
  if (portal == NULL && (door = find_exit( ch, arg2)) >= 0) {  	
    /* 'push' */
    EXIT_DATA *pexit;
    
    if ( (pexit = ch->in_room->exit[door]) != NULL ) {
	 if ( IS_SET(pexit->exit_info, EX_ISDOOR) )  {
	   if ( IS_SET(pexit->exit_info, EX_CLOSED) ) {
		send_to_char( "Direction is closed.\n\r",      ch ); 
		return;
	   }
	   else if ( IS_SET(pexit->exit_info, EX_LOCKED) ) {
	     send_to_char( "Direction is locked.\n\r",     ch ); 
		return;
	   }
	 }
	 else if (IS_SET(pexit->exit_info, EX_FIREWALL)) {
	   send_to_char("You suddeny notice that direction is blocked by a wall of fire!\n\r", ch);
	   return;	
	 }
	 else if (IS_SET(pexit->exit_info, EX_AIRWALL)) {
	   send_to_char("You suddeny notice that direction is blocked by a wall of air!\n\r", ch);
	   return;	
	 }
	 else if (IS_SET(pexit->exit_info, EX_BLOCKED)) {
	   send_to_char("You suddeny notice that direction is blocked by someone!\n\r", ch);
	   return;	
	 }
    }
    else {
	 send_to_char("Alas, but there is nothing in that direction.\n\r",ch);
	 return;      	
    }
  }
  else {    
    if (portal == NULL) {    	
	 return;
    }
    
    if (portal->item_type != ITEM_PORTAL) {
	 act("You can't push someone into $p.",ch,portal,NULL,TO_CHAR);
	 return;
    }		
  }
  
  if (portal != NULL && portal->item_type != ITEM_PORTAL) {
    act("You can't push someone into $p.",ch,portal,NULL,TO_CHAR);
    return;
  }		   

  if (portal != NULL && IS_SET(portal->value[2],GATE_SKIMMING_IN)) {
    act("You can't push someone into $p.",ch,portal,NULL,TO_CHAR);
    return;
  }
  
  WAIT_STATE( ch, skill_table[gsn_push].beats );
  percent  = number_percent( ) + ( IS_AWAKE(victim) ? 10 : -50 );
  percent += can_see( victim, ch ) ? -10 : 0;
  
  if ( /* ch->level + 5 < victim->level || */
	 victim->position == POS_FIGHTING
	 || ( !IS_NPC(ch) && percent > get_skill(ch,gsn_push) ) ) {
    /*
	* Failure.
	*/
    
    send_to_char( "Oops.\n\r", ch );
    if ( !IS_AFFECTED( victim, AFF_SLEEP ) ) {
	 victim->position= victim->position==POS_SLEEPING? POS_STANDING:
	   victim->position;
	 act( "$n tried to push you.\n\r", ch, NULL, victim,TO_VICT  );
    }
    act( "$n tried to push $N.\n\r",  ch, NULL, victim,TO_NOTVICT);
    
    if ( !IS_NPC(ch) ) {
	 if ( IS_NPC(victim) ) {
	   check_improve(ch,gsn_push,FALSE,2);
	   multi_hit( victim, ch, TYPE_UNDEFINED );
	 }
    }
    
    return;
  }
  
  // If immune to push, do a supplise move! 
  if (IS_SET(victim->imm_flags,IMM_PUSH) && portal == NULL) {
    sprintf(buf,"You try to push $N %sward but $E react quickly and pushes you %sward!",door < 6 ? dir_name[door] : "out", door < 6 ? dir_name[door] : "out");
    act(buf,ch,NULL,victim,TO_CHAR);
    sprintf(buf,"$n try to push you %sward but you react quick and pushes $e %sward.", door < 6 ? dir_name[door] : "out", door < 6 ? dir_name[door] : "out");
    act(buf,ch,NULL,victim,TO_VICT);
    sprintf(buf,"$n try to push $N %sward but $E react quickly and pushes $n %sward.", door < 6 ? dir_name[door] : "out", door < 6 ? dir_name[door] : "out");
    act(buf,ch,NULL,victim,TO_NOTVICT);
    move_char( ch , door , FALSE );
       return;    	
  }
  else if (IS_SET(victim->imm_flags,IMM_PUSH) && portal != NULL) {
    sprintf(buf,"You try to push $N but $E react quickly and jump aside!");
    act(buf,ch,NULL,victim,TO_CHAR);
    sprintf(buf,"$n try to push you but you react quick jump aside!");
    act(buf,ch,NULL,victim,TO_VICT);
    sprintf(buf,"$n try to push $N but $E react quickly and jump aside!");
    act(buf,ch,NULL,victim,TO_NOTVICT);
    return;
  }
  else {
    
    if (portal == NULL) {	    	    
	 sprintf(buf,"You push $N %sward.",door < 6 ? dir_name[door] : "out");
	 act(buf,ch,NULL,victim,TO_CHAR);
	 sprintf(buf,"$n pushes you %sward.", door < 6 ? dir_name[door] : "out");
	 act(buf,ch,NULL,victim,TO_VICT);
	 sprintf(buf,"$n pushes $N %sward.", door < 6 ? dir_name[door] : "out");
	 act(buf,ch,NULL,victim,TO_NOTVICT);
	 move_char( victim , door , FALSE );
	 
	 check_improve(ch,gsn_push,TRUE,1);
    }
    
    // Portal push
    else {       	       	         	  
	 old_room = victim->in_room;
	 
	 if (IS_SET(portal->value[1],EX_CLOSED)) {
	   act("You push $N toward the closed $p.",ch,portal,victim,TO_CHAR);
	   act("$n pushes you toward the closed $p.",ch,portal,victim,TO_VICT);
	   act("$n pushes $N toward the closed $p.",ch,portal,victim,TO_NOTVICT);
	   return;
	 } 	
	 
	 if (IS_SET(portal->value[2],GATE_RANDOM) || portal->value[3] == -1) {
	   location = get_random_room(ch, TRUE);
	   portal->value[3] = location->vnum; /* for record keeping :) */
	 }
	 else if (IS_SET(portal->value[2],GATE_BUGGY) && (number_percent() < 5))
	   location = get_random_room(ch, TRUE);
	 else
	   location = get_room_index(portal->value[3]);
	 
	 // If nowhere or same room
	 if (location == NULL
		||  location == old_room
		||  !can_see_room(victim,location) 
		||  (room_is_private(location) && !IS_TRUSTED(victim,IMPLEMENTOR))) {
	   
	   // Dreamgate check
	   if (IS_SET(portal->value[2],GATE_DREAMGATE)) {
		if (IS_DREAMING(victim)) {
		  act("You push $N toward the $p, but something is preventing $M from beeing pushed through it.",ch,portal,victim,TO_CHAR);
		  act("$n pushes you toward the $p, but something is preventing you from beeing pushed through it.",ch,portal,victim,TO_VICT);
		  act("$n pushes $N toward the $p, but something is preventing $M from beeing pushed through it.",ch,portal,victim,TO_NOTVICT);                	             	
		  return;
		}

		// Push into dreamgate
		act("You push $N into $p.",ch,portal,victim,TO_CHAR);
		act("$n pushes you into $p.",ch,portal,victim,TO_VICT);
		act("$n pushes $N info $p",ch,portal,victim,TO_NOTVICT);                
		TOGGLE_BIT(victim->world, WORLD_NORMAL);
		TOGGLE_BIT(victim->world, WORLD_TAR_FLESH);
		do_function(victim, &do_look, "auto");
		return;
	   }
	   else {
		act("You push $N toward the $p, but something is preventing $M from beeing pushed through it.",ch,portal,victim,TO_CHAR);
		act("$n pushes you toward the $p, but something is preventing you from beeing pushed through it.",ch,portal,victim,TO_VICT);
		act("$n pushes $N toward the $p, but something is preventing $M from beeing pushed through it.",ch,portal,victim,TO_NOTVICT);                	             	              	                
		return;
	   }
	 }
	 
	 // Normal push into portal type
	 act("You push $N into $p.",ch,portal,victim,TO_CHAR);
	 act("$n pushes you into $p.",ch,portal,victim,TO_VICT);
	 act("$n pushes $N info $p",ch,portal,victim,TO_NOTVICT);                
	 char_from_room(victim);
	 char_to_room(victim, location);
	 do_function(victim, &do_look, "auto");
	
	 // Make sure Triggers are enabled
	 if ( IS_NPC( victim ) && HAS_TRIGGER( victim, TRIG_ENTRY ) )
	   mp_percent_trigger( victim, NULL, NULL, NULL, TRIG_ENTRY );
	 if ( !IS_NPC( victim ) )
	   mp_greet_trigger( victim );
	 
	 return;   
    }
  }
  return;
}



void do_drag ( CHAR_DATA *ch, char * argument)
{
   char *arg;
   OBJ_DATA * obj;
   char buf[MAX_INPUT_LENGTH];

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

   arg = one_argument (argument,buf);
   obj = get_obj_list( ch, buf, ch->in_room->contents );
   if ( !obj )
   {
          do_drag_char(ch,argument);
	  return;
   }
   else
   {
      do_drag_obj(ch,argument);
     return;
   }
}

void do_drag_obj( CHAR_DATA * ch, char * argument )
{

   char buf[MAX_INPUT_LENGTH];
   OBJ_DATA  *obj;
   ROOM_INDEX_DATA *rvnum;
   CHAR_DATA *vch;
   EXIT_DATA *pexit;
   int direction;

   argument = one_argument (argument, buf);


   obj = get_obj_list( ch, buf, ch->in_room->contents );
   
   if ( !obj )
   {
	  send_to_char ( "I do not see anything like that here.\n\r", ch);
	  return;
   }

   if (MOUNTED(ch)) 
    {
        send_to_char("You can't drag anything while mounted.\n\r", ch);

        return;
    }
    if (RIDDEN(ch)) 
    {
        send_to_char("You can't drag anything while being ridden.\n\r", ch);

        return;
    }

/* ITEM_DRAGGABLE is flag which I added for items which I want to be dragged
   but not to be taken ( for example wagon in mine ).
   If you dislike this idea comment it out */

   if ( (!IS_SET( obj->wear_flags, ITEM_TAKE ) && 
        (!IS_SET( obj->extra_flags, ITEM_DRAGGABLE ) ) ) )
   {
 	  act( "You try to drag $p, but without succes.\n\r", ch,obj, NULL, TO_CHAR);
 	  act( "$n tries to move $p, but it doesn't budge.\n\r",ch,obj, NULL, TO_ROOM);
	  return;
   }

   if ( obj->weight >  (2 * can_carry_w (ch)) )
   {
      act( "You try, but $p is too heavy.\n\r", ch, obj, NULL, TO_CHAR);
      act( "$n tries to move $p, but $? fail.\n\r", ch, obj, NULL, TO_CHAR);
	  return;
   }

   if ( argument[0] == '\0' )
   {
      send_to_char ( "Where do you want to drag it ?\n\r", ch);
	  return;
   }

   /* get room num */
   rvnum = ch->in_room;
   
   /* Get exit integer value */
   direction = find_exit( ch,argument);

   /* get room exit info */
   pexit = rvnum->exit[direction];
   
   /* make sure it's an exit there or can be seen */
   if ((pexit == NULL) || !can_see_room(ch, pexit->u1.to_room)) {
     send_to_char("You don't see exit in that direction.\n\r", ch);
     return;
   }
   
   /* it's not natural to drag throgh closed doors so we don't */
   if (IS_SET(pexit->exit_info, EX_CLOSED)) {
     send_to_char("You try to drag it through a closed door.  You fail.\n\r",ch);
     return;
   }

   /* All ok, lets move the char */
   move_char( ch, direction,FALSE);

   /* Bug */
   if (ch->in_room == rvnum )
      return;                      /* For some reason player didn't move */

   obj_from_room (obj);
   obj_to_room (obj, ch->in_room);

   act ( "You dragged $p with you.\n\r", ch, obj, NULL, TO_CHAR );
   act ( "$n dragged $p with $?.\n\r", ch, obj, NULL, TO_ROOM );

   if ( !(vch = rvnum->people) )
      return;

   act ( "$N dragged $p out.\n\r", vch, obj, ch, TO_CHAR );
   act ( "$N dragged $p out.\n\r", vch, obj, ch, TO_ROOM );

   return;
}

void do_drag_char( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int door;

    argument=one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Who do you want to drag?\n\r", ch ); 
	return; 
    }

    if (MOUNTED(ch)) 
    {
        send_to_char("You can't drag anyone while mounted.\n\r", ch);

        return;
    }
    if (RIDDEN(ch)) 
    {
        send_to_char("You can't drag anyone while being ridden.\n\r", ch);

        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
	{ send_to_char( "They aren't here.\n\r",              ch ); return; }

    if ( is_safe( ch, victim ) )
	return;

    if (IS_NPC(victim) && victim->pIndexData->pShop != NULL) {
	send_to_char("The shopkeeper wouldn't like that.\n\r",ch); 
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

    if ( ch->position == POS_RESTING )
	{ send_to_char( "Maybe you should stand first?\n\r", ch); return; }

    if ( victim->position == POS_STANDING ) { 
    	act( "$N is standing. Try pushing $M.", ch, NULL, victim, TO_CHAR);
	return; 
    }

    if ( !IS_AWAKE(ch) )
	{ send_to_char( "Try waking up first!\n\r",       ch ); return; }

    if ( ch->position == POS_FIGHTING )
	{ send_to_char( "Maybe you should stop fighting first?\n\r", ch); return; }

    if ( victim->position == POS_FIGHTING )
	{ act( "$N is fighting. Wait your turn!", ch, NULL, victim, TO_CHAR);
	  return; }
       
    if ( (door = find_exit( ch, argument )) < 0 )
      return;

    drag_char( ch, victim, door, FALSE );

/*

    if ( get_curr_stat(ch, STAT_STR) > get_curr_stat(victim, STAT_STR)) {
       drag_char( ch, victim, door, FALSE );
    }
    else if (get_curr_stat(ch, STAT_STR) == get_curr_stat(victim, STAT_STR) && number_percent() < 5) {
       drag_char( ch, victim, door, FALSE );
    }    	
    else {
    	sprintf(buf, "You try to drag $N %sward, but fail.", door < 6 ? dir_name[door] : "out");
    	act(buf, ch, NULL, victim, TO_CHAR);
    	sprintf(buf, "$n tries to drag You %sward, but fails.", door < 6 ? dir_name[door] : "out");
	act(buf, ch, NULL, victim, TO_VICT);
	sprintf(buf, "$n tries to drag $N %sward, but fails.", door < 6 ? dir_name[door] : "out");
	act(buf, ch, NULL, victim, TO_ROOM);
    }
    */
   return;
}

void drag_char( CHAR_DATA *ch, CHAR_DATA *victim, int door, bool follow )
{
    char buf[MSL];
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;

    if ((!IS_NPC(victim)) && (IS_SET(ch->in_room->room_flags,ROOM_SAFE))) {
      send_to_char( "Not in this room.\n\r", ch);
      return;
    }
    
//    if (is_affected(ch,skill_lookup("web")))
//    {
//	send_to_char( "You attempt to leave the room, but the webs hold you tight.\n\r", ch );
//	act( "$n struggles vainly against the webs which hold $m in place.", ch, NULL, NULL, TO_ROOM );
//	return; 
//    }
//     if (is_affected(victim,skill_lookup("web")))
//    {
//	send_to_char( "You attempt to leave the room, but the webs hold you tight.\n\r", ch );
//	act( "$m struggles vainly against the webs which hold $n in place.", ch, NULL, NULL, TO_ROOM );
//	return; 
//    }	


    if ( door < 0 || door > 9 ) {
	bug( "Do_move: bad door %d.", door );
	return;
    }

    in_room = ch->in_room;
/* Fixed phantom drag bug Deth 11/02/94 */    
    if (( pexit   = in_room->exit[door] ) == NULL || ( to_room = pexit->u1.to_room   ) == NULL  || !can_see_room(ch,pexit->u1.to_room)) {
	send_to_char("You can't find your way out in that direction!\n\r",ch);
        if (IS_AWAKE(victim) )  
          send_to_char( "You get dragged around the room!\n\r",victim ); 
        return; 
    }

    if (IS_SET(pexit->exit_info, EX_CLOSED) && !IS_AFFECTED(ch, AFF_PASS_DOOR)) {
	act( "The door $d is closed, you are dragged against it.", victim, NULL, pexit->keyword, TO_CHAR );
        send_to_char("You try to drag them through a closed door.  You fail.\n\r",ch);
	return;
    }

    if (IS_SET(pexit->exit_info, EX_AIRWALL)) {
	 send_to_char("You suddeny notice that direction is blocked by a wall of air!\n\r", ch);
	 return;	
    }

/*
    if ( room_is_private( to_room ) ) {
	send_to_char( "That room is private right now.\n\r",ch );
	return;
    }
*/

    if ( in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR ) {
      if ( !IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch)) {
         send_to_char( "You are dragged into the air and fall down.\n\r", victim );
	 return;
      }
    }


   WAIT_STATE( ch, 1 );

    if ( !IS_AFFECTED(victim, AFF_SNEAK) && ( IS_NPC(victim) ) )
       if (IS_AWAKE(victim)) {
        //printf_to_char(victim, "%s drags you %s.\n\r", ch->name, door < 6 ? dir_name[door] : "out");
         sprintf(buf, "$n drags you %s.", door < 6 ? dir_name[door] : "out");
         act(buf, ch, NULL, victim, TO_VICT );
       }
    //printf_to_char(ch, "You drag %s %s.\n\r", victim->name, door < 6 ? dir_name[door] : "out");
    sprintf(buf, "You drag $N %s.", door < 6 ? dir_name[door] : "out");
    act(buf, ch, NULL, victim, TO_CHAR );

    char_from_room( ch );
    char_from_room(victim);
    char_to_room( ch, to_room );
    char_to_room(victim,to_room);
    act( "$n is dragged $T.", victim, NULL, door < 6 ? door < 6 ? dir_name[door] : "out" : "out", TO_ROOM );
    act("$n drags $N into the room.",ch, NULL, victim, TO_ROOM);
    do_look( ch, "auto" );
    
    if (!victim->position == POS_SLEEPING)  {
	do_look(victim,"auto");
    }

    if (in_room == to_room) /* no circular follows */
	return;

    for ( fch = in_room->people; fch != NULL; fch = fch_next ) {
	fch_next = fch->next_in_room;

	if ( fch->master == victim && IS_AFFECTED(fch,AFF_CHARM) 
	&&   fch->position < POS_STANDING)
	    do_stand(fch,"");

	if ( fch->master == victim && fch->position == POS_STANDING )
	{

	    if (IS_SET(victim->in_room->room_flags,ROOM_LAW)
	    &&  (IS_NPC(fch) && IS_SET(fch->act,ACT_AGGRESSIVE)))
	    {
		act("You can't bring $N into the city.",
		    victim,NULL,fch,TO_CHAR);
		act("You aren't allowed in the city.",
		    fch,NULL,NULL,TO_CHAR);
		return;
	    }

	    act( "You follow $N.", fch, NULL,victim, TO_CHAR );
	    move_char( fch, door, TRUE );
	}
    }

    return;
}

int send_arrow( CHAR_DATA *ch, CHAR_DATA *victim,OBJ_DATA *arrow , int door, int chance ,int bonus) 
{
  EXIT_DATA *pExit;
  ROOM_INDEX_DATA *dest_room;
  char buf[512];
  OBJ_DATA *corpse;
  AFFECT_DATA *paf;
  AFFECT_DATA af;
  int damroll=0,hitroll=0,sn;
  int dam;

  if (arrow->value[0] == WEAPON_SPEAR)  
    sn = gsn_throw;  
  else 
    sn = gsn_arrow;

  /* Check if exit from room is blocked */
  if (ch->in_room != victim->in_room) {
    pExit = ch->in_room->exit[door];
    if (IS_SET(pExit->exit_info, EX_FIREWALL)) {
	 act("The $p turn into {Rflames{x as it hit the {rwall of fire{x.", ch, arrow, NULL, TO_CHAR);
	 act("The $p turn into {Rflames{x as it hit the {rwall of fire{x.", ch, arrow, NULL, TO_ROOM);
	 extract_obj(arrow);
	 return 0;
    }
    if (IS_SET(pExit->exit_info, EX_AIRWALL)) {
	 act("The $p is blocked by a {Cblue haze{x floating in the {Wair{x.", ch, arrow, NULL, TO_CHAR);
	 act("The $p is blocked by a {Cblue haze{x floating in the {Wair{x.", ch, arrow, NULL, TO_ROOM);
	 obj_to_room(arrow,ch->in_room);
	 return 0;
    }
  }
  
  /* forbid target that cannot move back to revenge on the thrower */  
  if(IS_NPC(victim)) {
    if ( (IS_SET(victim->act, ACT_SENTINEL) && (victim->in_room->vnum != ch->in_room->vnum))
	    || IS_SET(ch->in_room->room_flags, ROOM_NO_MOB)
	    || ( IS_SET(victim->act, ACT_OUTDOORS)
		    && IS_SET(ch->in_room->room_flags,ROOM_INDOORS))
	    || ( IS_SET(victim->act, ACT_INDOORS)
		    && IS_SET(ch->in_room->room_flags,ROOM_INDOORS))) {
	 act("You miss.",ch,arrow,victim,TO_CHAR);
	 act("$N avoids the $p thrown by $n.",ch,arrow,victim,TO_NOTVICT);
	 act("You avoid the $p thrown by $n.",ch,arrow,victim,TO_VICT);
	 obj_to_room(arrow,victim->in_room);
	 return 0;
    }
  }
  
  for ( paf = arrow->affected; paf != NULL; paf = paf->next ) {
    if ( paf->location == APPLY_DAMROLL )
	 damroll += paf->modifier;
    
    if ( paf->location == APPLY_HITROLL )
	 hitroll += paf->modifier;
  }

	
  dest_room = ch->in_room;
  
//	chance += (hitroll + str_app[get_curr_stat(ch,STAT_STR)].tohit
//			+ (get_curr_stat(ch,STAT_DEX) - 18)) * 2;

  chance += (hitroll + str_app[get_curr_stat(ch,STAT_STR)].tohit);
	
  damroll *= 10;
	
  while (1) {
    chance -= 10;
    if ( victim->in_room == dest_room )  {
	 if (number_percent() < chance) { 
	   if ( check_obj_dodge(ch,victim,arrow,chance))
		return 0;
	   act("$p strikes you!", victim, arrow, NULL, TO_CHAR );
	   act("Your $p strikes $N!", ch, arrow, victim, TO_CHAR );
	      
	   if (ch->in_room == victim->in_room)
		act("$n's $p strikes $N!", ch, arrow, victim, TO_NOTVICT );	   
	   else  {
		act("$n's $p strikes $N!", ch, arrow, victim, TO_ROOM );
		act("$p strikes $n!", victim, arrow, NULL, TO_ROOM );
	   }
				
	   if (is_safe(ch,victim) || IS_SET(victim->imm_flags,IMM_ARROW)) {
		act("$p falls from $n doing no visible damage...",victim,arrow,NULL,TO_ALL);
		act("$p falls from $n doing no visible damage...",ch,arrow,NULL,TO_CHAR);
		obj_to_room(arrow,victim->in_room);
	   }
	   else {
		
		dam = dice(arrow->value[1],arrow->value[2]);
		dam = number_range(dam,2*dam);
		dam += damroll + bonus + 
		  (10 * str_app[get_curr_stat(ch,STAT_STR)].todam);
     	
		if (IS_WEAPON_STAT(arrow,WEAPON_POISON)) {
		  int level;
		  AFFECT_DATA *poison, af;
		  
		  if ((poison = affect_find(arrow->affected,gsn_poison)) == NULL)
		    level = arrow->level;
		  else
		    level = poison->level;
		  
		  if (!saves_spell(level,victim,DAM_POISON)) {
		    send_to_char("You feel poison coursing through your veins.", victim);
		    act("$n is poisoned by the venom on $p.",  victim,arrow,NULL,TO_ROOM);
		    
		    
		    af.where     = TO_AFFECTS;
		    af.casterId  = ch->id;
		    af.type      = gsn_poison;
		    af.level     = level * 3/4;
		    af.duration  = level / 2;
		    af.location  = APPLY_STR;
		    af.modifier  = -1;
		    af.bitvector = AFF_POISON;
		    affect_join( victim, &af );
		  }
		}
        	
		if (IS_WEAPON_STAT(arrow,WEAPON_FLAMING)) {
		  act("$n is burned by $p.",victim,arrow,NULL,TO_ROOM);
		  act("$p sears your flesh.",victim,arrow,NULL,TO_CHAR);
		  fire_effect( (void *) victim,arrow->level,dam,TARGET_CHAR);
		}
        	
		if (IS_WEAPON_STAT(arrow,WEAPON_FROST)) {
		  act("$p freezes $n.",victim,arrow,NULL,TO_ROOM);
		  act("The cold touch of $p surrounds you with ice.", victim,arrow,NULL,TO_CHAR);
		  cold_effect(victim,arrow->level,dam,TARGET_CHAR);
		}
					
		if (IS_WEAPON_STAT(arrow,WEAPON_SHOCKING)) {
		  act("$n is struck by lightning from $p.",victim,arrow,NULL,TO_ROOM);
		  act("You are shocked by $p.",victim,arrow,NULL,TO_CHAR);
		  shock_effect(victim,arrow->level,dam,TARGET_CHAR);
		}
		
		if ( dam > 50 && number_percent() < 80 )  {
		  af.where     = TO_AFFECTS;
		  af.casterId  = ch->id;
		  af.type      = sn;
		  af.level     = ch->level; 
		  af.duration  = -1;
		  af.location  = APPLY_HITROLL;
		  af.modifier  = - (dam );
		  
		  if (IS_NPC(victim)) 
		    af.bitvector = 0;
		  else 
						  /* af.bitvector = AFF_CORRUPTION; : put back later when more aff*/
		    af.bitvector = 0;
		  affect_join( victim, &af );
		  obj_to_char(arrow,victim);
		  if (!victim->arrow_count) {
		     equip_char(victim,arrow,WEAR_STUCK_IN);
		  }
		  victim->arrow_count++;
		  if (victim->arrow_count >= MAX_ARROWS_BEFORE_DEATH) {
			send_to_char("The last arrow hits you hard and you can take no more!\r\n",victim);
 	          if(IS_IMMORTAL(victim))
                     victim->hit=1;
                  else victim->hit=-15;
                     update_pos(victim);

                  }
		  /* Small chance of instant death */
   		   if (((get_skill(ch,gsn_bow) + get_skill(ch,gsn_arrow)) / 2      ) > 90 && number_percent() <= 5) 
		   {
 	               if(IS_IMMORTAL(victim))
                          victim->hit=1;
                       else victim->hit=-15;
                          update_pos(victim);
		   }

                   if(victim->position==POS_DEAD) {
                     group_gain(ch, victim, FALSE);
          
		  	/*** make them die ***/
		  	stop_fighting(victim,TRUE);
		  	raw_kill( victim );
	 		update_pos(victim);
	   		// Trolloc clan promotion by PK
	   		check_trolloc_kill(ch, victim);
	   		check_valid_pkill(ch,victim);

                     if (IS_AFFECTED(victim,AFF_CHANNELING)) {
                          do_function(victim, &do_unchannel, "" );
                     }
          
          
                     // Log the kill
                     if ( !IS_NPC(victim) ) {
                          sprintf(buf, "%s was shot down by %s at room %d [AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]",
                                     victim->name,
                                     (IS_NPC(ch) ? ch->short_descr : ch->name),
                                     ch->in_room->vnum,
                                     IS_SET(victim->comm,COMM_AFK) ? "Yes" : "No",
                                     victim->timer > 3             ? "Yes" : "No",
                                     victim->timer,
                                     IS_RP(ch)                     ? "Yes" : "No",
                                     IS_RP(victim)                 ? "Yes" : "No",
                                     position_table[victim->position].name);
          
                          log_string( buf );

		        if (!IS_NPC(ch))
            		{
               			sprintf(buf,"%s was killed (shot down) by %s at room %d\r\n%s - %s \r\n[AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]\r\n",
                    		victim->name,
                    		ch->name,
                    		ch->in_room->vnum,
                    		!IS_NULLSTR(ch->in_room->name) ? ch->in_room->name : "(none)",
                    		ch->in_room->area->name,
                    		IS_SET(victim->comm,COMM_AFK) ? "Yes" : "No",
                    		victim->timer > 3             ? "Yes" : "No",
                    		victim->timer,
                    		IS_RP(ch)                     ? "Yes" : "No",
                    		IS_RP(victim)                 ? "Yes" : "No",
                    		position_table[victim->position].name);
		
              			make_note("PK", ch->name, "Admin", "I'm playing at PK", 30, buf);
             		}

          
                          wiznet(log_buf,NULL,NULL,WIZ_MOBDEATHS,0,0);
                     }
          
          
                     /* RT new auto commands */
                     if ( !IS_NPC(ch) && IS_NPC(victim) ) {
                          corpse = get_obj_list( ch, "corpse", ch->in_room->contents );
          
                          if ( IS_SET(ch->act, PLR_AUTOEXAMINE) && corpse) {
                            do_function(ch, &do_examine, "corpse");
                          }
          
                          if ( IS_SET(ch->act, PLR_AUTOLOOT) &&
                                  corpse && corpse->contains) /* exists and not empty */
                            do_get( ch, "all corpse" );
          
                          if (IS_SET(ch->act,PLR_AUTOGOLD) &&
                              corpse && corpse->contains  && /*exists and not empty */
                              !IS_SET(ch->act,PLR_AUTOLOOT))
                            do_get(ch, "gold corpse");
          
                          if ( IS_SET(ch->act, PLR_AUTOSAC) ) {
                            if ( IS_SET(ch->act,PLR_AUTOLOOT) && corpse && corpse->contains)
                              return 0;  /* leave if corpse has treasure */
                            else
                              do_sacrifice( ch, "corpse" );
                          }
                     } /* if is_npc && is_npc */
                   } /* if dead */
		}
		else 
		  obj_to_room(arrow,victim->in_room); 
		
		damage( ch, victim,dam,sn,DAM_PIERCE,TRUE );
		
		if ( victim->position == POS_DEAD )
		  return 1;				
		
		if(IS_NPC(victim) && (number_percent() > 0 ) ) {
		  if (victim->position == POS_FIGHTING) {  
		    stop_fighting( victim, TRUE );
		  }
		  
		  do_function(victim, &do_hunt, ch->name );
		  
		  if( ch->in_room == victim->in_room ) {
		    (victim)->wait = UMAX((victim)->wait, (8));
		    act("$N scream and attack $n !!!",ch,NULL,victim,TO_NOTVICT);
		    act("$N scream and attack You !!!",ch,NULL,victim,TO_CHAR);
		    multi_hit( victim, ch, TYPE_UNDEFINED );
		  }
		}
		else  { 
		  switch(door)  { 
		    
		  case 0 : sprintf(buf,"The throw came from SOUTH !!!\n\r");
		    break;
		  case 1 : sprintf(buf,"The throw came from WEST !!!\n\r");
		    break;
		  case 2 : sprintf(buf,"The throw came from NORTH !!!\n\r");
		    break;
		  case 3 : sprintf(buf,"The throw came from EAST !!!\n\r");
		    break;
		  case 4 : sprintf(buf,"The throw came from BELOW !!!\n\r");
		    break;
		  case 5 : sprintf(buf,"The throw came from ABOVE !!!\n\r");
		    break;
		  }
		  send_to_char(buf,victim);
		}
	   }
	   check_improve(ch,gsn_arrow,TRUE,1);
	   return 1;
	 }
	 else  {
	   obj_to_room(arrow,victim->in_room);
	   act("$p sticks in the ground at your feet!",victim,arrow,NULL, TO_ALL );
	   check_improve(ch,gsn_arrow,FALSE,1);	   
	   return 0;
	 }
    }

    pExit = dest_room->exit[ door ];
    
    if ( !pExit ) 
	 break;
    else  {
	 dest_room = pExit->u1.to_room;
	 if ( dest_room->people ) {
	   sprintf(buf,"$p sails into the room from the %s!",dir_name[rev_dir[door]]);
	   act(buf, dest_room->people, arrow, NULL, TO_ALL );
	 }
    }    
  }
  check_improve(ch,gsn_arrow,FALSE,1);  
  return 0; 
}
	

void do_throw_spear( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *spear;
    char arg1[512],arg2[512],buf[512];
    bool success;
    int chance,direction;
    int range = (ch->level / 10) + 1;
    
   if ((ch->level > LEVEL_HERO) && (ch->level < LEVEL_ADMIN))
   {
  	send_to_char("Not at your level.\n\r",ch); 
        return;
   }
   if (IS_NPC(ch)) /* Mobs can't shoot spears */
	   return; 

   if ((chance = get_skill(ch,gsn_spear)) == 0) 
	{
	  send_to_char("You don't know how to throw a spear.\n\r",ch);
	  return;
	}

   argument=one_argument( argument, arg1 );
   one_argument( argument, arg2 );

   if ( arg1[0] == '\0' || arg2[0] == '\0')
    {
	send_to_char( "Throw the spear where?\n\r", ch );
	return;
    }

    if (ch->fighting)

    {
	send_to_char("You cannot concentrate on throwing spear.\n\r",ch);
	return;
    }

   direction = find_exit( ch, arg1 ); /*arg1 is the direction n,s,e,w, etc */

   if (direction<0 || direction > 5) 
	{
	 send_to_char("Throw which direction and whom?\n\r",ch);
	 return;
	}
	/*arg2 is the name of the target*/
    if ( ( victim = find_char( ch, arg2, direction, range) ) == NULL )
	return;

    if (!IS_NPC(victim) && victim->desc == NULL)
    {
	send_to_char("You can't do that.\n\r", ch);
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "That's pointless.\n\r", ch );
	return;
    }

    if (is_safe(ch,victim))
    {
		return;
    }

   spear = get_eq_char(ch, WEAR_WIELD);

   if (!spear || spear->item_type!=ITEM_WEAPON || spear->value[0]!=WEAPON_SPEAR)
   	{
	 send_to_char("You need a spear to throw!\n\r",ch);
	 return;    	
	}

   if (get_eq_char(ch,WEAR_SECOND_WIELD) || get_eq_char(ch,WEAR_SHIELD) )
    {
	 send_to_char("Your second hand should be free!\n\r",ch);
	 return;    	
	}

    
    WAIT_STATE( ch, skill_table[gsn_spear].beats );
   
    chance = (get_skill(ch,gsn_spear) - 50) * 2;
    if (ch->position == POS_SLEEPING)
	chance += 40;
    if (ch->position == POS_RESTING)
	chance += 10;
    if (victim->position == POS_FIGHTING)
	chance -= 40;
    chance += GET_HITROLL(ch);

    sprintf( buf, "You throw $p to the %s.", dir_name[ direction ] );
    act( buf, ch, spear, NULL, TO_CHAR );
    sprintf( buf, "$n throws $p to the %s.", dir_name[ direction ] );
    act( buf, ch, spear, NULL, TO_ROOM );

    obj_from_char(spear);
    success = send_arrow(ch,victim,spear,direction,chance,
	dice(spear->value[1],spear->value[2]) );
    check_improve(ch,gsn_spear,TRUE,1);
}
void do_throw( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    int chance;

   if ((ch->level > LEVEL_HERO) && (ch->level < LEVEL_ADMIN))
   {
  	send_to_char("Not at your level.\n\r",ch); 
        return;
   }
    if ( MOUNTED(ch) ) 
    {
        send_to_char("You can't throw while riding!\n\r", ch);
        return;
    }

    argument = one_argument(argument,arg);

    if (!str_cmp(arg,"spear"))
    {
     do_throw_spear(ch,argument);
     return;
    }

    if ( IS_NPC(ch) ||
         ch->level < skill_table[gsn_throw].skill_level[ch->class] )
    {
	send_to_char(
	    "A clutz like you couldn't throw down a worm.\n\r", ch );
	return;
    }

    if (IS_AFFECTED(ch,AFF_FLYING))
	{
	 send_to_char("Your feet should be on the ground for balance\n\r",ch);
	 return;
	}

    if ( ( victim = ch->fighting ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (is_safe(ch,victim))
      return;

    WAIT_STATE( ch, skill_table[gsn_throw].beats );


    chance = get_skill(ch,gsn_throw);

    if (IS_AFFECTED(victim,AFF_HASTE))
     {
     
	chance /= 2;

}
    if (IS_AFFECTED(ch,AFF_HASTE))
     {
     
	chance *= 1.5;

}
    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 10;
    else
	chance += (ch->size - victim->size) * 25; 


    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= get_curr_stat(victim,STAT_DEX) * 4/3;

    if (IS_AFFECTED(victim,AFF_FLYING) ) chance += 10;

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST))
	chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST))
	chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    if (ch->fighting != NULL && victim != ch->fighting) 
    {
	chance = (chance < 10 ? chance : UMIN(10,get_skill(ch,gsn_throw)/10));
    }

    if (number_percent() < chance )
    {
      act("You throw $N to the ground with stunning force.",
	  ch,NULL,victim,TO_CHAR);
      act("$n throws you to the ground with stunning force.",
	  ch,NULL,victim,TO_VICT);
      act("$n throws $N to the ground with stunning force.",
	  ch,NULL,victim,TO_NOTVICT);
      WAIT_STATE(victim,2 * PULSE_VIOLENCE);

      damage( ch, victim,ch->level + get_curr_stat(ch,STAT_STR), 
	     gsn_throw,DAM_BASH, TRUE );
      check_improve(ch,gsn_throw,TRUE,1);
      //ch->gain_xp = TRUE;
    }
    else
    {
	act( "You fail to grab your opponent.", ch, NULL, NULL, TO_CHAR);
	act( "$N tries to throw you, but fails.", victim, NULL, ch,TO_CHAR);
	act( "$n tries to grab $N's arm.", ch, NULL, victim, TO_NOTVICT);
	check_improve(ch,gsn_throw,FALSE,1);
    }

    return;
}




void do_shoot( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  OBJ_DATA *wield;
  OBJ_DATA *arrow; 
  char arg1[512],arg2[512],buf[512];
  bool success;
  int chance,direction;
  int range = (ch->level / 10) + 1;

  /* Mobs can't use bows */
  /*
  if (IS_NPC(ch)) 
    return;
  */
  
  if (!IS_NPC(ch) && ch->level < skill_table[gsn_bow].skill_level[ch->class]) {
    send_to_char("You don't know how to shoot.\n\r",ch);
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
  
  argument=one_argument( argument, arg1 );
  one_argument( argument, arg2 );
  
  if ( arg1[0] == '\0') {
    send_to_char( "Shoot what direction and whom?\n\r", ch );
    return;
  }

  if ( arg2[0] == '\0') {
    if (!(victim = get_char_room(ch, arg1)))  {
	 send_to_char("Shoot <direction> <target> OR shoot <target>\n\r",ch);
	 return;
    }
    direction = -1;
  }
  else {
    direction = find_exit( ch, arg1 );
    if (direction<0 || direction > 5)  {
	 send_to_char("Shoot which direction and whom?\n\r",ch);
	 return;
    }
    if ( ( victim = find_char( ch, arg2, direction, range) ) == NULL )
	 return;
  }

  if (!IS_NPC(victim) && victim->desc == NULL) {
    send_to_char("You can't do that.\n\r", ch);
    return;
  }
  
  if ( victim == ch ) {
    send_to_char( "That's pointless.\n\r", ch );
    return;
  }

  if (is_safe(ch,victim)) {
    sprintf(buf,"Gods protect %s.\n\r",victim->name);
    send_to_char(buf,ch);
    return;
  }

  wield = get_eq_char(ch, WEAR_WIELD);
  arrow = get_eq_char(ch, WEAR_HOLD);    
  
  if (!wield || wield->item_type!=ITEM_WEAPON || wield->value[0]!=WEAPON_BOW) {
    send_to_char("You need a bow to shoot!\n\r",ch);
    return;    	
  }

  if (get_eq_char(ch,WEAR_SECOND_WIELD) || get_eq_char(ch,WEAR_SHIELD) ) {
    send_to_char("Your second hand should be free!\n\r",ch);
    return;    	
  }
  
  if (!arrow) {
    send_to_char("You need an arrow held for your ammunition!\n\r",ch);
    return;    	
  }	
	
  if (arrow->item_type!=ITEM_WEAPON || arrow->value[0] != WEAPON_ARROW)  {
    send_to_char("That's not the right kind of arrow!\n\r",ch);
    return;
  }
	
  
  WAIT_STATE( ch, skill_table[gsn_bow].beats );
  
  chance = (get_skill(ch,gsn_bow));
  
  
  if (victim->position == POS_SLEEPING)
    chance += 50;
  if (victim->position == POS_RESTING)
    chance += 25;
  if (victim->position == POS_FIGHTING)
    chance -= 30;
  chance += GET_HITROLL(ch);
  
  if (direction >= 0) {
     sprintf( buf, "You shoot $p to %s.", dir_name[ direction ] );
     act( buf, ch, arrow, NULL, TO_CHAR );
     sprintf( buf, "$n shoots $p to %s.", dir_name[ direction ] );
     act( buf, ch, arrow, NULL, TO_ROOM );
  }
  
  obj_from_char(arrow);
  success = send_arrow(ch,victim,arrow,direction,chance, dice(wield->value[1],wield->value[2]) );
  check_improve(ch,gsn_bow,TRUE,1);
}

void do_mount( CHAR_DATA *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct char_data *mount;
  CHAR_DATA *old_mount=NULL;
  bool is_new_mount=FALSE;
  
  argument = one_argument(argument, arg);
  
  if (IS_NPC(ch)) 
    return;
  
  if (arg[0] == '\0' && ch->mount && ch->mount->in_room == ch->in_room) {
    mount = ch->mount;
  } 
  else if (arg[0] == '\0') {
    send_to_char("Mount what?\n\r", ch);
    return;
  }
  
  if (!(mount = get_char_room(ch, arg)))  {
    send_to_char("You don't see that here.\n\r", ch);
    return;
  }
  else {
      if (ch->mount && ch->mount != mount) {  	
        old_mount = ch->mount;        
        is_new_mount = TRUE;	  	
      }
  }
  
  if (!IS_NPC(ch) && ch->level < skill_table[gsn_riding].skill_level[ch->class]) {
    send_to_char("You don't know how to ride!\n\r", ch);
    return;
  } 
  
  if ( !IS_NPC(mount) || !IS_SET(mount->act,ACT_RIDEABLE)) { 
    send_to_char("You can't ride that.\n\r",ch); 
    return;
  }
  
  if (mount->level - 5 > ch->level) {
    send_to_char("That beast is too powerful for you to ride.", ch);
    return;
  }    
  
  if( (mount->mount) && (!mount->riding) && (mount->mount != ch)) {
    sprintf(buf, "%s belongs to someone, not you.\n\r", mount->short_descr);
    send_to_char(buf,ch);
    return;
  } 
  
  if (mount->position < POS_STANDING) {
    send_to_char("Your mount must be standing.\n\r", ch);
    return;
  }
  
  if (RIDDEN(mount)) {
    send_to_char("This beast is already ridden.\n\r", ch);
    return;
  } 
  else if (MOUNTED(ch)) {
    send_to_char("You are already riding.\n\r", ch);
    return;
  }
  
  if( !mount_success(ch, mount, TRUE) ) {
    send_to_char("You fail to mount the beast.\n\r", ch);  
    return; 
  }
  
  // New mount, no old
  if (ch->mount == NULL) {
    ch->mount = mount;
    add_follower( mount, ch );
    SET_BIT(mount->act, ACT_PET);
    SET_BIT(mount->affected_by, AFF_CHARM);
    mount->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
    mount->leader = ch;
  }
  
  // New mount, and still owner of old
  if (is_new_mount) {
     // get rid of old
     act("You let $N go free.", ch, NULL, old_mount, TO_CHAR);
     act("$n let $N go free.", ch, NULL, old_mount, TO_NOTVICT);
     act("$n let you go free.", ch, NULL, old_mount, TO_VICT);
     stop_follower(old_mount);     
     
     // set up new
     add_follower(mount, ch);
     SET_BIT(mount->act, ACT_PET);
     SET_BIT(mount->affected_by, AFF_CHARM);
     mount->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
     mount->leader = ch;     
  }  

  if (!ch->mount_quiet) {
    act("You hop on $N's back.", ch, NULL, mount, TO_CHAR);
    act("$n hops on $N's back.", ch, NULL, mount, TO_NOTVICT);
    act("$n hops on your back!", ch, NULL, mount, TO_VICT);
  }
  
  ch->mount = mount;
  ch->riding = TRUE;
  mount->mount = ch;
  mount->riding = TRUE;
  
  ch->mount_quiet = FALSE;
  
  /* No sneaky people on mounts */
  affect_strip(ch, gsn_sneak);
  REMOVE_BIT(ch->affected_by, AFF_SNEAK);
  affect_strip(ch, gsn_hide);
  REMOVE_BIT(ch->affected_by, AFF_HIDE);
  affect_strip(ch, gsn_invis);
  REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
}

void do_dismount( CHAR_DATA *ch, char *argument )
{
  struct char_data *mount;
  
  if(MOUNTED(ch))  {
    mount = MOUNTED(ch);
    
    act("You dismount from $N.", ch, NULL, mount, TO_CHAR);
    act("$n dismounts from $N.", ch, NULL, mount, TO_NOTVICT);
    act("$n dismounts from you.", ch, NULL, mount, TO_VICT);
    
    ch->riding = FALSE;
    mount->riding = FALSE;
  } 
  else {
    send_to_char("You aren't mounted.\n\r", ch);
    return;
  }
} 

void do_knock(CHAR_DATA *ch, char *argument)
{
  int door;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument,arg);

  if (arg[0] == '\0') {
   send_to_char("Knock on what?\n\r",ch);
   return;
  }

  if ( ( door = find_door( ch, arg ) ) >= 0 ) {
   ROOM_INDEX_DATA *to_room;
   EXIT_DATA *pexit;
   EXIT_DATA *pexit_rev;

   pexit = ch->in_room->exit[door];

   act( "$n knocks on the $d.", ch, NULL, pexit->keyword, TO_ROOM);
   act( "You knock on the $d.", ch, NULL, pexit->keyword, TO_CHAR);

   /* Notify the other side.  */
   if (   ( to_room   = pexit->u1.to_room            ) != NULL
       && ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
       && pexit_rev->u1.to_room == ch->in_room ) {
          CHAR_DATA *rch;
          for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
            act( "You hear someone knocking.", rch, NULL, pexit_rev->keyword, TO_CHAR);
        }
    }
   else
   {
	OBJ_DATA * obj;
	obj = get_obj_list (ch, argument, ch->in_room->contents);
        if (obj == NULL)
        {
		send_to_char("You don't see that here to knock on.\n\r",ch);
		return;
        }
	if (obj->item_type == ITEM_VEHICLE)
        {
    	    ROOM_INDEX_DATA *location;
	    location = get_room_index (obj->value[3]);
	    if (location != NULL)
            {
   		act( "$n knocks on $p.", ch, obj, NULL, TO_ROOM);
   		act( "You knock on $p.", ch, obj, NULL, TO_CHAR);
          	CHAR_DATA *rch;
          	for ( rch = location->people; rch != NULL; rch = rch->next_in_room )
            	act( "You hear someone knocking.", rch, NULL, NULL, TO_CHAR);
            }
	}
   }

  return;
}

// Call for your pet or mount
void do_call(CHAR_DATA *ch, char *argument)
{  
  //char buf[MAX_STRING_LENGTH];
  CHAR_DATA *called=NULL;

  if (IS_NPC(ch))
    return;

  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: call mount\n\r", ch);
    send_to_char("Syntax: call pet\n\r", ch);
    return;
  }
  
  if (!str_cmp( argument, "mount" )) {
    if (ch->mount == NULL) {
	 send_to_char("You don't have a mount to call for!\n\r", ch);
	 return;
    }
    called = ch->mount;
  }
  else if (!str_cmp( argument, "pet" )) {
    if (ch->pet == NULL) {
	 send_to_char("You don't have a pet to call for!\n\r", ch);
	 return;
    }
    called = ch->pet;
  }
  else {
    send_to_char("Syntax: call mount\n\r", ch);
    send_to_char("Syntax: call pet\n\r", ch);
    return;
  }
  
  if (ch->in_room == called->in_room) {
    act("There is no need to call for $N, $E is already here!", ch, NULL, called, TO_CHAR);
    return;
  }

  // Tell room what you do
  act("You {Bw{bh{Wist{bl{Be{x loudly for $N!", ch, NULL, called, TO_CHAR);
  act("$n {Bwh{bi{Wst{bl{Bes{x loudly, as if $e is calling for someone.", ch, NULL, NULL, TO_ROOM);

  // If called is bussy fighting...
  if (called->fighting) {
    return;
  }

  // Need to be skilled in riding
  if (ch->race == race_lookup("trolloc")) {
     if (number_percent() > ch->pcdata->learned[gsn_riding]/2)
       return;  	
  }
  else {
     if (number_percent() > ch->pcdata->learned[gsn_riding])
       return;
  }
  
  // Tell the mount/pet and message that mount/pet leave for master
  act("You arch your ears as you hear your master call for you!", called, NULL, NULL, TO_CHAR);
  if (called->in_room->people != NULL) {
    act("$n arch $s ears and suddenly leave for $s master!", called, NULL, NULL, TO_ROOM);	 
  }
  
  char_from_room(called);
  char_to_room(called, ch->in_room);
  
  // Arrival message
  act("$N arrives a short while after you {Bw{bh{Wist{bl{Be{x.", ch, NULL, called, TO_CHAR);
  act("$N arrives a short while after $n {Bwh{bi{Wst{bl{Bes{x.", ch, NULL, called, TO_ROOM);
  
  return;
}

// Retire a pet or mount
void do_retire(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *mob=NULL;

  if (IS_NPC(ch))
    return;

  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: retire mount\n\r", ch);
    send_to_char("Syntax: retire pet\n\r", ch);
    return;
  }

  if (!str_cmp( argument, "mount" )) {
    if (ch->mount == NULL) {
	 send_to_char("You don't have a mount to retire!\n\r", ch);
	 return;
    }
    mob = ch->mount;
  }
  else if (!str_cmp( argument, "pet" )) {
    if (ch->pet == NULL) {
	 send_to_char("You don't have a pet to retire!\n\r", ch);
	 return;
    }
    mob = ch->pet;
  }
  else {
    send_to_char("Syntax: retire mount\n\r", ch);
    send_to_char("Syntax: retire pet\n\r", ch);
    return;
  }

  if (ch->in_room != mob->in_room) {
    act("You need to call them $N before you can retire $E.", ch, NULL, mob, TO_CHAR);
    return;
  }

  if (mob->fighting) {
     act("$N is still fighting.", ch, NULL, mob, TO_CHAR);
	return;
  }

  act("You look at $N for a moment and retire it from your service.", ch, NULL, mob, TO_CHAR);

  act("$N leaves quietly.", ch, NULL, mob, TO_ROOM);
  act("$N leaves quietly.", ch, NULL, mob, TO_CHAR);

  extract_char(mob,TRUE,FALSE);

  return;
}

// Restring a pet
void do_prestring(CHAR_DATA *ch, char *argument) 
{
  CHAR_DATA *mob=NULL;
  char buf[MSL];
  char msg[MSL];
  char arg1 [MAX_INPUT_LENGTH];
  char arg2 [MAX_INPUT_LENGTH];
  int cost = 1000;
  int worth = 0;
  
  int learned=0;

  if (IS_NPC(ch))
    return;

  learned = get_level(ch);
  
  // Need to be level 25 to restring a pet
  if (learned <= 25) {
    send_to_char("You are not yet high enough level to restring your pet.\n\r", ch);    
    return;   	
  }

  if (ch->pet == NULL) {
    send_to_char("You don't have a pet to restring!\n\r", ch);
    return;
  }
  
  mob = ch->pet;
  
  smash_tilde( argument );
  argument = one_argument( argument, arg1 );
  memset(arg2, 0x00, sizeof(arg2));
  strcpy( arg2, argument );

  if ( arg1[0] == '\0' || (arg2[0] == '\0' && str_cmp( arg1, "desc" ))) {
    send_to_char("Syntax:\n\r",ch);
    send_to_char("  prestring <name> <field> <string>\n\r",ch);
    send_to_char("    fields: name short long desc\n\r",ch);
    return;
  }
  
  cost = get_level(mob) * 15;
  worth = ch->silver + ch->gold * 100;

  if (cost > worth) {
    sprintf(buf, "You don't have enough money to restring your pet.\n\r"
		  "It will cost you %d {Wsilver{x to restring your current pet.\n\r", cost);
    send_to_char(buf, ch);
    return;
  }

  if ( !str_prefix( arg1, "name" ) ) {
    
    // Length check
    if (strlen(arg2) < 3) {
	 send_to_char("Name field needs to be at least 3 characters long.\n\r", ch);
	 return;
    }
    
    free_string( mob->name );
    mob->name = str_dup( colorstrem( arg2 ) );
  }
  
  else if ( !str_prefix( arg1, "short" ) ) {
    
    // Length check
    if (strlen(arg2) < 8) {
	 send_to_char("Short field needs to be at least 8 characters long.\n\r", ch);
	 return;
    }
    
    sprintf(buf, "%s{x", arg2); // Make sure color is terminated
    
    free_string( mob->short_descr );
    mob->short_descr = str_dup( buf );
  }
  
  else if ( !str_prefix( arg1, "long" ) ) {
    
    // Length check
    if (strlen(arg2) < 8) {
	 send_to_char("Long field needs to be at least 8 characters long.\n\r", ch);
	 return;
    }
    
    if (arg2[strlen(arg2)] != '.')
	 sprintf(buf, "%s{x.\n\r", arg2); // Make sure color is terminated
    else
	 sprintf(buf, "%s{x\n\r", arg2); // Make sure color is terminated
    
    free_string( mob->long_descr );
    mob->long_descr = str_dup( buf );
  }
  
  else if ( !str_prefix( arg1, "desc" ) ) {
    string_append(ch, &mob->description);
  }
  
  else {
    send_to_char("That's not a valid Field.\n\r",ch);
    return;
  }
  
  deduct_cost(ch, cost);
  
  sprintf(msg, "You set the '%s' field on your pet to '%s' for %d {Wsilver{x.\n", arg1, arg2, cost);
  send_to_char(msg, ch);
  
  send_to_char("\n\rMake sure you understand the {yrules for restring{x in '{Whelp restring{x'.\n\r", ch);
  
  return;    
}

// Restring a mount
void do_mrestring(CHAR_DATA *ch, char *argument) 
{
  CHAR_DATA *mob=NULL;
  char buf[MSL];
  char msg[MSL];
  char arg1 [MAX_INPUT_LENGTH];
  char arg2 [MAX_INPUT_LENGTH];
  int cost = 1000;
  int worth = 0;

  int learned=0;
  
  if (IS_NPC(ch))
    return;

  learned = ch->pcdata->learned[gsn_riding];

  if (learned < 1) {
    send_to_char("You don't even know how to ride!\n\r", ch);
    return;
  }
  
  if (learned < 95) {
    send_to_char("You are not yet skilled enough to restring your mount.\n\r", ch);    
    return;   	
  }

  if (ch->mount == NULL) {
    send_to_char("You don't have a mount to restring!\n\r", ch);
    return;
  }
  
  mob = ch->mount;
  
  smash_tilde( argument );
  argument = one_argument( argument, arg1 );
  memset(arg2, 0x00, sizeof(arg2));
  strcpy( arg2, argument );
  
  if ( arg1[0] == '\0' || (arg2[0] == '\0' && str_cmp( arg1, "desc" ))) {
    send_to_char("Syntax:\n\r",ch);
    send_to_char("  mrestring <name> <field> <string>\n\r",ch);
    send_to_char("    fields: name short long desc\n\r",ch);
    return;
  }

  cost = get_level(mob) * 15;
  worth = ch->silver + ch->gold * 100;

  if (cost > worth) {
    sprintf(buf, "You don't have enough money to restring your mount.\n\r"
                 "It will cost you %d {Wsilver{x to restring your current mount.\n\r", cost);
    send_to_char(buf, ch);
    return;
  }
  
  if ( !str_prefix( arg1, "name" ) ) {

    // Length check
    if (strlen(arg2) < 3) {
	 send_to_char("Short field needs to be at least 3 characters long.\n\r", ch);
	 return;
    }

    free_string( mob->name );
    mob->name = str_dup( colorstrem( arg2 ) );
  }

  else if ( !str_prefix( arg1, "short" ) ) {
    
    // Length check
    if (strlen(arg2) < 8) {
	 send_to_char("Short field needs to be at least 8 characters long.\n\r", ch);
	 return;
    }

    sprintf(buf, "%s{x", arg2); // Make sure color is terminated
    
    free_string( mob->short_descr );
    mob->short_descr = str_dup( buf );
  }

  else if ( !str_prefix( arg1, "long" ) ) {

    // Length check
    if (strlen(arg2) < 8) {
	 send_to_char("Long field needs to be at least 8 characters long.\n\r", ch);
	 return;
    }

    if (arg2[strlen(arg2)] != '.')
	 sprintf(buf, "%s{x.\n\r", arg2); // Make sure color is terminated
    else
	 sprintf(buf, "%s{x\n\r", arg2); // Make sure color is terminated
    
    free_string( mob->long_descr );
    mob->long_descr = str_dup( buf );
  }

  else if ( !str_prefix( arg1, "desc" ) ) {
    string_append(ch, &mob->description);
  }
  
  else {
    send_to_char("That's not a valid Field.\n\r",ch);
    return;
  }
  
  deduct_cost(ch, cost);
  
  sprintf(msg, "You set the '%s' field on your mount to '%s' for %d {Wsilver{x.\n", arg1, arg2, cost);
  send_to_char(msg, ch);
  
  send_to_char("\n\rMake sure you understand the {yrules for restring{x in '{Whelp restring{x'.\n\r", ch);
  
  return;
}


CHAR_DATA *get_blocker(ROOM_INDEX_DATA *loc, int direction)
{
  CHAR_DATA *rch=NULL;
  ROOM_INDEX_DATA *to_location=NULL;
  EXIT_DATA *pexit=NULL;

  // Check in this room
  for ( rch = loc->people; rch != NULL; rch = rch->next_in_room ) 
  {
    if (rch->exit_block.direction == direction)
	 return rch;
  }

  // Else check in other side room
  if ((pexit = loc->exit[direction]) != NULL
	 && ( to_location = pexit->u1.to_room ) != NULL ) 
  {
    for ( rch = to_location->people; rch != NULL; rch = rch->next_in_room ) {
	 if (rch->exit_block.direction == rev_dir[direction])
	   return rch;
    }
  }
  
  return NULL;
}

void start_exit_block(CHAR_DATA *ch, int dir)
{
  ch->exit_block.vnum      = ch->in_room->vnum;
  ch->exit_block.direction = dir;
  ch->exit_block.blocking  = TRUE;
  return;
}

void stop_exit_block(CHAR_DATA *ch)
{  
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA       *victim=NULL;    // Victim if any at to location
  ROOM_INDEX_DATA *from_location=NULL;
  ROOM_INDEX_DATA *to_location=NULL;
  EXIT_DATA       *pexit=NULL;
  EXIT_DATA       *pexit_rev=NULL;
  int             direction;
  
  from_location = get_room_index(ch->exit_block.vnum);
  direction =  ch->exit_block.direction;
  
  if ((pexit = from_location->exit[direction]) != NULL
	 && ( to_location = pexit->u1.to_room ) != NULL ) {

    sprintf(buf, "You stop blocking the %s entrance.", dir_name[direction]);
    act(buf, ch, NULL, NULL, TO_CHAR);
    sprintf(buf, "$n stops blocking the %s entrance.", dir_name[direction]);
    act(buf, ch, NULL, NULL, TO_ROOM);

    /* If to location have players, get the first one */
    victim = to_location->people;
    
    if (victim != NULL) {
	 sprintf(buf, "$N stop blocking the %s entrance.", dir_name[rev_dir[direction]]);
	 act(buf, victim, NULL, ch, TO_ROOM);
	 act(buf, victim, NULL, ch, TO_CHAR);
    }
    
    if (IS_SET(pexit->exit_info, EX_BLOCKED))
	 REMOVE_BIT(pexit->exit_info, EX_BLOCKED);
    
    if ((pexit_rev = to_location->exit[rev_dir[direction]]) != 0) {
	 if (IS_SET(pexit_rev->exit_info, EX_BLOCKED))
	   REMOVE_BIT(pexit_rev->exit_info, EX_BLOCKED);
    }
  }
  
  ch->exit_block.vnum      = -1;
  ch->exit_block.direction = -1;
  ch->exit_block.blocking  = FALSE;   
}

void do_block(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA       *victim=NULL;    // Victim if any at to location
  ROOM_INDEX_DATA *from_location=NULL;
  ROOM_INDEX_DATA *to_location=NULL;
  EXIT_DATA       *pexit=NULL;
  EXIT_DATA       *pexit_rev=NULL;
  int             direction;  
  int             endurance = 0;
  int             sn = 0;
  int		  learned=0; 

   sn        = skill_lookup("block");
   endurance = skill_table[sn].min_endurance;  

  if (IS_NPC(ch))
  {
	learned = ch->level;
  }
  else 
  {
      if (ch->pcdata->learned[sn] < 1) {
         send_to_char("You don't know how to block.\r\n", ch);
         return;
      }
  }
  
  
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: block <direction>\n\r", ch);
    send_to_char("Syntax: block none\n\r", ch);
    return;
  }
  
  if (!IS_SET(ch->world,WORLD_NORMAL)) {
    send_to_char("Somehow you get a feeling doing that here will not be real...\n\r", ch);
    return;
  }

  if (!str_cmp(argument, "none")) {
    if (IS_BLOCKING(ch)) {
	 stop_exit_block(ch);
	 return;
    }
    else {
	 send_to_char("You are not blocking any exits.\n\r", ch);
	 return;
    }
  }

  if (IS_BLOCKING(ch)) {
    sprintf(buf, "Your focus is already directed toward the %s entrance.\n\r", dir_name[ch->exit_block.direction]);
    send_to_char(buf, ch);
    return;
  }
  
  if (ch->endurance < endurance) {
    send_to_char("You are too tired to concentrate or don't have enough endurance to focus!\n\r", ch);
    return;
  }  
  
  from_location = ch->in_room;

  if ((direction = find_exit(ch, argument)) != -1) {
    if ((pexit = from_location->exit[direction]) != NULL
	   && ( to_location = pexit->u1.to_room ) != NULL
	   && can_see_room(ch, pexit->u1.to_room)) {

 /*
	 if (IS_SET(pexit->exit_info, EX_CLOSED)) {
	   act("The $d is already closed.", ch, NULL, pexit->keyword, TO_CHAR);
	   return;
	 }
*/

	 if (IS_SET(pexit->exit_info, EX_BLOCKED)) {
	   act("Someone is already blocking the $d.", ch, NULL, pexit->keyword, TO_CHAR);
	   return;
	 }
	 
         if ( (!IS_NPC(ch)) && (number_percent( ) > ch->pcdata->learned[sn])) {
            sprintf(buf, "You try to block the %s entrance, but you fail.\n\r", dir_name[direction]);
            send_to_char(buf, ch );
            ch->endurance -= endurance/2;
            check_improve(ch,gsn_block,FALSE,1);
            return;
         }	 

	 // Tell
	 sprintf(buf, "You starts to block the %s entrance.", dir_name[direction]);
	 act(buf, ch, NULL, NULL, TO_CHAR);
	 sprintf(buf, "$n starts to block the %s entrance.", dir_name[direction]);
	 act(buf, ch, NULL, NULL, TO_ROOM);

	 /* If to location have players, get the first one */
	 victim = to_location->people;

	 /* Don't write message unless there are players in to_location */
	 if (victim != NULL) {
	   sprintf(buf, "$N starts to block the %s entrance.", dir_name[rev_dir[direction]]);
	   act(buf, victim, NULL, ch, TO_ROOM);
	   act(buf, victim, NULL, ch, TO_CHAR);
	 }
	 
	 /* Set exit to blocked */
	 if (!IS_SET(pexit->exit_info, EX_BLOCKED))
	   SET_BIT(pexit->exit_info, EX_BLOCKED);

	 /* Set other side exist info */
	 if ((pexit_rev = to_location->exit[rev_dir[direction]]) != 0) {
	   if (!IS_SET(pexit_rev->exit_info, EX_BLOCKED))
		SET_BIT(pexit_rev->exit_info, EX_BLOCKED);
	 }

	 start_exit_block(ch, direction);
	 check_improve(ch,gsn_block,TRUE,1);
	 ch->endurance -= endurance;
	 
	 return;
    }
    else {
	 send_to_char("You don't see exit in that direction.\n\r", ch);
	 return;
    }
  }
  else {
    send_to_char("You don't see exit in that direction.\n\r", ch);
    return;
  }
  
  return;  
}

void do_blocked(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA       *bch=NULL;
  ROOM_INDEX_DATA *from_location=NULL;
  ROOM_INDEX_DATA *to_location=NULL;
  EXIT_DATA       *pexit=NULL;
  int dir;
  bool found=FALSE;
  
  from_location = ch->in_room;
  
  for( dir = 0; dir <= 5; dir++ ) {
    if ((pexit = from_location->exit[dir]) != NULL
	   && ( to_location = pexit->u1.to_room ) != NULL
	   && can_see_room(ch, pexit->u1.to_room)) {
	 
	 if (IS_SET(pexit->exit_info, EX_BLOCKED)) {
	   found = TRUE;
	   bch = get_blocker(ch->in_room, dir);
	   sprintf(buf, "The %s entrance is blocked by $N.", dir_name[dir]);
	   act(buf, ch, NULL, bch, TO_CHAR);
	 }
    }
  }

  if (!found) {
    send_to_char("All exits are free!\n\r", ch);
  }

  return;
}

void do_arena( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  ROOM_INDEX_DATA *location=NULL;
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can go to the arena.\n\r",ch);
    return;
  }

  if (ch->fighting) {
     send_to_char("You are still fighting!\n\r", ch);
     return;
  }
  
  if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)) {
     send_to_char("The gods have forsaken you!\n\r", ch);
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
  
  if (IS_BLOCKING(ch)) {
    sprintf(buf, "You are still trying to block the %s entrance.\n\r", dir_name[ch->exit_block.direction]);
    send_to_char(buf, ch);
    return;  
  }
  
/*
  if (!IS_NPC(ch) && ch->pcdata->next_recall > current_time) {
       sprintf(buf, "You need to be in the room for %ld more seconds before you can recall.\n\r", (ch->pcdata->next_recall - current_time) < 0 ? 0 : (ch->pcdata->next_recall -  current_time));
       send_to_char(buf, ch);
       return;	
  }  
*/
  
  act( "$n disappears to the arena!", ch, 0, 0, TO_ROOM );
  
  if (!IS_NPC(ch))
  {
     if (argument[0] == '\0' || argument[0] == 'a' || argument[0] == 'A')
     {
        location = get_room_index( ROOM_VNUM_ARENA_A);
	ch->arena = 'a';
     }
     else
     if (argument[0] == '\0' || argument[0] == 'b' || argument[0] == 'B')
     {
        location = get_room_index( ROOM_VNUM_ARENA_B);
        ch->arena = 'b';
     }
     else
     if (argument[0] == '\0' || argument[0] == 'c' || argument[0] == 'C')
     {
        location = get_room_index( ROOM_VNUM_ARENA_C);
        ch->arena = 'c';
     }
     else
     if (argument[0] == '\0' || argument[0] == 'd' || argument[0] == 'D')
     {
        location = get_room_index( ROOM_VNUM_ARENA_D);
        ch->arena = 'd';
     }
     else {
	send_to_char("Valid choices are 'arena <a|b|c|d>'\r\n",ch);
	return;
     }
     // ######### PET RECALL #######
  }
  else if (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)) {
    if ( ( location = get_room_index( ch->master->in_room->vnum ) ) == NULL ) {
	 send_to_char( "You are completely lost.\n\r", ch );
	 send_to_char( "Your pet is hosed.\n\r", ch->master );
	 return;
    }
  }
  
  if ( ch->in_room == location ) {
    send_to_char("You are already in the arena!\n\r", ch);
    return;
  }
  
  ch->endurance /= 2;
  act( "$n heads to the arena.", ch, NULL, NULL, TO_ROOM );
  char_from_room( ch );
  char_to_room( ch, location );
  act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
  do_function(ch, &do_look, "auto" );
  
  if (ch->pet != NULL) {
    char_from_room(ch->pet);
    char_to_room(ch->pet,location);
  }
  else if (ch->mount != NULL && (ch->in_room == ch->mount->in_room)) {
    char_from_room(ch->mount);
    char_to_room(ch->mount,location);
    do_mount(ch, ch->mount->name);
  }
  
  return;
}

void do_dfrecall( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;
  ROOM_INDEX_DATA *location=NULL;
  
  if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET)) {
    send_to_char("Only players can recall.\n\r",ch);
    return;
  }

  if ((ch->pcdata->df_level < 0) && !IS_IMMORTAL(ch))
  {
       send_to_char("You aren't a friend of the dark.\n\r",ch);
       return;
  }

  if (!IS_HOODED(ch) && !IS_IMMORTAL(ch)) {
    send_to_char("You are not hooded.\n\r", ch);
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
    if (IS_SET(ch->act, PLR_GRADUATED)) {
	 if ( ( location = get_room_index( ROOM_VNUM_DFRECALL ) ) == NULL ) {
	   send_to_char( "You are completely lost.\n\r", ch );
	   return;
	 }
    }
    else {
	 if ( ( location = get_room_index( ROOM_VNUM_DFRECALL ) ) == NULL ) {
	   send_to_char( "You are completely lost.\n\r", ch );
	   return;
	 }
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
    send_to_char("You are already at your recall!\n\r", ch);
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
  else if (ch->mount != NULL && (ch->in_room == ch->mount->in_room)) {
    char_from_room(ch->mount);
    char_to_room(ch->mount,location);
    do_mount(ch, ch->mount->name);
  }
  
  return;
}
