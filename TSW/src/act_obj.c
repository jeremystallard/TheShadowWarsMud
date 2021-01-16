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
#include "tables.h"

/*
 * Local functions.
 */
#define CD CHAR_DATA
#define OD OBJ_DATA
bool	remove_obj	args( (CHAR_DATA *ch, int iWear, bool fReplace ) );
void	wear_obj	args( (CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, char *location) );
bool    can_wear_angreal args( (CHAR_DATA *ch));
CD *	find_keeper	args( (CHAR_DATA *ch ) );
int	get_cost	args( (CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy ) );
void 	obj_to_keeper	args( (OBJ_DATA *obj, CHAR_DATA *ch ) );
OD *	get_obj_keeper	args( (CHAR_DATA *ch,CHAR_DATA *keeper,char *argument));
void	save_keeper	args( (CHAR_DATA *ch, OBJ_DATA *obj));
extern void fwrite_obj	args( ( CHAR_DATA *ch,  OBJ_DATA  *obj, FILE *fp, int iNest ) );
extern void fwrite_keeper_obj	args( ( CHAR_DATA *ch,  OBJ_DATA  *obj, FILE *fp, int iNest ) );
	

#undef OD
#undef	CD

int find_exit( CHAR_DATA *ch, char *arg );          /* act_move.c */

/* RT part of the corpse looting code */

bool can_loot(CHAR_DATA *ch, OBJ_DATA *obj)
{
    CHAR_DATA *owner, *wch;

    if (IS_IMMORTAL(ch))
	return TRUE;

    if (!obj->owner || obj->owner == NULL)
	return TRUE;

    owner = NULL;
    for ( wch = char_list; wch != NULL ; wch = wch->next )
        if (!str_cmp(wch->name,obj->owner))
            owner = wch;

    if (owner == NULL)
	return TRUE;

    if (!str_cmp(ch->name,owner->name))
	return TRUE;

    if (!IS_NPC(owner) && IS_SET(owner->act,PLR_CANLOOT))
	return TRUE;

    if (is_same_group(ch,owner))
	return TRUE;

    return FALSE;
}


void get_obj( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container )
{
  /* variables for AUTOSPLIT */
  CHAR_DATA *gch;
  int members;
  char buffer[100];
  
  if ( !CAN_WEAR(obj, ITEM_TAKE) )  {
    send_to_char( "You can't take that.\n\r", ch );
    return;
  }
  
  if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) ) {
    act( "$d: you can't carry that many items.", ch, NULL, obj->name, TO_CHAR );
    return;
  }
  
  if ((!obj->in_obj || obj->in_obj->carried_by != ch)
	 &&  (get_carry_weight(ch) + get_obj_weight(obj) > can_carry_w(ch))) {
    act( "$d: you can't carry that much weight.",
	    ch, NULL, obj->name, TO_CHAR );
    return;
  }

  if (!can_loot(ch,obj)) {
    act("Corpse looting is not permitted.",ch,NULL,NULL,TO_CHAR );
    return;
  }
  
  if (obj->in_room != NULL) {
    for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
	 if (gch->on == obj) {
	   act("$N appears to be using $p.", ch,obj,gch,TO_CHAR);
	   return;
	 }
  }
  
  
  if ( container != NULL ) {
    if (container->pIndexData->vnum == OBJ_VNUM_PIT
	   &&  get_trust(ch) < obj->level) {
	 send_to_char("You are not powerful enough to use it.\n\r",ch);
	 return;
    }
    
    if (container->pIndexData->vnum == OBJ_VNUM_PIT
	   &&  !CAN_WEAR(container, ITEM_TAKE)
	   &&  !IS_OBJ_STAT(obj,ITEM_HAD_TIMER))
	 obj->timer = 0;	

    act( "You get $p from $P.", ch, obj, container, TO_CHAR );
    if (!IS_AFFECTED(ch,AFF_SNEAK))
	 act( "$n gets $p from $P.", ch, obj, container, TO_ROOM );

    REMOVE_BIT(obj->extra_flags,ITEM_HAD_TIMER);

    obj_from_obj( obj );
  }
  else {
    act( "You get $p.", ch, obj, container, TO_CHAR );
    if (!IS_AFFECTED(ch,AFF_SNEAK))
	 act( "$n gets $p.", ch, obj, container, TO_ROOM );

    obj_from_room( obj );
  }
  
  if ( obj->item_type == ITEM_MONEY) {
    ch->silver += obj->value[0];
    ch->gold += obj->value[1];
    if (IS_SET(ch->act,PLR_AUTOSPLIT)) { /* AUTOSPLIT code */
	 members = 0;
	 for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room ) {
	   if (!IS_AFFECTED(gch,AFF_CHARM) && is_same_group( gch, ch ) )
		members++;
	 }
	 
	 if ( members > 1 && (obj->value[0] > 1 || obj->value[1])) {
	   sprintf(buffer,"%d %d",obj->value[0],obj->value[1]);
	   do_function(ch, &do_split, buffer);	
	 }
    }
    
    extract_obj( obj );
  }
  else {

    if (IS_SET(obj->extra_flags,ITEM_HIDDEN))
	 REMOVE_BIT(obj->extra_flags,ITEM_HIDDEN);
    obj_to_char( obj, ch );
  }

  if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH)) {
    obj->timer = number_range(5,10);
    REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);
  }

  if (IS_SET(obj->extra_flags, ITEM_HAD_TIMER)) {
	if (obj->item_type == ITEM_KEY)
		obj->timer = number_range(150,200);
  }
  
  return;
}



void do_get( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *container;
    bool found, found_in_obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"from"))
	argument = one_argument(argument,arg2);

    /* Get type. */
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Get what?\n\r", ch );
	return;
    }

    if ((IS_DREAMING(ch) || IS_DREAMWALKING(ch)) && !IS_IMMORTAL(ch))
    {
	send_to_char( "You reach for the item, but after holding it for a moment, it disappears from your grasp and shows up back where you picked it up from.\r\n",ch);
	return;
    }
    if ( arg2[0] == '\0' )
    {
	if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
	{
	    /* 'get obj' */
            obj = NULL;
            found_in_obj = FALSE;

            if( ch->in_obj )
            {
               obj = get_obj_list(ch, arg1, ch->in_obj->contains);
               if( obj ) found_in_obj = TRUE;
            }
            if (!obj)
	       obj = get_obj_list( ch, arg1, ch->in_room->contents );
	    if ( obj == NULL )
	    {
		act( "I see no $T here.", ch, NULL, arg1, TO_CHAR );
		return;
	    }
            if (obj->who_in)
            {
                send_to_char("There is someone in that at the moment!!\n\r",ch);
                return;
            }
            get_obj(ch, obj, found_in_obj ? ch->in_obj : NULL);         
	    if (found_in_obj)
	    {
		if (IS_OBJ_STAT(ch->in_obj,ITEM_KEEPER))
		{
			save_keeper(ch,ch->in_obj);
		}
	    }
	}
	else
	{
	    /* 'get all' or 'get all.obj' */
	    found = FALSE;
            found_in_obj = FALSE;

            if( ch->in_obj )
            {
               for( obj = ch->in_obj->contains; obj; obj = obj->next_content )
               {  if( ( arg1[ 3 ] == '\0' || is_name( &arg1[ 4 ], obj->name ) )
                   && can_see_obj( ch, obj ) )
                  {
                   found_in_obj = TRUE;
                   break;
                  }
                }
            }

	    for ( obj = found_in_obj ? ch->in_obj->contains : ch->in_room->contents; obj != NULL; obj = obj_next )
	    {
		obj_next = obj->next_content;
		if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
		&&   can_see_obj( ch, obj ) )
		{
		    found = TRUE;
                    if (obj->who_in)
                    {
                        send_to_char("There is someone in that at the moment!!\n\r",ch);
                        return;
                    }
		    get_obj( ch, obj, found_in_obj ? ch->in_obj : NULL );
		    if (found_in_obj)
		    {
			if (IS_OBJ_STAT(ch->in_obj,ITEM_KEEPER))
			{
				save_keeper(ch,ch->in_obj);
			}
		    }
		}
	    }

	    if ( !found ) 
	    {
		if ( arg1[3] == '\0' )
		    send_to_char( "I see nothing here.\n\r", ch );
		else
		    act( "I see no $T here.", ch, NULL, &arg1[4], TO_CHAR );
	    }
	}
    }
    else
    {
	/* 'get ... container' */
	if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
	{
	    send_to_char( "You can't do that.\n\r", ch );
	    return;
	}

	if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
	{
	    act( "I see no $T here.", ch, NULL, arg2, TO_CHAR );
	    return;
	}

	switch ( container->item_type )
	{
	default:
	    send_to_char( "That's not a container.\n\r", ch );
	    return;

	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	    break;

	case ITEM_CORPSE_PC:
	    {

		if (!can_loot(ch,container))
		{
		    send_to_char( "You can't do that.\n\r", ch );
		    return;
		}
	    }
	}

	if ( IS_SET(container->value[1], CONT_CLOSED) )
	{
	    act( "The $d is closed.", ch, NULL, container->name, TO_CHAR );
	    return;
	}

        if( IS_SET( container->value[1], CONT_ENTERABLE )
         &&  ((container->carried_by != NULL)
         ||   (container->in_obj != NULL)) )
        {
           act( "$p needs to be on the ground for that.",
                ch, container, NULL, TO_CHAR );
           return;
        }

	if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
	{
	    /* 'get obj container' */
	    obj = get_obj_list( ch, arg1, container->contains );
	    if ( obj == NULL )
	    {
		act( "I see nothing like that in the $T.",
		    ch, NULL, arg2, TO_CHAR );
		return;
	    }
	    get_obj( ch, obj, container );
	    if (IS_OBJ_STAT(container,ITEM_KEEPER))
	    {
		save_keeper(ch,container);
	    }
	}
	else
	{
	    /* 'get all container' or 'get all.obj container' */
	    found = FALSE;
	    for ( obj = container->contains; obj != NULL; obj = obj_next )
	    {
		obj_next = obj->next_content;
		if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
		&&   can_see_obj( ch, obj ) )
		{
		    found = TRUE;
		    if (container->pIndexData->vnum == OBJ_VNUM_PIT
		    &&  !IS_IMMORTAL(ch))
		    {
			send_to_char("Don't be so greedy!\n\r",ch);
			return;
		    }
		    get_obj( ch, obj, container );
		    if (IS_OBJ_STAT(container,ITEM_KEEPER))
		    {
			    save_keeper(ch,container);
		    }
		}
	    }

	    if ( !found )
	    {
		if ( arg1[3] == '\0' )
		    act( "I see nothing in the $T.",
			ch, NULL, arg2, TO_CHAR );
		else
		    act( "I see nothing like that in the $T.",
			ch, NULL, arg2, TO_CHAR );
	    }
	}
    }

    return;
}



void do_put( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *container;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"in") || !str_cmp(arg2,"on"))
	argument = one_argument(argument,arg2);

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Put what in what?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
    {
	send_to_char( "You can't do that.\n\r", ch );
	return;
    }

    if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
    {
	act( "I see no $T here.", ch, NULL, arg2, TO_CHAR );
	return;
    }

    if ( container->item_type != ITEM_CONTAINER )
    {
	send_to_char( "That's not a container.\n\r", ch );
	return;
    }

  /*
   * Ok, making it so only way to put in a container if you are
   * in it is to drop it while in it
   */
   if (ch->in_obj != NULL && container == ch->in_obj)
   {
      send_to_char("Just drop it instead.\n\r", ch);
      return;
   }


    if ( IS_SET(container->value[1], CONT_CLOSED) )
    {
	act( "The $d is closed.", ch, NULL, container->name, TO_CHAR );
	return;
    }

   if( IS_SET( container->value[1], CONT_ENTERABLE )
   &&  ((container->carried_by != NULL)
   ||   (container->in_obj != NULL)) )
   {
      act( "$p needs to be on the ground for that.",
           ch, container, NULL, TO_CHAR );
      return;
   }


    if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
    {
	/* 'put obj container' */
	if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if ( obj == container )
	{
	    send_to_char( "You can't fold it into itself.\n\r", ch );
	    return;
	}

	if ( !can_drop_obj( ch, obj ) )
	{
	    send_to_char( "You can't let go of it.\n\r", ch );
	    return;
	}

    	if (WEIGHT_MULT(obj) != 100)
    	{
           send_to_char("You have a feeling that would be a bad idea.\n\r",ch);
            return;
        }

	if (get_obj_weight( obj ) + get_true_weight( container )
	     > (container->value[0] * 10) 
	||  get_obj_weight(obj) > (container->value[3] * 10))
	{
	    send_to_char( "It won't fit.\n\r", ch );
	    return;
	}
	
	if (container->pIndexData->vnum == OBJ_VNUM_PIT 
	&&  !CAN_WEAR(container,ITEM_TAKE))
	{    if (obj->timer)
		SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
	    else
	        obj->timer = number_range(100,200);
        }
	obj_from_char( obj );
	obj_to_obj( obj, container );
	if (IS_OBJ_STAT(container,ITEM_KEEPER))
	{
		save_keeper(ch,container);
	}
	

	if (IS_SET(container->value[1],CONT_PUT_ON))
	{
	    act("$n puts $p on $P.",ch,obj,container, TO_ROOM);
	    act("You put $p on $P.",ch,obj,container, TO_CHAR);
	}
	else
	{
	    act( "$n puts $p in $P.", ch, obj, container, TO_ROOM );
	    act( "You put $p in $P.", ch, obj, container, TO_CHAR );
	}
    }
    else
    {
	/* 'put all container' or 'put all.obj container' */
	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;

	    if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
	    &&   can_see_obj( ch, obj )
	    &&   WEIGHT_MULT(obj) == 100
	    &&   obj->wear_loc == WEAR_NONE
	    &&   obj != container
	    &&   obj != ch->in_obj
	    &&   can_drop_obj( ch, obj )
	    &&   get_obj_weight( obj ) + get_true_weight( container )
		 <= (container->value[0] * 10) 
	    &&   get_obj_weight(obj) < (container->value[3] * 10))
	    {
	    	if (container->pIndexData->vnum == OBJ_VNUM_PIT
	    	&&  !CAN_WEAR(obj, ITEM_TAKE) )
	    	{    if (obj->timer)
			SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
	    	    else
	    	    	obj->timer = number_range(100,200);
		}
		obj_from_char( obj );
		obj_to_obj( obj, container );
		if (IS_OBJ_STAT(container,ITEM_KEEPER))
		{
			save_keeper(ch,container);
		}

        	if (IS_SET(container->value[1],CONT_PUT_ON))
        	{
            	    act("$n puts $p on $P.",ch,obj,container, TO_ROOM);
            	    act("You put $p on $P.",ch,obj,container, TO_CHAR);
        	}
		else
		{
		    act( "$n puts $p in $P.", ch, obj, container, TO_ROOM );
		    act( "You put $p in $P.", ch, obj, container, TO_CHAR );
		}
	    }
	}
    }

    return;
}



void do_drop( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    bool found;
    bool dbroken;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Drop what?\n\r", ch );
	return;
    }
    if ((IS_DREAMING(ch) || IS_DREAMWALKING(ch)) && !IS_IMMORTAL(ch))
    {
	send_to_char( "You try to drop it, but it returns to its prior location.\r\n",ch);
	return;
    }

    if ( is_number( arg ) )
    {
	/* 'drop NNNN coins' */
	int amount, gold = 0, silver = 0;

	amount   = atoi(arg);
	argument = one_argument( argument, arg );
	if ( amount <= 0
	|| ( str_cmp( arg, "coins" ) && str_cmp( arg, "coin" ) && 
	     str_cmp( arg, "gold"  ) && str_cmp( arg, "silver") ) )
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	if ( !str_cmp( arg, "coins") || !str_cmp(arg,"coin") 
	||   !str_cmp( arg, "silver"))
	{
	    if (ch->silver < amount)
	    {
		send_to_char("You don't have that much silver.\n\r",ch);
		return;
	    }

	    ch->silver -= amount;
	    silver = amount;
	}
	else
	{
	    if (ch->gold < amount)
	    {
		send_to_char("You don't have that much gold.\n\r",ch);
		return;
	    }

	    ch->gold -= amount;
  	    gold = amount;
	}

	for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;

	    switch ( obj->pIndexData->vnum )
	    {
	    case OBJ_VNUM_SILVER_ONE:
		silver += 1;
		extract_obj(obj);
		break;

	    case OBJ_VNUM_GOLD_ONE:
		gold += 1;
		extract_obj( obj );
		break;

	    case OBJ_VNUM_SILVER_SOME:
		silver += obj->value[0];
		extract_obj(obj);
		break;

	    case OBJ_VNUM_GOLD_SOME:
		gold += obj->value[1];
		extract_obj( obj );
		break;

	    case OBJ_VNUM_COINS:
		silver += obj->value[0];
		gold += obj->value[1];
		extract_obj(obj);
		break;
	    }
	}

	obj = create_money( gold, silver );
	obj_to_room( obj, ch->in_room );
	if ( !IS_AFFECTED(ch, AFF_SNEAK) )
	  act( "$n drops some coins.", ch, NULL, NULL, TO_ROOM );
	send_to_char( "OK.\n\r", ch );
	if ( IS_WATER( ch->in_room ) )   {
	  extract_obj( obj );
	  if ( !IS_AFFECTED(ch, AFF_SNEAK) )
	    act("The coins sink down, and disapear in the water.", ch, NULL, NULL, TO_ROOM );
	  act("The coins sink down, and disapear in the water.", ch, NULL, NULL, TO_CHAR );
	}
        return;
    }

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
    {
	/* 'drop obj' */
	if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if ( !can_drop_obj( ch, obj ) )
	{
	    send_to_char( "You can't let go of it.\n\r", ch );
	    return;
	}

	obj_from_char( obj );

        if (ch->in_obj)
        {
           obj_to_obj(obj, ch->in_obj);
	   if (IS_OBJ_STAT(ch->in_obj,ITEM_KEEPER))
	   {
		save_keeper(ch,ch->in_obj);
	   }
	   act( "$n drops $p.", ch, obj, NULL, TO_CONT );
        }
        else
        {
	   obj_to_room( obj, ch->in_room );
	   act( "$n drops $p.", ch, obj, NULL, TO_ROOM );
        }
	act( "You drop $p.", ch, obj, NULL, TO_CHAR );
	if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
	{
            if (ch->in_obj)
	       act("$p dissolves into smoke.",ch,obj,NULL,TO_CONT);
            else
	       act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM);
	    act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR);
	    extract_obj(obj);
	}
    }
    else
    {
	argument = one_argument(argument,arg2);
	if (arg2 != NULL && !str_cmp(arg2,"broken"))
        {
		dbroken = TRUE;
        }
 	else
	{
		dbroken = FALSE;
	}
	/* 'drop all' or 'drop all.obj' */
	found = FALSE;
	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;

	    if ( ( (arg[3] == '\0' && !dbroken) || is_name( &arg[4], obj->name ) || (dbroken && IS_SET(obj->extra_flags,ITEM_BROKEN)) ) 
	    &&   can_see_obj( ch, obj )
	    &&   obj->wear_loc == WEAR_NONE
	    &&   can_drop_obj( ch, obj ) )
	    {
		found = TRUE;
		obj_from_char( obj );
                if (ch->in_obj)
	        {
		   obj_to_obj( obj, ch->in_obj );
	           if (IS_OBJ_STAT(ch->in_obj,ITEM_KEEPER))
	           {
		           save_keeper(ch,ch->in_obj);
	           }
		}
                else
		   obj_to_room( obj, ch->in_room );
		act( "$n drops $p.", ch, obj, NULL, TO_ROOM );
		act( "You drop $p.", ch, obj, NULL, TO_CHAR );
        	if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
        	{
             	    act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM);
            	    act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR);
            	    extract_obj(obj);
        	}
	    }
	}

	if ( !found )
	{
	    if ( arg[3] == '\0' )
		act( "You are not carrying anything.",
		    ch, NULL, arg, TO_CHAR );
	    else
		act( "You are not carrying any $T.",
		    ch, NULL, &arg[4], TO_CHAR );
	}
    }

    return;
}



