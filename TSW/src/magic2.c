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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "magic.h"

extern void	make_corpse	args( ( CHAR_DATA *ch ) );
extern char *target_name;

void release_one_sustained_weave(CHAR_DATA *ch, int sn);
int tie_duration(CHAR_DATA *ch, int tying_sn, int weave_sn);

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_seize( CHAR_DATA *ch, char *argument )
{
  AFFECT_DATA af;
  char arg[MAX_INPUT_LENGTH];
  int endurance  = 0;
  int sn         = 0;
  long chanpower = 0;

  one_argument(argument,arg);

/*
  if (IS_NPC(ch) && ch->desc == NULL)
    return;
*/
  
  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("You cannot channel the true source.\n\r", ch);
    return;
  }

  if (!IS_NPC(ch)) {
    if (ch->pcdata->true_sex != SEX_MALE) {
	 send_to_char("Seizing? What is that?\n\r",ch);
	 return;
    }
  }
  else {
    if (ch->sex != SEX_MALE) {
	 send_to_char("Seizing? What is that?\n\r",ch);
	 return;
    }
  }
  
  sn        = skill_lookup("seize");
  endurance = skill_table[sn].min_endurance;
  
  if (!IS_NPC(ch) && ch->pcdata->learned[sn] < 1) {
    send_to_char("You don't know how to channel the true source.\n\r",ch);
    return;
  }

  /* If room has no channeling flag */
  if (IS_SET(ch->in_room->room_flags,ROOM_STEDDING)) {
    send_to_char("It is as if the tranquilty and peace of this place prevent you from reaching the True Source.\n\r", ch);
    return;
  }
  if( is_wearing_foxhead(ch)) {
	 send_to_char( "You reach for the True Source and feel something stopping you.\n\r", ch );
	 return;
    }
  
  if( IS_AFFECTED( ch, AFF_SHIELDED ) ) {
    if (!break_shield(ch, arg)) {
	 send_to_char( "You reach for the True Source and feel something stopping you.\n\r", ch );
	 return;
    }
  }
  
  /* If player is stilled... */
  if (!IS_NPC(ch) && IS_SET(ch->act, PLR_STILLED)) {
    send_to_char("You yearn for the Power but all that is there is the Void.\n\r",ch);
    return;
  }
  
  if (get_curr_op(ch) <= 0) {
    send_to_char("You yearn for the Power but all that is there is the Void.\n\r",ch);
    return;
  }
  
  if (arg[0] == '\0') {
    if (ch->autoholding > 0) {
    	chanpower = get_curr_op(ch)/((double)100/ch->autoholding);
    }
    else {
       chanpower = get_curr_op(ch) / 2;
    }
  }
  else if (!(chanpower = atoi(arg))) {
    send_to_char("Syntax : seize <number>\n\r",ch);
    return;
  }

/*
	OBJ_DATA *weapon;
	weapon = get_eq_char(ch, WEAR_SECOND_WIELD);
	if (weapon != NULL)
	{
 		if (weapon->pIndexData->vnum == OBJ_VNUM_CALLANDOR) 
		{
			ch->insanity_points++;
			handle_mc_insanity(ch);
		}
		else
		{
			weapon = get_eq_char(ch,WEAR_WIELD);
                	if (weapon != NULL)
                	{
                        	if (weapon->pIndexData->vnum == OBJ_VNUM_CALLANDOR)
                        	{
                                	ch->insanity_points++;
                                	handle_mc_insanity(ch);
                        	}
                	}

		}
	}
	else
	{
        	weapon = get_eq_char(ch,WEAR_WIELD);
		if (weapon != NULL)
		{
 			if (weapon->pIndexData->vnum == OBJ_VNUM_CALLANDOR) 
			{
				ch->insanity_points++;
				handle_mc_insanity(ch);
			}
		}
	}
*/

  if (chanpower > 0) {
    if ((ch->holding + chanpower) > get_curr_op(ch)) {
	 if ( (ch->holding + chanpower) <= (get_curr_op(ch) + ((get_curr_op(ch)*10)/100))) {
	   send_to_char("You draw more of the Power than your ability and start to feel a {rburning pain{x.\n\r", ch);
	   ch->holding   += chanpower;
	   ch->endurance -= endurance;
	   SET_BIT(ch->affected_by, AFF_CHANNELING);

	   af.where	    = TO_AFFECTS;
	   af.casterId     = ch->id;
	   af.type         = sn;
	   af.level	    = ch->level;
	   af.duration     = 2;
	   af.location     = APPLY_HIT;
	   af.modifier     = -(ch->max_hit / 20);
	   af.bitvector    = 0;
	   affect_to_char( ch, &af );

	   af.where	    = TO_AFFECTS;
	   af.casterId     = ch->id;
	   af.type         = sn;
	   af.level	    = ch->level;
	   af.duration     = 2;
	   af.location     = APPLY_ENDURANCE;
	   af.modifier     = -(ch->max_endurance / 40);
	   af.bitvector    = 0;
	   affect_to_char( ch, &af );
	   
	   /* Tell other if in room or area */
           send_chpower_to_channelers(ch, sn);

	   return;
	 }
	 else {
	   send_to_char("You draw too much of the Power and fall over screaming in pain.\n\r",ch);

	   if (IS_AFFECTED(ch,AFF_CHANNELING))
		do_function(ch, &do_unchannel, "" );

	   if (number_chance((ch->holding + chanpower) - ((get_curr_op(ch) + (get_curr_op(ch)*10)/100)))) {
		send_to_char("{rYou feel the strain as you wield more than you can safely handle.{x\n\r", ch);
		gain_burnout(ch, dice(1,3));
	   }
	   
	   act("$n falls to the ground clutching at his head screaming in pain.",
		  ch,NULL,NULL,TO_ROOM);
	   
	   return;
	 }
    }
  }
  else {
    /* release ? message ?*/
    return;
  }
  
  if (IS_AFFECTED(ch,AFF_CHANNELING)) {
    if (ch->endurance < endurance) {
	 send_to_char("You don't have enough endurance.\n\r", ch);
	 return;
    }
    send_to_char("You draw upon more of the Power filling yourself with more of the Fire of Saidin.\n\r",ch);
    ch->holding   += chanpower;
    ch->endurance -= endurance;
    send_chpower_to_channelers(ch, sn);
    return;
  }
  else {
    if (!IS_NPC(ch) && number_percent() > ch->pcdata->learned[sn]) {
	 send_to_char("You try and seize Saidin but it seems to slide through\n\r",ch);
	 send_to_char("your fingers like water, leaving an oily nauseating residue.\n\r",ch);
	 check_improve(ch, gsn_seize, FALSE, 1);
	 return;
    }
    else {
	 if (ch->endurance < endurance) {
	   send_to_char("You don't have enough endurance.\n\r", ch);
	   return;
	 }
	 send_to_char("You reach out to Saidin and fill yourself with the Power.\n\r", ch);
    }
  }

  /* Adjust char info */
  ch->holding   += chanpower;
  ch->endurance -= endurance;
  SET_BIT(ch->affected_by, AFF_CHANNELING);

  /* How long can you safely hold OP until you can no longer manage ? */
  if (!IS_NPC(ch)) {
    if (ch->channeling_pulse <= 0) {
	 ch->channeling_pulse = ch->pcdata->learned[sn]/4;
    }
  }

  if (!IS_NPC(ch))
    check_improve(ch, gsn_seize, TRUE, 1);

  /* Tell other if in room or area */
  if (!IS_SET(ch->act2,PLR2_MASKCHAN))
  	send_chpower_to_channelers(ch, sn);
  
  /* Done, lets return */
  return;
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_embrace(CHAR_DATA *ch, char *argument)
{
  AFFECT_DATA af;
  CHAR_DATA *victim;
  char arg[MAX_INPUT_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  int endurance=0;
  int sn=0;
  long chanpower=0;
   
  one_argument(argument,arg);

/*
  if (IS_NPC(ch) && ch->desc == NULL)
    return;
*/

  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("You cannot channel the true source.\n\r", ch);
    return;
  }

  if (!IS_NPC(ch)) {
    if (ch->pcdata->true_sex != SEX_FEMALE) {
	 send_to_char("Embracing? What is that?\n\r",ch);
	 return;
    }
  }
  else {
    if (ch->sex != SEX_FEMALE) {
	 send_to_char("Embracing? What is that?\n\r",ch);
	 return;
    }
  }
   
  sn        = skill_lookup("embrace");
  endurance = skill_table[sn].min_endurance;
  
   if (!IS_NPC(ch) && ch->pcdata->learned[sn] < 1) {
	send_to_char("You don't know how to channel the true source.\n\r",ch);
	return;
   }

   /* If room has no channeling flag */
   if (IS_SET(ch->in_room->room_flags,ROOM_STEDDING)) {
	send_to_char("It is as if the tranquilty and peace of this place prevent you from reaching the True Source.\n\r", ch);
	return;
   }
   
  if( is_wearing_foxhead(ch)) {
	  send_to_char( "You reach for the True Source and feel something stopping you.\n\r", ch );
	  return;
   }
   if( IS_AFFECTED( ch, AFF_SHIELDED ) ) {
	if (!break_shield(ch, arg)) {
	  send_to_char( "You reach for the True Source and feel something stopping you.\n\r", ch );
	  return;
	}
   }
   
   /* If player is stilled... */
   if (!IS_NPC(ch) && IS_SET(ch->act, PLR_STILLED)) {
	send_to_char("You yearn for the Power but all that is there is the Void.\n\r",ch);
	return;
   }

   if (arg[0] == '\0') {
      if (ch->autoholding > 0) {
    	chanpower = get_curr_op(ch)/((double)100/ch->autoholding);
    }
    else {
	chanpower = get_curr_op(ch) / 2;
    }
   }
   else if (!(chanpower = atoi(arg))) {
	send_to_char("Syntax : embrace <number>\n\r",ch);
	return;
   }
   
   if (chanpower > 0) {
	if ((ch->holding + chanpower) > get_curr_op(ch)) {
	  if ( (ch->holding + chanpower) <= (get_curr_op(ch) + ((get_curr_op(ch)*10)/100))) {
	    send_to_char("You draw more of the Power than you can safely handle and start to feel a {rburning pain{x.\n\r", ch);
	    ch->holding   += chanpower;
	    ch->endurance -= endurance;
	    SET_BIT(ch->affected_by, AFF_CHANNELING);

	    af.where	     = TO_AFFECTS;
	    af.casterId     = ch->id;
	    af.type         = sn;
	    af.level	     = ch->level;
	    af.duration     = 2;
	    af.location     = APPLY_HIT;
	    af.modifier     = -(ch->max_hit / 20);
	    af.bitvector    = 0;
	    affect_to_char( ch, &af );
	    
	    af.where	     = TO_AFFECTS;
	    af.casterId     = ch->id;
	    af.type         = sn;
	    af.level	     = ch->level;
	    af.duration     = 2;
	    af.location     = APPLY_ENDURANCE;
	    af.modifier     = -(ch->max_endurance / 40);
	    af.bitvector    = 0;
	    affect_to_char( ch, &af );

            /* Tell other if in room or area */
            send_chpower_to_channelers(ch, sn);

	    return;
	  }
	  else {
	    send_to_char("You draw too much of the Power and fall over screaming in pain.\n\r",ch);

	    if (IS_AFFECTED(ch,AFF_CHANNELING))
		 do_function(ch, &do_unchannel, "" );
	    
	    if (number_chance((ch->holding + chanpower) - ((get_curr_op(ch) + (get_curr_op(ch)*10)/100)))) {
		 send_to_char("{rYou feel the strain as you wield more than you can safely handle.{x\n\r", ch);
		 gain_burnout(ch, dice(1,3));
	    }
	    
	    act("$n falls to the ground clutching at her head screaming in pain.",
		   ch,NULL,NULL,TO_ROOM);
	    
	    return;
	  }
	}
   }
   else {
	/* release ? message ?*/
	return;
   }

   if (IS_AFFECTED(ch,AFF_CHANNELING)) {
	if (ch->endurance < endurance) {
	  send_to_char("You don't have enough endurance.\n\r", ch);
	  return;
	}
	send_to_char("You draw upon more of the Power filling yourself with more of the Life of Saidar.\n\r",ch);
	ch->holding   += chanpower;
	ch->endurance -= endurance;
	send_chpower_to_channelers(ch, sn);
	return;
   }
   else {
	if (!IS_NPC(ch) && number_percent() > ch->pcdata->learned[sn]) {
	  send_to_char("You try and embrace Saidar but you can't embrace it.\n\r",ch);
	  check_improve(ch, gsn_embrace, FALSE, 1);
	  return;
	}
	else {
	  if (ch->endurance < endurance) {
	    send_to_char( "You don't have enough endurance.\n\r", ch );
	    return;
	  }
	  
	  send_to_char("You gently embrace Saidar and fill yourself with the Power.\n\r", ch);
	  
	  for ( victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room ) {
	    if (victim == ch)
		 continue;
	    if ((victim->sex == ch->sex) && (victim->class == CLASS_CHANNELER) && IS_SET(victim->chan_flags, CHAN_SEECHANNELING) && !IS_SET(ch->act2,PLR2_MASKCHAN)) {
		 sprintf(buffer, "A glow surrounds %s as she embraces Saidar.\n\r", PERS(ch, victim));
		 act(buffer, ch, NULL, victim, TO_VICT);
		 //send_to_char(buffer, victim);
	    }
	  }
	}
   }

   /* Adjust char info */
   ch->holding   += chanpower;
   ch->endurance -= endurance;
   SET_BIT(ch->affected_by, AFF_CHANNELING);

   /* How long can you safely hold OP until you can no longer manage ? */
   if (!IS_NPC(ch)) {
	if (ch->channeling_pulse <= 0) {
	  ch->channeling_pulse = ch->pcdata->learned[sn]/4;
	}
   }

   if (!IS_NPC(ch))
	check_improve(ch, gsn_embrace, TRUE, 1);

   /* Tell other if in room or area */
   send_chpower_to_channelers(ch, sn);
   
   return;
}

