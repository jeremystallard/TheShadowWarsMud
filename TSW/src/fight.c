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
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "tables.h"
#include "olc.h"

/*
 * Global variable
 */

/*
 * Local functions.
 */
void	check_assist	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	check_dodge	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	check_killer	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	check_parry	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool check_critical args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool    check_shield_block     args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void    dam_message 	args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
                            int dt, bool immune, int hit_location, int dam_type ) );
void	death_cry	args( ( CHAR_DATA *ch ) );
void	group_gain	args( ( CHAR_DATA *ch, CHAR_DATA *victim, bool bAssassinated) );
int	xp_compute	args( ( CHAR_DATA *gch, CHAR_DATA *victim, 
			    int total_levels ) );
bool	is_safe		args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	make_corpse	args( ( CHAR_DATA *ch ) );
void	make_fake_corpse	args( ( CHAR_DATA *ch ) );
void	one_hit		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt, int dual , long target) );
void    mob_hit		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
void	raw_kill	args( ( CHAR_DATA *victim ) );
void	set_fighting	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	disarm		args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
int     bsmod           args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool    check_valid_pkill args( (CHAR_DATA *ch, CHAR_DATA *victim) );
int sap_bonus( CHAR_DATA *ch );
bool gaze(CHAR_DATA *ch, CHAR_DATA *victim);

bool check_parse_name (char* name);  /* comm.c */
void reset_dead_char(CHAR_DATA *ch, char * argument, bool keep); /* handler.c */
int find_exit( CHAR_DATA *ch, char *arg );          /* act_move.c */
CHAR_DATA * get_target_group_member(CHAR_DATA *ch, CHAR_DATA * victim);


/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update( void )
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *victim;
    CHAR_DATA *guard;
    char buf[MAX_INPUT_LENGTH];

    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
	ch_next	= ch->next;

	if (IS_SUFFOCATING(ch) && !IS_AFFECTED(ch, AFF_SAP)) {
	  send_to_char( "You struggle for air, not being able to breathe.\r\n", ch );
	  act( "$n seems to be having trouble breathing.", ch, NULL, NULL, TO_ROOM);
	  damage(ch,ch,ch->max_hit/50,gsn_suffocate, DAM_SUFFOCATE,FALSE);
	}

	if ( ( victim = ch->fighting ) == NULL || ch->in_room == NULL )
	    continue;

	if ( IS_AWAKE(ch) && ch->in_room == victim->in_room ) {

    // Not hidden when a fight is starting
    if ( IS_AFFECTED( ch, AFF_HIDE )) {
       affect_strip ( ch, gsn_hide );
       REMOVE_BIT(ch->affected_by, AFF_HIDE);
       send_to_char( "You step out of the shadows.\r\n", ch );
       act( "$n steps out of the shadows.", ch, NULL, NULL, TO_ROOM);
    }
    
    if (IS_BLOCKING(ch))
       stop_exit_block(ch);
    
    // If hooded while fighting, chance to keep the hood up
    if (IS_HOODED(ch)) {
       if (ch->class == CLASS_THIEF && number_percent() < 2) {
          send_to_char("Your hood is pushed back as you fight.\r\n",ch);
          act("$n's hood is pushed back during the fight and reveals $s face.", ch, NULL, NULL, TO_ROOM);
          REMOVE_BIT(ch->app,APP_HOODED);    	
       }
       else if (ch->class != CLASS_THIEF && number_percent() < 10) {
       	  send_to_char("Your hood is pushed back as you fight.\r\n",ch);
          act("$n's hood is pushed back during the fight and reveals $s face.", ch, NULL, NULL, TO_ROOM);
          REMOVE_BIT(ch->app,APP_HOODED);
       }
    }
    		
	// If guarded, check if the one guarding will take blow	
	  if (!IS_NPC(victim) && IS_GUARDED(victim)) {
	  	guard = victim->pcdata->guarded_by;
	  	if ((guard != NULL) && 
	  	    (guard->in_room == victim->in_room) && 
	  	    (number_percent() < (guard->pcdata->learned[gsn_guard]-number_range(1,20)))) {

// --> Might want to add check for
//     weapon learned for attacker and defender and use as another level of checking
	  		
	  		// To you
	  		act("You defend $N against the blow.", guard, NULL, victim, TO_CHAR );	  		
	  		check_improve(guard,gsn_guard,TRUE,3);
	  		
	  		// To the one you guard against
	  		sprintf(buf, "%s manage to come between you and $N in a guard position!", PERS(guard, ch));
	  		act(buf, ch, NULL, victim, TO_CHAR );
	  		
	  		// To the one you are guarding
	  		sprintf(buf, "%s guards you against $N's attack!", PERS(guard, victim));
	  		act(buf, victim, NULL, ch, TO_CHAR );
	  		
	  		multi_hit( ch, guard, TYPE_UNDEFINED );	  			  		
	  	}
	  	else {
	  		multi_hit( ch, victim, TYPE_UNDEFINED );
	  	}
	  }
	  else
	     multi_hit( ch, victim, TYPE_UNDEFINED );
	}
	else
	  stop_fighting( ch, FALSE );

	if ( ( victim = ch->fighting ) == NULL )
	    continue;

	/*
	 * Fun for the whole family!
	 * if not in wait state
	 */
	if (ch->wait <= 0) 
	  check_assist(ch,victim);

	if ( IS_NPC( ch ) )
	{
	    if ( HAS_TRIGGER( ch, TRIG_FIGHT ) )		mp_percent_trigger( ch, victim, NULL, NULL, TRIG_FIGHT );
	    if ( HAS_TRIGGER( ch, TRIG_HPCNT ) )
		mp_hprct_trigger( ch, victim );
	}
    }

    return;
}

void do_assist(CHAR_DATA *ch, char * argument)
{
  CHAR_DATA *victim;
  char      arg[MAX_INPUT_LENGTH];
  
  one_argument(argument,arg);

  if (arg[0] == '\0' && ch->leader == NULL) {
    send_to_char ("Assist who?\n",ch);
    return;
  }

  if (arg[0] == '\0' && ch->leader != NULL && ch->leader->fighting == NULL) {
    send_to_char ("Assist who?\n",ch);
    return;
  }
   
  if ((victim = get_char_room(ch,arg)) == NULL) {
    send_to_char("They aren't here.\r\n",ch);
    return;
  }

  if (!IS_SAME_WORLD(victim, ch)) {
    send_to_char("They aren't here.\r\n",ch);
    return;
  }
    
  if (!victim->fighting)  {
    send_to_char("They aren't fighting anyone.\r\n",ch);
    return;
  }

  if (!IS_NPC(victim) && !is_same_group(ch,victim))
  {
	if (!IS_RP(ch) || !IS_RP(victim))
        {
		send_to_char("They may not want your assistance.  Why don't you ask if you can join them?\n\r",ch);
		return;
        }
  }
  
  if (IS_NPC(victim) && !is_same_group(ch,victim))
  {
	send_to_char("You can only assist mobiles that you are grouped with.\r\n",ch);
	return;
  }

  if (!IS_NPC(ch) && ch->pcdata->battlecry_voice) {
    act(ch->pcdata->battlecry_voice, ch, NULL, NULL, TO_ROOM);
    act(ch->pcdata->battlecry_voice, ch, NULL, NULL, TO_CHAR);
  }
  else
    do_function(ch, &do_emote, "screams and attacks!");
  
  //multi_hit(ch,victim->fighting,TYPE_UNDEFINED);
  multi_hit(ch,get_target_group_member(ch,victim->fighting),TYPE_UNDEFINED);

}

/* for auto assisting */
void check_assist(CHAR_DATA *ch,CHAR_DATA *victim)
{
  CHAR_DATA *rch, *rch_next;

  for (rch = ch->in_room->people; rch != NULL; rch = rch_next) {
    rch_next = rch->next_in_room;
    
    if (!IS_SAME_WORLD(rch, victim))
	 continue;
    
    if (IS_AWAKE(rch) && rch->fighting == NULL) {
	 
	 /* quick check for ASSIST_PLAYER */
	 if (!IS_NPC(ch) && IS_NPC(rch) 
		&& IS_SET(rch->off_flags,ASSIST_PLAYERS)
		&&  rch->level + 6 > victim->level)
	   {
		if (!IS_NPC(rch) && rch->pcdata->battlecry_voice) {
		  act(rch->pcdata->battlecry_voice, rch, NULL, NULL, TO_ROOM);
		  act(rch->pcdata->battlecry_voice, rch, NULL, NULL, TO_CHAR);
		}
		else
		  do_function(rch, &do_emote, "screams and attacks!");
		//multi_hit(rch,victim,TYPE_UNDEFINED);
		multi_hit(rch,get_target_group_member(rch,victim),TYPE_UNDEFINED);
		continue;
	   }

	 /* PCs next */
	 if (!IS_NPC(ch) || IS_AFFECTED(ch,AFF_CHARM)) {
	   if (((!IS_NPC(rch) && IS_SET(rch->act,PLR_AUTOASSIST))
		   || IS_AFFECTED(rch,AFF_CHARM)) 
		  && is_same_group(ch,rch) 
		  && !is_safe(rch, victim)) {
		if (rch->position < POS_STANDING) {
		  do_stand(rch,"");
		}
		//multi_hit (rch,victim,TYPE_UNDEFINED);
		multi_hit(rch,get_target_group_member(rch,victim),TYPE_UNDEFINED);
		
	   }
		
	   continue;
	 }
  	
	 /* now check the NPC cases */
	    
	 if (IS_NPC(ch) && !IS_AFFECTED(ch,AFF_CHARM)) {
	   if ( (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALL))		   
		   ||   (IS_NPC(rch) && rch->group && rch->group == ch->group)		   
		   ||   (IS_NPC(rch) && rch->race == ch->race 
			    && IS_SET(rch->off_flags,ASSIST_RACE))		   
		   ||   (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALIGN)
			    &&   ((IS_GOOD(rch)    && IS_GOOD(ch))
					||  (IS_EVIL(rch)    && IS_EVIL(ch))
					||  (IS_NEUTRAL(rch) && IS_NEUTRAL(ch)))) 
		   
		   ||   (rch->pIndexData == ch->pIndexData 
			    && IS_SET(rch->off_flags,ASSIST_VNUM)))
		
	   	{
		  CHAR_DATA *vch;
		  CHAR_DATA *target;
		  int number;
		  
		  if (number_bits(1) == 0)
		    continue;
		  
		  target = NULL;
		  number = 0;
		  for (vch = ch->in_room->people; vch; vch = vch->next) {
		    if (can_see(rch,vch)
			   &&  is_same_group(vch,victim)
			   &&  number_range(0,number) == 0) {
			 target = vch;
			 number++;
		    }
		  }
		  
		  if (target != NULL) {
		    if (!IS_NPC(rch) && rch->pcdata->battlecry_voice) {
			 act(rch->pcdata->battlecry_voice, rch, NULL, NULL, TO_ROOM);
			 act(rch->pcdata->battlecry_voice, rch, NULL, NULL, TO_CHAR);
		    }
		    else
			 do_function(rch, &do_emote, "screams and attacks!");
		    //multi_hit(rch,target,TYPE_UNDEFINED);
		    multi_hit(rch,get_target_group_member(rch,target),TYPE_UNDEFINED);
		  }
		}	
	 }
    }
  }
}


/*
 * Do one group of attacks.
 */
void multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
  int chance;
  int i=0;
  
  // decrement the wait and daze
  if (ch->desc == NULL) {
    ch->wait = UMAX(0,ch->wait - PULSE_VIOLENCE);
    ch->daze = UMAX(0,ch->daze - PULSE_VIOLENCE); 
  }
  
  // If still in a daze, no more action.. unless concentration merit
  if (ch->daze > 0) {
    if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)) {
	 act("{5You are in a {Ddaze{5, but manage to concentrate enough to regain your bearings{x.", ch, NULL, victim, TO_CHAR);
	 act("$n {5is in a {Ddaze{5, but manage to regain $s bearing{x.", ch, NULL, victim, TO_VICT);
	 act("$n {5is in a {Ddaze{5, but manage to regain $s bearing{x.", ch, NULL, victim, TO_NOTVICT);
    }
    else {
	 act("{5You are in a {Ddaze{5, struggling to regain your bearings{x.", ch, NULL, victim, TO_CHAR);
	 act("$n {5is in a {Ddaze{5, struggling to regain $s bearing{x.", ch, NULL, victim, TO_VICT);
	 act("$n {5is in a {Ddaze{5, struggling to regain $s bearing{x.", ch, NULL, victim, TO_NOTVICT);
    }
    //ch->gain_xp = TRUE; // Let em have exp even if in a daze
    return;
  }     
  
  /* no attacks for stunnies -- just a check */
  if (ch->position < POS_RESTING)
    return;
  
  // Auto Draw
  if (!IS_NPC(ch) && IS_SET(ch->auto_act, AUTO_DRAW)) {
    if (get_eq_char(ch, WEAR_SCABBARD_1) != NULL ||
	   get_eq_char(ch, WEAR_SCABBARD_2) != NULL)
	 if (get_eq_char(ch, WEAR_WIELD) == NULL)
	   do_draw(ch, "");
  }
  
  if (IS_NPC(ch)){
    mob_hit(ch,victim,dt);
    return;
  }
  
  one_hit( ch, victim, dt, FALSE, 0);
  
  /*
  if (!IS_NPC(ch))
    ch->gain_xp = TRUE;
  */
  
  // first attack for dual wield
  // ---------------------------
  if (get_eq_char(ch , WEAR_SECOND_WIELD)) {
    one_hit(ch , victim , dt, TRUE, 0);
    check_improve(ch, gsn_dual_wield,TRUE,2);
  }
  // Hand-to-hand chance of attack
  else {
    if (!get_eq_char(ch, WEAR_WIELD) && number_percent() <= get_skill(ch, gsn_hand_to_hand)/2) {
	 one_hit(ch , victim , dt, TRUE, 0);
	 check_improve(ch, gsn_hand_to_hand,TRUE,2);
    }
  }
    
  if (ch->fighting != victim)
    return;
  
  if (IS_AFFECTED(ch,AFF_HASTE)) {
    one_hit(ch,victim,dt, FALSE, 0);
    one_hit(ch,victim,dt, TRUE, 0);
  }
  if ( ch->fighting != victim || dt == gsn_backstab )
    return;
  
  chance = get_skill(ch,gsn_second_attack)/2;
  
  if (IS_AFFECTED(ch,AFF_SLOW))
    chance /= 2;
  
  if ( number_percent( ) < chance ) {
    one_hit( ch, victim, dt, FALSE, 0);
    check_improve(ch,gsn_second_attack,TRUE,5);
    if ( ch->fighting != victim )
	 return;
  }
  
  // second attack for dual wield 
  // ----------------------------
  chance = get_skill(ch,gsn_second_attack) / 3;
  if ( number_percent( ) < chance ) {
    if (get_eq_char(ch , WEAR_SECOND_WIELD)) {
	 one_hit(ch , victim , dt, TRUE, 0);
	 check_improve(ch, gsn_second_attack,TRUE,2);
	 if (ch->fighting != victim )
	   return;
    }
  }
  
  chance = get_skill(ch,gsn_third_attack)/4;
  
  if (IS_AFFECTED(ch,AFF_SLOW))
    chance = 0;
  
  if ( number_percent( ) < chance ) {
    one_hit( ch, victim, dt, FALSE, 0);
    check_improve(ch,gsn_third_attack,TRUE,6);
    if ( ch->fighting != victim )
	 return;
  }
  
  // third attack for dual wield
  // ---------------------------  
  chance = get_skill(ch,gsn_third_attack) / 5;
  if (IS_AFFECTED(ch,AFF_SLOW))
    chance = 0;
  if ( number_percent( ) < chance ) {
    if (get_eq_char(ch , WEAR_SECOND_WIELD)) {
	 one_hit(ch , victim , dt, TRUE, 0);
	 check_improve(ch, gsn_third_attack,TRUE,6);
	 if (ch->fighting != victim )
	   return;
    }
  }

  chance = get_skill(ch,gsn_fourth_attack)/4;

  if (IS_AFFECTED(ch,AFF_SLOW))
    chance = 0;

  if ( number_percent( ) < chance ) {
    one_hit( ch, victim, dt, FALSE, 0);
    check_improve(ch,gsn_fourth_attack,TRUE,6);
    if ( ch->fighting != victim )
         return;
  }

  chance = get_skill(ch,gsn_fifth_attack)/4;

  if (IS_AFFECTED(ch,AFF_SLOW))
    chance = 0;

  if ( number_percent( ) < chance ) {
    one_hit( ch, victim, dt, FALSE, 0);
    check_improve(ch,gsn_fifth_attack,TRUE,6);
    if ( ch->fighting != victim )
         return;
  }


  //sixth attack chance if sixth attack is above 150
  if (get_skill(ch,gsn_fifth_attack) > 150)
  {
     chance = get_skill(ch,gsn_fifth_attack)/4;
  
     if (IS_AFFECTED(ch,AFF_SLOW))
     	chance = 0;

    if ( number_percent( ) < chance ) {
       one_hit( ch, victim, dt, FALSE, 0);
       check_improve(ch,gsn_fifth_attack,TRUE,6);
       if ( ch->fighting != victim )
            return;
    }

  }

  // third attack for dual wield
  // ---------------------------
  chance = get_skill(ch,gsn_third_attack) / 5;
  if (IS_AFFECTED(ch,AFF_SLOW))
    chance = 0;
  if ( number_percent( ) < chance ) {
    if (get_eq_char(ch , WEAR_SECOND_WIELD)) {
         one_hit(ch , victim , dt, TRUE, 0);
         check_improve(ch, gsn_third_attack,TRUE,6);
         if (ch->fighting != victim )
           return;
    }
  }

  // Using a staff, you can get extra hit
  // ------------------------------------
  chance = get_skill(ch, gsn_staff) / 2;
  if (number_percent() < chance) {
    OBJ_DATA *wield=NULL;    
    wield = get_eq_char( ch, WEAR_WIELD );
    if (wield != NULL && wield->item_type == ITEM_WEAPON && wield->value[0] == WEAPON_STAFF) {
	 if (number_percent() < 25) {
	   act("You {Btwirls{x $p around in a {Rseries of attacks towards{x $N.", ch, wield, victim, TO_CHAR);
	   act("$n {Btwirls{x $p around in a {Rseries of attacks towards{x you!", ch, wield, victim, TO_VICT);
	   act("$n {Btwirls{x $p around in a {Rseries of attacks towards{x $N", ch, wield, victim, TO_NOTVICT);
	   chance = number_range(1, (get_skill(ch, gsn_staff) / 8));
	   for (i=0; i<chance; i++) {
		one_hit(ch , victim , dt, TRUE, 0);
	   }
	 }
	 else
	   one_hit(ch , victim , dt, TRUE, 0);

	 check_improve(ch, gsn_staff,TRUE,6);
	 if (ch->fighting != victim )
	   return;
    }
  }
  
    
  return;
}

/* procedure for all mobile attacks */
void mob_hit (CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
    int chance,number;
    CHAR_DATA *vch, *vch_next;

    one_hit(ch,victim,dt, FALSE, 0);

    if (ch->fighting != victim)
	return;

    /* Area attack -- BALLS nasty! */
 
    if (IS_SET(ch->off_flags,OFF_AREA_ATTACK))
    {
	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
	    vch_next = vch->next;
	    if ((vch != victim && vch->fighting == ch))
		one_hit(ch,vch,dt, FALSE, 0);
	}
    }

    if (IS_AFFECTED(ch,AFF_HASTE) 
    ||  (IS_SET(ch->off_flags,OFF_FAST) && !IS_AFFECTED(ch,AFF_SLOW)))
	one_hit(ch,victim,dt,FALSE, 0);

    if (ch->fighting != victim || dt == gsn_backstab)
	return;

    chance = get_skill(ch,gsn_second_attack)/2;

    if (IS_AFFECTED(ch,AFF_SLOW) && !IS_SET(ch->off_flags,OFF_FAST))
	chance /= 2;

    if (number_percent() < chance)
    {
	one_hit(ch,victim,dt, FALSE, 0);
	if (ch->fighting != victim)
	    return;
    }

    chance = get_skill(ch,gsn_third_attack)/4;

    if (IS_AFFECTED(ch,AFF_SLOW) && !IS_SET(ch->off_flags,OFF_FAST))
	chance = 0;

    if (number_percent() < chance)
    {
	one_hit(ch,victim,dt,FALSE, 0);
	if (ch->fighting != victim)
	    return;
    } 

    /* oh boy!  Fun stuff! */

    if (ch->wait > 0)
	return;

    number = number_range(0,2);


    if (number == 2 && IS_SET(ch->act,ACT_CLERIC))
    {	
	/* { mob_cast_cleric(ch,victim); return; } */ ;
    }

    /* now for the skills */

    number = number_range(0,8);

    switch(number) 
    {
    case (0) :
	if (IS_SET(ch->off_flags,OFF_BASH))
	    do_function(ch, &do_bash, "");
	break;

    case (1) :
	if (IS_SET(ch->off_flags,OFF_BERSERK) && !IS_AFFECTED(ch,AFF_BERSERK))
	    do_function(ch, &do_berserk, "");
	break;


    case (2) :
	if (IS_SET(ch->off_flags,OFF_DISARM) 
	|| (get_weapon_sn(ch) != gsn_hand_to_hand 
	&& (IS_SET(ch->act,ACT_WARRIOR)
   	||  IS_SET(ch->act,ACT_THIEF))))
	    do_function(ch, &do_disarm, "");
	break;

    case (3) :
	if (IS_SET(ch->off_flags,OFF_KICK))
	    do_function(ch, &do_kick, "");
	break;

    case (4) :
	if (IS_SET(ch->off_flags,OFF_KICK_DIRT))
	    do_function(ch, &do_dirt, "");
	break;

    case (5) :
	if (IS_SET(ch->off_flags,OFF_TAIL))
	{
	    /* do_function(ch, &do_tail, "") */ ;
	}
	break; 

    case (6) :
	if (IS_SET(ch->off_flags,OFF_TRIP))
	    do_function(ch, &do_trip, "");
	break;

    case (7) :	if (IS_SET(ch->off_flags,OFF_CRUSH))
	{
	    /* do_function(ch, &do_crush, "") */ ;
	}
	break;
    
	case (8) :
	if (IS_SET(ch->off_flags,OFF_BACKSTAB))
	{
	    do_function(ch, &do_backstab, "");
	}
    }
}

// Check if CH can do bm with current weapons
bool check_bm(CHAR_DATA *ch)
{
  OBJ_DATA *obj_wield;

  // If not wielding anything
  if ((obj_wield = get_eq_char(ch, WEAR_WIELD)) == NULL) {
    return FALSE;
  }
  
  // If what wielding isn't a sword
  if (obj_wield->value[0] != WEAPON_SWORD) {
    return FALSE;
  }
  
  // If wielding a shield also
  if (get_eq_char(ch, WEAR_SHIELD) != NULL) {
    return FALSE;
  }

  // Only with 1 weapon unless ambidex merit
  if (get_eq_char(ch, WEAR_SECOND_WIELD) != NULL && !IS_SET(ch->merits, MERIT_AMBIDEXTROUS)) {
    return FALSE;
  }
  
  return TRUE;
}	

/*
 * Hit one guy once.
 */
void one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt, int dual, long target )
{
  CHAR_DATA *vch=NULL;
  CHAR_DATA *vch_next=NULL;
  OBJ_DATA *wield=NULL;
  OBJ_DATA *v_wield=NULL;
  char mf_attackstring[MAX_STRING_LENGTH];
  char mf_defendstring[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  int victim_ac;
  int thac0;
  int thac0_00;
  int thac0_32;
  int dam;
  int diceroll;
  int sn,skill;
  int dam_type;
  int mf_skill;
  int mf_skill_victim;
  int mf_gsn;
  int tw_skill;
  int tw_skill_victim;
  bool mf_parried = FALSE;
  bool mf_attack = FALSE;
  int number;
  //int i;
  bool result;
  AFFECT_DATA af;
  sn = -1;
  int end_cost=0;

	
  /* just in case */
  if (victim == ch || ch == NULL || victim == NULL)
    return;
  
  /*
   * Can't beat a dead char!
   * Guard against weird room-leavings.
   */
  if ( victim->position == POS_DEAD || 
	  ch->in_room != victim->in_room ||
	  ( !IS_NPC(victim) && IS_AFFECTED(victim,AFF_SAP )))
    return;

  if (!IS_NPC(ch) && !IS_NPC(victim))
    ch->pcdata->next_quit = current_time + 120;
  
  /*
   * Master Form check 
   */
  
  if (dt == find_relevant_masterform(ch)/* && check_mf(ch,dt)*/ ) { 
    mf_gsn = dt;
    mf_attack = TRUE;
    dt = TYPE_UNDEFINED;
  }
  
  
  /*
   * Figure out the type of damage message.
   */
  if (dual)
    wield = get_eq_char( ch, WEAR_SECOND_WIELD);
  else
    wield = get_eq_char( ch, WEAR_WIELD );
  
  if ( dt == TYPE_UNDEFINED ) {
    dt = TYPE_HIT;
    if ( wield != NULL && wield->item_type == ITEM_WEAPON )
	 dt += wield->value[3];
    else 
	 dt += ch->dam_type;
  }
  
  if (dt < TYPE_HIT)
    if (wield != NULL)
	 dam_type = attack_table[wield->value[3]].damage;
    else
	 dam_type = attack_table[ch->dam_type].damage;
  else
    dam_type = attack_table[dt - TYPE_HIT].damage;
  
  if (dam_type == -1)
    dam_type = DAM_BASH;
  
  /* get the weapon skill */
  sn = get_weapon_sn(ch);
  skill = 20 + get_weapon_skill(ch,sn);
  
  
  /*
   * Check to see if too tired.
   * It takes endurance to swing
   */
  if (!IS_NPC(ch)) {
    if ( ch->endurance < 1 ) {
	 ch->endurance = 0;
	 send_to_char("{WYou're too {rTired{x!!!!\r\n", ch);
	 return;
    }
    else if ( wield == NULL || wield->item_type != ITEM_WEAPON ) {
	 end_cost = 1;

	 if (IS_SET(ch->merits, MERIT_HUGESIZE))
	   end_cost = end_cost * 1.5;
	 
	 ch->endurance -= end_cost;
    }
    else
	 switch (wield->value[0]) {
	 case(WEAPON_SWORD):   
	   end_cost = 3;	
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_DAGGER):  
	   end_cost = 2;		
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_SPEAR):   
	   end_cost = 3;	
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_MACE):   
	   end_cost = 3;		
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_AXE):     
	   end_cost = 3;		
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_FLAIL):   
	   end_cost = 3;		
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_WHIP):    
	   end_cost = 3;		
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_POLEARM): 
	   end_cost = 5;		
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_BOW):     
	   end_cost = 3;		
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_STAFF):   
	   end_cost = 4;		
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 case(WEAPON_LANCE):   
	   end_cost = 5;      
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 default :             
	   end_cost = 3;		
	   if (IS_SET(ch->merits, MERIT_HUGESIZE))
		end_cost = end_cost * 1.5;
	   break;
	 }
    
    ch->endurance -= end_cost;
  }
  
  /*
   * Calculate to-hit-armor-class-0 versus armor.
   */
  if ( IS_NPC(ch) ) {
    thac0_00 = 20;
    thac0_32 = -4;   /* as good as a thief */ 
    if (IS_SET(ch->act,ACT_WARRIOR))
	 thac0_32 = -10;
    else if (IS_SET(ch->act,ACT_THIEF))
	 thac0_32 = -4;
    else if (IS_SET(ch->act,ACT_CLERIC))
	 thac0_32 = 2;
  }
  else {
    thac0_00 = class_table[ch->class].thac0_00;
    thac0_32 = class_table[ch->class].thac0_32;
  }
  
  thac0  = interpolate( ch->level, thac0_00, thac0_32 );
  
  if (thac0 < 0)
    thac0 = thac0/2;
  
  if (thac0 < -5)
    thac0 = -5 + (thac0 + 5) / 2;
  
  thac0 -= GET_HITROLL(ch) * skill/100;
  thac0 += 5 * (100 - skill) / 100;
  
  if (dt == gsn_backstab)
    thac0 -= 10 * (100 - get_skill(ch,gsn_backstab > 100 ? 100 : get_skill(ch,gsn_backstab)));
  
  if (dt == gsn_circle)
    thac0 -= 10 * (100 - get_skill(ch,gsn_circle) > 100 ? 100 : get_skill(ch,gsn_circle));
  
  if(dt == gsn_whirlwind)
	  thac0 += ((100 - get_skill(ch, gsn_whirlwind)) > 0 ? 100 - get_skill(ch, gsn_whirlwind) : 0);


  switch(dam_type) {
  case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;	break;
  case(DAM_BASH):	 victim_ac = GET_AC(victim,AC_BASH)/10;		break;
  case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;	break;
  default:	 victim_ac = GET_AC(victim,AC_EXOTIC)/10;	break;
  };

  /* Armor use */
  if (!IS_NPC(victim) && get_skill( victim, gsn_armor_use ) > 0 ) {
    victim_ac += victim_ac * ((get_skill( victim, gsn_armor_use ) / 2) / 80);
    if (number_percent() > 95)
	 check_improve( victim, gsn_armor_use, TRUE, 2 );
  }
  
  if (victim_ac < -15)
    victim_ac = (victim_ac + 15) / 5 - 15;
  
  if ( !can_see( ch, victim ) )
    victim_ac -= 4;
  
  if ( victim->position < POS_FIGHTING)
    victim_ac += 4;
  
  if (victim->position < POS_RESTING)
    victim_ac += 6;
  
  /*
   * The moment of excitement!
   */
  while ( ( diceroll = number_bits( 5 ) ) >= 20 )
    ;