void do_give( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Give what to whom?\n\r", ch );
	return;
    }

    if ( is_number( arg1 ) )
    {
	/* 'give NNNN coins victim' */
	int amount;
	bool silver;

	amount   = atoi(arg1);
	if ( amount <= 0
	|| ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) && 
	     str_cmp( arg2, "gold"  ) && str_cmp( arg2, "silver")) )
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	silver = str_cmp(arg2,"gold");

	argument = one_argument( argument, arg2 );
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Give what to whom?\n\r", ch );
	    return;
	}

	if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}

	if ( (!silver && ch->gold < amount) || (silver && ch->silver < amount) )
	{
	    send_to_char( "You haven't got that much.\n\r", ch );
	    return;
	}

	if (silver)
	{
	    ch->silver		-= amount;
	    victim->silver 	+= amount;
	}
	else
	{
	    ch->gold		-= amount;
	    victim->gold	+= amount;
	}

	sprintf(buf,"$n gives you %d %s.",amount, silver ? "silver" : "gold");
	act( buf, ch, NULL, victim, TO_VICT    );
	act( "$n gives $N some coins.",  ch, NULL, victim, TO_NOTVICT );
	sprintf(buf,"You give $N %d %s.",amount, silver ? "silver" : "gold");
	act( buf, ch, NULL, victim, TO_CHAR    );

	/*
	 * Bribe trigger
	 */
	if ( IS_NPC(victim) && HAS_TRIGGER( victim, TRIG_BRIBE ) )
	    mp_bribe_trigger( victim, ch, silver ? amount : amount * 100 );

	if (IS_NPC(victim) && IS_SET(victim->act,ACT_IS_CHANGER))
	{
	    int change;

	    change = (silver ? 95 * amount / 100 / 100 
		 	     : 95 * amount);


	    if (!silver && change > victim->silver)
	    	victim->silver += change;

	    if (silver && change > victim->gold)
		victim->gold += change;

	    if (change < 1 && can_see(victim,ch))
	    {
		act(
	"$n tells you 'I'm sorry, you did not give me enough to change.'"
		    ,victim,NULL,ch,TO_VICT);
		//ch->reply = victim;
		sprintf(buf,"%d %s %s", 
			amount, silver ? "silver" : "gold",ch->name);
		do_function(victim, &do_give, buf);
	    }
	    else if (can_see(victim,ch))
	    {
		sprintf(buf,"%d %s %s", 
			change, silver ? "gold" : "silver",ch->name);
		do_function(victim, &do_give, buf);
		if (silver)
		{
		    sprintf(buf,"%d silver %s", 
			(95 * amount / 100 - change * 100),ch->name);
		    do_function(victim, &do_give, buf);
		}
		act("$n tells you 'Thank you, come again.'",
		    victim,NULL,ch,TO_VICT);
		//ch->reply = victim;
	    }
	}
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( obj->wear_loc != WEAR_NONE )
    {
	send_to_char( "You must remove it first.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
    {
	act("$N tells you 'Sorry, you'll have to sell that.'",
	    ch,NULL,victim,TO_CHAR);
	//ch->reply = victim;
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if ( victim->carry_number + get_obj_number( obj ) > can_carry_n( victim ) )
    {
	act( "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if (get_carry_weight(victim) + get_obj_weight(obj) > can_carry_w( victim ) )
    {
	act( "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( !can_see_obj( victim, obj ) )
    {
	act( "$N can't see it.", ch, NULL, victim, TO_CHAR );
	return;
    }

    obj_from_char( obj );
    obj_to_char( obj, victim );
    MOBtrigger = FALSE;
    act( "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT );
    act( "$n gives you $p.",   ch, obj, victim, TO_VICT    );
    act( "You give $p to $N.", ch, obj, victim, TO_CHAR    );
    MOBtrigger = TRUE;

    /*
     * Give trigger
     */
    if ( IS_NPC(victim) && HAS_TRIGGER( victim, TRIG_GIVE ) )
	mp_give_trigger( victim, ch, obj );

    return;
}
void do_plant( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    int skill,victimchance;
    bool failed = FALSE;

    if (IS_IMMORTAL(ch))
    {
       skill = 201;
    }
    else
    {
       skill = (get_skill(ch,gsn_plant) * 2) / 3;
    }
    if (ch->class == CLASS_THIEF)
    {
	skill += 25;
    }
    if (skill < 1)
    {
	send_to_char("Just give it to them!\n\r",ch);
	return;
    }
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Plant what on whom?\n\r", ch );
	return;
    }

    if ( is_number( arg1 ) )
    {
	/* 'give NNNN coins victim' */
	int amount;
	bool silver;

	amount   = atoi(arg1);
	if ( amount <= 0
	|| ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) && 
	     str_cmp( arg2, "gold"  ) && str_cmp( arg2, "silver")) )
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	silver = str_cmp(arg2,"gold");

	argument = one_argument( argument, arg2 );
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Plant what on whom?\n\r", ch );
	    return;
	}

	if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}

	if ( (!silver && ch->gold < amount) || (silver && ch->silver < amount) )
	{
	    send_to_char( "You haven't got that much.\n\r", ch );
	    return;
	}

	if (silver)
	{
	    ch->silver		-= amount;
	    victim->silver 	+= amount;
	}
	else
	{
	    ch->gold		-= amount;
	    victim->gold	+= amount;
	}

	sprintf(buf,"You plant %d %s on $N.",amount, silver ? "silver" : "gold");
	act( buf, ch, NULL, victim, TO_CHAR    );

        if (!IS_NPC(victim))
        {
           victimchance = ((get_skill(victim,gsn_observation) + get_skill(victim,gsn_alertness)) / 2) + number_range(1,15);
        }
        else
        {
	    victimchance = ((victim->level *2) / 3) + number_range(1,15);
        }
  
        if (skill > victimchance)
        {
           failed = FALSE;
    	   check_improve(ch,gsn_plant,TRUE,2);
        }
        else
        {
 	    failed = TRUE; 
    	   check_improve(ch,gsn_plant,FALSE,2);
        }
	/*
	 * Bribe trigger
	 */
	if ( IS_NPC(victim) && HAS_TRIGGER( victim, TRIG_BRIBE ) )
	    mp_bribe_trigger( victim, ch, silver ? amount : amount * 100 );

        if (failed)
        {
           act("$n planted something on $N",ch,NULL,victim,TO_ROOM);
	   sprintf(buf,"$n plants %d %s on you.",amount, silver ? "silver" : "gold");
	   act( buf, ch, NULL, victim, TO_VICT    );
        }
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( obj->wear_loc != WEAR_NONE )
    {
	send_to_char( "You must remove it first.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
    {
	act("$N tells you 'Sorry, you'll have to sell that.'",
	    ch,NULL,victim,TO_CHAR);
	//ch->reply = victim;
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if ( victim->carry_number + get_obj_number( obj ) > can_carry_n( victim ) )
    {
	act( "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if (get_carry_weight(victim) + get_obj_weight(obj) > can_carry_w( victim ) )
    {
	act( "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( !can_see_obj( victim, obj ) )
    {
	act( "$N can't see it.", ch, NULL, victim, TO_CHAR );
	return;
    }
    if (!IS_NPC(victim))
    {
       victimchance = ((get_skill(victim,gsn_observation) + get_skill(victim,gsn_alertness)) / 2) + number_range(1,15);
    }
    else
    {
	victimchance = ((victim->level *2) / 3) + number_range(1,15);
    }
  
    if (skill > victimchance)
    {
       failed = FALSE;
    	   check_improve(ch,gsn_plant,TRUE,2);
    }
    else
    {
 	failed = TRUE; 
    	   check_improve(ch,gsn_plant,FALSE,2);
    }

    obj_from_char( obj );
    obj_to_char( obj, victim );
    MOBtrigger = FALSE;
    act( "You plant $p on $N.", ch, obj, victim, TO_CHAR    );
    MOBtrigger = TRUE;
    if (failed)
    {
        act("$n planted something on $N",ch,NULL,victim,TO_ROOM);
        act( "$n plant $p on $N.", ch, obj, victim, TO_VICT   );
    }

    return;
}


/* for poisoning weapons and food/drink */
void do_envenom(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
	AFFECT_DATA af;
    int percent,skill;

    /* find out what */
    if (argument[0] == '\0')
    {
	send_to_char("Envenom what item?\n\r",ch);
	return;
    }

    obj =  get_obj_list(ch,argument,ch->carrying);

    if (obj== NULL)
    {
	send_to_char("You don't have that item.\n\r",ch);
	return;
    }

    if ((skill = get_skill(ch,gsn_envenom)) < 1)
    {
	send_to_char("Are you crazy? You'd poison yourself!\n\r",ch);
	return;
    }

    if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
    {
	if (IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
	{
	    act("You fail to poison $p.",ch,obj,NULL,TO_CHAR);
	    return;
	}

	if (number_percent() < skill)  /* success! */
	{
	    act("$n treats $p with deadly poison.",ch,obj,NULL,TO_ROOM);
	    act("You treat $p with deadly poison.",ch,obj,NULL,TO_CHAR);
	    if (!obj->value[3])
	    {
		obj->value[3] = 1;
		check_improve(ch,gsn_envenom,TRUE,4);
	    }
	    WAIT_STATE(ch,skill_table[gsn_envenom].beats);
	    return;
	}

	act("You fail to poison $p.",ch,obj,NULL,TO_CHAR);
	if (!obj->value[3])
	    check_improve(ch,gsn_envenom,FALSE,4);
	WAIT_STATE(ch,skill_table[gsn_envenom].beats);
	return;
     }

    if (obj->item_type == ITEM_WEAPON)
    {
        if (IS_WEAPON_STAT(obj,WEAPON_FLAMING)
        ||  IS_WEAPON_STAT(obj,WEAPON_FROST)
        ||  IS_WEAPON_STAT(obj,WEAPON_VAMPIRIC)
        //||  IS_WEAPON_STAT(obj,WEAPON_SHARP)
        ||  IS_WEAPON_STAT(obj,WEAPON_VORPAL)
        ||  IS_WEAPON_STAT(obj,WEAPON_SHOCKING)
        ||  IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
        {
            act("You can't seem to envenom $p.",ch,obj,NULL,TO_CHAR);
            return;
        }

	if (obj->value[3] < 0 
	||  attack_table[obj->value[3]].damage == DAM_BASH)
	{
	    send_to_char("You can only envenom edged weapons.\n\r",ch);
	    return;
	}

        if (IS_WEAPON_STAT(obj,WEAPON_POISON))
        {
            act("$p is already envenomed.",ch,obj,NULL,TO_CHAR);
            return;
        }

	percent = number_percent();
			
	if (percent < skill)
	{
 
            af.where     = TO_WEAPON;
            af.type      = gsn_poison;
            af.level     = ch->level * percent / 100;
            af.duration  = ch->level/2 * percent / 100;
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj(obj,&af);
 
            act("$n coats $p with deadly venom.",ch,obj,NULL,TO_ROOM);
	    act("You coat $p with venom.",ch,obj,NULL,TO_CHAR);
	    check_improve(ch,gsn_envenom,TRUE,3);
	    WAIT_STATE(ch,skill_table[gsn_envenom].beats);
            return;
        }
	else
	{
	    act("You fail to envenom $p.",ch,obj,NULL,TO_CHAR);
	    check_improve(ch,gsn_envenom,FALSE,3);
	    WAIT_STATE(ch,skill_table[gsn_envenom].beats);
	    return;
	}
    }
 
    act("You can't poison $p.",ch,obj,NULL,TO_CHAR);
    return;
}


void do_sharpen( CHAR_DATA *ch, char *argument )
{
  OBJ_DATA *obj;
  OBJ_DATA *sobj;
  AFFECT_DATA af;
  char      arg [MAX_INPUT_LENGTH ];
  int       percent;
  

   if (! IS_NPC( ch )
  && ch->level < skill_table [gsn_sharpen].skill_level [ch->class] )
    {
         send_to_char( "What do you think you are, a warrior?\n\r", ch );
         return;
    }

	one_argument ( argument, arg );

    if (arg[0] == '\0' )
    {
        send_to_char( "What are you trying to sharpen?\n\r", ch );
        return;
    }

    if ( ch->fighting )
    {
        send_to_char( "While you are fighting? Yea right!.\n\r", ch );
        return;
    }
	 obj =  get_obj_list(ch,argument,ch->carrying);
	 if (obj == NULL) 
	 {
		send_to_char("You don't have that to sharpen!!!\n\r",ch);
		return;
	 }

    if (obj->wear_loc != -1)
    {
	send_to_char("The item must be carried to be sharpened.\n\r",ch);
	return;
    }

	if (obj->item_type != ITEM_WEAPON)
    {
	send_to_char("That isn't a weapon.\n\r",ch);
	return;
    }

		if (obj->value[3] < 0 
	||  attack_table[obj->value[3]].damage == DAM_BASH)
	{
	    send_to_char("You can only sharpen edged weapons.\n\r",ch);
	    return;
	}


    if ( obj->item_type != ITEM_WEAPON )
    {
        send_to_char( "That item is not a weapon.\n\r", ch );
        return;
    }

    if (  IS_WEAPON_STAT(obj,WEAPON_SHARP) )
    {
        send_to_char( "That weapon is already sharpened.\n\r", ch );
        return;
    }

    /* Ok we have a sharpenable weapon but do we have the stone */ 
 
		for ( sobj = ch->carrying; sobj; sobj = sobj->next_content )
	{
	    if ( sobj->pIndexData->vnum == OBJ_VNUM_STONE )
        break;
    }
    if ( !sobj )
    {
        send_to_char( "You do not have a sharpening stone.\n\r", ch );
        return;
    }
	percent = number_percent();
   if ( !IS_NPC( ch )
   && percent > ch->pcdata->learned[gsn_sharpen] )
   {
       send_to_char( "You failed and slice your finger. Ouch!\n\r", ch );
       
	   damage( ch, ch, ch->level, TYPE_HIT, gsn_sharpen, TRUE );
       act("$n slices $s finger!", ch, NULL, NULL, TO_ROOM );
	   check_improve(ch,gsn_sharpen,FALSE,2);
              return;
   }
	        
			af.where     = TO_WEAPON;
            af.type      = gsn_sharpen;
            af.level     = ch->level * percent / 100;
            af.duration  = ch->level * 5;
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_SHARP;
            affect_to_obj(obj,&af);

      act("You sharpen $p.\n\r", ch, obj, NULL, TO_CHAR );
      act("$n pulls out a piece of stone and begins sharpening $p.", ch, obj, NULL, TO_ROOM );  
//      extract_obj( sobj );	
      check_improve(ch,gsn_sharpen,TRUE,2);
      return;
}


void do_fill( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *fountain;
    bool found;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Fill what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    found = FALSE;
    for ( fountain = ch->in_room->contents; fountain != NULL;
	fountain = fountain->next_content )
    {
	if ( fountain->item_type == ITEM_FOUNTAIN )
	{
	    found = TRUE;
	    break;
	}
    }

    if ( !found )
    {
	send_to_char( "There is no fountain here!\n\r", ch );
	return;
    }

    if ( obj->item_type != ITEM_DRINK_CON )
    {
	send_to_char( "You can't fill that.\n\r", ch );
	return;
    }

    if ( obj->value[1] != 0 && obj->value[2] != fountain->value[2] )
    {
	send_to_char( "There is already another liquid in it.\n\r", ch );
	return;
    }

    if ( obj->value[1] >= obj->value[0] )
    {
	send_to_char( "Your container is full.\n\r", ch );
	return;
    }

    sprintf(buf,"You fill $p with %s from $P.",
	liq_table[fountain->value[2]].liq_name);
    act( buf, ch, obj,fountain, TO_CHAR );
    sprintf(buf,"$n fills $p with %s from $P.",
	liq_table[fountain->value[2]].liq_name);
    act(buf,ch,obj,fountain,TO_ROOM);
    obj->value[2] = fountain->value[2];
    obj->value[1] = obj->value[0];
    return;
}

void do_pour (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH],buf[MAX_STRING_LENGTH];
    OBJ_DATA *out, *in;
    CHAR_DATA *vch = NULL;
    int amount;

    argument = one_argument(argument,arg);
    
    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Pour what into what?\n\r",ch);
	return;
    }
    

    if ((out = get_obj_carry(ch,arg, ch)) == NULL)
    {
	send_to_char("You don't have that item.\n\r",ch);
	return;
    }

    if (out->item_type != ITEM_DRINK_CON)
    {
	send_to_char("That's not a drink container.\n\r",ch);
	return;
    }

    if (!str_cmp(argument,"out"))
    {
	if (out->value[1] == 0)
	{
	    send_to_char("It's already empty.\n\r",ch);
	    return;
	}

	out->value[1] = 0;
	out->value[3] = 0;
	sprintf(buf,"You invert $p, spilling %s all over the ground.",
		liq_table[out->value[2]].liq_name);
	act(buf,ch,out,NULL,TO_CHAR);
	
	sprintf(buf,"$n inverts $p, spilling %s all over the ground.",
		liq_table[out->value[2]].liq_name);
	act(buf,ch,out,NULL,TO_ROOM);
	return;
    }

    if ((in = get_obj_here(ch,argument)) == NULL)
    {
	vch = get_char_room(ch,argument);

	if (vch == NULL)
	{
	    send_to_char("Pour into what?\n\r",ch);
	    return;
	}

	in = get_eq_char(vch,WEAR_HOLD);

	if (in == NULL)
	{
	    send_to_char("They aren't holding anything.",ch);
 	    return;
	}
    }

    if (in->item_type != ITEM_DRINK_CON)
    {
	send_to_char("You can only pour into other drink containers.\n\r",ch);
	return;
    }
    
    if (in == out)
    {
	send_to_char("You cannot change the laws of physics!\n\r",ch);
	return;
    }

    if (in->value[1] != 0 && in->value[2] != out->value[2])
    {
	send_to_char("They don't hold the same liquid.\n\r",ch);
	return;
    }

    if (out->value[1] == 0)
    {
	act("There's nothing in $p to pour.",ch,out,NULL,TO_CHAR);
	return;
    }

    if (in->value[1] >= in->value[0])
    {
	act("$p is already filled to the top.",ch,in,NULL,TO_CHAR);
	return;
    }

    amount = UMIN(out->value[1],in->value[0] - in->value[1]);

    in->value[1] += amount;
    out->value[1] -= amount;
    in->value[2] = out->value[2];
    
    if (vch == NULL)
    {
    	sprintf(buf,"You pour %s from $p into $P.",
	    liq_table[out->value[2]].liq_name);
    	act(buf,ch,out,in,TO_CHAR);
    	sprintf(buf,"$n pours %s from $p into $P.",
	    liq_table[out->value[2]].liq_name);
    	act(buf,ch,out,in,TO_ROOM);
    }
    else
    {
        sprintf(buf,"You pour some %s for $N.",
            liq_table[out->value[2]].liq_name);
        act(buf,ch,NULL,vch,TO_CHAR);
	sprintf(buf,"$n pours you some %s.",
	    liq_table[out->value[2]].liq_name);
	act(buf,ch,NULL,vch,TO_VICT);
        sprintf(buf,"$n pours some %s for $N.",
            liq_table[out->value[2]].liq_name);
        act(buf,ch,NULL,vch,TO_NOTVICT);
	
    }
}

void do_drink( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int amount;
    int liquid;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	for ( obj = ch->in_room->contents; obj; obj = obj->next_content )
	{
	    if ( obj->item_type == ITEM_FOUNTAIN )
		break;
	}

	if ( obj == NULL )
	{
	    send_to_char( "Drink what?\n\r", ch );
	    return;
	}
    }
    else
    {
	if ( ( obj = get_obj_here( ch, arg ) ) == NULL )
	{
	    send_to_char( "You can't find it.\n\r", ch );
	    return;
	}
    }

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10 )
    {
	send_to_char( "You fail to reach your mouth.  *Hic*\n\r", ch );
	return;
    }

    switch ( obj->item_type )
    {
    default:
	send_to_char( "You can't drink from that.\n\r", ch );
	return;

    case ITEM_FOUNTAIN:
        if ( ( liquid = obj->value[2] )  < 0 )
        {
            bug( "Do_drink: bad liquid number %d.", liquid );
            liquid = obj->value[2] = 0;
        }
	amount = liq_table[liquid].liq_affect[4] * 3;
	break;

    case ITEM_DRINK_CON:
	if ( obj->value[1] <= 0 )
	{
	    send_to_char( "It is already empty.\n\r", ch );
	    return;
	}

	if ( ( liquid = obj->value[2] )  < 0 )
	{
	    bug( "Do_drink: bad liquid number %d.", liquid );
	    liquid = obj->value[2] = 0;
	}

        amount = liq_table[liquid].liq_affect[4];
        amount = UMIN(amount, obj->value[1]);
	break;
     }
    if (!IS_NPC(ch) && !IS_IMMORTAL(ch) 
    &&  ch->pcdata->condition[COND_FULL] > 45)
    {
	send_to_char("You're too full to drink more.\n\r",ch);
	return;
    }

    act( "$n drinks $T from $p.",
	ch, obj, liq_table[liquid].liq_name, TO_ROOM );
    act( "You drink $T from $p.",
	ch, obj, liq_table[liquid].liq_name, TO_CHAR );

    gain_condition( ch, COND_DRUNK,
	amount * liq_table[liquid].liq_affect[COND_DRUNK] / 36 );
    gain_condition( ch, COND_FULL,
	amount * liq_table[liquid].liq_affect[COND_FULL] / 4 );
    gain_condition( ch, COND_THIRST,
	amount * liq_table[liquid].liq_affect[COND_THIRST] / 10 );
    gain_condition(ch, COND_HUNGER,
	amount * liq_table[liquid].liq_affect[COND_HUNGER] / 2 );

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]  > 10 )
	send_to_char( "You feel drunk.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL]   > 40 )
	send_to_char( "You are full.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 40 )
	send_to_char( "Your thirst is quenched.\n\r", ch );
	
    if ( obj->value[3] != 0 )
    {
	/* The drink was poisoned ! */
	AFFECT_DATA af;

	act( "$n chokes and gags.", ch, NULL, NULL, TO_ROOM );
	send_to_char( "You choke and gag.\n\r", ch );
	af.where     = TO_AFFECTS;
	af.type      = gsn_poison;
	af.level	 = number_fuzzy(amount); 
	af.duration  = 3 * amount;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_POISON;
	affect_join( ch, &af );
    }
	
    if (obj->value[0] > 0)
        obj->value[1] -= amount;

    return;
}



void do_eat( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Eat what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( !IS_IMMORTAL(ch) )
    {
	if ( obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL )
	{
	    send_to_char( "That's not edible.\n\r", ch );
	    return;
	}

	if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40 )
	{   
	    send_to_char( "You are too full to eat more.\n\r", ch );
	    return;
	}
    }

    act( "$n eats $p.",  ch, obj, NULL, TO_ROOM );
    act( "You eat $p.", ch, obj, NULL, TO_CHAR );

    switch ( obj->item_type )
    {

    case ITEM_FOOD:
	if ( !IS_NPC(ch) )
	{
	    int condition;

	    condition = ch->pcdata->condition[COND_HUNGER];
	    gain_condition( ch, COND_FULL, obj->value[0] );
	    gain_condition( ch, COND_HUNGER, obj->value[1]);
	    if ( condition == 0 && ch->pcdata->condition[COND_HUNGER] > 0 )
		send_to_char( "You are no longer hungry.\n\r", ch );
	    else if ( ch->pcdata->condition[COND_FULL] > 40 )
		send_to_char( "You are full.\n\r", ch );
	}

	if ( obj->value[3] != 0 )
	{
	    /* The food was poisoned! */
	    AFFECT_DATA af;

	    act( "$n chokes and gags.", ch, 0, 0, TO_ROOM );
	    send_to_char( "You choke and gag.\n\r", ch );

	    af.where	  = TO_AFFECTS;
	    af.type      = gsn_poison;
	    af.level 	  = number_fuzzy(obj->value[0]);
	    af.duration  = 2 * obj->value[0];
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = AFF_POISON;
	    affect_join( ch, &af );
	}
	break;

    case ITEM_PILL:
	obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
	obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
	obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
	break;
    }

    extract_obj( obj );
    return;
}



/*
 * Remove an object.
 */
bool remove_obj( CHAR_DATA *ch, int iWear, bool fReplace )
{
    OBJ_DATA *obj;

    if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
	return TRUE;

    if ( !fReplace )
	return FALSE;

    if ( IS_SET(obj->extra_flags, ITEM_NOREMOVE) )
    {
	act( "You can't remove $p.", ch, obj, NULL, TO_CHAR );
	return FALSE;
    }

	if (( obj->item_type == ITEM_TATTOO ) && ( !IS_IMMORTAL(ch) ) )
    {
	act( "You must scratch it to remove $p.", ch, obj, NULL, TO_CHAR );
	return FALSE;
    }

    if ( iWear == WEAR_STUCK_IN ) 
    {
        unequip_char( ch, obj );

	if ( get_eq_char(ch,WEAR_STUCK_IN) == NULL)
	{
	  if  (is_affected(ch,gsn_arrow)) affect_strip(ch,gsn_arrow);
	  if  (is_affected(ch,gsn_spear)) affect_strip(ch,gsn_spear);
	}
	act( "You scream in pain as you remove $p.", ch, obj, NULL, TO_CHAR );
	act( "$n screams in pain as they pull out $p.", ch, obj, NULL, TO_ROOM );
	WAIT_STATE(ch,4);
	return TRUE;
    }
    unequip_char( ch, obj );
    act( "$n stops using $p.", ch, obj, NULL, TO_ROOM );
    act( "You stop using $p.", ch, obj, NULL, TO_CHAR );
    
	if ( iWear == WEAR_WIELD 
	&& (obj = get_eq_char(ch,WEAR_SECOND_WIELD)) != NULL) 
    {
     unequip_char( ch, obj);
     equip_char(ch,obj,WEAR_WIELD);
    }

	return TRUE;
}



/*
 * can_wear_angreal
 * Checks to see if the character can wear another angreal
 */
bool can_wear_angreal (CHAR_DATA *ch)
{
   
   int CA_ADJUST = 25;  //max per sphere that can be adjusted before someone can't wear another angreal
   int NORMAL_ADJUST = 5; //MAX for non-create angreal people
   int adjustmentvalue = 0;
   int i = 0;
   bool retval = TRUE;

   if (IS_IMMORTAL(ch))
      return retval;
 
/*
    if (IS_SET(ch->talents, TALENT_CREATE_ANGREAL))
       adjustmentvalue = CA_ADJUST;
    else
       adjustmentvalue = NORMAL_ADJUST;
     
    for(i = 0; i < MAX_SPHERE; i++) {
         if (ch->perm_sphere[i] >=  ch->cre_sphere[i] + adjustmentvalue)
            retval = FALSE;
    }
*/

    return retval;

}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, char * location )
{
    char buf[MAX_STRING_LENGTH];

    //if ( ch->level < obj->level )
    if ( get_level(ch) < obj->level ) {
	 sprintf( buf, "You must be level %d to use this object.\n\r",
			obj->level );
	 send_to_char( buf, ch );
	 act( "$n tries to use $p, but is too inexperienced.",
		 ch, obj, NULL, TO_ROOM );
	 return;
    }

    if ((IS_SET(obj->wear_flags,  ITEM_WEAR_FEMALE_ONLY) && ch->sex != SEX_FEMALE) ||
        (IS_SET(obj->wear_flags,  ITEM_WEAR_MALE_ONLY) && ch->sex != SEX_MALE)) {
	sprintf( buf, "You are the wrong gender to use this object.\n\r");
	send_to_char(buf,ch);
	return;
    }
 

    if ( obj->item_type == ITEM_LIGHT && !strcasecmp(location,"default")) {
	 if ( !remove_obj( ch, WEAR_LIGHT, fReplace ) )
	   return;
	 act( "$p begins to float around $n's shoulder.", ch, obj, NULL, TO_ROOM );
	 act( "$p begins to float near your shoulder.",  ch, obj, NULL, TO_CHAR );
/*
	 act( "$n lights $p and holds it.", ch, obj, NULL, TO_ROOM );
	 act( "You light $p and hold it.",  ch, obj, NULL, TO_CHAR );
*/
	 equip_char( ch, obj, WEAR_LIGHT );
	 return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_FINGER ))) ||
        ((!strcasecmp(location,"finger")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_FINGER))))
    {
	if ( get_eq_char( ch, WEAR_FINGER_L ) != NULL
	&&   get_eq_char( ch, WEAR_FINGER_R ) != NULL
	&&   !remove_obj( ch, WEAR_FINGER_L, fReplace )
	&&   !remove_obj( ch, WEAR_FINGER_R, fReplace ) )
	    return;

	if ( get_eq_char( ch, WEAR_FINGER_L ) == NULL )
	{
	    act( "$n wears $p on $s left finger.",    ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your left finger.",  ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_FINGER_L );
	    return;
	}

	if ( get_eq_char( ch, WEAR_FINGER_R ) == NULL )
	{
	    act( "$n wears $p on $s right finger.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your right finger.", ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_FINGER_R );
	    return;
	}

	bug( "Wear_obj: no free finger.", 0 );
	send_to_char( "You already wear two rings.\n\r", ch );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_NECK ))) ||
        ((!strcasecmp(location,"neck")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_NECK))))
    {
	if ( get_eq_char( ch, WEAR_NECK_1 ) != NULL
	&&   get_eq_char( ch, WEAR_NECK_2 ) != NULL
	&&   !remove_obj( ch, WEAR_NECK_1, fReplace )
	&&   !remove_obj( ch, WEAR_NECK_2, fReplace ) )
	    return;

	if ( get_eq_char( ch, WEAR_NECK_1 ) == NULL )
	{
	    act( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_NECK_1 );
	    return;
	}

	if ( get_eq_char( ch, WEAR_NECK_2 ) == NULL )
	{
	    act( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_NECK_2 );
	    return;
	}

	bug( "Wear_obj: no free neck.", 0 );
	send_to_char( "You already wear two neck items.\n\r", ch );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_BODY ))) ||
        ((!strcasecmp(location,"body")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_BODY))))
    {
	if ( !remove_obj( ch, WEAR_BODY, fReplace ) )
	    return;
	act( "$n wears $p on $s torso.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your torso.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_BODY );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_HEAD ))) ||
        ((!strcasecmp(location,"head")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_HEAD))))
    {
	if ( !remove_obj( ch, WEAR_HEAD, fReplace ) )
	    return;
	act( "$n wears $p on $s head.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your head.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_HEAD );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_LEGS ))) ||
        ((!strcasecmp(location,"legs")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_LEGS))))
    {
	if ( !remove_obj( ch, WEAR_LEGS, fReplace ) )
	    return;
	act( "$n wears $p on $s legs.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your legs.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_LEGS );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_FEET ))) ||
        ((!strcasecmp(location,"feet")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_FEET))))
    {
	if ( !remove_obj( ch, WEAR_FEET, fReplace ) )
	    return;
	act( "$n wears $p on $s feet.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your feet.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_FEET );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_HANDS ))) ||
        ((!strcasecmp(location,"hands")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_HANDS))))
    {
	if ( !remove_obj( ch, WEAR_HANDS, fReplace ) )
	    return;
	act( "$n wears $p on $s hands.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your hands.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_HANDS );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_ARMS ))) ||
        ((!strcasecmp(location,"arms")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_ARMS))))
    {
	if ( !remove_obj( ch, WEAR_ARMS, fReplace ) )
	    return;
	act( "$n wears $p on $s arms.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your arms.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_ARMS );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_ABOUT ))) ||
        ((!strcasecmp(location,"about")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_ABOUT))))
    {
	if ( !remove_obj( ch, WEAR_ABOUT, fReplace ) )
	    return;
	act( "$n wears $p about $s torso.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p about your torso.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_ABOUT );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_WAIST ))) ||
        ((!strcasecmp(location,"waist")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_WAIST))))
    {
	if ( !remove_obj( ch, WEAR_WAIST, fReplace ) )
	    return;
	act( "$n wears $p about $s waist.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p about your waist.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_WAIST );
	return;
    }
    
    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_BACK ))) ||
        ((!strcasecmp(location,"back")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_BACK))))
    {
	if ( !remove_obj( ch, WEAR_BACK, fReplace ) )
	    return;
	act( "$n wears $p on $s back.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your back.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_BACK );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_FACE ))) ||
        ((!strcasecmp(location,"face")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_FACE))))
    {
	if ( !remove_obj( ch, WEAR_FACE, fReplace ) )
	    return;
	act( "$n wears $p over $s face.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p over your face.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_FACE );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_WRIST ))) ||
        ((!strcasecmp(location,"wrist")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_WRIST))))
    {
	if ( get_eq_char( ch, WEAR_WRIST_L ) != NULL
	&&   get_eq_char( ch, WEAR_WRIST_R ) != NULL
	&&   !remove_obj( ch, WEAR_WRIST_L, fReplace )
	&&   !remove_obj( ch, WEAR_WRIST_R, fReplace ) )
	    return;

	if ( get_eq_char( ch, WEAR_WRIST_L ) == NULL )
	{
	    act( "$n wears $p around $s left wrist.",
		ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your left wrist.",
		ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_WRIST_L );
	    return;
	}

	if ( get_eq_char( ch, WEAR_WRIST_R ) == NULL )
	{
	    act( "$n wears $p around $s right wrist.",
		ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your right wrist.",
		ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_WRIST_R );
	    return;
	}

	bug( "Wear_obj: no free wrist.", 0 );
	send_to_char( "You already wear two wrist items.\n\r", ch );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_EAR ))) ||
        ((!strcasecmp(location,"ear")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_EAR))))
    {
	if ( get_eq_char( ch, WEAR_EAR_L ) != NULL
	&&   get_eq_char( ch, WEAR_EAR_R ) != NULL
	&&   !remove_obj( ch, WEAR_EAR_L, fReplace )
	&&   !remove_obj( ch, WEAR_EAR_R, fReplace ) )
	    return;

	if ( get_eq_char( ch, WEAR_EAR_L ) == NULL )
	{
	    act( "$n wears $p in $s left ear.",
		ch, obj, NULL, TO_ROOM );
	    act( "You wear $p in your left ear.",
		ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_EAR_L );
	    return;
	}

	if ( get_eq_char( ch, WEAR_EAR_R ) == NULL )
	{
	    act( "$n wears $p in $s right ear.",
		ch, obj, NULL, TO_ROOM );
	    act( "You wear $p in your right ear.",
		ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_EAR_R );
	    return;
	}

	bug( "Wear_obj: no free ear.", 0 );
	send_to_char( "You already wear two ear items.\n\r", ch );
	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_SHIELD ))) ||
        ((!strcasecmp(location,"shield")) && 
         ( CAN_WEAR( obj, ITEM_WEAR_SHIELD))))
    {
	OBJ_DATA *weapon;

	if ( get_eq_char(ch, WEAR_SECOND_WIELD) != NULL ) {
	  send_to_char("You can't use a shield while using a second weapon.\n\r", ch);
	  return;
	 }

	weapon = get_eq_char(ch,WEAR_WIELD);
        
        if (!IS_NPC(ch) && weapon != NULL && IS_SET(ch->flaws, FLAW_ONEARM)) {
           send_to_char("You can't wield a weapon and hold a shield at the same time with one arm.\n\r", ch);
           return;	
        }
	
	if ( !remove_obj( ch, WEAR_SHIELD, fReplace ) )
	    return;

	if (weapon != NULL && ch->size < SIZE_LARGE 
	&&  IS_WEAPON_STAT(weapon,WEAPON_TWO_HANDS))
	{
	    send_to_char("Your hands are tied up with your weapon!\n\r",ch);
	    return;
	}

	act( "$n wears $p as a shield.", ch, obj, NULL, TO_ROOM );
	act( "You wear $p as a shield.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_SHIELD );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WIELD ) )
    {
	int sn,skill;
	OBJ_DATA *dual;

	if ( (dual = get_eq_char(ch, WEAR_SECOND_WIELD)) != NULL)
	  unequip_char(ch,dual);
	
	if ( !remove_obj( ch, WEAR_WIELD, fReplace ) )
	    return;

        if (!IS_NPC(ch) && IS_SET(ch->flaws, FLAW_ONEARM) && get_eq_char(ch,WEAR_SHIELD) != NULL) {
           send_to_char("You can't wield a weapon and hold a shield at the same time with one arm.\n\r", ch);
           return;	
        }

	if ( !IS_NPC(ch) 
	&& get_obj_weight(obj) > (str_app[get_curr_stat(ch,STAT_STR)].wield  
		* 10))
	{
	    send_to_char( "It is too heavy for you to wield.\n\r", ch );
	    if (dual) 
			equip_char(ch,dual,WEAR_SECOND_WIELD);
		return;
	}

	if (IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS) &&
		((!IS_NPC(ch) && ch->size < SIZE_LARGE
  		  && get_eq_char(ch,WEAR_SHIELD) != NULL)
		 || get_eq_char(ch,WEAR_SECOND_WIELD) !=NULL ) )
	{
	    send_to_char("You need two hands free for that weapon.\n\r",ch);
	    if (dual) equip_char(ch,dual,WEAR_SECOND_WIELD);
		return;
	}

	act( "$n wields $p.", ch, obj, NULL, TO_ROOM );
	act( "You wield $p.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_WIELD );
	if (dual) 
		equip_char(ch,dual,WEAR_SECOND_WIELD);
        sn = get_weapon_sn(ch);

	if (sn == gsn_hand_to_hand)
	   return;

        skill = get_weapon_skill(ch,sn);
 
        if (skill >= 100)
            act("$p feels like a part of you!",ch,obj,NULL,TO_CHAR);
        else if (skill > 85)
            act("You feel quite confident with $p.",ch,obj,NULL,TO_CHAR);
        else if (skill > 70)
            act("You are skilled with $p.",ch,obj,NULL,TO_CHAR);
        else if (skill > 50)
            act("Your skill with $p is adequate.",ch,obj,NULL,TO_CHAR);
        else if (skill > 25)
            act("$p feels a little clumsy in your hands.",ch,obj,NULL,TO_CHAR);
        else if (skill > 1)
            act("You fumble and almost drop $p.",ch,obj,NULL,TO_CHAR);
        else
            act("You don't even know which end is up on $p.",
                ch,obj,NULL,TO_CHAR);

	return;
    }

    if (((!strcasecmp(location,"default")) && 
         ( CAN_WEAR( obj, ITEM_HOLD ))) ||
        ((!strcasecmp(location,"hold")) && 
         ( CAN_WEAR( obj, ITEM_HOLD))))
    {
	if ( get_eq_char(ch, WEAR_SECOND_WIELD) != NULL )
	 {
	  send_to_char("You can't hold an item while using 2 weapons.\n\r",ch);
	  return;
	 }
		if ( !remove_obj( ch, WEAR_HOLD, fReplace ) )
	    return;
	act( "$n holds $p in $s hand.",   ch, obj, NULL, TO_ROOM );
	act( "You hold $p in your hand.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_HOLD );
	return;
    }

    if ( CAN_WEAR(obj,ITEM_WEAR_FLOAT) )
    {
	if (!remove_obj(ch,WEAR_FLOAT, fReplace) )
	    return;
	act("$n releases $p to float next to $m.",ch,obj,NULL,TO_ROOM);
	act("You release $p and it floats next to you.",ch,obj,NULL,TO_CHAR);
	equip_char(ch,obj,WEAR_FLOAT);
	return;
    }

   if ( CAN_WEAR(obj,ITEM_WEAR_TATTOO))
    {
	if (!remove_obj(ch,WEAR_TATTOO, fReplace) )
	    return;
	act("$n now uses $p as tattoo.",ch,obj,NULL,TO_ROOM);
	act("You now use $p as the tattoo.",ch,obj,NULL,TO_CHAR);
	equip_char(ch,obj,WEAR_TATTOO);
	return;
    }

    if ( (fReplace) && (!strcasecmp(location,"default") ) )
    {
	send_to_char( "You can't wear, wield, or hold that.\n\r", ch );
    }
    else if (fReplace)
    {
	send_to_char( "You can't wear, wield, or hold that there.\n\r", ch );
    }

    return;
}



void do_wear( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  
  argument = one_argument( argument, arg );

  if (IS_WOLFSHAPE(ch)) {
    send_to_char("You can't wear, wield or hold while beeing a wolf.\n\r", ch);
    return;
  }
  
  if ( arg[0] == '\0' ) {
    send_to_char( "Wear, wield, or hold what?\n\r", ch );
    return;
  }

  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\n\r", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  if (IS_NPC(ch))
  {
     if (race_table[ch->race].pc_race == FALSE)
     {
	send_to_char("You aren't in a form that can wear that.\r\n",ch);
	act("$n looks a little confused.",ch,NULL,NULL, TO_ROOM);
	return;
     }
  }


  if ( !str_cmp( arg, "all" ) ) {
    OBJ_DATA *obj_next;
    
    for ( obj = ch->carrying; obj != NULL; obj = obj_next ) {
	 obj_next = obj->next_content;
         if ( obj->item_type == ITEM_ANGREAL)
         {
		if (!can_wear_angreal(ch))
                {
		   continue;
                }
         }
	 if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
	   if ( !IS_OBJ_STAT(obj, ITEM_BROKEN))
	   {
		if ((obj->level < ch->level - 15) && obj->item_type != ITEM_LIGHT && obj->item_type != ITEM_NOTEPAPER)
		{
			continue;
		}
		else
			wear_obj( ch, obj, FALSE, "default");
	   }
    }
    return;
  }
  else {

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL ) {
	 send_to_char( "You do not have that item.\n\r", ch );
	 return;
    }
    if (IS_OBJ_STAT( obj, ITEM_BROKEN )) {
	 send_to_char("You cannot use that! It's {Rbroken{x!\n\r", ch );
	 return;
    }

/*
    if ((obj->level < ch->level - 15) && obj->item_type != ITEM_LIGHT && obj->pIndexData->vnum != OBJ_VNUM_STONE )
    {
	send_to_char("You've outgrown that equipment and can not use it.\n\r",ch);
	return;
    }
*/

    if ( obj->item_type == ITEM_ANGREAL)
    {
           if (!can_wear_angreal(ch))
           {
              send_to_char("That would be pushing your limits just a bit too much.\r\n",ch);
              return;
           }
    }

    

    /* Multiple wear locations, fix so first reflect where to wear it */
    if ( argument[0] != '\0') {
         char wearloc[256];
	 argument = one_argument(argument,wearloc);
	 if ((strcasecmp(wearloc,"head")) &&
             (strcasecmp(wearloc,"arms")) &&
             (strcasecmp(wearloc,"legs")) &&
             (strcasecmp(wearloc,"feet")) &&
             (strcasecmp(wearloc,"hands")) &&
             (strcasecmp(wearloc,"wrist")) &&
             (strcasecmp(wearloc,"ear")) &&
             (strcasecmp(wearloc,"face")) &&
             (strcasecmp(wearloc,"neck")) &&
             (strcasecmp(wearloc,"about")) &&
             (strcasecmp(wearloc,"body")) &&
             (strcasecmp(wearloc,"back")) &&
             (strcasecmp(wearloc,"waist")) &&
             (strcasecmp(wearloc,"finger")) &&
             (strcasecmp(wearloc,"hold")) &&
             (strcasecmp(wearloc,"tattoo")) &&
             (strcasecmp(wearloc,"shield")))
        {
  
	 send_to_char("Valid options are: head, face, arms, body, legs, back, about\r\n",ch);
	 send_to_char("wrist, ear, neck, back, waist, finger, shield, tattoo, hold\r\n",ch);
	 return;

	}
	else
        {

    		if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)) {
      			if (get_eq_char( ch, WEAR_SECOND_WIELD ) != NULL)
      			{
         			send_to_char ("It must not be a two-handed weapon with another weapon!\n\r",ch);
         			return;
      			}
    		}
		wear_obj( ch, obj, TRUE, wearloc);
                return;
        }
    }
    
    if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)) {
      if (get_eq_char( ch, WEAR_SECOND_WIELD ) != NULL)
      {
         send_to_char ("It must not be a two-handed weapon with another weapon!\n\r",ch);
         return;
      }
    }
    wear_obj( ch, obj, TRUE, "default" );
  }
  return;
}



