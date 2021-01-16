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
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@hypercube.org)				   *
*	    Gabrielle Taylor (gtaylor@hypercube.org)			   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

/*
 * Vehicle.c - ROM/ROT vehicle code. Ver. 2.1b2
 *
 * This Code is Copyright 1997-1999 by Dominic J. Eidson, however, the 
 * code may be freely distributed and modified.
 *
 * $Id: vehicle.c,v 1.1.1.1 2010/11/17 17:54:24 tsw Exp $
 */

/*
 * Port to ROM 2.4 copyright 1998 by Anthony Michael Tregre, permission
 * to use granted provided that this header and Dominic J. Eidson's are
 * both kept intact and unaltered.  -- Kohl Desenee
 */

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

/* command procedures needed */
DECLARE_DO_FUN (do_stand);

/*
 * Local functions.
 */
int find_hack_door (CHAR_DATA * ch, char *arg);
void move_vehicle (CHAR_DATA * ch, OBJ_DATA * obj, int door);
bool check_blind (CHAR_DATA * ch);
void show_char_to_char (CHAR_DATA * list, CHAR_DATA * ch);
void do_hack_exits (CHAR_DATA * ch, ROOM_INDEX_DATA * room,
		    char *argument);
void show_list_to_char (OBJ_DATA * list, CHAR_DATA * ch,
			bool fShort, bool fShowNothing);
void do_hack_look (CHAR_DATA * ch, char *argument);
void do_look (CHAR_DATA * ch, char *argument);

/*
 * Take care of exiting a vehicle.
 */
void do_vehicle_leave (CHAR_DATA * ch)
{
    ROOM_INDEX_DATA *room;
    char buf[MAX_STRING_LENGTH];

/* Are we in a vehicle? */
    if (ch->in_room->inside_of == NULL)
    {
	send_to_char ("You are not riding anything.", ch);
	return;
    }
    sprintf (buf, "%s leaves %s.", IS_NPC (ch) ? ch->short_descr :
	     capitalize (ch->name), ch->in_room->inside_of->short_descr);

    act ("You leave $T.", ch, NULL, ch->in_room->inside_of->short_descr,
	 TO_CHAR);

    act (buf, ch, NULL, NULL, TO_ROOM);

/* Icky long pointer dereferences */
    room = ch->in_room->inside_of->in_room;

/* Move character */
    char_from_room (ch);
    char_to_room (ch, room);
    act (buf, ch, NULL, NULL, TO_ROOM);
    do_look (ch, "auto");
}

void do_hack_look (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    int number, count;

/* NPC? - Are we inside a vehicle? */
    if (IS_NPC (ch) && !ch->in_room->inside_of)
	return;

/* Vehicle can be taken? */
    if(!ch->in_room->inside_of->in_room)
	return;

/* Nothing to be seen if we're sleeping */
    if (ch->position < POS_SLEEPING)
    {
	send_to_char ("You can't see anything but stars.\n\r", ch);
	return;
    }

/* Nothing to be seen if we're sleeping */
    if (ch->position == POS_SLEEPING)
    {
	send_to_char ("You feel a gentle rocking.\n\r", ch);
	return;
    }

/* Blind? */
    if (!check_blind (ch))
	return;

/* Om Natta er alle katter graa */
/*
    if (!IS_NPC (ch)
	&& !IS_SET (ch->act, PLR_HOLYLIGHT)
	&& room_is_dark (ch->in_room->inside_of->in_room))
    {
	send_to_char ("It is pitch black ... \n\r", ch);
	show_char_to_char (ch->in_room->inside_of->in_room->people, ch);
	return;
    }
*/

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    number = number_argument (arg1, arg3);
    count = 0;

/* Take care of showing everybody the surroundings... */
    if (arg1[0] == '\0' || !str_cmp (arg1, "auto"))
    {
    /* 'look' or 'look auto' */
	send_to_char ("{e", ch);
	send_to_char (ch->in_room->inside_of->in_room->name, ch);
	send_to_char ("{x", ch);

	if (IS_NPC (ch) || IS_SET (ch->act, PLR_HOLYLIGHT))
	{
	    sprintf (buf, " [Room %d]", ch->in_room->inside_of->in_room->vnum);
	    send_to_char (buf, ch);
	}

	send_to_char ("\n\r", ch);

      if ((ch->in_room->inside_of->in_room->sector_type != SECT_INSIDE) && 
          (IS_SET(ch->in_room->inside_of->in_room->room_flags, ROOM_VMAP))) {
         send_to_char( "{x\n\r", ch );
         do_function(ch, &do_map, "8");
         send_to_char( "{x\n\r", ch );
      }
	if (arg1[0] == '\0'
	    || (!IS_NPC (ch) && !IS_SET (ch->comm, COMM_BRIEF)))
	{
	    send_to_char ("  ", ch);
	    send_to_char (ch->in_room->inside_of->in_room->description, ch);
	}

	if (!IS_NPC (ch) && IS_SET (ch->act, PLR_AUTOEXIT))
	{
	    send_to_char ("\n\r", ch);
	    do_hack_exits (ch, ch->in_room->inside_of->in_room, "auto");
	}

	show_list_to_char (ch->in_room->inside_of->in_room->contents, ch, FALSE, FALSE);
	show_char_to_char (ch->in_room->inside_of->in_room->people, ch);
	return;
    }
    return;
}

