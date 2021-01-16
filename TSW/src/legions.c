// Depending on the one enticing legions
// set up the wolve accordingly
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"


bool can_legion(CHAR_DATA *ch)
{
   bool retval = FALSE;

   if (IS_IMMORTAL(ch))
	retval = TRUE;
   if (ch->race == race_lookup("fade")) 
	retval = TRUE;

   if (player_clan(ch) && !strcasecmp(player_clan(ch),"Children"))
   {
	if (ch->rank <=5)
	   retval = TRUE;
   }
 
   return retval;

}

int numlegions(CHAR_DATA *ch)
{
   int retval = 0;

   if (IS_IMMORTAL(ch))
   {
	retval = 10;
   }
   else
   if (ch->race == race_lookup("fade")) 
   {
        retval = 2; 
   } 
   if (ch->rank <= 5 && ch->rank >= 4)
   {
	retval = 1;
   }
   else
   if (ch->rank == 3)
   {
	retval = 2;
   }
   else
   if (ch->rank <=2)
   {
	retval = 3;
   }
  
   return retval;
}

CHAR_DATA *update_legions(CHAR_DATA *ch, CHAR_DATA *legion, int number, int enticed, char *legion_appearance)
{
  char buf[MSL];
  CHAR_DATA *_legion=legion;
  int level=0;
  int lowdam=0;
  int highdam=0;
  //int dammod=0;
  //int addition=0;
  int lowhit=0;
  int highhit=0;
  int hitmod=0;

  if (ch->race == race_lookup("fade")) {
     return legion;
  }
  // If standard wolf is higher level than what update will be
  // use standard wolf
  if (legion->level >= (get_level(ch)/2))
    return legion;
  
  // Max entice level is 90
  if (get_level(ch) >= 200 && number <= 1 && enticed <= 0)
    level = 96;
  else if (get_level(ch) >= 200 && enticed >= 1)
    level = 86;
  else if (get_level(ch) >= 200)
    level = 91;
  else
    level = get_level(ch)/2;
  
  _legion->level        = level-number;
  _legion->hitroll      = (level-15) - (number*2);
  _legion->damroll      = (level/2) - (number*2);

  //calculate hit dice
  lowhit = (level / 2) + 1;
  if (level % 2) {
    highhit = (level / 2) + 1;
  }
  else {
    highhit = (level / 2) + 2;
  }
  hitmod = (level * level);
  
  _legion->max_hit      = dice(lowhit, (highhit-number))+(hitmod-number);
  _legion->hit          = _legion->max_hit;
  
  _legion->hit_loc[LOC_LA] = get_max_hit_loc(_legion, LOC_LA);
  _legion->hit_loc[LOC_LL] = get_max_hit_loc(_legion, LOC_LL);
  _legion->hit_loc[LOC_HE] = get_max_hit_loc(_legion, LOC_HE);
  _legion->hit_loc[LOC_BD] = get_max_hit_loc(_legion, LOC_BD);
  _legion->hit_loc[LOC_RA] = get_max_hit_loc(_legion, LOC_RA);
  _legion->hit_loc[LOC_RL] = get_max_hit_loc(_legion, LOC_RL);
  
  _legion->max_endurance   = dice(level, level/2)+100;
  _legion->endurance       = _legion->max_endurance;
  
  _legion->damage[DICE_NUMBER] = lowdam;
  _legion->damage[DICE_TYPE]   = highdam;
  
  //Armor class
  if (level >= 45) {
    int newac = (level - 30);
    _legion->armor[0] = newac;
    _legion->armor[1] = newac;
    _legion->armor[2] = newac;
    _legion->armor[3] = newac;
  }
  else if ((level >= 0) && level <= 11) {
    int newac =  level - 11;
    _legion->armor[0] = newac;
    _legion->armor[1] = newac;
    _legion->armor[2] = newac;
    _legion->armor[3] = newac;
  }
  else if ((level >= 12) && (level <45)) {
    int newac = ((level / 2) - 4);
    _legion->armor[0] = newac;
    _legion->armor[1] = newac;
    _legion->armor[2] = newac;
    _legion->armor[3] = newac;
  }
  
  if (!IS_NULLSTR(legion_appearance)) {
  	if (strstr(legion_appearance, "guard") != NULL) {
  	   sprintf(buf, "%s", legion_appearance);
  	   buf[0] = LOWER(buf[0]);
  	   free_string(_legion->short_descr);
  	   _legion->short_descr = str_dup(buf);
  	   
  	   sprintf(buf, "%s is here.\n\r", legion_appearance);
  	   buf[0] = UPPER(buf[0]);
  	   free_string(_legion->long_descr);
  	   _legion->long_descr = str_dup(buf);
  	}
  }
    
  return _legion;
}