void do_remove( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  
  one_argument( argument, arg );
  
  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\n\r", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }
  if ( arg[0] == '\0' ) {
    send_to_char( "Remove what?\n\r", ch );
    return;
  }
  if ( !str_cmp( arg, "all" ) ) {
    OBJ_DATA *obj_next;
    
    for ( obj = ch->carrying; obj != NULL; obj = obj_next ) {
	 obj_next = obj->next_content;
	 if ( obj->wear_loc != WEAR_NONE && 
		 obj->wear_loc != WEAR_SCABBARD_1 &&
		 obj->wear_loc != WEAR_SCABBARD_2 &&
		 can_see_obj( ch, obj ) )
	   remove_obj( ch, obj->wear_loc, TRUE );
    }
    return;
  }

  if ( ( obj = get_obj_wear( ch, arg ) ) == NULL ) {
    send_to_char( "You do not have that item.\n\r", ch );
    return;
  }

  if (obj->wear_loc == WEAR_SCABBARD_1 ||  obj->wear_loc == WEAR_SCABBARD_2) {
    send_to_char("You need to draw weapons before they can be removed\n\r", ch);
    return;
  }
  
  remove_obj( ch, obj->wear_loc, TRUE );
  return;
}



void do_sacrifice( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *obj      = NULL;
  OBJ_DATA *obj_next = NULL;
  int silver;
  
  /* variables for AUTOSPLIT */
  CHAR_DATA *gch;
  int members;
  char buffer[100];
  
  
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' || !str_cmp( arg, ch->name ) ) {
    act( "$n offers $mself to the Creator, who graciously declines.",
	    ch, NULL, NULL, TO_ROOM );
    send_to_char("The Creator appreciates your offer and may accept it later.\n\r", ch );
    return;
  }

  if (!str_cmp(arg, "all")) {
    for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
	 obj_next = obj->next_content;
	 
	 if ( obj->item_type == ITEM_CORPSE_PC ) {
	   if (obj->contains) {
		continue;
	   }
	 }
    
	 if ( !CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC))
	   continue;

	 if (obj->in_room != NULL) {
	   for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
		if (gch->on == obj) {
		  act("$N appears to be using $p.", ch,obj,gch,TO_CHAR);
		  continue;
		}
	 }

	 silver = UMAX(5,obj->level/2);

	 if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
	 silver = UMIN(silver,obj->cost);
    
    if (silver == 1)
	 send_to_char("The Creator gives you one {Wsi{xl{Wver{x coin for your sacrifice.\n\r", ch );
    else {
	 sprintf(buf,"The Creator gives you {W%d si{xl{Wver{x coins for your sacrifice.\n\r", silver);
	 send_to_char(buf,ch);
    }
    
    ch->silver += silver;
    
    if (IS_SET(ch->act,PLR_AUTOSPLIT) ) { /* AUTOSPLIT code */
	 members = 0;
	 for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room ) {
	   if ( is_same_group( gch, ch ) )
		members++;
	 }
	 
	 if ( members > 1 && silver > 1) {
	   sprintf(buffer,"%d",silver);
	   do_function(ch, &do_split, buffer);	
	 }
    }
    
    act( "$n sacrifices $p to the Creator.", ch, obj, NULL, TO_ROOM );
    wiznet("$N sends up $p as a burnt offering.",
		 ch,obj,WIZ_SACCING,0,0);
    extract_obj( obj );

    continue;
    }
  }
  else {
    obj = get_obj_list( ch, arg, ch->in_room->contents );

    if ( obj == NULL ) {
	 send_to_char( "You can't find it.\n\r", ch );
	 return;
    }
  
    if ( obj->item_type == ITEM_CORPSE_PC ) {
	 if (obj->contains) {
	   send_to_char("The Creator wouldn't like that.\n\r",ch);
	   return;
	 }
    }
  
    if ( !CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC)) {
	 act( "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR );
	 return;
    }
  
    if (obj->in_room != NULL) {
	 for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
	   if (gch->on == obj) {
		act("$N appears to be using $p.", ch,obj,gch,TO_CHAR);
		return;
	   }
    }
  
    silver = UMAX(5,obj->level/2);
    
    if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
	 silver = UMIN(silver,obj->cost);
    
    if (silver == 1)
	 send_to_char("The Creator gives you one {Wsi{xl{Wver{x coin for your sacrifice.\n\r", ch );
    else {
	 sprintf(buf,"The Creator gives you {W%d si{xl{Wver{x coins for your sacrifice.\n\r", silver);
	 send_to_char(buf,ch);
    }
    
    ch->silver += silver;
    
    if (IS_SET(ch->act,PLR_AUTOSPLIT) ) { /* AUTOSPLIT code */
	 members = 0;
	 for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room ) {
	   if ( is_same_group( gch, ch ) )
		members++;
	 }
	 
	 if ( members > 1 && silver > 1) {
	   sprintf(buffer,"%d",silver);
	   do_function(ch, &do_split, buffer);	
	 }
    }
    
    act( "$n sacrifices $p to the Creator.", ch, obj, NULL, TO_ROOM );
    wiznet("$N sends up $p as a burnt offering.",
		 ch,obj,WIZ_SACCING,0,0);
    extract_obj( obj );
    return;
  }
  return;
}

