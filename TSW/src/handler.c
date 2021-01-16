/**************************************************************************r
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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "olc.h"

/*
 * Local functions.
 */
void reset_gstatus(CHAR_DATA *ch);        /* guild.c      */
void reset_oguild_status(CHAR_DATA *ch);  /* oguild.c     */
void reset_sguild_status(CHAR_DATA *ch);  /* sguild.c     */ 
void reset_ssguild_status(CHAR_DATA *ch); /* ssguild.c    */
void reset_dfstatus(CHAR_DATA *ch);       /* darkfriend.c */
void reset_mstatus(CHAR_DATA *ch);        /* minion.c     */
 
void	affect_modify	args( ( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd ) );


/* friend stuff -- for NPC's mostly */
bool is_friend(CHAR_DATA *ch,CHAR_DATA *victim)
{
    if (is_same_group(ch,victim))
	return TRUE;

    
    if (!IS_NPC(ch))
	return FALSE;

    if (!IS_NPC(victim))
    {
	if (IS_SET(ch->off_flags,ASSIST_PLAYERS))
	    return TRUE;
	else
	    return FALSE;
    }

    if (IS_AFFECTED(ch,AFF_CHARM))
	return FALSE;

    if (IS_SET(ch->off_flags,ASSIST_ALL))
	return TRUE;

    if (ch->group && ch->group == victim->group)
	return TRUE;

    if (IS_SET(ch->off_flags,ASSIST_VNUM) 
    &&  ch->pIndexData == victim->pIndexData)
	return TRUE;

    if (IS_SET(ch->off_flags,ASSIST_RACE) && ch->race == victim->race)
	return TRUE;
     
    if (IS_SET(ch->off_flags,ASSIST_ALIGN)
    &&  !IS_SET(ch->act,ACT_NOALIGN) && !IS_SET(victim->act,ACT_NOALIGN)
    &&  ((IS_GOOD(ch) && IS_GOOD(victim))
    ||	 (IS_EVIL(ch) && IS_EVIL(victim))
    ||   (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
	return TRUE;

    return FALSE;
}

/* returns number of people on an object */
int count_users(OBJ_DATA *obj)
{
    CHAR_DATA *fch;
    int count = 0;

    if (obj->in_room == NULL)
	return 0;

    for (fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room)
	if (fch->on == obj)
	    count++;

    return count;
}
     
/* returns material number */
int material_lookup (const char *name)
{
    return 0;
}

int weapon_lookup (const char *name)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
	if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
	&&  !str_prefix(name,weapon_table[type].name))
	    return type;
    }
 
    return -1;
}

int weapon_type (const char *name)
{
    int type;
 
    for (type = 0; weapon_class[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(weapon_class[type].name[0])
        &&  !str_prefix(name,weapon_class[type].name))
            return weapon_class[type].bit;
    }
 
    return WEAPON_EXOTIC;
}

char *item_name(int item_type)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
	if (item_type == item_table[type].type)
	    return item_table[type].name;
    return "none";
}

char *weapon_name( int weapon_type)
{
    int type;
 
    for (type = 0; weapon_class[type].name != NULL; type++)
        if (weapon_type == weapon_class[type].bit)
            return weapon_class[type].name;
    return "exotic";
}

int attack_lookup  (const char *name)
{
    int att;

    for ( att = 0; attack_table[att].name != NULL; att++)
    {
	if (LOWER(name[0]) == LOWER(attack_table[att].name[0])
	&&  !str_prefix(name,attack_table[att].name))
	    return att;
    }

    return 0;
}

/* returns a flag for wiznet */
long wiznet_lookup (const char *name)
{
    int flag;

    for (flag = 0; wiznet_table[flag].name != NULL; flag++)
    {
	if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
	&& !str_prefix(name,wiznet_table[flag].name))
	    return flag;
    }

    return -1;
}

/* returns class number */
int class_lookup (const char *name)
{
   int class;
 
   for ( class = 0; class < MAX_CLASS; class++)
   {
        if (LOWER(name[0]) == LOWER(class_table[class].name[0])
        &&  !str_prefix( name,class_table[class].name))
            return class;
   }
 
   return -1;
}

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.c */

int check_immune(CHAR_DATA *ch, int dam_type)
{
  int immune, def;
  int bit;
  
  immune = -1;
  def = IS_NORMAL;
  
  if (dam_type == DAM_NONE)
    return immune;
    
  if (dam_type <= 3 || dam_type == DAM_TW || dam_type == DAM_SD || dam_type == DAM_MF || dam_type == DAM_DU || dam_type == DAM_CRIT) {
    if (IS_SET(ch->imm_flags,IMM_WEAPON))
	 def = IS_IMMUNE;
    else if (IS_SET(ch->res_flags,RES_WEAPON))
	 def = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags,VULN_WEAPON))
	 def = IS_VULNERABLE;
  }
  else { /* magical attack */
    if (IS_SET(ch->imm_flags,IMM_MAGIC))
	 def = IS_IMMUNE;
    else if (IS_SET(ch->res_flags,RES_MAGIC))
	 def = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags,VULN_MAGIC))
	 def = IS_VULNERABLE;
  }
  
  /* set bits to check -- VULN etc. must ALL be the same or this will fail */
  switch (dam_type) {
  case(DAM_BASH):		bit = IMM_BASH;		break;
  case(DAM_PIERCE):	bit = IMM_PIERCE;	break;
  case(DAM_SLASH):	bit = IMM_SLASH;	break;
  case(DAM_FIRE):		bit = IMM_FIRE;		break;
  case(DAM_COLD):		bit = IMM_COLD;		break;
  case(DAM_LIGHTNING):	bit = IMM_LIGHTNING;	break;
  case(DAM_ACID):		bit = IMM_ACID;		break;
  case(DAM_POISON):	bit = IMM_POISON;	break;
  case(DAM_NEGATIVE):	bit = IMM_NEGATIVE;	break;
  case(DAM_HOLY):		bit = IMM_HOLY;		break;
  case(DAM_ENERGY):	bit = IMM_ENERGY;	break;
  case(DAM_WIND):	bit = IMM_WIND;	break;
  case(DAM_DISEASE):	bit = IMM_DISEASE;	break;
  case(DAM_DROWNING):	bit = IMM_DROWNING;	break;
  case(DAM_LIGHT):	bit = IMM_LIGHT;	break;
  case(DAM_CHARM):	bit = IMM_CHARM;	break;
  case(DAM_SOUND):	bit = IMM_SOUND;	break;
  case(DAM_SAP):		bit = IMM_BASH; 	break;
  case(DAM_HARM):		bit = IMM_HARM; 	break;
  default:		return def;
  }
  
  if (IS_SET(ch->imm_flags,bit))
    immune = IS_IMMUNE;
  else if (IS_SET(ch->res_flags,bit) && immune != IS_IMMUNE)
    immune = IS_RESISTANT;
  else if (IS_SET(ch->vuln_flags,bit)) {
    if (immune == IS_IMMUNE)
	 immune = IS_RESISTANT;
    else if (immune == IS_RESISTANT)
	 immune = IS_NORMAL;
    else
	 immune = IS_VULNERABLE;
  }
  
  if (immune == -1)
    return def;
  else
    return immune;
}

/* checks mob format */
bool is_old_mob(CHAR_DATA *ch)
{
    if (ch->pIndexData == NULL)
	return FALSE;
    else if (ch->pIndexData->new_format)
	return FALSE;
    return TRUE;
}
 
/* for returning skill information */
int get_skill(CHAR_DATA *ch, int sn)
{
  int skill=0;
  
  /* shorthand for level based skills */
  if (sn == -1)  {
    skill = ch->level * 5 / 2;
  }  
  else if (sn < -1 || sn > MAX_SKILL) {
    bug("Bad sn %d in get_skill.",sn);
    skill = 0;
  }  
  else if (!IS_NPC(ch)) {
    if (ch->level < skill_table[sn].skill_level[ch->class])
	 skill = 0;
    else
	 skill = ch->pcdata->learned[sn];
  }  
  else  { /* mobiles */    
    if (skill_table[sn].spell_fun != spell_null)
	 skill = 40 + 2 * ch->level;
    
    else if (sn == gsn_sneak || sn == gsn_hide)
	 skill = ch->level * 2 + 20;
    
    else if ((sn == gsn_dodge && 
		    IS_SET(ch->off_flags,OFF_DODGE)) ||
		   (sn == gsn_parry && IS_SET(ch->off_flags,OFF_PARRY)))
	 skill = ch->level * 2;
    
    else if (sn == gsn_shield_block)
	 skill = 10 + 2 * ch->level;
    
    else if (sn == gsn_second_attack 
		   && (IS_SET(ch->act,ACT_WARRIOR) || IS_SET(ch->act,ACT_THIEF)))
	 skill = 10 + 3 * ch->level;
    
    else if (sn == gsn_third_attack && IS_SET(ch->act,ACT_WARRIOR))
	 skill = 4 * ch->level - 40;
    
    else if (sn == gsn_fourth_attack && IS_SET(ch->act,ACT_WARRIOR))
	 skill = 4 * ch->level - 40;
    
    else if (sn == gsn_fifth_attack && IS_SET(ch->act,ACT_WARRIOR))
	 skill = 4 * ch->level - 40;
    
    else if (sn == gsn_hand_to_hand)
	 skill = 40 + 2 * ch->level;
    
    else if (sn == gsn_trip && IS_SET(ch->off_flags,OFF_TRIP))
	 skill = 10 + 3 * ch->level;
    
    else if (sn == gsn_bash && IS_SET(ch->off_flags,OFF_BASH))
	 skill = 10 + 3 * ch->level;
    
    else if (sn == gsn_disarm 
		   &&  (IS_SET(ch->off_flags,OFF_DISARM) 
			   ||   IS_SET(ch->act,ACT_WARRIOR)
			   ||	  IS_SET(ch->act,ACT_THIEF)))
	 skill = 20 + 3 * ch->level;
    
    else if (sn == gsn_berserk && IS_SET(ch->off_flags,OFF_BERSERK))
	 skill = 3 * ch->level;
    
    else if (sn == gsn_kick)
	 skill = 10 + 3 * ch->level;
    
    else if (sn == gsn_backstab && IS_SET(ch->act,ACT_THIEF))
	 skill = 20 + 2 * ch->level;
    
    else if (sn == gsn_assassinate && IS_SET(ch->act,ACT_THIEF))
	 skill = 20 + 2 * ch->level;
    
    else if (sn == gsn_rescue)
	 skill = 40 + ch->level; 
    
    else if (sn == gsn_alertness)
	 skill = ch->level + number_percent()/10; 
    
    else if (sn == gsn_awareness)
	 skill = ch->level + number_percent()/10; 
    
    else if (sn == gsn_observation)
	 skill = ch->level + number_percent()/10; 
    
	else if (sn == gsn_recall)
	  skill = 40 + ch->level;
    
    else if (sn == gsn_critical)
	 skill = ch->level / 2;
    
    else if (sn == gsn_sword
		   ||  sn == gsn_dagger
		   ||  sn == gsn_spear
		   ||  sn == gsn_mace
		   ||  sn == gsn_axe
		   ||  sn == gsn_flail
		   ||  sn == gsn_whip
		   ||  sn == gsn_polearm
		   ||  sn == gsn_lance
		   ||  sn == gsn_arrow
		   ||  sn == gsn_bow)
	 skill = 40 + 5 * ch->level / 2;
    
    else 
	 skill = 0;
  }
  
  if (ch->daze > 0) {
    if (skill_table[sn].spell_fun != spell_null)
	 skill /= 2;
    else
	 skill = 2 * skill / 3;
  }

  if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]  > 10 )
    skill = 9 * skill / 10;
  
  return skill; //URANGE(0,skill,100);
}

/* for returning weapon information */
int get_weapon_sn(CHAR_DATA *ch)
{
  OBJ_DATA *wield;
  int sn;
  
  wield = get_eq_char( ch, WEAR_WIELD );
  if (wield == NULL || wield->item_type != ITEM_WEAPON)
    sn = gsn_hand_to_hand;
  else switch (wield->value[0]) {
  default :               sn = -1;                break;
  case(WEAPON_SWORD):     sn = gsn_sword;         break;
  case(WEAPON_DAGGER):    sn = gsn_dagger;        break;
  case(WEAPON_SPEAR):     sn = gsn_spear;         break;
  case(WEAPON_STAFF):	 sn = gsn_staff;         break;
  case(WEAPON_MACE):      sn = gsn_mace;          break;
  case(WEAPON_AXE):       sn = gsn_axe;           break;
  case(WEAPON_FLAIL):     sn = gsn_flail;         break;
  case(WEAPON_WHIP):      sn = gsn_whip;          break;
  case(WEAPON_POLEARM):   sn = gsn_polearm;       break;
  case(WEAPON_LANCE):     sn = gsn_lance;         break;
  case(WEAPON_BOW):       sn = gsn_bow;           break;
  case(WEAPON_ARROW):     sn = gsn_arrow;       break;
  }
  return sn;
}

int get_weapon_skill(CHAR_DATA *ch, int sn)
{
     int skill;

     /* -1 is exotic */
    if (IS_NPC(ch))
    {
	if (sn == -1)
	    skill = 3 * ch->level;
	else if (sn == gsn_hand_to_hand)
	    skill = 40 + 2 * ch->level;
	else 
	    skill = 40 + 5 * ch->level / 2;
    }
    
    else
    {
	if (sn == -1)
	    skill = 3 * ch->level;
	else
	    skill = ch->pcdata->learned[sn];
    }

    return URANGE(0,skill,100);
} 

//Used in acceptdeath to reset a players skills, stats, name, etc.
void reset_dead_char(CHAR_DATA *ch, char * argument, bool keep) 
{
  int i = 0;
  int sn = 0;
  //long newexp = 0;
  /* Increment the id by 1 to take care of people knowing the character */
  ch->id++;
  
  /* Give enough experience to get to 3/4 of the original level */
  ch->exp = (((ch->level + ch->pcdata->extended_level) * 3) / 4) * 2000;
/*

  for (i = 1; i <= (((ch->level + ch->pcdata->extended_level) * 3)/ 4); i++) {
    newexp +=  i * i * 300;
  }
  ch->exp = newexp;
*/
  
  free_string(ch->name);
  ch->name = str_dup(capitalize(argument));
  ch->train 	  = 0;	
  ch->level         = 0;
  ch->hit                     = MIN_PC_HP;
  ch->pcdata->perm_hit        = MIN_PC_HP;
  ch->pcdata->perm_endurance  = MIN_PC_END;
  ch->max_hit                 = MIN_PC_HP;
  ch->endurance               = MIN_PC_END;
  ch->max_endurance           = MIN_PC_END;
  free_string(ch->pcdata->bondedby);
  ch->pcdata->bondedby = &str_empty[0];
  free_string(ch->pcdata->restoremessage);
  ch->pcdata->restoremessage = &str_empty[0];
  free_string(ch->wkname);
  ch->wkname                  = &str_empty[0];
  free_string(ch->real_name);
  ch->real_name               = &str_empty[0];
  free_string(ch->short_descr);
  ch->short_descr             = &str_empty[0];
  free_string(ch->long_descr);
  ch->long_descr              = &str_empty[0];
  free_string(ch->description);
  ch->description             = &str_empty[0];
  free_string(ch->hood_description);
  ch->hood_description        = &str_empty[0];
  free_string(ch->veil_description);
  ch->veil_description        = &str_empty[0];
  free_string(ch->wolf_description);
  ch->wolf_description        = &str_empty[0];
  free_string(ch->wound_description);
  ch->wound_description       = &str_empty[0];
  free_string(ch->aura_description);
  ch->aura_description        = &str_empty[0];

  // Free voices
  free_string(ch->pcdata->say_voice);
  ch->pcdata->say_voice       = &str_empty[0];
  free_string(ch->pcdata->ask_voice);
  ch->pcdata->ask_voice       = &str_empty[0];  
  free_string(ch->pcdata->exclaim_voice);
  ch->pcdata->exclaim_voice   = &str_empty[0];  
  free_string(ch->pcdata->battlecry_voice);
  ch->pcdata->battlecry_voice = &str_empty[0];  
  
  for (i=0; i<MAX_SHEAT_LOC; i++)
    ch->sheat_where_name[i] = NULL;
  
  // Reset guild, sguild, ssguild, df and minions    	
  reset_gstatus(ch);
  reset_oguild_status(ch);
  reset_sguild_status(ch);
  reset_ssguild_status(ch);
  reset_dfstatus(ch);
  reset_mstatus(ch);
  
  for (i = 0; i < 4; i++) {
    ch->armor[i]            = 100;
  }
  
  for ( sn = 0; sn < MAX_SKILL; sn++ ) {
    ch->pcdata->learned[sn] = 0;
  }
  
  ch->bIsLinked               = FALSE;
  ch->bIsReadyForLink         = FALSE;
  ch->link_info               = NULL;
  ch->pIsLinkedBy	      = NULL;
  ch->position                = POS_STANDING;
  ch->study = FALSE;
  ch->study_pulse = -1;
  ch->arrow_count = 0;
  /* Standard linked list search */
  ch->pcdata->names = NULL;

  ch->pcdata->extended_level = 0;
  ch->pcdata->keepoldstats = keep;
  ch->pcdata->prev_class = ch->class;
	
  if (IS_SET(ch->affected_by, AFF_INFRARED)) {
    REMOVE_BIT(ch->affected_by, AFF_INFRARED);
  }
  
  if (IS_SET(ch->act,PLR_STILLED)) {
    REMOVE_BIT(ch->act, PLR_STILLED);
    for(i = 0; i < MAX_SPHERE; i++) {
	 ch->perm_sphere[i] = ch->cre_sphere[i];
    }
  }
  
  if (IS_SET(ch->act, PLR_IS_NEWBIEHELPER))
    REMOVE_BIT(ch->act, PLR_IS_NEWBIEHELPER);
  
  ch->ic_flags = 0;
  
  if (IS_SET(ch->merits, MERIT_WARDER))
    REMOVE_BIT(ch->merits, MERIT_WARDER);
}


