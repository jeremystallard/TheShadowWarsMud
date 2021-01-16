/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
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
#include <stdlib.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

/* from magic2.c */
int compare_skill_names(const void *v1, const void *v2);

bool check_can_link(CHAR_DATA * ch, CHAR_DATA * victim);

void make_mc_report(CHAR_DATA *ch, CHAR_DATA *tr)
{
  char note_buf1[MAX_STRING_LENGTH];
  char note_buf2[MAX_STRING_LENGTH];

  // Only report PCs
  if (IS_NPC(ch))
   return;

  // Only report PCs left newbie school
  if (!IS_SET(ch->act, PLR_GRADUATED))
    return;
  
  sprintf(note_buf1, "{cAes Sedai{x,\n\n\r"
                     "Today a man approached me here in %s.\n\n\r"
                     "He was requesting to be trained in the One Power.\n\n\r"
                     "His appearance was something like this:\n\r"
                     "%s.\n\n\r"
                     "The danger of letting a male channeler walk free is something\n\r"
                     "history has told us too much about. My duty as the WhiteTower\n\r"
                     "eyes and ears have been served once more.\n\n\r"
                     "Please make sure this man is handled.\n\n\r"
                     "Signed by the hand of %s,\n\r"
                     "%s\n\r",
                     ch->in_room->area->name,
                     ch->pcdata->appearance,
                     tr->short_descr,
                     ch->in_room->area->name);
                     
  sprintf(note_buf2, "Report of a possible male channeler in %s",  ch->in_room->area->name);
  
  make_note("Guild", "A mobile", "AesSedai", note_buf2, 56, note_buf1);
  
  return;
}

/* 
 * New TSW train :: Swordfish 
 */