/**********************************************************************
*       Function      : do_autochannel
*       Author        : Swordfish
*       Description   : Set in percent how much of your OP you want to
*                       seize/embrace automatic
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_autochannel(CHAR_DATA *ch, char *argument)
{
   int percent=0;
   char buf[128];

  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
   
   if (IS_NULLSTR(argument) || !(percent = atoi(argument))) {
      if (argument[0] == '0')
        percent = 0;
      else {
         send_to_char("Syntax: autochannel <0-100%>\n\r", ch);
         return;
      }
   }
   
   if (percent < 0 || percent > 100) {
      send_to_char("Amount to auto channel must be between 0 to 100%.\n\r", ch);
      return;	
   }
   
   ch->autoholding = percent;
   if (percent == 0) {
      send_to_char("Auto channeling amount set to default.\n\r", ch);	
   }
   else {
      sprintf(buf, "Auto channeling amount set to %d%%.\n\r", percent);
      send_to_char(buf, ch);
   }
   
   return;
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_channel(CHAR_DATA *ch, char *argument)
{
  int sex;

  if (!IS_NPC(ch))
    sex = ch->pcdata->true_sex;
  else
    sex = ch->sex;
  
  if (sex == SEX_MALE)
    do_function(ch, &do_seize, argument);
  else if (sex == SEX_FEMALE)
    do_function(ch, &do_embrace, argument);
  
  return;
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_release(CHAR_DATA *ch, char *argument)
{
  long chanpower=0;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  int sn=0;

  one_argument(argument,arg);

  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  if (!IS_NPC(ch) && ch->pcdata->learned[skill_lookup(ch->sex == SEX_MALE ? "seize" : "embrace")] < 1) {
    send_to_char("You don't know how to channel the true source.\n\r",ch);
    return;
  }

  if (IS_SET(ch->act,PLR_STILLED)) {
    send_to_char("You can't release the Power...you can't even possess it!\n\r",ch);
    return;
  }

  if (!IS_AFFECTED(ch,AFF_CHANNELING)) {
    if (ch->sex == SEX_MALE) {
	 send_to_char("You are not channeling Saidin.\n\r", ch);
	 return;
    }
    else if (ch->sex == SEX_FEMALE) {
	 send_to_char("You are not channeling Saidar.\n\r", ch);
	 return;
    }
    else {
	 send_to_char("You must be with the Power to release it.\n\r",ch);
	 return;
    }
  }

  if (arg[0] == '\0') {
    do_function(ch, &do_unchannel, "" );
    return;
  }
  else {
    if (!(chanpower = atoi(arg))) {

	 // Release a sustained weave
	 if ((sn = skill_lookup( arg ) ) < 0 ) {
	   send_to_char("Syntax : release [number]\n\r",ch);
	   send_to_char("         release <sustained weave>\n\r", ch);
	   return;
	 }
	 else {
	   release_one_sustained_weave(ch, sn);
	   return;
	 }
    }
    
    if (chanpower > 0) {
	 if (ch->holding < chanpower) {
	   send_to_char("You are not holding that much of the One Power.\n\r",ch);
	   return;
	 }
	 else if (ch->holding == chanpower) {
	   do_function(ch, &do_unchannel, "" );
	   return;
	 }
	 
	 send_to_char("You release some of the Power you are holding.\n\r",ch);
	 ch->holding -= chanpower;

         if (!IS_SET(ch->act2,PLR2_MASKCHAN)) {
	    sprintf(buf,"You feel some of the Power leave $n.");
	    if (ch->sex == SEX_MALE)
	      send_to_malchan(buf,ch);
	    else
	      send_to_femalchan(buf,ch);
         }
    }
  }
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_unchannel(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *victim=NULL;
  char buffer[MAX_STRING_LENGTH];
  
/*
  if (IS_NPC(ch) && ch->desc == NULL)
    return;
*/

  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (!IS_NPC(ch) && ch->pcdata->learned[skill_lookup(ch->sex == SEX_MALE ? "seize" : "embrace")] < 1) {
    send_to_char("You don't know how to channel the true source.\n\r",ch);
    return;
  }

  if (IS_AFFECTED(ch,AFF_CHANNELING) && ch->bIsLinked) {
	send_to_char("You are part of a link.. You must be unlinked first.\n\r",ch);
        return;
  }
  
  if (IS_AFFECTED(ch,AFF_CHANNELING)) {
    if (ch->sex == SEX_MALE) {
	 send_to_char("You release Saidin and feel as if Life itself has left you.\n\r", ch);
    }   
    if (ch->sex == SEX_FEMALE) {
	 send_to_char("You release Saidar and feel as if Life itself has left you.\n\r", ch);
	 if (ch->in_room) {
	   for ( victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room ) {
	     if (victim == ch)
		  continue;
             if (victim->sex != ch->sex)
                  continue;
	     if ((victim->sex == ch->sex) && (victim->class == CLASS_CHANNELER) && IS_SET(victim->chan_flags, CHAN_SEECHANNELING) && !IS_SET(ch->act2,PLR2_MASKCHAN)) {
                sprintf(buffer, "The glow around %s slowly fades.\n\r", PERS(ch, victim));
		act(buffer, ch, NULL, victim, TO_VICT);		
	     }
	   }
	}
    }
    REMOVE_BIT(ch->affected_by, AFF_CHANNELING);
    ch->holding = 0;

    /* Sustained weaves are released upon unchannel */
    release_sustained_weaves(ch);
  }
  else {
    if (ch->sex == SEX_MALE) {
	 send_to_char("You are not channeling Saidin.\n\r", ch);
	 return;
    }   
    if (ch->sex == SEX_FEMALE) {
	 send_to_char("You are not channeling Saidar.\n\r", ch);
	 return;
    }
  }
}

void spell_farsight( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    if (IS_AFFECTED(ch,AFF_BLIND))
    {
        send_to_char("Maybe it would help if you could see?\n\r",ch);
        return;
    }
 
    do_function(ch, &do_scan, target_name);
}


void spell_portal( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;

        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   !can_see_room(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_FORSAKEN(victim)
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_NONE) ) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }   

    stone = get_eq_char(ch,WEAR_HOLD);
    if (!IS_IMMORTAL(ch) 
    &&  (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
	send_to_char("You lack the proper component for this spell.\n\r",ch);
	return;
    }

    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
     	act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR);
     	act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR);
     	extract_obj(stone);
    }

    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->timer = 2 + level / 25; 
    portal->value[3] = victim->in_room->vnum;

    obj_to_room(portal,ch->in_room);
    /* obj_to_room(portal,victim->in_room); */

    act("$p rises up from the ground.",ch,portal,victim,TO_ROOM);
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR);
}

void spell_nexus( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;
    ROOM_INDEX_DATA *to_room, *from_room;

    from_room = ch->in_room;
 
        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   (to_room = victim->in_room) == NULL
    ||   !can_see_room(ch,to_room) || !can_see_room(ch,from_room)
    ||   IS_SET(to_room->room_flags, ROOM_SAFE)
    ||	 IS_SET(from_room->room_flags,ROOM_SAFE)
    ||   IS_SET(to_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(to_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(to_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(from_room->room_flags,ROOM_NO_RECALL)
    ||   victim->level >= level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_NONE) ) 
    ||	 (is_clan(victim) && !is_same_clan(ch,victim)))
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }   
 
    stone = get_eq_char(ch,WEAR_HOLD);
    if (!IS_IMMORTAL(ch)
    &&  (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
        send_to_char("You lack the proper component for this spell.\n\r",ch);
        return;
    }
 
    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR);
        act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR);
        extract_obj(stone);
    }

    /* portal one */ 
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->timer = 1 + level / 10;
    portal->value[3] = to_room->vnum;
 
    obj_to_room(portal,from_room);
 
    act("$p rises up from the ground.",ch,portal,NULL,TO_ROOM);
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR);

    /* no second portal if rooms are the same */
    if (to_room == from_room)
	return;

    /* portal two */
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->timer = 1 + level/10;
    portal->value[3] = from_room->vnum;

    obj_to_room(portal,to_room);

    if (to_room->people != NULL)
    {
	act("$p rises up from the ground.",to_room->people,portal,NULL,TO_ROOM);
	act("$p rises up from the ground.",to_room->people,portal,NULL,TO_CHAR);
    }
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_balefire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
  CHAR_DATA *victim;
  DESCRIPTOR_DATA *d;
  OBJ_DATA *obj;
  int iWear;

  if ((victim = get_char_world(ch, target_name)) == NULL
	 || victim == ch
	 || victim->in_room == NULL
	 || !can_see_room(ch, victim->in_room)) {
    return;
  }
  
  /* FOR NPC */
  if (IS_NPC(victim)) {
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ ) {
	 if (( obj = get_eq_char(victim, iWear)) == NULL)
	   continue;
	 act("$n balefires $N's $t out of the pattern!", ch, obj->short_descr, victim, TO_NOTVICT);
	 act("You balefire $N's $t out of the pattern!", ch, obj->short_descr, victim, TO_CHAR);
    }
    act("$n balefires $N out of the pattern!",ch,NULL,victim,TO_NOTVICT);
    act("You balefire $N out of the pattern!",ch,NULL,victim,TO_CHAR);
    death_cry( victim );
    extract_char( victim, TRUE, FALSE );
    return;
  }
  else {
    
    /* FOR PC */
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ ) {
	 if (( obj = get_eq_char(victim, iWear)) == NULL)
	   continue;
	 act("$n balefires $N's $t out of the pattern!", ch, obj->short_descr, victim, TO_NOTVICT);
	 act("$n balefires your $t out of the pattern!", ch, obj->short_descr, victim, TO_VICT);
	 act("You balefire $N's $t out of the pattern!", ch, obj->short_descr, victim, TO_CHAR);
	obj_from_char(obj);
	extract_obj(obj);
    }
    
    act("$n balefires $N out of the pattern!",ch,NULL,victim,TO_NOTVICT);
    act("$n balefires you out of the pattern!",ch,NULL,victim,TO_VICT);
    act("You balefire $N out of the pattern!",ch,NULL,victim,TO_CHAR);

    stop_fighting( victim, TRUE );
    save_char_obj( victim, FALSE );
    //For immortals casting balefire, let them recover
    if (IS_IMMORTAL(ch)) {
    	if (!IS_NPC(victim)) {
		do_echo(ch,"{WFLASH!!!!!!  Someone just got burned from the pattern.\r\n{x");
	 	for ( d = descriptor_list; d != NULL; d = d->next ) {
	   		if ( d == victim->desc ) {
				close_socket( d );
				return;
	   		}
	 	}
    	}
    }
    else 
    {
    	make_corpse(victim);
	extract_obj(victim->in_room->contents);  //get rid of the corpse
    	stop_follower(victim);
    	char_from_room( victim );
    	char_to_room( victim, get_room_index( ROOM_VNUM_DEATH));
    	do_restore(victim,victim->name);
    	save_char_obj( victim, FALSE );
	do_echo(ch,"{WFLASH!!!!!!  Someone just got burned from the pattern.\r\n{x");
    }
  }
  return;
}