/* used to de-screw characters */
void reset_char(CHAR_DATA *ch)
{
     int loc,mod,stat;
     OBJ_DATA *obj;
     AFFECT_DATA *af;
     int i;
     char buf[MAX_STRING_LENGTH];

     if (IS_NPC(ch))
	return;

    if (ch->pcdata->perm_hit == 0 
    ||	ch->pcdata->perm_endurance == 0
    ||	ch->pcdata->last_level == 0)
    {
    /* do a FULL reset */
	for (loc = 0; loc < MAX_WEAR; loc++)
	{
	    obj = get_eq_char(ch,loc);
	    if (obj == NULL)
		continue;
	    if (!obj->enchanted)
	    for ( af = obj->pIndexData->affected; af != NULL; af = af->next )
	    {
		mod = af->modifier;
		switch(af->location)
		{
		    case APPLY_SEX:	ch->sex		-= mod;
					if (ch->sex < 0 || ch->sex >2)
					    ch->sex = IS_NPC(ch) ?
						0 :
						ch->pcdata->true_sex;
									break;
		    case APPLY_ENDURANCE:	ch->max_endurance	-= mod;		break;
		    case APPLY_HIT:	ch->max_hit	-= mod;		break;
		}
	    }

            for ( af = obj->affected; af != NULL; af = af->next )
            {
                mod = af->modifier;
                switch(af->location)
                {
                    case APPLY_SEX:     ch->sex         -= mod;         break;
                    case APPLY_ENDURANCE:    ch->max_endurance    -= mod;         break;
                    case APPLY_HIT:     ch->max_hit     -= mod;         break;
                }
            }
	}
	/* now reset the permanent stats */
	ch->pcdata->perm_hit 	= ch->max_hit;
	ch->pcdata->perm_endurance 	= ch->max_endurance;
	ch->pcdata->last_level	= ch->played/3600;
	if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
	{	if (ch->sex > 0 && ch->sex < 3)
	    	    ch->pcdata->true_sex	= ch->sex;
		else
		    ch->pcdata->true_sex 	= 0;
        }
    }

    /* now restore the character to his/her true condition */
    for (stat = 0; stat < MAX_STATS; stat++)
	ch->mod_stat[stat] = 0;

    if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
	ch->pcdata->true_sex = 0; 
    ch->sex		= ch->pcdata->true_sex;
    ch->max_hit 	= ch->pcdata->perm_hit;
    ch->max_endurance	= ch->pcdata->perm_endurance;
   
    for (i = 0; i < 4; i++)
    	ch->armor[i]	= 100;

    ch->hitroll		= 0;
    ch->damroll		= 0;
    ch->saving_throw	= 0;

    /* now start adding back the effects */
    for (loc = 0; loc < MAX_WEAR; loc++)
    {
        obj = get_eq_char(ch,loc);
        if (obj == NULL)
            continue;
	for (i = 0; i < 4; i++)
	    ch->armor[i] -= apply_ac( obj, loc, i );

	// Don't add affects from weapon in scabbard
	if (loc == WEAR_SCABBARD_1 || loc == WEAR_SCABBARD_2)
       return;	
	
        if (!obj->enchanted)
	for ( af = obj->pIndexData->affected; af != NULL; af = af->next )
        {
            mod = af->modifier;
            switch(af->location)
            {
		case APPLY_STR:		ch->mod_stat[STAT_STR]	+= mod;	break;
		case APPLY_DEX:		ch->mod_stat[STAT_DEX]	+= mod; break;
		case APPLY_INT:		ch->mod_stat[STAT_INT]	+= mod; break;
		case APPLY_WIS:		ch->mod_stat[STAT_WIS]	+= mod; break;
		case APPLY_CON:		ch->mod_stat[STAT_CON]	+= mod; break;

		case APPLY_SEX:		ch->sex			+= mod; break;
		case APPLY_ENDURANCE:	ch->max_endurance		+= mod; break;
		case APPLY_HIT:		ch->max_hit		+= mod; break;

		case APPLY_AC:		
		    for (i = 0; i < 4; i ++)
			ch->armor[i] += mod; 
		    break;
		case APPLY_HITROLL:	ch->hitroll		+= mod; break;
		case APPLY_DAMROLL:	ch->damroll		+= mod; break;
	
		case APPLY_SAVES:		ch->saving_throw += mod; break;
		case APPLY_SAVING_ROD: 		ch->saving_throw += mod; break;
		case APPLY_SAVING_PETRI:	ch->saving_throw += mod; break;
		case APPLY_SAVING_BREATH: 	ch->saving_throw += mod; break;
		case APPLY_SAVING_SPELL:	ch->saving_throw += mod; break;
	    }
        }
 
        for ( af = obj->affected; af != NULL; af = af->next )
        {
            mod = af->modifier;
            switch(af->location)
            {
                case APPLY_STR:         ch->mod_stat[STAT_STR]  += mod; break;
                case APPLY_DEX:         ch->mod_stat[STAT_DEX]  += mod; break;
                case APPLY_INT:         ch->mod_stat[STAT_INT]  += mod; break;
                case APPLY_WIS:         ch->mod_stat[STAT_WIS]  += mod; break;
                case APPLY_CON:         ch->mod_stat[STAT_CON]  += mod; break;
 
                case APPLY_SEX:         ch->sex                 += mod; break;
                case APPLY_ENDURANCE:        ch->max_endurance            += mod; break;
                case APPLY_HIT:         ch->max_hit             += mod; break;
 
                case APPLY_AC:
                    for (i = 0; i < 4; i ++)
                        ch->armor[i] += mod;
                    break;
		case APPLY_HITROLL:     ch->hitroll             += mod; break;
                case APPLY_DAMROLL:     ch->damroll             += mod; break;
 
                case APPLY_SAVES:         ch->saving_throw += mod; break;
                case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
                case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
                case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
                case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
            }
	}
    }
  
    /* now add back spell effects */
    for (af = ch->affected; af != NULL; af = af->next)
    {
        mod = af->modifier;
        switch(af->location)
        {
                case APPLY_STR:         ch->mod_stat[STAT_STR]  += mod; break;
                case APPLY_DEX:         ch->mod_stat[STAT_DEX]  += mod; break;
                case APPLY_INT:         ch->mod_stat[STAT_INT]  += mod; break;
                case APPLY_WIS:         ch->mod_stat[STAT_WIS]  += mod; break;
                case APPLY_CON:         ch->mod_stat[STAT_CON]  += mod; break;
 
                case APPLY_SEX:         ch->sex                 += mod; break;
                case APPLY_ENDURANCE:        ch->max_endurance            += mod; break;
                case APPLY_HIT:         ch->max_hit             += mod; break;
 
                case APPLY_AC:
                    for (i = 0; i < 4; i ++)
                        ch->armor[i] += mod;
                    break;
                case APPLY_HITROLL:     ch->hitroll             += mod; break;
                case APPLY_DAMROLL:     ch->damroll             += mod; break;
 
                case APPLY_SAVES:         ch->saving_throw += mod; break;
                case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
                case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
                case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
                case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
        } 
    }

    /* make sure sex is RIGHT!!!! */
    if (ch->sex < 0 || ch->sex > 2)
	ch->sex = ch->pcdata->true_sex;

  /* Only Immortals is allowed to have short_desc */
  if (!IS_IMMORTAL(ch) && !IS_FORSAKEN(ch)) {
    if (ch->short_descr != NULL) {
	 strcpy (buf, "");
	 free_string(ch->short_descr);
	 ch->short_descr = str_dup(buf);
    }
  }
}


/*
 * Retrieve a character's trusted level for permission checking.
 */
int get_trust( CHAR_DATA *ch )
{
    if ( ch->desc != NULL && ch->desc->original != NULL )
	ch = ch->desc->original;

    if (ch->trust)
	return ch->trust;

    if ( IS_NPC(ch) && ch->level >= LEVEL_HERO )
	return LEVEL_HERO - 1;
    else
	return ch->level;
}


/*
 * Retrieve a character's age.
 */
int get_age( CHAR_DATA *ch )
{
    return 17 + ( ch->played + (int) (current_time - ch->logon) ) / 72000;
}

/* command for retrieving a characters maximum OP */
unsigned long get_curr_op(CHAR_DATA *ch)
{
  unsigned long max=0;  
  max = ((ch->perm_sphere[SPHERE_AIR])    + 
	    (ch->perm_sphere[SPHERE_EARTH])  +
	    (ch->perm_sphere[SPHERE_FIRE])   +
	    (ch->perm_sphere[SPHERE_SPIRIT]) +
	    (ch->perm_sphere[SPHERE_WATER]));
  return(max);
}

int get_curr_flows(CHAR_DATA *ch)
{
  int flows=0;
  AFFECT_DATA *paf;
  AFFECT_DATA *pWeave;
  CHAR_DATA *vch;

  // Ward
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  WARD_DATA *pWard=NULL;
  unsigned int vnum;
  
  /* Spin through all char in the list and check if we have a sustained  */
  /* weave on them                                                        */
  /* Do NOT count link affects as flows                                  */
  for ( vch = char_list; vch != NULL; vch = vch->next) {
    for ( paf = vch->affected; paf != NULL; paf = paf->next ) {
	 if (paf->casterId != ch->id)
	   continue;
	 else
	   if (paf->duration == SUSTAIN_WEAVE) {
		if (paf->type != skill_lookup("link")) {
		  flows++;
		}
	   }
    }
  }
  
  // Check if caster have any wards up that is sustained.
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->wards_cnt <= 0)
       continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWard = pRoom->wards; pWard != NULL; pWard = pWard->next) {		
		if (pWard->casterId != ch->id)
		  continue;
		else
		  if (pWard->duration == SUSTAIN_WEAVE)
		    flows++;
	   }
	 }
    }
  }

  // Check if caster have any weaves set on a room that is sustained
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->weave_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWeave = pRoom->weaves; pWeave != NULL; pWeave = pWeave->next) {		
		if (pWeave->casterId != ch->id)
		  continue;
		else
		  if (pWeave->duration == SUSTAIN_WEAVE)
		    flows++;
	   }
	 }
    }
  }

  
  return flows;
}

/* command for retrieving stats */
int get_curr_stat( CHAR_DATA *ch, int stat )
{
    int max;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
	max = 25;

    else
    {
#if defined(FIRST_BOOT)
	max = pc_race_table[ch->race].max_stats[stat] + 4;
#else
	max = race_table[ch->race].max_stats[stat] + 4;
#endif

	if (class_table[ch->class].attr_prime == stat)
	    max += 2;

	if ( ch->race == race_lookup("human"))
	    max += 1;

 	max = UMIN(max,25);
	if (ch->perm_stat[stat] > max)
	   max = ch->perm_stat[stat];
    }
	//max = UMAX(max,ch->perm_stat[stat]);
   
    return URANGE(3,ch->perm_stat[stat] + ch->mod_stat[stat], max);
}

int get_tot_stat(CHAR_DATA *ch)
{
  int totStat=0;
  int i;

  for (i=0; i < MAX_STATS; i++) {
    totStat += get_curr_stat(ch, i);
  }
  return(totStat);
}

/* command for returning max training score */
int get_max_train( CHAR_DATA *ch, int stat )
{
    int max;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
	return 25;

#if defined(FIRST_BOOT)
    max = pc_race_table[ch->race].max_stats[stat];
#else
    max = race_table[ch->race].max_stats[stat];
#endif
    if (class_table[ch->class].attr_prime == stat)
    {	if (ch->race == race_lookup("human"))
	   max += 3;
	else
	   max += 2;
    }
    return UMIN(max,25);
}
   
	
/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n( CHAR_DATA *ch )
{
    if ( !IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL )
	return 1000;

    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_PET) )
	return 0;

    return MAX_WEAR +  2 * get_curr_stat(ch,STAT_DEX) + ch->level;
}



/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w( CHAR_DATA *ch )
{
    if ( !IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL )
	return 10000000;

    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_PET) )
	return 0;

    return str_app[get_curr_stat(ch,STAT_STR)].carry * 10 + ch->level * 25;
}



/*
 * See if a string is one of the names of an object.
 */

bool is_name ( char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
    	return FALSE;

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
	return FALSE;

    string = str;
    /* we need ALL parts of string to match part of namelist */
    for ( ; ; )  /* start parsing string */
    {
	str = one_argument(str,part);

	if (part[0] == '\0' )
	    return TRUE;

	/* check to see if this is part of namelist */
	list = namelist;
	for ( ; ; )  /* start parsing namelist */
	{
	    list = one_argument(list,name);
	    if (name[0] == '\0')  /* this name was not found */
		return FALSE;

	    /* if (!str_prefix(string,name)) */
            if (!str_cmp(string,name))
		return TRUE; /* full pattern match */

	    if (!str_prefix(part,name))
 		break; /* partial pattern match */

            if (!str_cmp(part, name))
		break;
	}
    }
}

bool is_exact_name(char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];

    if (namelist == NULL)
	return FALSE;

    for ( ; ; )
    {
	namelist = one_argument( namelist, name );
	if ( name[0] == '\0' )
	    return FALSE;
	if ( !str_cmp( str, name ) )
	    return TRUE;
    }
}

/* enchanted stuff for eq */
void affect_enchant(OBJ_DATA *obj)
{
    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
        AFFECT_DATA *paf, *af_new;
        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected;
             paf != NULL; paf = paf->next)
        {
	    af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;
 
	    af_new->where	= paf->where;
            af_new->type        = UMAX(0,paf->type);
            af_new->level       = paf->level;
            af_new->duration    = paf->duration;
            af_new->location    = paf->location;
            af_new->modifier    = paf->modifier;
            af_new->bitvector   = paf->bitvector;
        }
    }
}
           

/*
 * Apply or remove an affect to a character.
 */
void affect_modify( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd )
{
  OBJ_DATA *wield;
  int mod,i;
  bool angrealMsg=FALSE;
  
  mod = paf->modifier;
  
  if ( fAdd ) {
    switch (paf->where) {
    case TO_AFFECTS:
	 SET_BIT(ch->affected_by, paf->bitvector);
	 break;
    case TO_IMMUNE:
	 SET_BIT(ch->imm_flags,paf->bitvector);
	 break;
    case TO_RESIST:
	 SET_BIT(ch->res_flags,paf->bitvector);
	 break;
    case TO_VULN:
	 SET_BIT(ch->vuln_flags,paf->bitvector);
	 break;
    case TO_APP:
	 SET_BIT(ch->app, paf->bitvector);
	 break;
    }
  }
  else {
    switch (paf->where) {
    case TO_AFFECTS:
	 REMOVE_BIT(ch->affected_by, paf->bitvector);
	 break;
    case TO_IMMUNE:
	 REMOVE_BIT(ch->imm_flags,paf->bitvector);
	 break;
    case TO_RESIST:
	 REMOVE_BIT(ch->res_flags,paf->bitvector);
	 break;
    case TO_VULN:
	 REMOVE_BIT(ch->vuln_flags,paf->bitvector);
	 break;
    case TO_APP:
	 REMOVE_BIT(ch->app, paf->bitvector);
	 act("The air ripples around $n for a slight moment.", ch, NULL, NULL, TO_ROOM);
	 break;
    }
    mod = 0 - mod;
  }

    switch ( paf->location )
    {
    default:
	bug( "Affect_modify: unknown location %d.", paf->location );
	return;

    case APPLY_NONE:						break;
    case APPLY_STR:           ch->mod_stat[STAT_STR]	+= mod;	break;
    case APPLY_DEX:           ch->mod_stat[STAT_DEX]	+= mod;	break;
    case APPLY_INT:           ch->mod_stat[STAT_INT]	+= mod;	break;
    case APPLY_WIS:           ch->mod_stat[STAT_WIS]	+= mod;	break;
    case APPLY_CON:           ch->mod_stat[STAT_CON]	+= mod;	break;
    case APPLY_SEX:           ch->sex			+= mod;	break;
    case APPLY_CLASS:						break;
    case APPLY_LEVEL:						break;
    case APPLY_AGE:						break;
    case APPLY_HEIGHT:						break;
    case APPLY_WEIGHT:						break;
    case APPLY_ENDURANCE:          ch->max_endurance		+= mod;	break;
    case APPLY_HIT:           ch->max_hit		+= mod;	break;
    case APPLY_GOLD:						break;
    case APPLY_EXP:						break;
    case APPLY_AC:
        for (i = 0; i < 4; i ++)
            ch->armor[i] += mod;
        break;
    case APPLY_HITROLL:       ch->hitroll		+= mod;	break;
    case APPLY_DAMROLL:       ch->damroll		+= mod;	break;
    case APPLY_SAVES:   ch->saving_throw		+= mod;	break;
    case APPLY_SAVING_ROD:    ch->saving_throw		+= mod;	break;
    case APPLY_SAVING_PETRI:  ch->saving_throw		+= mod;	break;
    case APPLY_SAVING_BREATH: ch->saving_throw		+= mod;	break;
    case APPLY_SAVING_SPELL:  ch->saving_throw		+= mod;	break;
    case APPLY_SPELL_AFFECT:  					break;

    case APPLY_SPHERE_AIR:    
	 if (ch->class == CLASS_CHANNELER && !IS_SET(ch->act,PLR_STILLED)) {
	   angrealMsg = TRUE;
	   ch->perm_sphere[SPHERE_AIR] += mod;
	 }
	 break;
    case APPLY_SPHERE_EARTH:  
	 if (ch->class == CLASS_CHANNELER && !IS_SET(ch->act,PLR_STILLED)) {
	   angrealMsg = TRUE;
	   ch->perm_sphere[SPHERE_EARTH] += mod;
	 }
	 break;
    case APPLY_SPHERE_FIRE:   
	 if (ch->class == CLASS_CHANNELER && !IS_SET(ch->act,PLR_STILLED)) {
	   angrealMsg = TRUE;
	   ch->perm_sphere[SPHERE_FIRE] += mod;
	 }
	 break;
    case APPLY_SPHERE_SPIRIT: 
	 if (ch->class == CLASS_CHANNELER && !IS_SET(ch->act,PLR_STILLED)) {
	   angrealMsg = TRUE;
	   ch->perm_sphere[SPHERE_SPIRIT] += mod;
	 }
	 break;
    case APPLY_SPHERE_WATER:  
	 if (ch->class == CLASS_CHANNELER && !IS_SET(ch->act,PLR_STILLED)) {
	   angrealMsg = TRUE;
	   ch->perm_sphere[SPHERE_WATER] += mod;
	 }
	 break;
    }

/*
    if (angrealMsg && !IS_NPC(ch)) {
	 if (mod > 1) {
	   sprintf(buf, "You feel as if you can draw more of %s.\n\r ", ch->sex == SEX_MALE ? "Saidin" : "Saidar");
	   send_to_char(buf, ch);
	   angrealMsg = FALSE;
	 }
	 else {
	   sprintf(buf, "You feel as if you can draw less of %s.\n\r ", ch->sex == SEX_MALE ? "Saidin" : "Saidar");
	   send_to_char(buf, ch);
	   angrealMsg = FALSE;
	 }
    }
*/

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    if ( !IS_NPC(ch) && ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL
    &&   get_obj_weight(wield) > (str_app[get_curr_stat(ch,STAT_STR)].wield*10))
    {
	static int depth;

	if ( depth == 0 )
	{
	    depth++;
	    act( "You drop $p.", ch, wield, NULL, TO_CHAR );
	    act( "$n drops $p.", ch, wield, NULL, TO_ROOM );
	    obj_from_char( wield );
	    obj_to_room( wield, ch->in_room );
	    depth--;
	}
    }

    return;
}