/* Auto-exits, if they're set */

void do_hack_exits (CHAR_DATA * ch, ROOM_INDEX_DATA * room, char *argument)
{
    extern char *const dir_name[];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;
    int door;
    bool vmap_exit = FALSE;


    fAuto = !str_cmp (argument, "auto");

    if (!check_blind (ch))
	return;

    if (fAuto)
	sprintf (buf, "[Exits:");
    else if (IS_IMMORTAL (ch))
	sprintf (buf, "Obvious exits from room %d:\n\r", room->vnum);
    else
	sprintf (buf, "Obvious exits:\n\r");

    found = FALSE;
    for (door = 0; door <= 9; door++)
    {

	if ((pexit = room->exit[door]) != NULL
	    && pexit->u1.to_room != NULL
	    && can_see_room (ch, pexit->u1.to_room)
	    && !IS_SET (pexit->exit_info, EX_CLOSED))
	{
	    found = TRUE;
	    if (fAuto)
	    {
//		strcat (buf, " ");
//		strcat (buf, dir_name[door]);
                 /* Entrance from VMAP */
                 if (door > 5) {
                   if (room->exit[door] == NULL)
                        continue;
                   if (IS_SET(room->room_flags, ROOM_VMAP) && (room->sector_type == SECT_ENTER)) {
                        if (!vmap_exit) {
                          sprintf(buf2, " enter(");
                          strcat( buf, buf2);
                          vmap_exit = TRUE;
                        }
                        if (door == 6) {
                          sprintf(buf2, "n");
                          strcat( buf, buf2);
                        }
                        if (door == 7) {
                          sprintf(buf2, "e");
                          strcat( buf, buf2);
                        }
                        if (door == 8) {
                          sprintf(buf2, "s");
                          strcat( buf, buf2);
                        }
                        if (door == 9) {
                          sprintf(buf2, "w");
                          strcat( buf, buf2);
                        }
                   }
                   else {
                        sprintf(buf2, " leave(Area)");
                        strcat( buf, buf2);
                   }
                   continue;
                 }
                 if (IS_SET(pexit->exit_info,EX_ISDOOR))
                 {
                  if (IS_IMMORTAL(ch) && IS_SET(pexit->exit_info, EX_HIDDEN))
                  {
                     sprintf(buf2, " {y(%s){g%s", IS_SET(pexit->exit_info,EX_CLOSED) ? IS_SET(pexit->exit_info, EX_LOCKED) ? "HLD" : "HD" : "HO", dir_name[door]);
                     strcat (buf, buf2);
                  }
                  else if (IS_IMMORTAL(ch))
                  {
                              sprintf(buf2, " {y(%s){g%s",IS_SET(pexit->exit_info,EX_CLOSED) ? IS_SET(pexit->exit_info, EX_LOCKED) ? "LD" : "D" :"O", dir_name[door]);
                              strcat( buf, buf2);
                  }
                  else
                  {
                        if (!IS_SET(pexit->exit_info,EX_HIDDEN))
                        {
                              sprintf(buf2, " {y(%s){g%s",IS_SET(pexit->exit_info,EX_CLOSED) ? "D":"O",dir_name[door]);
                              strcat( buf, buf2);
                        }
                        else
                        {
	    	   		if ((((get_skill(ch,gsn_observation) + get_skill(ch,gsn_alertness) + get_skill(ch,gsn_awareness)) / 3) /4) > number_percent())
		   		{
             				sprintf(buf2, " {y(%s){g%s", IS_SET(pexit->exit_info,EX_CLOSED) ? IS_SET(pexit->exit_info, EX_LOCKED) ? "HLD" : "HD" : "HO", dir_name[door]);
             				strcat (buf, buf2);
		   		}
		   		else
				{
                           		continue;
				}
                        }
                  }
                }
                else
                {
                  if (IS_IMMORTAL(ch) && IS_SET(pexit->exit_info, EX_HIDDEN))
                  {
                     sprintf(buf2, " {y(H){g%s", dir_name[door]);
                     strcat (buf, buf2);
                  }
                  else
                  {
                        if (!IS_SET(pexit->exit_info,EX_HIDDEN))
                        {
                              sprintf(buf2," %s",dir_name[door]);
                              strcat(buf,buf2);
                        }
                        else
                        {
	    	   		if ((((get_skill(ch,gsn_observation) + get_skill(ch,gsn_alertness) + get_skill(ch,gsn_awareness)) / 3) /4) > number_percent())
		   		{
             				sprintf(buf2, " {y(%s){g%s", IS_SET(pexit->exit_info,EX_CLOSED) ? IS_SET(pexit->exit_info, EX_LOCKED) ? "HLD" : "HD" : "HO", dir_name[door]);
             				strcat (buf, buf2);
		   		}
		   		else
                           		continue;
                        }
                  }
               }
	    }
	    else
	    {
                if (IS_SET(pexit->exit_info,EX_HIDDEN)) {
	    	   if ((((get_skill(ch,gsn_observation) + get_skill(ch,gsn_alertness) + get_skill(ch,gsn_awareness)) / 3) /4) > number_percent())
		   {
             		sprintf(buf2, " {y(%s){g%s", IS_SET(pexit->exit_info,EX_CLOSED) ? IS_SET(pexit->exit_info, EX_LOCKED) ? "HLD" : "HD" : "HO", dir_name[door]);
             		strcat (buf, buf2);
		   }
		   else
                        continue;
                }
		sprintf (buf + strlen (buf), "%-5s - %s",
			 capitalize (dir_name[door]),
			 room_is_dark (pexit->u1.to_room)
			 ? "Too dark to tell"
			 : pexit->u1.to_room->name
		    );
		if (IS_IMMORTAL (ch))
		    sprintf (buf + strlen (buf),
			     " (room %d)\n\r", pexit->u1.to_room->vnum);
		else
		    sprintf (buf + strlen (buf), "\n\r");
	    }
	}
    }

   if (ch->in_obj != NULL || !ch->in_room->inside_of)
    {
      strcat(buf, fAuto ? " leave(Object) " : "Leave(Object){x\n\r");
      found = TRUE;
    }

    if (!found)
	strcat (buf, fAuto ? " none" : "None.\n\r");

    if (fAuto)
    {
	if (vmap_exit) {
	   strcat(buf,")");
	}
	strcat (buf, "]\n\r");
    }

    send_to_char (buf, ch);
    return;
}