void do_sacrificeOLD( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *obj;
  int silver;
  
  /* variables for AUTOSPLIT */
  CHAR_DATA *gch;
  int members;
  char buffer[100];
  
  
  one_argument( argument, arg );
  
  if ( arg[0] == '\0' || !str_cmp( arg, ch->name ) ) {
    act( "$n offers $mself to the Creator, who graciously declines.",
	    ch, NULL, NULL, TO_ROOM );
    send_to_char("The Creator appreciates your offer and may accept it later.\n\r", ch );
    return;
  }
  
  obj = get_obj_list( ch, arg, ch->in_room->contents );
  if ( obj == NULL ) {
    send_to_char( "You can't find it.\n\r", ch );
    return;
  }
  
  if ( obj->item_type == ITEM_CORPSE_PC ) {
    if (obj->contains) {
	 send_to_char("The Creator wouldn't like that.\n\r",ch);
	 return;
    }
  }
  
  
  if ( !CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC)) {
    act( "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR );
    return;
  }
  
  if (obj->in_room != NULL) {
    for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
	 if (gch->on == obj) {
	   act("$N appears to be using $p.", ch,obj,gch,TO_CHAR);
	   return;
	 }
  }
  
  silver = UMAX(1,obj->level * 3);
  
  if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
    silver = UMIN(silver,obj->cost);
  
  if (silver == 1)
    send_to_char("The Creator gives you one {Wsi{xl{Wver{x coin for your sacrifice.\n\r", ch );
  else {
    sprintf(buf,"The Creator gives you {W%d si{xl{Wver{x coins for your sacrifice.\n\r", silver);
    send_to_char(buf,ch);
  }
  
  ch->silver += silver;
  
  if (IS_SET(ch->act,PLR_AUTOSPLIT) ) { /* AUTOSPLIT code */
    members = 0;
    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room ) {
	 if ( is_same_group( gch, ch ) )
	   members++;
    }
    
    if ( members > 1 && silver > 1) {
	 sprintf(buffer,"%d",silver);
	 do_function(ch, &do_split, buffer);	
    }
  }
  
  act( "$n sacrifices $p to the Creator.", ch, obj, NULL, TO_ROOM );
  wiznet("$N sends up $p as a burnt offering.",
	    ch,obj,WIZ_SACCING,0,0);
  extract_obj( obj );
  return;
}

void do_quaff( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Quaff what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	send_to_char( "You do not have that potion.\n\r", ch );
	return;
    }

    if ( obj->item_type != ITEM_POTION )
    {
	send_to_char( "You can quaff only potions.\n\r", ch );
	return;
    }

    if (ch->level < obj->level)
    {
	send_to_char("This liquid is too powerful for you to drink.\n\r",ch);
	return;
    }

    act( "$n quaffs $p.", ch, obj, NULL, TO_ROOM );
    act( "You quaff $p.", ch, obj, NULL ,TO_CHAR );

    obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
    obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
    obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );

    extract_obj( obj );
    return;
}



