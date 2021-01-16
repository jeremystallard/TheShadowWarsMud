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
 * This file include code special dedicated to the Ogier race              *
 * TSW is copyright -2003 Swordfish and Zandor                             *
 **************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"

// Possible to sing wood from a tree object in the room
void do_singwood ( CHAR_DATA *ch, char *argument )
{
  OBJ_DATA *sobj=NULL;
  OBJ_DATA *obj=NULL;
  OBJ_DATA *tree_obj=NULL;
  OBJ_DATA *tree_obj_next=NULL;
  char buf[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  int nMatch=0;
  bool obj_created=FALSE;
  int sing_value=0;
  bool found_tree=FALSE;
  int endurance=0;
  
  // Only PCs
  if (IS_NPC(ch))
    return;
  
  // Only for Ogiers
  if (ch->race != race_lookup("ogier") && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  // Check if talent skill
  if (skill_table[gsn_singwood].talent_req != 0) {
    if (!IS_SET(ch->talents, skill_table[gsn_singwood].talent_req)) {
	 send_to_char("Your voice is not colored with the talent this require.\n\r", ch);
	 return;
    }
  }

  //Must be IC
  if (!IS_RP(ch))
  {
	send_to_char("You must be doing this ICly.\r\n",ch);
	return;
  }
  
  // Know the skill?
  if (ch->pcdata->learned[gsn_singwood] < 1) {
    send_to_char("You don't even know how to sing wood from trees.\n\r", ch);
    return;
  }
  else {
    sing_value = ch->pcdata->learned[gsn_singwood]/3;
    endurance = skill_table[gsn_singwood].min_endurance;
  }
  
  // Argument handeling
  argument = one_argument( argument, arg );
  
  if (IS_NULLSTR(argument) || IS_NULLSTR(arg)) {
    send_to_char("Syntax: singwood obj <object>\n\r", ch);
    send_to_char("        singwood weapon <object>\n\r", ch);
    return;
  }

  // Check if there is a tree in the room
  for ( tree_obj = ch->in_room->contents; tree_obj != NULL; tree_obj = tree_obj_next) {
    tree_obj_next = tree_obj->next_content;
    if (is_name("tree", tree_obj->name)) {
	 found_tree=TRUE;
	 break;
    }
  }
  
  // If no tree found, bail out
  if (!found_tree) {
    send_to_char("There are no trees here to sing wood from.\n\r", ch);
    return;
  }

  // Endurance ok?
  if (ch->endurance < endurance) {
    send_to_char("You are too tired to concentrate any more!\n\r", ch);
    return;	
  }

  // Enough Skilled?
  if (number_percent() > sing_value) {
    act("You sit down next to $p and start singing to it, but you are still young in your skills.", ch, tree_obj, NULL, TO_CHAR);
    act("$n sits down next to $p and sing to it, but it doesn't sound right.", ch, tree_obj, NULL, TO_ROOM);
    
    ch->endurance -= endurance/2;
    return;
  }

  //Find an object that is reset, e.g exist in this world
  for (sobj = object_list; sobj != NULL; sobj = sobj->next ) {
    if ( !can_see_obj( ch, sobj ) || !is_name( argument, sobj->name ) || ch->level < sobj->level || sobj->level > 80 || (strstr(sobj->name,"quest")))
	 continue;
    nMatch++;
    // If weapon, only look for weapon item types
    if (!str_cmp(arg, "weapon") && sobj->item_type != ITEM_WEAPON)
	 continue;     
    // If weapon, make sure not uber average dam
    if (!str_cmp(arg, "weapon") && sobj->item_type == ITEM_WEAPON) {
	 if (((1+sobj->value[2]) * sobj->value[1] / 2) > 90)
	   continue;
    }
    // Only from open areas
    if (!IS_SET(sobj->pIndexData->area->area_flags, AREA_OPEN))
	 continue;
    if (sobj->item_type == ITEM_TREASURE)
	 continue;
    if (sobj->item_type == ITEM_ANGREAL)
	 continue;
    if (sobj->item_type == ITEM_FOOD)
	 continue;
    if (sobj->item_type == ITEM_MONEY)
	 continue;
    if (sobj->item_type == ITEM_FOUNTAIN)
	 continue;                     
    if (sobj->item_type == ITEM_PILL)
	 continue;           
    if (sobj->item_type == ITEM_KEY)
	 continue;                      
    if (sobj->item_type == ITEM_PORTAL)
	 continue;                                 
    if (sobj->item_type == ITEM_ROOM_KEY)
	 continue;                                        
    // Not obj/weapons with special keywords
    if ((strstr(sobj->name, "heron") != NULL) ||
	   (strstr(sobj->name, "ichymojo") != NULL) ||
	   (strstr(sobj->name, "token") != NULL) ||
	   (strstr(sobj->name, "key") != NULL) ||
	   (strstr(sobj->name, "cloak") != NULL) ||
	   (strstr(sobj->name, "pants") != NULL) ||
	   (strstr(sobj->name, "cape") != NULL) ||
	   (strstr(sobj->name, "quest") != NULL) ||
	   (strstr(sobj->name, "katana") != NULL) ||
	   (strstr(sobj->name, "angreal") != NULL))
	 continue;
    if (is_name(argument, sobj->name)) {
	 if (number_chance(sing_value)) {
	   obj_created=TRUE;
	   obj = create_object(sobj->pIndexData, get_level(ch));
	   
	   // Log it!
	   sprintf(buf, "$N sung wood from a tree and created a copy of %s (vnum=%d, room=%d).", obj->short_descr, sobj->pIndexData->vnum, ch->in_room->vnum);
	   wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
	   sprintf(buf, "%s sung wood from a tree and created a copy of %s (vnum=%d, room=%d).", ch->name, obj->short_descr, sobj->pIndexData->vnum, ch->in_room->vnum);
	   log_string(buf);
	   break;
	 }
    }	
  }
  
  if (!obj_created) {
    act("You sit down next to $p and start singing to it, but the tree is not ready.", ch, tree_obj, NULL, TO_CHAR);
    act("$n sits down next to $p and sing to it.", ch, tree_obj, NULL, TO_ROOM);
    return;
  }
  else {
    // Fix the strings
    sprintf(buf, "a sungwood %s", argument);
    if (obj->short_descr)
	 free_string(obj->short_descr);
    obj->short_descr = str_dup(buf);

    sprintf(buf, "A sungwood %s is here with no seams or crafting marks.", argument);
    if (obj->description)
	 free_string(obj->description);
    obj->description = str_dup(buf);

    sprintf(buf, "sungwood %s", argument);
    if (obj->name)
	 free_string(obj->name);
    obj->name = str_dup(buf);
    
    // Messages
    act("You sit down next to $p and start singing to it.", ch, tree_obj, NULL, TO_CHAR);
    sprintf(buf, "Slowely a part of %s shape into $p until the two separates.", tree_obj->short_descr);
    act(buf, ch, obj, NULL, TO_CHAR);
    
    act("$n sits down next to $p and sing to it.", ch, tree_obj, NULL, TO_ROOM);
    sprintf(buf, "Slowely a part of %s shape into $p until the two separates.", tree_obj->short_descr);
    act(buf, ch, obj, NULL, TO_ROOM);
	 
    if (CAN_WEAR(obj, ITEM_TAKE)) {
	 obj_to_char( obj, ch );
	 act("You gently pick up $p and lower your voice.", ch, obj, NULL, TO_CHAR);
	 act("$n gently picks up $p and lower $e voice.", ch, obj, NULL, TO_ROOM);
    }
    else {
	 // Can pick up noget items, but once they are dropped they are no good to take again.
    	 act("Your hughe size makes it possible to pick up $p before you lower your voice.", ch, obj, NULL, TO_CHAR);
	 act("$n is so hughe that picking up $p seams easy!", ch, obj, NULL, TO_ROOM);
	 act("$n lower $e voice.", ch, NULL, NULL, TO_ROOM);
	 //obj_to_room( obj, ch->in_room );
	 obj_to_char( obj, ch );
    }
    
    // Cost endurance to sing
    ch->endurance -= endurance;
  }
  
  return;
}