/* Compare two skills by name */
int compare_skill_names(const void *v1, const void *v2)
{
   return strcmp((*(struct skill_type*)v1).name, (*(struct skill_type*)v2).name);
}

/**********************************************************************
*       Function      : do_cost
*       Author        : Swordfish
*       Description   : Show costs for the various waves
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_cost(CHAR_DATA *ch, char *argument)
{
  BUFFER *buffer;
  /* char arg[MAX_INPUT_LENGTH]; */
  int sn, level, min_lev = 1, max_lev = LEVEL_HERO;
  bool found = FALSE;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct skill_type local_skill_table[MAX_SKILL];
  int i=0;
  int local_sn = 0;

  if (IS_NPC(ch))
    return;

  if (!IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("You cannot use the One Power.\n\r", ch);
    return;
  }

  /* Copy skill_table into a local skill table for sort */
  for (sn = 0; sn < MAX_SKILL; sn++)
   local_skill_table[sn] = skill_table[sn];

  /* Quick sort local skill table */
  qsort (local_skill_table, sn, sizeof(struct skill_type), compare_skill_names);
  
  buffer = new_buf();
  
  for (sn = 0; sn < MAX_SKILL; sn++) {
    if (local_skill_table[sn].name == NULL )
	   continue;
	   local_sn = find_spell(ch, local_skill_table[sn].name);
    if ((level = local_skill_table[sn].skill_level[ch->class]) < LEVEL_HERO + 1
	   &&  (level <= ch->level)
	   &&  level >= min_lev && level <= max_lev
	   &&  local_skill_table[sn].spell_fun != spell_null
	   &&  ch->pcdata->learned[local_sn] > 0) {
	 found = TRUE;
	 
	 level = local_skill_table[sn].skill_level[ch->class];

	 if (ch->level < level)
	   continue;
	 else {
	   sprintf(buf,"%-18s ", local_skill_table[sn].name);

	   for (i = 0; i < MAX_SPHERE; i++) {
		sprintf(buf2, "%3d %s%c{x, ", local_skill_table[sn].spheres[i] ,
		i == SPHERE_AIR ? "{W" : i == SPHERE_EARTH ? "{y" : i == SPHERE_FIRE ? "{R" : i == SPHERE_SPIRIT ? "{Y" : i == SPHERE_WATER ? "{B" : "{x",
		UPPER(sphere_table[i].name[0]));
		strcat(buf, buf2);
	   }
	   sprintf(buf2, "%5d {gEnd{x.\n\r", local_skill_table[sn].min_endurance);
	   strcat(buf, buf2);
	   add_buf(buffer, buf);
	 }
    }
  }
  
  /* return results */
  
  if (!found) {
    send_to_char("No weaves found.\n\r",ch);
    return;
  }
  
  page_to_char(buf_string(buffer),ch);
  free_buf(buffer);
}

/**********************************************************************
*       Function      : do_cost
*       Author        : Swordfish
*       Description   : Show costs for the various waves
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_cost_OLD(CHAR_DATA *ch, char *argument)
{
  BUFFER *buffer;
  /* char arg[MAX_INPUT_LENGTH]; */
  int sn, level, min_lev = 1, max_lev = LEVEL_HERO;
  bool found = FALSE;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int i=0;

  if (IS_NPC(ch))
    return;

  if (!IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("You cannot use the One Power.\n\r", ch);
    return;
  }
  
  buffer = new_buf();
  
  for (sn = 0; sn < MAX_SKILL; sn++) {
    if (skill_table[sn].name == NULL )
	 break;
    if ((level = skill_table[sn].skill_level[ch->class]) < LEVEL_HERO + 1
	   &&  (level <= ch->level)
	   &&  level >= min_lev && level <= max_lev
	   &&  skill_table[sn].spell_fun != spell_null
	   &&  ch->pcdata->learned[sn] > 0) {
	 found = TRUE;
	 
	 level = skill_table[sn].skill_level[ch->class];

	 if (ch->level < level)
	   continue;
	 else {
	   sprintf(buf,"%-18s ", skill_table[sn].name);

	   for (i = 0; i < MAX_SPHERE; i++) {
		sprintf(buf2, "%3d %c, ", skill_table[sn].spheres[i] ,UPPER(sphere_table[i].name[0]));
		strcat(buf, buf2);
	   }
	   sprintf(buf2, "%5d End.\n\r", skill_table[sn].min_endurance);
	   strcat(buf, buf2);
	   add_buf(buffer, buf);
	 }
    }
  }
  
  /* return results */
  
  if (!found) {
    send_to_char("No weaves found.\n\r",ch);
    return;
  }
  
  page_to_char(buf_string(buffer),ch);
  free_buf(buffer);
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : Release one weaves you are sustaining. 
*                       
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void release_one_sustained_weave(CHAR_DATA *ch, int sn)
{
  char buf[MSL];
  AFFECT_DATA *paf;
  AFFECT_DATA *paf_next;
  CHAR_DATA *vch;

    /* Ward stuff */
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  WARD_DATA *pWard;
  WARD_DATA *pWard_next;
  unsigned int vnum;  

  /* Room weaves*/
  AFFECT_DATA *pWeave;
  AFFECT_DATA *pWeave_next;
  
  /* Blade of Flame handeling */
  int sn_bof     = skill_lookup("blade of flame");
  OBJ_DATA *bof;

  // Normal affects
  for (vch = char_list; vch != NULL; vch = vch = vch->next ) {
    for ( paf = vch->affected; paf != NULL; paf = paf_next ) {
	 paf_next	= paf->next;
	 if (vch->affected == NULL)
	   continue;
	 if (paf->casterId != ch->id) {
	   continue;
	 }
	 if (paf->type != sn) {
	   continue;
	 }
	 else {
	   if (paf->duration == SUSTAIN_WEAVE) {
		if ( paf->type > 0 && !IS_NULLSTR(skill_table[paf->type].msg_off) ) {
		  send_to_char( skill_table[paf->type].msg_off, vch );
		  send_to_char( "\n\r", vch );
		}
		
		if (paf->type == sn_bof) {
		  bof = get_eq_char(ch, WEAR_WIELD);
		  extract_obj( bof );
		}
		
		/* If wall of fire, extract objects */
		if (paf->type == gsn_wof) {
		  extract_wof(ch);
		}
		
		/* If wall of air, extract objects */
		if (paf->type == gsn_woa) {
		  extract_woa(ch);
		}

		sprintf(buf, "You release the flows sustaining the '%s' weave.\n\r", skill_table[paf->type].name);
		send_to_char(buf, ch);
		
		affect_remove( vch, paf );
		return;
	   }
	 }
    }
  }

    // Check wards
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->wards_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWard = pRoom->wards; pWard != NULL; pWard = pWard_next) {
		pWard_next = pWard->next;
	      if (pWard->casterId == ch->id && pWard->duration == SUSTAIN_WEAVE && pWard->sn == sn) {
		   
		   if ( pWard->sn > 0 && !IS_NULLSTR(skill_table[pWard->sn].msg_off)) {
			send_to_char( skill_table[pWard->sn].msg_off, ch );
			send_to_char( "\n\r", ch );
		   }

		   sprintf(buf, "You release the flows sustaining the '%s' weave.\n\r", skill_table[pWard->sn].name);
		   send_to_char(buf, ch);
		   
		   ward_remove(pRoom, pWard);	   
		   return;
		 }
	   }
	 }
    }
  }

  // Check room weaves
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->weave_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWeave = pRoom->weaves; pWeave != NULL; pWeave = pWeave_next) {
		pWeave_next = pWeave->next;
		if (pWeave->casterId == ch->id && pWeave->duration == SUSTAIN_WEAVE && pWeave->type == sn) {
		  
		  if ( pWeave->type > 0 && !IS_NULLSTR(skill_table[pWeave->type].msg_off)) {
		    send_to_char( skill_table[pWeave->type].msg_off, ch );
		    send_to_char( "\n\r", ch );
		  }
		  
		  sprintf(buf, "You release the flows sustaining the '%s' weave.\n\r", skill_table[pWeave->type].name);
		  send_to_char(buf, ch);
		  room_weave_remove(pRoom, pWeave);	  
		  return;
		}
	   }
	 }
    }
  }

  // If we get here, no weaves was found to remove
  send_to_char("You sustain no such weave.\n\r", ch);
  return;
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : Release all weaves you are sustaining. 
*                       Used when unchannel or shielded from OP etc.
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void release_sustained_weaves(CHAR_DATA *ch)
{
  AFFECT_DATA *paf;
  AFFECT_DATA *paf_next;
  CHAR_DATA *vch;

  /* Ward stuff */
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  WARD_DATA *pWard;
  WARD_DATA *pWard_next;
  unsigned int vnum;  

  /* Room weaves*/
  AFFECT_DATA *pWeave;
  AFFECT_DATA *pWeave_next;
  
  /* Blade of Flame handeling */
  int sn_bof     = skill_lookup("blade of flame");
  OBJ_DATA *bof;


  // Normal affects
  for (vch = char_list; vch != NULL; vch = vch = vch->next ) {
    for ( paf = vch->affected; paf != NULL; paf = paf_next ) {
	 paf_next	= paf->next;
	 if (vch->affected == NULL)
	   continue;
	 if (paf->casterId != ch->id) {
	   continue;
	 }
	 else {
	   if (paf->duration == SUSTAIN_WEAVE) {
		if ( paf->type > 0 && !IS_NULLSTR(skill_table[paf->type].msg_off) ) {
		  send_to_char( skill_table[paf->type].msg_off, vch );
		  send_to_char( "\n\r", vch );
		}

		if (paf->type == sn_bof) {
		  bof = get_eq_char(ch, WEAR_WIELD);
		  extract_obj( bof );
		}

		/* If wall of fire, extract objects */
		if (paf->type == gsn_wof) {
		  extract_wof(ch);
		}
		
		/* If wall of air, extract objects */
		if (paf->type == gsn_woa) {
		  extract_woa(ch);
		}

		affect_remove( vch, paf );
	   }
	 }
    }
  }
  
  // Check wards
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->wards_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWard = pRoom->wards; pWard != NULL; pWard = pWard_next) {
	      pWard_next = pWard->next;
	      if (pWard->casterId == ch->id && pWard->duration == SUSTAIN_WEAVE) {

		   if ( pWard->sn > 0 && !IS_NULLSTR(skill_table[pWard->sn].msg_off)) {
			send_to_char( skill_table[pWard->sn].msg_off, ch );
			send_to_char( "\n\r", ch );
		   }
		   
	         ward_remove(pRoom, pWard);	   
		 }
	   }
	 }
    }
  }

  // Check room weaves
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->weave_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWeave = pRoom->weaves; pWeave != NULL; pWeave = pWeave_next) {
		pWeave_next = pWeave->next;
		if (pWeave->casterId == ch->id && pWeave->duration == SUSTAIN_WEAVE) {
		  
		  if ( pWeave->type > 0 && !IS_NULLSTR(skill_table[pWeave->type].msg_off)) {
		    send_to_char( skill_table[pWeave->type].msg_off, ch );
		    send_to_char( "\n\r", ch );
		  }
		  
		  room_weave_remove(pRoom, pWeave);	   
		}
	   }
	 }
    }
  }
  
}

