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
*	    Gabrielle Taylor (gtaylor@efn.org)				   *
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

ROOM_INDEX_DATA *find_city(CHAR_DATA *ch, char *city_name)
{
  ROOM_INDEX_DATA *room;
  unsigned int vnum=0;

  for (vnum=40000; vnum <= 58000; vnum++) {
    room = get_room_index(vnum);
    
    if (room == NULL)
	 continue;

    if (room->sector_type != SECT_ENTER && room->sector_type != SECT_CITY)
	 continue;
    
    if (IS_NULLSTR(room->name))
	 continue;
    
    if (!str_prefix(city_name, room->name))
	 return room;
  }

  return NULL;
}

/* random room generation procedure */
ROOM_INDEX_DATA  *get_random_room(CHAR_DATA *ch, bool use_vmap )
{
  ROOM_INDEX_DATA *room;
  unsigned int vnum=0;
  
  for ( ; ; ) {
    vnum = number_range(0, 65535);
    //room = get_room_index( number_range( 0, 65535 ) );
    
    // If newbie school, continue.
    // A little hack, but alas
    if ( vnum >= 3700 && vnum <= 3799) {
	 continue;
    }
    
    // If we don't want to use the vmap
    if (!use_vmap) {
	 if (vnum >= 40000 && vnum <= 58000)
	   continue;
    }
    
    room = get_room_index(vnum);
    
    if ( room != NULL )
	 if ( can_see_room(ch,room)
		 &&   !room_is_private(room)
		 &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
		 &&   !IS_SET(room->room_flags, ROOM_SOLITARY) 
		 &&   !IS_SET(room->room_flags, ROOM_SAFE) 
		 &&    IS_SET(room->area->area_flags, AREA_OPEN)
		 &&   room->sector_type != SECT_WATER_SWIM
		 &&   room->sector_type != SECT_WATER_NOSWIM
		 &&   room->sector_type != SECT_AIR
		 &&   room->sector_type != SECT_OCEAN
		 &&   room->sector_type != SECT_RIVER
		 &&   room->sector_type != SECT_LAKE
		 &&   has_exits(room)
		 &&   !(IS_SET(room->room_flags, ROOM_NOQUEST))
		 &&   (IS_NPC(ch) || IS_SET(ch->act,ACT_AGGRESSIVE)
		 ||   !IS_SET(room->room_flags,ROOM_LAW)))
	   break;
  }

  return room;
}

void reset_skimming(CHAR_DATA *ch)
{
  ch->skim        = FALSE;
  ch->skim_pulse  = -1;
  ch->skim_motion = FALSE;
  ch->skim_to     = 0;
}

bool check_skim_group(CHAR_DATA *ch, OBJ_DATA *gate)
{
  char buf[MSL];
  CHAR_DATA *vch=NULL;
  CHAR_DATA *vch_next=NULL;

  int followers=0;
  int max_follow = gate->value[0];
  
  if (max_follow < 1)
    max_follow = 1;
    
  if (max_follow > 10)
    max_follow = 10;
    
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
   vch_next = vch->next;
   if (!IS_SAME_WORLD(vch, ch))
     continue;
   if (!is_same_group(ch, vch))
     continue;	   
   followers++;
  }
  
  if (followers <= max_follow)
     return TRUE;
  
  sprintf(buf, "You are to weak to sustain a platform in the {Ddark void{x large enough for a group of %d persons.\n\r", followers);
  send_to_char(buf, ch);  
  return FALSE;	
}

