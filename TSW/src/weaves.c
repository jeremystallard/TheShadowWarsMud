/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  TSW Mud improvments copyright (C) 2000-2003 by Swordfish and Zandor.   *
 *                                                                         *
 *  This file includes weaves that are used for The Shadow Wars            *
 *  Related functions for the difference weaves are also included          *
 *                                                                         *
 ***************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "olc.h"
#include "interp.h"
#include "recycle.h"
#include "magic.h"
#include "tables.h"

void do_wolfshape( CHAR_DATA *ch, char *argument ); /* wolfkin.c  */
int  find_exit( CHAR_DATA *ch, char *arg );          /* act_move.c */
bool check_parse_name (char* name);                 /* comm.c     */
void	affect_modify	 args( ( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd ) ); /* handler.c */
bool remove_obj      args( (CHAR_DATA *ch, int iWear, bool fReplace ) );     /* act_obj.c */

extern char *target_name;
extern char *cast_arg2;
extern char *cast_arg3;

/**********************************************************************
*       Function      : spell_create_angreal
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_create_angreal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{  
  OBJ_DATA *angreal;
  AFFECT_DATA af;
  char buf[MSL];
  int sn_skill=0;
  int sphere_max=get_skill(ch,sn)/10;      // Max sphere add
  long wearloc=0;
  const int MAX_HELD_ANGREAL=14;

  // Temp calc values
  int sphere_air_calc=0;
  int sphere_earth_calc=0;
  int sphere_fire_calc=0;
  int sphere_spirit_calc=0;
  int sphere_water_calc=0;
  
  // Real add values
  int sphere_air=0;
  int sphere_earth=0;
  int sphere_fire=0;
  int sphere_spirit=0;
  int sphere_water=0;

  bool isAngreal=FALSE;
  bool locAngreal=FALSE;
  
  
  // Only for PCs
  if(IS_NPC(ch))
    return;

  if (!IS_IMMORTAL(ch))
  {
     //Only for IC PC's
     if (!IS_RP(ch))
     {
	   send_to_char("This weave is for IC use only.  Enter RP mode to use it.!\n\r",ch);
	   return;
     }
  }
     // Check if able to make angreals to other wear locations    
    //CREATEANGREAL talent?
    if (IS_SET(ch->talents, TALENT_CREATE_ANGREAL)) {
        if (get_skill(ch,sn) >= 300)
           locAngreal=TRUE;
    }
  
  sn_skill=get_skill(ch,sn)/4;

  // If enough skilled, can make angreals to other wear locations
  if (locAngreal) {
     if (IS_NULLSTR(cast_arg2)) {
       send_to_char("Syntax: weave 'create angreal' <wear location>\n\r", ch);
       return;  	
     }
     else {
        if ((strcasecmp(cast_arg2,"head")) &&
            (strcasecmp(cast_arg2,"wrist")) &&
            (strcasecmp(cast_arg2,"neck")) &&
//            (strcasecmp(cast_arg2,"waist")) &&
            (strcasecmp(cast_arg2,"finger")) &&
            (strcasecmp(cast_arg2,"hold"))) {  
          //send_to_char("Valid options are: head, wrist, neck, waist, finger, hold\r\n",ch);
          send_to_char("Valid options are: head, wrist, neck, finger, hold\r\n",ch);
          return;
        }
        else {
          if ( (wearloc = flag_value( wear_flags, cast_arg2 )) == NO_FLAG ) {
             wearloc  = ITEM_TAKE;
             wearloc ^= ITEM_HOLD;
          }
          else {
             wearloc ^= ITEM_TAKE;
          }
        }
     }
  }
    
  if (number_percent() > sn_skill && !IS_IMMORTAL(ch)) {
    sprintf(buf, "You try to combine flows of %s into %s but you are just unable to set the pattern correct.\n\r", (char *)flow_text(sn, ch), skill_table[sn].name);
    send_to_char(buf, ch);
    return;
  }

   if (current_time < ch->pcdata->next_createangreal && !IS_IMMORTAL(ch))
   {
	send_to_char("You try and create an angreal, but find that you are too exhausted from your last attempt and will need to wait a while!\n\r",ch);
	ch->pcdata->createangrealcount++;
	return;
   }

  if (current_time < ch->pcdata->next_24hourangreal)
  {
  	ch->pcdata->createangrealcount++;
  }  
  else
  {
  	ch->pcdata->createangrealcount = 1;
  }
  ch->pcdata->next_24hourangreal = current_time+(60*60*24); /* Every other hour *///(86400 * 2); /* Every other day*/
  
  // Set up Max possible values for each sphere
  sphere_air_calc    = UMIN(ch->perm_sphere[SPHERE_AIR]/10, sphere_max);
  sphere_earth_calc  = UMIN(ch->perm_sphere[SPHERE_EARTH]/10, sphere_max);
  sphere_fire_calc   = UMIN(ch->perm_sphere[SPHERE_FIRE]/10, sphere_max);
  sphere_spirit_calc = UMIN(ch->perm_sphere[SPHERE_SPIRIT]/10, sphere_max);
  sphere_water_calc  = UMIN(ch->perm_sphere[SPHERE_WATER]/10, sphere_max);

  angreal = create_object(get_obj_index(OBJ_VNUM_ANGREAL), 0);
  
  angreal->level = level > 90 ? 90 : level;
  angreal->value[2] = 1900;
  
  if (locAngreal) {
    angreal->wear_flags = wearloc;

	   if (wearloc & ITEM_WEAR_FINGER) {
 	   	free_string(angreal->short_descr);
	   	angreal->short_descr = str_dup("a ring with a small stone");

		free_string(angreal->description);
	   	angreal->description = str_dup("A ring with a small stone is here.");
	   	
		free_string(angreal->name);
	   	angreal->name = str_dup("angreal ring");	   	
	   	
	  }
//	  else if (wearloc & ITEM_WEAR_WAIST) {
// 	   	free_string(angreal->short_descr);
//	   	angreal->short_descr = str_dup("a belt with a small stone");
//
//		free_string(angreal->description);
//	   	angreal->description = str_dup("A belt with a small stone is here.");
//	   	
//	   }
	   else if (wearloc & ITEM_WEAR_NECK) {
 	   	free_string(angreal->short_descr);
	   	angreal->short_descr = str_dup("a pendant with a small stone");

		free_string(angreal->description);
	   	angreal->description = str_dup("A pendant with a small stone is here.");
	   	
		free_string(angreal->name);
	   	angreal->name = str_dup("angreal pendant");	   		   		   	
	   }
	   else if (wearloc & ITEM_WEAR_WRIST) {
 	   	free_string(angreal->short_descr);
	   	angreal->short_descr = str_dup("a bracelet with a small stone");

		free_string(angreal->description);
	   	angreal->description = str_dup("A bracelet with a small stone is here.");
	   	
		free_string(angreal->name);
	   	angreal->name = str_dup("angreal bracelet");	   		   		   	
	   }
	   else if (wearloc & ITEM_WEAR_HEAD) {
 	   	free_string(angreal->short_descr);
	   	angreal->short_descr = str_dup("a circlet set with a small stone");

		free_string(angreal->description);
	   	angreal->description = str_dup("A circlet set with a small stone is here.");

		free_string(angreal->name);
	   	angreal->name = str_dup("angreal circlet");	   		   		   		   		   	
	   }   	
  }
  
  // Air
  //if (number_percent() <= sn_skill) {
    af.location   = APPLY_SPHERE_AIR;
    sphere_air    = number_range(1, sphere_air_calc) / ch->pcdata->createangrealcount;
    af.modifier   = sphere_air;
    if (af.modifier > 0)
	 isAngreal=TRUE;
    af.where      = TO_OBJECT;
    af.type       = sn;
    af.duration   = -1;
    af.bitvector  = 0;
    af.level      = angreal->level;
    affect_to_obj(angreal, &af);
  //}
  
  // Earth
  //if (number_percent() <= sn_skill) {
    af.location   = APPLY_SPHERE_EARTH;
    sphere_earth  = number_range(1, sphere_earth_calc) / ch->pcdata->createangrealcount;
    af.modifier   = sphere_earth;
    if (af.modifier > 0)
	 isAngreal=TRUE;
    af.where      = TO_OBJECT;
    af.type       = sn;
    af.duration   = -1;
    af.bitvector  = 0;
    af.level      = angreal->level;
    affect_to_obj(angreal, &af);
  //}
  
  // Fire
  //if (number_percent() <= sn_skill) {
    af.location   = APPLY_SPHERE_FIRE;
    sphere_fire   = number_range(1, sphere_fire_calc) / ch->pcdata->createangrealcount;
    af.modifier   = sphere_fire;
    if (af.modifier > 0)
	 isAngreal=TRUE;
    af.where      = TO_OBJECT;
    af.type       = sn;
    af.duration   = -1;
    af.bitvector  = 0;
    af.level      = angreal->level;
    affect_to_obj(angreal, &af);
  //}
  
  // Spirit
  //if (number_percent() <= sn_skill) {
    af.location   = APPLY_SPHERE_SPIRIT;
    sphere_spirit = number_range(1, sphere_spirit_calc) / ch->pcdata->createangrealcount;
    af.modifier   = sphere_spirit;
    if (af.modifier > 0)
	 isAngreal=TRUE;
    af.where      = TO_OBJECT;
    af.type       = sn;
    af.duration   = -1;
    af.bitvector  = 0;
    af.level      = angreal->level;
    affect_to_obj(angreal, &af);
  //}
  
  // Water
  //if (number_percent() <= sn_skill) {
    af.location   = APPLY_SPHERE_WATER;
    sphere_water  = number_range(1, sphere_water_calc) / ch->pcdata->createangrealcount;
    af.modifier   = sphere_water;
    if (af.modifier > 0)
	 isAngreal=TRUE;
    af.where      = TO_OBJECT;
    af.type       = sn;
    af.duration   = -1;
    af.bitvector  = 0;
    af.level      = angreal->level;
    affect_to_obj(angreal, &af);  
  //}
  
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;
  int angreal_count = 0;

  for ( obj = ch->carrying; obj != NULL; obj = obj_next ) {
       obj_next = obj->next_content;
       if (obj->item_type == ITEM_ANGREAL) {
	   angreal_count++;
       }
  }
  if (angreal_count > MAX_HELD_ANGREAL)
  {
	isAngreal = FALSE;
  }

  if (isAngreal)
  {
    angreal_count == 0;
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( obj->item_type != ITEM_ANGREAL )
            continue;
	if ( obj->owner == NULL || obj->owner[0] == '\0' || str_cmp(obj->owner, ch->name))
	    continue;
	angreal_count++;
    } 
    if (angreal_count > MAX_HELD_ANGREAL + 1)
    {
	isAngreal = FALSE;
    }

  }


  // If no modifiers, it's destroyed
  if (!isAngreal) {
    sprintf(buf, "You focus greatly on $p as you combine the flows of %s, but somehow when the pattern snap in place %s break into dust!", (char *)flow_text(sn, ch), angreal->short_descr);
    act(buf, ch, angreal, NULL, TO_CHAR);
    
    act("$n seems to concentrate deeply on $p for a long time then it suddenly break into dust!.", ch, angreal, NULL, TO_ROOM);
    
    extract_obj(angreal);
    return;
  }
  else {
    sprintf(buf, "You combine flows of %s into a intricating pattern that you set on $p.", (char *)flow_text(sn, ch));
    act(buf, ch, angreal, NULL, TO_CHAR);
    
    act("$n seems to concentrate deeply on $p for a long time before picking it up.", ch, angreal, NULL, TO_ROOM);
    
    angreal->owner = str_dup(ch->name);
    obj_to_char(angreal, ch);  
    
    ch->pcdata->next_createangreal = current_time+(60*60*2); /* Every other hour *///(86400 * 2); /* Every other day*/
    // Log it
    sprintf(buf, "$N created an angreal (wearloc=%s) (A=%d, E=%d, F=%d, S=%d, W=%d)", locAngreal ? cast_arg2 : "hold", sphere_air, sphere_earth, sphere_fire, sphere_spirit, sphere_water);
    wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
    sprintf(buf, "%s created an angreal (wearloc=%s) (A=%d, E=%d, F=%d, S=%d, W=%d)", ch->name, locAngreal ? cast_arg2 : "hold", sphere_air, sphere_earth, sphere_fire, sphere_spirit, sphere_water);
    log_string(buf);
  }
  
  return;
}

/**********************************************************************
*       Function      : spell_slice
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_slice( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim_flows=NULL;
  CHAR_DATA *victim_owner=NULL;
  AFFECT_DATA *paf;            // Normal affects
  AFFECT_DATA *paf_next;
  AFFECT_DATA *pWeave;         // Room weaves
  AFFECT_DATA *pWeave_next;
  WARD_DATA   *pWard;          // Ward weaves
  WARD_DATA   *pWard_next;
  char buf[MSL];
  bool found=FALSE;
  int i=0;
  long vic_flows=0;
  long ch_flows =0;
  int ch_skill=0;
  int vic_skill=0;
  

  // validate syntax
  if (IS_NULLSTR(cast_arg2) || IS_NULLSTR(cast_arg3)) {
    send_to_char("Syntax: weave slice <weave-to-slice> <target>\n\r", ch);
    send_to_char("Syntax: weave slice <weave-to-slice> room\n\r", ch);
    send_to_char("Syntax: weave slice <weave-to-slice> ward\n\r", ch);
    return;
  }
  
  if (!str_cmp(cast_arg3, "room")) {
    for (pWeave = ch->in_room->weaves; pWeave != NULL; pWeave = pWeave_next) {
	 pWeave_next = pWeave->next;
	 
	 // Only for sustained weaves
	 if (pWeave->duration != SUSTAIN_WEAVE)
	   continue;
	 
	 if (skill_table[pWeave->type].spell_fun == spell_null)
	   continue;
	 
	 if (pWeave->casterId == ch->id)
	   continue;

	  if (pWeave->inverted)
	     continue;
	 
	 if (str_cmp(cast_arg2, skill_table[pWeave->type].name))
	   continue;
	 else {
	   
	   pWeave->caster = get_charId_world(ch, pWeave->casterId);
	   
	   // If not same sex, roll to see if able to slice
	   if (pWeave->caster != NULL && pWeave->caster->sex != ch->sex && !IS_IMMORTAL(ch)) {
		if (number_percent() > 33)
		  continue;
	   }
	   
	   victim_owner = get_charId_world(ch, pWeave->casterId);
	   
	   if (victim_owner == NULL) {
		send_to_char("You try to slice through something that isn't there anymore.\n\r", ch);
		return;
	   }
	   
	   // Get strength in the weave
	   for (i = 0; i < MAX_SPHERE; i++) {
		if (skill_table[pWeave->type].spheres[i] > 0 || i == SPHERE_SPIRIT) {
		  vic_flows += victim_owner->perm_sphere[i];
		  ch_flows  += ch->perm_sphere[i];
		}
	   }
	   
	   // Get skill in the weaves
	   ch_skill  = get_skill(ch, pWeave->type);
	   vic_skill = get_skill(victim_owner, pWeave->type);
	   
	   vic_skill = (vic_skill*5)/4;
	   
	   // Skill in slice also count
	   ch_skill += get_skill(ch, sn);
	   vic_skill += get_skill(victim_owner, sn);
	   
	   if (IS_CODER(ch)) {
		sprintf(buf, "[ {YCoder {x]: ch_skill > vic_skill => %d > %d || ch_flows > vic_flow => %ld > %ld\n\r", ch_skill, vic_skill, ch_flows, vic_flows);
		send_to_char(buf, ch);	   
	   }
	   
	   // Slice ?
	   if ( (ch_skill > vic_skill) || (ch_flows > vic_flows) ) {
		sprintf(buf, "You {rslice{x through %s's flows of %s combined into %s!\n\r", PERS(victim_owner, ch),  (char *) flow_text(pWeave->type, ch), skill_table[pWeave->type].name);
		send_to_char(buf, ch);
		sprintf(buf, "%s {rslice{x through your flows of %s combined into %s!\n\r", PERS(ch, victim_owner), (char *) flow_text(pWeave->type, ch), skill_table[pWeave->type].name);
		send_to_char(buf, victim_owner);
		
		room_weave_remove( ch->in_room, pWeave );
  		WAIT_STATE(victim_owner,skill_table[sn].beats);
		
		return;
	   }
	   else {
		sprintf(buf, "You try to slice through %s's flow of %s, but fails!\n\r", PERS(victim_owner, ch), (char *) flow_text(pWeave->type, ch));
		send_to_char(buf, ch);
		sprintf(buf, "You feel %s try to slice through your flows of %s combined into %s!\n\r", PERS(ch, victim_owner), (char *) flow_text(pWeave->type, ch), skill_table[pWeave->type].name);
		send_to_char(buf, victim_owner);	 
		
		return;
	   }    
	 }
    }
    
    if (!found) {
	 send_to_char("You find no weave with that name that is sustained in this room by others.\n\r", ch);
	 return;
    }
  }
  else if (!str_cmp(cast_arg3, "ward")  || !str_cmp(cast_arg3, "wards")) {
    for (pWard = ch->in_room->wards; pWard != NULL; pWard = pWard_next) {
	 pWard_next = pWard->next;
	 
	 // Only for sustained weaves
	 if (pWard->duration != SUSTAIN_WEAVE)
	   continue;
	 
	 if (skill_table[pWard->sn].spell_fun == spell_null)
	   continue;
	 
	 if (pWard->casterId == ch->id)
	   continue;
	 
	  if (pWard->inverted)
	     continue;
	 
	 if (str_cmp(cast_arg2, skill_table[pWard->sn].name))
	   continue;
	 else {
	   
	   pWard->caster = get_charId_world(ch, pWard->casterId);
	   
	   // If not same sex, roll to see if able to slice
	   if (pWard->caster != NULL && pWard->caster->sex != ch->sex && !IS_IMMORTAL(ch)) {
		if (number_percent() > 33)
		  continue;
	   }
	   
	   victim_owner = get_charId_world(ch, pWard->casterId);
	   
	   if (victim_owner == NULL) {
		send_to_char("You try to slice through something that isn't there anymore.\n\r", ch);
		return;
	   }
	   
	   // Get strength in the weave
	   for (i = 0; i < MAX_SPHERE; i++) {
		if (skill_table[pWard->sn].spheres[i] > 0 || i == SPHERE_SPIRIT) {
		  vic_flows += victim_owner->perm_sphere[i];
		  ch_flows  += ch->perm_sphere[i];
		}
	   }
	   
	   // Get skill in the weaves
	   ch_skill  = get_skill(ch, pWard->sn);
	   vic_skill = get_skill(victim_owner, pWard->sn);
	   
	   vic_skill = (vic_skill*5)/4;
	   
	   // Skill in slice also count
	   ch_skill += get_skill(ch, sn);
	   vic_skill += get_skill(victim_owner, sn);
	   
	   if (IS_CODER(ch)) {
		sprintf(buf, "[ {YCoder {x]: ch_skill > vic_skill => %d > %d || ch_flows > vic_flow => %ld > %ld\n\r", ch_skill, vic_skill, ch_flows, vic_flows);
		send_to_char(buf, ch);	   
	   }
	   
	   // Slice ?
	   if ( (ch_skill > vic_skill) || (ch_flows > vic_flows) ) {
		sprintf(buf, "You {rslice{x through %s's flows of %s combined into %s!\n\r", PERS(victim_owner, ch),  (char *) flow_text(pWard->sn, ch), skill_table[pWard->sn].name);
		send_to_char(buf, ch);
		sprintf(buf, "%s {rslice{x through your flows of %s combined into %s!\n\r", PERS(ch, victim_owner), (char *) flow_text(pWard->sn, ch), skill_table[pWard->sn].name);
		send_to_char(buf, victim_owner);
		
		ward_remove( ch->in_room, pWard );
  		WAIT_STATE(victim_owner,skill_table[sn].beats);
		
		return;
	   }
	   else {
		sprintf(buf, "You try to slice through %s's flow of %s, but fails!\n\r", PERS(victim_owner, ch), (char *) flow_text(pWard->sn, ch));
		send_to_char(buf, ch);
		sprintf(buf, "You feel %s try to slice through your flows of %s combined into %s!\n\r", PERS(ch, victim_owner), (char *) flow_text(pWard->sn, ch), skill_table[pWard->sn].name);
		send_to_char(buf, victim_owner);	 
		
		return;
	   }    
	 }
    }
    
    if (!found) {
	 send_to_char("You find no ward weave with that name that is sustained here by others.\n\r",  ch);
	 return;
    }
  }


  // Target
  if ((victim_flows = get_char_room(ch, cast_arg3)) == NULL) {
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }
  else {
    // Find and try to slice it
    for (paf = victim_flows->affected; paf != NULL; paf = paf_next) {
	 paf_next = paf->next;
    
	 // Only for sustained weaves
	 if (paf->duration != SUSTAIN_WEAVE)
	   continue;
	 
	 if (skill_table[paf->type].spell_fun == spell_null)
	   continue;
	 
	 if (paf->casterId == ch->id)
	   continue;
	 
         if (paf->inverted)
	     continue;

	 if (str_cmp(cast_arg2, skill_table[paf->type].name))
	   continue;
	 else {
	   
	   paf->caster = get_charId_world(ch, paf->casterId);
	   
	   // If not same sex, roll to see if able to slice
	   if (paf->caster != NULL && paf->caster->sex != ch->sex && !IS_IMMORTAL(ch)) {
		if (number_percent() > 33)
		  continue;
	   }
	   
	   victim_owner = get_charId_world(ch, paf->casterId);
	   
	   if (victim_owner == NULL) {
		send_to_char("You try to slice through something that isn't there anymore.\n\r", ch);
		return;
	   }
	   
	   // Get strength in the weave
	   for (i = 0; i < MAX_SPHERE; i++) {
		if (skill_table[paf->type].spheres[i] > 0 || i == SPHERE_SPIRIT) {
		  vic_flows += victim_owner->perm_sphere[i];
		  ch_flows  += ch->perm_sphere[i];
		}
	   }
	   
	   // Get skill in the weaves
	   ch_skill  = get_skill(ch, paf->type);
	   vic_skill = get_skill(victim_owner, paf->type);
	   
	   vic_skill = (vic_skill*5)/4;
	   
	   // Skill in slice also count
	   ch_skill += get_skill(ch, sn);
	   vic_skill += get_skill(victim_owner, sn);
	   
	   if (IS_CODER(ch)) {
		sprintf(buf, "[ {YCoder {x]: ch_skill > vic_skill => %d > %d || ch_flows > vic_flow => %ld > %ld\n\r", ch_skill, vic_skill, ch_flows, vic_flows);
		send_to_char(buf, ch);	   
	   }
	   
	   // Slice ?
	   if ( (ch_skill > vic_skill) || (ch_flows > vic_flows) ) {
		sprintf(buf, "You {rslice{x through %s's flows of %s combined into %s!\n\r", PERS(victim_owner, ch),  (char *) flow_text(paf->type, ch), skill_table[paf->type].name);
		send_to_char(buf, ch);
		sprintf(buf, "%s {rslice{x through your flows of %s combined into %s!\n\r", PERS(ch, victim_owner), (char *) flow_text(paf->type, ch), skill_table[paf->type].name);
		send_to_char(buf, victim_owner);
		
  		WAIT_STATE(victim_owner,skill_table[sn].beats);
		affect_remove( victim_flows, paf );	 
		
		return;
	   }
	   else {
		sprintf(buf, "You try to slice through %s's flow of %s, but fails!\n\r", PERS(victim_owner, ch), (char *) flow_text(paf->type, ch));
		send_to_char(buf, ch);
		sprintf(buf, "You feel %s try to slice through your flows of %s combined into %s!\n\r", PERS(ch, victim_owner), (char *) flow_text(paf->type, ch), skill_table[paf->type].name);
		send_to_char(buf, victim_owner);	 
		
		return;
	   }    
	 }
    }

    if (!found) {
	 sprintf(buf,"You find no weave with that name that is sustained on %s.\n\r",  PERS(victim_flows, ch));
	 send_to_char(buf, ch);    
    }
  }
  
}

/**********************************************************************
*       Function      : spell_wrap
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_wrap( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  char buf[MSL];

  //  It would be completely IC to target someone who is fighting
/*
  if (!IS_NPC(victim) && victim->fighting) {
    send_to_char( "You are unable to target these flows on someone who is in combat.\n\r", ch );
    return;
  }
*/
  int chance=100;
  if (victim->fighting)
  {
	chance -= number_range(25,40);
  }
  if (ch->fighting)
  {
	chance -= number_range(33,66);
  }

  if (number_percent() > chance)
  {
	send_to_char("You are unable to focus through the combat and wrap this target.\n\r",ch);
	return;
  }
  
  if (!IS_NPC(victim) && !IS_NPC(ch) && ch->pcdata->next_wrap > current_time && !IS_IMMORTAL(ch)) {
    sprintf(buf, "You need to be in the room for %ld more seconds before you can wrap $N.", (ch->pcdata->next_wrap - current_time) < 0 ? 0 : (ch->pcdata->next_wrap -  current_time));
    act( buf, ch, NULL, victim, TO_CHAR);
    return;	
  }
  
  if (IS_AFFECTED(victim, AFF_WRAPPED)) {
    return;
  }

  if (IS_GHOLAM(victim))
  {
	send_to_char("Your weaving has no affect on this person!!\r\n",ch);
        if (!IS_NPC(victim))
        	act("$n just tried to weave on you!\r\n",ch,NULL,victim,TO_VICT);  
	return;
  }
    

  af.where	   = TO_AFFECTS;
  af.casterId     = ch->id;
  af.type         = sn;
  af.level        = level;
  if (!IS_NPC(ch))
    af.duration     = SUSTAIN_WEAVE;
  else
    af.duration     = ch->level / 10;
  af.location     = APPLY_NONE;
  if (IS_NPC(ch))
    af.modifier     = (ch->holding/10);
  else
    af.modifier     = (ch->holding/10) + get_skill(ch,sn);
  af.bitvector    = AFF_WRAPPED;
  affect_to_char(victim, &af );
  
  act("You suddenly feel as if {Wair{x entwines around you and prevents you from moving!", ch, NULL, victim, TO_VICT);
  act("You wrap $N with thin laces of {Wair{x, preventing them from moving.", ch, NULL, victim, TO_CHAR);
  
  // Log it if PC
  if (!IS_NPC(victim) && !IS_NPC(ch)) {
    sprintf(buf, "%s put a wrap weave on %s in room %d [AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]",
                 ch->name, 
                 victim->name, 
                 victim->in_room->vnum,
                 IS_SET(victim->comm,COMM_AFK) ? "Yes" : "No",
		 victim->timer > 3             ? "Yes" : "No",
		 victim->timer,
		 IS_RP(ch)                     ? "Yes" : "No",
		 IS_RP(victim)                 ? "Yes" : "No",
		 position_table[victim->position].name);
     log_string(buf);
  }
  
}