/* This is for debugging
    if (!IS_NPC(ch)) {
	 printf("\n## diceroll: %d\nthac: %d\nvictim_ac: %d\n", diceroll, thac0, victim_ac);
	 printf("## %d < %d\n", diceroll, thac0 - victim_ac);
    }
*/
  if (diceroll == 0 || ( diceroll != 19 && diceroll < thac0 - victim_ac )){

/* This is for debugging
	 if (!IS_NPC(ch))
	   printf("-=- MISS -=-\n");
*/

	 /* Miss. */
    damage( ch, victim, 0, dt, dam_type, TRUE );
    tail_chain( );
    return;
  }

  /*
   * Hit.
   * Calc damage.
   */
  if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL)) {
    if (!ch->pIndexData->new_format) {
	 dam = number_range( ch->level / 2, ch->level * 3 / 2 );
	 if ( wield != NULL )
	   dam += dam / 2;
    }
    else
	 dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
  }
  
  /* for players: */
  
  else {
    if (sn != -1) {
	 check_improve(ch,sn,TRUE,5);
    }
    if ( wield != NULL ) {
	 if (wield->pIndexData->new_format)
	   dam = dice(wield->value[1],wield->value[2]) * skill/100;
	 else
	   dam = number_range( wield->value[1] * skill/100, 
					   wield->value[2] * skill/100);
	 
	 if (get_eq_char(ch,WEAR_SHIELD) == NULL)  /* no shield = more */
	   dam = (dam * 11)/10;
	 
	 /* sharpness! */
	 if (IS_WEAPON_STAT(wield,WEAPON_SHARP)) {
	   int percent;
	   
	   if ((percent = number_percent()) <= (skill / 8))
		dam = 2 * dam + (dam * 2 * percent / 100);
	 }
    }
    else
	 dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
  }

  int strmod = get_curr_stat(ch,STAT_STR);

  dam = (dam * strmod) / 20;    //Adjust for strength value

  if (strmod > 25) //If the person spent extra effort to be extra strong
    dam = (dam * 5) / 4;

  
  /*
   * Bonuses.
   * Enhanced damage
   */
  if ( get_skill(ch,gsn_enhanced_damage) > 0 ) {
    diceroll = number_percent();
    if (diceroll <= get_skill(ch,gsn_enhanced_damage)) {
	 check_improve(ch,gsn_enhanced_damage,TRUE,6);
	 
	 if (ch->class == CLASS_WARRIOR && wield) {
	   dam = dam * number_range(1,3);
	 }
	 else if (ch->race == race_lookup("trolloc") && wield)
	   dam = dam * number_range(1,3);
	 else 
	   dam += 2 * ( dam * diceroll/300);
    }
  }  
    
  /*
   * Bonuses.
   * Critical strike
   */
  if (check_critical(ch,victim)) {
    dam_type = DAM_CRIT;
    if (number_percent() > 90)
	 check_improve(ch,gsn_critical,TRUE,3);
  }    

  /*
   * Bonuses.
   * Fade's hit a little harder, sometimes
   */
  if (ch->race == race_lookup("fade")) {
    if (number_percent() < 40 )
      dam = dam * number_range(1,3);
  }
  
  /*
   * Bonuses.
   * Trolloc's hit a little harder, sometimes
   */
  if (ch->race == race_lookup("trolloc")) {
    if (number_percent() < 30 )
      dam = dam * number_range(1,2);
  }

  /*
   * Bonuses.
   * Hugesize makes you hit harder, sometimes
   */
  if (IS_SET(ch->merits, MERIT_HUGESIZE)) {
    if (number_percent() < 30)
	 dam = dam * number_range(1,2);
  }
  
  /*
   * Whirlwind attack
   * We can disregard the extra if dt=gsn_whirlwind
   */
  if(dt == gsn_whirlwind){
	  if ( !IS_AWAKE(victim) )
		  dam *= 2;
	  else if (victim->position < POS_FIGHTING)
	      dam = dam * 3 / 2;

	  dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

	  if ( dam <= 0 )
		  dam = 1;

	  result = damage( ch, victim, dam, dt, dam_type, TRUE );
	  if(number_percent() >= 50)
		  DAZE_STATE(victim, number_range(1,2));

	  tail_chain( );
	    return;
  }

  /* -------------- */
  /* TrollocWarfare */
  /* -------------- */
  /* HeadButt, ShoulderBash, Elbow, Bite, Knee, Uppercut */
  
  tw_skill = get_skill(ch,gsn_trollocwarfare);
  tw_skill_victim = get_skill(victim, gsn_trollocwarfare); 
  
  if (!(victim->race == race_lookup("trolloc")))
    tw_skill_victim = 0;
  
  if (!(ch->race == race_lookup("trolloc")))
    tw_skill = 0;
  
  if ( (tw_skill) && (IS_SET(ch->auto_act, AUTO_MASTERFORMS)) && (dt != gsn_circle) && (dt != gsn_backstab) && (dt != gsn_trip)) {
    diceroll = number_percent();
    
    number = number_range (0,8); // change to 12 if extra forms is going in.
    
    if (!IS_SET(victim->auto_act, AUTO_MASTERFORMS))
	 tw_skill_victim = 0;
    
    switch(number) {	 
    case (0) : //headbutt
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, uppercut with elbow
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n tries to {Rheadbutt{x you.{x",ch,NULL,victim,TO_VICT);
		act("{wYou rear up to {Rheadbutt{x the opponent.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n rears up to {Rheadbutt{x $N {x",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n grabs you by the scruff and {Ruppercuts{x you with the elbow!{x",victim,NULL,ch,TO_VICT);
		act("{wYou grab $N by the scruff and {Ruppercut{x $M with the elbow!{x",victim,NULL,ch,TO_CHAR);
		act("{w$n grabs $N by the scruff and {Ruppercuts{x $M with the elbow!{x",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, 1042, dual, TARGET_HEAD);
		return;
		
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n tries to {Rheadbutt{x you.{x",ch,NULL,victim,TO_VICT);
		act("{wYou rear up to {Rheadbutt{x the opponent.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n rears up to {Rheadbutt{x $N {x",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n grabs you by the scruff and {Rshoves{x you aside.{x",victim,NULL,ch,TO_VICT);
		act("{wYou grab $N by the scruff and {Rshove{x $M aside.{x",victim,NULL,ch,TO_CHAR);
		act("{w$n grabs $N by the scruff and {Rshoves{x $M aside.{x",victim,NULL,ch,TO_NOTVICT);	 
		return;
	   }
	   //takes it        
	   else {
		act("{w$n rears up and {Rsmashes $s head{x into your face!{x",ch,NULL,victim,TO_VICT);
		act("{wYou rear up and {Rsmash your head{x into $S face!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n rears up and {Rheadbutts{x $N!{x",ch,NULL,victim,TO_NOTVICT);
		
		//dam += ( dam * diceroll/40);	  
		//dam_type = DAM_TW;
		//dt = 1041;
		dam += dam * 1.7;
		dam_type = DAM_CRIT;
	   }
	 }
	 break;

    case (1) : //shoulderbash
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, trip
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n tries to bash you with $s {Rshoulder{x.",ch,NULL,victim,TO_VICT);
		act("{wYou take a step back and {Rcharge{x at $N with your {Rshoulder{x.",ch,NULL,victim,TO_CHAR);
		act("{w$n takes a step back and {Rcharges{x at $N{x",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n moves aside and {Rtrips you to the ground{x!",victim,NULL,ch,TO_VICT);
		act("{wYou step aside and {Rtrip $N as $E runs by{x!",victim,NULL,ch,TO_CHAR);
		act("{w$n steps aside and {Rtrips $N as $E runs by{x!{",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, gsn_trip, dual, 0);
		return;	
	   }
	   //defends
	   else 
		if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		  act("{w$n tries to {Rbash you with $s shoulder{x.",ch,NULL,victim,TO_VICT);
		  act("{wYou take a step back and {Rcharge at $N with your shoulder{x.",ch,NULL,victim,TO_CHAR);
		  act("{w$n takes a step back and {Rcharges at $N{x",ch,NULL,victim,TO_NOTVICT);	 
		  act("{w$n just moves aside and lets you pass by.{x",victim,NULL,ch,TO_VICT);
		  act("{wYou move aside and let $N pass by.{x",victim,NULL,ch,TO_CHAR);
		  act("{w$n moves aside and lets $N pass by.{x",victim,NULL,ch,TO_NOTVICT);	 
		  return;
		}
	   //takes it  
		else {
		  act("{w$n {Rbulldozes{x you to the ground with $s shoulder!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou {Rbulldoze{x $N to the ground with your shoulder!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n {Rbulldozes{x $N to the ground with $s shoulder!{x",ch,NULL,victim,TO_NOTVICT);
		  
		  dam += ( dam * diceroll/40);
		  dam_type = DAM_TW;
		  dt = 1044;

		  if (number_percent() < 30)
		    DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
		}
	 }
	 break;
	 
	
    case (2) : //elbow
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, knees, a chance for throw attack here
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n aims $s {Relbow{x to your solar plexis.{x",ch,NULL,victim,TO_VICT);
		act("{wYou aim your {Relbow{x at $S solar plexis.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n aims $s {Relbow{x at $N solar plexis.{x",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n pushes your {Relbow{x down and {Rknees you in the abdomen{x!",victim,NULL,ch,TO_VICT);
		act("{wYou push the {Relbow{x down and {Rknee $N in the abdomen{x!",victim,NULL,ch,TO_CHAR);
		act("{w$n pushes the {Relbow{x down and {Rknees $N in the abdomen{x!",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, 1043, dual, TARGET_TORSO);
		return;	
	   }
	   //defends
	   else 
		if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		  act("{w$n aims $s {Relbow to your solar plexis{x.",ch,NULL,victim,TO_VICT);
		  act("{wYou aim your {Relbow at $S solar plexis{x.",ch,NULL,victim,TO_CHAR);
		  act("{w$n aims $s {Relbow at $N solar plexis{x.",ch,NULL,victim,TO_NOTVICT);	 
		  act("{w$n blocks your elbow with $s hands.{x",victim,NULL,ch,TO_VICT);
		  act("{wYou block $S elbow with your hands.{x",victim,NULL,ch,TO_CHAR);
		  act("{w$n blocks the elbow with $s hands.{x",victim,NULL,ch,TO_NOTVICT);	 
		  return;
		}
	   //takes it  
		else {
		  act("{w$n {Relbows{x you in the solar plexis!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou {Relbow{x $N in the solar plexis!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n {Relbows{x $N in the solar plexis!{x",ch,NULL,victim,TO_NOTVICT);
		  dt = 1040;
		  dam = dam * 2 / 3;
		  dam_type = DAM_TW;
		  DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
		  victim->position = POS_RESTING;		  
		}
	 }	 
	 break;
	 
	 
    case (3) : //bite
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, punch
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n tries to take a {Rbite{x out of your arm.{x",ch,NULL,victim,TO_VICT);
		act("{wYou try to rip out some flesh from $N with your {Rteeth{x.",ch,NULL,victim,TO_CHAR);
		act("{w$n tries to {Rbite{x $N{x",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n lets you take a {Rbite{x of $s fist!{x",victim,NULL,ch,TO_VICT);
		act("{wYou let $M take a {Rbite{x of you fist!{x",victim,NULL,ch,TO_CHAR);
		act("{w$n lets $N take a {Rbite{x of $s fist!{x",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, 1017, dual, TARGET_HEAD);
		return;		
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n tries to take a {Rbite{x out of your arm.{x",ch,NULL,victim,TO_VICT);
		act("{wYou try to rip out some flesh from $N with your {Rteeth{x.",ch,NULL,victim,TO_CHAR);
		act("{w$n tries to {Rbite{x $N{x",ch,NULL,victim,TO_NOTVICT);	 
		act("{wYou bang your teeth together as $n moves $s arm out of reach.{x",victim,NULL,ch,TO_VICT);
		act("{wYou quickly move your arm out of reach of sharp teeth.{x",victim,NULL,ch,TO_CHAR);
		act("{w$N makes a loud snap as $E takes a bite out of air.{x",victim,NULL,ch,TO_NOTVICT);	 
		return;
	   }
	   //takes it  
	   else {
		act("{w$n {Rrips out a pound of flesh{x from your arm!{x",ch,NULL,victim,TO_VICT);
		act("{wYou {Rtear out a good chunk of flesh{x from $N!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n gnaws on $N's arm before {Rripping out some flesh{x!{x",ch,NULL,victim,TO_NOTVICT);
		dt = 1032;
		dam += ( dam * diceroll/40);
		dam_type = DAM_TW;

		if (number_percent() < 35) {
		  if (!IS_AFFECTED(victim,AFF_POISON)) {
		    af.where	= TO_AFFECTS;
		    af.casterId  = victim->id;
		    af.type 	= gsn_poison;
		    af.level 	= ch->level;
		    af.duration	= 2;
		    af.location	= APPLY_STR;
		    af.modifier	= -1;
		    af.bitvector = AFF_POISON;
		    affect_to_char(victim,&af);
		    
		    send_to_char("{GYou feel poison coursing through your veins{x.\r\n", victim);
		    act("$N is poisoned from your bite!.", ch, NULL, victim, TO_CHAR);
		    //return;
		  }
		}
	   }
	 }	 
	 break;
	 
    case (4) : //knee
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, punch
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n tries to {Rknee{x you in the abdomen.{x",ch,NULL,victim,TO_VICT);
		act("{wYou bring your leg up to {Rknee{x $N in the abdomen.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n tries to {Rknee{x $N in the abdomen.{x",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n catches your leg and implants $s fist into your face!{x",victim,NULL,ch,TO_VICT);
		act("{wYou catch $S leg and implant your fist into $S face!{x",victim,NULL,ch,TO_CHAR);
		act("{w$n catches $N's leg and smashes a fist into $S face!!{x",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, 1017, dual, TARGET_HEAD);
		return;	
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n tries to {Rknee{x you in the abdomen.{x",ch,NULL,victim,TO_VICT);
		  act("{wYou bring your leg up to {Rknee{x $N in the abdomen.{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n tries to {Rknee{x $N in the abdomen.{x",ch,NULL,victim,TO_NOTVICT);	 
		  act("{wYou barely scratch $n as $e moves to the side.{x",victim,NULL,ch,TO_VICT);
		  act("{wYou quickly move away as $S leg fans your side.{x",victim,NULL,ch,TO_CHAR);
		  act("{w$n moves away as $N's knee misses its target.{x",victim,NULL,ch,TO_NOTVICT);	 
		  return;
	   }
	   //takes it  
	   else {
		act("{wYou knuckle down as $n {Rknees{x you in the abdomen!{x",ch,NULL,victim,TO_VICT);
		act("{wYou bring $N down as you {Rknee{x $M in the abdomen!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n {Rknees{x $N in the abdomen!{x",ch,NULL,victim,TO_NOTVICT);
		dt = 1043;
		dam += ( dam * diceroll/20);
		dam_type = DAM_TW;		
	   }
	 }
	 break;

    case (5) : //uppercut
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, headbutt
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n bends over just slight to {Ruppercut{x you in the chin.{x",ch,NULL,victim,TO_VICT);
		act("{wYou bend over just slightly in order to {Ruppercut{x $N.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n bends over just slightly to make an {Ruppercut{x.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n catches your hand and headbutts you in the face!{x",victim,NULL,ch,TO_VICT);
		act("{wYou catch $S hand and headbutt $m in the face!{x",victim,NULL,ch,TO_CHAR);
		act("{w$n catches the arm and headbutts $N in the face!{x",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, 1041, dual, TARGET_HEAD);
		return;	
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n bends over just slight to uppercut you in the chin.{x",ch,NULL,victim,TO_VICT);
		act("{wYou bend over just slightly in order to uppercut $N.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n bends over just slightly to make an uppercut.{x",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n shifts $s head to the side.{x",victim,NULL,ch,TO_VICT);
		act("{wYou shift your head to the side.{x",victim,NULL,ch,TO_CHAR);
		act("{w$n shifts $s head to the side.{x",victim,NULL,ch,TO_NOTVICT);	 
		return;
	   }
	   //takes it  
	   else {
		act("{wYour teeth chatter as $n {Ruppercuts{x you in the chin!{x",ch,NULL,victim,TO_VICT);
		act("{w$N's teeth chatter as you {Ruppercut{x $M in the chin!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n {Ruppercuts{x $N in the chin!{x",ch,NULL,victim,TO_NOTVICT);
		dt = 1042;
		dam += ( dam * diceroll/20);	  
		dam_type = DAM_TW;
	   }
	 }
	 break;

    case (6): // throwing sand
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n bends down and {Rthrow sand{x up toward your eyes.{x",ch,NULL,victim,TO_VICT);
		act("{wYou bend down and {Rthrow sand{x up towards $N eyes.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n bends down and {Rthrow sand{x up towards $N eyes.{x.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n manages to turn $S head away from the sand!{x",victim,NULL,ch,TO_VICT);
		act("{wYou manage to turn your head away from the sand!{x",victim,NULL,ch,TO_CHAR);
		act("{w$n manages to turn $S head away from the sand!{x",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, 1041, dual, TARGET_HEAD);
		return;	
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n bends down and {Rthrow sand{x up toward your eyes.{x",ch,NULL,victim,TO_VICT);
		act("{wYou bend down and {Rthrow sand{x up towards $N eyes.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n bends down and {Rthrow sand{x up towards $N eyes.{x.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n manages to turn $S head away from the sand!{x",victim,NULL,ch,TO_VICT);
		act("{wYou manage to turn your head away from the sand!{x",victim,NULL,ch,TO_CHAR);
		act("{w$n manages to turn $S head away from the sand!{x",victim,NULL,ch,TO_NOTVICT);
		return;
	   }
	   //takes it  
	   else {		
		act("{wYou are blinded from the {Rsand thrown{xinto your eyes!{x",ch,NULL,victim,TO_VICT);
		act("{w$N is blinded from the {Rsand thrown{x at $M!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n {Rthrow sand{x into $N's eyes!{x",ch,NULL,victim,TO_NOTVICT);
		dt = 1042;
		dam += ( dam * diceroll/20);	  
		dam_type = DAM_TW;
		
		if (!IS_AFFECTED(victim,AFF_BLIND)) {
		  af.where	= TO_AFFECTS;
		  af.casterId  = victim->id;
		  af.type 	= gsn_dirt;
		  af.level 	= ch->level;
		  af.duration	= 1;
		  af.location	= APPLY_HITROLL;
		  af.modifier	= -4;
		  af.bitvector = AFF_BLIND;
		  affect_to_char(victim,&af);
		  //return;
		}		
	   }
	 }
	 break;	 

    case (7): // spitting
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n laugh and {Rspray spittle{x toward your eyes.{x",ch,NULL,victim,TO_VICT);
		act("{wYou laugh and {Rspray spittle{x towards $N eyes.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n laugh and {Rspray spittle{x towards $N eyes.{x.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n manages to turn $S head away from the spittle!{x",victim,NULL,ch,TO_VICT);
		act("{wYou manage to turn your head away from the spittle!{x",victim,NULL,ch,TO_CHAR);
		act("{w$n manages to turn $S head away from the spittle!{x",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, 1041, dual, TARGET_HEAD);
		return;	
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n laugh and {Rspray spittle{x toward your eyes.{x",ch,NULL,victim,TO_VICT);
		act("{wYou laugh and {Rspray spittle{x towards $N eyes.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n laugh and {Rspray spittle{x towards $N eyes.{x.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n manages to turn $S head away from the spittle!{x",victim,NULL,ch,TO_VICT);
		act("{wYou manage to turn your head away from the spittle!{x",victim,NULL,ch,TO_CHAR);
		act("{w$n manages to turn $S head away from the spittle!{x",victim,NULL,ch,TO_NOTVICT);
		return;
	   }
	   //takes it  
	   else {		
		act("{w$n {Rspits{x into your face, the {Rspittle burning{x your eyes and blinding you!{x",ch,NULL,victim,TO_VICT);
		act("{wYou {Rspits{x into $N's face, the {Rspittle burning{x $S eyes and blinding $M!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n {Rspray spittle{x into $N's eyes!{x",ch,NULL,victim,TO_NOTVICT);
		dt = 1042;
		dam += ( dam * diceroll/20);	  
		dam_type = DAM_TW;
		
		if (!IS_AFFECTED(victim,AFF_BLIND)) {
		  af.where	= TO_AFFECTS;
		  af.casterId  = victim->id;
		  af.type 	= gsn_blindness;
		  af.level 	= ch->level;
		  af.duration	= 1;
		  af.location	= APPLY_HITROLL;
		  af.modifier	= -4;
		  af.bitvector = AFF_BLIND;
		  affect_to_char(victim,&af);
		  //return;
		}		
	   }
	 }
	 break;
	
	/* commenting out for Z to look at
	case (8): // claws
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, hurls
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n roars and slashes down with its {Rclaws{x toward you.",ch,NULL,victim,TO_VICT);
		act("{wYou roar and slash down with your {Rclaws{x towards $N{x.",ch,NULL,victim,TO_CHAR);
		act("{w$n roars and slashes down with its {Rclaws{x towards $N{x.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n catches your hand and {Rhurls{x you out of the way!{x.",victim,NULL,ch,TO_VICT);
		act("{wYou catch $S hand and {Rhurl{x $m out of the way!{x.",victim,NULL,ch,TO_CHAR);
		act("{w$n catches the arm and {Rhurls{x $N out of $S way!{x.",victim,NULL,ch,TO_NOTVICT);		 
		one_hit(victim, ch, gsn_throw, dual, TARGET_GENERAL);
		return;	
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n roars and slashes down with its {Rclaws{x toward you.",ch,NULL,victim,TO_VICT);
		act("{wYou roar and slash down with your {Rclaws{x towards $N.",ch,NULL,victim,TO_CHAR);
		act("{w$n roars and slashes down with its {Rclaws{x towards $N.",ch,NULL,victim,TO_NOTVICT);	
		act("{w$n manages to turn away from your {Rclaws{x.",victim,NULL,ch,TO_VICT);
		act("{wYou manage to turn away from $N's {Rclaws{x.",victim,NULL,ch,TO_CHAR);
		act("{w$n manages to turn away from $N's {Rclaws{x.",victim,NULL,ch,TO_NOTVICT);
		return;
	   }
	   //takes it  
	   else {
		act("{w$n {Rslashes you viciously{x with $s {Rclaws!{x",ch,NULL,victim,TO_VICT);
		act("{wYou {Rslash{x $N {Rviciously{x with your {Rclaws!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n {Rslashes{x $N {Rviciously{x with $s {Rclaws!{x",ch,NULL,victim,TO_NOTVICT);
		//dt = 1131;
		dam += ( dam + 20);	  
		dam_type = DAM_SLASH;
		  //return;		
	   }
	 }
	 break;

case (9): // hurls
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, claws
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n moves in to grab your arm, trying to send you flying with a good {Rhurl{x.",ch,NULL,victim,TO_VICT);
		act("{wYou move in to grab $N's arm and try to send $M flying with a good {Rhurl{x.",ch,NULL,victim,TO_CHAR);
		act("{w$n moves int to grab $N's arm and tries to send $M flying with a good {Rhurl{x",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n loses his grip on your arm as you slice him with your {Rclaws{x.",victim,NULL,ch,TO_VICT);
		act("{wYou lose your grip on $N's arm as $S slices you with $M {Rclaws{x.",victim,NULL,ch,TO_CHAR);
		act("{w$n loses his grip on $N's arm as $S is sliced with [{Rvicious claws{x.",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, 0, dual, TARGET_GENERAL);
		return;	
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n moves in to grab your arm, trying to send you flying with a good {Rhurl{x.",ch,NULL,victim,TO_VICT);
		act("{wYou move in to grab $N's arm and try to send him flying with a good {Rhurl{x.",ch,NULL,victim,TO_CHAR);
		act("{w$n moves in to grab $N's arm and tries to send $M flying with a good {Rhurl{x",ch,NULL,victim,TO_NOTVICT); 
		act("{w$n pulls back $s arm so you fumble with your hands in the air.",victim,NULL,ch,TO_VICT);
		act("{wYou pulls back your arm so $N fumbles with $S hands in the air.",victim,NULL,ch,TO_CHAR);
		act("{w$n pulls back $s arm so $N fumbles with $S hands in the air.",victim,NULL,ch,TO_NOTVICT);
		return;
	   }
	   //takes it  
	   else {		
		act("{w$n grabs your arm and {Rhurls{x you off to the side!{x",ch,NULL,victim,TO_VICT);
		act("{wYou grab $N's arm and {Rhurl{x $M off to one side!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n grabs $N by $S arm and {Rhurls{x $M off to one side!{x",ch,NULL,victim,TO_NOTVICT);
		dt = gsn_throw;
		dam += ( dam + 20);	  
		dam_type = DAM_TW;
		  //return;		
	   }
	 }
	 break;
case (10): // bear hug
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, stomp
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n rushes in to {Rbear hug{x you{x.",ch,NULL,victim,TO_VICT);
		act("{wYou rush in to {Rbear hug{x $N.",ch,NULL,victim,TO_CHAR);
		act("{w$n rushes in to {Rbear hug{x $N.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n {Rstomps{x your foot and wriggle out of your {Rbear hug{x!{x",victim,NULL,ch,TO_VICT);
		act("{wYou {Rstomp{x $N's foot and wriggle out of the {Rbear hug{x!{x",victim,NULL,ch,TO_CHAR);
		act("{w$n {Rstomps{x $N's foot and wriggle out of the incoming {Rbear hug{x!{x",victim,NULL,ch,TO_NOTVICT);	 
		one_hit(victim, ch, gsn_kick, dual, TARGET_GENERAL);
		return;	
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n rushes in to {Rbear hug{x you{x.",ch,NULL,victim,TO_VICT);
		act("{wYou rush in to {Rbear hug{x $N.",ch,NULL,victim,TO_CHAR);
		act("{w$n rushes in to {Rbear hug{x $N.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n wriggles out of the {Rbear hug{x.",victim,NULL,ch,TO_VICT);
		act("{wYou wriggle out of the {Rbear hug{x.",victim,NULL,ch,TO_CHAR);
		act("{w$n wriggle out of $N {Rbear hug{x.",victim,NULL,ch,TO_NOTVICT);	 
		return;
	   }
	   //takes it  
	   else {		
		act("{w$n wraps his arms around you and {Rcrushes{x you in a {Rbear hug{x.",ch,NULL,victim,TO_VICT);
		act("{wYou wrap your arms around $N and {Rcrush{x them in a {Rbear hug{x.",ch,NULL,victim,TO_CHAR);
		act("{w$n wraps their arms around $N and {Rcrushes{x them in a {Rbear hug{x.",ch,NULL,victim,TO_NOTVICT);
		dt = 0;
		dam += ( dam + 20);	  
		dam_type = DAM_TW;
		  //return;		
	   }
	 }
	 break;
case (11): // stomp
	 if (number_percent() <= (tw_skill/4)) {
	   //counter attack, bearhug
	   if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/10) ) {
		act("{w$n lifts his huge boot and {Rstomps{x at your foot.",ch,NULL,victim,TO_VICT);
		act("{wYou lift your huge boot and {Rstomp{x $N's foot.",ch,NULL,victim,TO_CHAR);
		act("{w$n lifts his huge boot and {Rstomps{x at $N's foot.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n sidesteps and brings you into a {Rbear hug{x.",victim,NULL,ch,TO_VICT);
		act("{wYou sidestep and bring $N into a {Rbear hug{x.",victim,NULL,ch,TO_CHAR);
		act("{w$n sidesteps and brings $N into a {Rbear hug{x.",victim,NULL,ch,TO_NOTVICT);	 	 
		one_hit(victim, ch, 0, dual, TARGET_GENERAL);
		return;	
	   }
	   //defends
	   else if ((tw_skill_victim) && (number_percent () <= tw_skill_victim/2) ) {
		act("{w$n lifts his huge boot and {Rstomps{x at your foot.",ch,NULL,victim,TO_VICT);
		act("{wYou lift your huge boot and {Rstomp{x $N's foot.",ch,NULL,victim,TO_CHAR);
		act("{w$n lifts his huge boot and {Rstomps{x at $N's foot.",ch,NULL,victim,TO_NOTVICT);	 
		act("{w$n moves out of reach of your {Rstomp{x.",victim,NULL,ch,TO_VICT);
		act("{wYou move out of reach from $N's {Rstomp{x.",victim,NULL,ch,TO_CHAR);
		act("{w$n moves out of reach from $N's {Rstomp{x.",victim,NULL,ch,TO_NOTVICT);
		return;
	   }
	   //takes it  
	   else {		
		act("{w$n {Rstomps{x you with his huge boot!{x",ch,NULL,victim,TO_VICT);
		act("{wYou {Rstomp{x $N with your huge boot!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n {Rstomps{x $N with his huge boot!{x",ch,NULL,victim,TO_NOTVICT);
		dt = gsn_kick;
		dam += ( dam + 20);	  
		dam_type = DAM_TW;
		  //return;		
	   }
	 }
	 break; 
	 */ 

    } // End switch Trolloc Warfare
  }
  /* ----------- */
  /* Archer */ 
  /* ----------- */
  if ( IS_NPC(ch) && ch->spec_fun == spec_lookup( "spec_archer" ))
  {
	do_shoot(ch,victim->name);
  }
  
  
  /* -----------------*/
  /*  Master Forms */
  /* -----------------*/
  if ( IS_NPC(ch) && ch->spec_fun == spec_lookup( "spec_blademaster" ))
    mf_skill = ch->level;
  else 
  {
    mf_skill = getmfskill(ch); 
 
    // If quick reflexes
    if  (IS_SET(ch->merits, MERIT_QUICKREFLEXES) && (mf_skill)) {
	 mf_skill += 10;
    }

    if (IS_CODER(ch)) {
	 mf_skill += 100;
    }
  }
  
  if ( mf_skill && check_mf(ch,find_relevant_masterform(ch)) && !IS_WOLFSHAPE(ch) && (IS_SET(ch->auto_act, AUTO_MASTERFORMS) || IS_NPC(ch))) 
  { 
    diceroll = number_percent();
    v_wield = get_eq_char( victim, WEAR_WIELD );	
    
    if ( IS_NPC(victim) && victim->spec_fun == spec_lookup( "spec_blademaster" ))
	 mf_skill_victim = victim->level;
    else {
	 if (!IS_SET(victim->auto_act, AUTO_MASTERFORMS))
	   mf_skill_victim = 0;
	 else if (!check_mf(victim,find_relevant_masterform(ch)))
	   mf_skill_victim = 0;
	 else {
	   mf_skill_victim = getmfskill(victim); 

	   // If quick reflexes
	   if  (mf_skill_victim > 0 && IS_SET(victim->merits, MERIT_QUICKREFLEXES)) {
		mf_skill_victim += 10;
	   }
	 }
    }
    
    if (mf_skill > 0 && mf_skill_victim > 0)
    {
    	if ((mf_skill_victim + number_range(1,20)) > (mf_skill + number_range(1,20)))
    	{
    		if (number_range(0,5) < 5)
    		{
    			mf_parried = TRUE;
    		}
    		else
    		{
    			mf_parried = FALSE;
    		}	
    	} 
    	else
    	{
    		mf_parried = FALSE;	
    	}
    }

    if ((mf_skill > 0) &&
	   (dt != gsn_circle) && 
	   (dt != gsn_backstab) && 
	   (!mf_attack)) 
	{
	 number = number_range(0, 11);
	 switch(number) {
	 case (0) :                     //Disarming move
	   if (v_wield == NULL)
	   	break;
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack(find_relevant_masterform(ch),number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (v_wield != NULL) && (v_wield->item_type == ITEM_WEAPON) 
			&& (mf_skill_victim) && (mf_parried) ) 
		{
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}        
		else {
		  //attacker
		  sprintf(buffer,"{w$n attacks you with %s!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);
/*
		  sprintf(buffer,"{w$n attacks with %s you!{x",get_master_extended_attack(ch,number));
		  act(buffer,ch,v_wield,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack with %s{x $N!{x",get_master_extended_attack(ch,number));
		  act(buffer,ch,v_wield,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks with %s{x $N!{x",get_master_extended_attack(ch,number));
		  act(buffer,ch,v_wield,victim,TO_NOTVICT);
*/
		  
		  dam += ( dam * diceroll/30);
		  dam_type = DAM_MF;
		  
		  if ( ( v_wield != NULL ) && !(IS_OBJ_STAT(v_wield,ITEM_NOREMOVE)) ){
		    act( "{w$n twists $p out of your hand!{x", ch,v_wield, victim, TO_VICT    );
		    act( "{wYou twist $p out of $S hand!{x",  ch, v_wield, victim, TO_CHAR    );
		    act( "{w$n twists $p out of $S hand!{x",  ch, v_wield, victim, TO_NOTVICT );
		    obj_from_char( v_wield );
		    
		    if (IS_SET(v_wield->extra_flags,ITEM_ROT_DEATH)) {
                       v_wield->timer = number_range(5,10);
                       REMOVE_BIT(v_wield->extra_flags,ITEM_ROT_DEATH);
                    }
		    
		    if ( IS_OBJ_STAT(v_wield,ITEM_NODROP) || IS_OBJ_STAT(v_wield,ITEM_INVENTORY) )
			 obj_to_char( v_wield, victim );
		    else {
			 if (!IS_NPC(victim))
			   obj_to_char( v_wield, victim );
			 else {
			   obj_to_room( v_wield, victim->in_room );
			   
			   if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,v_wield))
				get_obj(victim,v_wield,NULL);
			 }
		    }
		    wield = get_eq_char(victim,WEAR_SECOND_WIELD);
		    
		    if ( (wield != NULL) && !(IS_OBJ_STAT(wield,ITEM_NOREMOVE))
			    && (v_wield->wear_loc == WEAR_WIELD) ) {
			 unequip_char( victim, wield);
			 equip_char(victim, wield, WEAR_WIELD);
		    }
		    return;
		  }		  
		}
	   }	 	
	   break;
	   
	 case (1) :
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}        
		else {
		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);
/*		
		  act("{w$n attacks with Ribbon in the Air{x, making a forward front kick as $e swing the blade in a strong forward vertical slash toward you!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou attack with Ribbon in the Air{x, making a forward front kick as you swing the blade in a strong forward vertical slash towards $N!{x{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n attacks with Ribbon in the Air{x, making a forward front kick as $e swing the blade in a strong forward vertical slash $N!{x{x",ch,NULL,victim,TO_NOTVICT);
*/		
		  dam += ( dam * diceroll/30);
		  dam_type = DAM_MF;

		  // Free kick
		  if (number_percent() < 5)
		    do_kick(ch, "");
		}			
	   }
	   break;	
	   
	 case (2) :
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}		
		else {
		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);

/*		  act("{w$n attacks with Swallow Takes Flight{x, making a horizontal slash from weak to strong side towards you!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou attack with Swallow Takes Flight{x, making a horizontal slash from weak to strong side toward $N!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n attacks with Swallow Takes Flight{x, making a horizontal slash from weak to strong side towards $N!{x",ch,NULL,victim,TO_NOTVICT);
*/		  
		  dam += ( dam * diceroll/60);	  
		  dam_type = DAM_MF;

		  if (number_percent() < 70) {
		    one_hit(ch,victim,find_relevant_masterform(ch), dual, 0);
		    if (number_percent() < 30) {
			 one_hit(ch,victim,find_relevant_masterform(ch), dual, 0);
			 if (number_percent() < 10) {
			   one_hit(ch,victim,find_relevant_masterform(ch), dual, 0);
			 }
		    } 
		  }		  
		}
	   }		
	   break;	
	
	 case (3) :
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( find_relevant_masterform(ch),number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}		
		else {
		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);

/*		  act("{w$n attacks with Moon on the Water{x, penetrating your defenses!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou attack with Moon on the Water{x, penetrating $S defenses!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n attacks with Moon on the Water{x, penetrating $S defenses!{x",ch,NULL,victim,TO_NOTVICT);
*/
		  dam += ( dam * diceroll/80);	  
		  dam_type = DAM_MF;

		  for (vch = ch->in_room->people; vch != NULL; vch = vch_next) {
		    vch_next = vch->next;
		    if ((vch != victim && vch->fighting == ch))
			 one_hit(ch,vch,find_relevant_masterform(ch), dual, 0);
		  }
		}
	   }
	   break;	
	
	 case (4) :
	  if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		 return;
	    }
	    else {
		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);

	    	
/*		 act("{w$n attacks with Lightning of Three Prongs{x, spinning around you with a deadly slash horizontally towards you!{x",ch,NULL,victim,TO_VICT);
		 act("{wYou attack with Lightning of Three Prongs{x, spinning around $N with a deadly slash toward $M!{x",ch,NULL,victim,TO_CHAR);
		 act("{w$n attacks with Lightning of Three Prongs{x, spinning around $N with a deadly slash toward $M!{x",ch,NULL,victim,TO_NOTVICT);
*/
		 dam += ( dam * diceroll/40);	  
		 dam_type = DAM_MF;
		 
		 one_hit( ch, victim, find_relevant_masterform(ch), dual, 0);
		 one_hit( ch, victim, find_relevant_masterform(ch), dual, 0);
	    }
	  }
	  break;

	 case (5) :
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}
		else {
		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);

	/*		
		  act("{w$n attacks with The Boar Rushes Down the Mountain{x, making a vertical slash with both hands on the hilt towards you!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou attack with The Boar Rushes Down the Mountain{x, making a vertical slash with both hands on the hilt toward $N!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n attacks with The Boar Rushes Down the Mountain{x, making a vertical slash with both hands on the hilt towards $N!{x",ch,NULL,victim,TO_NOTVICT);
	*/
		  dam = dam * 2 / 3;
		  dam_type = DAM_MF;
		  act("{w$n then jabs you in the solar plexis!{x",ch,NULL,victim,TO_VICT);
		  act("{wYour keen placement of your strike leaves $M gasping for air!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n's jab leaves $N gasping for air.{x",ch,NULL,victim,TO_NOTVICT);
		  DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
		  victim->position = POS_RESTING;
		}
	   }
	   break;
	   
	 case (6) :
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (!mf_parried) ) {
			  sh_int mgsn = find_relevant_masterform(ch);
			  sprintf(buffer,"{w$n attacks with %s{x you!{x",get_master_attack(mgsn,number));
			  act(buffer,ch,NULL,victim,TO_VICT);
			  sprintf(buffer,"{wYou attack with %s{x $N!{x",get_master_attack(mgsn,number));
			  act(buffer,ch,NULL,victim,TO_CHAR);
			  sprintf(buffer,"{w$n attacks with %s{x $N!{x",get_master_attack(mgsn,number));
			  act(buffer,ch,NULL,victim,TO_NOTVICT);

/*			act("{w$n {BStrikes the Spark{x, leaving you blind!{x",ch,NULL,victim,TO_VICT);
			act("{wYou {BStrike the Spark{x, leaving $M in a daze!{x",ch,NULL,victim,TO_CHAR);
			act("{w$n {BStrikes the Spark{x, blinding $M!{x",ch,NULL,victim,TO_NOTVICT);
*/
			dam_type = DAM_MF;
	
			if (!IS_AFFECTED(victim,AFF_BLIND)) {
			  af.where	= TO_AFFECTS;
			  af.casterId  = victim->id;
			  af.type 	= gsn_blademaster;
			  af.level 	= ch->level;
			  af.duration	= 1;
			  af.location	= APPLY_HITROLL;
			  af.modifier	= -4;
			  af.bitvector 	= AFF_BLIND;
			  affect_to_char(victim,&af);
			  //return;
			}
		}
	   }
	   break;
	 case (7): 
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}		
		else {
			

		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);
/*		  act("{w$n attacks with Apple Blossoms in the Wind{x, striking a deadly blow aginst you!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou attack with Apple Blossoms in the Wind{x, striking a deadly blow against $M!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n attacks with Apple Blossoms in the Wind{x, striking a deadly blow against $N!{x",ch,NULL,victim,TO_NOTVICT);
*/		  
		  dam += dam * 1.5;
		  dam_type = DAM_CRIT;
		}
	   }
	   break;
	 case (8): 
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}		
		else {
		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);

/*		  act("{w$n attacks with Arc of the Moon{x, swinging the blade in an upper arch angle towards you!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou attack with Arc of the Moon{x, swinging the blade in an upper arch angle toward $M!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n attacks with Arc of the Moon{x, swinging the blade in an upper arch angle towards $N!{x",ch,NULL,victim,TO_NOTVICT);
*/
		  dam += ( dam * diceroll/45);
		  dam_type = DAM_MF;
		}
	   }
	   break;
	 case (9): 
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}		
		else {
		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);

/*			
		  act("{w$n attacks with The Cat Dances on the Wall{x, making a sophisticated twist with the blade, spinning the edge of the blade upwards and arching towards you!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou attack with The Cat Dances on the Wall{x, making a sophisticated twist with the blade, spinning the edge of the blade upwards and arching toward $M!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n attacks with The Cat Dances on the Wall{x, making a sophisticated twist with the blade, spinning the edge of the blade upwards and arching towards $N!{x",ch,NULL,victim,TO_NOTVICT);
*/
		  dam += ( dam * diceroll/30);	  
		  dam_type = DAM_MF;
		}
	   }
	   break;
	 case (10): 
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}		
		else {
		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);

/*			
		  act("{w$n attacks with The Falling Leaf{x, performing a slash in diagonal motion towards you!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou attack with The Falling Leaf{x, performing a slash in diagonal motion toward $M!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n attacks with The Falling Leaf{x, performing a slash in diagonal motion towards $N!{x",ch,NULL,victim,TO_NOTVICT);
*/		
		  dam += ( dam * diceroll/65);	  
		  dam_type = DAM_MF;
		}
	   }
	   break;
	 case (11): 
	   if (number_percent() <= (mf_skill/4)) {
	 	strcpy(mf_attackstring, get_master_attack( mf_gsn,number));
	 	if (mf_parried)
	 	{
	 		strcpy(mf_defendstring,get_master_defend( find_relevant_masterform(victim),number));
	 	}
	 
		if ( (mf_skill_victim) && (mf_parried) ) {
		  //attacker
		  sprintf(buffer,"{w$n attacks with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_VICT);
		  act(buffer,ch,NULL,victim,TO_NOTVICT);	 
		  sprintf(buffer,"{wYou attack with %s!{x",mf_attackstring);
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  //defender
		  sprintf(buffer,"{w$n meets with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_VICT);
		  act(buffer,victim,NULL,ch,TO_NOTVICT);
		  sprintf(buffer,"{wYou meet with %s!{x",mf_defendstring);
		  act(buffer,victim,NULL,ch,TO_CHAR);
		  return;
		}		
		else {
		  sprintf(buffer,"{w$n attacks you with %s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_VICT);
		  sprintf(buffer,"{wYou attack $N with %s{x{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_CHAR);
		  sprintf(buffer,"{w$n attacks $N with {B%s{x!{x",get_master_attack(find_relevant_masterform(ch),number));
		  act(buffer,ch,NULL,victim,TO_NOTVICT);

/*		  act("{w$n attacks with Parting the Silk{x, spinning in a complete circle using the velocity in a back handed horizontal slash towards you!{x",ch,NULL,victim,TO_VICT);
		  act("{wYou attack with Parting the Silk{x, spinning in a complete circle using the velocity in a back handed horizontal slash toward $M!{x",ch,NULL,victim,TO_CHAR);
		  act("{w$n attacks with Parting the Silk{x, spinning in a complete circle using the velocity in a back handed horizontal slash towards $N!{x",ch,NULL,victim,TO_NOTVICT);
*/
		  dam += dam * 2;
		  dam_type = DAM_CRIT;
		}
	   }
	   break;
	 }
    }
  } // End BM 

  
  
  
  if ( !IS_AWAKE(victim) )
    dam *= 2;
  else if (victim->position < POS_FIGHTING)
    dam = dam * 3 / 2;
  
  if ( dt == gsn_backstab && wield != NULL) {	
    if ( wield->value[0] != 2 )
	 dam *= 2 + (ch->level / 10); 
    else 
	 dam *= 2 + (ch->level / 8);
  }
  
  if ( dt == gsn_circle && wield != NULL) {
    if ( wield->value[0] != 2 ) {
	 dam *= 2; //+ (ch->level / 10);
    }
    else {
	 dam *=2; // + (ch->level / 8);
    }
    dam = dam/1.3;
  }
  
  dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;
  
  if ( dam <= 0 )
    dam = 1;
  
  result = damage( ch, victim, dam, dt, dam_type, TRUE );
    
  /* but do we have a funky weapon? */
  if (result && wield != NULL) { 
    int dam;

    if ((ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_POISON) && victim->race != race_lookup("fade")) || 
        (ch->race == race_lookup("fade") && wield->value[0] == WEAPON_SWORD && victim->race != race_lookup("fade"))) {
	 int level;
	 AFFECT_DATA *poison, af;
	 
	 if ((poison = affect_find(wield->affected,gsn_poison)) == NULL)
	   level = wield->level;
	 else
	   level = poison->level;
	 
	 if (!saves_spell(level / 2,victim,DAM_POISON)) {
	   send_to_char("{GYou feel poison coursing through your veins{x.\r\n", victim);
	   act("$n is poisoned by the venom on $p.", victim,wield,NULL,TO_ROOM);
	   
	   af.where     = TO_AFFECTS;
	   af.type      = gsn_poison;
	   af.level     = level * 3/4;
	   af.duration  = level / 2;
	   af.location  = APPLY_STR;
	   if (ch->race == race_lookup("fade"))
	      af.modifier  = -5;
	   else
	      af.modifier  = -1;
	   af.bitvector = AFF_POISON;
	   affect_join( victim, &af );
	 }
	 
	 /* weaken the poison if it's temporary */
	 if (poison != NULL) {
	   poison->level = UMAX(0,poison->level - 2);
	   poison->duration = UMAX(0,poison->duration - 1);
	   
	   if (poison->level == 0 || poison->duration == 0)
		act("The poison on $p has worn off.",ch,wield,NULL,TO_CHAR);
	 }
    }
    
    if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_VAMPIRIC)) {
	 dam = number_range(1, wield->level / 5 + 1);
	 act("$p draws life from $n.",victim,wield,NULL,TO_ROOM);
	 act("You feel $p drawing your life away.", victim,wield,NULL,TO_CHAR);
	 damage(ch,victim,dam,0,DAM_NEGATIVE,FALSE);
	 ch->alignment = UMAX(-1000,ch->alignment - 1);
	 ch->hit += dam/2;
    }
    
/*
    if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FLAMING)) {
	 dam = number_range(1,wield->level / 4 + 1);
	 act("$n is burned by $p.",victim,wield,NULL,TO_ROOM);
	 act("$p sears your flesh.",victim,wield,NULL,TO_CHAR);
	 fire_effect( (void *) victim,wield->level/2,dam,TARGET_CHAR);
	 damage(ch,victim,dam,0,DAM_FIRE,FALSE);
    }
*/
    
    if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FROST)) {
	 dam = number_range(1,wield->level / 6 + 2);
	 act("$p freezes $n.",victim,wield,NULL,TO_ROOM);
	 act("The cold touch of $p surrounds you with ice.",
		victim,wield,NULL,TO_CHAR);
	 cold_effect(victim,wield->level/2,dam,TARGET_CHAR);
	 damage(ch,victim,dam,0,DAM_COLD,FALSE);
    }
    
    if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_SHOCKING)) {
	 dam = number_range(1,wield->level/5 + 2);
	 act("$n is struck by lightning from $p.",victim,wield,NULL,TO_ROOM);
	 act("You are shocked by $p.",victim,wield,NULL,TO_CHAR);
	 shock_effect(victim,wield->level/2,dam,TARGET_CHAR);
	 damage(ch,victim,dam,0,DAM_LIGHTNING,FALSE);
    }
  }
  tail_chain( );
  return;
}


/*
 * Inflict damage from a hit.
 */
bool damage(CHAR_DATA *ch,CHAR_DATA *victim,int dam,int dt,int dam_type, bool show) 
{
  OBJ_DATA *corpse;
  char buf[MAX_STRING_LENGTH];
  bool immune;
  long sub_xp=0;
  int hit_location;
  int defend_bonus=0;
  bool always_hit = FALSE;
  int i;


  /* 
   * Make it so Lanfear won't hit her own mobs unless they are charmed
   */
  
  if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_CHOSEN_LANFEAR) {
	if (IS_NPC(victim)) {
		if ((victim->pIndexData->vnum == MOB_VNUM_LANFEARS_TROLLOC1 ) ||
	    	(victim->pIndexData->vnum == MOB_VNUM_LANFEARS_TROLLOC2 ) ||
	    	(victim->pIndexData->vnum == MOB_VNUM_LANFEARS_TROLLOC3 ) ||
	    	(victim->pIndexData->vnum == MOB_VNUM_LANFEARS_TROLLOC4 ) ||
	    	(victim->pIndexData->vnum == MOB_VNUM_LANFEARS_FADE1 ) ||
	    	(victim->pIndexData->vnum == MOB_VNUM_LANFEARS_FADE2 )) 
        	{
           		return FALSE;
        	}
	}
  }
  if ((IS_NPC(ch)) && 
     ((ch->pIndexData->vnum == MOB_VNUM_LANFEARS_TROLLOC1 ) ||
      (ch->pIndexData->vnum == MOB_VNUM_LANFEARS_TROLLOC2 ) ||
      (ch->pIndexData->vnum == MOB_VNUM_LANFEARS_TROLLOC3 ) ||
      (ch->pIndexData->vnum == MOB_VNUM_LANFEARS_TROLLOC4 ) ||
      (ch->pIndexData->vnum == MOB_VNUM_LANFEARS_FADE1 ) ||
      (ch->pIndexData->vnum == MOB_VNUM_LANFEARS_FADE2 ))) {
	if (IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_CHOSEN_LANFEAR) 
	{
		return FALSE;
   	}

  } 
 
  if ( victim->position == POS_DEAD )
    return FALSE;

    if ( !IS_NPC(victim) && IS_AFFECTED(victim,AFF_SAP ))
    {
	send_to_char("They're already unconscience!!\r\n",ch);
        return FALSE;
    }


   if (IS_NPC(victim) && IS_AFFECTED(victim,AFF_SAP))
   {
	send_to_char("Your hostility wakes them up.\r\n",ch); 
	affect_strip( victim, gsn_sleep);
	affect_strip( victim, gsn_sap );
   }
    /*
     * Stop up any residual loopholes.
     */
    if ( dam > 2200 && dt >= TYPE_HIT) {    	
	 sprintf(buf, "Damage <%d> more than 2200 points cause by %s!", dam, IS_NPC(ch) ? ch->short_descr : ch->name);
	 log_string(buf);	
	 wiznet(buf,ch,NULL,WIZ_SECURE,0,0);
	 if (!IS_IMMORTAL(ch) && !IS_GHOLAM(ch))
	   dam = 2200;

	 // If damage over 2200, and victim is channeler - they have chance to lose con and channeling
	 if (IS_AFFECTED(victim,AFF_CHANNELING) && number_chance(15)) {
	   if (victim->sex == SEX_MALE)
		sprintf(buf, "{DThe pain makes your void shatter, and you lose control of %s.{x\n\r", victim->sex == SEX_MALE ? "Saidin" : "Saidar");	   
	   else
		sprintf(buf, "{WThe pain makes you lose control of %s.{x\n\r", victim->sex == SEX_MALE ? "Saidin" : "Saidar");
	   send_to_char(buf, victim);
	   do_function(victim, &do_unchannel, "" );
	 }
	 
    }
    
    if (dam_type != DAM_SUFFOCATE) {
       /* damage reduction */
       if ( dam > 35)
	   dam = (dam - 35)/2 + 35;
       if ( dam > 80)
	   dam = (dam - 80)/2 + 80; 
    }
   
    if ( victim != ch )
    {
	/*
	 * Certain attacks are forbidden.
	 * Most other attacks are returned.
	 */
	if ( is_safe( ch, victim ) )
	    return FALSE;
	check_killer( ch, victim );

	if ( victim->position > POS_STUNNED )
	{
	    if ( victim->fighting == NULL )
	    {
		set_fighting( victim, ch );
		if ( IS_NPC( victim ) && HAS_TRIGGER( victim, TRIG_KILL ) )
		    mp_percent_trigger( victim, ch, NULL, NULL, TRIG_KILL );
	    }
	    if (victim->timer <= 4)
	    	victim->position = POS_FIGHTING;
	}

	if ( victim->position > POS_STUNNED )
	{
	    if ( ch->fighting == NULL )
		set_fighting( ch, victim );
	}

	/*
	 * More charm stuff.
	 */
	if ( victim->master == ch )
	  stop_follower( victim );
    }

    /*
     * Inviso attacks ... not.
     */
    if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
    {
	affect_strip( ch, gsn_invis );
	affect_strip( ch, gsn_mass_invis );
	REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
	act( "$n fades into existence.", ch, NULL, NULL, TO_ROOM );
    }

    /*
     * Damage modifiers.
     */

    if ( dam_type != DAM_SUFFOCATE) {
       if ( dam > 1 && !IS_NPC(victim) 
       &&   victim->pcdata->condition[COND_DRUNK]  > 10 )
	   dam = 9 * dam / 10;
   
       if ( dam > 1 && IS_AFFECTED(victim, AFF_SANCTUARY) )
	   dam /= 2;
      
   
    }
       immune = FALSE;

    /*
     * Check for parry, and dodge.
     */
    if ( dt >= TYPE_HIT && ch != victim && 
	    dam_type != DAM_MF && dam_type != DAM_TW && dam_type != DAM_CRIT && dam_type != DAM_SD && dam_type != DAM_SUFFOCATE && dam_type != DAM_DU) {
	 
	 if (!IS_WOLFSHAPE(victim) || IS_NPC(victim)) {
	   if (IS_SET(victim->auto_act, AUTO_SHIELDBLOCK) || IS_NPC(victim))
		if ( check_shield_block(ch,victim))
		  return FALSE;
	   
	   if ((!IS_NPC(victim) && IS_SET(victim->auto_act, AUTO_PARRY))  || IS_NPC(victim))
		if ( check_parry( ch, victim ) )
		  return FALSE;
	 }
	 if ((!IS_NPC(victim) && IS_SET(victim->auto_act, AUTO_DODGE))  || IS_NPC(victim))
	   if ( check_dodge( ch, victim ) )
		return FALSE;
    }
    
    switch(check_immune(victim,dam_type)) {
    case(IS_IMMUNE):
	 immune = TRUE;
	 dam = 0;
	 break;
    case(IS_RESISTANT):	
	 dam -= dam/3;
	 break;
    case(IS_VULNERABLE):
	 dam += dam/2;
	 break;
    }

    if (IS_GHOLAM(victim))  
    {
	dam = 0;
	immune = TRUE;
    }

    // allows you to sustain more damage with hugesize merit
    if (IS_SET(victim->merits, MERIT_HUGESIZE))
	 dam -= dam/10;
    
    /*
	* Hit locations
	*
	*/
    if (dt < TYPE_HIT || dam_type == DAM_MF || dam_type == DAM_TW || dam_type == DAM_CRIT || dam_type == DAM_SD || dam_type == DAM_DU)
	 always_hit = TRUE;
    
    // Roll a location
    hit_location = number_range(0, MAX_HIT_LOC-1);
    
    if (dam_type == DAM_SUFFOCATE) {
	hit_location = LOC_HE;
    }
    // If target set other than all, small chance to hit that location
    if (ch->target_loc != LOC_NA) {
	 if (number_percent() < 6) {
	   hit_location = ch->target_loc;
	 }
    }
    
    // If defend set other than all, small chance to defend that location
    if (victim->defend_loc != LOC_NA) {
	 if (victim->defend_loc == hit_location) {
	   defend_bonus = 5;
	   //sprintf(buf, "You see a swing comming against your %s and tries to defend against it.\r\n", 
	   //hit_flags[hit_location].name);
	   //send_to_char(buf, victim);
	 }
    }

    
    if (IS_GHOLAM(ch))
    {
	dam = number_range(800,2200);
    }
    switch(hit_location) {
	 
    case (LOC_LA):
	 if (!always_hit) {
	   if (number_percent() < 25+defend_bonus)
		dam = 0;
	 }	 
	 if (dam_type == DAM_CRIT)
  	 {
	   dam *= 2.5;
	   OBJ_DATA * obj;
	   if ((obj = get_eq_char( victim, WEAR_ARMS )) != NULL)
	   {
  		if (!IS_SET(obj->extra_flags, ITEM_NO_BREAK))
		   obj->condition--;
		if (obj->condition <= 0)
		{
			obj->condition = 0; 
	   		SET_BIT(obj->extra_flags, ITEM_BROKEN);
        		act ( "Your $p is broken and falls off!", victim, obj, NULL, TO_CHAR);
	   		unequip_char(victim, obj);
		}
           }
	 }
	 
	 if (IS_NPC(victim))
	   victim->hit_loc[LOC_LA] -= UMAX(1, dam/number_range(1, 2));
	 else
	   victim->hit_loc[LOC_LA] -= UMAX(1, dam/number_range(1, LOC_MOD_LA));
	 break;
	 
    case (LOC_LL):
	 if (!always_hit) {
	   if (number_percent() < 35+defend_bonus)
		dam = 0;
	 }
	 
	 if (dam_type == DAM_CRIT)
  	 {
	   dam *= 1.5;
	   OBJ_DATA * obj;
	   if ((obj = get_eq_char( victim, WEAR_LEGS )) != NULL)
	   {
  		if (!IS_SET(obj->extra_flags, ITEM_NO_BREAK))
		   obj->condition--;
		if (obj->condition <= 0)
		{
			obj->condition = 0; 
	   		SET_BIT(obj->extra_flags, ITEM_BROKEN);
        		act ( "Your $p is broken and falls off!", victim, obj, NULL, TO_CHAR);
	   		unequip_char(victim, obj);
		}
           }
	 }
	 if (IS_NPC(victim))
	    victim->hit_loc[LOC_LL] -= UMAX(1, dam/number_range(1, 2));
	 else
	    victim->hit_loc[LOC_LL] -= UMAX(1, dam/number_range(1, LOC_MOD_LL));
	 break;

    case (LOC_HE):
	 if (!always_hit) {
	   if (number_percent() < 90+defend_bonus)
		dam = 0;
	 }
	 if (dam_type == DAM_CRIT) {
	   if (number_percent() > 97)
		dam *= 10;
	   else
		dam *= 3;
	   OBJ_DATA * obj;
	   if ((obj = get_eq_char( victim, WEAR_HEAD )) != NULL)
	   {
  		if (!IS_SET(obj->extra_flags, ITEM_NO_BREAK))
		   obj->condition--;
		if (obj->condition <= 0)
		{
			obj->condition = 0; 
	   		SET_BIT(obj->extra_flags, ITEM_BROKEN);
        		act ( "Your $p is broken and falls off!", victim, obj, NULL, TO_CHAR);
	   		unequip_char(victim, obj);
		}
           }
	   if ((obj = get_eq_char( victim, WEAR_FACE )) != NULL)
	   {
  		if (!IS_SET(obj->extra_flags, ITEM_NO_BREAK))
		   obj->condition--;
		if (obj->condition <= 0)
		{
			obj->condition = 0; 
	   		SET_BIT(obj->extra_flags, ITEM_BROKEN);
        		act ( "Your $p is broken and falls off!", victim, obj, NULL, TO_CHAR);
	   		unequip_char(victim, obj);
		}
           }
	 }
	 
	 if (IS_NPC(victim))
	    victim->hit_loc[LOC_HE] -= UMAX(1, dam/number_range(1, 2));
	 else
	    victim->hit_loc[LOC_HE] -= UMAX(1, dam/number_range(1, LOC_MOD_HE));
	 break;

    case (LOC_BD):
	 if (!always_hit) {
	   if (number_percent() < 15+defend_bonus)
		dam = 0;
	 }	 
	 if (dam_type == DAM_CRIT)
  	 {
	   dam *= 2.5;
	   OBJ_DATA * obj;
	   if ((obj = get_eq_char( victim, WEAR_BODY )) != NULL)
	   {
  		if (!IS_SET(obj->extra_flags, ITEM_NO_BREAK))
		   obj->condition--;
		if (obj->condition <= 0)
		{
			obj->condition = 0; 
	   		SET_BIT(obj->extra_flags, ITEM_BROKEN);
        		act ( "Your $p is broken and falls off!", victim, obj, NULL, TO_CHAR);
	   		unequip_char(victim, obj);
		}
           }
	 }
	   
	 if (IS_NPC(victim))
	    victim->hit_loc[LOC_BD] -= UMAX(1, dam/number_range(1, 2));
	 else
	    victim->hit_loc[LOC_BD] -= UMAX(1, dam/number_range(1, LOC_MOD_BD));
	 break;

    case (LOC_RL):
	 if (!always_hit) {
	   if (number_percent() < 35+defend_bonus)
		dam = 0;
	 }
	 if (dam_type == DAM_CRIT)
  	 {
	   dam *= 1.5;
	   OBJ_DATA * obj;
	   if ((obj = get_eq_char( victim, WEAR_LEGS )) != NULL)
	   {
  		if (!IS_SET(obj->extra_flags, ITEM_NO_BREAK))
		   obj->condition--;
		if (obj->condition <= 0)
		{
			obj->condition = 0; 
	   		SET_BIT(obj->extra_flags, ITEM_BROKEN);
        		act ( "Your $p is broken and falls off!", victim, obj, NULL, TO_CHAR);
	   		unequip_char(victim, obj);
		}
           }
	 }
	 
	 if (IS_NPC(victim))
    	    victim->hit_loc[LOC_RL] -= UMAX(1, dam/number_range(1, 2));
    	 else
    	    victim->hit_loc[LOC_RL] -= UMAX(1, dam/number_range(1, LOC_MOD_RL));
	 break;
	 
    case (LOC_RA):
	 if (!always_hit) {
	   if (number_percent() < 25+defend_bonus)
		dam = 0;
	 }
	 if (dam_type == DAM_CRIT)
  	 {
	   dam *= 2.5;
	   OBJ_DATA * obj;
	   if ((obj = get_eq_char( victim, WEAR_ARMS )) != NULL)
	   {
		obj->condition--;
		if (obj->condition <= 0)
		{
			obj->condition = 0; 
	   		SET_BIT(obj->extra_flags, ITEM_BROKEN);
        		act ( "Your $p is broken and falls off!", victim, obj, NULL, TO_CHAR);
	   		unequip_char(victim, obj);
		}
           }
	 }
	   
	 if (IS_NPC(victim))
    	    victim->hit_loc[LOC_RA] -= UMAX(1, dam/number_range(1, 2));
    	 else
    	    victim->hit_loc[LOC_RA] -= UMAX(1, dam/number_range(1, LOC_MOD_RA));
	 break;    	 
    }
       
    if (show)
	 dam_message( ch, victim, dam, dt, immune, hit_location, dam_type);
   
    if (dam == 0)
	 return FALSE;

    //JAS
    OBJ_DATA *weapon;
    weapon = get_eq_char(ch,WEAR_WIELD);
    if ((weapon != NULL) && (IS_WEAPON_STAT(weapon,WEAPON_TWO_HANDS)))
    {
	dam = (dam * 5) / 4;
    }
    
    // Log real damage
    if ( dam > 2200 && dt >= TYPE_HIT) {    	
	 sprintf(buf, "Real damage <%d> more than 2200 points cause by %s!", dam, IS_NPC(ch) ? ch->short_descr : ch->name);
	 log_string(buf);	
	 wiznet(buf,ch,NULL,WIZ_SECURE,0,0);	 


	 // If real damage over 2200, and victim is channeler - they lose con and channeling
	 if (IS_AFFECTED(victim,AFF_CHANNELING)) {
	   if (victim->sex == SEX_MALE)
		sprintf(buf, "{DThe pain makes your void shatter, and you lose control of %s.{x\n\r", victim->sex == SEX_MALE ? "Saidin" : "Saidar");	   
	   else
		sprintf(buf, "{WThe pain makes you lose control of %s.{x\n\r", victim->sex == SEX_MALE ? "Saidin" : "Saidar");
	   send_to_char(buf, victim);
	   do_function(victim, &do_unchannel, "" );
	 }

    }
    
    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
    if (IS_GHOLAM(ch))
    {
	dam = number_range(800,2200);
    }

    if (!IS_NPC(ch) && get_level(ch) < 20) {
	dam = dam * 3 / 2;
    }
    victim->hit -= dam;
    if ( !IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1 )
	 victim->hit = 1;
    
    // Dragon Reborn will live, if IC!
    if ( !IS_NPC(victim) && IS_DR(victim) && IS_RP(victim) && victim->hit < 1 && !IS_IMMORTAL(ch))
	 victim->hit = 1;
	 
    if ( !IS_NPC(victim) && IS_TAVEREN(victim) && IS_RP(victim) && victim->hit < 10 && !IS_IMMORTAL(ch) && !IS_GHOLAM(victim)) {
         int luck = number_range(1,5);
         
         switch (luck) {

            case 1: // A tree fall over your victim
               act("{WSuddenly a {ytree{W falls and hit $N, {Wknocking $M out!{x", victim, NULL, ch, TO_CHAR    );
               act("{WSuddenly a {ytree{W falls and hit you, {Wknocking you out!{x", victim, NULL, ch, TO_VICT    );               
               act("{WSuddenly a {ytree{W falls and hit $N, {Wknocking $M out!{x", victim, NULL, ch, TO_NOTVICT );               
               victim->hit = 1;
               ch->hit = -100;               
               break;
            case 2: // A amazing critical strike
               act("{WAs if you have the luck of the {DDark One{W, you manage to strike a deadly blow to $N's head!{x", victim, NULL, ch, TO_CHAR    );
               act("{WAs if $n have the luck of the {DDark One{W, $e manage to strike a deadly blow to your head!{x", victim, NULL, ch, TO_VICT    );               
               act("{WAs if $n have the luck of the {DDark One{W, $e manage to strike a deadly blow to $N's head!{x", victim, NULL, ch, TO_NOTVICT );
               victim->hit = 1;
               ch->hit = -100;               
               break;                                       
            case 3: // Slip on a stone
               act("{W$N slip on a stone and knock $E head when falling!{x", victim, NULL, ch, TO_CHAR    );
               act("{WYou slip on a stone and knock your head when falling!{x", victim, NULL, ch, TO_VICT    );               
               act("{W$N slip on a stone and knock $E head when falling!{x", victim, NULL, ch, TO_NOTVICT );               
               victim->hit = 1;
               ch->hit = -100;               
               break;          
            default:
               break;               
        }	
    }
    
    /*
	* Hit location check.. 
	* if critical, tell and set HP according
	*/
    for (i=0; i<MAX_HIT_LOC; i++) {
	 if (victim->hit_loc[i] <= 0 && !immune) {

	   // Fade's don't die from critical blow. 
	   // They die only from HP
	   if (victim->race == race_lookup("fade"))
		break;

	   // Imms don't die from critical blow
	   if (IS_IMMORTAL(victim))
		break;
	   // The DR don't die from a critical blow
	   if (IS_DR(victim)) {
              victim->hit_loc[i] = get_max_hit_loc(victim, i)/2;  	
	      break;
	   }
	   
	   if (IS_TAVEREN(victim) && number_percent() < 44)
	      break;
	   
	   sprintf(buf, "{rYou take a critical blow to the %s, and drop to the ground{x.\r\n", hit_flags[i].name);
	   send_to_char(buf, victim);
	   sprintf(buf, "$n {rtakes a critical blow to the %s, and drops to the ground{x.", hit_flags[i].name);
	   act(buf, victim, NULL, NULL, TO_ROOM);
	   victim->hit = -100;
	   break;
	 }
    }
	 
    update_pos( victim );

    switch( victim->position )
    {
    case POS_MORTAL:
	act( "$n is mortally wounded, and will die soon, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char( 
	    "You are mortally wounded, and will die soon, if not aided.\r\n",
	    victim );
	break;

    case POS_INCAP:
	act( "$n is incapacitated and will slowly die, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char(
	    "You are incapacitated and will slowly die, if not aided.\r\n",
	    victim );
	break;

    case POS_STUNNED:
	act( "$n is stunned, but will probably recover.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char("You are stunned, but will probably recover.\r\n",
	    victim );
	break;

    case POS_DEAD:
	 if (!IS_NPC(victim)) {
	   do_bondnotify(victim, "subdued");
	   act( "$n has been {rSUBDUED!!{x", victim, 0, 0, TO_ROOM );
	   send_to_char( "You have been {rSUBDUED!!{x\r\n", victim );
	   	  
	   victim->pcdata->last_subdue = current_time;
	   
	   if (IS_AFFECTED(victim,AFF_CHANNELING)) {
		do_function(victim, &do_unchannel, "" );
	   }	     	   
	 }
	 else {
	   act( "$n has been {rKILLED!!{x", victim, 0, 0, TO_ROOM );
	   send_to_char( "You have been {rKILLED!!{x\r\n", victim );
	 }
	 
    default:
	 if ( dam > victim->max_hit / 4 )
	   send_to_char( "That really did {WHURT!{x\r\n", victim );
	 //if ( victim->hit < victim->max_hit / 4 )
	 //  send_to_char( "You sure are {rBLEEDING!{x\r\n", victim );
	 break;
    }
    
    /*
     * Sleep spells and extremely wounded folks.
     */
    if ( !IS_AWAKE(victim) )
	 stop_fighting( victim, FALSE );
    
    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD )
    {
	group_gain( ch, victim, FALSE );

	if ( !IS_NPC(victim) )
	{
          // Trolloc clan promotion by PK
           check_trolloc_kill(ch, victim);
	  sprintf( log_buf, "%s was killed by %s at room %d [AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s, PCvsPC=%s]",
			 victim->name,
			 (IS_NPC(ch) ? ch->short_descr : ch->name),
			 ch->in_room->vnum,
			 IS_SET(victim->comm,COMM_AFK) ? "Yes" : "No",
			 victim->timer > 3             ? "Yes" : "No",
			 victim->timer,
			 IS_RP(ch)                     ? "Yes" : "No",
			 IS_RP(victim)                 ? "Yes" : "No",
			 position_table[victim->position].name,
			 (!IS_NPC(ch) && !IS_NPC(victim)) ? "Yes" : "No");
	    log_string( log_buf );

	    if (!IS_NPC(ch))
            {
    	       sprintf(log_buf,"%s was killed by %s at room %d\r\n%s - %s \r\n[AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]\r\n",
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

              make_note("PK", ch->name, "Admin", "I'm playing at PK", 30, log_buf);
 	     }


	    /*
	     * Dying penalty:
	     * 2/3 way back to previous level.
	     */
/*	     
	    if ( victim->exp > exp_next_level(victim) 
		    * victim->level )
*/
         if (victim->exp > 0 && IS_NPC(ch)) {
            sub_xp = victim->exp/4;
	    gain_exp(victim, -sub_xp);
         }
		 
       }


	  if (!IS_NPC(victim)) {
	    sprintf( log_buf, "%s was killed by %s at %s [room %d] [AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]",
			   (IS_NPC(victim) ? victim->short_descr : victim->name),
			   (IS_NPC(ch) ? ch->short_descr : ch->name),
			   ch->in_room->name, ch->in_room->vnum,
			   IS_SET(victim->comm,COMM_AFK) ? "Yes" : "No",
			   victim->timer > 3             ? "Yes" : "No",
			   victim->timer,
			   IS_RP(ch)                     ? "Yes" : "No",
			   IS_RP(victim)                 ? "Yes" : "No",
			   position_table[victim->position].name);
	  }
	else {
        sprintf( log_buf, "%s [%d] was killed by %s at %s [room %d]",
            (IS_NPC(victim) ? victim->short_descr : victim->name),
	    victim->pIndexData->vnum,
            (IS_NPC(ch) ? ch->short_descr : ch->name),
            ch->in_room->name, ch->in_room->vnum );		
	}
 
        if (IS_NPC(victim))
            wiznet(log_buf,NULL,NULL,WIZ_MOBDEATHS,0,0);
        else
            wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);

	if (!IS_NPC(ch)
	&& !IS_NPC(victim)
	&& victim->pcdata->bounty > 0  && IS_RP(ch) && IS_RP(victim))
	{
	    sprintf(buf,"You recive a %d gold bounty, for killing %s.\r\n",
	    victim->pcdata->bounty, victim->name);
            send_to_char(buf, ch);
	    ch->gold += victim->pcdata->bounty;
	    victim->pcdata->bounty =0;
	}
	/*
	 * Death trigger
	 */
	if ( IS_NPC( victim ) && HAS_TRIGGER( victim, TRIG_DEATH) )
	{
	    victim->position = POS_STANDING;
	    mp_percent_trigger( victim, ch, NULL, NULL, TRIG_DEATH );
	}

        raw_kill( victim );
	check_valid_pkill(ch,victim);
        /* dump the flags */
        if (ch != victim && !IS_NPC(ch) && !is_same_clan(ch,victim))
        {
            if (IS_SET(victim->act,PLR_KILLER))
                REMOVE_BIT(victim->act,PLR_KILLER);
            else
                REMOVE_BIT(victim->act,PLR_THIEF);
        }

	if (!IS_NPC(victim) && IS_SET(victim->in_room->room_flags, ROOM_ARENA))
        {
  	       do_restore(victim,victim->name);
	       act( "$n is subdued and removed from the Arena!  Thank you for playing!\r\n", victim, 0, 0, TO_ROOM );
	       send_to_char("You have been subdued in the Arena! Thank you for playing!\r\n",victim);
               victim->pcdata->next_recall = current_time;
	       victim->position = POS_SITTING;
	       sprintf(buf,"Arena: %c\r\n",ch->arena);
	       log_string(buf);
               if (ch->arena == 'a')
               {
		  do_arena(victim,"a");
               }
	       else
	       if (ch->arena == 'b')
               {
		  do_arena(victim,"b");
               }
               else
	       if (is_clan(victim))
	       {
		  do_grecall(victim,"");
	       }
	       else
               {
		  do_recall(victim,"");
               }
 	   }
        /* RT new auto commands */

	if (!IS_NPC(ch)
	&&  (corpse = get_obj_list(ch,"corpse",ch->in_room->contents)) != NULL
	&&  corpse->item_type == ITEM_CORPSE_NPC && can_see_obj(ch,corpse))
	{
	    OBJ_DATA *coins;

	    corpse = get_obj_list( ch, "corpse", ch->in_room->contents ); 

	    if ( IS_SET(ch->act, PLR_AUTOEXAMINE) && corpse) {
	       do_function(ch, &do_examine, "corpse");	
	    }

	    if ( IS_SET(ch->act, PLR_AUTOLOOT) &&
		 corpse && corpse->contains) /* exists and not empty */
            {
		do_function(ch, &do_get, "all corpse");
	    }

 	    if (IS_SET(ch->act,PLR_AUTOGOLD) &&
	        corpse && corpse->contains  && /* exists and not empty */
		!IS_SET(ch->act,PLR_AUTOLOOT))
	    {
		if ((coins = get_obj_list(ch,"gcash",corpse->contains))
		     != NULL)
		{
		    do_function(ch, &do_get, "all.gcash corpse");
	      	}
	    }
            
	    if (IS_SET(ch->act, PLR_AUTOSAC))
	    {
       	        if (IS_SET(ch->act,PLR_AUTOLOOT) && corpse && corpse->contains)
       	      	{
		    return TRUE;  /* leave if corpse has treasure */
	      	}
	        else
		{
		    do_function(ch, &do_sacrifice, "corpse");
		}
	    }
	}

	return TRUE;
    }

    if ( victim == ch )
	return TRUE;

    /*
     * Take care of link dead people.
     */
    if ( !IS_NPC(victim) && victim->desc == NULL )
    {
    	stop_fighting(victim, TRUE);    	
    	
//	if ( number_range( 0, victim->wait ) == 0 )
//	{
//	    do_function(victim, &do_recall, "" );
//	    return TRUE;
//	}
    }

    /*
     * Wimp out NPCs?
     */
    if ( IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2)
	 {
	   if ( ( IS_SET(victim->act, ACT_WIMPY) && number_bits( 2 ) == 0
			&&   victim->hit < victim->max_hit / 5) 
		   ||   ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL
				&&     victim->master->in_room != victim->in_room ) )
		{
		  do_function(victim, &do_flee, "" );
		}
	 }

    // Wimp out PCs
    if (!IS_NPC(victim) && (victim->hit > 0) && (victim->hit <= victim->wimpy) && (victim->wait < PULSE_VIOLENCE/2)) {
	 do_function (victim, &do_flee, "" );
    }
    
    tail_chain( );
    return TRUE;
}

bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim)
{
  if (victim->in_room == NULL || ch->in_room == NULL)
    return TRUE;
  
  if (victim->fighting == ch || victim == ch)
    return FALSE;
  
  if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL)
    return FALSE;
  
  /* killing mobiles */
  if (IS_NPC(victim)) {
    
    /* safe room? */
    if (IS_SET(victim->in_room->room_flags,ROOM_SAFE)) {
	 send_to_char("Not in this room.\r\n",ch);
	 return TRUE;
    }

    if (victim->pIndexData->pShop != NULL) {
	 send_to_char("The shopkeeper wouldn't like that.\r\n",ch);
	 return TRUE;
    }

    if ( victim->spec_fun == spec_lookup( "spec_questmaster" )) {
	 send_to_char("You can't kill the questmaster.\r\n",ch);
	 return TRUE;
    }

    if (IS_PKILLER(victim) && current_time < victim->next_pkill)
    {
	send_to_char("They were just pkilled.  Give them a chance to recover.\r\n",ch);
	return TRUE;
    }

    if (IS_PKILLER(ch) && (current_time < victim->next_pkill) && !IS_NPC(victim))
    {
	send_to_char("You were just pkilled.  Take the chance to recover.\r\n",ch);
	return TRUE;

    }
    
    /* no killing healers, trainers, etc */
    if (IS_SET(victim->act,ACT_TRAIN)     ||  
	   IS_SET(victim->act,ACT_GAIN)      ||  
	   IS_SET(victim->act,ACT_IS_HEALER) ||
	   IS_SET(victim->act, ACT_REPAIRER) ||
	   IS_SET(victim->act,ACT_IS_CHANGER)) {
	 send_to_char("I don't think the Creator would approve.\r\n",ch);
	 return TRUE;
    }

    if (!IS_NPC(ch)) {
	 /* no pets */
	 if (IS_SET(victim->act,ACT_PET)) {
	   act("But $N looks so cute and cuddly...", ch,NULL,victim,TO_CHAR);
	   return TRUE;
	 }
	 
	 /* no charmed creatures unless owner */
	 /*
	 if (IS_AFFECTED(victim,AFF_CHARM) && ch != victim->master) {
	   send_to_char("You don't own that monster.\r\n",ch);
	   return TRUE;
	 }
	 */
    }
  }
  /* killing players */
  else {
    /* NPC doing the killing */
    if (IS_NPC(ch)) {
	 /* safe room check */
	 if (IS_SET(victim->in_room->room_flags,ROOM_SAFE)) {
	   send_to_char("Not in this room.\r\n",ch);
	   return TRUE;
	 }
	 
	 /* charmed mobs and pets cannot attack players while owned */
	 if (IS_AFFECTED(ch,AFF_CHARM) && ch->master != NULL
		&&  ch->master->fighting != victim) {
	   send_to_char("Players are your friends!\r\n",ch);
	   return TRUE;
	 }
    }

	/* player doing the killing */
    else {
           /* Disregard below*****
	    * if (!is_clan(ch))
	    * {
		send_to_char("Join a clan if you want to kill players.\r\n",ch);
		return TRUE;
	    }


	    if (!is_clan(victim))
	    {
		send_to_char("They aren't in a clan, leave them alone.\r\n",ch);
		return TRUE;
	    }

	    if (ch->level > victim->level + 8)
	    {
		send_to_char("Pick on someone your own size.\r\n",ch);
		return TRUE;
	    }
            */
        /* safe room? */
        if (IS_SET(victim->in_room->room_flags, ROOM_SAFE)) {
	    send_to_char("Not in this room.\r\n",ch);
	    return TRUE;
        }            
            
	 if (IS_SET(victim->act,PLR_KILLER) || IS_SET(victim->act,PLR_THIEF))
	   return FALSE;
    }
  }
  return FALSE;
}
 
bool is_safe_spell(CHAR_DATA *ch, CHAR_DATA *victim, bool area )
{
  if (victim->in_room == NULL || ch->in_room == NULL)
    return TRUE;
  
  if (victim == ch && area)
    return TRUE;
  
  if (victim->fighting == ch || victim == ch)
    return FALSE;
  
  if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL && !area)
    return FALSE;
  
  if (IS_GHOLAM(victim))
  {
	send_to_char("Your weaving has no affect on this person!!\r\n",ch);
        if (!IS_NPC(victim))
        	act("$n just tried to weave on you!\r\n",ch,NULL,victim,TO_VICT);  
	return TRUE;
  }
    
  /* killing mobiles */
  if (IS_NPC(victim)) {
    if ( victim->spec_fun == spec_lookup( "spec_questmaster" )) {
	 send_to_char("You can't kill the questmaster.\r\n",ch);
	 return TRUE;
    }

    /* safe room? */
    if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
	 return TRUE;
    
    if (victim->pIndexData->pShop != NULL)
	 return TRUE;
    
    /* no killing healers, trainers, etc */
    if (IS_SET(victim->act,ACT_TRAIN)
	   ||  IS_SET(victim->act,ACT_GAIN)
	   ||  IS_SET(victim->act,ACT_IS_HEALER)
	   ||  IS_SET(victim->act, ACT_REPAIRER)
	   ||  IS_SET(victim->act,ACT_IS_CHANGER))
	 return TRUE;
    
    if (!IS_NPC(ch)) {
	 /* no pets */
	 if (IS_SET(victim->act,ACT_PET))
	   return TRUE;
	 
	 /* no charmed creatures unless owner */
	 if (IS_AFFECTED(victim,AFF_CHARM) && (area || ch != victim->master))
	   return TRUE;
	 
	 /* legal kill? -- cannot hit mob fighting non-group member */
	 if (victim->fighting != NULL && !is_same_group(ch,victim->fighting))
	   return TRUE;
    }
    else {
	 /* area effect spells do not hit other mobs */
	 if (area && !is_same_group(victim,ch->fighting))
	   return TRUE;
    }
  }
  /* killing players */
  else {
    if (area && IS_IMMORTAL(victim) && victim->level > LEVEL_IMMORTAL)
	 return TRUE;
    
    /* NPC doing the killing */
    if (IS_NPC(ch)) {
	 /* charmed mobs and pets cannot attack players while owned */
	 if (IS_AFFECTED(ch,AFF_CHARM) && ch->master != NULL
		&&  ch->master->fighting != victim)
	   return TRUE;
	 
	 /* safe room? */
	 if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
	   return TRUE;
	 
	 /* legal kill? -- mobs only hit players grouped with opponent*/
	 if (ch->fighting != NULL && !is_same_group(ch->fighting,victim))
	   return TRUE;
    }
    
    /* player doing the killing */
    else {
	 if (!is_clan(ch))
	   return TRUE;
	 
	 if (IS_SET(victim->act,PLR_KILLER) || IS_SET(victim->act,PLR_THIEF))
	   return FALSE;
	 
	 if (!is_clan(victim))
	   return TRUE;
	 
	 if (ch->level > victim->level + 8)
	   return TRUE;
    }    
  }
  return FALSE;
}
/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char buf[MAX_STRING_LENGTH];
    /*
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */
    while ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL )
	victim = victim->master;

    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if ( IS_NPC(victim)
    ||   IS_SET(victim->act, PLR_KILLER)
    ||   IS_SET(victim->act, PLR_THIEF))
	return;

    /*
     * Charm-o-rama.
     */
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
    {
	if ( ch->master == NULL )
	{
	    char buf[MAX_STRING_LENGTH];

	    sprintf( buf, "Check_killer: %s bad AFF_CHARM",
		IS_NPC(ch) ? ch->short_descr : ch->name );
	    bug( buf, 0 );
	    affect_strip( ch, gsn_charm_person );
	    REMOVE_BIT( ch->affected_by, AFF_CHARM );
	    return;
	}
/*
	send_to_char( "*** You are now a KILLER!! ***\r\n", ch->master );
  	SET_BIT(ch->master->act, PLR_KILLER);
*/

	//stop_follower( ch );
	return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC(ch)
    ||   ch == victim
    ||   ch->level >= LEVEL_IMMORTAL
    ||	 ch->fighting  == victim)
	return;

/*
    send_to_char( "*** You are now a KILLER!! ***\r\n", ch );
    SET_BIT(ch->act, PLR_KILLER);
*/
    sprintf(buf,"$N is attempting to murder %s at room %d [AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]",
		  victim->name, 
		  ch->in_room->vnum, 
		  IS_SET(victim->comm,COMM_AFK) ? "Yes" : "No",
		  victim->timer > 3             ? "Yes" : "No",
		  victim->timer,
		  IS_RP(ch)                     ? "Yes" : "No",
		  IS_RP(victim)                 ? "Yes" : "No",
		  position_table[victim->position].name);
    wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);

    
    if (!IS_NPC(ch)) {
	 sprintf(buf,"%s is attempting to murder %s at room %d [AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]",
		    !IS_NPC(ch) ? ch->name : ch->short_descr,
		    victim->name, 
		    ch->in_room->vnum, 
		    IS_SET(victim->comm,COMM_AFK) ? "Yes" : "No",
		    victim->timer > 3             ? "Yes" : "No",
		    victim->timer,
		    IS_RP(ch)                     ? "Yes" : "No",
		    IS_RP(victim)                 ? "Yes" : "No",
		    position_table[victim->position].name);
	 
	 log_string(buf);
    }
    
    sprintf(buf,"%s is attempting to murder %s at room %d\r\n%s - %s \r\n[AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]\r\n",
		    !IS_NPC(ch) ? ch->name : ch->short_descr,
		    victim->name, 
		    ch->in_room->vnum, 
                    !IS_NULLSTR(ch->in_room->name) ? ch->in_room->name : "(none)", 
                    ch->in_room->area->name,
		    IS_SET(victim->comm,COMM_AFK) ? "Yes" : "No",
		    victim->timer > 3             ? "Yes" : "No",
		    victim->timer,
		    IS_RP(ch)                     ? "Yes" : "No",
		    IS_RP(victim)                 ? "Yes" : "No",
		    position_table[victim->position].name);
	 
    //make_note("PK", ch->name, "Admin", "I'm playing at PK", 30, buf);
    save_char_obj( ch, FALSE );
    return;
}

/*
 * Check if Duelling parry
 */
bool check_du_parry( CHAR_DATA *ch, CHAR_DATA *victim )
{
  OBJ_DATA *wielded=NULL;
  OBJ_DATA *dual_wielded=NULL;
  int du_skill;
  int du_number;
  int wtype=0;

  if (!IS_SET(victim->auto_act, AUTO_MASTERFORMS))
    return FALSE;
  
  du_skill = get_skill(victim, gsn_duelling);
  
  wtype = get_weapon_sn(victim);
  
  if (wtype != gsn_dagger)
    return FALSE;
  
  wielded      = get_eq_char( victim, WEAR_WIELD );
  dual_wielded = get_eq_char( victim, WEAR_SECOND_WIELD );
  
  if (du_skill && (number_percent() <= (du_skill/4 ))) {
    du_number = number_range(0,1);
    switch(du_number) {
    case (0):
	 // If dual wielding daggers
	 if ((wielded != NULL && wielded->value[0] == WEAPON_DAGGER) &&
		(dual_wielded != NULL && dual_wielded->value[0] == WEAPON_DAGGER)) {

	   // different msg depending on ch wielding or not
	   if (get_eq_char( ch, WEAR_WIELD ) == NULL) {
		// Free kick
		if (number_percent() <= 15) {
		  act( "You {Gcross your daggers{x in a risky manuever, parrying $n's attack and aim a solid kick!", ch, NULL, victim, TO_VICT);
		  act( "$N {Gcross $S daggers{x in a risky manuever, parrying your attack and aim a solid kick!", ch, NULL, victim, TO_CHAR);
		  act( "$N {Gcross $S daggers{x in a risky manuever, parrying $n's attack and aim a solid kick!", ch, NULL, victim, TO_NOTVICT);
		  do_kick(ch, "");
		}
		else {
		  act( "You {Gcross your daggers{x in a risky manuever, parrying $n's attack!", ch, NULL, victim, TO_VICT);
		  act( "$N {Gcross $S daggers{x in a risky manuever, parrying your attack!", ch, NULL, victim, TO_CHAR);
		  act( "$N {Gcross $S daggers{x in a risky manuever, parrying $n's attack!", ch, NULL, victim, TO_NOTVICT);
		}
		
		return TRUE;
	   }
	   else {
		// Free kick
		if (number_percent() <= 20) {
		  act( "You {Gcross your daggers{x in a risky manuever, parrying $n's attack with a loud clang of steel and a solid kick in progress!", ch, NULL, victim, TO_VICT);
		  act( "$N {Gcross $S daggers{x in a risky manuever, parrying your attack with a load clang of steel and a solid kick in progress!", ch, NULL, victim, TO_CHAR);
		  act( "$N {Gcross $S daggers{x in a risky manuever, parrying $n's attack with a load clang of steel and a solid kick in progress!", ch, NULL, victim, TO_NOTVICT);
		  do_kick(ch, "");
		}
		else {
		  act( "You {Gcross your daggers{x in a risky manuever, parrying $n's attack with a loud clang of steel!", ch, NULL, victim, TO_VICT);
		  act( "$N {Gcross $S daggers{x in a risky manuever, parrying your attack with a load clang of steel!", ch, NULL, victim, TO_CHAR);
		  act( "$N {Gcross $S daggers{x in a risky manuever, parrying $n's attack with a load clang of steel!", ch, NULL, victim, TO_NOTVICT);
		}
		return TRUE;
	   }
	 }
	 else
	   return FALSE;	 
	 break;
    case (1):
	 act( "You dodge $n's attack with a {Gquick duck and roll{x!", ch, NULL, victim, TO_VICT);
	 act( "$N dodge your attack with a {Gquick duck and roll{x!", ch, NULL, victim, TO_CHAR);
	 act( "$N dodge $n's attack with a {Gquick duck and roll{x!", ch, NULL, victim, TO_NOTVICT);
	 return TRUE;
	 break;
    }
  }

  return FALSE;
}

/*
 * Check if Blademaster parry
 */
bool check_bm_parry( CHAR_DATA *ch, CHAR_DATA *victim )
{
  int bm_skill;
  int bm_number;
  char mf_defendstring[MAX_STRING_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  
  if (!IS_SET(victim->auto_act, AUTO_MASTERFORMS))
    return FALSE;
  
  bm_skill = getmfskill(victim);

  if (bm_skill && (number_percent() <= (bm_skill/4 ))) {
    bm_number = number_range(0, 5);
    strcpy(mf_defendstring,get_master_defend(find_relevant_masterform(victim),bm_number));  
    sprintf(buffer,"{w$n parries your attack with %s!{x",mf_defendstring);
    act(buffer,victim,NULL,ch,TO_VICT);
    sprintf(buffer,"{w$n parries $N's attack with %s!{x",mf_defendstring);
    act(buffer,victim,NULL,ch,TO_NOTVICT);
    sprintf(buffer,"{wYou parry $N's attack with %s!{x",mf_defendstring);
    act(buffer,victim,NULL,ch,TO_CHAR);
    return TRUE;
  }  

  return FALSE;
}


bool check_weapon_destroy(CHAR_DATA *attacker, CHAR_DATA *attacked)
{
  int dlvl, dwlvl, dskill, rlvl, rwlvl, rskill, sn;
  int rone, rtwo;
  OBJ_DATA *dw;
  OBJ_DATA *rw;

  dlvl = attacked->level;
  dw   = get_eq_char(attacked, WEAR_WIELD);

  rlvl = attacker->level;
  sn = get_weapon_sn(attacker);
  rw = get_eq_char( attacker, WEAR_WIELD );

  
  if (rw == NULL)
    return FALSE;
  
  if (dw == NULL) {
    dw = get_eq_char (attacked, WEAR_HOLD);
    if (dw == NULL)
	 return FALSE;
    else {
	 dwlvl = dw->level;
	 rwlvl = rw->level;
    }
  }
  dwlvl = dw->level;
  rwlvl = rw->level;

  rskill = get_weapon_skill(attacker,sn);
  dskill = get_weapon_skill(attacked,get_weapon_sn(attacked));

  rone = number_percent ();
  rtwo = number_percent ();

  // Attacked is nobreak
  if (IS_SET(dw->extra_flags, ITEM_NO_BREAK))
    return FALSE;
  
  // Both Magical
  else if (IS_SET(rw->extra_flags, ITEM_MAGIC) && 
		 IS_SET(dw->extra_flags, ITEM_MAGIC)) {
    if (rone > 98)
	 if ((rlvl+rwlvl+rskill+75) > (dlvl+dwlvl+dskill+75+rtwo)) {
	   dw->condition -= number_range(1,5);
	   if (dw->condition <= 0)
		return TRUE;
	 }
  }

  // Only attacker magical
  else if (IS_SET(rw->extra_flags, ITEM_MAGIC) && 
		 !IS_SET(dw->extra_flags, ITEM_MAGIC)) {
    if (rone > 96)
	 if ((rlvl+rwlvl+rskill+75) > (dlvl+dwlvl+dskill+rtwo)) {
	   dw->condition -= number_range(1,5);
	   if (dw->condition <= 0)
		return TRUE;
	 }
  }
  
  // Only attacked magical
  else if (!IS_SET(rw->extra_flags, ITEM_MAGIC) &&
		 IS_SET(dw->extra_flags, ITEM_MAGIC)) {
    return FALSE;
  }

  // Both Mundane
  else {
    if (rone > 98) 
	 if ((rlvl+rwlvl+rskill) > (dlvl+dwlvl+dskill+rtwo)) {
	   dw->condition -= number_range(1,5);
	   if (dw->condition <= 0)
		return TRUE;
	 }
  }
  
  return FALSE;
}

/*
 * Check for parry.
 */
bool check_parry( CHAR_DATA *ch, CHAR_DATA *victim )
{
  OBJ_DATA *wielded;
  OBJ_DATA *dual_wielded;
  OBJ_DATA *weapon=NULL;
  int chance;
  
  if ( !IS_AWAKE(victim) )
    return FALSE;
  
  //chance = get_skill(victim,gsn_parry) / 2;
  chance = get_skill(victim,gsn_parry) / 5;
  
  if ((wielded = get_eq_char( victim, WEAR_WIELD )) == NULL ) {
    if (IS_NPC(victim))
	 chance /= 2;
    else
	 return FALSE;
  }

  // If using a staff, more chance to parry
  if (wielded != NULL &&  wielded->item_type == ITEM_WEAPON && wielded->value[0] == WEAPON_STAFF)
    chance += 5;
  
  // If dual wielding, a little more chance to parry
  if ((dual_wielded = get_eq_char( victim, WEAR_SECOND_WIELD )) != NULL ) {
    chance += get_skill(victim,gsn_dual_wield) / 10;
    
    // If ambidex, even more
    if (IS_SET(victim->merits, MERIT_AMBIDEXTROUS))
	 chance += 10;
  }
  
  // If quick reflexes
  if  (IS_SET(ch->merits, MERIT_QUICKREFLEXES)) {
    chance += 25;
  }
  
  if (!can_see(ch,victim))
    chance /= 2;
  
  if ( number_percent( ) >= chance + victim->level - ch->level )
  //if ( number_percent( ) >= chance + get_level(victim) - get_level(ch) )
    return FALSE;
  
  if (wielded) {
    if (dual_wielded && (number_percent() > 50) ) {
	 if (!check_bm_parry(ch, victim) && !check_du_parry(ch, victim)) {
	   act( "You parry $n's attack with $p.",  ch, dual_wielded, victim, TO_VICT    );
	   act( "$N parries your attack with $p.", ch, dual_wielded, victim, TO_CHAR    );
	   act( "$N parries $n's attack with $p.", ch, dual_wielded, victim, TO_NOTVICT );
	   weapon = dual_wielded;
	 }
    }
    else {
	 if (!check_bm_parry(ch, victim) && !check_du_parry(ch, victim)) {
	   act( "You parry $n's attack with $p.",  ch, wielded, victim, TO_VICT    );
	   act( "$N parries your attack with $p.", ch, wielded, victim, TO_CHAR    );
	   act( "$N parries $n's attack with $p.", ch, wielded, victim, TO_NOTVICT );
	   weapon = wielded;
	 }
    }
  }
  else {
    act( "You parry $n's attack with your arm.",  ch, NULL, victim, TO_VICT    );
    act( "$N parries your attack with $S arm.", ch, NULL, victim, TO_CHAR    );
    act( "$N parries $n's attack with $S arm", ch, NULL, victim, TO_NOTVICT );
  }

  // cost endurance to parry
  victim->endurance -= 8;

  if (IS_SET(victim->merits, MERIT_HUGESIZE))
    victim->endurance -= 4;

  if (victim->endurance < 1 )
    victim->endurance = 0;
  
  check_improve(victim,gsn_parry,TRUE,6);

  if (number_percent() > 25) {
    if (check_weapon_destroy(victim, ch)){
	 weapon = get_eq_char(ch, WEAR_WIELD);
	 if (weapon == NULL)
	   return FALSE;
	 else {
	   act ( "As $N parries you, your $p is {Rdestroyed{x!", ch, weapon, victim, TO_CHAR);
	   act ( "You {Rdestroy{x $n's $p as you parry $m.", ch, weapon, victim, TO_VICT);
	   act ( "$N {Rdestroy{x $n's $p with a powerfull parry.", ch, weapon, victim, TO_NOTVICT);
	   weapon->condition = 0;
	   SET_BIT(weapon->extra_flags, ITEM_BROKEN);
	   unequip_char(ch, weapon);
	   victim->endurance -= 2;
	   return TRUE;
	 }
    }
    
    if (check_weapon_destroy(ch, victim)) {     
	 weapon = get_eq_char(victim, WEAR_WIELD);
	 if (weapon == NULL)
	   return FALSE;
	 else {
        act ( "As $N parries you, your $p is {Rdestroyed{x!", ch, weapon, victim, TO_CHAR);
	   act ( "You {Rdestroy{x $n's $p as you parry $m.", ch, weapon, victim, TO_VICT);
	   act ( "$N {Rdestroy{x $n's $p with a powerfull parry.", ch, weapon, victim, TO_NOTVICT);
	   weapon->condition = 0;
	   SET_BIT(weapon->extra_flags, ITEM_BROKEN);
	   unequip_char(victim, weapon);
	   ch->endurance -= 2;
	   return TRUE;
	 }  	
    }
  }
  
  /*
  if (!IS_NPC(ch))
    ch->gain_xp = TRUE;
  */
  
  return TRUE;
}

bool check_shield_destroy(CHAR_DATA *attacker, CHAR_DATA *attacked)
{
  int dlvl, dslvl, dskill, rlvl, rwlvl, rskill, sn;
  OBJ_DATA *ds;
  OBJ_DATA *rw;

  dlvl = attacked->level;
  ds = get_eq_char(attacked, WEAR_SHIELD);

  rlvl = attacker->level;
  sn = get_weapon_sn(attacker);
  rw = get_eq_char( attacker, WEAR_WIELD );


  if (ds == NULL || rw == NULL)
    return FALSE;
  else {
    dslvl = ds->level;
    rwlvl = rw->level;
  }
  
  rskill = get_weapon_skill(attacker, sn);
  if (sn == gsn_axe)
    rskill += 15;
  
  dskill = get_skill(attacked, gsn_shield_block);
  
  // No break?
  if ( (rw == NULL) ||
	  (ds == NULL) ||
	  IS_SET(ds->extra_flags, ITEM_NO_BREAK) ) {
    return FALSE;
  }  
  // Both magical
  else if (IS_SET(rw->extra_flags, ITEM_MAGIC) &&
		 (IS_SET(ds->extra_flags, ITEM_MAGIC)) &&
		 number_percent() > 95) {
    if ((rlvl+rwlvl+rskill+75) > (dlvl+dslvl+dskill+75+number_percent())) {
	 ds->condition -= number_range(1,2);
	 if (ds->condition <= 0)
	   return TRUE;
    }
  }

  // Only rw magical
  else if (IS_SET(rw->extra_flags, ITEM_MAGIC) &&
		 (!IS_SET(ds->extra_flags, ITEM_MAGIC)) &&
		 number_percent() > 92) {
    if ((rlvl+rwlvl+rskill+75) > (dlvl+dslvl+dskill+number_percent())) {
	 ds->condition -= number_range(1,2);
	 if (ds->condition <= 0)
	   return TRUE;
    }
  }

  // Only ds magical
  else if (!IS_SET (rw->extra_flags, ITEM_MAGIC) && 
		 (IS_SET (ds->extra_flags, ITEM_MAGIC)) ) {
    return FALSE;
  }

  // Both mundane
  else {
    if (number_percent() > 95) {
	 if((rlvl+rwlvl+rskill) > (dlvl+dslvl+dskill+number_percent())) {
	   ds->condition -= number_range(1,2);
	   if (ds->condition <= 0)
		return TRUE;
	 }
    }
  }
  
  return FALSE;  
}

/*
 * Check for shield block.
 */
bool check_shield_block( CHAR_DATA *ch, CHAR_DATA *victim )
{
  OBJ_DATA *shield;
  int chance;
  
  if ( !IS_AWAKE(victim) )
    return FALSE;
  
  //chance = get_skill(victim,gsn_shield_block) / 5 + 3;
  chance = get_skill(victim,gsn_shield_block) / 3;
  
  if ((shield = get_eq_char( victim, WEAR_SHIELD )) == NULL )
    return FALSE;
  
  // If quick reflexes
  if  (IS_SET(ch->merits, MERIT_QUICKREFLEXES)) {
    chance += 25;
  }

  if ( number_percent( ) >= chance + victim->level - ch->level )
  //if ( number_percent( ) >= chance + get_level(victim) - get_level(ch) )
    return FALSE;

  if (number_percent() <= chance/4) {
    act( "You block $n's attack with $p, turning away $s weapon for a {Yfree attack!{x",  ch, shield, victim, TO_VICT  );
    act( "$N blocks your attack with $p, turning away your weapon for a {Yfree attack!{x", ch, shield, victim, TO_CHAR  );
    act( "$N blocks $n's attack with $p, turning away $s weapon for a {Yfree attack!{x", ch, shield, victim, TO_NOTVICT );
    one_hit(victim, ch, TYPE_UNDEFINED, FALSE, 0);
  }
  else {
    act( "You block $n's attack with $p.",  ch, shield, victim, TO_VICT );
    act( "$N blocks your attack with $p.", ch, shield, victim, TO_CHAR );
    act( "$N blocks $n's attack with $p.", ch, shield, victim, TO_NOTVICT );
  }

  // Cost endurance to block
  victim->endurance -= 10;

  if (IS_SET(victim->merits, MERIT_HUGESIZE))
    victim->endurance -= 5;  
  
  if (victim->endurance < 1 )
    victim->endurance = 0;
  
  check_improve(victim,gsn_shield_block,TRUE,6);

  // New check for shield destroy
  if (number_percent() > 40) {
    if ( check_shield_destroy(ch, victim)) {
	 act( "Your $p is {Rdestroyed{x by $n's blow!", ch, shield, victim, TO_VICT );
	 act( "You {Rdestroy{x $N's $p!", ch, shield, victim, TO_CHAR );
	 act( "$n {Rdestroy{x $N's $p with a powerful blow!", ch, shield, victim, TO_NOTVICT );

	 shield->condition = 0;
	 SET_BIT(shield->extra_flags, ITEM_BROKEN);
	 
	 unequip_char(victim, shield); 
	 
	 return FALSE;
    }
  }
  
  /*
  if (!IS_NPC(ch))
    ch->gain_xp = TRUE;
  */
  
  return TRUE;
}


/*
 * Check for dodge.
 */
bool check_dodge( CHAR_DATA *ch, CHAR_DATA *victim )
{
  int chance;
  
  if ( !IS_AWAKE(victim) )
    return FALSE;
  
  chance = get_skill(victim,gsn_dodge) / 2;
  
  if (IS_WOLFSHAPE(victim))
    chance += get_curr_stat(victim,STAT_DEX)/2;
  
  if (!can_see(victim,ch))
    chance /= 2;
  
  // If quick reflexes
  if  (IS_SET(ch->merits, MERIT_QUICKREFLEXES)) {
    chance += 15;
  }
  
  if ( number_percent( ) >= chance + victim->level - ch->level )
    //if ( number_percent( ) >= chance + get_level(victim) - get_level(ch) )
    return FALSE;
  
  if (victim->race == race_lookup("fade")) {
    act( "Your body flow away from $n's attack like {Bwater over rocks{x.", ch, NULL, victim, TO_VICT    );
    act( "$N's body flows away from your attack like {Bwater over rocks{x.", ch, NULL, victim, TO_CHAR    );
    act( "$N's body flows away from $n's attack like {Bwater over rocks{x.", ch, NULL, victim, TO_NOTVICT );
  }
  else {
    act( "You dodge $n's attack.", ch, NULL, victim, TO_VICT    );
    act( "$N dodges your attack.", ch, NULL, victim, TO_CHAR    );
    act( "$N dodges $n's attack.", ch, NULL, victim, TO_NOTVICT );
  }
  
  check_improve(victim,gsn_dodge,TRUE,6);

  // Cost endurance to dodge
  victim->endurance -= 6;

  if (IS_SET(victim->merits, MERIT_HUGESIZE))
    victim->endurance -= 3;

  if (victim->endurance < 1 )
    victim->endurance = 0;
  
  return TRUE;
}


/*
 * Set position of a victim.
 */
void update_pos( CHAR_DATA *victim )
{  
  if ( victim->hit > 0) {
    if ( victim->position <= POS_STUNNED )
	 victim->position = POS_SLEEPING;
    return;
  }
  
  if ( IS_NPC(victim) && victim->hit < 1 ) {
    victim->position = POS_DEAD;
    return;
  }

  if ( (!IS_NPC(victim) && victim->hit <= -11 )) {
    victim->position = POS_DEAD;
    return;
  }
  
  if ( victim->hit <= -6 ) 
    victim->position = POS_MORTAL;
  else if ( victim->hit <= -3 ) 
    victim->position = POS_INCAP;
  else                          
    victim->position = POS_STUNNED;
  
  return;
}



/*
 * Start fights.
 */
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->fighting != NULL )
    {
	bug( "Set_fighting: already fighting", 0 );
	return;
    }

    if ( !IS_AFFECTED(ch, AFF_SAP) )
    {
       if ( IS_AFFECTED(ch, AFF_SLEEP) )
   	   affect_strip( ch, gsn_sleep );

       ch->fighting = victim;
       ch->position = POS_FIGHTING;
    }
    return;
}



/*
 * Stop fights.
 */
void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
  CHAR_DATA *fch;
  
  for ( fch = char_list; fch != NULL; fch = fch->next ) {
    if ( fch == ch || ( fBoth && fch->fighting == ch ) ) {
	 fch->fighting	= NULL;
	 fch->position	= IS_NPC(fch) ? fch->default_pos : POS_STANDING;
	 update_pos( fch );
    }
  }
    
  return;
}