/* RT Enter portals                 */
/* Swordfish:: Enter Cities added   */
void do_enter( CHAR_DATA *ch, char *argument)
{    
  ROOM_INDEX_DATA *location;
  int door=10;
  char buf2[MAX_STRING_LENGTH];
  ROOM_INDEX_DATA *in_room;
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit;
  bool inTAR=FALSE;
  
  if ( ch->fighting != NULL ) 
    return;
  
  /* nifty portal stuff */
  if (argument[0] != '\0') {
    ROOM_INDEX_DATA *old_room;
    OBJ_DATA *portal;
    CHAR_DATA *fch, *fch_next, *mount;
    char buf[MAX_STRING_LENGTH];
    old_room = ch->in_room;


    portal = get_obj_list( ch, argument,  ch->in_room->contents );

    /********
	* Enter Areas from Vmap
	**********/
    
    if (IS_SET(ch->in_room->room_flags, ROOM_VMAP) && (ch->in_room->sector_type == SECT_ENTER) && portal == NULL) {
	 if(!str_prefix(argument, "north" )) {
	   door = 6;
	   sprintf(buf, "north");
	 }
	 if(!str_prefix(argument, "east" )) {
	   door = 7;
	   sprintf(buf, "east");
	 }
	 if(!str_prefix(argument, "south" )) {
	   door = 8;
	   sprintf(buf, "south");
	 }
	 if(!str_prefix(argument, "west" )) {
	   door = 9;
	   sprintf(buf, "west");
	 }
	 in_room = ch->in_room;
	 if (( pexit = in_room->exit[door]) == NULL
		|| (to_room = pexit->u1.to_room) == NULL) {
	   send_to_char("You don't see that here.\n\r",ch);
	   return;
	 }
	 sprintf(buf2, "$n enters the area from the %s.", buf);
	 act(buf2, ch, NULL, NULL, TO_ROOM );
	 char_from_room( ch );
	 char_to_room( ch, to_room );
	 act( "$n has entered the area.", ch, NULL, NULL, TO_ROOM );
	 do_function(ch, &do_look, "auto" );

	 /* FOLLOW CODE */
	 for ( fch = in_room->people; fch != NULL; fch = fch_next ) {
	   fch_next = fch->next_in_room;
	   
	   if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM) 
		   &&   fch->position < POS_STANDING)
		do_function(fch, &do_stand, "");
	   
	   if ( fch->master == ch && fch->position == POS_STANDING) {
		act( "You follow $N.", fch, NULL, ch, TO_CHAR );
		do_enter( fch, buf);
	   }
	 }
	 
	 if (ch->mount != NULL && (ch->in_room == ch->mount->in_room)) {
	    ch->mount_quiet = TRUE;
	    do_mount(ch, ch->mount->name);
	 }
	 
	 return;
    }
            
    if (portal == NULL) {
	 send_to_char("You don't see that here.\n\r",ch);
	 return;
    }
    
    if ( (portal->item_type != ITEM_PORTAL)
	    && (portal->item_type != ITEM_CONTAINER)
            && (portal->item_type != ITEM_VEHICLE))  {
	 send_to_char("You can't enter that.\n\r",ch);
	 return;
    }
    
    /**************************
	* Container handler first
	***************************/
    if (portal->item_type == ITEM_CONTAINER) {
	 if (ch->in_obj)  {
	   act( "You're already in $p.", ch, ch->in_obj, NULL, TO_CHAR );
	   return;
	 }
	 
	 if (!IS_SET(portal->value[1],CONT_ENTERABLE)) {
	   send_to_char("You can't enter that.\n\r",ch);
	   return;
	 }
	 if (portal->carried_by) {
	   send_to_char("It must be on the ground.\n\r",ch);
	   return;
	 }
	 if (IS_SET(portal->value[1],CONT_CLOSED)) {
	   send_to_char("It is closed.\n\r",ch);
	   return;
	 }
	 if (IS_SET(portal->value[1],CONT_LOCKED)) {
	   send_to_char("It is locked.\n\r",ch);
	   return;
	 }

	 if( portal->in_obj ) {
	   act( "Not while it's inside $p.", ch, portal->in_obj, NULL, TO_CHAR );
	   return;
	 }

	 char_to_obj(ch,portal); 
	 act( "$n climbs into $p you're in.", ch, portal, NULL, TO_CONT);
	 act( "$n climbs into $p.", ch, portal, NULL, TO_OUTSIDE_CONT );
	 act( "You climb into $p.", ch, portal, NULL, TO_CHAR );

       
	 return;

    }


   /* It's a vehicle *cheer* */
        if (portal->item_type == ITEM_VEHICLE)
        {
/*
            if ((!IS_TRUSTED (ch, ANGEL)
                 && !IS_SET (portal->value[2], GATE_NOCURSE)))
            {
                send_to_char ("Something prevents you from leaving...\n\r", ch);
                return;
            }
*/
            if (IS_SET(portal->value[1],EX_CLOSED) ) 
            {
	 	send_to_char("The door is closed.\n\r",ch);
	 	return;
   	    }


            location = get_room_index (portal->value[3]);

            if (location == NULL
                || location == old_room
                || !can_see_room (ch, location)
            || (room_is_private (location) && !IS_TRUSTED (ch, IMPLEMENTOR)))
            {
                act ("$p doesn't seem to go anywhere.", ch, portal, NULL, TO_CHAR);
                return;
            }

            if (IS_NPC (ch) && IS_SET (ch->act, ACT_AGGRESSIVE)
                && IS_SET (location->room_flags, ROOM_LAW))
            {
                send_to_char ("Something prevents you from leaving...\n\r", ch);
                return;
            }

            act ("$n enters $p.", ch, portal, NULL, TO_ROOM);

            char_from_room (ch);
            char_to_room (ch, location);

            act ("$n has entered $p.", ch, portal, NULL, TO_ROOM);

            do_look (ch, "auto");

        /* protect against circular follows */
            if (old_room == location)
                return;

            if (portal->item_type == ITEM_VEHICLE)
            {
                ch->in_room->inside_of = portal;
            }

            for (fch = old_room->people; fch != NULL; fch = fch_next)
            {
                fch_next = fch->next_in_room;

            /* no following through dead portals */
                if (portal == NULL || portal->value[0] == -1)
                    continue;

                if (fch->master == ch && IS_AFFECTED (fch, AFF_CHARM)
                    && fch->position < POS_STANDING)
                    do_stand (fch, "");

                if (fch->master == ch && fch->position == POS_STANDING)
                {
                    if (IS_SET (ch->in_room->room_flags, ROOM_LAW)
                     && (IS_NPC (fch) && IS_SET (fch->act, ACT_AGGRESSIVE)))
                    {
                        act ("You can't take $N into vehicles.",
                             ch, NULL, fch, TO_CHAR);
                        act ("You aren't allowed in vehicles.",
                             fch, NULL, NULL, TO_CHAR);
                        continue;
                    }

                    act ("You follow $N.", fch, NULL, ch, TO_CHAR);
                    do_enter (fch, argument);
                }
            }

      /*
         * If someone is following the char, these triggers get activated
         * for the followers before the char, but it's safer this way...
         *
         * Remove the following four lines of code if you do not have mob
         * programs installed.
         */

            if (IS_NPC (ch) && HAS_TRIGGER (ch, TRIG_ENTRY))
                mp_percent_trigger (ch, NULL, NULL, NULL, TRIG_ENTRY);
            if (!IS_NPC (ch))
                mp_greet_trigger (ch);

            return;
        }


    
    /******
	*  Portal stuff
	******/
    
    if ((portal->item_type == ITEM_PORTAL || portal->item_type == ITEM_VEHICLE)
        &&  (IS_SET(portal->value[1],EX_CLOSED) )) {
	 send_to_char("You can't seem to find a way in.\n\r",ch);
	 return;
    }

    if (portal->item_type == ITEM_CONTAINER 
        &&  (IS_SET(portal->value[1],CONT_CLOSED) && !IS_TRUSTED(ch,ANGEL))) {
	 send_to_char("You can't seem to find a way in.\n\r",ch);
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
    
    if (IS_SET(portal->value[2],GATE_DREAMGATE)) {
	   if (IS_DREAMING(ch)) {
		act("Something is preventing you from entering the $p.", ch, portal, NULL, TO_CHAR);
		return;
	   }
    }

    if (location == NULL
	   ||  location == old_room
	   ||  !can_see_room(ch,location) 
	   ||  (room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR))) {

	 /* DREAMGATE */
	 if (IS_SET(portal->value[2],GATE_DREAMGATE)) {
	   if (IS_DREAMING(ch)) {
		act("Something is preventing you from entering the $p.", ch, portal, NULL, TO_CHAR);
		return;
	   }
	   act("You enter $p.",ch,portal,NULL,TO_CHAR);
	   act("$n steps into $p.",ch,portal,NULL,TO_ROOM);
	   TOGGLE_BIT(ch->world, WORLD_NORMAL);
	   TOGGLE_BIT(ch->world, WORLD_TAR_FLESH);
	   inTAR=TRUE;
	 }
	 else {
	   act("$p doesn't seem to go anywhere.",ch,portal,NULL,TO_CHAR);
	   return;
	 }
    }

    if (IS_SET(portal->value[2],GATE_SKIMMING_IN) && !IS_SKIMMING(ch) && !ch->skim_follow) {
	 int sn = skill_lookup("skimming");
	 if (IS_NPC(ch))
	   return;
	 if (ch->pcdata->learned[sn] < 1) {
	   act("You look into $p and see a {Ddark void{x and get a feeling that entering alone might be deadly!", ch, portal, NULL, TO_CHAR);
	   return;
	 }
    }

    // Skimming
    if (IS_SET(portal->value[2],GATE_SKIMMING_IN) && IS_SKIMMING(ch)) {
	 act("You are unable to reach $p from the glistening silvery platform you stand on!", ch, portal, NULL, TO_CHAR);
	 return;
    }
    
    // Skimming
    if (IS_SET(portal->value[2],GATE_SKIMMING_OUT) && !IS_SKIMMING(ch)) {
	 act("Something prevent you from entering $p!", ch, portal, NULL, TO_CHAR);
	 return;
    }
    
    if (IS_SET(portal->value[2],GATE_SKIMMING_IN)) {
       if (!check_skim_group(ch, portal))
          return;
    }
    
    if (IS_NPC(ch) && IS_SET(ch->act,ACT_AGGRESSIVE)
        &&  IS_SET(location->room_flags,ROOM_LAW)) {
	 send_to_char("Something prevents you from leaving...\n\r",ch);
	 return;
    }
    if (MOUNTED(ch)) 
	 sprintf(buf,"$n steps into $p, riding on %s.", MOUNTED(ch)->short_descr );
    else 
	 sprintf(buf,"$n steps into $p." );
    if (!inTAR)
	 act(buf,ch,portal,NULL,TO_ROOM);

    if (IS_SET(portal->value[2],GATE_WAYGATE)) {
	 act("$n steps through the shimmering Waygate.",ch,portal,NULL,TO_ROOM);
	 act("You slide through the shimmering Waygate.",ch,portal,NULL,TO_CHAR);
    }
    
    if (IS_SET(portal->value[2],GATE_NORMAL_EXIT))
	 act("You enter $p.",ch,portal,NULL,TO_CHAR);
    else
	 act("You walk through $p and find yourself somewhere else...", ch,portal,NULL,TO_CHAR); 

    mount = MOUNTED(ch);
    char_from_room(ch);
    char_to_room(ch, location);

    // Skimming
    if (IS_SET(portal->value[2],GATE_SKIMMING_IN)) {
	 TOGGLE_BIT(ch->world, WORLD_NORMAL);
	 TOGGLE_BIT(ch->world, WORLD_SKIMMING);
    }
    else if (IS_SET(portal->value[2],GATE_SKIMMING_OUT)) {
	 TOGGLE_BIT(ch->world, WORLD_NORMAL);
	 TOGGLE_BIT(ch->world, WORLD_SKIMMING);
	 reset_skimming(ch);
    }

    if (ch->skim_follow)
	 ch->skim_follow = FALSE;
    
    if (IS_SET(portal->value[2],GATE_GOWITH)) { /* take the gate along */
	 obj_from_room(portal);
	 obj_to_room(portal,location);
    }
    
    if (IS_SET(portal->value[2],GATE_NORMAL_EXIT)) {
	 if (mount)
	   act("$n has arrived, riding $N",ch,portal,mount,TO_ROOM);
	 else 
	   act_new("$n has arrived.", ch,argument,NULL,TO_ROOM, POS_RESTING);
    }
    else {
	 if (mount)
	   act("$n has arrived through $p, riding $N.",ch,portal,mount,TO_ROOM);
	 else  act("$n has arrived through $p.",ch,portal,NULL,TO_ROOM);
    }
    
    do_function(ch, &do_look, "auto");

    if (IS_SET(portal->value[2],GATE_SKIMMING_IN)) {
	 send_to_char("A glistening {Wsilvery{x platform widens out beneath your feet to stand on!\n\r", ch);
    }

    if (mount) {
	 char_from_room( mount );
	 char_to_room( mount, location);
	 ch->riding = TRUE;
	 mount->riding = TRUE;
	 
	 /* DREAMGATE MOUNTS */
	 if (IS_SET(portal->value[2],GATE_DREAMGATE)) {
	   TOGGLE_BIT(mount->world, WORLD_NORMAL);
	   TOGGLE_BIT(mount->world, WORLD_TAR_FLESH);	   
	 }
	 
	 // SKIMMING MOUNTS
         if (IS_SET(portal->value[2],GATE_SKIMMING_IN) || IS_SET(portal->value[2],GATE_SKIMMING_OUT)) {
	   TOGGLE_BIT(mount->world, WORLD_NORMAL);
	   TOGGLE_BIT(mount->world, WORLD_SKIMMING);
         }    		 
    }
    
    /* charges */
    if (portal->value[0] > 0) {
	 portal->value[0]--;
	 if (portal->value[0] == 0)
	   portal->value[0] = -1;
    }

    /* protect against circular follows */
    if (old_room == location && !IS_SET(portal->value[2],GATE_DREAMGATE))
	 return;
    
    for ( fch = old_room->people; fch != NULL; fch = fch_next ) {
	 fch_next = fch->next_in_room;
	 
	 if (portal == NULL || portal->value[0] == -1)
	   /* no following through dead portals */
	   continue;
	 
	 if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM) &&   fch->position < POS_STANDING)
	   do_function(fch, &do_stand, "");
	 
	 if ( fch->master == ch && fch->position == POS_STANDING) {

	   if (mount && fch == mount)
		continue;

	   // already there
	   if (IS_SET(portal->value[2],GATE_DREAMGATE) && IS_SAME_WORLD(ch, fch))
		continue;
	   
	   if (IS_SET(portal->value[2],GATE_SKIMMING_IN) && IS_SAME_WORLD(ch, fch))
		continue;

	   if (IS_SET(portal->value[2],GATE_SKIMMING_OUT) && IS_SAME_WORLD(ch, fch))
		continue;
	   
	   if (IS_SET(ch->in_room->room_flags,ROOM_LAW) && (IS_NPC(fch) && IS_SET(fch->act,ACT_AGGRESSIVE))) {
		act("You can't bring $N into the city.", ch,NULL,fch,TO_CHAR);
		act("You aren't allowed in the city.", fch,NULL,NULL,TO_CHAR);
		continue;
	   }

	   if (IS_SET(portal->value[2],GATE_SKIMMING_IN) && IS_SHADOWSPAWN(fch)) {
		act("$N doesn't look like $E dare to enter.", ch,NULL,fch,TO_CHAR);
		act("You don't dare to enter $p.", fch,portal,NULL,TO_CHAR);
		continue;
	   }
	
	   fch->skim_follow = TRUE;
   
	   act( "You follow $N.", fch, NULL, ch, TO_CHAR );
	   do_function(fch, &do_enter, argument);
	 }
    }
    
    if (portal != NULL && portal->value[0] == -1) {
	 act("$p fades out of existence.",ch,portal,NULL,TO_CHAR);
	 if (ch->in_room == old_room)
	   act("$p fades out of existence.",ch,portal,NULL,TO_ROOM);
	 else if (old_room->people != NULL) {
	   act("$p fades out of existence.", 
		  old_room->people,portal,NULL,TO_CHAR);
	   act("$p fades out of existence.",
		  old_room->people,portal,NULL,TO_ROOM);
	 }
	 extract_obj(portal);
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
  
  send_to_char("Nope, can't do it.\n\r",ch);
  return;
}