/*
 * The fullowing function is tricky. We are using a portal object as 
 * vehicle. This means, we must dereference everything through 
 * ch->in_room->inside_of, which is the vehicle object.
 * I am not yet sure how well this will work.  -- Sauron
 */
void move_vehicle (CHAR_DATA * ch, OBJ_DATA * obj, int door)
{
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch;

    if (door < 0 || door > 9)
    {
	bug ("Do_move: bad door %d.", door);
	return;
    }

    in_room = obj->in_room;

    if ((pexit = in_room->exit[door]) == NULL)
    {
	send_to_char ("Unfortunately, the way is blocked off.", ch);
	return;
    }

    if ((pexit = in_room->exit[door]) == NULL
	|| (to_room = pexit->u1.to_room) == NULL
	|| !can_see_room (ch, pexit->u1.to_room))
    {
	return;
    }

    if (IS_SET (pexit->exit_info, EX_CLOSED))
    {
	return;
    }

    if (in_room->sector_type == SECT_AIR
	|| to_room->sector_type == SECT_AIR)
    {
	send_to_char ("Vehicles can't fly.\n\r", ch);
	return;
    }

    if (in_room->sector_type == SECT_WATER_NOSWIM
	|| to_room->sector_type == SECT_WATER_NOSWIM)
    {
	send_to_char ("You can't take a vehicle there!", ch);
	return;
    }

    sprintf (buf, "%s moves %s.", capitalize (ch->in_room->inside_of->short_descr), dir_name[door]);

    for (vch = ch->in_room->inside_of->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
	send_to_char (buf, vch);
    }

    obj_from_room (obj);
    obj_to_room (obj, to_room);

    sprintf (buf, "%s enters.", capitalize (ch->in_room->inside_of->short_descr));

    for (vch = ch->in_room->inside_of->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
	send_to_char (buf, vch);
    }

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	do_hack_look (vch, "auto");

    if (in_room == to_room)	/* no circular follows */
	return;
}