/**********************************************************************
*       Function      : spell_suffocate
*       Author        : Zandor
*       Description   : Block someones windpipe, making it so they can't breath
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_suffocate( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

  if (IS_NPC(ch))
  {
	send_to_char("This is not to be used by mobs!\r\n",ch);
	return;
  }
  if (IS_NPC(victim) && !IS_AFFECTED(victim,AFF_CHARM)) {
	send_to_char("This is not to be used on innocent mobs!\r\n",ch);
	return;
  }
  if (IS_AFFECTED(victim, AFF_SUFFOCATING)) {
    return;
  }

  af.where	   = TO_AFFECTS;
  af.casterId     = ch->id;
  af.type         = sn;
  af.level        = level;
  if (!IS_NPC(ch))
    af.duration     = SUSTAIN_WEAVE;
  else
    af.duration     = ch->level / 5;
  af.location     = APPLY_NONE;
  if (IS_NPC(ch))
    af.modifier     = (ch->holding/5);
  else
    af.modifier     = (ch->holding/5) + get_skill(ch,sn);
  af.bitvector    = AFF_SUFFOCATING;
  affect_to_char(victim, &af );
  
  act("You suddenly feel as if {Wsomething{x prevents you from breathing!", ch, NULL, victim, TO_VICT);
  act("You smother $N with thin laces of {Wair{x, preventing them from breathing.", ch, NULL, victim, TO_CHAR);
  
}


/**********************************************************************
*       Function      : spell_gag
*       Author        : Zandor
*       Description   : silence someone on all channels for X time 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_gag( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  char buf[MSL];

  if (!IS_NPC(victim) && victim->fighting) {
    send_to_char( "You are unable to target these flows on someone who is in combat.\n\r", ch );
    return;
  }

  if (!IS_NPC(victim) && !IS_NPC(ch) && ch->pcdata->next_wrap > current_time && !IS_IMMORTAL(ch)) {
    sprintf(buf, "You need to be in the room for %ld more seconds before you can gag $N.", (ch->pcdata->next_wrap - current_time) < 0 ? 0 : (ch->pcdata->next_wrap -  current_time));
    act( buf, ch, NULL, victim, TO_CHAR);
    return;	
  }
  
  if (IS_AFFECTED(victim, AFF_GAGGED)) {
    return;
  }

  af.where	   = TO_AFFECTS;
  af.casterId     = ch->id;
  af.type         = sn;
  af.level        = level;
  if (!IS_NPC(ch))
    af.duration     = SUSTAIN_WEAVE;
  else
    af.duration     = ch->level / 5;
  af.location     = APPLY_NONE;
  if (IS_NPC(ch))
    af.modifier     = (ch->holding/5);
  else
    af.modifier     = (ch->holding/5) + get_skill(ch,sn);
  af.bitvector    = AFF_GAGGED;
  affect_to_char(victim, &af );
  
  act("You suddenly feel as if {Wair{x prevents you from speaking!", ch, NULL, victim, TO_VICT);
  act("You gag $N with thin laces of {Wair{x, preventing them from speaking.", ch, NULL, victim, TO_CHAR);
  
  // Log it if PC
  if (!IS_NPC(victim) && !IS_NPC(ch)) {
    sprintf(buf, "%s put a gag weave on %s in room %d [AFK=%s, IDLE=%s (%d), CIC=%s, VIC=%s, VPOS=%s]",
                 ch->name, 
                 victim->name, 
                 victim->in_room->vnum,
                 IS_SET(victim->comm,COMM_AFK) ? "Yes" : "No",
		 victim->timer > 3             ? "Yes" : "No",
		 victim->timer,
		 IS_RP(ch)                     ? "Yes" : "No",
		 IS_RP(victim)                 ? "Yes" : "No",
		 position_table[victim->position].name);
     log_string(buf);
  }  
  
}

/**********************************************************************
*       Function      : spell_shielding
*       Author        : Swordfish
*       Description   : Shield another player from the True Source
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_shielding( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  double shield_strength_mod=0;
  //char buf[MSL];
 
  /* Do we want to tell this? */ 
  if (IS_AFFECTED(victim, AFF_SHIELDED)) {
    send_to_char("That person is already shielded from the True Source.\n\r", ch);
    return;
  }

  if (victim->class != CLASS_CHANNELER) {
     return;	
  }

  /*
  if ( saves_spell( level, victim, DAM_OTHER) && victim->class == CLASS_CHANNELER) {
    act("$N shivers slightly, but it passes quickly.", ch, NULL, victim, TO_CHAR );
    send_to_char("You shiver slightly, but it passes quickly.\n\r",victim);
    return;
  }
  */

  if (!IS_NPC(victim) && (IS_SET(victim->act,PLR_AUTOSLICE))) { 
     
	int victim_op = (victim->perm_sphere[SPHERE_SPIRIT] * 2) + victim->perm_sphere[SPHERE_WATER] + victim->perm_sphere[SPHERE_AIR] + victim->perm_sphere[SPHERE_FIRE] + victim->perm_sphere[SPHERE_EARTH];
	int ch_op = (ch->perm_sphere[SPHERE_SPIRIT] * 2) + ch->perm_sphere[SPHERE_WATER] + ch->perm_sphere[SPHERE_AIR] + ch->perm_sphere[SPHERE_FIRE] + ch->perm_sphere[SPHERE_EARTH];
	int level_modifier = get_skill(ch,sn) - get_skill(victim,sn);

  	//it's possible to shield someone already Channelling, but more difficult than
  	// if they weren't touching the Source. (III: 654)
  	if (IS_AFFECTED(victim,AFF_CHANNELING)) {
	   victim_op = (victim_op * 3)/2;
     	}

	//make the dice roll
	if (number_percent() > ch_op - victim_op + level_modifier) {
		//failed	
    		act("$N manages to keep you from being shielded.", ch, NULL, victim, TO_CHAR );
    		send_to_char("You detect the incoming shielding and avoid it.\n\r",victim);
    		return;
        }
			
  }
  
  shield_strength_mod = (double)(ch->holding/(double)get_curr_op(ch));
  if (shield_strength_mod > 1)
     shield_strength_mod = 1;
    
  af.where	   = TO_AFFECTS;
  af.casterId     = ch->id;
  af.type         = sn;
  af.level        = level;
  if (!IS_NPC(ch))
    af.duration     = SUSTAIN_WEAVE;
  else
    af.duration     = ch->level / 10;
  af.location     = APPLY_NONE;
  if (IS_NPC(ch)) {
    af.modifier = UMAX(1, (((ch->perm_sphere[SPHERE_SPIRIT]/2) + (ch->level/5) + (ch->holding/10)) * shield_strength_mod));
  }
  else {
    if (ch->main_sphere == SPHERE_SPIRIT) {
    
    af.modifier = UMAX(1, (((ch->perm_sphere[SPHERE_SPIRIT]/2) + (get_skill(ch,sn)/5) + (ch->holding/10) + 20) * shield_strength_mod));
    }
    else {
    	
    af.modifier = UMAX(1, (((ch->perm_sphere[SPHERE_SPIRIT]/2) + (get_skill(ch,sn)/5) + (ch->holding/10)) * shield_strength_mod));
    
    }
  }
  af.bitvector    = AFF_SHIELDED;
  affect_to_char(victim, &af );
  
  if (victim->class == CLASS_CHANNELER) { 
    send_to_char("{yYou feel as if you have lost touch with something.{x\n\r", victim );
  }
  
  act("You shield $N from the True Source.", ch, NULL, victim, TO_CHAR);
  
  if (IS_AFFECTED(victim,AFF_CHANNELING)) {
    REMOVE_BIT(victim->affected_by, AFF_CHANNELING);
    victim->holding = 0;
    release_sustained_weaves(victim);
  }
  
  return;
}

/**********************************************************************
*       Function      : spell_shimmer
*       Author        : Swordfish
*       Description   : Undiscovered traveling weave used by Ishamael
*                       Granted only - Chosen Ishamael should get this
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_shimmer( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA       *victim=NULL;
  ROOM_INDEX_DATA *tlocation;   // To location

  char toRoomKey[MAX_STRING_LENGTH];

  /* Validate syntax */
  if (IS_NULLSTR(cast_arg2)) {
    send_to_char("Syntax: weave shimmer <to room-key>\n\r", ch);
    send_to_char("        weave shimmer <to player>\n\r", ch);
    return;
  }

  /* Check if shimmer to player */
  if ((victim = get_char_world(ch, target_name)) != NULL) {

    if (victim == ch) {
	 send_to_char("You can't shimmer to your self.\n\r", ch);
	 return;
    }

    if (victim->in_room == ch->in_room) {
	 send_to_char("You can't shimmer there right now.\n\r", ch);
	 return;
    }

    if (!IS_AFFECTED(ch, AFF_INVISIBLE))
       act("The air shimmers around $n and $e vanishes.", ch, NULL, NULL, TO_ROOM);
    send_to_char("\n{yYou travel through a shimmer of a dream.{x\n\n\r", ch);
    
    char_from_room(ch);
    char_to_room(ch,victim->in_room);
    
    if (!IS_AFFECTED(ch, AFF_INVISIBLE))
       act("The air shimmers for a moment of time bending $n into existance.", ch, NULL, NULL, TO_ROOM);
    do_function(ch, &do_look, "auto");
    return;
  }

  /* If not to player, shimmer to room */
  

  char *key = getkey(ch,cast_arg2);
  if (key != NULL)
  {
        sprintf(toRoomKey, "%d", key2vnum(ch, key));
  }
  else
  {
        sprintf(toRoomKey, "%d", key2vnum(ch, cast_arg2));
  }
  if ((tlocation = find_location(ch, toRoomKey)) == NULL) {
    send_to_char("No such location.\n\r", ch);
    return;
  }

  if (tlocation == ch->in_room) {
    send_to_char("You are standing right here.\n\t", ch);
    return;
  }

  if (!IS_AFFECTED(ch, AFF_INVISIBLE))
     act("The air shimmers around $n and $e vanishes.", ch, NULL, NULL, TO_ROOM);
  send_to_char("\n{yYou travel through a shimmer of a dream.{x\n\n\r", ch);
  
  char_from_room(ch);
  char_to_room(ch,tlocation);
  
  if (!IS_AFFECTED(ch, AFF_INVISIBLE))
     act("The air shimmers for a moment of time bending $n into existance.", ch, NULL, NULL, TO_ROOM);
  do_function(ch, &do_look, "auto");

  return; 
}

/**********************************************************************
*       Function      : spell_gate
*       Author        : Swordfish
*       Description   : Open a doorway from the room the player are in
*                       to another room in the world
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_gate( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA       *victim=NULL; // Victim if any at to location
  ROOM_INDEX_DATA *flocation;   // From location
  ROOM_INDEX_DATA *tlocation;   // To location
  OBJ_DATA        *fgate;       // From gate
  OBJ_DATA        *tgate;       // To gate
  char key1[MAX_STRING_LENGTH];
  char key2[MAX_STRING_LENGTH];
  
  /* Validate syntax */
  if (IS_NULLSTR(cast_arg2) || IS_NULLSTR(cast_arg3)) {
    send_to_char("Syntax: weave gate <from key> <to key>\n\r", ch);
    return;
  }

  /* Does from location exist, and does the player know it well enogh? */
  //First see if it's a key_alias or a key

  char *key = getkey(ch,cast_arg2);
  if (key != NULL)
  {
	sprintf(key1, "%d", key2vnum(ch, key));
  }
  else
  {
  	sprintf(key1, "%d", key2vnum(ch, cast_arg2));
  }
  if (( flocation = find_location(ch, key1)) == NULL) {
    send_to_char("No such location.\n\r", ch);
    return;
  }
  
  /* Is the from location the room we are standing in? */
  if (flocation != ch->in_room) {
    send_to_char("You must open a gate from where you are.\n\r", ch);
    return;
  }
  
  /* Does to location exists, and does the player know it well enough? */
  key = getkey(ch,cast_arg3);
  if (key != NULL) 
  {
 	 sprintf(key2, "%d", key2vnum(ch, key));
  }
  else
  {
 	 sprintf(key2, "%d", key2vnum(ch, cast_arg3));
  }
  if (( tlocation = find_location(ch, key2)) == NULL) {
    send_to_char("No such location.\n\r", ch);
    return;
  }

  if (flocation == tlocation) {
    send_to_char("You can't open a gateway to this room.\n\t", ch);
    return;
  }

  if (!is_gate_room(ch, tlocation)) {
    send_to_char("You can't open a gateway to this room.\n\t", ch);
    return;
  }
  
    //TRAVELING talent?
    if (!IS_SET(ch->talents, TALENT_TRAVELING)) {
       if (number_percent() > get_skill(ch,sn) / 5)
       {
	  send_to_char("You try hard, but it just doesn't work.\n\r",ch);
	  return;
       }
    }
  
  /* Open gate */
  send_to_char("A vertical slash of light appears and a gateway seems to turn into existence.\n\r",ch);
  act("A vertical slash of light appears and a gateway seems to turn into existence.", ch,NULL,NULL,TO_ROOM);

  /* If to location have players, get the first one */
  victim = tlocation->people;

  /* Don't write message unless players in to-room */
  if (victim != NULL) {
    send_to_char("A vertical slash of light appears and a gateway seems to turn into existence.\n\r",victim);
    act("A vertical slash of light appears and a gateway seems to turn into existence.",
	   victim,NULL,NULL,TO_ROOM);
  }

  /* Create the objects and set them up */
  fgate = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
  fgate->contains=NULL;	
  
  tgate = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
  tgate->contains=NULL;
  
  fgate->value[3] = tlocation->vnum;
  tgate->value[3] = ch->in_room->vnum;
  fgate->level = get_level(ch);
  fgate->timer = (ch->level/10);
  tgate->level = get_level(ch);
  tgate->timer = (ch->level/10);
  
  obj_to_room( fgate, ch->in_room);
  obj_to_room( tgate, tlocation);
  return;
}

/**********************************************************************
*       Function      : spell_dreamgate
*       Author        : Swordfish
*       Description   : Open a doorway to T'A'R to enter in FLESH!
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_dreamgate( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  
  OBJ_DATA *dreamgate;

  /* If dreaming and channie, can't cast dreamgate */
  /* to prevent messing up flags */
  if (IS_DREAMING(ch)) {
    send_to_char("You can't manage to weave the flows for this while dreaming.\n\r", ch);
    return;
  }

  if (!is_gate_room(ch, ch->in_room)) {
    send_to_char("There is something that prevent the flows to form into a dreamgate here.\n\t", ch);
    return;
  }
  
    //TRAVELING talent?
    if (!IS_SET(ch->talents, TALENT_TRAVELING)) {
       if (number_percent() > get_skill(ch,sn) / 10)
       {
	  send_to_char("You try hard, but it just doesn't work.\n\r",ch);
	  return;
       }
    }
  
  /* Open gate */
  send_to_char("A vertical slash of light appears and a gateway seems to turn into existence.\n\r",ch);
  act_old("A vertical slash of light appears and a gateway seems to turn into existence.", ch,NULL,NULL,TO_ROOM, POS_RESTING);

  dreamgate = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
  dreamgate->contains=NULL;
  
  dreamgate->value[3] = ch->in_room->vnum;
  
  dreamgate->level = ch->level;
  dreamgate->timer = (ch->level/10);
  dreamgate->value[2] = GATE_DREAMGATE;
  
  obj_to_room( dreamgate, ch->in_room);
  return;
}

/**********************************************************************
*       Function      : spell_skimming
*       Author        : Swordfish
*       Description   : Open a doorway from the room the player are in
*                       to skimming world
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_skimming( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA       *victim=NULL; // Victim if any at to location
  ROOM_INDEX_DATA *flocation;   // From location
  ROOM_INDEX_DATA *tlocation;   // To location
  OBJ_DATA        *fgate;       // From gate
  OBJ_DATA        *tgate;       // To gate
  int followers=0;

  /* If dreaming and channie, can't cast dreamgate */
  /* to prevent messing up flags */
  if (IS_DREAMING(ch)) {
    send_to_char("You can't manage to weave the flows for this while dreaming.\n\r", ch);
    return;
  }

  // We know from/to
  flocation = ch->in_room;
  if (!IS_SKIMMING(ch))
    tlocation = get_room_index( ROOM_VNUM_SKIMMING );
  else
    tlocation = get_room_index( ch->skim_to );
  
  // Open gate
  send_to_char("A vertical slash of light appears and a gateway seems to turn into existence.\n\r",ch);
  act("A vertical slash of light appears and a gateway seems to turn into existence.", ch,NULL,NULL,TO_ROOM);
  
  // If to location have players, get the first one
  victim = tlocation->people;

  // Don't write message unless players in to-room
  if (victim != NULL) {
    send_to_char("A vertical slash of light appears and a gateway seems to turn into existence.\n\r",victim);
    act("A vertical slash of light appears and a gateway seems to turn into existence.", victim,NULL,NULL,TO_ROOM);
  }
  
  // Create the objects and set them up
  fgate = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
  fgate->contains=NULL;	
  
  tgate = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
  tgate->contains=NULL;
  
  if (IS_NPC(ch))
    followers = 1;
  else
     followers = get_skill(ch,sn)/15;
  
  if (followers < 1)
     followers = 1;
  
  fgate->value[0] = followers;
  tgate->value[0] = followers;
  
  if (!IS_SKIMMING(ch)) {
    fgate->value[3] = tlocation->vnum;
    tgate->value[3] = ch->in_room->vnum;
    
    fgate->value[2] = GATE_SKIMMING_IN;
    tgate->value[2] = GATE_SKIMMING_IN;
    ch->skim_to = ch->in_room->vnum;    // Default if you want to get out of skim
  }
  else {
    fgate->value[3] = ch->skim_to;
    tgate->value[3] = ch->in_room->vnum;
    
    fgate->value[2] = GATE_SKIMMING_OUT;
    tgate->value[2] = GATE_SKIMMING_OUT;
  }

  fgate->level = get_level(ch);
  fgate->timer = (ch->level/10);
  tgate->level = get_level(ch);
  tgate->timer = (ch->level/10);
  
  obj_to_room( fgate, ch->in_room);
  obj_to_room( tgate, tlocation);

  return;
}

void do_slice(CHAR_DATA *ch, char *argument)
{
	char buffer[MAX_STRING_LENGTH];
	sprintf(buffer,"slice %s",argument);
	do_function(ch, &do_cast, buffer);
}

/**********************************************************************
*       Function      : do_skim
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_skim(CHAR_DATA *ch, char *argument)
{
  ROOM_INDEX_DATA *location;
  char buf[MSL];
  CHAR_DATA *vch=NULL;
  CHAR_DATA *vch_next=NULL;
  int sn;
  int learned=0;
  int pulse=0;
    
  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("You have no idea about channeling.\n\r", ch);
    return;
  }
  
  if (!IS_SKIMMING(ch)) {
    send_to_char("You need to be inside the {Ddark void{x to be able to skim.\n\r", ch);
    return;
  }
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: skim <city>\n\r", ch);    
    return;
  }
  
  sn = skill_lookup("skimming");

  if (IS_NPC(ch))
    learned = ch->level;
  else
    learned = get_skill(ch,sn);
  
  if (learned < 1) {
    send_to_char("You don't even know how to skim!\n\r",ch); 
    return;
  }

  if (!can_handle_weave(ch, sn)) {
    send_to_char("You are not yet strong enough to focus your will on skimming.\n\r", ch);
    return;
  }
  

  if (!IS_SET(ch->talents, TALENT_TRAVELING))
  { 
     if (number_percent() > learned / 2) {
       send_to_char("You try to focus your will but feel lost inside this place!\n\r", ch);
       return;
     }
  }

  if ((location = find_city( ch, argument )) == NULL) {
    sprintf(buf, "You focus your will toward %s, but it feels like the {Ddark void{x get more tense!\n\r", argument);
    send_to_char(buf, ch);
    return;
  }
  
  sprintf(buf, "You focus your will toward %s and feel the glistening silvery platform you stand on suddenly start to gain motion!\n\r", location->name);
  send_to_char(buf, ch);
  
  pulse = number_range(0, 2);

  ch->skim        = TRUE;
  ch->skim_to     = location->vnum;
  ch->skim_motion = TRUE;
  ch->skim_pulse  = pulse;
  
  // Followers
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next = vch->next;
    if (ch == vch)
	 continue;
    if (!IS_SAME_WORLD(vch, ch))
	 continue;
    if (!is_same_group(ch, vch))
	 continue;
    if (!IS_SKIMMING(vch))
	 continue;
    vch->skim_motion = TRUE;
    send_to_char("You suddenly feel as if the glistening silvery platform you stand on is gaining motion!\n\r", vch);
  }
    
  return;
}

/**********************************************************************
*       Function      : do_dream
*       Author        : Swordfish
*       Description   : Enter the T'A'R if sleeping and have dreaming
*                       talent.
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_dream(CHAR_DATA *ch, char *argument)
{
  int sn;
  int endurance;
  int div_base=2;

  /* Only PC for now */
  if (IS_NPC(ch))
    return;

  /* Already dreaming? */
  if (IS_DREAMING(ch)) {
    if (IS_WOLFKIN(ch))
       send_to_char("You are already in a wolf dream.\n\r", ch);
    else
       send_to_char("You are already in the dream world.\n\r", ch);
    return;
  }

  if (IS_WOLFKIN(ch))
    sn = skill_lookup("wolfdream");
  else
    sn = skill_lookup("dreaming");

  if (get_skill(ch,sn) < 1) {
    if (IS_WOLFKIN(ch))
	 send_to_char("You don't even know how to enter the wolf dream.\n\r", ch);
    else
	 sn = find_spell(ch,"dreamgate");
	 if (get_skill(ch,sn) < 150)
	 {
	 	send_to_char("You don't even know how to enter the dream world.\n\r", ch);
    		return;
	 }
  }

  /* Have talent ? */
  /*
  if (!IS_SET( ch->talents, skill_table[sn].talent_req)) {
    if (IS_WOLFKIN(ch))
	 send_to_char("You don't know how to enter the wolf dream.\n\r", ch);
    else
	 send_to_char("You don't know how to enter the dream world.\n\r", ch);
    return;
  }
  */

  /* Is sleeping? */
  if (ch->position != POS_SLEEPING) {
    if (IS_WOLFKIN(ch))
	 send_to_char("You need to be sleeping to enter the wolf dream.\n\r", ch);
    else
	 send_to_char("You need to be sleeping to enter the dream world.\n\r", ch);
    return;
  }

  /* Wolfkin in TAR */
  if (IS_WOLFKIN(ch)) {
    if (IS_NULLSTR(ch->pcdata->wolf_appearance)) {
	 send_to_char("You need to set an wolf apperance before entering the wolf dream.\n\r", ch);
	 send_to_char("See 'help wolfapperance' for details.\n\r", ch);
	 return;
    }
  }
  
  endurance = skill_table[sn].min_endurance;
  
  if (ch->endurance < endurance) {
    send_to_char("You are too tired to concentrate any more!\n\r", ch);
    return;	
  }

  /* For WK, you need to be skilled to get into TAR */
  if (IS_WOLFKIN(ch))
    div_base = 5;
  
  if (number_percent() > (get_skill(ch,sn)/div_base)) {
    if (IS_WOLFKIN(ch))
	 send_to_char("You try to enter the wolf dream, but fail!\n\r", ch);
    else
	 send_to_char("You try to enter the dream world, but fail!\n\r", ch);
    check_improve(ch,sn,FALSE,1);
    ch->endurance -= endurance/2;
    return;
  }  
    
  /* Ok, enter world */  
  TOGGLE_BIT(ch->world, WORLD_NORMAL);
  TOGGLE_BIT(ch->world, WORLD_TAR_DREAM);
  ch->world_vnum = ch->in_room->vnum;
  
  if (IS_WOLFKIN(ch)) {
    /* Switch to wolfshape */
    do_wolfshape(ch, "");
    
    /* Make sure you are alone */
    die_follower( ch );
    
    /* No cloak or hood allowed for WK */
    if (IS_CLOAKED(ch))
	 REMOVE_BIT(ch->app, APP_CLOAKED);
    if (IS_COLORCLOAKED(ch))
	 REMOVE_BIT(ch->app, APP_COLORCLOAKED);
    if (IS_HOODED(ch))
	 REMOVE_BIT(ch->app, APP_HOODED);
    if (IS_VEILED(ch))
	 REMOVE_BIT(ch->app, APP_VEILED);	 
    
    send_to_char("{rYou enter the dream world in {Ywolf{r form!{x\n\r", ch);
  }
  else {
    send_to_char("{rYou enter the dream world!{x\n\r", ch);
  }

  check_improve(ch,sn,TRUE,1);

  act("$n fades into view.", ch,NULL,NULL,TO_ROOM);
  
  ch->position = POS_STANDING;
  do_function(ch, &do_look, "auto");
  
  ch->endurance -= endurance;
  WAIT_STATE(ch,skill_table[sn].beats);
  return;
}

/**********************************************************************
*       Function      : do_dreamgrab
*       Author        : Zandor
*       Description   : Pull someone else into T'A'R
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_dreamgrab(CHAR_DATA *ch, char *argument)
{
  int sn, dgsn;
  int endurance;
  CHAR_DATA *victim;
  char arg[MAX_STRING_LENGTH];
  argument = one_argument (argument, arg);

  sn = skill_lookup("dreaming");
  dgsn = skill_lookup("dreamgate");

  if (get_skill(ch,sn) < 1 && get_skill(ch,dgsn)< 1)  {
    send_to_char("You don't even know how to dream!\n\r",ch); 
    return;
  }

  if (get_skill(ch,sn) < 200)  {
    send_to_char("You don't have the skill for this!\n\r", ch);
    return;
  }

  if (arg == NULL || arg[0] == '\0')
  {
	send_to_char("Grab Who?\r\n",ch);
	return;
  }

  if ( ( victim = get_realname_char_world( ch, arg ) ) == NULL ) {
    send_to_char("They aren't available!\n\r", ch);
    return;
  }

  if ( !IS_SLEEPING(victim) && !IS_DREAMING(victim)) {
    send_to_char("They aren't asleep!\n\r", ch); 
    return;
  }

  /* Only PC for now */
  if (IS_NPC(victim))
    return;

  if (victim == ch)
  {
       send_to_char("They have another word for that you know!\n\r", ch); 
       return;
  }
  if (!IS_DREAMING(ch) && !IS_DREAMWALKING(ch))
   {
       send_to_char("You aren't asleep!\n\r", ch); 
       return;
   }
  
  /* Already dreaming? */
  // Allow forsaken to grab someone already in T'A'R
  if ((IS_DREAMING(victim)) && 
      (get_skill(ch,sn) < 150 && ch->pcdata->learned[dgsn] < 175))  {
    send_to_char("They are already in the dream world.\n\r", ch);
    return;
  }

  /* Is sleeping? */
  if (victim->position != POS_SLEEPING && !IS_DREAMING(victim) ) {
    send_to_char("They need to be sleeping\n\r", ch);
    return;
  }

  if (IS_AFFECTED(victim, AFF_DREAMWARD)) {
    send_to_char("There seems to be a shield protecting their dreams!\n\r", ch);
    send_to_char("You feel a preasure toward the shield protecting your dreams!\n\r", victim);
    return;
  }

  endurance = skill_table[sn].min_endurance;
  
  if (ch->endurance < endurance) {
    send_to_char("You are too tired to concentrate any more!\n\r", ch);
    return;	
  }

  /* Ok, enter world */  
  TOGGLE_BIT(victim->world, WORLD_NORMAL);
  TOGGLE_BIT(victim->world, WORLD_TAR_DREAM);
  victim->world_vnum = victim->in_room->vnum;

  char_from_room(victim);
  char_to_room(victim,ch->in_room);
  
  send_to_char("{rThey are pulled into the dream world!{x\n\r", ch);
  send_to_char("{rYou are pulled into the dream world!{x\n\r", victim);
  
  check_improve(ch,sn,TRUE,1);
  
  act("$n fades into view.", victim,NULL,NULL,TO_ROOM);
  
  victim->position = POS_STANDING;
  do_function(victim, &do_look, "auto");
  
  ch->endurance -= endurance;
  WAIT_STATE(ch,skill_table[sn].beats);
  return;
}

