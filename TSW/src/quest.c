#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "command.h"

struct reward_type
{
  char *name;
  char *keyword;
  int cost;
  bool object;
  int value;
  void *      where;
};

/* Descriptions of quest items go here:
Format is: "keywords", "Short description", "Long description" */
const struct quest_desc_type quest_desc[] =
{
{"quest sceptre", 	"the Sceptre of Courage",
"{G(Quest obj){x The Sceptre of Courage is lieing here, waiting to be returned to its owner."},

{"quest crown", 	"the Crown of Wisdom",
"{G(Quest obj){x The Crown of Wisdom is lieing here, waiting to be returned to its owner."},

{"quest gauntlet", 	"the Gauntlets of Strength",
"{G(Quest obj){x The Gauntlets of Strength are lieing here, waiting to be returned to its owner."},

{NULL, NULL, NULL}
};

/* Local functions */
void generate_quest	args(( CHAR_DATA *ch, CHAR_DATA *questman ));
void quest_update	args(( void ));
bool quest_level_diff   args(( int clevel, int mlevel));

/* The main quest function */
void do_quest(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *questman;
  OBJ_DATA *obj=NULL, *obj_next;
  OBJ_INDEX_DATA *index=NULL;
  OBJ_INDEX_DATA *questinfoobj;
  MOB_INDEX_DATA *questinfo;
  char buf [MAX_STRING_LENGTH];
  char arg1 [MAX_INPUT_LENGTH];
  char arg2 [MAX_INPUT_LENGTH];
  int i;
  bool found=FALSE;
  int chance;

/* Add your rewards here.  Works as follows:
"Obj name shown when quest list is typed", "keywords for buying",
"Amount of quest points",  Does it load an object?,  IF it loads an
object, then the vnum, otherwise a value to set in the next thing,  This
is last field is a part of the char_data to be modified */

const struct reward_type reward_table[]=
{
/*
  { "A Flask of Endless Water","flask water",   500, TRUE,  46,    0},
  { "Jug O' Moonshine",        "jug moonshine", 300, TRUE,  47,    0},
  { "Potion of Extra Healing", "extra healing", 300, TRUE,  4639,  0},
  { "Potion of Sanctuary",     "sanctuary",     150, TRUE,  3081,  0},
*/
  { "3000 Gold Pieces",     "3000 gold",	500, FALSE, 3000,&ch->gold},
  { "40 Trains",    	       "trains", 	500, FALSE, 40,    &ch->train},
  { "1 Token",   	       "token", 	750, TRUE, 9920, 0},
/*
  { "Aura of Doom",    	       "aura",	 	4000, TRUE, 5893,    0},
  { "Crescent Moon Tattoo",    "tattoo", 	4000, TRUE, 5894,    0},
  { "Foxhead Medallion",       "medallion", 	5000, TRUE, 5876,    0},
  { "Sturdy Boots",    	       "boots", 	6000, TRUE, 5881,    0},
  { "Glimmering Cloak",        "cloak", 	6000, TRUE, 5882,    0},
  { "Shimmering Veil",         "veil", 		6000, TRUE, 5895,    0},
  { "Ruby encrusted Dagger",   "dagger", 	10000, TRUE, 5878,    0},
  { "Beautiful Sword",         "sword", 	10000, TRUE, 5877,    0},
*/
  { "Death Certificate",       "certificate", 	20000, TRUE, 5880,    0},
  { "Black Sphere",	       "sphere", 	8000,  TRUE, 44,    0},
  { "Sword",                   "sword", 	8000,  TRUE, 40050, 0},
  { "Dagger",                  "dagger", 	8000,  TRUE, 40051, 0},
  { "Lance",                   "lance", 	8000,  TRUE, 40052, 0},
  { "Spear",	               "spear", 	8000,  TRUE, 40053, 0},
  { "Axe",                     "axe", 		8000,  TRUE, 40054, 0},
  { "Flail",                   "flail", 	8000,  TRUE, 40055, 0},
  { "Mace",	               "mace",  	8000,  TRUE, 40056, 0},
  { "Whip",                    "whip",   	8000,  TRUE, 40057, 0},
  { "Polearm",                 "polearm", 	8000,  TRUE, 40058, 0},
  { "Staff",                   "staff", 	8000,  TRUE, 40059, 0},
  { "Swordbreaker",            "swordbreaker", 	7000,  TRUE, 40048, 0},
  { "Black Leather Bracer",    "bracer", 	4000,  TRUE, 40047, 0},
  { "Black Leather Belt",      "belt", 		3000,  TRUE, 40046, 0},
  { "Black Silk Cloak",        "cloak", 	5000,  TRUE, 40045, 0},
  { "Black Mail Backplate",    "backplate", 	6000,  TRUE, 40044, 0},
  { "Black Kite Shield",       "shield", 	6000,  TRUE, 40043, 0},
  { "Scale Mail Sleeves",      "sleeves", 	5000,  TRUE, 40042, 0},
  { "Black Leather Gauntlets", "gauntlets", 	4000,  TRUE, 40041, 0},
  { "Black Leather Boots",     "boots", 	4000,  TRUE, 40040, 0},
  { "Black Leather Breeches",  "breeches", 	5000,  TRUE, 40039, 0},
  { "Black Plumed Helmet",    "helmet", 	7000,  TRUE, 40038, 0},
  { "Pure Black Breastplate", "breastplate", 	6000,  TRUE, 40037, 0},
  { "Short Silver Chain",     "chain", 		4000,  TRUE, 40036, 0},
  { "Silver Onyx Ring",       "ring", 		4000,  TRUE, 40035, 0},
  { "Red Tattoo of an Onyx",  "tattoo", 	4000,  TRUE, 40034, 0},
  { "Remnants of the Void",   "void", 		4000,  TRUE, 40033, 0},
  { "Flame of the Void",      "flame", 		4000,  TRUE, 40032, 0},
  { "Silver Stud Earring",    "stud",	 	4000,  TRUE, 40031, 0},
  { "Cold Stony Expression",   "expression", 	3000,  TRUE, 40030, 0},
  { NULL, NULL, 0, FALSE, 0, 0  } /* Never remove this!!! */
};


 argument = one_argument(argument, arg1);
 argument = one_argument(argument, arg2);
 
 if(IS_NPC(ch)) { 
   send_to_char("NPC's can't quest.\n\r",ch); 
   return; 
 }
 
 if (arg1[0] == '\0') {
   send_to_char("Quest commands: Info, Time, Request, Complete, Quit, List, Lore, and Buy.\n\r",ch);
   send_to_char("For more information, type 'Help Quest'.\n\r",ch);
   return;
 }
 
 if (!strcmp(arg1, "info")) {
   if (IS_SET(ch->act,PLR_QUESTING)) {
	if (ch->pcdata->questmob == -1 && ch->pcdata->questgiver->short_descr != NULL) {
	  sprintf(buf,"Your quest is ALMOST complete!\n\rGet back to %s before your time runs out!\n\r",ch->pcdata->questgiver->short_descr);
	  send_to_char(buf,ch);
	}
	else if (ch->pcdata->questobj > 0) {
	  questinfoobj = get_obj_index(ch->pcdata->questobj);
	  if (questinfoobj != NULL) {
	    sprintf(buf,"You are on a quest to recover the fabled %s!\n\r",questinfoobj->name);
	    send_to_char(buf,ch);
	  }
	  else send_to_char("You aren't currently on a quest.\n\r",ch);
	  return;
	}
	else if (ch->pcdata->questmob > 0) {
	  questinfo = get_mob_index(ch->pcdata->questmob);
	  if (questinfo != NULL) {
	    sprintf(buf,"You are on a quest to slay the dreaded %s!\n\r",questinfo->short_descr);
	    send_to_char(buf,ch);
	  }
	  else send_to_char("You aren't currently on a quest.\n\r",ch);
	  return;
	}
   }
   else
	send_to_char("You aren't currently on a quest.\n\r",ch);
   return;
 }
 else if (!strcmp(arg1, "time")) {
   if (!IS_SET(ch->act,PLR_QUESTING)) {
	send_to_char("You aren't currently on a quest.\n\r",ch);
	if (ch->pcdata->nextquest > 1) {
	  sprintf(buf,"There are %d minutes remaining until you can go on another quest.\n\r",ch->pcdata->nextquest);
	  send_to_char(buf,ch);
	}
	else if (ch->pcdata->nextquest == 1) {
	  sprintf(buf, "There is less than a minute remaining until you can go on another quest.\n\r");
	  send_to_char(buf,ch);
	}
   }
   else if (ch->pcdata->countdown > 0) {
	sprintf(buf, "Time left for current quest: %d\n\r",ch->pcdata->countdown);
	send_to_char(buf, ch);
   }
   return;
 }
 
 for ( questman = ch->in_room->people; questman != NULL; questman = questman->next_in_room ) {
   if (!IS_NPC(questman)) 
	continue;
   if (questman->spec_fun == spec_lookup( "spec_questmaster" )) 
	break;
 }
 
 if (questman == NULL || questman->spec_fun != spec_lookup("spec_questmaster" )) {
   send_to_char("You can't do that here.\n\r",ch);
   return;
 }
 
 ch->pcdata->questgiver = questman;
 
 if (!strcmp(arg1, "list")) {
   //act("$n asks $N for a list of quest items.", ch, NULL, questman, TO_ROOM);
   act("You ask $N for a list of quest items.",ch, NULL, questman, TO_CHAR);
   send_to_char("Current Quest Items available for Purchase:\n\r", ch);
   if(reward_table[0].name == NULL) {
	send_to_char("  Nothing.\n\r",ch);
   }
   else {
	send_to_char("\n\r", ch);
	send_to_char(" {WCost{w:     {BName{W:{x\n\r",ch);
	send_to_char(" -----     -------------------------------\n\r", ch);
	for(i=0;reward_table[i].name != NULL;i++) {
	  sprintf(buf," {W%5d{w     {B%s{w\n\r" ,reward_table[i].cost,reward_table[i].name);
	  send_to_char(buf,ch);
	}
	send_to_char("\n\rTo buy an item, type '{Wquest buy <item>{x'.\n\r",ch);
	send_to_char("\n\rTo sell back an item, type '{Wquest sell <item>{x'.\n\r",ch);
	return;
   }
 }

 else if (!str_cmp(arg1, "lore")) {
   
   // check syntax
   if (IS_NULLSTR(arg2)) {
	sprintf(buf, "What item do you want to study more closely?");
	do_say(questman, buf);
	return;
   }

   found = FALSE;
   
   for(i=0;reward_table[i].name != NULL;i++) {
	if (is_name(arg2, reward_table[i].keyword)) { 
	  found = TRUE;

	  if (reward_table[i].object) {
	    index = get_obj_index(reward_table[i].value);
	    if (index != NULL)
		 obj = create_object(index,ch->level);
	  }

	  if (obj) {
	    act("$N gives you $p so you can study it more closely.", ch, obj, questman, TO_CHAR);
	    act("$N gives $n $p so $e can study it more closely.", ch, obj, questman, TO_ROOM);
	  }
	  else {
	    sprintf(buf, "I don't have that item.");
	    do_say(questman,buf);
	    return;
	  }
	  break;
	}
   }

   if (found) {
	lore_item(ch, obj);
	
	act("You give $p back to $N after studying it.", ch, obj, questman, TO_CHAR);
	act("$n gives $p back to $N after studying it.", ch, obj, questman, TO_ROOM);

	extract_obj(obj);
	return;
	
   }
   else {
	sprintf(buf, "I don't have that item.");
	do_say(questman,buf);
   }	  
   
   return;
 }
 else if (!strcmp(arg1, "sell")) {
   if (arg2[0] == '\0') {
	send_to_char("To sell an item, type '{Wquest sell <item>{x'.\n\r",ch);
	return;
   }
   
   found = FALSE;
   for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
   {
      if (is_name(arg2, obj->name))
      {
         if (obj->wear_loc == WEAR_NONE)
	 	break;
      }
   }
   if (obj == NULL) {
	send_to_char("You do not have that item in your inventory.\n\r",ch);
	return;
   }
   for (i = 0; reward_table[i].name != NULL; i++) 
   {
	if ((reward_table[i].object == TRUE) && (reward_table[i].value == obj->pIndexData->vnum)) 
        {
		found = TRUE;
		obj_from_char(obj);
                int qpreturning = (reward_table[i].cost * 3) / 4;
	        sprintf(buf, "In exchange for %s, %s gives you %d quest points.\n\r",obj->short_descr, questman->short_descr, qpreturning);
		send_to_char(buf,ch);
	        ch->pcdata->quest_curr += qpreturning;
		extract_obj(obj);	
		break;
        }
   }
   if (found == FALSE)
   {
   	send_to_char("That isn't a quest item.\r\n",ch);
   }
   return;

 }
 else if (!strcmp(arg1, "buy")) {
   if (arg2[0] == '\0') {
	send_to_char("To buy an item, type '{Wquest buy <item>{x'.\n\r",ch);
	return;
   }
   
   found = FALSE;

   /* Use keywords rather than the name listed in quest list */
   /* Do this to avoid problems with something like 'quest buy the' */
   /* And people getting things they don't want... */
   for(i=0;reward_table[i].name != NULL;i++)
	if (is_name(arg2, reward_table[i].keyword)) { 
	  found = TRUE;
	  if (ch->pcdata->quest_curr >= reward_table[i].cost) {
	    ch->pcdata->quest_curr -= reward_table[i].cost;
	    if(reward_table[i].object)
		 obj = create_object(get_obj_index(reward_table[i].value),ch->level);
	    else {
		 sprintf(buf,"In exchange for %d quest points, %s gives you %s.\n\r",
			    reward_table[i].cost, questman->short_descr, reward_table[i].name );
		 send_to_char(buf,ch);
		 *(int *)reward_table[i].where += reward_table[i].value;
	    }
	    break;
	  }
	  else {
	    sprintf(buf, "Sorry, but you don't have enough quest points for that.");
	    do_say(questman,buf);
	    return;
	  }
	}
   if(!found) {
	sprintf(buf, "I don't have that item.");
	do_say(questman,buf);
   }
   if (obj != NULL) {
	sprintf(buf,"In exchange for %d quest points, %s gives you %s.\n\r",
		   reward_table[i].cost, questman->short_descr, obj->short_descr);
	send_to_char(buf,ch);
	if (obj->level > ch->level) {
	  obj->level = ch->level;
	}
	obj->owner = str_dup(ch->name);
	obj_to_char(obj, ch);
   }
   return;
 }

 else if (!strcmp(arg1, "request")) {
   act( "$n asks $N for a quest.", ch, NULL, questman, TO_ROOM);
   act("You ask $N for a quest.",ch, NULL, questman, TO_CHAR);
   if (IS_SET(ch->act,PLR_QUESTING)) {
	sprintf(buf, "But you're already on a quest!");
	do_say(questman, buf);
	return;
   }
   if (ch->pcdata->nextquest > 0) {
	sprintf(buf, "You're very brave, but let someone else have a chance.");
	do_say(questman, buf);
	sprintf(buf, "Come back later.");
	do_say(questman, buf);
	return;
   }
   
   sprintf(buf, "Thank you, brave adventurer!");
   if (!IS_SET(ch->act,PLR_QUESTING))
	do_say(questman, buf);
   ch->pcdata->questmob = 0;
   ch->pcdata->questobj = 0;
   generate_quest(ch, questman);
   
   if (ch->pcdata->questmob > 0 || ch->pcdata->questobj > 0) {
	ch->pcdata->countdown = number_range(25,35);
	SET_BIT(ch->act,PLR_QUESTING);
	sprintf(buf, "You have %d minutes to complete this quest.",ch->pcdata->countdown);
	do_say(questman, buf);
   }
   return;
 }
 
 else if (!strcmp(arg1, "complete")) {
   act("$n informs $N $e has completed $s quest.", ch, NULL, questman, TO_ROOM);
   act("You inform $N you have completed $s quest.",ch, NULL, questman, TO_CHAR);
   if (ch->pcdata->questgiver != questman) {
	sprintf(buf, "I never sent you on a quest! Perhaps you're thinking of someone else.");
	do_say(questman,buf);
	return;
   }
   
   if (IS_SET(ch->act,PLR_QUESTING)) {
	bool obj_found = FALSE;
	if (ch->pcdata->questobj > 0 && ch->pcdata->countdown > 0) {
	  for (obj = ch->carrying; obj != NULL; obj= obj_next) {
	    obj_next = obj->next_content;
	    
	    if (obj != NULL && obj->pIndexData->vnum == ch->pcdata->questobj) {
		 obj_found = TRUE;
		 break;
	    }
	  }
	}
	
     if ((ch->pcdata->questmob == -1 || (ch->pcdata->questobj && obj_found)) && ch->pcdata->countdown > 0) {
	  int reward, pointreward;
	  reward = number_range(1000,2000);
	  pointreward = number_range(40,60);
	  
	  sprintf(buf, "Congratulations on completing your quest!");
	  do_say(questman,buf);
	  
	  if (ch->pcdata->last_web_vote+3600 >= current_time) {
	     pointreward = pointreward*2;
	     send_to_char("{WD{ro{Wu{rb{Wl{re {WQP!!!{x\r\n", ch);
	  }
  	if (reward_multiplier > 0)
  	{
       	 	if (current_time <= reward_time)
       	 	{
                	pointreward=pointreward*reward_multiplier;
                	send_to_char("{WB{RO{WN{RU{WS XP {RR{WE{RW{WA{RR{WD!!!{x\r\n",ch);
        	}
        	else {
                	reward_multiplier = 0;
        	}
	 }

	  	  
	  sprintf(buf,"As a reward, I am giving you %d quest points, and %d silver.",pointreward,reward);
	  do_say(questman,buf);
	  
	  REMOVE_BIT(ch->act,PLR_QUESTING);
	  ch->pcdata->questgiver = NULL;
	  ch->pcdata->countdown = 0;
	  ch->pcdata->questmob = 0;
	  ch->pcdata->questobj = 0;
	  ch->pcdata->nextquest = number_range(5,10);
	  ch->silver += reward;
	  ch->pcdata->quest_curr += pointreward;
	  ch->pcdata->quest_accum += pointreward;
	  if(obj_found) extract_obj(obj);
	  return;
	}
     else if((ch->pcdata->questmob > 0 || ch->pcdata->questobj > 0) && ch->pcdata->countdown > 0) {
	  sprintf(buf, "You haven't completed the quest yet, but there is still time!");
	  do_say(questman, buf);
	  return;
	}
   }
   if (ch->pcdata->nextquest > 0)
	sprintf(buf,"But you didn't complete your quest in time!");
   else
	sprintf(buf, "You have to request a quest first.");
   do_say(questman, buf);
   return;
 }
 
 else if (!strcmp(arg1, "quit")) {
   act("$n informs $N $e wishes to quit $s quest.", ch, NULL,questman, TO_ROOM);
   act("You inform $N you wish to quit $s quest.",ch, NULL, questman, TO_CHAR);
   if (ch->pcdata->questgiver != questman) {
	sprintf(buf, "I never sent you on a quest! Perhaps you're thinking of someone else.");
	do_say(questman,buf);
	return;
   }
   
   if (IS_SET(ch->act,PLR_QUESTING)) {
	REMOVE_BIT(ch->act,PLR_QUESTING);
	ch->pcdata->questgiver = NULL;
	ch->pcdata->countdown = 0;
	ch->pcdata->questmob = 0;
	ch->pcdata->questobj = 0;
	ch->pcdata->nextquest = number_range(15,30);
	sprintf(buf, "Your quest is over, but for your cowardly behavior, you may not quest again for %d minutes.", ch->pcdata->nextquest);
	do_say(questman,buf);
	return;
   }
   else {
	send_to_char("You aren't on a quest!",ch);
	return;
   }
 }
 
 send_to_char("Quest commands: Info, Time, Request, Complete, Quit, List, Lore, and Buy.\n\r",ch);
 send_to_char("For more information, type 'Help Quest'.\n\r",ch);
 return;
}

