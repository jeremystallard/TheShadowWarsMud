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
*	ROM 2.4 is copyright 1993-1998 Russ Taylor			               *
*	ROM has been brought to you by the ROM consortium		               *
*	    Russ Taylor (rtaylor@hypercube.org)				               *
*	    Gabrielle Taylor (gtaylor@hypercube.org)			               *
*	    Brian Moore (zump@rom.org)					               *
*	By using this code, you have agreed to follow the terms of the	     *
*	ROM license, in the file Rom24/doc/rom.license			          *
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

char * const dist_name [] =
{
  "right here",
  "close by", "not far off", "a brief walk away",
  "rather far off", "in the distance", "almost out of sight"
};

char * const dir_desc [] =
{
  "to the north", "to the east", "to the south", "to the west",
  "upwards", "downwards"
};

void scan_room           args( ( CHAR_DATA *ch, int door ) );

void scan_room( CHAR_DATA *ch, int door )
{
  int distance, visibility;
  bool found;
  ROOM_INDEX_DATA *was_in_room;
  char buf[MAX_INPUT_LENGTH];
  
  visibility = 6;
  if( !IS_SET( ch->act, PLR_HOLYLIGHT ) && !IS_SET(ch->affected_by, AFF_INFRARED) && ch->race != race_lookup("fade") && ch->race != race_lookup("gholam")) {
    switch( weather_info.sunlight ) {
    case SUN_SET:   visibility = 4; break;
    case SUN_DARK:  visibility = 2; break;
    case SUN_RISE:  visibility = 4; break;
    case SUN_LIGHT: visibility = 6; break;
    }
    if (ch->race != race_lookup("fade") && ch->race != race_lookup("trolloc")) {
       switch( weather_info.sky ) {
          case SKY_CLOUDLESS: break;
          case SKY_CLOUDY:    visibility -= 1; break;
          case SKY_RAINING:   visibility -= 2; break;
          case SKY_LIGHTNING: visibility -= 3; break;
       }
    }
  }

  was_in_room = ch->in_room;
  found = FALSE;
  for( distance = 1; distance <= 6; distance++ ) {
    EXIT_DATA *pexit;
    CHAR_DATA *list;
    CHAR_DATA *rch;
    
    if( ( pexit = ch->in_room->exit[door] ) != NULL
	   && pexit->u1.to_room != NULL
	   && pexit->u1.to_room != was_in_room ) {

	 /* If the door is closed, stop looking... */
	 if( IS_SET( pexit->exit_info, EX_CLOSED ) ) {
	   char door_name[80];
	   
	   one_argument( pexit->keyword, door_name );
	   if( door_name[0] == '\0' )
		strcat( door_name, "{ydoor{x" );
	   sprintf( buf, "A closed %s %s %s.\n\r",
			  door_name, dist_name[distance-1], dir_desc[door] );
	   send_to_char( buf, ch );
	   found = TRUE;
	   break;
	 }
	 
	 ch->in_room = pexit->u1.to_room;
	 if( IS_OUTSIDE(ch) ? distance > visibility : distance > 4 )
	   break;
	 
	 list = ch->in_room->people;
	 for( rch = list; rch != NULL; rch = rch->next_in_room ) {
	   if (IS_COLORCLOAKED(rch))
		continue;
	   if( can_see( ch, rch ) ) {
		found = TRUE;
		sprintf( buf, "%s%s%s%s%s%s is %s %s.\n\r",
			    IS_RP(rch)  ? "{W(IC){x " : "",
			    rch->position == POS_FIGHTING ? "{R(Fighting){x " : "",
			    rch->timer > 3 ? "{Y(Idle){x " : "",			    
			    IS_SET(rch->comm, COMM_AFK) ? "{Y(AFK){x " : "",
			    IS_NPC(rch) ? "" : IS_IMMORTAL(rch) ? "{Y({WI{Y){x " : "{y(P){x ",
			    PERS( rch, ch ),
			    dist_name[distance],
			    dir_desc[door] );
		buf[0] = UPPER(buf[0]);
		send_to_char( buf, ch );
	   }
	 }
    }
  }
  
  ch->in_room = was_in_room;
  
  if( !found ) {
    sprintf( buf, "You can't see anything %s.\n\r",
		   dir_desc[door] );
    send_to_char( buf, ch );
  }
  
  return;
}

void do_scan( CHAR_DATA *ch, char *argument )
{
  int dir;
  bool found;
  char buf[MAX_INPUT_LENGTH];

  EXIT_DATA *pexit;
   
  /*  Put in blind check if needed...
	 if( !check_blind(ch) )
	 return;
  */

  if( argument[0] == '\0' ) {
    act( "$n scans intensely all around.", ch, NULL, NULL, TO_ROOM );
    
    found = FALSE;
    for( dir = 0; dir <= 5; dir++ )
	 if( (pexit = ch->in_room->exit[dir]) != NULL && 
		!IS_SET(pexit->exit_info, EX_HIDDEN)) {
	   sprintf(buf, "\n\r**** {m%s{x ****\n\r",dir_desc[dir]);
 	   send_to_char(buf, ch);
	   scan_room( ch, dir );
	   found = TRUE;
	 }

    if( !found )
	 send_to_char( "There are no exits here!\n\r", ch );
  }
  else {
    if( !str_prefix( argument, "north" ) ) dir = DIR_NORTH;
    else if( !str_prefix( argument, "east"  ) ) dir = DIR_EAST;
    else if( !str_prefix( argument, "south" ) ) dir = DIR_SOUTH;
    else if( !str_prefix( argument, "west"  ) ) dir = DIR_WEST;
    else if( !str_prefix( argument, "up"    ) ) dir = DIR_UP;
    else if( !str_prefix( argument, "down"  ) ) dir = DIR_DOWN;
    else {
	 send_to_char( "That's not a direction!\n\r", ch );
	 return;
    }

    act( "$n scans intensly $T.", ch, NULL, dir_desc[dir], TO_ROOM );
    
    pexit = ch->in_room->exit[dir];
    if( (pexit == NULL || 
		IS_SET(pexit->exit_info, EX_HIDDEN))) {
	 send_to_char( "There is no exit in that direction!\n\r", ch );
	 return;
    }

    sprintf(buf, "\n\r**** {m%s{x ****\n\r",dir_desc[dir]);
    send_to_char(buf, ch);
    scan_room( ch, dir );
  }
  
  return;
}