void do_dreamgoto( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    CHAR_DATA *victim=NULL;
    int count = 0;
    int sn, dgsn, wgsn;
    bool inobj = FALSE;

    if (IS_NPC(ch))
	return;

    if (!IS_DREAMING(ch) && !IS_DREAMWALKING(ch)) {
    	send_to_char("You aren't asleep!!!\n\r",ch);
	return; 
    }

  sn = skill_lookup("dreaming");
  dgsn = skill_lookup("dreamgate");
  wgsn = skill_lookup("wolfdream");
  if (get_skill(ch,sn) < 1 && ch->pcdata->learned[dgsn] < 1 && ch->pcdata->learned[wgsn] < 1) 
  {
 	send_to_char("You don't even know how to dream!\n\r",ch); 
       	return;
  }
 
  if (!IS_SET(ch->talents,TALENT_DREAMING) && !IS_SET(ch->talents,TALENT_TRAVELING))
  {
    send_to_char("You don't have the skill for this!\n\r", ch);
    return;
  }
  
  if (get_skill(ch,sn) < 200 && ch->pcdata->learned[dgsn] < 200 && ch->pcdata->learned[wgsn] < 200) 
  {
    send_to_char("You don't have the skill for this!\n\r", ch);
    return;
  }
  if ( argument[0] == '\0' )
  {
      send_to_char( "Goto where?\n\r", ch );
      return;
  }

    if (!IS_IMMORTAL(ch) && is_number(argument))
    {
       send_to_char( "No such location.\n\r", ch );
       return;
    }

    if ( ( location = find_location( ch, argument ) ) == NULL )
    {
       send_to_char( "No such location.\n\r", ch );
       return;
    }

   if ((location != NULL)
       && ((victim = get_char_same_world(ch, argument)) != NULL)
       && (victim->in_obj != NULL))
      inobj = TRUE;

    if ((victim != NULL) && (victim->world != ch->world)) {
       send_to_char( "No such location.\n\r", ch );
       return;
    }

    count = 0;
    for ( rch = location->people; rch != NULL; rch = rch->next_in_room )
        count++;


    if (!is_room_owner(ch,location) && room_is_private(location) 
    &&  (count > 1 || get_trust(ch) < MAX_LEVEL))
    {
   send_to_char( "That room is private right now.\n\r", ch );
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
      send_to_char("Goto failed to get you inside the object!\n\r", ch);


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

/**********************************************************************
*       Function      : do_seal
*       Author        : Swordfish
*       Description   : Seal a open gateway
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_seal(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA       *victim=NULL;
  ROOM_INDEX_DATA *tlocation;   // To location
  OBJ_DATA        *fgate;       // From gate
  OBJ_DATA        *tgate;       // To gate
  int              sn=0;

  /* None channelers can't do anything with a open gate apart from enter */
  if (ch->class != CLASS_CHANNELER) {
    send_to_char("You don't know how that got here.\n\r", ch);
    return;
  }

  /* Seal what gate ? */
  if ( (fgate  = get_obj_here(ch, argument)) == NULL) {
    send_to_char("Seal what?\n\r", ch);
    return;
  }

  if (fgate->item_type != ITEM_PORTAL) {
    do_function(ch, &do_sealletter, argument);
    return;
  }

  /* Do he/she know gate at all? If not, they can't do shit */
  // If dreamget
  if (IS_SET(fgate->value[2],GATE_DREAMGATE)) {
    sn = skill_lookup("dreamgate");
    if (!IS_NPC(ch) && get_skill(ch,sn) < 1) {
    sn = skill_lookup("dreamgate");
	 send_to_char("You don't even know how to make one.\n\r", ch);
	 return;
    }    
  }
  else {  
    sn = skill_lookup("gate");
    if (!IS_NPC(ch) && get_skill(ch,sn) < 1) {
         sn = skill_lookup("skimming");
	 if (!IS_NPC(ch) && get_skill(ch,sn) < 1) {	
	    send_to_char("You don't even know how to make one.\n\r", ch);
	    return;
	 }
    }
  }

  /* Needs to channel to seal a gate */
  if (!IS_AFFECTED(ch, AFF_CHANNELING)) {
    do_function(ch, &do_channel, "");
    if (!IS_AFFECTED(ch ,AFF_CHANNELING))
	 return;
  }
  
  /* Only if higher level currently. Might want to change to strenght */
  if (fgate->level > get_level(ch)) {
    send_to_char("You examine the gateway but are not able to seal it.\n\r", ch);
    return;
  }
  
  send_to_char("You examine the flows combined into the gateway and seal it.\n\r", ch);
  
  sprintf(buf, "%d", fgate->value[3]);

  if (!IS_SET(fgate->value[2], GATE_DREAMGATE)) {  
    if ((tlocation = find_location(ch, buf)) != NULL) {
	 if ((tgate = get_obj_list(ch, "gate", tlocation->contents)) != NULL) {
	   if (tgate->value[3] == ch->in_room->vnum) {
		victim = tlocation->people;
		if (victim != NULL) {
		  act("$p winks out of existance.", victim, tgate, NULL, TO_CHAR);
		  act("$p winks out of existance.", victim, tgate, NULL, TO_ROOM);
		}
		extract_obj(tgate);
	   }
	 }
    }
  }

  act("$p winks out of existance.", ch, fgate, NULL, TO_CHAR);
  act_old("$p winks out of existance.", ch, fgate, NULL, TO_ROOM, POS_RESTING);
  extract_obj(fgate);
  
  return;
}

// Return the total modification to the given sphere
// E.g. affects like angreals
int get_sphere_mod(CHAR_DATA *ch, int sphere)
{
   int iWear;
   int mod=0;
   OBJ_DATA *obj=NULL;
   AFFECT_DATA *paf=NULL;
   sh_int location;
   
   // Set apply location for the spheres
   switch (sphere) {
      case SPHERE_AIR:
         location=APPLY_SPHERE_AIR;
         break;
      case SPHERE_EARTH:
         location=APPLY_SPHERE_EARTH;
         break;
      case SPHERE_FIRE:
         location=APPLY_SPHERE_FIRE;
         break;
      case SPHERE_SPIRIT:
         location=APPLY_SPHERE_SPIRIT;
         break;
      case SPHERE_WATER:
         location=APPLY_SPHERE_WATER;
         break;
      default:
         return 0;
         break;
    }
   
   // Check equip, see if Angreal and if location match
   for (iWear = 0; iWear < MAX_WEAR; iWear++) {
      if ((obj = get_eq_char(ch, iWear)) == NULL)
         continue;
      if (obj->item_type == ITEM_ANGREAL) {
      	for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next ) {
      	   if ( paf->location == location)
      	      mod += paf->modifier;
      	}
      	for ( paf = obj->affected; paf != NULL; paf = paf->next ) {
      	   if ( paf->location == location)
      	      mod += paf->modifier;
      	}
      }
   }   
   return mod;
}

/**********************************************************************
*       Function      : spell_still
*       Author        : Swordfish
*       Description   : Still a channeler and remove his/her ability to
*                       touch the source for ever
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_still( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  int i=0;

  if (IS_NPC(victim)) {
    send_to_char("You can't still mobiles.\n\r", ch);
    return;
  }
  
  // Not for NPC now
  if (IS_NPC(ch)) 
    return;
  
  if (victim->class != CLASS_CHANNELER) {
    send_to_char("There's nothing to still in your victim.\n\r",ch);
    return;
  }

  if (victim->pcdata->learned[skill_lookup(victim->sex == SEX_MALE ? "seize" : "embrace")] < 1) {
    send_to_char("There's nothing to still in your victim.\n\r",ch);
    return;
  }
  
    
  if (IS_SET(victim->act,PLR_STILLED)) {
    send_to_char("Your victim is already stilled.\n\r",ch);
    return;
  }
  
/*
  if ( saves_spell( level, victim, DAM_STILL) && victim->class == CLASS_CHANNELER) {
    act("$N shivers slightly, but it passes quickly.", ch, NULL, victim, TO_CHAR );
    send_to_char("You shiver slightly, but it passes quickly.\n\r",victim);
    return;
  }
*/

  if (IS_AFFECTED(victim,AFF_CHANNELING)) {
    REMOVE_BIT(victim->affected_by, AFF_CHANNELING);
    victim->holding = 0;
    release_sustained_weaves(victim);
  }

  SET_BIT(victim->act,PLR_STILLED);
  
  for(i = 0; i < MAX_SPHERE; i++) {
    if (victim->perm_sphere[i] > victim->cre_sphere[i]) {    	
	 victim->cre_sphere[i] = victim->perm_sphere[i] - get_sphere_mod(victim,i);
    }
    victim->perm_sphere[i] = 0;
  }
  
  send_to_char("{rYou have been cut off from the True Source!{x\n\r",victim);
  
  act("You fall to the ground screaming in denial.", victim,NULL,NULL,TO_CHAR);
  act("$n falls to the ground screaming in denial.", victim,NULL,NULL,TO_ROOM);
  return;
}

/**********************************************************************
*       Function      : spell_heal_still
*       Author        : Swordfish
*       Description   : Heal a stilled player and return their ability
*                       to channel once again
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_heal_still( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  int i=0;
  int iWear;
  OBJ_DATA *obj=NULL;
  AFFECT_DATA *paf=NULL;

  /* NPC's can't use this */
  if (IS_NPC(ch)) {
    return;
  }
   
  if (!IS_SET(victim->act,PLR_STILLED)) {
    send_to_char("You probe your victim finding that they are not cut off.", ch);
    return;
  }

  if (IS_NPC(victim)) {
    send_to_char("Mobiles cannot be healed of the Stilling.\n\r",ch);
    return;
  } 

  if (ch->class != CLASS_CHANNELER) {
    send_to_char("You can't seem to find out what's wrong with them.\n\r", ch);
    return;
  }

  if (number_percent() > get_skill(ch,sn)) {
    send_to_char("You can't seem to find out what's wrong with them.\n\r", ch);
    return;
  }


  if (number_percent() > 60 && !IS_IMMORTAL(victim)) {
    for(i = 0; i < MAX_SPHERE; i++) {
    	 if (victim->cre_sphere[i] > 30)
	    victim->cre_sphere[i] *= .8;
	 victim->perm_sphere[i] = victim->cre_sphere[i];
    }
    send_to_char("{WYou feel the True Source again, only it is fainter now.{x\n\r", victim);
  }
  else if (number_percent() < 5 && !IS_IMMORTAL(victim)) {
    for(i = 0; i < MAX_SPHERE; i++) {
	 victim->cre_sphere[i] *= 1.3;
	 victim->perm_sphere[i] = victim->cre_sphere[i];
    }
    send_to_char("{WYou feel the True Source again stronger than ever!{x\n\r", victim);
  }
  else {
    for(i = 0; i < MAX_SPHERE; i++) {
	 victim->perm_sphere[i] = victim->cre_sphere[i];
    }
    send_to_char("{WYou feel the glow of the True Source again!{x\n\r",victim);
  }
  
  REMOVE_BIT(victim->act,PLR_STILLED);
  send_to_char("You remove their inability to channel.\n\r",ch);
  
  // If wearing angreals, re-activate em
   for (iWear = 0; iWear < MAX_WEAR; iWear++) {
      if ((obj = get_eq_char(victim, iWear)) == NULL)
         continue;
      if (obj->item_type == ITEM_ANGREAL) {
      	for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next ) {
      	   if (paf->location == APPLY_SPHERE_AIR ||
      	       paf->location == APPLY_SPHERE_EARTH ||
      	       paf->location == APPLY_SPHERE_FIRE ||
      	       paf->location == APPLY_SPHERE_SPIRIT ||
      	       paf->location == APPLY_SPHERE_WATER)
      	      affect_modify(victim, paf, TRUE);
      	}
      	for ( paf = obj->affected; paf != NULL; paf = paf->next ) {
      	   if (paf->location == APPLY_SPHERE_AIR ||
      	       paf->location == APPLY_SPHERE_EARTH ||
      	       paf->location == APPLY_SPHERE_FIRE ||
      	       paf->location == APPLY_SPHERE_SPIRIT ||
      	       paf->location == APPLY_SPHERE_WATER)
      	      affect_modify(victim, paf, TRUE);
      	}
      }
   }  
}

/**********************************************************************
*       Function      : spell_blade
*       Author        : Swordfish
*       Description   : Forge a blade with the OP.
*                       Blade can be forged from all sphere base
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_blade( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  OBJ_DATA *wielded;
  OBJ_DATA *blade;
  AFFECT_DATA af;
  
  wielded = get_eq_char(ch, WEAR_WIELD);
  
  if (wielded != NULL) {
    send_to_char("You start to combine the flows but realize that you already wield a weapon.\n\r", ch);
    return;
  }

  af.where	   = TO_AFFECTS;
  af.casterId     = ch->id;
  af.type         = sn;
  af.level        = level;
  af.duration     = SUSTAIN_WEAVE;
  af.location     = APPLY_NONE;

  af.modifier     = ch->holding/20;
  if (af.modifier > 40)
    af.modifier = 40;
  af.bitvector    = 0;
  
  blade = create_object(get_obj_index(OBJ_VNUM_BOF), 0);
  blade->level    = af.level;
  blade->value[0] = WEAPON_SWORD;
  blade->value[1] = af.modifier;
  blade->value[2] = af.modifier/2;
  blade->timer    = 0;

  if (IS_IMMORTAL(ch)) {
    if (!IS_NULLSTR(cast_arg2)) {
	 if ( strstr(colorstrem(cast_arg2), "blade") == NULL || 
		 strstr(colorstrem(cast_arg2), "fire") == NULL) {
	   send_to_char("You must include the keywords 'blade' and 'fire'\n\r", ch);
	   return;
	 }
	 else {
	   free_string(blade->short_descr);
	   blade->short_descr = str_dup(cast_arg2);
	 }
    }
  }

  affect_to_char( ch, &af );

  obj_to_char(blade, ch);
  equip_char(ch, blade, WEAR_WIELD);
  
  act("$p erupts from your hand.", ch, blade, NULL, TO_CHAR);
  act("$p erupts from $n hand.",ch,blade,NULL,TO_ROOM);
  
  return;
}

/**********************************************************************
*       Function      : spell_toke
*       Author        : Swordfish
*       Description   : Set a token weave on a object
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_token( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  OBJ_DATA  *obj;
  int        idkey=0;
  
  /* Validate syntax */
  if (IS_NULLSTR(cast_arg2)) {
    send_to_char("Syntax: weave token <id-key number>\n\r", ch);
    return;
  }
  
  if (!( idkey = atoi(cast_arg2))) {
    send_to_char("Syntax: weave token <id-key number>\n\r", ch);
    return;
  }

  obj = create_object(get_obj_index(OBJ_VNUM_TOKEN), 0);
  obj->value[0] = idkey;  // Unique number key to this token
  obj->value[1] = ch->id; // Owner
  obj_to_char(obj,ch);
  
  act("You set a token weave on $p.", ch, obj, NULL, TO_CHAR);
  
  return;
}

/**********************************************************************
*       Function      : do_token
*       Author        : Swordfish
*       Description   : Sense the token
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_token(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA  *obj;
  OBJ_DATA  *container_obj;
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  int idkey=0;
  int sn=0;
  int found=FALSE;
  
  if (ch->class != CLASS_CHANNELER) {
    send_to_char("You have no idea what a token is.\n\r", ch);
    return;
  }

  // Not for NPCs now
  if (IS_NPC(ch))
    return;
  
  sn = skill_lookup("token");
  
  if (!IS_NPC(ch) && get_skill(ch,sn) < 1) {
    send_to_char("You don't even know how to make a token weave.\n\r", ch);
    return;
  }
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Syntax: token <id-key>\n\r", ch);
    return;
  }
  
  idkey = atoi(argument);
  
  /* Spin through all players on-line and see if they have */
  /* the token in their inv. - if not, can't feel it       */
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next = vch->next;
    
    if (IS_NPC(vch))
      continue;
    
    for (obj = vch->carrying; obj != NULL; obj = obj->next_content) {
     if (obj->item_type == ITEM_CONTAINER) {
      for (container_obj = obj->contains; container_obj != NULL; container_obj = container_obj->next_content) {
         if (container_obj->item_type != ITEM_TOKEN)
            continue;
         if (container_obj->value[0] == idkey && container_obj->value[1] == ch->id) {
            do_bondfind(ch, vch->name);
            found=TRUE;
         }
      }     
     }
     else {    
	     if (obj->item_type != ITEM_TOKEN)
	        continue;
	     if (obj->value[0] == idkey && obj->value[1] == ch->id) {
	        do_bondfind(ch, vch->name);
	        found=TRUE;
	     }
      }
    }
  }
   
  if (found)
    return;
   
  send_to_char("You can't feel the precense of this token right now.\n\rPerhaps it is lost?\n\r", ch);
  return;
}

/**********************************************************************
*       Function      : spell_grow_berries
*       Author        : Swordfish
*       Description   : Use OP to get a shrub to grow faster
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_grow_berries( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *berry=NULL;
  static char * const bname [] = {"red", "green", "yellow", "blue"};
  static char * const bcol  [] = { "{Rred{x", "{ggreen{x", "{Yyellow{x", "{Bblue{x"};
  int num=0;
  int op_bonus=0;

  /* Small chance it all fails */
  if (number_chance(2)) {
    act( "a shrub starts to tremble then wither slowly.", ch, berry, NULL, TO_ROOM );
    act( "a shrub starts to tremble then wither slowly.", ch, berry, NULL, TO_CHAR );
    return;
  }

  /* Created object */
  berry = create_object( get_obj_index( OBJ_VNUM_BERRY ), 0 );

  /* Pick a number, random */
  num = number_range(0,3);
  
  /* Fix short description */
  sprintf(buf, "a %s berry", bcol[num]);
  free_string(berry->short_descr);
  berry->short_descr = str_dup(buf);
  
  /* Fix long description */
  sprintf(buf, "A %s berry is laying beneath a shrub.", bcol[num]);
  free_string(berry->description);  
  berry->description  = str_dup(buf);

  /* Fix name */
  sprintf(buf, "%s berry", bname[num]);
  free_string(berry->name);
  berry->name = str_dup(buf);

  /* Set timer */
  if (!IS_NPC(ch)) {
    berry->timer    = (get_skill(ch,sn) / 10) + get_curr_op(ch)/40;
    op_bonus        = get_curr_op(ch)/100;
  }
  else {    
    berry->timer    = ch->level/10;
  }

  /* Do the hunger calcs */
  switch (num) {
  case 0:
    berry->value[0] = 1;
    berry->value[1] = 1;
    berry->value[3] = 1; // Poisoned
    break;
  case 1:
    berry->value[0] = (num*6)+op_bonus;
    berry->value[1] = (num*6)+op_bonus;
    break;
  case 2:
    berry->value[0] = (num*8)+op_bonus;
    berry->value[1] = (num*8)+op_bonus;
    break;
  case 3:
    berry->value[0] = (num*10)+op_bonus;
    berry->value[1] = (num*10)+op_bonus;
    break;
  default:
    act( "a shrub starts to tremble then wither slowly.", ch, berry, NULL, TO_ROOM );
    act( "a shrub starts to tremble then wither slowly.", ch, berry, NULL, TO_CHAR );
    return;
    break;
  }

  /* Load and tell */
  obj_to_room( berry, ch->in_room );
  act( "a shrub seem to suddenly blossom and $p falls to the ground.", ch, berry, NULL, TO_ROOM );
  act( "a shrub seem to suddenly blossom and $p falls to the ground.", ch, berry, NULL, TO_CHAR );
  return;
}