void do_train( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH];
  ROOM_INDEX_DATA *room;
  OBJ_DATA  *sword;
  CHAR_DATA *trainer;
  int sn       = 0;
  int tr_cost  = 1;
  /* int exp_cost = 0; */
  int add_hp   = 0;
  int add_end  = 0;
  
  /* NPCs can't train now */
  if ( IS_NPC(ch) )
    return;
  
  buf[0] = '\0';
  
  argument = one_argument(argument, arg1);
  
  /* Number to train, or just text? */
  if (argument[0] == '\0') {
    strcpy(argument, arg1);
  }
  else {
    if (!(tr_cost = atoi(arg1)))
	 tr_cost = 1;
  }

  /* Make sure - training isn't allowed */
  if (tr_cost < 1) {
    send_to_char("Syntax: train # <skill/weave/hp/endurance>\n\r", ch);
    return;
  }
  
  /* Check for trainer. */
  for ( trainer = ch->in_room->people; trainer; trainer = trainer->next_in_room ) {
    if ( IS_NPC(trainer) && IS_SET(trainer->act, ACT_TRAIN) )
	 break;
  }
  
  if ( trainer == NULL || !can_see(ch,trainer)) {
    send_to_char( "You can't do that here.\n\r", ch );
    return;
  }

  if (IS_SET(trainer->act,ACT_ROAMERTRAINER) && trainer->next_trainmove == 0)
  {
	trainer->next_trainmove=current_time + 180;
  }

  /* No argument just give trains left to spend */
  if ( argument[0] == '\0' ) {
    sprintf( buf, "You have {W%d{x trains left.\n\r", ch->train );
    send_to_char( buf, ch );
    return;
  }

  if (!str_prefix(argument,"list")) {
    int col = 0;

    sprintf(buf, "%-18s%4s  %-18s%4s  %-18s%4s\n\r",
		  "skill/weave","train","skill/weave","train","skill/weave","train");
    send_to_char(buf,ch);

    for (sn = 0; sn < MAX_SKILL; sn++) {
	 if (skill_table[sn].name == NULL)
	   break;
	 
	 if (ch->pcdata->learned[sn]          > 0
		&&  ch->pcdata->learned[sn]      < MAX_PC_TRAIN
		&&  ch->pcdata->learned[sn]      < trainer->train_level
		&&  skill_table[sn].restriction != RES_NOGAIN
		&&  skill_table[sn].restriction != RES_GRANTED
		&&  skill_table[sn].restriction != RES_NOTRAIN		
		&&  ch->level >= skill_table[sn].skill_level[ch->class]) {

	   if (!IS_SET(trainer->train_flags, TRAIN_WEAVE) 
		   && (skill_table[sn].spell_fun != spell_null))
		continue;

	   if (IS_SET(trainer->train_flags, TRAIN_WEAVE)
		  && (skill_table[sn].spell_fun != spell_null)
		  && ch->sex != trainer ->sex)
		continue;
	   
	   if (!IS_SET(trainer->train_flags, TRAIN_SKILL) && 
		  (skill_table[sn].spell_fun == spell_null)) {
		if (!is_masterform(sn))
		  continue;
	   }

	   if (!IS_SET(trainer->train_flags, TRAIN_MASTERFORMS) && is_masterform(sn))
		continue;
/**
 * do all masterforms with the TRAIN_MASTERFORMS flag for now
 **/
/*
	   if (!IS_SET(trainer->train_flags, TRAIN_BLADEMASTER) 
		  && sn == gsn_blademaster)
		continue;	   

	   if (!IS_SET(trainer->train_flags, TRAIN_SPEARDANCER) 
		  && sn == gsn_speardancer)
		continue;	   

	   if (!IS_SET(trainer->train_flags, TRAIN_DUELLING) 
		  && sn == gsn_duelling)
		continue;	   
*/

	   sprintf(buf,"%-18s %4d  ", skill_table[sn].name, tr_cost);
	   send_to_char(buf,ch);
	   if (++col % 3 == 0)
		send_to_char("\n\r",ch);
	 }
    }
    if (col % 3 != 0)
	 send_to_char("\n\r",ch);
    return;
  }
  else if (!str_prefix(argument,"hp")) {
    if (tr_cost > ch->train) {
	 act("$N tells you '{7You don't have that many trains left.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    /* Calc how much hp to add per train */
    add_hp = con_app[get_curr_stat(ch,STAT_CON)].hitp + number_range(
		   class_table[ch->class].hp_min,
		   class_table[ch->class].hp_max );
    add_hp = add_hp * 9/10;
    add_hp = UMAX( 2, add_hp );

    add_hp                = add_hp * tr_cost;

    if (ch->pcdata->perm_hit >= MAX_PC_HP) {
	 act("$N tells you '{7I can't teach you anymore. It's beyond my powers.{x", ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (ch->pcdata->perm_hit+add_hp > MAX_PC_HP) {
	 ch->train            -= tr_cost;
	 ch->pcdata->perm_hit = MAX_PC_HP;
	 ch->max_hit          = MAX_PC_HP;
	 if (ch->hit+add_hp > MAX_PC_HP)
	   ch->hit              = MAX_PC_HP;
	 else
	   ch->hit            += add_hp;
    }
    else {
	 ch->train            -= tr_cost;
	 ch->pcdata->perm_hit += add_hp;
	 ch->max_hit          += add_hp;
	 ch->hit              += add_hp;

	 /* Fix hit locations */
	 // LA:
	 if ((ch->hit_loc[LOC_LA] + (add_hp/LOC_MOD_LA)) < get_max_hit_loc(ch, LOC_LA))
	   ch->hit_loc[LOC_LA] += (add_hp/LOC_MOD_LA);
	 else
	   ch->hit_loc[LOC_LA] = get_max_hit_loc(ch, LOC_LA);

	 // LL:
	 if ((ch->hit_loc[LOC_LL] + (add_hp/LOC_MOD_LL)) < get_max_hit_loc(ch, LOC_LL))
	   ch->hit_loc[LOC_LL] += (add_hp/LOC_MOD_LL);
	 else
	   ch->hit_loc[LOC_LL] = get_max_hit_loc(ch, LOC_LL);

	 // HE:
	 if ((ch->hit_loc[LOC_HE] + (add_hp/LOC_MOD_HE)) < get_max_hit_loc(ch, LOC_HE))
	   ch->hit_loc[LOC_HE] += (add_hp/LOC_MOD_HE);
	 else
	   ch->hit_loc[LOC_HE] = get_max_hit_loc(ch, LOC_HE);

	 // BD:
	 if ((ch->hit_loc[LOC_BD] + (add_hp/LOC_MOD_BD)) < get_max_hit_loc(ch, LOC_BD))
	   ch->hit_loc[LOC_BD] += (add_hp/LOC_MOD_BD);
	 else
	   ch->hit_loc[LOC_BD] = get_max_hit_loc(ch, LOC_BD);

	 // RA:
	 if ((ch->hit_loc[LOC_RA] + (add_hp/LOC_MOD_RA)) < get_max_hit_loc(ch, LOC_RA))
	   ch->hit_loc[LOC_RA] += (add_hp/LOC_MOD_RA);
	 else
	   ch->hit_loc[LOC_RA] = get_max_hit_loc(ch, LOC_RA);

	 // RL:
	 if ((ch->hit_loc[LOC_RL] + (add_hp/LOC_MOD_RL)) < get_max_hit_loc(ch, LOC_RL))
	   ch->hit_loc[LOC_RL] += (add_hp/LOC_MOD_RL);
	 else
	   ch->hit_loc[LOC_RL] = get_max_hit_loc(ch, LOC_RL);

    }

    act( "Your durability increases!",ch,NULL,NULL,TO_CHAR);
    return;
  }
  else if (!str_prefix(argument,"endurance")) {
    if (tr_cost > ch->train) {
	 act("$N tells you '{7You don't have that many trains left.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }

    /* Calc how much end to add per train */
    add_end = number_range(10,(2*get_curr_stat(ch,STAT_CON)
						+ get_curr_stat(ch,STAT_DEX)));
    if (!class_table[ch->class].fEndurance)
	 add_end /= 2;
    add_end	= UMAX(10, add_end );

    add_end                = add_end * tr_cost;

    if (ch->max_endurance >= MAX_PC_END) {
	 act("$N tells you '{7I can't teach you anymore. It's beyond my powers.{x", ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (ch->max_endurance+add_end > MAX_PC_END) {
	 ch->train                  -= tr_cost;
	 ch->pcdata->perm_endurance = MAX_PC_END;
	 ch->max_endurance          = MAX_PC_END;
	 if (ch->endurance+add_end > MAX_PC_END)
	   ch->endurance            = MAX_PC_END;
	 else
	   ch->endurance           += add_end;
    }
    else {
	 ch->train                  -= tr_cost;
	 ch->pcdata->perm_endurance += add_end;
	 ch->max_endurance          += add_end;
	 ch->endurance              += add_end;
    }

    act( "Your endurance increases!",ch,NULL,NULL,TO_CHAR);
    return;
  }
  
  /* else train a skill/weave */
  sn = skill_lookup(argument);
  
  if (sn > -1) {
    if (ch->sex != trainer->sex && skill_table[sn].spell_fun != spell_null && ch->class == CLASS_CHANNELER) {
	 if (ch->sex == SEX_MALE) {
	   act("$N gasp and look at you as $E says '{7You are a bloody male channeler! I will report you to the White Tower!{x'", ch,NULL,trainer,TO_CHAR);
	   make_mc_report(ch, trainer);
	   return;
	 }
	 else {
	   act("$N shake the head saying, '{7I can't teach a fish to fly.{x'", ch,NULL,trainer,TO_CHAR);
	   return;
	 }
    }
    
    // NO train restriction
    if (skill_table[sn].restriction == RES_NOTRAIN) {
    	act("$N tells you '{7I do not understand...{x'",ch,NULL,trainer,TO_CHAR);
    	return;
    }

    if (ch->pcdata->learned[sn] < 1) {
	 act("$N tells you '{7You must learn before you can train my friend.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (ch->pcdata->learned[sn] >= MAX_PC_TRAIN) {
	 act("$N tells you '{7My training have come to it's limit my friend.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (tr_cost > ch->train) {
	 act("$N tells you '{7You don't have that many trains left.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if ( (ch->pcdata->learned[sn] + tr_cost) > trainer->train_level) {
	 act("$N tells you '{7I can't teach you that much. It's beyond my powers.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (!IS_SET(trainer->train_flags, TRAIN_WEAVE)
        && (skill_table[sn].spell_fun != spell_null)) {
       act("$N tells you '{7Do I look like a bloody channeler?{x'", ch,NULL,trainer,TO_CHAR);
       return;
    }

    if (!IS_SET(trainer->train_flags, TRAIN_SKILL) &&
        (skill_table[sn].spell_fun == spell_null)) {
	 if (!is_masterform(ch)) {
	   act("$N tells you '{7I can't help you with skills.{x'", ch,NULL,trainer,TO_CHAR);
	   return;
	 }
    }

    if (!IS_SET(trainer->train_flags, TRAIN_MASTERFORMS) && is_masterform(sn))
    {
	 act("$N tells you '{7Do I look like I know that to you??{x'",ch,NULL,trainer,TO_CHAR);
	 return;
    }

    /* Make sure only 1 train per time for Masterforms*/
    if (is_masterform(sn)) {
	 if (ch->pcdata->next_bmtrain > current_time) {
	   act("$N glares at you with an evil eye and says, '{7It is to soon to continue practicing the forms. Come back later and we will continue the dance.{x'",ch,NULL,trainer,TO_CHAR);
	   return;
	 }
	 else
	   tr_cost = 1;
    }

    ch->train               -= tr_cost;
    ch->pcdata->learned[sn] += tr_cost;
    sprintf(buf, "$N trains you {W%d{x time%s in the art of $t", tr_cost, tr_cost > 1 ? "s" : "");
    act(buf, ch,skill_table[sn].name,trainer,TO_CHAR);

    /* Set timer if Masterform : Blademaster */
    if (is_masterform(sn)) {
	 ch->pcdata->next_bmtrain = current_time+43200; /* Once per day */
	 if (IS_CODER(ch)) {
	   sprintf(buf, "[ {YCoder {x]: Train MF current time =  <%ld> next_bmtrain = <%ld>\n\r", current_time, ch->pcdata->next_bmtrain);
	   send_to_char(buf, ch);
	 }

	 // If BM and BM trained to last level that the trainer can train
	 // and higher than 90:
	 // - Give him/her the heron-marked sword.
	 // TODO: get weapon VNUM from table
	 if (is_masterform(sn) && ch->pcdata->learned[sn] > 90 && ch->pcdata->learned[sn] == trainer->train_level) {
	   sword = create_object(get_obj_index(get_mf_weapon(sn)), 0);
	   act("$N sheathes his sword and says, '{7Our training has come to an end, my friend. I have nothing more to teach you in this art.{x'",ch,NULL,trainer,TO_CHAR);
	   act("$N continues, '{7I would like you to have the $p to aid you in your tasks. It's made by my master, Ichymojo the great. Serve well.{x'", ch,sword,trainer,TO_CHAR);
	   if (sword != NULL) {
		act("$N studies $p for a moment before $E gives it to you!",  ch,sword,trainer,TO_CHAR);
		obj_to_char(sword, ch);
	   }

	   // Log it
	   sprintf(buf, "$N trained MF to %d and was given the $p.", ch->pcdata->learned[sn]);
	   wiznet(buf, ch, sword, WIZ_LEVELS, 0, get_trust(ch));
	   sprintf(buf, "%s trained MF to %d and was given the %s.", ch->name, ch->pcdata->learned[sn], sword->short_descr);
	   log_string(buf);
	   
	   return;
	 }

	 /* Once this man trains a BM he move to another place */
	 room = get_random_room(trainer, FALSE);
	 if (room != NULL) {
	   act("$N sheathes his sword and says before leaving, '{7It is time for me to bring my services elsewhere.{x'",ch,NULL,trainer,TO_CHAR);
	   char_from_room(trainer);
	   char_to_room(trainer, room);
	 }
    }
           
    return;
  }
  
  act("$N tells you '{7I do not understand...{x'",ch,NULL,trainer,TO_CHAR);
}

// Check if a char already know a masterform
bool can_masterforms(CHAR_DATA *ch)
{
   bool ret=FALSE;
	
   if (IS_NPC(ch))
      return ret;
   
   if (ch->pcdata->learned[gsn_blademaster] > 0) {
      ret=TRUE;   	
   }
   else if (ch->pcdata->learned[gsn_speardancer] > 0) {
      ret=TRUE;
   }
   else if (ch->pcdata->learned[gsn_duelling] > 0) {
      ret=TRUE;
   }
   else
      ret=FALSE;

   //printf("## can_master <%s>\n", ret == TRUE ? "TRUE" : "FALSE");

   return ret;
}

/*
 * New TSW gain :: Swordfish 
 */
void do_gain(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *trainer;
  int sn = 0;
  
  if (IS_NPC(ch))
    return;

  if (IS_FACELESS(ch))
  {
        send_to_char("Faceless characters are temporary and do not need more skills than those given at creation.\r\n",ch);
        return;
  }

  buf[0] = '\0';
  
  /* find a trainer */
  for ( trainer = ch->in_room->people; 
	   trainer != NULL; 
	   trainer = trainer->next_in_room)
    if (IS_NPC(trainer) && IS_SET(trainer->act,ACT_GAIN))
	 break;
  
  if (trainer == NULL || !can_see(ch,trainer)) {
    send_to_char("You can't do that here.\n\r",ch);
    return;
  }
  
  one_argument(argument,arg);
  
  if (arg[0] == '\0') {
    do_function(trainer, &do_say, "Pardon me?");
    return;
  }
  
  if (!str_prefix(arg,"list")) {
    int col=0;

    sprintf(buf, "%-18s%4s   %-18s%4s   %-18s%4s\n\r",
		  "skill/weave","train","skill/weave","train","skill/weave","train");
    send_to_char(buf,ch);
    
    for (sn = 0; sn < MAX_SKILL; sn++) {
	 if (skill_table[sn].name == NULL)
	   break;
	 
	 if (!ch->pcdata->learned[sn]
		&&  skill_table[sn].rating[ch->class] > 0
		&&  skill_table[sn].restriction != RES_NOGAIN
		&&  skill_table[sn].restriction != RES_GRANTED
		&&  skill_table[sn].skill_level[ch->class] > 0
		&&  ch->level >= skill_table[sn].skill_level[ch->class]
		&&  skill_table[sn].skill_level[ch->class] <= trainer->level) {

	   if (!CAN_CHANNEL(ch) && skill_table[sn].spell_fun != spell_null)
		continue;
	   
	   if (!IS_SET(trainer->gain_flags, GAIN_WEAVE) 
		  && (skill_table[sn].spell_fun != spell_null))		
		continue;
		
	   if (IS_SET(trainer->gain_flags, GAIN_WEAVE)
		  && (skill_table[sn].spell_fun != spell_null)
		  && ch->sex != trainer ->sex)
		continue;

	   if (!IS_SET(trainer->gain_flags, GAIN_SKILL)
		  && (skill_table[sn].spell_fun == spell_null)) {
		if (!is_masterform(sn))
		  continue;
	   }
	   
	   if (!IS_SET(trainer->gain_flags, GAIN_MASTERFORMS) &&
		is_masterform(sn))
	   	continue;
	   
	   if ((skill_table[sn].restriction == RES_MALE) && (ch->sex != SEX_MALE))
		continue;
	   
	   if ((skill_table[sn].restriction == RES_FEMALE) && (ch->sex != SEX_FEMALE))
		continue;
	   
	   /* Talent req not met? */
	   if (skill_table[sn].talent_req != 0)
		if (!IS_SET(ch->talents, skill_table[sn].talent_req))
		  continue;
	   
	   sprintf(buf,"%-18s %4d%s  ", skill_table[sn].name,skill_table[sn].rating[ch->class], 
			 skill_table[sn].talent_req != 0 ? "{CT{x" : " ");
	   send_to_char(buf,ch);
	   if (++col % 3 == 0)
		send_to_char("\n\r",ch);
	 }
    }
    if (col % 3 != 0)
	 send_to_char("\n\r",ch);
    if (buf[0] == '\0')
	 act("$N tells you '{7I have nothing more to teach you.{x'",
		ch,NULL,trainer,TO_CHAR);
    return;
  }
  
  /* else add a skill/weave */
  sn = skill_lookup(argument);

  if (sn > -1) {
     if (ch->sex != trainer->sex && skill_table[sn].spell_fun != spell_null && ch->class == CLASS_CHANNELER) {
	 if (ch->sex == SEX_MALE) {
	   act("$N gasp and look at you as $E says '{7You are a bloody male channeler! I will report you to the White Tower!{x'", ch,NULL,trainer,TO_CHAR);
	   make_mc_report(ch, trainer);
	   return;
	 }
	 else {
	   act("$N shake the head saying, '{7I can't teach a fish to fly.{x'", ch,NULL,trainer,TO_CHAR);
	   return;
	 }
     }

    
    if (ch->pcdata->learned[sn] == -1 ) {
	send_to_char("Your misbehavior is prohibiting this. Plead your case to the admin if you think you have a valid reason.\n\r",ch);
        return;
    }
    if (ch->pcdata->learned[sn]) {
	 act("$N tells you '{7You already know that skill!{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    if (skill_table[sn].rating[ch->class] <= 0) {
	 act("$N tells you '{7That skill is beyond your powers.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    if (ch->train < skill_table[sn].rating[ch->class]) {
	 act("$N tells you '{7You are not yet ready for that skill.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (ch->level < skill_table[sn].skill_level[ch->class]) {
	 act("$N tells you '{7You are not yet ready for that skill.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    if ( skill_table[sn].skill_level[ch->class] > trainer->level) {
    	act("$N tells you '{7I can't teach something that complex. It's beyond my powers.{x'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (!CAN_CHANNEL(ch) && skill_table[sn].spell_fun != spell_null) {
      act("$N tells you '{7I do not understand...{x'",ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    if (!IS_SET(trainer->gain_flags, GAIN_WEAVE) 
	   && (skill_table[sn].spell_fun != spell_null)) {
	 act("$N tells you '{7I do not understand...{x'",ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (!IS_SET(trainer->gain_flags, GAIN_SKILL) 
	   && (skill_table[sn].spell_fun == spell_null)) {
	 if (!is_masterform(sn)) {
	   act("$N tells you '{7I do not understand...{x'",ch,NULL,trainer,TO_CHAR);
	   return;
	 }
    }

    if (!IS_SET(trainer->gain_flags, GAIN_MASTERFORMS)
	   && is_masterform(sn)) {
	 act("$N tells you '{7Do I look like a weapons master?{x'",ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    if ((skill_table[sn].restriction == RES_MALE) && (ch->sex != SEX_MALE)) {
	 act("$N tells you '{7I do not understand...{x'",ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if ((skill_table[sn].restriction == RES_FEMALE) && (ch->sex != SEX_FEMALE)) {
	 act("$N tells you '{7I do not understand...{x'",ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (skill_table[sn].restriction == RES_NOGAIN) {
	 act("$N tells you '{7I can't teach you that!{x'",ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (skill_table[sn].talent_req != 0)
	 if (!IS_SET(ch->talents, skill_table[sn].talent_req)) {
	   act("$N tells you '{7You will not understand the complexity of this.{x'",ch,NULL,trainer,TO_CHAR);
	   return;
	 }

    /* Speardancer only for Aiels */
   /*
    if (sn == gsn_speardancer && ch->race != race_lookup("aiel")) {
       	act("$N tells you '{7This knowledge is kept among the Aiels.{x'",ch,NULL,trainer,TO_CHAR);
       	return;
    }

	*/

    /* Blademaster NOT for Trollocs/Ogiers */
/*
    if ((sn == gsn_blademaster && ch->race == race_lookup("trolloc")) ||
        (sn == gsn_blademaster && ch->race == race_lookup("ogier")))  {
        act("$N tells you '{7This is beyound your nature!{x'",ch,NULL,trainer,TO_CHAR);
       	return;        	
    }
*/
    
    /* Duelling NOT for Trollocs/Ogiers/Fades */
/*
    if ((sn == gsn_duelling && ch->race == race_lookup("trolloc")) ||
        (sn == gsn_duelling && ch->race == race_lookup("ogier")) ||
        (sn == gsn_duelling && ch->race == race_lookup("fade")))  {
        act("$N tells you '{7This is beyound your nature!{x'",ch,NULL,trainer,TO_CHAR);
       	return;        	
    }    
*/
    
    if (ch->race == race_lookup("trolloc") && is_masterform(sn))
    {
        act("$N tells you '{7This is beyond your nature!{x'",ch,NULL,trainer,TO_CHAR);
       	return;        	
    }

    if (ch->race == race_lookup("ogier") && is_masterform(sn) && sn != gsn_staffmaster && sn!= gsn_axemaster)
    {
        act("$N tells you '{7This is beyond your nature!{x'",ch,NULL,trainer,TO_CHAR);
       	return;        	
    }


/*
    if (((sn == gsn_blademaster) ||
         (sn == gsn_speardancer) ||
         (sn == gsn_duelling)) && (can_masterforms(ch) == TRUE)) {
       act("$N tells you, '{7You already know another master form!{x'", ch, NULL, trainer, TO_CHAR);
       return;    	
    }
*/

    /* add the skill */
    ch->pcdata->learned[sn] = 1;
    act("$N trains you in the art of $t", ch,skill_table[sn].name,trainer,TO_CHAR);
    ch->train -= skill_table[sn].rating[ch->class];

    /* Set timer if Masterform */
    if (is_masterform(sn)) {
	 ch->pcdata->next_bmtrain = current_time+43200; /* Once per day */
	 if (IS_CODER(ch)) {
	   sprintf(buf, "[ {YCoder {x]: Gain MF current time =  <%ld> next_bmtrain = <%ld>\n\r", current_time, ch->pcdata->next_bmtrain);
	   send_to_char(buf, ch);
	 }
         /* Once this man trains a BM he move to another place */
	 ROOM_INDEX_DATA * room;
         room = get_random_room(trainer, FALSE);
         if (room != NULL) {
           act("$N gets an evil glint in his eye and says before leaving, '{7It is time for me to bring my services elsewhere.{x'",ch,NULL,trainer,TO_CHAR);
           char_from_room(trainer);
           char_to_room(trainer, room);
         }

    }
    
    return;
  }
  
  act("$N tells you '{7I do not understand...{x'",ch,NULL,trainer,TO_CHAR);
}

/* used to get new skills */
void do_gain_OLDROM(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *trainer;
  int gn = 0, sn = 0;
  
  if (IS_NPC(ch))
    return;
  
  /* find a trainer */
  for ( trainer = ch->in_room->people; 
	   trainer != NULL; 
	   trainer = trainer->next_in_room)
    if (IS_NPC(trainer) && IS_SET(trainer->act,ACT_GAIN))
	 break;

  if (trainer == NULL || !can_see(ch,trainer)) {
    send_to_char("You can't do that here.\n\r",ch);
    return;
  }

  one_argument(argument,arg);

  if (arg[0] == '\0') {
    do_function(trainer, &do_say, "Pardon me?");
    return;
  }

  if (!str_prefix(arg,"list")) {
    int col=0;

    /*
    sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
		  "group","cost","group","cost","group","cost");
    send_to_char(buf,ch);
    */

    for (gn = 0; gn < MAX_GROUP; gn++) {
	 if (group_table[gn].name == NULL)
	   break;
	 
	 if (!ch->pcdata->group_known[gn]
		&&  group_table[gn].rating[ch->class] > 0) {
	   sprintf(buf,"%-18s %-5d ",
			 group_table[gn].name,group_table[gn].rating[ch->class]);
	   send_to_char(buf,ch);
	   if (++col % 3 == 0)
		send_to_char("\n\r",ch);
	 }
    }
    if (col % 3 != 0)
	 send_to_char("\n\r",ch);
	
    send_to_char("\n\r",ch);		
    
    col = 0;
    
    sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
		  "skill","cost","skill","cost","skill","cost");
    send_to_char(buf,ch);
    
    for (sn = 0; sn < MAX_SKILL; sn++) {
	 if (skill_table[sn].name == NULL)
	   break;
	 
	 if (!ch->pcdata->learned[sn]
		&&  skill_table[sn].rating[ch->class] > 0
		&&  skill_table[sn].spell_fun == spell_null) {
	   sprintf(buf,"%-18s %-5d ",
			 skill_table[sn].name,skill_table[sn].rating[ch->class]);
	   send_to_char(buf,ch);
	   if (++col % 3 == 0)
		send_to_char("\n\r",ch);
	 }
    }
    if (col % 3 != 0)
	 send_to_char("\n\r",ch);
    return;
  }
  
/*   if (!str_prefix(arg,"convert")) { */
/*     if (ch->practice < 10) { */
/* 	 act("$N tells you 'You are not yet ready.'", */
/* 		ch,NULL,trainer,TO_CHAR); */
/* 	 return; */
/*     } */

/*     act("$N helps you apply your practice to training", */
/* 	   ch,NULL,trainer,TO_CHAR); */
/*     ch->practice -= 10; */
/*     ch->train +=1 ; */
/*     return; */
/*   } */
  
  if (!str_prefix(arg,"points")) {
    if (ch->train < 2) {
	 act("$N tells you 'You are not yet ready.'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    if (ch->pcdata->points <= 40) {
	 act("$N tells you 'There would be no point in that.'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }

    act("$N trains you, and you feel more at ease with your skills.",
	   ch,NULL,trainer,TO_CHAR);
    
	ch->train -= 2;
	ch->pcdata->points -= 1;
	ch->exp = exp_per_level(ch,ch->pcdata->points) * ch->level;
	return;
  }
  
  /* else add a group/skill */
  
  gn = group_lookup(argument);
  if (gn > 0) {
    if (ch->pcdata->group_known[gn]) {
	 act("$N tells you 'You already know that group!'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }

    if (group_table[gn].rating[ch->class] <= 0) {
	 act("$N tells you 'That group is beyond your powers.'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    if (ch->train < group_table[gn].rating[ch->class]) {
	 act("$N tells you 'You are not yet ready for that group.'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    /* add the group */
    gn_add(ch,gn);
    act("$N trains you in the art of $t",
	   ch,group_table[gn].name,trainer,TO_CHAR);
    ch->train -= group_table[gn].rating[ch->class];
    return;
  }
  
  sn = skill_lookup(argument);
  if (sn > -1) {
    if (skill_table[sn].spell_fun != spell_null) {
	 act("$N tells you 'You must learn the full group.'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    
    if (ch->pcdata->learned[sn]) {
	 act("$N tells you 'You already know that skill!'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
    
    if (skill_table[sn].rating[ch->class] <= 0) {
	 act("$N tells you 'That skill is beyond your powers.'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
 
    if (ch->train < skill_table[sn].rating[ch->class]) {
	 act("$N tells you 'You are not yet ready for that skill.'",
		ch,NULL,trainer,TO_CHAR);
	 return;
    }
 
    /* add the skill */
    ch->pcdata->learned[sn] = 1;
    act("$N trains you in the art of $t",
	   ch,skill_table[sn].name,trainer,TO_CHAR);
    ch->train -= skill_table[sn].rating[ch->class];
    return;
  }
  
  act("$N tells you 'I do not understand...'",ch,NULL,trainer,TO_CHAR);
}
    



/* RT spells and skills show the players spells (or skills) */

void do_spells(CHAR_DATA *ch, char *argument)
{
  BUFFER *buffer;
  char arg[MAX_INPUT_LENGTH];
  char spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
  char spell_columns[LEVEL_HERO + 1];
  int sn, level, min_lev = 1, max_lev = LEVEL_HERO;
  bool fAll = FALSE, found = FALSE;
  char buf[MAX_STRING_LENGTH];
  
  if (IS_NPC(ch))
    return;
  
  if (ch->class != CLASS_CHANNELER) {
    send_to_char("You cannot channel the true source.\n\r", ch);
    return;
  }
  
  if (argument[0] != '\0') {
    fAll = TRUE;

    if (str_prefix(argument,"all")) {
	 argument = one_argument(argument,arg);
	 if (!is_number(arg)) {
	   send_to_char("Arguments must be numerical or all.\n\r",ch);
	   return;
	 }
	 max_lev = atoi(arg);
	 
	 if (max_lev < 1 || max_lev > LEVEL_HERO) {
	   sprintf(buf,"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
	   send_to_char(buf,ch);
	   return;
	 }

	 if (argument[0] != '\0') {
	   argument = one_argument(argument,arg);
	   if (!is_number(arg)) {
		send_to_char("Arguments must be numerical or all.\n\r",ch);
		return;
	   }
	   min_lev = max_lev;
	   max_lev = atoi(arg);
	   
	   if (max_lev < 1 || max_lev > LEVEL_HERO) {
		sprintf(buf, "Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		send_to_char(buf,ch);
		return;
	   }
	   
	   if (min_lev > max_lev) {
		send_to_char("That would be silly.\n\r",ch);
		return;
	   }
	 }
    }
  }


  /* initialize data */
  for (level = 0; level < LEVEL_HERO + 1; level++) {
    spell_columns[level] = 0;
    spell_list[level][0] = '\0';
  }
  
  for (sn = 0; sn < MAX_SKILL; sn++) {
    if (skill_table[sn].name == NULL )
	 break;
    
    if ((level = skill_table[sn].skill_level[ch->class]) < LEVEL_HERO + 1
	   &&  (fAll || level <= ch->level)
	   &&  level >= min_lev && level <= max_lev
	   &&  skill_table[sn].spell_fun != spell_null
	   &&  ch->pcdata->learned[sn] > 0) {
	 found = TRUE;
	 level = skill_table[sn].skill_level[ch->class];
	 if (ch->level < level)
	   sprintf(buf,"%-18s   n/a         ", skill_table[sn].name);
	 else {
	   sprintf(buf,"%-18s  %3d (%s)%s     ",skill_table[sn].name,
			 ch->pcdata->learned[sn],
			 skill_table[sn].restriction == RES_NORMAL  ? "{gt{x" : 
			 skill_table[sn].restriction == RES_GRANTED ? "{Wg{x" : 
			 skill_table[sn].restriction == RES_MALE    ? "{Bm{x" :
			 skill_table[sn].restriction == RES_FEMALE  ? "{rf{x" : "{rn{x",
			 skill_table[sn].talent_req != 0 ? "{C!{x" : "{C {x");
	 }
	 
	 if (spell_list[level][0] == '\0')
	   sprintf(spell_list[level],"\n\rLevel %2d: %s",level,buf);
	 else /* append */ {
	   if ( ++spell_columns[level] % 2 == 0)
		strcat(spell_list[level],"\n\r          ");
	   strcat(spell_list[level],buf);
	 }
    }
  }
  
  /* return results */
  
  if (!found) {
    send_to_char("No weaves found.\n\r",ch);
    return;
  }
  
  buffer = new_buf();
  for (level = 0; level < LEVEL_HERO + 1; level++)
    if (spell_list[level][0] != '\0')
	 add_buf(buffer,spell_list[level]);
  add_buf(buffer,"\n\r");
  page_to_char(buf_string(buffer),ch);
  free_buf(buffer);
}

void do_spells_OLD(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    char arg[MAX_INPUT_LENGTH];
    char spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char spell_columns[LEVEL_HERO + 1];
    int sn, level, min_lev = 1, max_lev = LEVEL_HERO, endurance;
    bool fAll = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH];
 
    if (IS_NPC(ch))
      return;

    if (argument[0] != '\0')
    {
	fAll = TRUE;

	if (str_prefix(argument,"all"))
	{
	    argument = one_argument(argument,arg);
	    if (!is_number(arg))
	    {
		send_to_char("Arguments must be numerical or all.\n\r",ch);
		return;
	    }
	    max_lev = atoi(arg);

	    if (max_lev < 1 || max_lev > LEVEL_HERO)
	    {
		sprintf(buf,"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		send_to_char(buf,ch);
		return;
	    }

	    if (argument[0] != '\0')
	    {
		argument = one_argument(argument,arg);
		if (!is_number(arg))
		{
		    send_to_char("Arguments must be numerical or all.\n\r",ch);
		    return;
		}
		min_lev = max_lev;
		max_lev = atoi(arg);

		if (max_lev < 1 || max_lev > LEVEL_HERO)
		{
		    sprintf(buf,
			"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		    send_to_char(buf,ch);
		    return;
		}

		if (min_lev > max_lev)
		{
		    send_to_char("That would be silly.\n\r",ch);
		    return;
		}
	    }
	}
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

	if ((level = skill_table[sn].skill_level[ch->class]) < LEVEL_HERO + 1
	&&  (fAll || level <= ch->level)
	&&  level >= min_lev && level <= max_lev
	&&  skill_table[sn].spell_fun != spell_null
	&&  ch->pcdata->learned[sn] > 0)
        {
	    found = TRUE;
	    level = skill_table[sn].skill_level[ch->class];
	    if (ch->level < level)
	    	sprintf(buf,"%-18s n/a      ", skill_table[sn].name);
	    else
	    {
		endurance = UMAX(skill_table[sn].min_endurance,
		    100/(2 + ch->level - level));
	        sprintf(buf,"%-18s  %3d endurance  ",skill_table[sn].name,endurance);
	    }
 
	    if (spell_list[level][0] == '\0')
          	sprintf(spell_list[level],"\n\rLevel %2d: %s",level,buf);
	    else /* append */
	    {
          	if ( ++spell_columns[level] % 2 == 0)
		    strcat(spell_list[level],"\n\r          ");
          	strcat(spell_list[level],buf);
	    }
	}
    }
 
    /* return results */
 
    if (!found)
    {
      	send_to_char("No spells found.\n\r",ch);
      	return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
      	if (spell_list[level][0] != '\0')
	    add_buf(buffer,spell_list[level]);
    add_buf(buffer,"\n\r");
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
}

void do_skills(CHAR_DATA *ch, char *argument)
{
  BUFFER *buffer;
  char arg[MAX_INPUT_LENGTH];
  char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
  char skill_columns[LEVEL_HERO + 1];
  int sn, level, min_lev = 1, max_lev = LEVEL_HERO;
  bool fAll = FALSE, found = FALSE;
  char buf[MAX_STRING_LENGTH];
  
  if (IS_NPC(ch))
    return;
  
  if (argument[0] != '\0') {
    fAll = TRUE;
    
    if (str_prefix(argument,"all")) {
	 argument = one_argument(argument,arg);
	 if (!is_number(arg)) {
	   send_to_char("Arguments must be numerical or all.\n\r",ch);
	   return;
	 }
	 max_lev = atoi(arg);

	 if (max_lev < 1 || max_lev > LEVEL_HERO) {
	   sprintf(buf,"Levels must be between 1 and %d.\n\r",LEVEL_HERO);
	   send_to_char(buf,ch);
	   return;
	 }

	 if (argument[0] != '\0') {
	   argument = one_argument(argument,arg);
	   if (!is_number(arg)) {
		send_to_char("Arguments must be numerical or all.\n\r",ch);
		return;
	   }
	   min_lev = max_lev;
	   max_lev = atoi(arg);
	   
	   if (max_lev < 1 || max_lev > LEVEL_HERO) {
		sprintf(buf, "Levels must be between 1 and %d.\n\r",LEVEL_HERO);
		send_to_char(buf,ch);
		return;
	   }
	   
	   if (min_lev > max_lev) {
		send_to_char("That would be silly.\n\r",ch);
		return;
	   }
	 }
    }
  }
  

  /* initialize data */
  for (level = 0; level < LEVEL_HERO + 1; level++) {
    skill_columns[level] = 0;
    skill_list[level][0] = '\0';
  }
  
  for (sn = 0; sn < MAX_SKILL; sn++) {
    if (skill_table[sn].name == NULL )
	 break;
    
    if ((level = skill_table[sn].skill_level[ch->class]) < LEVEL_HERO + 1
	   &&  (fAll || level <= ch->level)
	   &&  level >= min_lev && level <= max_lev
	   &&  skill_table[sn].spell_fun == spell_null
	   &&  ch->pcdata->learned[sn] > 0) {
	 found = TRUE;
	 level = skill_table[sn].skill_level[ch->class];
	 if (ch->level < level)
	   sprintf(buf,"%-18s   n/a        ", skill_table[sn].name);
	 else
	   sprintf(buf,"%-18s %3d (%s)%s     ",skill_table[sn].name,
			 ch->pcdata->learned[sn],
			 skill_table[sn].restriction == RES_NORMAL  ? "{gt{x" : 
			 skill_table[sn].restriction == RES_GRANTED ? "{Wg{x" : 
			 skill_table[sn].restriction == RES_MALE    ? "{Bm{x" :
			 skill_table[sn].restriction == RES_FEMALE  ? "{rf{x" : "{rn{x",
			 skill_table[sn].talent_req != 0 ? "{C!{x" : "{C {x");
	 
	 if (skill_list[level][0] == '\0')
	   sprintf(skill_list[level],"\n\rLevel %2d: %s",level,buf);
	 else {
	   if ( ++skill_columns[level] % 2 == 0)
		strcat(skill_list[level],"\n\r          ");
	   strcat(skill_list[level],buf);
	 }
    }
  }
  
  /* return results */
 
  if (!found) {
    send_to_char("No skills found.\n\r",ch);
    return;
  }
  
  buffer = new_buf();
  for (level = 0; level < LEVEL_HERO + 1; level++)
    if (skill_list[level][0] != '\0')
	 add_buf(buffer,skill_list[level]);
  add_buf(buffer,"\n\r");
  page_to_char(buf_string(buffer),ch);
  free_buf(buffer);
}

/* shows skills, groups and costs (only if not bought) */
void list_group_costs(CHAR_DATA *ch)
{
    char buf[100];
    int gn,sn,col;

    if (IS_NPC(ch))
	return;

    col = 0;

    sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s\n\r","group","cp","group","cp","group","cp");
    send_to_char(buf,ch);

    for (gn = 0; gn < MAX_GROUP; gn++)
    {
	if (group_table[gn].name == NULL)
	    break;

        if (!ch->gen_data->group_chosen[gn] 
	&&  !ch->pcdata->group_known[gn]
	&&  group_table[gn].rating[ch->class] > 0)
	{
	    sprintf(buf,"%-18s %-5d ",group_table[gn].name,
				    group_table[gn].rating[ch->class]);
	    send_to_char(buf,ch);
	    if (++col % 3 == 0)
		send_to_char("\n\r",ch);
	}
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);

    col = 0;
 
    sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s\n\r","skill","cp","skill","cp","skill","cp");
    send_to_char(buf,ch);
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL)
            break;
 
        if (!ch->gen_data->skill_chosen[sn] 
	&&  ch->pcdata->learned[sn] == 0
	&&  skill_table[sn].spell_fun == spell_null
	&&  skill_table[sn].rating[ch->class] > 0)
        {
            sprintf(buf,"%-18s %-5d ",skill_table[sn].name,
                                    skill_table[sn].rating[ch->class]);
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);

    sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
    send_to_char(buf,ch);
    sprintf(buf,"Experience per level: %d\n\r",
	    exp_per_level(ch,ch->gen_data->points_chosen));
    send_to_char(buf,ch);
    return;
}


void list_group_chosen(CHAR_DATA *ch)
{
    char buf[100];
    int gn,sn,col;
 
    if (IS_NPC(ch))
        return;
 
    col = 0;
 
    sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s","group","cp","group","cp","group","cp\n\r");
    send_to_char(buf,ch);
 
    for (gn = 0; gn < MAX_GROUP; gn++)
    {
        if (group_table[gn].name == NULL)
            break;
 
        if (ch->gen_data->group_chosen[gn] 
	&&  group_table[gn].rating[ch->class] > 0)
        {
            sprintf(buf,"%-18s %-5d ",group_table[gn].name,
                                    group_table[gn].rating[ch->class]);
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);
 
    col = 0;
 
    sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s","skill","cp","skill","cp","skill","cp\n\r");
    send_to_char(buf,ch);
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL)
            break;
 
        if (ch->gen_data->skill_chosen[sn] 
	&&  skill_table[sn].rating[ch->class] > 0)
        {
            sprintf(buf,"%-18s %-5d ",skill_table[sn].name,
                                    skill_table[sn].rating[ch->class]);
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);
 
    sprintf(buf,"Creation points: %d\n\r",ch->gen_data->points_chosen);
    send_to_char(buf,ch);
    sprintf(buf,"Experience per level: %d\n\r",
	    exp_per_level(ch,ch->gen_data->points_chosen));
    send_to_char(buf,ch);
    return;
}

/* Needed exp for a CHAR to level this level */
long exp_next_level(CHAR_DATA *ch)
{
  long levelExp=0;
  
  //Redone... this is the old way
  //levelExp = ((get_level(ch)*get_level(ch))*3)*100;

  //and this is the new one.. constant 2000 xp per level
  levelExp = 2000L;
  
		    
  return(levelExp);
}

int exp_per_level(CHAR_DATA *ch, int points)
{
    int expl,inc;

    if (IS_NPC(ch))
	return 1000; 

    expl = 1000;
    inc = 500;

    if (points < 40)
#if defined(FIRST_BOOT)
	return 1000 * (pc_race_table[ch->race].class_mult[ch->class] ?
		       pc_race_table[ch->race].class_mult[ch->class]/100 : 1);
#else
	return 1000 * (race_table[ch->race].class_mult[ch->class] ?
		       race_table[ch->race].class_mult[ch->class]/100 : 1);
#endif

    /* processing */
    points -= 40;

    while (points > 9)
    {
	expl += inc;
        points -= 10;
        if (points > 9)
	{
	    expl += inc;
	    inc *= 2;
	    points -= 10;
	}
    }

    expl += points * inc / 10;  

#if defined(FIRST_BOOT)
    return expl * pc_race_table[ch->race].class_mult[ch->class]/100;
#else
    return expl * race_table[ch->race].class_mult[ch->class]/100;
#endif
}

/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups(CHAR_DATA *ch,char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int gn,sn,i;
 
    if (argument[0] == '\0')
	return FALSE;

    argument = one_argument(argument,arg);

    if (!str_prefix(arg,"help"))
    {
	if (argument[0] == '\0')
	{
	    do_function(ch, &do_help, "group help");
	    return TRUE;
	}

        do_function(ch, &do_help, argument);
	return TRUE;
    }

    if (!str_prefix(arg,"add"))
    {
	if (argument[0] == '\0')
	{
	    send_to_char("You must provide a skill name.\n\r",ch);
	    return TRUE;
	}

	gn = group_lookup(argument);
	if (gn != -1)
	{
	    if (ch->gen_data->group_chosen[gn]
	    ||  ch->pcdata->group_known[gn])
	    {
		send_to_char("You already know that group!\n\r",ch);
		return TRUE;
	    }

	    if (group_table[gn].rating[ch->class] < 1)
	    {
	  	send_to_char("That group is not available.\n\r",ch);
	 	return TRUE;
	    }

	    /* Close security hole */
	    if (ch->gen_data->points_chosen + group_table[gn].rating[ch->class]
		> 300)
	    {
		send_to_char(
		    "You cannot take more than 300 creation points.\n\r", ch);
		return TRUE;
	    }

	    sprintf(buf,"%s group added\n\r",group_table[gn].name);
	    send_to_char(buf,ch);
	    ch->gen_data->group_chosen[gn] = TRUE;
	    ch->gen_data->points_chosen += group_table[gn].rating[ch->class];
	    gn_add(ch,gn);
	    ch->pcdata->points += group_table[gn].rating[ch->class];
	    return TRUE;
	}

	sn = skill_lookup(argument);
	if (sn != -1)
	{
	    if (ch->gen_data->skill_chosen[sn]
	    ||  ch->pcdata->learned[sn] > 0)
	    {
		send_to_char("You already know that skill!\n\r",ch);
		return TRUE;
	    }

	    if (skill_table[sn].rating[ch->class] < 1
	    ||  skill_table[sn].spell_fun != spell_null)
	    {
		send_to_char("That skill is not available.\n\r",ch);
		return TRUE;
	    }

	    /* Close security hole */
	    if (ch->gen_data->points_chosen + skill_table[sn].rating[ch->class]
		> 300)
	    {
		send_to_char(
		    "You cannot take more than 300 creation points.\n\r", ch);
		return TRUE;
	    }
	    sprintf(buf, "%s skill added\n\r",skill_table[sn].name);
	    send_to_char(buf,ch);
	    ch->gen_data->skill_chosen[sn] = TRUE;
	    ch->gen_data->points_chosen += skill_table[sn].rating[ch->class];
	    ch->pcdata->learned[sn] = 1;
	    ch->pcdata->points += skill_table[sn].rating[ch->class];
	    return TRUE;
	}

	send_to_char("No skills or groups by that name...\n\r",ch);
	return TRUE;
    }

    if (!strcmp(arg,"drop"))
    {
	if (argument[0] == '\0')
  	{
	    send_to_char("You must provide a skill to drop.\n\r",ch);
	    return TRUE;
	}

	gn = group_lookup(argument);
	if (gn != -1 && ch->gen_data->group_chosen[gn])
	{
	    send_to_char("Group dropped.\n\r",ch);
	    ch->gen_data->group_chosen[gn] = FALSE;
	    ch->gen_data->points_chosen -= group_table[gn].rating[ch->class];
	    gn_remove(ch,gn);
	    for (i = 0; i < MAX_GROUP; i++)
	    {
		if (ch->gen_data->group_chosen[gn])
		    gn_add(ch,gn);
	    }
	    ch->pcdata->points -= group_table[gn].rating[ch->class];
	    return TRUE;
	}

	sn = skill_lookup(argument);
	if (sn != -1 && ch->gen_data->skill_chosen[sn])
	{
	    send_to_char("Skill dropped.\n\r",ch);
	    ch->gen_data->skill_chosen[sn] = FALSE;
	    ch->gen_data->points_chosen -= skill_table[sn].rating[ch->class];
	    ch->pcdata->learned[sn] = 0;
	    ch->pcdata->points -= skill_table[sn].rating[ch->class];
	    return TRUE;
	}

	send_to_char("You haven't bought any such skill or group.\n\r",ch);
	return TRUE;
    }

    if (!str_prefix(arg,"premise"))
    {
	do_function(ch, &do_help, "premise");
	return TRUE;
    }

    if (!str_prefix(arg,"list"))
    {
	list_group_costs(ch);
	return TRUE;
    }

    if (!str_prefix(arg,"learned"))
    {
	list_group_chosen(ch);
	return TRUE;
    }

    if (!str_prefix(arg,"info"))
    {
	do_function(ch, &do_groups, argument);
	return TRUE;
    }

    return FALSE;
}

/* shows all groups, or the sub-members of a group */
void do_groups(CHAR_DATA *ch, char *argument)
{
    char buf[100];
    int gn,sn,col;

    if (IS_NPC(ch))
	return;

    col = 0;

    if (argument[0] == '\0')
    {   /* show all groups */
	
	for (gn = 0; gn < MAX_GROUP; gn++)
        {
	    if (group_table[gn].name == NULL)
		break;
	    if (ch->pcdata->group_known[gn])
	    {
		sprintf(buf,"%-20s ",group_table[gn].name);
		send_to_char(buf,ch);
		if (++col % 3 == 0)
		    send_to_char("\n\r",ch);
	    }
        }
        if ( col % 3 != 0 )
            send_to_char( "\n\r", ch );
        sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
	send_to_char(buf,ch);
	return;
     }

     if (!str_cmp(argument,"all"))    /* show all groups */
     {
        for (gn = 0; gn < MAX_GROUP; gn++)
        {
            if (group_table[gn].name == NULL)
                break;
	    sprintf(buf,"%-20s ",group_table[gn].name);
            send_to_char(buf,ch);
	    if (++col % 3 == 0)
            	send_to_char("\n\r",ch);
        }
        if ( col % 3 != 0 )
            send_to_char( "\n\r", ch );
	return;
     }
	
     
     /* show the sub-members of a group */
     gn = group_lookup(argument);
     if (gn == -1)
     {
	send_to_char("No group of that name exist.\n\r",ch);
	send_to_char(
	    "Type 'groups all' or 'info all' for a full listing.\n\r",ch);
	return;
     }

     for (sn = 0; sn < MAX_IN_GROUP; sn++)
     {
	if (group_table[gn].spells[sn] == NULL)
	    break;
	sprintf(buf,"%-20s ",group_table[gn].spells[sn]);
	send_to_char(buf,ch);
	if (++col % 3 == 0)
	    send_to_char("\n\r",ch);
     }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
}

/* checks for skill improvement */
void check_improve( CHAR_DATA *ch, int sn, bool success, int multiplier )
{
  int chance;
  char buf[100];
  int improve_max = 85;
  if (IS_DR(ch) || IS_FORSAKEN(ch))
  {
	improve_max=135;
  }
  
  if (IS_NPC(ch))
    return;

  /* Not while wolfshape */
  if (IS_WOLFSHAPE(ch))
    return;

  // Not for disguise
  if (sn == gsn_disguise)
    return;

  // Fastlearner -> learn a little more from usage
  // Slowlearner -> learn a little less from usage
  if (IS_SET(ch->merits, MERIT_FASTLEARNER)) {
     improve_max += 5;
  }
  
  if (IS_SET(ch->flaws, FLAW_SLOWLEARNER)) {
     improve_max -= 5;
  }
  
  if (ch->level < skill_table[sn].skill_level[ch->class]
	 //||  skill_table[sn].rating[ch->class] == 0
	 ||  ch->pcdata->learned[sn] <= 0
	 ||  ch->pcdata->learned[sn] >= improve_max) {
    return;
  }

  if (ch->pcdata->learned[sn] > 60 && number_percent() > 30)
    return;
  
  /* check to see if the character has a chance to learn */
  chance = 10 * int_app[get_curr_stat(ch,STAT_INT)].learn;
  /* chance /= (		multiplier */
  chance /= multiplier;
  chance += (multiplier * skill_table[sn].rating[ch->class] * 4);
  chance += ch->level;
  
  if (number_range(1,1000) > chance)
    return;
  
  /* now that the character has a CHANCE to learn, see if they really have */	
  
  if (success) {
    if (IS_DR(ch)  || IS_FORSAKEN(ch))
    {
	chance=50;
    }
   else
    {
       chance = URANGE(5,100 - ch->pcdata->learned[sn], 95);
    }
    if (number_percent() < chance) {
	 sprintf(buf,"{yWith training you gain new knowledge with your %s %s.{x\n\r",
		    skill_table[sn].name,
		    skill_table[sn].spell_fun == spell_null ? "skill" : "weave");
	 send_to_char(buf,ch);
	 ch->pcdata->learned[sn]++;
	 /* gain_exp(ch,2 * skill_table[sn].rating[ch->class]); */
    }
  }
  
  else {
    if (IS_DR(ch)  || IS_FORSAKEN(ch))
    {
	chance=50;
    }
   else
    {
       chance = URANGE(5,ch->pcdata->learned[sn]/2,30);
    }
    if (number_percent() < chance) {
	 sprintf(buf, "{yYou learn from your mistakes, and gain new knowledge with your %s %s.{x\n\r",
		    skill_table[sn].name,
		    skill_table[sn].spell_fun == spell_null ? "skill" : "weave");
	 send_to_char(buf,ch);
	 ch->pcdata->learned[sn] += number_range(1,3);
	 ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn],improve_max);
	 gain_exp(ch,2 * skill_table[sn].rating[ch->class]);
    }
  }
}

/* returns a group index number given the name */
int group_lookup( const char *name )
{
    int gn;
 
    for ( gn = 0; gn < MAX_GROUP; gn++ )
    {
        if ( group_table[gn].name == NULL )
            break;
        if ( LOWER(name[0]) == LOWER(group_table[gn].name[0])
        &&   !str_prefix( name, group_table[gn].name ) )
            return gn;
    }
 
    return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add( CHAR_DATA *ch, int gn)
{
    int i;
    
    ch->pcdata->group_known[gn] = TRUE;
    for ( i = 0; i < MAX_IN_GROUP; i++)
    {
        if (group_table[gn].spells[i] == NULL)
            break;
        group_add(ch,group_table[gn].spells[i],FALSE);
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove( CHAR_DATA *ch, int gn)
{
    int i;

    ch->pcdata->group_known[gn] = FALSE;

    for ( i = 0; i < MAX_IN_GROUP; i ++)
    {
	if (group_table[gn].spells[i] == NULL)
	    break;
	group_remove(ch,group_table[gn].spells[i]);
    }
}
	
/* use for processing a skill or group for addition  */
void group_add( CHAR_DATA *ch, const char *name, bool deduct)
{
    int sn,gn;

    if (IS_NPC(ch)) /* NPCs do not have skills */
	return;

    sn = skill_lookup(name);

    if (sn != -1)
    {
	if (ch->pcdata->learned[sn] == 0) /* i.e. not known */
	{
	    if (!IS_FACELESS(ch))
	    {
	    	ch->pcdata->learned[sn] = 1;
	    	if (deduct)
	   		ch->pcdata->points += skill_table[sn].rating[ch->class]; 
	    }
	    else
	    {
		ch->pcdata->learned[sn] = 66;	
	    }
	}
	return;
    }
	
    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1)
    {
	if (ch->pcdata->group_known[gn] == FALSE)  
	{
	    ch->pcdata->group_known[gn] = TRUE;
	    if (deduct)
		ch->pcdata->points += group_table[gn].rating[ch->class];
	}
	gn_add(ch,gn); /* make sure all skills in the group are known */
    }
}

/* used for processing a skill or group for deletion -- no points back! */
void group_remove(CHAR_DATA *ch, const char *name)
{
  int sn, gn;
    
  sn = skill_lookup(name);
  
  if (sn != -1) {
    ch->pcdata->learned[sn] = 0;
    return;
  }
  
  /* now check groups */
  gn = group_lookup(name);
  
  if (gn != -1 && ch->pcdata->group_known[gn] == TRUE) {
    ch->pcdata->group_known[gn] = FALSE;
    gn_remove(ch,gn);  /* be sure to call gn_add on all remaining groups */
  }
}

#if !defined(FIRST_BOOT)
int race_exp_per_level( int race, int class, int points )
{
    int expl,inc;

    expl = 1000;
    inc = 500;

    if (points < 40)
	return 1000 * (race_table[race].class_mult[class] ?
		       race_table[race].class_mult[class]/100 : 1);

    /* processing */
    points -= 40;

    while (points > 9)
    {
	expl += inc;
        points -= 10;
        if (points > 9)
	{
	    expl += inc;
	    inc *= 2;
	    points -= 10;
	}
    }

    expl += points * inc / 10;  

    return expl * race_table[race].class_mult[class]/100;
}
#endif

/**********************************************************************
*       Function      : do_teach()
*       Author        : Swordfish
*       Description   : Teach another player that is in your group
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_teach( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim = NULL;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int self_teach=0;
  int tr_nr=0;
  long exp_cost=0;
  int sn;
  
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  /* NPCs can't use this command */
  if (IS_NPC(ch))
    return;
  
  if (IS_FACELESS(ch))
  {
	send_to_char("Faceless characters are temporary and do not teach or need to be taught.\r\n",ch);
	return;
  }
  /* Validate syntax */
  if (IS_NULLSTR(arg1) || IS_NULLSTR(arg2)) {
    send_to_char("Syntax: teach <character> # <skill/weave>\n\r", ch);
    return;
  }

  if (( victim = get_char_world( ch, arg1 )) == NULL) {
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }
  
  /* Need to be grouped */
  if (!is_same_group(ch,victim) ) {
    send_to_char("They need to be in your group before you can teach them.\n\r", ch);
    return;
  }

  /* Well, no */
  if (IS_NPC(victim)) {
    send_to_char("You can't teach NPCs.\n\r", ch);
    return;
  }
  if (IS_FACELESS(victim))
  {
	send_to_char("Faceless characters are temporary and do not teach or need to be taught.\r\n",ch);
	return;
  }

  if (!IS_RP(ch))
  {
	send_to_char("You must be IC to teach.\n\r",ch);
	return;
  }

  if (!IS_RP(victim))
  {
	send_to_char("Your student must be IC to learn.\n\r",ch);
        return;
  }

  /* Is it a number ? */
  if (!(tr_nr = atoi(arg2))) {
    tr_nr = 1;
    strcpy(argument, arg2);
  }

  if (tr_nr < 1) {
    send_to_char("You can't teach negative values!\n\r", ch);
    return;
  }

  if ((sn = skill_lookup(argument)) < 1) {
    send_to_char("You sudden realize that you don't know what you are talking about.\n\r"
			  "You {Rblush{x and determine to stick to what you know from now.\n\r", ch);
    return;
  }
  
  if (ch->pcdata->learned[sn] < 1) {
    send_to_char("You sudden realize that you don't know what you are talking about.\n\r"
		  "You {Rblush{x and determine to stick to what you know from now.\n\r", ch);
    return;
  }

  /* Is the person being taught being punished and unable to learn the skill */
  if (victim->pcdata->learned[sn] == -1) {
	send_to_char("They can not learn that at this time.\n\r",ch);
	return;
  }
  /*-----------------------------------------------------------------------*/
  /* SELF TEACHING                                                         */
  /*-----------------------------------------------------------------------*/
  if (ch == victim) {
  	
    // Teach self disabled
    send_to_char("You can't teach your self, try seeking knowledge else.\n\r", ch);
    return;
  	
    self_teach = ((get_curr_stat(ch,STAT_WIS) + get_curr_stat(ch,STAT_INT)) * 2);

    if (IS_SET(ch->merits, MERIT_FASTLEARNER))
	 if (self_teach == MAX_PC_TRAIN)
	   self_teach += 1;
    
    if (tr_nr > ch->train) {
	 send_to_char("You don't have that many trains available.\n\r", ch);
	 return;
    }   

    if ((tr_nr + ch->pcdata->learned[sn]) > self_teach) {
	 send_to_char("You try but can't seem to learn anymore by your self.\n\r", ch);
	 return;
    }
    
    exp_cost = 0;
 
    if (IS_SET(ch->merits, MERIT_FASTLEARNER))
	 exp_cost = 50; //ch->pcdata->learned[sn] * skill_table[sn].skill_level[ch->class] * (ch->level/4) * tr_nr / 2;
    else if (IS_SET(ch->flaws, FLAW_SLOWLEARNER))
	 exp_cost = 200; //ch->pcdata->learned[sn] * skill_table[sn].skill_level[ch->class] * (ch->level/4) * tr_nr * 2;
    else
	 exp_cost = 100; //ch->pcdata->learned[sn] * skill_table[sn].skill_level[ch->class] * (ch->level/4) * tr_nr;

    if (ch->exp < exp_cost) {
	 send_to_char("You don't have that much experience points available.\n\r", ch);
	 return;
    }
    
    ch->pcdata->learned[sn] += tr_nr;
    ch->train               -= tr_nr;
    ch->exp                 -= exp_cost;

    sprintf(buf, "You train your self %d time%s in %s.\n\r",
		  tr_nr, 
		  tr_nr > 1 ? "s" : "",
		  skill_table[sn].name);
    send_to_char(buf, ch);
    
    sprintf(buf, "You have lost {r%ld{x experience points.\n\r", exp_cost);
    send_to_char(buf,ch);
    
    return;
  }

  /*-----------------------------------------------------------------------*/
  /* TEACH OTHER                                                           */
  /*-----------------------------------------------------------------------*/

  /* Is it a weave? */
  if (skill_table[sn].spell_fun != spell_null && victim->class != CLASS_CHANNELER) {
    send_to_char("They will not understand the complexity of weaving.\n\r", ch);
    return;
  }

/*
  if (skill_table[sn].rating[victim->class] <= 0 || skill_table[sn].skill_level[ch->class] <= 0) {
    send_to_char("They will not understand the complexity of this.\n\r", ch);
    return;
  }
*/
  
  /* Perhaps victim is better learned ? */
  if ((ch->pcdata->learned[sn]-1 <= victim->pcdata->learned[sn]) ||
      (victim->pcdata->learned[sn]>=105)) {
    sprintf(buf, "You have nothing more to teach %s\n\r", PERS(victim, ch));
    send_to_char(buf, ch);
    return;
  }

  /* Restrictions */
  if (skill_table[sn].restriction != RES_NORMAL) {
    if (skill_table[sn].restriction == RES_NOTEACH ||
/*	   skill_table[sn].restriction == RES_GRANTED || */
	   skill_table[sn].restriction == RES_TALENT)
		{
	 send_to_char("This is beyond their powers.\n\r", ch);
	 return;
    }
    if (skill_table[sn].restriction == RES_FEMALE && victim->sex != SEX_FEMALE) {
	 send_to_char("You can't teach a fish to fly.\n\r", ch);
	 return;
    }
    if (skill_table[sn].restriction == RES_MALE && victim->sex != SEX_MALE) {
	 send_to_char("You can't teach a fish to fly.\n\r", ch);
	 return;
    }
    if (skill_table[sn].restriction == RES_TRAINSAMESEX && victim->sex != ch->sex) {
	 send_to_char("You can't teach a fish to fly nor a bird to swim.\n\r", ch);
	 return;
    }
    if (skill_table[sn].restriction == RES_NOTRAIN) {
    	send_to_char("You can't teach this.\n\r", ch);
	return;
    }
  }

  if (skill_table[sn].spell_fun != spell_null && victim->sex != ch->sex) {
    send_to_char("You can't teach a fish to fly.\n\r", ch);
    return;
  }

  if (victim->level < skill_table[sn].skill_level[victim->class]) {
    sprintf(buf, "%s is not yet ready for learning %s.\n\r", PERS(victim, ch), skill_table[sn].name);
    send_to_char(buf, ch);
    return;
  }

  /* Masterform */
  if (is_masterform(sn)) {
    tr_nr = 1;
    if (victim->pcdata->next_bmtrain > current_time) {
	 sprintf(buf, "It is to soon for %s to proceed with the training.\n\r",  PERS(victim, ch));
	 send_to_char(buf, ch);
	 return;
    }
    
    // Can't gain it from PC, only train
    if (victim->pcdata->learned[sn] < 1) {
	 sprintf(buf, "You can't teach %s the first basics of the mastery of that weapon.\n\r",  PERS(victim, ch));
	 send_to_char(buf, ch);
	 return;
    }    
  }
  
  /* Victim have enough trains left? */
  if (tr_nr > victim->train) {
    sprintf(buf, "%s has only %d trains left.\n\r", PERS(victim, ch), victim->train);
    send_to_char(buf, ch);
    return;
  }

//JAS
    exp_cost = 0;

    if (IS_SET(victim->merits, MERIT_FASTLEARNER))
         exp_cost = 50; //ch->pcdata->learned[sn] * skill_table[sn].skill_level[ch->class] * (ch->level/4) * tr_nr / 2;
    else if (IS_SET(victim->flaws, FLAW_SLOWLEARNER))
         exp_cost = 200; //ch->pcdata->learned[sn] * skill_table[sn].skill_level[ch->class] * (ch->level/4) * tr_nr * 2;
    else
         exp_cost = 100; //ch->pcdata->learned[sn] * skill_table[sn].skill_level[ch->class] * (ch->level/4) * tr_nr;


  if ((tr_nr + victim->pcdata->learned[sn]) >= ch->pcdata->learned[sn]-1) {
    tr_nr = (ch->pcdata->learned[sn] - 1) - victim->pcdata->learned[sn];
  }

/*
  if (victim->pcdata->learned[sn] < 1) {
    exp_cost = UMAX(1, skill_table[sn].rating[ch->class]) * skill_table[sn].skill_level[ch->class] * (ch->level/4) * tr_nr;
  }
  else {
    exp_cost = victim->pcdata->learned[sn] * skill_table[sn].skill_level[ch->class] * (ch->level/4) * tr_nr;
  }

*/
  /* Master for cost */
  if (is_masterform(sn)) {
    exp_cost = exp_cost*10;
  }
  
  if (exp_cost > victim->exp) {
    sprintf(buf, "You need {W%ld{x experience points to be able to learn that.\n\r", exp_cost);
    send_to_char(buf, victim);
    return;
  }
  
  /* Do the math */
  if ((victim->pcdata->learned[sn]  + tr_nr)  > 105) {
	send_to_char("You can not teach them to that point.\n\r",ch);
	return;
  }
  victim->pcdata->learned[sn] += tr_nr;
  victim->train               -= tr_nr;
  victim->exp                     -= exp_cost;

  /* Set timer if Masterform */
  if (is_masterform(sn)) {
    victim->pcdata->next_bmtrain = current_time+43200; /* Once per day */
    
    if (IS_CODER(ch)) {
	 sprintf(buf, "[ {YCoder {x]: Teach MF current time =  <%ld> next_bmtrain = <%ld>\n\r", current_time, victim->pcdata->next_bmtrain);
	 send_to_char(buf, ch);
    }
  }
  
  /* Tell victim about it */
  sprintf(buf, "%s teach you %d time%s in %s.\n\r", 
		PERS(ch, victim), 
		tr_nr, 
		tr_nr > 1 ? "s" : "",
		skill_table[sn].name);
  send_to_char(buf, victim);

  /* Tell your self about it, you could have forgott it already */
  sprintf(buf, "You teach %s %d time%s in %s.\n\r", 
		PERS(victim, ch), 
		tr_nr, 
		tr_nr > 1 ? "s" : "",
		skill_table[sn].name);
  send_to_char(buf, ch);

  sprintf(buf, "You have used {r%ld{x experience points.\n\r", exp_cost);
  send_to_char(buf,victim);
  
  return;
}

void do_slist(CHAR_DATA *ch, char *argument)
{
  BUFFER *buffer;
  struct skill_type local_skill_table[MAX_SKILL];
  int sn;
  char buf[MAX_STRING_LENGTH];
  char tmpbuf[MAX_STRING_LENGTH];
  int class;
  
  if (IS_NPC(ch))
    return;
  
  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  /* Copy skill_table into a local skill table for sort */
  for (sn = 0; sn < MAX_SKILL; sn++)
    local_skill_table[sn] = skill_table[sn];
  
  qsort (local_skill_table, sn, sizeof(struct skill_type), compare_skill_names);
  
  buffer = new_buf();
  
  sprintf(buf, "%-18s          {Y%-4s    {R%-4s    {B%-4s{x\n", "{cSkill{x/{WWeave{x", "Cha", "Thi", "War");
  add_buf(buffer, buf);  
  sprintf(buf, "-----------------------------------------\n");
  add_buf(buffer, buf);

  for (sn = 0; sn < MAX_SKILL; sn++) {
    if (local_skill_table[sn].name[0] == '\0' )
	 continue;
    
    sprintf(buf, "%s%-18s{x ", 
		  local_skill_table[sn].spell_fun != spell_null ? "{W" : "{c",
		  local_skill_table[sn].name);
    
    for (class = 0; class < MAX_CLASS; class++) {
	 sprintf(tmpbuf, "%s%4d{x/%-2d ", 
		    class == 0 ? "{Y" : class == 1 ? "{R" : class == 2 ? "{B" : "", 
		    local_skill_table[sn].skill_level[class],
		    local_skill_table[sn].rating[class]);
	 strcat(buf, tmpbuf);
    }
    
    strcat(buf, "\n");
    add_buf(buffer, buf);
  }
  
  page_to_char(buf_string(buffer),ch);
  free_buf(buffer);  
  
  return;
}

int get_males_in_link(CHAR_DATA *ch)
{
  int maleCnt=0;
  LINK_DATA * pLinked=NULL;
  
  if (!IS_LINKED(ch))
    return 0;
  
  if (ch->link_info == NULL)
    return 0;
  
  pLinked = ch->link_info;
  
  do {
    if (pLinked->linked->sex == SEX_MALE)
	 maleCnt++;
    
    pLinked = pLinked->next;
  } while (pLinked);
  
  return maleCnt;
}

int get_females_in_link(CHAR_DATA *ch)
{
  int femaleCnt=0;
  LINK_DATA * pLinked=NULL;
  
  if (!IS_LINKED(ch))
    return 0;
  
  if (ch->link_info == NULL)
    return 0;
  
  pLinked = ch->link_info;
  
  do {
    if (pLinked->linked->sex == SEX_FEMALE)
	 femaleCnt++;
    
    pLinked = pLinked->next;
  } while (pLinked);
  
  return femaleCnt;
}

void do_link(CHAR_DATA *ch, char *argument) {
  char arg[MAX_STRING_LENGTH];
  CHAR_DATA * victim;
  AFFECT_DATA afLeaderAir;
  AFFECT_DATA afLeaderWater;
  AFFECT_DATA afLeaderFire;
  AFFECT_DATA afLeaderEarth;
  AFFECT_DATA afLeaderSpirit;
  AFFECT_DATA afVictimAir;
  AFFECT_DATA afVictimWater;
  AFFECT_DATA afVictimFire;
  AFFECT_DATA afVictimEarth;
  AFFECT_DATA afVictimSpirit;
  int sn;
  LINK_DATA * pNewLink=NULL;

    if (IS_NPC(ch))
       return;

/*
    if (!IS_RP(ch))
    {
	send_to_char("You must be IC to link.\n\r",ch);
	return;
    }
*/
    if ( !IS_NPC(ch)
    &&   ch->level < skill_table[gsn_link].skill_level[ch->class] )
    {
	send_to_char(
	    "Are you sure you know what you're doing?\n\r", ch );
	return;
    }
    if ( get_skill(ch,gsn_link) > number_percent())
    {
	check_improve(ch,gsn_link,TRUE,1);
    }
    else 
    {
	check_improve(ch,gsn_link,FALSE,1);
	send_to_char("You really need to work on that..\n\r",ch);
	return;
    }
  /* Need to be able to hold OP to LINK*/
  if (!IS_AFFECTED(ch, AFF_CHANNELING)) {
    do_function(ch, &do_channel, "");
    if (!IS_AFFECTED(ch ,AFF_CHANNELING))
	 return;
  }

  sn = skill_lookup("link");

  argument = one_argument (argument, arg);
  if (arg[0] == '\0') 
  {
	if (ch->bIsReadyForLink)
        {
		send_to_char("You are no longer ready to link!\n\r",ch);
        }
        else
        {
		send_to_char("You are now ready to link!\n\r",ch);
        }
	ch->bIsReadyForLink = !ch->bIsReadyForLink;
        return;
  }
  if ( ( victim = get_char_room( ch, arg ) ) == NULL ) { 
	 send_to_char( "They aren't here.\n\r",              ch ); 
	 return; 
  }

  if (victim == ch) {
     send_to_char("You can't link with your self.\n\r", ch);
     return;  	
  }
  
  if (IS_NPC(victim)) {
     send_to_char("You can't link with mobiles.\n\r", ch);
     return;  	  	
  }
  
  if (!is_same_group(ch,victim))  
  {   
	send_to_char("You must be grouped with them!\n\r",ch);
        return;
  }
  
  if (!IS_RP(victim))
  {
      send_to_char("They must be IC to link.\n\r",ch);
      return;
  }
  if (!victim->bIsReadyForLink)
  {
	send_to_char("They are not prepared for linking!\n\r",ch);
        return;
  }
  
  if (victim->bIsLinked) 
  {
	send_to_char("They are already linked!\n\r",ch);
  	return;
  } 

  if (!IS_AFFECTED(ch, AFF_CHANNELING)) {
    do_function(ch, &do_channel, "");
    if (!IS_AFFECTED(ch ,AFF_CHANNELING))
	 return;
  }
  if (!IS_AFFECTED(victim, AFF_CHANNELING)) {
    do_function(victim, &do_channel, "");
    if (!IS_AFFECTED(victim ,AFF_CHANNELING)) {
	send_to_char("They can't channel at the moment.\n\r",ch);	
	 return;
    }
  }
  if (!check_can_link(ch,victim)) 
  {
	send_to_char("The laws associated with linking prevent this from happening\n\r",ch);
        return;	
  }
  if (ch->link_info) {
	fprintf(stderr,"Link: Already have link_info allocated\n");
  	pNewLink = new_link_info();
  	pNewLink->linked = victim;
  	pNewLink->next = ch->link_info;
  	ch->link_info = pNewLink;
  }
  else
  {
	fprintf(stderr,"Link:  Creating new link_info\n");
	pNewLink = new_link_info();
	ch->link_info = new_link_info();
	ch->link_info->linked = victim;
	ch->link_info->next = pNewLink;
	pNewLink->linked = ch;
	pNewLink->next   = NULL;
  }

  victim->pIsLinkedBy = ch;
  

  afLeaderAir.where     = TO_AFFECTS;
  afLeaderAir.casterId  = victim->id;
  afLeaderAir.type      = sn;
  afLeaderAir.level     = ch->level;
  afLeaderAir.duration  = SUSTAIN_WEAVE;
  afLeaderAir.location  = APPLY_SPHERE_AIR;
  afLeaderAir.modifier  = victim->perm_sphere[SPHERE_AIR];
  afLeaderAir.bitvector = AFF_LINKED;
  affect_to_char(ch,&afLeaderAir);

  afLeaderWater.where     = TO_AFFECTS;
  afLeaderWater.casterId  = victim->id;
  afLeaderWater.type      = sn;
  afLeaderWater.level     = ch->level;
  afLeaderWater.duration  = SUSTAIN_WEAVE;
  afLeaderWater.location  = APPLY_SPHERE_WATER;
  afLeaderWater.modifier  = victim->perm_sphere[SPHERE_WATER];
  afLeaderWater.bitvector = AFF_LINKED;
  affect_to_char(ch,&afLeaderWater);

  afLeaderSpirit.where     = TO_AFFECTS;
  afLeaderSpirit.casterId  = victim->id;
  afLeaderSpirit.type      = sn;
  afLeaderSpirit.level     = ch->level;
  afLeaderSpirit.duration  = SUSTAIN_WEAVE;
  afLeaderSpirit.location  = APPLY_SPHERE_SPIRIT;
  afLeaderSpirit.modifier  = victim->perm_sphere[SPHERE_SPIRIT];
  afLeaderSpirit.bitvector = AFF_LINKED;
  affect_to_char(ch,&afLeaderSpirit);

  afLeaderFire.where     = TO_AFFECTS;
  afLeaderFire.casterId  = victim->id;
  afLeaderFire.type      = sn;
  afLeaderFire.level     = ch->level;
  afLeaderFire.duration  = SUSTAIN_WEAVE;
  afLeaderFire.location  = APPLY_SPHERE_FIRE;
  afLeaderFire.modifier  = victim->perm_sphere[SPHERE_FIRE];
  afLeaderFire.bitvector = AFF_LINKED;
  affect_to_char(ch,&afLeaderFire);

  afLeaderEarth.where     = TO_AFFECTS;
  afLeaderEarth.casterId  = victim->id;
  afLeaderEarth.type      = sn;
  afLeaderEarth.level     = ch->level;
  afLeaderEarth.duration  = SUSTAIN_WEAVE;
  afLeaderEarth.location  = APPLY_SPHERE_EARTH;
  afLeaderEarth.modifier  = victim->perm_sphere[SPHERE_EARTH];
  afLeaderEarth.bitvector = AFF_LINKED;
  affect_to_char(ch,&afLeaderEarth);

  afVictimAir.where     = TO_AFFECTS;
  afVictimAir.casterId  = ch->id;
  afVictimAir.type      = sn;
  afVictimAir.level     = ch->level;
  afVictimAir.duration  = SUSTAIN_WEAVE;
  afVictimAir.location  = APPLY_SPHERE_AIR;
  afVictimAir.modifier  = (-1) * victim->perm_sphere[SPHERE_AIR];
  afVictimAir.bitvector = AFF_LINKED;
  affect_to_char(victim,&afVictimAir);

  afVictimWater.where     = TO_AFFECTS;
  afVictimWater.casterId  = ch->id;
  afVictimWater.type      = sn;
  afVictimWater.level     = ch->level;
  afVictimWater.duration  = SUSTAIN_WEAVE;
  afVictimWater.location  = APPLY_SPHERE_WATER;
  afVictimWater.modifier  = (-1) * victim->perm_sphere[SPHERE_WATER];
  afVictimWater.bitvector = AFF_LINKED;
  affect_to_char(victim,&afVictimWater);

  afVictimSpirit.where     = TO_AFFECTS;
  afVictimSpirit.casterId  = ch->id;
  afVictimSpirit.type      = sn;
  afVictimSpirit.level     = ch->level;
  afVictimSpirit.duration  = SUSTAIN_WEAVE;
  afVictimSpirit.location  = APPLY_SPHERE_SPIRIT;
  afVictimSpirit.modifier  = (-1) * victim->perm_sphere[SPHERE_SPIRIT];
  afVictimSpirit.bitvector = AFF_LINKED;
  affect_to_char(victim,&afVictimSpirit);

  afVictimFire.where     = TO_AFFECTS;
  afVictimFire.casterId  = ch->id;
  afVictimFire.type      = sn;
  afVictimFire.level     = ch->level;
  afVictimFire.duration  = SUSTAIN_WEAVE;
  afVictimFire.location  = APPLY_SPHERE_FIRE;
  afVictimFire.modifier  = (-1) * victim->perm_sphere[SPHERE_FIRE];
  afVictimFire.bitvector = AFF_LINKED;
  affect_to_char(victim,&afVictimFire);

  afVictimEarth.where     = TO_AFFECTS;
  afVictimEarth.casterId  = ch->id;
  afVictimEarth.type      = sn;
  afVictimEarth.level     = ch->level;
  afVictimEarth.duration  = SUSTAIN_WEAVE;
  afVictimEarth.location  = APPLY_SPHERE_EARTH;
  afVictimEarth.modifier  = (-1) * victim->perm_sphere[SPHERE_EARTH];
  afVictimEarth.bitvector = AFF_LINKED;
  affect_to_char(victim,&afVictimEarth);
 
  victim->bIsLinked = TRUE;
  ch->bIsLinked = TRUE;

  send_to_char("Oh YES!!!! Power!!!\n\r",ch);
  send_to_char("You are drawn into the link.\n\r",victim);


}

void do_unlink(CHAR_DATA *ch, char *argument) {
  //char arg[MAX_STRING_LENGTH];
  CHAR_DATA * victim;
  AFFECT_DATA * paf;
  AFFECT_DATA * paf_next;
  int sn = skill_lookup("link");
  LINK_DATA * pLink=NULL;
  LINK_DATA * pPriorLink=NULL;

    if ( !IS_NPC(ch)
    &&   ch->level < skill_table[gsn_link].skill_level[ch->class] )
    {
	send_to_char(
	    "Are you sure you know what you're doing?\n\r", ch );
	return;
    }
  if ((argument[0] == '\0') || !strcmp(argument,"all"))
  {
	while (ch->link_info)
        {
		ch->link_info->linked->bIsLinked = FALSE;
		ch->link_info->linked->bIsReadyForLink = FALSE;
		ch->link_info->linked->pIsLinkedBy = NULL;
		do_function(ch->link_info->linked,&do_unchannel,"");
		//send_to_char("You feel a sudden lack of energy as you are released from the exhileration of the link.\n\r",ch->link_info->linked);
	 	for ( paf = ch->link_info->linked->affected; paf != NULL; paf = paf_next ) 
                {
			paf_next = paf->next;
	   		if (paf->type == sn) {
              			affect_remove( ch->link_info->linked, paf );
           		}
                }
		pLink = ch->link_info;
		ch->link_info = ch->link_info->next;
		free_link_info(pLink);
        }
	ch->bIsLinked = FALSE;
	ch->bIsReadyForLink = FALSE;
	send_to_char("You feel a loss as you release them from the link.\n",ch);
	return;
  }
    if ( ( victim = get_char_room( ch, argument ) ) == NULL ) { 
	 send_to_char( "They aren't here.\n\r",              ch ); 
	 return; 
    }

    pLink = ch->link_info;
    pPriorLink = ch->link_info;

    while ((pLink) && (pLink->linked != victim))
    {
	pPriorLink = pLink;
	pLink = pLink->next;
    } 
    if (!pLink)
    {
	send_to_char("You are not linked with them.\n\r",ch);
	return;
    }
    else
    {
	pLink->linked->bIsLinked = FALSE;
	pLink->linked->bIsReadyForLink = FALSE;
	pLink->linked->pIsLinkedBy = NULL;
	for ( paf = pLink->linked->affected; paf != NULL; paf = paf_next ) 
        {
	   paf_next = paf->next;
	   if (paf->type == sn) {
              affect_remove( pLink->linked, paf );
           }
        }
	do_function(pLink->linked,&do_unchannel,"");
	send_to_char("You feel a sudden lack of energy as you are released from the exhileration of the link.\n\r",pLink->linked);
        send_to_char("You feel a loss as you release them from the link.\n",ch);
	
	if (pLink == ch->link_info)
        {
		ch->link_info = ch->link_info->next;
		free_link_info(pLink);
        }
        else
        {
		pPriorLink->next = pLink->next;	
		free_link_info(pLink);
        }
    }

}
  
bool check_can_link(CHAR_DATA * ch, CHAR_DATA * victim) 
{
  int  nCircleCount = 0;
  int  nMaleCount = 0;
  int  nFemaleCount = 0;

  if (ch->sex == SEX_MALE)
    return FALSE;
  if (ch->link_info != NULL) 
  {
     LINK_DATA * pLinked = ch->link_info;
     do
     {
	nCircleCount++;
    	if (pLinked->linked->sex == SEX_FEMALE) 
        {
	    nFemaleCount++;
	}
	else
        {
	    nMaleCount++;
        }
        pLinked = pLinked->next;
     } while (pLinked);
     if ((nMaleCount >= nFemaleCount) && (victim->sex == SEX_MALE) )
     {
	return FALSE;
     }
     else if ( (nCircleCount >= 12) && (nMaleCount < 1) && (victim->sex != SEX_MALE))
     {
	return FALSE;
     }
     else if ( (nCircleCount >= 25) && (nMaleCount < 2) && (victim->sex != SEX_MALE))
     {
	return FALSE;
     }
     else if ( (nCircleCount >= 38) && (nMaleCount < 3) && (victim->sex != SEX_MALE))
     {
	return FALSE;
     }
     else if ( (nCircleCount >= 51) && (nMaleCount < 4) && (victim->sex != SEX_MALE))
     {
	return FALSE;
     }
     else if ( (nCircleCount >= 62) && (nMaleCount < 5) && (victim->sex != SEX_MALE))
     {
	return FALSE;
     }
     else if (nCircleCount >= 72)
     {
	return FALSE;
     }
     else
     {
        return TRUE;
     }
  }
  else 
  {
        return TRUE;
  }
}
  
bool check_can_lead_link(CHAR_DATA * ch) 
{
  int  nCircleCount = 0;
  int  nMaleCount = 0;
  int  nFemaleCount = 0;
   
  if (ch->link_info != NULL) 
  {
     LINK_DATA * pLinked = ch->link_info;
     do
     {
	nCircleCount++;
    	if (pLinked->linked->sex == SEX_FEMALE) 
        {
	    nFemaleCount++;
	}
	else
        {
	    nMaleCount++;
        }
        pLinked = pLinked->next;
     } while (pLinked);
     if ((nCircleCount <= 13) && (nMaleCount > 0) && (ch->sex == SEX_FEMALE) && (nFemaleCount <= nMaleCount))
     {
	return FALSE;
     }
     else
     {
        return TRUE;
     }
  }
  else 
  {
        return TRUE;
  }
}

void do_unlink_char(CHAR_DATA *ch, CHAR_DATA * victim) {
  AFFECT_DATA * paf;
  AFFECT_DATA * paf_next;
  int sn = skill_lookup("link");

    LINK_DATA * pLink = ch->link_info;
    LINK_DATA * pPriorLink = ch->link_info;
    while ((pLink) && (pLink->linked != victim))
    {
	pPriorLink = pLink;
	pLink = pLink->next;
    } 
    if (!pLink)
    {
	send_to_char("You are not linked with them.\n\r",ch);
	return;
    }
    else
    {
	pLink->linked->bIsLinked = FALSE;
	pLink->linked->bIsReadyForLink = FALSE;
	pLink->linked->pIsLinkedBy = NULL;
	for ( paf = pLink->linked->affected; paf != NULL; paf = paf_next ) 
        {
	   paf_next = paf->next;
	   if (paf->type == sn) {
              affect_remove( pLink->linked, paf );
           }
        }
	do_function(pLink->linked,&do_unchannel,"");
	send_to_char("You feel a sudden lack of energy as you are released from the exhileration of the link.\n\r",pLink->linked);
	
	if (pLink == ch->link_info)
        {
		ch->link_info = ch->link_info->next;
		free_link_info(pLink);
        }
        else
        {
		pPriorLink->next = pLink->next;	
		free_link_info(pLink);
        }
    }

}

void do_passlink (CHAR_DATA * ch, char * argument) 
{
  CHAR_DATA   * victim;
  AFFECT_DATA * paf;
  AFFECT_DATA * paf_next;
  AFFECT_DATA afLeaderAir;
  AFFECT_DATA afLeaderWater;
  AFFECT_DATA afLeaderFire;
  AFFECT_DATA afLeaderEarth;
  AFFECT_DATA afLeaderSpirit;
  AFFECT_DATA afVictimAir;
  AFFECT_DATA afVictimWater;
  AFFECT_DATA afVictimFire;
  AFFECT_DATA afVictimEarth;
  AFFECT_DATA afVictimSpirit;
  LINK_DATA * pLink = NULL;
  LINK_DATA * pPriorLink = NULL;

  int sn = skill_lookup("link");

  if (!ch->bIsLinked) 
  {
	send_to_char("You aren't linked.\n\r",ch);
        return;
  }
  if (argument[0] == '\0') {
     send_to_char("Syntax: passlink <person>\n\r",ch);
     return;
  }
 
  if (!ch->link_info) {
     send_to_char("You aren't controlling the link\n\r",ch);
     return;
  }

    if ( ( victim = get_char_room( ch, argument ) ) == NULL ) { 
	 send_to_char( "They aren't here.\n\r",              ch ); 
	 return; 
    }
    
    pLink = ch->link_info;
    pPriorLink = ch->link_info;
    
    while ((pLink) && (pLink->linked->id != victim->id))
    {
	pPriorLink = pLink;
	pLink = pLink->next;
    } 
    if (!pLink)
    {
	send_to_char("You are not linked with them.\n\r",ch);
	return;
    }
    for ( paf = pLink->linked->affected; paf != NULL; paf = paf_next ) 
    {
      paf_next = paf->next;
      if (paf->type == sn) {
         affect_remove( pLink->linked, paf );
      }
    }

    for ( paf = ch->affected; paf != NULL; paf = paf_next ) 
    {
      paf_next = paf->next;
      if (paf->type == sn)
      {
         if (paf->casterId != victim->id)
         {
  	    affect_to_char(victim, paf );
         }
         affect_remove( ch, paf );
      }
    }

  afLeaderAir.where     = TO_AFFECTS;
  afLeaderAir.casterId  = ch->id;
  afLeaderAir.type      = sn;
  afLeaderAir.level     = victim->level;
  afLeaderAir.duration  = SUSTAIN_WEAVE;
  afLeaderAir.location  = APPLY_SPHERE_AIR;
  afLeaderAir.modifier  = ch->perm_sphere[SPHERE_AIR];
  afLeaderAir.bitvector = AFF_LINKED;
  affect_to_char(victim,&afLeaderAir);

  afLeaderWater.where     = TO_AFFECTS;
  afLeaderWater.casterId  = ch->id;
  afLeaderWater.type      = sn;
  afLeaderWater.level     = victim->level;
  afLeaderWater.duration  = SUSTAIN_WEAVE;
  afLeaderWater.location  = APPLY_SPHERE_WATER;
  afLeaderWater.modifier  = ch->perm_sphere[SPHERE_WATER];
  afLeaderWater.bitvector = AFF_LINKED;
  affect_to_char(victim,&afLeaderWater);

  afLeaderSpirit.where     = TO_AFFECTS;
  afLeaderSpirit.casterId  = ch->id;
  afLeaderSpirit.type      = sn;
  afLeaderSpirit.level     = victim->level;
  afLeaderSpirit.duration  = SUSTAIN_WEAVE;
  afLeaderSpirit.location  = APPLY_SPHERE_SPIRIT;
  afLeaderSpirit.modifier  = ch->perm_sphere[SPHERE_SPIRIT];
  afLeaderSpirit.bitvector = AFF_LINKED;
  affect_to_char(victim,&afLeaderSpirit);

  afLeaderFire.where     = TO_AFFECTS;
  afLeaderFire.casterId  = ch->id;
  afLeaderFire.type      = sn;
  afLeaderFire.level     = victim->level;
  afLeaderFire.duration  = SUSTAIN_WEAVE;
  afLeaderFire.location  = APPLY_SPHERE_FIRE;
  afLeaderFire.modifier  = ch->perm_sphere[SPHERE_FIRE];
  afLeaderFire.bitvector = AFF_LINKED;
  affect_to_char(victim,&afLeaderFire);

  afLeaderEarth.where     = TO_AFFECTS;
  afLeaderEarth.casterId  = ch->id;
  afLeaderEarth.type      = sn;
  afLeaderEarth.level     = victim->level;
  afLeaderEarth.duration  = SUSTAIN_WEAVE;
  afLeaderEarth.location  = APPLY_SPHERE_EARTH;
  afLeaderEarth.modifier  = ch->perm_sphere[SPHERE_EARTH];
  afLeaderEarth.bitvector = AFF_LINKED;
  affect_to_char(victim,&afLeaderEarth);

  afVictimAir.where     = TO_AFFECTS;
  afVictimAir.casterId  = victim->id;
  afVictimAir.type      = sn;
  afVictimAir.level     = ch->level;
  afVictimAir.duration  = SUSTAIN_WEAVE;
  afVictimAir.location  = APPLY_SPHERE_AIR;
  afVictimAir.modifier  = (-1) * ch->perm_sphere[SPHERE_AIR];
  afVictimAir.bitvector = AFF_LINKED;
  affect_to_char(ch,&afVictimAir);

  afVictimWater.where     = TO_AFFECTS;
  afVictimWater.casterId  = victim->id;
  afVictimWater.type      = sn;
  afVictimWater.level     = ch->level;
  afVictimWater.duration  = SUSTAIN_WEAVE;
  afVictimWater.location  = APPLY_SPHERE_WATER;
  afVictimWater.modifier  = (-1) * ch->perm_sphere[SPHERE_WATER];
  afVictimWater.bitvector = AFF_LINKED;
  affect_to_char(ch,&afVictimWater);

  afVictimSpirit.where     = TO_AFFECTS;
  afVictimSpirit.casterId  = victim->id;
  afVictimSpirit.type      = sn;
  afVictimSpirit.level     = ch->level;
  afVictimSpirit.duration  = SUSTAIN_WEAVE;
  afVictimSpirit.location  = APPLY_SPHERE_SPIRIT;
  afVictimSpirit.modifier  = (-1) * ch->perm_sphere[SPHERE_SPIRIT];
  afVictimSpirit.bitvector = AFF_LINKED;
  affect_to_char(ch,&afVictimSpirit);

  afVictimFire.where     = TO_AFFECTS;
  afVictimFire.casterId  = victim->id;
  afVictimFire.type      = sn;
  afVictimFire.level     = ch->level;
  afVictimFire.duration  = SUSTAIN_WEAVE;
  afVictimFire.location  = APPLY_SPHERE_FIRE;
  afVictimFire.modifier  = (-1) * ch->perm_sphere[SPHERE_FIRE];
  afVictimFire.bitvector = AFF_LINKED;
  affect_to_char(ch,&afVictimFire);

  afVictimEarth.where     = TO_AFFECTS;
  afVictimEarth.casterId  = victim->id;
  afVictimEarth.type      = sn;
  afVictimEarth.level     = ch->level;
  afVictimEarth.duration  = SUSTAIN_WEAVE;
  afVictimEarth.location  = APPLY_SPHERE_EARTH;
  afVictimEarth.modifier  = (-1) * ch->perm_sphere[SPHERE_EARTH];
  afVictimEarth.bitvector = AFF_LINKED;
  affect_to_char(ch,&afVictimEarth);

  victim->pIsLinkedBy = NULL;
  ch->pIsLinkedBy = victim;
  victim->link_info = ch->link_info;
  ch->link_info = NULL;
  send_to_char("You now control the link!\n\r",victim);
 
}

//Toggles the masking of channelling capability
void do_maskchan(CHAR_DATA * ch, char * argument)
{

    int chance = get_skill(ch,gsn_mask_channelling);

    if (chance <= 0) {
	send_to_char("Huh?\r\n", ch);
	return;
    }

    if (chance < number_percent()) {
	send_to_char("You need to work on that a bit more.\r\n",ch);
	check_improve(ch,gsn_mask_channelling,FALSE,1);
	return;
    }
    else
	check_improve(ch,gsn_mask_channelling,TRUE,1);

    if (IS_SET(ch->act2,PLR2_MASKCHAN)) {
	REMOVE_BIT(ch->act2,PLR2_MASKCHAN);
	send_to_char("You no longer hide your ability to channel.\r\n",ch);
    }
    else
    {
	SET_BIT(ch->act2,PLR2_MASKCHAN);
	send_to_char("You conceal your ability to channel.\r\n",ch);
    }

}

void do_gopk (CHAR_DATA * ch, char * argument) 
{

	if (IS_FACELESS(ch))
        {
		send_to_char("Temporary characters may not participate in the PK aspect\r\n",ch);
		return;
 	}
	send_to_char("Welcome to the world of PK!!!\r\n",ch);
	SET_BIT(ch->act2,PLR2_PKILLER);
   	if ( ch->incog_level) {
      		ch->incog_level = 0;
      		send_to_char( "You are no longer whoinvis.\n\r", ch );
	};

}
void do_rempk (CHAR_DATA * ch, char * argument) 
{
  	CHAR_DATA   * victim;
	
	if (!IS_IMMORTAL(ch))
	{
		send_to_char("Only Immortals can remove the pk flag!\r\n",ch);
		return;
	}
	victim = get_char_anywhere(ch,argument);
	if (!victim)
	{
		send_to_char("Player not found.\r\n",ch);
		return;	
	}
	send_to_char("You are now leaving the world of PK!!!\r\n",victim);
	send_to_char("Ok\r\n",ch);
	REMOVE_BIT(victim->act2,PLR2_PKILLER);

}
/**********************************************************************
 * *       Function      : do_conceal
 * *       Author        : Zandor
 * *       Description   : Lets someone conceal their full sphere potential
 * *       Parameters    :
 * *       Returns       :
 * **********************************************************************/
void do_conceal(CHAR_DATA *ch, char *argument)
{
  char arg[MAX_STRING_LENGTH];
  char sphere[MAX_STRING_LENGTH];
  AFFECT_DATA afAir;
  AFFECT_DATA afWater;
  AFFECT_DATA afFire;
  AFFECT_DATA afEarth;
  AFFECT_DATA afSpirit;
  AFFECT_DATA * paf;
  AFFECT_DATA * paf_next;
  int sn;

    if ( get_skill(ch,gsn_conceal) <= 0)
    {
	send_to_char("Huh?\r\n",ch);
	return;
    }

    if ( get_skill(ch,gsn_conceal) > number_percent())
    {
	check_improve(ch,gsn_conceal,TRUE,1);
    }
    else 
    {
	check_improve(ch,gsn_conceal,FALSE,1);
	send_to_char("You really need to work on that..\n\r",ch);
	return;
    }
    argument = one_argument(argument, sphere);
    if (is_number(sphere))
    {
          send_to_char("Syntax: conceal <SPHERE|ALL> #.\n\r",ch);
	  return;
    }
    argument = one_argument(argument,arg);
    if (!is_number(arg))
    {
          send_to_char("Syntax: conceal <SPHERE|ALL> #.\n\r",ch);
	  return;
    }
    int value = atoi(arg);

    if (value < 0)
    {
	send_to_char("It must be a positive value that you are subtracting or '0' to clear.\r\n",ch);
	return;
    }

    if (value == 0)
    {
	 for ( paf = ch->affected; paf != NULL; paf = paf_next ) 
         {
		paf_next = paf->next;
	   	if (paf->type == gsn_conceal) {
              	   affect_remove( ch, paf );
           	}
         } 
	 return;
    }

/*
    if ((value > ch->perm_sphere[SPHERE_EARTH])  ||
	(value > ch->perm_sphere[SPHERE_FIRE])  ||
	(value > ch->perm_sphere[SPHERE_WATER])  ||
	(value > ch->perm_sphere[SPHERE_SPIRIT])  ||
	(value > ch->perm_sphere[SPHERE_AIR] ))
    {
	send_to_char("That value is more than one or more of your spheres will allow.\r\n",ch);
	return;
    }
*/

  if (!str_prefix(sphere,"air") || !str_prefix(sphere,"all"))
  {
	if (value > ch->perm_sphere[SPHERE_AIR])
	{
		send_to_char("You can not conceal more Air you can hold.\r\n",ch);
	}
	else
	{
  		afAir.where     = TO_AFFECTS;
  		afAir.casterId  = ch->id;
  		afAir.type      = gsn_conceal;
  		afAir.level     = ch->level;
  		afAir.duration  = -1;
  		afAir.location  = APPLY_SPHERE_AIR;
  		afAir.modifier  = -1*value;
  		afAir.inverted  = TRUE;
  		afAir.bitvector = 0;
  		affect_to_char(ch,&afAir);
	}
  }

  if (!str_prefix(sphere,"water") || !str_prefix(sphere,"all"))
  {
	if (value > ch->perm_sphere[SPHERE_WATER])
	{
		send_to_char("You can not conceal more Water you can hold.\r\n",ch);
	}
	else
	{
  		afWater.where     = TO_AFFECTS;
  		afWater.casterId  = ch->id;
  		afWater.type      = gsn_conceal;
  		afWater.level     = ch->level;
  		afWater.duration  = -1;
  		afWater.location  = APPLY_SPHERE_WATER;
  		afWater.modifier  = -1*value;
  		afWater.inverted  = TRUE;
  		afWater.bitvector = 0;
  		affect_to_char(ch,&afWater);
	}
  }

  if (!str_prefix(sphere,"spirit") || !str_prefix(sphere,"all"))
  {
       if (value > ch->perm_sphere[SPHERE_SPIRIT])
        {
                send_to_char("You can not conceal more Spirit you can hold.\r\n",ch);
        }
        else
        {
  		afSpirit.where     = TO_AFFECTS;
  		afSpirit.casterId  = ch->id;
  		afSpirit.type      = gsn_conceal;
  		afSpirit.level     = ch->level;
  		afSpirit.duration  = -1;
  		afSpirit.location  = APPLY_SPHERE_SPIRIT;
  		afSpirit.modifier  = -1*value;
  		afSpirit.inverted  = TRUE;
  		afSpirit.bitvector = 0;
  		affect_to_char(ch,&afSpirit);
	}
  }

  if (!str_prefix(sphere,"fire") || !str_prefix(sphere,"all"))
  {
       if (value > ch->perm_sphere[SPHERE_FIRE])
        {
                send_to_char("You can not conceal more Fire you can hold.\r\n",ch);
        }
        else
        {
  		afFire.where     = TO_AFFECTS;
  		afFire.casterId  = ch->id;
  		afFire.type      = gsn_conceal;
  		afFire.level     = ch->level;
  		afFire.duration  = -1;
  		afFire.location  = APPLY_SPHERE_FIRE;
  		afFire.modifier  = -1*value;
  		afFire.inverted  = TRUE;
  		afFire.bitvector = 0;
  		affect_to_char(ch,&afFire);
  	}
  }

  if (!str_prefix(sphere,"earth") || !str_prefix(sphere,"all"))
  {
       if (value > ch->perm_sphere[SPHERE_EARTH])
        {
                send_to_char("You can not conceal more Earth you can hold.\r\n",ch);
        }
        else
        {
  		afEarth.where     = TO_AFFECTS;
  		afEarth.casterId  = ch->id;
  		afEarth.type      = gsn_conceal;
  		afEarth.level     = ch->level;
  		afEarth.duration  = -1;
  		afEarth.location  = APPLY_SPHERE_EARTH;
  		afEarth.modifier  = -1*value;
  		afEarth.inverted  = TRUE;
  		afEarth.bitvector = 0;
  		affect_to_char(ch,&afEarth);
	}
  }
  send_to_char("Your spheres have been adjusted.\r\n",ch);

  return;

}


void do_mine(CHAR_DATA *ch, char *argument)
{
    int cost = 50000; //50000 silcer or 500 gold
    char arg[MAX_STRING_LENGTH];
    int nType;       //ore type
    int nQuality;    //ore quality

    if (IS_SET(ch->res_flags,RES_MINING)) {
	send_to_char("You can't do that yet.\n\r", ch);
	return;
    }
    argument = one_argument(argument, arg);
    if ( get_skill(ch,gsn_mine) <= 0)
    {
	send_to_char("You're going to have to learn how first?\n\r",ch);
	return;
    }

    if ( (ch->silver + 100 * ch->gold) < cost ) {
	   send_to_char( "You can't afford it.\n\r", ch );
	   return;
     }

    if ( !IS_SET(ch->in_room->room_flags, ROOM_MINING_GOLD) &&
         !IS_SET(ch->in_room->room_flags, ROOM_MINING_SILVER) &&
         !IS_SET(ch->in_room->room_flags, ROOM_MINING_COPPER)) 
    {
	send_to_char("You can't do that here.\n\r", ch);
	return;
    }

    if (arg[0] == '\0') {  //No type of ore specified, pick one 
        bool found = FALSE;
	while (!found) 
	{
		int choice = number_range(1,4);
		switch (choice) {
                        case MINING_ORE_COPPER:
                            if (IS_SET(ch->in_room->room_flags, ROOM_MINING_COPPER)) //Copper
                            {
                                found = TRUE;
                                nType = MINING_ORE_COPPER;
                            }
			    break;
                        case MINING_ORE_SILVER:
                            if (IS_SET(ch->in_room->room_flags, ROOM_MINING_SILVER)) //Silver
                            {
                                found = TRUE;
                                nType = MINING_ORE_SILVER;
                            }
			    break;
                        case MINING_ORE_GOLD:
                            if (IS_SET(ch->in_room->room_flags, ROOM_MINING_GOLD)) //Gold
                            {
                                found = TRUE;
                                nType = MINING_ORE_GOLD;
                            }
			    break;
                        default: found = FALSE; break;


		}
		
	}
    }
    else {
  	if (!str_prefix(arg,"copper")) {
		if  (!IS_SET(ch->in_room->room_flags, ROOM_MINING_COPPER)) {
			send_to_char("There isn't any copper available here for mining.\n\r",ch);
			return;
		}
		else
		{
			nType = MINING_ORE_COPPER;
		}
	}
	else
  	if (!str_prefix(arg,"silver")) 
	{
		if (!IS_SET(ch->in_room->room_flags, ROOM_MINING_SILVER)) {
			send_to_char("There isn't any silver available here for mining.\n\r",ch);
			return;
		}
		else 
		{
			nType = MINING_ORE_SILVER;
		}
	}
	else
  	if (!str_prefix(arg,"gold"))
	{
		if (!IS_SET(ch->in_room->room_flags, ROOM_MINING_GOLD)) {
			send_to_char("There isn't any gold available here for mining.\n\r",ch);
			return;
		}
		else 
		{
			nType = MINING_ORE_GOLD;
		}
	}
    }

    int skill = get_skill(ch,gsn_mine);
    int chance = 0;

    //determine chance of mining ore
    if (skill < 50) {chance = 10;}
    else if (skill < 75) {chance = 30;}
    else if (skill < 105) {chance = 50;}
    else if (skill < 165) {chance = 70;}
    else if (skill < 230) {chance = 85;}
    else if (skill < 269) {chance = 95;}
    else chance = 100;
	
    if ( chance > number_percent())
    {
	check_improve(ch,gsn_mine,TRUE,1);
    }
    else 
    {
	check_improve(ch,gsn_mine,FALSE,1);
	send_to_char("You try and try but come back empty handed.\n\r",ch);
	return;
    }

    //determine quality
    int rQuality = number_percent(); //get a random number from 1 to 100
    if (skill < 75) { nQuality = MINING_QUALITY_FLAWED; }
    else if (skill < 105) 
	{ if (rQuality < 85) 
		nQuality = MINING_QUALITY_FLAWED;
          else
		nQuality = MINING_QUALITY_FLAWLESS;
        }
    else if (skill < 165) 
	{
		if (rQuality >= 65)
			nQuality = MINING_QUALITY_FLAWLESS;
		else
			nQuality = MINING_QUALITY_FLAWED;
	}
    else if (skill < 230) 
	{
		if (rQuality >= 95) 
			nQuality = MINING_QUALITY_EXCELLENT;
		else if (rQuality >= 45)
			nQuality = MINING_QUALITY_FLAWLESS;
		else
			nQuality = MINING_QUALITY_FLAWED;
	}
    else if (skill < 269) 
	{
		if (rQuality >= 99)
			nQuality = MINING_QUALITY_PERFECT;
		else if (rQuality >= 89) 
			nQuality = MINING_QUALITY_EXCELLENT;
		else if (rQuality >= 35)
			nQuality = MINING_QUALITY_FLAWLESS;
		else
			nQuality = MINING_QUALITY_FLAWED;
	}
    else 
	{
		if (rQuality >= 97)
			nQuality = MINING_QUALITY_PERFECT;
		else if (rQuality >= 77) 
			nQuality = MINING_QUALITY_EXCELLENT;
		else if (rQuality >= 20)
			nQuality = MINING_QUALITY_FLAWLESS;
		else
			nQuality = MINING_QUALITY_FLAWED;
	}

	if (!IS_IMMORTAL(ch)) 
	{
        	  AFFECT_DATA af;
                  af.where      = TO_RESIST;
                  af.bitvector  = RES_MINING;
		  af.type 	= gsn_mine;
                  af.level      = ch->level;
                  af.duration   = 3;
                  af.location   = APPLY_NONE;
                  af.modifier   = 0;
                  affect_to_char(ch,&af);
	}
	
	OBJ_DATA * obj = create_ore(nType, nQuality);
	char buffer[MAX_STRING_LENGTH];
	sprintf(buffer,"SUCCESS!! You found %s.\n\r",obj->short_descr);
	send_to_char(buffer, ch);
	obj_to_char(obj, ch);
	deduct_cost(ch,cost);

	return;	
}

void do_gemmine(CHAR_DATA *ch, char *argument)
{
    int cost = 50000; //50000 silcer or 500 gold
    char arg[MAX_STRING_LENGTH];
    int nType;       //ore type
    int nQuality;    //ore quality

    if (IS_SET(ch->res_flags,RES_MINING)) {
	send_to_char("You can't do that yet.\n\r", ch);
	return;
    }
    argument = one_argument(argument, arg);
    if ( get_skill(ch,gsn_gemmine) <= 0)
    {
	send_to_char("You're going to have to learn how first?\n\r",ch);
	return;
    }

    if ( (ch->silver + 100 * ch->gold) < cost ) {
	   send_to_char( "You can't afford it.\n\r", ch );
	   return;
     }

    if ( !IS_SET(ch->in_room->room_flags, ROOM_MINING_EMERALD) &&
         !IS_SET(ch->in_room->room_flags, ROOM_MINING_RUBY) &&
         !IS_SET(ch->in_room->room_flags, ROOM_MINING_DIAMOND)) 
    {
	send_to_char("You can't do that here.\n\r", ch);
	return;
    }

    if (arg[0] == '\0') {  //No type of gem specified, pick one 
        bool found = FALSE;
	while (!found) 
	{
		int choice = number_range(1,4);
		switch (choice) {
			case MINING_GEM_RUBY:
			    if (IS_SET(ch->in_room->room_flags, ROOM_MINING_RUBY)) //Copper	
			    {
				found = TRUE;
			        nType = MINING_GEM_RUBY;
			    }
			    break;
			case MINING_GEM_DIAMOND:
			    if (IS_SET(ch->in_room->room_flags, ROOM_MINING_DIAMOND)) //Silver
			    {
				found = TRUE;
			        nType = MINING_GEM_DIAMOND;
			    }
			    break;
			case MINING_GEM_EMERALD:
			    if (IS_SET(ch->in_room->room_flags, ROOM_MINING_EMERALD)) //Gold
			    {
				found = TRUE;
			        nType = MINING_GEM_EMERALD;
			    }
			    break;
			default: found = FALSE; break;

		} 
	}
    }
    else {
  	if (!str_prefix(arg,"ruby")) {
		if  (!IS_SET(ch->in_room->room_flags, ROOM_MINING_RUBY)) {
			send_to_char("There aren't any rubies available here for mining.\n\r",ch);
			return;
		}
		else
		{
			nType = MINING_GEM_RUBY;
		}
	}

	else
  	if (!str_prefix(arg,"diamond")) 
	{
		if (!IS_SET(ch->in_room->room_flags, ROOM_MINING_DIAMOND)) {
			send_to_char("There aren't any diamond's available here for mining.\n\r",ch);
			return;
		}
		else 
		{
			nType = MINING_GEM_DIAMOND;
		}
	}
	else
  	if (!str_prefix(arg,"emerald"))
	{
		if (!IS_SET(ch->in_room->room_flags, ROOM_MINING_EMERALD)) {
			send_to_char("There aren't any emeralds available here for mining.\n\r",ch);
			return;
		}
		else 
		{
			nType = MINING_GEM_EMERALD;
		}
	}
    }

    int skill = get_skill(ch,gsn_gemmine);
    int chance = 0;

    //determine chance of mining ore
    if (skill < 50) {chance = 10;}
    else if (skill < 75) {chance = 30;}
    else if (skill < 105) {chance = 50;}
    else if (skill < 165) {chance = 70;}
    else if (skill < 230) {chance = 85;}
    else if (skill < 269) {chance = 95;}
    else chance = 100;
	
    if ( chance > number_percent())
    {
	check_improve(ch,gsn_gemmine,TRUE,1);
    }
    else 
    {
	check_improve(ch,gsn_gemmine,FALSE,1);
	send_to_char("You try and try but come back empty handed.\n\r",ch);
	return;
    }

    //determine quality
    int rQuality = number_percent(); //get a random number from 1 to 100
    if (skill < 75) { nQuality = MINING_QUALITY_FLAWED; }
    else if (skill < 105) 
	{ if (rQuality < 85) 
		nQuality = MINING_QUALITY_FLAWED;
          else
		nQuality = MINING_QUALITY_FLAWLESS;
        }
    else if (skill < 165) 
	{
		if (rQuality >= 65)
			nQuality = MINING_QUALITY_FLAWLESS;
		else
			nQuality = MINING_QUALITY_FLAWED;
	}
    else if (skill < 230) 
	{
		if (rQuality >= 95) 
			nQuality = MINING_QUALITY_EXCELLENT;
		else if (rQuality >= 45)
			nQuality = MINING_QUALITY_FLAWLESS;
		else
			nQuality = MINING_QUALITY_FLAWED;
	}
    else if (skill < 269) 
	{
		if (rQuality >= 99)
			nQuality = MINING_QUALITY_PERFECT;
		else if (rQuality >= 89) 
			nQuality = MINING_QUALITY_EXCELLENT;
		else if (rQuality >= 35)
			nQuality = MINING_QUALITY_FLAWLESS;
		else
			nQuality = MINING_QUALITY_FLAWED;
	}
    else 
	{
		if (rQuality >= 97)
			nQuality = MINING_QUALITY_PERFECT;
		else if (rQuality >= 77) 
			nQuality = MINING_QUALITY_EXCELLENT;
		else if (rQuality >= 20)
			nQuality = MINING_QUALITY_FLAWLESS;
		else
			nQuality = MINING_QUALITY_FLAWED;
	}

	if (!IS_IMMORTAL(ch)) 
	{
        	  AFFECT_DATA af;
                  af.where      = TO_RESIST;
                  af.bitvector  = RES_MINING;
		  af.type 	= gsn_mine;
                  af.level      = ch->level;
                  af.duration   = 3;
                  af.location   = APPLY_NONE;
                  af.modifier   = 0;
                  affect_to_char(ch,&af);
	}
	
	OBJ_DATA * obj = create_gem(nType, nQuality);
	char buffer[MAX_STRING_LENGTH];
	sprintf(buffer,"SUCCESS!! You found %s.\n\r",obj->short_descr);
	send_to_char(buffer, ch);
	obj_to_char(obj, ch);
	deduct_cost(ch,cost);
	return;	
}

void do_smelt(CHAR_DATA *ch, char *argument)
{
	char ore[MAX_STRING_LENGTH];
	char quality [MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH];

	if (argument[0] == '\0')
	{
		send_to_char("Syntax: smelt <copper|silver|gold> <flawless|excellent>\r\nYou must have 5 of the lower grade chunks of ore to smelt a higher quality chunk.\r\n",ch);
		return;
	}
	int skill = get_skill(ch,gsn_smelt);
	int cost = 50000;
	int nType = 0;
        int nQuality = 0;
        int nQualityNeeded = 0;

	if (skill < 1) {
		send_to_char("You don't know how to do that.\n\r", ch);
		return;
	}
        if ( (ch->silver + 100 * ch->gold) < cost ) {
	   send_to_char( "You can't afford it.\n\r", ch );
	   return;
        }

	argument = one_argument(argument, ore);
	if (ore[0] == '\0') {
		send_to_char("Syntax: smelt <ore> <quality>\n\r", ch);
		return;	   
	}
	argument = one_argument(argument, quality);
	if (quality[0] == '\0') {
		send_to_char("Syntax: smelt <ore> <quality>\n\r", ch);
		return;	   
	}

	if (!str_prefix(ore, "copper")) {
		nType = MINING_ORE_COPPER;
	}
	else
	if (!str_prefix(ore, "silver")) {
		nType = MINING_ORE_SILVER;
	}
	else
	if (!str_prefix(ore, "gold")) {
		nType = MINING_ORE_GOLD;
	}
	else
	{
		send_to_char("Invalid type of ore. Choices are copper, silver, or gold.\n\r", ch);
		return;
	}

	if (!str_prefix(quality, "flawless")) {
		nQuality = MINING_QUALITY_FLAWLESS;
		nQualityNeeded = MINING_QUALITY_FLAWED;
	}
	else
	if (!str_prefix(quality, "excellent")) {
		nQuality = MINING_QUALITY_EXCELLENT;
		nQualityNeeded = MINING_QUALITY_FLAWLESS;
	}
	else 
	{
		send_to_char("Invalid type of quality. Choices are flawless and excellent.\n\r", ch);
		return;

	}

	int count = 0;  //Need 5 to smelt
	OBJ_DATA * obj = NULL;	
  	for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    	{
        	if ( obj->wear_loc == WEAR_NONE
        	&&   (can_see_obj( ch, obj ) ) )
        	{
		    if ((obj->item_type == ITEM_ORE) &&
			(IS_SET(obj->value[0], nType)) &&
			(IS_SET(obj->value[1], nQualityNeeded))) {
				count++;	
			}
        	}
    	}
	
	if (count < 5) //if we don't have enough ore of the correct type
	{
		send_to_char("You don't have the required 5 resources to do this.\n\r",ch);
		return;
	}

	deduct_cost(ch,cost);
	int chance = skill / 3;
	int REALLYFailed = 0;
	if (chance < number_percent()) {
		if (number_percent() > ((skill / 3) + 25)) {
			send_to_char("You REALLY slipped up this time. Your ore breaks down into worthless black slag.\n\r", ch);
			REALLYFailed = 1;
		}
		else
		{
			send_to_char("You failed.\n\r", ch);
			return;
		}
	}

	//Success - they have the skill, the ore, and the money. 
	//Let's start by giving them the new ore
	if (!REALLYFailed)
	{
		OBJ_DATA * ore_obj = create_ore(nType,nQuality);
		obj_to_char(ore_obj, ch);
	}
	else
	{
		OBJ_DATA * ore_obj = create_ore(nType,nQuality);
		ore_obj->value[0] = 0;
		ore_obj->value[1] = 0;
		ore_obj->name = str_dup("worthless slag");
		ore_obj->short_descr = str_dup("a lump of black slag");
		ore_obj->description = str_dup("A lump of black slag lays here.");
		obj_to_char(ore_obj, ch);

	}

	//then deducting the old ores
        OBJ_DATA * obj_next;
	count = 0;
  	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    	{
       		obj_next = obj->next_content;
        	if ( obj->wear_loc == WEAR_NONE
        	&&   (can_see_obj( ch, obj ) ) )
        	{
		    if ((obj->item_type == ITEM_ORE) &&
			(IS_SET(obj->value[0], nType)) &&
			(IS_SET(obj->value[1], nQualityNeeded))) {
				obj_from_char(obj);
				extract_obj(obj);	
				count++;
			}
		    if (count == 5) //once we get them all, break out of the loop
			break;
        	}
    	}
	
	//And we're done, let the player know what they got
       
	if (!REALLYFailed)
	{
		sprintf(buffer,"Success!! You have create a %s ore of %s quality.\n\r",flag_string(ore_types,nType), flag_string(mining_quality,nQuality));
		send_to_char(buffer, ch);
	}

	return;

}

void do_gemcut(CHAR_DATA *ch, char *argument)
{
	char gem[MAX_STRING_LENGTH];
	char quality [MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH];

	if (argument[0] == '\0')
	{
		send_to_char("Syntax: gemcut <emerald|ruby|diamond> <flawless|excellent>\r\nYou must have 5 of the lower grade gems to cut a higher quality chunk.\r\n",ch);
		return;
	}
	int skill = get_skill(ch,gsn_gemcut);
	int cost = 50000;
	int nType = 0;
        int nQuality = 0;
        int nQualityNeeded = 0;

	if (skill < 1) {
		send_to_char("You don't know how to do that.\n\r", ch);
		return;
	}
        if ( (ch->silver + 100 * ch->gold) < cost ) {
	   send_to_char( "You can't afford it.\n\r", ch );
	   return;
        }

	argument = one_argument(argument, gem);
	if (gem[0] == '\0') {
		send_to_char("Syntax: gemcut <gem> <quality>\n\r", ch);
		return;	   
	}
	argument = one_argument(argument, quality);
	if (quality[0] == '\0') {
		send_to_char("Syntax: gemcut <gem> <quality>\n\r", ch);
		return;	   
	}

	if (!str_prefix(gem, "ruby")) {
		nType = MINING_GEM_RUBY;
	}
	else
	if (!str_prefix(gem, "emerald")) {
		nType = MINING_GEM_EMERALD;
	}
	else
	if (!str_prefix(gem, "diamond")) {
		nType = MINING_GEM_DIAMOND;
	}
	else
	{
		send_to_char("Invalid type of gem. Choices are ruby, diamond, or emerald.\n\r", ch);
		return;
	}

	if (!str_prefix(quality, "flawless")) {
		nQuality = MINING_QUALITY_FLAWLESS;
		nQualityNeeded = MINING_QUALITY_FLAWED;
	}
	else
	if (!str_prefix(quality, "excellent")) {
		nQuality = MINING_QUALITY_EXCELLENT;
		nQualityNeeded = MINING_QUALITY_FLAWLESS;
	}
	else 
	{
		send_to_char("Invalid type of quality. Choices are flawless and excellent.\n\r", ch);
		return;

	}

	int count = 0;  //Need 5 to smelt
	OBJ_DATA * obj = NULL;	
  	for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    	{
        	if ( obj->wear_loc == WEAR_NONE
        	&&   (can_see_obj( ch, obj ) ) )
        	{
		    if ((obj->item_type == ITEM_GEMSTONE) &&
			(IS_SET(obj->value[0], nType)) &&
			(IS_SET(obj->value[1], nQualityNeeded))) {
				count++;	
			}
        	}
    	}
	
	if (count < 5) //if we don't have enough ore of the correct type
	{
		send_to_char("You don't have the required 5 resources to do this.\n\r",ch);
		return;
	}

	deduct_cost(ch,cost);
	int chance = skill / 3;
	int REALLYFailed = 0;
	if (chance < number_percent()) {
		if (number_percent() > ((skill / 3) + 25)) {
			send_to_char("You slip with the hammer and destroy the precious stones.\n\r", ch);
			REALLYFailed = 1;
		}
		else
		{
			send_to_char("You failed.\n\r", ch);
			return;
		}
	}

	//Success - they have the skill, the ore, and the money. 
	//Let's start by giving them the new ore
	if (!REALLYFailed)
	{ 
		OBJ_DATA * ore_obj = create_gem(nType,nQuality);
		obj_to_char(ore_obj, ch);
	}

	//then deducting the old ores
	count = 0;
        OBJ_DATA * obj_next;
  	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    	{
       		obj_next = obj->next_content;
        	if ( obj->wear_loc == WEAR_NONE
        	&&   (can_see_obj( ch, obj ) ) )
        	{
		    if ((obj->item_type == ITEM_GEMSTONE) &&
			(IS_SET(obj->value[0], nType)) &&
			(IS_SET(obj->value[1], nQualityNeeded))) {
				obj_from_char(obj);
				extract_obj(obj);	
				count++;
			}
		    if (count == 5) //once we get them all, break out of the loop
			break;
        	}
    	}
	
	//And we're done, let the player know what they got
       
	if (!REALLYFailed)
	{
		sprintf(buffer,"Success!! You have create a %s of %s quality.\n\r",flag_string(gem_types,nType), flag_string(mining_quality,nQuality));
		send_to_char(buffer, ch);
	}
	
	return;

}

void do_armorcraft(CHAR_DATA *ch, char *argument)
{
	char slot[MAX_STRING_LENGTH];
	char ore1[MAX_STRING_LENGTH];
	char ore2[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH];
	char short_desc[MAX_STRING_LENGTH];
	char long_desc[MAX_STRING_LENGTH];
	char name[MAX_STRING_LENGTH];
	char stone[20];
	char type[20];

	int nSlot = ITEM_TAKE;
	int nOre1Type = 0;
        int nOre2Type = 0;	
	int cost = 10000;


	int skill = get_skill(ch, gsn_armorcraft);
	if (skill <= 0) {
		send_to_char("You don't have the skill to do this yet.\n\r",ch);
		return;
	}

	if (argument[0] == '\0') 
	{
		send_to_char("Syntax: armorcraft <legs|arms|hands|body|head> <ore type> <ore type>\n\r",ch);
		return;
	}
	argument = one_argument(argument,slot);
	if (slot[0] == '\0') 
	{
		send_to_char("Syntax: armorcraft <legs|arms|hands|body|head> <ore type> <ore type>\n\r",ch);
		return;
	}
	argument = one_argument(argument,ore1);
	if (ore1[0] == '\0') 
	{
		send_to_char("Syntax: armorcraft <legs|arms|hands|body|head> <ore type> <ore type>\n\r",ch);
		return;
	}
	argument = one_argument(argument,ore2);
	if (ore2[0] == '\0') 
	{
		send_to_char("Syntax: armorcraft <legs|arms|hands|body|head> <ore type> <ore type>\n\r",ch);
		return;
	}

        if (!str_prefix(ore1, "copper")) {
                nOre1Type = MINING_ORE_COPPER;
        }
        else
        if (!str_prefix(ore1, "silver")) {
                nOre1Type = MINING_ORE_SILVER;
        }
        else
        if (!str_prefix(ore1, "gold")) {
                nOre1Type = MINING_ORE_GOLD;
        }
        else
        {
                send_to_char("Invalid type of ore. Choices are copper, silver, or gold.\n\r", ch);
                return;
        }

        if (!str_prefix(ore2, "copper")) {
                nOre2Type = MINING_ORE_COPPER;
        }
        else
        if (!str_prefix(ore2, "silver")) {
                nOre2Type = MINING_ORE_SILVER;
        }
        else
        if (!str_prefix(ore2, "gold")) {
                nOre2Type = MINING_ORE_GOLD;
        }
        else
        {
                send_to_char("Invalid type of ore. Choices are copper, silver, or gold.\n\r", ch);
                return;
        }

	if (!str_prefix(slot,"head")) 
	{
		SET_BIT(nSlot,  ITEM_WEAR_HEAD);
		strcpy(type, "helmet");
	}
	else
	if (!str_prefix(slot,"arms")) 
	{
		SET_BIT(nSlot,  ITEM_WEAR_ARMS);
		strcpy(type,"set of armbands");
	}
	else
	if (!str_prefix(slot,"hands")) 
	{
		SET_BIT(nSlot,  ITEM_WEAR_HANDS);
		strcpy(type,"pair of gloves");
	}
	else
	if (!str_prefix(slot,"legs")) 
	{
		SET_BIT(nSlot,  ITEM_WEAR_LEGS);
		strcpy(type, "pair of leggings");
	}
	else
	if (!str_prefix(slot,"body")) 
	{
		SET_BIT(nSlot,  ITEM_WEAR_BODY);
		strcpy(type,"chain armor");
	}
	else
	{
		send_to_char("Wear location choices are: head, arms, hands, legs, or body.\n\r", ch);
		return;
	}
        if ( (ch->silver + 100 * ch->gold) < cost ) {
	   char buff[256];
	   sprintf(buff, "You can't afford the %d gold that it would cost.\n\r", cost / 100);
	   send_to_char( buff, ch );
	   return;
        }


	//At this point, we know what the character wants to make, and what the character wants to use to make it, now we have to find out if the character has what it takes to make it.
	int coppercount = 0, silvercount=0, goldcount = 0;
        OBJ_DATA * obj = NULL;
        for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
        {
                if ( obj->wear_loc == WEAR_NONE
                &&   (can_see_obj( ch, obj ) ) )
                {
                    if (obj->item_type == ITEM_ORE) 
 		    {
			if (IS_SET(obj->value[0], MINING_ORE_COPPER))
			{
				if (IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) || 
				    IS_SET(obj->value[1], MINING_QUALITY_PERFECT)) 
				{
					coppercount++;
				} } 
			if (IS_SET(obj->value[0], MINING_ORE_SILVER))
			{
				if (IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) || 
				    IS_SET(obj->value[1], MINING_QUALITY_PERFECT)) 
				{
					silvercount++;
				}
			} 
			if (IS_SET(obj->value[0], MINING_ORE_GOLD))
			{
				if (IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) || 
				    IS_SET(obj->value[1], MINING_QUALITY_PERFECT)) 
				{
					goldcount++;
				}
			} 
                    }
                }
        }
	switch (nOre1Type) 
	{
		case MINING_ORE_COPPER: coppercount--; break;
		case MINING_ORE_SILVER: silvercount--; break;
		case MINING_ORE_GOLD: goldcount--; break;
		default: break;
	}

	switch (nOre2Type) 
	{
		case MINING_ORE_COPPER: coppercount--; break;
		case MINING_ORE_SILVER: silvercount--; break;
		case MINING_ORE_GOLD: goldcount--; break;
		default: break;
	}

	if ((coppercount < 0)  || (silvercount < 0) || (goldcount < 0))
	{
		send_to_char("You do not have all of the required ores for that.\n\r", ch);
		return;
	}
	
	int REALLYFailed = 0;
	int chance = skill / 3;
	if (chance < number_percent()) {
		if (number_percent() > ((skill / 3) + 25)) {
			send_to_char("You REALLY slipped up this time. Your ore breaks down into worthless black slag.\n\r", ch);
			REALLYFailed = 1;
		}
		else
		{
			send_to_char("You failed, but managed to save the pieces.\n\r", ch);
			return;
		}
	}

	//Extract ore 1 from char
        OBJ_DATA * ore1Obj;
        OBJ_DATA * ore2Obj;

        OBJ_DATA * obj_next;
  	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    	{
       		obj_next = obj->next_content;
        	if ( obj->wear_loc == WEAR_NONE
        	&&   (can_see_obj( ch, obj ) ) )
        	{
		    if ((obj->item_type == ITEM_ORE) &&
			(IS_SET(obj->value[0], nOre1Type)) &&
			((IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) ||
			  IS_SET(obj->value[1], MINING_QUALITY_PERFECT)))) {
				obj_from_char(obj);
				ore1Obj = obj;
				break;
			}
        	}
    	}
	
  	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    	{
       		obj_next = obj->next_content;
        	if ( obj->wear_loc == WEAR_NONE
        	&&   (can_see_obj( ch, obj ) ) )
        	{
		    if ((obj->item_type == ITEM_ORE) &&
			(IS_SET(obj->value[0], nOre2Type)) &&
			((IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) ||
			  IS_SET(obj->value[1], MINING_QUALITY_PERFECT)))) {
				obj_from_char(obj);
				ore2Obj = obj;
				break;
			}
        	}
    	}
	int dammod = 0;
	int hitmod = 0;
	int hpmod = 0;
	int endurmod = 0;
	int modifier = number_range(5,10);

	if (IS_SET(ore1Obj->value[1], MINING_QUALITY_PERFECT)) {
		modifier += 10;
	}

	switch (ore1Obj->value[0]) 
	{
		case MINING_ORE_COPPER: 
		{
			if (number_range(1,2) == 1) {
				hpmod += (modifier * 5);
			}
			else {
				endurmod += (modifier * 5);
			}		
		}
		break;
		case MINING_ORE_SILVER: 
		{
			hitmod += modifier;
		}
		break;
		case MINING_ORE_GOLD: 
		{
			dammod += modifier;
		}
		break;
	}

	modifier = number_range(5,10);
	if (IS_SET(ore2Obj->value[1], MINING_QUALITY_PERFECT)) {
		modifier += 10;
	}

	switch (ore2Obj->value[0]) 
	{
		case MINING_ORE_COPPER: 
		{
			if (number_range(1,2) == 1) {
				hpmod += (modifier * 5);
			}
			else {
				endurmod += (modifier * 5);
			}		
		}
		break;
		case MINING_ORE_SILVER: 
		{
			hitmod += modifier;
		}
		break;
		case MINING_ORE_GOLD: 
		{
			dammod += modifier;
		}
		break;
	}
	//clean up the old objects
	extract_obj(ore1Obj);
	extract_obj(ore2Obj);

	if (REALLYFailed) {
		return;
	}

	//make the new armor
	OBJ_DATA * newItem = create_object(get_obj_index(OBJ_VNUM_CRAFTING_ARMOR), 0);
	SET_BIT(newItem->wear_flags, nSlot);


	if (nOre1Type == MINING_ORE_GOLD && nOre2Type == MINING_ORE_GOLD) 
		sprintf(buffer,"a pure {Yg{yol{Yd{yen{x %s",type);
	else if (nOre1Type == MINING_ORE_GOLD && nOre2Type == MINING_ORE_SILVER)
		sprintf(buffer,"a {Yg{yol{Yd{yen{x %s with inlaid {Ws{Di{Wl{Dv{We{Dr{x vines", type);
	else if (nOre1Type == MINING_ORE_GOLD && nOre2Type == MINING_ORE_COPPER)
		sprintf(buffer,"a {Yg{yol{Yd{yen{x %s with inlaid {rc{Rop{rp{Re{rr{x vines", type);
	else if (nOre1Type == MINING_ORE_SILVER && nOre2Type == MINING_ORE_GOLD)
		sprintf(buffer,"a %s with braided {Ws{Di{Wl{Dv{We{Dr{x and {Yg{yol{Yd{yen{x adornments", type);
	else if (nOre1Type == MINING_ORE_SILVER && nOre2Type == MINING_ORE_SILVER)
		sprintf(buffer,"a %s made of pure {Ws{Di{Wl{Dv{We{Dr{x", type);
	else if (nOre1Type == MINING_ORE_SILVER && nOre2Type == MINING_ORE_COPPER)
		sprintf(buffer,"a %s with braided {Ws{Di{Wl{Dv{We{Dr{x and {rc{Rop{rp{Re{rr{x adornments", type);
	else if (nOre1Type == MINING_ORE_COPPER && nOre2Type == MINING_ORE_GOLD)
		sprintf(buffer,"a {rc{Rop{rp{Re{rr{x %s with inlaid {Yg{yol{Yd{yen{x engravings", type);
	else if (nOre1Type == MINING_ORE_COPPER && nOre2Type == MINING_ORE_SILVER)
		sprintf(buffer,"a {rc{Rop{rp{Re{rr{x %s with inlaid {Ws{Di{Wl{Dv{We{Dr{x engravings", type);
	else if (nOre1Type == MINING_ORE_COPPER && nOre2Type == MINING_ORE_COPPER)
		sprintf(buffer,"a solid {rc{Rop{rp{Re{rr{x %s", type);


	newItem->short_descr = str_dup(buffer);
	sprintf(buffer,"%s lies here",newItem->short_descr);
	newItem->description  = str_dup(buffer);
	sprintf(buffer, "crafted armor %s %s", slot, type);
	newItem->name =  str_dup(buffer);
	newItem->owner = str_dup(ch->name);
	
	if (dammod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_DAMROLL;
        	af.modifier   = dammod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_armorcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	
	if (hitmod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_HITROLL;
        	af.modifier   = hitmod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_armorcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	if (hpmod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_HIT;
        	af.modifier   = hpmod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_armorcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	
	if (endurmod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_ENDURANCE;
        	af.modifier   = endurmod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_armorcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	deduct_cost(ch,cost);
	obj_to_char(newItem,ch);
	sprintf(buffer,"SUCCESS: You have just created %s", newItem->short_descr);
	send_to_char(buffer,ch);
	

}


void do_jewelcraft(CHAR_DATA *ch, char *argument)
{
	char slot[MAX_STRING_LENGTH];
	char ore1[MAX_STRING_LENGTH];
	char gem2[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH];
	char short_desc[MAX_STRING_LENGTH];
	char long_desc[MAX_STRING_LENGTH];
	char name[MAX_STRING_LENGTH];
	char stone[20];
	char type[20];

	int nSlot = ITEM_TAKE;
	int nOre1Type = 0;
        int nGem2Type = 0;	
	int cost = 0;

	int skill = get_skill(ch, gsn_jewelcraft);
	if (skill <= 0) {
		send_to_char("You don't have the skill to do this yet.\n\r",ch);
		return;
	}

	if (argument[0] == '\0') 
	{
		send_to_char("Syntax: jewelcraft <finger | ears | neck> <ore type> <gem type>\n\r",ch);
		return;
	}
	argument = one_argument(argument,slot);
	if (slot[0] == '\0') 
	{
		send_to_char("Syntax: jewelcraft <finger | ears | neck> <ore type> <gem type>\n\r",ch);
		return;
	}
	argument = one_argument(argument,ore1);
	if (ore1[0] == '\0') 
	{
		send_to_char("Syntax: jewelcraft <finger | ears | neck> <ore type> <gem type>\n\r",ch);
		return;
	}
	argument = one_argument(argument,gem2);
	if (gem2[0] == '\0') 
	{
		send_to_char("Syntax: jewelcraft <finger | ears | neck> <ore type> <gem type>\n\r",ch);
		return;
	}

        if (!str_prefix(ore1, "copper")) {
                nOre1Type = MINING_ORE_COPPER;
		cost = 500000;
        }
        else
        if (!str_prefix(ore1, "silver")) {
                nOre1Type = MINING_ORE_SILVER;
		cost = 1500000;
		
        }
        else
        if (!str_prefix(ore1, "gold")) {
                nOre1Type = MINING_ORE_GOLD;
		cost = 1000000;
        }
        else
        {
                send_to_char("Invalid type of ore. Choices are copper, silver, gold.\n\r", ch);
                return;
        }

        if ( (ch->silver + 100 * ch->gold) < cost ) {
	   char buff[256];
	   sprintf(buff, "You can't afford the %d gold that it would cost.\n\r", cost / 100);
	   send_to_char( buff, ch );
	   return;
        }

        if (!str_prefix(gem2, "ruby")) {
                nGem2Type = MINING_GEM_RUBY;
		strcpy(stone, "a {rr{Rub{ry{x");
        }
        else
        if (!str_prefix(gem2, "diamond")) {
                nGem2Type = MINING_GEM_DIAMOND;
		strcpy(stone, "a {Wd{wi{Wam{wm{Wond{x");
        }
        else
        if (!str_prefix(gem2, "emerald")) {
                nGem2Type = MINING_GEM_EMERALD;
		strcpy(stone, "an {gem{Ger{ga{Gl{gd{x");
        }
        else
        {
                send_to_char("Invalid type of gemstone. Choices are ruby, emerald, or diamond.\n\r", ch);
                return;
        }

	if (!str_prefix(slot,"ear")) 
	{
		SET_BIT(nSlot,  ITEM_WEAR_EAR);
		strcpy(type, "earring");
	}
	else
	if (!str_prefix(slot,"finger")) 
	{
		SET_BIT(nSlot,  ITEM_WEAR_FINGER);
		strcpy(type, "ring");
	}
	else
	if (!str_prefix(slot,"neck")) 
	{
		SET_BIT(nSlot,  ITEM_WEAR_NECK);
		strcpy(type, "necklace");
	}
	else
	{
		send_to_char("Wear location choices are: ear, finger, or neck.\n\r", ch);
		return;
	}

	//At this point, we know what the character wants to make, and what the character wants to use to make it, now we have to find out if the character has what it takes to make it.



        int coppercount = 0, silvercount=0, goldcount = 0;
        OBJ_DATA * obj = NULL;
        for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
        {
                if ( obj->wear_loc == WEAR_NONE
                &&   (can_see_obj( ch, obj ) ) )
                {
                    if (obj->item_type == ITEM_ORE)
                    {
                        if (IS_SET(obj->value[0], MINING_ORE_COPPER))
                        {
                                if (IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) ||
                                    IS_SET(obj->value[1], MINING_QUALITY_PERFECT))
                                {
                                        coppercount++;
                                } }
                        if (IS_SET(obj->value[0], MINING_ORE_SILVER))
                        {
                                if (IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) ||
                                    IS_SET(obj->value[1], MINING_QUALITY_PERFECT))
                                {
                                        silvercount++;
                                }
                        }
                        if (IS_SET(obj->value[0], MINING_ORE_GOLD))
                        {
                                if (IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) ||
                                    IS_SET(obj->value[1], MINING_QUALITY_PERFECT))
                                {
                                        goldcount++;
                                }
                        }
                    }
                }
        }

	int rubycount = 0, emeraldcount=0, diamondcount = 0;
        obj = NULL;
        for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
        {
                if ( obj->wear_loc == WEAR_NONE
                &&   (can_see_obj( ch, obj ) ) )
                {
                    if (obj->item_type == ITEM_GEMSTONE) 
 		    {
			if (IS_SET(obj->value[0], MINING_GEM_RUBY))
			{
				if (IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) || 
				    IS_SET(obj->value[1], MINING_QUALITY_PERFECT)) 
				{
					rubycount++;
				} } 
			if (IS_SET(obj->value[0], MINING_GEM_EMERALD))
			{
				if (IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) || 
				    IS_SET(obj->value[1], MINING_QUALITY_PERFECT)) 
				{
					emeraldcount++;
				}
			} 
			if (IS_SET(obj->value[0], MINING_GEM_DIAMOND))
			{
				if (IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) || 
				    IS_SET(obj->value[1], MINING_QUALITY_PERFECT)) 
				{
					diamondcount++;
				}
			} 
                    }
                }
        }
	switch (nOre1Type) 
	{
		case MINING_ORE_COPPER: coppercount--; break;
		case MINING_ORE_SILVER: silvercount--; break;
		case MINING_ORE_GOLD  : goldcount--; break;
		default: break;
	}

	switch (nGem2Type) 
	{
		case MINING_GEM_RUBY: rubycount--; break;
		case MINING_GEM_EMERALD: emeraldcount--; break;
		case MINING_GEM_DIAMOND: diamondcount--; break;
		default: break;
	}

	if ((coppercount < 0)  || (silvercount < 0) || (goldcount < 0))
	{
		send_to_char("You do not have the required ore for that.\n\r", ch);
		return;
	}
	if ((rubycount < 0)  || (emeraldcount < 0) || (diamondcount < 0))
	{
		send_to_char("You do not have the required gem for that.\n\r", ch);
		return;
	}
	
	int REALLYFailed = 0;
	int chance = skill / 3;
	if (chance < number_percent()) {
		if (number_percent() > ((skill / 3) + 25)) {
			send_to_char("You REALLY slipped up this time. Your materials shatter under the hammer.\n\r", ch);
			REALLYFailed = 1;
		}
		else
		{
			send_to_char("You failed, but managed to save the pieces.\n\r", ch);
			return;
		}
	}

	//Extract ore 1 from char
        OBJ_DATA * ore1Obj;
        OBJ_DATA * gem2Obj;

        OBJ_DATA * obj_next;
  	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    	{
       		obj_next = obj->next_content;
        	if ( obj->wear_loc == WEAR_NONE
        	&&   (can_see_obj( ch, obj ) ) )
        	{
		    if ((obj->item_type == ITEM_ORE) &&
			(IS_SET(obj->value[0], nOre1Type)) &&
			((IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) ||
			  IS_SET(obj->value[1], MINING_QUALITY_PERFECT)))) {
				obj_from_char(obj);
				ore1Obj = obj;
				break;
			}
        	}
    	}
	
  	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    	{
       		obj_next = obj->next_content;
        	if ( obj->wear_loc == WEAR_NONE
        	&&   (can_see_obj( ch, obj ) ) )
        	{
		    if ((obj->item_type == ITEM_GEMSTONE) &&
			(IS_SET(obj->value[0], nGem2Type)) &&
			((IS_SET(obj->value[1], MINING_QUALITY_EXCELLENT) ||
			  IS_SET(obj->value[1], MINING_QUALITY_PERFECT)))) {
				obj_from_char(obj);
				gem2Obj = obj;
				break;
			}
        	}
    	}
	int dammod = 0;
	int hitmod = 0;
	int hpmod = 0;
	int endurmod = 0;
	int airmod = 0;
	int firemod = 0;
	int earthmod = 0;
	int watermod = 0;
	int spiritmod = 0;
	int perfectOreBonus = 0;
	int perfectGemBonus = 0;
	if (IS_SET(ore1Obj->value[1], MINING_QUALITY_PERFECT)) {
		perfectOreBonus = number_range(10,15);
	}
	if (IS_SET(gem2Obj->value[1], MINING_QUALITY_PERFECT)) {
		perfectGemBonus = number_range(10,15);
	}

	int oreType = ore1Obj->value[0];
	int gemType = gem2Obj->value[0];
	//Calculate the bonuses
	if (oreType == MINING_ORE_SILVER)
	{
		if (gemType == MINING_GEM_RUBY) 
		{
			int sphere = number_range(1,5);
			int value = number_range(2,7);
			switch (sphere)
			{
				case 1: airmod += value + (perfectOreBonus / 2) + (perfectGemBonus / 2); break;
				case 2: watermod += value + (perfectOreBonus / 2) + (perfectGemBonus / 2); break;
				case 3: firemod += value + (perfectOreBonus / 2) + (perfectGemBonus / 2); break;
				case 4: earthmod += value + (perfectOreBonus / 2) + (perfectGemBonus / 2); break;
				case 5: spiritmod += value + (perfectOreBonus / 2) + (perfectGemBonus / 2); break;
				
			}	
		}
		else if (gemType == MINING_GEM_EMERALD)
		{
			firemod += number_range(1,3) + (perfectOreBonus / 2) + (perfectGemBonus / 2);
			earthmod += number_range(1,3) + (perfectOreBonus / 2) + (perfectGemBonus / 2);	
		}
		else if (gemType == MINING_GEM_DIAMOND)
		{
			watermod += number_range(1,3) + (perfectOreBonus / 2) + (perfectGemBonus / 2);
			airmod += number_range(1,3) + (perfectOreBonus / 2) + (perfectGemBonus / 2);
		}	
	}
	else
	if (oreType == MINING_ORE_GOLD) 
	{
		if (gemType == MINING_GEM_RUBY) 
		{
			hitmod = number_range(9,18) + perfectOreBonus + perfectGemBonus;
		}
		else if (gemType == MINING_GEM_EMERALD)
		{
			hitmod = number_range(7,13) + perfectGemBonus;
			dammod = number_range(7,13) + perfectOreBonus;

		}
		else if (gemType == MINING_GEM_DIAMOND)
		{
			dammod = number_range(9,18) + perfectOreBonus + perfectGemBonus;

		}	

	}
	else
	if (oreType == MINING_ORE_COPPER)
	{
		if (gemType == MINING_GEM_RUBY) 
		{
			endurmod += number_range(50,100) + (perfectOreBonus * 2) + (perfectGemBonus * 2);
		}
		else if (gemType == MINING_GEM_EMERALD)
		{
			endurmod += number_range(25,50) + (perfectOreBonus * 2);
			hpmod += number_range(25,50) + (perfectGemBonus * 2);

		}
		else if (gemType == MINING_GEM_DIAMOND)
		{
			hpmod += number_range(50,100) + (perfectOreBonus * 2) + (perfectGemBonus * 2);

		}	

	}

	//clean up the old objects
	extract_obj(ore1Obj);
	extract_obj(gem2Obj);

	if (REALLYFailed) {
		return;
	}

	//make the new armor
	OBJ_DATA * newItem = create_object(get_obj_index(OBJ_VNUM_CRAFTING_ARMOR), 0);
	SET_BIT(newItem->wear_flags, nSlot);

	if (nOre1Type == MINING_ORE_SILVER) {
		sprintf(short_desc,"a {Ws{Di{Wl{Dv{We{Dr{x %s with %s solitaire",type, stone);
	}
	else if (nOre1Type == MINING_ORE_COPPER) {
		sprintf(short_desc,"%s double row {rc{Rop{rp{Re{rr{x %s",stone, type);
	}
	else if (nOre1Type == MINING_ORE_GOLD) {
		sprintf(short_desc,"%s seven-stone {Yg{yol{Yd{yen{x %s",stone, type);
	}
	sprintf(long_desc,"%s lies here.",short_desc);
	sprintf(name, "jewelcrafted %s %s %s", flag_string(gem_types,nGem2Type), flag_string(ore_types, nOre1Type), type) ;

	newItem->short_descr = str_dup(short_desc);
	newItem->description  = str_dup(long_desc);
	newItem->name =  str_dup(name);
	newItem->owner = str_dup(ch->name);
	
	if (dammod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_DAMROLL;
        	af.modifier   = dammod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_jewelcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	
	if (hitmod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_HITROLL;
        	af.modifier   = hitmod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_jewelcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	if (hpmod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_HIT;
        	af.modifier   = hpmod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_jewelcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	
	if (endurmod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_ENDURANCE;
        	af.modifier   = endurmod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_jewelcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	if (airmod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_SPHERE_AIR;
        	af.modifier   = airmod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_jewelcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	if (watermod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_SPHERE_WATER;
        	af.modifier   = watermod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_jewelcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	if (earthmod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_SPHERE_EARTH;
        	af.modifier   = earthmod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_jewelcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	if (firemod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_SPHERE_FIRE;
        	af.modifier   = firemod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_jewelcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	if (spiritmod > 0)
        {
        	AFFECT_DATA af;
        	af.location   = APPLY_SPHERE_SPIRIT;
        	af.modifier   = spiritmod;
        	af.where      = TO_OBJECT;
        	af.type       = gsn_jewelcraft;
        	af.duration   = -1;
        	af.bitvector  = 0;
        	af.level      = newItem->level;
        	affect_to_obj(newItem, &af);
	}
	
	deduct_cost(ch,cost);
	obj_to_char(newItem,ch);
	sprintf(buffer,"SUCCESS: You have just created %s", newItem->short_descr);
	send_to_char(buffer,ch);
	

}


/*
 * Ironwill
 * Granted skill that provides stoneskin and sanctuary to non-casters
 * Caldazar 2009-11-21
 */
void do_ironwill(CHAR_DATA *ch, char *argument){

	AFFECT_DATA af;
	int skill;
	int duration;
	int modifier;

	skill = get_skill(ch, gsn_ironwill);
	duration = skill / 3;
	modifier = -1 * (skill / 5);
	if(skill < 1){
		send_to_char("Ironwill? What's that?", ch);
		return;
	}
    if (is_affected(ch, gsn_ironwill)){
    	affect_strip ( ch, gsn_ironwill );
		send_to_char("You feel more relaxed.\n\r",ch);
		act("$n suddenly seems more relaxed.", ch, NULL, NULL, TO_ROOM);
		return;
    }

    af.where     = TO_AFFECTS;
    af.casterId  = ch->id;
    af.type      = gsn_ironwill;
    af.level     = ch->level;
    af.duration  = duration;
    af.location  = APPLY_AC;
    af.modifier  = modifier;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char(ch, &af);
    act( "$n steels himself mentally.", ch, NULL, NULL, TO_ROOM );
    send_to_char("You steel yourself mentally.\n\r", ch);
    return;
}

/*
 * Concussion Blow
 * Granted skill that allows the character to blind and weaken an opponent
 * Caldazar 2009-11-21
 */
void do_concussionblow(CHAR_DATA *ch, char *argument){
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	AFFECT_DATA af;
	int chance;
	int duration;
	int modifier;
	int level;

	one_argument(argument,arg);

	/* If wrapped */
	if (IS_AFFECTED(ch, AFF_WRAPPED)) {
		send_to_char("{WAn unseen force prevents you from moving!{x\r\n", ch);
	    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
	    return;
	}

	/* If dazed */
	if (ch->daze > 0) {
		if(IS_SET(ch->merits, MERIT_CONCENTRATION) && number_chance(20)){
		// nothing
	    } else {
			send_to_char("You can't use concussion blow while in a daze.\r\n", ch);
			return;
		}
	}

	/* If bound */
	if(IS_AFFECTED(ch, AFF_BIND)){
		send_to_char("You are tied up and unable to move!\n\r", ch);
		act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
		return;
	}

	if((victim = get_char_room(ch,arg)) == NULL){
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

	if (!IS_SAME_WORLD(victim, ch)){ // If the potential victim is in TAR
		send_to_char("They aren't here.", ch);
		return;
	}
	if (ch == victim){ // If the potential victim is the character executing, disregard
		send_to_char("You probably don't want to hit yourself.", ch);
		return;
	}
	if (IS_GHOLAM(victim)){ // If the potential victim is a Gholam, don't bother
		send_to_char("Don't bother.", ch);
		return;
	}
	if (is_same_group(victim, ch)){ // If the potential victim is in the same group as the character, disregard
	   	send_to_char("But they're in your group!", ch);
		return;
	}

	chance = get_skill(ch, gsn_concussionblow)/4;
	if(number_percent() > chance){
		act("{wYou try to lay waste to $N's head, but miss.{x",ch,NULL,victim,TO_CHAR);
		act("{w$n tries to lay waste to your head, but misses.{x",ch,NULL,victim,TO_VICT);
		act("{w$n tries to lay waste to $N's head, but misses.!{x.",ch,NULL,victim,TO_NOTVICT);
	} else {
		duration = get_skill(ch, gsn_concussionblow)/50;
		modifier = -1 * (get_skill(ch, gsn_concussionblow)/12);
		level = ch->level;

		// Blind them
		af.where     = TO_AFFECTS;
		af.casterId  = ch->id;
		af.type      = gsn_concussionblow;
		af.level     = level;
		af.location  = APPLY_HITROLL;
		af.modifier  = modifier;
		af.duration  = duration;
		af.bitvector = AFF_BLIND;
		affect_to_char( victim, &af );
		send_to_char( "You are blinded!\n\r", victim );
		act("$n appears to be blinded.",victim,NULL,NULL,TO_ROOM);

		// Weaken them
		af.where     = TO_AFFECTS;
		af.casterId  = ch->id;
		af.type      = gsn_concussionblow;
		af.level     = level;
		af.duration  = duration;
		af.location  = APPLY_STR;
		af.modifier  = modifier;
		af.bitvector = AFF_WEAKEN;
		affect_to_char( victim, &af );
		send_to_char( "You feel your strength slip away.\n\r", victim );
		act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM);

		act("{wYou lay waste to $N's head, sending $M reeling!{x",ch,NULL,victim,TO_CHAR);
		act("{w$n lays waste to your head, sending you reeling!{x",ch,NULL,victim,TO_VICT);
		act("{w$n lays waste to $N's head, sending $M reeling!{x.",ch,NULL,victim,TO_NOTVICT);
	}
	damage(ch, victim, 0, gsn_concussionblow,DAM_NONE,FALSE);
	WAIT_STATE(ch, skill_table[gsn_concussionblow].beats);
}


/*
 * Flame and the void
 * Skill adding hitroll for now
 * Acec 2010-11-18
 */
void do_flamevoid (CHAR_DATA *ch, char *argument){

	AFFECT_DATA af;
	int skill;
	int duration;
	int modifier;
	int chance;

	skill = get_skill(ch, gsn_flamevoid);
	duration = skill / 3;
	modifier = 1 * (skill / 10);
	
	if(IS_SET(ch->merits, MERIT_CONCENTRATION)){
		modifier = 1 * (skill / 8);
	}
	
	if(skill < 1){
		send_to_char("You haven't got that figured out yet.\n\r", ch);
		return;
	}

	if (is_affected(ch, gsn_flamevoid)){
    	affect_strip ( ch, gsn_flamevoid );
		send_to_char("You let go of the void and all emotions and thoughts falls back onto your shoulders.\n\r",ch);
		act("$n suddenly seems more relaxed.", ch, NULL, NULL, TO_ROOM);
		return;
		
		
	}
	chance = get_skill(ch, gsn_flamevoid);
	if(number_percent() > chance){
		act( "$n's serene expression faulters.", ch, NULL, NULL, TO_ROOM );
		send_to_char("You enter the void, visualizing a flame, but it breaks into a thousand pieces.\n\r", ch);
		return;
	}
    af.where     = TO_AFFECTS;
    af.casterId  = ch->id;
    af.type      = gsn_flamevoid;
    af.level     = ch->level;
    af.duration  = duration;
    af.location  = APPLY_HITROLL;
    af.modifier  = modifier;
    af.bitvector = 0;
    affect_to_char(ch, &af);
    act( "$n suddenly seems more focused.", ch, NULL, NULL, TO_ROOM );
    send_to_char("You enter the void, visualizing a flame, feeding all concerns, emotions and thoughts into it.\n\r", ch);
    return;
}