/* find an effect in an affect list */
AFFECT_DATA  *affect_find(AFFECT_DATA *paf, int sn)
{
    AFFECT_DATA *paf_find;
    
    for ( paf_find = paf; paf_find != NULL; paf_find = paf_find->next )
    {
        if ( paf_find->type == sn )
	return paf_find;
    }

    return NULL;
}

/* fix object affects when removing one */
void affect_check(CHAR_DATA *ch,int where,int vector)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0)
	return;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
	if (paf->where == where && paf->bitvector == vector)
	{
	    switch (where)
	    {
	        case TO_AFFECTS:
		    SET_BIT(ch->affected_by,vector);
		    break;
	        case TO_IMMUNE:
		    SET_BIT(ch->imm_flags,vector);   
		    break;
	        case TO_RESIST:
		    SET_BIT(ch->res_flags,vector);
		    break;
	        case TO_VULN:
		    SET_BIT(ch->vuln_flags,vector);
		    break;
	    }
	    return;
	}

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
	if (obj->wear_loc == -1)
	    continue;

            for (paf = obj->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS:
                        SET_BIT(ch->affected_by,vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT(ch->imm_flags,vector);
                        break;
                    case TO_RESIST:
                        SET_BIT(ch->res_flags,vector);
                        break;
                    case TO_VULN:
                        SET_BIT(ch->vuln_flags,vector);
                  
                }
                return;
            }

        if (obj->enchanted)
	    continue;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS:
                        SET_BIT(ch->affected_by,vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT(ch->imm_flags,vector);
                        break;
                    case TO_RESIST:
                        SET_BIT(ch->res_flags,vector);
                        break;
                    case TO_VULN:
                        SET_BIT(ch->vuln_flags,vector);
                        break;
                }
                return;
            }
    }
}





/*
 * Add a weave to a room.
 */
void weave_to_room(ROOM_INDEX_DATA *room, AFFECT_DATA *wd)
{
  AFFECT_DATA *wd_new;
  AFFECT_DATA *weave;
  AFFECT_DATA *weave_next;
  
  /* first check if weave of this kind already exist on room */
  /* if exist, only update weave, if same caster and weave type*/
  for (weave = room->weaves; weave != NULL; weave = weave_next) {
    weave_next = weave->next;
    if (weave->type == wd->type && weave->casterId == wd->casterId && weave->bitvector == wd->bitvector) {
	 weave->duration = wd->duration;
	 weave->tied_strength = wd->tied_strength;
	 return;
    }
  }
  
  wd_new = new_room_weave();
  
  wd->inverted = FALSE;
  
  *wd_new = *wd;
  
  VALIDATE(wd);
  
  wd_new->next = room->weaves;
  room->weaves  = wd_new;
  
  room->area->weave_cnt++;
  
  return;
}

bool is_room_weave_set(ROOM_INDEX_DATA *room, long weave_flag)
{
  AFFECT_DATA *pWeave;
  AFFECT_DATA *pWeave_next;
  
  for (pWeave = room->weaves; pWeave != NULL; pWeave = pWeave_next) {
     pWeave_next = pWeave->next;
     
     if (IS_SET(pWeave->bitvector, weave_flag))
        return TRUE;
  }
  
  return FALSE;
}

/*
 * Remove a weave from a Room
 */
void room_weave_remove(ROOM_INDEX_DATA *Room, AFFECT_DATA *Weave)
{

//  printf("room_weave_remove called for <%s>\n", skill_table[Weave->type].name);
  
  if ( Room->weaves == NULL ) {
    bug ("weave_remove: no residue to remove.", 0);
    return;
  }
  
  if (Weave == Room->weaves) {
    Room->weaves = Weave->next;
  }
  else {
    AFFECT_DATA *prev;

    for (prev = Room->weaves; prev != NULL; prev = prev->next) {
	 if (prev->next == Weave) {
	   prev->next = Weave->next;
	   break;
	 }
    }
    if (prev == NULL) {
	 bug ("weave_remove: cannot find Weave.", 0);
	 return;
    }
  }
  
  free_room_weave(Weave);
  
  Room->area->weave_cnt--;
  
  return;
}

/*
 * Add a ward to a room.
 */
void ward_to_room(ROOM_INDEX_DATA *room, WARD_DATA *wd)
{
  WARD_DATA *wd_new;
  WARD_DATA *ward;
  WARD_DATA *ward_next;
  
  /* first check if ward of this kind already exist on room */
  /* if exist, only update ward, if same caster and ward type*/
  for (ward = room->wards; ward != NULL; ward = ward_next) {
    ward_next = ward->next;
    if (ward->sn == wd->sn && ward->casterId == wd->casterId && ward->bitvector == wd->bitvector) {
	 ward->duration = wd->duration;
	 ward->strength = wd->strength;
	 return;
    }
  }
  
  wd_new = new_ward();
  
  wd->inverted = FALSE;
  
  *wd_new = *wd;
  
  VALIDATE(wd);
  
  wd_new->next = room->wards;
  room->wards  = wd_new;
  
  room->area->wards_cnt++;
  
  return;
}

bool is_ward_set(ROOM_INDEX_DATA *room, long ward_flag)
{
  WARD_DATA *pWard;
  WARD_DATA *pWard_next;
  
  for (pWard = room->wards; pWard != NULL; pWard = pWard_next) {
     pWard_next = pWard->next;
     
     if (IS_SET(pWard->bitvector, ward_flag))
        return TRUE;
  }
  
  return FALSE;
}

/*
 * Remove a ward from a Room
 */
void ward_remove(ROOM_INDEX_DATA *Room, WARD_DATA *Ward)
{
//  printf("ward_remove called for <%s>\n", skill_table[Ward->sn].name);
  CHAR_DATA *ch = NULL;

  if ( Room->wards == NULL ) {
    bug ("ward_remove: no residue to remove.", 0);
    return;
  }
  
  if (Ward == Room->wards) {
    Room->wards = Ward->next;
  }
  else {
    WARD_DATA *prev;

    for (prev = Room->wards; prev != NULL; prev = prev->next) {
	 if (prev->next == Ward) {
	   prev->next = Ward->next;
	   break;
	 }
    }
    if (prev == NULL) {
	 bug ("ward_remove: cannot find Ward.", 0);
	 return;
    }
  }
  
  ch = get_charId_world(ch,Ward->casterId);
  if (ch)
  {
     send_to_char("Your ward dissipitates.\r\n",ch);
  }
  free_ward(Ward);
  
  Room->area->wards_cnt--;
  
  return;
}

/*
 * Add a residue to a room.
 */
void residue_to_room(ROOM_INDEX_DATA *room, RESIDUE_DATA *rd)
{
  RESIDUE_DATA *rd_new;
  RESIDUE_DATA *res;
  RESIDUE_DATA *res_next;
  
  /* first check if residue of this kind already exist on room */
  /* if exist, only update residue, if same sex */
  for (res = room->residues; res != NULL; res = res_next) {
     res_next = res->next;
     if (res->sn == rd->sn && res->sex == rd->sex) {
       res->duration = rd->duration;
       return;
     }
  }

  rd_new = new_residue();

  *rd_new = *rd;

  VALIDATE(rd);

  rd_new->next = room->residues;
  room->residues = rd_new;
  
  return;
}

/*
 * Remove a residue from a Room
 */
void residue_remove(ROOM_INDEX_DATA *Room, RESIDUE_DATA *Residue)
{
  
  if ( Room->residues == NULL ) {
    bug ("Residue_remove: no residue to remove.", 0);
    return;
  }

  if (Residue == Room->residues) {
    Room->residues = Residue->next;
  }
  else {
    RESIDUE_DATA *prev;

    for (prev = Room->residues; prev != NULL; prev = prev->next) {
	 if (prev->next == Residue) {
	   prev->next = Residue->next;
	   break;
	 }
    }
    if (prev == NULL) {
	 bug ("Residue_remove: cannot find Residue.", 0);
	 return;
    }
  }

  free_residue(Residue);
  return;
}

/*
 * Give an affect to a char.
 */
void affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    paf->inverted = FALSE;

    *paf_new		= *paf;

    VALIDATE(paf);	/* in case we missed it when we set up paf */
    paf_new->next	= ch->affected;
    ch->affected	= paf_new;

    affect_modify( ch, paf_new, TRUE );
    return;
}

/* give an affect to an object */
void affect_to_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new		= *paf;

    VALIDATE(paf);	/* in case we missed it when we set up paf */
    paf_new->next	= obj->affected;
    obj->affected	= paf_new;

    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector)
        switch (paf->where)
        {
        case TO_OBJECT:
    	    SET_BIT(obj->extra_flags,paf->bitvector);
	    break;
        case TO_WEAPON:
	    if (obj->item_type == ITEM_WEAPON)
	        SET_BIT(obj->value[4],paf->bitvector);
	    break;
        }
    

    return;
}



/*
 * Remove an affect from a char.
 */
void affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    int where;
    int vector;

    if ( ch->affected == NULL )
    {
	bug( "Affect_remove: no affect.", 0 );
	return;
    }

    affect_modify( ch, paf, FALSE );
    where = paf->where;
    vector = paf->bitvector;

    if (paf->type == gsn_sap && ch->position == POS_DEAD) {
	ch->position = POS_SLEEPING;
    }

    if ( paf == ch->affected )
    {
	ch->affected	= paf->next;
    }
    else
    {
	AFFECT_DATA *prev;

	for ( prev = ch->affected; prev != NULL; prev = prev->next )
	{
	    if ( prev->next == paf )
	    {
		prev->next = paf->next;
		break;
	    }
	}

	if ( prev == NULL )
	{
	    bug( "Affect_remove: cannot find paf.", 0 );
	    return;
	}
    }

    free_affect(paf);

    affect_check(ch,where,vector);
    return;
}

void affect_remove_obj( OBJ_DATA *obj, AFFECT_DATA *paf)
{
    int where, vector;
    if ( obj->affected == NULL )
    {
        bug( "Affect_remove_object: no affect.", 0 );
        return;
    }

    if (obj->carried_by != NULL && obj->wear_loc != -1)
	affect_modify( obj->carried_by, paf, FALSE );

    where = paf->where;
    vector = paf->bitvector;

    /* remove flags from the object if needed */
    if (paf->bitvector)
	switch( paf->where)
        {
        case TO_OBJECT:
            REMOVE_BIT(obj->extra_flags,paf->bitvector);
            break;
        case TO_WEAPON:
            if (obj->item_type == ITEM_WEAPON)
                REMOVE_BIT(obj->value[4],paf->bitvector);
            break;
        }

    if ( paf == obj->affected )
    {
        obj->affected    = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;

        for ( prev = obj->affected; prev != NULL; prev = prev->next )
        {
            if ( prev->next == paf )
            {
                prev->next = paf->next;
                break;
            }
        }

        if ( prev == NULL )
        {
            bug( "Affect_remove_object: cannot find paf.", 0 );
            return;
        }
    }

    free_affect(paf);

    if (obj->carried_by != NULL && obj->wear_loc != -1)
	affect_check(obj->carried_by,where,vector);
    return;
}



/*
 * Strip all affects of a given sn.
 */
void affect_strip( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
	paf_next = paf->next;
	if ( paf->type == sn )
	    affect_remove( ch, paf );
    }

    return;
}



/*
 * Return true if a char is affected by a spell.
 */
bool is_affected( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;

    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
	if ( paf->type == sn )
	    return TRUE;
    }

    return FALSE;
}



/*
 * Add or enhance an affect.
 */
void affect_join( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_old;
    bool found;

    found = FALSE;
    for ( paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next )
    {
	if ( paf_old->type == paf->type )
	{
	    paf->level = (paf->level += paf_old->level) / 2;
	    paf->duration += paf_old->duration;
	    paf->modifier += paf_old->modifier;
	    affect_remove( ch, paf_old );
	    break;
	}
    }

    affect_to_char( ch, paf );
    return;
}



/*
 * Move a char out of a room.
 */

void char_from_room( CHAR_DATA * ch)
{
    OBJ_DATA *obj;
    char buf[MSL];

    if ( ch->in_room == NULL )
    {
	bug( "Char_from_room: NULL.", 0 );
	return;
    }

    if ( !IS_NPC(ch) )
	--ch->in_room->area->nplayer;

    if( ch->in_obj && ch->in_obj->item_type >= 0 ) remove_char_from_obj(ch);

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
    &&   obj->item_type == ITEM_LIGHT
    &&   obj->value[2] != 0
    &&   ch->in_room->light > 0 )
	--ch->in_room->light;

    if ( ch == ch->in_room->people )
    {
	ch->in_room->people = ch->next_in_room;
    }
    else
    {
	CHAR_DATA *prev;

	for ( prev = ch->in_room->people; prev; prev = prev->next_in_room )
	{
	    if ( prev->next_in_room == ch )
	    {
		prev->next_in_room = ch->next_in_room;
		break;
	    }
	}

	if ( prev == NULL ) {
	    sprintf(buf, "Char_from_room: ch <%s> not found in room <%d>.", IS_NPC(ch) ? ch->short_descr : ch->name, ch->in_room->vnum);
	    bug(buf, 0); 
	    //bug( "Char_from_room: ch not found.", 0 );
	}
    }

    ch->in_room      = NULL;
    ch->next_in_room = NULL;
    ch->on 	     = NULL;  /* sanity check! */
    if (MOUNTED(ch))
    {
     ch->mount->riding	= FALSE;
     ch->riding		= FALSE;
    }

    if (RIDDEN(ch))
    {
     ch->mount->riding	= FALSE;
     ch->riding		= FALSE;
    }

	return;
}



/*
 * Move a char into a room.
 */
void char_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    OBJ_DATA *obj;

    if ( pRoomIndex == NULL )
    {
	ROOM_INDEX_DATA *room;

	bug( "Char_to_room: NULL.", 0 );
	
	if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
	    char_to_room(ch,room);
	
	return;
    }

    ch->in_room		= pRoomIndex;
    ch->next_in_room	= pRoomIndex->people;
    pRoomIndex->people	= ch;

    if ( !IS_NPC(ch) )
    {
	if (ch->in_room->area->empty)
	{
	    ch->in_room->area->empty = FALSE;
	    ch->in_room->area->age = 0;
	}
	++ch->in_room->area->nplayer;

	if (ch->desc->editor == ED_ROOM)
	{
		do_redit(ch,"");
	}
    }

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
    &&   obj->item_type == ITEM_LIGHT
    &&   obj->value[2] != 0 )
	++ch->in_room->light;
	
    if (IS_AFFECTED(ch,AFF_PLAGUE))
    {
        AFFECT_DATA *af, plague;
        CHAR_DATA *vch;
        
        for ( af = ch->affected; af != NULL; af = af->next )
        {
            if (af->type == gsn_plague)
                break;
        }
        
        if (af == NULL)
        {
            REMOVE_BIT(ch->affected_by,AFF_PLAGUE);
            return;
        }
        
        if (af->level == 1)
            return;
        
	plague.where		= TO_AFFECTS;
        plague.type 		= gsn_plague;
        plague.level 		= af->level - 1; 
        plague.duration 	= number_range(1,2 * plague.level);
        plague.location		= APPLY_STR;
        plague.modifier 	= -5;
        plague.bitvector 	= AFF_PLAGUE;
        
        for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!saves_spell(plague.level - 2,vch,DAM_DISEASE) 
	    &&  !IS_IMMORTAL(vch) &&
            	!IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(6) == 0)
            {
            	send_to_char("You feel hot and feverish.\n\r",vch);
            	act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM);
            	affect_join(vch,&plague);
            }
        }
    }

    if (is_ward_set(ch->in_room, WARD_LIGHT)) {
	if (( IS_AFFECTED(ch, AFF_HIDE)) || 
	    ( IS_AFFECTED(ch, AFF_SNEAK)))
	{
		send_to_char("There is no place to hide here!\r\n",ch);
		do_visible(ch,"");
	}
    }

    return;
}



/*
 * Give an obj to a char.
 */
void obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch )
{
    obj->next_content	 = ch->carrying;
    ch->carrying	 = obj;
    obj->carried_by	 = ch;
    obj->in_room	 = NULL;
    obj->in_obj		 = NULL;
    if ((obj->item_type != ITEM_KEY) && (obj->item_type != ITEM_ROOM_KEY)) {
    	ch->carry_number	+= get_obj_number( obj );
    	ch->carry_weight	+= get_obj_weight( obj );
    }
}




/*
 * Take an obj from its character.
 */
void obj_from_char( OBJ_DATA *obj )
{
    CHAR_DATA *ch;

    if ( ( ch = obj->carried_by ) == NULL )
    {
	bug( "Obj_from_char: null ch.", 0 );
	return;
    }

    if ( obj->wear_loc != WEAR_NONE )
	unequip_char( ch, obj );

    if ( ch->carrying == obj )
    {
	ch->carrying = obj->next_content;
    }
    else
    {
	OBJ_DATA *prev;

	for ( prev = ch->carrying; prev != NULL; prev = prev->next_content )
	{
	    if ( prev->next_content == obj )
	    {
		prev->next_content = obj->next_content;
		break;
	    }
	}

	if ( prev == NULL )
	    bug( "Obj_from_char: obj not in list.", 0 );
    }

    obj->carried_by	 = NULL;
    obj->next_content	 = NULL;
    if ((obj->item_type != ITEM_KEY) && (obj->item_type != ITEM_ROOM_KEY)) {
       ch->carry_number	-= get_obj_number( obj );
       ch->carry_weight	-= get_obj_weight( obj );
    }
    return;
}



/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac( OBJ_DATA *obj, int iWear, int type )
{
    if ( obj->item_type != ITEM_ARMOR )
	return 0;

    switch ( iWear )
    {
    case WEAR_BODY:	return 3 * obj->value[type];
    case WEAR_HEAD:	return 2 * obj->value[type];
    case WEAR_LEGS:	return 2 * obj->value[type];
    case WEAR_FEET:	return     obj->value[type];
    case WEAR_HANDS:	return     obj->value[type];
    case WEAR_ARMS:	return     obj->value[type];
    case WEAR_SHIELD:	return     obj->value[type];
    case WEAR_NECK_1:	return     obj->value[type];
    case WEAR_NECK_2:	return     obj->value[type];
    case WEAR_ABOUT:	return 2 * obj->value[type];
    case WEAR_WAIST:	return     obj->value[type];
    case WEAR_WRIST_L:	return     obj->value[type];
    case WEAR_WRIST_R:	return     obj->value[type];
    case WEAR_HOLD:	return     obj->value[type];
    case WEAR_TATTOO:	return     obj->value[type];
    case WEAR_BACK:	return     obj->value[type];
    case WEAR_STUCK_IN:	return     obj->value[type];
    case WEAR_FACE:	return     obj->value[type];
    case WEAR_EAR_L:	return     obj->value[type] / 2;
    case WEAR_EAR_R:	return     obj->value[type] / 2;
    case WEAR_FINGER_L: return     obj->value[type];
    case WEAR_FINGER_R: return     obj->value[type];
    case WEAR_FLOAT:    return     obj->value[type] / 4;
    case WEAR_LIGHT:    return     obj->value[type] / 4;
    }

    return 0;
}



/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *get_eq_char( CHAR_DATA *ch, int iWear )
{
    OBJ_DATA *obj;

    if (ch == NULL)
	return NULL;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
	if ( obj->wear_loc == iWear )
	    return obj;
    }

    return NULL;
}



/*
 * Equip a char with an obj.
 */
void equip_char( CHAR_DATA *ch, OBJ_DATA *obj, int iWear )
{
    AFFECT_DATA *paf;
    int i;

    if ( get_eq_char( ch, iWear ) != NULL )
    {
	bug( "Equip_char: already equipped (%d).", iWear );
	return;
    }


    for (i = 0; i < 4; i++)
    	ch->armor[i]      	-= apply_ac( obj, iWear,i );
    obj->wear_loc	 = iWear;

    // Don't add affects from weapon in scabbard
    if (iWear == WEAR_SCABBARD_1 || iWear == WEAR_SCABBARD_2)
       return;

    if (!obj->enchanted)
	for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
	    if ( paf->location != APPLY_SPELL_AFFECT )
	        affect_modify( ch, paf, TRUE );
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
	if ( paf->location == APPLY_SPELL_AFFECT )
    	    affect_to_char ( ch, paf );
	else
	    affect_modify( ch, paf, TRUE );

    if ( obj->item_type == ITEM_LIGHT
    &&   obj->value[2] != 0
    &&   ch->in_room != NULL )
	++ch->in_room->light;

    if (iWear == WEAR_STUCK_IN && IS_NPC(ch)) {
	if (number_percent() < 50)
		do_remove(ch,obj->name);
    }

    //Is the player putting on the foxhead medallion?
    if (!IS_NPC(ch))
    {
    	if (obj->pIndexData->vnum == OBJ_VNUM_FOXHEAD_MEDALLION)
    	{
		do_function(ch, &do_unchannel, "" );
                AFFECT_DATA * aff;
                AFFECT_DATA * aff_next;
		for (aff = ch->affected; aff != NULL; aff = aff_next) {
			aff_next = aff->next;
			int csn = aff->type;
			if (foxhead_immune(ch,csn)) {
            			affect_strip( ch, csn );
			}
		}
		char buf[MAX_STRING_LENGTH];
        	sprintf( buf, "{D[{WBOND{D]{W: {s%s {s%s{x\n\r",COLORNAME(ch), colorcontinue('s', "can no longer be felt in your head.\r\n"));
		CHAR_DATA *bch;
        	for ( bch = char_list; bch != NULL; bch = bch->next ) {
	   		if (bch == ch)
				continue;
           		if (!IS_NPC(bch)) {
	      			if ( is_bonded_to( ch, bch ) ) {
	         			send_to_char( buf, bch );
	         			do_addchannelhistory(ch,bch->pcdata->bond_history,buf, NULL);
		 			if (!str_cmp(ch->name, bch->pcdata->bondedby)) {
  						strcpy (buf, "");
  						free_string(bch->pcdata->bondedby);
  						bch->pcdata->bondedby = str_dup(buf);
		 			}
	      			}
           		}
        	}
        	if (!IS_NULLSTR(ch->pcdata->bondedby)) {
			send_to_char("You no longer feel your bonded connection.\r\n",ch);	
  			strcpy (buf, "");
  			free_string(ch->pcdata->bondedby);
  			ch->pcdata->bondedby = str_dup(buf);
        	}
   	}
   }
}



/*
 * Unequip a char with an obj.
 */
void unequip_char( CHAR_DATA *ch, OBJ_DATA *obj )
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *lpaf = NULL;
    AFFECT_DATA *lpaf_next = NULL;
    int i;

    if ( obj->wear_loc == WEAR_NONE )
    {
	bug( "Unequip_char: already unequipped.", 0 );
	return;
    }

    if ( obj->wear_loc == WEAR_STUCK_IN) {
	if (ch->arrow_count > 1) {
		ch->arrow_count--;
		return;
        }
	else {
		ch->arrow_count--;
        }
    }
    for (i = 0; i < 4; i++)
    	ch->armor[i]	+= apply_ac( obj, obj->wear_loc,i );
    obj->wear_loc	 = -1;

    if (!obj->enchanted)
    {	for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
	    if ( paf->location == APPLY_SPELL_AFFECT )
	    {
	        for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
	        {
		    lpaf_next = lpaf->next;
		    if ((lpaf->type == paf->type) &&
		        (lpaf->level == paf->level) &&
		        (lpaf->location == APPLY_SPELL_AFFECT))
		    {
		        affect_remove( ch, lpaf );
			lpaf_next = NULL;
		    }
	        }
	    }
	    else
	    {
	        affect_modify( ch, paf, FALSE );
		affect_check(ch,paf->where,paf->bitvector);
	    }
    }
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
	if ( paf->location == APPLY_SPELL_AFFECT )
	{
	    bug ( "Norm-Apply: %d", 0 );
	    for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
	    {
		lpaf_next = lpaf->next;
		if ((lpaf->type == paf->type) &&
		    (lpaf->level == paf->level) &&
		    (lpaf->location == APPLY_SPELL_AFFECT))
		{
		    bug ( "location = %d", lpaf->location );
		    bug ( "type = %d", lpaf->type );
		    affect_remove( ch, lpaf );
		    lpaf_next = NULL;
		}
	    }
	}
	else
	{
	    affect_modify( ch, paf, FALSE );
	    affect_check(ch,paf->where,paf->bitvector);	
	}

    if ( obj->item_type == ITEM_LIGHT
	    &&   obj->value[2] != 0
	    &&   ch->in_room != NULL
	    &&   ch->in_room->light > 0 )
	 --ch->in_room->light;

    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT)) {
	 if (IS_CLOAKED(ch)) {
	   REMOVE_BIT(ch->app,APP_CLOAKED);
	 }
	 if (IS_COLORCLOAKED(ch)) {
	   REMOVE_BIT(ch->app, APP_COLORCLOAKED);
	 }
	 if (IS_HOODED(ch)) {
	   REMOVE_BIT(ch->app,APP_HOODED);
	 }
    }
    
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD)) {
       if (IS_VEILED(ch)) {
         REMOVE_BIT(ch->app,APP_VEILED);
       }        	
    }

    return;
}



/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list( OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list )
{
    OBJ_DATA *obj;
    int nMatch;

    nMatch = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
	if ( obj->pIndexData == pObjIndex )
	    nMatch++;
    }

    return nMatch;
}



/*
 * Move an obj out of a room.
 */
void obj_from_room( OBJ_DATA *obj )
{
    ROOM_INDEX_DATA *in_room;
    CHAR_DATA *ch;

    if ( ( in_room = obj->in_room ) == NULL )
    {
	bug( "obj_from_room: NULL.", 0 );
	return;
    }

    for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
	if (ch->on == obj)
	    ch->on = NULL;

    if ( obj == in_room->contents )
    {
	in_room->contents = obj->next_content;
    }
    else
    {
	OBJ_DATA *prev;

	for ( prev = in_room->contents; prev; prev = prev->next_content )
	{
	    if ( prev->next_content == obj )
	    {
		prev->next_content = obj->next_content;
		break;
	    }
	}

	if ( prev == NULL )
	{
	    bug( "Obj_from_room: obj not found.", 0 );
	    return;
	}
    }

    obj->in_room      = NULL;
    obj->next_content = NULL;
    return;
}



/*
 * Move an obj into a room.
 */
void obj_to_room( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex )
{
    obj->next_content		= pRoomIndex->contents;
    pRoomIndex->contents	= obj;
    obj->in_room		= pRoomIndex;
    obj->carried_by		= NULL;
    obj->in_obj			= NULL;
    return;
}



/*
 * Move an object into an object.
 */
void obj_to_obj( OBJ_DATA *obj, OBJ_DATA *obj_to )
{
    obj->next_content		= obj_to->contains;
    obj_to->contains		= obj;
    obj->in_obj			= obj_to;
    obj->in_room		= NULL;
    obj->carried_by		= NULL;
    if (obj_to->pIndexData->vnum == OBJ_VNUM_PIT)
        obj->cost = 0; 

    for ( ; obj_to != NULL; obj_to = obj_to->in_obj )
    {
	if ( obj_to->carried_by != NULL )
	{
	    if ((obj->item_type != ITEM_KEY) && (obj->item_type != ITEM_ROOM_KEY)) {
	    	obj_to->carried_by->carry_number += get_obj_number( obj );
	    	obj_to->carried_by->carry_weight += get_obj_weight( obj )
			* WEIGHT_MULT(obj_to) / 100;
	    }
	}
    }

    return;
}

/*
 * Move an Character into an object.
 */
void char_to_obj(CHAR_DATA * ch, OBJ_DATA * obj)
{
   CHAR_IN_DATA *who_in, *prev;

   ch->in_obj = obj;

   who_in       = (CHAR_IN_DATA *) alloc_mem( sizeof( CHAR_IN_DATA ) );
   who_in->ch   = ch;
   who_in->next = NULL;

   if( !obj->who_in )
   {
    obj->who_in = who_in;
    return;
   } 

   for( prev = obj->who_in; prev->next; prev = prev->next ); 
   prev->next = who_in;
   return;
}


/*
 * Move an Character out of an object.
 */

void char_from_obj(CHAR_DATA * ch)
{
   CHAR_IN_DATA *who_in, *prev;
   OBJ_DATA *obj;

   obj = ch->in_obj;

   for( prev = NULL, who_in = obj->who_in;
        who_in->ch != ch;
        prev = who_in, who_in = who_in->next );

   if( !prev )
      obj->who_in = who_in->next;
   else
      prev->next = who_in->next;

   ch->in_obj = NULL;
   free_mem( who_in, sizeof( CHAR_IN_DATA ) );
   return;
}


/*
 * Move an object out of an object.
 */
void obj_from_obj( OBJ_DATA *obj )
{
    OBJ_DATA *obj_from;

    if ( ( obj_from = obj->in_obj ) == NULL )
    {
	bug( "Obj_from_obj: null obj_from.", 0 );
	return;
    }

    if ( obj == obj_from->contains )
    {
	obj_from->contains = obj->next_content;
    }
    else
    {
	OBJ_DATA *prev;

	for ( prev = obj_from->contains; prev; prev = prev->next_content )
	{
	    if ( prev->next_content == obj )
	    {
		prev->next_content = obj->next_content;
		break;
	    }
	}

	if ( prev == NULL )
	{
	    bug( "Obj_from_obj: obj not found.", 0 );
	    return;
	}
    }

    obj->next_content = NULL;
    obj->in_obj       = NULL;

    for ( ; obj_from != NULL; obj_from = obj_from->in_obj )
    {
	if ( obj_from->carried_by != NULL )
	{
	    if ((obj->item_type != ITEM_KEY) && (obj->item_type != ITEM_ROOM_KEY)) {
	    	obj_from->carried_by->carry_number -= get_obj_number( obj );
	    	obj_from->carried_by->carry_weight -= get_obj_weight( obj ) 
			* WEIGHT_MULT(obj_from) / 100;
	    }
	}
    }

    return;
}



/*
 * Extract an obj from the world.
 */
void extract_obj( OBJ_DATA *obj )
{
    OBJ_DATA *obj_content;
    OBJ_DATA *obj_next;

    if ( obj->in_room != NULL )
	obj_from_room( obj );
    else if ( obj->carried_by != NULL )
	obj_from_char( obj );
    else if ( obj->in_obj != NULL )
	obj_from_obj( obj );

    for ( obj_content = obj->contains; obj_content; obj_content = obj_next )
    {
	obj_next = obj_content->next_content;
	extract_obj( obj_content );
    }

    if ( object_list == obj )
    {
	object_list = obj->next;
    }
    else
    {
	OBJ_DATA *prev;

	for ( prev = object_list; prev != NULL; prev = prev->next )
	{
	    if ( prev->next == obj )
	    {
		prev->next = obj->next;
		break;
	    }
	}

	if ( prev == NULL )
	{
	    bug( "Extract_obj: obj %d not found.", obj->pIndexData->vnum );
	    return;
	}
    }

    --obj->pIndexData->count;
    free_obj(obj);
    return;
}



/*
 * Extract a char from the world.
 */
void extract_char( CHAR_DATA *ch, bool fPull, bool bKeepConnection )
{
    CHAR_DATA *wch;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    /* doesn't seem to be necessary
    if ( ch->in_room == NULL )
    {
	bug( "Extract_char: NULL.", 0 );
	return;
    }
    */

    if (!IS_NPC(ch)) {
       if (IS_GUARDING(ch) || IS_GUARDED(ch))
        	remove_guard(ch, TRUE);
        	
       if (IS_BLOCKING(ch))
          stop_exit_block(ch);
    }
    
    nuke_pets(ch);
    ch->pet = NULL; /* just in case */

    if ( fPull )

	die_follower( ch );
    
    stop_fighting( ch, TRUE );

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
	obj_next = obj->next_content;
	extract_obj( obj );
    }
    
    if (ch->in_room != NULL)
        char_from_room( ch );

    /* Death room is set in the clan tabe now */
    //    if ( !fPull )
    //    {
    //	 char_to_room(ch,get_room_index(clan_table[ch->clan].room[0]));
    //	return;
    //    }

    if ( IS_NPC(ch) )
	--ch->pIndexData->count;
    else
    {
	nuke_pets(ch);
	ch->pet = NULL; 
    }

    if ( ch->mount && ch->mount->mount == ch )
    {
	ch->mount->mount = NULL;

	if (ch->mount->riding)
	{
	    act("You fall off of $N.", ch->mount, NULL, ch, TO_CHAR);
	    act("$n falls off of $N.", ch->mount, NULL, ch, TO_ROOM);
	    ch->mount->riding = FALSE;
	    if (!IS_IMMORTAL(ch->mount))
		ch->mount->position = POS_SITTING;
	}
    }

    if ( ch->desc != NULL && ch->desc->original != NULL )
    {
	do_function(ch, &do_return, "" );
	ch->desc = NULL;
    }

    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
	if ( wch->reply == ch )
	    wch->reply = NULL;
	if (!IS_NPC(wch) && wch->dfreply == ch)
	  wch->dfreply = NULL;
	if ( ch->mprog_target == wch )
	    wch->mprog_target = NULL;
    }

    if ( ch == char_list )
    {
       char_list = ch->next;
    }
    else
    {
	CHAR_DATA *prev;

	for ( prev = char_list; prev != NULL; prev = prev->next )
	{
	    if ( prev->next == ch )
	    {
		prev->next = ch->next;
		break;
	    }
	}

	if ( prev == NULL )
	{
	    bug( "Extract_char: char not found.", 0 );
	    return;
	}
    }


    if (!bKeepConnection) {
       if ( ch->desc != NULL )
   	ch->desc->character = NULL;
       free_char( ch );
    }
    return;
}