/**********************************************************************
*       Function      : spell_minor_heal
*       Author        : Swordfish
*       Description   : heal weave
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_minor_heal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  long ch_flows = 0;
  int i = 0;

  if (ch->fighting == victim) {
     send_to_char("You are not able to concentrate enough to heal someone you are fighting.\n\r", ch);
     return;	
  }
  
  for (i = 0; i < MAX_SPHERE; i++) {
    if (skill_table[sn].spheres[i] > 0) {
	 ch_flows += ch->perm_sphere[i];
    }
  }

  // It's a object with weave affect, like a pill
  // Since ch == victim is not a legal cast for this
  if (ch == victim)
     ch_flows = level*2;

  if (!IS_NPC(ch)) {
    if (IS_SET(ch->talents, skill_table[sn].talent_req)) {
	 victim->hit = UMIN( victim->hit + ch_flows/2, victim->max_hit );
	 victim->hit_loc[LOC_LA] = UMIN( victim->hit_loc[LOC_LA] + ((ch_flows/2)/LOC_MOD_LA), get_max_hit_loc(victim, LOC_LA));
	 victim->hit_loc[LOC_LL] = UMIN( victim->hit_loc[LOC_LL] + ((ch_flows/2)/LOC_MOD_LL), get_max_hit_loc(victim, LOC_LL));
	 victim->hit_loc[LOC_HE] = UMIN( victim->hit_loc[LOC_HE] + ((ch_flows/2)/LOC_MOD_HE), get_max_hit_loc(victim, LOC_HE));
	 victim->hit_loc[LOC_BD] = UMIN( victim->hit_loc[LOC_BD] + ((ch_flows/2)/LOC_MOD_BD), get_max_hit_loc(victim, LOC_BD));
	 victim->hit_loc[LOC_RA] = UMIN( victim->hit_loc[LOC_RA] + ((ch_flows/2)/LOC_MOD_RA), get_max_hit_loc(victim, LOC_RA));
	 victim->hit_loc[LOC_RL] = UMIN( victim->hit_loc[LOC_RL] + ((ch_flows/2)/LOC_MOD_RL), get_max_hit_loc(victim, LOC_RL));
    }
    else {
	 victim->hit = UMIN( victim->hit + ch_flows/4, victim->max_hit );
	 victim->hit_loc[LOC_LA] = UMIN( victim->hit_loc[LOC_LA] + ((ch_flows/4)/LOC_MOD_LA), get_max_hit_loc(victim, LOC_LA));
	 victim->hit_loc[LOC_LL] = UMIN( victim->hit_loc[LOC_LL] + ((ch_flows/4)/LOC_MOD_LL), get_max_hit_loc(victim, LOC_LL));
	 victim->hit_loc[LOC_HE] = UMIN( victim->hit_loc[LOC_HE] + ((ch_flows/4)/LOC_MOD_HE), get_max_hit_loc(victim, LOC_HE));
	 victim->hit_loc[LOC_BD] = UMIN( victim->hit_loc[LOC_BD] + ((ch_flows/4)/LOC_MOD_BD), get_max_hit_loc(victim, LOC_BD));
	 victim->hit_loc[LOC_RA] = UMIN( victim->hit_loc[LOC_RA] + ((ch_flows/4)/LOC_MOD_RA), get_max_hit_loc(victim, LOC_RA));
	 victim->hit_loc[LOC_RL] = UMIN( victim->hit_loc[LOC_RL] + ((ch_flows/4)/LOC_MOD_RL), get_max_hit_loc(victim, LOC_RL));
    }
  }
  else {
    victim->hit = UMIN( victim->hit + ch_flows/4, victim->max_hit );
    victim->hit_loc[LOC_LA] = UMIN( victim->hit_loc[LOC_LA] + ((ch_flows/4)/LOC_MOD_LA), get_max_hit_loc(victim, LOC_LA));
    victim->hit_loc[LOC_LL] = UMIN( victim->hit_loc[LOC_LL] + ((ch_flows/4)/LOC_MOD_LL), get_max_hit_loc(victim, LOC_LL));
    victim->hit_loc[LOC_HE] = UMIN( victim->hit_loc[LOC_HE] + ((ch_flows/4)/LOC_MOD_HE), get_max_hit_loc(victim, LOC_HE));
    victim->hit_loc[LOC_BD] = UMIN( victim->hit_loc[LOC_BD] + ((ch_flows/4)/LOC_MOD_BD), get_max_hit_loc(victim, LOC_BD));
    victim->hit_loc[LOC_RA] = UMIN( victim->hit_loc[LOC_RA] + ((ch_flows/4)/LOC_MOD_RA), get_max_hit_loc(victim, LOC_RA));
    victim->hit_loc[LOC_RL] = UMIN( victim->hit_loc[LOC_RL] + ((ch_flows/4)/LOC_MOD_RL), get_max_hit_loc(victim, LOC_RL));
  }
  
  // Cost endurance to be healed
  victim->endurance -= skill_table[sn].min_endurance;
  
  update_pos( victim );
  
  if (ch->sex == SEX_FEMALE)
    send_to_char( "A cold feeling fills your body.\n\r", victim );
  else
    send_to_char( "A burning feeling shoot through your body.\n\r", victim );
  
  if ( ch != victim )
    send_to_char( "Ok.\n\r", ch );
  
  return;
}

/**********************************************************************
*       Function      : spell_heal
*       Author        : Swordfish
*       Description   : heal weave
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_heal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  long ch_flows = 0;
  int i = 0;

  if (ch->fighting == victim) {
     send_to_char("You are not able to concentrate enough to heal someone you are fighting.\n\r", ch);
     return;	
  }
  
  for (i = 0; i < MAX_SPHERE; i++) {
    if (skill_table[sn].spheres[i] > 0) {
	 ch_flows += ch->perm_sphere[i];
    }
  }
  
  // It's a object with weave affect, like a pill
  // Since ch == victim is not a legal cast for this
  if (ch == victim)
     ch_flows = level*2;
  
  if (!IS_NPC(ch)) {
    if (IS_SET(ch->talents, skill_table[sn].talent_req)) {
	 victim->hit = UMIN( victim->hit + ch_flows, victim->max_hit );
	 victim->hit_loc[LOC_LA] = UMIN( victim->hit_loc[LOC_LA] + (ch_flows/LOC_MOD_LA), get_max_hit_loc(victim, LOC_LA));
	 victim->hit_loc[LOC_LL] = UMIN( victim->hit_loc[LOC_LL] + (ch_flows/LOC_MOD_LL), get_max_hit_loc(victim, LOC_LL));
	 victim->hit_loc[LOC_HE] = UMIN( victim->hit_loc[LOC_HE] + (ch_flows/LOC_MOD_HE), get_max_hit_loc(victim, LOC_HE));
	 victim->hit_loc[LOC_BD] = UMIN( victim->hit_loc[LOC_BD] + (ch_flows/LOC_MOD_BD), get_max_hit_loc(victim, LOC_BD));
	 victim->hit_loc[LOC_RA] = UMIN( victim->hit_loc[LOC_RA] + (ch_flows/LOC_MOD_RA), get_max_hit_loc(victim, LOC_RA));
	 victim->hit_loc[LOC_RL] = UMIN( victim->hit_loc[LOC_RL] + (ch_flows/LOC_MOD_RL), get_max_hit_loc(victim, LOC_RL));
    }
    else {
	 victim->hit = UMIN( victim->hit + ch_flows/2, victim->max_hit );
	 victim->hit_loc[LOC_LA] = UMIN( victim->hit_loc[LOC_LA] + ((ch_flows/2)/LOC_MOD_LA), get_max_hit_loc(victim, LOC_LA));
	 victim->hit_loc[LOC_LL] = UMIN( victim->hit_loc[LOC_LL] + ((ch_flows/2)/LOC_MOD_LL), get_max_hit_loc(victim, LOC_LL));
	 victim->hit_loc[LOC_HE] = UMIN( victim->hit_loc[LOC_HE] + ((ch_flows/2)/LOC_MOD_HE), get_max_hit_loc(victim, LOC_HE));
	 victim->hit_loc[LOC_BD] = UMIN( victim->hit_loc[LOC_BD] + ((ch_flows/2)/LOC_MOD_BD), get_max_hit_loc(victim, LOC_BD));
	 victim->hit_loc[LOC_RA] = UMIN( victim->hit_loc[LOC_RA] + ((ch_flows/2)/LOC_MOD_RA), get_max_hit_loc(victim, LOC_RA));
	 victim->hit_loc[LOC_RL] = UMIN( victim->hit_loc[LOC_RL] + ((ch_flows/2)/LOC_MOD_RL), get_max_hit_loc(victim, LOC_RL));
    }
  }
  else {
    victim->hit = UMIN( victim->hit + ch_flows/2, victim->max_hit );
    victim->hit_loc[LOC_LA] = UMIN( victim->hit_loc[LOC_LA] + ((ch_flows/2)/LOC_MOD_LA), get_max_hit_loc(victim, LOC_LA));
    victim->hit_loc[LOC_LL] = UMIN( victim->hit_loc[LOC_LL] + ((ch_flows/2)/LOC_MOD_LL), get_max_hit_loc(victim, LOC_LL));
    victim->hit_loc[LOC_HE] = UMIN( victim->hit_loc[LOC_HE] + ((ch_flows/2)/LOC_MOD_HE), get_max_hit_loc(victim, LOC_HE));
    victim->hit_loc[LOC_BD] = UMIN( victim->hit_loc[LOC_BD] + ((ch_flows/2)/LOC_MOD_BD), get_max_hit_loc(victim, LOC_BD));
    victim->hit_loc[LOC_RA] = UMIN( victim->hit_loc[LOC_RA] + ((ch_flows/2)/LOC_MOD_RA), get_max_hit_loc(victim, LOC_RA));
    victim->hit_loc[LOC_RL] = UMIN( victim->hit_loc[LOC_RL] + ((ch_flows/2)/LOC_MOD_RL), get_max_hit_loc(victim, LOC_RL));
  }
  
  // Cost endurance to be healed
  victim->endurance -= skill_table[sn].min_endurance;

  update_pos( victim );
  
  if (ch->sex == SEX_FEMALE)
    send_to_char( "A cold feeling fills your body.\n\r", victim );
  else
    send_to_char( "A burning feeling shoot through your body.\n\r", victim );
  
  if ( ch != victim )
    send_to_char( "Ok.\n\r", ch );
  
  return;
}

/**********************************************************************
*       Function      : spell_major_heal
*       Author        : Swordfish
*       Description   : heal weave
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_major_heal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  long ch_flows = 0;
  int i = 0;

  if (ch->fighting == victim) {
     send_to_char("You are not able to concentrate enough to heal someone you are fighting.\n\r", ch);
     return;	
  }

  for (i = 0; i < MAX_SPHERE; i++) {
    if (skill_table[sn].spheres[i] > 0) {
	 ch_flows += ch->perm_sphere[i];
    }
  }

  if (IS_SET(ch->talents,TALENT_HEALING))
  {
	ch_flows = (ch_flows * 3) / 2;
  }
  // It's a object with weave affect, like a pill
  // Since ch == victim is not a legal cast for this
  if (ch == victim)
     ch_flows = level*2;  
  
  victim->hit = UMIN( victim->hit + (ch_flows*2), victim->max_hit );
  victim->hit_loc[LOC_LA] = UMIN( victim->hit_loc[LOC_LA] + ((ch_flows*2)/LOC_MOD_LA), get_max_hit_loc(victim, LOC_LA));
  victim->hit_loc[LOC_LL] = UMIN( victim->hit_loc[LOC_LL] + ((ch_flows*2)/LOC_MOD_LL), get_max_hit_loc(victim, LOC_LL));
  victim->hit_loc[LOC_HE] = UMIN( victim->hit_loc[LOC_HE] + ((ch_flows*2)/LOC_MOD_HE), get_max_hit_loc(victim, LOC_HE));
  victim->hit_loc[LOC_BD] = UMIN( victim->hit_loc[LOC_BD] + ((ch_flows*2)/LOC_MOD_BD), get_max_hit_loc(victim, LOC_BD));
  victim->hit_loc[LOC_RA] = UMIN( victim->hit_loc[LOC_RA] + ((ch_flows*2)/LOC_MOD_RA), get_max_hit_loc(victim, LOC_RA));
  victim->hit_loc[LOC_RL] = UMIN( victim->hit_loc[LOC_RL] + ((ch_flows*2)/LOC_MOD_RL), get_max_hit_loc(victim, LOC_RL));
  
  // Cost endurance to be healed
  victim->endurance -= skill_table[sn].min_endurance;

  update_pos( victim );
  
  if (ch->sex == SEX_FEMALE)
    send_to_char( "A cold feeling fills your body.\n\r", victim );
  else
    send_to_char( "A burning feeling shoot through your body.\n\r", victim );
  
  if ( ch != victim )
    send_to_char( "Ok.\n\r", ch );
  
  return;
}

/**********************************************************************
*       Function      : spell_delve
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_delve( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  char buf[MAX_INPUT_LENGTH];
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  int learned=0;
  
  // Need to be grouped - make sure not abused
/*
  if (!is_same_group(ch,victim) ) {
    send_to_char("They need to be in your group before you can delve them.\n\r", ch);
    return;
  }	
*/
  
  if (!IS_NPC(ch))
    learned = get_skill(ch,sn);	
  else
    learned = ch->level;
  
  if (number_percent() > learned) {
    send_to_char("You can't seem to find out what's wrong with them.\n\r", ch);
    return;   	
  }
  
  sprintf(buf, "You send flows of %s into $N and delve $M.", (char *)flow_text(sn, ch));
  act(buf, ch, NULL, victim, TO_CHAR);
  
  if (ch->sex == SEX_FEMALE)
    act("$n place $s hands on you and a cold feeling fills your body.", ch, NULL, victim, TO_VICT);
  else
    act("$n place $s hands on you and a burning feeling shoot through your body.", ch, NULL, victim, TO_VICT);
  
  send_to_char("\n[{YWounds{x]{W:{x\n\r", ch);

  // Wound description
  if (!IS_NULLSTR(victim->wound_description))
    send_to_char(victim->wound_description, ch);
  else
    act("$N has no wound description set.", ch, NULL, victim, TO_CHAR);
  
  send_to_char("\n[{rHealth{x]{W:{x\n\r", ch);

  // Hit locations:
  act("You delve $N's body parts and notice:", ch, NULL, victim, TO_CHAR);
  
  	sprintf(buf, "$N's %s is %s{x.", hit_flags[LOC_LA].name, get_hit_loc_wound_str(victim, LOC_LA));
  	act(buf, ch, NULL,victim, TO_CHAR);
 	 
  	sprintf(buf, "$N's %s is %s{x.", hit_flags[LOC_LL].name, get_hit_loc_wound_str(victim, LOC_LL));
  	act(buf, ch, NULL,victim, TO_CHAR);
  
  	sprintf(buf, "$N's %s is %s{x.", hit_flags[LOC_HE].name, get_hit_loc_wound_str(victim, LOC_HE));
  	act(buf, ch, NULL,victim, TO_CHAR);
 	 
  	sprintf(buf, "$N's %s is %s{x.", hit_flags[LOC_BD].name, get_hit_loc_wound_str(victim, LOC_BD));
  	act(buf, ch, NULL,victim, TO_CHAR);
 	 
  	sprintf(buf, "$N's %s is %s{x.", hit_flags[LOC_RA].name, get_hit_loc_wound_str(victim, LOC_RA));
  	act(buf, ch, NULL,victim, TO_CHAR);
 	 
  	sprintf(buf, "$N's %s is %s{x.", hit_flags[LOC_RL].name, get_hit_loc_wound_str(victim, LOC_RL));
  	act(buf, ch, NULL,victim, TO_CHAR);
	
  // HP
  if (victim->hit == victim->max_hit) {
    act("$N's health is perfect.",ch, NULL, victim, TO_CHAR);
  }
  else {
    	act("$N's is injured.",ch, NULL, victim, TO_CHAR);
  }
  
  // Endurance
  if (victim->endurance == victim->max_endurance) {
    act("$N's stamina is perfect.",ch, NULL, victim, TO_CHAR);
  }
  else {
    act("$N's is fatigued.",ch, NULL, victim, TO_CHAR);
  }
  
  // Hunger / Thirst / Drunk
  if ( !IS_NPC(victim) && victim->pcdata->condition[COND_DRUNK]   > 10 )
    act("$N is drunk.", ch, NULL, victim, TO_CHAR);
  if ( !IS_NPC(victim) && victim->pcdata->condition[COND_THIRST] ==  0 )
    act("$N is dehydrated." , ch, NULL, victim, TO_CHAR);
  if ( !IS_NPC(victim) && victim->pcdata->condition[COND_HUNGER]   ==  0 )
    act("$N is hungry." , ch, NULL, victim, TO_CHAR);
  
  // Affects
  if (IS_SET(ch->talents, TALENT_HEALING)) {
  	send_to_char("\n[{CAffects{x]{W:{x\n\r", ch);
  	do_affects(ch, victim->name);
  }
}