void do_north (CHAR_DATA * ch, char *argument)
{
    if (ch->in_room->inside_of)
	send_to_char ("But you are riding a vehicle!\n\r", ch);
    else
	move_char (ch, DIR_NORTH, FALSE);
    return;
}

void do_east (CHAR_DATA * ch, char *argument)
{
    if (ch->in_room->inside_of)
	send_to_char ("But you are riding a vehicle!\n\r", ch);
    else
	move_char (ch, DIR_EAST, FALSE);
    return;
}

void do_south (CHAR_DATA * ch, char *argument)
{
    if (ch->in_room->inside_of)
	send_to_char ("But you are riding a vehicle!\n\r", ch);
    else
	move_char (ch, DIR_SOUTH, FALSE);
    return;
}

void do_west (CHAR_DATA * ch, char *argument)
{
    if (ch->in_room->inside_of)
	send_to_char ("But you are riding a vehicle!\n\r", ch);
    else
	move_char (ch, DIR_WEST, FALSE);
    return;
}

void do_up (CHAR_DATA * ch, char *argument)
{
    if (ch->in_room->inside_of)
	send_to_char ("But you are riding a vehicle!\n\r", ch);
    else
	move_char (ch, DIR_UP, FALSE);
    return;
}

void do_down (CHAR_DATA * ch, char *argument)
{
    if (ch->in_room->inside_of)
	send_to_char ("But you are riding a vehicle!\n\r", ch);
    else
	move_char (ch, DIR_DOWN, FALSE);
    return;
}

/* RT Enter portals */
void do_vehicle_enter (CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *location;

    if (ch->fighting != NULL)
	return;

/* nifty portal stuff */
    if (argument[0] != '\0')
    {
	ROOM_INDEX_DATA *old_room;
	OBJ_DATA *portal;
	CHAR_DATA *fch, *fch_next;

	old_room = ch->in_room;

	portal = get_obj_list (ch, argument, ch->in_room->contents);

	if (portal == NULL)
	{
	    send_to_char ("You don't see that here.\n\r", ch);
	    return;
	}

    /* It's a portal! */
	if (portal->item_type == ITEM_PORTAL)
	{
	    if ((!IS_TRUSTED (ch, ANGEL)
		 && !IS_SET (portal->value[2], GATE_NOCURSE)))
	    {
		send_to_char ("Something prevents you from leaving...\n\r", ch);
		return;
	    }

	    if (IS_SET (portal->value[2], GATE_RANDOM) || portal->value[3] == -1)
	    {
		location = get_random_room (ch,TRUE);
		portal->value[3] = location->vnum;	/* for record keeping :) */
	    }
	    else if (IS_SET (portal->value[2], GATE_BUGGY) && (number_percent () < 5))
		location = get_random_room (ch,TRUE);
	    else
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

	    act ("$n steps into $p.", ch, portal, NULL, TO_ROOM);

	    if (IS_SET (portal->value[2], GATE_NORMAL_EXIT))
		act ("You enter $p.", ch, portal, NULL, TO_CHAR);
	    else
		act ("You walk through $p and find yourself somewhere else...",
		     ch, portal, NULL, TO_CHAR);

	    char_from_room (ch);
	    char_to_room (ch, location);

	    if (IS_SET (portal->value[2], GATE_GOWITH))		/* take the gate along */
	    {
		obj_from_room (portal);
		obj_to_room (portal, location);
	    }

	    if (IS_SET (portal->value[2], GATE_NORMAL_EXIT))
		act ("$n has arrived.", ch, portal, NULL, TO_ROOM);
	    else
		act ("$n has arrived through $p.", ch, portal, NULL, TO_ROOM);

	    do_look (ch, "auto");

	/* charges */
	    if (portal->value[0] > 0)
	    {
		portal->value[0]--;
		if (portal->value[0] == 0)
		    portal->value[0] = -1;
	    }

	/* protect against circular follows */
	    if (old_room == location)
		return;

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
			act ("You can't bring $N into the city.",
			     ch, NULL, fch, TO_CHAR);
			act ("You aren't allowed in the city.",
			     fch, NULL, NULL, TO_CHAR);
			continue;
		    }

		    act ("You follow $N.", fch, NULL, ch, TO_CHAR);
		    do_vehicle_enter (fch, argument);
		}
	    }

	    if (portal != NULL && portal->value[0] == -1)
	    {
		act ("$p fades out of existence.", ch, portal, NULL, TO_CHAR);
		if (ch->in_room == old_room)
		    act ("$p fades out of existence.", ch, portal, NULL, TO_ROOM);
		else if (old_room->people != NULL)
		{
		    act ("$p fades out of existence.",
			 old_room->people, portal, NULL, TO_CHAR);
		    act ("$p fades out of existence.",
			 old_room->people, portal, NULL, TO_ROOM);
		}
		extract_obj (portal);
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
		    do_vehicle_enter (fch, argument);
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

    }
    send_to_char ("Hmm. This doesn't seem to be working out as planned..\n\r", ch);
    return;
}