/*
 * Find a char in the room.
 */
CHAR_DATA *get_char_room( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *rch;
  int number;
  int count;

  number = number_argument( argument, arg );
  count  = 0;

  if ( !str_cmp( arg, "self" ) )
    return ch;

  for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room ) {

    if (!IS_SAME_WORLD(rch, ch))
	 continue;

    if (arg[0] != '\0') {
         if (IS_NPC(rch)) {
           if ( !can_see( ch, rch )  || !is_name( arg, rch->name ) )
                continue;
         }
         else {
           if ( !can_see( ch, rch ) || !is_name( arg, colorstrem(removechars(PERS( rch, ch), "(|<>)"))))
                continue;
           if ((ch == rch) && str_cmp( arg, ch->name ))
                continue;
         }
    }

    if ( ++count == number ) {
	 if (can_see(ch, rch)) {
	   return rch;
	 }
	 else {
	   return NULL;
	 }
    }
  }

  return NULL;
}




CHAR_DATA *get_char_room_NEWTRY( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *rch;
  int number;
  int count;
  int hidden=0;

  number = number_argument( argument, arg );
  count  = 0;

  if ( !str_cmp( arg, "self" ) )
    return ch;
  
  for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room ) {

    count++;

    if (!can_see(ch, rch))
	 hidden++;

    if (arg[0] != '\0') {
	 if (IS_NPC(rch)) {
	   if ( !can_see( ch, rch )  || !is_name( arg, rch->name ) )
		continue;
	 }
	 else {
	   if ( !can_see( ch, rch ) || !is_name( arg, colorstrem(removechars(PERS( rch, ch), "(|<>)"))))
		continue;
	   if ((ch == rch) && str_cmp( arg, ch->name ))
		continue;
	 }
    }

    if ( count-hidden == number && can_see( ch, rch ))
	 return rch;

    if ( count-hidden == number && !can_see( ch, rch ))
	 return ch;
  }
  
  return NULL;
}

CHAR_DATA *get_introname_char_room( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *rch;
  int number;
  int count;
  
  number = number_argument( argument, arg );
  count  = 0;
  
  if ( !str_cmp( arg, "self" ) )
    return ch;
  
  for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room ) {
    
    if (arg[0] != '\0') {
	 if (IS_NPC(rch)) {
	   if ( !can_see( ch, rch )  || !is_name( arg, rch->name ) )
		continue;
	 }
	 else {
	   if ( !can_see( ch, rch ) || 
		   (!is_name( arg, PERS_NAME( rch, ch)) && !IS_IMMORTAL(rch)))
		continue;
	 }
	 if ((ch == rch) && str_cmp( arg, ch->name ))
	   continue;
    }
    
    if ( ++count == number )
	 return rch;
  }
  
  return NULL;
}

/* 
 *  Excepts parsed argument, (ie number and the name) and also the room to check in
 */
CHAR_DATA * get_char_room2( CHAR_DATA *ch, ROOM_INDEX_DATA *room, char *argument, int *number )
{
    CHAR_DATA *rch;
    int count;
    
    if (room == NULL ) 
		return NULL;
    count  = 0;
	if ( !str_cmp( argument, "self" ) )
	return ch;
    
    for ( rch = room->people; rch != NULL; rch = rch->next_in_room )
    {
	if ( !can_see( ch, rch )  || !is_name( argument, rch->name ) )
	    continue;

	if ( ++count == *number )
	    return rch;
    }

    *number -= count;
    return NULL;
}


char* Stripstr(char* str)
{
  char* nyStr      = str;
  int   strIndex   = 0;
  int   nyStrIndex = 0;
  
  while (str[strIndex] != '\0') {
    if (str[strIndex] != ' ') {
      nyStr[nyStrIndex++] = str[strIndex];
    }
    else {
      if (nyStrIndex > 0)
        break;
    }
    strIndex++;
  }
  nyStr[nyStrIndex] = '\0';
  
  return nyStr;
}

/*
 * Find a char in the world.
 */
CHAR_DATA *get_char_world( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *wch;
  int number;
  int count;

  /* In room search first */
  if ( ( wch = get_char_room( ch, argument ) ) != NULL )
    return wch;
  
  number = number_argument( argument, arg );
  count  = 0;

  /* Look at the world  - PC's ONLY*/
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {

    if (IS_NPC(wch))
      continue;

    if ( wch->in_room == NULL || !can_see( ch, wch ) 
	    ||   !is_name( arg, Stripstr(colorstrem(removechars(PERS_NAME( wch, ch), "-=*[(<|>)]" )))))
	 continue;

    if ( ++count == number )
	 return wch;
  }

  /* Look at the world - NPC's*/
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {

    if (!IS_NPC(wch))
    {
	continue;
    }
    if (IS_NPC(wch)) 
    {
	 if (is_name(arg,wch->name))
	 {
	    if (++count == number)
            {
	       return wch;
            }
         }
    } 
    if ( wch->in_room == NULL || !can_see( ch, wch ) 
	    ||   !is_name( arg, Stripstr(colorstrem(removechars(PERS_NAME( wch, ch), "-=*[(<|>)]" )))))
	 continue;

    if ( ++count == number )
	 return wch;
  }
  
  return NULL;
}

/*
 * Find a char in the world.
 */
CHAR_DATA *get_char_same_world( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *wch;
  int number;
  int count;

  /* In room search first */
  if ( ( wch = get_char_room( ch, argument ) ) != NULL ){
    if (ch->world == wch->world) {
       return wch;
    }
    else {
       wch = NULL;
    };
  }
  
  number = number_argument( argument, arg );
  count  = 0;

  /* Look at the world  - PC's ONLY*/
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {

    if (IS_NPC(wch))
      continue;

    if ( wch->in_room == NULL || !can_see( ch, wch ) 
	    ||   !is_name( arg, Stripstr(colorstrem(removechars(PERS_NAME( wch, ch), "-=*[(<|>)]" )))))
	 continue;
    if (ch->world != wch->world) {
        if (IS_SET(ch->world, WORLD_TAR_DREAM) && !IS_SLEEPING(wch))
        continue;
    };

    if ( ++count == number )
	 return wch;
  }

  /* Look at the world - NPC's*/
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {

    if (!IS_NPC(wch))
    {
	continue;
    }
    if (ch->world != wch->world) {
        continue;
    };
    if (IS_NPC(wch)) 
    {
	 if (is_name(arg,wch->name))
	 {
	    if (++count == number)
            {
	       return wch;
            }
         }
    } 
    if ( wch->in_room == NULL || !can_see( ch, wch ) 
	    ||   !is_name( arg, Stripstr(colorstrem(removechars(PERS_NAME( wch, ch), "-=*[(<|>)]" )))))
	 continue;

    if ( ++count == number )
	 return wch;
  }
  
  return NULL;
}
/*
 * Find a char anywhere
 */
CHAR_DATA *get_char_anywhere( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *wch;
  int number;
  int count;

  /* In room search first */
  if ( ( wch = get_char_room( ch, argument ) ) != NULL )
    return wch;
  
  number = number_argument( argument, arg );
  count  = 0;

  /* Look at the world  - PC's ONLY*/
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {

    if (IS_NPC(wch))
      continue;

    if ( wch->in_room == NULL 
	    ||   !is_name( arg, Stripstr(colorstrem(removechars(PERS_NAME( wch, ch), "-=*[(<|>)]" )))))
	 continue;

    if ( ++count == number )
	 return wch;
  }

  /* Look at the world - NPC's*/
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {

    if (!IS_NPC(wch))
    {
	continue;
    }
    if (IS_NPC(wch)) 
    {
	 if (is_name(arg,wch->name))
	 {
	    if (++count == number)
            {
	       return wch;
            }
         }
    } 
    if ( wch->in_room == NULL 
	    ||   !is_name( arg, Stripstr(colorstrem(removechars(PERS_NAME( wch, ch), "-=*[(<|>)]" )))))
	 continue;

    if ( ++count == number )
	 return wch;
  }
  
  return NULL;
}

CHAR_DATA *get_introname_char_world( CHAR_DATA *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *wch;
  int number;
  int count;

  number = number_argument( argument, arg );
  count  = 0;

  if ( !str_cmp( arg, "self" ) )
    return ch;

  /* Look at the world */
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {  
    if ( wch->in_room == NULL || 
	    !can_see( ch, wch )  || 
	    !is_name( arg, PERS_NAME(wch, ch)))
	 continue;

    if ( ++count == number )
	 return wch;
  }
  
  return NULL;
}

CHAR_DATA *get_realname_char_world( CHAR_DATA *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *wch;
  int number;
  int count;

  number = number_argument( argument, arg );
  count  = 0;

  if ( !str_cmp( arg, "self" ) )
    return ch;
  
  /* Look at the world */
  for ( wch = char_list; wch != NULL ; wch = wch->next ) {  
    if ( wch->in_room == NULL || 
	    !can_see_channel( ch, wch )  || 
	    !is_name( arg, wch->name))
	 continue;

    if ( ++count == number )
	 return wch;
  }
  
  return NULL;
}

CHAR_DATA *get_charId_world( CHAR_DATA *ch, long id)
{
  CHAR_DATA *wch;
  
  for (wch = char_list; wch != NULL; wch = wch->next) {
    if (wch->id == id)
	 return wch;
  }
  
  /* None found */
  return NULL;
}

/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 * Swordfish: Added vnum to make sure not loading into other rooms container
 */
OBJ_DATA *get_obj_type( OBJ_INDEX_DATA *pObjIndex, unsigned int vnum )
{
  OBJ_DATA *obj;
  
  for ( obj = object_list; obj != NULL; obj = obj->next ) {
    if (obj->in_room == NULL)
	 continue;
    if (obj->in_room->vnum != vnum && vnum != 0) continue;
    if ( obj->pIndexData == pObjIndex )
	 return obj;
  }
  
  return NULL;
}

/*
 * Find an obj in a list.
 */
OBJ_DATA *get_obj_list( CHAR_DATA *ch, char *argument, OBJ_DATA *list )
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  int number;
  int count;
  
  number = number_argument( argument, arg );
  count  = 0;
  for ( obj = list; obj != NULL; obj = obj->next_content ) {
    if ( can_see_obj( ch, obj ) && is_name( arg, obj->name ) ) { if ( ++count == number ) return obj; } }
  
  return NULL;
}



/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *get_obj_carry( CHAR_DATA *ch, char *argument, CHAR_DATA *viewer )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
	if ( obj->wear_loc == WEAR_NONE
	&&   (can_see_obj( viewer, obj ) ) 
	&&   is_name( arg, obj->name ) )
	{
	    if ( ++count == number )
		return obj;
	}
    }

    return NULL;
}



/*
 * Find an obj in player's equipment.
 */
OBJ_DATA *get_obj_wear( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
	if ( obj->wear_loc != WEAR_NONE
	&&   can_see_obj( ch, obj )
	&&   is_name( arg, obj->name ) )
	{
	    if ( ++count == number )
		return obj;
	}
    }

    return NULL;
}



/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA *get_obj_here( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;

    obj = NULL;

 if( ch->in_obj )
    {
      obj = get_obj_list(ch, argument, ch->in_obj->contains);
      if (obj)
      {
         return obj;
      }
    }       
    obj = get_obj_list( ch, argument, ch->in_room->contents );
    if ( obj ) return obj;

    if ( ( obj = get_obj_carry( ch, argument, ch ) ) != NULL )
	return obj;

    if ( ( obj = get_obj_wear( ch, argument ) ) != NULL )
	return obj;

    return NULL;
}



/*
 * Find an obj in the world.
 */
OBJ_DATA *get_obj_world( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    if ( ( obj = get_obj_here( ch, argument ) ) != NULL )
	return obj;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
	if ( can_see_obj( ch, obj ) && is_name( arg, obj->name ) )
	{
	    if ( ++count == number )
		return obj;
	}
    }

    return NULL;
}

/* deduct cost from a character */

void deduct_cost(CHAR_DATA *ch, int cost)
{
    int silver = 0, gold = 0;

    silver = UMIN(ch->silver,cost); 

    if (silver < cost)
    {
	gold = ((cost - silver + 99) / 100);
	silver = cost - 100 * gold;
    }

    ch->gold -= gold;
    ch->silver -= silver;

    if (ch->gold < 0)
    {
	bug("deduct costs: gold %d < 0",ch->gold);
	ch->gold = 0;
    }
    if (ch->silver < 0)
    {
	bug("deduct costs: silver %d < 0",ch->silver);
	ch->silver = 0;
    }
}   
/*
 * Create a 'money' obj.
 */
OBJ_DATA *create_money( int gold, int silver )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    if ( gold < 0 || silver < 0 || (gold == 0 && silver == 0) )
    {
	bug( "Create_money: zero or negative money.",UMIN(gold,silver));
	gold = UMAX(1,gold);
	silver = UMAX(1,silver);
    }

    if (gold == 0 && silver == 1)
    {
	obj = create_object( get_obj_index( OBJ_VNUM_SILVER_ONE ), 0 );
    }
    else if (gold == 1 && silver == 0)
    {
	obj = create_object( get_obj_index( OBJ_VNUM_GOLD_ONE), 0 );
    }
    else if (silver == 0)
    {
        obj = create_object( get_obj_index( OBJ_VNUM_GOLD_SOME ), 0 );
        sprintf( buf, obj->short_descr, gold );
        free_string( obj->short_descr );
        obj->short_descr        = str_dup( buf );
        obj->value[1]           = gold;
        obj->cost               = gold;
	obj->weight		= gold/5;
    }
    else if (gold == 0)
    {
        obj = create_object( get_obj_index( OBJ_VNUM_SILVER_SOME ), 0 );
        sprintf( buf, obj->short_descr, silver );
        free_string( obj->short_descr );
        obj->short_descr        = str_dup( buf );
        obj->value[0]           = silver;
        obj->cost               = silver;
	obj->weight		= silver/20;
    }
 
    else
    {
	obj = create_object( get_obj_index( OBJ_VNUM_COINS ), 0 );
	sprintf( buf, obj->short_descr, silver, gold );
	free_string( obj->short_descr );
	obj->short_descr	= str_dup( buf );
	obj->value[0]		= silver;
	obj->value[1]		= gold;
	obj->cost		= 100 * gold + silver;
	obj->weight		= gold / 5 + silver / 20;
    }

    return obj;
}



/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number( OBJ_DATA *obj )
{
    int number;
 
    if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY
    ||  obj->item_type == ITEM_GEM || obj->item_type == ITEM_JEWELRY)
        number = 0;
    else
        number = 1;
 
    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        number += get_obj_number( obj );
 
    return number;
}


/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight( OBJ_DATA *obj )
{
    int weight;
    OBJ_DATA *tobj;

    weight = obj->weight;
    for ( tobj = obj->contains; tobj != NULL; tobj = tobj->next_content )
	weight += get_obj_weight( tobj ) * WEIGHT_MULT(obj) / 100;

    return weight;
}

int get_true_weight(OBJ_DATA *obj)
{
    int weight;
 
    weight = obj->weight;
    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        weight += get_obj_weight( obj );
 
    return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark( ROOM_INDEX_DATA *pRoomIndex )
{
    if ( pRoomIndex->light > 0 )
	return FALSE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_DARK) )
	return TRUE;

    if ( pRoomIndex->sector_type == SECT_INSIDE
    ||   pRoomIndex->sector_type == SECT_CITY )
	return FALSE;

    if ( weather_info.sunlight == SUN_SET
    ||   weather_info.sunlight == SUN_DARK )
	return TRUE;

    return FALSE;
}


bool is_room_owner(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
    if (room->owner == NULL || room->owner[0] == '\0')
	return FALSE;

    return is_name(ch->name,room->owner);
}

/*
 * True if room is private.
 */
bool room_is_private( ROOM_INDEX_DATA *pRoomIndex )
{
    CHAR_DATA *rch;
    int count;


/*
    if (pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0')
	return TRUE;
*/

    count = 0;
    for ( rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room )
	count++;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)  && count >= 2 )
	return TRUE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) && count >= 1 )
	return TRUE;
    
    if ( IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) )
	return TRUE;

    return FALSE;
}

/* visibility on a room -- for entering and exits */
bool can_see_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) 
    &&  get_trust(ch) < MAX_LEVEL)
	return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_GODS_ONLY)
    &&  !IS_IMMORTAL(ch))
	return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_HEROES_ONLY)
    &&  !IS_IMMORTAL(ch))
	return FALSE;

    /* if (IS_SET(pRoomIndex->room_flags,ROOM_NEWBIES_ONLY) */
    /* &&  ch->level > 5 && !IS_IMMORTAL(ch)) */
    /* && !IS_NEWBIE(ch) && !IS_IMMORTAL(ch)) */
    /*	return FALSE; */

    if (!IS_IMMORTAL(ch) && pRoomIndex->clan && ch->clan != pRoomIndex->clan)
	return FALSE;

    return TRUE;
}



int opposite_door(int door)
{
  int opdoor;

  switch ( door )
   {
    case 0: opdoor=2;	break;
    case 1: opdoor=3;	break;
    case 2: opdoor=0;	break;
    case 3: opdoor=1;	break;
    case 4: opdoor=5;	break;
    case 5: opdoor=4;	break;
    default: opdoor=-1; break;
  }
  return opdoor;
}