/**********************************************************************
*       Function      : spell_pinch
*       Author        : Swordfish
*       Description   : Rand and Avienda pinch each other with Air
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_pinch( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;

  act("You pinch $N with a flow of {Wair{x.", ch, NULL, victim, TO_CHAR);
  if (victim->class == CLASS_CHANNELER) {
    if (victim->sex == ch->sex) {
	 act("$n pinch you with a flow of {Wair{x.", ch, NULL, victim, TO_VICT);
    }
    else {
	 act("You feel as if someone is pinching you in the rear.", ch, NULL, victim, TO_VICT);
    }
  }
  else {
    act("You feel as if someone is pinching you in the rear.", ch, NULL, victim, TO_VICT);
  }

  if (victim->hit > victim->max_hit/4  && !IS_NPC(victim))
    victim->hit--;
  else
    damage(ch, victim, 1, sn, DAM_PIERCE, TRUE);
  
  return;
}

/**********************************************************************
*       Function      : spell_wall_of_fire
*       Author        : Swordfish
*       Description   : A wall of Fire to block and burn with
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_wall_of_fire( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA       *victim=NULL;    // Victim if any at to location
  ROOM_INDEX_DATA *from_location;
  ROOM_INDEX_DATA *to_location;
  OBJ_DATA        *from_wall;
  OBJ_DATA        *to_wall;
  EXIT_DATA       *pexit;
  EXIT_DATA       *pexit_rev=NULL;
  int             direction;
  AFFECT_DATA af;
  AFFECT_DATA *paf;

   if (!IS_SET(ch->world,WORLD_NORMAL)) {
      send_to_char("Somehow you are unable to combine the flows into this here...\n\r", ch);
      return;
   }
  
  /* Validate syntax */
  if (IS_NULLSTR(cast_arg2)) {
    send_to_char("Syntax: weave 'wall of fire' <direction>\n\r", ch);
    return;
  }

  /* Only 1 WoF at time */
  if (!IS_IMMORTAL(ch)) {
     for ( paf = ch->affected; paf != NULL; paf = paf->next) {
       if (paf->type == sn) {
	    send_to_char("You are not strong enough to control any more walls of fire.\n\r", ch);
	    return;
       }
     }
   }

  from_location = ch->in_room;

  if ((direction = find_exit(ch, cast_arg2)) != -1) {
    if ((pexit = from_location->exit[direction]) != NULL
	   && ( to_location = pexit->u1.to_room ) != NULL
	   && can_see_room(ch, pexit->u1.to_room)) {

	 if (IS_SET(pexit->exit_info, EX_CLOSED)) {
	   act("You are unable to direct the flows through the closed $d", ch, NULL, pexit->keyword, TO_CHAR);
	   return;
	 }

	 af.where	       = TO_AFFECTS;
	 //af.where	 = TO_AFF_ROOM;
	 af.casterId     = ch->id;
	 af.type         = sn;
	 af.level        = level;
	 af.duration     = SUSTAIN_WEAVE;
	 af.location     = APPLY_NONE;
	 af.modifier     = ch->holding/20;
	 if (af.modifier > 50)
	   af.modifier = 50;
	 af.bitvector    = 0;
	 

	 sprintf(buf, "A {rwall of fire{x rise from the ground blocking the %s entrance.\n\r", dir_name[direction]);
	 send_to_char(buf, ch);
	 act(buf, ch,NULL,NULL,TO_ROOM);

	 /* If to location have players, get the first one */
	 victim = to_location->people;

	 /* Don't write message unless there are players in to_location */
	 if (victim != NULL) {
	   sprintf(buf, "A {rwall of fire{x rise from the ground blocking the %s entrance.\n\r", dir_name[rev_dir[direction]]);
	   send_to_char(buf, victim);
	   act(buf, victim,NULL,NULL,TO_ROOM);
	 }
	 
	 from_wall = create_object(get_obj_index(OBJ_VNUM_FIREWALL), 0);
	 to_wall   = create_object(get_obj_index(OBJ_VNUM_FIREWALL), 0);

	 free_string(from_wall->description);
	 sprintf(buf, "{rA wall of fire blocks the %s entrance.{x", dir_name[direction]);
	 from_wall->description = str_dup(buf);
	 
	 free_string(to_wall->description);
	 sprintf(buf, "{rA wall of fire blocks the %s entrance.{x", dir_name[rev_dir[direction]]);
	 to_wall->description = str_dup(buf);

	 from_wall->level = af.level;
	 from_wall->timer = 0;
	 from_wall->value[0] = ch->id;
	 from_wall->value[1] = direction;
	 
	 to_wall->level = af.level;
	 to_wall->timer = 0;
	 to_wall->value[0] = ch->id;
	 to_wall->value[1] = rev_dir[direction];
	 
	 obj_to_room( from_wall, from_location);
	 obj_to_room( to_wall, to_location);
	
	 //weave_to_room(from_location, &af );
	 affect_to_char( ch, &af );

	 /* Set exit to firewall */
	 if (!IS_SET(pexit->exit_info, EX_FIREWALL))
	   SET_BIT(pexit->exit_info, EX_FIREWALL);

	 /* Set other side exist info */
	 if ((pexit_rev = to_location->exit[rev_dir[direction]]) != 0) {
	   if (!IS_SET(pexit_rev->exit_info, EX_FIREWALL))
		SET_BIT(pexit_rev->exit_info, EX_FIREWALL);
	 }

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

/**********************************************************************
*       Function      : spell_wall_of_air
*       Author        : Swordfish
*       Description   : A wall of Air to block an entrance
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_wall_of_air( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA       *victim=NULL;    // Victim if any at to location
  ROOM_INDEX_DATA *from_location;
  ROOM_INDEX_DATA *to_location;
  OBJ_DATA        *from_wall;
  OBJ_DATA        *to_wall;
  EXIT_DATA       *pexit;
  EXIT_DATA       *pexit_rev=NULL;
  int             direction;
  AFFECT_DATA af;
  AFFECT_DATA *paf;

   if (!IS_SET(ch->world,WORLD_NORMAL)) {
      send_to_char("Somehow you are unable to combine the flows into this here...\n\r", ch);
      return;
   }
  
  /* Validate syntax */
  if (IS_NULLSTR(cast_arg2)) {
    send_to_char("Syntax: weave 'wall of air' <direction>\n\r", ch);
    return;
  }
  
  /* Only 1 WoF at time */
  if (!IS_IMMORTAL(ch)) {
    for ( paf = ch->affected; paf != NULL; paf = paf->next) {
	 if (paf->type == sn) {
	   send_to_char("You are not strong enough to control any more walls of air.\n\r", ch);
	   return;
	 }
    }
  }
  
  from_location = ch->in_room;
  
  if ((direction = find_exit(ch, cast_arg2)) != -1) {
    if ((pexit = from_location->exit[direction]) != NULL
	   && ( to_location = pexit->u1.to_room ) != NULL
	   && can_see_room(ch, pexit->u1.to_room)) {
	 
	 if (IS_SET(pexit->exit_info, EX_CLOSED)) {
	   act("You are unable to direct the flows through the closed $d", ch, NULL, pexit->keyword, TO_CHAR);
	   return;
	 }
	 
	 af.where	 = TO_AFFECTS;
	 //af.where	 = TO_AFF_ROOM;
	 af.casterId     = ch->id;
	 af.type         = sn;
	 af.level        = level;
	 af.duration     = SUSTAIN_WEAVE;
	 af.location     = APPLY_NONE;
	 af.modifier     = ch->holding/20;
	 if (af.modifier > 50)
	   af.modifier = 50;
	 af.bitvector    = 0;
	 
	 
	 sprintf(buf, "The {Wair{x shimmers for a moment and a {Cblue haze{x seems to block the %s entrance.\n\r", dir_name[direction]);
	 send_to_char(buf, ch);
	 act(buf, ch,NULL,NULL,TO_ROOM);

	 /* If to location have players, get the first one */
	 victim = to_location->people;

	 /* Don't write message unless there are players in to_location */
	 if (victim != NULL) {
	   sprintf(buf, "The {Wair{x shimmers for a moment and a {Cblue haze{x seems to block the %s entrance.\n\r", dir_name[rev_dir[direction]]);
	   send_to_char(buf, victim);
	   act(buf, victim,NULL,NULL,TO_ROOM);
	 }
	 
	 from_wall = create_object(get_obj_index(OBJ_VNUM_AIRWALL), 0);
	 to_wall   = create_object(get_obj_index(OBJ_VNUM_AIRWALL), 0);
	 
	 free_string(from_wall->description);
	 //sprintf(buf, "{CA wall of air blocks the %s entrance.{x", dir_name[direction]);
	 sprintf(buf, "{CA blue haze in the {Wair {Cblocks the %s entrance.{x", dir_name[direction]);
	 from_wall->description = str_dup(buf);
	 
	 free_string(to_wall->description);
	 //sprintf(buf, "{CA wall of air blocks the %s entrance.{x", dir_name[rev_dir[direction]]);
	 sprintf(buf, "{CA blue haze in the {Wair {Cblocks the %s entrance.{x", dir_name[rev_dir[direction]]);
	 to_wall->description = str_dup(buf);
	 
	 from_wall->level = af.level;
	 from_wall->timer = 0;
	 from_wall->value[0] = ch->id;
	 from_wall->value[1] = direction;
	 
	 to_wall->level = af.level;
	 to_wall->timer = 0;
	 to_wall->value[0] = ch->id;
	 to_wall->value[1] = rev_dir[direction];
	 
	 obj_to_room( from_wall, from_location);
	 obj_to_room( to_wall, to_location);
	
	 //weave_to_room(ch->in_room, &af );
	 affect_to_char( ch, &af );

	 /* Set exit to firewall */
	 if (!IS_SET(pexit->exit_info, EX_AIRWALL))
	   SET_BIT(pexit->exit_info, EX_AIRWALL);

	 /* Set other side exist info */
	 if ((pexit_rev = to_location->exit[rev_dir[direction]]) != 0) {
	   if (!IS_SET(pexit_rev->exit_info, EX_AIRWALL))
		SET_BIT(pexit_rev->exit_info, EX_AIRWALL);
	 }

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

/**********************************************************************
*       Function      : spell_gust_of_wind
*       Author        : Swordfish
*       Description   : Cloud Dancer weave - a gust the caster can throw
*                       the target out of the room with.
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_gust_of_wind( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  EXIT_DATA *pexit=NULL;
  ROOM_INDEX_DATA *to_room=NULL;
  char buf  [MAX_STRING_LENGTH];
  int door=0;

  // portal stuff
  OBJ_DATA *portal=NULL;
  ROOM_INDEX_DATA *location=NULL;
  ROOM_INDEX_DATA *old_room=NULL;

  /* Validate syntax */
  if (IS_NULLSTR(cast_arg3)) {
    send_to_char("Syntax: weave 'gust of wind' <target> <direction>\n\r", ch);
    return;
  }
  
  if (victim->position == POS_FIGHTING) {
    send_to_char("Wait till the end of fight.\n\r",ch);
    return;
  }
  
  if (is_safe(ch,victim)) {
    return;
  }

  portal = get_obj_list( ch, cast_arg3,  ch->in_room->contents );
  
  if (portal == NULL && (door = find_exit( ch, cast_arg3 )) >= 0 ) {
    if ( (pexit = ch->in_room->exit[door]) != NULL ) {
	 if ( IS_SET(pexit->exit_info, EX_ISDOOR) )  {
	   if ( IS_SET(pexit->exit_info, EX_CLOSED) ) {
		send_to_char( "That direction is closed.\n\r",      ch ); 
		return;
	   }
	   else if ( IS_SET(pexit->exit_info, EX_LOCKED) )  {
	     send_to_char( "That direction is locked.\n\r",     ch ); 
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
    if (portal == NULL) 
	 // find_exit will give error msg
	 return;

    if (portal->item_type != ITEM_PORTAL && portal->item_type != ITEM_VEHICLE) {
	 act("You can't throw someone into $p.",ch,portal,NULL,TO_CHAR);
	 return;
    }
  }
  
  if (portal != NULL && portal->item_type != ITEM_PORTAL && portal->item_type != ITEM_VEHICLE) {
    act("You can't throw someone into $p.",ch,portal,NULL,TO_CHAR);
    return;
  }

  if (portal != NULL && IS_SET(portal->value[2],GATE_SKIMMING_IN)) {
    act("You can't throw someone into $p.",ch,portal,NULL,TO_CHAR);
    return;
  }
  
  if (IS_SET(victim->imm_flags,IMM_WIND))
  {
    act("$N stands strong against the wind.\r\n",ch,NULL,victim,TO_CHAR);
    return;
  }

  if (portal == NULL) {
    sprintf(buf,"You create a gust of wind and toss $N %sward.",dir_name[door]);
    act(buf,ch,NULL,victim,TO_CHAR);
    sprintf(buf,"Suddeny a gust seems to grab around you and tosses you %sward.", dir_name[door]);
    act(buf,ch,NULL,victim,TO_VICT);
    sprintf(buf,"Suddeny a gust seems to grab $N and tosses $M %sward.", dir_name[door]);
    act(buf,ch,NULL,victim,TO_NOTVICT);
    
    to_room = pexit->u1.to_room;
    char_from_room( victim );
    char_to_room( victim, to_room );
    victim->position = POS_SITTING;
    update_pos(victim);
  
    sprintf(buf, "Suddeny a wind blows up from the %s and $n is tossed into the room and to the ground.", dir_name[rev_dir[door]]);
    act( buf, victim, NULL, NULL, TO_ROOM );
    DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
  }

  // portal gust
  else {
    old_room = victim->in_room;
    
    if (IS_SET(portal->value[1],EX_CLOSED)) {
	 act("You create a gust of wind and toss $N toward the closed $p.",ch,portal,victim,TO_CHAR);
	 act("Suddenly a gust seems to grab around you and tosses you toward the closed $p.",ch,portal,victim,TO_VICT);
	 act("Suddenly a gust seems to grab around $N and tosses $M toward the closed $p.",ch,portal,victim,TO_NOTVICT);
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
		act("You create a gust of wind and toss $N toward the $p, but $E bounce back by an unseen force.",ch,portal,victim,TO_CHAR);
		act("Suddenly a gust seems to grab around you and tosses you toward the $p, but you bounce back by an unseen force.",ch,portal,victim,TO_VICT);
		act("Suddenly a gust seems to grab arund $N and tosses $M toward the $p, but $E bounce back by an unseen force.",ch,portal,victim,TO_NOTVICT);                	             	
		return;
	   }
	   
	   // gust into dreamgate
	   act("You create a gust of wind and toss $N into $p.",ch,portal,victim,TO_CHAR);
	   act("Suddenly a gust seems to grab around you and tosses you into $p.",ch,portal,victim,TO_VICT);
	   act("Suddenly a gust seems to grab around $N and tosses $M info $p.",ch,portal,victim,TO_NOTVICT);                
	   TOGGLE_BIT(victim->world, WORLD_NORMAL);
	   TOGGLE_BIT(victim->world, WORLD_TAR_FLESH);
	   do_function(victim, &do_look, "auto");
	   return;
	 }
	 else {
	   act("You create a gust of wind and toss $N toward the $p, but $E bounce back by an unseen force.",ch,portal,victim,TO_CHAR);
	   act("Suddenly a gust seems to grab around you and tosses you toward the $p, but you bounce back by an unseen force.",ch,portal,victim,TO_VICT);
	   act("Suddenly a gust seems to grab arund $N and tosses $M toward the $p, but $E bounce back by an unseen force.",ch,portal,victim,TO_NOTVICT);
	   return;
	 }
    }
    
    // Normal gust into portal type
    act("You create a gust of wind and toss $N into $p.",ch,portal,victim,TO_CHAR);
    act("Suddenly a gust seems to grab around you and tosses you into $p.",ch,portal,victim,TO_VICT);
    act("Suddenly a gust seems to grab around $N and tosses $M info $p.",ch,portal,victim,TO_NOTVICT);
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
  
  return;
}

/**********************************************************************
*       Function      : spell_thunderclap
*       Author        : Swordfish
*       Description   : Cloud Dancer weave - the caster can cause a 
*                       deafiening and startling thunderclap, whose
*                       effect is similar to the bash skill.
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_thunderclap( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;

  if (!IS_OUTSIDE(ch)) {
    send_to_char("You start to reach into the sky, but sudden realize that you are not outside!\n\r", ch);
    return;
  }
  
  if (weather_info.sky < SKY_CLOUDY) {
    send_to_char("You reach into the sky, but there are no clouds to generate a thunderclap from.\n\r", ch);
    return;
  }
  
  act("You reach into the sky bringing {Dclouds{x that generates a deafening and startling {Wthun{Dder{Wclap{x!", ch, NULL, NULL, TO_CHAR);
  
  act("Your heart jumps as the sky suddenly crack into a deafening and startling {Wthun{Dder{Wclap{x right above you!", ch, NULL, NULL, TO_ROOM);  
  
  // Set the effects to chars in room
  // tell all in same area about the thunder
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if (vch->in_room == NULL)
	 continue;
    //Thunderclap is a little too powerful at the moment. If it's going to go off in the middle of a fight, it's going to affect everyone but the weaver
//    if (is_same_group(ch, vch))
//	 continue;	 
    //if (ch == vch)
//	 continue;
    if (!IS_SAME_WORLD(vch, ch))
	 continue;
    if (IS_GHOLAM(vch))
	continue;
    //CLOUDDANCING talent?
    if (!IS_NPC(ch))
    {
       if (!IS_SET(ch->talents, TALENT_CLOUDDANCING)) {
          if (number_percent() > get_skill(ch,sn) / 5)
          {
	     continue;
          }
       }
    }
  
    if (vch->in_room == ch->in_room) {
	 vch->position = POS_RESTING;
	 DAZE_STATE(vch, number_range(1,3) * PULSE_VIOLENCE);
    }
    if ( vch->in_room != ch->in_room && vch->in_room->area == ch->in_room->area
	    &&   IS_OUTSIDE(vch)
	    &&   IS_AWAKE(vch) )	 
	 send_to_char( "{DTh{xun{Dder{x rolls through the sky, making the earth ShIvEr slightly beneath you.\n\r", vch );    
  }
  
  return;
}

/**********************************************************************
*       Function      : spell_create_rain
*       Author        : Swordfish
*       Description   : Cloud Dancer weave - the caster can cause the
*                       moisture within clouds to condense and fall to
*                       the ground in a shower.
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_create_rain( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;
  
  if (!IS_OUTSIDE(ch)) {
    send_to_char("You start to reach into the sky, but sudden realize that you are not outside!\n\r", ch);
    return;
  }

  if (weather_info.sky < SKY_CLOUDY) {
    send_to_char("You reach into the sky, but there are no clouds to create rain from.\n\r", ch);
    return;
  }
  
    //CLOUDDANCING talent?
    if (!IS_SET(ch->talents, TALENT_CLOUDDANCING)) {
       if (number_percent() > get_skill(ch,sn) / 5)
       {
    	  send_to_char("You reach into the sky, but the clouds don't respond.\n\r", ch);
	  return;
       }
    }
  
  act("You reach into the sky causing the moisture within the clouds to condense and fall to the ground in a shower.", ch, NULL, NULL, TO_CHAR);
  act("Rain starts to pour down from the sky.", ch, NULL, NULL, TO_ROOM);

  for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
    obj_next = obj->next_content;
    if (obj->item_type == ITEM_DRINK_CON) {
	 if (obj->value[1] != 0 && obj->value[2] != LIQ_WATER)
	   continue;
	 if (obj->value[1] >= obj->value[0])
	   continue;
	 
	 act("The rain fills $p quickly.", ch, obj, NULL, TO_CHAR);
	 act("The rain fills $p quickly.", ch, obj, NULL, TO_ROOM);
	 obj->value[2] = LIQ_WATER;
	 obj->value[1] = obj->value[0];
    }
  }
  
  return;
}

/* ========================= */
/* BELOW IS OFFENSIVE WEAVES */ 
/* ========================= */


/**********************************************************************
*       Function      : calculate_weave_dam
*       Author        : Swordfish
*       Description   : General calculation of weave damage based on
                        the highest sphere in cost of weave and strength
                        of character, then allow to adjust dam with holding.
*       Parameters    : sn - skill
*                     : ch - char
*                       reduction - rank 1 should get 3
                                    rank 2 should get 2
                                    rank 3 should get 1
*       Returns       : 
**********************************************************************/
int calculate_weave_dam(int sn, CHAR_DATA *ch, int reduction)
{
  int dam = 0;
  int high_sphere = -1;
  int high_sphere_value = 0;
  int i = 0;
  //char buf[MAX_INPUT_LENGTH];

  /* Better safe than sorry */
  if (reduction <= 0)
   reduction = 1;

  /* Find highest sphere in cost */
  for (i = 0; i < MAX_SPHERE; i++) {
    if (skill_table[sn].spheres[i] > 0) {
	    if (skill_table[sn].spheres[i] > high_sphere_value) {
	      high_sphere = i;
	      high_sphere_value = skill_table[sn].spheres[i];
	   }
    }
  }
/*
  if (IS_CODER(ch)) {
   sprintf(buf, "High sphere is %s with value %d\n", capitalize(sphere_table[high_sphere].name), high_sphere_value);
   send_to_char(buf, ch);
  }
*/
  
  /* Calc damage */
  if (high_sphere != -1 && high_sphere <= MAX_SPHERE)
    dam = ((high_sphere_value * (ch->perm_sphere[high_sphere]/(double)(reduction*number_range(13,16)))) * ch->holding/get_curr_op(ch));
  else 
    dam = 0;

  return(dam);
}

/* ########################################################################## */
/* ## Start Offence Fire Weaves:                                           ## */
/* ## Rank 1: Bar of fire                                                  ## */
/* ## Rank 2: Fireball                                                     ## */
/* ## Rank 3: Inferno                                                      ## */
/* ########################################################################## */

/**********************************************************************
*       Function      : spell_baroffire
*       Author        : Swordfish
*       Description   : Fire weave rank 1
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_baroffire( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  int dam = 0;
  char buf[MAX_INPUT_LENGTH];
  bool mainsphere = FALSE;
  
  dam = calculate_weave_dam(sn, ch, 3);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_FIRE) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_FIRE]/2;
    }
  }

  if (IS_CODER(ch)) {
     sprintf(buf, "[ {YCoder{x ]: MAX DAM 'bar of fire' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
     send_to_char(buf, ch);
  }

  act("A {Rbar {rof {Rf{Yi{Rr{Ye{x shoots from $n's hand toward $N.", ch, NULL, victim, TO_NOTVICT);
  act("A {Rbar {rof {Rf{Yi{Rr{Ye{x shoots from your hand toward $N.", ch, NULL, victim, TO_CHAR);
  act("A {Rbar {rof {Rf{Yi{Rr{Ye{x shoots from $n's hand toward you!", ch, NULL, victim, TO_VICT);
  
  damage(ch, victim, dam, sn, DAM_FIRE, TRUE);
  return;
}

/**********************************************************************
*       Function      : spell_fireball
*       Author        : Swordfish
*       Description   : Fire weave rank 2
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_fireball( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  char buf[MAX_INPUT_LENGTH];
  static char * const numbername [] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty", "lots of"};
  static char * const numberorder [] = {"none existing", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eight", "ninth", "tenth", "eleventh", "twelfth", "thirteenth", "fourteenth", "fifteenth", "sixteenth", "seventeenth", "eighteenth", "nineteenth", "twentieth", "next"};
  int dam = 0;
  int fireballs = 1;
  int i = 0;
  bool mainsphere = FALSE;
  
  dam = calculate_weave_dam(sn, ch, 2);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_FIRE) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_FIRE]/2;
    }
  }
  
  if (IS_CODER(ch)) {  
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'fireball' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }
  
  if (!IS_NPC(ch)) {
    fireballs = get_skill(ch,sn)/25;    
    
    if (fireballs < 1)
	 fireballs = 1;
  }
  else {
    fireballs = ch->level/25;
  }

  //Cap of 12
  if (fireballs > 12)
	fireballs = 12;
  
  sprintf(buf, "%s {Rfi{mr{Yeb{ma{Rll%s{x leaps from $n's hand and spreads towards $N.", fireballs > 20 ? numbername[20] : numbername[fireballs],
		fireballs <= 1 ? "" : "s");
  act(buf, ch,NULL,victim,TO_NOTVICT);
  sprintf(buf, "%s {Rfi{mr{Yeb{ma{Rll%s{x leaps from your hand and spreads toward $N.", fireballs > 20 ? numbername[20] : numbername[fireballs],
		fireballs <= 1 ? "" : "s");
  act(buf, ch,NULL,victim,TO_CHAR);
  sprintf(buf, "%s {Rfi{mr{Yeb{ma{Rll%s{x leaps from $n's hand and arcs toward you!", fireballs > 20 ? numbername[20] : numbername[fireballs],
		fireballs <= 1 ? "" : "s");
  act(buf, ch, NULL,victim,TO_VICT);
  
  for (i = 1; i <= fireballs; i++) {
    if (fireballs > 1 && number_chance(10*i)) {
         if (i > 20)
	 sprintf(buf, "The %s {Rfi{mr{Yeb{ma{Rll{x misses $N.", numberorder[20]);
         else
	 sprintf(buf, "The %s {Rfi{mr{Yeb{ma{Rll{x misses $N.", numberorder[i]);
	 act(buf, ch,NULL,victim,TO_CHAR);
	 act(buf, ch,NULL,victim,TO_NOTVICT);
         if (i > 20)
	 sprintf(buf, "The %s {Rfi{mr{Yeb{ma{Rll{x miss you!", numberorder[20]);
         else
	 sprintf(buf, "The %s {Rfi{mr{Yeb{ma{Rll{x miss you!", numberorder[i]);
	 act(buf, ch, NULL,victim,TO_VICT);
    }
    else {
	 if (victim->position == POS_DEAD)
         {
	    continue;
	 }
	 damage( ch, victim, number_range(dam/2, dam), sn, DAM_FIRE ,TRUE);
    }
  }
  
  return;
}

/**********************************************************************
*       Function      : spell_inferno
*       Author        : Swordfish
*       Description   : Fires weave rank 3
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_inferno( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  CHAR_DATA *vch;  
  CHAR_DATA *vch_next;
  char buf[MAX_INPUT_LENGTH];
  int dam = 0; // Max Dam
  int rdam= 0; // Real Dam
  int infernos=1;
  int i=0;
  bool mainsphere = FALSE;

  OBJ_DATA *obj_lose, *obj_next;   // For drop objects 'heat metal' alike
  bool fail = TRUE;
  bool remove = FALSE;
  
  dam = calculate_weave_dam(sn, ch, 1);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_FIRE) {
	 mainsphere = TRUE;
	 dam += number_range(ch->perm_sphere[SPHERE_FIRE]/3, ch->perm_sphere[SPHERE_FIRE]);
    }
  }
  
  if (IS_CODER(ch)) {
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'inferno' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }
  
  if (!IS_NPC(ch)) {
    infernos = get_skill(ch,sn)/33;
    
    if (infernos < 1)
	 infernos = 1;
  }
  else {
    infernos = ch->level/25;
  }
  
  
  act("An {rInferno{x of {rf{Yl{ra{Ym{re{Ys{x suddenly arise around $N.", ch, NULL, victim, TO_NOTVICT);
  act("You make an {rInferno{x of {rf{Yl{ra{Ym{re{Ys{x suddenly arise around $N.", ch, NULL, victim, TO_CHAR);
  act("An {rInferno{x of {rf{Yl{ra{Ym{re{Ys{x suddenly arise around you!", ch, NULL, victim, TO_VICT);
  
  // Do the dam
  for (i = 1; i <= infernos; i++) {  
    rdam = number_range(dam/2, dam);
    if (victim->position == POS_DEAD)
    {
         continue;
    }
    damage(ch, victim, rdam, sn, DAM_FIRE, TRUE);

  }

  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if ( vch->in_room == NULL )
	 continue;
    if (ch == vch)
	 continue;
    if (IS_GHOLAM(vch))
         continue;
    if (vch == victim)
	 continue;
    if (is_same_group(vch, ch)) {
	   if (get_skill(ch,sn) >= number_percent()) {
		continue;
	   }
    }
    if (!IS_SAME_WORLD(vch, ch))
    	continue;
    if ( vch->in_room == ch->in_room )  {
	 if ( vch != ch )  {
	   if (vch->fighting == ch)
           {
  		for (i = 1; i <= infernos/3; i++) {
    		   if (infernos > 1 && number_chance(10*i)) {
  			act("An {rInferno{x of {rf{Yl{ra{Ym{re{Ys{x suddenly arise around $N.", ch, NULL, vch, TO_NOTVICT);
  			act("You make an {rInferno{x of {rf{Yl{ra{Ym{re{Ys{x suddenly arise around $N.", ch, NULL, vch, TO_CHAR);
  			act("An {rInferno{x of {rf{Yl{ra{Ym{re{Ys{x suddenly arise around you!", ch, NULL, vch, TO_VICT);
    		   }
    		   else {
         		   if (vch->position == POS_DEAD)
         		   {
               		      continue;
         		   }
    			   damage(ch, vch, rdam, sn, DAM_FIRE, TRUE);
		
         		   if (number_percent() < 5)
           		      DAZE_STATE(vch, number_range(1,2) * PULSE_VIOLENCE);
    		   }
  		}

           }
	   else
	   if (number_percent() > 33 )  {
  		act("An {rInferno{x of {rf{Yl{ra{Ym{re{Ys{x suddenly arise around $N.", ch, NULL, vch, TO_NOTVICT);
  		act("You make an {rInferno{x of {rf{Yl{ra{Ym{re{Ys{x suddenly arise around $N.", ch, NULL, vch, TO_CHAR);
  		act("An {rInferno{x of {rf{Yl{ra{Ym{re{Ys{x suddenly arise around you!", ch, NULL, vch, TO_VICT);
	        if (vch->position == POS_DEAD)
         	{
            	   continue;
         	}
    		damage(ch, vch, rdam, sn, DAM_FIRE, TRUE);
	   }
	 }
	 continue;
    }
  }
	
  // Bonus
  // If the heat gets HOT, lets sear or DROP! :)
  if (number_percent() < number_range(15,35)) {
    
    // Remove objects if burned?
    if (number_percent() <= 5)
	 remove = TRUE;
    
    if (!saves_spell(level + 2,victim,DAM_FIRE)  &&  !IS_SET(victim->imm_flags,IMM_FIRE)) {	 
	 for ( obj_lose = victim->carrying; obj_lose != NULL; obj_lose = obj_next) {
	   obj_next = obj_lose->next_content;

	   if (number_range(1, 2*level) > obj_lose->level &&
		  !saves_spell(level,victim,DAM_FIRE) &&
		  !IS_OBJ_STAT(obj_lose,ITEM_NONMETAL) &&
		  !IS_OBJ_STAT(obj_lose,ITEM_ROT_DEATH) &&
		  !IS_OBJ_STAT(obj_lose,ITEM_BURN_PROOF)) {
		
		switch ( obj_lose->item_type ) {
		case ITEM_ARMOR:
		  if (obj_lose->wear_loc != -1) { /* remove the item */
		    if (can_drop_obj(victim,obj_lose) && 
			   (obj_lose->weight / 10) < number_range(1,2 * get_curr_stat(victim,STAT_DEX)) &&  
			   remove_obj( victim, obj_lose->wear_loc, remove )) {
			 act("$n yelps and throws $p to the ground!", victim,obj_lose,NULL,TO_ROOM);
			 act("You remove and drop $p before it {Yburns{x you.", victim,obj_lose,NULL,TO_CHAR);
			 dam += (number_range(1,obj_lose->level) / 3);
			 obj_from_char(obj_lose);
			 obj_to_room(obj_lose, victim->in_room);
			 fail = FALSE;
		    }
		    else {/* stuck on the body! ouch! */
			 act("Your skin is {Yseared{x by $p!", ch,obj_lose,victim,TO_VICT);
			 act("$N's skin is {Yseared{x by $p!", ch,obj_lose,victim,TO_ROOM);
			 act("$N's skin is {Yseared{x by $p!", ch,obj_lose,victim,TO_CHAR);
			 dam += (number_range(1,obj_lose->level));
			 fail = FALSE;
		    }
		  }
		  break;
		}
	   }
	 }
    }
  }		  
  
  return;
}

/* ########################################################################## */
/* ## Start Offence Earth Weaves:                                          ## */
/* ## Rank 1: Flying rocks                                                 ## */
/* ## Rank 2: Rolling ring                                                 ## */
/* ## Rank 3: Earthquake                                                   ## */
/* ########################################################################## */

/**********************************************************************
*       Function      : spell_flyingrocks
*       Author        : Swordfish
*       Description   : Earth weave rank 1
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_flyingrocks( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  int dam = 0;
  char buf[MAX_INPUT_LENGTH];
  bool mainsphere = FALSE;
  
  dam = calculate_weave_dam(sn, ch, 3);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_EARTH) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_EARTH]/2;
    }
  }
  
  if (IS_CODER(ch)) {
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'flying rocks' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }
  
  act("$n makes a slight motion with $s hand and suddenly a {yrock{x on the ground shoots toward $N.", ch, NULL, victim, TO_NOTVICT);
  act("You make a slight motion with your hand and suddenly a {yrock{x on the ground shoots toward $N.", ch, NULL, victim, TO_CHAR);
  act("$n makes a slight motion with $s hand and suddenly a {yrock{x on the ground shoots toward you!", ch, NULL, victim, TO_VICT);
 
  damage(ch, victim, dam, sn, DAM_PIERCE, TRUE);
  return;
}


/**********************************************************************
*       Function      : spell_rollingring
*       Author        : Swordfish
*       Description   : Earth weave rank 2
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_rollingring( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  char buf[MAX_INPUT_LENGTH];
  static char * const numbername [] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"};
  static char * const numberorder [] = {"none existing", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eight", "ninth", "tenth"};
  int dam = 0;
  int rings=1;
  int i=0;
  bool mainsphere = FALSE;
  
  dam = calculate_weave_dam(sn, ch, 2);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_EARTH) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_EARTH]/2;
    }
  }
  
  if (IS_CODER(ch)) {  
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'rolling ring' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }

  if (!IS_NPC(ch)) {
    rings = get_skill(ch,sn)/25;
    
    if (rings < 1)
	 rings = 1;
  }
  else {
    rings = ch->level/25;
  }

  sprintf(buf, "%s ring%s of {rf{Yi{rr{Ye{x and {yearth{x emanates from around $n and %s pushed towards $N.", rings > 20 ? numbername[20] : numbername[rings], rings <= 1 ? "" : "s", rings <= 1 ? "is" : "are");
  act(buf, ch, NULL, victim, TO_NOTVICT);
  sprintf(buf, "%s ring%s of {rf{Yi{rr{Ye{x and {yearth{x emanates from around you and %s pushed toward $N.", rings > 20 ? numbername[20] : numbername[rings], rings <= 1 ? "" : "s", rings <= 1 ? "is" : "are");
  act(buf, ch, NULL, victim, TO_CHAR);
  sprintf(buf, "%s ring%s of {rf{Yi{rr{Ye{x and {yearth{x emanates from around $n and %s pushed towards you!", rings > 20 ? numbername[20] : numbername[rings], rings <= 1 ? "" : "s", rings <= 1 ? "is" : "are");
  act(buf, ch, NULL, victim, TO_VICT);

  for (i = 1; i <= rings; i++) {
    if (rings > 1 && number_chance(10*i)) {
      sprintf(buf, "The %s ring of {rf{Yi{rr{Ye{x and {yearth{x terminates before reaching $N.", i > 20 ? numberorder[20] : numberorder[i]);
	 act(buf, ch,NULL,victim,TO_CHAR);
         act(buf, ch,NULL,victim,TO_NOTVICT);
	 sprintf(buf, "The %s ring of {rf{Yi{rr{Ye{x and {yearth{x terminates before reaching you!", i > 20 ? numberorder[20] : numberorder[i]);
	 act(buf, ch, NULL,victim,TO_VICT);
    }
    else {
         if (victim->position == POS_DEAD)
         {
            continue;
         }

	 damage( ch, victim, number_range(dam/2, dam), sn, DAM_FIRE ,TRUE);

	 if (number_percent() < 10)
	   DAZE_STATE(victim, number_range(1,2) * PULSE_VIOLENCE);
    }
  }   
}

/**********************************************************************
*       Function      : spell_earthquake
*       Author        : Swordfish
*       Description   : Earth weave rank 3
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  char buf[MAX_INPUT_LENGTH];
  int dam = 0;
  bool mainsphere = FALSE;
 
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;

  send_to_char( "The {yearth{x tReMbLeS visciously beneath your feet!\n\r", ch );
  act( "Suddenly the {yearth{x starts tReMbLiNg and shiver visciously around $n.", ch, NULL, NULL, TO_ROOM );

  dam = calculate_weave_dam(sn, ch, 1);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_EARTH) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_EARTH]/2;
    }
  }

  if (IS_CODER(ch)) {
     sprintf(buf, "[ {YCoder{x ]: MAX DAM 'earthquake' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
     send_to_char(buf, ch);
  }

  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if ( vch->in_room == NULL )
	 continue;
    if (ch == vch)
	 continue;
    if (is_same_group(vch, ch))
	 continue;
    if (!IS_SAME_WORLD(vch, ch))
    	continue;
    if (IS_GHOLAM(vch))
        continue;
    if ( vch->in_room == ch->in_room ) {
	 if ( vch != ch ) {
	   if (IS_AFFECTED(vch,AFF_FLYING)) {
		 //damage( ch, vch, 0, sn, DAM_BASH, TRUE);
	    }
	    else {
		 damage( ch, vch, dam, sn, DAM_BASH, TRUE);
		 
   	 if (number_percent() < 10)
	      DAZE_STATE(vch, number_range(1,2) * PULSE_VIOLENCE);		 
	    }
	  }
	  continue;
	}
	
	if ( vch->in_room->area == ch->in_room->area && IS_SET(vch->chan_flags, CHAN_SEEAREAWEAVES))
	  send_to_char( "The {yearth{x trembles and shivers.\n\r", vch );
  }
  
  return; 
}

/* ########################################################################## */
/* ## Start Offence Air weave:                                             ## */
/* ## Rank 1: Blast of air                                                 ## */
/* ## Rank 2: Lightning bolt                                               ## */
/* ## Rank 3: Call lightning                                               ## */
/* ########################################################################## */