void do_drive (CHAR_DATA * ch, char *argument)
{
    int door;

    if (ch->in_room->inside_of == NULL)
    {
	send_to_char ("You are not riding anything.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char ("Syntax: drive <direction>\n\r", ch);
	return;
    }

    if(!ch->in_room->inside_of->in_room)
	return;

    door = find_hack_door (ch, argument);
    move_vehicle (ch, ch->in_room->inside_of, door);
    save_vehicle (ch, ch->in_room->inside_of);
    return;
}

int find_hack_door (CHAR_DATA * ch, char *arg)
{
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    char buff[MAX_STRING_LENGTH];
    int door = -1;

    if (!str_cmp (arg, "n") || !str_cmp (arg, "north"))
	door = 0;
    else if (!str_cmp (arg, "e") || !str_cmp (arg, "east"))
	door = 1;
    else if (!str_cmp (arg, "s") || !str_cmp (arg, "south"))
	door = 2;
    else if (!str_cmp (arg, "w") || !str_cmp (arg, "west"))
	door = 3;
    else if (!str_cmp (arg, "u") || !str_cmp (arg, "up"))
	door = 4;
    else if (!str_cmp (arg, "d") || !str_cmp (arg, "down"))
	door = 5;
    else if (!str_cmp (arg,"enter n") || !str_cmp(arg,"enter north"))
	door = 6;
    else if (!str_cmp (arg,"enter e") || !str_cmp(arg,"enter east"))
	door = 7;
    else if (!str_cmp (arg,"enter s") || !str_cmp(arg,"enter south"))
	door = 8;
    else if (!str_cmp (arg,"enter w") || !str_cmp(arg,"enter west"))
	door = 9;
    else if (!str_cmp(arg,"leave"))
    {
      /********
       * Leav Areas linked to Vmap
       **********/
      if (ch->in_room != NULL && ch->in_room->inside_of != NULL && ch->in_room->inside_of->in_room != NULL)
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
       else
       {
	   send_to_char("You can't do that here.\n\r",ch);
	   return -1;
       }
    }
    else
    {
	if (ch->in_room->inside_of != NULL)
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
	return -1;
    }

    if (ch->in_room->inside_of != NULL)
    {
       if (ch->in_room->inside_of->in_room != NULL)
       {
          if ((pexit = ch->in_room->inside_of->in_room->exit[door]) == NULL)
          {
	      act ("I see no door $T here.", ch, NULL, arg, TO_CHAR);
	      return -1;
          }
       }
       else
       {
	      act ("I see no door $T here.", ch, NULL, arg, TO_CHAR);
	      return -1;
       }
    }
    /*
    sprintf(buff,"Using door: %d\r\n",door);
    send_to_char(buff,ch);
    */
    
	
    return door;
}