void do_recite( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *scroll;
    OBJ_DATA *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( ( scroll = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that scroll.\n\r", ch );
	return;
    }

    if ( scroll->item_type != ITEM_SCROLL )
    {
	send_to_char( "You can recite only scrolls.\n\r", ch );
	return;
    }

    if ( ch->level < scroll->level)
    {
	send_to_char(
		"This scroll is too complex for you to comprehend.\n\r",ch);
	return;
    }

    obj = NULL;
    if ( arg2[0] == '\0' )
    {
	victim = ch;
    }
    else
    {
	if ( ( victim = get_char_room ( ch, arg2 ) ) == NULL
	&&   ( obj    = get_obj_here  ( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You can't find it.\n\r", ch );
	    return;
	}
    }

    act( "$n recites $p.", ch, scroll, NULL, TO_ROOM );
    act( "You recite $p.", ch, scroll, NULL, TO_CHAR );

    if (number_percent() >= 20 + get_skill(ch,gsn_scrolls) * 4/5)
    {
	send_to_char("You mispronounce a syllable.\n\r",ch);
	check_improve(ch,gsn_scrolls,FALSE,2);
    }

    else
    {
    	obj_cast_spell( scroll->value[1], scroll->value[0], ch, victim, obj );
    	obj_cast_spell( scroll->value[2], scroll->value[0], ch, victim, obj );
    	obj_cast_spell( scroll->value[3], scroll->value[0], ch, victim, obj );
	check_improve(ch,gsn_scrolls,TRUE,2);
    }

    extract_obj( scroll );
    return;
}



void do_brandish( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    OBJ_DATA *staff;
    int sn;

    if ( ( staff = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
	send_to_char( "You hold nothing in your hand.\n\r", ch );
	return;
    }

    if ( staff->item_type != ITEM_STAFF )
    {
	send_to_char( "You can brandish only with a staff.\n\r", ch );
	return;
    }

    if ( ( sn = staff->value[3] ) < 0
    ||   sn >= MAX_SKILL
    ||   skill_table[sn].spell_fun == 0 )
    {
	bug( "Do_brandish: bad sn %d.", sn );
	return;
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( staff->value[2] > 0 )
    {
	act( "$n brandishes $p.", ch, staff, NULL, TO_ROOM );
	act( "You brandish $p.",  ch, staff, NULL, TO_CHAR );
	if ( ch->level < staff->level 
	||   number_percent() >= 20 + get_skill(ch,gsn_staves) * 4/5)
 	{
	    act ("You fail to invoke $p.",ch,staff,NULL,TO_CHAR);
	    act ("...and nothing happens.",ch,NULL,NULL,TO_ROOM);
	    check_improve(ch,gsn_staves,FALSE,2);
	}
	
	else for ( vch = ch->in_room->people; vch; vch = vch_next )
	{
	    vch_next	= vch->next_in_room;

	    switch ( skill_table[sn].target )
	    {
	    default:
		bug( "Do_brandish: bad target for sn %d.", sn );
		return;

	    case TAR_IGNORE:
		if ( vch != ch )
		    continue;
		break;

	    case TAR_CHAR_OFFENSIVE:
		if ( IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch) )
		    continue;
		break;
		
	    case TAR_CHAR_DEFENSIVE:
		if ( IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch) )
		    continue;
		break;

	    case TAR_CHAR_SELF:
		if ( vch != ch )
		    continue;
		break;
	    }

	    obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
	    check_improve(ch,gsn_staves,TRUE,2);
	}
    }

    if ( --staff->value[2] <= 0 )
    {
	act( "$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM );
	act( "Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR );
	extract_obj( staff );
    }

    return;
}



void do_zap( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wand;
    OBJ_DATA *obj;

    one_argument( argument, arg );
    if ( arg[0] == '\0' && ch->fighting == NULL )
    {
	send_to_char( "Zap whom or what?\n\r", ch );
	return;
    }

    if ( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
	send_to_char( "You hold nothing in your hand.\n\r", ch );
	return;
    }

    if ( wand->item_type != ITEM_WAND )
    {
	send_to_char( "You can zap only with a wand.\n\r", ch );
	return;
    }

    obj = NULL;
    if ( arg[0] == '\0' )
    {
	if ( ch->fighting != NULL )
	{
	    victim = ch->fighting;
	}
	else
	{
	    send_to_char( "Zap whom or what?\n\r", ch );
	    return;
	}
    }
    else
    {
	if ( ( victim = get_char_room ( ch, arg ) ) == NULL
	&&   ( obj    = get_obj_here  ( ch, arg ) ) == NULL )
	{
	    send_to_char( "You can't find it.\n\r", ch );
	    return;
	}
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( wand->value[2] > 0 )
    {
	if ( victim != NULL )
	{
	    act( "$n zaps $N with $p.", ch, wand, victim, TO_NOTVICT );
	    act( "You zap $N with $p.", ch, wand, victim, TO_CHAR );
	    act( "$n zaps you with $p.",ch, wand, victim, TO_VICT );
	}
	else
	{
	    act( "$n zaps $P with $p.", ch, wand, obj, TO_ROOM );
	    act( "You zap $P with $p.", ch, wand, obj, TO_CHAR );
	}

 	if (ch->level < wand->level 
	||  number_percent() >= 20 + get_skill(ch,gsn_wands) * 4/5) 
	{
	    act( "Your efforts with $p produce only smoke and sparks.",
		 ch,wand,NULL,TO_CHAR);
	    act( "$n's efforts with $p produce only smoke and sparks.",
		 ch,wand,NULL,TO_ROOM);
	    check_improve(ch,gsn_wands,FALSE,2);
	}
	else
	{
	    obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
	    check_improve(ch,gsn_wands,TRUE,2);
	}
    }

    if ( --wand->value[2] <= 0 )
    {
	act( "$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM );
	act( "Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR );
	extract_obj( wand );
    }

    return;
}



void do_steal( CHAR_DATA *ch, char *argument )
{
  char buf  [MAX_STRING_LENGTH];
  char log_buf[MSL];
  char arg1 [MAX_INPUT_LENGTH];
  char arg2 [MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  OBJ_DATA *obj;
  int percent;
  int level_range=0;
  
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );
  
  if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
    send_to_char( "Steal what from whom?\n\r", ch );
    return;
  }
  
    
  if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
    send_to_char( "They aren't here.\n\r", ch );
    return;
  }

  if ( victim == ch ) {
    send_to_char( "That's pointless. You already have it in your pocket!\n\r", ch );
    return;
  }
  
  if (is_safe(ch,victim)) {
    send_to_char( "Somehow you are just not able to do that.\n\r", ch);
    return;
  }
  
  if ( IS_NPC(victim)  && victim->position == POS_FIGHTING) {
    send_to_char(  "Kill stealing is not permitted.\n\r"
			    "You'd better not -- you might get hit.\n\r",ch);
    return;
  }

  if (!IS_NPC(victim)) 
  {
  	if (!IS_RP(ch) || !IS_RP(victim))
  	{
		send_to_char("Both you and your victim must be completely IC to do this.\n\r",ch);
		return;
  	}
  }
  
  WAIT_STATE( ch, skill_table[gsn_steal].beats );
  percent  = number_percent();
  
  if (!IS_AWAKE(victim))
    percent -= 10;
  else if (!can_see(victim,ch))
    percent += 25;
  else 
    percent += 50;

  if (ch->class == CLASS_THIEF)
    percent -= 15;
  else
    percent += 40;
  
/* Level is calced into percent chance now */
  level_range = (victim->level - ch->level)/2;
  percent += level_range;
  

/*    if ( ((ch->level + 7 < victim->level || ch->level -7 > victim->level) 
    && !IS_NPC(victim) && !IS_NPC(ch) )
    || ( !IS_NPC(ch) && percent > get_skill(ch,gsn_steal))
    || ( !IS_NPC(ch) && !is_clan(ch)) )
*/

  if (!IS_NPC(ch) && percent > get_skill(ch,gsn_steal)) {
    /*
	* Failure.
	*/
    send_to_char( "Oops.\n\r", ch );
    check_improve(ch,gsn_steal,FALSE,2);
    affect_strip(ch,gsn_sneak);
    REMOVE_BIT(ch->affected_by,AFF_SNEAK);
    
    act( "$n tried to steal from you.\n\r", ch, NULL, victim, TO_VICT    );
    act( "$n tried to steal from $N.\n\r",  ch, NULL, victim, TO_NOTVICT );
    switch(number_range(0,3)) {
    case 0 :
	 sprintf( buf, "%s is a lousy thief!", PERS(ch, victim));
	 break;
    case 1 :
	 sprintf( buf, "%s couldn't rob %s way out of a paper bag!",
			PERS(ch, victim),(ch->sex == 2) ? "her" : "his");
	 break;
    case 2 :
	 sprintf( buf,"%s tried to rob me!", PERS(ch, victim) );
	 break;
    case 3 :
	 sprintf(buf,"Keep your hands out of there, %s!",PERS(ch, victim));
	 break;
    }

    if (!IS_AWAKE(victim))
	 do_function(victim, &do_wake, "");

    if (IS_AWAKE(victim))
	 do_function(victim, &do_yell, buf );

    if ( !IS_NPC(ch) ) {
	 if ( IS_NPC(victim) ) {
	   check_improve(ch,gsn_steal,FALSE,2);
	   multi_hit( victim, ch, TYPE_UNDEFINED );
	 }
	 else {
	   sprintf(buf,"$N tried to steal from %s.",victim->name);
	   wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
/*
		if ( !IS_SET(ch->act, PLR_THIEF) )
		{
		    SET_BIT(ch->act, PLR_THIEF);
		    send_to_char( "*** You are now a THIEF!! ***\n\r", ch );
		    save_char_obj( ch, FALSE );
		}
*/
	 }
    }
    
    return;
  }
  
  if ( !str_cmp( arg1, "coin"  )
	  ||   !str_cmp( arg1, "coins" )
	  ||   !str_cmp( arg1, "gold"  ) 
	  ||	 !str_cmp( arg1, "silver")) {
    int gold, silver;
    
    gold = victim->gold * number_range(1, ch->level) / MAX_LEVEL;
    silver = victim->silver * number_range(1,ch->level) / MAX_LEVEL;

    if ( gold <= 0 && silver <= 0 ) {
	 send_to_char( "You couldn't get any coins.\n\r", ch );
	 check_improve(ch,gsn_steal,FALSE,2);
	 return;
    }
    
    ch->gold     	+= gold;
    ch->silver   	+= silver;
    victim->silver 	-= silver;
    victim->gold 	-= gold;
    if (silver <= 0) {
	 sprintf( buf, "Bingo!  You got %d gold coins.\n\r", gold );
	 sprintf(log_buf, "%s stole %d gold coins from %s", !IS_NPC(ch) ? ch->name : ch->short_descr, gold, !IS_NPC(victim) ? victim->name : victim->short_descr);
    }
    else if (gold <= 0) {
	 sprintf( buf, "Bingo!  You got %d silver coins.\n\r",silver);
	 sprintf(log_buf, "%s stole %d silver coins from %s", !IS_NPC(ch) ? ch->name : ch->short_descr, silver, !IS_NPC(victim) ? victim->name : victim->short_descr);
    }
    else {
	 sprintf(buf, "Bingo!  You got %d silver and %d gold coins.\n\r",
		    silver,gold);
	 sprintf(log_buf, "%s stole %d gold and %d silver coins from %s", !IS_NPC(ch) ? ch->name : ch->short_descr, gold, silver, !IS_NPC(victim) ? victim->name : victim->short_descr);
    }
    
    send_to_char( buf, ch );
    check_improve(ch,gsn_steal,TRUE,2);

    // Log it
    wiznet(log_buf,ch,NULL,WIZ_FLAGS,0,0);
    log_string(log_buf);

    return;
  }

  if (( obj = get_obj_carry( victim, arg1, ch ) ) == NULL ) {
    send_to_char( "You can't find it.\n\r", ch );
    return;
  }
  
  if (IS_SET(obj->extra_flags, ITEM_NOSTEAL) ) {
    act( "You can't quite get $p.", ch, obj, NULL, TO_CHAR );
    return;
  }
  
  if ( !can_drop_obj( ch, obj )
	  ||   IS_SET(obj->extra_flags, ITEM_INVENTORY)
	  ||   obj->level > ch->level ) {
    send_to_char( "You can't pry it away.\n\r", ch );
    return;
  }
  
  if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) ) {
    send_to_char( "You have your hands full.\n\r", ch );
    return;
  }
  
  if ( ch->carry_weight + get_obj_weight( obj ) > can_carry_w( ch ) )  {
    send_to_char( "You can't carry that much weight.\n\r", ch );
    return;
  }
  
  obj_from_char( obj );
  obj_to_char( obj, ch );
  act("You pocket $p.",ch,obj,NULL,TO_CHAR);
  check_improve(ch,gsn_steal,TRUE,2);
  send_to_char( "Got it!\n\r", ch );

  // Log it
  sprintf(log_buf, "%s stole %s [vnum=%d] from %s", !IS_NPC(ch) ? ch->name : ch->short_descr, obj->short_descr, obj->pIndexData->vnum, !IS_NPC(victim) ? victim->name : victim->short_descr);
  
  wiznet(log_buf,ch,NULL,WIZ_FLAGS,0,0);
  log_string(log_buf);
  
  return;
}



/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper( CHAR_DATA *ch )
{
    /*char buf[MAX_STRING_LENGTH];*/
    CHAR_DATA *keeper;
    SHOP_DATA *pShop;

    pShop = NULL;
    for ( keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room )
    {
	if ( IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL )
	    break;
    }

    if ( pShop == NULL )
    {
	send_to_char( "You can't do that here.\n\r", ch );
	return NULL;
    }

    /*
     * Undesirables.
     *
    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_KILLER) )
    {
	do_function(keeper, &do_say, "Killers are not welcome!");
	sprintf(buf, "%s the KILLER is over here!\n\r", ch->name);
	do_function(keeper, &do_yell, buf );
	return NULL;
    }

    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_THIEF) )
    {
	do_function(keeper, &do_say, "Thieves are not welcome!");
	sprintf(buf, "%s the THIEF is over here!\n\r", ch->name);
	do_function(keeper, &do_yell, buf );
	return NULL;
    }
	*/
    /*
     * Shop hours.
     */
    if ( time_info.hour < pShop->open_hour )
    {
	do_function(keeper, &do_say, "Sorry, I am closed. Come back later.");
	return NULL;
    }
    
    if ( time_info.hour > pShop->close_hour )
    {
	do_function(keeper, &do_say, "Sorry, I am closed. Come back tomorrow.");
	return NULL;
    }

    /*
     * Invisible or hidden people.
     */
    if ( !can_see( keeper, ch ) )
    {
	do_function(keeper, &do_say, "I don't trade with folks I can't see.");
	return NULL;
    }

    return keeper;
}

/* insert an object at the right spot for the keeper */
void obj_to_keeper( OBJ_DATA *obj, CHAR_DATA *ch )
{
    OBJ_DATA *t_obj, *t_obj_next;

    /* see if any duplicates are found */
    for (t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj_next)
    {
	t_obj_next = t_obj->next_content;

	if (obj->pIndexData == t_obj->pIndexData 
	&&  !str_cmp(obj->short_descr,t_obj->short_descr))
	{
	    /* if this is an unlimited item, destroy the new one */
	    if (IS_OBJ_STAT(t_obj,ITEM_INVENTORY))
	    {
		extract_obj(obj);
		return;
	    }
	    obj->cost = t_obj->cost; /* keep it standard */
	    break;
	}
    }

    if (t_obj == NULL)
    {
	obj->next_content = ch->carrying;
	ch->carrying = obj;
    }
    else
    {
	obj->next_content = t_obj->next_content;
	t_obj->next_content = obj;
    }

    obj->carried_by      = ch;
    obj->in_room         = NULL;
    obj->in_obj          = NULL;
    ch->carry_number    += get_obj_number( obj );
    ch->carry_weight    += get_obj_weight( obj );
}

/* get an object from a shopkeeper's list */
OBJ_DATA *get_obj_keeper( CHAR_DATA *ch, CHAR_DATA *keeper, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;
 
    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = keeper->carrying; obj != NULL; obj = obj->next_content )
    {
        if (obj->wear_loc == WEAR_NONE
        &&  can_see_obj( keeper, obj )
	&&  can_see_obj(ch,obj)
        &&  is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;
	
	    /* skip other objects of the same name */
	    while (obj->next_content != NULL
	    && obj->pIndexData == obj->next_content->pIndexData
	    && !str_cmp(obj->short_descr,obj->next_content->short_descr))
		obj = obj->next_content;
        }
    }
 
    return NULL;
}

int get_cost( CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy )
{
    SHOP_DATA *pShop;
    int cost;

    if ( obj == NULL || ( pShop = keeper->pIndexData->pShop ) == NULL )
	return 0;

    if ( fBuy )
    {
	cost = obj->cost * pShop->profit_buy  / 100;
    }
    else
    {
	OBJ_DATA *obj2;
	int itype;

	cost = 0;
	for ( itype = 0; itype < MAX_TRADE; itype++ )
	{
	    if ( obj->item_type == pShop->buy_type[itype] )
	    {
		cost = obj->cost * pShop->profit_sell / 100;
		break;
	    }
	}

	if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
	    for ( obj2 = keeper->carrying; obj2; obj2 = obj2->next_content )
	    {
	    	if ( obj->pIndexData == obj2->pIndexData
		&&   !str_cmp(obj->short_descr,obj2->short_descr) )
	 	{    if (IS_OBJ_STAT(obj2,ITEM_INVENTORY))
			cost /= 2;
		    else
                    	cost = cost * 3 / 4;
        	}
	    }
    }

    if ( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
    {
	if (obj->value[1] == 0)
	    cost /= 4;
	else
	    cost = cost * obj->value[2] / obj->value[1];
    }

    return cost;
}



void do_buy( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  int cost,roll;
  
  if (IS_NULLSTR(argument)) {
    send_to_char( "Buy what?\n\r", ch );
    return;
  }

  if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP)) {
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *pet;
    ROOM_INDEX_DATA *pRoomIndexNext;
    ROOM_INDEX_DATA *in_room;
   
    smash_tilde(argument);
    
    if ( IS_NPC(ch) )
	 return;

    argument = one_argument(argument,arg);

    /* hack to make new thalos pets work */
    if (ch->in_room->vnum == 9621)
	 pRoomIndexNext = get_room_index(9706);
    else
	 pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
    if ( pRoomIndexNext == NULL ) {
	 bug( "Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum );
	 send_to_char( "Sorry, you can't buy that here.\n\r", ch );
	 return;
    }

    in_room     = ch->in_room;
    ch->in_room = pRoomIndexNext;
    pet         = get_char_room( ch, arg );
    ch->in_room = in_room;
    
    if ( pet == NULL || !IS_SET(pet->act, ACT_PET) ) {
	 send_to_char( "Sorry, you can't buy that here.\n\r", ch );
	 return;
    }
    
    if (ch->pet != NULL) {
	 send_to_char( "You already have a pet.\n\r", ch );
	 return;
    }
    else if (ch->mount != NULL) {
	 send_to_char( "You already have a mount.\n\r", ch );
	 return;
    }
    
    if ( IS_SET(pet->act,ACT_RIDEABLE) && !MOUNTED(ch) ) {
 	 
	 if  (ch->pet != NULL) {
	   send_to_char( "You already have a mount.\n\r", ch );
	   return;
	 }

	 cost = 10 * pet->level * pet->level;
	 	 
	 if ( ch->level < pet->level) {
	   send_to_char("You're not powerful enough to master this pet.\n\r", ch );
	   return;
	 }
	 
	/*haggle*/
	
	 roll = number_percent();

	 if (roll < get_skill(ch,gsn_haggle)) {
	   cost -= cost / 2 * roll / 100;
	   sprintf(buf,"You haggle the price down to %d coins.\n\r",cost);
	   send_to_char(buf,ch);
	   check_improve(ch,gsn_haggle,TRUE,4);
	 }

	 if ( (ch->silver + 100 * ch->gold) < cost ) {
	   send_to_char( "You can't afford it.\n\r", ch );
	   return;
	 }

	 deduct_cost(ch,cost);
	 pet = create_mobile( pet->pIndexData );
	 pet->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
	 char_to_room( pet, ch->in_room );

         // Give the pet a name you want
	 if (!IS_NULLSTR(argument)) {
	    sprintf(buf, "%s %s", pet->name, argument);
	    if (pet->name)
	       free_string(pet->name);
	    pet->name = str_dup( buf );
	 }
	 
	 //ch->mount = pet; 
	 add_follower( pet, ch );
	 do_mount(ch, pet->name);
	 SET_BIT(pet->affected_by, AFF_CHARM);
	 //	 pet->master = ch;
	 pet->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
//	 ch->mount = pet; 
	 //	 add_follower( pet, ch );
	 pet->leader = ch;
	 if (!IS_NULLSTR(argument)) {
	    sprintf(buf, "You named your mount {y%s{x.\n\r", argument);
	    send_to_char(buf, ch);
	 }
	 send_to_char( "Enjoy your mount.\n\r", ch );
	 act( "$n bought $N as a mount.", ch, NULL, pet, TO_ROOM );
	 return;
    }
    
    if ( ch->pet != NULL ) {
	 send_to_char("You already own a pet.\n\r",ch);
	 return;
    }
    
    cost = 10 * pet->level * pet->level;
    
    
    if ( ch->level < pet->level ) {
	 send_to_char("You're not powerful enough to master this pet.\n\r", ch );
	 return;
    }
    
    /* haggle */
    roll = number_percent();
    if (roll < get_skill(ch,gsn_haggle)) {
	 cost -= cost / 2 * roll / 100;
	 sprintf(buf,"You haggle the price down to %d coins.\n\r",cost);
	 send_to_char(buf,ch);
	 check_improve(ch,gsn_haggle,TRUE,4);
	 
    }

    if ( (ch->silver + 100 * ch->gold) < cost ) {
	 send_to_char( "You can't afford it.\n\r", ch );
	 return;
    }
    
    deduct_cost(ch,cost);
    pet	= create_mobile( pet->pIndexData );
    SET_BIT(pet->act, ACT_PET);
    SET_BIT(pet->affected_by, AFF_CHARM);
    pet->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
    
    argument = one_argument( argument, arg );
    if ( !IS_NULLSTR(arg)) {
	 sprintf( buf, "%s %s", pet->name, arg );
	 free_string( pet->name );
	 pet->name = str_dup( buf );
    }

    //    sprintf( buf, "%sA neck tag says 'I belong to %s'.\n\r",
    //		   pet->description, ch->name );
    //    free_string( pet->description );
    //    pet->description = str_dup( buf );

    char_to_room( pet, ch->in_room );
    add_follower( pet, ch );
    pet->leader = ch;
    ch->pet = pet;
    if (!IS_NULLSTR(arg)) {
       sprintf(buf, "You named your pet {y%s{x.\n\r", arg);
       send_to_char(buf, ch);
    }
    send_to_char( "Enjoy your pet.\n\r", ch );	
    act( "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM );
    return;
  }
  else {
    CHAR_DATA *keeper;
    OBJ_DATA *obj,*t_obj;
    char arg[MAX_INPUT_LENGTH];
    int number, count = 1;
    
    if ( ( keeper = find_keeper( ch ) ) == NULL )
	 return;
    
    number = mult_argument(argument,&arg[0]);
    obj    = get_obj_keeper( ch,keeper, arg );
    cost   = get_cost( keeper, obj, TRUE );


    if (number < 1 || number > 99) {
	 act("$n tells you 'Get real!",keeper,NULL,ch,TO_VICT);
	 return;
    }

    if ((cost <= 0) || !can_see_obj(ch, obj)) {
    /* if (!can_see_obj(ch, obj) || (cost <= 0)) { */
	 act( "$n tells you '{7I don't sell that -- try 'list'.{x'",
		 keeper, NULL, ch, TO_VICT );
	 //ch->reply = keeper;
	 return;
    }

    if (!IS_OBJ_STAT(obj,ITEM_INVENTORY)) {
	 for (t_obj = obj->next_content;
		 count < number && t_obj != NULL; 
		 t_obj = t_obj->next_content) {
	   if (t_obj->pIndexData == obj->pIndexData
		  &&  !str_cmp(t_obj->short_descr,obj->short_descr))
		count++;
	   else
		break;
	 }
	 
	 if (count < number) {
	   act("$n tells you '{7I don't have that many in stock.{x'",
		  keeper,NULL,ch,TO_VICT);
	   //ch->reply = keeper;
	   return;
	 }
    }
        
    if ( obj->level > ch->level ) {
	 act( "$n tells you '{7You can't use $p yet.{x'",
		 keeper, obj, ch, TO_VICT );
	 //ch->reply = keeper;
	 return;
    }
    
    if (ch->carry_number +  number * get_obj_number(obj) > can_carry_n(ch)) {
	 send_to_char( "You can't carry that many items.\n\r", ch );
	 return;
    }
    
    if ( ch->carry_weight + number * get_obj_weight(obj) > can_carry_w(ch)) {
	 send_to_char( "You can't carry that much weight.\n\r", ch );
	 return;
    }
    
    /* haggle */
    roll = number_percent();
    if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT) 
	   && roll < get_skill(ch,gsn_haggle)) {
	 cost -= obj->cost / 2 * roll / 100;
	 act("You haggle with $N.",ch,NULL,keeper,TO_CHAR);
	 check_improve(ch,gsn_haggle,TRUE,4);
    }

    if ((ch->silver + ch->gold * 100) < cost * number ) {
	 if (number > 1)
	   act("$n tells you '{7You can't afford to buy that many.{x'", keeper,obj,ch,TO_VICT);
	 else
	   act( "$n tells you '{7You can't afford to buy $p.{x'", keeper, obj, ch, TO_VICT );
	 return;
    }
    
    if (number > 1) {
	 sprintf(buf,"$n buys $p[%d].",number);
	 act(buf,ch,obj,NULL,TO_ROOM);
	 sprintf(buf,"You buy $p[%d] for %d silver.",number,cost * number);
	 act(buf,ch,obj,NULL,TO_CHAR);
    }
    else {
	 act( "$n buys $p.", ch, obj, NULL, TO_ROOM );
	 sprintf(buf,"You buy $p for %d silver.",cost);
	 act( buf, ch, obj, NULL, TO_CHAR );
    }

    deduct_cost(ch,cost * number);
    keeper->gold += cost * number/100;
    keeper->silver += cost * number - (cost * number/100) * 100;
    
    for (count = 0; count < number; count++) {
	 if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
	   t_obj = create_object( obj->pIndexData, obj->level );
	 else {
	   t_obj = obj;
	   obj = obj->next_content;
	   obj_from_char( t_obj );
	 }
	 
	 if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj,ITEM_HAD_TIMER))
	   t_obj->timer = 0;
	 REMOVE_BIT(t_obj->extra_flags,ITEM_HAD_TIMER);
	 obj_to_char( t_obj, ch );
	 if (cost < t_obj->cost)
	   t_obj->cost = cost;
    }
  }
}