/**********************************************************************
*       Function      : spell_blastofair
*       Author        : Swordfish
*       Description   : Air weave rank 1
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_blastofair( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  int dam = 0;
  char buf[MAX_INPUT_LENGTH];
  bool mainsphere = FALSE;
  
  dam = calculate_weave_dam(sn, ch, 3);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_AIR) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_AIR]/2;
    }
  }

  if (IS_CODER(ch)) {
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'blast of air' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }


  act("A {Bc{bol{Bd{x blast of {Wair{x appears in front of $n and {bfr{Bee{bzes{x $N.", ch, NULL, victim, TO_NOTVICT);
  act("A {Bc{bol{Bd{x blast of {Wair{x appears in front of you and {bfr{Bee{bzes{x $N.", ch, NULL, victim, TO_CHAR);
  act("A {Bc{bol{Bd{x blast of {Wair{x appears in front of $n and {bfr{Bee{bzes{x you!", ch, NULL, victim, TO_VICT);
 
  damage(ch, victim, dam, sn, DAM_COLD, TRUE);
  return;
}

/**********************************************************************
*       Function      : spell_lightningbolt
*       Author        : Swordfish
*       Description   : Air weave rank 2
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_lightningbolt( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  OBJ_DATA *wield_weapon;
  OBJ_DATA *wear_shield;
  OBJ_DATA *wield_dual;
  char buf[MAX_INPUT_LENGTH];
  static char * const numbername [] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty", "lots of"};
  static char * const numberorder [] = {"none existing", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eight", "ninth", "tenth", "eleventh", "twelfth", "thirteenth", "fourteenth", "fifteenth", "sixteenth", "seventeenth", "eighteenth", "nineteenth", "twentieth", "next"};
  int dam = 0;
  int bolts = 1;
  int i = 0;
  bool mainsphere = FALSE;

  /* Is the PC wielding a weaon, or have 2 hands free? */
  wield_weapon = get_eq_char(ch, WEAR_WIELD);
  wield_dual   = get_eq_char(ch, WEAR_SECOND_WIELD);
  wear_shield  = get_eq_char(ch, WEAR_SHIELD);
  
  dam = calculate_weave_dam(sn, ch, 2);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_AIR) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_AIR]/2;
    }
  }

  if (IS_CODER(ch)) {  
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'lightning bolt' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }

  /* Max 2 bolts - 1 per free hand */
  if (!IS_NPC(ch)) {
    bolts = get_skill(ch,sn)/33;
    
    if (bolts < 1)
	 bolts = 1;
  }
  else {
    bolts = ch->level/33;
  }

  /* If one free hand, then release 1 bolt. */
  /* If both hands free, possible 2 if skill high */  
  if ((wield_weapon) || (wear_shield)) {
    bolts = 1;
     
    sprintf(buf, "$n lifts one hand and %s deadly fork%s of {Ylightning{w arcs toward $N.", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
    act(buf, ch, NULL, victim, TO_NOTVICT);
  
    sprintf(buf, "You lift your free hand and %s deadly fork%s of {Ylightning{w arcs toward $N.", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
    act(buf, ch, NULL, victim, TO_CHAR);
  
    sprintf(buf, "$n lift one hand and %s deadly fork%s of {Ylightning{w arcs toward You.", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
    act(buf, ch, NULL, victim, TO_VICT);
     
    damage( ch, victim, dam, sn, DAM_LIGHTNING ,TRUE);
  }
  else {
    sprintf(buf, "$n lifts both hands and %s deadly fork%s of {Ylightning{w arcs toward $N.", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
    act(buf, ch, NULL, victim, TO_NOTVICT);
  
    sprintf(buf, "You lift both your hands and %s deadly fork%s of {Ylightning{w arcs toward $N.", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
    act(buf, ch, NULL, victim, TO_CHAR);
  
    sprintf(buf, "$n lift both hands and %s deadly fork%s of {Ylightning{w arcs toward You.", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
    act(buf, ch, NULL, victim, TO_VICT);
  
    for (i = 1; i <= bolts; i++) {
	 if (bolts > 1 && number_chance(10*i)) {
	   sprintf(buf, "The %s deadly fork of {Ylightning{w misses $N.", i > 20 ? numberorder[20] : numberorder[i]);
	   act(buf, ch,NULL,victim,TO_CHAR);
	   act(buf, ch,NULL,victim,TO_NOTVICT);
	   sprintf(buf, "The %s deadly fork of {Ylightning{w miss you!", i > 20 ? numberorder[20] : numberorder[i]);
	   act(buf, ch, NULL,victim,TO_VICT);
	 }
	 else {
            if (victim->position == POS_DEAD)
            {
            	continue;
            }
	    damage( ch, victim, dam, sn, DAM_LIGHTNING ,TRUE);
	 }
    }
  }
    
  return;
}

/**********************************************************************
*       Function      : spell_call_lightning
*       Author        : Swordfish
*       Description   : Air weave rank 3
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_call_lightning( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  CHAR_DATA *vch;  
  CHAR_DATA *vch_next;
  int dam = 0;
  char buf[MAX_INPUT_LENGTH];
  static char * const numbername [] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty", "lots of"};
  static char * const numberorder [] = {"none existing", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eight", "ninth", "tenth", "eleventh", "twelfth", "thirteenth", "fourteenth", "fifteenth", "sixteenth", "seventeenth", "eighteenth", "nineteenth", "twentieth", "next"};
  int bolts = 1;
  int i = 0;
  bool mainsphere = FALSE;
  
  dam = calculate_weave_dam(sn, ch, 1);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_AIR) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_AIR]/2;
    }
  }
  
  if (IS_CODER(ch)) {
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'call lightning' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }
  
  if (!IS_NPC(ch)) {
    bolts = get_skill(ch,sn)/33;
    
    if (bolts < 1)
	 bolts = 1;
  }

  // If main sphere, and level 150->
  if (ch->main_sphere == SPHERE_AIR && get_level(ch) >= 150)
    bolts++;
  
  /* More dam if outside */
  if (IS_OUTSIDE(ch)) {
    bolts++;
    
    /* Even more */
    if (weather_info.sky >= SKY_RAINING)
	 bolts++;
  }
  
  sprintf(buf, "$n look up to the sky and %s {Ylightning{x fork%s flashes down and strikes at $N.", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
  act(buf, ch, NULL, victim, TO_NOTVICT);
  
  sprintf(buf, "You look up to the sky and %s {Ylightning{x fork%s flashes down and strikes at $N.", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
  act(buf, ch, NULL, victim, TO_CHAR);
  
  sprintf(buf, "$n look up to the sky and %s {Ylightning{x fork%s flashes down towards you!", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
  act(buf, ch, NULL, victim, TO_VICT);
  
  act("{WThe hair on the back of your neck stands on end for a moment...{Ythen...{x", ch, NULL, victim, TO_VICT);
  
  for (i = 1; i <= bolts; i++) {
    if (bolts > 1 && number_chance(10*i)) {
	 sprintf(buf, "The %s deadly fork of {Ylightning{w misses $N.", i > 20 ? numberorder[i] : numberorder[i]);
	 act(buf, ch,NULL,victim,TO_CHAR);
	 sprintf(buf, "The %s deadly fork of {Ylightning{w miss you!", i > 20 ? numberorder[i] : numberorder[i]);
	 act(buf, ch, NULL,victim,TO_VICT);
    }
    else {
         if (victim->position == POS_DEAD)
         {
            continue;
         }
	 damage( ch, victim, number_range(dam/3, dam), sn, DAM_LIGHTNING ,TRUE);
	 
	 if (number_percent() < 5)
	   DAZE_STATE(victim, number_range(1,2) * PULSE_VIOLENCE);
    }
  }
  
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if ( vch->in_room == NULL )
	 continue;
    if (ch == vch)
	 continue;
    if (IS_GHOLAM(vch))
         continue;
    if (vch == victim)
	 continue;
    if (is_same_group(vch, ch)) {
	 //sn = skill_lookup("call lightning");
	 if (IS_SET(ch->talents, TALENT_CLOUDDANCING)) {
	   if (get_skill(ch,sn)+5 >= number_percent()) {
		continue;
	   }
	 }
	 else {
	   if (get_skill(ch,sn) >= number_percent()) {
		continue;
	   }
	 }
    }
    if (!IS_SAME_WORLD(vch, ch))
    	continue;
    if ( vch->in_room == ch->in_room )  {
	 if ( vch != ch )  {

	   if (vch->fighting == ch)
           {
  		for (i = 1; i <= bolts/3; i++) {
    		   if (bolts > 1 && number_chance(10*i)) {
         		sprintf(buf, "The %s deadly fork of {Ylightning{w misses $N.", i > 20 ? numberorder[i] : numberorder[i]);
         		act(buf, ch,NULL,vch,TO_CHAR);
         		sprintf(buf, "The %s deadly fork of {Ylightning{w miss you!", i > 20 ? numberorder[i] : numberorder[i]);
         		act(buf, ch, NULL,vch,TO_VICT);
    		   }
    		   else {
         		   if (vch->position == POS_DEAD)
         		   {
               		      continue;
         		   }
         		   damage( ch, vch, number_range(dam/3, dam), sn, DAM_LIGHTNING ,TRUE);
		
         		   if (number_percent() < 5)
           		      DAZE_STATE(vch, number_range(1,2) * PULSE_VIOLENCE);
    		   }
  		}

           }
	   else
	   if (number_percent() > 33 )  {
  		sprintf(buf, "You look up to the sky and %s {Ylightning{x fork%s flashes down and strikes at $N.", bolts > 20 ? numbername[20] : numbername[bolts], bolts <= 1 ? "" : "s");
  		act(buf, ch, NULL, vch, TO_CHAR);
	        if (vch->position == POS_DEAD)
         	{
            	   continue;
         	}

		damage( ch, vch, number_range(dam/3, dam), sn, DAM_LIGHTNING ,TRUE);
	   }
	 }
	 continue;
    }
	
    if ( vch->in_room->area == ch->in_room->area
	    &&   IS_OUTSIDE(vch)
	    &&   IS_AWAKE(vch) 
	    &&   IS_SET(vch->chan_flags, CHAN_SEEAREAWEAVES))
	    
	 send_to_char( "{YLightning{x flashes in the sky.\n\r", vch );
  }
}

/* ########################################################################## */
/* ## Start Offence Water weave:                                           ## */
/* ## Rank 1: Stream of Water                                              ## */
/* ## Rank 2: Waterballs                                                   ## */
/* ## Rank 3: Blizzard                                                     ## */
/* ########################################################################## */

/**********************************************************************
*       Function      : spell_stream_of_water
*       Author        : Swordfish
*       Description   : Water weave rank 1
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_stream_of_water( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  int dam = 0;
  char buf[MAX_INPUT_LENGTH];
  bool mainsphere = FALSE;
  
  dam = calculate_weave_dam(sn, ch, 3);
  
  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_WATER) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_WATER]/2;
    }
  }
  
  if (IS_CODER(ch)) {
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'stream of water' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }

  act("A {Bs{Wt{Br{We{Ba{Wm{x of {Bwater{x seems to squeeze through the ground in front of $n and blasts toward $N.", ch, NULL, victim, TO_NOTVICT);
  act("A {Bs{Wt{Br{We{Ba{Wm{x of {Bwater{x seems to squeeze through the ground in front of you and blasts toward $N.", ch, NULL, victim, TO_CHAR);
  act("A {Bs{Wt{Br{We{Ba{Wm{x of {Bwater{x seems to squeeze through the ground in front of $n and blasts toward you!", ch, NULL, victim, TO_VICT);
  
  damage(ch, victim, dam, sn, DAM_DROWNING, TRUE);
  return;
}

/**********************************************************************
*       Function      : spell_waterball
*       Author        : Swordfish
*       Description   : Water weave rank 2
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_waterball( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  char buf[MAX_INPUT_LENGTH];
  static char * const numbername [] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"};
  static char * const numberorder [] = {"none existing", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eight", "ninth", "tenth"};
  int dam = 0;
  int waterballs = 1;
  int i = 0;
  bool mainsphere = FALSE;
  
  dam = calculate_weave_dam(sn, ch, 2);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_WATER) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_WATER]/2;
    }
  }

  if (IS_CODER(ch)) {  
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'waterball' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }

  if (!IS_NPC(ch)) {
    waterballs = get_skill(ch,sn)/25;
    
    if (waterballs < 1)
	 waterballs = 1;
  }
  else {
    waterballs = ch->level/25;
  }

  sprintf(buf, "Water starts to seep through the ground and form into shape%s of %s ball%s of {Bwater{x.", waterballs <= 1 ? "" : "s", numbername[waterballs], waterballs <= 1 ? "" : "s");
  act(buf, ch, NULL, NULL, TO_ROOM);
  act(buf, ch, NULL, NULL, TO_CHAR);
  
  sprintf(buf, "%s {Bw{Wa{Bt{We{Br{Wb{Ba{Wl{Bl{W%s{x circle around $n before %s blasts toward $N.", numbername[waterballs], waterballs <= 1 ? "" : "s", waterballs <= 1 ? "it" : "they");
  act(buf, ch, NULL, victim, TO_NOTVICT);

  sprintf(buf, "%s {Bw{Wa{Bt{We{Br{Wb{Ba{Wl{Bl{W%s{x circle around you before %s blasts toward $N.", numbername[waterballs], waterballs <= 1 ? "" : "s", waterballs <= 1 ? "it" : "they");
  act(buf, ch, NULL, victim, TO_CHAR);

  sprintf(buf, "%s {Bw{Wa{Bt{We{Br{Wb{Ba{Wl{Bl{W%s{x circle around $n before %s blasts toward you!", numbername[waterballs], waterballs <= 1 ? "" : "s", waterballs <= 1 ? "it" : "they");
  act(buf, ch, NULL, victim, TO_VICT);

  for (i = 1; i <= waterballs; i++) {
    if (waterballs > 1 && number_chance(10*i)) {
	 sprintf(buf, "The %s {Bw{Wa{Bt{We{Br{Wb{Ba{Wl{Bl{x misses $N.", numberorder[i]);
	 act(buf, ch,NULL,victim,TO_CHAR);
	 act(buf, ch,NULL,victim,TO_NOTVICT);
	 sprintf(buf, "The %s {Bw{Wa{Bt{We{Br{Wb{Ba{Wl{Bl{x miss you!", numberorder[i]);
	 act(buf, ch, NULL,victim,TO_VICT);
    }
    else {
         if (victim->position == POS_DEAD)
         {
            continue;
         }
	 damage( ch, victim, number_range(dam/2, dam), sn, DAM_DROWNING ,TRUE);
    }
  }
}

/**********************************************************************
*       Function      : spell_blizzard
*       Author        : Swordfish
*       Description   : Water weave rank 3
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_blizzard( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  int dam = 0;
  char buf[MAX_INPUT_LENGTH];
  //static char * const numbername [] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"};
  static char * const numbername [] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty", "lots of"};
  static char * const numberorder [] = {"none existing", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eight", "ninth", "tenth", "eleventh", "twelfth", "thirteenth", "fourteenth", "fifteenth", "sixteenth", "seventeenth", "eighteenth", "nineteenth", "twentieth", "next"};
  int icebolts = 1;
  int i = 0;
  bool mainsphere = FALSE;

  if ( !IS_OUTSIDE(ch) && (ch->insanity_points % 5) ) {
    send_to_char( "You must be out of doors.\n\r", ch );
    return;
  }
  
  dam = calculate_weave_dam(sn, ch, 1);

  /* If main sphere, add dam */
  if (!IS_NPC(ch)) {
    if (ch->main_sphere == SPHERE_WATER) {
	 mainsphere = TRUE;
	 dam += ch->perm_sphere[SPHERE_WATER]/2;
    }
  }
  
  if (IS_CODER(ch)) {
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'blizzard' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }
  
  if (!IS_NPC(ch)) {
    icebolts = get_skill(ch,sn)/33;
    
    if (icebolts < 1)
	 icebolts = 1;
  }
  
  if (icebolts > 8)
    icebolts = 8;
  /* More icebolts if bad weather */
  if (weather_info.sky >= SKY_RAINING)
    icebolts++;
  
 
  act("You combine the flows creating deadly {Dcl{Wou{Dds{x of raining {Bi{Wc{Be{Wb{Bo{Wl{Bt{Ws{x.", ch, NULL, NULL, TO_CHAR);
  act("Strange {Dcl{Wou{Dds{x form above $n, raining deadly {Bi{Wc{Be{Wb{Bo{Wl{Bt{Ws{x.", ch, NULL, NULL, TO_ROOM);
  
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if ( vch->in_room == NULL )
	 continue;
    if (ch == vch)
	 continue;
    if (is_same_group(ch, vch))
	 continue;
    if (!IS_SAME_WORLD(ch, vch))
    	continue;
    if ( vch->in_room == ch->in_room ) {
	 for (i = 1; i <= icebolts; i++) {
	   if (vch != NULL) {
	      if (icebolts > 1 && number_chance(10*i)) {
		     sprintf(buf, "The %s deadly {Bi{Wc{Be{Wb{Bo{Wl{Bt{Ws{x miss you!", i > 20 ? numberorder[20] : numberorder[i]);
		     act(buf, ch, NULL,vch,TO_VICT);
	      }
	      else {
	         if (vch->position == POS_DEAD)
         	{
            		continue;
         	}
		damage( ch, vch, number_range(dam/3, dam), sn, DAM_COLD ,TRUE);
	      }
	   }
	  }
    }
   }

  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if ( vch->in_room == NULL )
	 continue;
    if (ch == vch)
	 continue;
    if (!IS_SET(vch->chan_flags, CHAN_SEEAREAWEAVES))
         continue;	 
    if ( vch->in_room->area == ch->in_room->area
	    &&   IS_OUTSIDE(vch)
	    &&   IS_AWAKE(vch) )
	 send_to_char( "Thunder rumble in the sky, and a cold breeze shoot through the air.\n\r", vch );
  }
}

/**********************************************************************
*       Function      : do_studyresidues
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_studyresidues( CHAR_DATA *ch, char *argument )
{
  RESIDUE_DATA *rd;
  char buf [MAX_STRING_LENGTH];
  int sn=0;
  int endurance;
  int chance=0;

  /* only PCs */
  if (IS_NPC(ch))
    return;
    
  if (ch->class != CLASS_CHANNELER)
     return;

  if (!IS_SET(ch->talents, TALENT_RESIDUES) && !IS_IMMORTAL(ch)) {
    send_to_char("You don't know how to read residues.\n\r", ch);
    return;
  }
  
  sn = skill_lookup("study residue");
  
  if (get_skill(ch,sn) < 1) {
    send_to_char("You don't even know how to study residues.\n\r", ch);
    return;
  }
  
  endurance = skill_table[sn].min_endurance;
  
  if (ch->endurance < endurance) {
     send_to_char("You are too tired to concentrate any more!\n\r", ch);
     return;	
  }
  
  if (ch->in_room->residues != NULL) {
   for (rd = ch->in_room->residues; rd != NULL; rd = rd->next) {
    	if (rd->sex != ch->sex)
    	   continue;
	if (skill_table[rd->sn].skill_level[ch->class] > ch->level)
	   continue;
	if (ch->pcdata->learned[rd->sn] > 0)
	   continue;
	if((skill_table[rd->sn].restriction != RES_NORMAL) && (skill_table[rd->sn].restriction != RES_TALENT ) )
	   continue;

	chance = (100 - (get_skill(ch,sn)/10));
	//sprintf(buf, "Chance is %d learned is %d\n\r", chance, get_skill(ch,sn));
	//send_to_char(buf, ch);
	if (number_percent() > chance) {
	  sprintf(buf, "You examine the residues that was woven with flows of %s and learn to weave '%s'!\n\r", (char *)flow_text(rd->sn ,ch), skill_table[rd->sn].name);
	  send_to_char(buf, ch);
	  ch->pcdata->learned[rd->sn] = 1;
	  ch->endurance -= endurance;
	  WAIT_STATE(ch,skill_table[sn].beats);
	  return;
	}
	else {
	  sprintf(buf, "You examine the residues that was woven with flows of %s but can't figure it out.\n\r", (char *)flow_text(rd->sn ,ch));
	  send_to_char(buf, ch);
	  ch->endurance -= endurance;
	  WAIT_STATE(ch,skill_table[sn].beats);
	  return;
	}
    }
  }
  else {
    send_to_char("There are no residues here to study!\n\r", ch);
    return;
  }

   
  send_to_char("There are no more residues here to study for you!\n\r", ch);
  return;
}

/**********************************************************************
*       Function      : spell_bond
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_bond(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  static char * const his_her [] = { "its", "his", "her" };
  
  // Not NPCs
  if (IS_NPC(victim)) {
     send_to_char("You can't bond Mobiles!\r\n",ch);
     return;  	
  }

  // Need to be in group
  if (!is_same_group(ch, victim)) {
     send_to_char("You must be grouped with them.\n\r",ch);
     return;
  }

  // Can't bond while linked
  if (IS_LINKED(ch))
  {
	send_to_char("You can not bond someone while linked.\r\n",ch);
	return;
  }
  
  // Already bonded once?
  if (victim->pcdata->bondedby[0] != '\0') {
    send_to_char("They are already bonded.\n\r",ch);
    return;
  }

  // Ok, connect em..
  strcpy(buf,ch->name);
  victim->pcdata->bondedby = str_dup( buf );
  
  sprintf(buf, "You feel the presense of %s become more and more acute, until %s physical and emotional presence ever-present in the corner of your mind.\n\r", PERS_NAME(victim, ch), his_her[URANGE(0, victim->sex, 2)]);
  send_to_char(buf, ch);  
  sprintf(buf, "Suddenly you become aware of the physical and emotional status of %s through the bond, pocketed in a corner of your mind and leaving you slightly tired and drained.\n\r", PERS_NAME(ch, victim));
  send_to_char(buf, victim);  

  ch->pcdata->bondcount++;
  victim->pcdata->bondcount++;
  victim->pcdata->bondedbysex = ch->sex;
  // Make victim ch's warder
  if (!IS_WARDER(victim) && ch->sex == SEX_FEMALE)
     SET_BIT(victim->merits, MERIT_WARDER);

  // Log it
  sprintf(buf, "%s has bonded %s", ch->name, victim->name);
  log_string(buf);

  //wiznet
  sprintf(buf, "%s weave flows of %s and bond poor %s", ch->name, (char *)flow_text(sn, ch), victim->name);
  wiznet(buf,NULL,NULL,WIZ_CHANNELING,0,0);

  return;
}

/**********************************************************************
*       Function      : do_unbond
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_unbond( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];
  
  victim = get_char_world(ch,argument);

  if (!victim) {
    send_to_char("They aren't here!!\n\r",ch);
    return;
  }

  if (str_cmp(ch->name, victim->pcdata->bondedby) || !IS_TRUSTED(ch,IMMORTAL)) {
    send_to_char("You aren't bonded to them!\n\r",ch);
    return;
  }

  strcpy (buf, "");
  free_string(victim->pcdata->bondedby);
  /* victim->pcdata->bondedby = NULL; */
  victim->pcdata->bondedby = str_dup(buf);
   
  send_to_char("{WYou have been released from your bond.{x\n\r", victim);
  sprintf(buf, "{WYou have released the bond to %s.\n\r", victim->name);
  send_to_char(buf, ch);

  // remove victim as ch's warder
  if (IS_WARDER(victim))
     REMOVE_BIT(victim->merits, MERIT_WARDER);
  
  sprintf(buf, "%s has unbonded %s", ch->name, victim->name);
  log_string(buf);
  
  //wiznet
  sprintf(buf, "%s unbond happy %s", ch->name, victim->name);
  wiznet(buf,NULL,NULL,WIZ_CHANNELING,0,0); 

}

/**********************************************************************
*       Function      : do_bondstatus
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_bondstatus(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *bch;

  send_to_char("{sThrough the physical and emotional bond you sense{x:\n\r", ch);

  for ( bch = char_list; bch != NULL; bch = bch->next ) {
    if (!IS_NPC(bch)) {
      if ( is_bonded_to( ch, bch ) ) {
         sprintf(buf, "%s [ Hp: {R%5d{x End: {G%5d{x ] %s\n\r", get_hit_loc_str(ch, bch), bch->hit, bch->endurance, PERS(bch, ch));
         send_to_char(buf, ch);
      }
    }
  }

  return;
}

// ====================================
// Below is the Disguise code...
// ====================================

/**********************************************************************
*       Function      : is_disguise
*       Author        : Swordfish
*       Description   : check if argument is one of ch's disguise images
*       Parameters    : 
*       Returns       : 
**********************************************************************/
bool is_disguise(CHAR_DATA *ch, char *argument) 
{
  int pos;
  CHAR_DATA *rch;

  if (ch->desc == NULL)
    rch = ch;
  else
    rch = ch->desc->original ? ch->desc->original : ch;
   
  for (pos = 0; pos < MAX_DISGUISE; pos++) {
    if (rch->pcdata->disguise[pos] == NULL)
	 continue;    
    if (!str_cmp(rch->pcdata->disguise[pos], argument))
      return TRUE;
  }
  
  return FALSE;
}

/**********************************************************************
*       Function      : pos_disguise
*       Author        : Swordfish
*       Description   : Finf position in the disguise array to the 
*                       disguise name
*       Parameters    : 
*       Returns       : 
**********************************************************************/
int pos_disguise(CHAR_DATA *ch, char *argument)
{
  int pos;
  CHAR_DATA *rch;
  
  if (ch->desc == NULL)
    rch = ch;
  else
    rch = ch->desc->original ? ch->desc->original : ch;
  
  for (pos = 0; pos < MAX_DISGUISE; pos++) {
    if (rch->pcdata->disguise[pos] == NULL)
	 continue;    
    if (!str_cmp(rch->pcdata->disguise[pos], argument))
      return pos;
  }
  
  return -1;
}

/**********************************************************************
*       Function      : list_disguise
*       Author        : Swordfish
*       Description   : List created disguise images
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void list_disguise(CHAR_DATA *ch)
{ 
  char buf[MAX_STRING_LENGTH];
  int pos=0;
  int disguises=0;
  
  send_to_char("List of disguise images you have created for your self:\n\r", ch);
  for (pos = 0; pos < MAX_DISGUISE; pos++) {
    if (ch->pcdata->disguise[pos] != NULL) {
	 disguises++;
	 sprintf(buf, "[{W%2d{x] %s\n\r", disguises, ch->pcdata->disguise[pos]);
	 send_to_char(buf, ch);
    }
  }
  if (disguises < 1)
    send_to_char("None.\n\r", ch);
  
  return;
}

/**********************************************************************
*       Function      : create_disguise
*       Author        : Swordfish
*       Description   : Create a disguise image and save it.
*                       Must use the weave to set the web.
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void create_disguise(CHAR_DATA *ch, int maxdisguise, char *argument)
{
  CHAR_DATA *nch;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char arg4[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int pos=0;
  int sex;
  int race;
  int level=1;    /* Default */
  struct stat sb;
  int er;
  int sn;
  
  // Set up the args
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);
  argument = one_argument(argument, arg4);
  
  // Check arguments
  if (IS_NULLSTR(arg1) && IS_NULLSTR(arg2) && IS_NULLSTR(arg3) && IS_NULLSTR(arg4) && IS_NULLSTR(argument)) {
    send_to_char("Syntax: disguise new <name> <sex> <race> <level> <appearance>\n\r", ch);
    return;
  }
  
  arg1[0] = UPPER(arg1[0]);
  
  // Check if name already created.
  if (is_disguise(ch, arg1)) {
     send_to_char("You have already created a disguise image with that name.\n\r", ch);
     return;	
  }
  
  // Check name
  if (!check_parse_name(colorstrem(arg1))) {
    send_to_char("Illegal disguise name, try another name.\n\r", ch);
    return;
  }

  // Check if name already exist?
  sprintf(buf, "%s%s", PLAYER_DIR, arg1);
  er = stat(buf, &sb);
  if (er != -1) {
    send_to_char("That name already exists as a player.\n\r", ch);
    return;
  }
  else {
    sprintf(buf, "%s%s", PLAYER_DISGUISE_DIR, arg1);
    er  = stat(buf, &sb);
    if (er != -1) {
	 send_to_char("That name already exists as a player.\n\r", ch);
	 return;
    }
  }
      
  // Check sex
  switch(arg2[0]) {
  case 'm': case 'M': sex = SEX_MALE;    
    break;
  case 'f': case 'F': sex = SEX_FEMALE; 
    break;
  default:
    send_to_char("Illegal sex, try another sex.\n\r", ch);
    return;
  }
    
  // Check race
  race = race_lookup(arg3);
  if (race == 0 || !race_table[race].pc_race || race_table[race].granted) {
    if (!IS_FORSAKEN(ch))
    {
       send_to_char("Illegal race, try another race.\n\r", ch);
       return;
    }
  }

  // Check level
  if (!(level = atoi(arg4))) {
    send_to_char("Illegal level. Level must be a number.\n\r", ch);
    return;
  }
  else {
    if (level < 1 || level > get_level(ch)) {
	 send_to_char("Illegal level. Level must be between 1 and your current level.\n\r", ch);
	 return;
    }
  }
  
  // Check appearnace
  if (argument[0]=='\0' || strlen(argument) > 35 || strlen(argument) < 5) {
    send_to_char("Illegal appearance, try another appearance.\n\r", ch);
    return;
  }
  
  // Find a pos
  for (pos = 0; pos < MAX_DISGUISE; pos++) {
    if (ch->pcdata->disguise[pos] == NULL)
	 break;
  }	 
  
  // Check if more than max
  if (pos >= MAX_DISGUISE || pos >= maxdisguise) {
    send_to_char("You are unable to control any more disguises.\n\r", ch);
    return;
  }

  // Ok, add name as a new disguise
  ch->pcdata->disguise[pos] = str_dup( arg1 );
    
  // Create the new character and turn into it/him/she
  nch         = new_char();
  nch->pcdata = new_pcdata();
  
  // Copy relevant info from "main"
  if (level > LEVEL_HERO-1) {
     nch->level          = LEVEL_HERO-1;
     nch->pcdata->extended_level = level - (LEVEL_HERO-1);
  }
  else
     nch->level         = level;
  
  nch->name             = str_dup( arg1 );
  nch->real_name        = str_dup(ch->name);
  nch->id               = get_pc_id(); // New id for this char
  nch->pcdata->true_sex = ch->pcdata->true_sex;
  nch->pcdata->email    = str_dup(ch->pcdata->email);
  nch->pcdata->lastlog  = str_dup(ch->pcdata->lastlog);
  nch->pcdata->lastsite = str_dup(ch->pcdata->lastsite);
  nch->pcdata->pwd      = str_dup(ch->pcdata->pwd);
  nch->prompt           = str_dup(ch->prompt);
  
  // Copy stats
  for (pos = 0; pos < MAX_STATS; pos ++)
    nch->perm_stat[pos]        = ch->perm_stat[pos];

  // Copy spheres
  for (pos = 0; pos < MAX_SPHERE; pos ++) {
    nch->cre_sphere[pos]       = ch->cre_sphere[pos];
    nch->perm_sphere[pos]      = ch->perm_sphere[pos];
  }

  // Copy skill/weaves if level > 1
  if (level >= 1) {
    for (sn = 0; sn < MAX_SKILL; sn++) {
	 if (skill_table[sn].name == NULL)
	   break;
	 if (skill_table[sn].skill_level[ch->class] <= 0 || skill_table[sn].skill_level[ch->class] > level)
	   continue;
	 if (get_skill(ch,sn) > 0)
	   nch->pcdata->learned[sn] = get_skill(ch,sn);	   
    }
  }

  // If switching to other sex, switch seize->embrace etc.
  // This is a little dirty and not bookwise, but to make a disguise harder to
  // find, we current do it like this.