/*
 * Make a corpse out of a character.
 */
void make_corpse( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *corpse;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    char *name;

    if ( IS_NPC(ch) )
    {
	name		= ch->short_descr;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
	corpse->timer	= number_range( 3, 6 );
	if ( (ch->gold > 0 ) || (ch->silver > 0))
	{
	    obj_to_obj( create_money( ch->gold, ch->silver ), corpse );
	    ch->gold = 0;
	    ch->silver = 0;
	}
	corpse->cost = 0;
    }
    else
    {
	name		= ch->name;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
	corpse->timer	= number_range( 25, 40 );
	REMOVE_BIT(ch->act,PLR_CANLOOT);
	if (!is_clan(ch))
	    corpse->owner = str_dup(ch->name);
	else
	{
	    corpse->owner = NULL;
	    if (ch->gold > 1 || ch->silver > 1)
	    {
		obj_to_obj(create_money(ch->gold / 2, ch->silver/2), corpse);
		ch->gold -= ch->gold/2;
		ch->silver -= ch->silver/2;
	    }
	}
		
	corpse->cost = 0;
    }

    corpse->level = ch->level;

    sprintf( buf, corpse->short_descr, name );
    free_string( corpse->short_descr );
    corpse->short_descr = str_dup( buf );

    sprintf( buf, corpse->description, name );
    free_string( corpse->description );
    corpse->description = str_dup( buf );

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
	bool floating = FALSE;

	obj_next = obj->next_content;
	if (obj->wear_loc == WEAR_FLOAT)
	    floating = TRUE;
	obj_from_char( obj );
	if (obj->item_type == ITEM_POTION)
	    obj->timer = number_range(500,1000);
	if (obj->item_type == ITEM_SCROLL)
	    obj->timer = number_range(1000,2500);
	if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH) && !floating)
	{
	    obj->timer = number_range(5,10);
	    REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);
	}
	REMOVE_BIT(obj->extra_flags,ITEM_VIS_DEATH);

	if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
	    extract_obj( obj );
	else if (floating)
	{
	    if (IS_OBJ_STAT(obj,ITEM_ROT_DEATH)) /* get rid of it! */
	    { 
		if (obj->contains != NULL)
		{
		    OBJ_DATA *in, *in_next;

		    act("$p evaporates,scattering its contents.",
			ch,obj,NULL,TO_ROOM);
		    for (in = obj->contains; in != NULL; in = in_next)
		    {
			in_next = in->next_content;
			obj_from_obj(in);
			obj_to_room(in,ch->in_room);
		    }
		 }
		 else
		    act("$p evaporates.",
			ch,obj,NULL,TO_ROOM);
		 extract_obj(obj);
	    }
	    else
	    {
		act("$p falls to the floor.",ch,obj,NULL,TO_ROOM);
		obj_to_room(obj,ch->in_room);
	    }
	}
	else
	    obj_to_obj( obj, corpse );
    }

    if (ch->in_obj)
       obj_to_obj( corpse, ch->in_obj );
    else 
       obj_to_room( corpse, ch->in_room );
    return;
}