bool has_exits(ROOM_INDEX_DATA *room)
{
  EXIT_DATA *pexit;
  int door;
  bool found=FALSE;;
  
  for ( door = 0; door <= 9; door++ ) {
    if (( pexit = room->exit[door] ) != NULL &&  pexit->u1.to_room != NULL)
	 found = TRUE;
  }
  
  return (found);  
}

void generate_quest(CHAR_DATA *ch, CHAR_DATA *questman)
{
  CHAR_DATA *victim;
  MOB_INDEX_DATA *vsearch;
  ROOM_INDEX_DATA *room;
  OBJ_DATA *questitem;
  char buf [MAX_STRING_LENGTH];
  long mcounter;
  bool found = FALSE;

  do 
  {
     for (mcounter = 0; mcounter < 99999; mcounter ++) {
       vsearch = get_mob_index(number_range(50, 65535));

       if (vsearch != NULL) {
	    if (quest_level_diff(ch->level, vsearch->level) == TRUE
		   && vsearch->pShop == NULL
    		   && !IS_SET(vsearch->act, ACT_TRAIN)
    		   && !IS_SET(vsearch->act, ACT_GAIN)
		   && !IS_SET(vsearch->act, ACT_REPAIRER)
    		   && !IS_SET(vsearch->act, ACT_IS_HEALER)
		   && !IS_SET(vsearch->act, ACT_IS_STRINGER)
		   && !IS_SET(vsearch->act, ACT_IS_CHANGER)
		   && !IS_SET(vsearch->act, ACT_PET)
		   && !IS_SET(vsearch->affected_by, AFF_CHARM)
		   && !IS_SET(vsearch->affected_by, AFF_INVISIBLE)
		   && IS_SET(vsearch->area->area_flags, AREA_OPEN)
		   && (strcmp(vsearch->area->name, "WORLD MAP")!=0) )
	      break;
	    else
	      vsearch = NULL;
       }
     }
     victim =  get_char_world( ch, vsearch->player_name);

     if (victim != NULL && !IS_SET(victim->in_room->room_flags, ROOM_NOQUEST)) {
	found = TRUE;
     }
   } while (!found);

  if ((victim == NULL)  || (vsearch->spec_fun == spec_lookup( "spec_questmaster" ))) 
  {
    		
    sprintf(buf, "I'm sorry, but I don't have any quests for you at this time.");
    do_say(questman, buf);
    sprintf(buf, "Try again later.");
    do_say(questman, buf);
    ch->pcdata->nextquest = 2;
    return;
  }
  else {
    // Since vsearch and acctual found victim don't have to be the same
    // same check as for vsearch here too.
    if (IS_NPC(victim) && (quest_level_diff(ch->level, victim->level) != TRUE
	   || victim->pIndexData->pShop != NULL
	   || IS_SET(victim->act, ACT_TRAIN)
	   || IS_SET(victim->act, ACT_GAIN)
	   || IS_SET(victim->act, ACT_ROAMERTRAINER)
	   || IS_SET(victim->act, ACT_REPAIRER)
	   || IS_SET(victim->act, ACT_IS_HEALER)
	   || IS_SET(victim->act, ACT_IS_STRINGER)
	   || IS_SET(victim->act, ACT_IS_CHANGER)
	   || IS_SET(victim->act, ACT_PET)
	   || IS_SET(victim->affected_by, AFF_CHARM)
	   || IS_SET(victim->affected_by, AFF_INVISIBLE)
           || IS_SET(victim->in_room->room_flags, ROOM_NOQUEST)
           || (victim->pIndexData->pShop != NULL)
           || (victim->spec_fun == spec_lookup( "spec_questmaster" ))
	   || (!str_cmp(victim->in_room->area->file_name, "school.are"))
	   || (!str_cmp(victim->in_room->area->file_name, "vmap.are"))
	   || (!IS_SET(victim->in_room->area->area_flags, AREA_OPEN))
	   || (!has_exits(victim->in_room))))
	 {
	 sprintf(buf, "I'm sorry, but I don't have any quests for you at this time.");
	 do_say(questman, buf);
	 sprintf(buf, "Try again later.");
	 do_say(questman, buf);
	 ch->pcdata->nextquest = 1;
	 return;
    }
  }

  /* Find_path lags the mud to no end.  Only uncomment this if tracking is updated to use a better algorithm */

 /*
  if ( -1 == find_path( ch->in_room->vnum, victim->in_room->vnum, ch, -40000,FALSE))
  {
	 sprintf(buf,"Your quest would have been for %s, but they are unreachable.",victim->name);
	 do_say(questman,buf);
         sprintf(buf, "I'm sorry, but I don't have any quests for you at this time.");
         do_say(questman, buf);
         sprintf(buf, "Try again later.");
         do_say(questman, buf);
         ch->pcdata->nextquest = 1;
         return;
  }
  */

  room = victim->in_room;

  if (room == NULL) {
//  if ( ( room = find_location( ch, victim->name ) ) == NULL ) {
    sprintf(buf, "I'm sorry, but I don't have any quests for you at this time.");
    do_say(questman, buf);
    sprintf(buf, "Try again later.");
    do_say(questman, buf);
    ch->pcdata->nextquest = 1;
    return;
  }

  /*  40% chance it will send the player on a 'recover item' quest. */
  if (number_percent() < 40)  {
    int numobjs=0;
    int descnum = 0;

    room = NULL;
    // Get a random room then
    while ( (room == NULL)
		  || (!str_cmp(room->area->file_name, "school.are"))
		  || (!str_cmp(room->area->file_name, "vmap.are"))
		  || (!IS_SET(room->area->area_flags, AREA_OPEN))
		  || (IS_SET(room->room_flags, ROOM_NOQUEST))
		  || (!has_exits(room))) 
    {
        room = get_random_room(ch, TRUE);
    }
    if (IS_CODER(ch)) {
	 sprintf(buf, "[ {YCoder {x]: Quest object is located in room <%d>\n\r", room->vnum);
	 send_to_char(buf, ch);
    }

    for(numobjs=0;quest_desc[numobjs].name != NULL;numobjs++);

    numobjs--;
    descnum = number_range(0,numobjs);
    questitem = create_object( get_obj_index(OBJ_VNUM_QUEST), ch->level );
    if(descnum > -1) {
	 if(questitem->short_descr)
	   free_string(questitem->short_descr);
	 if(questitem->description)
	   free_string(questitem->description);
	 if(questitem->name)
	   free_string(questitem->name);

	 questitem->name        = str_dup(quest_desc[descnum].name);
	 questitem->description = str_dup(quest_desc[descnum].long_descr);
	 questitem->short_descr = str_dup(quest_desc[descnum].short_descr);
    }
    obj_to_room(questitem, room);
    ch->pcdata->questobj = questitem->pIndexData->vnum;

    sprintf(buf, "Vile pilferers have stolen %s from me!",questitem->short_descr);
    do_say(questman, buf);
    do_say(questman, "My advisors have pinpointed its location.");

    sprintf(buf, "Look in the general area of %s for %s!",room->area->name, room->name);
    do_say(questman, buf);
    return;
  }

  /* Quest to kill a mob */
  else {
    switch(number_range(0,1)) {
    case 0:
	 sprintf(buf, "An enemy of mine, %s, is making vile threats against the crown.",victim->short_descr);
	 do_say(questman, buf);
	 sprintf(buf, "This threat must be eliminated!");
	 do_say(questman, buf);
	 break;

    case 1:
	 sprintf(buf, "%s's most heinous criminal, %s, has escaped from the dungeon!", questman->in_room->area->name, victim->short_descr);
	 do_say(questman, buf);
	 sprintf(buf, "Since the escape, %s has murdered %d civillians!",victim->short_descr, number_range(2,20));
	 do_say(questman, buf);
	 do_say(questman,"The penalty for this crime is death, and you are to deliver the sentence!");
	 break;
    }

    if (room->name != NULL) {
	 sprintf(buf, "Seek %s out somewhere in the vicinity of %s!",victim->short_descr,room->name);
	 do_say(questman, buf);

	 sprintf(buf, "That location is in the general area of %s.",room->area->name);
	 do_say(questman, buf);

	 if (IS_CODER(ch)) {
	   sprintf(buf, "[ {YCoder {x]: Quest MOB is located in room <%d>\n\r", room->vnum);
	   send_to_char(buf, ch);
	 }
    }
    ch->pcdata->questmob = victim->pIndexData->vnum;
  }

  return;
}