/*   if (sex != ch->sex) { */
/*     if (sex == SEX_MALE) { */
/* 	 nch->pcdata->learned[gsn_seize]   = ch->pcdata->learned[gsn_embrace]; */
/* 	 nch->pcdata->learned[gsn_embrace] = 0; */
/*     } */
/*     else { */
/* 	 nch->pcdata->learned[gsn_embrace] = ch->pcdata->learned[gsn_seize]; */
/* 	 nch->pcdata->learned[gsn_seize]   = 0; */
/*     } */
	 
/*   } */

  // Set up hp and end
  if (level > 5) {
    nch->max_hit          = ch->max_hit/(get_level(ch)/level);
    nch->hit              = nch->max_hit;
    nch->max_endurance    = ch->max_endurance/(get_level(ch)/level);
    nch->endurance        = nch->max_endurance;
  }

  // Fix hunger, thirst etc.
  nch->pcdata->condition[COND_HUNGER] = 100;
  nch->pcdata->condition[COND_THIRST] = 100;
  nch->pcdata->condition[COND_FULL]   = 100;

  /* Set up base hit locations */
  nch->hit_loc[LOC_LA] = get_max_hit_loc(nch, LOC_LA);
  nch->hit_loc[LOC_LL] = get_max_hit_loc(nch, LOC_LL);
  nch->hit_loc[LOC_HE] = get_max_hit_loc(nch, LOC_HE);
  nch->hit_loc[LOC_BD] = get_max_hit_loc(nch, LOC_BD);
  nch->hit_loc[LOC_RA] = get_max_hit_loc(nch, LOC_RA);
  nch->hit_loc[LOC_RL] = get_max_hit_loc(nch, LOC_RL);

  nch->talents          = ch->talents;
  nch->flaws            = ch->flaws;
  nch->merits           = ch->merits;
  nch->main_sphere      = ch->main_sphere;
  nch->world            = ch->world;
  nch->act              = ch->act;
  nch->comm             = ch->comm;
  //nch->forsaken         = ch->forsaken;
  nch->ic_flags         = ch->ic_flags;  
  if (IS_RP(nch))
    REMOVE_BIT(nch->ic_flags, IC_RP);
  nch->in_room          = ch->in_room;
  nch->pcdata->ictitle  = str_dup( "" );
  nch->pcdata->imm_info = str_dup( "" );
  
  // Fix title to look like "new" character
  if (sex == SEX_MALE)
    set_title(nch,"probably wants to change his title.");
  else
    set_title(nch,"probably wants to change her title.");
  
  // Fill in from disguise creation
  nch->sex                = sex;
  nch->race               = race;
  nch->pcdata->appearance = str_dup(argument);
  
  // Set default colors
  default_colour(nch);
  
  // Toggle on that he/she is disguised
  if (!IS_DISGUISED(nch))
    SET_BIT(nch->app, APP_DISGUISED);
  
  // Save the character
  save_char_obj( nch, FALSE );
  save_char_obj( ch, FALSE );

  // Make a note
  sprintf(buf,
		"Name    : %s\n\r"
		"RName   : %s\n\r"
		"Race    : %s\n\r"
		"Sex     : %s\n\r"
		"Level   : %d\n\r",
		nch->name,
		ch->name,
		capitalize(race_table[nch->race].name),
		nch->sex == 1 ? "Male" : "Female",
		level);		  
  sprintf(buf2,
		"Class   : %s\n\r"
		"App     : %s\n\n\r"
		"Stats   : %d Str, %d Int, %d Wis, %d Dex, %d Con\n\r"
		"Spheres : %d/%d A, %d/%d E, %d/%d F, %d/%d S, %d/%d W (M: %s)\n\r"
		"Merits  : %s\n\r"
		"Flaws   : %s\n\r"
		"Talents : %s\n\r",
		capitalize(class_table[nch->class].name),
		nch->pcdata->appearance,
		get_curr_stat(nch,STAT_STR), get_curr_stat(nch,STAT_INT), 
		get_curr_stat(nch,STAT_WIS), get_curr_stat(nch,STAT_DEX),
		get_curr_stat(nch,STAT_CON),			  
		nch->perm_sphere[SPHERE_AIR],    nch->cre_sphere[SPHERE_AIR],
		nch->perm_sphere[SPHERE_EARTH],  nch->cre_sphere[SPHERE_EARTH],
		nch->perm_sphere[SPHERE_FIRE],   nch->cre_sphere[SPHERE_FIRE],
		nch->perm_sphere[SPHERE_SPIRIT], nch->cre_sphere[SPHERE_SPIRIT],
		nch->perm_sphere[SPHERE_WATER],  nch->cre_sphere[SPHERE_WATER],
		nch->main_sphere != -1 ? sphere_table[nch->main_sphere].name : "none",
		background_flag_string(merit_table, nch->merits),
		background_flag_string(flaw_table, nch->flaws),
		background_flag_string(talent_table, nch->talents));
  strcat(buf, buf2);
  sprintf(buf2, "I am a new disguise! (%s)", ch->name);
  make_note("Newplayers", nch->name, "Admin", buf2, 56, buf);

  free_char( nch );

  if (ch->class == CLASS_CHANNELER)
  {
     send_to_char("New disguise image created. Use the weave to set the web of disguise on your self.\n\r", ch);  
  }
  else
  {
     send_to_char("New disguise image created and ready to be used.\n\r", ch);  
  }
  
  // Log it
  sprintf(buf, "%s has created a new disguise image with the name %s (level=%d).", ch->name, arg1, level);
  wiznet(buf, ch, NULL, WIZ_CHANNELING, 0, get_trust(ch));
  log_string(buf);

  return;
}

/**********************************************************************
*       Function      : return_from_disguise
*       Author        : Swordfish
*       Description   : Return to original character from a disguise
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void return_from_disguise(CHAR_DATA *ch)
{
  DESCRIPTOR_DATA *d=NULL;
  CHAR_DATA *nch;
  ROOM_INDEX_DATA *in_room;
  bool found=FALSE;

  if (!IS_DISGUISED(ch)) {
    send_to_char("You are not in disguise.\n\r", ch);
    return;
  }

  // Make sure the real name is set...
  if (IS_NULLSTR(ch->real_name)) {
    send_to_char("Unable to return to your normal image. Please report this to the immortals. (Ref: 83423)\n\r", ch);
    return;
  }

  // We need to keep this, cause we don't want to log the char
  d = ch->desc;
  in_room = ch->in_room;

  // Messages
  if (ch->class == CLASS_CHANNELER)
  {
     send_to_char("You release the web of disguise and return to your normal image.\n\r", ch);
     act("The air ripples around $n for a slight moment.", ch, NULL, NULL, TO_ROOM);
  }
  else
  {
     send_to_char("You peel off your disguise.\n\r", ch);
     act("$n removes $s disguise.", ch, NULL, NULL, TO_ROOM);
  }
  
  // Save and extract the disguise before loading real char
  if (IS_AFFECTED(ch,AFF_CHANNELING)) {
    do_function(ch, &do_unchannel, "" );
  }
  save_char_obj( ch, FALSE );
  extract_char(ch, TRUE, FALSE);
  
  found = load_char_obj( d, ch->real_name, FALSE );

  if (!found) {
    send_to_char("Unable to remove the disguise. Please report this to the immortals.\n\rLog out and back in with original character to fix.\n\r", ch);
    return;
  }

  // Descriptor tricks
  d->character->next = char_list;
  char_list          = d->character;
  nch                = d->character;
  nch->desc          = d;

  char_to_room(nch, in_room);

  //Toggle on that is disguised
  if (IS_DISGUISED(nch))
    REMOVE_BIT(nch->app, APP_DISGUISED);
  
  return;  
}

/**********************************************************************
*       Function      : load_disguise
*       Author        : Swordfish
*       Description   : Change into an already created disguise image
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void load_disguise(CHAR_DATA *ch, char *argument)
{
  DESCRIPTOR_DATA *d=NULL;
  CHAR_DATA *nch;
  ROOM_INDEX_DATA *in_room;
  bool found=FALSE;
  struct stat sb;
  int er;
  char filename[MAX_INPUT_LENGTH];

  if (IS_DISGUISED(ch)) {
    send_to_char("You are already in disguise.\n\r", ch);
    return;
  }

  argument[0] = UPPER(argument[0]);

  // Check that disguise file exists
  sprintf(filename, "%s%s", PLAYER_DISGUISE_DIR, argument);
  er = stat(filename, &sb);
  if (er == -1) {
    send_to_char("Unable to change into the disguise. Disguise profile is missing.\n\r"
			  "Please report this to the immortals.\n\r", ch);
    return;
  }

  // We need to keep this, cause we don't want to log the char
  d = ch->desc;
  in_room = ch->in_room;

  // Messages
  if (ch->class == CLASS_CHANNELER)
  {
     send_to_char("You set a web of disguise on your self.\n\r", ch);  
     act("The air ripples around $n for a slight moment.", ch, NULL, NULL, TO_ROOM);
  }
  else
  {
     send_to_char("You put on your disguise.\n\r", ch);  
     act("$n takes a few moments to change outfits.", ch, NULL, NULL, TO_ROOM);
  }
  
  // Save and extract the real char before loading disguise
  if (IS_AFFECTED(ch,AFF_CHANNELING)) {
    do_function(ch, &do_unchannel, "" );
  }
  save_char_obj( ch, FALSE );
  extract_char(ch, TRUE, FALSE);  
  
  found = load_char_obj( d, argument, TRUE );

  if (!found) {
    send_to_char("Unable to change into the disguise. Please report this to the immortals.\n\r", ch);
    return;
  }

  // Descriptor tricks
  d->character->next = char_list;
  char_list          = d->character;  
  nch                = d->character;
  nch->desc          = d;
  
  char_to_room(nch, in_room);
  
  // Toggle on that is disguised
  if (!IS_DISGUISED(nch))
    SET_BIT(nch->app, APP_DISGUISED);
   
  return;  
}

/**********************************************************************
*       Function      : do_disguise
*       Author        : Swordfish
*       Description   : Main disguise function for:
*                       1) set up a new disguise to be used with the weave
*                       2) List and maintain disguises
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_disguise(CHAR_DATA *ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int pos=0;
  int sn=0;
  int max_disguise=1;
  
  if (IS_NPC(ch))
    return;
  
  argument = one_argument(argument, arg1);

  sn = skill_lookup("disguise");
  
  // Forsaken and imms even without the weave
  if (!IS_FORSAKEN(ch) && !IS_IMMORTAL(ch) && !IS_DISGUISED(ch) && get_skill(ch,sn) < 1) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (get_skill(ch,sn) < 1 && !IS_DISGUISED(ch)) {
    send_to_char("You don't know how to disguise your self.\n\r", ch);
    return;
  }
  
  if (ch->class == CLASS_CHANNELER)
  {
     if (!IS_DISGUISED(ch))
	max_disguise = get_skill(ch,sn)/10;
  }
  else
  {
	max_disguise = 2;
  }
  
  // Syntax
  if (IS_NULLSTR(arg1)) {
    send_to_char("Syntax: disguise new <name> <sex> <race> <level> <appearance>\n\r", ch);
     if (ch->class != CLASS_CHANNELER)
    {
    	send_to_char("        disguise use name\n\r", ch);
    }
    send_to_char("        disguise list\n\r", ch);
    send_to_char("        disguise delete <name>\n\r", ch);
    send_to_char("        disguise release\n\r", ch);
    return;
  }


  if (!str_cmp(arg1, "use")) {
     argument = one_argument(argument, arg2);
     if (is_disguise(ch, arg2)) 
     {
       if (IS_DISGUISED(ch)) 
       {
	    send_to_char("You are already disguised.\n\r", ch);
	    return;
       }
       load_disguise(ch, arg2);
       return;
     }
     else {
       send_to_char("You don't have any disguise images with that name created.\n\r", ch);
       return;
     }
  }
  
  // Show a list of all current disguises....
  if (!str_cmp(arg1, "list")) {

    if (IS_DISGUISED(ch)) {
	 send_to_char("You can't use this option while in disguise.\n\r", ch);
	 return;
    }    
    list_disguise(ch);
    return;
  }
     
  // New disguise
  if (!str_cmp(arg1, "new")) {
    if (IS_DISGUISED(ch)) {
	 send_to_char("You can't use this option while in disguise.\n\r", ch);
	 return;
    }
    create_disguise(ch, max_disguise, argument);
    return;
  }

  // Delete disguise
  if (!str_cmp(arg1, "delete")) {
    if (IS_DISGUISED(ch)) {
	 send_to_char("You can't use this option while in disguise.\n\r", ch);
	 return;
    }
    
    if (!is_disguise(ch, argument)) {
	 send_to_char("You don't have any disguises with that name.\n\r", ch);
	 return;
    }

    pos = pos_disguise(ch, argument);    
    free_string(ch->pcdata->disguise[pos]);
    ch->pcdata->disguise[pos] = NULL;
    sprintf(buf, "You erase %s as one of your disguise images.\n\r", capitalize(argument));
    send_to_char(buf, ch);
    sprintf(buf, "%s%s", PLAYER_DISGUISE_DIR, capitalize( argument ) );
    unlink(buf);
    save_char_obj(ch, FALSE);

    // Make a note that erased
    sprintf(buf, "%s has erased %s as one of the disguise images.\n\r",
		  ch->name, capitalize(argument));
    sprintf(buf2, "I am a dead disguise! (%s)", ch->name);
    make_note("Newplayers", capitalize(argument), "Admin", buf2, 56, buf);
		  

    return;
  }    

  // Release a disguise, return to your self
  if (!str_cmp(arg1, "release")) {
    return_from_disguise(ch);
    return;
  }

  // Syntax again
  send_to_char("Syntax: disguise new <name> <sex> <race> <level> <appearance>\n\r", ch);
  send_to_char("        disguise list\n\r", ch);
  send_to_char("        disguise delete <name>\n\r", ch);
  send_to_char("        disguise release\n\r", ch);
  return;
}

/**********************************************************************
*       Function      : spell_disguise
*       Author        : Swordfish
*       Description   : The acctual weave to set a disguise
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_disguise( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{

  if (IS_NPC(ch))
    return;

  /* Validate syntax */
  if (IS_NULLSTR(cast_arg2)) {
    send_to_char("Syntax: weave disguise <disguisename>\n\r", ch);
    send_to_char("        weave disguise release\n\r", ch);
    return;
  }

  if (!str_cmp(cast_arg2, "release")) {
    return_from_disguise(ch);
    return;
  }

  if (is_disguise(ch, cast_arg2)) {
    if (IS_DISGUISED(ch)) {
	 send_to_char("You start to combine the flows, but are unable to finish it while already having a disguise set on your self.\n\r", ch);
	 return;
    }
    
    load_disguise(ch, cast_arg2);
    return;
  }
  else {
    send_to_char("You don't have any disguise images with that name created.\n\r", ch);
    return;
  }
  
  return;
}


// ====================================
// Below is the ward code...
// ====================================


/**********************************************************************
*       Function      : spell_dreamward
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_dreamward( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  double strength_mod=0;
  
  if (IS_NPC(ch))
    return;
  
  if (IS_AFFECTED(victim, AFF_DREAMWARD)) {
    send_to_char("You already have shielded your dreams.\n\r", ch);
    return;
  }  

  if (!IS_SET(victim->world, WORLD_NORMAL)) {
    send_to_char("You start to combine the flows but it seems to be unable to form them here.\n\r", ch);
    return;
  }

  strength_mod = (double)(ch->holding/(double)get_curr_op(ch));
  if (strength_mod > 1)
    strength_mod = 1;
  
  af.where	   = TO_AFFECTS;
  af.casterId     = ch->id;
  af.type         = sn;
  af.level        = level;
  af.duration     = SUSTAIN_WEAVE;
  af.location     = APPLY_NONE;
  if (ch->main_sphere == SPHERE_SPIRIT)
    af.modifier     = UMAX(1, ((ch->perm_sphere[SPHERE_SPIRIT] + get_skill(ch,sn)/5 + 20) * strength_mod));
  else	 
    af.modifier     = UMAX(1, ((ch->perm_sphere[SPHERE_SPIRIT] + get_skill(ch,sn)/5) * strength_mod));
  af.bitvector    = AFF_DREAMWARD;
  affect_to_char(victim, &af );
  
  send_to_char("You set a web of spirit to shield your dreams.\n\r", ch);
  
  return;
}

int cnt_eavesdrop(CHAR_DATA *ch)
{
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  AFFECT_DATA *pWeave;
  unsigned int vnum;
  int num=0;

  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->weave_cnt <= 0)
	 continue;
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   for (pWeave = pRoom->weaves; pWeave != NULL; pWeave = pWeave->next) {		
		if (pWeave->casterId != ch->id)
		  continue;
		
		num++;
	   }
	 }
    }
  }
  
  return (num);
}

/**********************************************************************
*       Function      : spell_eavesdrop
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_eavesdrop( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  AFFECT_DATA      wd;
  ROOM_INDEX_DATA *from_location;
  ROOM_INDEX_DATA *to_location;
  EXIT_DATA       *pexit;
  int              direction;
  double           strength_mod=0;
  char             buf[MSL];
  
  if (IS_NPC(ch))
    return;

  /* Validate syntax */
  if (IS_NULLSTR(cast_arg2)) {
    send_to_char("Syntax: weave 'eavesdrop' <direction>\n\r", ch);
    return;
  }
  
  /* Only 1 eavesdrop at a time */
  if (!IS_IMMORTAL(ch) && (cnt_eavesdrop(ch) > 0)) {
    send_to_char("You are not strong enough to control any more eavesdrop flows.\n\r", ch);
    return;
  }
    
  if (!IS_SET(ch->world, WORLD_NORMAL)) {
    send_to_char("You start to combine the flows but it seems to be unable to form them here.\n\r", ch);
    return;
  }

  from_location = ch->in_room;
  
  if ((direction = find_exit(ch, cast_arg2)) != -1) {
    if ((pexit = from_location->exit[direction]) != NULL
	   && ( to_location = pexit->u1.to_room ) != NULL
	   && can_see_room(ch, pexit->u1.to_room)) {

	 if (IS_WARDED(to_location, WARD_EAVESDROP)) {
	   send_to_char("You start to combine the flows but it seems to be impossible to form them upon a ward against this!\n\r", ch);
	   return;
	 }
  
	 strength_mod = (double)(ch->holding/(double)get_curr_op(ch));
	 if (strength_mod > 1)
	   strength_mod = 1;
	 
	 wd.where	 = TO_AFF_ROOM;
	 wd.casterId     = ch->id;
	 wd.type         = sn;
	 wd.type_learned = get_skill(ch, sn);
	 wd.sex          = ch->sex;
	 wd.world        = ch->world;
	 wd.level        = get_level(ch);
	 wd.duration     = SUSTAIN_WEAVE;
	 wd.location     = APPLY_NONE;  
	 if (ch->main_sphere == SPHERE_AIR)
	   wd.modifier     = UMAX(1, ((ch->perm_sphere[SPHERE_AIR] + get_skill(ch,sn)/5 + 20) * strength_mod));
	 else	 
	   wd.modifier     = UMAX(1, ((ch->perm_sphere[SPHERE_AIR] + get_skill(ch,sn)/5) * strength_mod));
	 wd.bitvector    = RAFF_EAVESDROP;
	 
	 weave_to_room(to_location, &wd );
	 
	 sprintf(buf, "You set a web of %s to eavesdrop on the room %sward.\n\r", (char *)flow_text(sn, ch), dir_name[direction]);
	 send_to_char(buf, ch);

   	 
	 return;
    }
  }
  
  send_to_char("You don't see exit in that direction.\n\r", ch);
  return;

}

/**********************************************************************
*       Function      : spell_eavesdropward
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_eavesdropward( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  WARD_DATA wd;
  CHAR_DATA *vch;
  double strength_mod=0;
  char buf[MSL];
  
  if (IS_NPC(ch))
    return;
  
  if (!IS_SET(ch->world, WORLD_NORMAL)) {
    send_to_char("You start to combine the flows but it seems to be unable to form them here.\n\r", ch);
    return;
  }

  strength_mod = (double)(ch->holding/(double)get_curr_op(ch));
  if (strength_mod > 1)
    strength_mod = 1;
  

  wd.casterId     = ch->id;
  wd.sn           = sn;
  wd.sn_learned   = get_skill(ch, sn);
  wd.world        = ch->world;
  wd.sex          = ch->sex;
  wd.level        = get_level(ch);
  wd.duration     = SUSTAIN_WEAVE;
  
  if (ch->main_sphere == SPHERE_AIR)
    wd.strength     = UMAX(1, ((ch->perm_sphere[SPHERE_AIR] + get_skill(ch,sn)/5 + 20) * strength_mod));
  else	 
    wd.strength     = UMAX(1, ((ch->perm_sphere[SPHERE_AIR] + get_skill(ch,sn)/5) * strength_mod));
  wd.bitvector    = WARD_EAVESDROP;
  
  ward_to_room(ch->in_room, &wd );
  
  sprintf(buf, "You set a web of %s to ward this room against eavesdropping.\n\r", (char *)flow_text(sn, ch));
  send_to_char(buf, ch);

  if (is_room_weave_set(ch->in_room, RAFF_EAVESDROP)) 
  {
      AFFECT_DATA *pWeave;
      for (pWeave = ch->in_room->weaves; pWeave != NULL; pWeave = pWeave->next) 
      {
	    vch=NULL;
	    vch = get_charId_world(ch, pWeave->casterId);
	    if (vch)
	    {
		    if (IS_FORSAKEN(vch))
                    {
			    send_to_char("You feel someone try and cut off your eavesdropping, but your skill with the weave keeps it in place.\r\n",vch);
                    }
		    else
                    {
		       send_to_char("You feel your eavesdrop weave sliced as a ward against eavesdropping is put in place.\r\n",vch);
		       room_weave_remove( ch->in_room, pWeave );
		    }
	    }	
      }
      
   }
  return;
}

/**********************************************************************
*       Function      : spell_shadowspawn_ward
*       Author        : Swordfish
*       Description   : 
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_shadowspawn_ward( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  WARD_DATA wd;
  double strength_mod=0;
  char buf[MSL];
  
  if (IS_NPC(ch))
    return;
  
  if (!IS_SET(ch->world, WORLD_NORMAL)) {
    send_to_char("You start to combine the flows but it seems to be unable to form them here.\n\r", ch);
    return;
  }
  
  strength_mod = (double)(ch->holding/(double)get_curr_op(ch));
  if (strength_mod > 1)
    strength_mod = 1;
  
  wd.casterId     = ch->id;
  wd.sn           = sn;
  wd.sn_learned   = get_skill(ch, sn);
  wd.world        = ch->world;
  wd.sex          = ch->sex;
  wd.level        = get_level(ch);
  wd.duration     = SUSTAIN_WEAVE;
  
  if (ch->main_sphere == SPHERE_FIRE)
    wd.strength     = UMAX(1, ((ch->perm_sphere[SPHERE_FIRE] + get_skill(ch,sn)/5 + 20) * strength_mod));
  else	 
    wd.strength     = UMAX(1, ((ch->perm_sphere[SPHERE_FIRE] + get_skill(ch,sn)/5) * strength_mod));

  wd.bitvector    = WARD_SHADOWSPAWN;
  
  ward_to_room(ch->in_room, &wd );
  
  sprintf(buf, "You set a web of %s to ward this room against shadowspawn.\n\r", (char *)flow_text(sn, ch));
  send_to_char(buf, ch);
  
  return;
}

/**********************************************************************
*       Function      : spell_light_ward
*       Author        : Zandor
*       Description   : Wards the room against shadows (no shadowtravel to and no sneaking or hiding in)
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_light_ward( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  WARD_DATA wd;
  double strength_mod=0;
  char buf[MSL];

  if (IS_NPC(ch))
    return;

  if (!IS_SET(ch->world, WORLD_NORMAL)) {
    send_to_char("You start to combine the flows but it seems to be unable to form them here.\n\r", ch);
    return;
  }

  strength_mod = (double)(ch->holding/(double)get_curr_op(ch));
  if (strength_mod > 1)
    strength_mod = 1;

  wd.casterId     = ch->id;
  wd.sn           = sn;
  wd.sn_learned   = get_skill(ch, sn);
  wd.world        = ch->world;
  wd.sex          = ch->sex;
  wd.level        = get_level(ch);
  wd.duration     = SUSTAIN_WEAVE;

  if (ch->main_sphere == SPHERE_FIRE)
    wd.strength     = UMAX(1, ((ch->perm_sphere[SPHERE_FIRE] + get_skill(ch,sn)/5 + 20) * strength_mod));
  else
    wd.strength     = UMAX(1, ((ch->perm_sphere[SPHERE_FIRE] + get_skill(ch,sn)/5) * strength_mod));

  wd.bitvector    = WARD_LIGHT;

  ward_to_room(ch->in_room, &wd );

  sprintf(buf, "You set a web of %s to ward this room against shadowspawn.\n\r", (char *)flow_text(sn, ch));
  send_to_char(buf, ch);



  CHAR_DATA *ich;

  act("A {rhot{x dampening {Wwind{x starts to conjure around $n", ch, NULL, NULL, TO_ROOM);
  send_to_char("You draw upon the elements of {Rfire{x, {Bwater{x and {Wair{x and create a revealing wind of {Gglowing{x dust.\n\r", ch);

  for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room ) {
    if (ich->invis_level > 0)
         continue;

    if ( ich == ch) /* || saves_spell( level, ich,DAM_OTHER) ) */
         continue;

    // Not on fades
    affect_strip ( ich, gsn_invis                       );
    affect_strip ( ich, gsn_mass_invis          );
    affect_strip ( ich, gsn_sneak                       );
    affect_strip ( ich, gsn_hide );
    REMOVE_BIT   ( ich->affected_by, AFF_HIDE   );
    REMOVE_BIT   ( ich->affected_by, AFF_INVISIBLE      );
    REMOVE_BIT   ( ich->affected_by, AFF_SNEAK  );
    REMOVE_BIT   ( ich->affected_by, AFF_CAMOUFLAGE     );
    
    if (!IS_NPC(ich)) {
    	ich->pcdata->next_hide = current_time + 60;
    }
    act( "$n is revealed by a {WBright{x light!", ich, NULL, NULL, TO_ROOM );
    send_to_char( "You are revealed by a {WBright{x light!\n\r", ich );
  }
    
  return;
}