void do_list( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  
  if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) ) {
    ROOM_INDEX_DATA *pRoomIndexNext;
    CHAR_DATA *pet;
    bool found;
    
    /* hack to make new thalos pets work */
    if (ch->in_room->vnum == 9621)
	 pRoomIndexNext = get_room_index(9706);
    else
	 pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
    
    if ( pRoomIndexNext == NULL ) {
	 bug( "Do_list: bad pet shop at vnum %d.", ch->in_room->vnum );
	 send_to_char( "You can't do that here.\n\r", ch );
	 return;
    }
    
    found = FALSE;
    for ( pet = pRoomIndexNext->people; pet; pet = pet->next_in_room ) {
	 if ( IS_SET(pet->act, ACT_PET) ) {
	   if ( !found ) {
		found = TRUE;
		send_to_char( "Pets for sale:\n\r", ch );
	   }
	   sprintf( buf, "[%2d] %8d - %s\n\r",
			  pet->level,
			  10 * pet->level * pet->level,
			  pet->short_descr );
	   send_to_char( buf, ch );
	 }
    }
    if ( !found )
	 send_to_char( "Sorry, we're out of pets right now.\n\r", ch );
    return;
  }
  else {
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost,count;
    bool found;
    int chance;
    char arg[MAX_INPUT_LENGTH];
    
    if ( ( keeper = find_keeper( ch ) ) == NULL )
	 return;

    argument = one_argument(argument,arg);

    if (!str_cmp(arg, "lore")) {

	 if (IS_NULLSTR(argument)) {
	   sprintf(buf, "What item you want to study more closely?");
	   do_say(keeper, buf);
	   return;
	 }
	 
	 found = FALSE;
	 
	 for ( obj = keeper->carrying; obj; obj = obj->next_content ) {
	   if ( obj->wear_loc == WEAR_NONE
		   &&   can_see_obj( ch, obj )
		   &&   ( cost = get_cost( keeper, obj, TRUE ) ) > 0 
		   &&  is_name(argument,obj->name)) {

		found = TRUE;
		
		act("$N gives you $p so you can study it more closely.", ch, obj, keeper, TO_CHAR);
		act("$N gives $n $p so $e can study it more closely.", ch, obj, keeper, TO_ROOM);
		
		if ( (chance = get_skill(ch,gsn_lore)) == 0 && (ch->level < skill_table[gsn_lore].skill_level[ch->class])) {	
		  send_to_char("It's a rock, it looks very old.\n\r",ch);
		  
		  act("You give $p back to $N.", ch, obj, keeper, TO_CHAR);
		  act("$n gives $p back to $N.", ch, obj, keeper, TO_ROOM);
		  
		  return;
		}
		
		if (ch->endurance<(obj->level*2)) {
		  send_to_char("You just can't think straight, maybe you should rest!\n\r",ch);
		  
		  act("You give $p back to $N.", ch, obj, keeper, TO_CHAR);
		  act("$n gives $p back to $N.", ch, obj, keeper, TO_ROOM);
		  return;
		}
		
		ch->endurance-=(2*obj->level);	
		if (number_percent() < chance) {
		  send_to_char("You know what this is!\n\r",ch);
		  check_improve( ch, gsn_lore, TRUE, 1 );	
		}
		else {
		  send_to_char("You just can't figure it out!\n\r",ch);
		  check_improve( ch, gsn_lore, FALSE, 1 );	
		  return;
		}
		
		lore_item(ch, obj);
		
		act("You give $p back to $N after studying it.", ch, obj, keeper, TO_CHAR);
		act("$n gives $p back to $N after studying it.", ch, obj, keeper, TO_ROOM);
		return;
	   }
	 }
	 if (!found) {
	   sprintf(buf, "I don't have that item!");
	   do_say(keeper, buf);
	   return;
	 }
    }
    
    found = FALSE;
    for ( obj = keeper->carrying; obj; obj = obj->next_content ) {
	 if ( obj->wear_loc == WEAR_NONE
		 &&   can_see_obj( ch, obj )
		 &&   ( cost = get_cost( keeper, obj, TRUE ) ) > 0 
		 &&   ( arg[0] == '\0'  
			   ||  is_name(arg,obj->name) )) {

	   if ( !found ) {
		found = TRUE;
		send_to_char( "[Lv Price Qty] Item\n\r", ch );
	   }
	   
	   if (IS_OBJ_STAT(obj,ITEM_INVENTORY))
		sprintf(buf,"[%2d %5d -- ] %s\n\r",
			   obj->level,cost,obj->short_descr);
	   else {
		count = 1;
		
		while (obj->next_content != NULL 
			  && obj->pIndexData == obj->next_content->pIndexData
			  && !str_cmp(obj->short_descr,
					    obj->next_content->short_descr)) {
		  obj = obj->next_content;
		  count++;
		}
		sprintf(buf,"[%2d %5d %2d ] %s\n\r",
			   obj->level,cost,count,obj->short_descr);
	   }
	   send_to_char( buf, ch );
	 }
    }
    
    if ( !found )
	 send_to_char( "You can't buy anything here.\n\r", ch );
    return;
  }
}



void do_sell( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost,roll;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Sell what?\n\r", ch );
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	act( "$n tells you 'You don't have that item'.",
	    keeper, NULL, ch, TO_VICT );
	//ch->reply = keeper;
	return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if (!can_see_obj(keeper,obj))
    {
	act("$n doesn't see what you are offering.",keeper,NULL,ch,TO_VICT);
	return;
    }

    if (IS_OBJ_STAT(obj,ITEM_BROKEN)) {
	 act("$n spits on the broken $p in disgust.",keeper, obj, ch,TO_VICT);
	 return;
    }

    if ( ( cost = get_cost( keeper, obj, FALSE ) ) <= 0 )
    {
	act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
	return;
    }
    if ( cost > (keeper-> silver + 100 * keeper->gold) )
    {
	act("$n tells you 'I'm afraid I don't have enough wealth to buy $p.",
	    keeper,obj,ch,TO_VICT);
	return;
    }

    act( "$n sells $p.", ch, obj, NULL, TO_ROOM );
    /* haggle */
    roll = number_percent();
    if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT) && roll < get_skill(ch,gsn_haggle))
    {
        send_to_char("You haggle with the shopkeeper.\n\r",ch);
        cost += obj->cost / 2 * roll / 100;
//        cost = UMAX(cost,95 * get_cost(keeper,obj,TRUE) / 100);
//	cost = UMAX(cost,(keeper->silver + 100 * keeper->gold));
        check_improve(ch,gsn_haggle,TRUE,4);
    }
    sprintf( buf, "You sell $p for %d silver and %d gold piece%s.",
	cost - (cost/100) * 100, cost/100, cost == 1 ? "" : "s" );
    act( buf, ch, obj, NULL, TO_CHAR );
    ch->gold     += cost/100;
    ch->silver 	 += cost - (cost/100) * 100;
    deduct_cost(keeper,cost);
    if ( keeper->gold < 0 )
	keeper->gold = 0;
    if ( keeper->silver< 0)
	keeper->silver = 0;

    if ( obj->item_type == ITEM_TRASH || IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
    {
	extract_obj( obj );
    }
    else
    {
	obj_from_char( obj );
	if (obj->timer)
	    SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
	else
	    obj->timer = number_range(50,100);
	obj_to_keeper( obj, keeper );
    }

    return;
}



void do_value( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Value what?\n\r", ch );
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
	act( "$n tells you 'You don't have that item'.",
	    keeper, NULL, ch, TO_VICT );
	//ch->reply = keeper;
	return;
    }

    if (!can_see_obj(keeper,obj))
    {
        act("$n doesn't see what you are offering.",keeper,NULL,ch,TO_VICT);
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if ( ( cost = get_cost( keeper, obj, FALSE ) ) <= 0 )
    {
	act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
	return;
    }

    sprintf( buf, 
	"$n tells you 'I'll give you %d silver and %d gold coins for $p'.", 
	cost - (cost/100) * 100, cost/100 );
    act( buf, keeper, obj, ch, TO_VICT );
    //ch->reply = keeper;

    return;
}

void do_herbs(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *victim;
  char arg[MAX_INPUT_LENGTH];
  
  one_argument(argument,arg);
  
  if (IS_NPC(ch))
    return;

  if (is_affected(ch,gsn_herbs))
    {
      send_to_char("You can't find any more herbs.\n\r",ch);
      return;
    }

  if (arg[0] == '\0')
    victim = ch;
  else if ( (victim = get_char_room(ch,arg)) == NULL)
    {
      send_to_char("They're not here.\n\r",ch);
      return;
    }
  WAIT_STATE( ch, skill_table[gsn_herbs].beats );

  if (ch->in_room->sector_type != SECT_INSIDE && 
      ch->in_room->sector_type != SECT_CITY && 
      (IS_NPC(ch) || number_percent() < get_skill(ch,gsn_herbs)))
    {
      AFFECT_DATA af;
      af.where  = TO_AFFECTS;
	 af.casterId  = ch->id;
      af.type 	= gsn_herbs;
      af.level 	= ch->level;
      af.duration = 5;
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bitvector = 0;

      affect_to_char(ch,&af);

      send_to_char("You gather some beneficial herbs.\n\r",ch);
      act("$n gathers some herbs.",ch,NULL,NULL,TO_ROOM);
      
      if (ch != victim)
	{
	  act("$n gives you some herbs to eat.",ch,NULL,victim,TO_VICT);
	  act("You give the herbs to $N.",ch,NULL,victim,TO_CHAR);
	  act("$n gives the herbs to $N.",ch,NULL,victim,TO_NOTVICT);
	}
	      
      if (victim->hit < victim->max_hit)
	{
	  send_to_char("You feel better.\n\r",victim);
	  act("$n looks better.",victim,NULL,NULL,TO_ROOM);
	}
      victim->hit = UMIN(victim->max_hit,victim->hit + 5 * ch->level);
      check_improve(ch,gsn_herbs,TRUE,1);
      if (is_affected(victim, gsn_plague))
	{
	  if (check_dispel(ch->level,victim,gsn_plague))
	    { 
	      send_to_char("Your sores vanish.\n\r",victim);
	      act("$n looks relieved as $s sores vanish.",victim,NULL,NULL,TO_ROOM);
	    }
	}
    }
  else
    {
      send_to_char("You search for herbs but find none here.\n\r",ch);
      act("$n looks around for herbs.",ch,NULL,NULL,TO_ROOM);
      check_improve(ch,gsn_herbs,FALSE,1);
    }
}


void do_butcher(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_STRING_LENGTH];
  OBJ_DATA *tmp_obj;
  OBJ_DATA *tmp_next;

  if (IS_NPC(ch))
    return;

  one_argument(argument,arg);
  if ( arg[0]=='\0' )
  {
    send_to_char( "Butcher what?\n\r", ch );
    return;
  }
  if ( (obj = get_obj_here(ch,arg)) == NULL )
    {
      send_to_char("You do not see that here.\n\r",ch);
      return;
    }

  if (obj->item_type != ITEM_CORPSE_PC && obj->item_type != ITEM_CORPSE_NPC)
    {
      send_to_char("You can't butcher that.\n\r",ch);
      return;
    }

  if (obj->carried_by != NULL)
    {
      send_to_char("Put it down first.\n\r",ch);
      return;
    }

  if ( !IS_NPC(ch) && get_skill(ch,gsn_butcher) < 1)
    {
      send_to_char("You don't have the precision instruments for that.", ch);
      return;
    }

  obj_from_room(obj);
  
  for (tmp_obj = obj->contains;tmp_obj != NULL;
       tmp_obj = tmp_next)
    {
      tmp_next = tmp_obj->next_content;
      obj_from_obj(tmp_obj);
      obj_to_room(tmp_obj,ch->in_room);
    }
  

  if (IS_NPC(ch) || number_percent() < get_skill(ch,gsn_butcher))
    {
      int numsteaks;
      int i;
      OBJ_DATA *steak;
        
      numsteaks = number_bits(2) + 1; 
      
      if (numsteaks > 1)
	{
	  sprintf(buf, "$n butchers $p and creates %i steaks.",numsteaks);
	  act(buf,ch,obj,NULL,TO_ROOM);

	  sprintf(buf, "You butcher $p and create %i steaks.",numsteaks);
	  act(buf,ch,obj,NULL,TO_CHAR);
	}

      else 
	{
	  act("$n butchers $p and creates a steak."
	      ,ch,obj,NULL,TO_ROOM);

	  act("You butcher $p and create a steak."
	      ,ch,obj,NULL,TO_CHAR);
	}
      check_improve(ch,gsn_butcher,TRUE,1);

      for (i=0; i < numsteaks; i++)
	{
	  steak = create_object(get_obj_index(OBJ_VNUM_STEAK),0);
	  sprintf( buf, steak->short_descr, obj->short_descr);
	  free_string( steak->short_descr );
	  steak->short_descr = str_dup( buf );

	  sprintf( buf, steak->description, obj->short_descr );
	  free_string( steak->description );
	  steak->description = str_dup( buf );

	  obj_to_room(steak,ch->in_room);
	}
    }	
  else 
    {
      act("You fail and destroy $p.",ch,obj,NULL,TO_CHAR);
      act("$n fails to butcher $p and destroys it.",
	  ch,obj,NULL,TO_ROOM);

      check_improve(ch,gsn_butcher,FALSE,1);
    }
  extract_obj(obj);
}

int get_second_sn(CHAR_DATA *ch)
{
    OBJ_DATA *wield;
    int sn;

    wield = get_eq_char( ch, WEAR_SECOND_WIELD );
    if (wield == NULL || wield->item_type != ITEM_WEAPON)
        sn = 0;
    else switch (wield->value[0])
    {
    default :               sn = 0;                break;
    case(WEAPON_SWORD):     sn = gsn_sword;         break;
    case(WEAPON_DAGGER):    sn = gsn_dagger;        break;
    case(WEAPON_SPEAR):     sn = gsn_spear;         break;
    case(WEAPON_STAFF):     sn = gsn_staff;         break;
    case(WEAPON_MACE):      sn = gsn_mace;          break;
    case(WEAPON_AXE):       sn = gsn_axe;           break;
    case(WEAPON_FLAIL):     sn = gsn_flail;         break;
    case(WEAPON_WHIP):      sn = gsn_whip;          break;
    case(WEAPON_POLEARM):   sn = gsn_polearm;       break;
    case(WEAPON_BOW):   	sn = gsn_bow;       break;
    case(WEAPON_ARROW):   	sn = gsn_arrow;       break;
    case(WEAPON_LANCE):   	sn = gsn_lance;       break;
   }
   return sn;
}
/* wear object as a secondary weapon */