/*
 * Make a corpse out of a character.
 */
void make_fake_corpse( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    OBJ_DATA *corpse;
    char *name;

    if ( IS_NPC(ch) )
    {
	name		= ch->short_descr;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
	corpse->timer	= number_range( 3, 6 );
	if ( (ch->gold > 0 ) || (ch->silver > 0))
	{
	    obj_to_obj( create_money( ch->gold, ch->silver ), corpse );
	    ch->gold = 0;
	    ch->silver = 0;
	}
	corpse->cost = 0;
    }
    else
    {
	name		= ch->name;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
	corpse->timer	= number_range( 25, 40 );
	REMOVE_BIT(ch->act,PLR_CANLOOT);
	if (!is_clan(ch))
	    corpse->owner = str_dup(ch->name);
	else
	{
	    corpse->owner = NULL;
	}
		
	corpse->cost = 0;
    }

    corpse->level = ch->level;

    sprintf( buf2,"someone that looks like %s",name);
    sprintf( buf, corpse->short_descr, buf2 );
    free_string( corpse->short_descr );
    corpse->short_descr = str_dup( buf );

    sprintf( buf, corpse->description, buf2 );
    free_string( corpse->description );
    corpse->description = str_dup( buf );

    if (ch->in_obj)
       obj_to_obj( corpse, ch->in_obj );
    else 
       obj_to_room( corpse, ch->in_room );
    return;
}


/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *was_in_room;
    char *msg;
    int door;
    int vnum;

    vnum = 0;
    msg = "You hear $n's death cry.";

    switch ( number_bits(4))
    {
    case  0: msg  = "$n hits the ground ... {rDEAD{x.";			break;
    case  1: 
	if (ch->material == 0)
	{
	    msg  = "$n splatters blood on your armor.";		
	    break;
	}
    case  2: 							
	if (IS_SET(ch->parts,PART_GUTS))
	{
	    msg = "$n spills $s guts all over the floor.";
	    vnum = OBJ_VNUM_GUTS;
	}
	break;
    case  3: 
	if (IS_SET(ch->parts,PART_HEAD))
	{
	    msg  = "$n's severed head plops on the ground.";
	    vnum = OBJ_VNUM_SEVERED_HEAD;				
	}
	break;
    case  4: 
	if (IS_SET(ch->parts,PART_HEART))
	{
	    msg  = "$n's heart is torn from $s chest.";
	    vnum = OBJ_VNUM_TORN_HEART;				
	}
	break;
    case  5: 
	if (IS_SET(ch->parts,PART_ARMS))
	{
	    msg  = "$n's arm is sliced from $s dead body.";
	    vnum = OBJ_VNUM_SLICED_ARM;				
	}
	break;
    case  6: 
	if (IS_SET(ch->parts,PART_LEGS))
	{
	    msg  = "$n's leg is sliced from $s dead body.";
	    vnum = OBJ_VNUM_SLICED_LEG;				
	}
	break;
    case 7:
	if (IS_SET(ch->parts,PART_BRAINS))
	{
	    msg = "$n's head is shattered, and $s brains splash all over you.";
	    vnum = OBJ_VNUM_BRAINS;
	}
    }

    act( msg, ch, NULL, NULL, TO_ROOM );

    if ( vnum != 0 )
    {
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	char *name;

	name		= IS_NPC(ch) ? ch->short_descr : ch->name;
	obj		= create_object( get_obj_index( vnum ), 0 );
	obj->timer	= number_range( 4, 7 );

	sprintf( buf, obj->short_descr, name );
	free_string( obj->short_descr );
	obj->short_descr = str_dup( buf );

	sprintf( buf, obj->description, name );
	free_string( obj->description );
	obj->description = str_dup( buf );

	if (obj->item_type == ITEM_FOOD)
	{
	    if (IS_SET(ch->form,FORM_POISON))
		obj->value[3] = 1;
	    else if (!IS_SET(ch->form,FORM_EDIBLE))
		obj->item_type = ITEM_TRASH;
	}

	obj_to_room( obj, ch->in_room );
    }

    if ( IS_NPC(ch) )
	msg = "You hear something's death cry.";
    else
	msg = "You hear someone's death cry.";

    was_in_room = ch->in_room;
    for ( door = 0; door <= 5; door++ )
    {
	EXIT_DATA *pexit;

	if ( ( pexit = was_in_room->exit[door] ) != NULL
	&&   pexit->u1.to_room != NULL
	&&   pexit->u1.to_room != was_in_room )
	{
	    ch->in_room = pexit->u1.to_room;
	    act( msg, ch, NULL, NULL, TO_ROOM );
	}
    }
    ch->in_room = was_in_room;

    return;
}



void raw_kill( CHAR_DATA *victim )
{
  //int i;
  AFFECT_DATA af;
  char buffer[MAX_INPUT_LENGTH];
  
  stop_fighting( victim, TRUE );
  if (IS_NPC(victim))
  {
    death_cry( victim );
    make_corpse( victim );
    victim->pIndexData->killed++;
    kill_table[URANGE(0, victim->level, MAX_LEVEL-1)].killed++;
    extract_char( victim, TRUE, FALSE );
    return;
  }

  sprintf(buffer,"Raw Kill: %s",victim->name);
  _logf(buffer);
  victim->hit		= UMAX( 1, victim->hit  );
  victim->endurance	= UMAX( 1, victim->endurance );
  victim->position = POS_DEAD;
  
  af.where     = TO_AFFECTS;
  af.type      = gsn_sap; 
  af.level     = victim->level;
  af.duration  = 4;
  af.location  = APPLY_NONE;
  af.modifier  = 0;
  af.bitvector = AFF_SAP;
  affect_join( victim, &af );
  
  return;
}



void group_gain( CHAR_DATA *ch, CHAR_DATA *victim, bool bAssassinated )
{
    /* char buf[MAX_STRING_LENGTH]; */
    CHAR_DATA *gch;
    CHAR_DATA *lch;
    int xp;
    int members;
    int group_levels;

    /*
     * Monsters don't get kill xp's or alignment changes.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if ( victim == ch )
	   return;

    if (!IS_NPC(victim))
      return;
    
    members = 0;
    group_levels = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
	if ( is_same_group( gch, ch ) )
        {
	    if (!IS_NPC(gch))
	    {
	    	members++;
	    	group_levels += IS_NPC(gch) ? gch->level / 2 : gch->level;
	    }
	}
    }

    if ( members == 0 )
    {
	bug( "Group_gain: members.", members );
	members = 1;
	group_levels = ch->level ;
    }

    lch = (ch->leader != NULL) ? ch->leader : ch;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	if ( !is_same_group( gch, ch ) || IS_NPC(gch))
	    continue;

	if (!IS_NPC(gch) && IS_NPC(victim)) {// && gch->gain_xp == TRUE) { 
	    xp = xp_compute( gch, victim, group_levels );  
	    if (bAssassinated)
	    {
		xp = xp / 2;
	    }
	    if (xp > 10 && ((get_level(gch) + (gch->exp / 2000)) > 90))
            {
		int plevel = get_level(gch) + (gch->exp / 2000);
		if (plevel < 300) 
		{
			//do nothing
		}
		if (plevel > 300 && plevel <= 499)
		{
			xp = (xp * 9) / 10;
		}
		if (plevel >= 500 && plevel <= 699)
		{
			xp = (xp * 8) / 10;
		}
		if (plevel >= 700 && plevel <= 899)
		{
			xp = (xp * 7)/10;
		}
		if (plevel >= 900 && plevel <= 1099)
		{
			xp = (xp * 6)/10;
		}
		if (plevel >= 1100 && plevel <= 1299)
		{
			xp = (xp * 5)/10;
		}
		if (plevel >= 1300 && plevel <= 1499)
		{
			xp = (xp * 4)/10;
		}
		else
		if (plevel >= 1500 && plevel <= 1699)
		{
			xp = (xp * 3)/10;
		}
		else
		if (plevel >= 1700 && plevel <= 1899)
		{
			xp = (xp * 2)/10;
		}
		else
		if (plevel >= 1900)
		{
			xp = xp /10;
		}
		if (xp < 10)
			xp = 10;
            }

	    gain_exp( gch, xp );	    
	    gch->gain_xp = FALSE; 
	}

	for ( obj = gch->carrying; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if ( obj->wear_loc == WEAR_NONE )
		continue;

	}

        if (victim->position == POS_DEAD)
        {
            if ((!IS_NPC(gch)) && (IS_SET(gch->act,PLR_QUESTING ))
	       && (IS_NPC(victim)))
            {
                if (gch->pcdata->questmob == victim->pIndexData->vnum)
                {
	          send_to_char("You have almost completed your quest!\r\n",gch);
	          send_to_char("Return to the questmaster before your time runs out!.\r\n",gch);
	          gch->pcdata->questmob = -1;
	        }
            }
       }
   }
    return;
}



/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute( CHAR_DATA *gch, CHAR_DATA *victim, int total_levels )
{
  int xp = 0,base_exp = 0;
  int level_range;
  //int time_per_level;
  int members=0;               // Number in group
  //char buf[MSL];
  CHAR_DATA *vch=NULL;
  bool double_xp=FALSE;
  int lucky_number=0;
  int lucky_mod=0;
  char buf[MSL];

  if (IS_NPC(gch))
  {
	return 0;
  }

  level_range = victim->level - gch->level;
  
  /* compute the base exp */
  switch (level_range) {
  default : 	base_exp =   0;		break;
  case -9 :	base_exp =   1;		break;
  case -8 :	base_exp =   2;		break;
  case -7 :	base_exp =   5;		break;
  case -6 : 	base_exp =   9;		break;
  case -5 :	base_exp =  11;		break;
  case -4 :	base_exp =  22;		break;
  case -3 :	base_exp =  33;		break;
  case -2 :	base_exp =  50;		break;
  case -1 :	base_exp =  66;		break;
  case  0 :	base_exp =  100;	break;
  case  1 :	base_exp =  150;	break;
  case  2 :	base_exp = 165;		break;
  case  3 :	base_exp = 150;		break;
  case  4 :	base_exp = 200;		break;
  case  5 :	base_exp = 250;		break;
  case  6 :	base_exp = 300;		break;
  case  7 :	base_exp = 350;		break;
  case  8 :	base_exp = 400;		break;
  case  9 :	base_exp = 450;		break;
  case 10 :	base_exp = 500;		break;
  } 
    
  /* randomize the rewards */

  if (gch->level > 50)
  {
    xp = number_range (base_exp * 3/4, base_exp * 5/4);
  }
  else
  {
    xp = number_range(base_exp, base_exp * 3/2);
  }
  
  /* adjust for grouping */
  //xp = xp * gch->level/( UMAX(1,total_levels -1) );

  // A little bonus for grouping
  for (vch = gch->in_room->people; vch != NULL; vch = vch->next_in_room ) {
     if (is_same_group( vch, gch ) && !IS_NPC(vch))
        members++;
  }
  
  if (members > 1) {
    xp = xp * (1 + (members/(double)10));
  }
  
 //CAP XP per kill at 500 before the double xp and reward multiplier bonuses kick in
  if (xp > 400)
  {
    if( (get_level(gch) + (gch->exp / 2000)) > 90)
        xp = 200;
    else
        xp = 400;
  }


  if (get_level(gch) < 25) {
	xp = xp * 2;
  }

  // Double xp day?
  //if (!double_xp && current_time >= 1096610400 && current_time <= 1096696799) {
  if (!double_xp && current_time >= 1111993200 && current_time <= 1112079599) {
    double_xp = TRUE;
    send_to_char("{WD{ro{Wu{rb{Wl{re {Wxp {rd{Wa{ry{W!!!{x\r\n", gch);
    //send_to_char("{YHa{Dll{Yow{Dee{Yn {Dx{Yp{W!!!{x\r\n", gch);
    //xp = xp*number_range(1,3);
    xp = xp*2;	
  }
  
  if (reward_multiplier > 0)
  {
	if (current_time <= reward_time)
        {
		xp=xp*reward_multiplier;
		send_to_char("{WB{RO{WN{RU{WS XP {RR{WE{RW{WA{RR{WD!!!{x\r\n",gch);
	}
	else {
		reward_multiplier = 0;
	}
  }
  // Double xp MOB?
  if (gch->pcdata->reward_multiplier > 0)
  {
	if (current_time <= gch->pcdata->reward_time)
        {
		xp=xp*gch->pcdata->reward_multiplier;
		send_to_char("{WB{RO{WN{RU{WS XP {RR{WE{RW{WA{RR{WD!!!{x\r\n",gch);
	}
	else
	{
		gch->pcdata->reward_multiplier = 0;
	}
  }
  // Double xp MOB?
  if (!double_xp && IS_SET(victim->gain_flags, GAIN_DOUBLE_XP)) {
    double_xp = TRUE;
    send_to_char("{WD{ro{Wu{rb{Wl{re {Wxp {rm{Wo{rb{W!!!{x\r\n", gch);
    xp = xp*2;
  }
  
  // Double xp from web vote (1 hour window)
  //if (!double_xp && gch->pcdata->last_web_vote+3600 >= current_time) {
  if (gch->pcdata->last_web_vote+3600 >= current_time) {  	
    double_xp = TRUE;
    if (!double_xp)
       send_to_char("{WD{ro{Wu{rb{Wl{re {Wxp!!!{x\r\n", gch);
    else
       send_to_char("{Y+ {rWebvote {Wd{ro{Wu{rb{Wl{re {Wxp!!!{x\r\n", gch);
    xp = xp*2;
  }
  
  // Fortune rolled XP.
  if (!double_xp && xp > 0) {
     lucky_number = number_range(1,1000);
     if (lucky_number == 356) {
        send_to_char("{DFor{Btune {Rro{rll{Red {Wxp!!!{x\n\r", gch);
        lucky_mod = number_range(2,4);
        xp = xp*lucky_mod;
        
        // Log it
        if (!IS_NPC(gch)) {
           sprintf(buf, "%s gained a fortune rolled xp [lucky_number=%d (from 1000), lucky_mod=%d (5-70) xp=%d]", gch->name, lucky_number, lucky_mod, xp);
           log_string(buf);
           
           wiznet(buf, NULL, NULL, WIZ_LEVELS, 0, 0);
        }
     }
  }
  
  if (g_plevelFlag == FALSE && g_extraplevelFlag == FALSE) 
  {
     if (members > 1)
     {
	int avg_level = total_levels / members; //Find the average level of the group
	if (avg_level > gch->level + 10)
	{
		xp = 0;
	}
     }
  }

  return xp;
}