void do_legionsummon (  CHAR_DATA *ch, char *argument )
{
  static char * const numbername [] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"};
  CHAR_DATA *pet;
  CHAR_DATA *npc;    
  CHAR_DATA *npc_next;
  OBJ_DATA * sword;
  char buf[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  int sn = 0;
  int endurance = 0;
  int legions = 1;
  int i = 0;
  int enticed_legions = 0;
  int wanted_legions = 0;

  if (!can_legion(ch))
  {
	send_to_char("You can't do that.\r\n",ch);
        return;
  }
  if ( ch->master != NULL || ( ch->leader != NULL && ch->leader != ch )) {
     send_to_char( "You need to lead the group to be able to controll it!\n\r", ch );
     return;
  }
  
  /* Find out how many legions already are enticed */
  for ( npc = char_list; npc != NULL; npc = npc_next ) {
    npc_next	= npc->next;
    if ( npc->in_room == NULL )
	    continue;
	 if (!IS_NPC(npc))
	    continue;
	 if (npc->pIndexData->vnum != MOB_VNUM_LEGION && npc->pIndexData->vnum != MOB_VNUM_TROLLOC)
	    continue;
	 if (is_same_group( npc, ch ))
      enticed_legions++;
   }

  if (!IS_NPC(ch))
  {
    legions = numlegions(ch); 
  
  }
  else
    return;
  /* Can only have skill/25 legions at any time */
  if (enticed_legions > 0) {
   if (enticed_legions >= legions) {
    send_to_char("You are unable to summon any more guards to assist you.\n\r", ch);
    return;
   }
   else {
     legions = legions - enticed_legions; 
   }
  }

  argument = one_argument(argument, arg);

  if (!IS_NULLSTR(arg)) {
    if (!(wanted_legions = atoi(arg))) {
	 send_to_char("Syntax: legion <number of guards> [<guard appearance>]\n\r", ch);
	 return;
    }
    
    if (wanted_legions <= 0 || wanted_legions > legions) {
	 sprintf(buf, "You can only summon a max of %d legions\n\r", legions);
	 send_to_char(buf, ch);
	 return;
    }
    
    legions = wanted_legions;
  }
  

  if (ch->endurance < endurance*legions) {
    send_to_char("You are too tired to concentrate or don't have enough endurance to focus!\n\r", ch);
    return;
  }
  
  if (legions <= 0) {
    send_to_char("You don't have the authority to summon any guards.\n\r", ch);
    ch->endurance -= (endurance*legions)/2;
    return;
  }
  
  ch->endurance -= endurance*legions;;

  send_to_char("You call out to the guards who might be nearby.\n\r", ch);

  WAIT_STATE( ch, skill_table[sn].beats );

  sprintf(buf, "%s %s arrive and fall into line.", numbername[legions], legions > 1 ? "guards" : "guard");
  act(buf, ch, NULL, NULL, TO_CHAR);
  sprintf(buf, "%s %s arrive and fall in behind $n.", numbername[legions], legions > 1 ? "guards" : "guard");
  act(buf, ch, NULL, NULL, TO_ROOM);

  for (i = 1; i <= legions; i++) {    
    if (ch->race == race_lookup("fade")) {
       pet = create_mobile(get_mob_index(MOB_VNUM_TROLLOC));
    }
    else
    {
       pet = create_mobile(get_mob_index(MOB_VNUM_LEGION));
       pet = update_legions(ch, pet, legions, enticed_legions, argument);
       sword = create_object(get_obj_index(OBJ_VNUM_LEGION_SWORD),pet->level);
       obj_to_char(sword,pet);
       equip_char(pet,sword,WEAR_WIELD);
    }
    pet->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
    char_to_room( pet, ch->in_room );
    SET_BIT(pet->affected_by, AFF_CHARM);
    add_follower( pet, ch );
    pet->leader = ch;
  }  
}

void do_legiondismiss (  CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *npc;    
  CHAR_DATA *npc_next;
  char buf[MAX_INPUT_LENGTH];
  int enticed_legions   = 0;
  int dismiss_legions   = 0;
  int dismissed_legions = 0;


  if (!can_legion(ch))
  {
	send_to_char("You don't have the authority for that.\r\n",ch);
	return;
  }

  /* Find out how many legions are enticed            */
  for ( npc = char_list; npc != NULL; npc = npc_next ) {
    npc_next	= npc->next;
    if ( npc->in_room == NULL )
	 continue;
    if (!IS_NPC(npc))
	 continue;
    if (npc->pIndexData->vnum != MOB_VNUM_LEGION && npc->pIndexData->vnum != MOB_VNUM_TROLLOC)
	 continue;
    if (is_same_group( npc, ch ))
      enticed_legions++;
  }

  if (!IS_NULLSTR(argument)) {
    if (!(dismiss_legions = atoi(argument))) {
	 send_to_char("Syntax: legiondismiss <number of guards>\n\r", ch);
	 return;
    }

    if (dismiss_legions <= 0) {
	 send_to_char("You must give a number higher then 0.\n\r", ch);
	 return;
    }

    if (dismiss_legions > enticed_legions) {
	 sprintf(buf, "You have %d guards that could be dismissed.\n\r", enticed_legions);
	 send_to_char(buf, ch);
	 return;
    }
    
    /* dismiss amount of legions */
    for ( npc = char_list; npc != NULL; npc = npc_next ) {
	 npc_next	= npc->next;
	 if ( npc->in_room == NULL )
	   continue;
	 if (!IS_NPC(npc))
	   continue;
	 if (npc->pIndexData->vnum != MOB_VNUM_LEGION && npc->pIndexData->vnum != MOB_VNUM_TROLLOC)
	   continue;
	 if (dismissed_legions >= dismiss_legions)
	   continue;
	 if (is_same_group( npc, ch )) {
	   dismissed_legions++;
	   act("You look at $N for a moment and dismiss it from your group.", ch, NULL, npc, TO_CHAR);		
	   stop_follower( npc );
	   act("$N leaves quietly.", ch, NULL, npc, TO_ROOM);
	   act("$N leaves quietly.", ch, NULL, npc, TO_CHAR);
	   extract_char(npc,TRUE,FALSE);
	 }
    }
  }
  else {
    send_to_char("Syntax: legiondismiss <number of guards>\n\r", ch);
    return;
  }

}