/**********************************************************************
*       Function      : spell_autokiller
*       Author        : Swordfish
*       Description   : Immortal play weave (The weave Rand used in the stone of Tear)
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void spell_autokiller( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim=NULL;
  CHAR_DATA *victim_next=NULL;
  unsigned int room_vnum=0;
  AFFECT_DATA af;
  char buf[MSL];
  bool world=FALSE;

  // IMM only for now
  if (!IS_IMMORTAL(ch))
    return;

  /* Validate syntax */
  if (IS_NULLSTR(cast_arg2)) {
    send_to_char("Syntax: weave autokiller mob         [world]\n\r", ch);
    send_to_char("        weave autokiller pc          [world]\n\r", ch);
    send_to_char("        weave autokiller shadowspawn [world] \n\r", ch);
    return;
  }

  if (str_cmp(cast_arg2, "mob") && str_cmp(cast_arg2, "pc") && str_cmp(cast_arg2, "shadowspwan")) {
    send_to_char("Syntax: weave autokiller mob         [world] \n\r", ch);
    send_to_char("        weave autokiller pc          [world]\n\r", ch);
    send_to_char("        weave autokiller shadowspawn [world]\n\r", ch);
    return;
  }

  if (IS_IMMORTAL(ch) && !str_cmp(cast_arg3, "world"))
    world=TRUE;

  sprintf(buf, "You combine flows of %s into a deadly {Dcl{Wou{Dd{x and release it!", (char *)flow_text(sn, ch));
  act(buf, ch,NULL,victim,TO_CHAR);

  // Loop
  for (victim = char_list; victim != NULL; victim = victim_next) {
    victim_next = victim->next;

    if (!world)
	 if (ch->in_room->area != victim->in_room->area)
	   continue;

    if (IS_IMMORTAL(victim))
	 continue;

    if (IS_GHOLAM(victim))
	continue;
    
    if (!str_cmp(cast_arg2, "pc") && IS_NPC(victim))
	 continue;
    
    if (!str_cmp(cast_arg2, "mob") && !IS_NPC(victim))
	 continue;
	 
    // Don't kill mounts	 
    if (!str_cmp(cast_arg2, "mob") && victim->mount != NULL)
	 continue;

    // Don't kill pets
    if (!str_cmp(cast_arg2, "mob") && victim->pet != NULL)
	 continue; 
    
    if (!str_cmp(cast_arg2, "shadowspawn") && !IS_SHADOWSPAWN(victim))
	 continue;	
    
    if (room_vnum != victim->in_room->vnum) {
	 room_vnum =  victim->in_room->vnum;
	 act("{WStreams{x of {Ylightning{x shoot into the room from everywhere!", victim, NULL, NULL, TO_ROOM);
	 act("{WStreams{x of {Ylightning{x shoot into the room from everywhere!", victim, NULL, NULL, TO_CHAR);
    }
    
    if (IS_NPC(victim)) {
	 act("A {Wstream{x of {Ylightning{x finds you!", victim, NULL, NULL, TO_CHAR);
	 act("A {Wstream{x of {Ylightning{x finds $n!", victim, NULL, NULL, TO_ROOM);
	 death_cry( victim );
	 extract_char( victim, TRUE, FALSE );
    }
    else {
	 act("A {Wstream{x of {Ylightning{x finds you!", victim, NULL, NULL, TO_CHAR);
	 act("A {Wstream{x of {Ylightning{x finds $n!", victim, NULL, NULL, TO_ROOM);

	 stop_fighting( victim, TRUE );
	 
	 victim->hit		= -10;
	 victim->endurance	= -10;
	 victim->position   = POS_SLEEPING;
	 
	 af.where     = TO_AFFECTS;
	 af.casterId  = victim->id;
	 af.type      = gsn_sap; 
	 af.level     = victim->level;
	 af.duration  = 4;
	 af.location  = APPLY_NONE;
	 af.modifier  = 0;
	 af.bitvector = AFF_SAP;
	 affect_join( victim, &af );
    }    
  }    
}

void spell_keep( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  char buf[MSL];
  OBJ_DATA *obj = (OBJ_DATA *) vo;
  int duration=0;
  double strength_mod=0;
  
  if (obj->wear_loc != -1) {
    send_to_char("You need to carry the item before you are able to set the flows on it.\n\r", ch);
    return;
  }
  
  strength_mod = (double)(ch->holding/(double)get_curr_op(ch));
  if (strength_mod > 1)
    strength_mod = 1;

  duration = (get_skill(ch, sn)/2) * strength_mod;

    //KEEPING talent?
    if (!IS_SET(ch->talents, TALENT_KEEPING)) {
	duration = duration / 2;
    }
  
  // Not started to tick yet
  if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH)) {
    obj->timer = number_range(duration/2,duration);
    REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);
    sprintf(buf, "You combine flows of %s into %s and set a weave of protection on $p that will keep it fresh for %d hours.", (char *)flow_text(sn, ch), skill_table[sn].name, duration);
    act(buf, ch, obj, NULL, TO_CHAR);
    return;
  }
  
  // ticking
  if (obj->timer <= 0) {
    act("You start to combine the flows but realize that $p never will perish.", ch, obj, NULL, TO_CHAR);
    return;
  }
  
  sprintf(buf, "You combine flows of %s into %s and set a weave of protection on $p that will keep it fresh for %d hours.", (char *)flow_text(sn, ch), skill_table[sn].name, duration);
  act(buf, ch, obj, NULL, TO_CHAR);
  
  obj->timer = duration;

  return;
}

void spell_mirror_of_mists( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  //CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  char buf[MSL];
  int sn_skill;
  char illusion_app[MSL];

  if (IS_NPC(ch))
    return;
    
  sn_skill = get_skill(ch,sn)/5;
  
  if (IS_NULLSTR(cast_arg2)) {
    send_to_char("Syntax: weave 'mirror of mists' <illusion appearance>\n\r", ch);
    send_to_char("        {WYou need to have the illusion appearance quoted like '{YA tall man{W'{x\n\r", ch);
    return;
  }
  
  if (IS_ILLUSIONAPP(ch)) {
    send_to_char("You start to combine the flows but realize you already have set a mirror of mists on your self.\n\r", ch);
    return;
  }

  if (IS_DISGUISED(ch)) {
    send_to_char("You start to combine the flows, but are unable to finish it while already having a disguise set on your self.\n\r", ch);
    return;
  }
  
  if (strlen(cast_arg2) > 35 || strlen(cast_arg2) < 5) {
    send_to_char("Illusion appearance must be between 5 and 35 characters long (no colors allowed).\n\r", ch);
    return;
  }
  
  //Harder to make a illusion that is not close to your true app
  if (ch->sex == SEX_MALE) {
    if (strstr(cast_arg2, "man") != NULL) {
	 if (number_percent() > sn_skill) {
	   send_to_char("You try to combine the flows but the illusion is difficult when not close to your true appearance.\n\r", ch);
	   return;
	 }
    }
  }
  else if (ch->sex == SEX_FEMALE) {
    if (strstr(cast_arg2, "woman") != NULL) {
	 if (number_percent() > sn_skill) {
	   send_to_char("You try to combine the flows but the illusion is difficult when not close to your true appearance.\n\r", ch);
	   return;
	 }
    }
  }

  send_to_char("You combine flows into mirror of mists and set the illusion on your self.\n\r", ch);
  act("The air ripples around $n for a slight moment.", ch, NULL, NULL, TO_ROOM);
    
  af.where        = TO_APP;
  af.casterId     = ch->id;
  af.type         = sn;
  af.level        = level;
  af.duration     = SUSTAIN_WEAVE;
  af.location     = APPLY_NONE;
  af.modifier     = (ch->holding/10) + get_skill(ch,sn);
  af.bitvector    = APP_ILLUSION;
  affect_to_char( ch, &af );
    
  free_string(ch->pcdata->illusion_appearance);
  sprintf(illusion_app, "%s", colorstrem(capitalize(cast_arg2)));
  ch->pcdata->illusion_appearance = str_dup(illusion_app);
  
  //Log it
  sprintf(buf, "%s used mirror of mists and set illusion appearance to '%s'.", ch->name, illusion_app);
  wiznet(buf, ch, NULL, WIZ_CHANNELING, 0, get_trust(ch));
  log_string(buf);
    
  return;
}

void spell_giant_size( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  char buf[MSL];
  AFFECT_DATA af;

  if (IS_NPC(ch))
    return;
  
  if (IS_GIANTILLUSION(ch)) {
    send_to_char("You start to combine the flows but realize you already have set this illusion on your self.\n\r", ch);
    return;
  }
  
  af.where        = TO_APP;
  af.casterId     = ch->id;
  af.type         = sn;
  af.level        = level;
  af.duration     = SUSTAIN_WEAVE;
  af.location     = APPLY_NONE;
  af.modifier     = (ch->holding/10) + get_skill(ch,sn);
  af.bitvector    = APP_SIZE_ILLUSION;
  affect_to_char( ch, &af );  
  
  sprintf(buf, "You combine flows of %s into '%s' and set a illusion on your self.\n\r", (char *)flow_text(sn, ch), skill_table[sn].name); 
  send_to_char(buf, ch);

  act("The air ripples around $n for a slight moment.", ch, NULL, NULL, TO_ROOM);
  act("$n suddenly grow to a giant size!", ch, NULL, NULL, TO_ROOM);  
}

/*
 * Spinning Earthfire
 * The ultimate AOE for MC's.
 * Caldazar - 2009-11-24
 */

void spell_spinning_earthfire( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  char buf[MAX_INPUT_LENGTH];
  int dam = 0; // Max Dam
  int rdam= 0; // Real Dam
  int rings=1;
  int i=0;
  bool mainsphere = FALSE;

  OBJ_DATA *obj_lose, *obj_next;   // For drop objects 'heat metal' alike
  bool fail = TRUE;
  bool remove = FALSE;

  dam = calculate_weave_dam(sn, ch, 1);

  // Added Damage modifier
  dam += number_range((ch->perm_sphere[SPHERE_FIRE] + ch->perm_sphere[SPHERE_EARTH])/5, (ch->perm_sphere[SPHERE_FIRE] + ch->perm_sphere[SPHERE_EARTH])/3);

  if (IS_CODER(ch)) {
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'spinning earthfire' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }

  if (!IS_NPC(ch)) {
    rings = get_skill(ch,sn)/33;

    if (rings < 1)
	 rings = 1;
  }
  else {
    rings = ch->level/25;
  }


  // Do the dam
  for (i = 1; i <= rings; i++) {
	if (victim->position == POS_DEAD)
		continue;
    rdam = number_range(dam/2, dam);
    rdam = (rdam * 5) / 4;
    damage(ch, victim, rdam, sn, DAM_FIRE, TRUE);
    act("Roaring {rf{Yl{ra{Ym{re{Ys{x suddenly burst from the {ye{Da{yr{Dt{yh{x and spin {Rviolently{x around $N!", ch, NULL, victim, TO_NOTVICT);
    act("You make roaring {rf{Yl{ra{Ym{re{Ys{x suddenly burst from the {ye{Da{yr{Dt{yh{x and spin {Rviolently{x around $N!", ch, NULL, victim, TO_CHAR);
    act("Roaring {rf{Yl{ra{Ym{re{Ys{x suddenly burst from the {ye{Da{yr{Dt{yh{x and spin {Rviolently{x around you!", ch, NULL, victim, TO_VICT);
  }

  for ( vch = char_list; vch != NULL; vch = vch_next ) {
	  vch_next	= vch->next;
	  if ( vch->in_room == NULL )
		  continue;
	  if (ch == vch)
		  continue;
	  if (IS_GHOLAM(vch))
		  continue;
	  if (vch == victim)
		  continue;
	  if (is_same_group(vch, ch)) {
		  if (get_skill(ch,sn) >= number_percent())
			  continue;
	  }
	  if (!IS_SAME_WORLD(vch, ch))
		  continue;
	  if ( vch->in_room != ch->in_room )
		  continue;
	  if ( vch == ch )
		  continue;
	  if (vch->fighting == ch){
		  for (i = 1; i <= rings/3; i++) {
			  if (vch->position == POS_DEAD)
				  continue;
			  if (rings > 1 && number_chance(10*i)) {
				  act("Roaring {rf{Yl{ra{Ym{re{Ys{x suddenly burst from the {ye{Da{yr{Dt{yh{x and spin {Rviolently{x around $N!", ch, NULL, vch, TO_NOTVICT);
				  act("You make roaring {rf{Yl{ra{Ym{re{Ys{x suddenly burst from the {ye{Da{yr{Dt{yh{x and spin {Rviolently{x around $N!", ch, NULL, vch, TO_CHAR);
				  act("Roaring {rf{Yl{ra{Ym{re{Ys{x suddenly burst from the {ye{Da{yr{Dt{yh{x and spin {Rviolently{x around you!", ch, NULL, vch, TO_VICT);
				  rdam = number_range(dam/2, dam);
				  rdam = (rdam * 4) / 3;
				  damage(ch, vch, rdam, sn, DAM_FIRE, TRUE);
			  }
		  }
		  if (number_percent() < 20) // Possible Bash Effect
			  DAZE_STATE(vch, number_range(1,2) * PULSE_VIOLENCE);
		  if (number_percent() < number_range(15,35)) { // Possible Sear Effect
		      // Remove objects if burned?
			  if (number_percent() <= 5)
				  remove = TRUE;
		      if (!saves_spell(level + 2,victim,DAM_FIRE)  &&  !IS_SET(victim->imm_flags,IMM_FIRE)) {
		    	  for ( obj_lose = vch->carrying; obj_lose != NULL; obj_lose = obj_next) {
		    		  obj_next = obj_lose->next_content;
		    		  if (number_range(1, 2*level) > obj_lose->level &&
		    				  !saves_spell(level,victim,DAM_FIRE) &&
		    				  !IS_OBJ_STAT(obj_lose,ITEM_NONMETAL) &&
		    				  !IS_OBJ_STAT(obj_lose,ITEM_ROT_DEATH) &&
		    				  !IS_OBJ_STAT(obj_lose,ITEM_BURN_PROOF)) {
		    			  switch ( obj_lose->item_type ) {
							  case ITEM_ARMOR:
								  if (obj_lose->wear_loc != -1) { /* remove the item */
									  if (can_drop_obj(vch,obj_lose) &&
											  (obj_lose->weight / 10) < number_range(1,2 * get_curr_stat(vch,STAT_DEX)) &&
											  remove_obj( vch, obj_lose->wear_loc, remove )) {
										  act("$n yelps and throws $p to the ground!", vch,obj_lose,NULL,TO_ROOM);
										  act("You remove and drop $p before it {Yburns{x you.", vch,obj_lose,NULL,TO_CHAR);
										  dam += (number_range(1,obj_lose->level) / 3);
										  obj_from_char(obj_lose);
										  obj_to_room(obj_lose, vch->in_room);
										  fail = FALSE;
									  } else {/* stuck on the body! ouch! */
										  act("Your skin is {Yseared{x by $p!", ch,obj_lose,vch,TO_VICT);
										  act("$N's skin is {Yseared{x by $p!", ch,obj_lose,vch,TO_ROOM);
										  act("$N's skin is {Yseared{x by $p!", ch,obj_lose,vch,TO_CHAR);
										  dam += (number_range(1,obj_lose->level));
										  fail = FALSE;
									  }
								  }
								  break;
							  default:
								  break;
		    			  }
		    		  }
		    	  }
		      }
		  }
	  } else {
		  if (number_percent() > 50 )  {
			  act("Roaring {rf{Yl{ra{Ym{re{Ys{x suddenly burst from the {ye{Da{yr{Dt{yh{x and spin {Rviolently{x around $N!", ch, NULL, vch, TO_NOTVICT);
			  act("You make roaring {rf{Yl{ra{Ym{re{Ys{x suddenly burst from the {ye{Da{yr{Dt{yh{x and spin {Rviolently{x around $N!", ch, NULL, vch, TO_CHAR);
			  act("Roaring {rf{Yl{ra{Ym{re{Ys{x suddenly burst from the {ye{Da{yr{Dt{yh{x and spin {Rviolently{x around you!", ch, NULL, vch, TO_VICT);
			  if (vch->position == POS_DEAD)
				  continue;
			  rdam = number_range(dam/2, dam);
			  rdam = (rdam * 4) / 3;
			  damage(ch, vch, rdam, sn, DAM_FIRE, TRUE);
		  }
	  }
  }

  return;
}

void spell_hurricane( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  char buf[MAX_INPUT_LENGTH];
  int dam = 0; // Max Dam
  int rdam= 0; // Real Dam
  int rings=1;
  int i=0;
  bool mainsphere = FALSE;

  OBJ_DATA *obj_lose, *obj_next;
  bool fail = TRUE;
  bool remove = FALSE;

  dam = calculate_weave_dam(sn, ch, 1);

  // Added Damage modifier
  dam += number_range((ch->perm_sphere[SPHERE_AIR] + ch->perm_sphere[SPHERE_WATER])/5, (ch->perm_sphere[SPHERE_AIR] + ch->perm_sphere[SPHERE_WATER])/3);

  if (IS_CODER(ch)) {
    sprintf(buf, "[ {YCoder{x ]: MAX DAM 'hurricane' = %d (%s)\n", dam, mainsphere ? "main sphere" : "not main sphere");
    send_to_char(buf, ch);
  }

  if (!IS_NPC(ch)) {
    rings = get_skill(ch,sn)/33;

    if (rings < 1)
	 rings = 1;
  }
  else {
    rings = ch->level/25;
  }


  // Do the dam
  for (i = 1; i <= rings; i++) {
	if (victim->position == POS_DEAD)
		continue;
    rdam = number_range(dam/2, dam);
    rdam = (rdam * 5) / 4;
    damage(ch, victim, rdam, sn, DAM_LIGHTNING, TRUE);
    act("A raging tempest of {Cw{ci{Cn{cd{x and {Yl{yig{wh{Wt{wn{yin{Yg{x suddenly begins to form around $N!", ch, NULL, victim, TO_NOTVICT);
    act("You make a raging tempest of {Cw{ci{Cn{cd{x and {Yl{yig{wh{Wt{wn{yin{Yg{x form around $N!", ch, NULL, victim, TO_CHAR);
    act("A raging tempest of {Cw{ci{Cn{cd{x and {Yl{yig{wh{Wt{wn{yin{Yg{x suddenly begins to form around you!", ch, NULL, victim, TO_VICT);
  }

  for ( vch = char_list; vch != NULL; vch = vch_next ) {
	  vch_next	= vch->next;
	  if ( vch->in_room == NULL )
		  continue;
	  if (ch == vch)
		  continue;
	  if (IS_GHOLAM(vch))
		  continue;
	  if (vch == victim)
		  continue;
	  if (is_same_group(vch, ch)) {
		  if (get_skill(ch,sn) >= number_percent())
			  continue;
	  }
	  if (!IS_SAME_WORLD(vch, ch))
		  continue;
	  if ( vch->in_room != ch->in_room )
		  continue;
	  if ( vch == ch )
		  continue;
	  if (vch->fighting == ch){
		  for (i = 1; i <= rings/3; i++) {
			  if (vch->position == POS_DEAD)
				  continue;
			  if (rings > 1 && number_chance(10*i)) {
				  act("A raging tempest of {Cw{ci{Cn{cd{x and {Yl{yig{wh{Wt{wn{yin{Yg{x suddenly forms around $N!", ch, NULL, vch, TO_NOTVICT);
				  act("You make a raging tempest of {Cw{ci{Cn{cd{x and {Yl{yig{wh{Wt{wn{yin{Yg{x form around $N!", ch, NULL, vch, TO_CHAR);
				  act("A raging tempest of {Cw{ci{Cn{cd{x and {Yl{yig{wh{Wt{wn{yin{Yg{x suddenly begins to form around you!", ch, NULL, vch, TO_VICT);
				  rdam = number_range(dam/2, dam);
				  rdam = (rdam * 4) / 3;
				  damage(ch, vch, rdam, sn, DAM_LIGHTNING, TRUE);
			  }
		  }
		  if (number_percent() < 30) // Possible Bash Effect
			  DAZE_STATE(vch, number_range(1,2) * PULSE_VIOLENCE);
	  } else {
		  if (number_percent() > 50 )  {
			  act("A raging tempest of {Cw{ci{Cn{cd{x and {Yl{yig{wh{Wt{wn{yin{Yg{x suddenly forms around $N!", ch, NULL, vch, TO_NOTVICT);
			  act("You make a raging tempest of {Cw{ci{Cn{cd{x and {Yl{yig{wh{Wt{wn{yin{Yg{x form around $N!", ch, NULL, vch, TO_CHAR);
			  act("A raging tempest of {Cw{ci{Cn{cd{x and {Yl{yig{wh{Wt{wn{yin{Yg{x suddenly begins to form around you!", ch, NULL, vch, TO_VICT);
			  if (vch->position == POS_DEAD)
				  continue;
			  rdam = number_range(dam/2, dam);
			  rdam = (rdam * 4) / 3;
			  damage(ch, vch, rdam, sn, DAM_LIGHTNING, TRUE);
		  }
	  }
  }

  return;
}

/*
 * Air Dome
 * Combined sanc and stoneskin
 * Inverted
 * Caldazar 2009-12-01
 */
void spell_air_dome( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int modifier;
    int skill;

    if (victim == NULL) {
	victim == ch;
    }    

    if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
    {
	if (victim == ch)
	  send_to_char("You are already protected.\n\r",ch);
	else
	  act("$N is already surrounded by a {Cd{co{Wm{Ce{x of {Ca{Wi{cr{x.",ch,NULL,victim,TO_CHAR);
	return;
    }

    skill = get_skill(ch, sn);
    modifier = -1 * (skill / 5);

    af.where     = TO_AFFECTS;
    af.casterId  = ch->id;
    af.type      = sn;
    af.level     = level;
    af.duration  = SUSTAIN_WEAVE;
    af.location  = APPLY_AC;
    af.modifier  = modifier;
    af.bitvector = AFF_SANCTUARY;
    af.inverted  = TRUE;

    affect_to_char( victim, &af );
    act( "$n is surrounded by a {Cd{co{Wm{Ce{x of {Ca{Wi{cr{x.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You are surrounded by a {Cd{co{Wm{Ce{x of {Ca{Wi{cr{x.\n\r", victim );
    return;
}

/*
 * Mind Flay
 * Combined blind and weaken
 * Inverted
 * Caldazar 2009-12-01
 */
void spell_mind_flay( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int duration=0;
    int skill;
    int modifier;

    if ( IS_AFFECTED(victim, AFF_BLIND) || saves_spell(level,victim,DAM_OTHER)) {
      act("Some how you were not able to create the flows upon $N.", ch, NULL, victim, TO_CHAR);
      return;
    }

    duration = SUSTAIN_WEAVE;
    skill = get_skill(ch, sn);
    modifier = -1 * (skill/12);


    af.where     = TO_AFFECTS;
    af.casterId  = ch->id;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_HITROLL;
    af.modifier  = -4;
    if (!IS_NPC(ch))
	 af.duration  = duration;
    else
	 af.duration  = ch->level / 10;
    af.bitvector = AFF_BLIND;
    af.inverted  = TRUE;
    affect_to_char( victim, &af );

    af.where     = TO_AFFECTS;
    af.casterId  = ch->id;
    af.type      = sn;
    af.level     = level;
    if (!IS_NPC(ch))
     af.duration  = duration;
    else
     af.duration  = ch->level / 10;
    af.location  = APPLY_STR;
    af.modifier  = modifier;
    af.bitvector = AFF_WEAKEN;
    af.inverted  = TRUE;
    affect_to_char( victim, &af );

    send_to_char( "You clutch your head and scream loudly!\n\r", victim );
    act("$n cluthes $s head and screams loudly!",victim,NULL,NULL,TO_ROOM);
    return;
}

bool is_wearing_foxhead(CHAR_DATA *ch) {
	bool found = FALSE;
	OBJ_DATA * obj;
        obj  = get_eq_char( ch, WEAR_NECK_1 );
	if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_FOXHEAD_MEDALLION)
         {
                found = TRUE;
         }
         else
         {
                 obj = get_eq_char( ch, WEAR_NECK_2 );
                if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_FOXHEAD_MEDALLION)
                {
                        found = TRUE;
                }
                else
                {
                        obj = get_eq_char( ch, WEAR_HOLD );
                        if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_FOXHEAD_MEDALLION)
                        {
                                found = TRUE;
                        }

                }
         }
	 return found;
}

bool foxhead_immune(CHAR_DATA *ch, int gsn) {

         OBJ_DATA *obj;
         bool found;

         if (ch == NULL)
		return FALSE;
         /*
          * Look for a foxhead medallion.
          */
	 if (!is_wearing_foxhead(ch)) {
		return FALSE;
	 }

	 //Have medallion worn, is the skill one of the ones that the medallion protects against
	 int immune_sn[] = {
		skill_lookup("burning hands"),
		skill_lookup("Bar of fire"),
		skill_lookup("Blast of air"),
		skill_lookup("Shocking Grasp"),
		skill_lookup("Refresh"),
		skill_lookup("Blindness"),
		skill_lookup("Chill touch"),
		skill_lookup("Pinch"),
		skill_lookup("Levitate"),
		skill_lookup("Minor heal"),
		skill_lookup("Invisibility"),
		skill_lookup("Shielding"),
		skill_lookup("Wrap"),
		skill_lookup("Delve"),
		skill_lookup("Giant strength"),
		skill_lookup("Gag"),
		skill_lookup("Sleep"),
		skill_lookup("Calm"),
		skill_lookup("Compulsion"),
		skill_lookup("Cure poison"),
		skill_lookup("Harm"),
		skill_lookup("Heal"),
		skill_lookup("Stone skin"),
		skill_lookup("Armor"),
		skill_lookup("Bond"),
		skill_lookup("Major heal"),
		skill_lookup("Gust"),
		skill_lookup("Mass invis"),
		skill_lookup("Haste"),
		skill_lookup("Weaken"),
		skill_lookup("Detect invis"),
		skill_lookup("Cure disease"),
		skill_lookup("Fireproof"),
		skill_lookup("Mass healing"),
		skill_lookup("Sanctuary"),
		skill_lookup("Slow"),
		skill_lookup("Balefire"),
		skill_lookup("Still"),
		skill_lookup("Heal still"),
		skill_lookup("Suffocate"),
		-2
	   };
	   int i = 0;
	   for (i = 0; immune_sn[i] != -2; i++)
	   {
		if (gsn == immune_sn[i]) {
			return TRUE;
		}
	   }
	   return FALSE;
}