int xp_compute_OLD( CHAR_DATA *gch, CHAR_DATA *victim, int total_levels )
{
    int xp,base_exp;
    int align,level_range;
    int change;
    int time_per_level;

    level_range = victim->level - gch->level;
 
    /* compute the base exp */
    switch (level_range)
    {
 	default : 	base_exp =   0;		break;
	case -9 :	base_exp =   1;		break;
	case -8 :	base_exp =   2;		break;
	case -7 :	base_exp =   5;		break;
	case -6 : 	base_exp =   9;		break;
	case -5 :	base_exp =  11;		break;
	case -4 :	base_exp =  22;		break;
	case -3 :	base_exp =  33;		break;
	case -2 :	base_exp =  50;		break;
	case -1 :	base_exp =  66;		break;
	case  0 :	base_exp =  83;		break;
	case  1 :	base_exp =  99;		break;
	case  2 :	base_exp = 121;		break;
	case  3 :	base_exp = 143;		break;
	case  4 :	base_exp = 165;		break;
    } 
    
    if (level_range > 4)
	base_exp = 160 + 20 * (level_range - 4);

    /* do alignment computations */
   
    align = victim->alignment - gch->alignment;

    if (IS_SET(victim->act,ACT_NOALIGN))
    {
	/* no change */
    }

    else if (align > 500) /* monster is more good than slayer */
    {
	change = (align - 500) * base_exp / 500 * gch->level/total_levels; 
	change = UMAX(1,change);
	gch->alignment = UMAX(-1000,gch->alignment - change);
    }

    else if (align < -500) /* monster is more evil than slayer */
    {
	change =  ( -1 * align - 500) * base_exp/500 * gch->level/total_levels;
	change = UMAX(1,change);
	gch->alignment = UMIN(1000,gch->alignment + change);
    }

    else /* improve this someday */
    {
	change =  gch->alignment * base_exp/500 * gch->level/total_levels;  
	gch->alignment -= change;
    }
    
    /* calculate exp multiplier */
    if (IS_SET(victim->act,ACT_NOALIGN))
	xp = base_exp;

    else if (gch->alignment > 500)  /* for goodie two shoes */
    {
	if (victim->alignment < -750)
	    xp = (base_exp *4)/3;
   
 	else if (victim->alignment < -500)
	    xp = (base_exp * 5)/4;

        else if (victim->alignment > 750)
	    xp = base_exp / 4;

   	else if (victim->alignment > 500)
	    xp = base_exp / 2;

        else if (victim->alignment > 250)
	    xp = (base_exp * 3)/4; 

	else
	    xp = base_exp;
    }

    else if (gch->alignment < -500) /* for baddies */
    {
	if (victim->alignment > 750)
	    xp = (base_exp * 5)/4;
	
  	else if (victim->alignment > 500)
	    xp = (base_exp * 11)/10; 

   	else if (victim->alignment < -750)
	    xp = base_exp/2;

	else if (victim->alignment < -500)
	    xp = (base_exp * 3)/4;

	else if (victim->alignment < -250)
	    xp = (base_exp * 9)/10;

	else
	    xp = base_exp;
    }

    else if (gch->alignment > 200)  /* a little good */
    {

	if (victim->alignment < -500)
	    xp = (base_exp * 6)/5;

 	else if (victim->alignment > 750)
	    xp = base_exp/2;

	else if (victim->alignment > 0)
	    xp = (base_exp * 3)/4; 
	
	else
	    xp = base_exp;
    }

    else if (gch->alignment < -200) /* a little bad */
    {
	if (victim->alignment > 500)
	    xp = (base_exp * 6)/5;
 
	else if (victim->alignment < -750)
	    xp = base_exp/2;

	else if (victim->alignment < 0)
	    xp = (base_exp * 3)/4;

	else
	    xp = base_exp;
    }

    else /* neutral */
    {

	if (victim->alignment > 500 || victim->alignment < -500)
	    xp = (base_exp * 4)/3;

	else if (victim->alignment < 200 && victim->alignment > -200)
	    xp = base_exp/2;

 	else
	    xp = base_exp;
    }

    /* more exp at the low levels */
    if (gch->level < 6)
    	xp = 10 * xp / (gch->level + 4);

    /* less at high */
    if (gch->level > 35 )
	xp =  15 * xp / (gch->level - 25 );

    /* reduce for playing time */
    
    {
	/* compute quarter-hours per level */
	time_per_level = 4 *
			 (gch->played + (int) (current_time - gch->logon))/3600
			 / gch->level;

	time_per_level = URANGE(2,time_per_level,12);
	if (gch->level < 15)  /* make it a curve */
	    time_per_level = UMAX(time_per_level,(15 - gch->level));
	xp = xp * time_per_level / 12;
    }
   
    /* randomize the rewards */
    xp = number_range (xp * 3/4, xp * 5/4);

    /* adjust for grouping */
    xp = xp * gch->level/( UMAX(1,total_levels -1) );

    return xp;
}

void dam_message( CHAR_DATA *ch, CHAR_DATA *victim,int dam,int dt,bool immune, int hit_location , int dam_type)
{
  char buf1[256], buf2[256], buf3[256];
  const char *vs;
  const char *vp;
  const char *attack;
  const char *attacks;
  char punct;
  bool is_weave=FALSE;
  
  if (ch == NULL || victim == NULL)
    return;

  if ( dam ==   0 )      { vs = "miss";	vp = "misses";		}
  else if ( dam <=   4 ) { vs = "bruise";	vp = "bruises";	}
  else if ( dam <=   8 ) { vs = "scratch";	vp = "scratches";		}
  else if ( dam <=  12 ) { vs = "nick";	vp = "nicks";		}
  else if ( dam <=  16 ) { vs = "graze";	vp = "grazes";		}
  else if ( dam <=  20 ) { vs = "hit";	vp = "hits";		}
  else if ( dam <=  24 ) { vs = "injure";       vp = "injures";		}
  else if ( dam <=  28 ) { vs = "maul";	vp = "mauls";	}
  else if ( dam <=  32 ) { vs = "decimate";	vp = "decimates";	}
  else if ( dam <=  36 ) { vs = "devastate";	vp = "devastates";		}
  else if ( dam <=  40 ) { vs = "MUTILATE";	vp = "MUTILATES";	}
  else if ( dam <=  44 ) { vs = "DISEMBOWEL";	vp = "DISEMBOWELS";	}
  else if ( dam <=  48 ) { vs = "DISMEMBER";	vp = "DISMEMBERS";	}
  else if ( dam <=  52 ) { vs = "MASSACRE";	vp = "MASSACRES";	}
  else if ( dam <=  56 ) { vs = "MANGLE";	vp = "MANGLES";		}
  else if ( dam <=  60 ) { vs = "*** DEMOLISH ***"; vp = "*** DEMOLISHES ***";			}
  else if ( dam <=  74 ) { vs = "=== OBLITERATE  ==="; vp = "=== OBLITERATES ===";			}
  else if ( dam <=  86 ) { vs = "!!! MAKE LIFE DIFFICULT !!!{x for"; vp = "!!! MAKES LIFE DIFFICULT !!!{x for"; }
  else if ( dam <=  98 ) { vs = "!!! RIP THE LUNGS !!!"; vp = "!!! RIPS THE LUNGS !!!{x from";			}
  else if ( dam <= 110)  { vs = "do UNSPEAKABLE things{x to"; vp = "does UNSPEAKABLE things{x to";		}
  else if ( dam <= 122)  { vs = "drop AN ANVIL{x on"; vp = "drops AN ANVIL{x on";		}
  else if ( dam <= 134)  { vs = "get MEDIEVAL{x on"; vp = "gets MEDIEVAL{x on";		}
  else if ( dam <= 146)  { vs = "RIP & EAT THE HEART{x of"; vp = "RIPS & EATS THE HEART{x of";}
  else if ( dam <= 158)  { vs = "CRUSH the SKULL{x of"; vp = "CRUSHES the SKULL{x of";}
  else                   { vs = "IMPLODE the TORSO{x of"; vp = "IMPLODES the TORSO{x of";		}
  
  punct   = (dam <= 24) ? '.' : '!';


  // Set up attack word - more simple in this version of dam_msg()
  if (IS_WOLFSHAPE(ch)) {
    attack  = "bite";
    attacks = "bites";
  }
  else if (get_eq_char(ch, WEAR_WIELD) || get_eq_char(ch, WEAR_SECOND_WIELD)) {
    attack  = "swing";
    attacks = "swings";
  }
  else {
    attack  = "punch";
    attacks = "punches";
  }

  if (immune) {
    if (dt >= TYPE_HIT) {
	 sprintf( buf1, "$n %s at $N's %s, but $E looks to be {3unaffected!{x",
			attack,
			hit_flags[hit_location].name);
	 
	 sprintf( buf2, "You %s at $N's %s, but $E looks to be {2unaffected!{x",
			attack,
			hit_flags[hit_location].name);
	 
	 sprintf( buf3, "$n %s at your %s, but you are {4immune to the puny atempt!{x",
			attack,
			hit_flags[hit_location].name);    
    }
    // Weaves or other below HIT
    else {
	 if ( dt >= 0 && dt < MAX_SKILL ) {
	   attack  = skill_table[dt].noun_damage;
	   if (skill_table[dt].spell_fun != spell_null)
		is_weave = TRUE;
	 }
	 else if (dt >= TYPE_HIT && dt < TYPE_HIT + MAX_DAMAGE_MESSAGE)
	   attack  = attack_table[dt - TYPE_HIT].noun;
	 else {
	   bug( "Dam_message: bad dt %d.", dt );
	   dt      = TYPE_HIT;
	   attack  = attack_table[0].name;
	 }
	 if (!is_weave) {
	   sprintf( buf1, "$n's %s looks to have no effect on $N!{x", attack );	 	 
	   sprintf( buf2, "Your %s looks to have no effect on $N!{x", attack );	 
	   sprintf( buf3, "$n's %s looks to have no effect on $N!{x", attack );
	 }
	 else {
	   sprintf( buf1, "$n's %s {3dissolves on $N!{x", attack );	 	 
	   sprintf( buf2, "Your %s {2dissolves on $N!{x", attack );	 
	   sprintf( buf3, "$n's %s {4dissolves on $N!{x", attack );
	 }
    }
  }
  else {    
    if (dt >= TYPE_HIT) {
	 if (IS_SET(ch->merits, MERIT_ACUTESENSES) || IS_IMMORTAL(ch)) {
	   if (hit_location == LOC_BD) {
		sprintf( buf1, "$n makes a %s attack against $N, %s {3%s{x $M%c{x",
			    hit_flags[hit_location].name,
			    dam == 0 ? "but" : "and",
			    vp, 
			    punct);
		
		sprintf( buf2, "%sYou make a %s attack against $N, %s {2%s{x $M%c {W({y%d{W){x", 
			    dam_type == DAM_CRIT ? "{r*C*{x " : "", 
			    hit_flags[hit_location].name, 
			    dam == 0 ? "but" : "and", 
			    vp, 
			    punct, 
			    dam);
		
		sprintf( buf3, "%s$n makes a %s attack against you, %s {4%s{x it%c{x", 
			    dam_type == DAM_CRIT ? "{r*C*{x " : "",  
			    hit_flags[hit_location].name, 
			    dam == 0 ? "but" : "and",  
			    vp, 
			    punct);
	   }
	   else {
		sprintf( buf1, "$n %s at $N's %s, %s {3%s{x $M%c{x",
			    attack,
			    hit_flags[hit_location].name,
			    dam == 0 ? "but" : "and",
			    vp, 
			    punct);
		
		sprintf( buf2, "%sYou %s at $N's %s, %s {2%s{x $M%c {W({y%d{W){x", 
			    dam_type == DAM_CRIT ? "{r*C*{x " : "",  
			    attack,
			    hit_flags[hit_location].name, 
			    dam == 0 ? "but" : "and", 
			    vp, 
			    punct, 
			    dam);
		
		sprintf( buf3, "%s$n %s at your %s, %s {4%s{x you%c{x", 
			    dam_type == DAM_CRIT ? "{r*C*{x " : "", 
			    attack,
			    hit_flags[hit_location].name, 
			    dam == 0 ? "but" : "and", 
			    vp, 
			    punct);
	   }
	 }
	 else {
	   if (hit_location == LOC_BD) {
		sprintf( buf1, "$n makes a %s attack against $N, %s {3%s{x $M%c{x",
			    hit_flags[hit_location].name,
			    dam == 0 ? "but" : "and",
			    vp, 
			    punct);
		
		sprintf( buf2, "%sYou make a %s attack against $N, %s {2%s{x $M%c{x",  
			    dam_type == DAM_CRIT ? "{r*C*{x " : "", 
			    hit_flags[hit_location].name, 
			    dam == 0 ? "but" : "and", 
			    vp, 
			    punct);
		
		sprintf( buf3, "%s$n makes a %s attack against you, %s {4%s{x it%c{x", 
			    dam_type == DAM_CRIT ? "{r*C*{x " : "", 
			    hit_flags[hit_location].name, 
			    dam == 0 ? "but" : "and", 
			    vp, 
			    punct);
	   }
	   else {
		sprintf( buf1, "$n %s at $N's %s, %s {3%s{x $M%c{x",
			    attack,
			    hit_flags[hit_location].name,
			    dam == 0 ? "but" : "and", 
			    vp, 
			    punct);
		
		sprintf( buf2, "%sYou %s at $N's %s, %s {2%s{x $M%c{x",  
			    dam_type == DAM_CRIT ? "{r*C*{x " : "", 
			    attack,
			    hit_flags[hit_location].name, 
			    dam == 0 ? "but" : "and", 
			    vp, 
			    punct);
		
		sprintf( buf3, "%s$n %s at your %s, %s {4%s{x you%c{x",  
			    dam_type == DAM_CRIT ? "{r*C*{x " : "", 
			    attacks,
			    hit_flags[hit_location].name, 
			    dam == 0 ? "but" : "and", 
			    vp, 
			    punct);
	   }
	 }
    }
    // Weaves or other below HIT
    else {
	 if ( dt >= 0 && dt < MAX_SKILL )
	   attack  = skill_table[dt].noun_damage;
	 else if (dt >= TYPE_HIT && dt < TYPE_HIT + MAX_DAMAGE_MESSAGE)
	   attack  = attack_table[dt - TYPE_HIT].noun;
	 else {
	   bug( "Dam_message: bad dt %d.", dt );
	   dt      = TYPE_HIT;
	   attack  = attack_table[0].name;
	 }

//	 if (!IS_NPC(ch))
//	   printf("#### HER HER HER dt <%d> TYPE_HIT <%d>\n\r", dt, TYPE_HIT);
	 
	 sprintf( buf1, "$n's %s {3%s{x $N's %s%c", attack, vp, hit_flags[hit_location].name, punct);
	 
	 if (IS_SET(ch->merits, MERIT_ACUTESENSES) || IS_IMMORTAL(ch))
	   sprintf( buf2, "Your %s {2%s{x $N's %s%c {W({y%d{W){x", attack, vp, hit_flags[hit_location].name, punct, dam);
	 else
	   sprintf( buf2, "Your %s {2%s{x $N's %s%c{x", attack, vp, hit_flags[hit_location].name, punct);
	 
	 sprintf( buf3, "$n's %s {4%s{x your %s%c{x", attack, vp, hit_flags[hit_location].name, punct);
    }
  }
  
  //Print the messages to the involved characters, and all the others.
  act( buf1, ch, NULL, victim, TO_NOTVICT );
  act( buf2, ch, NULL, victim, TO_CHAR );
  act( buf3, ch, NULL, victim, TO_VICT );
  
  return;
}

void dam_message_OLD( CHAR_DATA *ch, CHAR_DATA *victim,int dam,int dt,bool immune, int hit_location , int dam_type)
{
  char buf1[256], buf2[256], buf3[256];
  const char *vs;
  const char *vp;
  const char *attack;
  char punct;
  
  if (ch == NULL || victim == NULL)
    return;
  
  if ( dam ==   0 )      { vs = "miss";	vp = "misses";		}
  else if ( dam <=   4 ) { vs = "bruise";	vp = "bruises";	}
  else if ( dam <=   8 ) { vs = "scratch";	vp = "scratches";		}
  else if ( dam <=  12 ) { vs = "nick";	vp = "nicks";		}
  else if ( dam <=  16 ) { vs = "graze";	vp = "grazes";		}
  else if ( dam <=  20 ) { vs = "hit";	vp = "hits";		}
  else if ( dam <=  24 ) { vs = "injure";       vp = "injures";		}
  else if ( dam <=  28 ) { vs = "maul";	vp = "mauls";	}
  else if ( dam <=  32 ) { vs = "decimate";	vp = "decimates";	}
  else if ( dam <=  36 ) { vs = "devastate";	vp = "devastates";		}
  else if ( dam <=  40 ) { vs = "MUTILATE";	vp = "MUTILATES";	}
  else if ( dam <=  44 ) { vs = "DISEMBOWEL";	vp = "DISEMBOWELS";	}
  else if ( dam <=  48 ) { vs = "DISMEMBER";	vp = "DISMEMBERS";	}
  else if ( dam <=  52 ) { vs = "MASSACRE";	vp = "MASSACRES";	}
  else if ( dam <=  56 ) { vs = "MANGLE";	vp = "MANGLES";		}
  else if ( dam <=  60 ) { vs = "*** DEMOLISH ***"; vp = "*** DEMOLISHES ***";			}
  else if ( dam <=  74 ) { vs = "=== OBLITERATE  ==="; vp = "=== OBLITERATES ===";			}
  else if ( dam <=  86 ) { vs = "!!! MAKE LIFE DIFFICULT !!! for"; vp = "!!! MAKES LIFE DIFFICULT !!! for";			}
  else if ( dam <=  98 ) { vs = " !!! RIP THE LUNGS !!!"; vp = " !!! RIPS THE LUNGS !!!";			}
  else if ( dam <= 110)  { vs = "do UNSPEAKABLE things to"; vp = "does UNSPEAKABLE things to";		}
  else if ( dam <= 122)  { vs = "drop AN ANVIL on"; vp = "drops AN ANVIL on";		}
  else if ( dam <= 134)  { vs = "get MEDIEVAL on"; vp = "gets MEDIEVAL on";		}
  else if ( dam <= 146)  { vs = "RIP & EAT THE HEART of"; vp = "RIPS & EATS THE HEART of";}
  else if ( dam <= 158)  { vs = "CRUSH the SKULL of"; vp = "CRUSHES the SKULL of";}
  else                   { vs = "IMPLODE the TORSO of"; vp = "IMPLODES the TORSO of";		}
  
  punct   = (dam <= 24) ? '.' : '!';

  if ( dt == TYPE_HIT ) {
    if (ch  == victim) {
	 sprintf( buf1, "$n {3%s{x $melf%c{x",vp,punct);
	 sprintf( buf2, "You {2%s{x yourself%c{x",vs,punct);
    }
    else {
	 sprintf( buf1, "$n {3%s{x $N%c{x",  vp, punct );
	 sprintf( buf2, "You {2%s{x $N%c{x", vs, punct );
	 sprintf( buf3, "$n {4%s{x you%c{x", vp, punct );
    }
  }
  else {
    if ( dt >= 0 && dt < MAX_SKILL )
	 attack	= skill_table[dt].noun_damage;
    else if ( dt >= TYPE_HIT
		    && dt < TYPE_HIT + MAX_DAMAGE_MESSAGE) 
	 attack	= attack_table[dt - TYPE_HIT].noun;
    else {
	 bug( "Dam_message: bad dt %d.", dt );
	 dt  = TYPE_HIT;
	 attack  = attack_table[0].name;
    }
    
    if (IS_WOLFSHAPE(ch)) {
	 char wolfattack[64];
	 sprintf(wolfattack, "deadly bite");
	 attack = wolfattack;
    }
	 

    if (immune) {
	 if (ch == victim) {
	   sprintf(buf1,"{3$n is unaffected by $s own %s.{x",attack);
	   sprintf(buf2,"{2Luckily, you are immune to that.{x");
	 } 
	 else {
	   sprintf(buf1,"{3$N is unaffected by $n's %s!{x",attack);
	   sprintf(buf2,"{2$N is unaffected by your %s!{x",attack);
	   sprintf(buf3,"{4$n's %s is powerless against you.{x",attack);
	 }
    }
    else {
	 if (ch == victim) {
	   sprintf( buf1, "$n's %s {3%s{x $m%c",attack,vp,punct);
	   sprintf( buf2, "Your %s {2%s{x you%c",attack,vp,punct);
	 }
	 else {
	   sprintf( buf1, "$n's %s {3%s{x $N%c{x",  attack, vp, punct );

	   /* 
	    * If player have acute_senses merit or is an Immortal - Show damage 
	    */
	   if (IS_SET(ch->merits, MERIT_ACUTESENSES) || IS_IMMORTAL(ch)) {
		if (dt >= TYPE_HIT) {
		  if (hit_location == LOC_BD) {
		    sprintf( buf2, "%sYou make a %s attack against $N, %s {2%s{x $M%c {W({y%d{W){x", 
				   dam_type == DAM_CRIT ? "{r*C*{x " : "", 
				   hit_flags[hit_location].name, 
				   dam == 0 ? "but" : "and", 
				   vp, 
				   punct, 
				   dam);
		    sprintf( buf3, "%s$n makes a %s attack against you, %s {4%s{x you%c{x", 
				   dam_type == DAM_CRIT ? "{r*C*{x " : "",  
				   hit_flags[hit_location].name, 
				   dam == 0 ? "but" : "and",  
				   vp, 
				   punct);
		  }
		  else {
		    sprintf( buf2, "%sYou %s at $N's %s, %s {2%s{x $M%c {W({y%d{W){x", 
				   dam_type == DAM_CRIT ? "{r*C*{x " : "",  
				   attack,
				   hit_flags[hit_location].name, 
				   dam == 0 ? "but" : "and", 
				   vp, 
				   punct, 
				   dam);
		    sprintf( buf3, "%s$n %ss at your %s, %s {4%s{x you%c{x", 
				   dam_type == DAM_CRIT ? "{r*C*{x " : "",  
				   attack,
				   hit_flags[hit_location].name, 
				   dam == 0 ? "but" : "and", 
				   vp, 
				   punct);
		  }
		}
		else { // Weave
		  sprintf( buf2, "Your %s {2%s{x $N%c {W({y%d{W){x", attack, vp, punct, dam);
		  sprintf( buf3, "$n's %s {4%s{x you%c{x", attack, vp, punct);
		}

/* OLD BEFORE BODY PARTS
		sprintf( buf2, "Your %s {2%s{x $N%c {W({y%d{W){x", attack, vp, punct, dam);
		sprintf( buf3, "$n's %s {4%s{x you%c{x", attack, vp, punct);
*/
	   }
	   // Not Acute senses or IMM
	   else {
		if (dt >= TYPE_HIT) {
		  if (hit_location == LOC_BD) {
		    sprintf( buf2, "%sYou make a %s attack against $N, %s {2%s{x $M%c{x",  
				   dam_type == DAM_CRIT ? "{r*C*{x " : "", 
				   hit_flags[hit_location].name, 
				   dam == 0 ? "but" : "and", 
				   vp, 
				   punct);
		    sprintf( buf3, "%s$n makes a %s attack against you, %s {4%s{x you%c{x", 
				   dam_type == DAM_CRIT ? "{r*C*{x " : "", 
				   hit_flags[hit_location].name, 
				   dam == 0 ? "but" : "and", 
				   vp, 
				   punct);
		  }
		  else {
		    sprintf( buf2, "%sYou %s at $N's %s, %s {2%s{x $M%c{x",  
				   dam_type == DAM_CRIT ? "{r*C*{x " : "", 
				   attack,
				   hit_flags[hit_location].name, 
				   dam == 0 ? "but" : "and", 
				   vp, 
				   punct);
		    sprintf( buf3, "%s$n %ss at your %s, %s {4%s{x you%c{x",  
				   dam_type == DAM_CRIT ? "{r*C*{x " : "", 
				   attack,
				   hit_flags[hit_location].name, 
				   dam == 0 ? "but" : "and", 
				   vp, 
				   punct);
		  }
		}
		else { // Weave		  
		  sprintf( buf2, "Your %s {2%s{x $N%c",  attack, vp, punct );
		  sprintf( buf3, "$n's %s {4%s{x you%c", attack, vp, punct );
		}
/*  
	    	sprintf( buf2, "Your %s {2%s{x $N%c",  attack, vp, punct );
	    	sprintf( buf3, "$n's %s {4%s{x you%c", attack, vp, punct );
*/
	   }
	 }
    }
  }
  
  if (ch == victim) {
    act(buf1,ch,NULL,NULL,TO_ROOM);
    act(buf2,ch,NULL,NULL,TO_CHAR);
  }
  else {
    act( buf1, ch, NULL, victim, TO_NOTVICT );
    act( buf2, ch, NULL, victim, TO_CHAR );
    act( buf3, ch, NULL, victim, TO_VICT );
  }
  return;
}



/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
  OBJ_DATA *obj, *dual_obj;
  char buf[MAX_STRING_LENGTH];
  char wbuf[MAX_STRING_LENGTH];
  
  if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
    return;
  
  if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE)) {
    act("{5$S weapon won't budge!{x",ch,NULL,victim,TO_CHAR);
    act("{5$n tries to disarm you, but your weapon won't budge!{x",ch,NULL,victim,TO_VICT);
    act("{5$n tries to disarm $N, but fails.{x",ch,NULL,victim,TO_NOTVICT);
    return;
  }
  
  if (obj->short_descr != NULL) {
    sprintf(wbuf, "%s", obj->short_descr);	
  }
  else {
    sprintf(wbuf, "weapon");	
  }
  
  sprintf(buf, "{5$n {5disarms you and sends your{x %s {5flying!{x", wbuf);
  act( buf, ch, NULL, victim, TO_VICT);
    
  sprintf(buf, "{5You disarm $N {5and send $S {x%s {5flying!{x", wbuf);
  act( buf, ch, NULL, victim, TO_CHAR);
    
  sprintf(buf, "{5$n {5disarms $N and sends $S {x%s {5flying!{x", wbuf);
  act( buf, ch, NULL, victim, TO_NOTVICT);

  if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH)) {
    obj->timer = number_range(5,10);
    REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);
  }
  
  obj_from_char( obj );
  if ( IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY) )
    obj_to_char( obj, victim );
  else {
    if (!IS_NPC(victim))
	 obj_to_char( obj, victim );
    else {
	 obj_to_room( obj, victim->in_room );
	 
	 if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,obj))
	   get_obj(victim,obj,NULL);
    }
  }
  dual_obj = get_eq_char(victim,WEAR_SECOND_WIELD);
  if ( (dual_obj != NULL) && !(IS_OBJ_STAT(dual_obj,ITEM_NOREMOVE))) {
    //		&& (obj->wear_loc == WEAR_WIELD) )     
    unequip_char( victim, dual_obj);
    equip_char(victim, dual_obj, WEAR_WIELD);
  }
  return;
}

void do_berserk( CHAR_DATA *ch, char *argument)
{
    int chance, hp_percent;

    if ((chance = get_skill(ch,gsn_berserk)) == 0
    ||  (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BERSERK))
    ||  (!IS_NPC(ch)
    &&   ch->level < skill_table[gsn_berserk].skill_level[ch->class]))
    {
	send_to_char("You turn red in the face, but nothing happens.\r\n",ch);
	return;
    }

    if (IS_AFFECTED(ch,AFF_BERSERK) || is_affected(ch,gsn_berserk)
    ||  is_affected(ch,skill_lookup("frenzy")))
    {
	send_to_char("You get a little madder.\r\n",ch);
	return;
    }

    if (IS_AFFECTED(ch,AFF_CALM))
    {
	send_to_char("You're feeling to mellow to berserk.\r\n",ch);
	return;
    }

    if (ch->endurance < 50)
    {
	send_to_char("You can't get up enough energy.\r\n",ch);
	return;
    }

    /* modifiers */

    /* fighting */
    if (ch->position == POS_FIGHTING)
	chance += 10;

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = 100 * ch->hit/ch->max_hit;
    chance += 25 - hp_percent/2;

    if (number_percent() < chance)
    {
	AFFECT_DATA af;

	WAIT_STATE(ch,PULSE_VIOLENCE);
	ch->endurance -= 50;

	/* heal a little damage */
	ch->hit += ch->level * 2;
	ch->hit = UMIN(ch->hit,ch->max_hit);

	send_to_char("Your pulse races as you are consumed by rage!\r\n",ch);
	act("$n gets a wild look in $s eyes.",ch,NULL,NULL,TO_ROOM);
	check_improve(ch,gsn_berserk,TRUE,2);

	af.where	= TO_AFFECTS;
	af.casterId  = ch->id;
	af.type		= gsn_berserk;
	af.level	= ch->level;
	af.duration	= number_fuzzy(ch->level / 8);
	af.modifier	= UMAX(1,ch->level/5);
	af.bitvector 	= AFF_BERSERK;

//	af.location	= APPLY_HITROLL;
//	affect_to_char(ch,&af);

	af.location	= APPLY_DAMROLL;
	affect_to_char(ch,&af);

	af.modifier	= UMAX(10,10 * (ch->level/5));
	af.location	= APPLY_AC;
	affect_to_char(ch,&af);
    }

    else
    {
	WAIT_STATE(ch,3 * PULSE_VIOLENCE);
	ch->endurance -= 25;

	send_to_char("Your pulse speeds up, but nothing happens.\r\n",ch);
	check_improve(ch,gsn_berserk,FALSE,2);
    }
}

void do_bash( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int chance;
  
  one_argument(argument,arg);
  
  if ( MOUNTED(ch) )  {
    send_to_char("You can't bash while riding!\r\n", ch);
    return;
  }

  //If NPC and Mount
  if (IS_NPC(ch) && IS_SET(ch->act, ACT_RIDEABLE) && ch->mount != NULL)
    return;
    
  if (IS_AFFECTED(ch,AFF_FLYING)) {
    act("Your feet aren't on the ground.",ch,NULL,NULL,TO_CHAR);
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
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }
  
  if (ch->daze > 0) {
    if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)) {
	 
    }
    else {
	 send_to_char("You can't bash while in a daze.\r\n", ch);
	 return;
    }
  }
  
  if ( (chance = get_skill(ch,gsn_bash)) == 0
	  ||	 (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BASH))
	  ||	 (!IS_NPC(ch)
		  &&	  ch->level < skill_table[gsn_bash].skill_level[ch->class]))
    {	
	 send_to_char("Bashing? What's that?\r\n",ch);
	 return;
    }
  
  if (arg[0] == '\0') {
    victim = ch->fighting;
    if (victim == NULL) {
	 send_to_char("But you aren't fighting anyone!\r\n",ch);
	 return;
    }
  }  
  else if ((victim = get_char_room(ch,arg)) == NULL) {
    send_to_char("They aren't here.\r\n",ch);
    return;
  }
  
  if (victim->position < POS_FIGHTING) {
    act("You'll have to let $M get back up first.",ch,NULL,victim,TO_CHAR);
    return;
  } 
  
  if (victim == ch) {
    send_to_char("You try to bash your brains out, but fail.\r\n",ch);
    return;
  }
  
  if ( MOUNTED(victim) )  {
    send_to_char("You can't bash a riding one!\r\n", ch);
    return;
  }
  
  if (is_safe(ch,victim))
    return;
  
  if ( IS_NPC(victim) &&  victim->fighting != NULL && !is_same_group(ch,victim->fighting)) {
    send_to_char("Kill stealing is not permitted.\r\n",ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim) {
    act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
    return;
  }
  
  /* modifiers */
  
  /* size  and weight */
  chance += ch->carry_weight / 250;
  chance -= victim->carry_weight / 200;
  
  if (ch->size < victim->size)
    chance += (ch->size - victim->size) * 15;
  else
    chance += (ch->size - victim->size) * 10; 
    
  /* stats */
  chance += get_curr_stat(ch,STAT_STR);
  chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
  chance -= GET_AC(victim,AC_BASH) /25;
  
  /* speed */
  if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
    chance += 10;
  if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
    chance -= 30;
  
  /* level */
  chance += (ch->level - victim->level);
  
  /* perfect balance merit */
  if (IS_SET(victim->merits, MERIT_PERFECTBALANCE))
    chance -= 15;
  
  /* perfect balance merit */
  if (IS_SET(ch->merits, MERIT_PERFECTBALANCE))
    chance += 15;
  
  /* Check if they are using shields */
  if (get_eq_char(ch,WEAR_SHIELD) != NULL)
    chance += 10;
  if (get_eq_char(victim,WEAR_SHIELD) != NULL)
    chance -= 20;
  
  /* If blinded? */
  if (IS_AFFECTED( ch, AFF_BLIND) || IS_AFFECTED(ch, AFF_BLINDFOLDED))
    chance = chance/2;
  
  if (!IS_NPC(victim) && chance < get_skill(victim,gsn_dodge) ) {	
    /*
	 act("{5$n tries to bash you, but you dodge it.{x",ch,NULL,victim,TO_VICT);
	 act("{5$N dodges your bash, you fall flat on your face.{x",ch,NULL,victim,TO_CHAR);
	 WAIT_STATE(ch,skill_table[gsn_bash].beats);
	 return;
    */
    chance -= 3 * (get_skill(victim,gsn_dodge) - chance);
  }
  
  if (ch->fighting != NULL && victim != ch->fighting) 
  {
	chance = (chance < 10 ? chance : UMIN(10,get_skill(ch,gsn_bash)/10));
  }
  // If immune to bash
  //
  if (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_BASH)) {
    chance = 0;
  }

  
  
  /* now the attack */
  if (number_percent() < chance ){
    act("{5$n sends you sprawling with a powerful bash!{x", ch,NULL,victim,TO_VICT);
    act("{5You slam into $N, and send $M flying!{x",ch,NULL,victim,TO_CHAR);
    act("{5$n sends $N sprawling with a powerful bash.{x", ch,NULL,victim,TO_NOTVICT);
    check_improve(ch,gsn_bash,TRUE,1);
    
    victim->position = POS_RESTING;
    DAZE_STATE(victim, 1 * PULSE_VIOLENCE);
    WAIT_STATE(ch,skill_table[gsn_bash].beats);
    damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash, DAM_BASH,FALSE);
    
  }
  else {
    damage(ch,victim,0,gsn_bash,DAM_BASH,FALSE);
    act("{5You fall flat on your face!{x", ch,NULL,victim,TO_CHAR);
    act("{5$n falls flat on $s face.{x", ch,NULL,victim,TO_NOTVICT);
    act("{5You evade $n's bash, causing $m to fall flat on $s face.{x", ch,NULL,victim,TO_VICT);
    check_improve(ch,gsn_bash,FALSE,1);
    ch->position = POS_RESTING;
    WAIT_STATE(ch,skill_table[gsn_bash].beats * 3/2); 
  }
  check_killer(ch,victim);
  
  /*
  if (!IS_NPC(ch))
    ch->gain_xp = TRUE;	
  */
}