void do_dual_wield (CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;
  OBJ_DATA *obj_wield;
  char buf[MAX_STRING_LENGTH]; 
  int sn,skill;
    
  if ( !IS_NPC(ch) &&   ch->level < skill_table[gsn_dual_wield].skill_level[ch->class] ) {
    send_to_char("You don't know how to dual wield.\n\r", ch );
    return;
  }

  if (IS_SET(ch->flaws, FLAW_ONEARM)) {
    send_to_char("You only have one arm to use.\n\r", ch);
    return;
  }
  
  if (argument[0] == '\0')  {
    send_to_char ("Wear which weapon in your off-hand?\n\r",ch);
    return;
  }
  
  obj = get_obj_carry (ch, argument, ch); 
  
  if (obj == NULL) {
    send_to_char ("You don't have that item.\n\r",ch);
    return;
  }

  if (!CAN_WEAR( obj, ITEM_WIELD ) ) {
    send_to_char( "You can't wield that as your second weapon.\n\r",ch);
    return;
  }
  
  if ( (get_eq_char (ch,WEAR_SHIELD) != NULL) ||
	  (get_eq_char (ch,WEAR_HOLD)   != NULL) ) {
    send_to_char ("You cannot use a secondary weapon while using a shield or holding an item\n\r",ch);
    return;
  }
  

//  if ( ch->level < (obj->level - 3) ) {
   if ( get_level(ch) < obj->level ) {
    sprintf( buf, "You must be level %d to use this object.\n\r",
		   obj->level );
    send_to_char( buf, ch );
    act( "$n tries to use $p, but is too inexperienced.",
	    ch, obj, NULL, TO_ROOM );
    return;
  }

  if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)) {
    send_to_char ("It mustn't be a two-handed weapon!\n\r",ch);
    return;
  }
  
  if (get_eq_char (ch, WEAR_WIELD) == NULL) {
    send_to_char ("You need to wield a primary weapon, before using a secondary one!\n\r",ch);
    return;
  }
  
  if (get_obj_weight(obj) > (str_app[get_curr_stat(ch,STAT_STR)].wield * 5)) {
    send_to_char( "This weapon is too heavy to be used as a secondary weapon by you.\n\r", ch );
    return;
  }

  if ((obj_wield = get_eq_char(ch, WEAR_WIELD)) != NULL) {
     if (IS_WEAPON_STAT(obj_wield,WEAPON_TWO_HANDS) && ((!IS_NPC(ch) && ch->size < SIZE_LARGE && get_eq_char(ch,WEAR_SHIELD) != NULL) || get_eq_char(ch,WEAR_WIELD) !=NULL ) ) {
        send_to_char("You can't dual wield with a two-handed weapon wielded.\n\r",ch);
        return;
     }
  }
  
  if (!remove_obj(ch, WEAR_SECOND_WIELD, TRUE)) 
    return;                                

  act ("$n wields $p in $s off-hand.",ch,obj,NULL,TO_ROOM);
  act ("You wield $p in your off-hand.",ch,obj,NULL,TO_CHAR);
  equip_char ( ch, obj, WEAR_SECOND_WIELD);
  sn = get_second_sn(ch);
  
  if  (sn) {
    skill = get_weapon_skill(ch,sn);
    
    if (skill >= 100)
	 act("$p feels like a part of you!",ch,obj,NULL,TO_CHAR);
    else if (skill > 85)
	 act("You feel quite confident with $p.",ch,obj,NULL,TO_CHAR);
    else if (skill > 70)
	 act("You are skilled with $p.",ch,obj,NULL,TO_CHAR);
    else if (skill > 50)
	 act("Your skill with $p is adequate.",ch,obj,NULL,TO_CHAR);
    else if (skill > 25)
	 act("$p feels a little clumsy in your hands.",ch,obj,NULL,TO_CHAR);
    else if (skill > 1)
	 act("You fumble and almost drop $p.",ch,obj,NULL,TO_CHAR);
    else
	 act("You don't even know which end is up on $p.",
		ch,obj,NULL,TO_CHAR);
  }  
  return;  
}

bool can_restring_angreal(CHAR_DATA *ch)
{
  if (IS_NPC(ch))
    return FALSE;
  
  if (!IS_SET(ch->talents, TALENT_CREATE_ANGREAL))
    return FALSE;
  
  if (ch->pcdata->learned[gsn_create_angreal] < 90)
    return FALSE;
  
  return TRUE;    
}

void do_restring( CHAR_DATA *ch, char *argument )
{
  char buf [MAX_INPUT_LENGTH];
  char type [MAX_INPUT_LENGTH];
  char arg1 [MAX_INPUT_LENGTH];
  char arg2 [MAX_INPUT_LENGTH];
  CHAR_DATA *mob=NULL;
  int cost = 1000;
  int worth = 0;
  OBJ_DATA *obj;

  if (!IS_FORSAKEN(ch)) {
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room ) {
	 if ( IS_NPC(mob) && IS_SET(mob->act, ACT_IS_STRINGER) )
	   break;
    }
    
    if ( mob == NULL ) {
	 send_to_char( "You can't do that here.\n\r", ch );
	 return;
    }
  } 

  smash_tilde( argument );
  argument = one_argument( argument, type );
  argument = one_argument( argument, arg1 );
  memset(arg2, 0x00, sizeof(arg2));
  strcpy( arg2, argument );
  
  if ( type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' ) {
    send_to_char("Syntax:\n\r",ch);
    send_to_char("  restring <name> <field> <string>\n\r",ch);
    send_to_char("    fields: name short long \n\r",ch);
    return;
  }
  
  /* string an obj */  
  if ( ( obj = get_obj_carry( ch, type, ch ) ) == NULL ) {
    if (!IS_FORSAKEN(ch)) {
      act("$N tells you, '{7You don't have that item.{x'", ch, NULL, mob, TO_CHAR);
    }
    else  {  
	 send_to_char("You aren't carrying that.\n\r",ch);
    }
    return;
  }
  
  /* Possible to add check for sex and report to WT if MC */
/*
  if (!IS_FORSAKEN(ch)) {
    if (obj->item_type == ITEM_ANGREAL && !can_restring_angreal(ch)) {
	 sprintf(buf, "%s takes %s and study it for a moment shaking his head slowely as he says, '{7I can't help you with this!{x'", PERS(mob, ch), obj->short_descr);
	 send_to_char(buf, ch);
	 return;
    }
  } 
*/
  
  if (!IS_FORSAKEN(ch)) {
    cost = (obj->level * 5);
    worth = ch->silver + ch->gold * 100;
    if (cost > worth) {
	 act("$N tells you, '{7You don't have enough money for my services.{x'", ch, NULL, mob, TO_CHAR);
	 sprintf(buf, "$N look at you then to the %s and says, '{7It will cost you %d silver for me to do any work on %s{x'", obj->short_descr, cost, obj->short_descr);
	 act(buf, ch, NULL, mob, TO_CHAR);
	 return;
    }
  } 
  
  if ( !str_prefix( arg1, "name" ) ) {
    free_string( obj->name );
    obj->name = str_dup( colorstrem( arg2 ) );
  }

  else if ( !str_prefix( arg1, "short" ) ) {
    sprintf(buf, "%s{x", arg2); // Make sure color is terminated
    
    free_string( obj->short_descr );
    obj->short_descr = str_dup( buf );
  }
  
  else if ( !str_prefix( arg1, "long" ) ) {
    sprintf(buf, "%s{x", arg2); // Make sure color is terminated
    
    free_string( obj->description );
    obj->description = str_dup( buf );
  }
  
  else {
    send_to_char("That's not a valid Field.\n\r",ch);
    return;
  }
  
  WAIT_STATE(ch,PULSE_VIOLENCE);
  
  if (!IS_FORSAKEN(ch)) {
    deduct_cost(ch,cost);
    mob->silver += cost;
    
    sprintf(buf, "$N takes $n's item, strings it, and returns it to $n.");
    act(buf,ch,NULL,mob,TO_ROOM);
    sprintf(buf, "%s takes your item, strings it, and returns '%s' to you.\n\r", PERS(mob, ch), obj->short_descr);
    send_to_char(buf, ch);
  }
  send_to_char("Done! Make sure you understand every line in '{Whelp restring{x'.\n\r", ch);
  
  return;
}

void do_sheath( CHAR_DATA *ch, char *argument )
{
   //char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj_wield;
   OBJ_DATA *obj_dual;
   int scabbard;

   if ( (obj_wield = get_eq_char(ch, WEAR_WIELD)) == NULL) {
      send_to_char("You are not wielding a weapon.\n\r", ch);
      return;
   }

    if (obj_wield->pIndexData->vnum == OBJ_VNUM_BOF) {
	act( "You can't sheath $p.", ch, obj_wield, NULL, TO_CHAR );
	return;
    }
    /*
    if ( IS_SET(obj_wield->extra_flags, ITEM_NOREMOVE) ) {
	act( "You can't sheath $p.", ch, obj_wield, NULL, TO_CHAR );
	return;
    }
   */

   if ((get_eq_char(ch, WEAR_SCABBARD_1)) == NULL) {
      scabbard = WEAR_SCABBARD_1;
   }
   else if ((get_eq_char(ch, WEAR_SCABBARD_2)) == NULL) {
      scabbard = WEAR_SCABBARD_2;
   }
   else {
     act("You have no empty places left to sheath $p into.",ch,obj_wield,NULL,TO_CHAR);
     return;
   }

   // If blademaster
   if (get_skill(ch, gsn_blademaster) > 0 && obj_wield->value[0] == WEAPON_SWORD && IS_SET(ch->auto_act, AUTO_MASTERFORMS)) {
	act("You swing $p smoothly around from guard stance and sheath it, all in one fluid motion.", ch,obj_wield,NULL,TO_CHAR);
	act("$n swing $p smoothly around from guard stance and sheath it, all in one fluid motion.", ch,obj_wield,NULL,TO_ROOM);
   }
   // if Speardancer
   else if (get_skill(ch, gsn_speardancer) > 0 && obj_wield->value[0] == WEAPON_SPEAR && IS_SET(ch->auto_act, AUTO_MASTERFORMS)) {
	act("You twirl $p around in a flurry for a moment before you suddenly sheath it.", ch,obj_wield,NULL,TO_CHAR);
	act("$n twirls $p around in a flurry for a moment before $e suddenly sheath it.", ch,obj_wield,NULL,TO_ROOM);
   }
   // If Duelling
   else if (get_skill(ch, gsn_duelling) > 0 && obj_wield->value[0] == WEAPON_DAGGER && IS_SET(ch->auto_act, AUTO_MASTERFORMS)) {
	act("You twirl $p around very fast before you suddenly sheath it.", ch,obj_wield,NULL,TO_CHAR);
	act("$n twirls $p around very fast before $e suddenly sheath it.", ch,obj_wield,NULL,TO_ROOM);
   }   
   // All other
   else {
	act("You sheath $p.",ch,obj_wield,NULL,TO_CHAR);
	act("$n sheaths $p.",ch,obj_wield,NULL,TO_ROOM);
   }

   unequip_char(ch,obj_wield);
   obj_wield->wear_loc = scabbard;
      
   if ((obj_dual = get_eq_char(ch,WEAR_SECOND_WIELD)) != NULL) {
     unequip_char( ch, obj_dual);
     equip_char(ch,obj_dual,WEAR_WIELD);
     act("You switch $p into your good hand.", ch,obj_dual,NULL,TO_CHAR);
   }
   
   return;   
}

void do_draw( CHAR_DATA *ch, char *argument )
{
   OBJ_DATA *obj_s1;
   OBJ_DATA *obj_s2;
   OBJ_DATA *obj;
   OBJ_DATA *obj_wield;
   OBJ_DATA *obj_dual;
   int scabbard;
   int wear;
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];

   one_argument(argument,arg);

   // Get sheathed possible weapons
   obj_s1 = get_eq_char(ch, WEAR_SCABBARD_1);
   obj_s2 = get_eq_char(ch, WEAR_SCABBARD_2);

   // Check and set up defaults
   if (obj_s1 != NULL) {
      scabbard = WEAR_SCABBARD_1;
      obj = obj_s1;
   }
   else if (obj_s2 != NULL) {
      scabbard = WEAR_SCABBARD_2;
      obj = obj_s2;
   }
   else {
      send_to_char("You have no weapons sheathed to draw.\n\r", ch);
      return;	
   }

  /* If Bind */
  if (IS_AFFECTED(ch, AFF_BIND)) {
    send_to_char("You are tied up and unable to move!\n\r", ch);
    act("$n struggle against the ropes around $s body.", ch, NULL, NULL, TO_ROOM);
    return;
  }

  /* If wrapped */
  if (IS_AFFECTED(ch, AFF_WRAPPED)) {
    send_to_char("{WAn unseen force prevents you from moving!{x\n\r", ch);
    act("$n struggle against unseen bonds.", ch, NULL, NULL, TO_ROOM);
    return;
  }

   if (!IS_NULLSTR(arg)) {
      if ((obj_s1 != NULL) && is_name( arg, obj_s1->name)) {
      	 scabbard = WEAR_SCABBARD_1;
         obj = obj_s1;
      }
      else if ((obj_s2 != NULL) && is_name( arg, obj_s2->name)) {
      	scabbard = WEAR_SCABBARD_2;
         obj = obj_s2;
      }
      else {
      	sprintf(buf, "You don't have %s sheathed.\n\r", arg);
      	send_to_char(buf, ch);
      	return;
      }
   }

   // Check what the character is wielding
   if ((obj_wield = get_eq_char(ch, WEAR_WIELD)) == NULL) {
     wear = WEAR_WIELD;	
   }
   else if ((obj_dual  = get_eq_char(ch, WEAR_SECOND_WIELD)) == NULL) {
     wear = WEAR_SECOND_WIELD;
   }
   else {
      send_to_char("You have both arms full already.\n\r", ch);
      return;	
   }
   
   // Make sure the character know dual wield for this
   if (wear == WEAR_SECOND_WIELD) {
     if ( !IS_NPC(ch) &&  ch->level < skill_table[gsn_dual_wield].skill_level[ch->class] ) {
        send_to_char("You already is wielding a weapon.\n\r", ch);
        return;
     }
     
     if ((get_eq_char (ch, WEAR_SHIELD) != NULL) || (get_eq_char (ch, WEAR_HOLD)  != NULL)) { 
     	send_to_char("You cannot use a secondary weapon while using a shield or holding an item\n\r", ch);
     	return;
     }
     
     if (IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS) && ((!IS_NPC(ch) && ch->size < SIZE_LARGE && get_eq_char(ch,WEAR_SHIELD) != NULL) || get_eq_char(ch,WEAR_WIELD) !=NULL ) ) {
        send_to_char("You need two hands free before you can draw that weapon.\n\r",ch);
        return;
     }

     if (IS_WEAPON_STAT(obj_wield,WEAPON_TWO_HANDS) && ((!IS_NPC(ch) && ch->size < SIZE_LARGE && get_eq_char(ch,WEAR_SHIELD) != NULL) || get_eq_char(ch,WEAR_WIELD) !=NULL ) ) {
        send_to_char("You are already wielding a weapon with both your hands.\n\r",ch);
        return;
     }     
     
   }

   // If blademaster
   if (get_skill(ch, gsn_blademaster) > 0 && obj->value[0] == WEAPON_SWORD && IS_SET(ch->auto_act, AUTO_MASTERFORMS)) {
	act("You lean slightly forward and smoothly draw $p, all in one fluid motion.", ch,obj,NULL,TO_CHAR);
	act("$n leans slightly forward and smoothly draws $p, all in one fluid motion.", ch,obj,NULL,TO_ROOM);
   }
   else if (get_skill(ch, gsn_speardancer) > 0 && obj->value[0] == WEAPON_SPEAR && IS_SET(ch->auto_act, AUTO_MASTERFORMS)) {
	act("You jump into a backflip and draw $p as you spin around to your feet.", ch,obj,NULL,TO_CHAR);
	act("$n jumps into a backflip and draw $p as $e spin around to $s feet.", ch,obj,NULL,TO_ROOM);
   }
   else if (get_skill(ch, gsn_duelling) > 0 && obj->value[0] == WEAPON_DAGGER && IS_SET(ch->auto_act, AUTO_MASTERFORMS)) {
	act("You move your arm slightly and draw $p, twirling it around in your hand for a moment!", ch,obj,NULL,TO_CHAR);
	act("$n move $s arms slightly and draw $p, twirling it around in $s hand for a moment!.", ch,obj,NULL,TO_ROOM);
   }   
   else {
	act("You draw $p.",ch,obj,NULL,TO_CHAR);
	act("$n draws $p.",ch,obj,NULL,TO_ROOM);
   }
   
   obj->wear_loc = -1;
   equip_char(ch,obj,wear);
   
   return;
}

/**********************************************************************
*       Function      : do_repair
*       Author        : Swordfish
*       Description   : repair a broken weapon
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_repair(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *mob;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  int cost; int Nchant;
  int worth = 0;
  AFFECT_DATA *paf;

  for ( mob = ch->in_room->people; mob; mob = mob->next_in_room ) {
    if (IS_NPC(mob) && IS_SET(mob->act, ACT_REPAIRER) )
	 break;
  }

  if (mob == NULL) {
    send_to_char("You can't do that here.\n\r", ch );
    return;
  }
  
  one_argument(argument,arg);
  
  if (IS_NULLSTR(arg)) {
    act("$N says '{7I will repair a weapon or piece of armor for you, for a price..{x'", ch,NULL,mob,TO_CHAR);
    send_to_char(" Type '{Westimate <weapon>{x' to be assessed for damage.\n\r",ch);
    return;
  }

  if (!str_cmp(arg,"all"))
  {
   for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
      if (obj->condition < 100 )
      {
         if (obj->wear_loc > WEAR_NONE)
         {
            act("You remove $p.", ch, obj, mob, TO_CHAR);
            unequip_char(ch, obj);
         }
         Nchant = 0;
         for ( paf = obj->affected; paf != NULL; paf = paf->next ) {
            if (paf->location == APPLY_DAMROLL)
              Nchant = (paf->modifier);
         }

         cost = (100 - obj->condition) * 10;
         if (IS_SET(obj->extra_flags,ITEM_BROKEN))
               cost = cost * 2;

         worth = ch->silver + (ch->gold * 100);

         if (cost > worth) {
           act("$N says '{7You do not have enough money for my services.{x'", ch,NULL,mob,TO_CHAR);
           return;
         }

         WAIT_STATE(ch,PULSE_VIOLENCE);
    
         deduct_cost(ch, cost);
         mob->silver += cost;

         sprintf(buf, "$N takes %s from $n, repairs it, and returns it to $n.", obj->short_descr);
         act(buf,ch,NULL,mob,TO_ROOM);
         sprintf(buf, "%s takes %s, repairs it, and returns it.\n\r", mob->short_descr, obj->short_descr);
         send_to_char(buf, ch);

         REMOVE_BIT(obj->extra_flags, ITEM_BROKEN);

        // Can only make condition up to repaire's level
        obj->condition = 100; //get_level(mob);
        
      }
      return;
  }
  else
  {
     if (( obj = get_obj_carry(ch, arg, ch)) == NULL) {
       act("$N says '{7You don't have that item.{x'",ch,NULL,mob,TO_CHAR);
       return;
     }
     
     if (obj->condition >= 100) {
       act("$N says '{7But that item is not damaged.{x'",ch,NULL,mob,TO_CHAR);
       return;
     }
     
     Nchant = 0;
     for ( paf = obj->affected; paf != NULL; paf = paf->next ) {
       if (paf->location == APPLY_DAMROLL)
         Nchant = (paf->modifier);
     }
     
     
     cost = (100 - obj->condition) * 10;
     if (IS_SET(obj->extra_flags,ITEM_BROKEN))
	   cost = cost * 2;
   
     worth = ch->silver + (ch->gold * 100);
   
     if (cost > worth) {
       act("$N says '{7You do not have enough money for my services.{x'", ch,NULL,mob,TO_CHAR);
       return;
     }
   
     WAIT_STATE(ch,PULSE_VIOLENCE);
   
     deduct_cost(ch, cost);
     mob->silver += cost;
   
     sprintf(buf, "$N takes %s from $n, repairs it, and returns it to $n.", obj->short_descr); 
     act(buf,ch,NULL,mob,TO_ROOM);
     sprintf(buf, "%s takes %s, repairs it, and returns it.\n\r", mob->short_descr, obj->short_descr);
     send_to_char(buf, ch);
     
     REMOVE_BIT(obj->extra_flags, ITEM_BROKEN);
   
        // Can only make condition up to repaire's level
     obj->condition = 100; //get_level(mob);
   }
     
     return;
}

/**********************************************************************
*       Function      : do_estimate
*       Author        : Swordfish
*       Description   : estimate the cost to fix a broken weapon
*       Parameters    : 
*       Returns       : 
**********************************************************************/
void do_estimate(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  CHAR_DATA *mob; 
  char arg[MAX_INPUT_LENGTH];
  int cost; int Nchant;
  AFFECT_DATA *paf;
  
  for ( mob = ch->in_room->people; mob; mob = mob->next_in_room ) {
    if ( IS_NPC(mob) && IS_SET(mob->act, ACT_REPAIRER) )
	 break;
  }
 
  if ( mob == NULL ) {
    send_to_char( "You can't do that here.\n\r", ch );
    return;
  }
    
  one_argument(argument, arg);
  
  if (IS_NULLSTR(arg)) {
    act("$N says '{7Try estimate <item>.{x'",ch,NULL,mob,TO_CHAR);
    return; 
  } 
    
  if ((obj = (get_obj_carry(ch, arg, ch))) == NULL) {
    act("$N look at you for a moment then says '{7You don't have that item.{x'",ch,NULL,mob,TO_CHAR);
    return;
  }
  
  if (obj->condition >= 100) {
    act("$N says '{7But that item is not broken.{x'",ch,NULL,mob,TO_CHAR);
    return;
  }

/*
  if (obj->cost == 0) {
    act("$N says '{7I'm sorry, but that item is beyond repair. What did you do to it?{x'",ch,NULL,mob,TO_CHAR);
    return;
  } 
*/
    
  Nchant = 0;
  for ( paf = obj->affected; paf != NULL; paf = paf->next ) {
    if (paf->location == APPLY_DAMROLL)
      Nchant = (paf->modifier);
  }
    
  cost = (100 - obj->condition) * 10;
  if (IS_SET(obj->extra_flags,ITEM_BROKEN))
	cost = cost * 2;

  //cost = ((obj->level * 100) + (Nchant * 1000) + ((obj->cost * 2)/3));
  
  sprintf(buf, "$N says '{7It will cost {W%d Si{xl{Wver{7 to fix that item.{x'\n\r", cost);
  act(buf,ch,NULL,mob,TO_CHAR);
  
  return;
}