void do_leave( CHAR_DATA *ch, char *argument)
{
  int door=10;
  /* char buf[MAX_STRING_LENGTH]; */
  ROOM_INDEX_DATA *in_room;
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit;
  CHAR_DATA *fch;
  CHAR_DATA *fch_next;
  
  /********
   * Leav Areas linked to Vmap
   **********/
  if (!IS_SET(ch->in_room->room_flags, ROOM_VMAP)) {
    in_room = ch->in_room;
    for (door=6; door <= 9; door++) {
	 if (( pexit = in_room->exit[door]) == NULL
		|| (to_room = pexit->u1.to_room) == NULL) {
	   continue;
	 }
	 act( "$n leaves the area.", ch, NULL, NULL, TO_ROOM );
	 char_from_room( ch );
	 char_to_room( ch, to_room );
	 act( "$n arrives from the area.", ch, NULL, NULL, TO_ROOM );
	 do_function(ch, &do_look, "auto" );

	 /* FOLLOW CODE */
	 for ( fch = in_room->people; fch != NULL; fch = fch_next ) {
	   fch_next = fch->next_in_room;

	   if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM) 
		   &&   fch->position < POS_STANDING)
		do_function(fch, &do_stand, "");
	   
	   if ( fch->master == ch && fch->position == POS_STANDING) {
		act( "You follow $N.", fch, NULL, ch, TO_CHAR );
		do_leave( fch, "");
	   }
	 }
	 
	 if (ch->mount != NULL) {
	    if (ch->mount->in_room == ch->in_room)
            {
	       ch->mount_quiet = TRUE;
	       do_mount(ch, ch->mount->name);
	    }
	 }
	 return;
    }
  }

  if (!ch->in_obj && !ch->in_room->inside_of) {
    send_to_char("You aren't in anything.\n\r",ch);
    return;
  }
  
  if (ch->in_obj)
  {
     if (IS_SET(ch->in_obj->value[1],CONT_CLOSED)) {
       send_to_char("The entrance is closed.\n\r",ch);
       return;
     }

     act("$n leaves $p you're in.", ch, ch->in_obj, NULL, TO_CONT );
     act("$n comes out of $p.", ch, ch->in_obj, NULL, TO_OUTSIDE_CONT );
     act("You leave $p.", ch, ch->in_obj, NULL, TO_CHAR);
     char_from_obj(ch);
  }   
  else
  if (ch->in_room->inside_of)
  {
    char buf[MAX_STRING_LENGTH];
    if (IS_SET(ch->in_room->inside_of->value[1],EX_CLOSED))
    {
	send_to_char("The door is closed.\r\n",ch);
	return;
    }
    sprintf (buf, "%s leaves %s.", IS_NPC (ch) ? ch->short_descr :
             capitalize (ch->name), ch->in_room->inside_of->short_descr);

    act ("You leave $T.", ch, NULL, ch->in_room->inside_of->short_descr,
         TO_CHAR);

    act (buf, ch, NULL, NULL, TO_ROOM);

/* Icky long pointer dereferences */
    to_room = ch->in_room->inside_of->in_room;

/* Move character */
    char_from_room (ch);
    char_to_room (ch, to_room);
    act (buf, ch, NULL, NULL, TO_ROOM);
  }
  do_look(ch,"auto");
  return;
}