void do_dirt( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  AFFECT_DATA af;
  int chance;
  int learned=0;
  int learned_gust=0;
  
  one_argument(argument,arg);
  
  if (MOUNTED(ch)) {
    send_to_char("You can't dirt while riding!\r\n", ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_FLYING)) {
    send_to_char("Your feet aren't on the ground.",ch);
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
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }    
  
  if (ch->daze > 0) {
    if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)) {
	 
    }
    else {
	 send_to_char("You can't dirt kick while in a daze.\r\n", ch);
	 return;
    }
  }
  if (ch->in_room->sector_type == SECT_INSIDE) 
  {
	send_to_char("The maintainers of this building keep it too clean to do that.!\r\n",ch);
	return;
  }
  
  if ( (chance = get_skill(ch,gsn_dirt)) == 0
	  ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK_DIRT))
	  ||   (!IS_NPC(ch)
		   &&    ch->level < skill_table[gsn_dirt].skill_level[ch->class]))
    {
	 send_to_char("You get your feet dirty.\r\n",ch);
	 return;
    }
  
  if (arg[0] == '\0') {
    victim = ch->fighting;
    if (victim == NULL) {
	 send_to_char("But you aren't in combat!\r\n",ch);
	 return;
    }
  }
  else if ((victim = get_char_room(ch,arg)) == NULL) {
    send_to_char("They aren't here.\r\n",ch);
    return;
  }
  
  if (IS_AFFECTED(victim,AFF_BLIND)) {
    do_kick(ch, victim->name);
    //       act("$E's already been blinded.",ch,NULL,victim,TO_CHAR);
    //       WAIT_STATE(ch,skill_table[gsn_dirt].beats);
    return;
  }
  
  if (victim == ch) {
    send_to_char("You can't dirt kick your self.\r\n",ch);
    return;
  }
  
  if ( MOUNTED(victim) )  {
    send_to_char("You can't dirt a riding one!\r\n", ch);
    return;
  }
  
  if (is_safe(ch,victim))
    return;
  
  if (IS_NPC(victim) &&
	 victim->fighting != NULL && 
	 !is_same_group(ch,victim->fighting)) {
    send_to_char("Kill stealing is not permitted.\r\n",ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim) {
    act("But $N is such a good friend!",ch,NULL,victim,TO_CHAR);
    return;
  }
  
  /* modifiers */
  
  /* dexterity */
  chance += get_curr_stat(ch,STAT_DEX);
  chance -= 2 * get_curr_stat(victim,STAT_DEX);
  
  /* speed  */
  if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
    chance += 10;
  if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
    chance -= 25;
  
  /* level */
  chance += (ch->level - victim->level) * 2;
  
  /* sloppy hack to prevent FALSE zeroes */
  if (chance % 5 == 0)
    chance += 1;
  
  /* terrain */
  
  switch(ch->in_room->sector_type) {
  case(SECT_INSIDE):		chance -= 20;	break;
  case(SECT_CITY):		chance -= 10;	break;
  case(SECT_FIELD):		     chance +=  5;	break;
  case(SECT_FOREST):				break;
  case(SECT_HILLS):				break;
  case(SECT_MOUNTAIN):		chance -= 10;	break;
  case(SECT_WATER_SWIM):		chance  =  0;	break;
  case(SECT_WATER_NOSWIM):	chance  =  0;	break;
  case(SECT_AIR):			chance  =  0;  	break;
  case(SECT_DESERT):		chance += 10;   break;
  }
  
  if (chance == 0) {
    send_to_char("There isn't any dirt to kick.\r\n",ch);
    return;
  }

  if (ch->fighting != NULL && victim != ch->fighting) 
  {
	chance = (chance < 10 ? chance : UMIN(10,get_skill(ch,gsn_dirt)/10));
  }
  
  /* now the attack */
  if (number_percent() < chance) {
    
    // If channeler, and knows how to make a wall of air
    // they can possible create a smal shield of air to protect from
    // the dirt.
    if (!IS_NPC(victim) && victim->class == CLASS_CHANNELER) {
	 learned = victim->pcdata->learned[gsn_woa];
	 learned_gust = victim->pcdata->learned[skill_lookup("gust of wind")];
	 
	 if (learned > 1 && IS_CHANNELING(victim)) {
	   if ((number_percent() < learned) && (number_percent() < 75 )) {
		// If know gust of wind, then chance to grab dirst and throw back
		if ((number_percent() <= learned_gust) && (number_percent() < 55)) {
		  act("Suddenly a {Cblue haze{x appear in front of $n preventing the {ydirt{x to hit $s eyes!", victim,NULL,NULL,TO_ROOM);
		  act("$n tries to kick {ydirt{x in your eyes, but you react and weave a {Cshield of air{x to protect from the dirt and flows of {Wair{x to hurl the {ydirt{x back at $m!", ch,NULL,victim,TO_VICT);
		  act("Suddenly the {ydirt{x shoots back toward you!", ch, NULL, NULL, TO_CHAR);
		  
		  af.where	= TO_AFFECTS;
		  af.casterId  = ch->id;
		  af.type 	= gsn_dirt;
		  af.level 	= ch->level;
		  af.duration	= ch->level/15;
		  af.location	= APPLY_HITROLL;
		  af.modifier	= -4;
		  af.bitvector = AFF_BLIND;
		  
		  affect_to_char(ch,&af);
		}
		else {
		  act("Suddenly a {Cblue haze{x appear in front of $n preventing the {ydirt{x to hit $s eyes!", victim,NULL,NULL,TO_ROOM);
		  act("$n tries to kick {ydirt{x in your eyes, but you react and weave a {Cshield of air{x to protect from the {ydirt{x!", ch,NULL,victim,TO_VICT);
		  
		}
		WAIT_STATE(ch,skill_table[gsn_dirt].beats);
		return;
	   }
	 }
    }
        
    act("{5$n is blinded by the {ydirt{x in $s eyes!{x",victim,NULL,NULL,TO_ROOM);
    act("{5$n kicks {ydirt{x in your eyes!{x",ch,NULL,victim,TO_VICT);
    damage(ch,victim,number_range(2,5),gsn_dirt,DAM_NONE,FALSE);
    send_to_char("{5You can't see a thing!{x\r\n",victim);
    check_improve(ch,gsn_dirt,TRUE,2);
    WAIT_STATE(ch,skill_table[gsn_dirt].beats);
    
    af.where	= TO_AFFECTS;
    af.casterId  = victim->id;
    af.type 	= gsn_dirt;
    af.level 	= ch->level;
    af.duration	= ch->level/15;
    af.location	= APPLY_HITROLL;
    af.modifier	= -4;
    af.bitvector 	= AFF_BLIND;
    
    affect_to_char(victim,&af);
  }
  else {
    damage(ch,victim,0,gsn_dirt,DAM_NONE,TRUE);
    check_improve(ch,gsn_dirt,FALSE,2);
    WAIT_STATE(ch,skill_table[gsn_dirt].beats);
  }
  check_killer(ch,victim);
}

void do_trip( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int chance;
  
  one_argument(argument,arg);
  
  if ((ch->level > LEVEL_HERO) && (ch->level < LEVEL_ADMIN)) {
    send_to_char("Not at your level.\r\n",ch);
    return;
  }

  if ( MOUNTED(ch) )  {
    send_to_char("You can't trip while riding!\r\n", ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_FLYING)) {
    act("Your feet aren't on the ground.",ch,NULL,NULL,TO_CHAR);
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
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  if (ch->daze > 0) {
    if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)) {
	 
    }
    else {
	 send_to_char("You can't trip while in a daze.\r\n", ch);
	 return;
    }
  }
  
  if ( (chance = get_skill(ch,gsn_trip)) == 0
	  ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_TRIP))
	  ||   (!IS_NPC(ch) 
		   && ch->level < skill_table[gsn_trip].skill_level[ch->class])) {
    send_to_char("Tripping?  What's that?\r\n",ch);
    return;
  }
  
  
  if (arg[0] == '\0') {
    victim = ch->fighting;
    if (victim == NULL) {
	 send_to_char("But you aren't fighting anyone!\r\n",ch);
	 return;
    }
  } 
  else if ((victim = get_char_room(ch,arg)) == NULL) {
    send_to_char("They aren't here.\r\n",ch);
    return;
  }
  
  if ( MOUNTED(victim) )      {
    send_to_char("You can't trip a riding one!\r\n", ch);
    return;
  }
  
  if (is_safe(ch,victim))
    return;
  
  if (IS_NPC(victim) &&
	 victim->fighting != NULL && 
	 !is_same_group(ch,victim->fighting))     {
    send_to_char("Kill stealing is not permitted.\r\n",ch);
    return;
  }
  
  if (IS_AFFECTED(victim,AFF_FLYING)) {
    act("$S feet aren't on the ground.",ch,NULL,victim,TO_CHAR);
    return;
  }
  
  if (victim->position < POS_FIGHTING) {
    act("$N is already down.",ch,NULL,victim,TO_CHAR);
    return;
  }
  
  if (victim == ch) {
    send_to_char("{5You fall flat on your face!{x\r\n",ch);
    WAIT_STATE(ch,2 * skill_table[gsn_trip].beats);
    act("{5$n trips over $s own feet!{x",ch,NULL,NULL,TO_ROOM);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim) {
    act("$N is your beloved master.",ch,NULL,victim,TO_CHAR);
    return;
  }

  /* modifiers */
  
  /* size */
  if (ch->size < victim->size)
    chance += (ch->size - victim->size) * 10;  /* bigger = harder to trip */
  
  /* dex */
  chance += get_curr_stat(ch,STAT_DEX);
  chance -= get_curr_stat(victim,STAT_DEX) * 3 / 2;
  
  /* speed */
  if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
    chance += 10;
  if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
    chance -= 20;
  
  /* level */
  chance += (ch->level - victim->level) * 2;

  /* perfect balance merit */
  if (IS_SET(victim->merits, MERIT_PERFECTBALANCE))
     chance -= 15; 

  /* perfect balance merit */
  if (IS_SET(ch->merits, MERIT_PERFECTBALANCE))
      chance += 15; 

    /* If blinded? */
   if (IS_AFFECTED( ch, AFF_BLIND) || IS_AFFECTED(ch, AFF_BLINDFOLDED))
      chance = chance/2;

  if (ch->fighting != NULL && victim != ch->fighting) 
  {
	chance = UMIN(10,get_skill(ch,gsn_trip)/10);
  }
  
  

    /* now the attack */
  if (number_percent() < chance) {
    act("{5$n trips you and you go down!{x",ch,NULL,victim,TO_VICT);
    act("{5You trip $N and $N goes down!{x",ch,NULL,victim,TO_CHAR);
    act("{5$n trips $N, sending $M to the ground.{x",ch,NULL,victim,TO_NOTVICT);
    check_improve(ch,gsn_trip,TRUE,1);
    
    DAZE_STATE(victim,1 * PULSE_VIOLENCE);
    WAIT_STATE(ch,skill_table[gsn_trip].beats);
    victim->position = POS_RESTING;
    damage(ch,victim,number_range(2, 2 +  2 * victim->size),gsn_trip, DAM_BASH,TRUE);
  }
  else {
    damage(ch,victim,0,gsn_trip,DAM_BASH,TRUE);
    WAIT_STATE(ch,skill_table[gsn_trip].beats*2/3);
    check_improve(ch,gsn_trip,FALSE,1);
  } 
  
  check_killer(ch,victim);
  
  //  if (!IS_NPC(ch))
  //    ch->gain_xp = TRUE;	
}

void do_whirlwind(CHAR_DATA *ch, char *argument){
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	int fighting_against;
	char arg[MAX_STRING_LENGTH];
	bool bAll = FALSE;
        bool bHostile = FALSE;
        bool bDescriptive = FALSE;

	argument = one_argument(argument,arg);


	if (arg[0] == '\0') //no arguments, use hostile
        {
		bHostile = TRUE;
        }
	else if (!str_prefix(arg,"hostile"))
        {
		bHostile = TRUE;
        }
	else if (!str_prefix(arg,"all"))
	{
		bAll = TRUE;
  	}
	else 
	{
		bDescriptive = TRUE;
  	}
        
	if(MOUNTED(ch)){ // Mounted
	    send_to_char("You can't whirlwind attack while riding!\r\n", ch);
	    return;
	}

	if (IS_NPC(ch) && IS_SET(ch->act, ACT_RIDEABLE) && ch->mount != NULL) //NPC and Mount
		return;

	if(IS_AFFECTED(ch,AFF_FLYING)){ //Levitated
		act("Your feet aren't on the ground.",ch,NULL,NULL,TO_CHAR);
	    return;
	}

	if(IS_AFFECTED(ch, AFF_BIND)){ // Bound
		send_to_char("You are tied up and unable to move!\n\r", ch);
	    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
	    return;
	}

	if(IS_AFFECTED(ch, AFF_WRAPPED)){ // Wrapped
	    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
	    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
	    return;
	}

	if(ch->daze > 0) { // Dazed
	    //special skill, give 50% chance of it working while dazed. more with merit
	    if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(75)) {

	    }
	    else {
		 if (number_chance(25)) {
		 	send_to_char("You are stunned beyond being able to do that..\r\n", ch);
		 	return;
		}
	    }
	}

	if (get_skill(ch,gsn_whirlwind) == 0 || (!IS_NPC(ch) &&	ch->level < skill_table[gsn_whirlwind].skill_level[ch->class])){
		send_to_char("Whirlwind? What's that?\r\n",ch);
		return;
	}

	fighting_against = 0;
	for ( vch = char_list; vch != NULL; vch = vch_next ) { // Go through the character list
	    vch_next = vch->next; // Next character
	    if ( vch->in_room == NULL ) // If the potential victim isn't in a room, disregard
		 continue;
	    if (ch == vch) // If the potential victim is the character executing, disregard
		 continue;
	    if (IS_GHOLAM(vch)) // If the potential victim is a Gholam, don't bother
	         continue;
	    if (is_same_group(vch, ch)) // If the potential victim is in the same group as the character, disregard
	    	continue;
	    if (!IS_SAME_WORLD(vch, ch)) // If the potential victim is in TAR, disregard
	    	continue;
	    if ( vch->in_room == ch->in_room && ch->master != vch){ // Check to see if they're in the same room and not the master
	    	if(vch->fighting == ch || bAll || (bDescriptive && (strstr(arg,vch->short_descr)|| is_name(arg,vch->name)))){ // Get em!
	    		fighting_against++;
	    		act("{wYou perform a {Cwhirlwind{w attack against $M!{x",ch,NULL,vch,TO_CHAR);
	    		act("{w$n performs a {Cwhirlwind{w attack against you!{x",ch,NULL,vch,TO_VICT);
	    		//one_hit(ch, vch, gsn_whirlwind, FALSE, 0); //
	    		multi_hit(ch, vch, gsn_whirlwind); //
	    	} else {
	    		act("{w$n performs a {Cwhirlwind{w attack against $s enemies!{x.",ch,NULL,vch,TO_NOTVICT);
	    	}
	    }
	}
	if(fighting_against == 0)
		send_to_char("But you aren't fighting anyone!\r\n", ch);

	WAIT_STATE(ch, skill_table[gsn_whirlwind].beats);
	return;
}

void do_kill( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  
  one_argument( argument, arg );
/*
  if ((ch->level > LEVEL_HERO) && (ch->level < LEVEL_ADMIN))
  {
  send_to_char("Not at your level.\r\n",ch);
  return;
  }
*/

  if ( arg[0] == '\0' ) {
    send_to_char( "Kill whom?\r\n", ch );
    return;
  }
  
  if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }

  if ( !IS_NPC(victim) ) {
    if ( !IS_SET(victim->act, PLR_KILLER) && !IS_SET(victim->act, PLR_THIEF) ) {
	 send_to_char( "You must MURDER a player.\r\n", ch );
	 return;
    }
  }

  if ( victim == ch ) {
    send_to_char( "You hit yourself.  Ouch!\r\n", ch );
    multi_hit( ch, ch, TYPE_UNDEFINED );
    return;
  }
  
  if ( is_safe( ch, victim ) )
    return;
  
  if ( victim->fighting != NULL &&  !is_same_group(ch,victim->fighting)) {
    send_to_char("Kill stealing is not permitted.\r\n",ch);
    return;
  }
  
  if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim ) {
    act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
    return;
  }
  
  if ( ch->position == POS_FIGHTING ) {
    send_to_char( "You do the best you can!\r\n", ch );
    return;
  }
  
  // Not hidden when a fight is starting
  if ( IS_AFFECTED( ch, AFF_HIDE )) {
    affect_strip ( ch, gsn_hide );
    REMOVE_BIT(ch->affected_by, AFF_HIDE);
    send_to_char( "You step out of the shadows.\r\n", ch );
    act( "$n steps out of the shadows.", ch, NULL, NULL, TO_ROOM);
  }
  
  WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
  check_killer( ch, victim );
  multi_hit( ch, victim, TYPE_UNDEFINED );
  return;
}

void do_murde( CHAR_DATA *ch, char *argument )
{
  send_to_char( "If you want to MURDER, spell it out.\r\n", ch );
  return;
}

void do_murder( CHAR_DATA *ch, char *argument )
{
  //    char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim=NULL;
  int direction;
  ROOM_INDEX_DATA *from_location=NULL;
  ROOM_INDEX_DATA *to_location=NULL;
  EXIT_DATA       *pexit=NULL;
  
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Murder whom?\r\n", ch );
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_CHARM) || (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)))
    return;
  
  if (( victim = get_char_room( ch, arg ) ) == NULL ) {
    
    //Check if direction kill. E.g kill someone who block an exit.
    if ((direction = find_exit(ch, arg)) != -1) { 
	 from_location = ch->in_room;
	 if ((pexit = from_location->exit[direction]) != NULL
		&& ( to_location = pexit->u1.to_room ) != NULL
		&& can_see_room(ch, pexit->u1.to_room)) {
	   if (IS_SET(pexit->exit_info, EX_BLOCKED)) {
		victim = get_blocker(ch->in_room, direction);
		
		if (victim != NULL) {
		  stop_exit_block(victim);
		  
		  char_from_room( victim );
		  char_to_room( victim, from_location );
		}
	   }
	 }       
    }
    
    if (victim == NULL) {    	    	
	 send_to_char( "They aren't here.\r\n", ch );
	 return;
    }
  }
  
  if ( victim == ch ) {
    send_to_char( "Suicide is a mortal sin.\r\n", ch );
    return;
  }

  if (IS_GUARDING(ch) && IS_GUARDED(victim) && ch->pcdata->guarding == victim) {
    act("You are actual trying to guard $N.", ch, NULL, victim, TO_CHAR);
    return;
  }
  
  if ( is_safe( ch, victim ) )
    return;
  
  if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch,victim->fighting)) {
    send_to_char("Kill stealing is not permitted.\r\n",ch);
    return;
  }
  
  if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim ) {
    act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
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
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }
  
  if ( ch->position == POS_FIGHTING ) {
    send_to_char( "You do the best you can!\r\n", ch );
    return;
  }
  
  // Not hidden when a fight is starting
  if ( IS_AFFECTED( ch, AFF_HIDE )) {
    affect_strip ( ch, gsn_hide );
    REMOVE_BIT(ch->affected_by, AFF_HIDE);
    send_to_char( "You step out of the shadows.\r\n", ch );
    act( "$n steps out of the shadows.", ch, NULL, NULL, TO_ROOM);
  }
  
  WAIT_STATE( ch, 1 * PULSE_VIOLENCE );

  //    if (IS_NPC(ch))
  //	sprintf(buf, "Help! I am being attacked by %s!",ch->short_descr);
  //   else
  //    	sprintf( buf, "Help!  I am being attacked by %s!", ch->name );
  //    do_function(victim, &do_yell, buf );
  
  check_killer( ch, victim );
  multi_hit( ch, victim, TYPE_UNDEFINED );
  return;
}


void do_assassinate( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MSL];
  CHAR_DATA *victim;
  OBJ_DATA *obj, *corpse;
  int  pernum,modifier;
  
  if (!IS_NPC(ch) && ch->pcdata->learned[gsn_assassinate] < 1) {
    send_to_char("You don't know how to assassinate yet.\r\n", ch);
    return;
  }
  
  if ( MOUNTED(ch) )  {
    send_to_char("You can't assassinate while riding!\r\n", ch);
    return;
  }
  
  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }
  
  if (ch->daze > 0) {
    if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)) {
	 
    }
    else {
	 send_to_char("You can't assassinate while in a daze.\r\n", ch);
	 return;
    }
  }
  
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Assassinate whom?\r\n", ch );
    return;
  }
  
  if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }
  
  if ( victim == ch ) {
    send_to_char( "How can you assassinate yourself?\r\n", ch );
    return;
  }
  
  if ( is_safe( ch, victim ))
    return;
  
  if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL) {
    send_to_char( "You need to wield a weapon to assassinate.\r\n", ch );
    return;
  }
  
  if (get_weapon_sn(ch) != gsn_dagger) {
    send_to_char( "You need to wield a dagger to assassinate.\r\n", ch);
    return;	
  }
  
  if ( victim->fighting != NULL ) {
    send_to_char( "You can't assassinate a fighting person.\r\n", ch );
    return;
  }
  
  if (!IS_NPC(ch) && !IS_NPC(victim) && ch->pcdata->next_assassinate > current_time) {
    sprintf(buf, "You need to be in the room for %ld more seconds before you can try to assassinate $N.", (ch->pcdata->next_assassinate - current_time) < 0 ? 0 : (ch->pcdata->next_assassinate -  current_time));
    act( buf, ch, NULL, victim, TO_CHAR);
    return;	
  }
  
  if (IS_SET(victim->imm_flags,IMM_ASSASSINATE) || (((ch->race == race_lookup("trolloc") && !IS_NPC(victim))) && (number_percent() < 50))  ) {
    act("You try to to slip your $p in through $N's back, but $E reacts quickly and twist $p out of your hand!", ch, obj, victim, TO_CHAR);
    
    act("$n try to slip $s $p into you, but you reacts quickly and twist $p out of $s hand", ch, NULL, victim, TO_VICT);				
    
    act("$N reacts quickly and twist $p out of $n's hand as he try to sink it into $M!", ch, obj, victim, TO_NOTVICT);    	
    
    obj_from_char( obj );
    if ( IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY) )
	 obj_to_char( obj, victim );
    else
	 obj_to_room( obj, victim->in_room );
    
    multi_hit( victim, ch, TYPE_UNDEFINED );
    
    return;
  }

//    if ( victim->level > ch->level + 5 )
//    {
//	send_to_char( "Try assassinating someone closer to your level.\r\n", ch );
//	return;
//    }
  
  if ( victim->hit < (victim->max_hit *3)/ 4) {
    act( "$N is hurt and suspicious ... you can't sneak up.", ch, NULL, victim, TO_CHAR );
    return;
  }
  WAIT_STATE( ch, skill_table[gsn_assassinate].beats );
  pernum = number_percent();
  if (IS_NPC(victim)) 
  {
  	modifier =  (100 - (ch->pcdata->learned[gsn_assassinate] / 2 )  - (bsmod( ch, victim) /4 )) / 3; 
  }
  else {
  	modifier =  (300 - (ch->pcdata->learned[gsn_assassinate] / 2 )  - (bsmod( ch, victim) /4 )) / 3; 
	//For a PC, make it a bit harder 
	//modifier will be greater than the number+percent (random 1 to 100 to be successful)
	//Max current PC value on assassinate is 300 so start with that
	//Subtract half of the players skill in assassinate (potential being between subtracting 0 and 150)
	//Subtract the backstab modifier divided by 4
 	//divide by 3
 	//So, for a maxed at 302 assassinate PC and a high level pc we have 300 - 150 - (-25) = 175 / 3 
  }
  

  if  (IS_NPC(ch) || pernum < ch->pcdata->learned[gsn_assassinate] ) {
    check_improve(ch,gsn_assassinate,TRUE,1);
     if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(50)) {
	modifier -= number_range(25,33);
     }

  if (IS_IMMORTAL(victim)) {
    pernum = modifier - 1;
  }

    if (pernum < modifier ) {
	 do_visible(ch,"");
	 multi_hit(ch, victim, gsn_assassinate);
    }
    else {
	 do_visible(ch,"");
	 //sprintf(buf, "Your assassination attempt towards $N has struck a mortal blow!!!");
	 //act( buf, ch, NULL, victim, TO_CHAR);
	 
	 //send_to_char("A slight gurgle escapes your lips as you succumb to the weapon in your back!!!\r\n", victim);
	 
	 act("Without a sound you slip your $p in through $N's back to pierce $S {Rheart{x. Without a sound $E {Rdies{x!!!", ch, obj, victim, TO_CHAR);
	 
	 act("{ROnly a second to late do you react to the knife passing through your back to pierce your heart and kill you instantly{x!!!", ch, NULL, victim, TO_VICT);				
	 
	 act("$N drop lifeless to the ground as $n sink $s $p deep into $N's back!!!", ch, obj, victim, TO_NOTVICT);
	 
	 if(IS_IMMORTAL(victim))
	   victim->hit=1;
	 else victim->hit=-15;
	 update_pos(victim);
	 
	 if(victim->position==POS_DEAD) {
	   group_gain(ch, victim, TRUE);
	   
	   /*** make the mob die ***/
	   stop_fighting(victim,TRUE);
	   raw_kill( victim );
	   // Trolloc clan promotion by PK
	   check_trolloc_kill(ch, victim);
	   check_valid_pkill(ch,victim);

	   if (IS_AFFECTED(victim,AFF_CHANNELING)) {
		do_function(victim, &do_unchannel, "" );
	   }	
	   
		  
	   // Log the kill
	   if ( !IS_NPC(victim) ) {
		sprintf(buf, "%s was assassinated by %s at room %d [AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]",
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
               sprintf(buf,"%s was assassinated by %s at room %d\r\n%s - %s \r\n[AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]\r\n",
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
		    return;  /* leave if corpse has treasure */
		  else
		    do_sacrifice( ch, "corpse" );
		}
	   } /* if is_npc && is_npc */
	 } /* if dead */
    }
  }
  else {
    check_improve(ch,gsn_assassinate,FALSE,1);
    damage( ch, victim, 0, gsn_assassinate,DAM_NONE,TRUE );
    do_visible(ch,"");
  }
  
  
  return;
}


void do_backstab( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  OBJ_DATA *obj;
  
  one_argument( argument, arg );
  
  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }
  
  if ((ch->level > LEVEL_HERO) && (ch->level < LEVEL_ADMIN)) {
    send_to_char("Not at your level.\r\n",ch);
    return;
  }
  if ( MOUNTED(ch) )  {
    send_to_char("You can't backstab while riding!\r\n", ch);
    return;
  }
  
  if (ch->daze > 0) {
    if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)) {
	 
    }
    else {
	 send_to_char("You can't backstab while in a daze.\r\n", ch);
	 return;
    }
  }
  
  if (arg[0] == '\0') {
    send_to_char("Backstab whom?\r\n",ch);
    return;
  }
  
  if (ch->fighting != NULL) {
    send_to_char("You're facing the wrong end.\r\n",ch);
    return;
  }  
  else if ((victim = get_char_room(ch,arg)) == NULL) {
    send_to_char("They aren't here.\r\n",ch);
    return;
  }
  
  if ( victim == ch ) {
    send_to_char( "How can you sneak up on yourself?\r\n", ch );
    return;
  }
  
  if ( is_safe( ch, victim ) )
    return;
  
  if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch,victim->fighting)) {
    send_to_char("Kill stealing is not permitted.\r\n",ch);
    return;
  }
  
  if (( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL) {
    send_to_char( "You need to wield a weapon to backstab.\r\n", ch );
    return;
  }
  
  if ( victim->hit < victim->max_hit / 3) {
    act( "$N is hurt and suspicious ... you can't sneak up.", ch, NULL, victim, TO_CHAR );
    return;
  }
  
  check_killer( ch, victim );
  WAIT_STATE( ch, skill_table[gsn_backstab].beats );
  if ( number_percent( ) < get_skill(ch,gsn_backstab)
	  || ( get_skill(ch,gsn_backstab) >= 2 && !IS_AWAKE(victim) ) )
    {
	 check_improve(ch,gsn_backstab,TRUE,1);
	 multi_hit( ch, victim, gsn_backstab );
    }
  else {
    check_improve(ch,gsn_backstab,FALSE,1);
    damage( ch, victim, 0, gsn_backstab,DAM_NONE,TRUE);
  }
  
  /*
  if (!IS_NPC(ch))
    ch->gain_xp = TRUE;
  */
  
  return;
}

void do_circle( CHAR_DATA *ch, char *argument )
{  
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  OBJ_DATA *obj;
  
  one_argument( argument, arg );
  
  if (get_skill(ch,gsn_circle) <= 0)
  {
	send_to_char("You don't know how to do that.\r\n",ch);
	return;
  }

  if (MOUNTED(ch)) {
    send_to_char("You can't circle while riding!\r\n", ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_FLYING)) {
      act("Your feet aren't on the ground.",ch,NULL,NULL,TO_CHAR);
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
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  if (IS_NULLSTR(arg) && ch->fighting == NULL) {
    send_to_char("Circle who?\r\n", ch);
    return;
  }
  else if (IS_NULLSTR(arg) && ch->fighting) {
    victim = ch->fighting;
  }
  else if (( victim = get_char_room( ch, arg) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }


/*  if ( ( victim = ch->fighting ) == NULL ) { */
/*     send_to_char( "You aren't fighting anyone.\r\n", ch ); */
/*     return; */
/*   } */


  WAIT_STATE( ch, skill_table[gsn_circle].beats );
  if (ch->daze > 0) {
    if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)) {

    }
    else {
	 send_to_char("You can't circle while in a daze.\r\n", ch);
	 return;
    }
  }

  // Not when trying to do Masterforms
  /*
  if (IS_SET(ch->auto_act, AUTO_MASTERFORMS) && !IS_SET(ch->merits, MERIT_CONCENTRATION)  ) {
  	send_to_char("You are unable to circle when trying to focus on duelling!\n\r", ch);
  	return;
  }
  */

  // Not when focus is on you, Only small chance
  if (victim->fighting == ch) {
    if (number_percent() > (get_skill(ch,gsn_circle)/20)) {
	 act("You are not able to circle around $N, $S attention is focused directly on you!", ch, NULL, victim, TO_CHAR );
	 return;
    }
  }

  if ( is_safe( ch, victim ) )
    return;
 
  if (IS_NPC(victim) &&
	 victim->fighting != NULL &&
	 !is_same_group(ch,victim->fighting)) {
    send_to_char("Kill stealing is not permitted.\r\n",ch);
    return;
  }
  
  if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL) {
    send_to_char( "You need to wield a weapon to circle.\r\n", ch );
    return;
  }
  
  check_killer( ch, victim );
  WAIT_STATE( ch, skill_table[gsn_circle].beats );

  if ( number_percent( ) < get_skill(ch,gsn_circle)
	  || ( get_skill(ch,gsn_circle) >= 2 && !IS_AWAKE(victim) ) ) {
    check_improve(ch,gsn_circle,TRUE,1);
    //one_hit( ch, victim, gsn_circle, TRUE, 0);
    multi_hit( ch, victim, gsn_circle );
    victim->fighting = ch;
  }
  else {
    check_improve(ch,gsn_circle,FALSE,1);
    damage( ch, victim, 0, gsn_circle,DAM_NONE,TRUE);
    victim->fighting = ch;
  }
  
  // Gain exp from circle
/*
  if (!IS_NPC(ch))
    ch->gain_xp = TRUE;
*/
  
  return;
}

void do_flee( CHAR_DATA *ch, char *argument )
{
  ROOM_INDEX_DATA *was_in=NULL;
  ROOM_INDEX_DATA *now_in=NULL;
  CHAR_DATA *victim;
  int attempt;
  int exp_cost=0;
  int dir=0;
  EXIT_DATA *pexit;
  
  if ( ( victim = ch->fighting ) == NULL ) {
    if ( ch->position == POS_FIGHTING )
	 ch->position = POS_STANDING;
    send_to_char( "You aren't fighting anyone.\r\n", ch );
    return;
  }

  // Optional directional fleeing
  if (!IS_NULLSTR(argument)) 
  {
    if (!str_prefix(argument, "north"))
	 dir = DIR_NORTH; 
    else if( !str_prefix( argument,  "east"  ))
	 dir = DIR_EAST;  
    else if( !str_prefix( argument,  "south" ))
	 dir = DIR_SOUTH; 
    else if( !str_prefix( argument,  "west"  ))
	 dir = DIR_WEST;  
    else if( !str_prefix( argument,  "up"    ))
	 dir = DIR_UP;    
    else if( !str_prefix( argument,  "down"  ))
	 dir = DIR_DOWN;  
    else {
	 send_to_char("Syntax: flee [direction]\n\r", ch);
	 return;
    }
    
    // Make sure exit exists
    pexit = ch->in_room->exit[dir];
    if( (pexit == NULL ) )
    {
	 send_to_char( "There is no exit in that direction!\n\r", ch );
	 return;
    }
    
    if (IS_SET(pexit->exit_info, EX_AIRWALL)) {
	 send_to_char("You suddeny notice that direction is blocked by a wall of air!\n\r", ch);
	 return;	
    }
    
    // Able to flee?
    if (number_range(0,ch->daze) <= 0 &&
	   (number_percent() < number_range(70,90))) 
    {
	 act( "$n has fled!", ch, NULL, NULL, TO_ROOM );
         was_in = ch->in_room;
	 move_char( ch, dir, FALSE );
	 
	 if ( !IS_NPC(ch) ) {
	   send_to_char( "You flee from combat!\r\n", ch );
	   if(  (number_percent() < ch->level/2 ) && (ch->endurance > 0))
		send_to_char( "{WYou snuck away safely{x.\r\n", ch);
	   else {
		if(ch->exp > 0 && IS_NPC(victim)) {
		  exp_cost = UMIN(500,ch->exp/100);   
		  if (ch->endurance <= 0) {
		    exp_cost *=2;
		  }
		  gain_exp(ch, -exp_cost);
		}
	   }
	 }
	 stop_fighting( ch, TRUE );
         CHAR_DATA * wasinroompeople = NULL;
         CHAR_DATA * wasinroompeople_next = NULL;
    	 for (wasinroompeople = was_in->people; wasinroompeople != NULL; wasinroompeople = wasinroompeople_next)
    	 {
        	 if (wasinroompeople->to_hunt == ch)
        	 {
                	 do_function(wasinroompeople, &do_hunt, ch->name );
        	 }
    	 }
	 return;
    }
    else {
	 send_to_char( "{RPANIC{W!{x You couldn't escape!\r\n", ch );
	 return;
    }    
  }
  
  // Normal fleeing  
  was_in = ch->in_room;
  for ( attempt = 0; attempt < 6; attempt++ ) {
    int door;
    
    door = number_door( );
    if ( ( pexit = was_in->exit[door] ) == 0
	    ||   pexit->u1.to_room == NULL
	    ||   IS_SET(pexit->exit_info, EX_CLOSED)
//	    ||   IS_SET(pexit->exit_info, EX_FIREWALL)
	    ||   IS_SET(pexit->exit_info, EX_AIRWALL)
	    ||   number_range(0,ch->daze) != 0
	    ||   number_percent() > number_range(55,65)
	    || ( IS_NPC(ch)
		    &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
	 continue;

    move_char( ch, door, FALSE );
    if ( ( now_in = ch->in_room ) == was_in )
	 continue;

    ch->in_room = was_in;
    act( "$n has fled!", ch, NULL, NULL, TO_ROOM );
    ch->in_room = now_in;
    
    if ( !IS_NPC(ch) ) {
	 send_to_char( "You flee from combat!\r\n", ch );
	 if(  (number_percent() < ch->level/2 ) )
	   send_to_char( "{WYou snuck away safely{x.\r\n", ch);
	 else {
	   if(ch->exp > 0 && IS_NPC(victim)) {
		exp_cost = (ch->exp/100);   
		gain_exp(ch, -exp_cost);
	   }
	 }
    }
    
    stop_fighting( ch, TRUE );
    CHAR_DATA * wasinroompeople = NULL;
    CHAR_DATA * wasinroompeople_next = NULL;
    for (wasinroompeople = was_in->people; wasinroompeople != NULL; wasinroompeople = wasinroompeople_next) 
    {
	if (wasinroompeople->to_hunt == ch)
        {
   		do_function(wasinroompeople, &do_hunt, ch->name );	
	}
    }
    return;
  }
  
  send_to_char( "{RPANIC{W!{x You couldn't escape!\r\n", ch );
  return;
}


void do_rescue( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  CHAR_DATA *fch;
  
  one_argument( argument, arg );
  if ( arg[0] == '\0' ) {
    send_to_char( "Rescue whom?\r\n", ch );
    return;
  }
  
  if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }
  
  if ( victim == ch ) {
    send_to_char( "What about fleeing instead?\r\n", ch );
    return;
  }
  
  if ( !IS_NPC(ch) && IS_NPC(victim) ) {
    send_to_char( "Doesn't need your help!\r\n", ch );
    return;
  }
  
  if ( ch->fighting == victim ) {
    send_to_char( "Too late.\r\n", ch );
    return;
  }
  
  if ( ( fch = victim->fighting ) == NULL ) {
    send_to_char( "That person is not fighting right now.\r\n", ch );
    return;
  }

  if (!IS_RP(victim) && !IS_RP(ch)) {
    if ( IS_NPC(fch) && !is_same_group(ch,victim)) {
	 send_to_char("Kill stealing is not permitted.\r\n",ch);
	 return;
    }
  }
  
  WAIT_STATE( ch, skill_table[gsn_rescue].beats );
  if ( number_percent( ) > get_skill(ch,gsn_rescue)) {
    send_to_char( "You fail the rescue.\r\n", ch );
    check_improve(ch,gsn_rescue,FALSE,1);
    return;
  }

  act( "{5You rescue $N!{x",  ch, NULL, victim, TO_CHAR    );
  act( "{5$n rescues you!{x", ch, NULL, victim, TO_VICT    );
  act( "{5$n rescues $N!{x",  ch, NULL, victim, TO_NOTVICT );
  check_improve(ch,gsn_rescue,TRUE,1);
  
  stop_fighting( fch, FALSE );
  stop_fighting( victim, FALSE );

  WAIT_STATE( victim, skill_table[gsn_rescue].beats );

  check_killer( ch, fch );
  set_fighting( ch, fch );
  set_fighting( fch, ch );
  return;
}


void do_focus( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    //CHAR_DATA *fch;

    one_argument( argument, arg );

    // Can only focus if in a fight.
    if (ch->fighting == NULL) {
      if (ch->position == POS_FIGHTING )
        ch->position = POS_STANDING;
      send_to_char("You aren't fighting anyone.\r\n", ch );
      return;
    }
    
    if ( arg[0] == '\0' )
    {
	send_to_char( "Target whom?\r\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\r\n", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "There are easier ways to commit suicide!\r\n", ch );
	return;
    }

    if ( ch->fighting == victim )
    {
	send_to_char( "You're already fighting them.\r\n", ch );
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_rescue].beats );
    if ( number_percent( ) > 50)
    {
	send_to_char( "You can't make it through the fighting to reach them.\r\n", ch );
	return;
    }

    if ( is_safe( ch, victim ) )
    {
	send_to_char( "You can't make it through the fighting to reach them.\r\n", ch );
	return;
    }

    act( "{5You focus your attack on $N!{x",  ch, NULL, victim, TO_CHAR    );
    act( "{5$n focuses on you!{x", ch, NULL, victim, TO_VICT    );
    act( "{5$n focuses on $N!{x",  ch, NULL, victim, TO_NOTVICT );

    stop_fighting( ch, FALSE );
    set_fighting( ch, victim );
    if (victim->fighting == NULL)
    {
       set_fighting( victim, ch );
    }
    return;
}