CHAR_DATA * find_char( CHAR_DATA *ch, char *argument,int door, int range ) 
{
 
 EXIT_DATA *pExit,*bExit;
 ROOM_INDEX_DATA *dest_room,*back_room;
 CHAR_DATA *target;
 int number = 0,opdoor;
 char arg[MAX_INPUT_LENGTH];

 number = number_argument(argument,arg);
 dest_room = ch->in_room;
/* checks if in the same room as the ch. arg is the name of the char, 
 number is which char (ie 2.mob, 3.mob) */
 if ( (target = get_char_room2(ch,dest_room,arg,&number)) != NULL)
	return target;


 if ( (opdoor = opposite_door( door )) == -1)
  {
   bug("In find_char wrong door: %d",door);
   send_to_char("You don't see that there.\n\r",ch);
   return NULL;
 }
/* range is how far you can shoot the arrow */

 while (range > 0)
 {
  range--;
  /* find target room */
  back_room = dest_room;
  
  /*this is one hard to follow code! 
  here's what happens: in the next first line of code:
  pExit is assigned the exit of the direction that is targeted,
  then (2nd line), dest_room is assigned the room through that exit,
  (5)bExit is assigned the exit going back to the orginal room and in
  the next line is verified to back_room, which was assigned to
  dest_room in the first place. */

  if ( (pExit = dest_room->exit[door]) == NULL
      || (dest_room = pExit->u1.to_room) == NULL
      || IS_SET(pExit->exit_info,EX_CLOSED) )
   break;
  
  if ( (bExit = dest_room->exit[opdoor]) == NULL
      || bExit->u1.to_room != back_room)
   {
    send_to_char("The path you choose prevents your power to pass.\n\r",ch);
    return NULL;
   }
  
  if ((target = get_char_room2(ch,dest_room,arg,&number)) != NULL ) 
	return target;
	
   }

 send_to_char("You don't see that there.\n\r",ch);
 return NULL;
}
	
int check_exit( char *arg )
{
    int door = -1;

	 if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) ) door = 0;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east"  ) ) door = 1;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) ) door = 2;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west"  ) ) door = 3;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up"    ) ) door = 4;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down"  ) ) door = 5;

    return door;
}




/*
 * True if char can see victim.
 */
bool can_see( CHAR_DATA *ch, CHAR_DATA *victim )
{
   OBJ_DATA *to_obj;
   OBJ_DATA *ch_obj;

//   printf("ch=%s vict=%s\n", IS_NPC(ch) ? ch->short_descr : ch->name, IS_NPC(victim) ? victim->short_descr : victim->name);

   if ((!IS_SAME_WORLD(ch, victim) && !IS_SLEEPING(victim)) || 
       (!IS_SAME_WORLD(ch, victim) && IS_SLEEPING(victim) && IS_SET(ch->world, WORLD_NORMAL))) {
//       	log_string("## can_see 1");
	return FALSE;
   }

   if (!IS_SAME_WORLD(ch,victim) && !CAN_DREAM(ch)) {
//       	log_string("## can_see 2");   	
        return FALSE;        
   }
   
   if (!IS_NPC(ch)
	  && !IS_SET(ch->act, PLR_HOLYLIGHT)
	  && room_is_dark(ch->in_room)
	  && !IS_AFFECTED(ch, AFF_INFRARED)
	  && (ch->race != race_lookup("fade"))
	  && (ch->race != race_lookup("trolloc"))) {	  	
//       	log_string("## can_see 3");	  	
	return FALSE;
   }
   
   /* RT changed so that WIZ_INVIS has levels */
   if ( ch == victim )
	return TRUE;
   
   if ( get_trust(ch) < victim->invis_level) {
//       	log_string("## can_see 4");   	
	return FALSE;
   }
   
   if (get_trust(ch) < victim->incog_level  && ch->in_room != victim->in_room) {
//       	log_string("## can_see 5");   	
	return FALSE;
   }
   
   if ( (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) 
	   ||   (IS_NPC(ch) && IS_IMMORTAL(ch)))
	return TRUE;

   if (IS_AFFECTED(ch, AFF_BLINDFOLDED))
   {
      return FALSE;
   }
   
   // Fade's can't go blind.. already
   if ( IS_AFFECTED(ch, AFF_BLIND) && (ch->race != race_lookup("fade"))) {
//       	log_string("## can_see 6");   	
	return FALSE;
   }
   
   if (room_is_dark( ch->in_room ) && !IS_AFFECTED(ch, AFF_INFRARED) && (ch->race != race_lookup("fade")) && (ch->race != race_lookup("trolloc"))) {
//       	log_string("## can_see 7");   	
	return FALSE;
   }
   
   if ( IS_AFFECTED(victim, AFF_INVISIBLE)) { 
   	if ( IS_NPC(victim) && !IS_AFFECTED(ch, AFF_DETECT_INVIS) ) {
//       	log_string("## can_see 8");   		
	   return FALSE;
        }
        else if (!IS_NPC(victim) && !IS_AFFECTED(ch, AFF_DETECT_INVIS)) {
//       	log_string("## can_see 9");        	
	   return FALSE;
        }
   }
   
   //should add in a check to detect camouflage
   if ( IS_AFFECTED(victim, AFF_CAMOUFLAGE)
	   &&   !IS_AFFECTED(ch, AFF_DETECT_HIDDEN) 
           &&   get_trust(ch) <= LEVEL_HERO ) {
//       	log_string("## can_see 10");           	
	return FALSE;
    }
   
      
   if ( IS_AFFECTED(victim, AFF_HIDE)
	   &&   !IS_AFFECTED(ch, AFF_DETECT_HIDDEN)
	   &&   victim->fighting == NULL)
   {

	// Fades stay hidden
	if (IS_FADE(victim))
	  return FALSE;

	//chance to see
	int chance;
	chance = get_skill(ch,gsn_observation)/2 + get_skill(ch,gsn_awareness);
	chance -= (number_percent() / 2);
	chance -= get_skill(victim,gsn_hide);
	
	if (IS_SET(victim->merits, MERIT_STEALTHY)) {
	   chance -= 20;
	}

	if (IS_SET(victim->flaws, FLAW_SHORT))
	  chance -= 5;

	if (chance < 10)
	  chance = 10;

	if (number_percent() < chance)
	{
	 	check_improve(ch,gsn_awareness,TRUE,1);
	  	check_improve(ch,gsn_observation,TRUE,1);
		return TRUE;
	}
	else
	{
	     check_improve(ch,gsn_awareness,FALSE,1);
	     check_improve(ch,gsn_observation,FALSE,1);
//      	log_string("## can_see 11");
	   return FALSE;
	}
   }
   
   for(to_obj=victim->in_obj; to_obj; to_obj=to_obj->in_obj )
	if( IS_SET(to_obj->value[1],CONT_CLOSED)
	    &&  to_obj->value[3] == 0 )
	  break;
   
   for(ch_obj=ch->in_obj; ch_obj; ch_obj=ch_obj->in_obj )
	if( IS_SET(ch_obj->value[1],CONT_CLOSED)
	    &&  ch_obj->value[3] == 0 )
	  break;
   
   if (to_obj == ch_obj)
	return TRUE;
   
   return TRUE;
}



/*
 * True if char can see obj.
 */
bool can_see_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
  if (IS_SET(ch->act, PLR_HOLYLIGHT) && !IS_NPC(ch)) {
    return TRUE;
  }
  
  if ( IS_SET(obj->extra_flags,ITEM_VIS_DEATH))
    return FALSE;
  
  if ( IS_AFFECTED (ch, AFF_BLINDFOLDED))
	return FALSE;
  if ( IS_AFFECTED( ch, AFF_BLIND ) && obj->item_type != ITEM_POTION)
    return FALSE;
  
  if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
    return TRUE;
  
  if ( IS_SET(obj->extra_flags, ITEM_INVIS)
	  &&   !IS_AFFECTED(ch, AFF_DETECT_INVIS) )
    return FALSE;
  
  if ( IS_OBJ_STAT(obj,ITEM_GLOW))
    return TRUE;
  
  if ( room_is_dark( ch->in_room ) && !IS_AFFECTED(ch, AFF_INFRARED)) {
    if (IS_SHADOWSPAWN(ch))
	 return TRUE;
    else if (IS_WOLFKIN(ch))
	 return TRUE;
    else
	 return FALSE;
  }
  
  return TRUE;
}



/*
 * True if char can drop obj.
 */
bool can_drop_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( !IS_SET(obj->extra_flags, ITEM_NODROP) )
	return TRUE;

    if ( !IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL )
	return TRUE;

    return FALSE;
}

/*
 * Return ascii name of an item type.
 */
char *item_type_name( OBJ_DATA *obj )
{
    switch ( obj->item_type )
    {
    case ITEM_LIGHT:		return "light";
    case ITEM_SCROLL:		return "scroll";
    case ITEM_WAND:		return "wand";
    case ITEM_STAFF:		return "staff";
    case ITEM_WEAPON:		return "weapon";
    case ITEM_TREASURE:		return "treasure";
    case ITEM_ARMOR:		return "armor";
    case ITEM_CLOTHING:		return "clothing";
    case ITEM_POTION:		return "potion";
    case ITEM_FURNITURE:	return "furniture";
    case ITEM_TRASH:		return "trash";
    case ITEM_CONTAINER:	return "container";
    case ITEM_DRINK_CON:	return "drink container";
    case ITEM_KEY:		return "key";
    case ITEM_FOOD:		return "food";
    case ITEM_MONEY:		return "money";
    case ITEM_NOTEPAPER:	return "notepaper";
    case ITEM_BOAT:		return "boat";
    case ITEM_CORPSE_NPC:	return "npc corpse";
    case ITEM_CORPSE_PC:	return "pc corpse";
    case ITEM_FOUNTAIN:		return "fountain";
    case ITEM_PILL:		return "pill";
    case ITEM_MAP:		return "map";
    case ITEM_PORTAL:		return "portal";
    case ITEM_WARP_STONE:	return "warp stone";
    case ITEM_GEM:		return "gem";
    case ITEM_JEWELRY:		return "jewelry";
    case ITEM_JUKEBOX:		return "juke box";
    case ITEM_TATTOO:		return "tattoo";
    case ITEM_TOKEN:          return "token";
    case ITEM_ANGREAL:        return "angreal";
    case ITEM_FIREWALL:         return "firewall";
    case ITEM_VEHICLE:         return "vehicle";
    case ITEM_ORE:         	return "ore";
    case ITEM_GEMSTONE:         return "gemstone";
    }

    bug( "Item_type_name: unknown type %d.", obj->item_type );
    return "(unknown)";
}


/*
 * Return ascii name of an affect location.
 */
char *affect_loc_name( int location )
{
    switch ( location )
    {
    case APPLY_NONE:		return "none";
    case APPLY_STR:		return "strength";
    case APPLY_DEX:		return "dexterity";
    case APPLY_INT:		return "intelligence";
    case APPLY_WIS:		return "wisdom";
    case APPLY_CON:		return "constitution";
    case APPLY_SEX:		return "sex";
    case APPLY_CLASS:		return "class";
    case APPLY_LEVEL:		return "level";
    case APPLY_AGE:		return "age";
    case APPLY_ENDURANCE:		return "endurance";
    case APPLY_HIT:		return "hp";
    case APPLY_GOLD:		return "gold";
    case APPLY_EXP:		return "experience";
    case APPLY_AC:		return "armor class";
    case APPLY_HITROLL:		return "hit roll";
    case APPLY_DAMROLL:		return "damage roll";
    case APPLY_SAVES:		return "saves";
    case APPLY_SAVING_ROD:	return "save vs rod";
    case APPLY_SAVING_PETRI:	return "save vs petrification";
    case APPLY_SAVING_BREATH:	return "save vs breath";
    case APPLY_SAVING_SPELL:	return "save vs spell";
    case APPLY_SPELL_AFFECT:	return "none";
    case APPLY_SPHERE_AIR:    return "air sphere";
    case APPLY_SPHERE_EARTH:  return "earth sphere";
    case APPLY_SPHERE_FIRE:   return "fire sphere";
    case APPLY_SPHERE_SPIRIT: return "spirit sphere";
    case APPLY_SPHERE_WATER:  return "water sphere";
    }

    bug( "Affect_location_name: unknown location %d.", location );
    return "(unknown)";
}



/*
 * Return ascii name of an affect bit vector.
 */
char *affect_bit_name( int vector )
{
    static char buf[512];

    buf[0] = '\0';
    if ( vector & AFF_BLIND         ) strcat( buf, " blind"         );
    if ( vector & AFF_INVISIBLE     ) strcat( buf, " invisible"     );
	if ( vector & AFF_DETECT_INVIS  ) strcat( buf, " detect_invis"  );
    if ( vector & AFF_DETECT_HIDDEN ) strcat( buf, " detect_hidden" );
    if ( vector & AFF_CAMOUFLAGE    ) strcat( buf, " camouflage"     );
    if ( vector & AFF_SANCTUARY     ) strcat( buf, " sanctuary"     );
    /* if ( vector & AFF_FAERIE_FIRE   ) strcat( buf, " faerie_fire"   ); */
    if ( vector & AFF_INFRARED      ) strcat( buf, " infrared"      );
    if ( vector & AFF_POISON        ) strcat( buf, " poison"        );
    if ( vector & AFF_BIND          ) strcat( buf, " bind"          );
    if ( vector & AFF_BLINDFOLDED   ) strcat( buf, " blindfolded"   );
    if ( vector & AFF_SAP           ) strcat( buf, " sap"           );
    if ( vector & AFF_SLEEP         ) strcat( buf, " sleep"         );
    if ( vector & AFF_SNEAK         ) strcat( buf, " sneak"         );
    if ( vector & AFF_HIDE          ) strcat( buf, " hide"          );
    if ( vector & AFF_CHARM         ) strcat( buf, " charm"         );
    if ( vector & AFF_FLYING        ) strcat( buf, " flying"        );
    if ( vector & AFF_PASS_DOOR     ) strcat( buf, " pass_door"     );
    if ( vector & AFF_BERSERK	    ) strcat( buf, " berserk"	    );
    if ( vector & AFF_CALM	    ) strcat( buf, " calm"	    );
    if ( vector & AFF_HASTE	    ) strcat( buf, " haste"	    );
    if ( vector & AFF_SLOW          ) strcat( buf, " slow"          );
    if ( vector & AFF_PLAGUE	    ) strcat( buf, " plague" 	    );
    if ( vector & AFF_LINKED	    ) strcat( buf, " linked"	    );
    if ( vector & AFF_GAGGED	    ) strcat( buf, " gagged"	    );
    if ( vector & AFF_SUFFOCATING   ) strcat( buf, " suffocating"	    );
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}



/*
 * Return ascii name of extra flags vector.
 */