// Check if a object is good for donation
bool is_donation(OBJ_DATA *obj)
{
  // No broken items
  if (IS_OBJ_STAT(obj, ITEM_BROKEN))
    return FALSE;
  
  // Check types
  switch(obj->item_type) {
  case ITEM_CORPSE_NPC:
  case ITEM_CORPSE_PC:
    return FALSE;
    break;
  default:
    return TRUE;
    break;
  }
  
  return FALSE;
}

// Normal donation
void do_donate( CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *pit;
  OBJ_DATA *obj;
  ROOM_INDEX_DATA *original;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  
  argument = one_argument(argument, arg);
  
  if (IS_NULLSTR(arg)) {
    send_to_char("Donate what?\n\r",ch);
    return;
  }
  
  original = ch->in_room;

  if (ch->position == POS_FIGHTING) {
    send_to_char(" You're {Yfighting!{x\n\r",ch);
    return;
  }
  
  if ((obj = get_obj_carry (ch, arg, ch)) == NULL) {
    send_to_char("You do not have that!\n\r",ch);
    return;
  }
  else {
    if (!can_drop_obj(ch, obj) && !IS_IMMORTAL(ch)) {
	 send_to_char("Its stuck to you.\n\r",ch);
	 return;
    }

    if (!is_donation(obj)) {
	 send_to_char("That is not a valid donation!\n\r",ch);
	 return;
    }
    
    // Hmm
    if (obj->timer > 0) {
	 send_to_char("You cannot donate that.\n\r",ch);
	 return;
    }
    
    // Check if a chest exsist
    char_from_room(ch);
    char_to_room(ch,get_room_index(ROOM_VNUM_RECALL));
    if ((pit = get_obj_list(ch, "donation", ch->in_room->contents)) == NULL) {
	 char_from_room(ch);
	 char_to_room(ch,original);
	 send_to_char("The donation chest is not ready for your donations.\n\r", ch);
	 return;	 
    }

    // Back to original room, we have object now
    char_from_room(ch);
    char_to_room(ch,original);
    
    // Check if full?
    if (get_obj_weight(obj) + get_true_weight(pit) > (pit->value[0] * 10) ||
	   get_obj_weight(obj) > (pit->value[3] * 10)) {
	 sprintf(buf, "The %s is full.\n\r", !IS_NULLSTR(pit->short_descr) ? pit->short_descr : "chest");
	 send_to_char(buf, ch);
	 return;
    }

    if (ch->in_room != get_room_index(ROOM_VNUM_RECALL))	 
	 act("$n donates $p{x and it vanish in a flash.",ch,obj,NULL,TO_ROOM);
    act("You donate $p{x and it vanish in a flash.",ch,obj,NULL,TO_CHAR);
    
    obj_from_char(obj);
    obj_to_obj(obj, pit);
    if (IS_OBJ_STAT(pit,ITEM_KEEPER))
    {
	save_keeper(ch,pit);
    }
    char_from_room(ch);
    char_to_room(ch,original);
    return;
  }   
}

// Guild donation
void do_gdonate( CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *pit;
  OBJ_DATA *obj;
  ROOM_INDEX_DATA *original;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];

  // Only if in a guild
  if (!is_clan(ch)) {
    send_to_char("You aren't in a guild.\n\r",ch);
    return;
  }
  
  argument = one_argument(argument, arg);
  
  if (IS_NULLSTR(arg)) {
    send_to_char("Donate what to your guild?\n\r",ch);
    return;
  }
  
  original = ch->in_room;
  
  if (ch->position == POS_FIGHTING) {
    send_to_char(" You're {Yfighting!{x\n\r",ch);
    return;
  }
  
  if ((obj = get_obj_carry (ch, arg, ch)) == NULL) {
    send_to_char("You do not have that!\n\r",ch);
    return;
  }
  else {
    if (!can_drop_obj(ch, obj) && !IS_IMMORTAL(ch)) {
	 send_to_char("Its stuck to you.\n\r",ch);
	 return;
    }
    
    if (!is_donation(obj)) {
	 send_to_char("That is not a valid donation!\n\r",ch);
	 return;
    }
    
    // Hmm
    if (obj->timer > 0) {
	 send_to_char("You cannot donate that.\n\r",ch);
	 return;
    }

    // Check if guild room exists
    if (get_room_index(clan_table[ch->clan].room[0]) == NULL) {
	 send_to_char("Your guild don't have a donation chest location yet.\n\r", ch);
	 return;
    }
    
    // Check if a chest exsist
    char_from_room(ch);
    char_to_room(ch,get_room_index(clan_table[ch->clan].room[0]));
    if ((pit = get_obj_list(ch, "donation", ch->in_room->contents)) == NULL) {
	 char_from_room(ch);
	 char_to_room(ch,original);
	 send_to_char("Your guild don't have a donation chest yet.\n\r", ch);
	 return;	 
    }

    // Back to original room, we have object now
    char_from_room(ch);
    char_to_room(ch,original);
    
    // Check if full?
    if (get_obj_weight(obj) + get_true_weight(pit) > (pit->value[0] * 10) ||
	   get_obj_weight(obj) > (pit->value[3] * 10)) {
	 sprintf(buf, "The %s is full.\n\r", !IS_NULLSTR(pit->short_descr) ? pit->short_descr : "chest");
	 send_to_char(buf, ch);
	 return;
    }

    if (ch->in_room != get_room_index(clan_table[ch->clan].room[0]))	 
	 act("$n donates $p{x to $s guild.",ch,obj,NULL,TO_ROOM);
    act("You donate $p{x to your guild.",ch,obj,NULL,TO_CHAR);
    
    obj_from_char(obj);
    obj_to_obj(obj, pit);
    if (IS_OBJ_STAT(pit,ITEM_KEEPER))
    {
	save_keeper(ch,pit);
    }
    char_from_room(ch);
    char_to_room(ch,original);
    return;
  }   
}

void do_toss( CHAR_DATA *ch, char *argument)
{
  CHAR_DATA       *victim=NULL;              // Victim if any at to location	
  char             arg[MAX_INPUT_LENGTH];
  char             buf[MAX_STRING_LENGTH];
  ROOM_INDEX_DATA *from_location=NULL;
  ROOM_INDEX_DATA *to_location=NULL;  
  OBJ_DATA        *obj=NULL;
  int              direction;
  EXIT_DATA       *pexit=NULL;

  argument = one_argument( argument, arg );
  
  if (IS_NULLSTR(arg) || IS_NULLSTR(argument)) {
     send_to_char("Syntax: toss <object> <direction>\n\r", ch);
     return;	
  }
  
  if ((obj = get_obj_carry( ch, arg, ch )) == NULL ) {
     send_to_char("You aren't carrying that.\n\r",ch);
     return;
  }
  
  if ( !can_drop_obj( ch, obj ) ) {
     send_to_char( "You can't let go of it.\n\r", ch );
     return;
  }
	
  if ((direction = find_exit( ch, argument ) ) < 0 ) {
     send_to_char("You don't see a exit in that direction.\n\r", ch);  	
     return;	
  }
  
  // MELT DROP?
  if (IS_OBJ_STAT(obj,ITEM_MELT_DROP)) {
     sprintf(buf, "You take $p out from your inventory and toss it %sward, but in mid-air it dissolves into smoke!", dir_name[direction]);
     act(buf,ch,obj,NULL,TO_CHAR);
     sprintf(buf, "$n take $p out from $s inventory and tosses it %sward, but in mid-air it dissolves into smoke!", dir_name[direction]);
     act(buf,ch,obj,NULL,TO_ROOM);
     extract_obj(obj);
     return;
  }
  
  
 // Tossing weapons is not healthy
  if (obj->item_type == ITEM_WEAPON) {
     sprintf(buf, "You are about to toss $p %sward when you come to think that throwing a weapon can hurt someone!", dir_name[direction]);
     act(buf,ch,obj,NULL,TO_CHAR);
     sprintf(buf, "$n move $s hand like $e is about to toss $p %sward, but suddenly blush and lower $s hand.", dir_name[direction]);
     act(buf,ch,obj,NULL,TO_ROOM);
     return;
  }

  from_location = ch->in_room;
  
  if ((pexit = ch->in_room->exit[direction]) != NULL  && (to_location = pexit->u1.to_room) != NULL && can_see_room(ch, pexit->u1.to_room)) {
     if ( IS_SET(pexit->exit_info, EX_ISDOOR) )  {
	if ( IS_SET(pexit->exit_info, EX_CLOSED) ) {
           send_to_char( "That direction is closed.\n\r",      ch ); 
	   return;
	 } 
	 else if ( IS_SET(pexit->exit_info, EX_LOCKED) ) {
	    send_to_char( "That direction is locked.\n\r",     ch ); 
	    return;
	 }
     }
     else if (IS_SET(pexit->exit_info, EX_FIREWALL)) {
	  sprintf(buf, "You take $p out from your inventory and toss it %sward.", dir_name[direction]);
	  act(buf, ch,obj,NULL,TO_CHAR);
	  sprintf(buf, "$n take $p out from $s inventory and tosses it %sward.", dir_name[direction]);
	  act(buf, ch,obj,NULL,TO_ROOM);
	  
	  act("The $p turn into {Rflames{x as it hit the {rwall of fire{x.", ch, obj, NULL, TO_CHAR);
	  act("The $p turn into {Rflames{x as it hit the {rwall of fire{x.", ch, obj, NULL, TO_ROOM);
	  extract_obj(obj);

	return;	
     }
     else if (IS_SET(pexit->exit_info, EX_AIRWALL)) {
	send_to_char("You suddeny notice that direction is blocked by a wall of air!\n\r", ch);
	return;	
     }  
  }
  else {
     send_to_char("You don't see a exit in that direction.\n\r", ch);
     return;  	
  }
  
  // Room msgs
  sprintf(buf, "You take $p out from your inventory and toss it %sward.", dir_name[direction]);
  act(buf, ch,obj,NULL,TO_CHAR);
  sprintf(buf, "$n take $p out from $s inventory and tosses it %sward.", dir_name[direction]);
  act(buf, ch,obj,NULL,TO_ROOM);
  
  // Other room msgs
  victim = to_location->people;
  
  /* Don't write message unless there are players in to_location */
  if (victim != NULL) {
     if (number_chance(5) && !victim->fighting) {
        sprintf(buf, "Suddeny $p is thrown into the room from the %s and hit you in the back of your head!", dir_name[rev_dir[direction]]);     
        act(buf, victim,obj,NULL,TO_CHAR);
        sprintf(buf, "Suddeny $p is thrown into the room from the %s and hit $n in the back of $s head!", dir_name[rev_dir[direction]]);     
        act(buf, victim,obj,NULL,TO_ROOM);

        victim->position = POS_RESTING;
	DAZE_STATE(victim, 3 * PULSE_VIOLENCE);     	
     }
     else {
        sprintf(buf, "Suddeny $p is thrown into the room from the %s.", dir_name[rev_dir[direction]]);     
        act(buf, victim,obj,NULL,TO_CHAR);
        sprintf(buf, "Suddeny $p is thrown into the room from the %s.", dir_name[rev_dir[direction]]);     
        act(buf, victim,obj,NULL,TO_ROOM);
     }
   }

   obj_from_char( obj );	
   obj_to_room( obj, to_location);
	
   return;	
}

void do_loot(CHAR_DATA * ch, char *argument)
{
   OBJ_DATA *obj;
   CHAR_DATA *victim;
   char log_buf[MAX_INPUT_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];

   argument = one_argument(argument, arg);
   argument = one_argument(argument, arg1);

   if (arg[0] == '\0')
   {
      send_to_char("Syntax: loot <objectname> <playername>.\r\n", ch);
      return;
   }

   if (arg1[0] == '\0')
   {
      send_to_char("Take it from whom?\r\n", ch);
      return;
   }

   if (!(victim = get_char_room(ch, arg1)))
   {
      send_to_char("They aren't here.\r\n", ch);
      return;
   }

   /*
   if (!IS_RP(ch)) {
	send_to_char("You must be Completely IC to do this\r\n",ch);
	return;
   }
   if (!IS_RP(victim)) {
	send_to_char("They must be Completely IC to do this\r\n",ch);
	return;
   }
   */

   if (victim->position != POS_DEAD) {
	send_to_char("They're still willing to put up a fight for it.\r\n",ch);
        return;
   }
   for (obj = victim->carrying; obj != NULL; obj = obj->next_content)
      if (is_name(arg, obj->name))
      {
         if (obj->wear_loc > WEAR_NONE)
            unequip_char(victim, obj);
         obj_from_char(obj);
         obj_to_char(obj, ch);
         act("You remove $N's $p and take it.", ch, obj, victim, TO_CHAR);
  	 act("$n removes $p from $N and takes it.",ch,obj,victim,TO_ROOM);
  	 sprintf(log_buf, "%s looted %s [vnum=%d] from %s", !IS_NPC(ch) ? ch->name : ch->short_descr, obj->short_descr, obj->pIndexData->vnum, !IS_NPC(victim) ? victim->name : victim->short_descr);
  	 wiznet(log_buf,ch,NULL,WIZ_FLAGS,0,0);
  	 log_string(log_buf);
         return;
      }

   act("$N does not seem to have that.", ch, obj, victim, TO_CHAR);
   act("$n is going through $N's gear.", ch, obj, victim, TO_ROOM);
   return;
}

void do_show(CHAR_DATA *ch, char *argument)
{
  char buf[MSL];
  char arg[MSL];
  char arg2[MSL];
  char show_buf[MSL];
  OBJ_DATA *obj=NULL;
  CHAR_DATA *victim=NULL;
  bool show2room=FALSE;
  bool show_short=FALSE;

  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg2);

  //printf("argument=%s, arg=%s, arg2=%s\n", argument, arg, arg2);

  if (IS_NULLSTR(arg2) || IS_NULLSTR(arg)) {
    send_to_char("Syntax: show <object> <victim/room> [short]\n\r", ch);
    return;
  }

  if (!str_cmp(arg2, "room") || !str_cmp(arg2, "all"))
    show2room = TRUE;

  if (!str_cmp(argument, "short"))
    show_short = TRUE;

  if (ch->position == POS_FIGHTING) {
    send_to_char(" You're {Yfighting!{x\n\r",ch);
    return;
  }

  if (!show2room) {
    if (( victim = get_char_room( ch, arg2)) == NULL ) {
	 send_to_char( "They aren't here.\n\r", ch );
	 return;
    }
  }
  
  if ((obj = get_obj_carry (ch, arg, ch)) == NULL) {
    send_to_char("You do not have that!\n\r",ch);
    return;
  }
  else {
    if (show_short)
	 sprintf(show_buf, "%s", obj->short_descr);
    else
	 sprintf(show_buf, "%s", obj->description);

    if (!show2room) {
	 sprintf(buf, "You hold out '%s' for $N to see.", show_buf);
	 act(buf, ch, NULL, victim, TO_CHAR);
	 sprintf(buf, "$n holds out '%s' for you to see.", show_buf);
	 act(buf, ch, NULL, victim, TO_VICT);
	 act("$n holds out and show $N something.", ch, NULL, victim, TO_NOTVICT);

	 return;
    }
    else {
	 sprintf(buf, "You hold out '%s' for all in the room to see.", show_buf);
	 act(buf, ch, NULL, victim, TO_CHAR);
	 sprintf(buf, "$n holds out '%s' for all in the room to see.", show_buf);
	 act(buf, ch, NULL, victim, TO_ROOM);
	 
	 return;
    }
  }
  

  return;
}

void save_keeper(CHAR_DATA * ch, OBJ_DATA * obj)
{
	char filename[MAX_STRING_LENGTH];
	sprintf(filename,"keeper/%d",obj->in_room->vnum);	
	FILE * fp = fopen(filename,"w");
        if (fp)	
	{
    //	   fprintf( fp, "#OBJECT\n" );
	   fwrite_keeper_obj(ch,obj,fp,0);
    	   fprintf( fp, "#END\n" );
	   fclose(fp);
	}
}


void save_vehicle(CHAR_DATA * ch, OBJ_DATA * obj)
{
        char filename[MAX_STRING_LENGTH];
        sprintf(filename,"vehicles/%d",obj->pIndexData->vnum);
        FILE * fp = fopen(filename,"w");
        if (fp)
        {
	   fprintf(fp,"%d",obj->in_room->vnum);
           fclose(fp);
        }
}

void do_remkeeper(CHAR_DATA *ch, char *argv)
{
   char filename[MAX_STRING_LENGTH];
   sprintf(filename,"keeper/%d",ch->in_room->vnum);
   FILE * fp = fopen(filename,"r");
   if (!fp)
   {
	send_to_char("There is no keeper set for this room\r\n",ch);
   }
   else
   {
	fclose(fp);
	remove(filename);
	send_to_char("Ok\r\n",ch);
   }	
   return;

}