void do_kick( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    one_argument( argument, arg );

	
    if ((ch->level > LEVEL_HERO) && (ch->level < LEVEL_ADMIN))
    {
	send_to_char("Not at your level.\r\n",ch);
        return;
    }
    if ( MOUNTED(ch) ) 
    {
        send_to_char("You can't kick while riding!\r\n", ch);
        return;
    }

    if ( !IS_NPC(ch)
    &&   ch->level < skill_table[gsn_kick].skill_level[ch->class] )
    {
	send_to_char(
	    "You better leave the martial arts to fighters.\r\n", ch );
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
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }
    if (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK))
	return;

    if ( (arg[0] == '\0') && (ch->fighting == NULL))
    {
	send_to_char( "Kick Who?.\r\n", ch );
	return;

    }
    else
    if ((arg[0] == '\0') && (ch->fighting))
    {
	victim = ch->fighting;
    }
    else
    if ( ( victim = get_char_room( ch, arg) ) == NULL )
    {
	send_to_char( "They aren't here.\r\n", ch );
	return;
    }
    
    if (is_safe(ch, victim))
	 return;

    if (victim == ch) {
	send_to_char("You try to kick yourself, but miss.\r\n",ch);
    	WAIT_STATE(ch,skill_table[gsn_kick].beats);
	return;
    }
    WAIT_STATE( ch, skill_table[gsn_kick].beats );
    int chance = get_skill(ch, gsn_kick);
  if (ch->fighting != NULL && victim != ch->fighting) 
  {
	chance = UMIN(10,get_skill(ch,gsn_kick)/10);
  }
  

    if ( number_percent() < chance)
    {
	damage(ch,victim,number_range( 1, ch->level ), gsn_kick,DAM_BASH,TRUE);
	check_improve(ch,gsn_kick,TRUE,1);
    }
    else
    {
	damage( ch, victim, 0, gsn_kick,DAM_BASH,TRUE);
	check_improve(ch,gsn_kick,FALSE,1);
    }
	check_killer(ch,victim);

/*
  if (!IS_NPC(ch))
     ch->gain_xp = TRUE;	
*/
	
   return;
}




void do_disarm( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance,hth,ch_weapon,vict_weapon,ch_vict_weapon;

    hth = 0;

  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }

    if ((chance = get_skill(ch,gsn_disarm)) == 0) {
       send_to_char( "You don't know how to disarm opponents.\r\n", ch );
       return;
    }

    if (ch->daze > 0) {
	 if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)) {

	 }
	 else {
	   send_to_char("You can't disarm while in a daze.\r\n", ch);
	   return;
	 }
    }

    if ( get_eq_char( ch, WEAR_WIELD ) == NULL 
    &&   ((hth = get_skill(ch,gsn_hand_to_hand)) == 0
    ||    (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_DISARM))))
    {
	send_to_char( "You must wield a weapon to disarm.\r\n", ch );
	return;
    }

    if ( ( victim = ch->fighting ) == NULL ) {
	send_to_char( "You aren't fighting anyone.\r\n", ch );
	return;
    }

    if (is_safe(ch, victim))
	 return;

    if (IS_WOLFSHAPE(victim)) {
	 send_to_char( "Your opponent is not wielding a weapon.\r\n", ch );
	 return;
    }

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
    {
	send_to_char( "Your opponent is not wielding a weapon.\r\n", ch );
	return;
    }

    /* find weapon skills */
    ch_weapon = get_weapon_skill(ch,get_weapon_sn(ch));
    vict_weapon = get_weapon_skill(victim,get_weapon_sn(victim));
    ch_vict_weapon = get_weapon_skill(ch,get_weapon_sn(victim));

    /* modifiers */

    /* skill */
    if ( get_eq_char(ch,WEAR_WIELD) == NULL)
	chance = chance * hth/150;
    else
	chance = chance * ch_weapon/100;

    chance += (ch_vict_weapon/2 - vict_weapon) / 2; 

    /* dex vs. strength */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_STR);

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* If blinded? */
   if (IS_AFFECTED( ch, AFF_BLIND)|| IS_AFFECTED(ch, AFF_BLINDFOLDED) )
      chance = chance/2;
 
  if (ch->fighting != NULL && victim != ch->fighting) 
  {
	chance = (chance < 10 ? chance : UMIN(10,get_skill(ch,gsn_bash)/10));
  }
    /* and now the attack */
    if (number_percent() < chance)
    {
    	WAIT_STATE( ch, skill_table[gsn_disarm].beats );
	disarm( ch, victim );
	check_improve(ch,gsn_disarm,TRUE,1);
    }
    else
    {
	WAIT_STATE(ch,skill_table[gsn_disarm].beats);
	act("{5You fail to disarm $N.{x",ch,NULL,victim,TO_CHAR);
	act("{5$n tries to disarm you, but fails.{x",ch,NULL,victim,TO_VICT);
	act("{5$n tries to disarm $N, but fails.{x",ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,gsn_disarm,FALSE,1);
    }
    check_killer(ch,victim);
    return;
}

void do_surrender( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *mob;
  CHAR_DATA *mount=NULL;
  CHAR_DATA *pet=NULL;    
  AFFECT_DATA af;

  if ( (mob = ch->fighting) == NULL ) {
    send_to_char( "But you're not fighting!\r\n", ch );
    return;
  }

  act( "You surrender to $N!", ch, NULL, mob, TO_CHAR );
  act( "$n surrenders to you!", ch, NULL, mob, TO_VICT );
  act( "$n tries to surrender to $N!", ch, NULL, mob, TO_NOTVICT );
  stop_fighting( ch, TRUE );
  ch->surrender_timeout = current_time + 120;
  
  // If surrender, also stop mount/pet  
  if (!IS_NPC(ch)) {
    if (ch->mount != NULL && ch->in_room == ch->mount->in_room) {
	 mount = ch->mount;
	 mount->fighting	= NULL;
	 mount->position	= IS_NPC(mount) ? mount->default_pos : POS_STANDING;
	 update_pos( mount );
    }
     
    if (ch->pet != NULL) {
	 pet = ch->pet;
	 pet->fighting	= NULL;
	 pet->position	= IS_NPC(pet) ? pet->default_pos : POS_STANDING;
	 update_pos( pet );
    }
  }    

  if ( !IS_NPC( ch ) && IS_NPC( mob ) && ( !HAS_TRIGGER( mob, TRIG_SURR ) || !mp_percent_trigger( mob, ch, NULL, NULL, TRIG_SURR ) ) ) {
    act( "$N seems to ignore your cowardly act!", ch, NULL, mob, TO_CHAR );
    multi_hit( mob, ch, TYPE_UNDEFINED );
  }
  else if (!IS_NPC(mob)) {
    if (mob->pcdata->learned[gsn_bind] > 0 && IS_SET(mob->auto_act, AUTO_BIND)) {
	 act ( "$N quickly bind you so you can't escape!", ch, NULL, mob, TO_CHAR );
	 act ( "$N quickly bind $n so $e can't escape!", ch, NULL, mob, TO_NOTVICT );
	 act ( "You quickly bind $n so $e can't escape!", ch, NULL, mob, TO_VICT );
	 af.where     = TO_AFFECTS;
	 af.casterId  = mob->id;
	 af.type      = gsn_bind;
	 af.level     = mob->level;
	 af.duration  = mob->pcdata->learned[gsn_bind]/4;
	 af.location  = APPLY_NONE;
	 af.modifier  = 0;
	 af.bitvector = AFF_BIND;
	 affect_to_char(ch,&af);
    }
    if (IS_SET(mob->auto_act, AUTO_BLINDFOLD))
    {
  	act ( "$n blindfolds $N!", ch, NULL, mob, TO_NOTVICT );
  	act ( "You blindfold $N!", ch, NULL, mob, TO_CHAR );
  	act ( "$n blindfolds you!", ch, NULL, mob, TO_VICT );
  	SET_BIT(ch->affected_by, AFF_BLINDFOLDED);
    }
  }
}

void do_sla( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SLAY, spell it out.\r\n", ch );
    return;
}



void do_slay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Slay whom?\r\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\r\n", ch );
	return;
    }

    if ( ch == victim )
    {
	send_to_char( "Suicide is a mortal sin.\r\n", ch );
	return;
    }

    if ( !IS_NPC(victim) && victim->level >= get_trust(ch) )
    {
	send_to_char( "You failed.\r\n", ch );
	return;
    }

    act( "{1You slay $M in cold blood!{x",  ch, NULL, victim, TO_CHAR    );
    act( "{1$n slays you in cold blood!{x", ch, NULL, victim, TO_VICT    );
    act( "{1$n slays $N in cold blood!{x",  ch, NULL, victim, TO_NOTVICT );
    raw_kill( victim );
    check_valid_pkill(ch,victim);
    return;
}

void do_slayroom( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    CHAR_DATA *victim_next;
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );
  
    for (victim = ch->in_room->people; victim != NULL; victim = victim_next) 
    {
       victim_next = victim->next_in_room;
       if ( ch == victim )
       {
          continue;
       }

       act( "{1You take down  $M in cold blood!{x",  ch, NULL, victim, TO_CHAR    );
       act( "{1$n takes you out in cold blood!{x", ch, NULL, victim, TO_VICT    );
       act( "{1$n takes down $N in cold blood!{x",  ch, NULL, victim, TO_NOTVICT );
       raw_kill( victim );
       check_valid_pkill(ch,victim);
    }
    return;
}

void do_finish( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];


    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Finish off whom?\r\n", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\r\n", ch );
	return;
    }

    if ( ch == victim )
    {
	send_to_char( "Suicide is a mortal sin.\r\n", ch );
	return;
    }

    if ( IS_NPC(victim)) {
  	send_to_char("You have to kill them the hard way.\r\n", ch);
	return; 
    }

    if (!IS_RP(ch)) {
	send_to_char("You must be flagged as Completely IC to do this.\r\n",ch);
	return;
    }
    if (!IS_RP(victim)) {
	send_to_char("Your victim must be flagged as Completely IC to do this.\r\n",ch);
	return;
    }
    if (victim->position != POS_DEAD) {
	send_to_char("They aren't ready to go without a fight yet.\r\n",ch);
	return; 
    }

    if (IS_TROLLOC(ch) || IS_TROLLOC(victim) || IS_FACELESS(ch) || IS_FACELESS(victim))
    {
       sprintf(buf,"$N Finished off someone that looks like %s at room %d.",victim->name, victim->in_room->vnum);
       wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
       act( "{1You finish off someone that looks like $M!{x",  ch, NULL, victim, TO_CHAR    );
       act( "{1$n finishes off someone that looks like you!{x", ch, NULL, victim, TO_VICT    );
       act( "{1$n finishes someone that looks like $N off!{x",  ch, NULL, victim, TO_NOTVICT );
       make_fake_corpse(victim);
       stop_follower(victim);
       char_from_room( victim );
       char_to_room( victim, get_room_index( ROOM_VNUM_RECALL));
       do_restore(victim,victim->name);
       return;
    }

    sprintf(buf,"$N Finished off %s at room %d.",victim->name, victim->in_room->vnum);
    wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
    act( "{1You finish off $M!{x",  ch, NULL, victim, TO_CHAR    );
    act( "{1$n finishes you off!{x", ch, NULL, victim, TO_VICT    );
    act( "{1$n finishes $N off!{x",  ch, NULL, victim, TO_NOTVICT );
    send_to_char("You have been killed off!\r\n",victim);
    make_corpse(victim);
    stop_follower(victim);
    char_from_room( victim );
    char_to_room( victim, get_room_index( ROOM_VNUM_DEATH));
    do_restore(victim,victim->name);
    save_char_obj( ch, FALSE );
    return;
}

int bsmod( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int modifier;
    modifier = 0;
    if ( IS_AFFECTED( ch, AFF_HIDE) &&
          !IS_AFFECTED( victim, AFF_DETECT_HIDDEN) )
        modifier += 15;
    if ( IS_AFFECTED( ch, AFF_INVISIBLE) &&
         !IS_AFFECTED( victim, AFF_DETECT_INVIS) )
        modifier += 15;
    if (IS_AFFECTED( victim, AFF_BLIND)|| IS_AFFECTED(victim, AFF_BLINDFOLDED) )
        modifier = 35;
    if (IS_AFFECTED( ch, AFF_SNEAK) )
        modifier += 7;
    if (IS_AFFECTED( victim, AFF_SLEEP) )
        modifier = 45;
    if (!IS_NPC(victim)) {
    	if (IS_DR( victim )) {
		modifier = -100;
    	} 
    	if (IS_FORSAKEN( victim )) {
		modifier = -50;
    	} 
    }
    if (!IS_NPC(victim)) {
	modifier -= get_skill(victim,gsn_observation) / 10;
	modifier -= get_skill(victim,gsn_alertness) / 10;
	modifier -= get_skill(victim,gsn_awareness) / 10;
    }
    return(modifier);
}

void do_sap( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  OBJ_DATA *obj;
  AFFECT_DATA af;
  int dam, roll; 
  int hitc, hitv, hitcheck;
  bool immune = FALSE;
  
  one_argument( argument, arg );
  
  if (!IS_NPC(ch) && ch->pcdata->learned[gsn_sap] < 1) {
    send_to_char( "I'd recommend learning how to swing that first?\r\n", ch );
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
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }  

  if ( arg[0] =='\0' ) {
    send_to_char( "Who do you want to sap?\r\n", ch );
    return;
  }

  if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
    send_to_char( "They aren't here!\r\n", ch );
    return;
  }
  
  if ( victim == ch ) {
    send_to_char( "How do you plan on doing that??\r\n", ch );
    return;
  }
  
  if ( is_safe( ch, victim ) ) {
   // send_to_char( "Bummer!! They're not able to be sapped.",ch);
    return;
  }
  
  if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) ==NULL) {
    send_to_char( "You need something to sap them with...\r\n", ch );
    return;
  }
  
  if ( victim->fighting != NULL ) {
    send_to_char( "You can not sap a fighting person.\r\n", ch);
    return;
  }
  
  if(!IS_AWAKE(victim) ) { 
    send_to_char( "You can not sap an already sleeping person.\r\n",ch);
    return;
  }
  
  WAIT_STATE( ch, skill_table[gsn_sap].beats );
  
  roll=60+dice(1,15);
  hitc = ch->level * ch->pcdata->learned[gsn_sap] * dice(1,30) *100;
  hitv = victim->level * roll * get_curr_stat(victim, STAT_DEX);

  if (hitv) 
    hitcheck = sap_bonus(ch) + (hitc/hitv) + bsmod(ch,victim);
  else 
    hitcheck=0;
  
  switch(check_immune(victim,IMM_SAP)) {
  case(IS_IMMUNE):
    immune = TRUE;
    dam = 0;
    break;
  }
  // make immortals immune
  if ((victim->level >= LEVEL_IMMORTAL) && (ch->level <= LEVEL_IMMORTAL)) { 
    immune = TRUE;
    dam = 0;
  }
  
  if( (hitcheck > 130) && (!immune))  { 
    
    check_improve(ch, gsn_sap, TRUE, 1);
    
    af.where     = TO_AFFECTS;
    af.type      = gsn_sap; 
    af.level     = ch->level;
    af.duration  = ch->level/8;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SAP;
    affect_join( victim, &af );
    
    if (IS_AWAKE(victim)) {
	 send_to_char("Someone clubs you over the head with something...ugh...\r\n", victim );
	 act("$n slumps to the ground.", victim, NULL, NULL, TO_ROOM );
	 victim->position = POS_SLEEPING;
	 	   
	 if (IS_AFFECTED(victim,AFF_CHANNELING)) {
	   do_function(victim, &do_unchannel, "" );
	 }	 
    }
    return;
  }  
  else {
    send_to_char("You miss and strike somewhere in the back.\r\n",ch );
    check_improve(ch,gsn_sap,FALSE,1);
    dam=(dice(4,6)+get_curr_stat(ch,STAT_STR))*ch->pcdata->learned[gsn_sap] /75;
    damage(ch, victim, dam, gsn_sap,DAM_BASH, TRUE);
    return;
  }
  send_to_char( "There must be a bug....\r\n",ch );  
  return;
}

int sap_bonus( CHAR_DATA *ch )
{
	int wtype, mod;

	wtype = get_weapon_sn(ch);
	mod = 0;

	if( wtype == gsn_mace) mod += 25; /*mace*/
	if( wtype == gsn_flail) mod += 25; /*flail*/
	if( wtype == gsn_axe) mod += 15; /*axe*/
	if( wtype == gsn_spear) mod += 10; /*spear/staff*/
	if( wtype == gsn_sword) mod += -5; /*sword*/
	if( wtype == gsn_dagger) mod += -5; /*dagger*/
	if( wtype == gsn_polearm) mod += -5; /*polearm*/
	if( wtype == gsn_whip) mod += -20; /*whip*/
	return(mod);
}