bool quest_level_diff(int clevel, int mlevel)
{
    if (clevel < 9 && mlevel < clevel + 2) return TRUE;
    else if (clevel <= 9 && mlevel < clevel + 3
	  && mlevel > clevel - 5) return TRUE;
    else if (clevel <= 14 && mlevel < clevel + 4
	  && mlevel > clevel - 5) return TRUE;
    else if (clevel <= 21 && mlevel < clevel + 5
	  && mlevel > clevel - 4) return TRUE;
    else if (clevel <= 29 && mlevel < clevel + 6
	  && mlevel > clevel - 3) return TRUE;
    else if (clevel <= 37 && mlevel < clevel + 7
	  && mlevel > clevel - 2) return TRUE;
    else if (clevel <= 55 && mlevel < clevel + 8
	  && mlevel > clevel - 1) return TRUE;
    else if(clevel > 55) return TRUE; /* Imms can get anything :) */
    else return FALSE;
}

/* Called from update_handler() by pulse_area */

void quest_update(void)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *ch;
    char buf[MAX_INPUT_LENGTH];

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if (d->character != NULL && d->connected == CON_PLAYING)
        {
        ch = d->character;
	if(IS_NPC(ch))
	continue;
        if (ch->pcdata->nextquest > 0)
        {
            ch->pcdata->nextquest--;
            if (ch->pcdata->nextquest == 0)
            {
                send_to_char("{WYou may now quest again.{x\n\r",ch);
                return;
            }
        }
        else if (IS_SET(ch->act,PLR_QUESTING))
        {
            if (--ch->pcdata->countdown <= 0)
            {
                ch->pcdata->nextquest = 10;
                sprintf(buf,"{RYou have run out of time for your quest!{x\n\rYou may quest again in %d minutes.\n\r",ch->pcdata->nextquest);
		send_to_char(buf,ch);
                REMOVE_BIT(ch->act,PLR_QUESTING);
                ch->pcdata->questgiver = NULL;
                ch->pcdata->countdown = 0;
                ch->pcdata->questmob = 0;
                ch->pcdata->questobj = 0;
            }
            if (ch->pcdata->countdown > 0 && ch->pcdata->countdown < 6)
            {
                send_to_char("{CBetter hurry, you're almost out of time for your quest!{x\n\r",ch);
                return;
            }
        }
        }
    }
    return;
}