char *extra_bit_name( int extra_flags )
{
    static char buf[512];

    buf[0] = '\0';
    if ( extra_flags & ITEM_GLOW         ) strcat( buf, " glow"         );
    if ( extra_flags & ITEM_HUM          ) strcat( buf, " hum"          );
    if ( extra_flags & ITEM_DARK         ) strcat( buf, " dark"         );
    if ( extra_flags & ITEM_LOCK         ) strcat( buf, " lock"         );
    if ( extra_flags & ITEM_EVIL         ) strcat( buf, " evil"         );
    if ( extra_flags & ITEM_INVIS        ) strcat( buf, " invis"        );
    if ( extra_flags & ITEM_MAGIC        ) strcat( buf, " magic"        );
    if ( extra_flags & ITEM_NODROP       ) strcat( buf, " nodrop"       );
    if ( extra_flags & ITEM_BLESS        ) strcat( buf, " bless"        );
    if ( extra_flags & ITEM_BROKEN       ) strcat( buf, " broken"       );
    if ( extra_flags & ITEM_REPOP_ON_CHANCE     ) strcat( buf, " chancerepop"     );
    if ( extra_flags & ITEM_REPOP_RANDOM_LOCATION     ) strcat( buf, " randomlocation"     );
    if ( extra_flags & ITEM_FULL_RANDOM  ) strcat( buf, " fullrandom"   );
    if ( extra_flags & ITEM_NOREMOVE     ) strcat( buf, " noremove"     );
    if ( extra_flags & ITEM_INVENTORY    ) strcat( buf, " inventory"    );
    if ( extra_flags & ITEM_NOPURGE	 ) strcat( buf, " nopurge"	);
    if ( extra_flags & ITEM_VIS_DEATH	 ) strcat( buf, " vis_death"	);
    if ( extra_flags & ITEM_ROT_DEATH	 ) strcat( buf, " rot_death"	);
    if ( extra_flags & ITEM_NOLOCATE	 ) strcat( buf, " no_locate"	);
    if ( extra_flags & ITEM_SELL_EXTRACT ) strcat( buf, " sell_extract" );
    if ( extra_flags & ITEM_BURN_PROOF	 ) strcat( buf, " burn_proof"	);
    if ( extra_flags & ITEM_NOUNCURSE	 ) strcat( buf, " no_uncurse"	);
    if ( extra_flags & ITEM_HIDDEN       ) strcat( buf, " hidden"     );
    if ( extra_flags & ITEM_NOSTEAL      ) strcat( buf, " nosteal"    );
    if ( extra_flags & ITEM_NO_BREAK     ) strcat( buf, " nobreak"    );
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

/* return ascii name of an act vector */
char *act_bit_name( int act_flags )
{
    static char buf[512];

    buf[0] = '\0';

    if (IS_SET(act_flags,ACT_IS_NPC))
    { 
 	strcat(buf," npc");
    	if (act_flags & ACT_SENTINEL 	) strcat(buf, " sentinel");
    	if (act_flags & ACT_SCAVENGER	) strcat(buf, " scavenger");
	if (act_flags & ACT_REPAIRER ) strcat(buf, " repairer");
	if (act_flags & ACT_BANKER ) strcat(buf, " banker");
	if (act_flags & ACT_AGGRESSIVE	) strcat(buf, " aggressive");
	if (act_flags & ACT_STAY_AREA	) strcat(buf, " stay_area");
	if (act_flags & ACT_WIMPY	) strcat(buf, " wimpy");
	if (act_flags & ACT_PET		) strcat(buf, " pet");
	if (act_flags & ACT_TRAIN	) strcat(buf, " train");
	if (act_flags & ACT_UNDEAD	) strcat(buf, " undead");
	if (act_flags & ACT_CLERIC	) strcat(buf, " cleric");
	if (act_flags & ACT_THIEF	) strcat(buf, " thief");
	if (act_flags & ACT_WARRIOR	) strcat(buf, " warrior");
	if (act_flags & ACT_NOALIGN	) strcat(buf, " no_align");
	if (act_flags & ACT_NOPURGE	) strcat(buf, " no_purge");
	if (act_flags & ACT_IS_HEALER	) strcat(buf, " healer");
	if (act_flags & ACT_IS_CHANGER  ) strcat(buf, " changer");
	if (act_flags & ACT_GAIN	) strcat(buf, " gainer");
	if (act_flags & ACT_UPDATE_ALWAYS) strcat(buf," update_always");
	if (act_flags & ACT_ROAMERTRAINER) strcat(buf," roamertrainer");
    }
    else
    {
	strcat(buf," player");
	if (act_flags & PLR_AUTOASSIST	) strcat(buf, " autoassist");
	if (act_flags & PLR_AUTOEXIT	) strcat(buf, " autoexit");
	if (act_flags & PLR_AUTOLOOT	) strcat(buf, " autoloot");
	if (act_flags & PLR_AUTOSAC	) strcat(buf, " autosac");
	if (act_flags & PLR_AUTOGOLD	) strcat(buf, " autogold");
	if (act_flags & PLR_AUTOSPLIT	) strcat(buf, " autosplit");
	if (act_flags & PLR_HOLYLIGHT	) strcat(buf, " holy_light");
	if (act_flags & PLR_CANLOOT	) strcat(buf, " loot_corpse");
	if (act_flags & PLR_NOSUMMON	) strcat(buf, " no_summon");
	if (act_flags & PLR_NOFOLLOW	) strcat(buf, " no_follow");
	if (act_flags & PLR_FREEZE	) strcat(buf, " frozen");
	if (act_flags & PLR_THIEF	) strcat(buf, " thief");
	if (act_flags & PLR_KILLER	) strcat(buf, " killer");
    }
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *comm_bit_name(int comm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (comm_flags & COMM_QUIET		) strcat(buf, " quiet");
    if (comm_flags & COMM_DEAF		) strcat(buf, " deaf");
    if (comm_flags & COMM_NOWIZ		) strcat(buf, " no_wiz");
    if (comm_flags & COMM_NOAUCTION	) strcat(buf, " no_auction");
    if (comm_flags & COMM_NOGOSSIP	) strcat(buf, " no_gossip");
    if (comm_flags & COMM_NOMUSIC	) strcat(buf, " no_music");
    if (comm_flags & COMM_COMPACT	) strcat(buf, " compact");
    if (comm_flags & COMM_BRIEF		) strcat(buf, " brief");
    if (comm_flags & COMM_PROMPT	) strcat(buf, " prompt");
    if (comm_flags & COMM_COMBINE	) strcat(buf, " combine");
    if (comm_flags & COMM_NOEMOTE	) strcat(buf, " no_emote");
    if (comm_flags & COMM_NOSHOUT	) strcat(buf, " no_shout");
    if (comm_flags & COMM_NOTELL	) strcat(buf, " no_tell");
    if (comm_flags & COMM_NOCHANNELS	) strcat(buf, " no_channels");
    if (comm_flags & COMM_NOHINT	) strcat(buf, " no_hint" );
    

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *imm_bit_name(int imm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (imm_flags & IMM_SUMMON		) strcat(buf, " summon");
    if (imm_flags & IMM_CHARM		) strcat(buf, " charm");
    if (imm_flags & IMM_MAGIC		) strcat(buf, " magic");
    if (imm_flags & IMM_WEAPON		) strcat(buf, " weapon");
    if (imm_flags & IMM_BASH		) strcat(buf, " blunt");
    if (imm_flags & IMM_PIERCE		) strcat(buf, " piercing");
    if (imm_flags & IMM_SLASH		) strcat(buf, " slashing");
    if (imm_flags & IMM_FIRE		) strcat(buf, " fire");
    if (imm_flags & IMM_COLD		) strcat(buf, " cold");
    if (imm_flags & IMM_LIGHTNING	) strcat(buf, " lightning");
    if (imm_flags & IMM_ACID		) strcat(buf, " acid");
    if (imm_flags & IMM_POISON		) strcat(buf, " poison");
    if (imm_flags & IMM_SAP		) strcat(buf, " sap");
    if (imm_flags & IMM_PUSH		) strcat(buf, " push");
    if (imm_flags & IMM_ASSASSINATE	) strcat(buf, " assassinate");
    if (imm_flags & IMM_ARROW		) strcat(buf, " arrow");
    if (imm_flags & IMM_NEGATIVE	) strcat(buf, " negative");
    if (imm_flags & IMM_HOLY		) strcat(buf, " holy");
    if (imm_flags & IMM_ENERGY		) strcat(buf, " energy");
    if (imm_flags & IMM_WIND		) strcat(buf, " wind");
    if (imm_flags & IMM_HARM		) strcat(buf, " harm");
    if (imm_flags & IMM_DISEASE	) strcat(buf, " disease");
    if (imm_flags & IMM_DROWNING	) strcat(buf, " drowning");
    if (imm_flags & IMM_LIGHT		) strcat(buf, " light");
    if (imm_flags & VULN_IRON		) strcat(buf, " iron");
    if (imm_flags & VULN_WOOD		) strcat(buf, " wood");
    if (imm_flags & VULN_SILVER	) strcat(buf, " silver");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *wear_bit_name(int wear_flags)
{
    static char buf[512];

    buf [0] = '\0';
    if (wear_flags & ITEM_TAKE		) strcat(buf, " take");
    if (wear_flags & ITEM_WEAR_FINGER	) strcat(buf, " finger");
    if (wear_flags & ITEM_WEAR_NECK	) strcat(buf, " neck");
    if (wear_flags & ITEM_WEAR_BODY	) strcat(buf, " torso");
    if (wear_flags & ITEM_WEAR_HEAD	) strcat(buf, " head");
    if (wear_flags & ITEM_WEAR_LEGS	) strcat(buf, " legs");
    if (wear_flags & ITEM_WEAR_FEET	) strcat(buf, " feet");
    if (wear_flags & ITEM_WEAR_HANDS	) strcat(buf, " hands");
    if (wear_flags & ITEM_WEAR_ARMS	) strcat(buf, " arms");
    if (wear_flags & ITEM_WEAR_SHIELD	) strcat(buf, " shield");
    if (wear_flags & ITEM_WEAR_ABOUT	) strcat(buf, " body");
    if (wear_flags & ITEM_WEAR_WAIST	) strcat(buf, " waist");
    if (wear_flags & ITEM_WEAR_WRIST	) strcat(buf, " wrist");
    if (wear_flags & ITEM_WIELD		) strcat(buf, " wield");
    if (wear_flags & ITEM_HOLD		) strcat(buf, " hold");
    if (wear_flags & ITEM_NO_SAC	) strcat(buf, " nosac");
    if (wear_flags & ITEM_WEAR_FLOAT	) strcat(buf, " float");
    if (wear_flags & ITEM_WEAR_BACK	) strcat(buf, " back");
    if (wear_flags & ITEM_STUCK_IN	) strcat(buf, " stuck_in");
    if (wear_flags & ITEM_WEAR_EAR	) strcat(buf, " ear");
    if (wear_flags & ITEM_WEAR_FACE	) strcat(buf, " face");
    if (wear_flags & ITEM_WEAR_MALE_ONLY) strcat(buf, " maleonly");
    if (wear_flags & ITEM_WEAR_FEMALE_ONLY) strcat(buf, " femaleonly");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *form_bit_name(int form_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (form_flags & FORM_POISON	) strcat(buf, " poison");
    else if (form_flags & FORM_EDIBLE	) strcat(buf, " edible");
    if (form_flags & FORM_MAGICAL	) strcat(buf, " magical");
    if (form_flags & FORM_INSTANT_DECAY	) strcat(buf, " instant_rot");
    if (form_flags & FORM_OTHER		) strcat(buf, " other");
    if (form_flags & FORM_ANIMAL	) strcat(buf, " animal");
    if (form_flags & FORM_SENTIENT	) strcat(buf, " sentient");
    if (form_flags & FORM_UNDEAD	) strcat(buf, " undead");
    if (form_flags & FORM_CONSTRUCT	) strcat(buf, " construct");
    if (form_flags & FORM_MIST		) strcat(buf, " mist");
    if (form_flags & FORM_INTANGIBLE	) strcat(buf, " intangible");
    if (form_flags & FORM_BIPED		) strcat(buf, " biped");
    if (form_flags & FORM_CENTAUR	) strcat(buf, " centaur");
    if (form_flags & FORM_INSECT	) strcat(buf, " insect");
    if (form_flags & FORM_SPIDER	) strcat(buf, " spider");
    if (form_flags & FORM_CRUSTACEAN	) strcat(buf, " crustacean");
    if (form_flags & FORM_WORM		) strcat(buf, " worm");
    if (form_flags & FORM_BLOB		) strcat(buf, " blob");
    if (form_flags & FORM_MAMMAL	) strcat(buf, " mammal");
    if (form_flags & FORM_BIRD		) strcat(buf, " bird");
    if (form_flags & FORM_REPTILE	) strcat(buf, " reptile");
    if (form_flags & FORM_SNAKE		) strcat(buf, " snake");
    if (form_flags & FORM_DRAGON	) strcat(buf, " dragon");
    if (form_flags & FORM_AMPHIBIAN	) strcat(buf, " amphibian");
    if (form_flags & FORM_FISH		) strcat(buf, " fish");
    if (form_flags & FORM_COLD_BLOOD 	) strcat(buf, " cold_blooded");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *part_bit_name(int part_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (part_flags & PART_HEAD		) strcat(buf, " head");
    if (part_flags & PART_ARMS		) strcat(buf, " arms");
    if (part_flags & PART_LEGS		) strcat(buf, " legs");
    if (part_flags & PART_HEART		) strcat(buf, " heart");
    if (part_flags & PART_BRAINS	) strcat(buf, " brains");
    if (part_flags & PART_GUTS		) strcat(buf, " guts");
    if (part_flags & PART_HANDS		) strcat(buf, " hands");
    if (part_flags & PART_FEET		) strcat(buf, " feet");
    if (part_flags & PART_FINGERS	) strcat(buf, " fingers");
    if (part_flags & PART_EAR		) strcat(buf, " ears");
    if (part_flags & PART_EYE		) strcat(buf, " eyes");
    if (part_flags & PART_LONG_TONGUE	) strcat(buf, " long_tongue");
    if (part_flags & PART_EYESTALKS	) strcat(buf, " eyestalks");
    if (part_flags & PART_TENTACLES	) strcat(buf, " tentacles");
    if (part_flags & PART_FINS		) strcat(buf, " fins");
    if (part_flags & PART_WINGS		) strcat(buf, " wings");
    if (part_flags & PART_TAIL		) strcat(buf, " tail");
    if (part_flags & PART_CLAWS		) strcat(buf, " claws");
    if (part_flags & PART_FANGS		) strcat(buf, " fangs");
    if (part_flags & PART_HORNS		) strcat(buf, " horns");
    if (part_flags & PART_SCALES	) strcat(buf, " scales");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *weapon_bit_name(int weapon_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (weapon_flags & WEAPON_FLAMING	) strcat(buf, " flaming");
    if (weapon_flags & WEAPON_FROST	) strcat(buf, " frost");
    if (weapon_flags & WEAPON_VAMPIRIC	) strcat(buf, " vampiric");
    if (weapon_flags & WEAPON_SHARP	) strcat(buf, " sharp");
    if (weapon_flags & WEAPON_VORPAL	) strcat(buf, " vorpal");
    if (weapon_flags & WEAPON_TWO_HANDS ) strcat(buf, " two-handed");
    if (weapon_flags & WEAPON_SHOCKING 	) strcat(buf, " shocking");
    if (weapon_flags & WEAPON_POISON	) strcat(buf, " poison");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *cont_bit_name( int cont_flags)
{
  static char buf[512];
  
  buf[0] = '\0';
  
  if (cont_flags & CONT_CLOSEABLE	) strcat(buf, " closable");
  if (cont_flags & CONT_PICKPROOF	) strcat(buf, " pickproof");
  if (cont_flags & CONT_CLOSED	) strcat(buf, " closed");
  if (cont_flags & CONT_LOCKED	) strcat(buf, " locked");
  if (cont_flags & CONT_ENTERABLE	) strcat(buf, " enterable");
  if (cont_flags & CONT_SOUNDPROOF	) strcat(buf, " soundproof");
  if (cont_flags & CONT_SEE_OUT	) strcat(buf, " seeout");
  if (cont_flags & CONT_SEE_IN	) strcat(buf, " seein");
  if (cont_flags & CONT_SEE_THROUGH) strcat(buf, " seethrough");
  
  return (buf[0] != '\0' ) ? buf+1 : "none";
}


char *off_bit_name(int off_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (off_flags & OFF_AREA_ATTACK	) strcat(buf, " area attack");
    if (off_flags & OFF_BACKSTAB	) strcat(buf, " backstab");
    if (off_flags & OFF_BASH		) strcat(buf, " bash");
    if (off_flags & OFF_BERSERK		) strcat(buf, " berserk");
    if (off_flags & OFF_DISARM		) strcat(buf, " disarm");
    if (off_flags & OFF_DODGE		) strcat(buf, " dodge");
    if (off_flags & OFF_FADE		) strcat(buf, " fade");
    if (off_flags & OFF_FAST		) strcat(buf, " fast");
    if (off_flags & OFF_KICK		) strcat(buf, " kick");
    if (off_flags & OFF_KICK_DIRT	) strcat(buf, " kick_dirt");
    if (off_flags & OFF_PARRY		) strcat(buf, " parry");
    if (off_flags & OFF_RESCUE		) strcat(buf, " rescue");
    if (off_flags & OFF_TAIL		) strcat(buf, " tail");
    if (off_flags & OFF_TRIP		) strcat(buf, " trip");
    if (off_flags & OFF_CRUSH		) strcat(buf, " crush");
    if (off_flags & ASSIST_ALL		) strcat(buf, " assist_all");
    if (off_flags & ASSIST_ALIGN	) strcat(buf, " assist_align");
    if (off_flags & ASSIST_RACE		) strcat(buf, " assist_race");
    if (off_flags & ASSIST_PLAYERS	) strcat(buf, " assist_players");
    if (off_flags & ASSIST_GUARD	) strcat(buf, " assist_guard");
    if (off_flags & ASSIST_VNUM		) strcat(buf, " assist_vnum");
    if (off_flags & TARGET_HEAD		) strcat(buf, " target_head");
    if (off_flags & TARGET_NECK		) strcat(buf, " target_neck");
    if (off_flags & TARGET_TORSO	) strcat(buf, " target_torso");
    if (off_flags & TARGET_BACK		) strcat(buf, " target_back");
    if (off_flags & TARGET_ARMS		) strcat(buf, " target_arms");
    if (off_flags & TARGET_HANDS	) strcat(buf, " target_hands");
    if (off_flags & TARGET_LEGS		) strcat(buf, " target_legs");
    if (off_flags & TARGET_FEET		) strcat(buf, " target_feet");
    if (off_flags & TARGET_GENERAL	) strcat(buf, " target_general");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

bool emptystring( const char * str )
{
	int i = 0;

	for ( ; str[i]; i++ )
		if ( str[i] != ' ' )
			return FALSE;

	return TRUE;
}

char *itos( int temp )
{
	static char buf[64];

	sprintf( buf, "%d", temp );

	return buf;
}

int get_vnum_mob_name_area( char * name, AREA_DATA * pArea )
{
	int hash;
	MOB_INDEX_DATA * mob;

	for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
		for ( mob = mob_index_hash[hash]; mob; mob = mob->next )
			if ( mob->area == pArea
			&&  !str_prefix(name, mob->player_name) )
				return mob->vnum;

	return 0;
}

int get_vnum_obj_name_area( char * name, AREA_DATA * pArea )
{
	int hash;
	OBJ_INDEX_DATA * obj;

	for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
		for ( obj = obj_index_hash[hash]; obj; obj = obj->next )
			if ( obj->area == pArea
			&&  !str_prefix(name, obj->name) )
				return obj->vnum;

	return 0;
}

#if !defined(FIRST_BOOT)
int get_points( int race, int class )
{
	int x;

	x = group_lookup(class_table[class].default_group);

	if ( x == -1 )
	{
		bugf( "get_points : grupo %s inexistente, raza %d, clase %d",
			class_table[class].default_group,
			race, class );
		return -1;
	}

	return group_table[x].rating[class] + race_table[race].points;
}
#endif

 
 /*
  * Config Colour stuff
  */
 void default_colour( CHAR_DATA *ch )
 {
     if( IS_NPC( ch ) )
 	return;
 
     if( !ch->pcdata )
 	return;
 
     ch->pcdata->auction[1]	= ( MAGENTA );
     ch->pcdata->gossip[1]	= ( YELLOW );
     ch->pcdata->music[1]	= ( YELLOW );
     ch->pcdata->chat[1]	= ( CYAN );
     ch->pcdata->minion[1]	= ( YELLOW );
     ch->pcdata->immtalk[1]     = ( MAGENTA );
     ch->pcdata->game[1]	= ( CYAN );
     ch->pcdata->tell[1]	= ( GREEN );
     ch->pcdata->reply[1]	= ( RED );
     ch->pcdata->gtell[1]	= ( GREEN );
     ch->pcdata->room_exits[1]	= ( GREEN );
     ch->pcdata->room_exits[1]	= ( GREEN );
     ch->pcdata->room_things[1]	= ( WHITE );
     ch->pcdata->room_people[1]	= ( WHITE );
     ch->pcdata->room[1]	= ( WHITE );
     ch->pcdata->bondtalk[1]	= ( WHITE );
     ch->pcdata->channel_name[1]	= ( WHITE );
     ch->pcdata->wiznet[1]	= ( WHITE );
     ch->pcdata->prompt[1]	= ( WHITE );
     ch->pcdata->fight_death[1]	= ( RED );
     ch->pcdata->fight_yhit[1]	= ( GREEN );
     ch->pcdata->fight_ohit[1]	= ( YELLOW );
     ch->pcdata->fight_thit[1]	= ( RED );
     ch->pcdata->fight_skill[1]	= ( WHITE );
     ch->pcdata->sayt[1]	= ( GREEN );
     ch->pcdata->osay[1]	= ( RED );
     ch->pcdata->guild_talk[1]	= ( MAGENTA );
     ch->pcdata->wolfkin_talk[1]= ( YELLOW );
     ch->pcdata->race_talk[1]	= ( RED );
     ch->pcdata->df_talk[1]	= ( BLACK );
     ch->pcdata->newbie[1]	= ( BLUE );
	ch->pcdata->sguild_talk[1] = ( BLUE );
	ch->pcdata->ssguild_talk[1] = ( CYAN );
	ch->pcdata->leader_talk[1] = ( RED );
	ch->pcdata->oguild_talk[1] = ( MAGENTA );

     ch->pcdata->auction[0]	= ( NORMAL );
     ch->pcdata->gossip[0]	= ( BRIGHT );
     ch->pcdata->music[0]	= ( NORMAL );
     ch->pcdata->chat[0]	= ( BRIGHT );
     ch->pcdata->minion[0]	= ( BRIGHT );
     ch->pcdata->immtalk[0]     = ( NORMAL );
     ch->pcdata->game[0]	= ( NORMAL );
     ch->pcdata->tell[0]	= ( NORMAL );
     ch->pcdata->reply[0]	= ( NORMAL );
     ch->pcdata->gtell[0]	= ( NORMAL );
     ch->pcdata->room_exits[0]	= ( NORMAL );
     ch->pcdata->room_exits[0]	= ( NORMAL );
     ch->pcdata->room_things[0]	= ( NORMAL );
     ch->pcdata->room_people[0]	= ( NORMAL );
     ch->pcdata->room[0]	= ( NORMAL );
     ch->pcdata->bondtalk[0]	= ( BRIGHT );
     ch->pcdata->channel_name[0]	= ( BRIGHT );
     ch->pcdata->wiznet[0]	= ( BRIGHT );
     ch->pcdata->prompt[0]	= ( NORMAL );
     ch->pcdata->fight_death[0]	= ( BRIGHT );
     ch->pcdata->fight_yhit[0]	= ( NORMAL );
     ch->pcdata->fight_ohit[0]	= ( NORMAL );
     ch->pcdata->fight_thit[0]	= ( NORMAL );
     ch->pcdata->fight_skill[0]	= ( BRIGHT );
     ch->pcdata->sayt[0]	= ( BRIGHT );
     ch->pcdata->osay[0]	= ( BRIGHT );
     ch->pcdata->guild_talk[0]	= ( BRIGHT );
     ch->pcdata->wolfkin_talk[0]= ( BRIGHT );
     ch->pcdata->race_talk[0]	= ( BRIGHT );
     ch->pcdata->df_talk[0]	= ( BRIGHT );
     ch->pcdata->newbie[0]	= ( BRIGHT );
	ch->pcdata->sguild_talk[0] = ( BRIGHT );
	ch->pcdata->ssguild_talk[0] = ( BRIGHT );
	ch->pcdata->leader_talk[0] = ( BRIGHT );
	ch->pcdata->oguild_talk[0] = ( BRIGHT );	

     ch->pcdata->auction[2]	= 0;
     ch->pcdata->gossip[2]	= 0;
     ch->pcdata->music[2]	= 0;
     ch->pcdata->chat[2]	= 0;
     ch->pcdata->minion[2]	= 0;
     ch->pcdata->immtalk[2]     = 0;
     ch->pcdata->game[2]	= 0;
     ch->pcdata->tell[2]	= 0;
     ch->pcdata->reply[2]	= 0;
     ch->pcdata->gtell[2]	= 0;
     ch->pcdata->room_exits[2]	= 0;
     ch->pcdata->room_exits[2]	= 0;
     ch->pcdata->room_things[2]	= 0;
     ch->pcdata->room_people[2]	= 0;
     ch->pcdata->room[2]	= 0;
     ch->pcdata->bondtalk[2]	= 0;
     ch->pcdata->channel_name[2]	= 0;
     ch->pcdata->wiznet[2]	= 0;
     ch->pcdata->prompt[2]	= 0;
     ch->pcdata->fight_death[2]	= 0;
     ch->pcdata->fight_yhit[2]	= 0;
     ch->pcdata->fight_ohit[2]	= 0;
     ch->pcdata->fight_thit[2]	= 0;
     ch->pcdata->fight_skill[2]	= 0;
     ch->pcdata->sayt[2]	= 0;
     ch->pcdata->osay[2]	= 0;
     ch->pcdata->guild_talk[2]	= 0;
     ch->pcdata->wolfkin_talk[2]= 0;
     ch->pcdata->race_talk[2]	= 0;
     ch->pcdata->df_talk[2]	= 0;
     ch->pcdata->newbie[2]	= 0;
	ch->pcdata->sguild_talk[2] = 0;
	ch->pcdata->ssguild_talk[2] = 0;
	ch->pcdata->leader_talk[2] = 0;
	ch->pcdata->oguild_talk[2] = 0;	

 
     return;
 }
 
 void all_colour( CHAR_DATA *ch, char *argument )
 {
     char	buf[  100 ];
     char	buf2[ 100 ];
     int		colour;
     int		bright;
 
     if( IS_NPC( ch ) || !ch->pcdata )
 	return;
 
     if( !*argument )
 	return;
 
     if( !str_prefix( argument, "red" ) )
     {
 	colour = ( RED );
 	bright = NORMAL;
 	sprintf( buf2, "Red" );
     }
     if( !str_prefix( argument, "hi-red" ) )
     {
 	colour = ( RED );
 	bright = BRIGHT;
 	sprintf( buf2, "Red" );
     }
     else if( !str_prefix( argument, "green" ) )
     {
 	colour = ( GREEN );
 	bright = NORMAL;
 	sprintf( buf2, "Green" );
     }
     else if( !str_prefix( argument, "hi-green" ) )
     {
 	colour = ( GREEN );
 	bright = BRIGHT;
 	sprintf( buf2, "Green" );
     }
     else if( !str_prefix( argument, "yellow" ) )
     {
 	colour = ( YELLOW );
 	bright = NORMAL;
 	sprintf( buf2, "Yellow" );
     }
     else if( !str_prefix( argument, "hi-yellow" ) )
     {
 	colour = ( YELLOW );
 	bright = BRIGHT;
 	sprintf( buf2, "Yellow" );
     }
     else if( !str_prefix( argument, "blue" ) )
     {
 	colour = ( BLUE );
 	bright = NORMAL;
 	sprintf( buf2, "Blue" );
     }
     else if( !str_prefix( argument, "hi-blue" ) )
     {
 	colour = ( BLUE );
 	bright = BRIGHT;
 	sprintf( buf2, "Blue" );
     }
     else if( !str_prefix( argument, "magenta" ) )
     {
 	colour = ( MAGENTA );
 	bright = NORMAL;
 	sprintf( buf2, "Magenta" );
     }
     else if( !str_prefix( argument, "hi-magenta" ) )
     {
 	colour = ( MAGENTA );
 	bright = BRIGHT;
 	sprintf( buf2, "Magenta" );
     }
     else if( !str_prefix( argument, "cyan" ) )
     {
 	colour = ( CYAN );
 	bright = NORMAL;
 	sprintf( buf2, "Cyan" );
     }
     else if( !str_prefix( argument, "hi-cyan" ) )
     {
 	colour = ( CYAN );
 	bright = BRIGHT;
 	sprintf( buf2, "Cyan" );
     }
     else if( !str_prefix( argument, "white" ) )
     {
 	colour = ( WHITE );
 	bright = NORMAL;
 	sprintf( buf2, "White" );
     }
     else if( !str_prefix( argument, "hi-white" ) )
     {
 	colour = ( WHITE );
 	bright = BRIGHT;
 	sprintf( buf2, "White" );
     }
     else if( !str_prefix( argument, "grey" ) )
     {
 	colour = ( BLACK );
 	bright = BRIGHT;
 	sprintf( buf2, "White" );
     }
     else
     {
 	send_to_char_bw( "Unrecognised colour, unchanged.\n\r", ch );
 	return;
     }
 
     ch->pcdata->auction[1]	= colour;
     ch->pcdata->gossip[1]	= colour;
     ch->pcdata->music[1]	= colour;
     ch->pcdata->chat[1]	= colour;
     ch->pcdata->minion[1]	= colour;
     ch->pcdata->immtalk[1]     = colour;
     ch->pcdata->game[1]	= colour;
     ch->pcdata->tell[1]	= colour;
     ch->pcdata->reply[1]	= colour;
     ch->pcdata->gtell[1]	= colour;
     ch->pcdata->room_exits[1]	= colour;
     ch->pcdata->room_exits[1]	= colour;
     ch->pcdata->room_things[1]	= colour;
     ch->pcdata->room_people[1]	= colour;
     ch->pcdata->room[1]	= colour;
     ch->pcdata->bondtalk[1]	= colour;
     ch->pcdata->channel_name[1]	= colour;
     ch->pcdata->wiznet[1]	= colour;
     ch->pcdata->prompt[1]	= colour;
     ch->pcdata->fight_death[1]	= colour;
     ch->pcdata->fight_yhit[1]	= colour;
     ch->pcdata->fight_ohit[1]	= colour;
     ch->pcdata->fight_thit[1]	= colour;
     ch->pcdata->fight_skill[1]	= colour;
     ch->pcdata->sayt[1]	= colour;
     ch->pcdata->osay[1]	= colour;
     ch->pcdata->guild_talk[1]	= colour;
     ch->pcdata->wolfkin_talk[1]= colour;
     ch->pcdata->race_talk[1]	= colour;
     ch->pcdata->df_talk[1]	= colour;
     ch->pcdata->newbie[1]	= colour;
	ch->pcdata->sguild_talk[1] = colour;
	ch->pcdata->ssguild_talk[1] = colour;
	ch->pcdata->leader_talk[1] = colour;
	ch->pcdata->oguild_talk[1] = colour;	


     ch->pcdata->auction[0]	= bright;
     ch->pcdata->gossip[0]	= bright;
     ch->pcdata->music[0]	= bright;
     ch->pcdata->chat[0]	= bright;
     ch->pcdata->minion[0]	= bright;
     ch->pcdata->immtalk[0]     = bright;
     ch->pcdata->game[0]	= bright;
     ch->pcdata->tell[0]	= bright;
     ch->pcdata->reply[0]	= bright;
     ch->pcdata->gtell[0]	= bright;
     ch->pcdata->room_exits[0]	= bright;
     ch->pcdata->room_exits[0]	= bright;
     ch->pcdata->room_things[0]	= bright;
     ch->pcdata->room_people[0]	= bright;
     ch->pcdata->room[0]	= bright;
     ch->pcdata->bondtalk[0]	= bright;
     ch->pcdata->channel_name[0]	= bright;
     ch->pcdata->wiznet[0]	= bright;
     ch->pcdata->prompt[0]	= bright;
     ch->pcdata->fight_death[0]	= bright;
     ch->pcdata->fight_yhit[0]	= bright;
     ch->pcdata->fight_ohit[0]	= bright;
     ch->pcdata->fight_thit[0]	= bright;
     ch->pcdata->fight_skill[0]	= bright;
     ch->pcdata->sayt[0]	= bright;
     ch->pcdata->osay[0]	= bright;
     ch->pcdata->guild_talk[0]	= bright;
     ch->pcdata->wolfkin_talk[0]= bright;
     ch->pcdata->race_talk[0]	= bright;
     ch->pcdata->df_talk[0]	= bright;
     ch->pcdata->newbie[0]	= bright;
 	ch->pcdata->sguild_talk[0] = bright;
	ch->pcdata->ssguild_talk[0] = bright;
	ch->pcdata->leader_talk[0] = bright;
	ch->pcdata->oguild_talk[0] = bright;

     sprintf( buf, "All Colour settings set to %s.\n\r", buf2 );
     send_to_char_bw( buf, ch );
 
     return;
 }
 
 /*
  * See if a string is one of the names of an object.
  */
 
 bool is_full_name( const char *str, char *namelist )
 {
     char name[MAX_INPUT_LENGTH];
 
     for ( ; ; )
     {
         namelist = one_argument( namelist, name );
         if ( name[0] == '\0' )
             return FALSE;
         if ( !str_cmp( str, name ) )
             return TRUE;
     }
 }
 
int put_char_in_obj(CHAR_DATA * ch, OBJ_DATA * iobj)
{

   if (ch == NULL || iobj == NULL)
   {
      bug("put_char_in_obj called with NULL parameter(s)", 0);
      return FALSE;
   }

   if (!IS_SET(iobj->value[1], CONT_ENTERABLE))
      return FALSE;

   if (((get_obj_weight(iobj) + ch->carry_weight) > iobj->value[0])
       && (ch->level < IMMORTAL))
      return FALSE;

   while (ch->in_obj != iobj)
   {
      OBJ_DATA *obj;

      for (obj = iobj; (obj) && (obj->in_obj != ch->in_obj); obj = obj->in_obj) ;
      if (!IS_SET(obj->value[1], CONT_ENTERABLE))
      {
         remove_char_from_obj(ch);
         return FALSE;
      }
      else
         char_to_obj(ch, obj);
   }
   return TRUE;
}


/*
 * Removes a char from all objects.  This should be called before
 * commands like "do_goto", teleport speels, etc. are called.
 */
void remove_char_from_obj(CHAR_DATA * ch)
{
   while (ch->in_obj)
      char_from_obj(ch);
}

char *obj_list (OBJ_DATA *obj, CHAR_DATA *to)
{
  static char buffer[MAX_STRING_LENGTH];
  int count = 0;

  *buffer = '\0';
  if (!obj) {
    strcat(buffer, "nothing" );
    return buffer;
  }

  for( ; obj; obj = obj->next_content, count++ ) {
    if( !obj->next_content ) {
	 if( count == 1 )
	   strcat( buffer, " and " );
	 else if( count > 1 )
	   strcat( buffer, ", and " );
    }
    else if( count > 0 )
	 strcat( buffer, ", " );
    strcat( buffer, obj->short_descr );
  }
  return buffer;
  
}

char *name_list( CHAR_IN_DATA *who_in, CHAR_DATA *to )
{
 static char buffer[MAX_STRING_LENGTH];
 int count = 0;

 *buffer = '\0';
 if( !who_in )
 {
  strcat( buffer, "no one" );
  return buffer;
 }

 for( ; who_in; who_in = who_in->next, count++ )
 {
  if( !who_in->next )
  {
   if( count == 1 )
    strcat( buffer, " and " );
   else if( count > 1 )
    strcat( buffer, ", and " );
  }
  else if( count > 0 )
   strcat( buffer, ", " );
  strcat( buffer, PERS( who_in->ch, to) );
 }
 return buffer;
}

char *removechars(char *source, char *charset)
{
  static char target[512];
  int i=0;
  int k=0;
  int cnt=0;
  int match=FALSE;

  /* Reset target each time before use */
  memset(target, 0x00, sizeof(target));
  
  /* Go through each char in source and check if defined in charset */
  for (i=0;i<strlen(source);i++) {
    match = FALSE;
    for (k=0;k<strlen(charset);k++) {
      if (source[i] == charset[k])
        match = TRUE;
    }
    if (!match) {
      target[cnt] = source[i];
      cnt++;
    }
  }
  return(target);
}

/*
 * True if char can see victim on the channels.
 */
bool can_see_channel( CHAR_DATA *ch, CHAR_DATA *victim )
{
  //OBJ_DATA *to_obj;
  //OBJ_DATA *ch_obj;

   /* RT changed so that WIZ_INVIS has levels */
   if ( ch == victim )
	return TRUE;
   
   if ( get_trust(ch) < victim->invis_level)
	return FALSE;

   if (get_trust(ch) < victim->incog_level && ch->in_room != victim->in_room)
	return FALSE;
   
   if ( (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) 
	   ||   (IS_NPC(ch) && IS_IMMORTAL(ch)))
	return TRUE;
   
   return TRUE;
}

int get_level(CHAR_DATA *ch)
{
  if (!IS_NPC(ch))
    return(ch->level + ch->pcdata->extended_level);
  else
    return(ch->level);
}

int get_max_hit_loc(CHAR_DATA *ch, int location)
{
  switch(location) {
  case (LOC_LA):
    if (ch->max_hit < LOC_MIN_HIT)
	 return(LOC_MIN_HIT);
    else
	 return(UMAX(1, ch->max_hit/LOC_MOD_LA));
    break;
  case (LOC_LL):
    if (ch->max_hit < LOC_MIN_HIT)
	 return(LOC_MIN_HIT);
    else
	 return(UMAX(1, ch->max_hit/LOC_MOD_LL));
    break;
  case (LOC_HE):
    if (ch->max_hit < LOC_MIN_HIT)
	 return(LOC_MIN_HIT);
    else
	 return(UMAX(1, ch->max_hit/LOC_MOD_HE));
    break;
  case (LOC_BD):
    if (ch->max_hit < LOC_MIN_HIT)
	 return(LOC_MIN_HIT);
    else
	 return(UMAX(1, ch->max_hit/LOC_MOD_BD));
    break;
  case (LOC_RA):
    if (ch->max_hit < LOC_MIN_HIT)
	 return(LOC_MIN_HIT);
    else
	 return(UMAX(1, ch->max_hit/LOC_MOD_RA));
    break;
  case (LOC_RL):
    if (ch->max_hit < LOC_MIN_HIT)
	 return(LOC_MIN_HIT);
    else
	 return(UMAX(1, ch->max_hit/LOC_MOD_RL));
    break;
  }

  return(0);
}