void do_defend(CHAR_DATA *ch, char *argument)
{
  int i=0;
  char buf[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  if (IS_NULLSTR(argument)) {
    if (ch->defend_loc == LOC_NA)
	 send_to_char("You defend all locations equal.\r\n", ch);
    else {
	 sprintf(buf, "You are trying to defend the %s mostly.\r\n", hit_flags[ch->defend_loc].name);
	 send_to_char(buf, ch);	 
    }
    return;
  }
  else if (!str_cmp(argument, "all") ||
		 !str_cmp(argument, "reset") ||
		 !str_cmp(argument, "default")) {
    ch->defend_loc = LOC_NA;
    send_to_char("You will now try to defend all locations equal.\r\n", ch);
    return;
  }
  else {
    for (i=0; i<MAX_HIT_LOC; i++) {
	 if (!str_cmp(argument, short_hit_flags[i].name) ||
		!str_cmp(argument, hit_flags[i].name)) {
	   ch->defend_loc = i;
	   sprintf(buf, "You will now try to defend the %s mostly.\r\n", hit_flags[ch->defend_loc].name);
	   send_to_char(buf, ch);
	   return;
	 }
    }
  }
  
  send_to_char("There is no such defend option.\r\n", ch);
  sprintf(buf, "Possible options are: ");
  
  for (i=0; i<MAX_HIT_LOC; i++) {
    strcat(buf, "'");
    strcat(buf, short_hit_flags[i].name);
    strcat(buf, "' ");
  }
  
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
  return;
}

void do_target(CHAR_DATA *ch, char *argument)
{
  int i=0;
  char buf[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  if (IS_NULLSTR(argument)) {
    if (ch->target_loc == LOC_NA)
	 send_to_char("You target all locations equal.\r\n", ch);
    else {
	 sprintf(buf, "You are trying to target the %s mostly.\r\n", hit_flags[ch->target_loc].name);
	 send_to_char(buf, ch);	 
    }
    return;
  }
  else if (!str_cmp(argument, "all") ||
		 !str_cmp(argument, "reset") ||
		 !str_cmp(argument, "default")) {
    ch->target_loc = LOC_NA;
    send_to_char("You will now try to target all locations equal.\r\n", ch);
    return;
  }
  else {
    for (i=0; i<MAX_HIT_LOC; i++) {
	 if (!str_cmp(argument, short_hit_flags[i].name) ||
		!str_cmp(argument, hit_flags[i].name)) {
	   ch->target_loc = i;
	   sprintf(buf, "You will now try to target the %s mostly.\r\n", hit_flags[ch->target_loc].name);
	   send_to_char(buf, ch);
	   return;
	 }
    }
  }
  
  send_to_char("There is no such target option.\r\n", ch);
  sprintf(buf, "Possible options are: ");
  
  for (i=0; i<MAX_HIT_LOC; i++) {
    strcat(buf, "'");
    strcat(buf, short_hit_flags[i].name);
    strcat(buf, "' ");
  }  
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  return;
}

void do_target_OLD ( CHAR_DATA *ch, char *argument )
{
  long bit_to_set = 0;
  char buf[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  
  buf[0] = '\0';
  
  one_argument( argument, arg );
  
  
  if ( arg[0] == '\0' ) {

    if (IS_SET(ch->off_flags,TARGET_HEAD)) strcat(buf, "You target the head mostly.\r\n");
    if (IS_SET(ch->off_flags,TARGET_NECK)) strcat(buf, "You target the neck mostly.\r\n");
    if (IS_SET(ch->off_flags,TARGET_TORSO)) strcat(buf, "You target the torso mostly.\r\n");
    if (IS_SET(ch->off_flags,TARGET_ARMS)) strcat(buf, "You target the arms mostly.\r\n");
    if (IS_SET(ch->off_flags,TARGET_HANDS)) strcat(buf, "You target the hands mostly.\r\n");
    if (IS_SET(ch->off_flags,TARGET_BACK)) strcat(buf, "You target the back mostly.\r\n");
    if (IS_SET(ch->off_flags,TARGET_LEGS)) strcat(buf, "You target the legs mostly.\r\n");
    if (IS_SET(ch->off_flags,TARGET_FEET)) strcat(buf, "You target the feet mostly.\r\n");
    if (IS_SET(ch->off_flags,TARGET_GENERAL)) strcat(buf, "You don't target anything in particular.\r\n");
			
    
    send_to_char(buf, ch );
    return;
  }

  if (strcmp (arg, "head") == 0) SET_BIT(bit_to_set, TARGET_HEAD);
  if (strcmp (arg, "neck") == 0) SET_BIT(bit_to_set, TARGET_NECK);
  if (strcmp (arg, "torso") == 0) SET_BIT(bit_to_set, TARGET_TORSO);
  if (strcmp (arg, "arms") == 0) SET_BIT(bit_to_set, TARGET_ARMS);
  if (strcmp (arg, "hands") == 0) SET_BIT(bit_to_set, TARGET_HANDS);
  if (strcmp (arg, "back") == 0) SET_BIT(bit_to_set, TARGET_BACK);
  if (strcmp (arg, "legs") == 0) SET_BIT(bit_to_set, TARGET_LEGS);
  if (strcmp (arg, "feet") == 0) SET_BIT(bit_to_set, TARGET_FEET);
  if (strcmp (arg, "general") == 0) SET_BIT(bit_to_set, TARGET_GENERAL);
  
  if (bit_to_set) {
    REMOVE_BIT(ch->off_flags, TARGET_HEAD);
    REMOVE_BIT(ch->off_flags, TARGET_NECK);
    REMOVE_BIT(ch->off_flags, TARGET_TORSO);
    REMOVE_BIT(ch->off_flags, TARGET_ARMS);
    REMOVE_BIT(ch->off_flags, TARGET_HANDS);
    REMOVE_BIT(ch->off_flags, TARGET_BACK);
    REMOVE_BIT(ch->off_flags, TARGET_LEGS);
    REMOVE_BIT(ch->off_flags, TARGET_FEET);
    REMOVE_BIT(ch->off_flags, TARGET_GENERAL);
    SET_BIT(ch->off_flags, bit_to_set);
    
    if (IS_SET(ch->off_flags,TARGET_HEAD)) strcat(buf, "You will now target the head.\r\n");
    if (IS_SET(ch->off_flags,TARGET_NECK)) strcat(buf, "You will now target the neck.\r\n");
    if (IS_SET(ch->off_flags,TARGET_TORSO)) strcat(buf, "You will now target the torso.\r\n");
    if (IS_SET(ch->off_flags,TARGET_ARMS)) strcat(buf, "You will now target the arms.\r\n");
    if (IS_SET(ch->off_flags,TARGET_HANDS)) strcat(buf, "You will now target the hands.\r\n");
    if (IS_SET(ch->off_flags,TARGET_BACK)) strcat(buf, "You will now target the back.\r\n");
    if (IS_SET(ch->off_flags,TARGET_LEGS)) strcat(buf, "You will now target the legs.\r\n");
    if (IS_SET(ch->off_flags,TARGET_FEET)) strcat(buf, "You will now target the feet.\r\n");
    if (IS_SET(ch->off_flags,TARGET_GENERAL)) strcat(buf, "You won't target anything in particular.\r\n");
    send_to_char(buf, ch );
    return;
  }
  
  send_to_char("There is no such option",ch);
  
  return;
}

bool check_critical(CHAR_DATA *ch, CHAR_DATA *victim)
{
  OBJ_DATA *obj;

  obj = get_eq_char(ch,WEAR_WIELD);
  
  if (( get_eq_char(ch,WEAR_WIELD) == NULL ) || 
	 ( get_skill(ch,gsn_critical)  <  1 ) ||
	 ( get_weapon_skill(ch,get_weapon_sn(ch)) <  90 ) ||
	 ( number_range(0,100) > get_skill(ch,gsn_critical) )
	 )
    return FALSE;
  

  if ( number_range(0,100) > 10 )
    return FALSE;

  /* Now, if it passed all the tests... */  
  return TRUE;
}

void do_flush( CHAR_DATA *ch, char *argument )
{
  OBJ_DATA *obj;
  OBJ_DATA *container_obj;
  char buf[MAX_STRING_LENGTH];
  int amount=0;
  int liquid=0;

  // Is blinded (not by weave)
  if (!IS_AFFECTED(ch, AFF_BLIND) ) {
    send_to_char("You are not blinded.\r\n", ch);
    return;
  }

  // Character have container with water to flush ?
  for ( obj = ch->carrying; obj != NULL; obj = obj->next_content ) {
    if (obj->item_type == ITEM_CONTAINER) {
	 for (container_obj = obj->contains; container_obj != NULL; container_obj = container_obj->next_content) {
	   if (container_obj->item_type == ITEM_DRINK_CON || container_obj->item_type == ITEM_FOUNTAIN) {
	   	if (container_obj->value[1] <= 0)
	   	   continue;
	   	else {
	           if ((liquid = container_obj->value[2]) < 0)
	              continue;
	           else {
	              amount = liq_table[liquid].liq_affect[4];
	              amount = UMIN(amount, container_obj->value[1]);
	              if (container_obj->value[0] > 0)
	                container_obj->value[1] -= amount;
		      sprintf(buf, "You manage to find %s and flush your eyes!\r\n", container_obj->short_descr);
		      send_to_char(buf, ch);
		      affect_strip(ch,gsn_dirt);
		      return;
		   }
		}
	   }
	 }
    }
    else {
	 if (obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOUNTAIN) {
	   if (obj->value[1] <= 0)
	     continue;
	   else {
	      if ((liquid = obj->value[2]) < 0)
		continue;
	      else {
	      	if (number_percent() > 65) {
	          amount = liq_table[liquid].liq_affect[4];
	          amount = UMIN(amount, obj->value[1]);
	          if (obj->value[0] > 0)
	             obj->value[1] -= amount;
	          sprintf(buf, "You manage to find %s and flush your eyes!\r\n", obj->short_descr);
	          send_to_char(buf, ch);
	          affect_strip(ch,gsn_dirt);
	          WAIT_STATE(ch,3 * PULSE_VIOLENCE);
	          return;
	        }
	        else {
	          send_to_char("You can't find anything to flush your eyes with!\r\n", ch);
                  return;	        	
	        }
	      }
	   }       
	}
    }
  }
  
  // If player don't have container with water, look in room...
  // Spring in room then?
  for ( obj = ch->in_room->contents; obj; obj = obj->next_content ) {
    if ( obj->item_type == ITEM_FOUNTAIN) {
	 act("$n fumble for the $p and try to flush $s eyes.", ch, obj, NULL, TO_ROOM);
	 if (number_percent() > 80) {		  
	   if ((liquid = obj->value[2]) < 0)
	      continue;
	   else {
	      amount = liq_table[liquid].liq_affect[4] * 3;
	      if (obj->value[0] > 0)
	         obj->value[1] -= amount;
	      sprintf(buf, "You manage to find %s and flush your eyes!\r\n", obj->short_descr);
	      send_to_char(buf, ch);
	      affect_strip(ch,gsn_dirt);
	      WAIT_STATE(ch,3 * PULSE_VIOLENCE);
	      return;
	   }
	 }
	 else {
	   send_to_char("You can't find anything to flush your eyes with!\r\n", ch);
           return;
        }
    }
  }

  send_to_char("You can't find anything to flush your eyes with!\r\n", ch);
  return;  
}

// Try to find a victim
CHAR_DATA *find_charge_victim(CHAR_DATA *ch, int dir, int distance, char *target, int *room_vnum, int *real_distance, long *exit_flag)
{
  CHAR_DATA *victim=NULL;
  int visibility;
  int i=0;
  ROOM_INDEX_DATA *was_in_room=NULL;
  //char buf[MSL];

  // We have a target name at least?
  if (IS_NULLSTR(target)) {
    *room_vnum = 0;
    *real_distance = 0;
    *exit_flag = 0;
    return NULL;
  }

  visibility = 6;
  if( !IS_SET( ch->act, PLR_HOLYLIGHT ) ) {
    switch( weather_info.sunlight ) {
    case SUN_SET:   visibility = 4; break;
    case SUN_DARK:  visibility = 2; break;
    case SUN_RISE:  visibility = 4; break;
    case SUN_LIGHT: visibility = 6; break;
    }
    switch( weather_info.sky ) {
    case SKY_CLOUDLESS: break;
    case SKY_CLOUDY:    visibility -= 1; break;
    case SKY_RAINING:   visibility -= 2; break;
    case SKY_LIGHTNING: visibility -= 3; break;
    }
  }

  // Store starting room
  was_in_room = ch->in_room;

  for(i=1; i <= distance; i++ ) {
    EXIT_DATA *pexit=NULL;
    EXIT_DATA *pexit_rev=NULL;
    
    if(( pexit = ch->in_room->exit[dir]) != NULL
	  && pexit->u1.to_room != NULL
	  && pexit->u1.to_room != was_in_room ) {

         pexit_rev = pexit->u1.to_room->exit[rev_dir[dir]];

	 /* If the door is closed, stop looking... */
	 if(IS_SET(pexit->exit_info, EX_CLOSED )) {
	   ch->in_room = was_in_room;	 	
	   *room_vnum = pexit->u1.to_room->vnum;
	   *real_distance = i;
	   *exit_flag = 0;
	   return NULL;
	 }
	 
	 // If firewall
	 if (IS_SET(pexit->exit_info, EX_FIREWALL)) {
	   ch->in_room = was_in_room;	 	
	   *room_vnum = pexit_rev->u1.to_room->vnum;
	   *real_distance = i;
	   *exit_flag = EX_FIREWALL;
	   return NULL;
	 }
	 
	 // if airwall
	 if (IS_SET(pexit->exit_info, EX_AIRWALL)) {
	   ch->in_room = was_in_room;	 	
	   *room_vnum = pexit_rev->u1.to_room->vnum;
	   *real_distance = i;
	   *exit_flag = EX_AIRWALL;
	   return NULL;
	 }

	 // if blocked by someone
	 if (IS_SET(pexit->exit_info, EX_BLOCKED)) {
	   ch->in_room = was_in_room;
	   *room_vnum = pexit_rev->u1.to_room->vnum;
	   *real_distance = i;
	   *exit_flag = EX_BLOCKED;
	   return NULL;
	 }

	 ch->in_room = pexit->u1.to_room;	 	 
	 if( IS_OUTSIDE(ch) ? i > visibility : i > 4 )
	   break;	
	 
	 if (( victim = get_char_room( ch, target )) != NULL ) {
	   ch->in_room = was_in_room;
	   *room_vnum = pexit->u1.to_room->vnum;	
	   *real_distance = i;
	   *exit_flag = 0;
	   return victim;
	 }
    }
  }

  *exit_flag = 0;
  ch->in_room = was_in_room;
  return NULL;
}

void do_charge( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *victim=NULL;
  CHAR_DATA *mount=NULL;
  OBJ_DATA *obj=NULL;
  int chance=0;
  int dam=0;
  int real_dam=0;
  int endurance=0;
  int dir=0;
  int distance=0;
  int real_distance=0;
  int room_vnum=0;
  long exit_flag;
  bool from_distance=FALSE;

  // Need to know how to handle a lance
  if (!IS_NPC(ch) && ch->pcdata->learned[gsn_lance] < 1) {
    send_to_char("You don't know how to use a lance!\r\n", ch);
    return;
  }

  // Know how to charge?
  if (!IS_NPC(ch) && ch->pcdata->learned[gsn_charge] < 1) {
    send_to_char("You don't know how to use a lance and horse to charge yet.\r\n", ch);
    return;
  }

  // Need to be mounted
  if (!MOUNTED(ch)) {
    send_to_char("You can't charge without a mount.\r\n", ch );
    return;
  }

  // Not charmed
  if ( IS_NPC(ch) && IS_AFFECTED( ch, AFF_CHARM ) ) {
    send_to_char( "You can't do that right now.\r\n", ch );
    return;
  }

  /* Is blocking an exit? */
  if (IS_BLOCKING(ch)) {
    sprintf(buf, "You are still trying to block the %s entrance.\n\r", dir_name[ch->exit_block.direction]);
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
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }
  
  argument = one_argument( argument, arg );
  
  endurance = skill_table[gsn_charge].min_endurance;
  
  if (  IS_NULLSTR(arg) ) {
    send_to_char( "Syntax: charge <target>\r\n", ch );
    send_to_char( "        charge <direction> <target>\r\n", ch);
    return;
  }
  
  if (ch->endurance < endurance) {
    send_to_char("You are too tired to keep your lance steady!\r\n", ch);
    return;
  }
  
  if (!IS_NPC(ch) && ch->pcdata->next_charge > current_time) {
  	send_to_char("You are steadying your mount, getting ready to charge again.\r\n", ch);
        return;	
    }  
  
  // ---------- NORTH ------------
  if (!str_prefix(arg, "north")) { 
    dir = DIR_NORTH; 
    from_distance = TRUE; 
    distance = UMAX(1, ch->pcdata->learned[gsn_charge]/20); 
  }
  // ---------- EAST ------------
  else if( !str_prefix( arg,  "east"  )) { 
    dir = DIR_EAST;  
    from_distance = TRUE; 
    distance = UMAX(1, ch->pcdata->learned[gsn_charge]/20); 
  }
  // ---------- SOUTH ------------
  else if( !str_prefix( arg,  "south" )) { 
    dir = DIR_SOUTH; 
    from_distance = TRUE; 
    distance = UMAX(1, ch->pcdata->learned[gsn_charge]/20); 
  }
  // ---------- WEST ------------
  else if( !str_prefix( arg,  "west"  )) { 
    dir = DIR_WEST;  
    from_distance = TRUE; 
    distance = UMAX(1, ch->pcdata->learned[gsn_charge]/20); 
  }
  // ---------- UP ------------
  else if( !str_prefix( arg,  "up"    )) { 
    dir = DIR_UP;    
    from_distance = TRUE; 
    distance = UMAX(1, ch->pcdata->learned[gsn_charge]/20); 
  }
  // ---------- DOWN ------------
  else if( !str_prefix( arg,  "down"  )) { 
    dir = DIR_DOWN;  
    from_distance = TRUE; 
    distance = UMAX(1, ch->pcdata->learned[gsn_charge]/20); }
  // ---------- IN ROOM ------------
  else {
    // Victim in room?
    if (( victim = get_char_room( ch, arg )) == NULL ) {
	 send_to_char( "They aren't here.\r\n", ch );
	 return;
    }
    else
	 from_distance = FALSE;
  }
  
  if (from_distance) {
    victim = find_charge_victim(ch, dir, distance, argument, &room_vnum, &real_distance, &exit_flag);
    if ((victim) && ( IS_SET(victim->in_room->room_flags, ROOM_PRIVATE) ))
    {
	send_to_char("That room is too small for you to charge into!\r\n",ch);
	return;
    }


    if (victim == NULL) {
	 
	 // Check if any special exit flags detected
	 if (exit_flag == 0) {
	   sprintf(buf, "You rise in your stirrups and aim to charge %s but settle back down as your target seems to be out of range or not there.\r\n", dir_name[dir]);
	   send_to_char( buf, ch );
	   return;
	 }
	 
	 // Firewall exit flag
	 if (exit_flag == EX_FIREWALL) {

	   // Move char to the place where firewall is
	   char_from_room(ch);
	   char_to_room(ch, get_room_index(room_vnum));

	   sprintf(buf, "$n arrive apon $s mount in a wild charge from the %s.", dir_name[rev_dir[dir]]);
	   act(buf,  ch, NULL, NULL, TO_ROOM );

	   // Also move mount, but do it quiet
	   if (ch->mount != NULL) {
	   	mount=ch->mount;
		char_from_room( mount );
		char_to_room( mount,  get_room_index(room_vnum));
		ch->mount_quiet = TRUE;
		do_mount(ch, mount->name);
	   }
	   
	   act("You ride into a {rwall of fire{x and is thrown off the horse as the {Rfire{x surrounds you!", ch, NULL, NULL, TO_CHAR);
	   act("$n ride into the {rwall of fire{x and is thrown off the horse as the {Rfire{x surrounds $m!", ch, NULL, NULL, TO_ROOM);
	   damage(ch, ch, UMAX(100, ch->hit/2), gsn_wof, DAM_FIRE, FALSE);	   
	   
	   // Throw off horse
	   ch->riding = FALSE;
	   ch->mount->riding = FALSE;
	   if (ch->position > POS_STUNNED) 
		ch->position=POS_SITTING;
	   update_pos(ch);
    	   WAIT_STATE(ch,skill_table[gsn_charge].beats);
	   
	   return;
	 }
	 
	 // Airwall exit flag
	 if (exit_flag == EX_AIRWALL) {

	   // Move char to the place where Airwall is
	   char_from_room(ch);
	   char_to_room(ch, get_room_index(room_vnum));

	   sprintf(buf, "$n arrive apon $s mount in a wild charge from the %s.", dir_name[rev_dir[dir]]);
	   act(buf,  ch, NULL, NULL, TO_ROOM );

	   // Also move mount
	   if (ch->mount != NULL) {
	   	mount=ch->mount;
		char_from_room( mount );
		char_to_room( mount,  get_room_index(room_vnum));
		ch->mount_quiet = TRUE;
		do_mount(ch, mount->name);
	   }
	   
	   act("You ride into a {Cwall of air{x and is knocked off the horse!", ch, NULL, NULL, TO_CHAR);
	   act("$n ride into the {Cwall of air{x and is knocked off the horse!", ch, NULL, NULL, TO_ROOM);
	   
	   // Throw off horse
	   ch->riding = FALSE;
	   ch->mount->riding = FALSE;
	   if (ch->position > POS_STUNNED) 
		ch->position=POS_SITTING;
	   update_pos(ch);
	   
	   if (mount->position > POS_STUNNED) 
		mount->position=POS_SITTING;
	   update_pos(mount);
    	   WAIT_STATE(ch,skill_table[gsn_charge].beats);
	   
	   return;
	 }

	 // If exit is blocked, the blocker gets to be the target!
	 if (exit_flag == EX_BLOCKED) {
	   // Move char to the room where the entrance is blocked
	   char_from_room(ch);
	   char_to_room(ch, get_room_index(room_vnum));

	   sprintf(buf, "$n arrive apon $s mount in a wild charge from the %s.", dir_name[rev_dir[dir]]);
	   act(buf,  ch, NULL, NULL, TO_ROOM );
	   
	   // Also move mount
	   if (ch->mount != NULL) {
	   	mount=ch->mount;
		char_from_room( mount );
		char_to_room( mount,  get_room_index(room_vnum));
		ch->mount_quiet = TRUE;
		do_mount(ch, mount->name);
	   }

	   victim = get_blocker(ch->in_room, dir);
	   if (victim != NULL) {

		sprintf(buf, "$N blocks the %s entrance, and you are unable to avoid charging into $M!", dir_name[dir]);
		act(buf, ch, NULL, victim, TO_CHAR);

		sprintf(buf, "$N blocks the %s entrance, and $n is unable to avoid charging into $M!", dir_name[dir]);
		act(buf, ch, NULL, victim, TO_ROOM);

		stop_exit_block(victim);

		// Move victim to the correct room
		char_from_room( victim );
		char_to_room( victim, ch->in_room );
	   }
	 }
	 
    }
  }
  
  if ( victim == ch ) {
    send_to_char("How can you charge at yourself?\r\n", ch );
    return;
  }
  
  if ( is_safe( ch, victim ) )
    return;  
  
  // Only for lances
  if (( obj = get_eq_char(ch, WEAR_WIELD)) == NULL || ( obj->value[0] != WEAPON_LANCE )) {
    send_to_char("You need to wield a lance before you can charge!\r\n", ch );
    return;
  }
    
  if (victim == ch->mount) {
    send_to_char("How can you charge at your mount?\r\n", ch);
    return;	
  }
  
  
  if (victim->fighting) {
    send_to_char( "You can't charge at someone who is in combat.\r\n", ch );
    return;
  }
  
  if (!IS_NPC(ch)) {
    dam = (((ch->pcdata->learned[gsn_charge]/2) + (ch->pcdata->learned[gsn_riding]/10) + (ch->pcdata->learned[gsn_lance]/2))  * number_range( obj->value[1] * ch->pcdata->learned[gsn_lance]/100, obj->value[2] * ch->pcdata->learned[gsn_lance]/100));
    chance = ch->pcdata->learned[gsn_charge];
  }
  else {
    dam = ch->level*2;
    chance = ch->level;
  }
  
  // Reduce chance based on victims attributes
  if (!IS_NPC(victim)) {
    chance -= victim->pcdata->learned[gsn_charge]/20;
    chance -= victim->pcdata->learned[gsn_riding]/20;
    chance -= victim->pcdata->learned[gsn_lance]/20;
    if  (IS_SET(victim->merits, MERIT_QUICKREFLEXES))
	 chance -= 15;
  }
  else {
    chance -= victim->level/10;
  }
  
  // If charge from distance, transf char and his mount to the victim's room
  // used move_char() for realistic move accross several rooms.
  if (from_distance) {
    char_from_room(ch);
    char_to_room(ch, get_room_index(room_vnum));
    
    sprintf(buf, "$n arrive apon $s mount in a wild charge from the %s.", dir_name[rev_dir[dir]]);
    act(buf,  ch, NULL, NULL, TO_ROOM );
   
    // Also move mount
    if (ch->mount != NULL) {
	 char_from_room( ch->mount );
	 char_to_room( ch->mount,  get_room_index(room_vnum));
	 ch->mount_quiet = TRUE;
	 do_mount(ch, ch->mount->name);
    }
  }
  
  if (number_percent() < chance) {
    act("Apon your mount you wildly charge at $N, thrusting $p toward $S body!", ch, obj, victim, TO_CHAR );
    act("Apon $s mount $n charges wildly at you, thrusting $s $p toward your body!", ch, obj, victim, TO_VICT );
    act("Apon $s mount $n wildly charges at $N, thrusting $p toward $S body!", ch, obj, victim, TO_NOTVICT );
    
    real_dam = number_range(dam/2, dam);
    
    // More dam if from a distance...
    if (from_distance) {
	 real_dam = real_dam*(distance/2);
    }
    
    if (IS_CODER(ch)) {
	 sprintf(buf, "[ {YCoder{x ]: charge dam calc = %d, charge dam real = %d\r\n", dam, real_dam);
	 send_to_char(buf, ch);
    }
    
    if ( (victim->hit - real_dam) < 1)  {
	 act("{RYour lance wickedly thrusts into $N!{x", ch, NULL, victim, TO_CHAR );
	 act("{R$n's {Rlance wickedly thrusts into $N!{x", ch, NULL, victim, TO_NOTVICT );
	 act("{R$n's {Rlance wickedly thrusts into YOU!{x", ch, NULL, victim, TO_VICT );
    }
    
    damage(ch, victim, real_dam, gsn_charge, DAM_CRIT ,TRUE);

    // Possible to bash the victim if from a distance
    if (from_distance) {
	 if (number_percent() <= 10) {
	   act("{5Your mount hit $N{5 and throw $M off balance!{x", ch, obj, victim, TO_CHAR );
	   act("{5$n's{5 mount hit you and throw you off balance!{x", ch, obj, victim, TO_VICT );
	   act("{5$n's{5 mount hit $N{5 and throw $M off balance!{x", ch, obj, victim, TO_NOTVICT );
	   DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
	 }
    }
        
    if (number_percent() > 65)
	 check_improve(victim,gsn_charge,TRUE,1);
    
    ch->endurance -= endurance;
  }
  else {
    if (number_percent() > 65)
	 check_improve(victim,gsn_charge,FALSE,1);
    
    act("Apon your mount you wildly charge at $N, but $E dodges out of the way!", ch, NULL, victim, TO_CHAR );
    act("Apon $s mount $n wildly charges at you, but you quickly dodge out of $s way.", ch, NULL, victim, TO_VICT );
    act("Apon $s mount $n wildly charges at $N, but $E dodges out of the way!", ch, NULL, victim, TO_NOTVICT );
    
    damage( ch, victim, 0, gsn_charge, DAM_PIERCE ,TRUE);
    
    ch->endurance -= endurance/2;
  }
  
  // Only 1 charge per 60 sec
  if (!IS_NPC(ch)) {
    ch->pcdata->next_charge = current_time + 60;
  }
  WAIT_STATE(ch,skill_table[gsn_charge].beats);
  
  return;
}

void add_guard(CHAR_DATA *ch, CHAR_DATA *vch)
{
	act("You now guard $N!", ch, NULL, vch, TO_CHAR );
	act("$n starts guarding you!", ch, NULL, vch, TO_VICT );
	act("$n move closer to $N.",ch, NULL, vch, TO_NOTVICT );
	
	ch->pcdata->guarding = vch;
	vch->pcdata->guarded_by = ch;
	
	return;
}

void remove_guard(CHAR_DATA *ch, bool msg)
{
  CHAR_DATA *vch=NULL;
  
  if (IS_NPC(ch))
     return;
  
  if (IS_GUARDING(ch)) {
      vch = ch->pcdata->guarding;
      
      if (msg && vch != NULL) {
      	act("You stop guarding $N!", ch, NULL, vch, TO_CHAR );
      	act("$n stops guarding you!", ch, NULL, vch, TO_VICT );
      	act("$n move a little away from $N.",ch, NULL, vch, TO_NOTVICT );
      }
      
      vch->pcdata->guarded_by = NULL;
      ch->pcdata->guarding = NULL;
  }
  
  if (IS_GUARDED(ch)) {
      vch = ch->pcdata->guarded_by;
      
      vch->pcdata->guarding = NULL;
      ch->pcdata->guarded_by = NULL;
  }	
}

void do_guard( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  
  if (IS_NPC(ch))
  	return;
  	
  if (ch->pcdata->learned[gsn_guard] < 1) {
  	send_to_char("You don't know how to guard someone.\r\n", ch);
  	return;
  }
  
  if ( IS_NULLSTR(argument)) {
    send_to_char( "Guard whom?\r\n", ch );
    return;
  }

  if ( ( victim = get_char_room( ch, argument ) ) == NULL ) {
    send_to_char( "They aren't here.\r\n", ch );
    return;
  }
  
  if (IS_NPC(victim)) {
    send_to_char( "You can't guard mobiles.\r\n", ch );
    return;  	
  }
  
  if (!is_same_group(ch,victim)) {
    send_to_char("They need to be in your group before you can guard them.\r\n", ch);
    return;
  }

  if (IS_GUARDED(victim)) {
    send_to_char("They are already guarded!\r\n", ch);
    return;
  }
  
  
  if (ch == victim) {
  	if (IS_GUARDING(ch)) {
  		remove_guard(ch, TRUE);
  		send_to_char("You return to only make sure you are the healthy one.\r\n", ch);
  		return;
  	}
  	else {
  		send_to_char("You already have the focus at your self!\r\n", ch);
  		return;
  	}
  }
  else {
  	if (IS_GUARDING(ch)) {
	  	remove_guard(ch, TRUE);
	  	add_guard(ch, victim);
	  	return;
  	}
  	else {
	  	add_guard(ch, victim);
	  	return;
  	}  	
  }
  
  return;
}

// Trolloc clan promotion by PK
void check_trolloc_kill(CHAR_DATA *ch, CHAR_DATA *victim)
{
   char buf[MSL];
   int sn=skill_lookup(skill_table[gsn_trollocwarfare].name);;
   
   // If killing another higher rank trolloc in the clan 3 times, advance rank
   if (!IS_NPC(victim) && !IS_NPC(ch)) {
       if ((victim->race == race_lookup("trolloc")) && (ch->race == race_lookup("trolloc")) ){ // && is_same_sguild(ch, victim)) {
         //if (victim->sguild_rank < ch->sguild_rank) {
      	   ch->pcdata->clan_kill_cnt++;	     	
      	   //if (ch->pcdata->clan_kill_cnt >= 3 && ch->sguild_rank > 0) {
      	   if (ch->sguild_rank > 0) {
      	   	if ((ch->pcdata->clan_kill_cnt % 3) == 0) {
                     sguild_tr_promote(ch, TRUE, FALSE);	     	                     
                     sguild_tr_promote(victim, FALSE, FALSE);
                }
        	else {
      	          sprintf(buf, "You start to make your self more visible in the clan by killing higher ranked Trollocs!\r\n");
      	          send_to_char(buf, ch);
      	        }
      	   }
       }
       // If killing PCs 10 times, advance rank
       else if ( (ch->race == race_lookup("trolloc")) && 
              (victim->race != race_lookup("trolloc")) &&
              (victim->race != race_lookup("fade")) &&
              !IS_NEWBIE(victim) &&
              !IS_FORSAKEN(victim) &&
	      (victim->level >= ch->level - 50)) {
   
            ch->pcdata->pc_kill_cnt++;
      
            // Don't have TW, gain it
            if (get_level(ch) >= skill_table[sn].skill_level[ch->class] && ch->pcdata->learned[sn] < 1) {
               ch->pcdata->learned[sn] = 1;

                sprintf(buf, "You have been TR granted '%s' as a result of killing homans!\n\r", skill_table[sn].name);
		send_to_char(buf, ch);
		    
		sprintf(buf, "%s has been TR granted '%s' as a result of killing homans.", ch->name, skill_table[sn].name);
		log_string(buf);
		    
		sprintf(buf, "$N has been TR granted '%s' as a result of killing homans.", skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));               
            }
            // Have, improve it
            if (get_level(ch) >= skill_table[sn].skill_level[ch->class] && (ch->pcdata->learned[sn] > 0 && ch->pcdata->learned[sn] < MAX_PC_TRAIN)) {
               	ch->pcdata->learned[sn] += 1;
               	
               	sprintf(buf, "You feel your '%s' improve as you slay more homans!\n\r", skill_table[sn].name);
               	send_to_char(buf, ch);
            }
      
            if (ch->sguild_rank > 0) {
               if ((ch->pcdata->pc_kill_cnt % 10) == 0) {
   	     	         sguild_tr_promote(ch, TRUE, TRUE);
               }
               else {
                  send_to_char("You make your self more visible in the clan by killing puny homans!\r\n", ch);	
               }
            }
       }
       // If killed by a human, decrease the pc kill cnt and maybe demote a rank
       else if ( (ch->race != race_lookup("trolloc")) &&
                  (victim->race == race_lookup("trolloc"))) {

            if (victim->pcdata->learned[sn] > 0) {
            	if (victim->pcdata->learned[sn] == 1) {
            	   victim->pcdata->learned[sn] = 0;
            	   
            	   sprintf(buf, "You have lost '%s' as a result of beeing killed by a puny homan!\n\r", skill_table[sn].name);
		   send_to_char(buf, victim);

		   sprintf(buf, "%s has lost '%s' as a result of beeing killed by a puny homan.", victim->name, skill_table[sn].name);
		   log_string(buf);
		    
		   sprintf(buf, "$N has lost '%s' as a result of beeing killed by a puny homan.", skill_table[sn].name);
		   wiznet(buf, victim, NULL, WIZ_LEVELS, 0, get_trust(ch));		   		   
            	}
            	else {
            	    victim->pcdata->learned[sn] -= 1;
            	    
            	    sprintf(buf, "You feel your '%s' decrese as you get slayed by a puny homan!\n\r", skill_table[sn].name);
               	    send_to_char(buf, victim);
            	}                        	
            }
                
            if (victim->pcdata->pc_kill_cnt > 0) {
               victim->pcdata->pc_kill_cnt--;
                   
                if (victim->sguild_rank < 8) {
                   if ((victim->pcdata->pc_kill_cnt % 10) == 0) {
   	     	      sguild_tr_promote(victim, FALSE, TRUE);
                   }
                   else {
                      send_to_char("You make your self less visible in the clan by getting killed by a puny homan!\r\n", victim);	
                   }                    	                    	
                }
             }
         }
     }
   
}

void do_acceptdeath( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char player_file[MAX_INPUT_LENGTH];
    char filename[MAX_INPUT_LENGTH];
    char disguisefilename[MAX_INPUT_LENGTH];
    int col=0;
    int race=0;
    FILE * fp;


    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2);
    if ( arg[0] == '\0' )
    {
	send_to_char( "Syntax: acceptdeath <new name> [keep]\r\n", ch );
	send_to_char( "        This will allow you to complete the death cycle\r\n",ch);
	send_to_char( "        and get on with your life, under a new name.\r\n",ch);
	send_to_char( "\r\n        User the 'keep' option if you wish to keep your stats and spheres.\r\n",ch);
	send_to_char( "        If you do not use this, you will need to reroll\r\n",ch);
	return;
    }

    if (ch->in_room->vnum != ROOM_VNUM_DEATH) {
	send_to_char("You can only do this after you have been finished off!\r\n",ch);
	return;
    }
    if ( !check_parse_name( arg ) )
    {
        send_to_char( "Illegal name, try another.\n\r", ch );
    	return;
    }
    sprintf(filename,"%s%s", PLAYER_DIR, capitalize(arg));
    sprintf(disguisefilename,"%s%s", PLAYER_DISGUISE_DIR, capitalize(arg));
    if ( ( fp = fopen( filename, "r" ) ) != NULL ) {
	send_to_char("Name is already used. Try another\r\n",ch);
	fclose(fp);
	return;
    }
    if ( ( fp = fopen( disguisefilename, "r" ) ) != NULL ) {
	send_to_char("Name is already used. Try another\r\n",ch);
	fclose(fp);
	return;
    }
    sprintf(buf,"$N Accepts Death and is renamed to %s.",arg);
    wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
    
    // Make a note that accepted death
    sprintf(buf, "%s has accepted death and is renamed to %s.\n\r",
		  ch->name, capitalize(arg));
    sprintf(buf2, "I am a dead player! (%s)", ch->name);
    make_note("Newplayers", capitalize(arg), "Admin", buf2, 56, buf);
    
    REMOVE_BIT(ch->act,PLR_GRADUATED);
    ch->gold = 0;
    ch->silver = 0;
    ch->pcdata->gold_bank = 0;
    ch->pcdata->silver_bank = 0;

    extract_char(ch,TRUE,TRUE);
    if (IS_DISGUISED(ch)) {
	sprintf(player_file,"%s%s",PLAYER_DISGUISE_DIR, capitalize(ch->name));
    }
    else
    {
	sprintf(player_file,"%s%s",PLAYER_DIR, capitalize(ch->name));
    }
    if (arg2[0] != '\0') {
       reset_dead_char(ch, arg, TRUE);
    }
    else {
       reset_dead_char(ch, arg, FALSE);
    }
    save_char_obj(ch,FALSE);
    unlink(player_file);

    // Put in race list
    send_to_char("The following races are available:\n\n\r",ch);
    buf[0] = '\0';
    col=0;
    
    for ( race = 1; race_table[race].name != NULL; race++ ) {
	 if (!race_table[race].pc_race)
	   break;
	 else {
	   if (IS_FADE_GRANTED(ch) && !str_cmp(race_table[race].name, "Fade")) {
		sprintf( buf, "%-14s \t", capitalize(race_table[race].name));
		send_to_char(buf, ch);
		col++;
		if (col == 3) {
		  send_to_char("\n\r", ch);
		  col=0;
		}	      
	   }
	   else if (!race_table[race].granted) {
		sprintf( buf, "%-14s \t", capitalize(race_table[race].name));
		send_to_char(buf, ch);
		col++;
		if (col == 3) {
		  send_to_char("\n\r", ch);
		  col=0;
		}
	   }
	 }
    }
    send_to_char("\n\n\r", ch);
    
    send_to_char("Please enter your new Race: ",ch);    
    ch->desc->connected=CON_GET_NEW_RACE;

    return;
}

// Lets a non channie type try to dismount a mounted
// fighter
void do_overwhelm( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim=NULL;
  CHAR_DATA *mount=NULL;
  int chance=0;

  one_argument(argument,arg);
  
  if (MOUNTED(ch)) {
    send_to_char("You can't overwhelm while riding!\r\n", ch);
    return;
  }

  if (IS_AFFECTED(ch,AFF_FLYING)) {
    act("Your feet aren't on the ground.",ch,NULL,NULL,TO_CHAR);
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
    send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }
  
  if (ch->daze > 0) {
    if (IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)) {
	 
    }
    else {
	 send_to_char("You can't overwhelm while in a daze.\r\n", ch);
	 return;
    }
  }

  if ((chance = get_skill(ch, gsn_overwhelm)) == 0) {
    send_to_char("Huh?\r\n",ch);
    return;
  }
  
  if (IS_NULLSTR(arg)) {
    victim = ch->fighting;
    if (victim == NULL) {
	 send_to_char("But you aren't fighting anyone!\r\n",ch);
	 return;
    }
  }
  else if ((victim = get_char_room(ch,arg)) == NULL) {
    send_to_char("They aren't here.\r\n",ch);
    return;    
  }

  if (!MOUNTED(victim)) {
    send_to_char("They are not mounted!\n\r", ch);
    return;
  }

  if (is_safe(ch,victim))
    return;

  if (IS_NPC(victim) && victim->fighting != NULL &&  !is_same_group(ch,victim->fighting)) {
    send_to_char("Kill stealing is not permitted.\r\n",ch);
    return;
  }
  
  if (ch == victim) {
    send_to_char("Uh.. you are already dismounted...\n\r", ch);
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim) {
    act("$N is your beloved master.",ch,NULL,victim,TO_CHAR);
    return;
  }
  
  // ========== modifiers ===========
  // size
  // dex
  // skill - riding
  // race  - saldaean, shienar gets 10 bonus
  // level - horse vs ch
  
  // size
  if (ch->size < victim->size)
    chance += (ch->size - victim->size) * 2;
  
  // dex
  chance += get_curr_stat(ch,STAT_DEX);
  chance -= get_curr_stat(victim,STAT_DEX) * 3 / 2;

  // skill
  chance += get_skill(ch, gsn_riding)/10;
  chance -= get_skill(victim, gsn_riding)/5;

  // race
  if ((ch->race == race_lookup("saldaean"))  ||
	 (ch->race == race_lookup("shienaran")) ||
	 (ch->race == race_lookup("tairen")))
    chance += 20;
  
  if ((victim->race == race_lookup("saldaean"))  ||
	 (victim->race == race_lookup("shienaran")) ||
	 (victim->race == race_lookup("tairen")))
    chance -= 20;

  // horses level compared to ch's level
  mount = victim->mount;  
  chance += (ch->level - mount->level);

  
  // =============== the attack ================

  if (number_percent() < chance && number_percent() < 25 ) {
    act("$n overwhelms you and you are thrown off the mount!{x",ch,NULL,victim,TO_VICT);
    act("You overwhelm $N and $E is thrown off the mount!{x",ch,NULL,victim,TO_CHAR);
    act("$n overwhelms $N, sending $M flying off the mount!{x",ch,NULL,victim,TO_NOTVICT);
    check_improve(ch,gsn_overwhelm,TRUE,1);
    
    DAZE_STATE(victim,2 * PULSE_VIOLENCE);
    WAIT_STATE(ch,skill_table[gsn_overwhelm].beats);
    victim->position = POS_RESTING;
    
    // Throw off horse
    victim->riding = FALSE;
    mount->riding = FALSE;
    update_pos(victim);
    damage(ch,victim,number_range(2, 2 +  2 * victim->size),gsn_overwhelm, DAM_BASH,TRUE);

  }
  else {
    act("$n tries to overwhelms you, but charge into the mount and is knocked to the ground!{x",ch,NULL,victim,TO_VICT);
    act("You try to overwhelm $N, but charge into the mount and is knocked to the ground!{x",ch,NULL,victim,TO_CHAR);
    act("$n tries to overwhelms $N, but charge into the mount and is knocked to the ground!{x",ch,NULL,victim,TO_NOTVICT);
    
    check_improve(ch,gsn_overwhelm,TRUE,1);

    damage(ch,victim,0,gsn_overwhelm,DAM_BASH,TRUE);
    
    ch->position = POS_SITTING;
    DAZE_STATE(ch, 3 * PULSE_VIOLENCE);
    update_pos(ch);    
  }

  check_killer(ch,victim);

  return;
}

void do_submit(CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim;
	char arg[MAX_STRING_LENGTH];
        AFFECT_DATA af;

	act("$n submits $mself for punishment.\n\r",ch,NULL,NULL,TO_ROOM);
	act("You submit yourself for punishment.\n\r",ch,NULL,NULL,TO_CHAR);
	ch->surrender_timeout = current_time + 120;

    one_argument(argument,arg);
    if (arg[0] == '\0')
	return;
    if ((victim = get_char_room(ch,arg)) != NULL) {
    	if (victim->pcdata->learned[gsn_bind] > 0 && IS_SET(victim->auto_act, AUTO_BIND)) {
         	act ( "$N quickly bind you so you can't escape!", ch, NULL, victim, TO_CHAR );
         	act ( "$N quickly bind $n so $e can't escape!", ch, NULL, victim, TO_NOTVICT );
         	act ( "You quickly bind $n so $e can't escape!", ch, NULL, victim, TO_VICT );
         	af.where     = TO_AFFECTS;
         	af.casterId  = victim->id;
         	af.type      = gsn_bind;
         	af.level     = victim->level;
         	af.duration  = victim->pcdata->learned[gsn_bind]/4;
         	af.location  = APPLY_NONE;
         	af.modifier  = 0;
         	af.bitvector = AFF_BIND;
         	affect_to_char(ch,&af);
    	}
    	if (IS_SET(victim->auto_act, AUTO_BLINDFOLD))
    	{
        	act ( "$n blindfolds $N!", victim, NULL, ch, TO_NOTVICT );
        	act ( "You blindfold $N!", ch, NULL, victim, TO_VICT );
        	act ( "$n blindfolds you!", ch, NULL, victim, TO_CHAR );
        	SET_BIT(ch->affected_by, AFF_BLINDFOLDED);
    	}
   }

}

// Binds a character if in the right "stance"
void do_blindfold( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim=NULL;

  if (IS_NPC(ch))
    return;

  if (IS_NULLSTR(argument)) {
    send_to_char("Who do you want to blindfold?\n\r", ch);
    return;
  }

  if ((victim = get_char_room(ch,argument)) == NULL) {
    send_to_char("They aren't here.\r\n",ch);
    return;
  }

  if ((victim->position > POS_SLEEPING && current_time > victim->surrender_timeout) && (victim != ch) ) {
    send_to_char("They are not subdued yet!\n\r", ch);
    return;
  }

  if (IS_AFFECTED(victim, AFF_BLINDFOLDED)) {
    send_to_char("They are already blindfolded!\n\r", ch);
    return;
  }

  act ( "$n blindfolds $N!", ch, NULL, victim, TO_NOTVICT );
  act ( "You blindfold $N!", ch, NULL, victim, TO_CHAR );
  act ( "$n blindfolds you!", ch, NULL, victim, TO_VICT );
  SET_BIT(victim->affected_by, AFF_BLINDFOLDED);
  return;
}

void do_unblindfold( CHAR_DATA *ch, char *argument)
{
	CHAR_DATA *victim = NULL;
        if (IS_NPC(ch))
	   return;

	if (IS_AFFECTED(ch, AFF_WRAPPED) || IS_AFFECTED(ch, AFF_BIND))
	{
		send_to_char("You can't do that in your current situation.\r\n",ch);
		return;
	}

        if (IS_NULLSTR(argument))
	{
		send_to_char("Who do you want to unblindfold?\n\r", ch);
		return;
	}

 	if ((victim = get_char_room(ch,argument)) == NULL) {
    		send_to_char("They aren't here.\r\n",ch);
    		return;
  	}

  	if (!IS_AFFECTED(victim, AFF_BLINDFOLDED)) {
    		send_to_char("They are not blindfolded!\n\r", ch);
    		return;
  	}

	if (victim != ch && !can_see(ch,victim))
	{
		send_to_char("They aren't here.\r\n",ch);
		return;
	}

	act ( "$n moves over to $N and unblindfolds $M!", ch, NULL, victim, TO_NOTVICT );
	act ( "You move over to $N and unblindfold $M!", ch, NULL, victim, TO_CHAR );
	act ( "$n move over to you and unblindfolds you!", ch, NULL, victim, TO_VICT );
  	REMOVE_BIT(victim->affected_by, AFF_BLINDFOLDED);
  return;

}
void do_bind( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim=NULL;
  AFFECT_DATA af;

  if (IS_NPC(ch))
    return;

  if (ch->pcdata->learned[gsn_bind] < 1) {
    send_to_char("You don't even know how to bind yet.\n\r", ch);
    return;
  }

  if (IS_NULLSTR(argument)) {
    send_to_char("Who do you want to bind?\n\r", ch);
    return;
  }

  if ((victim = get_char_room(ch,argument)) == NULL) {
    send_to_char("They aren't here.\r\n",ch);
    return;
  }

  if (victim == ch) {
    send_to_char("You can't bind your self.\n\r", ch);
    return;
  }

  if (IS_NPC(victim)) {
    send_to_char("You can't bind mobiles.\n\r", ch);
    return;
  }
  
  if (victim->position > POS_SLEEPING && current_time > ch->surrender_timeout) { // && !IS_AFFECTED(ch, AFF_SLEEP) && !IS_AFFECTED(ch,AFF_SAP)) {
    send_to_char("They are not subdued yet!\n\r", ch);
    return;
  }

  if (IS_AFFECTED(victim, AFF_BIND)) {
    send_to_char("They are already tied up!\n\r", ch);
    return;
  }

  if (number_percent() > ch->pcdata->learned[gsn_bind]) {
    act ( "You try to bind $N but are unable to get a solid knot on it!",  ch, NULL, victim, TO_CHAR );
    act ( "$n fumble with ropes trying to bind $N, but fails misserable!", ch, NULL, victim, TO_ROOM );
    check_improve(ch,gsn_bind,FALSE,1);
    return;
  }
  else {
    act ( "$n quickly move over to $N and bind $M!", ch, NULL, victim, TO_ROOM );
    act ( "You quickly move over to $N and bind $M!", ch, NULL, victim, TO_CHAR );
    if (number_percent() > 60)
      check_improve(ch,gsn_bind,TRUE,1);
    
    af.where	  = TO_AFFECTS;
    af.casterId    = ch->id;
    af.type 	  = gsn_bind;
    af.level 	  = ch->level;
    af.duration    = ch->level/4;
    af.location    = APPLY_NONE;
    af.modifier    = 0;
    af.bitvector   = AFF_BIND;
    affect_to_char(victim,&af);
  }
  
  return;
}

void do_free( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim=NULL;
  AFFECT_DATA *paf=NULL;
  bool found = FALSE;
  int ch_bind=0;
  int owner_bind=0;

  if (IS_NPC(ch))
    return;

  if (IS_NULLSTR(argument)) {
    send_to_char("Who do you want to try to free?\n\r", ch);
    return;
  }
  
  if ((victim = get_char_room(ch,argument)) == NULL) {
    send_to_char("They aren't here.\r\n",ch);
    return;
  }

  if (victim == ch) {
    send_to_char("You can't free your self. Use escape to try to get loose if tied up.\n\r", ch);
    return;
  }

  if (!IS_AFFECTED(victim, AFF_BIND)) {
    send_to_char("They aren't tied up.\n\r", ch);
    return;
  }

  for (paf = victim->affected; paf != NULL; paf = paf->next) {
    if (paf->bitvector == AFF_BIND) {

	 found = TRUE;

	 // Owner of bind
	 if (paf->casterId == ch->id) {
	   if (number_percent() < ch->pcdata->learned[gsn_bind]) {
		act ( "$n quickly moves over to $N and free the ropes that bind $M!", ch, NULL, victim, TO_NOTVICT );
		act ( "You quickly move over to $N and free the ropes that bind $M!", ch, NULL, victim, TO_CHAR );
		act ( "$n quickly moves over to you and free the ropes that bind you!", ch, NULL, victim, TO_VICT );
		
		affect_remove(victim, paf);
	   }
	   else {
		act ( "You try to remove the ropes that bind $N, but you can't get em loose!",  ch, NULL, victim, TO_CHAR );
		act ( "$n fumble with the ropes that bind $N, but it acctual looks like it gets more tight!", ch, NULL, victim, TO_NOTVICT );
		act ( "$n fumble with the ropes that binds you, but it acctual looks like it gets more tight!", ch, NULL, victim, TO_VICT );
		return;
	   }
	 }
	 
	 // Not owner
	 else {
	   if (number_percent() < ch->pcdata->learned[gsn_bind]) {
		ch_bind = get_curr_stat(ch, STAT_STR) + get_curr_stat(ch, STAT_DEX) + get_curr_stat(ch, STAT_INT) + (get_level(ch)/10);
		paf->caster = get_charId_world(ch, paf->casterId);

		if (paf->caster == NULL)
		  owner_bind = 0;
		else
		  owner_bind = get_curr_stat(paf->caster, STAT_STR) + get_curr_stat(paf->caster, STAT_DEX) + get_curr_stat(paf->caster, STAT_INT) + (get_level(paf->caster)/10);

		if ((ch_bind+number_range(0,25)) > owner_bind) {
		  act ( "$n quickly moves over to $N and free the ropes that bind $M!", ch, NULL, victim, TO_NOTVICT );
		  act ( "You quickly move over to $N and free the ropes that bind $M!", ch, NULL, victim, TO_CHAR );
		  act ( "$n quickly moves over to you and free the ropes that bind you!", ch, NULL, victim, TO_VICT );
		  
		  affect_remove(victim, paf);

		  return;
		}
		else {
		  act ( "You try to remove the ropes that bind $N, but you can't get em loose!",  ch, NULL, victim, TO_CHAR );
		  act ( "$n fumble with the ropes that bind $N, but it acctual looks like it gets more tight!", ch, NULL, victim, TO_NOTVICT );
		  act ( "$n fumble with the ropes that binds you, but it acctual looks like it gets more tight!", ch, NULL, victim, TO_VICT );
		  return;
		}		
	   }
	   else {
		act ( "You try to remove the ropes that bind $N, but you can't get em loose!",  ch, NULL, victim, TO_CHAR );
		act ( "$n fumble with the ropes that bind $N, but it acctual looks like it gets more tight!", ch, NULL, victim, TO_ROOM );
		act ( "$n fumble with the ropes that binds you, but it acctual looks like it gets more tight!", ch, NULL, victim, TO_VICT );
		return;
	   }
	 }
    }
  }

  if (!found) {
    send_to_char("You find no ropes to free them from.\n\r", ch);
    return;
  }
  
}

void do_checkaff( CHAR_DATA *ch, char *argument )
{
  char buf[MSL];
  CHAR_DATA *victim=NULL;
  CHAR_DATA *victim_next=NULL;

  for (victim = char_list; victim != NULL; victim = victim_next) {
    victim_next = victim->next;

    if (!IS_NPC(victim))
	 continue;

    if (IS_AFFECTED(victim, AFF_BIND)) {
	 sprintf(buf, "Mob vnum=%d has BIND affect!\n\r", victim->pIndexData->vnum);
	 send_to_char(buf, ch);
    }
	 
  }

}

// Escape
void do_escape( CHAR_DATA *ch, char *argument )
{
  AFFECT_DATA *paf=NULL;
  int ch_bind=0;
  int owner_bind=0;

  if (IS_NPC(ch))
    return;
  
  if (!IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are not tied up!\n\r", ch);
    return;
  }

  for (paf = ch->affected; paf != NULL; paf = paf->next) {
    if (paf->bitvector == AFF_BIND) {
	 if (number_percent() < ch->pcdata->learned[gsn_escape]) {
	   ch_bind = get_curr_stat(ch, STAT_STR) + get_curr_stat(ch, STAT_DEX) + get_curr_stat(ch, STAT_INT) + (get_level(ch)/10) + ch->pcdata->learned[gsn_escape];
	   paf->caster = get_charId_world(ch, paf->casterId);
	   
	   if (paf->caster == NULL)
		owner_bind = 0;
	   else
		owner_bind = get_curr_stat(paf->caster, STAT_STR) + get_curr_stat(paf->caster, STAT_DEX) + get_curr_stat(paf->caster, STAT_INT) + (get_level(paf->caster)/10) + ch->pcdata->learned[gsn_bind];
	   
	   if ((ch_bind+number_range(0,25)) > owner_bind) {
		act ( "$n quickly manage to escape from the ropes that binds $m", ch, NULL, NULL, TO_ROOM );
		act ( "You escape from the ropes that binds you!", ch, NULL, NULL, TO_CHAR );
		if (number_percent() > 60)
		   check_improve(ch,gsn_escape,TRUE,1);
		
		affect_remove(ch, paf);

		return;
	   }
	   else {
		act ( "You try to escape from the ropes that binds you, but you are unable to move!",  ch, NULL, NULL, TO_CHAR );
		act ( "$n tries to get loose of the ropes that binds $m!",  ch, NULL, NULL, TO_ROOM );
		
    		check_improve(ch,gsn_escape,FALSE,1);
		return;
	   }
	 }
	 else {
	   act ( "You try to escape from the ropes that binds you, but you are unable to move!",  ch, NULL, NULL, TO_CHAR );
	   act ( "$n tries to get loose of the ropes that binds $m!",  ch, NULL, NULL, TO_ROOM );
    	   check_improve(ch,gsn_escape,FALSE,1);
	   
	   return;
	 }
    }
  }
  
}

bool    check_valid_pkill args( (CHAR_DATA *ch, CHAR_DATA *victim) )
{
	if (!IS_PKILLER(ch) || !IS_PKILLER(victim))
		return FALSE;
	if (ch->level - 7  > victim->level)
		return FALSE;
	if (IS_SET(victim->in_room->room_flags, ROOM_ARENA))
   		return FALSE;

    	ch->next_pkill = current_time + 1800; // 30 minutes 
	long exp_exchange = 4000; //(victim->level + victim->pcdata->extended_level) * 10000;
	
	gain_exp(ch,exp_exchange);
	gain_exp(victim,-exp_exchange);
	
	ch->pk_count++;
	victim->pk_died_count++;

	pkupdate(ch);
	pkupdate(victim);
    	send_to_char("You have been PKILLED!\r\n",victim);

	//For now, remove corpse generation from pkilling
        /*
    	make_corpse(victim);
    	stop_follower(victim);
    	char_from_room( victim );
    	char_to_room( victim, get_room_index( ROOM_VNUM_RECOVERY));
	*/
    	save_char_obj( ch, FALSE );
	return TRUE;
}

int get_skill_difference(CHAR_DATA *ch, CHAR_DATA *victim, int gsn)
{
	int playerSkill = get_skill(ch,gsn);
	int victimSkill = get_skill(victim,gsn);

	int difference = playerSkill - victimSkill;

	return difference / 3;


}

CHAR_DATA * get_target_group_member(CHAR_DATA *ch, CHAR_DATA * victim) {

    CHAR_DATA *gch;
    CHAR_DATA *lch;
    int xp;
    int members;
    int group_levels;

    if ( victim == ch )
           return ch;

    members = 0;
    group_levels = 0;
    
    //First, find out how many group members of the victim are in the room
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group( gch, victim ) )
        {
           members++;
        }
    }

    if (members == 0) {
	return victim;
    }

    //pick a random group member to target
    int target = number_range(1,members);
    int count = 0;


    //find the target member of the group
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( is_same_group( gch, victim ) )
        {
           count++;
	   if (count == target) {
		return gch;
	   }
        }
    }

    //if for some reason it fails, return the victim
    return victim;

}