/**********************************************************************
*       Function      : remove_sustained_weaves
*       Author        : Swordfish
*       Description   : Removes all sustained weaves on a character
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void remove_sustained_weaves(CHAR_DATA *ch)
{
  AFFECT_DATA *paf;
  AFFECT_DATA *paf_next;
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  WARD_DATA *pWard;
  WARD_DATA *pWard_next;
  unsigned int vnum;  
  AFFECT_DATA *pWeave;
  AFFECT_DATA *pWeave_next;
  CHAR_DATA *caster=NULL;
  
  for (paf = ch->affected; paf != NULL; paf = paf_next) {
    paf_next = paf->next;
    
    if (ch->affected == NULL)
	 continue;
    if (paf->duration == SUSTAIN_WEAVE) {

	 // Here is the deal. PPl are logging out to get sustained weaves off their back
	 // To try to handle this, we auto_tie sustained weaves when quitting.
	 paf->caster = get_charId_world(ch, paf->casterId);	 
	 caster = paf->caster;
	 
	 if (caster != NULL && caster != ch) {
	   paf->duration = tie_duration(caster, gsn_tying, paf->type);
	   if (paf->duration <= 4)
		paf->duration = 4;
	   paf->tied_strength = caster->holding + get_skill(caster, paf->type);	   
	   if (!IS_NPC(caster))
		paf->tied_strength += caster->pcdata->extended_level;
	 }
	 else {
	   affect_remove( ch, paf );
	 }
    }
  }

  // Wards
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->wards_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWard = pRoom->wards; pWard != NULL; pWard = pWard_next) {
		pWard_next = pWard->next;
		if (pWard->casterId == ch->id && pWard->duration == SUSTAIN_WEAVE) {
		  ward_remove(pRoom, pWard);	   
		}
	   }
	 }
    }
  }
  
  // Check room weaves
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->weave_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWeave = pRoom->weaves; pWeave != NULL; pWeave = pWeave_next) {
		pWeave_next = pWeave->next;
		if (pWeave->casterId == ch->id && pWeave->duration == SUSTAIN_WEAVE) {
		  room_weave_remove(pRoom, pWeave);	   
		}
	   }
	 }
    }
  }  
  
}

/**********************************************************************
*       Function      : update_sustained_weaves
*       Author        : Swordfish
*       Description   : Update endur cost of sustained weaves
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void update_sustained_weaves(CHAR_DATA *ch)
{
  AFFECT_DATA *paf=NULL;
  AFFECT_DATA *paf_next;
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  WARD_DATA *pWard;
  WARD_DATA *pWard_next;
  unsigned int vnum;  
  AFFECT_DATA *pWeave;
  AFFECT_DATA *pWeave_next;
  CHAR_DATA   *vch;
  char buf[MAX_STRING_LENGTH];
  int endurance  = 0;
  
  for (vch = char_list; vch != NULL; vch = vch = vch->next ) {
    for ( paf = vch->affected; paf != NULL; paf = paf_next ) {
	 paf_next	= paf->next;
	 if (vch->affected == NULL)
	   continue;
	 if (paf->casterId != ch->id)
	   continue;
	 if (paf->duration != SUSTAIN_WEAVE)
	   continue;
	 endurance = skill_table[paf->type].min_endurance;
	 if (ch->endurance < endurance) {
	   send_to_char("The cost for sustaining your weaves becomes more than you can handle.\n\r", ch);
	   do_function(ch, &do_unchannel, "" );
	   return;
	 }
	 else {
	   sprintf(buf, "Updating sustained weave '%s' on (%s) with -%d endurance for (%s).", skill_table[paf->type].name,
			 vch->name, endurance, ch->name);
	   wiznet(buf,NULL,NULL,WIZ_CHANNELING,0,0);
	   ch->endurance -= endurance;
	 }
    }
  }

  // Wards
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->wards_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWard = pRoom->wards; pWard != NULL; pWard = pWard_next) {
		pWard_next = pWard->next;
		if (pWard->casterId == ch->id && pWard->duration == SUSTAIN_WEAVE) {
		  endurance = skill_table[pWard->sn].min_endurance;
		  
		  if (ch->endurance < endurance) {
		    send_to_char("The cost for sustaining your weaves becomes more than you can handle.\n\r", ch);
		    do_function(ch, &do_unchannel, "" );
		    return;
		  }
		  else {
		    sprintf(buf, "Updating sustained ward '%s' on (room) with -%d endurance for (%s).", skill_table[pWard->sn].name, endurance, ch->name);
		    wiznet(buf,NULL,NULL,WIZ_CHANNELING,0,0);
		    ch->endurance -= endurance;
		  }
		}
	   }
	 }
    }
  }
  
  // Check room weaves
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->weave_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWeave = pRoom->weaves; pWeave != NULL; pWeave = pWeave_next) {
		pWeave_next = pWeave->next;
		if (pWeave->casterId == ch->id && pWeave->duration == SUSTAIN_WEAVE) {
		  endurance = skill_table[pWeave->type].min_endurance;
		  
		  if (ch->endurance < endurance) {
		    send_to_char("The cost for sustaining your weaves becomes more than you can handle.\n\r", ch);
		    do_function(ch, &do_unchannel, "" );
		    return;
		  }
		  else {
		    sprintf(buf, "Updating sustained room weave '%s' on (room) with -%d endurance for (%s).", skill_table[pWeave->type].name, endurance, ch->name);
		    wiznet(buf,NULL,NULL,WIZ_CHANNELING,0,0);
		    ch->endurance -= endurance;
		  }
		}
	   }
	 }
    }
  }

  
  if (get_curr_flows(ch) > MAX_FLOWS) {
    send_to_char("{rYou are unable to handle all the sustained flows and lose control!{x\n\r", ch);     	
    release_sustained_weaves(ch);
    sprintf(buf, "%s sustained more than %d flows - lost control of all", ch->name, MAX_FLOWS);
    wiznet(buf,NULL,NULL,WIZ_CHANNELING,0,0);
  }
}

/**********************************************************************
*       Function      : tie_duration
*       Author        : Swordfish
*       Description   : Calculate a tie duration for a weave
*       Parameters    : 
*       Returns       : 
**********************************************************************/
int tie_duration(CHAR_DATA *ch, int tying_sn, int weave_sn)
{
  double calc=0;
  int duration=0;
  int i=0;
  
  for (i = 0; i < MAX_SPHERE; i++) {
    if (skill_table[weave_sn].spheres[i] > 0) {
	 calc += ((ch->perm_sphere[i] - skill_table[weave_sn].spheres[i]) / skill_table[weave_sn].spheres[i]);
    }
  }
  
  /* Max from strenght is 10 */
  if (calc > 10)
    calc = 10;

  /* Add bonus for level */
  calc += ((double)ch->level / 10);

  /* Add bonus for skill */
  if (!IS_NPC(ch))
    calc += ((double)ch->pcdata->learned[tying_sn] / 10);
  
  duration = calc;
  return duration;
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_tie (CHAR_DATA *ch,  char *argument)
{
  AFFECT_DATA *paf=NULL;
  AFFECT_DATA *paf_next;
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  WARD_DATA *pWard;
  WARD_DATA *pWard_next;
  AFFECT_DATA *pWeave;
  AFFECT_DATA *pWeave_next;  
  unsigned int vnum;
  CHAR_DATA   *vch;
  CHAR_DATA   *vch_next;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  int endurance  = 0;
  int sn         = 0;
  bool found     = FALSE;
  
  argument = one_argument(argument, arg);

/*
  if (IS_NPC(ch) && ch->desc == NULL)
    return;
*/

  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("You cannot channel the true source.\n\r", ch);
    return;
  }
  
  sn        = skill_lookup("tying");
  endurance = skill_table[sn].min_endurance;

  if (!IS_NPC(ch) && ch->pcdata->learned[sn] < 1) {
    send_to_char("You don't know how to tie a weave.\n\r",ch);
    return;
  }

  if (arg[0] == '\0') {
    send_to_char("Tie what ?\n\r", ch);
    return;
  }
  
  /* Need to hold OP to tie a weave */
  if (!IS_AFFECTED(ch, AFF_CHANNELING)) {
    do_function(ch, &do_channel, "");
    if (!IS_AFFECTED(ch ,AFF_CHANNELING))
	 return;
  }

  /* SELF */
  if (argument[0] == '\0') {

    /* tie all */
    if (!str_cmp(arg, "all")) {
	 for (vch = char_list; vch != NULL; vch = vch_next) {
	   vch_next = vch->next;
	   for ( paf = vch->affected; paf != NULL; paf = paf_next ) {
		paf_next = paf->next;
		if (vch->affected == NULL)
		  continue;
		if (paf->casterId != ch->id)
		  continue;
		if (paf->duration != SUSTAIN_WEAVE)
		  continue;
		if (paf->type == skill_lookup("link"))
		  continue;
		
		found = TRUE;
		paf->duration = tie_duration(ch, sn, paf->type);
		paf->tied_strength = ch->holding + get_skill(ch, paf->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
		
		sprintf(buffer, "You examine the flows sustaining the '%s' weave on %s and successfully tie a knot on it.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
		send_to_char(buffer, ch);		
	   }
	 }


	 for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
	   if (pArea->wards_cnt <= 0)
		continue;
	   for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
		if ( ( pRoom = get_room_index(vnum) ) ) {
		  for (pWard = pRoom->wards; pWard != NULL; pWard = pWard_next) {
		    pWard_next = pWard->next;
		    if (pWard->casterId != ch->id)
			 continue;
		    if (pWard->duration != SUSTAIN_WEAVE)
			 continue;
		    
		    found = TRUE;
		    pWard->duration = tie_duration(ch, sn, pWard->sn);
		    pWard->tied_strength = ch->holding + get_skill(ch, pWard->sn) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
		    
		    sprintf(buffer, "You examine the flows sustaining the '%s' ward and successfully tie a knot on it.\n\r", skill_table[pWard->sn].name);
		    send_to_char(buffer, ch);		                
		  }
		}
	   }
	 }
	 
	 for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
	   if (pArea->weave_cnt <= 0)
		continue;
	   for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
		if ( ( pRoom = get_room_index(vnum) ) ) {
		  for (pWeave = pRoom->weaves; pWeave != NULL; pWeave = pWeave_next) {
		    pWeave_next = pWeave->next;
		    if (pWeave->casterId != ch->id)
			 continue;
		    if (pWeave->duration != SUSTAIN_WEAVE)
			 continue;
		    
		    found = TRUE;
		    pWeave->duration = tie_duration(ch, sn, pWeave->type);
		    pWeave->tied_strength = ch->holding + get_skill(ch, pWeave->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
		    
		    sprintf(buffer, "You examine the flows sustaining the '%s' room weave and successfully tie a knot on it.\n\r", skill_table[pWeave->type].name);
		    send_to_char(buffer, ch);		                
		  }
		}
	   }
	 }	 
	 
	 if (!found) {
	   send_to_char("You sustain no weaves on your self or others.\n\r", ch);
	 }
	 else {
	   if (!IS_NPC(ch))
		check_improve(ch, sn, TRUE, 1);
	 }
	 return;
    }
    
    /* tie spesific weave */
    else {
	 for ( paf = ch->affected; paf != NULL; paf = paf->next ) {
	   if (str_prefix(arg, skill_table[paf->type].name)) {
		continue;
	   }
	   else {
		if (paf->duration != SUSTAIN_WEAVE)
		  continue;		
                if (paf->type == skill_lookup("link"))
                  continue;
	        paf->duration = tie_duration(ch, sn, paf->type);
		paf->tied_strength = ch->holding + get_skill(ch, paf->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;

		sprintf(buffer, "You examine the flows sustaining the '%s' weave and successfully tie a knot on it.\n\r",
			   skill_table[paf->type].name);
		send_to_char(buffer, ch);
		if (!IS_NPC(ch))
	          check_improve(ch, sn, TRUE, 1);
		return;
	   }
	 }
	 send_to_char("You sustain no weave with that name on your self.\n\r", ch);
	 return;
    }
  }
  
  /* WARDS */
  else if (!str_cmp(argument, "ward")) {
  	
    /* tie all */
    if (!str_cmp(arg, "all")) {
       for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
         if (pArea->wards_cnt <= 0)
            continue;
         for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
     	  if ( ( pRoom = get_room_index(vnum) ) ) {
     	   for (pWard = pRoom->wards; pWard != NULL; pWard = pWard_next) {
     		pWard_next = pWard->next;
                if (pWard->casterId != ch->id)
                   continue;
                if (pWard->duration != SUSTAIN_WEAVE)
                   continue;
                
                found = TRUE;
                pWard->duration = tie_duration(ch, sn, pWard->sn);
                pWard->tied_strength = ch->holding + get_skill(ch, pWard->sn) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
                
		sprintf(buffer, "You examine the flows sustaining the '%s' ward and successfully tie a knot on it.\n\r", skill_table[pWard->sn].name);
		send_to_char(buffer, ch);		                
     	   }
     	  }
         }
       }
       if (!found) {
          send_to_char("You sustain no wards.\n\r", ch);
       }
       return;              
     }    	
     
    /* tie spesific ward */
    else {
    	 for (pWard = ch->in_room->wards; pWard != NULL; pWard = pWard_next) {
    	    pWard_next = pWard->next;
    	    
    	    if (str_prefix(arg, skill_table[pWard->sn].name))
    	       continue;
	   else {
		if (pWard->casterId != ch->id)
		  continue;	   	
		if (pWard->duration != SUSTAIN_WEAVE)
		  continue;
		  		
	        pWard->duration = tie_duration(ch, sn, pWard->sn);
		pWard->tied_strength = ch->holding + get_skill(ch, pWard->sn) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;

		sprintf(buffer, "You examine the flows sustaining the '%s' ward and successfully tie a knot on it.\n\r",
			   skill_table[pWard->sn].name);
		send_to_char(buffer, ch);
		
		return;
	   }
	 }
	 send_to_char("You sustain no ward with that name in this room.\n\r", ch);
	return;
    }  	  	  	
  }

  /* TARGET */
  else {
    
    /* Find target if present in room */
    argument = one_argument(argument, arg2);
    if ((vch = get_char_room( ch, arg2 )) == NULL) {
	 send_to_char( "They aren't here.\n\r", ch );
	 return;
    }
    
    /* tie all weaves you sustain on target */
    if (!str_cmp(arg, "all")) {
	 for ( paf = vch->affected; paf != NULL; paf = paf->next ) {
           if (paf->type == skill_lookup("link"))
               continue;
	   if (paf->duration != SUSTAIN_WEAVE) {
		continue;
	   }
	   else if (paf->casterId != ch->id) {
		continue;
	   }
	   else {
		found = TRUE;
		if (IS_FORSAKEN(ch)) 
                {
                   if (argument[0] != '\0')
                   {
  		      argument = one_argument(argument, arg3);
                      if (!strcmp(arg3,"long"))
                      {
		         paf->duration = (tie_duration(ch, sn, paf->type) * 100);
                      }
		      else
                      {
		         paf->duration = tie_duration(ch, sn, paf->type);
                      }
                   }
		   else
                   {
		      paf->duration = tie_duration(ch, sn, paf->type);
                   }
                }
                else
                {
		   paf->duration = tie_duration(ch, sn, paf->type);
                }
		paf->tied_strength = ch->holding + get_skill(ch, paf->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
		sprintf(buffer, "You examine the flows sustaining the '%s' weave and successfully tie a knot on it.\n\r",
			   skill_table[paf->type].name);
		send_to_char(buffer, ch);
	   }
	 }
	 if (!found) {
	   sprintf(buffer, "You sustain no weaves on %s\n\r", PERS(vch, ch));
	   send_to_char(buffer, ch);
	 }
	 else {
	    if (!IS_NPC(ch))
	       check_improve(ch, sn, TRUE, 1);
	 }
	 return;
    }
    
    /* tie spesific weave you sustain on target */
    else {
	 for ( paf = vch->affected; paf != NULL; paf = paf->next ) {
           if (paf->type == skill_lookup("link"))
                continue;
	   if (str_prefix(arg, skill_table[paf->type].name)) {
		continue;
	   }
	   else if (paf->casterId != ch->id) {
		continue;
	   }
	   else {
		if (paf->duration != SUSTAIN_WEAVE)
		  continue;
		if (IS_FORSAKEN(ch)) 
                {
                   if (argument[0] != '\0')
                   {
  		      argument = one_argument(argument, arg3);
                      if (!strcmp(arg3,"long"))
                      {
		         paf->duration = (tie_duration(ch, sn, paf->type) * 100);
                      }
		      else
                      {
		         paf->duration = tie_duration(ch, sn, paf->type);
                      }
                   }
		   else
                   {
		      paf->duration = tie_duration(ch, sn, paf->type);
                   }
                }
                else
                {
		   paf->duration = tie_duration(ch, sn, paf->type);
                }
		paf->tied_strength = ch->holding + get_skill(ch, paf->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
		sprintf(buffer, "You examine the flows sustaining the '%s' weave and successfully tie a knot on it.\n\r",
			   skill_table[paf->type].name);
		send_to_char(buffer, ch);
		if (!IS_NPC(ch))
	          check_improve(ch, sn, TRUE, 1);
		return;
	   }
	 }
	 sprintf(buffer, "You sustain no weave with that name on %s\n\r", PERS(vch, ch));
	 send_to_char(buffer, ch);
	 return;
    }
  }
}

/* Extract all Walls of air that is set by ch */
void extract_woa(CHAR_DATA *ch) 
{
  CHAR_DATA  *victim=NULL;
  OBJ_DATA   *obj;
  OBJ_DATA   *obj_next;
  EXIT_DATA  *pexit;
  
  for ( obj = object_list; obj != NULL; obj = obj_next ) {
    obj_next = obj->next;
    if (obj->item_type == ITEM_AIRWALL && obj->value[0] == ch->id) {
	 
	 /* Remove the exit flag*/
	 if ((pexit = obj->in_room->exit[obj->value[1]]) != NULL) {
	 if (IS_SET(pexit->exit_info, EX_AIRWALL))
	   REMOVE_BIT(pexit->exit_info, EX_AIRWALL);    	
	 
	 /* tell about it if ppl in room */    	
	 victim = obj->in_room->people;
	 if (victim != NULL) {
	   act("The {Cblue haze{x blocking the $T entrance suddenly vanishes.", victim, NULL, dir_name[obj->value[1]], TO_CHAR);
	   act("The {Cblue haze{x blocking the $T entrance suddenly vanishes.", victim, NULL, dir_name[obj->value[1]], TO_ROOM);
	 }
	 extract_obj(obj);
      }
    }
  }
}

/* Extract all Walls of fire that is set by ch*/
void extract_wof(CHAR_DATA *ch) 
{
  CHAR_DATA  *victim=NULL;
  OBJ_DATA   *obj;
  OBJ_DATA   *obj_next;
  EXIT_DATA  *pexit=NULL;
 
  for ( obj = object_list; obj != NULL; obj = obj_next ) {
    obj_next = obj->next;
    if (obj->item_type == ITEM_FIREWALL && obj->value[0] == ch->id) {
    	
    	/* Remove the exit flag*/
    	if ((pexit = obj->in_room->exit[obj->value[1]]) != NULL) {
        if (IS_SET(pexit->exit_info, EX_FIREWALL))
	   REMOVE_BIT(pexit->exit_info, EX_FIREWALL);    	

        /* tell about it if ppl in room */    	
	 victim = obj->in_room->people;
	 if (victim != NULL) {
	   act("$p slowly dies out, leaving a {Dcharred mark{x on the ground.", victim, obj, NULL, TO_CHAR);
	   act("$p slowly dies out, leaving a {Dcharred mark{x on the ground.", victim, obj, NULL, TO_ROOM);
	 }
	 extract_obj(obj);
      }
    }
  }
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_untie (CHAR_DATA *ch,  char *argument)
{
  AFFECT_DATA *paf;
  AFFECT_DATA *paf_next;
  WARD_DATA *pWard;
  WARD_DATA *pWard_next;
  AFFECT_DATA *pWeave;
  AFFECT_DATA *pWeave_next;  
  CHAR_DATA   *vch;
  char arg[MAX_INPUT_LENGTH];
  char buffer[MAX_STRING_LENGTH];
  int endurance  = 0;
  int sn         = 0;
  int channie_strength=0;

  /* Blade of Flame handeling */
  OBJ_DATA *bof;

  bool found     = FALSE;

  argument = one_argument(argument, arg);

  if (IS_NPC(ch) && ch->desc == NULL)
    return;

  if (!IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("You cannot channel the true source.\n\r", ch);
    return;
  }

  sn        = skill_lookup("tying");
  endurance = skill_table[sn].min_endurance;
  
  if (ch->pcdata->learned[sn] < 1) {
    send_to_char("You don't know how to untie a weave.\n\r",ch);
    return;
  }

  if (arg[0] == '\0') {
    send_to_char("Untie what ?\n\r", ch);
    return;
  }

  /* Need to hold OP to untie a weave */
  if (!IS_AFFECTED(ch, AFF_CHANNELING)) {
    do_function(ch, &do_channel, "");
    if (!IS_AFFECTED(ch ,AFF_CHANNELING))
	 return;
  }

  /* SELF */
  if (argument[0] == '\0') {

    /* untie all */
    if (!str_cmp(arg, "all")) {
	 for ( paf = ch->affected; paf != NULL; paf = paf_next ) {
	   paf_next = paf->next;
	   
	   if (paf->duration < 0) {
		continue;
	   }
           if (paf->inverted && paf->casterId != ch->id && !IS_IMMORTAL(ch))
           {
		continue;
	   }
	   
	   if (paf->type == gsn_poison)
	      continue;
	   
	   if (skill_table[paf->type].spell_fun == spell_null)
		continue;
	   
	   else {
		found = TRUE;
		channie_strength =  ch->holding + get_skill(ch, paf->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
		
		if ((channie_strength > paf->tied_strength + 20) && (paf->casterId != ch->id)) {
		  sprintf(buffer, "You examine the knot on the '%s' weave but fails to untie it.\n\r",
				skill_table[paf->type].name);
		  send_to_char(buffer, ch);
		}
		else {
		  sprintf(buffer, "You examine the knot on the '%s' weave and successfully untie it.\n\r",
				skill_table[paf->type].name);
		  send_to_char(buffer, ch);
		  
		  if ( paf->type > 0 && !IS_NULLSTR(skill_table[paf->type].msg_off )) {
		    send_to_char( skill_table[paf->type].msg_off, ch );
		    send_to_char( "\n\r", ch );
		  }
		  
		  if (paf->type == gsn_bof) {
		    bof = get_eq_char(ch, WEAR_WIELD);
		    extract_obj( bof );
		  }
		  
		  /* If wall of fire, extract objects */
		  if (paf->type == gsn_wof) {
		    extract_wof(ch);
		  }
		  
		  /* If wall of air, extract objects */
		  if (paf->type == gsn_woa) {
		    extract_woa(ch);
		  }
		  
		  affect_remove( ch, paf );
		}
	   }
	 }
	 if (!found) {
	   send_to_char("You find no weaves that is tied on your self.\n\r", ch);
	 }
	 return;
    }
    
    /* tie spesific weave */
    else {
	 for ( paf = ch->affected; paf != NULL; paf = paf->next ) {
	   if (str_prefix(arg, skill_table[paf->type].name)) {
		continue;
	   }
	   else {

		if (paf->duration < 0)
		  continue;
		  
                if (paf->inverted && paf->casterId != ch->id && !IS_IMMORTAL(ch))
                {
		     continue;
	        }

                if (paf->type == gsn_poison)
	          continue;		  

		if (skill_table[paf->type].spell_fun == spell_null)
		  continue;

		channie_strength =  ch->holding + get_skill(ch, paf->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
		if ((channie_strength > paf->tied_strength + 20) && (paf->casterId != ch->id)) {
		  sprintf(buffer, "You examine the knot on the '%s' weave but fails to untie it.\n\r",
				skill_table[paf->type].name);
		  send_to_char(buffer, ch);
		  return;
		}
		else {
		  sprintf(buffer, "You examine the knot on the '%s' weave and successfully untie it.\n\r",
				skill_table[paf->type].name);
		  send_to_char(buffer, ch);
		  
		  if ( paf->type > 0 && !IS_NULLSTR(skill_table[paf->type].msg_off)) {
		    send_to_char( skill_table[paf->type].msg_off, ch );
		    send_to_char( "\n\r", ch );
		  }

		  if (paf->type == gsn_bof) {
		    bof = get_eq_char(ch, WEAR_WIELD);
		    extract_obj( bof );
		  }

		  /* If wall of fire, extract objects */
		  if (paf->type == gsn_wof) {
		    extract_wof(ch);
		  }
		  
		  /* If wall of air, extract objects */
		  if (paf->type == gsn_woa) {
		    extract_woa(ch);
		  }
		  
		  affect_remove( ch, paf );
		  return;
		}
	   }
	 }
	 send_to_char("You find no weave with that name that is tied on your self.\n\r", ch);
	 return;
    }
  }
  
  /* WARDS */
  else if (!str_cmp(argument, "ward") || !str_cmp(argument, "wards")) {
  	
    /* untie all */
    if (!str_cmp(arg, "all")) {
	 for (pWard = ch->in_room->wards; pWard != NULL; pWard = pWard_next) {
    	   pWard_next = pWard->next;
    	   
    	   if (pWard->duration < 0)
		continue;
    	   if (skill_table[pWard->sn].spell_fun == spell_null)
		continue;
    	   
           if (pWard->inverted && pWard->casterId != ch->id && !IS_IMMORTAL(ch))
           {
		continue;
	   }
    	   found = TRUE;
    	   channie_strength =  ch->holding + get_skill(ch, pWard->sn) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
	   
    	   if ((channie_strength > pWard->tied_strength + 20) && (pWard->casterId != ch->id)) {
		sprintf(buffer, "You examine the knot on the '%s' ward but fails to untie it.\n\r",
			   skill_table[pWard->sn].name);
		send_to_char(buffer, ch);
	   }
	   else {
		sprintf(buffer, "You examine the knot on the '%s' ward and successfully untie it.\n\r",
			   skill_table[pWard->sn].name);
		send_to_char(buffer, ch);
		
		if ( pWard->sn > 0 && !IS_NULLSTR(skill_table[pWard->sn].msg_off)) {
		  send_to_char( skill_table[pWard->sn].msg_off, ch );
		  send_to_char( "\n\r", ch );
		}
		
		ward_remove(ch->in_room, pWard);
	   }
      }
      
      if (!found) {
	   send_to_char("You find no wards that is tied here.\n\r", ch);
      }
      return;            
    }
    
    // Untie spesific ward
    else {
	 for (pWard = ch->in_room->wards; pWard != NULL; pWard = pWard_next) {
    	   pWard_next = pWard->next;
    	   if (str_prefix(arg, skill_table[pWard->sn].name))
		continue;
	   if (pWard->duration < 0)
		continue;
	   if (skill_table[pWard->sn].spell_fun == spell_null)
		continue;
	   
           if (pWard->inverted && pWard->casterId != ch->id && !IS_IMMORTAL(ch))
           {
		continue;
	   }

	   channie_strength =  ch->holding + get_skill(ch, pWard->sn) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
	   
	   if ((channie_strength > pWard->tied_strength + 20) && (pWard->casterId != ch->id)) {
		sprintf(buffer, "You examine the knot on the '%s' ward but fails to untie it.\n\r",
			   skill_table[pWard->sn].name);
		send_to_char(buffer, ch);
		return;
	   }
	   else {
		sprintf(buffer, "You examine the knot on the '%s' ward and successfully untie it.\n\r",
			   skill_table[pWard->sn].name);
		send_to_char(buffer, ch);
		
		if ( pWard->sn > 0 && !IS_NULLSTR(skill_table[pWard->sn].msg_off)) {
		  send_to_char( skill_table[pWard->sn].msg_off, ch );
		  send_to_char( "\n\r", ch );
		}    	   
		
		ward_remove(ch->in_room, pWard);
		return;
        }
	 }
	 send_to_char("You find no ward with that name that is tied in this room.\n\r", ch);
	 return;     
    }
  }


  /* ROOM WEAVES */
  else if (!str_cmp(argument, "room")) {
  	
    /* untie all */
    if (!str_cmp(arg, "all")) {
	 for (pWeave = ch->in_room->weaves; pWeave != NULL; pWeave = pWeave_next) {
    	   pWeave_next = pWeave->next;
    	   
    	   if (pWeave->duration < 0)
		continue;
           if (pWeave->inverted && pWeave->casterId != ch->id && !IS_IMMORTAL(ch))
           {
		continue;
	   }
    	   if (skill_table[pWeave->type].spell_fun == spell_null)
		continue;
    	   
    	   found = TRUE;
    	   channie_strength =  ch->holding + get_skill(ch, pWeave->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
	   
    	   if ((channie_strength > pWeave->tied_strength + 20) && (pWeave->casterId != ch->id)) {
		sprintf(buffer, "You examine the knot on the '%s' room weave but fails to untie it.\n\r",
			   skill_table[pWeave->type].name);
		send_to_char(buffer, ch);
	   }
	   else {
		sprintf(buffer, "You examine the knot on the '%s' room weave and successfully untie it.\n\r",
			   skill_table[pWeave->type].name);
		send_to_char(buffer, ch);
		
		if ( pWeave->type > 0 && !IS_NULLSTR(skill_table[pWeave->type].msg_off)) {
		  send_to_char( skill_table[pWeave->type].msg_off, ch );
		  send_to_char( "\n\r", ch );
		}
		
		room_weave_remove(ch->in_room, pWeave);
	   }
      }
      
      if (!found) {
	   send_to_char("You find no weaves that is tied in this room.\n\r", ch);
      }
      return;            
    }
    
    // Untie spesific ward
    else {
	 for (pWeave = ch->in_room->weaves; pWeave != NULL; pWeave = pWeave_next) {
    	   pWeave_next = pWeave->next;
    	   if (str_prefix(arg, skill_table[pWeave->type].name))
		continue;
	   if (pWeave->duration < 0)
		continue;
           if (pWeave->inverted && pWeave->casterId != ch->id && !IS_IMMORTAL(ch))
           {
		continue;
	   }
	   if (skill_table[pWeave->type].spell_fun == spell_null)
		continue;
	   
	   channie_strength =  ch->holding + get_skill(ch, pWeave->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
	   
	   if ((channie_strength > pWeave->tied_strength + 20) && (pWeave->casterId != ch->id)) {
		sprintf(buffer, "You examine the knot on the '%s' room weave but fails to untie it.\n\r",
			   skill_table[pWeave->type].name);
		send_to_char(buffer, ch);
		return;
	   }
	   else {
		sprintf(buffer, "You examine the knot on the '%s' room weave and successfully untie it.\n\r",
			   skill_table[pWeave->type].name);
		send_to_char(buffer, ch);
		
		if ( pWeave->type > 0 && !IS_NULLSTR(skill_table[pWeave->type].msg_off)) {
		  send_to_char( skill_table[pWeave->type].msg_off, ch );
		  send_to_char( "\n\r", ch );
		}    	   
		
		room_weave_remove(ch->in_room, pWeave);
		return;
        }
	 }
	 send_to_char("You find no weaves with that name that is tied in this room.\n\r", ch);
	 return;     
    }
  }

  
 /* TARGET */
  else {

    /* Find target if present in room */
    if ((vch = get_char_room( ch, argument )) == NULL) {
	 send_to_char( "They aren't here.\n\r", ch );
	 return;
    }

    /* untie all weaves you sustain on target */
    if (!str_cmp(arg, "all")) {
	 for ( paf = vch->affected; paf != NULL; paf = paf_next ) {
	   paf_next = paf->next;
	   
	   if (paf->duration < 0) {
		continue;
	   }
	   
	   if (paf->type == gsn_poison)
	      continue;

           if (paf->inverted && paf->casterId != ch->id && !IS_IMMORTAL(ch))
           {
		continue;
	   }
	   if (skill_table[paf->type].spell_fun == spell_null)
		continue;

	    channie_strength =  ch->holding + get_skill(ch, paf->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
	    if ((channie_strength > paf->tied_strength + 20) && (paf->casterId != ch->id)) {
		sprintf(buffer, "You examine the knot on the '%s' weave but fails to untie it.\n\r",
			   skill_table[paf->type].name);
		send_to_char(buffer, ch);
	   }
	   else {
		found = TRUE;
		sprintf(buffer, "You examine the knot on the '%s' weave and successfully untie it.\n\r",
			   skill_table[paf->type].name);
		send_to_char(buffer, ch);
		
		if ( paf->type > 0 && skill_table[paf->type].msg_off ) {
		  send_to_char( skill_table[paf->type].msg_off, vch );
		  send_to_char( "\n\r", vch );
		}

		  /* If wall of fire, extract objects */
		  if (paf->type == gsn_wof) {
		    extract_wof(vch);
		  }
		  
		  /* If wall of air, extract objects */
		  if (paf->type == gsn_woa) {
		    extract_woa(vch);
		  }
		
		  if (paf->type == gsn_bof) {
		    bof = get_eq_char(vch, WEAR_WIELD);
		    extract_obj( bof );
		  }

		affect_remove( vch, paf );
	   }
	 }
	 if (!found) {
	   sprintf(buffer,"You find no weaves that is tied on %s\n\r",  PERS(vch, ch));
	   send_to_char(buffer, ch);
	 }
	 return;
    }

    /* try to untie spesific weave on target */
    else {
	 for ( paf = vch->affected; paf != NULL; paf = paf->next ) {
	   if (str_prefix(arg, skill_table[paf->type].name)) {
		continue;
	   }
	   else {

		if (paf->duration < 0)
		  continue;
		  
           	if (paf->inverted && paf->casterId != ch->id && !IS_IMMORTAL(ch))
           	{
			continue;
	   	}

		if (paf->type == gsn_poison)
	          continue;
		
		if (skill_table[paf->type].spell_fun == spell_null)
		  continue;

	        channie_strength =  ch->holding + get_skill(ch, paf->type) + (IS_NPC(ch)) ? 0 : ch->pcdata->extended_level;
	        if ((channie_strength > paf->tied_strength + 20) && (paf->casterId != ch->id)) {
		  sprintf(buffer, "You examine the knot on the '%s' weave but fails to untie it.\n\r",
				skill_table[paf->type].name);
		  send_to_char(buffer, ch);
		  return;
		}
		else {
		  sprintf(buffer, "You examine the knot on the '%s' weave and successfully untie it.\n\r",
				skill_table[paf->type].name);
		  send_to_char(buffer, ch);
		  
		  if ( paf->type > 0 && skill_table[paf->type].msg_off ) {
		    send_to_char( skill_table[paf->type].msg_off, vch );
		    send_to_char( "\n\r", vch );
		  }

		  /* If wall of fire, extract objects */
		  if (paf->type == gsn_wof) {
		    extract_wof(vch);
		  }
		  
		  /* If wall of air, extract objects */
		  if (paf->type == gsn_woa) {
		    extract_woa(vch);
		  }

		  if (paf->type == gsn_bof) {
		    bof = get_eq_char(vch, WEAR_WIELD);
		    extract_obj( bof );
		  }

		  
		  affect_remove( vch, paf );
		  return;
		}
	   }
	 }
	 sprintf(buffer,"You find no weave with that name that is tied on %s\n\r",  PERS(vch, ch));
	 send_to_char(buffer, ch);
	 return;
    }
  }
  return;
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void judge_spheres(CHAR_DATA *ch, CHAR_DATA *vch)
{
  char buf[MAX_STRING_LENGTH];
  int i;
  int cnt =0;
  int cnt2=0;

  /* Count how many flows stronger */
  for (i = 0; i < MAX_SPHERE; i++) {
    if (vch->perm_sphere[i] > ch->perm_sphere[i])
	 cnt++;
  }
    
  sprintf(buf, "%s seems to be stronger than you in ", PERS(vch, ch));
  for (i = 0; i < MAX_SPHERE; i++) {
    if (vch->perm_sphere[i] > ch->perm_sphere[i]) {
	 cnt2++;
	 if ( (cnt == cnt2) && (cnt > 1))
	   strcat(buf, " and ");
	 if ( (cnt2 < cnt) && (cnt > 0) && (cnt != 2) && (i > 0))
	   strcat(buf, ", ");
	 strcat(buf, capitalize(sphere_table[i].name));
    }
  }
  
  /* If no spheres, return */
  if (cnt == 0)
    return;

  strcat(buf, ".\n\r");
  send_to_char(buf, ch);
  return;
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_judge (CHAR_DATA *ch,  char *argument)
{
  CHAR_DATA   *vch;
  char buffer[MAX_STRING_LENGTH];
  long vch_op=0; 
  long ch_op =0;
  int sn=0;
  
  if (!IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER) && !IS_NPC(ch)) {
    send_to_char("You cannot judge others.\n\r", ch);
    return;
  }

  sn = skill_lookup(ch->sex == SEX_MALE ? "seize" : "embrace");

  if (ch->pcdata->learned[sn] < 1) {
    send_to_char("You don't know how to channel the true source.\n\r",ch);
    return;
  }
  

  if (argument[0] == '\0') {
    send_to_char("Judge who?\n\r", ch);
    return;
  }

  /* Find target if present in room */
  if ((vch = get_char_room( ch, argument )) == NULL) {
    send_to_char("They aren't here.\n\r", ch );
    return;
  }

  if (!IS_RP(ch) || !IS_RP(vch))
  {
	send_to_char("You both must be IC for this to happen.\n\r",ch);
	return;
  }
  if (ch->sex != vch->sex) {
    sprintf(buffer, "You cannot judge a %s.\n\r",
		  vch->sex == SEX_MALE ? "male" : "female");
    send_to_char(buffer, ch);
    return;
  }

  if (vch->sex == SEX_MALE && !IS_AFFECTED(vch, AFF_CHANNELING) && !IS_SET(vch->act2,PLR2_MASKCHAN)) {
    sprintf(buffer, "%s is not holding %s.\n\r", PERS(vch, ch),
		  ch->sex == SEX_MALE ? "Saidin" : "Saidar");
    send_to_char(buffer, ch);
    return;
  }
  
  if (vch->sex == SEX_FEMALE && (vch->class != CLASS_CHANNELER || (vch->class == CLASS_CHANNELER && IS_SET(vch->act2,PLR2_MASKCHAN)))) {
	sprintf(buffer, "%s does not have the ability to learn how to channel.\n\r", PERS(vch,ch));
	send_to_char(buffer,ch);
	return;
  }
  vch_op      = get_curr_op(vch);
  ch_op       = get_curr_op(ch);

  if (vch_op < ch_op) {
    sprintf(buffer, "You are overall stronger in the Power than %s.\n\r", PERS(vch, ch));
    send_to_char(buffer, ch);

    judge_spheres(ch, vch);
    
    return;
  }

  else if (vch_op == ch_op) {
    sprintf(buffer, "You seem to be as strong in the Power as %s.\n\r", PERS(vch, ch));
    send_to_char(buffer, ch);

    judge_spheres(ch, vch);

    return;
  }

  else {
    sprintf(buffer, "%s seems to be overall stronger than you in the Power.\n\r", PERS(vch, ch));
    send_to_char(buffer, ch);

    judge_spheres(ch, vch);

    return;
  }
  
  return;
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
bool break_shield (CHAR_DATA *ch, char *argument)
{
  AFFECT_DATA *paf;
  char buf[MAX_STRING_LENGTH];
  long ch_op                 = 0;
  int sn                     = 0;
  int learned                = 0;
  double shield_strength_mod = 0;
  long break_strength        = 0;

  /* To break a shield you at least need to know the weave */
  sn = skill_lookup("shielding");

  if (!IS_NPC(ch))
    learned = ch->pcdata->learned[sn];
  else
    learned = ch->level;

  if (!IS_NPC(ch)) {
    if (learned < 1) {
      send_to_char("You can't break a shield, you don't even know how to make one.\n\r", ch);
      return FALSE;
    }
  }
  else {
     if (learned < skill_table[sn].skill_level[CLASS_CHANNELER]) {
        return FALSE;
     }
  }
  
  for ( paf = ch->affected; paf != NULL; paf = paf->next ) {
    if (paf->type != sn)
	 continue;
      
    // Get the amount of OP that is tried to used for the break
    if (IS_NULLSTR(argument)) {
    	ch_op = get_curr_op(ch)/((double)100/ch->autoholding);
    }
    else {
       ch_op = atoi(argument);

       // Make sure you can't break a shield by trying to channel high above your max op
       if ( ch_op > (get_curr_op(ch) + ((get_curr_op(ch)*10)/100))) {
         ch_op = (get_curr_op(ch) + ((get_curr_op(ch)*10)/100));
       }
     }
    
    // Modifier, e.g in percent what is used to try break the shield?
    shield_strength_mod = (double)(ch_op/(double)get_curr_op(ch));   
    if (shield_strength_mod > 1)
      shield_strength_mod = 1;
    
    // Break strength algo
    if (ch->main_sphere == SPHERE_SPIRIT) {
      break_strength = UMAX(1, (((ch->perm_sphere[SPHERE_SPIRIT]/2) + (learned/5) + (ch_op/10) + 20) * shield_strength_mod));
    }
    else {
      break_strength = UMAX(1, (((ch->perm_sphere[SPHERE_SPIRIT]/2) + (learned/5) + (ch_op/10)) * shield_strength_mod));
    }
    
    /* Harder to break a sustained shield */
    if (paf->duration == SUSTAIN_WEAVE) {
      break_strength -= 100;
    }
    
    // Add a little range to it
    break_strength += number_range(1,20);
    
    //Debug lines
    //sprintf(buf, "break_shield() :: ch_op=%ld  shield_strength_mod=%f -> shield_strength=%d < break_strength=%ld", ch_op, shield_strength_mod, paf->modifier, break_strength);
    //log_string(buf);
    
    if (paf->modifier < break_strength) {
	 send_to_char("You press against the shield and {Rbreaks through{x!\n\r", ch);

	 /* Who is the caster ? */
	 paf->caster = get_charId_world(ch, paf->casterId);

	 /* If caster not logged on, no need to tell about it */
	 if (paf->caster != NULL) {
	   if (IS_AWAKE(paf->caster)) {
		sprintf(buf, "%s {Rbreaks your shielding{x.\n\r", PERS(ch, paf->caster));
		send_to_char(buf, paf->caster);
	   }
	 }

	 affect_remove( ch, paf );
	 return TRUE;
    }
    else {
	 paf->caster = get_charId_world(ch, paf->casterId);

	 if (paf->caster != NULL) {
	   if (IS_AWAKE(paf->caster)) {
		sprintf(buf, "You feel %s pressing against your shielding, trying to break free.\n\r",
			   PERS(ch, paf->caster));
		send_to_char(buf, paf->caster);
	   }
	 }
	 return FALSE;
    }
  }
  
  return FALSE;
}

void do_seeareaweaves(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;
 
  if (IS_SET(ch->chan_flags, CHAN_SEEAREAWEAVES)) {
    send_to_char("You will no longer see area weave messages.\n\r",ch);
    REMOVE_BIT(ch->chan_flags, CHAN_SEEAREAWEAVES);
  }
  else {
    send_to_char("You will now see area weave messages.\n\r",ch);
    SET_BIT(ch->chan_flags,CHAN_SEEAREAWEAVES);
  }
}

void do_seechanneling(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;

  if (ch->class != CLASS_CHANNELER) {
    send_to_char("You have no idea about channeling.\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->chan_flags, CHAN_SEECHANNELING)) {
    if (ch->pcdata->true_sex == SEX_MALE)
       send_to_char("You will no longer see channeling messages when someone channel Saidin.\n\r",ch);
    else
       send_to_char("You will no longer see channeling messages when someone channel Saidar.\n\r",ch);
    REMOVE_BIT(ch->chan_flags, CHAN_SEECHANNELING);
  }
  else {
    if (ch->pcdata->true_sex == SEX_MALE)
       send_to_char("You will now see channeling messages when someone channel Saidin.\n\r",ch);
    else
       send_to_char("You will now see channeling messages when someone channel Saidar.\n\r",ch);
    SET_BIT(ch->chan_flags,CHAN_SEECHANNELING);
  }
}

/**********************************************************************
*       Function      : 
*       Author        : Swordfish
*       Description   : Invert a weave, or all weaves
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_invert (CHAR_DATA *ch,  char *argument)
{
  AFFECT_DATA *paf;
  AFFECT_DATA *paf_next;
  WARD_DATA *pWard;
  WARD_DATA *pWard_next;
  AFFECT_DATA *pWeave;
  AFFECT_DATA *pWeave_next;
  CHAR_DATA   *vch;
  CHAR_DATA   *vch_next;
  char arg[MSL];
  char arg2[MSL];
  char buffer[MSL];
  int endurance  = 0;
  int sn         = 0;
  int learned    = 0;
  bool found     = FALSE;
  
  argument = one_argument(argument, arg);

  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("You cannot channel the true source.\n\r", ch);
    return;
  }
  
  sn        = skill_lookup("invert weave");
  endurance = skill_table[sn].min_endurance;

  if (!IS_NPC(ch))
    learned = ch->pcdata->learned[sn];
  else
    learned = get_level(ch);
  
  if (learned < 1) {
    send_to_char("You don't know how to invert a weave.\n\r",ch);
    return;
  }
  
  if (arg[0] == '\0') {
    send_to_char("Invert what weave?\n\r", ch);
    return;
  }
  
  /* Need to hold OP to invert a weave */
  if (!IS_AFFECTED(ch, AFF_CHANNELING)) {
    do_function(ch, &do_channel, "");
    if (!IS_AFFECTED(ch ,AFF_CHANNELING))
	 return;
  }

  if (ch->endurance < endurance) {
    send_to_char("You are too tired to concentrate or don't have enough endurance to focus!\n\r", ch);
    return;
  }
  
  /* SELF */
  if (argument[0] == '\0') {

    /* invert all */
    if (!str_cmp(arg, "all")) {
	 for (vch = char_list; vch != NULL; vch = vch_next) {
	   vch_next = vch->next;
	   for ( paf = vch->affected; paf != NULL; paf = paf_next ) {
		paf_next = paf->next;
		if (vch->affected == NULL)
		  continue;
		if (paf->casterId != ch->id)
		  continue;
		if (paf->type == skill_lookup("link"))
		  continue;
		if (number_percent() > learned) {
		  sprintf(buffer, "You examine the flows woven into '%s' on %s and try to invert the flows but you fail.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
		  send_to_char(buffer, ch);

		  ch->endurance -= endurance/2;
		  return;
		}
		
		found = TRUE;

		if (paf->inverted) {
		  paf->inverted = FALSE;
		  sprintf(buffer, "You examine the flows woven into '%s' on %s and remove the inversion on the flows.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
		}
		else {
		  paf->inverted = TRUE;
		  sprintf(buffer, "You examine the flows woven into '%s' on %s and invert the flows.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
		  
		}
		
		send_to_char(buffer, ch);
	   }
	 }
	 if (!found) {
	   send_to_char("You find no weaves on your self or others to invert.\n\r", ch);
	 }
	 else {
	   if (!IS_NPC(ch))
		check_improve(ch, sn, TRUE, 1);

	   ch->endurance -= endurance;
	 }
	 return;
    }
    
    /* invert spesific weave */
    else {
	 for ( paf = ch->affected; paf != NULL; paf = paf->next ) {
	   if (str_prefix(arg, skill_table[paf->type].name)) {
		continue;
	   }
	   else {
		if (paf->type == skill_lookup("link"))
		  continue;
		if (number_percent() > learned) {
		  sprintf(buffer, "You examine the flows woven into '%s' and try to invert the flows but you fail.\n\r",  skill_table[paf->type].name);
		  send_to_char(buffer, ch);

		  ch->endurance -= endurance/2;
		  return;
		}
		if (paf->inverted) {
		  paf->inverted = FALSE;
		  sprintf(buffer, "You examine the flows woven into '%s' and remove the inversion on the flows.\n\r",  skill_table[paf->type].name);
		}
		else {
		  paf->inverted = TRUE;
		  sprintf(buffer, "You examine the flows woven into '%s' and invert the flows.\n\r",  skill_table[paf->type].name);
		}		
		send_to_char(buffer, ch);

		if (!IS_NPC(ch))
		  check_improve(ch, sn, TRUE, 1);

		ch->endurance -= endurance;
		
		return;
	   }
	 }
	 send_to_char("You find no weave with that name on your self.\n\r", ch);
	 return;
    }
  }

  // WARDS
  else if (!str_cmp(argument, "ward") || !str_cmp(argument,"wards")) {
    
    // Invert all wards
    if (!str_cmp(arg, "all")) {
	 for (pWard = ch->in_room->wards; pWard != NULL; pWard = pWard_next) {
	   pWard_next = pWard->next;
	   if (pWard->casterId != ch->id)
		continue;
	   
	   if (number_percent() > learned) {
		sprintf(buffer, "You examine the flows woven into '%s' and try to invert the flows but you fail.\n\r", skill_table[pWard->sn].name);
		send_to_char(buffer, ch);
		
		ch->endurance -= endurance/2;
		return;
	   }
	   
	   found = TRUE;
	   
	   if (pWard->inverted) {
		pWard->inverted = FALSE;
		sprintf(buffer, "You examine the flows woven into '%s' on and remove the inversion on the flows.\n\r", skill_table[pWard->sn].name);
	   }
	   else {
		pWard->inverted = TRUE;
		sprintf(buffer, "You examine the flows woven into '%s' on and invert the flows.\n\r", skill_table[pWard->sn].name);			 
	   }
	   
	   send_to_char(buffer, ch);
	 }
	 if (!found) {
	   send_to_char("You sustain no wards.\n\r", ch);
	 }
	 else {
	   if (!IS_NPC(ch))
		check_improve(ch, sn, TRUE, 1);
	   
	   ch->endurance -= endurance;
	 }
	 return;              
    }
  
    /* invert spesific ward */
    else {
    	 for (pWard = ch->in_room->wards; pWard != NULL; pWard = pWard_next) {
	   pWard_next = pWard->next;
	   
	   if (str_prefix(arg, skill_table[pWard->sn].name))
		continue;
	   else {
		if (pWard->casterId != ch->id)
		  continue;
		
		if (number_percent() > learned) {
		  sprintf(buffer, "You examine the flows woven into '%s' and try to invert the flows but you fail.\n\r",  skill_table[pWard->sn].name);
		  send_to_char(buffer, ch);
		  
		  ch->endurance -= endurance/2;
		  return;
		}
		if (pWard->inverted) {
		  pWard->inverted = FALSE;
		  sprintf(buffer, "You examine the flows woven into '%s' and remove the inversion on the flows.\n\r",  skill_table[pWard->sn].name);
		}
		else {
		  pWard->inverted = TRUE;
		  sprintf(buffer, "You examine the flows woven into '%s' and invert the flows.\n\r",  skill_table[pWard->sn].name);
		}		
		send_to_char(buffer, ch);
		
		if (!IS_NPC(ch))
		  check_improve(ch, sn, TRUE, 1);

		ch->endurance -= endurance;
		
		return;
	   }
	 }
	 send_to_char("You find no wards with that name.\n\r", ch);
	 return;
    }
  }


  // ROOM WEAVES
  else if (!str_cmp(argument, "room")) {
    
    // Invert all wards    
    if (!str_cmp(arg, "all")) {
	 for (pWeave = ch->in_room->weaves; pWeave != NULL; pWeave = pWeave_next) {
	   pWeave_next = pWeave->next;
	   if (pWeave->casterId != ch->id)
		continue;
	   
	   if (number_percent() > learned) {
		sprintf(buffer, "You examine the flows woven into '%s' and try to invert the flows but you fail.\n\r", skill_table[pWeave->type].name);
		send_to_char(buffer, ch);
		
		ch->endurance -= endurance/2;
		return;
	   }
	   
	   found = TRUE;
	   
	   if (pWeave->inverted) {
		pWeave->inverted = FALSE;
		sprintf(buffer, "You examine the flows woven into '%s' on and remove the inversion on the flows.\n\r", skill_table[pWeave->type].name);
	   }
	   else {
		pWeave->inverted = TRUE;
		sprintf(buffer, "You examine the flows woven into '%s' on and invert the flows.\n\r", skill_table[pWeave->type].name);			 
	   }
	   
	   send_to_char(buffer, ch);
	 }
	 if (!found) {
	   send_to_char("You sustain no weaves.\n\r", ch);
	 }
	 else {
	   if (!IS_NPC(ch))
		check_improve(ch, sn, TRUE, 1);
	   
	   ch->endurance -= endurance;
	 }
	 return;              
    }
  
    /* invert spesific weave */
    else {
    	 for (pWeave = ch->in_room->weaves; pWeave != NULL; pWeave = pWeave_next) {
	   pWeave_next = pWeave->next;
	   
	   if (str_prefix(arg, skill_table[pWeave->type].name))
		continue;
	   else {
		if (pWeave->casterId != ch->id)
		  continue;
		
		if (number_percent() > learned) {
		  sprintf(buffer, "You examine the flows woven into '%s' on the room and try to invert the flows but you fail.\n\r",  skill_table[pWeave->type].name);
		  send_to_char(buffer, ch);
		  
		  ch->endurance -= endurance/2;
		  return;
		}
		if (pWeave->inverted) {
		  pWeave->inverted = FALSE;
		  sprintf(buffer, "You examine the flows woven into '%s' on the room and remove the inversion on the flows.\n\r",  skill_table[pWeave->type].name);
		}
		else {
		  pWeave->inverted = TRUE;
		  sprintf(buffer, "You examine the flows woven into '%s' and invert the flows.\n\r",  skill_table[pWeave->type].name);
		}		
		send_to_char(buffer, ch);
		
		if (!IS_NPC(ch))
		  check_improve(ch, sn, TRUE, 1);

		ch->endurance -= endurance;
		
		return;
	   }
	 }
	 send_to_char("You find no weaves with that name on the room.\n\r", ch);
	 return;
    }
  }
  
  
  /* TARGET */
  else {
    
    /* Find target if present in room */
    argument = one_argument(argument, arg2);
    if ((vch = get_char_room( ch, arg2 )) == NULL) {
	 send_to_char( "They aren't here.\n\r", ch );
	 return;
    }
    
    /* invert all weaves you sustain on target */
    if (!str_cmp(arg, "all")) {
	 for ( paf = vch->affected; paf != NULL; paf = paf_next ) {
	   paf_next = paf->next;	   
	   if (paf->casterId != ch->id)
		continue;	   
	   if (paf->type == skill_lookup("link"))
		continue;
	   if (number_percent() > learned) {
		sprintf(buffer, "You examine the flows woven into '%s' on %s try to invert the flows but you fail.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
		send_to_char(buffer, ch);

		ch->endurance -= endurance/2;
		return;
	   }

	   found = TRUE;
	   if (paf->inverted) {
		paf->inverted = FALSE;
		sprintf(buffer, "You examine the flows woven into '%s' on %s and remove the inversion on the flows.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
	   }
	   else {
		paf->inverted = TRUE;
		sprintf(buffer, "You examine the flows woven into '%s' on %s and invert the flows.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
	   }	   

	   send_to_char(buffer, ch);
	 }
	 if (!found) {
	   sprintf(buffer, "You find no weaves on %s to invert.\n\r", PERS(vch, ch));
	   send_to_char(buffer, ch);
	 }
	 else {
	   if (!IS_NPC(ch))
		check_improve(ch, sn, TRUE, 1);

	   ch->endurance -= endurance;
	 }
	 return;
    }
    
    /* invert spesific weave on target */
    else {
	 for ( paf = vch->affected; paf != NULL; paf = paf->next ) {
	   if (paf->type == skill_lookup("link"))
		continue;
	   if (str_prefix(arg, skill_table[paf->type].name)) {
		continue;
	   }
	   else if (paf->casterId != ch->id) {
		continue;
	   }
	   else {
		if (number_percent() > learned) {
		  sprintf(buffer, "You examine the flows woven into '%s' on %s and try to invert the flows but you fail.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
		  send_to_char(buffer, ch);

		  ch->endurance -= endurance/2;
		  return;
		}

		if (paf->inverted) {
		  paf->inverted = FALSE;
		  sprintf(buffer, "You examine the flows woven into '%s' on %s and remove the inversion on the flows.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
		}
		else {
		  paf->inverted = TRUE;
		  sprintf(buffer, "You examine the flows woven into '%s' on %s and invert the flows.\n\r", skill_table[paf->type].name, vch == ch ? "your self" : PERS(vch, ch));
		}
		send_to_char(buffer, ch);
		
		if (!IS_NPC(ch))
		  check_improve(ch, sn, TRUE, 1);

		ch->endurance -= endurance;
		return;
	   }
	 }
	 sprintf(buffer, "You find no weave with that name on %s\n\r", PERS(vch, ch));
	 send_to_char(buffer, ch);
	 return;
    }
  }
}

// Remove a residue you have caused in the room
void do_rremove( CHAR_DATA *ch, char *argument )
{
  RESIDUE_DATA *rd;
  RESIDUE_DATA *rd_next;
  char buf [MSL];
  int sn=0;
  int learned=0;
  
  // Only for channelers
  if (ch->class != CLASS_CHANNELER) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  sn = skill_lookup("remove residues");
  
  if (!IS_NPC(ch))
    learned = ch->pcdata->learned[sn];
  else
    learned = get_level(ch);
  
  if (learned < 1) {
    send_to_char("You don't know how to remove vestiges from your flows.\n\r", ch);
    return;
  }

  // Those with residue talent, is a little better
  if (learned > 0 && IS_SET(ch->talents, TALENT_RESIDUES))
    learned += 20;
  
  if (ch->in_room->residues == NULL) {
    send_to_char("You are unable to find any vestiges to remove here.\n\r", ch);
    return;
  }
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Remove vestiges from what flows?\n\r", ch);
    return;
  }

  // Need to hold OP to remove residues
  if (!IS_AFFECTED(ch, AFF_CHANNELING)) {
    do_function(ch, &do_channel, "");
    if (!IS_AFFECTED(ch ,AFF_CHANNELING))
	 return;
  }
  
  for (rd = ch->in_room->residues; rd != NULL; rd = rd_next) {
    rd_next = rd->next;
    if (str_cmp(argument, skill_table[rd->sn].name))
	 continue;
    if (rd->casterId != ch->id)
	 continue;
    
    if (number_percent() > learned) {
	 sprintf(buf, "You examine the residues from the flows woven into '%s' but you fail to remove the vestiges.\n\r", skill_table[rd->sn].name);
    }
    else {
	 sprintf(buf, "You examine the residues from the flows woven into '%s' and remove the vestiges.\n\r", skill_table[rd->sn].name);
	 
	 residue_remove(ch->in_room, rd);    
    }
    send_to_char(buf, ch);
    
    if (!IS_NPC(ch))
       check_improve(ch, sn, TRUE, 1);
    
    return;
  }

  send_to_char("You are unable to find any vestiges from such flows made by you here.\n\r", ch);
  return;            
}

// Check if a room is possible to gate/dreamgate to
bool is_gate_room(CHAR_DATA *ch, ROOM_INDEX_DATA *to_room)
{
  // Live is bright on the right side!
  if (IS_IMMORTAL(ch))
    return TRUE;

  // No gate from the room you stand in
  if (IS_SET(ch->in_room->room_flags, ROOM_NO_GATE))
    return FALSE;
  
  // No gate to the room you try to get to
  if (IS_SET(to_room->room_flags, ROOM_NO_GATE))
    return FALSE;

  // Immortal area is nono
  if (!str_cmp(to_room->area->file_name, "immortal.are"))
    return FALSE;
  
  // Only to open areas
  //if (!IS_SET(to_room->area->area_flags, AREA_OPEN))
  //  return FALSE;
  
  // Else game
  return TRUE;
}
