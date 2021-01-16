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
 *	ROM 2.4 is copyright 1993-1998 Russ Taylor                            *
 *	ROM has been brought to you by the ROM consortium                     *
 *	    Russ Taylor (rtaylor@hypercube.org)                               *
 *	    Gabrielle Taylor (gtaylor@hypercube.org)                          *
 *	    Brian Moore (zump@rom.org)                                        *
 *	By using this code, you have agreed to follow the terms of the        *
 *	ROM license, in the file Rom24/doc/rom.license                        *
 ***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>  /* for finger. GWAR 8-2-95 */
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "blowfish.h"

/* Prototypes */
bool check_parse_name (char* name);
char * flag_string( const struct flag_type *, long );

int find_exit( CHAR_DATA *ch, char *arg );          /* act_move.c */

char *	const	where_name	[] =
{
    "<floating as light> ",
    "<floating nearby>   ",
    "<scratched tattoo>  ",
    "<worn on finger>    ",
    "<worn on finger>    ",
    "<worn around neck>  ",
    "<worn around neck>  ",
    "<worn on body>      ",
    "<worn on head>      ",
    "<worn on legs>      ",
    "<worn on feet>      ",
    "<worn on hands>     ",
    "<worn on arms>      ",
    "<worn as shield>    ",
    "<worn on back>      ",
    "<worn about body>   ",
    "<worn about waist>  ",
    "<worn around wrist> ",
    "<worn around wrist> ",
    "<held>              ",
    "<wielded>           ",
    "<dual wielded>      ",
    "<stuck in>          ",
    "<left ear>          ",
    "<right ear>         ",
    "<over face>         ",
    "<sheathed>          ",
    "<sheathed>          "

};

#define ACT_BUFF_SIZE 4*MAX_STRING_LENGTH
char act_buff [ACT_BUFF_SIZE];

/* for  keeping track of the player count */
int max_on = 0;
int max_on_ever = 0;

/*Used for gateway and vnum encryption/decryipt */
unsigned char key [8] = 
{
   0xf3, 0xda, 0xba, 0x91, 0x67, 0x45,0x32,0x01
};

/* bit.c */
char * background_flag_string( const struct background_type *, long );
bool use_christmas_layout(); /* board.c */

/*
 * Local functions.
 */
char *	format_obj_to_char	args( ( OBJ_DATA *obj, CHAR_DATA *ch,
				    bool fShort, bool fShowLevel ) );
void	show_list_to_char	   args( ( OBJ_DATA *list, CHAR_DATA *ch,
				    bool fShort, bool fShowNothing, bool fShowLevel ) );
void	show_char_to_char_0	args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void	show_char_to_char_1     args( ( CHAR_DATA *victim, CHAR_DATA *ch, bool brief) );
void	show_char_to_char	   args( ( CHAR_DATA *list, CHAR_DATA *ch, bool together ) );
int   colorstrlen          args( (char *argument ) );

char *indefinite( char *str )
{
  char first;
  
  /* Get first character in lower case */
  first = tolower( str[0] );
  switch( first )
    {
	 /* If it's a vowel, return "an" */
    case 'a' :
    case 'e' :
    case 'i' :
    case 'o' :
    case 'u' :
	 return "an";
	 /* Otherwise return "a" */
    default :
	 return "a";
    }
  
  /* To prevent warnings from dumb compilers */
  return "a";
}

bool IS_INTRONAME( CHAR_DATA *ch, char *argument)
{
  struct idName *name;

  for ( name = ch->pcdata->names; name; name = name->next) {
    if (name) {
    	if (!str_prefix(argument,name->name )) {
	 	return TRUE;
    	}
    }
  }
  return FALSE;
}

/* PERS2 */
char *PERS( CHAR_DATA *ch, CHAR_DATA *looker)
{
  /* Buffer to contain final string */
  static char buf[MAX_INPUT_LENGTH];
  char def[MAX_INPUT_LENGTH];

  /* Linked list index for names */
  struct idName *name;

  memset(buf, 0x00, sizeof(buf));
  memset(def, 0x00, sizeof(def));
  
  /* If looker can't see ch, just return "someone" */
  if ( !can_see( looker, ch ) )
    return "Someone";
  
  /* NPC's just show their short descriptions */
  if ( IS_NPC( ch ) )
    return ch->short_descr;

  /* Eveyone knows an immortal, and immortals know everyone */
  if ( IS_IMMORTAL( ch ) ||
	  IS_IMMORTAL( looker ) ||
	  IS_NPC( looker ) ||
	  ch == looker ) {
    if (IS_NPC( looker))
	   sprintf( buf, "(%s) %s", COLORNAME(ch), ch->pcdata->appearance);
    else if (IS_HOODED(ch) && !IS_IMMORTAL(looker)) {
	 sprintf( buf, "%s", ch->pcdata->hood_appearance);
    }
    else if (IS_VEILED(ch) && !IS_IMMORTAL(looker)) {
	 sprintf( buf, "%s", ch->pcdata->veil_appearance);
    }
    else if (IS_WOLFSHAPE(ch)) {
	 sprintf( buf, "%s", ch->pcdata->wolf_appearance);
    }
    else if (IS_DREAMING(ch) && !IS_NPC(ch) && ch->pcdata->dreaming_appearance != NULL && ch->pcdata->dreaming_appearance[0] != '\0') {
	 sprintf( buf, "%s", ch->pcdata->dreaming_appearance);
    }
    else if (IS_ILLUSIONAPP(ch)) {
	 sprintf( buf, "(%s) %s", COLORNAME(ch), ch->pcdata->illusion_appearance);
    }
    else if (IS_MASQUERADED(ch) && !IS_IMMORTAL(looker)) {
	 sprintf( buf, "%s", ch->pcdata->masquerade_appearance);
    }
    else {
	 if (!IS_SET(looker->comm, COMM_COMPACT) )
	   sprintf( buf, "(%s) %s", COLORNAME(ch), ch->pcdata->appearance);
	 else
	   sprintf( buf, "%s", COLORNAME(ch));
    }
    return buf;
  }
  
  /* Standard linked list search */
  for( name = looker->pcdata->names; name; name = name->next ) {
    /* Comparing numbers is faster than str_cmp each time */
    if ( name->id == ch->id ) {
	 if (IS_HOODED(ch) && !IS_NPC(looker)) {
	   sprintf( buf, "%s", ch->pcdata->hood_appearance);
	 }
	 else if (IS_VEILED(ch) && !IS_NPC(looker)) {
	   sprintf( buf, "%s", ch->pcdata->veil_appearance);
	 }	 
	 else if (IS_WOLFSHAPE(ch) && !IS_NPC(looker)) {
	   sprintf( buf, "%s", ch->pcdata->wolf_appearance);
	 }
    else if (IS_DREAMING(ch) && !IS_NPC(ch) && ch->pcdata->dreaming_appearance != NULL && ch->pcdata->dreaming_appearance[0] != '\0' && !IS_NPC(looker)) {
	 sprintf( buf, "%s", ch->pcdata->dreaming_appearance);
    }
	 else if (IS_ILLUSIONAPP(ch) && !IS_NPC(looker)) {
	   sprintf( buf, "%s", ch->pcdata->illusion_appearance);
	 }
	 else if (IS_MASQUERADED(ch) && !IS_NPC(looker)) {
	   sprintf( buf, "%s", ch->pcdata->masquerade_appearance);
	 }
	 else {
	   if (!IS_SET(looker->comm, COMM_COMPACT) )
		sprintf( buf, "(%s) %s", name->name, ch->pcdata->appearance);
	   else
		sprintf( buf, "%s", name->name);
	 }
	 return buf;
    }
  }
  
  /* Looker has not assigned ch a name yet */
  if (IS_HOODED(ch) && !IS_NPC(looker)) {
    sprintf( buf, "%s",ch->pcdata->hood_appearance);
  }
  else if (IS_VEILED(ch) && !IS_NPC(looker)) {
    sprintf( buf, "%s",ch->pcdata->veil_appearance);
  }  
  else if (IS_WOLFSHAPE(ch) && !IS_NPC(looker)) {
    sprintf( buf, "%s", ch->pcdata->wolf_appearance);
  }
  else if (IS_DREAMING(ch) && !IS_NPC(ch) && ch->pcdata->dreaming_appearance != NULL && ch->pcdata->dreaming_appearance[0] != '\0') {
	 sprintf( buf, "%s", ch->pcdata->dreaming_appearance);
    }
  else if (IS_ILLUSIONAPP(ch) && !IS_NPC(looker)) {
    sprintf( buf, "%s",ch->pcdata->illusion_appearance);
  } 
  else if (IS_MASQUERADED(ch) && !IS_NPC(looker)) {
    sprintf( buf, "%s", ch->pcdata->masquerade_appearance);
  }
  else {
    sprintf( buf, "%s",ch->pcdata->appearance);
  }
  return buf;
}

char *PERS_NAME( CHAR_DATA *ch, CHAR_DATA *looker)
{
  /* Buffer to contain final string */
  static char buf[MAX_INPUT_LENGTH];
  char def[MAX_INPUT_LENGTH];
  
  /* Linked list index for names */
  struct idName *name;
  
  memset(buf, 0x00, sizeof(buf));
  memset(def, 0x00, sizeof(def));
  
  /* If looker can't see ch, just return "someone" */
  if ( !can_see( looker, ch ) )
    return "Someone";
  
  /* NPC's just show their short descriptions */
  if ( IS_NPC( ch ) )
    return ch->short_descr;

  /* Eveyone knows an immortal, and immortals know everyone */
  if ( IS_IMMORTAL( ch ) ||
	  IS_IMMORTAL( looker ) ||
	  IS_NPC( looker ) ||
	  ch == looker ) {
    sprintf( buf, "%s", COLORNAME(ch));
    return buf;
  }
  
  /* Standard linked list search */
  for( name = looker->pcdata->names; name; name = name->next ) {
    /* Comparing numbers is faster than str_cmp each time */
    if ( name->id == ch->id ) {
	 sprintf( buf, "%s", name->name);
	 return buf;
    }
  }
  
  /* Looker has not assigned ch a name yet */
  return ch->name;
}

bool has_introed(CHAR_DATA *ch, CHAR_DATA *vch)
{
  /* Linked list index for names */
  struct idName *name;

  if (IS_NPC(vch))
    return FALSE;
 
  for( name = vch->pcdata->names; name; name = name->next ) {
    /* Comparing numbers is faster than str_cmp each time */
    if ( name->id == ch->id ) {
	 return TRUE;
    }
  }
  return FALSE;
}

void add_know( CHAR_DATA *ch, long id, char *name )
{
  struct idName *cName;
  
  /* Search for existing name */
  for( cName = ch->pcdata->names; cName; cName = cName->next ) {
    if ( cName->id == id )
	 break;
  }
  
  /* Not found */
  if ( !cName ) {
    /* The stuff you added to recycle.c */
    cName = new_idname();
    cName->name = str_dup( capitalize( name ) );
    cName->id = id;
    cName->next = ch->pcdata->names;
    ch->pcdata->names = cName;
    return;
  }
  
  /* Found so rename */
  free_string( cName->name );
  cName->name = str_dup( capitalize( name ) );
}

void do_pkrank(CHAR_DATA *ch, char *argument)
{
	PKINFO_TYPE	  *     pk = pkranks;
	BUFFER * output;
    	char buffer [MAX_STRING_LENGTH];
	output = new_buf();
	sprintf(buffer, "%15s %6s %6s\n\r","Character","Kills","Deaths");
	add_buf(output,buffer);
	while (pk)
	{
		sprintf(buffer,"%15s %6d %6d\n\r",pk->character,pk->pk_count,pk->pk_death_count);
		add_buf(output,buffer);
		pk = pk->next;
	}
   	page_to_char( buf_string(output), ch );
	free_buf(output);

}

void do_intro(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *victim;
  CHAR_DATA *to;
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int realname=FALSE;

  argument = one_argument (argument, arg1);

  // Only for PCs
  if (IS_NPC(ch))
    return;

  if (IS_IMMORTAL(ch)) {
    send_to_char("Immortals don't have to introduce them self. They know all..\n\r", ch);
    return;
  }
  
  if (IS_HOODED(ch)) {
    send_to_char("You can't introduce your self with the hood raised.\n\r", ch);
    return;  	
  }

  if (IS_VEILED(ch)) {
    send_to_char("You can't introduce your self when your face is veiled.\n\r", ch);
    return;  	
  }  

  if (arg1[0] == '\0') {
    send_to_char("Syntax:\n\r", ch );
    send_to_char("        introduce <player>\n\r", ch); // [name]\n\r", ch);
    send_to_char("or      introduce room\n\r", ch);
    return;
  }

  if (argument[0] == '\0') {
    realname = TRUE;
  }
  //realname = TRUE; // False intro disabled

  if (!realname) {
    if (!check_parse_name ( argument)) {
	 send_to_char("Illegal name, try another!\n\r", ch);
	 return;
    }
  }

  if (!str_cmp(arg1, "room")) {
    for ( to = ch->in_room->people; to != NULL; to = to->next_in_room ) {
	if ( (!IS_NPC(to) && to->desc == NULL )
	||   ( IS_NPC(to) )
	||    to->position < POS_RESTING )
            continue;

	if (!IS_SAME_WORLD(to, ch))
	  continue;

        if (ch == to)  //already know yourself
          continue;

  	if (has_introed(ch, to))  //already knows the person
          continue;

        if (IS_IMMORTAL(to))
          continue;

        /* All check ok, lets add to known list */      
        /* Add to chars know list */
        if (!realname)
          add_know(to, ch->id, colorstrem(argument));
          //add_know(to, ch->id, ch->name);
        else
          add_know(to, ch->id, ch->name);
          
        sprintf(buf, "%s introduces $mself to you.", PERS(ch, to));
        act (buf, ch, NULL, to, TO_VICT);
        
        /* Ats: Do we want to see previous given names in this ? */
        sprintf(buf, "%s is %s.\n\r", ch->pcdata->appearance, PERS(ch, to));
        send_to_char(buf, to);          
          
  	if (!realname) {
      		sprintf(buf, "You introduce your self to %s as (%s) %s.\n\r", PERS(to, ch), 
      		argument, ch->pcdata->appearance);
  	}
  	else 
	{
      		sprintf(buf, "You introduce your self to %s as (%s) %s.\n\r", PERS(to, ch), 
      		ch->name, ch->pcdata->appearance);
  	}
        send_to_char(buf, ch);
  
     }
     return;
  }

  if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
    send_to_char("You can't introduce your self to someone not here!\n\r", ch);
    return;
  }

  if (IS_IMMORTAL(victim)) {
    send_to_char("No need to introduce your self to the immortals, they know you!\n\r", ch);
    return;
  }
  
  if (IS_NPC(victim)) {
    send_to_char("You can't introduce your self to mobiles!\n\r", ch);
    return;
  }

  if (victim == ch) {
    send_to_char("You feel like you start to know your self, after all these years!\n\r", ch);
    return;
  }

//  if (has_introed(ch, victim)) {
    //act("You must be getting old, cause you already did this before.", ch, NULL, victim, TO_CHAR);
    //return;
  //}
  
  /* All check ok, lets add to known list */
  sprintf(buf, "%s introduce $mself to you.", PERS(ch, victim));
  act (buf, ch, NULL, victim, TO_VICT);

  /* Add to chars know list */
  if (!realname)
    add_know(victim, ch->id, colorstrem(argument));
    //add_know(victim, ch->id, ch->name);
  else
    add_know(victim, ch->id, ch->name);

  /* Ats: Do we want to see previous given names in this ? */
  sprintf(buf, "%s is %s.\n\r", ch->pcdata->appearance, PERS(ch, victim));
  send_to_char(buf, victim);

  if (!realname) {
      sprintf(buf, "You introduce your self to %s as (%s) %s.\n\r", PERS(victim, ch), 
      argument, ch->pcdata->appearance);
  }
  else {
      sprintf(buf, "You introduce your self to %s as (%s) %s.\n\r", PERS(victim, ch), 
      ch->name, ch->pcdata->appearance);
  }
  send_to_char(buf, ch);
  
  return;
}

char *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort, bool fShowLevel )
{
  static char buf[MAX_STRING_LENGTH];
  char in_obj_buf[MAX_STRING_LENGTH];
  char levelbuf[10];
  
  buf[0] = '\0';
  
  if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
	 ||  (obj->description == NULL || obj->description[0] == '\0'))
    return buf;
  
  if ( IS_OBJ_STAT(obj, ITEM_INVIS)     )   strcat( buf, "({WInvis{x) "     );
  if ( IS_OBJ_STAT(obj, ITEM_GLOW)      )   strcat( buf, "({YGlowing{x) "   );
  if ( IS_OBJ_STAT(obj, ITEM_HUM)       )   strcat( buf, "({mHumming{x) "   );
  if ( IS_OBJ_STAT(obj, ITEM_BROKEN)    )   strcat( buf, "({rBroken{x) "    );
  if (IS_SET(obj->extra_flags,ITEM_HIDDEN)) strcat( buf, "{Y(h){x "         );
  if (fShowLevel) 
  {
      sprintf(levelbuf,"({C%d{x) ",obj->level);
      strcat(buf,levelbuf);
  } 

  if ( fShort ) {
    if ( obj->short_descr != NULL )
	 strcat( buf, obj->short_descr );

  }
  else {
    if( ch->in_obj == obj ) {
	 sprintf( in_obj_buf, "%s in which you are sitting.", obj->short_descr );
	 *in_obj_buf = UPPER( *in_obj_buf );
	 strcat( buf, in_obj_buf );
    }
    else
	 if( obj->who_in
		&& ( !ch->in_obj
			|| IS_SET( ch->in_obj->value[1], CONT_SEE_OUT )
			|| !IS_SET( ch->in_obj->value[1], CONT_CLOSED ) )
		&& ( IS_SET( obj->value[1], CONT_SEE_IN )
			|| !IS_SET( obj->value[1], CONT_CLOSED ) ) ) {
	   sprintf( in_obj_buf, "%s containing %s is here.", obj->short_descr,
			  name_list( obj->who_in, ch ) );
	   *in_obj_buf = UPPER( *in_obj_buf );
	   strcat( buf, in_obj_buf );
      }
	 else if (IS_SET( obj->value[1], CONT_SEE_THROUGH)) {
	   sprintf( in_obj_buf, "%s containing %s is here.", obj->short_descr,
			  obj_list(obj->contains, ch));
	   *in_obj_buf = UPPER( *in_obj_buf );
	   strcat( buf, in_obj_buf );
	 }
	 else
	   if ( obj->description != NULL)
		strcat( buf, obj->description );
  }
  return buf;
}



/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing, bool fShowLevel )
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;
    char **prgpstrShow;
    int *prgnShow;
    char *pstrShow;
    OBJ_DATA *obj;
    int nShow;
    int iShow;
    int count;
    bool fCombine;

    if ( ch->desc == NULL )
	return;

    /*
     * Alloc space for output lines.
     */
    output = new_buf();

    count = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
	count++;
    prgpstrShow	= alloc_mem( count * sizeof(char *) );
    prgnShow    = alloc_mem( count * sizeof(int)    );
    nShow	= 0;

    /*
     * Format the list of objects.
     */
    for ( obj = list; obj != NULL; obj = obj->next_content ) { 
      if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )) {
	   /* Items above EXIT line */
	   if ((obj->item_type == ITEM_FOUNTAIN && !obj->carried_by) ||
		  (obj->item_type == ITEM_FURNITURE && !obj->carried_by && !obj->in_obj)) {
	    
		/* Portals are kept under Exit line                       */
		/* (obj->item_type == ITEM_PORTAL && !obj->carried_by)) { */
		continue;
	   }
         /* Hidden objects, small chance of showing to user don't show user */
         if (IS_SET(obj->extra_flags,ITEM_HIDDEN) && !IS_IMMORTAL(ch)) {

	    if ((((get_skill(ch,gsn_observation) + get_skill(ch,gsn_alertness) + get_skill(ch,gsn_awareness)) / 3) /4) < number_percent())
	    {
	
               continue;
            }
         }
	      pstrShow = format_obj_to_char( obj, ch, fShort, fShowLevel );

	      fCombine = FALSE;

	      if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) ){
		      /*
		      * Look for duplicates, case sensitive.
		      * Matches tend to be near end so run loop backwords.
		      */
		      for ( iShow = nShow - 1; iShow >= 0; iShow-- ) {
		         if ( !strcmp( prgpstrShow[iShow], pstrShow ) ) {
                  prgnShow[iShow]++;
			         fCombine = TRUE;
			         break;
		         }
		      }
	      }

	    /*
	     * Couldn't combine, or didn't want to.
	     */
	      if ( !fCombine ) {
		      prgpstrShow [nShow] = str_dup( pstrShow );
		      prgnShow    [nShow] = 1;
		      nShow++;
	      }
      }
    }

    /*
     * Output the formatted list.
     */
    for ( iShow = 0; iShow < nShow; iShow++ )
    {
	if (prgpstrShow[iShow][0] == '\0')
	{
	    free_string(prgpstrShow[iShow]);
	    continue;
	}

	if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
	{
	    if ( prgnShow[iShow] != 1 )
	    {
		sprintf( buf, "(%2d) ", prgnShow[iShow] );
		add_buf(output,buf);
	    }
	    else
	    {
		add_buf(output,"     ");
	    }
	}
	add_buf(output,prgpstrShow[iShow]);
	add_buf(output,"\n\r");
	free_string( prgpstrShow[iShow] );
    }

    if ( fShowNothing && nShow == 0 )
    {
	if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
	    send_to_char( "     ", ch );
	send_to_char( "Nothing.\n\r", ch );
    }
    page_to_char(buf_string(output),ch);

    /*
     * Clean up.
     */
    free_buf(output);
    free_mem( prgpstrShow, count * sizeof(char *) );
    free_mem( prgnShow,    count * sizeof(int)    );

    return;
}



void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch )
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char message[MAX_STRING_LENGTH];
  int sex=-1;
  int learned=0;

  buf[0] = '\0';
  buf2[0] = '\0';
    
  if (!IS_COLORCLOAKED(victim)) {
    if ( victim == ch )                            strcat( buf, "{W(You){x "      );
    if (IS_RP(victim))                             strcat( buf, "{W(IC){x "       );
    if (!IS_NPC(victim) && (victim->desc == NULL)) strcat( buf, "{Y(Link dead){x ");
    if ( victim->timer > 3)                        strcat( buf, "{Y(Idle){x "     );
    if ( IS_SET(victim->comm,COMM_AFK	  )   )  strcat( buf, "{Y(AFK){x "      );
  }
  
  // Quest mob
  if (!IS_NPC(ch) && IS_SET(ch->act,PLR_QUESTING ) && IS_NPC(victim)) {
    if (ch->pcdata->questmob == victim->pIndexData->vnum) {
	 strcat( buf, "{G(Quest mob){x ");
    }
  }


  if (IS_NPC(victim) && !IS_NPC(ch) && is_guild_guard(victim, ch)) {
    strcat( buf, "{8(Guild guard){x ");
  }

  /* If not the normal world, tell */
  if (!IS_SET(victim->world,WORLD_NORMAL)) {
    sprintf(buf2, "{R(%s){x ", flag_string(world_table, victim->world));
    strcat(buf, buf2);
  }

  if (IS_NPC(ch)) {
    sex = ch->sex;
    learned = get_skill(ch, -1); // -1 = level based skill
  }
  else {
    sex = ch->pcdata->true_sex;
    learned = ch->pcdata->learned[skill_lookup(sex == SEX_MALE ? "seize" : "embrace")];    
  }
  
  if ((victim->sex == sex) 
	 && (IS_AFFECTED(victim, AFF_CHANNELING)) 
	 && (ch->class == CLASS_CHANNELER)
         && (!IS_SET(victim->act2,PLR2_MASKCHAN))
	 && (learned > 0))
    strcat(buf, "{W(Channeling){x ");
  
  if ( IS_AFFECTED(victim, AFF_FLYING)      ) strcat( buf, "({cLevitated{x) "     );
  if ( IS_GIANTILLUSION(victim))              strcat( buf, "({cGiant Size{x) "         );
  if ( IS_MASQUERADED(victim))                strcat( buf, "({mMasked{x) " );
  
  if (IS_PKILLER(victim) && !IS_IMMORTAL(victim))
  {
	strcat(buf,"{R(PK){x ");
  }
  if ((number_percent() < (get_curr_stat(ch, STAT_WIS))) || ch == victim) {
    if (!IS_HOODED(victim) && !IS_VEILED(victim) && victim->position != POS_SLEEPING)
	 if (IS_WOLFKIN(victim))               strcat ( buf, "({YGoldeneyes{x) ");
  }
  
/*
    if ( IS_AFFECTED(victim, AFF_INVISIBLE)   ) strcat( buf, "(Invis) "      );
    if ( victim->invis_level >= LEVEL_HERO    ) strcat( buf, "(Wizi) "	     );
    if ( IS_AFFECTED(victim, AFF_HIDE)        ) strcat( buf, "(Hide) "       );
    if ( IS_AFFECTED(victim, AFF_CAMOUFLAGE)  ) strcat( buf, "(Camouflaged) ");
    if ( IS_AFFECTED(victim, AFF_FLYING)      ) strcat( buf, "(Flying) "     );
*/

  if (IS_AFFECTED(victim, AFF_CHARM) && IS_SET(victim->act, ACT_RIDEABLE)
	 && (victim->mount != NULL)) {
    if (ch->mount != victim)
	 strcat( buf, "({yM{Do{yu{Dn{yt{De{yd{x) ");
    else
	 strcat( buf, "({WYour {ym{Do{yu{Dn{yt{x) ");
    //strcat( buf, "(");
    //strcat( buf, victim->mount->name);
    //strcat( buf, "'s mount) " );
  }
  
/*
  if( IS_AFFECTED(victim, AFF_CHARM) && (victim->mount == NULL)) strcat( buf, "(Charmed) "    );
  if ( IS_AFFECTED(victim, AFF_PASS_DOOR)   ) strcat( buf, "(Translucent) ");
  if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) ) strcat( buf, "(Pink Aura) "  );
  if ( IS_AFFECTED(victim, AFF_SANCTUARY)   ) strcat( buf, "(White Aura) " );
  if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER ) )
  strcat( buf, "(KILLER) "     );
  if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF  ) )
  strcat( buf, "(THIEF) "      );
*/

  if ( victim->position == victim->start_pos 
	  && ( !victim->in_obj
		  || victim->in_obj == ch->in_obj )
	  && victim->long_descr[0] != '\0' ) {
    strcat( buf, victim->long_descr );
    send_to_char( buf, ch );
    return;
  }

  /*-----------------------------------------------------------------*/
  /* Char flags (P) = Person (I) = Immortal (T) = Trolloc (F) = Fade */
  if (!IS_COLORCLOAKED(victim)) {
    if (IS_MASQUERADED(victim) && !IS_IMMORTAL(victim)) {
	 strcat( buf, "{y(\?\?){x " );
    }
    else if (!IS_NPC(victim) &&  !IS_IMMORTAL(victim) 
	   && (victim->race != race_lookup("trolloc"))
	   && (victim->race != race_lookup("fade")) 
	   && (victim->race != race_lookup("ogier"))
	   && !IS_HOODED(victim)
	   && !IS_VEILED(victim)
	   && !IS_WOLFSHAPE(victim)){
	 strcat( buf, "{y(P){x " );
    }
    else if (!IS_NPC(victim) && (IS_IMMORTAL(victim))) {
	 strcat( buf, "{Y({WI{Y){x " );
    }
    else if (!IS_NPC(victim) && IS_HOODED(victim)) {
	 strcat( buf, "{D({yH{D){x " );
    }    
    else if (!IS_NPC(victim) && IS_VEILED(victim)) {
	 strcat( buf, "{Y({DV{Y){x " );
    }    
    else if (!IS_NPC(victim) && IS_WOLFSHAPE(victim)) {
	 strcat( buf, "{D({YW{D){x " );
    }
    else if (!IS_NPC(victim) && (victim->race == race_lookup("trolloc"))) {
	 strcat( buf, "{D({RT{D){x " );
    }
    else if (!IS_NPC(victim) && (victim->race == race_lookup("fade"))) {
	 strcat( buf, "{D({WF{D){x " );
    }
    else if (!IS_NPC(victim) && (victim->race == race_lookup("ogier"))) {
	 strcat( buf, "{g({GO{g){x " );
    }
  }

  /*-----------------------------------------------------------------*/
  /* Players with viewing talent can see (Aura) if other player      */
  /* have set the viewing info. Talented players can then examine    */
  /* Aura with the viewing command : Swordfish                       */
  if (!IS_NULLSTR(victim->aura_description) && IS_SET(ch->talents, TALENT_VIEWING)) {
    strcat(buf, "({mAura{x) " );
  }

  /*-----------------------------------------------------------------*/
  /* Players with see_taveren talent can see (Tav) is other player   */
  /* is a tavere.                                                    */
  /* -SF                                                             */
  if (IS_TAVEREN(victim) && IS_SET(ch->talents, TALENT_SENSETAV)) {
    if (number_percent() < (get_curr_stat(ch, STAT_WIS))) {
	 strcat(buf, "({RTav{x) " );
    }
  }

  /*-----------------------------------------------------------------*/
  /* Show other players if this person is bound or blindfolded       */
  if (IS_AFFECTED(victim,AFF_BIND))
  {
	strcat(buf, "({RBOUND{x) ");
  }

  if (IS_AFFECTED(victim,AFF_BLINDFOLDED))
  {
	strcat(buf, "({RBLNDFLD{x) ");
  }

  /*-----------------------------------------------------------------*/
  /* Handy intro flags  - PCs only                                   */

  if (!IS_NPC(ch) && !IS_NPC(victim)) {
     if (has_introed(ch, victim) && !has_introed(victim, ch))
        strcat(buf, "({WNIB{x) ");
     else if (has_introed(victim, ch) && !has_introed(ch, victim))
        strcat(buf, "({WYNIB{x) ");
  }


  strcat(buf, PERS( victim, ch));
  
  if (!IS_NPC(victim) && !IS_HOODED(victim) && !IS_VEILED(victim) && !IS_WOLFSHAPE(victim)) {
    if (strlen(victim->pcdata->ictitle) > 1) {
	 strcat( buf, " ");
	 strcat( buf, victim->pcdata->ictitle );
    }
  }

/*    } */
    
/*    
	 if ( !IS_NPC(victim) && !IS_SET(ch->comm, COMM_BRIEF) 
	 &&   victim->position == POS_STANDING && ch->on == NULL ) {
*/


  switch ( victim->position ) {
  case POS_DEAD:     strcat( buf, " is lying here ALMOST DEAD!!" ); break;
  case POS_MORTAL:   strcat( buf, " is mortally wounded." );   break;
  case POS_INCAP:    strcat( buf, " is incapacitated." );      break;
  case POS_STUNNED:  strcat( buf, " is lying here stunned." ); break;
  case POS_SLEEPING: 
    if (victim->on != NULL) {
	 if (IS_SET(victim->on->value[2],SLEEP_AT)) {
	   sprintf(message," is sleeping at %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
	 else if (IS_SET(victim->on->value[2],SLEEP_ON)) {
	   sprintf(message," is sleeping on %s.",
			 victim->on->short_descr); 
	   strcat(buf,message);
	 }
	 else {
	   sprintf(message, " is sleeping in %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
    }
    else 
	 strcat(buf," is sleeping here.");
    break;
    
  case POS_RESTING:  
    if (victim->on != NULL) {
	 if (IS_SET(victim->on->value[2],REST_AT)) {
	   sprintf(message," is resting at %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
	 else if (IS_SET(victim->on->value[2],REST_ON)) {
	   sprintf(message," is resting on %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
	 else  {
	   sprintf(message, " is resting in %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
    }
    else
	 strcat( buf, " is resting here." );       
    break;
    
  case POS_SITTING:  
    if (victim->on != NULL) {
	 if (IS_SET(victim->on->value[2],SIT_AT)) {
	   sprintf(message," is sitting at %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
	 else if (IS_SET(victim->on->value[2],SIT_ON)) {
	   sprintf(message," is sitting on %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
	 else {
	   sprintf(message, " is sitting in %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
    }
    else
	 strcat(buf, " is sitting here.");
    break;
    
  case POS_STANDING: 
    if (victim->on != NULL) {
	 if (IS_SET(victim->on->value[2],STAND_AT)) {
	   sprintf(message," is standing at %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
	 else if (IS_SET(victim->on->value[2],STAND_ON)) {
	   sprintf(message," is standing on %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
	 else {
	   sprintf(message," is standing in %s.",
			 victim->on->short_descr);
	   strcat(buf,message);
	 }
    }
    else if (MOUNTED(victim)) {
	 sprintf(message," is here, riding %s.",PERS(MOUNTED(victim),ch));
	 strcat(buf, message);
    }
    else
	 strcat( buf, " is here." );               
    break;
    
  case POS_FIGHTING:
    strcat( buf, " is here, fighting " );
    if ( victim->fighting == NULL )
	 strcat( buf, "thin air??" );
    else if ( victim->fighting == ch )
	 strcat( buf, "YOU!" );
    else if ( victim->in_room == victim->fighting->in_room ) {
	 strcat( buf, PERS( victim->fighting, ch) );
	 strcat( buf, "." );
    }
    else
	 strcat( buf, "someone who left??" );
    break;
  }
  
  strcat( buf, "\n\r" );
  buf[0] = UPPER(buf[0]);
  send_to_char( buf, ch );
  return;
}



void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch, bool brief)
{
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *obj;
  int iWear;
  int percent;
  bool found;
  
/* Comment out: don't want this stuff
   if ( can_see( victim, ch ) ) {
   if (ch == victim)
   act( "$n looks at $mself.",ch,NULL,NULL,TO_ROOM);
   else {
   act( "$n looks at you.", ch, NULL, victim, TO_VICT    );
   act( "$n looks at $N.",  ch, NULL, victim, TO_NOTVICT );
   }
   }
*/
    
  if ( victim->description[0] != '\0' && !IS_HOODED(victim) && !IS_VEILED(victim) && !IS_WOLFSHAPE(victim)) {
    send_to_char( victim->description, ch );
  }
  else if (!IS_NPC(victim) && !IS_NULLSTR(victim->hood_description) && IS_HOODED(victim)) {
    send_to_char( victim->hood_description, ch);
  }
  else if (!IS_NPC(victim) && !IS_NULLSTR(victim->veil_description) && IS_VEILED(victim)) {
    send_to_char( victim->veil_description, ch);
  }  
  else if (!IS_NPC(victim) && !IS_NULLSTR(victim->wolf_description) && IS_WOLFSHAPE(victim)) {
     send_to_char( victim->wolf_description, ch);
  }
  else {
    act( "You see nothing special about $M.", ch, NULL, victim, TO_CHAR );
  }

  send_to_char("\n\r", ch);
  
  if ( MOUNTED(victim) ) {
    sprintf(buf, "%s is riding %s.\n\r", 
		  PERS(victim,ch), PERS( MOUNTED(victim),ch) );
    send_to_char( buf,ch);
  }
  if ( RIDDEN(victim) )  {
    sprintf(buf,"%s is being ridden by %s.\n\r", 
		  PERS(victim,ch), PERS( RIDDEN(victim),ch) );
    send_to_char( buf,ch);
  }
  
  if ( victim->max_hit > 0 )
    percent = ( 100 * victim->hit ) / victim->max_hit;
  else
    percent = -1;
  
  strcpy( buf, PERS(victim, ch) );
  
  if (percent >= 100) 
    strcat( buf, " is in excellent condition.\n\r");
  else if (percent >= 90) 
    strcat( buf, " has a few scratches.\n\r");
  else if (percent >= 75) 
    strcat( buf," has some small wounds and bruises.\n\r");
  else if (percent >=  50) 
    strcat( buf, " has quite a few wounds.\n\r");
  else if (percent >= 30)
    strcat( buf, " has some big nasty wounds and scratches.\n\r");
  else if (percent >= 15)
    strcat ( buf, " looks pretty hurt.\n\r");
  else if (percent >= 0 )
    strcat (buf, " is in awful condition.\n\r");
  else
    strcat(buf, " is bleeding to death.\n\r");
  
  buf[0] = UPPER(buf[0]);
  send_to_char( buf, ch );
  
  found = FALSE;
  if (!brief) {
    if (!IS_WOLFSHAPE(victim)) {
	 for ( iWear = 0; iWear < MAX_WEAR; iWear++ ) {
	   if ( ( obj = get_eq_char( victim, iWear ) ) != NULL &&   can_see_obj( ch, obj ) ) {
		if ( !found ) {
		  send_to_char( "\n\r", ch );
		  sprintf(buf, "%s is using:\n\r", PERS(victim, ch));
		  send_to_char(buf, ch);
		  found = TRUE;
		}
		if (!IS_IMMORTAL(ch)  && (IS_CLOAKED(victim) || IS_COLORCLOAKED(victim))) {
		  switch (iWear) {
		  case WEAR_ABOUT:
		  case WEAR_SHIELD:
		  case WEAR_HOLD:
		  case WEAR_WIELD:
		  case WEAR_SECOND_WIELD:
		  case WEAR_STUCK_IN:
		    break;
		  default:
		    continue;
		    break;
		  }
		}
		if (iWear == WEAR_SCABBARD_1 && !IS_NPC(victim) && !IS_NULLSTR(victim->sheat_where_name[0]))
		  send_to_char(victim->sheat_where_name[0], ch);
		else if (iWear == WEAR_SCABBARD_2 && !IS_NPC(victim) && !IS_NULLSTR(victim->sheat_where_name[1]))
		  send_to_char(victim->sheat_where_name[1], ch);
		else
		  send_to_char( where_name[iWear], ch );
		if (iWear == WEAR_STUCK_IN && victim->arrow_count > 1) {
		  char buf[MAX_STRING_LENGTH];
		  sprintf(buf,"(%d) ",victim->arrow_count);
		  send_to_char(buf,ch);
		}
		send_to_char( format_obj_to_char( obj, ch, TRUE, FALSE ), ch );
		send_to_char( "\n\r", ch );
	   }
	 }
    }
  }
  
  if(IS_IMMORTAL(ch) && (!brief)) { 
    if ( victim != ch
	    &&   !IS_NPC(ch)
	    &&   number_percent( ) < get_skill(ch,gsn_peek)) {
	 send_to_char( "\n\rYou peek at the inventory:\n\r", ch );
	 check_improve(ch,gsn_peek,TRUE,4);
	 show_list_to_char( victim->carrying, ch, TRUE, TRUE, TRUE );
    }
  }
  
  return;
}

void show_warder_to_char( CHAR_DATA *list, CHAR_DATA *ch, bool together )
{
  CHAR_DATA *rch;
  
  for ( rch = list; rch != NULL; rch = rch->next_in_room ) {
    if ((rch == ch) && (!IS_SET(ch->act,PLR_SEESELF)))
	 continue;

    if (!IS_SAME_WORLD(ch, rch) && !IS_SLEEPING(rch))
	 continue;

    // Warders that are color cloaked go into room desc
    if (!IS_COLORCLOAKED(rch))
	 continue;
    
    if ( get_trust(ch) < rch->invis_level)
	 continue;
    
    if( ( together && ( !ch->in_obj || rch->in_obj != ch->in_obj ) )
	   || ( !together && rch->in_obj ) )
	 continue;
    
    
    if ( can_see( ch, rch ) ) {
	 show_char_to_char_0( rch, ch );
    }
  }  
  return;
} 

void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch, bool together )
{
  CHAR_DATA *rch;
  
  for ( rch = list; rch != NULL; rch = rch->next_in_room ) {
    if ((rch == ch) && (!IS_SET(ch->act,PLR_SEESELF)))
	 continue;

    if (!IS_SAME_WORLD(ch, rch) && !IS_SLEEPING(rch))
	 continue;

    // Warders that are color cloaked go into room desc
    if (IS_COLORCLOAKED(rch))
	 continue;
    
    if ( get_trust(ch) < rch->invis_level)
	 continue;
    
    if( ( together && ( !ch->in_obj || rch->in_obj != ch->in_obj ) )
	   || ( !together && rch->in_obj ) )
	 continue;
    
    
    if ( can_see( ch, rch ) ) {
	 show_char_to_char_0( rch, ch );
    }
    else if ( room_is_dark( ch->in_room ) && IS_AFFECTED(rch, AFF_INFRARED ) ) {
	 send_to_char( "You see glowing red eyes watching YOU!\n\r", ch );
    }
  }  
  return;
} 



bool check_blind( CHAR_DATA *ch )
{

    if (!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT))
	return TRUE;

    if ( IS_AFFECTED(ch, AFF_BLIND) || IS_AFFECTED(ch, AFF_BLINDFOLDED))
    { 
	send_to_char( "You can't see a thing!\n\r", ch ); 
	return FALSE; 
    }

    return TRUE;
}

/* changes your scroll */
void do_scroll(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int lines;

    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
	if (ch->lines == 0)
	    send_to_char("You do not page long messages.\n\r",ch);
	else
	{
	    sprintf(buf,"You currently display %d lines per page.\n\r",
		    ch->lines + 2);
	    send_to_char(buf,ch);
	}
	return;
    }

    if (!is_number(arg))
    {
	send_to_char("You must provide a number.\n\r",ch);
	return;
    }

    lines = atoi(arg);

    if (lines == 0)
    {
        send_to_char("Paging disabled.\n\r",ch);
        ch->lines = 0;
        return;
    }

    if (lines < 10 || lines > 100)
    {
	send_to_char("You must provide a reasonable number.\n\r",ch);
	return;
    }

    sprintf(buf,"Scroll set to %d lines.\n\r",lines);
    send_to_char(buf,ch);
    ch->lines = lines - 2;
}

int compare_social_names(const void *v1, const void *v2)
{
  return strcmp((*(struct social_type*)v1).name, (*(struct social_type*)v2).name);
}

void do_socials(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  int iSocial;
  int col;
  int i=0;

  col = 0;
  
  for (iSocial = 0; !IS_NULLSTR(social_table[iSocial].name); iSocial++);

  qsort(social_table, iSocial, sizeof(struct social_type), compare_social_names);
  
  for (i=0; i < iSocial; i++) {
    sprintf(buf,"%-12s",social_table[i].name);
    send_to_char(buf,ch);
    if (++col % 6 == 0)
	 send_to_char("\n\r",ch);
  }
  
  if ( col % 6 != 0)
    send_to_char("\n\r",ch);
  return;
}

/* RT does socials */
void do_socials_OLD(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int iSocial;
    int col;
     
    col = 0;
   
    for (iSocial = 0; !IS_NULLSTR(social_table[iSocial].name); iSocial++)
    {
	sprintf(buf,"%-12s",social_table[iSocial].name);
	send_to_char(buf,ch);
	if (++col % 6 == 0)
	    send_to_char("\n\r",ch);
    }

    if ( col % 6 != 0)
	send_to_char("\n\r",ch);
    return;
}


 
/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "motd");
}

void do_imotd(CHAR_DATA *ch, char *argument)
{  
    do_function(ch, &do_help, "imotd");
}

void do_rules(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "rules");
}

void do_story(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "story");
}

void do_wizlist(CHAR_DATA *ch, char *argument)
{
    do_function(ch, &do_help, "wizlist");
}

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */

void do_autolist(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];

    /* lists most player flags */
    if (IS_NPC(ch))
      return;

    buf[0] = '\0';

    send_to_char("\n                  AUTOLIST\n\n\r", ch);
    send_to_char("Option         Status   Option         Status\n\r",ch);
    send_to_char("---------------------   ---------------------\n\r",ch);

    /*------------------------------------------------------*/
    /* COMBAT                                               */
    send_to_char("{yCombat:{x\n\r", ch);
    
    if (IS_SET(ch->act,PLR_AUTOASSIST))
      sprintf( buf, "%-14s\t", "Autoassist        {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autoassist        {gOFF{x");
    send_to_char(buf, ch);

    if (IS_SET(ch->act,PLR_AUTOGOLD))
      sprintf( buf, "%-14s\t", "Autogold          {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autogold          {gOFF{x");
    send_to_char(buf, ch);

    send_to_char("\n\r",ch);

    if (IS_SET(ch->act,PLR_AUTOLOOT))
      sprintf( buf, "%-14s\t", "Autoloot          {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autoloot          {gOFF{x");
    send_to_char(buf, ch);

    if (IS_SET(ch->act,PLR_AUTOSAC))
      sprintf( buf, "%-14s\t", "Autosac           {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autosac           {gOFF{x");
    send_to_char(buf, ch);

    send_to_char("\n\r",ch);

    if (IS_SET(ch->act,PLR_AUTOSPLIT))
      sprintf( buf, "%-14s\t", "Autosplit         {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autosplit         {gOFF{x");
    send_to_char(buf, ch);

    if (IS_SET(ch->act,PLR_AUTOEXAMINE))
      sprintf( buf, "%-14s\t", "Autoexamine       {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autoexamine       {gOFF{x");
    send_to_char(buf, ch);    

    send_to_char("\n\r",ch);

    if (IS_SET(ch->auto_act, AUTO_PARRY))
	 sprintf( buf, "%-14s\t", "Autoparry         {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autoparry         {gOFF{x");
    send_to_char(buf, ch);

    if (IS_SET(ch->auto_act, AUTO_DODGE))
	 sprintf( buf, "%-14s\t", "Autododge         {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autododge         {gOFF{x");
    send_to_char(buf, ch);

    send_to_char("\n\r",ch);

    if (IS_SET(ch->auto_act, AUTO_SHIELDBLOCK))
	 sprintf( buf, "%-14s\t", "Autoshieldblock   {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autoshieldblock   {gOFF{x");
    send_to_char(buf, ch);

    if (IS_SET(ch->auto_act, AUTO_MASTERFORMS))
	 sprintf( buf, "%-14s\t", "Automasterforms   {RON{x");
    else
      sprintf( buf, "%-14s\t", "Automasterforms   {gOFF{x");
    send_to_char(buf, ch);

    send_to_char("\n\r",ch);

    if (IS_SET(ch->auto_act, AUTO_DRAW))
	 sprintf( buf, "%-14s\t", "Autodraw          {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autodraw          {gOFF{x");
    send_to_char(buf, ch);

    if (IS_SET(ch->auto_act, AUTO_BIND))
	 sprintf( buf, "%-14s\t", "Autobind          {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autobind          {gOFF{x");
    send_to_char(buf, ch);
    send_to_char("\n\r",ch);
    if (IS_SET(ch->auto_act, AUTO_BLINDFOLD))
	 sprintf( buf, "%-14s\t", "Autoblindfold     {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autoblindfold     {gOFF{x");
    send_to_char(buf, ch);
    
    send_to_char("\n\n\r",ch);

    /*------------------------------------------------------*/
    /* CHANNELING:                                          */
    if (IS_IMMORTAL(ch) || (ch->class == CLASS_CHANNELER)) {
       send_to_char("{yChanneling:{x\n\r", ch);

       if (IS_SET(ch->act,PLR_AUTOSLICE))
         sprintf( buf, "%-14s\t", "Autoslice         {RON{x");
       else
         sprintf( buf, "%-14s\t", "Autoslice         {gOFF{x");
       send_to_char(buf, ch);
       
       if (!IS_NPC(ch) && ch->autoholding != 0)
          sprintf( buf, "%-14s\t", "Autochannel       {RON{x");
       else
          sprintf( buf, "%-14s\t", "Autochannel       {gOFF{x");
       send_to_char(buf, ch);
   
       send_to_char("\n\r",ch);
    
       if (IS_SET(ch->chan_flags, CHAN_SEECHANNELING))
         sprintf( buf, "%-14s\t", "Seechanneling     {RON{x");
       else
         sprintf( buf, "%-14s\t", "Seechanneling     {gOFF{x");
       send_to_char(buf, ch);   
   
       send_to_char("\n\n\r",ch);
    }

    /*------------------------------------------------------*/
    /* TOGGLES:                                             */
    send_to_char("{yToggle:{x\n\r", ch);

    if (IS_SET(ch->act,PLR_AUTOEXIT))
      sprintf( buf, "%-14s\t", "Autoexit          {RON{x");
    else
      sprintf( buf, "%-14s\t", "Autoexit          {gOFF{x");
    send_to_char(buf, ch);
    
    if (IS_SET(ch->act,PLR_SEESELF))
      sprintf( buf, "%-14s\t", "Seeself           {RON{x");
    else
      sprintf( buf, "%-14s\t", "Seeself           {gOFF{x");
    send_to_char(buf, ch);

    send_to_char("\n\r",ch);

    if (IS_SET(ch->comm,COMM_COMPACT))
      sprintf( buf, "%-14s\t", "Compact mode      {RON{x");
    else
      sprintf( buf, "%-14s\t", "Compact mode      {gOFF{x");
    send_to_char(buf, ch);

    if (IS_SET(ch->comm,COMM_PROMPT))
      sprintf( buf, "%-14s\t", "Prompt            {RON{x");
    else
      sprintf( buf, "%-14s\t", "Prompt            {gOFF{x");
    send_to_char(buf, ch);

    send_to_char("\n\r",ch);

    if (IS_SET(ch->comm,COMM_COMBINE))
      sprintf( buf, "%-14s\t", "Combine items     {RON{x");
    else
      sprintf( buf, "%-14s\t", "Combine items     {gOFF{x");
    send_to_char(buf, ch);

    if (IS_SET(ch->act,PLR_NOFOLLOW))
      sprintf( buf, "%-14s\t", "Nofollow          {RON{x");
    else
      sprintf( buf, "%-14s\t", "Nofollow          {gOFF{x");
    send_to_char(buf, ch);

    send_to_char("\n\r",ch);
 
   if (IS_SET(ch->app, APP_CLOAKED))
	 sprintf( buf, "%-14s\t", "Cloaked           {RON{x");
    else
      sprintf( buf, "%-14s\t", "Cloaked           {gOFF{x");
    send_to_char(buf, ch);

    if (IS_SET(ch->app, APP_HOODED))
	 sprintf( buf, "%-14s\t", "Hooded            {RON{x");
    else
      sprintf( buf, "%-14s\t", "Hooded            {gOFF{x");
    send_to_char(buf, ch);

    send_to_char("\n\r",ch);
    
    if (IS_SET(ch->chan_flags, CHAN_SEEAREAWEAVES))
      sprintf( buf, "%-14s\t", "Seeareaweaves     {RON{x");
    else
      sprintf( buf, "%-14s\t", "Seeareaweaves     {gOFF{x");
    send_to_char(buf, ch);
    
    if (IS_AIEL(ch)) {
	 send_to_char("\n\r",ch);
	 if (IS_VEILED(ch))
	   sprintf( buf, "%-14s\t", "Veiled            {RON{x");
	 else
	   sprintf( buf, "%-14s\t", "Veiled            {gOFF{x");
	 send_to_char(buf, ch);    	
    }
    
    if (IS_WARDER(ch)) {
	 send_to_char("\n\r",ch);
	 if (IS_COLORCLOAKED(ch))
	   sprintf( buf, "%-14s\t", "Color Cloaked     {RON{x");
	 else
	   sprintf( buf, "%-14s\t", "Color Cloaked     {gOFF{x");
	 send_to_char(buf, ch);
    }
    
    if (IS_SET(ch->auto_act, AUTO_VOTEREMINDER))
	 sprintf( buf, "%-14s\r\n", "AutoVoteReminder  {RON{x");
    else
         sprintf( buf, "%-14s\r\n", "AutoVoteReminder  {gOFF{x");
    send_to_char(buf, ch);
    
    send_to_char("\n\n\r",ch);

/*
    if (!IS_SET(ch->act,PLR_CANLOOT))
      send_to_char("Your corpse is safe from thieves.\n\r",ch);
    else 
      send_to_char("Your corpse may be looted.\n\r",ch);
    
    if (IS_SET(ch->act,PLR_NOSUMMON))
      send_to_char("You cannot be summoned.\n\r",ch);
    else
      send_to_char("You can be summoned.\n\r",ch);
*/      
}

void do_hoodappearance(CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char hood_app[MAX_STRING_LENGTH];

  if ( IS_NPC(ch) )
    return;

  if (IS_IMMORTAL(ch)) {
    send_to_char("Immortals should not go about sneaking.\n\r", ch);
    return;
  }

  if (IS_NULLSTR(argument)) {
    sprintf(buf, "Your hood appearance is currently: %s\n\r", IS_NULLSTR(ch->pcdata->hood_appearance) ? "(none)" : ch->pcdata->hood_appearance);
    send_to_char(buf, ch);
    return;
  }

  if (IS_HOODED(ch)) {
    send_to_char("You can't change your hood appearance while hooded.\n\r", ch);
    return;
  }
  
  if ((strstr(argument, "hood") == NULL &&
	  strstr(argument, "hooded") == NULL ) ||
	 (strstr(argument, "figure") == NULL &&
	  strstr(argument, "beeing") == NULL)) {
    send_to_char("Please read '{Whelp hood{x' for required keywords.\n\r", ch);
    return;
  }

  if (strlen(argument) > 35 || strlen(argument) < 5) {
    send_to_char("Hood appearance must be between 5 and 35 characters long (no colors allowed).\n\r", ch);
    return;
  }

  free_string( ch->pcdata->hood_appearance );
  smash_tilde( argument );
  sprintf(hood_app, "%s", colorstrem(argument));
  ch->pcdata->hood_appearance = str_dup( hood_app );

  sprintf(buf, "You set your hood appearance to: %s\n\r", ch->pcdata->hood_appearance);
  send_to_char(buf, ch);

  send_to_char("Make sure you understand every line in '{Whelp hood{x'.\n\r", ch);

  sprintf(buf,"%s set hood appearance to: %s",ch->name,ch->pcdata->hood_appearance);
  wiznet(buf,ch,NULL,WIZ_ON,WIZ_SECURE,0);
  
  return;  
}

void do_hood(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;

  /* NPC can't hood for now */
  if (IS_NPC(ch))
    return;

  /* Can't if wolfshape */
  if (IS_IMMORTAL(ch)) {
    send_to_char("Immortals should not go about sneaking.\n\r", ch);
    return;
  }

  if (IS_WOLFSHAPE(ch)) {
     send_to_char("You need to wear a cloak to be able to hood your self.\n\r", ch);
     return;
  }
  
  if (IS_AIEL(ch)) {
     send_to_char("Aiels needs to use the veil to conceal their face.\n\r", ch);
     return;  	
  }

  if (IS_VEILED(ch)) {
    send_to_char("You are already veiled.\n\r", ch);
    return;  
  }

  /* check that realy have a cloak */
  if ((obj = get_eq_char(ch, WEAR_ABOUT)) != NULL) {
    if(!IS_NULLSTR(obj->short_descr)) {
	 if ( (strstr(colorstrem(obj->short_descr), "cloak") == NULL) &&
		 (strstr(colorstrem(obj->short_descr), "Cloak") == NULL) ){
	   send_to_char("You need to wear a cloak to be able to hood your self.\n\r", ch);
	   return;
	 }
    }
  }
  else {
    send_to_char("You need to wear a cloak to be able to hood your self.\n\r", ch);
    return;
  }

  if (IS_NULLSTR(ch->pcdata->hood_appearance)) {
    send_to_char("You need to set an hood appearance first.\n\r", ch);
    return;
  }

  if (IS_SET(ch->app,APP_HOODED)) {
    send_to_char("You push back the hood of your cloak.\n\r",ch);
    act("$n pushes back the hood of $s cloak and reveals $s face.", ch, NULL, NULL, TO_ROOM);
    REMOVE_BIT(ch->app,APP_HOODED);
  }
  else {
    send_to_char("You put hood of your cloak on your head.\n\r", ch);
    act("$n raises the hood of $s cloak and conceals $s face.", ch, NULL, NULL, TO_ROOM);
    SET_BIT(ch->app,APP_HOODED);
  }
  
}

bool can_use_masquerade(CHAR_DATA *ch)
{
  extern MASQUERADE_TYPE masquerade;
  
  if (!masquerade.on)
    return FALSE;

  // Race check
  if (masquerade.race != 0) {
    if (masquerade.race != ch->race)
	 return FALSE;
  }
  
  // Room check
  if (masquerade.vnum != 0) {
    if (masquerade.vnum != ch->in_room->vnum)
	 return FALSE;
  }
  
  // Town check
  if (masquerade.anum != 0) {
    if (masquerade.anum != ch->in_room->area->vnum)
	 return FALSE;
  }

  // All OK
  return TRUE;
}

void do_maskappearance(CHAR_DATA *ch, char *argument )
{
  char buf[MSL];
  char masq_app[MSL];

  if ( IS_NPC(ch) )
    return;

  if (!can_use_masquerade(ch)) {
    send_to_char("You don't fulfill the requirements to use masquerade, now.\n\r", ch);
    return;
  }
  
  if (IS_NULLSTR(argument)) {
    sprintf(buf, "Your masquerade appearance is currently: %s\n\r", IS_NULLSTR(ch->pcdata->masquerade_appearance) ? "(none)" : ch->pcdata->masquerade_appearance);
    send_to_char(buf, ch);
    return;
  }

  if (IS_MASQUERADED(ch)) {
    send_to_char("You can't change your masquerade appearance while masked.\n\r", ch);
    return;
  }

  if (colorstrlen(argument) > 50 || colorstrlen(argument) < 5) {
    send_to_char("Masquerade appearance must be between 5 and 50 characters long.\n\r", ch);
    return;
  }
  
  free_string( ch->pcdata->masquerade_appearance );
  smash_tilde( argument );
  sprintf(masq_app, "%s", argument);
  ch->pcdata->masquerade_appearance = str_dup( masq_app );
  
  sprintf(buf, "You set your masquerade appearance to: %s\n\r", ch->pcdata->masquerade_appearance);
  send_to_char(buf, ch);
  
  sprintf(buf,"%s set masquerade appearance to: %s",ch->name,ch->pcdata->masquerade_appearance);
  wiznet(buf,ch,NULL,WIZ_ON,WIZ_SECURE,0);
  
  return;  
  
}

void do_mask(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;
  //char buf[MSL];

  /* NPC can't hood for now */
  if (IS_NPC(ch))
    return;

  if (!can_use_masquerade(ch)) {
    send_to_char("You don't fulfill the requirements to use masquerade, now.\n\r", ch);
    return;
  }

  if (IS_WOLFSHAPE(ch)) {
    send_to_char("You can not use mask while in wolf shape.\n\r", ch);
    return;
  }
  
  if (IS_HOODED(ch)) {
    send_to_char("You are already hooded.\n\r", ch);
    return;  	  	
  }

  if (IS_VEILED(ch)) {
    send_to_char("You are already veiled.\n\r", ch);
    return; 
  }
  
  if (IS_NULLSTR(ch->pcdata->masquerade_appearance)) {
    send_to_char("You need to set an masquerade appearance first.\n\r", ch);
    return;
  }
  
  if (IS_MASQUERADED(ch)) {

    send_to_char("You remove the mask from your face.\n\r", ch);
    act("$n remove the mask from $s face.", ch, obj, NULL, TO_ROOM);
    REMOVE_BIT(ch->app,APP_MASQUERADE);
  }
  else {
    send_to_char("You put a mask on your face.\n\r", ch);
    act("$n put a mask on $s face.", ch, obj, NULL, TO_ROOM);
    SET_BIT(ch->app,APP_MASQUERADE);
  }
  
}

void do_veilappearance(CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char veil_app[MAX_STRING_LENGTH];

  if ( IS_NPC(ch) )
    return;

  if (IS_IMMORTAL(ch)) {
    send_to_char("Immortals should not go about sneaking.\n\r", ch);
    return;
  }
  
  if (!IS_AIEL(ch) && !IS_TARABONER(ch)) {
    send_to_char("You are not one of the Aiels.\n\r", ch);
    return;  	
  }
  
  if (IS_NULLSTR(argument)) {
    sprintf(buf, "Your veil appearance is currently: %s\n\r", IS_NULLSTR(ch->pcdata->veil_appearance) ? "(none)" : ch->pcdata->veil_appearance);
    send_to_char(buf, ch);
    return;
  }
  
  if (IS_VEILED(ch)) {
    send_to_char("You can't change your veil appearance while veiled.\n\r", ch);
    return;
  }
  
  if ((strstr(argument, "veil") == NULL &&
	  strstr(argument, "veiled") == NULL ) ||
	 (strstr(argument, "figure") == NULL &&
	  strstr(argument, "aiel") == NULL && 
	  strstr(argument, "Aiel") == NULL)) {
    send_to_char("Please read '{Whelp veil{x' for required keywords.\n\r", ch);
    return;
  }
  
  if (strlen(argument) > 35 || strlen(argument) < 5) {
    send_to_char("Veil appearance must be between 5 and 35 characters long (no colors allowed).\n\r", ch);
    return;
  }
  
  free_string( ch->pcdata->veil_appearance );
  smash_tilde( argument );
  sprintf(veil_app, "%s", colorstrem(argument));
  ch->pcdata->veil_appearance = str_dup( veil_app );
  
  sprintf(buf, "You set your veil appearance to: %s\n\r", ch->pcdata->veil_appearance);
  send_to_char(buf, ch);
  
  send_to_char("Make sure you understand every line in '{Whelp veil{x'.\n\r", ch);
  
  sprintf(buf,"%s set veil appearance to: %s",ch->name,ch->pcdata->veil_appearance);
  wiznet(buf,ch,NULL,WIZ_ON,WIZ_SECURE,0);
  
  return;  
}

void do_veil(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;
  char buf[MSL];

  /* NPC can't hood for now */
  if (IS_NPC(ch))
    return;

  /* Can't if wolfshape */
  if (IS_IMMORTAL(ch)) {
    send_to_char("Immortals should not go about sneaking.\n\r", ch);
    return;
  }

  if (IS_WOLFSHAPE(ch)) {
     send_to_char("You need to wear a shofa to be able to veil your self.\n\r", ch);
     return;
  }
  
  if (!IS_AIEL(ch) && !IS_TARABONER(ch)) {
    send_to_char("You are not one of the Aiels.\n\r", ch);
    return;  	
  }
  
  if (IS_HOODED(ch)) {
    send_to_char("You are already hooded.\n\r", ch);
    return;  	  	
  }
  
  /* check that realy have a shofa or veil */
  if ((obj = get_eq_char(ch, WEAR_HEAD)) != NULL) {
    if(!IS_NULLSTR(obj->short_descr)) {
	 if ((strstr(colorstrem(obj->short_descr), "shoufa") == NULL) && 
	     (strstr(colorstrem(obj->short_descr), "veil") == NULL)) {
	   send_to_char("You need to wear a shofa or veil to be able to veil your self.\n\r", ch);
	   return;
	 }
    }
  }
  else {
    send_to_char("You need to wear a shofa or veil to be able to veil your self.\n\r", ch);
    return;
  }

  if (IS_NULLSTR(ch->pcdata->veil_appearance)) {
    send_to_char("You need to set an veil appearance first.\n\r", ch);
    return;
  }

  if (IS_SET(ch->app,APP_VEILED)) {
    sprintf(buf, "You remove %s from your head.\n\r", obj->short_descr);
    send_to_char(buf,ch);
    act("$n opens $p around $s head and and reveals $s face.", ch, obj, NULL, TO_ROOM);
    REMOVE_BIT(ch->app,APP_VEILED);
  }
  else {
    sprintf(buf, "You rais %s around your face.\n\r", obj->short_descr);
    send_to_char(buf, ch);
    act("$n raises $p and conceals $s face.", ch, obj, NULL, TO_ROOM);
    SET_BIT(ch->app,APP_VEILED);
  }
  
}

void do_cloak(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;

  /* NPC can't cloak for now */
  if (IS_NPC(ch))
    return;

  /* Can't if wolfshape */
  if (IS_WOLFSHAPE(ch)) {
     send_to_char("You need to wear a cloak to be able to hood your self.\n\r", ch);
     return;
  }

  if (IS_COLORCLOAKED(ch)) {
    send_to_char("You are already colorcloaked.\n\r", ch);
    return;
  }
  
  /* check that realy have a cloak */
  if ((obj = get_eq_char(ch, WEAR_ABOUT)) != NULL) {
    if(!IS_NULLSTR(obj->short_descr)) {
	 if (strstr(colorstrem(obj->short_descr), "cloak") == NULL) {
	   send_to_char("You need to wear a cloak to be able to cloak your self.\n\r", ch);
	   return;
	 }
    }
  }
  else {
    send_to_char("You need to wear a cloak to be able to cloak your self.\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->app,APP_CLOAKED)) {
    send_to_char("You open the cloak and reveal what was hidden under it.\n\r",ch);
    REMOVE_BIT(ch->app,APP_CLOAKED);
    act("$n opens $s cloak and reveal $s equipment.", ch, NULL, NULL, TO_ROOM);
  }
  else {
    send_to_char("You spread the cloak around your self and your equipment.\n\r", ch);    
    SET_BIT(ch->app,APP_CLOAKED);
    act("$n closes $s cloak and hides $s equipment.", ch, NULL, NULL, TO_ROOM);
  }
}

void do_colorcloak(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;
  
  /* NPC can't cloak for now */
  if (IS_NPC(ch))
    return;

  /* Only warders */
  if (!IS_WARDER(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  /* Can't if wolfshape */
  if (IS_WOLFSHAPE(ch)) {
    send_to_char("Not while in wolf shape\n\r", ch);
    return;
  }

  /* Make sure not already cloaked */
  if (IS_CLOAKED(ch)) {
    send_to_char("You are already using a normal cloak.\n\r", ch);
    return;
  }

  /* check that realy have a cloak */
  if ((obj = get_eq_char(ch, WEAR_ABOUT)) != NULL) {
    if(!IS_NULLSTR(obj->short_descr)) {
	 if ( (strstr(colorstrem(obj->short_descr), "cloak") == NULL) &&
	      (strstr(colorstrem(obj->short_descr), "Cloak") == NULL) &&
	      (strstr(colorstrem(obj->short_descr), "fancloth") == NULL) &&	    
	      (strstr(colorstrem(obj->short_descr), "Fancloth") == NULL)	      
	    ) {
	   send_to_char("You need to wear a fancloth cloak to be able to color cloak your self.\n\r", ch);
	   return;
	 }
    }
  }
  else {
    send_to_char("You need to wear a fancloth cloak to be able to color cloak your self.\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->app,APP_COLORCLOAKED)) {
    send_to_char("You open the fancloth cloak and remove your abilities to blend into the surroundings.\n\r",ch);
    REMOVE_BIT(ch->app,APP_COLORCLOAKED);
    act("$n opens $s color-shifting cloak and reveal $m self.", ch, NULL, NULL, TO_ROOM);
  }
  else {
    send_to_char("You spread the fancloth cloak around your self and your equipment, and blend into the surroundings.\n\r", ch);
    SET_BIT(ch->app,APP_COLORCLOAKED);
    act("$n closes $s color-shifting cloak and blend into the surroundings.", ch, NULL, NULL, TO_ROOM);
  }
}

void do_seeself(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;
  
  if (IS_SET(ch->act,PLR_SEESELF)) {
    send_to_char("You will no longer see your self in the room.\n\r",ch);
    REMOVE_BIT(ch->act,PLR_SEESELF);
  }
  else {
    send_to_char("You will now see your self in the room.\n\r",ch);
    SET_BIT(ch->act,PLR_SEESELF);
  }
}

void do_autoparry(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;

  if (get_skill(ch,gsn_parry) <= 0) {
    send_to_char("You don't know how to parry yet.\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->auto_act, AUTO_PARRY)) {
    send_to_char("Autoparry removed.\n\r",ch);
    REMOVE_BIT(ch->auto_act,AUTO_PARRY);
  }
  else {
    send_to_char("You will now try to parry attacks.\n\r", ch);
    SET_BIT(ch->auto_act, AUTO_PARRY);
  }
}

void do_autododge(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;

  if (get_skill(ch,gsn_dodge) <= 0) {
    send_to_char("You don't know how to dodge yet.\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->auto_act, AUTO_DODGE)) {
    send_to_char("Auto dodge removed.\n\r",ch);
    REMOVE_BIT(ch->auto_act,AUTO_DODGE);
  }
  else {
    send_to_char("You will now try to dodge attacks.\n\r", ch);
    SET_BIT(ch->auto_act, AUTO_DODGE);
  }
}

void do_autoshieldblock(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;

  if (get_skill(ch,gsn_shield_block) <= 0) {
    send_to_char("You don't know how to use a shield to block attacks with yet.\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->auto_act, AUTO_SHIELDBLOCK)) {
    send_to_char("Auto shieldblock removed.\n\r",ch);
    REMOVE_BIT(ch->auto_act, AUTO_SHIELDBLOCK);
  }
  else {
    send_to_char("You will now try to use your shield to block attacks.\n\r", ch);
    SET_BIT(ch->auto_act, AUTO_SHIELDBLOCK);
  }
}

void do_automasterforms(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;
  
  if (!char_knows_masterform(ch) &&
      (get_skill(ch, gsn_trollocwarfare) <= 0)) {
    send_to_char("You have no idea about master forms.\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->auto_act, AUTO_MASTERFORMS)) {
    send_to_char("Auto masterforms removed.\n\r",ch);
    REMOVE_BIT(ch->auto_act,AUTO_MASTERFORMS);
  }
  else {
    send_to_char("You will now use master forms in combat.\n\r", ch);
    SET_BIT(ch->auto_act, AUTO_MASTERFORMS);
  }
}

void do_autodraw(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;
  
  if (IS_SET(ch->auto_act, AUTO_DRAW)) {
    send_to_char("Auto draw of sheathed weapons removed.\n\r",ch);
    REMOVE_BIT(ch->auto_act,AUTO_DRAW);
  }
  else {
    send_to_char("You will now auto draw sheathed weapons if attacked and not wielding.\n\r", ch);
    SET_BIT(ch->auto_act, AUTO_DRAW);
  }
}

void do_autobind(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;

  if (ch->pcdata->learned[gsn_bind] <= 0) {
    send_to_char("You have no idea about how to bind up someone...\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->auto_act, AUTO_BIND)) {
    send_to_char("Auto bind of surrending victims removed.\n\r",ch);
    REMOVE_BIT(ch->auto_act,AUTO_BIND);
  }
  else {
    send_to_char("You will now try to auto bind victims who surrender to you in combat.\n\r", ch);
    SET_BIT(ch->auto_act, AUTO_BIND);
  }
}

void do_autoblindfold(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;

  if (IS_SET(ch->auto_act, AUTO_BLINDFOLD)) {
    send_to_char("Auto blindfold of surrending victims removed.\n\r",ch);
    REMOVE_BIT(ch->auto_act,AUTO_BLINDFOLD);
  }
  else {
    send_to_char("You will now try to auto blindfold victims who surrender to you in combat.\n\r", ch);
    SET_BIT(ch->auto_act, AUTO_BLINDFOLD);
  }
}

void do_autovotereminder(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;
  if (IS_SET(ch->auto_act, AUTO_VOTEREMINDER)) {
    send_to_char("A reminder of when you can vote will no longer appear on your prompt.\n\r",ch);
    REMOVE_BIT(ch->auto_act,AUTO_VOTEREMINDER);
  }
  else {
    send_to_char("A reminder of when you can vote will now appear on your prompt.\n\r", ch);
    SET_BIT(ch->auto_act, AUTO_VOTEREMINDER);
  }

}

void do_autoslice(CHAR_DATA *ch, char *argument)
{
  if (IS_NPC(ch))
    return;

  if (!IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char("You cannot channel the true source.\n\r", ch);
    return;
  }
  
  if (IS_SET(ch->act,PLR_AUTOSLICE)) {
    send_to_char("Autoslice removed.\n\r",ch);
    REMOVE_BIT(ch->act,PLR_AUTOSLICE);
  }
  else {
    send_to_char("You will now try to slice weaves when possible.\n\r",ch);
    SET_BIT(ch->act,PLR_AUTOSLICE);
  }
}

void do_autoassist(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
    
    if (IS_SET(ch->act,PLR_AUTOASSIST))
    {
      send_to_char("Autoassist removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOASSIST);
    }
    else
    {
      send_to_char("You will now assist when needed.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOASSIST);
    }
}

void do_autoexit(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOEXIT))
    {
      send_to_char("Exits will no longer be displayed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOEXIT);
    }
    else
    {
      send_to_char("Exits will now be displayed.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOEXIT);
    }
}

void do_autogold(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOGOLD))
    {
      send_to_char("Autogold removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOGOLD);
    }
    else
    {
      send_to_char("Automatic gold looting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOGOLD);
    }
}

void do_autoloot(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOLOOT))
    {
      send_to_char("Autolooting removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOLOOT);
    }
    else
    {
      send_to_char("Automatic corpse looting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOLOOT);
    }
}

void do_autoexamine(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOEXAMINE))
    {
      send_to_char("Autoexamine corpse removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOEXAMINE);
    }
    else
    {
      send_to_char("Automatic examining corpse set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOEXAMINE);
    }
}

void do_autosac(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOSAC))
    {
      send_to_char("Autosacrificing removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOSAC);
    }
    else
    {
      send_to_char("Automatic corpse sacrificing set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOSAC);
    }
}

void do_autosplit(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOSPLIT))
    {
      send_to_char("Autosplitting removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOSPLIT);
    }
    else
    {
      send_to_char("Automatic gold splitting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOSPLIT);
    }
}

void do_brief(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_BRIEF))
    {
      send_to_char("Full descriptions activated.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_BRIEF);
    }
    else
    {
      send_to_char("Short descriptions activated.\n\r",ch);
      SET_BIT(ch->comm,COMM_BRIEF);
    }
}

void do_compact(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMPACT))
    {
      send_to_char("Compact mode removed.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_COMPACT);
    }
    else
    {
      send_to_char("Compact mode set.\n\r",ch);
      SET_BIT(ch->comm,COMM_COMPACT);
    }
}

/* Not in use anymore - Swordfish
void do_show(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_SHOW_AFFECTS))
    {
      send_to_char("Affects will no longer be shown in score.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_SHOW_AFFECTS);
    }
    else
    {
      send_to_char("Affects will now be shown in score.\n\r",ch);
      SET_BIT(ch->comm,COMM_SHOW_AFFECTS);
    }
}
*/

void do_prompt(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
 
  if ( argument[0] == '\0' ) {
    if (IS_SET(ch->comm,COMM_PROMPT)) {
	 send_to_char("You will no longer see prompts.\n\r",ch);
	 REMOVE_BIT(ch->comm,COMM_PROMPT);
    }
    else {
	 send_to_char("You will now see prompts.\n\r",ch);
	 SET_BIT(ch->comm,COMM_PROMPT);
    }
    return;
  }
 
  if( !strcmp( argument, "all" )) {
    if (ch->class == CLASS_CHANNELER)
	 strcpy( buf, "[{R%h{x/{R%H{xhp {G%m{x/{G%M{xen {W%p{x/{W%P{xop] ");
    else
	 strcpy( buf, "[{R%h{x/{R%H{xhp {G%m{x/{G%M{xen] ");
  }
  else {
    if ( colorstrlen(argument) > 80 ) {
	 //argument[50] = '\0';
       send_to_char("Max prompt length is 80 characters long (color codes not included).\n\r", ch);
       return;
    }
    strcpy( buf, argument );
    smash_tilde( buf );
    if (str_suffix("%c",buf))
	 strcat(buf," ");
    
  }
  
  free_string( ch->prompt );
  ch->prompt = str_dup( buf );
  sprintf(buf,"Prompt set to %s\n\r",ch->prompt );
  send_to_char(buf,ch);
  return;
}

void do_combine(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMBINE))
    {
      send_to_char("Long inventory selected.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_COMBINE);
    }
    else
    {
      send_to_char("Combined inventory selected.\n\r",ch);
      SET_BIT(ch->comm,COMM_COMBINE);
    }
}

void do_noloot(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_CANLOOT))
    {
      send_to_char("Your corpse is now safe from thieves.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_CANLOOT);
    }
    else
    {
      send_to_char("Your corpse may now be looted.\n\r",ch);
      SET_BIT(ch->act,PLR_CANLOOT);
    }
}

void do_nofinger(CHAR_DATA *ch, char *argument)
{
	if (IS_SET(ch->act2,PLR2_NOFINGER))
	{
      		REMOVE_BIT(ch->act2,PLR2_NOFINGER);
      		send_to_char("You can now be fingered.\n\r",ch);

	}
	else
	{
		send_to_char("You are now immune to fingering.\r\n",ch);
      		SET_BIT(ch->act2,PLR2_NOFINGER);

	}
}
void do_nofollow(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_NOFOLLOW))
    {
      send_to_char("You now accept followers.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_NOFOLLOW);
    }
    else
    {
      send_to_char("You no longer accept followers.\n\r",ch);
      SET_BIT(ch->act,PLR_NOFOLLOW);
      die_follower( ch );
    }
}

void do_nosummon(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    {
      if (IS_SET(ch->imm_flags,IMM_SUMMON))
      {
	send_to_char("You are no longer immune to summon.\n\r",ch);
	REMOVE_BIT(ch->imm_flags,IMM_SUMMON);
      }
      else
      {
	send_to_char("You are now immune to summoning.\n\r",ch);
	SET_BIT(ch->imm_flags,IMM_SUMMON);
      }
    }
    else
    {
      if (IS_SET(ch->act,PLR_NOSUMMON))
      {
        send_to_char("You are no longer immune to summon.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_NOSUMMON);
      }
      else
      {
        send_to_char("You are now immune to summoning.\n\r",ch);
        SET_BIT(ch->act,PLR_NOSUMMON);
      }
    }
}

bool room_has_wards(CHAR_DATA *ch)
{
  WARD_DATA *wd;
  ROOM_INDEX_DATA *room;
  
  /* Only Channelers can sense residues */
  if(ch->class != CLASS_CHANNELER)
    return FALSE;
  
  room = get_room_index(ch->in_room->vnum);
  
  for (wd = room->wards; wd != NULL; wd = wd->next) {
    if (wd->casterId == ch->id)
      return TRUE;
    if (wd->sex == ch->sex && IS_SAME_WORLD(wd, ch))
      return TRUE;
  }
  
  return FALSE;
} 

bool room_has_residues(CHAR_DATA *ch)
{
  RESIDUE_DATA *rd;
  ROOM_INDEX_DATA *room;

  /* Only Channelers can sense residues */
  if(ch->class != CLASS_CHANNELER)
    return FALSE;
  
  room = get_room_index(ch->in_room->vnum);
  
  for (rd = room->residues; rd != NULL; rd = rd->next) {
    if (rd->sex == ch->sex && IS_SAME_WORLD(rd, ch))
      return TRUE;
  }
  
  return FALSE;
}

void do_glance( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    //char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_STRING_LENGTH];

    argument = one_argument( argument, arg1 );
    if ( ch->desc == NULL )
	return;
    if ( ch->position < POS_SLEEPING )
    {
	send_to_char( "You can't see anything but stars!\n\r", ch );
	return;
    }
    if ( ch->position == POS_SLEEPING )
    {
	send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
	return;
    }
    if ( !check_blind( ch ) )
	return;

    if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT)
	   && room_is_dark( ch->in_room )
	   && !IS_AFFECTED(ch, AFF_INFRARED)) {
	 
	 if (ch->race == race_lookup("fade")) {
	   send_to_char( "{DThe room is pitch black, but you can feel every corner of it...{x\n\r", ch );
	   show_char_to_char( ch->in_room->people, ch, TRUE );
	 }
	 else {
	   send_to_char( "It is pitch black ... \n\r", ch );
	   show_char_to_char( ch->in_room->people, ch, TRUE );
	   return;
	 }
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
	show_char_to_char_1( victim, ch, TRUE );
	return;
    }
    else
    {
	send_to_char("They aren't here!\n\r",ch);
	return;
    }
}
void do_look( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    OBJ_DATA *above_exit_obj;
    char buffer [MAX_STRING_LENGTH];
    char *pdesc;
    int door;
    int number,count;
    int first=TRUE;
    char *pString;

    if ( ch->desc == NULL )
	return;

    if ( ch->position < POS_SLEEPING )
    {
	send_to_char( "You can't see anything but stars!\n\r", ch );
	return;
    }

    if ( ch->position == POS_SLEEPING )
    {
	send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
	return;
    }

    if ( !check_blind( ch ) )
	return;

    if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT)
	   && room_is_dark( ch->in_room )
	   && !IS_AFFECTED(ch, AFF_INFRARED)) {

	 if (ch->race == race_lookup("fade")) {
	   send_to_char( "{DThe room is pitch black, but you can feel every corner of it...{x\n\r", ch );
	   show_char_to_char( ch->in_room->people, ch, TRUE );
	 }
	 else if (ch->race == race_lookup("trolloc")) {
	   send_to_char( "{DThe room is pitch black, but you are able to see almost clear...{x\n\r", ch );
	   show_char_to_char( ch->in_room->people, ch, TRUE );
	 }	 	
	 else if (IS_WOLFKIN(ch)) {
	   send_to_char( "{YThe room is pitch black, but you are able to see almost clear...{x\n\r", ch );
	   show_char_to_char( ch->in_room->people, ch, TRUE );
	 }
	 else {
	   send_to_char( "It is pitch black ... \n\r", ch );
	   show_char_to_char( ch->in_room->people, ch, TRUE );
	   return;
	 }
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    number = number_argument(arg1,arg3);
    count = 0;

    if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
    {
	/* 'look' or 'look auto' */

        obj = ch->in_obj;
        if ( !(obj = ch->in_obj)
          || IS_SET( obj->value[1], CONT_SEE_OUT )
          || !IS_SET( obj->value[1], CONT_CLOSED ) )
        { 
    	   sprintf( buf, "%s", !IS_NULLSTR(ch->in_room->name) ? ch->in_room->name : "(none)" );
 	   send_to_char( buf, ch );
   
	   if ( (IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act,PLR_HOLYLIGHT)))
	   ||   IS_BUILDER(ch, ch->in_room->area) )
	   {
 	       /* sprintf(buf," {r[{RRoom %d{r]",ch->in_room->vnum); */
               sprintf(buf," {x[{qRoom %d{x] [{q%s{x]",ch->in_room->vnum,
                       ch->in_room->area->name);
	       send_to_char(buf,ch);
	   }

	   if (IS_SET(ch->in_room->room_flags, ROOM_MINING_GOLD) || IS_SET(ch->in_room->room_flags, ROOM_MINING_SILVER) || IS_SET(ch->in_room->room_flags, ROOM_MINING_COPPER) || IS_SET(ch->in_room->room_flags, ROOM_MINING_DIAMOND) || IS_SET(ch->in_room->room_flags, ROOM_MINING_RUBY) || IS_SET(ch->in_room->room_flags, ROOM_MINING_EMERALD)) {
		send_to_char( " {Y$$$$$${x", ch);
	   }
 	   send_to_char( "{x\n\r", ch );

      if ((ch->in_room->sector_type != SECT_INSIDE) && 
          (IS_SET(ch->in_room->room_flags, ROOM_VMAP))) {
         send_to_char( "{x\n\r", ch );
         do_function(ch, &do_map, "8");
         send_to_char( "{x\n\r", ch );
      }

	   if ( arg1[0] == '\0'
	   || ( !IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF) ) )
	   {
 	       sprintf( buf, "{q  %s{x", ch->in_room->description );
 	       send_to_char( buf, ch );
	   }

	   // Any warders in the room, show em in the description part
	   show_warder_to_char(ch->in_room->people,   ch, FALSE );

/* Objects above EXIT go in here */
   
      for ( above_exit_obj = ch->in_room->contents; above_exit_obj != NULL; above_exit_obj = above_exit_obj->next_content ) {
	      if (above_exit_obj->item_type ==   ITEM_FOUNTAIN  ||
             above_exit_obj->item_type ==   ITEM_FURNITURE) {

		   /*  Portals are kept under Exit line                       */
             /* above_exit_obj->item_type ==   ITEM_PORTAL) { */
            if (IS_SET(above_exit_obj->extra_flags,ITEM_HIDDEN) && !IS_IMMORTAL(ch)) {
	       if ((((get_skill(ch,gsn_observation) + get_skill(ch,gsn_alertness) + get_skill(ch,gsn_awareness)) / 3) /4) < number_percent())
               {
               	continue;
	       }
            }
            if (first) {
               send_to_char("\n\r",ch);
            }
            if (IS_SET(above_exit_obj->extra_flags,ITEM_HIDDEN)) {
               sprintf(buffer, "{Y(h){x %s\n\r", above_exit_obj->description);
               pString = format_string(str_dup(buffer));
               send_to_char(pString, ch);
            }
            else {
	       sprintf(buffer, "%s\n\r", above_exit_obj->description);
               pString = format_string(str_dup(buffer));
               send_to_char(pString, ch);
            }
            first=FALSE;
	      }
      }

      if (room_has_residues(ch))
	   send_to_char("\n{CYou feel residues of flows recently woven here!{x\n\r", ch);
	 
//	 if (room_has_wards(ch))
//	   send_to_char("\n{RYou sense a ward have been set in place here!{x\n\r", ch);
		
      if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT) ) {
	       send_to_char("\n\r",ch);
          do_function(ch, &do_exits, "auto" );
	   }

	 send_to_char("{O",ch);
	 show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE, FALSE );
	 send_to_char("{P",ch);
	 show_char_to_char( ch->in_room->people,   ch, FALSE );
	 send_to_char("{x",ch);
       }
       if (ch->in_obj)
       {
         sprintf( buf, "You see the inside of %s.\n\r", obj->short_descr );
         send_to_char(buf, ch);
         pdesc = get_extra_descr("inside_description",
                                 obj->pIndexData->extra_descr);
         if( pdesc )
         {
          send_to_char(pdesc, ch);
         }
         show_list_to_char(obj->contains, ch, FALSE, FALSE, FALSE);
         show_char_to_char(ch->in_room->people, ch, TRUE);
         return;
       }
      return;
    }

    if ( !str_cmp( arg1, "i" ) || !str_cmp(arg1, "in")  || !str_cmp(arg1,"on"))
    {
	/* 'look in' */
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Look in what?\n\r", ch );
	    return;
	}

	if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You do not see that here.\n\r", ch );
	    return;
	}

	switch ( obj->item_type )
	{
	default:
	    send_to_char( "That is not a container.\n\r", ch );
	    break;

   case ITEM_VEHICLE:
   case ITEM_PORTAL:
      act("$n peers intently into $p.",ch,obj,NULL,TO_ROOM);
      act("You peer into $p.",ch,obj,NULL,TO_CHAR);

      /* Normal exit shows the proper exit */
      if ( IS_SET(obj->value[2], GATE_NORMAL_EXIT)
      &&  !IS_SET(obj->value[1], EX_CLOSED) )
      location = get_room_index(obj->value[3]);

      /*  Buggy exit shows a random exit anywhere in world
       *  gate's exit not necessarily the shown exit
       */
      else if ( IS_SET(obj->value[2], GATE_BUGGY)
      &&  !IS_SET(obj->value[1], EX_CLOSED) )
      location = get_random_room(ch, TRUE);
        
      /*  Closed or other exit flags */
      else {
        act("You try to look into $p, but something is blocking your from seeing through it.", 
		  ch, obj, NULL, TO_CHAR);
        return;
	 }

     act("Through $p you see:", ch, obj, NULL, TO_CHAR);
     /* show room desc without the room name or room vnum (IMM) */ 
     sprintf(buf,"{w%s{x",location->description);
     send_to_char(buf,ch);

    /* show exit location. show objects and people in the room */
    send_to_char("\n\r", ch);

    original = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    do_function(ch, &do_exits, "auto" );
    char_from_room( ch );
    char_to_room( ch, original);

    show_list_to_char( location->contents, ch, FALSE, FALSE, FALSE );
    show_char_to_char( location->people,   ch, FALSE);
    return;
    break;

	case ITEM_DRINK_CON:
	    if ( obj->value[1] <= 0 )
	    {
		send_to_char( "It is empty.\n\r", ch );
		break;
	    }

	    sprintf( buf, "It's %sfilled with  a %s liquid.\n\r",
		obj->value[1] <     obj->value[0] / 4
		    ? "less than half-" :
		obj->value[1] < 3 * obj->value[0] / 4
		    ? "about half-"     : "more than half-",
		liq_table[obj->value[2]].liq_color
		);

	    send_to_char( buf, ch );
	    break;

	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
            if( ch->in_obj == obj )
            {
               send_to_char("Just type look.\n\r", ch);
               break;
            }

	    if ( IS_SET(obj->value[1], CONT_CLOSED) )
	    {
		send_to_char( "It is closed.\n\r", ch );
		break;
	    }
            pdesc = get_extra_descr("inside_description",
                                    obj->pIndexData->extra_descr);
            if( pdesc )
             send_to_char(pdesc, ch);

            act("You look inside $p and see:", ch, obj, NULL, TO_CHAR);
            if( IS_SET( obj->value[1], CONT_ENTERABLE ) && obj->who_in )
             show_list_to_char(obj->contains, ch, TRUE, FALSE, FALSE);
            else
             show_list_to_char(obj->contains, ch, TRUE, TRUE, obj->carried_by == NULL ? TRUE : FALSE);
            if( obj->who_in )
            {
               OBJ_DATA *in_obj = ch->in_obj;
               char_to_obj(ch, obj);
               show_char_to_char(ch->in_room->people, ch, TRUE);
               char_from_obj(ch);
               ch->in_obj = in_obj;
            }

	    break;
	}
	return;
    }

   if (!str_cmp(arg1, "o") || !str_cmp(arg1, "out"))
   {
   /* 'look out' */
      if (!ch->in_obj && !ch->in_room->inside_of)
      {
         send_to_char("You would have to be in something to do this.\n\r", ch);
         return;
      }

      if (ch->in_obj)
      {
         obj = ch->in_obj;
         if( IS_SET( obj->value[1], CONT_CLOSED )
          && !IS_SET( obj->value[1], CONT_SEE_OUT ) )
         {
            send_to_char("It is closed.\n\r", ch);
            return;
         }
         char_from_obj(ch);
         if (obj->in_obj == NULL)
            act("You look out of $p and you see:", ch, obj, NULL, TO_CHAR);
         else
            act("You look out of $p:", ch, obj, obj, TO_CHAR);
         do_look(ch, "");
         char_to_obj(ch, obj);
         return;
     }
     else
     {
	do_hack_look(ch,"auto");
     }
   }


    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
	show_char_to_char_1( victim, ch, FALSE );
	return;
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
	if ( can_see_obj( ch, obj ) )
	{  /* player can see object */
	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    {	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}
	    	else continue;
            }
 	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
 	    if ( pdesc != NULL )
 	    { 	if (++count == number)
 	    	{	
		    send_to_char( pdesc, ch );
		    return;
	     	}
		else continue;
	    }
	    if ( is_name( arg3, obj->name ) )
	    	if (++count == number)
	    	{
	    	    send_to_char( obj->description, ch );
	    	    send_to_char( "\n\r",ch);
		    return;
		  }
	  }
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
	if ( can_see_obj( ch, obj ) )
	{
	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    if ( is_name( arg3, obj->name ) )
		if (++count == number)
		{
		    send_to_char( obj->description, ch );
		    send_to_char("\n\r",ch);
		    return;
		}
	}
    }

if ( ch->in_obj )
    for ( obj = ch->in_obj->contains; obj != NULL; obj = obj->next_content )
    {
        
	if ( can_see_obj( ch, obj ) )
	{
	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    if ( is_name( arg3, obj->name ) )
		if (++count == number)
		{
		    send_to_char( obj->description, ch );
		    send_to_char("\n\r",ch);
		    return;
		}
	}
    }

    pdesc = get_extra_descr(arg3,ch->in_room->extra_descr);
    if (pdesc != NULL)
    {
	if (++count == number)
	{
	    send_to_char(pdesc,ch);
	    return;
	}
    }
    
    if (count > 0 && count != number)
    {
    	if (count == 1)
    	    sprintf(buf,"You only see one %s here.\n\r",arg3);
    	else
    	    sprintf(buf,"You only see %d of those here.\n\r",count);
    	
    	send_to_char(buf,ch);
    	return;
    }

         if ( !str_cmp( arg1, "n" ) || !str_cmp( arg1, "north" ) ) door = 0;
    else if ( !str_cmp( arg1, "e" ) || !str_cmp( arg1, "east"  ) ) door = 1;
    else if ( !str_cmp( arg1, "s" ) || !str_cmp( arg1, "south" ) ) door = 2;
    else if ( !str_cmp( arg1, "w" ) || !str_cmp( arg1, "west"  ) ) door = 3;
    else if ( !str_cmp( arg1, "u" ) || !str_cmp( arg1, "up"    ) ) door = 4;
    else if ( !str_cmp( arg1, "d" ) || !str_cmp( arg1, "down"  ) ) door = 5;
    else
    {
	send_to_char( "You do not see that here.\n\r", ch );
	return;
    }

    /* 'look direction' */
    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
	send_to_char( "Nothing special there.\n\r", ch );
	return;
    }

    if ( pexit->description != NULL && pexit->description[0] != '\0' )
	send_to_char( pexit->description, ch );
    else
	send_to_char( "Nothing special there.\n\r", ch );

    if ( pexit->keyword    != NULL
    &&   pexit->keyword[0] != '\0'
    &&   pexit->keyword[0] != ' ' )
    {
	if ( IS_SET(pexit->exit_info, EX_CLOSED) )
	{
	    act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
	}
	else if ( IS_SET(pexit->exit_info, EX_ISDOOR) )
	{
	    act( "The $d is open.",   ch, NULL, pexit->keyword, TO_CHAR );
	}
    }

    return;
}

void do_search( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    OBJ_DATA *above_exit_obj;
    char buffer [MAX_STRING_LENGTH];
    char *pdesc;
    int door;
    int number,count;
    int first=TRUE;
    char *pString;

    if ( ch->desc == NULL )
	return;

    if ( ch->position < POS_SLEEPING )
    {
	send_to_char( "You can't see anything but stars!\n\r", ch );
	return;
    }

    if ( ch->position == POS_SLEEPING )
    {
	send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
	return;
    }

    if ( !check_blind( ch ) )
	return;

    if ( !IS_NPC(ch)
    &&   !IS_SET(ch->act, PLR_HOLYLIGHT)
    &&   room_is_dark( ch->in_room ) )
    {
	 if (ch->race == race_lookup("fade")) {
	   send_to_char( "{DThe room is pitch black, but you can feel every corner of it...{x\n\r", ch );
	   show_char_to_char( ch->in_room->people, ch, TRUE );
	 }
	 else {
	   send_to_char( "It is pitch black ... \n\r", ch );
	   show_char_to_char( ch->in_room->people, ch, TRUE );
	   return;
	 }
/* 	send_to_char( "It is pitch black ... \n\r", ch ); */
/* 	show_char_to_char( ch->in_room->people, ch, TRUE ); */
/* 	return; */
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    number = number_argument(arg1,arg3);
    count = 0;

    if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
    {
	/* 'look' or 'look auto' */

        obj = ch->in_obj;
        if ( !(obj = ch->in_obj)
          || IS_SET( obj->value[1], CONT_SEE_OUT )
          || !IS_SET( obj->value[1], CONT_CLOSED ) )
        { 
    	   sprintf( buf, "%s", ch->in_room->name );
 	   send_to_char( buf, ch );
   
	   if ( (IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act,PLR_HOLYLIGHT)))
	   ||   IS_BUILDER(ch, ch->in_room->area) )
	   {
               sprintf(buf," {x[{DRoom %d{x] [{D%s{x]",ch->in_room->vnum,
                       ch->in_room->area->name);
	       send_to_char(buf,ch);
	   }

 	   send_to_char( "{x\n\r", ch );

      if ((ch->in_room->sector_type != SECT_INSIDE) && 
          (IS_SET(ch->in_room->room_flags, ROOM_VMAP))) {
         send_to_char( "{x\n\r", ch );
         do_function(ch, &do_map, "8");
         send_to_char( "{x\n\r", ch );
      }

	   if ( arg1[0] == '\0'
	   || ( !IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF) ) )
	   {
 	       sprintf( buf, "{q  %s{x", ch->in_room->description );
 	       send_to_char( buf, ch );
	   }

/* Objects above EXIT go in here */
   
      for ( above_exit_obj = ch->in_room->contents; above_exit_obj != NULL; above_exit_obj = above_exit_obj->next_content ) {
	      if (above_exit_obj->item_type ==   ITEM_FOUNTAIN  ||
             above_exit_obj->item_type ==   ITEM_FURNITURE) {

		   /*  Portals are kept under Exit line                       */
             /* above_exit_obj->item_type ==   ITEM_PORTAL) { */
            //CHECK SKILL PERCENT HERE - SEARCH
            //if (IS_SET(above_exit_obj->extra_flags,ITEM_HIDDEN) && !IS_IMMORTAL(ch)) {
               //continue;
            //}
            if (first) {
               send_to_char("\n\r",ch);
            }
            //CHECK SKILL PERCENT HERE - SEARCH
            if (IS_SET(above_exit_obj->extra_flags,ITEM_HIDDEN)) {
               sprintf(buffer, "{Y(h){x %s\n\r", above_exit_obj->description);
               pString = format_string(str_dup(buffer));
               send_to_char(pString, ch);
            }
            else {
	       sprintf(buffer, "%s\n\r", above_exit_obj->description);
               pString = format_string(str_dup(buffer));
               send_to_char(pString, ch);
            }
            first=FALSE;
	      }
      }
      
      if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT) ) {
	       send_to_char("\n\r",ch);
          do_function(ch, &do_exits, "auto" );
	   }

	 send_to_char("{O",ch);
	 show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE, FALSE );
	 send_to_char("{P",ch);
	 show_char_to_char( ch->in_room->people,   ch, FALSE );
	 send_to_char("{x",ch);
       }
       if (ch->in_obj)
       {
         sprintf( buf, "You see the inside of %s.\n\r", obj->short_descr );
         send_to_char(buf, ch);
         pdesc = get_extra_descr("inside_description",
                                 obj->pIndexData->extra_descr);
         if( pdesc )
         {
          send_to_char(pdesc, ch);
         }
         show_list_to_char(obj->contains, ch, FALSE, FALSE, FALSE);
         show_char_to_char(ch->in_room->people, ch, TRUE);
         return;
       }
      return;
    }

    if ( !str_cmp( arg1, "i" ) || !str_cmp(arg1, "in")  || !str_cmp(arg1,"on"))
    {
	/* 'look in' */
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Look in what?\n\r", ch );
	    return;
	}

	if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You do not see that here.\n\r", ch );
	    return;
	}

	switch ( obj->item_type )
	{
	default:
	    send_to_char( "That is not a container.\n\r", ch );
	    break;

   case ITEM_PORTAL:
      act("$n peers intently into $p.",ch,obj,NULL,TO_ROOM);
      act("You peer into $p.",ch,obj,NULL,TO_CHAR);

      /* Normal exit shows the proper exit */
      if ( IS_SET(obj->value[2], GATE_NORMAL_EXIT)
      &&  !IS_SET(obj->value[1], EX_CLOSED) )
      location = get_room_index(obj->value[3]);

      /*  Buggy exit shows a random exit anywhere in world
       *  gate's exit not necessarily the shown exit
       */
      else if ( IS_SET(obj->value[2], GATE_BUGGY)
      &&  !IS_SET(obj->value[1], EX_CLOSED) )
      location = get_random_room(ch, TRUE);
        
      /*  Closed or other exit flags */
      else {
        act("You try to look into $p, but something is blocking your from seeing through it.", 
		  ch, obj, NULL, TO_CHAR);
        return;
	 }

     act("Through $p you see:", ch, obj, NULL, TO_CHAR);
     /* show room desc without the room name or room vnum (IMM) */ 
     sprintf(buf,"{w%s{x",location->description);
     send_to_char(buf,ch);

    /* show exit location. show objects and people in the room */
    send_to_char("\n\r", ch);

    original = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    do_function(ch, &do_exits, "auto" );
    char_from_room( ch );
    char_to_room( ch, original);

    show_list_to_char( location->contents, ch, FALSE, FALSE, FALSE );
    show_char_to_char( location->people,   ch, FALSE);
    return;
    break;

	case ITEM_DRINK_CON:
	    if ( obj->value[1] <= 0 )
	    {
		send_to_char( "It is empty.\n\r", ch );
		break;
	    }

	    sprintf( buf, "It's %sfilled with  a %s liquid.\n\r",
		obj->value[1] <     obj->value[0] / 4
		    ? "less than half-" :
		obj->value[1] < 3 * obj->value[0] / 4
		    ? "about half-"     : "more than half-",
		liq_table[obj->value[2]].liq_color
		);

	    send_to_char( buf, ch );
	    break;

	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
            if( ch->in_obj == obj )
            {
               send_to_char("Just type look.\n\r", ch);
               break;
            }

	    if ( IS_SET(obj->value[1], CONT_CLOSED) )
	    {
		send_to_char( "It is closed.\n\r", ch );
		break;
	    }
            pdesc = get_extra_descr("inside_description",
                                    obj->pIndexData->extra_descr);
            if( pdesc )
             send_to_char(pdesc, ch);

//	    act( "$p holds:", ch, obj, NULL, TO_CHAR );
//	    show_list_to_char( obj->contains, ch, TRUE, TRUE );
            act("You look inside $p and see:", ch, obj, NULL, TO_CHAR);
            if( IS_SET( obj->value[1], CONT_ENTERABLE ) && obj->who_in )
             show_list_to_char(obj->contains, ch, TRUE, FALSE, FALSE);
            else
             show_list_to_char(obj->contains, ch, TRUE, TRUE, obj->carried_by == NULL ? TRUE: FALSE );
            if( obj->who_in )
            {
               OBJ_DATA *in_obj = ch->in_obj;
               char_to_obj(ch, obj);
               show_char_to_char(ch->in_room->people, ch, TRUE);
               char_from_obj(ch);
               ch->in_obj = in_obj;
            }

	    break;
	}
	return;
    }

   if (!str_cmp(arg1, "o") || !str_cmp(arg1, "out"))
   {
   /* 'look out' */
      if (ch->in_obj == NULL)
      {
         send_to_char("You would have to be in something to do this.\n\r", ch);
         return;
      }

      obj = ch->in_obj;
      if( IS_SET( obj->value[1], CONT_CLOSED )
       && !IS_SET( obj->value[1], CONT_SEE_OUT ) )
      {
         send_to_char("It is closed.\n\r", ch);
         return;
      }
      char_from_obj(ch);
      if (obj->in_obj == NULL)
         act("You look out of $p and you see:", ch, obj, NULL, TO_CHAR);
      else
         act("You look out of $p:", ch, obj, obj, TO_CHAR);
      do_look(ch, "");
      char_to_obj(ch, obj);
      return;

   }


    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
	show_char_to_char_1( victim, ch, FALSE );
	return;
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
	if ( can_see_obj( ch, obj ) )
	{  /* player can see object */
	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    {	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}
	    	else continue;
            }
 	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
 	    if ( pdesc != NULL )
 	    { 	if (++count == number)
 	    	{	
		    send_to_char( pdesc, ch );
		    return;
	     	}
		else continue;
	    }
	    if ( is_name( arg3, obj->name ) )
	    	if (++count == number)
	    	{
	    	    send_to_char( obj->description, ch );
	    	    send_to_char( "\n\r",ch);
		    return;
		  }
	  }
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
	if ( can_see_obj( ch, obj ) )
	{
	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    if ( is_name( arg3, obj->name ) )
		if (++count == number)
		{
		    send_to_char( obj->description, ch );
		    send_to_char("\n\r",ch);
		    return;
		}
	}
    }

if ( ch->in_obj )
    for ( obj = ch->in_obj->contains; obj != NULL; obj = obj->next_content )
    {
        
	if ( can_see_obj( ch, obj ) )
	{
	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    if ( is_name( arg3, obj->name ) )
		if (++count == number)
		{
		    send_to_char( obj->description, ch );
		    send_to_char("\n\r",ch);
		    return;
		}
	}
    }

    pdesc = get_extra_descr(arg3,ch->in_room->extra_descr);
    if (pdesc != NULL)
    {
	if (++count == number)
	{
	    send_to_char(pdesc,ch);
	    return;
	}
    }
    
    if (count > 0 && count != number)
    {
    	if (count == 1)
    	    sprintf(buf,"You only see one %s here.\n\r",arg3);
    	else
    	    sprintf(buf,"You only see %d of those here.\n\r",count);
    	
    	send_to_char(buf,ch);
    	return;
    }

         if ( !str_cmp( arg1, "n" ) || !str_cmp( arg1, "north" ) ) door = 0;
    else if ( !str_cmp( arg1, "e" ) || !str_cmp( arg1, "east"  ) ) door = 1;
    else if ( !str_cmp( arg1, "s" ) || !str_cmp( arg1, "south" ) ) door = 2;
    else if ( !str_cmp( arg1, "w" ) || !str_cmp( arg1, "west"  ) ) door = 3;
    else if ( !str_cmp( arg1, "u" ) || !str_cmp( arg1, "up"    ) ) door = 4;
    else if ( !str_cmp( arg1, "d" ) || !str_cmp( arg1, "down"  ) ) door = 5;
    else
    {
	send_to_char( "You do not see that here.\n\r", ch );
	return;
    }

    /* 'look direction' */
    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
	send_to_char( "Nothing special there.\n\r", ch );
	return;
    }

    if ( pexit->description != NULL && pexit->description[0] != '\0' )
	send_to_char( pexit->description, ch );
    else
	send_to_char( "Nothing special there.\n\r", ch );

    if ( pexit->keyword    != NULL
    &&   pexit->keyword[0] != '\0'
    &&   pexit->keyword[0] != ' ' )
    {
	if ( IS_SET(pexit->exit_info, EX_CLOSED) )
	{
	    act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
	}
	else if ( IS_SET(pexit->exit_info, EX_ISDOOR) )
	{
	    act( "The $d is open.",   ch, NULL, pexit->keyword, TO_CHAR );
	}
    }

    return;
}

/* RT added back for the hell of it */
void do_read (CHAR_DATA *ch, char *argument )
{
    do_function(ch, &do_look, argument);
}

void do_examine( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Examine what?\n\r", ch );
	return;
    }

    do_function(ch, &do_look, arg );

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
	switch ( obj->item_type )
	{
	default:
	    break;
	
	case ITEM_JUKEBOX:
	    do_function(ch, &do_play, "list");
	    break;

	case ITEM_MONEY:
	    if (obj->value[0] == 0)
	    {
	        if (obj->value[1] == 0)
		    sprintf(buf,"Odd...there's no coins in the pile.\n\r");
		else if (obj->value[1] == 1)
		    sprintf(buf,"Wow. One gold coin.\n\r");
		else
		    sprintf(buf,"There are %d gold coins in the pile.\n\r",
			obj->value[1]);
	    }
	    else if (obj->value[1] == 0)
	    {
		if (obj->value[0] == 1)
		    sprintf(buf,"Wow. One silver coin.\n\r");
		else
		    sprintf(buf,"There are %d silver coins in the pile.\n\r",
			obj->value[0]);
	    }
	    else
		sprintf(buf,
		    "There are %d gold and %d silver coins in the pile.\n\r",
		    obj->value[1],obj->value[0]);
	    send_to_char(buf,ch);
	    break;

	case ITEM_DRINK_CON:
	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    sprintf(buf,"in %s",argument);
	    do_function(ch, &do_look, buf );
	}
    }

    return;
}



/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits( CHAR_DATA *ch, char *argument )
{
    extern char * const dir_name[];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;
    int door;
    bool vmap_exit;

    fAuto  = !str_cmp( argument, "auto" );

    if ( !check_blind( ch ) )
	return;

    if (fAuto)
	sprintf(buf,"{o[Exits:");
    else if (IS_IMMORTAL(ch))
	sprintf(buf,"{oObvious exits from room %d:\n\r",ch->in_room->vnum);
    else
	sprintf(buf,"{oObvious exits:\n\r");

    found = FALSE;
    vmap_exit = FALSE;
    for ( door = 0; door <= 9; door++ )
    {
	if ( ( pexit = ch->in_room->exit[door] ) != NULL
	&&   pexit->u1.to_room != NULL
	&&   can_see_room(ch,pexit->u1.to_room) 
        && ( !ch->in_obj
          || IS_SET( ch->in_obj->value[1], CONT_SEE_OUT )
          || !IS_SET( ch->in_obj->value[1], CONT_CLOSED ) )
        && ((!IS_SET(pexit->exit_info, EX_HIDDEN) || IS_IMMORTAL(ch))
	||
	    (IS_SET(pexit->exit_info,EX_HIDDEN) && ((((get_skill(ch,gsn_observation) + get_skill(ch,gsn_alertness) + get_skill(ch,gsn_awareness)) / 3) /4) > number_percent()))))

	{
	    found = TRUE;
	    if ( fAuto ) {
		 /* Entrance from VMAP */
		 if (door > 5) {
		   if (ch->in_room->exit[door] == NULL)
			continue;
		   if (IS_SET(ch->in_room->room_flags, ROOM_VMAP) && (ch->in_room->sector_type == SECT_ENTER)) 
                   {
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
		   else  
                   {
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
		   {
		   	continue;
		   }
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
		   {
			continue;
		   }
		}
		sprintf( buf + strlen(buf), "{o%-5s - %s{x",
		    capitalize( dir_name[door] ),
		    room_is_dark( pexit->u1.to_room )
			?  "Too dark to tell"
			: pexit->u1.to_room->name
		    );
		if (IS_IMMORTAL(ch))
		    sprintf(buf + strlen(buf), 
			" (room %d)\n\r",pexit->u1.to_room->vnum);
		else
		    sprintf(buf + strlen(buf), "{x\n\r");
    }
	}
    }

    if (ch->in_obj != NULL)
    {
      strcat(buf, fAuto ? " leave " : "Leave{x\n\r");
      found = TRUE; 
    }

    if ( !found )
	strcat( buf, fAuto ? " none" : "None.{x\n\r" );

    if ( fAuto ) {
	 if (vmap_exit) {
	   strcat( buf, ")");
	 }
         strcat( buf, "]{x\n\r" );
    }
    send_to_char( buf, ch );

    return;
}

void do_worth( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  
  if (IS_NPC(ch)) {
    sprintf(buf,"You have %ld gold and %ld silver.\n\r",
		  ch->gold,ch->silver);
    send_to_char(buf,ch);
    return;
  }

  sprintf(buf,  "You have %ld {Ygo{yl{Yd{x, %ld {Wsi{xl{Wver{x, and %d experience (%s%ld exp to level{x).\n\r",
		ch->gold, ch->silver,ch->exp, (exp_next_level(ch) - ch->exp) < 0 ? "{g" : "{x", (exp_next_level(ch) - ch->exp));    
  send_to_char(buf,ch);

  sprintf(buf, "You have %ld {YGo{yl{Yd{x in the bank and %ld {Wsi{xl{Wver{x in the bank.\n\r",ch->pcdata->gold_bank, ch->pcdata->silver_bank);
  send_to_char(buf,ch);
  sprintf(buf, "You have %s%d{x trains, and %s%ld{x quest points.\n\r", 
		ch->train > 0 ? "{g" : "{R", ch->train,
		ch->pcdata->quest_curr > 0 ? "{g" : "{R", ch->pcdata->quest_curr);
  send_to_char(buf, ch);
  
  return;
}

void do_score( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char l_buf[4];
  char r_buf[4];
  
  if (IS_DR(ch)) {
    send_to_char("You are the {WDragon Reborn{x.\n\r", ch);
  }
  else if (IS_FORSAKEN(ch)) {
    send_to_char("You are one of the {WChosen{x by {rthe Great Lord of the Dark{x.\n\r", ch);    
  }
  
  sprintf( buf, "You are (%s) %s %s.\n\r",
		 ch->name,
		 IS_NPC(ch) ? "" : !IS_NULLSTR(ch->pcdata->appearance) ? ch->pcdata->appearance : "(none)",
		 IS_NPC(ch) ? "" : !IS_NULLSTR(ch->pcdata->ictitle) ? ch->pcdata->ictitle : "(none)");
  send_to_char( buf, ch );
  
  if (!IS_NPC(ch) && IS_WOLFKIN(ch) && !IS_NULLSTR(ch->wkname) && !IS_NULLSTR(ch->pcdata->wolf_appearance)) {
    sprintf( buf, "You are (%s) %s{x.\n\r",
		   ch->wkname,
		   ch->pcdata->wolf_appearance);
    send_to_char( buf, ch );   
  } 
  else if (IS_WOLFKIN(ch) && ch->wkname[0] != '\0') {
    sprintf( buf, "You are (%s) A %s with {YGoldeneyes{x.\n\r",
		   ch->wkname,
		   ch->sex == 0 ? "Sexless" : ch->sex == 1 ? "Male" : "Female");
    send_to_char( buf, ch );
  }
  
  sprintf( buf, "Level: %3d (%s%ld exp to level{x)  Played: %d hours.\n\r",
		 get_level(ch),
		 (exp_next_level(ch) - ch->exp) < 0 ? "{g" : "{x",
		 exp_next_level(ch) - ch->exp,
		 (ch->played + (int) (current_time - ch->logon)) / 3600);
  send_to_char( buf, ch );
  
  if ( is_clan(ch) ) {
    sprintf( buf, "Guild:  %24s  Rank: %2d  Title: %s{x\n\r",
		   player_clan(ch), ch->rank+1, ch->gtitle ? ch->gtitle : player_rank(ch));
    send_to_char(buf, ch);
  }
  
  if ( is_sguild(ch) ) {
    sprintf( buf, "SGuild: %24s  Rank: %2d  Title: %s{x\n\r",
		   player_sguild(ch), ch->sguild_rank+1, ch->sguild_title ? ch->sguild_title : player_sguild_rank(ch));
    send_to_char(buf, ch);
  }
  
  if ( is_ssguild(ch) ) {
    sprintf( buf, "SSGuild: %23s  Rank: %2d  Title: %s{x\n\r",
		   player_ssguild(ch), ch->ssguild_rank+1, ch->ssguild_title ? ch->ssguild_title : player_ssguild_rank(ch));
    send_to_char(buf, ch);
  }
  
  if (ch->minion != 0) {
    sprintf( buf, "Minion: %24s  Rank: %2d  Title: %s{x\n\r",
		   ch->mname, ch->mrank+1, ch->mtitle ? ch->mtitle : "Unassigned");
    send_to_char(buf, ch);
  }

  if (!IS_NPC(ch) && !IS_NULLSTR(ch->pcdata->df_name)) {
    sprintf(buf, "Darkfriend: %24s  {xRank: %2d\n\r",
		  ch->pcdata->df_name, ch->pcdata->df_level);
    send_to_char(buf, ch);
  }

  if ( get_trust( ch ) != ch->level ) {
    sprintf( buf, "You are trusted at level %d.\n\r", get_trust( ch ) );
    send_to_char( buf, ch );
  }
  
  sprintf(buf, "Race:   %-16s Str: %3d/ %s%2d   {WAir{x: %s%12d{x  {yArmor{x:\n\r",
		capitalize(race_table[ch->race].name),
		ch->perm_stat[STAT_STR], 
		get_curr_stat(ch,STAT_STR) > ch->perm_stat[STAT_STR] ? "{c" : "{x",
		get_curr_stat(ch,STAT_STR),
		ch->main_sphere == SPHERE_AIR ? "{W" : "{x",
		ch->perm_sphere[SPHERE_AIR]);
  send_to_char(buf,ch);
  
  sprintf(buf, "Sex:    %-16s Int: %3d/ %s%2d   {yEarth{x: %s%10d{x  Pierce: %5d\n\r",
		ch->sex == 0 ? "Sexless" : ch->sex == 1 ? "Male" : "Female",
		ch->perm_stat[STAT_INT],
		get_curr_stat(ch,STAT_INT) > ch->perm_stat[STAT_INT] ? "{c" : "{x",
		get_curr_stat(ch,STAT_INT),
		ch->main_sphere == SPHERE_EARTH ? "{y" : "{x",
		ch->perm_sphere[SPHERE_EARTH],
		GET_AC(ch,AC_PIERCE));
  send_to_char(buf,ch);
  
  sprintf(buf, "Class:  %-16s Wis: %3d/ %s%2d   {RFire{x: %s%11d{x  Bash:   %5d\n\r",
		IS_NPC(ch) ? "Mobile" : capitalize(class_table[ch->class].name),
		ch->perm_stat[STAT_WIS], 
		get_curr_stat(ch,STAT_WIS) > ch->perm_stat[STAT_WIS] ? "{c" : "{x",
		get_curr_stat(ch,STAT_WIS),
		ch->main_sphere == SPHERE_FIRE ? "{R" : "{x",
		ch->perm_sphere[SPHERE_FIRE],
		GET_AC(ch,AC_BASH));
  send_to_char(buf,ch);
    
  sprintf(buf, "HP:     %5d/%5d      Dex: %3d/ %s%2d   {YSpirit{x: %s%9d{x  Slash:  %5d\n\r",
		ch->hit,  ch->max_hit,
		ch->perm_stat[STAT_DEX], 
		get_curr_stat(ch,STAT_DEX) > ch->perm_stat[STAT_DEX] ? "{c" : "{x",
		get_curr_stat(ch,STAT_DEX),
		ch->main_sphere == SPHERE_SPIRIT ? "{Y" : "{x",
		ch->perm_sphere[SPHERE_SPIRIT],
		GET_AC(ch,AC_SLASH));
  send_to_char(buf,ch);
  
  sprintf(buf, "End:    %5d/%5d      Con: %3d/ %s%2d   {BWater{x: %s%10d{x  Weaves: %5d\n\r",
		ch->endurance, ch->max_endurance,
		ch->perm_stat[STAT_CON], 
		get_curr_stat(ch,STAT_CON) > ch->perm_stat[STAT_CON] ? "{c" : "{x",
		get_curr_stat(ch,STAT_CON),
		ch->main_sphere == SPHERE_WATER ? "{B" : "{x",
		ch->perm_sphere[SPHERE_WATER],
		GET_AC(ch,AC_EXOTIC));
  send_to_char(buf,ch);

  if (IS_PKILLER(ch))
  {
	sprintf(buf, "PK Kills: %6d                     PK Deaths:  %6d\n\r",ch->pk_count, ch->pk_died_count);
  	send_to_char(buf,ch);

  }

  if (ch->insanity_points >0)
  {
	//sprintf(buf,"Insanity Points: %02d\n\r",ch->insanity_points);
	if (ch->insanity_points > 0 && ch->insanity_points <= 10)
	{
	   sprintf(buf,"Insanity Status: Starting to be paranoid\n\r");
	}
	else
	if (ch->insanity_points > 10 && ch->insanity_points <= 20)
	{
	   sprintf(buf,"Insanity Status: You are starting to hear voices\n\r");
	}
	else
	if (ch->insanity_points > 20 && ch->insanity_points <= 30)
	{
	   sprintf(buf,"Insanity Status: You are arguing with the voices\n\r");
	}
	else
	if (ch->insanity_points > 30 && ch->insanity_points <= 40)
	{
	   sprintf(buf,"Insanity Status: Everyone is looking at you funny, knowing that you are going insane.\n\r");
	}
	else
	if (ch->insanity_points > 40 && ch->insanity_points <= 50)
	{
	   sprintf(buf,"Insanity Status: They're after you.  They really are, and the voices in your head ARE real\n\r");
	}
	send_to_char(buf,ch);
  }
  
  sprintf(buf, "Items:  %5d/%5d         %s%s@%s         OP: %6ld/%6ld  Hit:    %5d\n\r",
		ch->carry_number, can_carry_n(ch),
		IS_CHANNELING(ch) ? "{W({x" : " {x",
		get_hit_loc_col(ch, ch, LOC_HE),
		IS_CHANNELING(ch) ? "{W){x" : " {x",
		ch->holding, get_curr_op(ch),
		GET_HITROLL(ch));
  send_to_char(buf,ch);
  
  sprintf(l_buf, "%s", get_hit_loc_col(ch, ch, LOC_LA));
  sprintf(r_buf, "%s", get_hit_loc_col(ch, ch, LOC_RA));
  
  if (IS_IMMORTAL(ch))
    sprintf(buf, "Weight: %5ld/%5d     %s---{x+%s---{x       Burnout:  %3d/%3d  Dam:    %5d\n\r",
		  get_carry_weight(ch) / 10, can_carry_w(ch) /10,
		  l_buf,
		  r_buf,
		  ch->burnout, ch->max_burnout,
		  GET_DAMROLL(ch));
  else
    sprintf(buf, "Weight: %5ld/%5d       %s---{x+%s---{x       Burnout:  %3d/%3d  Dam:    %5d\n\r",
		  get_carry_weight(ch) / 10, can_carry_w(ch) /10,
		  l_buf,
		  r_buf,
		  ch->burnout, ch->max_burnout,
		  GET_DAMROLL(ch));
  send_to_char(buf,ch);
  
  sprintf(buf, "{YGo{yl{Yd{x: %13ld          %s|{x          Autochannel: %3d%%  Wimpy:  %5d\n\r",
		ch->gold,
		get_hit_loc_col(ch, ch, LOC_BD),
		ch->autoholding,
		ch->wimpy);		  
  send_to_char(buf,ch);
  
  sprintf(l_buf, "%s", get_hit_loc_col(ch, ch, LOC_LL));
  sprintf(r_buf, "%s", get_hit_loc_col(ch, ch, LOC_RL));
  
  sprintf(buf, "{WSi{xl{Wver{x: %11ld         %s/ %s\\{x         Exp: %12d  Trains: %s%5d{x\n\r",
		ch->silver, 
		l_buf,
		r_buf,
		ch->exp, ch->train > 0 ? "{g" : "{R", ch->train);
  send_to_char(buf,ch);
  
  sprintf(buf, "Position:  %-16s%s/   %s\\{x        World: %10s  QP:     %5ld\n\r",
		capitalize(position_table[ch->position].name),
		l_buf,
		r_buf,
		IS_NPC(ch) ? "Normal" : flag_string(world_table, ch->world),
		IS_NPC(ch) ? 0 : ch->pcdata->quest_curr);
  send_to_char(buf,ch);
  
  if (!IS_NPC(ch) && ch->race == race_lookup("trolloc")) {
     sprintf(buf, "{RHo{rma{Rns{x: %11d\n\r", ch->pcdata->pc_kill_cnt);     
     send_to_char(buf,ch);
     sprintf(buf, "{DTr{Ro{Dll{Ro{Dcs{x: %9d\n\r", ch->pcdata->clan_kill_cnt);
     send_to_char(buf,ch);
  }

  if (!IS_NPC(ch)) {
     if (ch->pcdata->condition[COND_DRUNK]   > 10 )
    	send_to_char( "You are drunk.\n\r",   ch );
     if (ch->pcdata->condition[COND_THIRST] ==  0 )
    	send_to_char( "You are thirsty.\n\r", ch );
     if (ch->pcdata->condition[COND_HUNGER]   ==  0 )
    	send_to_char( "You are hungry.\n\r",  ch );
  }
  
  /* Target */
  if(!IS_NPC(ch)) {
    sprintf(buf, "\n{rT{xarget/{gD{xefend: {r%s{x/{g%s{x\n\r",
		  ch->target_loc == LOC_NA ? "all" : hit_flags[ch->target_loc].name,
		  ch->defend_loc == LOC_NA ? "all" : hit_flags[ch->defend_loc].name);	 
    send_to_char(buf, ch);
  }
  
  sprintf(buf, "\n{GM{xerits/{RF{xlaws/{BT{xalents: {G%s{x/{R%s{x/{B%s{x\n\r", 
		ch->merits  ? background_flag_string(merit_table, ch->merits)   : "none",
		ch->flaws   ? background_flag_string(flaw_table, ch->flaws)     : "none",
		ch->talents ? background_flag_string(talent_table, ch->talents) : "none");
  send_to_char(buf, ch);

  if (!IS_NPC(ch)) {
    sprintf(buf, "{WIC{x: {W%s{x\n\r",  flag_string( ic_flags, ch->ic_flags));
    send_to_char(buf, ch);
  }
  
  /* RT wizinvis and holy light */
  if ( IS_IMMORTAL(ch)) {
    send_to_char("Holy Light: ",ch);
    if (IS_SET(ch->act,PLR_HOLYLIGHT))
	 send_to_char("on",ch);
    else
	 send_to_char("off",ch);
    
    if (ch->invis_level) {
	 sprintf( buf, "  Invisible: level %d",ch->invis_level);
	 send_to_char(buf,ch);
    }
    
    if (ch->incog_level) {
	 sprintf(buf,"  Incognito: level %d",ch->incog_level);
	 send_to_char(buf,ch);
    }
    send_to_char("\n\r",ch);
  }
  
  send_to_char("\n\r",ch);
  
  if (!IS_NPC(ch) && ch->pcdata->learned[gsn_create_angreal] > 0)
  {
    if (ch->pcdata->next_createangreal) {
	 if (ch->pcdata->next_createangreal > current_time)
	   sprintf(buf, "{BNext Angreal{x: {R%s{x", (char *) ctime(&ch->pcdata->next_createangreal));
	 else
	   sprintf(buf, "{BNext Angreal{x: {gNow!{x\n\r");
	 send_to_char(buf, ch);
    }


  }
  if (!IS_NPC(ch) && char_knows_masterform(ch)) {
    if (ch->pcdata->next_bmtrain) {
	 if (ch->pcdata->next_bmtrain > current_time)
	   sprintf(buf, "{BMF{xtrain: {R%s{x", (char *) ctime(&ch->pcdata->next_bmtrain));
	 else
	   sprintf(buf, "{BMF{xtrain: {gNow!{x\n\r");
	 send_to_char(buf, ch);
    }
  }

 
  
/*
  if (!IS_NPC(ch) && ch->pcdata->learned[gsn_speardancer] > 0) {
    if (ch->pcdata->next_sdtrain) {
	 if (ch->pcdata->next_sdtrain > current_time)
	   sprintf(buf, "{YSD{xtrain: {R%s{x", (char *) ctime(&ch->pcdata->next_sdtrain));
	 else
	   sprintf(buf, "{YSD{xtrain: {gNow!{x\n\r");
	 send_to_char(buf, ch);
    }
  }
  
  if (!IS_NPC(ch) && ch->pcdata->learned[gsn_duelling] > 0) {
    if (ch->pcdata->next_dutrain) {
	 if (ch->pcdata->next_dutrain > current_time)
	   sprintf(buf, "{GDU{xtrain: {R%s{x", (char *) ctime(&ch->pcdata->next_dutrain));
	 else
	   sprintf(buf, "{GDU{xtrain: {gNow!{x\n\r");
	 send_to_char(buf, ch);
    }
  }
*/
         
}

void do_score_OLD( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  
  sprintf( buf, "You are (%s) %s %s.\n\r",
		 ch->name,
		 IS_NPC(ch) ? "" : ch->pcdata->appearance,
		 IS_NPC(ch) ? "" : ch->pcdata->ictitle);
  send_to_char( buf, ch );
  
  if (IS_WOLFKIN(ch) && ch->wkname[0] != '\0') {
     sprintf( buf, "You are (%s) A %s with {YGoldeneyes{x.\n\r",
              ch->wkname,
              ch->sex == 0 ? "Sexless" : ch->sex == 1 ? "Male" : "Female");
     send_to_char( buf, ch );
  }

  sprintf( buf, "Level: %3d (%s%ld exp to level{x)  Played: %d hours.\n\r",
		 get_level(ch),
		 (exp_next_level(ch) - ch->exp) < 0 ? "{g" : "{x",
		 exp_next_level(ch) - ch->exp,
//		 get_age(ch),
		 (ch->played + (int) (current_time - ch->logon)) / 3600);
  send_to_char( buf, ch );
  
  if ( is_clan(ch) ) {
    sprintf( buf, "Guild: %16s  Rank: %2d  Title: %s{x\n\r",
		   player_clan(ch), ch->rank+1, ch->gtitle ? ch->gtitle : player_rank(ch));
    send_to_char(buf, ch);
  }
  
  if (ch->minion != 0) {
    sprintf( buf, "Minion: %16s Rank: %2d  Title: %s{x\n\r",
		   ch->mname, ch->mrank+1, ch->mtitle ? ch->mtitle : "Unassigned");
    send_to_char(buf, ch);
  }
  
    if ( get_trust( ch ) != ch->level ) {
	 sprintf( buf, "You are trusted at level %d.\n\r", get_trust( ch ) );
	 send_to_char( buf, ch );
    }
    
    sprintf(buf, "Race:   %-16s Str: %3d/ %s%2d   {WAir{x: %s%12d{x  {yArmor{x:\n\r",
		  capitalize(race_table[ch->race].name),
		  ch->perm_stat[STAT_STR], 
		  get_curr_stat(ch,STAT_STR) > ch->perm_stat[STAT_STR] ? "{c" : "{x",
		  get_curr_stat(ch,STAT_STR),
		  ch->main_sphere == SPHERE_AIR ? "{W" : "{x",
		  ch->perm_sphere[SPHERE_AIR]);
    send_to_char(buf,ch);
    
    sprintf(buf, "Sex:    %-16s Int: %3d/ %s%2d   {yEarth{x: %s%10d{x  Pierce: %5d\n\r",
		  ch->sex == 0 ? "Sexless" : ch->sex == 1 ? "Male" : "Female",
		  ch->perm_stat[STAT_INT],
		  get_curr_stat(ch,STAT_INT) > ch->perm_stat[STAT_INT] ? "{c" : "{x",
		  get_curr_stat(ch,STAT_INT),
		  ch->main_sphere == SPHERE_EARTH ? "{y" : "{x",
		  ch->perm_sphere[SPHERE_EARTH],
		  GET_AC(ch,AC_PIERCE));
    send_to_char(buf,ch);
    
    sprintf(buf, "Class:  %-16s Wis: %3d/ %s%2d   {RFire{x: %s%11d{x  Bash:   %5d\n\r",
		  IS_NPC(ch) ? "Mobile" : capitalize(class_table[ch->class].name),
		  ch->perm_stat[STAT_WIS], 
		  get_curr_stat(ch,STAT_WIS) > ch->perm_stat[STAT_WIS] ? "{c" : "{x",
		  get_curr_stat(ch,STAT_WIS),
		  ch->main_sphere == SPHERE_FIRE ? "{R" : "{x",
		  ch->perm_sphere[SPHERE_FIRE],
		  GET_AC(ch,AC_BASH));
    send_to_char(buf,ch);
    
    sprintf(buf, "HP:     %5d/%5d      Dex: %3d/ %s%2d   {YSpirit{x: %s%9d{x  Slash:  %5d\n\r",
		  ch->hit,  ch->max_hit,
		  ch->perm_stat[STAT_DEX], 
		  get_curr_stat(ch,STAT_DEX) > ch->perm_stat[STAT_DEX] ? "{c" : "{x",
		  get_curr_stat(ch,STAT_DEX),
		  ch->main_sphere == SPHERE_SPIRIT ? "{Y" : "{x",
		  ch->perm_sphere[SPHERE_SPIRIT],
		  GET_AC(ch,AC_SLASH));
    send_to_char(buf,ch);
    
    sprintf(buf, "End:    %5d/%5d      Con: %3d/ %s%2d   {BWater{x: %s%10d{x  Weaves: %5d\n\r",
		  ch->endurance, ch->max_endurance,
		  ch->perm_stat[STAT_CON], 
                  get_curr_stat(ch,STAT_CON) > ch->perm_stat[STAT_CON] ? "{c" : "{x",
                  get_curr_stat(ch,STAT_CON),
		  ch->main_sphere == SPHERE_WATER ? "{B" : "{x",
		  ch->perm_sphere[SPHERE_WATER],
		  GET_AC(ch,AC_EXOTIC));
    send_to_char(buf,ch);
    
    sprintf(buf, "Items:  %5d/%5d                     OP: %6ld/%6ld  Hit:    %5d\n\r",
		  ch->carry_number, can_carry_n(ch),
		  ch->holding, get_curr_op(ch),
		  GET_HITROLL(ch));
    send_to_char(buf,ch);

    if (IS_IMMORTAL(ch))
	 sprintf(buf, "Weight: %5ld/%5d                   Burnout:  %3d/%3d  Dam:    %5d\n\r",
		    get_carry_weight(ch) / 10, can_carry_w(ch) /10,
		    ch->burnout, ch->max_burnout,
		    GET_DAMROLL(ch));
    else
	 sprintf(buf, "Weight: %5ld/%5d                     Burnout:  %3d/%3d  Dam:    %5d\n\r",
		    get_carry_weight(ch) / 10, can_carry_w(ch) /10,
		    ch->burnout, ch->max_burnout,
		    GET_DAMROLL(ch));
    send_to_char(buf,ch);

    sprintf(buf, "{YGo{yl{Yd{x: %13ld                     Autochannel: %3d%%  Wimpy:  %5d\n\r",
		  ch->gold,
		  ch->autoholding,
		  ch->wimpy);		  
    send_to_char(buf,ch);
    
    sprintf(buf, "{WSi{xl{Wver{x: %11ld                     Exp: %12d  Trains: %s%5d{x\n\r",
            ch->silver, ch->exp, ch->train > 0 ? "{g" : "{R", ch->train);
    send_to_char(buf,ch);

    sprintf(buf, "Position:  %-16s             World: %10s  QP:     %5ld\n\r",
		  capitalize(position_table[ch->position].name),
		  IS_NPC(ch) ? "Normal" : flag_string(world_table, ch->world),
		  IS_NPC(ch) ? 0 : ch->pcdata->quest_curr);
    send_to_char(buf,ch);
	    
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]   > 10 )
      send_to_char( "You are drunk.\n\r",   ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] ==  0 )
      send_to_char( "You are thirsty.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER]   ==  0 )
      send_to_char( "You are hungry.\n\r",  ch );

    if(!IS_NPC(ch)) {
      if (IS_SET(ch->off_flags,TARGET_HEAD))    sprintf(buf, "\nYou target the head more often.\n\r");
      if (IS_SET(ch->off_flags,TARGET_NECK))    sprintf(buf, "\nYou target the neck more often.\n\r");
      if (IS_SET(ch->off_flags,TARGET_TORSO))   sprintf(buf, "\nYou target the torso more often.\n\r");
      if (IS_SET(ch->off_flags,TARGET_ARMS))    sprintf(buf, "\nYou target the arms more often.\n\r");
      if (IS_SET(ch->off_flags,TARGET_HANDS))   sprintf(buf, "\nYou target the hands more often.\n\r");
      if (IS_SET(ch->off_flags,TARGET_BACK))    sprintf(buf, "\nYou target the back more often.\n\r");
      if (IS_SET(ch->off_flags,TARGET_LEGS))    sprintf(buf, "\nYou target the legs more often.\n\r");
      if (IS_SET(ch->off_flags,TARGET_FEET))    sprintf(buf, "\nYou target the feet more often.\n\r");
      if (IS_SET(ch->off_flags,TARGET_GENERAL)) sprintf(buf, "\nYou don't target anything in particular.\n\r");
      if (ch->off_flags)
	   send_to_char(buf,ch);
    }

   sprintf(buf, "\n{GM{xerits/{RF{xlaws/{BT{xalents: {G%s{x/{R%s{x/{B%s{x\n\r", 
		 background_flag_string(merit_table, ch->merits),
		 background_flag_string(flaw_table, ch->flaws),
		 background_flag_string(talent_table, ch->talents));
   send_to_char(buf, ch);

   /* RT wizinvis and holy light */
   if ( IS_IMMORTAL(ch)) {
	send_to_char("Holy Light: ",ch);
	if (IS_SET(ch->act,PLR_HOLYLIGHT))
	  send_to_char("on",ch);
	else
	  send_to_char("off",ch);
	
	if (ch->invis_level) {
	  sprintf( buf, "  Invisible: level %d",ch->invis_level);
	  send_to_char(buf,ch);
	}
	
	if (ch->incog_level) {
	  sprintf(buf,"  Incognito: level %d",ch->incog_level);
	  send_to_char(buf,ch);
	}
	send_to_char("\n\r",ch);
   }

   if (!IS_NPC(ch) && char_knows_masterform(ch)) {
	if (ch->pcdata->next_bmtrain) {
	  if (ch->pcdata->next_bmtrain > current_time)
	    sprintf(buf, "\n\rMFtrain: {R%s{x", (char *) ctime(&ch->pcdata->next_bmtrain));
	  else
	    sprintf(buf, "\n\rMFtrain: {gNow!{x\n\r");
	  send_to_char(buf, ch);
	}
   }

}

/* Show wards in a room */
void do_waffects( CHAR_DATA *ch, char *argument )
{
  WARD_DATA       *wd;
  char             buf[MAX_STRING_LENGTH];
  bool             found=FALSE;
  ROOM_INDEX_DATA *from_location;
  ROOM_INDEX_DATA *to_location;
  EXIT_DATA       *pexit;
  int              direction;

  /* only PCs */
  if (IS_NPC(ch))
    return;
  
  if (ch->class != CLASS_CHANNELER)
    return;

  if (!IS_NULLSTR(argument)) {
    from_location = ch->in_room;

    if ((direction = find_exit(ch, argument)) != -1) {
	 if ((pexit = from_location->exit[direction]) != NULL
		&& ( to_location = pexit->u1.to_room ) != NULL
		&& can_see_room(ch, pexit->u1.to_room))
         {
	   
	   sprintf(buf, "Wards sensed %sward:\n\r", dir_name[direction]);
	   send_to_char(buf, ch);
	   
	   if (to_location->wards != NULL) {
		for (wd = to_location->wards; wd != NULL; wd = wd->next) {
		  if (wd->inverted && wd->casterId != ch->id && !IS_IMMORTAL(ch))
		    continue;
		  if ((wd->sex == ch->sex || IS_IMMORTAL(ch)) && IS_SAME_WORLD(wd, ch)) {
		    
		    if (wd->sn_learned > get_skill(ch, wd->sn) && wd->casterId != ch->id && !IS_IMMORTAL(ch))
			 continue;
		    
		    wd->caster = get_charId_world(ch, wd->casterId);
		    
		    found=TRUE;
		    
		    sprintf(buf, "Ward (%s): '%s'", 
				  wd->caster != NULL ? wd->caster->sex == SEX_MALE ? "{Bm{x" : "{rf{x" : "{ya{x",
				  skill_table[wd->sn].name );
		    send_to_char(buf, ch);
		    
		    sprintf(buf, " modifies room by %d for %d hours.",
				  wd->strength,
				  wd->duration);
		    send_to_char(buf, ch);
		    
		    if (wd->duration == SUSTAIN_WEAVE)
			 send_to_char (" ({rs{x)", ch);
		    
		    if (wd->caster != NULL) {
			 if (wd->caster == ch) {
			   if (wd->inverted)
				send_to_char(" ({Yi{x)", ch);
			   send_to_char(" (yours)", ch);
			 }
			 else if (IS_IMMORTAL(ch)) {
			   if (wd->inverted)
				send_to_char(" ({Yi{x)", ch);
			   sprintf(buf, " (%s)", wd->caster->name);
			   send_to_char(buf, ch);
			 }		
		    }
		    
		    send_to_char( "\n\r", ch );
		  }
		}
	   }    
	   if (!found)
		send_to_char("Nothing you can sense.\n\r", ch);
	 }
	 else {
	   send_to_char("You don't see an exit in that direction.\n\r", ch);
	   return;
	 }
    }
    else {
	 send_to_char("You don't see an exit in that direction.\n\r", ch);
	 return;
    }
    
  }
  else {
    send_to_char("Wards sensed here:\n\r", ch);
    
    if (ch->in_room->wards != NULL) {
	 for (wd = ch->in_room->wards; wd != NULL; wd = wd->next) {
	   if (wd->inverted && wd->casterId != ch->id && !IS_IMMORTAL(ch))
		continue;
	   if ((wd->sex == ch->sex || IS_IMMORTAL(ch)) && IS_SAME_WORLD(wd, ch)) {
		
		if (wd->sn_learned > get_skill(ch, wd->sn) && wd->casterId != ch->id && !IS_IMMORTAL(ch))
		  continue;
		
		wd->caster = get_charId_world(ch, wd->casterId);
		
		found=TRUE;
		
		sprintf(buf, "Ward (%s): '%s'", 
			   wd->caster != NULL ? wd->caster->sex == SEX_MALE ? "{Bm{x" : "{rf{x" : "{ya{x",
			   skill_table[wd->sn].name );
		send_to_char(buf, ch);
		
		sprintf(buf, " modifies room by %d for %d hours.",
			   wd->strength,
			   wd->duration);
		send_to_char(buf, ch);
		
		if (wd->duration == SUSTAIN_WEAVE)
		  send_to_char (" ({rs{x)", ch);
		
		if (wd->caster != NULL) {
		  if (wd->caster == ch) {
		    if (wd->inverted)
			 send_to_char(" ({Yi{x)", ch);
		    send_to_char(" (yours)", ch);
		  }
		  else if (IS_IMMORTAL(ch)) {
		    if (wd->inverted)
			 send_to_char(" ({Yi{x)", ch);
		    sprintf(buf, " (%s)", wd->caster->name);
		    send_to_char(buf, ch);
		  }		
		}
		
		send_to_char( "\n\r", ch );
	   }
	 }
    }    
    if (!found)
	 send_to_char("Nothing you can sense.\n\r", ch);
  }
  
  return;
}

/* Show residues in a room */
void do_raffects( CHAR_DATA *ch, char *argument )
{
  RESIDUE_DATA *rd;
  char buf [MAX_STRING_LENGTH];

  /* only PCs */
  if (IS_NPC(ch))
    return;
    
  if (ch->class != CLASS_CHANNELER)
     return;
/*
  if (!IS_SET(ch->talents, TALENT_RESIDUES) && !IS_IMMORTAL(ch)) {
    send_to_char("You don't know how to read residues.\n\r", ch);
    return;
  }
*/

  send_to_char("Residues sensed here:\n\r", ch);

  if (ch->in_room->residues != NULL) {
    for (rd = ch->in_room->residues; rd != NULL; rd = rd->next) {
	 if ((rd->sex == ch->sex || IS_IMMORTAL(ch)) && IS_SAME_WORLD(rd, ch)) {
	   sprintf(buf, "Flows of %s was woven into '{W%s{x' here.", (char *)flow_text(rd->sn ,ch), ch->pcdata->learned[rd->sn] < 1 ? "something" : skill_table[rd->sn].name);
	   send_to_char(buf, ch);
	   if (IS_SET(ch->talents, TALENT_RESIDUES))
		sprintf(buf, " (%s) (dissipates in %d hours).\n\r", rd->sex == SEX_MALE ? "{Bm{x" : rd->sex == SEX_FEMALE ? "{rf{x" : "n", rd->duration);
	   else
		sprintf(buf, " (%s)\n\r", rd->sex == SEX_MALE ? "{Bm{x" : rd->sex == SEX_FEMALE ? "{rf{x" : "n");
	   send_to_char(buf, ch);
	 }
    }
  }
  else
    send_to_char("Nothing.\n\r", ch);
  
  return;
}

void do_affects( CHAR_DATA *ch, char *argument )
{
  char arg [MAX_INPUT_LENGTH];
  char buf [MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  AFFECT_DATA *paf;
  AFFECT_DATA *wd;
  bool found=FALSE;
  
  one_argument( argument, arg );
   
  /* You */
  if ( arg[0] == '\0' ) {       
    send_to_char( "You are affected by:\n\r", ch );
    if ( ch->affected != NULL ) {
	 for ( paf = ch->affected; paf != NULL; paf = paf->next ) {

	   paf->caster = get_charId_world(ch, paf->casterId);

	   if (skill_table[paf->type].spell_fun == spell_null)
		sprintf(act_buff, "Skill :  '%s'", skill_table[paf->type].name);
	   else
		sprintf(act_buff, "Weave (%s): '%s'",
			   paf->caster != NULL ? paf->caster->sex == SEX_MALE ? "{Bm{x" : "{rf{x" : "{ya{x",
			   skill_table[paf->type].name );
	   send_to_char( act_buff, ch );

	   sprintf( act_buff,
			  " modifies %s by %d for %d hours.",
			  affect_loc_name( paf->location ),
			  paf->modifier,
			  paf->duration );
	   send_to_char( act_buff, ch );

	   if (paf->duration == SUSTAIN_WEAVE)
		send_to_char (" ({rs{x)", ch);

	   if (paf->inverted)
	       send_to_char(" ({Yi{x)", ch);

	   if (paf->caster != NULL) {
	        if (paf->caster == ch) {
		    send_to_char(" (yours)", ch);
		}
		else if (IS_IMMORTAL(ch)) {
		  sprintf(buf, " (%s)", paf->caster->name);
		  send_to_char(buf, ch);
		}
	   }
	   send_to_char( "\n\r", ch );
	 }
    }
    else {
	 send_to_char( "Nothing you can sense.\n\r", ch );
    }
    return;
  }
  else if (!str_cmp(arg, "room")) {       
    send_to_char( "Room is affected by:\n\r", ch );
    if (ch->in_room->weaves != NULL) {
	 for (wd = ch->in_room->weaves; wd != NULL; wd = wd->next) {
	   if (wd->inverted && wd->casterId != ch->id && !IS_IMMORTAL(ch))
		continue;	   
	   if ((wd->sex == ch->sex || IS_IMMORTAL(ch)) && IS_SAME_WORLD(wd, ch)) {

		if (wd->type_learned > get_skill(ch, wd->type) && wd->casterId != ch->id && !IS_IMMORTAL(ch))
		  continue;
		
		wd->caster = get_charId_world(ch, wd->casterId);
		
		found=TRUE;
		
		sprintf(act_buff, "Weave (%s): '%s'",
			   wd->caster != NULL ? wd->caster->sex == SEX_MALE ? "{Bm{x" : "{rf{x" : "{ya{x",
			   skill_table[wd->type].name );
		send_to_char( act_buff, ch );
		
		sprintf( act_buff,
			    " modifies %s by %d for %d hours.",
			    affect_loc_name( wd->location ),
			    wd->modifier,
			    wd->duration );
		send_to_char( act_buff, ch );
		
		if (wd->duration == SUSTAIN_WEAVE)
		  send_to_char (" ({rs{x)", ch);
		
		if (wd->caster != NULL) {
		  if (wd->caster == ch) {
		    if (wd->inverted)
			 send_to_char(" ({Yi{x)", ch);
		    send_to_char(" (yours)", ch);
		  }
		  else if (IS_IMMORTAL(ch)) {
		    if (wd->inverted)
			 send_to_char(" ({Yi{x)", ch);
		    sprintf(buf, " (%s)", wd->caster->name);
		    send_to_char(buf, ch);
		  }
		}
		send_to_char( "\n\r", ch );
	   }
	 }
	 if (!found) {
	   send_to_char( "Nothing you can sense.\n\r", ch );
	 }
    }
    else {
	 send_to_char( "Nothing you can sense.\n\r", ch );
    }
    return;
  }
  else if (!str_cmp("ward",arg) || !str_cmp("wards",arg))
  {
	do_waffects(ch,"");
	/*
  	WARD_DATA *pWard;
  	WARD_DATA *pWard_next;
	char buffer[MAX_STRING_LENGTH];
	bool found = FALSE;
	send_to_char("Wards found:\n\r",ch);
         for (pWard = ch->in_room->wards; pWard != NULL; pWard = pWard_next) 
         {
           pWard_next = pWard->next;

           if (skill_table[pWard->sn].spell_fun == spell_null)
                continue;

	   if (pWard->sex == ch->sex)
	   {
              found = TRUE;
              sprintf(buffer, "Ward: '%s' for %d hours.\n\r",
                           skill_table[pWard->sn].name,
			   pWard->duration);
                send_to_char(buffer, ch);
	   }
         }
	 if (!found)
	 {
		send_to_char("None\r\n",ch);
	 }
	*/
	return;
  }

  
  /* Add check if ch is forsaken: Atwain */
  if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
    send_to_char( "You are not a channeler!\n\r", ch);
    return;
  }
  
  if (!check_blind( ch ))
    return;

  /* VICTIM NOT MOBILE */
  if ( ( victim = get_char_room( ch, arg ) ) != NULL ) {
    if ( ( victim->affected != NULL )
	    && ( !IS_NPC(victim))) {
	 act( "$N is affected by:", ch, NULL, victim, TO_CHAR );
	 for ( paf = victim->affected; paf != NULL; paf = paf->next ) {

	   paf->caster = get_charId_world(ch, paf->casterId);
	   
	   /* Only see weaves off same sex unless immortal */
	   if (paf->caster != NULL) {
		if (paf->caster->sex != ch->sex && !IS_IMMORTAL(ch))
		  continue;
		if (paf->inverted && paf->casterId != ch->id && !IS_IMMORTAL(ch))
		  continue;
	   }
	   else {
		paf->caster   = victim;
		paf->casterId = victim->id;
		
		if ((paf->caster->sex != ch->sex) && !IS_IMMORTAL(ch))
		  continue;
	   }
	   
	   found = TRUE;

	   if (skill_table[paf->type].spell_fun == spell_null)
		sprintf(act_buff, "Skill :  '%s'", skill_table[paf->type].name);
	   else
		sprintf(act_buff, "Weave (%s): '%s'",
			   paf->caster != NULL ? paf->caster->sex == SEX_MALE ? "{Bm{x" : "{rf{x" : "{ya{x",
			   !IS_NPC(ch) ? ch->pcdata->learned[paf->type] < 1 ? "something" : skill_table[paf->type].name :
			   skill_table[paf->type].name);
	   send_to_char( act_buff, ch );

	   sprintf( act_buff,
			  " modifies %s by %d for %d hours.",
			  affect_loc_name( paf->location ),
			  paf->modifier,
			  paf->duration );
	   send_to_char( act_buff, ch );

	   if (paf->duration == SUSTAIN_WEAVE)
		send_to_char (" ({rs{x)", ch);

	   if (paf->caster == ch) {
		if (paf->inverted)
		  send_to_char(" ({Yi{x)", ch);
		send_to_char(" (yours)", ch);
	   }
	   else if (IS_IMMORTAL(ch)) {
		if (paf->inverted)
		  send_to_char(" ({Yi{x)", ch);
		sprintf(buf, " (%s)", paf->caster->name);
		send_to_char(buf, ch);
	   }
	   send_to_char( "\n\r", ch );
	 }
	 if (!found)
	   send_to_char( "Nothing you can sense.\n\r", ch );
    }
    
    /* MOBILE */
    else
	 if ((victim->affected != NULL) && (IS_NPC(victim))) {
	   act( "$N is affected by:", ch, NULL, victim, TO_CHAR );
	   for ( paf = victim->affected; paf != NULL; paf = paf->next ) {

		paf->caster = get_charId_world(ch, paf->casterId);

		/* Only see weaves off same sex unless immortal */
		if (paf->caster != NULL) {
		  if ((paf->caster->sex != ch->sex) && !IS_IMMORTAL(ch))
		    continue;
		  if (paf->inverted && paf->casterId != ch->id && !IS_IMMORTAL(ch))
		    continue;
		}
		else {
		  paf->caster   = victim;
		  paf->casterId = victim->id;
		  
		  if ((paf->caster->sex != ch->sex) && !IS_IMMORTAL(ch))
		    continue;
		  if (paf->inverted && paf->casterId != ch->id && !IS_IMMORTAL(ch))
		    continue;
		}

		found = TRUE;

		if (skill_table[paf->type].spell_fun == spell_null)
		  sprintf(act_buff, "Skill     :  '%s'", skill_table[paf->type].name);
		else
		  sprintf(act_buff, "Weave (%s): '%s'",
				paf->caster != NULL ? paf->caster->sex == SEX_MALE ? "{Bm{x" : "{rf{x" : "{ya{x",
				skill_table[paf->type].name );
		send_to_char( act_buff, ch );

		sprintf( act_buff,
			    " modifies %s by %d for %d hours.",
			    affect_loc_name( paf->location ),
			    paf->modifier,
			    paf->duration );
		send_to_char( act_buff, ch );

		if (paf->duration == SUSTAIN_WEAVE)
		  send_to_char (" ({rs{x)", ch);

		if (paf->caster == ch) {
		  if (paf->inverted)
		    send_to_char(" ({Yi{x)", ch);
		  send_to_char(" (yours)", ch);
		}
		else if (IS_IMMORTAL(ch)) {
		  if (paf->inverted)
		    send_to_char(" ({Yi{x)", ch);
		  sprintf(buf, " (%s)", paf->caster->name);
		  send_to_char(buf, ch);
		}
		send_to_char( "\n\r", ch );
	   }
	   if (!found)
		send_to_char( "Nothing you can sense.\n\r", ch );
	 }
	 else {
	   act( "$N is affected by:", ch, NULL, victim, TO_CHAR );
	   send_to_char( "Nothing you can sense.\n\r", ch );
	 }
    
  }
  else {
    send_to_char( "They aren't here.\n\r", ch );
  }
  return;
}

void do_affects_OLD(CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA *paf, *paf_last = NULL;
    char buf[MAX_STRING_LENGTH];
    
    if ( ch->affected != NULL )
    {
	send_to_char( "You are affected by the following spells:\n\r", ch );
	for ( paf = ch->affected; paf != NULL; paf = paf->next )
	{
	    if (paf_last != NULL && paf->type == paf_last->type)
		if (ch->level >= 20)
		    sprintf( buf, "                      ");
		else
		    continue;
	    else
	    	sprintf( buf, "Spell: %-15s", skill_table[paf->type].name );

	    send_to_char( buf, ch );

	    if ( ch->level >= 20 )
	    {
		sprintf( buf,
		    ": modifies %s by %d ",
		    affect_loc_name( paf->location ),
		    paf->modifier);
		send_to_char( buf, ch );
		if ( paf->duration == -1 )
		    sprintf( buf, "permanently" );
		else
		    sprintf( buf, "for %d hours", paf->duration );
		send_to_char( buf, ch );
	    }

	    send_to_char( "\n\r", ch );
	    paf_last = paf;
	}
    }
    else 
	send_to_char("You are not affected by any spells.\n\r",ch);

    return;
}



char *	const	day_name	[] =
{
    "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
    "the Great Gods", "the Sun"
};

char *	const	month_name	[] =
{
  "Taisham", "Jumara", "Saban", "Ain", "Adar", "Saven",
  "Amadaine", "Tammaz", "Maigdhal", "Choren", "Shaldine",
  "Nesan", "Danu"
};

void do_time( CHAR_DATA *ch, char *argument )
{
    extern char str_boot_time[];
    char buf[MAX_STRING_LENGTH];
    char *suf;
    int day;

    day     = time_info.day + 1;

         if ( day > 4 && day <  20 ) suf = "th";
    else if ( day % 10 ==  1       ) suf = "st";
    else if ( day % 10 ==  2       ) suf = "nd";
    else if ( day % 10 ==  3       ) suf = "rd";
    else                             suf = "th";

    sprintf( buf,
	     "\n\rIt is %d o'clock %s, %d%s the Month of %s.\n\r",
	     (time_info.hour % 12 == 0) ? 12 : time_info.hour %12,
	     time_info.hour >= 12 ? "pm" : "am",
	     day, suf,
	     month_name[time_info.month]);
    send_to_char(buf,ch);
    sprintf(buf,"{wThe {DShadow {rWars{x started up at %s"
	    "The system time is %s", str_boot_time,
	    (char *) ctime(&current_time));
    send_to_char( buf, ch );
    return;
}



void do_weather( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    static char * const sky_look[4] =
    {
	"cloudless",
	"cloudy",
	"rainy",
	"lit by flashes of lightning"
    };

    if ( !IS_OUTSIDE(ch) )
    {
	send_to_char( "You can't see the weather indoors.\n\r", ch );
	return;
    }

    sprintf( buf, "The sky is %s and %s.\n\r",
	sky_look[weather_info.sky],
	weather_info.change >= 0
	? "a warm southerly breeze blows"
	: "a cold northern gust blows"
	);
    send_to_char( buf, ch );
    return;
}


void do_help_OLD( CHAR_DATA *ch, char *argument )
{
    HELP_DATA *pHelp;
    BUFFER *output;
    bool found = FALSE;
    char argall[MAX_INPUT_LENGTH],argone[MAX_INPUT_LENGTH];
    char nohelp[MAX_STRING_LENGTH];
    int level;

    output = new_buf();

    strcpy(nohelp, argument);

    if ( argument[0] == '\0' )
	argument = "summary";

    /* this parts handles help a b so that it returns help 'a b' */
    argall[0] = '\0';
    while (argument[0] != '\0' )
    {
	argument = one_argument(argument,argone);
	if (argall[0] != '\0')
	    strcat(argall," ");
	strcat(argall,argone);
    }

    for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
    {
    	level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;

	if (level > get_trust( ch ) )
	    continue;

	if ( is_exact_name( argall, pHelp->keyword ) ) {
	  /* add seperator if found */
	  if (found)
	    add_buf(output,
			  "\n\r============================================================\n\r\n\r");
	  if ( pHelp->level >= 0 && str_cmp( argall, "imotd" ) )
	    {
		 add_buf(output,pHelp->keyword);
		 add_buf(output,"\n\r");
	    }

	    /*
	     * Strip leading '.' to allow initial blanks.
	     */
	    if ( pHelp->text[0] == '.' )
		 add_buf(output,pHelp->text+1);
	    else
		 add_buf(output,pHelp->text);
	    found = TRUE;
	    /* small hack :) */
	    if (ch->desc != NULL && ch->desc->connected != CON_PLAYING 
	    &&  		    ch->desc->connected != CON_GEN_GROUPS)
		break;
	}
    }

    if (!found) {
      send_to_char( "No help entry found on that word.\n\r", ch );
      append_file( ch, HELP_FILE, nohelp );
    }
    else
	 page_to_char(buf_string(output),ch);
    free_buf(output);
}


void do_help( CHAR_DATA *ch, char *argument )
{
  HELP_DATA *pHelp;
  BUFFER *output;
  BUFFER *outputKEYS;
  bool found = FALSE;
  bool kfound = FALSE;
  char argall[MAX_INPUT_LENGTH];
  char argone[MAX_INPUT_LENGTH];
  char nohelp[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int level;
  
  output = new_buf();
  outputKEYS = new_buf();
  
  strcpy(nohelp, argument);
  
  if ( argument[0] == '\0' )
    argument = "summary";
  
  /* this parts handles help a b so that it returns help 'a b' */
  argall[0] = '\0';
  
  while (argument[0] != '\0' ) {
    argument = one_argument(argument,argone);
    if (argall[0] != '\0')
	 strcat(argall," ");
    strcat(argall,argone);
  }
  
  for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next ) {
    level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;
    
    if (level > get_trust( ch ) )
	 continue;
    
    if (is_name( argall, pHelp->keyword )) {

	 /* add seperator if found */	 
	 if (found)
	   add_buf(output, 
			 "\n\r{W------------------------------ {yNEXT MATCH{W ------------------------------{x\n\r");

	 /* if ( pHelp->level >= 0 && str_cmp( argall, "imotd" ) ) { */
	 if (str_cmp( argall, "imotd" ) ) {
	   kfound = TRUE;
	   sprintf(buf, "[{y%s{x]\n\r", pHelp->keyword);
	   add_buf(outputKEYS, buf);
	   add_buf(output,pHelp->keyword);
	   add_buf(output,"\n\r");
	 }
	 
	 /* If key match, show : else show possible hits */
	 if (is_exact_name( argall, pHelp->keyword )) {
	   if ( pHelp->text[0] == '.' ) {
		add_buf(output, pHelp->text+1);
		found = TRUE;
	   }
	   else {
		add_buf(output, pHelp->text);
		found = TRUE;
	   }
	 }
	 else if (str_prefix(pHelp->keyword, argall))
	   ; /* Go through with partial matches */
	 
	 /* small hack :) */
	 if (ch->desc != NULL && 
		ch->desc->connected != CON_PLAYING &&  		    
		ch->desc->connected != CON_GEN_GROUPS)
	   break;
    }
  }

  if (!found) {
    send_to_char( "No help entry found on that word.\n\r", ch );
    append_file( ch, HELP_FILE, nohelp );
    if (kfound) {
	 send_to_char( "Possible other entries:\n\r", ch);
	 page_to_char(buf_string(outputKEYS),ch);
    }
  }
  else
    page_to_char(buf_string(output),ch);
  
  free_buf(output);
  free_buf(outputKEYS);
}

/* whois command */
void do_whois (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found = FALSE;

    one_argument(argument,arg);
  
    if (arg[0] == '\0')
    {
	send_to_char("You must provide a name.\n\r",ch);
	return;
    }

    output = new_buf();

    for (d = descriptor_list; d != NULL; d = d->next)
    {
	CHAR_DATA *wch;
	char const *class;

 	if (d->connected != CON_PLAYING || !can_see_channel(ch,d->character))
	    continue;
	
	wch = ( d->original != NULL ) ? d->original : d->character;

 	if (!can_see_channel(ch,wch))
	    continue;

	if (!str_prefix(arg,wch->name))
	{
	    found = TRUE;
	    
	    /* work out the printing */
	    class = class_table[wch->class].who_name;
	    switch(wch->level)
	    {
		case MAX_LEVEL - 0 : class = "IMP"; 	break;
		case MAX_LEVEL - 1 : class = "CRE";	break;
		case MAX_LEVEL - 2 : class = "SUP";	break;
		case MAX_LEVEL - 3 : class = "DEI";	break;
		case MAX_LEVEL - 4 : class = "GOD";	break;
		case MAX_LEVEL - 5 : class = "IMM";	break;
		case MAX_LEVEL - 6 : class = "DEM";	break;
		case MAX_LEVEL - 7 : class = "ANG";	break;
		case MAX_LEVEL - 8 : class = "AVA";	break;
		case MAX_LEVEL - 9 : class = "HER";	break;
	    }
    
	    /* a little formatting */
            if (ch->level > (MAX_LEVEL - 7))
            {
	        sprintf(buf, "[%2d %6s %s] %s%s%s%s%s%s%s%s\n\r",
		    wch->level,




		    race_table[wch->race].who_name,

		    class,
	     wch->incog_level >= LEVEL_HERO ? "(Incog) ": "",
 	     wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	     clan_table[wch->clan].who_name,
	     IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
             IS_SET(wch->act,PLR_KILLER) ? "(KILLER) " : "",
             IS_SET(wch->act,PLR_THIEF) ? "(THIEF) " : "",
		COLORNAME(wch), IS_NPC(wch) ? "" : wch->pcdata->title);
		//wch->name, IS_NPC(wch) ? "" : wch->pcdata->title);
	    add_buf(output,buf);
	}
    }
}
    if (!found)
    {
	send_to_char("No one of that name is playing.\n\r",ch);
	return;
    }

    page_to_char(buf_string(output),ch);
    free_buf(output);
}

/*
 * Will keep a max_on_ever counter stored on file
 * max_on and max_on_ever is global defined in this file 
 * @ Swordfish
 */
void set_counterhistory(void) 
{
  FILE *fp;
  char buf[MAX_STRING_LENGTH];
  char filename[MAX_STRING_LENGTH];

  sprintf(filename, "%s%s", DATA_DIR, COUNTER_FILE);

  /* only read from file if not already done */
  /* E.g max_on_ever == 0                    */
  if (max_on_ever == 0) {
    if ((fp = fopen(filename, "r")) == NULL) {
	 sprintf(buf, "Error reading from file <%s>\n", COUNTER_FILE);
	 log_string(buf);
	 return;
    }
    
    max_on_ever = fread_number( fp );
    fclose(fp);
  }
  
  if (max_on > max_on_ever) {
    max_on_ever = max_on;
    if ((fp = fopen(filename, "w")) == NULL) {
	 sprintf(buf, "Error writing to file <%s>\n", COUNTER_FILE);
	 log_string(buf);
	 return;
    }
    fprintf(fp, "%d\n", max_on_ever);
    fclose(fp);
  }
  
  return;
}

/* Compare two characters by name */
int compare_char_names(const void *v1, const void *v2)
{
   return strcmp((*(CHAR_DATA**)v1)->name, (*(CHAR_DATA**)v2)->name);
}

void do_who( CHAR_DATA *ch, char *argument )
{
   extern time_t boot_time;
   time_t        up_time=0;
   int days=0, hours=0, mins=0, secs=0;

   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   char levelbuf[32];
   char colbuf[6];
   int  coln;
   int nWhoInvis = 0;
   BUFFER *output;
   BUFFER *output2;
   int nMatch;
   int Ifirst=FALSE;
   int Mfirst=FALSE;
   bool fImmortalOnly=FALSE;
   bool fNewbieOnly=FALSE;
   bool fshowLevel=FALSE;

   CHAR_DATA *pcs[MAX_PC_ONLINE]; // 150 PC's
   int cnt=0;
   int i=0;
   CHAR_DATA *wch;
   int tMatch;
   int fillers;
   
   nMatch = 0;
   tMatch = 0;
   buf[0] = '\0';
   output = new_buf();
   output2 = new_buf();

   if ( argument[0] == 'i' ) {
	fImmortalOnly = TRUE;
   }

   if ( argument[0] == 'n' ) {
        fNewbieOnly = TRUE;
   }

   // Only immortals can use this option
   if (IS_IMMORTAL(ch) && argument[0] == 'l') {
       	fshowLevel = TRUE;
   }

   /* Count PCs online and init pcs array to point to wch */
   for ( wch = char_list; wch != NULL; wch = wch->next ) {
      if (IS_NPC(wch))
         continue;
      pcs[cnt++] = wch;
   }

   /* sort PCS array */  
   qsort (pcs, cnt, sizeof(wch), compare_char_names);

   /*for ( wch = char_list; wch != NULL; wch = wch->next ) {*/
   for (i=0; i < cnt; i++) {

     /* Only want to show PC's */
     if (IS_NPC(pcs[i]))
       continue;

     /* If who i */
     if (fImmortalOnly && !IS_IMMORTAL(pcs[i]))
        continue;
  
     /* Count total PC online, including IMMS for now */
     /* Setting wizi larger than MAX_LEVEL (100) you are not seen */
     if ((pcs[i]->invis_level >= LEVEL_HERO) && (pcs[i] != ch))
        continue;

       if ((pcs[i]->incog_level >= get_trust(ch)))
       {
	   continue;
       }
    
     tMatch++;

     /* If who n */
     if (fNewbieOnly && !IS_NEWBIE(pcs[i]))
        continue;

     /* Don't show those you can't see, or that don't wanat you */
     /* to see                                                  */
     /*
     if (!can_see(ch,pcs[i]))
       continue;
       */
              
     if (get_trust(ch) < pcs[i]->invis_level)
        continue;

     /* Whoinvis */
     if ((pcs[i]->incog_level > 0 && pcs[i]->incog_level < 2) && (pcs[i] != ch) && (!IS_IMMORTAL(ch)/* && !IS_FORSAKEN(ch) && !IS_DR(ch)*/))
     {
        nWhoInvis++;
        continue;
     }

     /*wizi*/
     if ((pcs[i]->incog_level > 2) && (pcs[i] != ch) && !IS_IMMORTAL(ch) )
        continue;

     /* PC found visible */
     nMatch++;
       
     if ( (pcs[i]->level >= LEVEL_HERO) && (!Ifirst)) {
       Ifirst = TRUE;
       sprintf(buf, "%s", "\n{D[{WImmortals & Heroes{D]{W:{x\n\r");
       add_buf(output,buf);
     }

     if ( (pcs[i]->level < LEVEL_HERO) && (!Mfirst)) {
       Mfirst = TRUE;                     
       if (use_christmas_layout()) {
           sprintf(buf, "\n{g_.:{R*{g~{R*{g:._.:{R*{g~{R*{g:._.:{R*{g~{R*{g:._.:{R*{g~{R*{g:._.:{R*{g~{R*{g:._.:{R*{g~{R*{g:._.:{R*{g~{R*{g:._.:{R*{g~{R*{g:._.:{R*{g~{R*{g:._{x\n");
           add_buf(output2,buf);
        }

        sprintf(buf, "%s", "\n{D[{WMortals{D]{W:{x\n\r");
        add_buf(output2,buf);        	
     }

     fillers = (18 - colorstrlen(pcs[i]->pcdata->imm_info));

     if (pcs[i]->level >= LEVEL_HERO) {	
       sprintf( buf, "{w[%s%*s{w][{W%s%s%s%s%s%s%s%s%s%s%s{w]{x %s%s{x\n\r",
		IS_NPC(pcs[i])                             ? ""  : pcs[i]->pcdata->imm_info,
		fillers, "",
		IS_SET(pcs[i]->comm,COMM_AFK)              ? "A" : " ",
		IS_SET(pcs[i]->comm,COMM_NOCHAT)           ? "C" : " ",
		((IS_SET(pcs[i]->world, WORLD_TAR_FLESH) ||
		 IS_SET(pcs[i]->world, WORLD_TAR_DREAM))
                 && (IS_IMMORTAL(ch)))   ? "D" : " ",
		pcs[i]->editor                             ? "E" : " ",
		pcs[i]->position == POS_FIGHTING           ? "F" : " ",
		IS_SET(pcs[i]->comm,COMM_NOGAME)           ? "G" : " ",
		pcs[i]->timer > 3                          ? "I" : " ",
          pcs[i]->desc == NULL                       ? "L" : " ",
		pcs[i]->pcdata->in_progress                ? "N" : " ",
		IS_SET(pcs[i]->comm,COMM_QUIET)            ? "Q" : " ",
		pcs[i]->invis_level >= LEVEL_HERO          ? "W" : " ",
		COLORNAME(pcs[i]),
		IS_NPC(pcs[i])                             ? ""  : IS_SET(pcs[i]->comm,COMM_AFK) && !IS_NULLSTR(pcs[i]->pcdata->afkmsg) ? pcs[i]->pcdata->afkmsg : pcs[i]->pcdata->title );
       add_buf(output,buf);
     }
     else {
       if (fshowLevel) {	
	    sprintf(levelbuf, "[%s%5d{x][%s%c{x]", 
			  get_level(pcs[i]) >= LEVEL_HERO-1 ? "{B" : IS_NEWBIE(pcs[i]) ? "{M" : "{x", 
			  get_level(pcs[i]), 
			  pcs[i]->class == 0 ? "{Y" : pcs[i]->class == 1 ? "{R" : pcs[i]->class == 2 ? "{B" : "{x",
			  UPPER(class_table[pcs[i]->class].name[0]));
       }
       if (use_christmas_layout()) {
         coln = number_range(1,2);
         if (coln==1)
           sprintf(colbuf, "{g");
         else
           sprintf(colbuf, "{R");	
       }
       sprintf( buf, "{w[%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s{w]%s %4s %s%s{x%s{x\n\r",
          use_christmas_layout()                  ? colbuf : "{W",           
		IS_SET(pcs[i]->comm,COMM_AFK)           ? "A" : " ",
		IS_SET(pcs[i]->comm,COMM_NOCHAT)        ? "C" : " ",
         ((IS_SET(pcs[i]->world, WORLD_TAR_FLESH) ||
          IS_SET(pcs[i]->world, WORLD_TAR_DREAM))
          && (IS_IMMORTAL(ch))) 		  ? "D" : " ",
          pcs[i]->editor                          ? "E" : " ",
          pcs[i]->position == POS_FIGHTING && IS_IMMORTAL(ch)  ? "F" : " ",
		IS_SET(pcs[i]->comm,COMM_NOGAME)        ? "G" : " ",
	IS_SET(pcs[i]->act, PLR_IS_NEWBIEHELPER)        ? "H" : " ",
		pcs[i]->timer > 3 && IS_IMMORTAL(ch)    ? "I" : " ",
          pcs[i]->desc == NULL && IS_IMMORTAL(ch)       ? "L" : " ",
		pcs[i]->pcdata->in_progress && IS_IMMORTAL(ch) ? "N" : " ",
		IS_SET(pcs[i]->comm,COMM_QUIET)         ? "Q" : " ",
          IS_RP(pcs[i]) && (IS_IMMORTAL(ch) || IS_SET(pcs[i]->act2,PLR2_SEEKINGRP)) ? "R" : " ",
		IS_SET(pcs[i]->act, PLR_QUESTING) && IS_IMMORTAL(ch) ? "S" : " ",
		IS_SET(pcs[i]->act,PLR_UNVALIDATED)     ? "U" : " ",
		pcs[i]->incog_level > 0                 ? "W" : " ",
		IS_FACELESS(pcs[i]) && IS_IMMORTAL(ch)  ? "T" : " ",
		(IS_SLEEPING(pcs[i]) && CAN_DREAM(ch))   ? "P" : " ",
		fshowLevel && IS_IMMORTAL(ch)           ? levelbuf : "",
		IS_PKILLER(pcs[i]) 			? "(PK)" : "",
		IS_SET(pcs[i]->act,PLR_UNVALIDATED)     ? "{M" : "{x",
		COLORNAME(pcs[i]),
		IS_NPC(pcs[i])                          ? ""  : IS_SET(pcs[i]->comm,COMM_AFK) && !IS_NULLSTR(pcs[i]->pcdata->afkmsg) ? pcs[i]->pcdata->afkmsg : pcs[i]->pcdata->title );
       add_buf(output2,buf);
     }
   }
   int wc = 0;
   for (wc = 0; wc < nWhoInvis; wc++)
   {
       if (!IS_IMMORTAL(ch))
       {
       	sprintf( buf, "{w[                 {w]      Someone who is hiding{x\n\r");
        add_buf(output2,buf);
       }
   }
   
  
   max_on = UMAX(tMatch,max_on);

   /* Will store max_on_ever to a file if larger */
   set_counterhistory();

   if (use_christmas_layout()) {
      sprintf( buf2, "\n{xPlayers found: {G%d{x of {G%d{x online."
                  "  Best count this startup: {G%d{x.  Ever: {G%d{x.\n\r", nMatch, tMatch, max_on, max_on_ever);
      add_buf(output2,buf2);
   }
   else {
      sprintf( buf2, "\n{xPlayers found: {y%d{x of {y%d{x online."
                  "  Best count this startup: {y%d{x.  Ever: {y%d{x.\n\r", nMatch, tMatch, max_on, max_on_ever);
      add_buf(output2,buf2);
   }

	
   /* Do the math */
   up_time = (current_time - boot_time);
   days    = (up_time / (3600 * 24));
   hours   = (up_time / 3600) % 24;
   mins    = (up_time % 3600) / 60;
   secs    = (up_time % 60);
	
   /* Build the string */
   sprintf( buf2, "The {DShadow {rWars{x has been running for ");

   if (use_christmas_layout()) {
      sprintf(buf, "[{G%02d{xd:{G%02d{xh:{G%02d{xm:{G%02d{xs] ", days, hours, mins, secs);
      strcat(buf2, buf);
   }
   else {
      sprintf(buf, "[{y%02d{xd:{y%02d{xh:{y%02d{xm:{y%02d{xs] ", days, hours, mins, secs);
      strcat(buf2, buf);   	
   }      
   
   sprintf(buf, "this startup.\n\r");
   strcat(buf2, buf);
   add_buf(output2,buf2);

   /* Send to char the buffers */
   page_to_char( buf_string(output), ch );
   page_to_char( buf_string(output2), ch );
   free_buf(output);
   free_buf(output2);

   return;
}

// Same as do_who, just for web
void do_webwho()
{
  extern time_t boot_time;
  time_t        up_time=0;
  int days=0, hours=0, mins=0, secs=0;
  FILE *wfp;
  char filename[MSL];
  
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char out_buf_imm[MAX_STRING_LENGTH*4];
  char out_buf_mort[MAX_STRING_LENGTH*6];
  char *immptr=NULL;
  char *mortptr=NULL;
  int nMatch;
  int Ifirst=FALSE;
  int Mfirst=FALSE;
  bool fImmortalOnly=FALSE;
  bool fNewbieOnly=FALSE;
  
  CHAR_DATA *pcs[MAX_PC_ONLINE]; // 150 PC's
  int cnt=0;
  int i=0;
  CHAR_DATA *wch;
  int tMatch;
  int fillers;
  int nWhoInvis = 0;
  
  nMatch = 0;
  tMatch = 0;
  buf[0] = '\0';

  memset(out_buf_imm, 0x00, sizeof(out_buf_imm));
  memset(out_buf_mort, 0x00, sizeof(out_buf_mort));

  /* Count PCs online and init pcs array to point to wch */
  for ( wch = char_list; wch != NULL; wch = wch->next ) {
    if (IS_NPC(wch))
	 continue;
    pcs[cnt++] = wch;
  }

  /* sort PCS array */  
  qsort (pcs, cnt, sizeof(wch), compare_char_names);

  for (i=0; i < cnt; i++) {
    
    /* Only want to show PC's */
    if (IS_NPC(pcs[i]))
	 continue;
    
    /* If who i */
    if (fImmortalOnly && !IS_IMMORTAL(pcs[i]))
	 continue;
    
    /* Count total PC online, including IMMS for now */
    /* Setting wizi larger than MAX_LEVEL - 8 (100) you are not seen */
    if ((pcs[i]->invis_level > LEVEL_HERO))
    {
	 continue;
    }

    tMatch++;
    
    /* If who n */
    if (fNewbieOnly && !IS_NEWBIE(pcs[i]))
	 continue;

    if (pcs[i]->invis_level && (!IS_IMMORTAL(pcs[i])))
    {
	 nWhoInvis++;
	 continue;
    }
    
    /* Whoinvis */
    if ((pcs[i]->incog_level > 0) || pcs[i]->invis_level > 0)
    {
	 continue;
    }
    
    /* PC found visible */
    nMatch++;
    
    if ( (pcs[i]->level >= LEVEL_HERO) && (!Ifirst)) {
	 Ifirst = TRUE;
	 sprintf(buf, "%s", "\n{D[{WImmortals & Heroes{D]{W:{x<BR>");
	 sprintf(out_buf_imm, buf);
    }
    
    if ( (pcs[i]->level < LEVEL_HERO) && (!Mfirst)) {
	 Mfirst = TRUE;
	 sprintf(buf, "%s", "\n{D[{WMortals{D]{W:{x<BR>");
	 sprintf(out_buf_mort, buf);
    }
    
    fillers = (18 - colorstrlen(pcs[i]->pcdata->imm_info));
    
    if (pcs[i]->level >= LEVEL_HERO) {	
       sprintf( buf, "{w[%s%*s{w][{W%s%s%s%s%s%s%s%s%s%s%s{w]{x %s%s{x<BR>",
		IS_NPC(pcs[i])                             ? ""  : pcs[i]->pcdata->imm_info,
		fillers, "",
		IS_SET(pcs[i]->comm,COMM_AFK)              ? "A" : " ",
		IS_SET(pcs[i]->comm,COMM_NOCHAT)           ? "C" : " ",
		(IS_SET(pcs[i]->world, WORLD_TAR_FLESH) ||
		 IS_SET(pcs[i]->world, WORLD_TAR_DREAM))
		&& IS_IMMORTAL(pcs[i])  ? "D" : " ",
		pcs[i]->editor                             ? "E" : " ",
		pcs[i]->position == POS_FIGHTING           ? "F" : " ",
		IS_SET(pcs[i]->comm,COMM_NOGAME)           ? "G" : " ",
		pcs[i]->timer > 3 && IS_IMMORTAL(pcs[i])   ? "I" : " ", 
          	pcs[i]->desc == NULL && IS_IMMORTAL(pcs[i])? "L" : " ",
		pcs[i]->pcdata->in_progress && IS_IMMORTAL(pcs[i])               ? "N" : " ",
		IS_SET(pcs[i]->comm,COMM_QUIET)            ? "Q" : " ",
		pcs[i]->invis_level >= LEVEL_HERO          ? "W" : " ",
		COLORNAME(pcs[i]),
		IS_NPC(pcs[i])                             ? ""  : IS_SET(pcs[i]->comm,COMM_AFK) && !IS_NULLSTR(pcs[i]->pcdata->afkmsg) ? pcs[i]->pcdata->afkmsg : pcs[i]->pcdata->title );

	  strcat(out_buf_imm, buf);
     }
     else {
       sprintf( buf, "{w[{W%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s{w]{x %s%s{x<BR>",
		IS_SET(pcs[i]->comm,COMM_AFK)           ? "A" : " ",
		IS_SET(pcs[i]->comm,COMM_NOCHAT)        ? "C" : " ",
	//Don't show mortal dreamers to the web
	/*
         (IS_SET(pcs[i]->world, WORLD_TAR_FLESH) ||
          IS_SET(pcs[i]->world, WORLD_TAR_DREAM)) ? "D" : " ",
	*/
	  " ",
          pcs[i]->editor                          ? "E" : " ",
	  //No reason to show people levelling
          //pcs[i]->position == POS_FIGHTING        ? "F" : " ",
	  " ",
		IS_SET(pcs[i]->comm,COMM_NOGAME)        ? "G" : " ",
	IS_SET(pcs[i]->act, PLR_IS_NEWBIEHELPER)        ? "H" : " ",
	//Don't show idle to the web
	//	pcs[i]->timer > 3                       ? "I" : " ",
	" ",
	//Don't show linkdead to the web
        //  pcs[i]->desc == NULL                    ? "L" : " ",
	" ",
	//Don't show who is writing notes to the web
	//pcs[i]->pcdata->in_progress             ? "N" : " ",
	" ",
		IS_SET(pcs[i]->comm,COMM_QUIET)         ? "Q" : " ",
          IS_RP(pcs[i]) && IS_SET(pcs[i]->act2,PLR2_SEEKINGRP) ? "R" : " ",
	//Don't show questing to the web
	//IS_SET(pcs[i]->act, PLR_QUESTING)       ? "S" : " ",
	" ",
		IS_SET(pcs[i]->act,PLR_UNVALIDATED)     ? "U" : " ",
		pcs[i]->incog_level > 0                 ? "W" : " ",
		COLORNAME(pcs[i]),
		IS_NPC(pcs[i])                          ? ""  : IS_SET(pcs[i]->comm,COMM_AFK) && !IS_NULLSTR(pcs[i]->pcdata->afkmsg) ? pcs[i]->pcdata->afkmsg : pcs[i]->pcdata->title );

	  strcat(out_buf_mort, buf);
     }
   }
   
   int wc = 0;
   for (wc = 0; wc < nWhoInvis; wc++)
   {
        sprintf( buf, "{w[                 {w]      Someone who is hiding{x\n\r");
        strcat(out_buf_mort,buf);
   }

   max_on = UMAX(tMatch,max_on);

   /* Will store max_on_ever to a file if larger */
   set_counterhistory();
   
   sprintf( buf2, "\n{xPlayers found: {y%d{x."
		  "  Best count this startup: {y%d{x.  Ever: {y%d{x.<BR>", nMatch, max_on, max_on_ever);
   strcat(out_buf_mort, buf2);

   /* Do the math */
   up_time = (current_time - boot_time);
   days    = (up_time / (3600 * 24));
   hours   = (up_time / 3600) % 24;
   mins    = (up_time % 3600) / 60;
   secs    = (up_time % 60);
   
   /* Build the string */
   sprintf( buf2, "The {DShadow {rWars{x has been running for ");
   
   sprintf(buf, "[{y%02d{xd:{y%02d{xh:{y%02d{xm:{y%02d{xs] ", days, hours, mins, secs);
   strcat(buf2, buf);
   
   sprintf(buf, "this startup.<BR>");
   strcat(buf2, buf);
   strcat(out_buf_mort, buf2);


   /* Send to char the buffers */
   //sprintf(filename, "%s%s.html", WEB_DIR, "who");
   sprintf(filename, "/home/tsw/public_html/%s.html", "who");
   if ((wfp = fopen(filename, "w")) == NULL) {
	sprintf(buf, "Unable to open <%s> for write.", filename);
	log_string(buf);
	return;
   }
   
   fprintf(wfp, "<BODY TEXT=\"darkgray\" BGCOLOR=\"black\" LINK=\"darkgray\">");
   fprintf(wfp, "<FONT FACE=\"Courier\"><B>\n");
   fprintf(wfp, "<PRE><H1>The Shadow <FONT COLOR=\"darkred\">Wars</FONT> Online players</H1>");
   fprintf(wfp, "Created with webwho Copyright &copy; 1995-2004 by The Shadow Wars\n");
   sprintf(buf, "%s",  (char *) ctime(&current_time));
   buf[strlen(buf)-1] = '\0';
   fprintf(wfp, "Last update at <FONT COLOR=\"goldenrod\">%s</FONT> [next update in <FONT COLOR=\"goldenrod\">%d</FONT> minutes]\n", buf, (PULSE_WEB/PULSE_PER_SECOND)/60);

   immptr  = str_dup(out_buf_imm);
   mortptr = str_dup(out_buf_mort);
   
   fprintf(wfp, color2web(immptr));
   fprintf(wfp, color2web(mortptr));
   fclose(wfp);
  
   return;
}

void do_whoinvis( CHAR_DATA *ch, char *argument )
{
   if (IS_NPC(ch))
     return;
     
	
   if (IS_PKILLER(ch))
   {
	send_to_char("Pkillers can not hide that they are online.\r\n",ch);
	return;
   }

   if (IS_FACELESS(ch))
   {
	send_to_char("There is no reason for a temporary character to hide that they are online.\r\n",ch);
	return;
   }

   if ( ch->incog_level) {
      ch->incog_level = 0;
      send_to_char( "You are no longer whoinvis.\n\r", ch );
      return;
   }
   else {
      ch->incog_level = 1;
      send_to_char( "You are now whoinvis.\n\r", ch );
      return;	
   }
   return;
}

void do_count ( CHAR_DATA *ch, char *argument )
{
    int count;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];

    count = 0;

    for ( d = descriptor_list; d != NULL; d = d->next )
        if ( d->connected == CON_PLAYING && can_see_channel( ch, d->character ) )
	    count++;

    max_on = UMAX(count,max_on);

    if (max_on == count)
        sprintf(buf,"There are %d characters on, the most so far today.\n\r",
	    count);
    else
	sprintf(buf,"There are %d characters on, the most on today was %d.\n\r",
	    count,max_on);

    send_to_char(buf,ch);
}

void do_inventory( CHAR_DATA *ch, char *argument )
{
   OBJ_DATA *obj;
   bool found=FALSE;

   send_to_char( "You are carrying:\n\r", ch );

   if (argument[0] != '\0') {
      if (!str_cmp (argument, "name")) {
        for ( obj = ch->carrying; obj != NULL; obj = obj->next_content ) {
            if (obj->wear_loc == WEAR_NONE) {
               found=TRUE;
               send_to_char(obj->name, ch);
               send_to_char("\n\r", ch);
            }
        }
        if (!found) {
            send_to_char("Nothing.\n\r", ch);
        }
        return;
      }
      else {
         show_list_to_char( ch->carrying, ch, TRUE, TRUE, FALSE );
         return;
      }
   }
   else {
      show_list_to_char( ch->carrying, ch, TRUE, TRUE, FALSE );
      return;
   }    
    return;
}



void do_equipment( CHAR_DATA *ch, char *argument )
{
  OBJ_DATA *obj;
  int iWear;
  bool found;
  
  if (IS_WOLFSHAPE(ch)) {
    send_to_char("You have the shape of a wolf.\n\r", ch);	 
    return;
  }
  
  send_to_char( "You are using:\n\r", ch );
  found = FALSE;
  
  for ( iWear = 0; iWear < MAX_WEAR; iWear++ ) {
    if (iWear == WEAR_SCABBARD_1 && !IS_NPC(ch) && !IS_NULLSTR(ch->sheat_where_name[0]))
	 send_to_char(ch->sheat_where_name[0], ch);
    else if (iWear == WEAR_SCABBARD_2 && !IS_NPC(ch) && !IS_NULLSTR(ch->sheat_where_name[1]))
	 send_to_char(ch->sheat_where_name[1], ch);
    else
	 send_to_char( where_name[iWear], ch );
    
    if ( ( obj = get_eq_char( ch, iWear ) ) == NULL ) {
	 send_to_char( "{Dnothing{x\n\r", ch );
	 continue;
    }

    if ( can_see_obj( ch, obj ) ) {
	 send_to_char( format_obj_to_char( obj, ch, TRUE, FALSE ), ch );
	 if (iWear == WEAR_STUCK_IN && ch->arrow_count > 1) {
	   char buf[MAX_STRING_LENGTH];
	   sprintf(buf,"(%d) ",ch->arrow_count);
	   send_to_char(buf,ch);
	 }
	 send_to_char( "\n\r", ch );
    }
    else {
	 send_to_char( "{Dsomething{x\n\r", ch );
    }
    found = TRUE;
  }

  if ( !found )
    send_to_char( "{Dnothing{x\n\r", ch );

  return;
}



void do_compare( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj1;
    OBJ_DATA *obj2;
    int value1;
    int value2;
    char *msg;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Compare what to what?\n\r", ch );
	return;
    }

    if ( ( obj1 = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if (arg2[0] == '\0')
    {
	for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
	{
	    if (obj2->wear_loc != WEAR_NONE
	    &&  can_see_obj(ch,obj2)
	    &&  obj1->item_type == obj2->item_type
	    &&  (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0 )
		break;
	}

	if (obj2 == NULL)
	{
	    send_to_char("You aren't wearing anything comparable.\n\r",ch);
	    return;
	}
    } 

    else if ( (obj2 = get_obj_carry(ch,arg2,ch) ) == NULL )
    {
	send_to_char("You do not have that item.\n\r",ch);
	return;
    }

    msg		= NULL;
    value1	= 0;
    value2	= 0;

    if ( obj1 == obj2 )
    {
	msg = "You compare $p to itself.  It looks about the same.";
    }
    else if ( obj1->item_type != obj2->item_type )
    {
	msg = "You can't compare $p and $P.";
    }
    else
    {
	switch ( obj1->item_type )
	{
	default:
	    msg = "You can't compare $p and $P.";
	    break;

	case ITEM_ARMOR:
	    value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
	    value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
	    break;

	case ITEM_WEAPON:
	    if (obj1->pIndexData->new_format)
		value1 = (1 + obj1->value[2]) * obj1->value[1];
	    else
	    	value1 = obj1->value[1] + obj1->value[2];

	    if (obj2->pIndexData->new_format)
		value2 = (1 + obj2->value[2]) * obj2->value[1];
	    else
	    	value2 = obj2->value[1] + obj2->value[2];
	    break;
	}
    }

    if ( msg == NULL )
    {
	     if ( value1 == value2 ) msg = "$p and $P look about the same.";
	else if ( value1  > value2 ) msg = "$p looks better than $P.";
	else                         msg = "$p looks worse than $P.";
    }

    act( msg, ch, obj1, obj2, TO_CHAR );
    return;
}



void do_credits( CHAR_DATA *ch, char *argument )
{
    do_function(ch, &do_help, "diku" );
    return;
}



void do_where( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    bool found;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Players near you:\n\r", ch );
	found = FALSE;
	for ( d = descriptor_list; d; d = d->next )
	{
	    if ( d->connected == CON_PLAYING
	    && ( victim = d->character ) != NULL
	    &&   !IS_NPC(victim)
	    &&   victim->in_room != NULL
	    &&   !IS_SET(victim->in_room->room_flags,ROOM_NOWHERE)
 	    &&   (is_room_owner(ch,victim->in_room) 
	    ||    !room_is_private(victim->in_room))
	    &&   victim->in_room->area == ch->in_room->area
	    &&   can_see( ch, victim ) )
	    {
		found = TRUE;
		sprintf( buf, "%-28s %s\n\r",
		    victim->name, victim->in_room->name );
		send_to_char( buf, ch );
	    }
	}
	if ( !found )
	    send_to_char( "None\n\r", ch );
    }
    else
    {
	found = FALSE;
	for ( victim = char_list; victim != NULL; victim = victim->next )
	{
	    if ( victim->in_room != NULL
	    &&   victim->in_room->area == ch->in_room->area
	    &&   !IS_AFFECTED(victim, AFF_HIDE)
	    &&   !IS_AFFECTED(victim, AFF_SNEAK)
	    &&   can_see( ch, victim )
	    &&   is_name( arg, victim->name ) )
	    {
		found = TRUE;
		sprintf( buf, "%-28s %s\n\r",
		    PERS(victim, ch), victim->in_room->name );
		send_to_char( buf, ch );
		break;
	    }
	}
	if ( !found )
	    act( "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
    }

    return;
}




void do_consider( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char *msg;
    int diff;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Consider killing whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They're not here.\n\r", ch );
	return;
    }

    if (is_safe(ch,victim))
    {
	send_to_char("Don't even think about it.\n\r",ch);
	return;
    }

    diff = victim->level - ch->level;

         if ( diff <= -10 ) msg = "$N is not worth the effort";
    else if ( diff <=  -6 ) msg = "You won't even break a sweat with $N.";
    else if ( diff <=  -2 ) msg = "$N looks like he'd be good for sparring practice.";
    else if ( diff <=   2 ) msg = "Looks like it'd be a fair fight!";
    else if ( diff <=   6 ) msg = "$N might hurt a bit.";
    else if ( diff <=   9 ) msg = "$N could be taken, on a really good day.";
    else if ( diff <=   12 ) msg = "$N will bring on the pain.";
    else                    msg = "Trust me, just walk away";

    act( msg, ch, NULL, victim, TO_CHAR );
    return;
}

void set_ictitle( CHAR_DATA *ch, char *title )
{
  char buf[MAX_STRING_LENGTH];
  
  if ( IS_NPC(ch) ) {
    bug( "Set_ictitle: NPC.", 0 );
    return;
  }
  
  if (!str_cmp(title, "reset")) {
    strcpy(buf, "");
  }
  else {
    if ( title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' ) {
	 strcpy( buf, title );
    }
    else {
	 buf[0] = ' ';
	 strcpy( buf+1, title );
    }
  }
  
  free_string( ch->pcdata->ictitle );
  ch->pcdata->ictitle = str_dup( buf );
  return;
}

void set_title( CHAR_DATA *ch, char *title )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
    {
	bug( "Set_title: NPC.", 0 );
	return;
    }

    if ( title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' )
    {
	buf[0] = ' ';
	strcpy( buf+1, title );
    }
    else
    {
	strcpy( buf, title );
    }

    free_string( ch->pcdata->title );
    ch->pcdata->title = str_dup( buf );
    return;
}

void set_imminfo( CHAR_DATA *ch, char *title )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
    {
	bug( "Set_imminfo: NPC.", 0 );
	return;
    }
    
    if (!str_cmp(title, "reset")) {
      strcpy (buf, "");
    }
    else {
      strcpy( buf, title );
    }

    free_string( ch->pcdata->imm_info );
    ch->pcdata->imm_info = str_dup( buf );
    return;
}

void set_appearance( CHAR_DATA *ch, char *app )
{
  char buf[MAX_STRING_LENGTH];

  if ( IS_NPC(ch) ) {
    bug( "Set_appearance: NPC.", 0 );
    return;
  }

  if ( app[0] != '.' && app[0] != ',' && app[0] != '!' && app[0] != '?' ) {
    strcpy( buf, app );
  }
  else {
    buf[0] = ' ';
    strcpy( buf+1, app );
  }
  
  free_string( ch->pcdata->appearance );
  ch->pcdata->appearance = str_dup( buf );
  return;
}

void set_dfname( CHAR_DATA *ch, char *app )
{
  char buf[MAX_STRING_LENGTH];

  if ( IS_NPC(ch) ) {
    bug( "Set_dfname: NPC.", 0 );
    return;
  }

  if ( app[0] != '.' && app[0] != ',' && app[0] != '!' && app[0] != '?' ) {
    strcpy( buf, app );
  }
  else {
    buf[0] = ' ';
    strcpy( buf+1, app );
  }
  
  free_string( ch->pcdata->df_name );
  ch->pcdata->df_name = str_dup( buf );
  return;
}

int colorstrlen(char *argument)
{
  char *str;
  int strlength;

  if (argument == NULL || argument[0] == '\0')
    return 0;

  strlength = 0;
  str = argument;

  while (*str != '\0') {
    if ( *str != '{' ) {
      str++;
      strlength++;
      continue;
    }
    
    if (*(++str) == '{')
      strlength++;
    
    str++;
  }
  return strlength;
}

void do_imminfo( CHAR_DATA *ch, char *argument )
{
  if ( IS_NPC(ch) )
    return;

  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
    
  
  if ( argument[0] == '\0' ) {
    send_to_char( "Change your imminfo field to what?\n\r", ch );
    return;
  }
  
  if ( colorstrlen(argument) > 18 ) {
    send_to_char("Imminfo must be 18 (or under) characters long.\n\r", ch); 
    return;
    /* argument[18] = '\0'; */
  }
  
  smash_tilde( argument );
  set_imminfo( ch, argument );
  send_to_char( "Ok.\n\r", ch );
}

void do_appearance( CHAR_DATA *ch, char *argument )
{
  char name[MAX_INPUT_LENGTH];
  char app[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;

  smash_tilde( argument );
  argument = one_argument( argument, name );
  strcpy(app, argument);

  if ( name[0] == '\0') {
    send_to_char("Syntax:\n\r", ch );
    send_to_char("setapp <name> <string>\n\r", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && !IS_NEWBIEHELPER(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if ( ( victim = get_char_world( ch, name ) ) == NULL ) {
    buf[0] = '\0';
    sprintf(buf, "%s aren't here.\n\r", name);
    send_to_char(buf, ch);
    return;
  }

  if (IS_NEWBIEHELPER(ch) && !IS_SET(victim->act, PLR_UNVALIDATED)) {
     sprintf(buf, "%s is already validated.\n\r", victim->name);
     send_to_char(buf, ch );
     return;
  }

  if (IS_NPC(victim)) {
    send_to_char("You can't set a NPCs appearance. Use medit and set long string for that!\n\r", ch);
    return;
  }
  
  if (victim->level > ch->level ) {
    send_to_char( "You can't change a high level persons appearance!\n\r", ch);
    return;
  }    

  if ( app[0] == '\0') {
    buf[0] = '\0';
    strcpy(buf, "");
    sprintf(buf, "%s's appearance is currently: %s\n\r", victim->name,
	    victim->pcdata->appearance);
    send_to_char(buf, ch);
    return;
  }

  set_appearance(victim, app);

  /* print info to char */
  buf[0] = '\0';
  sprintf(buf, "%s's appearance set to: %s\n\r", victim->name,
	  victim->pcdata->appearance);
  send_to_char(buf, ch);

  /* print info to victim */
  if (victim != ch) {
    buf[0] = '\0';
    sprintf(buf,"Your appearance has been set by an immortal to: %s\n\r", 
	    victim->pcdata->appearance);
    send_to_char(buf, victim);
  }

  /* send info to wiznet */
  buf[0] = '\0';
  sprintf(buf,"$N sets %s's appearance to: %s",victim->name,victim->pcdata->appearance);
  wiznet(buf,ch,NULL,WIZ_ON,WIZ_SECURE,0);
  return;
}

void do_ictitle( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
	return;

    if ( argument[0] == '\0' )
    {
      buf[0] = '\0';
      strcpy(buf, "");
      sprintf(buf, "Your ictitle is currently: %s\n\r", !IS_NULLSTR(ch->pcdata->ictitle) ? ch->pcdata->ictitle : "(none)");
      send_to_char(buf, ch);
      return;
    }

    if ( strlen(argument) > 45 )
	argument[45] = '\0';

    smash_tilde( argument );
    set_ictitle( ch, argument );
    send_to_char( "Ok.\n\r", ch );
}

void do_title( CHAR_DATA *ch, char *argument )
{
  if ( IS_NPC(ch) )
    return;
 
  if (IS_SET(ch->act, PLR_UNVALIDATED)) {
    send_to_char("Unvalidated players are not allowed to change their title.\n\r", ch);
    return;
  }
 
  if ( argument[0] == '\0' ) {
    send_to_char( "Change your title to what?\n\r", ch );
    return;
  }
  
  if ( colorstrlen(argument) > 45 ) {    
    send_to_char("Titles must be less than 45 characters long (not including color codes).\n\r", ch);
    return;
  }
  
  smash_tilde( argument );
  set_title( ch, argument );
  send_to_char( "Ok.\n\r", ch );
}

void do_description( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( argument[0] != '\0' ) {
      buf[0] = '\0';
      smash_tilde( argument );

      if (!str_cmp(argument, "write")) {
	string_append(ch, &ch->description);
	return;
      }
      
      if (argument[0] == '-') {
	int len;
	bool found = FALSE;
	
	if (ch->description == NULL || ch->description[0] == '\0') {
	  send_to_char("No lines left to remove.\n\r",ch);
	  return;
	}
	
	strcpy(buf,ch->description);
	
	for (len = strlen(buf); len > 0; len--) {
	  if (buf[len] == '\r') {
	    if (!found)  { /* back it up */
	      if (len > 0)
		len--;
	      found = TRUE;
	    }
	    else { /* found the second one */
	      buf[len + 1] = '\0';
	      free_string(ch->description);
	      ch->description = str_dup(buf);
	      send_to_char( "Your description is:\n\r", ch );
	      send_to_char( ch->description ? ch->description : 
			    "(None).\n\r", ch );
	      return;
	    }
	  }
	}
	buf[0] = '\0';
	free_string(ch->description);
	ch->description = str_dup(buf);
	send_to_char("Description cleared.\n\r",ch);
	return;
      }
      if ( argument[0] == '+' ) {
	if ( ch->description != NULL )
	  strcat( buf, ch->description );
	argument++;
	while ( isspace(*argument) )
	  argument++;
      }
      
      if ( strlen(buf) >= 1024) {
	send_to_char( "Description too long.\n\r", ch );
	return;
      }
      
      strcat( buf, argument );
      strcat( buf, "\n\r" );
      free_string( ch->description );
      ch->description = str_dup( buf );
    }
    
    send_to_char( "Your description is:\n\r", ch );
    send_to_char( ch->description ? ch->description : "(None).\n\r", ch );
    return;
}

void do_hooddescription( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
     return;

  if (IS_IMMORTAL(ch)) {
    send_to_char("Immortals should not go about sneaking.\n\r", ch);
    return;
  }

    if ( argument[0] != '\0' ) {
      buf[0] = '\0';
      smash_tilde( argument );

      if (!str_cmp(argument, "write")) {
	string_append(ch, &ch->hood_description);
	return;
      }
      
      if (argument[0] == '-') {
	int len;
	bool found = FALSE;
	
	if (ch->hood_description == NULL || ch->hood_description[0] == '\0') {
	  send_to_char("No lines left to remove.\n\r",ch);
	  return;
	}
	
	strcpy(buf,ch->hood_description);
	
	for (len = strlen(buf); len > 0; len--) {
	  if (buf[len] == '\r') {
	    if (!found)  { /* back it up */
	      if (len > 0)
		len--;
	      found = TRUE;
	    }
	    else { /* found the second one */
	      buf[len + 1] = '\0';
	      free_string(ch->hood_description);
	      ch->hood_description = str_dup(buf);
	      send_to_char( "Your hood description is:\n\r", ch );
	      send_to_char( ch->hood_description ? ch->hood_description : 
			    "(None).\n\r", ch );
	      return;
	    }
	  }
	}
	buf[0] = '\0';
	free_string(ch->hood_description);
	ch->hood_description = str_dup(buf);
	send_to_char("Hood description cleared.\n\r",ch);
	return;
      }
      if ( argument[0] == '+' ) {
	if ( ch->hood_description != NULL )
	  strcat( buf, ch->hood_description );
	argument++;
	while ( isspace(*argument) )
	  argument++;
      }
      
      if ( strlen(buf) >= 1024) {
	send_to_char( "Hood description too long.\n\r", ch );
	return;
      }
      
      strcat( buf, argument );
      strcat( buf, "\n\r" );
      free_string( ch->hood_description );
      ch->hood_description = str_dup( buf );
    }
    
    send_to_char( "Your hood description is:\n\r", ch );
    send_to_char( ch->hood_description ? ch->hood_description : "(None).\n\r", ch );
    return;
}

void do_veildescription( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
     return;

  if (IS_IMMORTAL(ch)) {
    send_to_char("Immortals should not go about sneaking.\n\r", ch);
    return;
  }

  if (!IS_AIEL(ch) && !IS_TARABONER(ch)) {
    send_to_char("You are not one of the Aiels.\n\r", ch);
    return;  	
  }

    if ( argument[0] != '\0' ) {
      buf[0] = '\0';
      smash_tilde( argument );

      if (!str_cmp(argument, "write")) {
	string_append(ch, &ch->veil_description);
	return;
      }
      
      if (argument[0] == '-') {
	int len;
	bool found = FALSE;
	
	if (ch->veil_description == NULL || ch->veil_description[0] == '\0') {
	  send_to_char("No lines left to remove.\n\r",ch);
	  return;
	}
	
	strcpy(buf,ch->veil_description);
	
	for (len = strlen(buf); len > 0; len--) {
	  if (buf[len] == '\r') {
	    if (!found)  { /* back it up */
	      if (len > 0)
		len--;
	      found = TRUE;
	    }
	    else { /* found the second one */
	      buf[len + 1] = '\0';
	      free_string(ch->veil_description);
	      ch->veil_description = str_dup(buf);
	      send_to_char( "Your veil description is:\n\r", ch );
	      send_to_char( ch->veil_description ? ch->veil_description : 
			    "(None).\n\r", ch );
	      return;
	    }
	  }
	}
	buf[0] = '\0';
	free_string(ch->veil_description);
	ch->veil_description = str_dup(buf);
	send_to_char("Veil description cleared.\n\r",ch);
	return;
      }
      if ( argument[0] == '+' ) {
	if ( ch->veil_description != NULL )
	  strcat( buf, ch->veil_description );
	argument++;
	while ( isspace(*argument) )
	  argument++;
      }
      
      if ( strlen(buf) >= 1024) {
	send_to_char( "Veil description too long.\n\r", ch );
	return;
      }
      
      strcat( buf, argument );
      strcat( buf, "\n\r" );
      free_string( ch->veil_description );
      ch->veil_description = str_dup( buf );
    }
    
    send_to_char( "Your veil description is:\n\r", ch );
    send_to_char( ch->veil_description ? ch->veil_description : "(None).\n\r", ch );
    return;
}

void do_wounddescription( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  
  if (IS_NPC(ch))
    return;
  
  if ( argument[0] != '\0' ) {
    buf[0] = '\0';
    smash_tilde( argument );
    
    if (!str_cmp(argument, "write")) {
	 string_append(ch, &ch->wound_description);
	 return;
    }
    
    if (argument[0] == '-') {
	 int len;
	 bool found = FALSE;
	 
	 if (ch->wound_description == NULL || ch->wound_description[0] == '\0') {
	   send_to_char("No lines left to remove.\n\r",ch);
	   return;
	 }
	 
	 strcpy(buf,ch->wound_description);
	 
	 for (len = strlen(buf); len > 0; len--) {
	   if (buf[len] == '\r') {
		if (!found)  { /* back it up */
		  if (len > 0)
		    len--;
		  found = TRUE;
		}
		else { /* found the second one */
		  buf[len + 1] = '\0';
		  free_string(ch->wound_description);
		  ch->wound_description = str_dup(buf);
		  send_to_char( "Your wound description is:\n\r", ch );
		  send_to_char( ch->wound_description ? ch->wound_description : 
					 "(None).\n\r", ch );
		  return;
		}
	   }
	 }
	 buf[0] = '\0';
	 free_string(ch->wound_description);
	 ch->wound_description = str_dup(buf);
	 send_to_char("Wound description cleared.\n\r",ch);
	 return;
    }
    if ( argument[0] == '+' ) {
	 if ( ch->wound_description != NULL )
	   strcat( buf, ch->wound_description );
	 argument++;
	 while ( isspace(*argument) )
	   argument++;
    }
    
    if ( strlen(buf) >= 1024) {
	 send_to_char( "Wound description too long.\n\r", ch );
	 return;
    }
    
    strcat( buf, argument );
    strcat( buf, "\n\r" );
    free_string( ch->wound_description );
    ch->wound_description = str_dup( buf );
  }
  
  send_to_char( "Your wound description is:\n\r", ch );
  send_to_char( ch->wound_description ? ch->wound_description : "(None).\n\r", ch );
  return;
}

void do_auradescription( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;

  if (IS_NPC(ch))
    return;
  
  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (IS_NULLSTR(argument)) {
    send_to_char("Who do you want to set an aura on?\n\r", ch);
    return;
  }
    
  if ((victim = get_char_world(ch, argument)) == NULL) {
    send_to_char("They aren't here.\n\r", ch);
    return;
  }

  sprintf(buf, "%s set aura description on %s.", ch->name, victim->name);
  wiznet(buf, ch, NULL, WIZ_SECURE, 0, get_trust(ch));
  
  sprintf(buf, "%s set aura description on %s.", ch->name, victim->name);
  log_string(buf);

  string_append(ch, &victim->aura_description);
  return;
}

void do_viewaura( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;

  if (IS_NPC(ch))
    return;
  
  if (!IS_SET(ch->talents, TALENT_VIEWING) && !IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (IS_NULLSTR(argument)) {
    send_to_char("Who do you want to view the aura from?\n\r", ch);
    return;
  }

  if ((victim = get_char_room(ch, argument)) == NULL) {
    send_to_char("They aren't here.\n\r", ch);
    return;
  }
  if (!IS_RP(victim)) {
	send_to_char("They are not in IC mode.\n\r", ch);
	return;
  }
  if (!IS_RP(ch) && !IS_IMMORTAL(ch)) {
	send_to_char("You are not in IC mode.\n\r", ch);
	return;
  }

  if (!IS_NULLSTR(victim->aura_description)) {
    send_to_char(victim->aura_description, ch);    
  }
  else {
    act( "$N have no visible aura around $M.", ch, NULL, victim, TO_CHAR);
  }

  return;
}

void do_report( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];

    if (argument[0]  == '\0') {
       send_to_char("Syntax:  Report health\n\r"
                    "         Report health%\n\r"
                    "         Report spheres\n\r"
                    "         Report stats\n\r", ch);
       return;
    }

    if (!str_prefix (argument, "health")) {
       sprintf( buf,
               "You say '{7I have %d/%d hp %d/%d endurance.{x'\n\r",
               ch->hit,  ch->max_hit,
               ch->endurance, ch->max_endurance);
       send_to_char( buf, ch );

       sprintf( buf, "$n says '{7I have %d/%d hp %d/%d endurance.{x'",
                ch->hit,  ch->max_hit,
                ch->endurance, ch->max_endurance);
       act( buf, ch, NULL, NULL, TO_ROOM );
       return;
    }

    if (!str_prefix (argument, "health%")) {
	 sprintf( buf,
			"You say '{7I have %3d%c hp %3d%c endurance.{x'\n\r",
			((100*ch->hit)/ch->max_hit),
			'%',
			((100*ch->endurance)/ch->max_endurance),
			'%');
	 send_to_char( buf, ch );
	 
	 sprintf( buf, "$n says '{7I have %3d%c hp %3d%c endurance.{x'",
			((100*ch->hit)/ch->max_hit),
			'%',
			((100*ch->endurance)/ch->max_endurance),
			'%');
	 act( buf, ch, NULL, NULL, TO_ROOM );
	 return;
    }

    if (!str_prefix (argument, "spheres")) {
       if (!IS_IMMORTAL(ch) && (ch->class != CLASS_CHANNELER)) {
          send_to_char("You have no idea about channeling, what so ever!\n\r", ch);
          return;
       }
	  
       sprintf( buf,
			 "You say '{7I have  %d Air  %d Earth  %d Fire  %d Spirit  %d Water.{x'\n\r",
			 ch->perm_sphere[SPHERE_AIR], ch->perm_sphere[SPHERE_EARTH], 
			 ch->perm_sphere[SPHERE_FIRE], ch->perm_sphere[SPHERE_SPIRIT],
			 ch->perm_sphere[SPHERE_WATER]);
       send_to_char( buf, ch );

       sprintf( buf, "$n says '{7I have  %d Air  %d Earth  %d Fire  %d Spirit  %d Water.{x'\n\r",
			 ch->perm_sphere[SPHERE_AIR], ch->perm_sphere[SPHERE_EARTH], 
			 ch->perm_sphere[SPHERE_FIRE], ch->perm_sphere[SPHERE_SPIRIT],
			 ch->perm_sphere[SPHERE_WATER]);
       act( buf, ch, NULL, NULL, TO_ROOM );
       return;
    }

    if (!str_prefix (argument, "stats")) {
       sprintf( buf,
               "You say '{7I have  %d/%d Str  %d/%d Int  %d/%d Wis  %d/%d Dex  %d/%d Con.{x'\n\r",
               ch->perm_stat[STAT_STR], get_curr_stat(ch,STAT_STR),
               ch->perm_stat[STAT_INT], get_curr_stat(ch,STAT_INT),
               ch->perm_stat[STAT_WIS], get_curr_stat(ch,STAT_WIS),
               ch->perm_stat[STAT_DEX], get_curr_stat(ch,STAT_DEX),
               ch->perm_stat[STAT_CON], get_curr_stat(ch,STAT_CON));
       send_to_char( buf, ch );

       sprintf( buf, "$n says '{7I have  %d/%d Str  %d/%d Int  %d/%d Wis  %d/%d Dex  %d/%d Con.{x'\n\r",
               ch->perm_stat[STAT_STR], get_curr_stat(ch,STAT_STR),
               ch->perm_stat[STAT_INT], get_curr_stat(ch,STAT_INT),
               ch->perm_stat[STAT_WIS], get_curr_stat(ch,STAT_WIS),
               ch->perm_stat[STAT_DEX], get_curr_stat(ch,STAT_DEX),
               ch->perm_stat[STAT_CON], get_curr_stat(ch,STAT_CON));
       act( buf, ch, NULL, NULL, TO_ROOM );
       return;
    }

    return;    
}

/* 
 * We don't do it like this anymore: Atwain

void do_practice( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int sn;

    if ( IS_NPC(ch) )
	return;

    if ( argument[0] == '\0' )
    {
	int col;

	col    = 0;
	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
	    if ( skill_table[sn].name == NULL )
		break;
	    if ( ch->level < skill_table[sn].skill_level[ch->class] 
	    || ch->pcdata->learned[sn] < 1)
		continue;

	    sprintf( buf, "%-18s %3d%%  ",
		skill_table[sn].name, ch->pcdata->learned[sn] );
	    send_to_char( buf, ch );
	    if ( ++col % 3 == 0 )
		send_to_char( "\n\r", ch );
	}

	if ( col % 3 != 0 )
	    send_to_char( "\n\r", ch );

	sprintf( buf, "You have %d practice sessions left.\n\r",
	    ch->practice );
	send_to_char( buf, ch );
    }
    else
    {
	CHAR_DATA *mob;
	int adept;

	if ( !IS_AWAKE(ch) )
	{
	    send_to_char( "In your dreams, or what?\n\r", ch );
	    return;
	}

	for ( mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room )
	{
	    if ( IS_NPC(mob) && IS_SET(mob->act, ACT_PRACTICE) )
		break;
	}

	if ( mob == NULL )
	{
	    send_to_char( "You can't do that here.\n\r", ch );
	    return;
	}

	if ( ch->practice <= 0 )
	{
	    send_to_char( "You have no practice sessions left.\n\r", ch );
	    return;
	}

	if ( ( sn = find_spell( ch,argument ) ) < 0
		|| ( !IS_NPC(ch)
	&&   (ch->level < skill_table[sn].skill_level[ch->class] 
 	||    ch->pcdata->learned[sn] < 1
	||    skill_table[sn].rating[ch->class] == 0)))
	{
	    send_to_char( "You can't practice that.\n\r", ch );
	    return;
	}

	adept = IS_NPC(ch) ? 100 : class_table[ch->class].skill_adept;

	if ( ch->pcdata->learned[sn] >= adept )
	{
	    sprintf( buf, "You are already learned at %s.\n\r",
		skill_table[sn].name );
	    send_to_char( buf, ch );
	}
	else
	{
	    ch->practice--;
	    ch->pcdata->learned[sn] += 
		int_app[get_curr_stat(ch,STAT_INT)].learn / 
	        skill_table[sn].rating[ch->class];
	    if ( ch->pcdata->learned[sn] < adept )
	    {
		act( "You practice $T.",
		    ch, NULL, skill_table[sn].name, TO_CHAR );
		act( "$n practices $T.",
		    ch, NULL, skill_table[sn].name, TO_ROOM );
	    }
	    else
	    {
		ch->pcdata->learned[sn] = adept;
		act( "You are now learned at $T.",
		    ch, NULL, skill_table[sn].name, TO_CHAR );
		act( "$n is now learned at $T.",
		    ch, NULL, skill_table[sn].name, TO_ROOM );
	    }
	}
    }
    return;
}
*/


/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
	wimpy = ch->max_hit / 5;
    else
	wimpy = atoi( arg );

    if ( wimpy < 0 )
    {
	send_to_char( "Your courage exceeds your wisdom.\n\r", ch );
	return;
    }

    if ( wimpy > ch->max_hit/2 )
    {
	send_to_char( "Such cowardice ill becomes you.\n\r", ch );
	return;
    }

    ch->wimpy	= wimpy;
    sprintf( buf, "Wimpy set to %d hit points.\n\r", wimpy );
    send_to_char( buf, ch );
    return;
}



void do_password( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;

    if ( IS_NPC(ch) )
	return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Syntax: password <old> <new>.\n\r", ch );
	return;
    }

    if ( strcmp( crypt( arg1, ch->pcdata->pwd ), ch->pcdata->pwd ) )
    {
	WAIT_STATE( ch, 40 );
	send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
	return;
    }

    if ( strlen(arg2) < 5 )
    {
	send_to_char(
	    "New password must be at least five characters long.\n\r", ch );
	return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt( arg2, ch->name );
    for ( p = pwdnew; *p != '\0'; p++ )
    {
	if ( *p == '~' )
	{
	    send_to_char(
		"New password not acceptable, try again.\n\r", ch );
	    return;
	}
    }

    free_string( ch->pcdata->pwd );
    ch->pcdata->pwd = str_dup( pwdnew );

    save_char_obj( ch, FALSE );

    send_to_char( "Ok.\n\r", ch );
    return;
}

void do_finger (CHAR_DATA *ch, char *arg)  
{
 struct stat sb;
 char buf[MAX_INPUT_LENGTH];
 int er;
 time_t com_time;
 CHAR_DATA *victim;
 
 if (arg[0] == '\0')     
 {
  printf_to_char(ch,"\n\rFinger usage:\n\r\n\r");
  printf_to_char(ch," finger bob\n\r");
  printf_to_char(ch," where bob is a character name.\n\r");
  printf_to_char(ch," characters not visiable on your who list\n\r");
  printf_to_char(ch," but who are on will finger as not on.\n\r");
  return;
 } 

 if ((!str_cmp(arg,"Ishamael") ||
     !str_cmp(arg,"Asmodean") ||
     !str_cmp(arg,"Rahvin") ||
     !str_cmp(arg,"Sammael") ||
     !str_cmp(arg,"Belal") ||
     !str_cmp(arg,"Aginor") ||
     !str_cmp(arg,"Moridin") ||
     !str_cmp(arg,"Graendal") ||
     !str_cmp(arg,"Lanfear") ||
     !str_cmp(arg,"Moghedian") ||
     !str_cmp(arg,"Mesaana") ||
     !str_cmp(arg,"Cyndane") ||
     !str_cmp(arg,"Semirhage")) && !IS_IMMORTAL(ch))
 {
	send_to_char("You can not finger the forsaken!\n\r",ch);
	return;
 }

 if ((victim = get_char_world(ch,arg)) != NULL && !str_cmp(victim->name, arg)) {  /* they are online */
   if (IS_NPC(victim)) {
	printf_to_char(ch,"\n\rYou can't finger mobiles!\n\r");
     return;
   }

   if ((IS_IMMORTAL(victim)) && (!IS_IMMORTAL(ch)))   {
	printf_to_char(ch,"\n\rYou can't finger immortals!\n\r");
	return;
   }

   if (!IS_IMMORTAL(ch) && IS_SET(victim->act2,PLR2_NOFINGER))
   {
	printf_to_char(ch,"\n\rYou can not finger that person.\n\r");
	return;
   }
   if (victim->desc == NULL) {
	printf_to_char(ch, "\n\r%s is link dead\n\r", capitalize(arg));
	return;
   }

  if ((!IS_IMMORTAL(ch)) && (victim->incog_level == 1)) {
    com_time = (victim->logon); // + (ch->pcdata->jetlag * 3600);
    printf_to_char(ch,"\n\r%s last logged off at %s",capitalize(arg),  
		   ctime( &com_time ) );  
    return;
  }

  if (IS_IMMORTAL(ch) && (victim->incog_level > get_trust(ch) || victim->invis_level > get_trust(ch)))
  {
    com_time = (victim->logon); // + (ch->pcdata->jetlag * 3600);
    printf_to_char(ch,"\n\r%s last logged off at %s",capitalize(arg),  
		   ctime( &com_time ) );  
    return;
  }
  printf_to_char(ch,"\n\r%s is online.\n\r",capitalize(arg));  
 }   
 else
 {
  if (!IS_IMMORTAL(ch))
  {
   sprintf(buf,"%s%s",GOD_DIR,capitalize(arg));
   er = stat(buf,&sb);
   if (er == 0 )
   {
    printf_to_char(ch,"\n\rYou can't finger immortals!\n\r");
    return;
   }
  }

  sprintf(buf,"%s%s",PLAYER_DIR,capitalize(arg));
  er = stat(buf,&sb);
  if (er == 0)
  {
    DESCRIPTOR_DATA *d=NULL;
    d = new_descriptor();
    bool found = FALSE;
    found = load_char_obj(d,arg,FALSE);
    if (IS_SET(d->character->act2,PLR2_NOFINGER))
    {
   	if (!IS_IMMORTAL(ch) && IS_SET(d->character->act2,PLR2_NOFINGER))
   	{
		printf_to_char(ch,"\n\rYou can not finger that person.\n\r");
    		free_char(d->character);
    		free_descriptor(d);
		return;
   	}
    }
    free_char(d->character);
    free_descriptor(d);
    com_time = (sb.st_mtime); // + (ch->pcdata->jetlag * 3600);  
    printf_to_char(ch,"\n\r%s last logged off at %s",capitalize(arg),
			    ctime( &com_time ) );    
    return;
  }

  /* Also check diguise directory */
  if (er == -1) {
    sprintf(buf,"%s%s",PLAYER_DISGUISE_DIR,capitalize(arg));
    er = stat(buf,&sb);
  }
    
  if (er == 0)
  {
    DESCRIPTOR_DATA *d= new_descriptor();
    bool found = FALSE;
    found = load_char_obj(d,arg,TRUE);
    if (IS_SET(d->character->act2,PLR2_NOFINGER))
    {
        if (!IS_IMMORTAL(ch) && IS_SET(d->character->act2,PLR2_NOFINGER))
        {
                printf_to_char(ch,"\n\rYou can not finger that person.\n\r");
                free_char(d->character);
                free_descriptor(d);
                return;
        }
    }
    free_char(d->character);
    free_descriptor(d);
    com_time = (sb.st_mtime); // + (ch->pcdata->jetlag * 3600);  
    printf_to_char(ch,"\n\r%s last logged off at %s",capitalize(arg),
			    ctime( &com_time ) );    
    return;
  }
  else
  if (er == -1) {
    printf_to_char(ch,"\n\r%s is not a character!\n\r",capitalize(arg));   
    return;
  }

 }   
 return;
}  

void lore_item(CHAR_DATA *ch, OBJ_DATA *obj) 
{
  char buf[MSL];
  AFFECT_DATA *paf=NULL;

  sprintf( buf,
		 "Object '{y%s{x' is type {y%s{x, extra flags {y%s{x.\n\rWeight is {y%d{x, value is {y%d{x, level is {y%d{x.\n\r",		 
		 obj->name,
		 item_name(obj->item_type),
		 extra_bit_name( obj->extra_flags ),
		 obj->weight / 10,
		 obj->cost,
		 obj->level
		 );
  send_to_char( buf, ch );
  

  switch ( obj->item_type ) {
  case ITEM_SCROLL: 
  case ITEM_POTION:
  case ITEM_PILL:
    sprintf( buf, "Level {y%d{x spells of:", obj->value[0] );
    send_to_char( buf, ch );
    
    if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL ) {
	 send_to_char( " '", ch );
	 send_to_char( skill_table[obj->value[1]].name, ch );
	 send_to_char( "'", ch );
    }
    
    if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL ) {
	 send_to_char( " '", ch );
	 send_to_char( skill_table[obj->value[2]].name, ch );
	 send_to_char( "'", ch );
    }
    
    if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL ) {
	 send_to_char( " '", ch );
	 send_to_char( skill_table[obj->value[3]].name, ch );
	 send_to_char( "'", ch );
    }
    
    if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL) {
	 send_to_char(" '",ch);
	 send_to_char(skill_table[obj->value[4]].name,ch);
	 send_to_char("'",ch);
    }
    
    send_to_char( ".\n\r", ch );
    break;

  case ITEM_WAND: 
  case ITEM_STAFF: 
    sprintf( buf, "Has %d charges of level %d",
		   obj->value[2], obj->value[0] );
    send_to_char( buf, ch );
      
    if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL ) {
	 send_to_char( " '", ch );
	 send_to_char( skill_table[obj->value[3]].name, ch );
	 send_to_char( "'", ch );
    }

    send_to_char( ".\n\r", ch );
    break;
    
  case ITEM_DRINK_CON:
    if (obj->value[1] <= 0) {
	 send_to_char("It is empty!\n\r", ch);	
    }
    else {
	 sprintf(buf,"It is {y%d{x%% full and holds %s-colored %s.\n\r",
		    obj->value[0] > 0 ? (int)((obj->value[1]/(double)obj->value[0])*100) : obj->value[1],
            liq_table[obj->value[2]].liq_color,
		    liq_table[obj->value[2]].liq_name);
	 send_to_char(buf,ch);        	
    }
    break;

  case ITEM_CONTAINER:
    sprintf(buf, "Condition is %s%d{x\n\r", obj->condition <= 10 ? "{R" : obj->condition <= 30 ? "{Y" : "{g", obj->condition);
    send_to_char(buf, ch);
    sprintf(buf,"Capacity: {y%d{x#  Maximum weight: {y%d{x#  flags: {y%s{x\n\r",
		  obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
    send_to_char(buf,ch);

    if (obj->value[4] != 100) {
	 sprintf(buf,"Weight multiplier: {y%d{x%%\n\r",
		    obj->value[4]);
	 send_to_char(buf,ch);
    }
    break;
    
  case ITEM_WEAPON:

    /* weapon condition */
    sprintf(buf, "Condition is %s%d{x\n\r", obj->condition <= 10 ? "{R" : obj->condition <= 30 ? "{Y" : "{g", obj->condition);
    send_to_char(buf, ch);
    
    send_to_char("Weapon type is ",ch);
    switch (obj->value[0]) {
    case(WEAPON_EXOTIC) : send_to_char("exotic.\n\r",ch);	break;
    case(WEAPON_SWORD)  : send_to_char("sword.\n\r",ch);	break;	
    case(WEAPON_DAGGER) : send_to_char("dagger.\n\r",ch);	break;
    case(WEAPON_SPEAR)  : send_to_char("spear.\n\r",ch);	break;
    case(WEAPON_MACE)   : send_to_char("mace/club.\n\r",ch);break;
    case(WEAPON_AXE)    : send_to_char("axe.\n\r",ch);	 break;
    case(WEAPON_FLAIL)  : send_to_char("flail.\n\r",ch);	break;
    case(WEAPON_WHIP)   : send_to_char("whip.\n\r",ch);	break;
    case(WEAPON_POLEARM): send_to_char("polearm.\n\r",ch);	break;
    case(WEAPON_BOW)    : send_to_char("bow.\n\r", ch); break;
    case(WEAPON_ARROW)  : send_to_char("arrow.\n\r", ch); break;
    case(WEAPON_LANCE)  : send_to_char("lance.\n\r", ch); break;
    case(WEAPON_STAFF)  : send_to_char("staff.\n\r", ch);   break;	  
    default		: send_to_char("unknown.\n\r",ch);	break;
    }

    if (obj->pIndexData->new_format)
	 sprintf(buf,"Damage is {y%d{xd{y%d{x (average {Y%d{x).\n\r",
		    obj->value[1],obj->value[2],
		    (1 + obj->value[2]) * obj->value[1] / 2);
    else
	 sprintf( buf, "Damage is {y%d{x to {y%d{x (average {Y%d{x).\n\r",
			obj->value[1], obj->value[2],
			( obj->value[1] + obj->value[2] ) / 2 );
    send_to_char( buf, ch );

    /* weapon flags */
    if (obj->value[4]) {  
	 sprintf(buf,"Weapons flags: {y%s{x\n\r",weapon_bit_name(obj->value[4]));
	 send_to_char(buf,ch);
    }
    
    break;

  case ITEM_ARMOR:

    sprintf(buf, "Condition is %s%d{x\n\r", obj->condition <= 10 ? "{R" : obj->condition <= 30 ? "{Y" : "{g", obj->condition);
    send_to_char(buf, ch);
    
    sprintf( buf, "Armor class is {y%d{x pierce, {y%d{x bash, {y%d{x slash, "
		   "and {y%d{x vs. weaves.\n\r",
		   obj->value[0], 
		   obj->value[1], 
		   obj->value[2], 
		   obj->value[3] );
    send_to_char( buf, ch );

    sprintf(buf,"Worn: %s\n\r", wear_bit_name(obj->wear_flags));
    send_to_char( buf, ch);

    break;
  }

  if (!obj->enchanted)
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next ) {
	 if ( paf->location != APPLY_NONE && paf->modifier != 0 ) {
	   sprintf( buf, "A{rff{xects {y%s{x by {y%d{x.\n\r",
			  affect_loc_name( paf->location ), paf->modifier );
	   send_to_char(buf,ch);
	   if (paf->bitvector) {
		switch(paf->where) {
		case TO_AFFECTS:
		  sprintf(buf,"Adds %s affect.\n",
				affect_bit_name(paf->bitvector));
		  break;
		case TO_OBJECT:
		  sprintf(buf,"Adds %s object flag.\n",
				extra_bit_name(paf->bitvector));
		  break;
		case TO_IMMUNE:
		  sprintf(buf,"Adds immunity to %s.\n",
				imm_bit_name(paf->bitvector));
		  break;
		case TO_RESIST:
		  sprintf(buf,"Adds resistance to %s.\n\r",
				imm_bit_name(paf->bitvector));
		  break;
		case TO_VULN:
		  sprintf(buf,"Adds vulnerability to %s.\n\r",
				imm_bit_name(paf->bitvector));
		  break;
		default:
		  sprintf(buf,"Unknown bit %d: %d\n\r",
				paf->where,paf->bitvector);
                        break;
		}
		send_to_char( buf, ch );
	   }
	 }
    }
  
  for ( paf = obj->affected; paf != NULL; paf = paf->next ) {
    if ( paf->location != APPLY_NONE && paf->modifier != 0 ) {
	 sprintf( buf, "A{rff{xects {y%s{x by {y%d{x",
			affect_loc_name( paf->location ), paf->modifier );
	 send_to_char( buf, ch );
	 if ( paf->duration > -1)
	   sprintf(buf,", {y%d{x hours.\n\r",paf->duration);
	 else
	   sprintf(buf,".\n\r");
	 send_to_char(buf,ch);
	 if (paf->bitvector) {
	   switch(paf->where) {
	   case TO_AFFECTS:
		sprintf(buf,"Adds %s affect.\n",
			   affect_bit_name(paf->bitvector));
		break;
	   case TO_OBJECT:
		sprintf(buf,"Adds %s object flag.\n",
			   extra_bit_name(paf->bitvector));
		break;
	   case TO_WEAPON:
		sprintf(buf,"Adds %s weapon flags.\n",
			   weapon_bit_name(paf->bitvector));
		break;
	   case TO_IMMUNE:
		sprintf(buf,"Adds immunity to %s.\n",
			   imm_bit_name(paf->bitvector));
		break;
	   case TO_RESIST:
		sprintf(buf,"Adds resistance to %s.\n\r",
			   imm_bit_name(paf->bitvector));
		break;
	   case TO_VULN:
		sprintf(buf,"Adds vulnerability to %s.\n\r",
			   imm_bit_name(paf->bitvector));
		break;
	   default:
		sprintf(buf,"Unknown bit %d: %d\n\r",
			   paf->where,paf->bitvector);
		break;
	   }
	   send_to_char(buf,ch);
	 }
    }
  }
  
}

void do_lore( CHAR_DATA *ch, char *argument )
{
  int chance;
  //  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *obj;
  //  AFFECT_DATA *paf;

  if (IS_NPC(ch)) 
    return;

  if ( argument[0] == '\0' ) {
    send_to_char( "Use your lore on what?\n\r",ch);
    return;
  }

  if ( ( obj = get_obj_carry( ch, argument, ch ) ) == NULL ) {
    send_to_char("You do not have that item.\n\r",ch);
    return;
  }
  
  printf_to_char(ch,"You study the %s very carefully\n\r",obj->short_descr); 

  WAIT_STATE(ch, skill_table[gsn_lore].beats);    

  if ( (chance = get_skill(ch,gsn_lore)) == 0 && (ch->level < skill_table[gsn_lore].skill_level[ch->class])) {	
    send_to_char("It's a rock, it looks very old.\n\r",ch);
    return;
  }
  
  if (ch->endurance<(obj->level*2)) {
    send_to_char("You just can't think straight, maybe you should rest!\n\r",ch);
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

  
  return;
}

void do_peek( CHAR_DATA *ch, char *argument )
{
  AFFECT_DATA af;
  char arg[MSL];
  char buf[MSL];
  CHAR_DATA *victim;
  int check = 0;
  int chance = 0;
  
  // Not for mobs
  if (IS_NPC(ch)) 
    return;
  
  one_argument( argument, arg );
  
  // Can you see?
  if ( !check_blind( ch ) )
    return;
  
  // Argument
  if (IS_NULLSTR(arg)) {
    send_to_char( "You peek at yourself.\n\r", ch );
    send_to_char( "Try 'inventory'.\n\r", ch );
    return;
  }
  
  // The self check
  if ( ( victim = get_char_room( ch, arg ) ) != NULL ) {

    if ( victim == ch ) {
	 send_to_char( "You peek at yourself.\n\r", ch );
	 send_to_char( "Try 'inventory'.\n\r", ch );
	 return;
    }
    
    // Immortal non immortal check
    if (IS_IMMORTAL(victim) && !IS_IMMORTAL(ch)) {
	 act("You start to peek into $N's inventory and is {Yblinded by the sharp light{x that suddenly hit your eyes!", ch, NULL, victim, TO_CHAR);

	 af.where	     = TO_AFFECTS;
	 af.casterId   = ch->id;
	 af.type 	     = gsn_blindness;
	 af.level 	= ch->level;
	 af.duration	= 3;
	 af.location	= APPLY_HITROLL;
	 af.modifier	= -4;
	 af.bitvector 	= AFF_BLIND;
	 affect_to_char(ch,&af);
	 
	 return;
    }

    // Set up the check vs chance
    if (ch->class == CLASS_THIEF || IS_IMMORTAL(ch)) {
	 check  = number_percent();
	 chance = ch->pcdata->learned[gsn_peek];
    }
    else {
	 check = number_percent()+15;
	 chance = ch->pcdata->learned[gsn_peek]/2;
    }
    
    if (check < chance) {
	 act("You peek at $N's inventory:", ch, NULL, victim, TO_CHAR);
	 check_improve(ch,gsn_peek,TRUE,4);
	 show_list_to_char( victim->carrying, ch, TRUE, TRUE, FALSE );

	 // For skilled thiefs, show gold/silver
	 if (chance >= 95) {
	   sprintf(buf, "\n\r(%3ld) {Ygo{yl{Yd{x \n\r(%3ld) {Wsi{xl{Wver{x", victim->gold, victim->silver);
	   act(buf, ch, NULL, victim, TO_CHAR);
	 }
	 
	 return;
    }
    else {
	 act("$N starts to turn $S head, and you quickly look away.", ch, NULL, victim, TO_CHAR);
	 check_improve(ch,gsn_peek,FALSE,4);
	 if ((number_percent() + 25) > check-chance)
	   act("You catch $n peeking at your inventory.", ch, NULL, victim, TO_VICT);
    }
  }
  else {
    send_to_char( "You catch a glimpse of something else, and momentarily forget who your victim is.\n\r", ch );
    return;
  }
}

/* Crypt a vnum to use with gateways */
char *vnum2key(CHAR_DATA *ch, int vnum)
{
  char * asciikey;
  char result[17];
  blf_ctx c;
  unsigned long data[2];
  
  data[0] = vnum;
  data[1] = ch->id;
  blf_key(&c, key, 8);
  blf_enc(&c, data,2);
  sprintf(result,"%08x%08x", (unsigned int)data[0],(unsigned int)data[1]);
  asciikey = result;
  return(asciikey);
};

/* Get vnum from crypted key */
int key2vnum(CHAR_DATA *ch, char *asciikey)
{
  blf_ctx c;
  unsigned int w;
  unsigned long data[2];
  char res1[9],res2[9];
  
  if (strlen(asciikey) != 16) {
    return 0;
  }

  strncpy(res1,asciikey,8);
  res1[8] = '\0';
  strncpy(res2,&asciikey[8],8);
  res2[8] = '\0';
  sscanf(res1,"%08x",&w);
  data[0] = w;
  sscanf(res2,"%08x",&w);
  data[1] = w;
  blf_key(&c, key, 8);
  blf_dec(&c, data,1);

  if (data[1] == ch->id) {
    return data[0];
  }
  else {
    return 0;
  };
};

/* Find the key for this room */
void do_study( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char key_alias[MAX_STRING_LENGTH];
  char key[17];


  if ((ch->class != CLASS_CHANNELER) && (ch->race != race_lookup("fade"))) {
    send_to_char("You look around the room, it looks nice!\n\r", ch);
    return;
  }

  if (IS_SET(ch->world,  WORLD_TAR_DREAM) || IS_SET(ch->world,  WORLD_TAR_FLESH)) {
    send_to_char("Somehow you feel studying here will just not work.\n\r", ch);
    return;  	
  }
  
  send_to_char("You start to study your surroundings.\n\r", ch);
  if (argument != NULL && argument[0] != '\0') {
	argument = one_argument(argument,key_alias);
	ch->study_name = str_dup(key_alias);
  }
  else 
  {
	ch->study_name = NULL;
  }
  
  if (IS_IMMORTAL(ch)) {
    memset(key, 0x00, sizeof(key));
    send_to_char("{yYour immortality reveals the key!{x\n\r", ch);
    strcpy(key,vnum2key(ch, ch->in_room->vnum));
    sprintf(buf, "Key is: {r%s{x\n\r", key);
    send_to_char(buf, ch);
    if (key_alias[0] != '\0') {
       sprintf(buf, "%s %s",key_alias, key);
       do_addkey(ch,buf);
    }
  }
  else if (IS_FORSAKEN(ch)) {
    memset(key, 0x00, sizeof(key));
    send_to_char("{yYour favor with the Great Lord reveals the key!{x\n\r", ch);
    strcpy(key,vnum2key(ch, ch->in_room->vnum));
    sprintf(buf, "Key is: {r%s{x\n\r", key);
    send_to_char(buf, ch);
    if (key_alias[0] != '\0') {
	sprintf(buf,"%s %s",key_alias, key);
	do_addkey(ch,buf);
    }
  }
/*
  else if (IS_DR(ch)) {
    memset(key, 0x00, sizeof(key));
    send_to_char("{yYour favor with the Creator reveals the key!{x\n\r", ch);
    strcpy(key,vnum2key(ch, ch->in_room->vnum));
    sprintf(buf, "Key is: {r%s{x\n\r", key);
    send_to_char(buf, ch); 
  }
*/
  else {
    ch->study       = TRUE;
    ch->study_pulse = 4;
  }
  
  return;
}

void do_backup(CHAR_DATA * ch, char *argument)
{
  if (IS_NPC(ch) || IS_SWITCHED(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char ("NPCs, switched immortals, or being charmed prevent backups from occuring.\n\r", ch);
    return;
  }

  if (IS_DISGUISED(ch)) {
    send_to_char("Disguised characters are under system  backup routines.\n\r", ch);
    return;
  }
  
  save_char_obj(ch, TRUE);
  
  ch->pcdata->last_backup = current_time;
  
  send_to_char("Character saved to backup. Use 'checkbackup' to see last backup date.\n\r", ch);
  send_to_char("To get a character restored, please see one of the immortals.\n\r", ch);
  
  return;
}

void do_checkbackup(CHAR_DATA * ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];

  if (!ch->pcdata->last_backup) {
    send_to_char("No backup found for your character.\n\r"
			  "Use 'backup' to back up your character profile.\n\r", ch);
    return;
  }
  
  sprintf(buf, "Your character was last backuped at %s", (char *) ctime(&ch->pcdata->last_backup));
  send_to_char(buf, ch);
  return;
}

typedef struct web_click_data {
  char localtime[MSL];
  time_t unixtime;
  char ident[MSL];
  char hostname[MSL];
  char remove_adr[MSL];
  char http_user_agent[MSL];
  char http_via[MSL];
  char http_x_forwarded_for[MSL];
  char http_referer[MSL];
}web_click_data;

void init_web_click_element(struct web_click_data elem)
{
  memset(elem.localtime, 0x00, sizeof(elem.localtime));
  elem.unixtime=0;
  memset(elem.ident, 0x00, sizeof(elem.ident));
  memset(elem.hostname, 0x00, sizeof(elem.hostname));
  memset(elem.remove_adr, 0x00, sizeof(elem.remove_adr));
  memset(elem.http_user_agent, 0x00, sizeof(elem.http_user_agent));
  memset(elem.http_via, 0x00, sizeof(elem.http_via));
  memset(elem.http_x_forwarded_for, 0x00, sizeof(elem.http_x_forwarded_for));
  memset(elem.http_referer, 0x00, sizeof(elem.http_referer));
}

bool vote_click_check(CHAR_DATA *ch)
{
  FILE *fp;
  char filename[MSL];
  char buf[MSL];
  int i=0;
  char *recptr=NULL;
  bool first=TRUE;
  bool isvalidclick=FALSE;
  struct web_click_data wclick;
  int cnt=0;
  time_t st_time;
  time_t en_time;

  st_time = time(NULL);

  // Only for PCs
  if (IS_NPC(ch))
    return FALSE;
  
  // File name
  //sprintf(filename, "%s%s", WEB_DIR, "logs/logfile.txt");
  sprintf(filename, "/home/tsw/public_html/log/cgi.log");
  
  // Open file for read
  if ((fp = fopen(filename, "r")) == NULL) {
    log_string("Can't open cgi.log file");
    return FALSE;
  }
  
  memset(buf, 0x00, sizeof(buf));
  init_web_click_element(wclick);
  
  while (fgets(buf, MSL, fp) != NULL) {
    cnt++;
    for (i=0; i < 5; i++) {
	 if (first == TRUE) {
	   first = FALSE;
	   recptr = strtok(buf, ",\n");
	 }
	 else
	   recptr = strtok(NULL, ",\n");
	 
	 if (recptr != NULL) {
	   switch(i) {
	   case 1:
		wclick.unixtime = atol(recptr);
		break;
	   case 2:
		strcpy(wclick.hostname, recptr);
		break;
	   }
	 }
    }
    
    // If same host, and click is new (12 hours since) add to char
    if(!IS_NULLSTR(ch->desc->ipaddr))
    {
       if (!strcmp(wclick.hostname, ch->desc->ipaddr)) {
	 if (ch->pcdata->last_web_vote+43200 <= wclick.unixtime) {
	   sprintf(buf, "Web vote update for %s (last vote=%ld, new vote=%ld)", ch->name, ch->pcdata->last_web_vote,wclick.unixtime);
	   log_string(buf);
	   ch->pcdata->last_web_vote=wclick.unixtime;
	   isvalidclick=TRUE;
	 }
       }
    }
    
    // Reset elements
    init_web_click_element(wclick);
    first=TRUE;    
  }

  // If double xp window is open, notify
  if (ch->pcdata->last_web_vote+3600 >= current_time) {
    send_to_char("\n[ {YVote{x ]: {WD{ro{Wu{rb{Wl{re {Wxp window is active!!!{x\n\r", ch);
    sprintf(buf, "$N have an open (1 hour) double xp vote window (last vote=%s current time=%s).", sec2str(ch->pcdata->last_web_vote), sec2str(current_time));
    wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
  }
  
  fclose(fp);

  en_time = time(NULL);
  
  sprintf(buf, "Vote_click_check for <%s> checked <%d> entries - used time=%ldsec",ch->name, cnt, en_time - st_time);
  log_string(buf);

  if (isvalidclick)
    return TRUE;  
  else
    return FALSE;
}

void do_clickcheck(CHAR_DATA * ch, char *argument)
{
  vote_click_check(ch);
}

void do_checkwebvote(CHAR_DATA * ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  time_t next_vote=0;
  time_t rem_time=0;
  int hours=0, mins=0, secs=0;

  if (!ch->pcdata->last_web_vote) {
    send_to_char("No web vote found for your character.\n\r"
			  "Go to the web page listed in '{Whelp webvote{x' and vote to get active.\n\r", ch);
  return;
  }
  
  sprintf(buf, "Last registerd valid web vote was at {g%s{x\r", (char *) ctime(&ch->pcdata->last_web_vote));
  send_to_char(buf, ch);
  next_vote = ch->pcdata->last_web_vote+43200;
  if (next_vote <= current_time) {
     send_to_char("{GNext vote can be done now!{x\n\r", ch);
  }
  else {
     sprintf(buf, "Next vote can be done at {y%s{x\r", (char *) ctime(&next_vote));
     send_to_char(buf, ch);
  }
  
  rem_time = (ch->pcdata->last_web_vote+3600) - current_time;
  
  if ( rem_time >= 0) {
     hours = (rem_time / 3600) % 24;
     mins  = (rem_time % 3600 ) / 60;
     secs  = (rem_time % 60);
     sprintf(buf, "\nDouble experience window {gopen{x for [{y%02d{xh:{y%02d{xm:{y%02d{xs] until closed.\n\r", hours, mins, secs);
     send_to_char(buf, ch);
  }
  else {
    send_to_char("\nDouble experience window {Rclosed{x!\n\r", ch);
  }
  
  return;
}

void do_rp(CHAR_DATA * ch, char *argument)
{
  char arg1[MAX_STRING_LENGTH];
  bool bPublic = FALSE;
  if (IS_NPC(ch))
    return;

  if (argument != NULL && argument[0] != '\0') {
	argument = one_argument(argument,arg1);
	if (!str_cmp("pub",arg1)) {
		bPublic = TRUE;
	}
  }

  if (IS_RP(ch)) {
    send_to_char("You leave the RP and mark your self as being Out Of Character.\n\r",ch);
    act( "$n leaves the RP and marks $mself as being Out Of Character.",ch,NULL,NULL,TO_ROOM);
    REMOVE_BIT(ch->ic_flags,IC_RP);
    REMOVE_BIT(ch->act2, PLR2_SEEKINGRP);
    ch->pcdata->rprewardtimer = 0;
    ch->pcdata->rpbonus = 0;
    ch->roleplayed += current_time - ch->lastrpupdate;
    save_char_obj (ch, FALSE);
  }
  else {
    send_to_char("You enter the RP and mark your self as being In Character.\n\r", ch);
    act( "$n enters the RP and marks $mself as being In Character.",ch,NULL,NULL,TO_ROOM);
    SET_BIT(ch->ic_flags, IC_RP);
    if (bPublic) {
	SET_BIT(ch->act2, PLR2_SEEKINGRP);
    }
    ch->pcdata->rprewardtimer = current_time + 750 + (number_range (0, 600));
    char buff[256];
    sprintf(buff,"Do_RP: %s : RPBonus: %d  : NextReward: %s",ch->name,ch->pcdata->rpbonus,(char *)ctime(&ch->pcdata->rprewardtimer));
    wiznet(buff, ch, NULL, WIZ_ROLEPLAY, 0, get_trust(ch));

    ch->pcdata->rpbonus = 0;
    ch->lastrpupdate = current_time;
    save_char_obj (ch, FALSE);
  }
  
  return;
}

void do_sheathwherename(CHAR_DATA * ch, char *argument)
{
  char buf[MSL];
  char arg[MSL];
  char sheat_name[MSL];
  int sheat_num=0;
  int fillers=0;
  
  // Only for PCs
  if (IS_NPC(ch))
    return;

  argument = one_argument( argument, arg );

  if (IS_NULLSTR(arg) || IS_NULLSTR(argument)) {
    send_to_char("Syntax: sheathwherename <sheath number> <where name>\n\r", ch);
    return;
  }
  
  if (!(sheat_num = atoi(arg))) {
    send_to_char("Sheath number must be a number!\n\r", ch);
    return;
  }

  if (sheat_num > MAX_SHEAT_LOC) {
    sprintf(buf, "Sheath number must be between 1 and %d.\n\r", MAX_SHEAT_LOC);
    send_to_char(buf, ch);
    return;
  }

  sprintf(sheat_name, "%s", colorstrem(argument));
  
  if (strstr(sheat_name, "sheath") == NULL) {
    send_to_char("Sheath where name must include keyword '{Wsheath{x'.\n\r", ch);
    return;
  }
  
  if (strlen(sheat_name) > 17 || strlen(sheat_name) < 6) {
    send_to_char("Sheath where name must be between 6 and 17 characters long (no colors allowed).\n\r", ch);
    return;
  }
  
  fillers = (18 - strlen(sheat_name));
  
  sprintf(buf, "<%s>%*s", sheat_name, fillers, "");
  
  free_string( ch->sheat_where_name[sheat_num-1]);
  ch->sheat_where_name[sheat_num-1] = str_dup(buf);

  sprintf(buf, "Sheath where name location number %d set to: %s\n\r", sheat_num, sheat_name);
  send_to_char(buf, ch);

  return;
}

void do_gatelist (CHAR_DATA * ch, char *argument) {

	char buffer[MAX_STRING_LENGTH];
	if (IS_NPC(ch)) {
		send_to_char("This information is stored on a PC file.\n\r",ch);
		return;
	}

	if (ch->pcdata->keys == NULL) {
		send_to_char("Your key list is empty.\n\r",ch);
		return;
	}

	GATEKEY_DATA * key = ch->pcdata->keys;
	while (key != NULL) {
		sprintf(buffer,"%s - %s\n\r", key->key_alias, key->key_value);
		send_to_char(buffer,ch);
		key = key->next;
	}

	return;	

}

void do_addkey(CHAR_DATA * ch, char *argument) {
	char aliasarg[MAX_STRING_LENGTH];
	char keyarg[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH];

	if (argument == NULL || argument[0] == '\0') {
		send_to_char("addkey <alias> <key>\n\r",ch);
		return;
	}

	argument = one_argument(argument, aliasarg);
	if (argument == NULL || argument[0] == '\0') {
		send_to_char("addkey <alias> <key>\n\r",ch);
		return;
	}

	argument = one_argument(argument, keyarg);
	if (argument == NULL || keyarg[0] == '\0') {
		send_to_char("addkey <alias> <key>\n\r",ch);
		return;
	}

	char * existing = getkey(ch,aliasarg);

	if ( existing != NULL)
	{
		do_delkey(ch,aliasarg);
	}

	GATEKEY_DATA * key = malloc(sizeof(GATEKEY_DATA));
	key->key_alias = str_dup(aliasarg);
	key->key_value = str_dup(keyarg);
	key->next = ch->pcdata->keys;
	ch->pcdata->keys = key;

	if (ch->desc->connected == CON_PLAYING) {
		sprintf(buffer,"Added key: %s - %s\n\r",key->key_alias, key->key_value);
		send_to_char(buffer,ch);
	}

}

void do_delkey(CHAR_DATA *ch, char *argument) {
	GATEKEY_DATA * key;
	GATEKEY_DATA * prior;
	char buffer[MAX_STRING_LENGTH];
	int found = 0;
	if (IS_NPC(ch)) 
		return;
	key = ch->pcdata->keys;
	prior = key;
	while (key != NULL) {
	   if (!str_cmp(key->key_alias,argument)) {
		if (ch->pcdata->keys == key)
		{
			ch->pcdata->keys = key->next;
		}
		else
		{
			prior->next = key->next;
		}
		sprintf(buffer,"Deleting key: %s - %s\n\r",key->key_alias, key->key_value);
		send_to_char(buffer,ch);
		free_string(key->key_alias);
		free_string(key->key_value);
		free(key);
		found = 1;
		break;
	   }
	   else
	   {
		prior = key;
		key = key->next;	
	   }
	}
	if (found == 0) {
		send_to_char("No such key found.\n\r",ch);
	}

}

char * getkey(CHAR_DATA *ch, char*argument) {

	char * retval = NULL;
	char buf[MAX_STRING_LENGTH];
        if (IS_NPC(ch)) {
		return NULL;
        }

	if (argument[0] == '\0') {
		return NULL;
	}

	GATEKEY_DATA * key = ch->pcdata->keys;
        while (key != NULL) {
	   if (!str_cmp(key->key_alias, argument)) {
	      retval = key->key_value;
	      break;
	   }	
	   key = key->next;
        }
	
	return retval;
}

void do_setdreamapp( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  //CHAR_DATA *victim;

  //argument = one_argument( argument, arg1 );

  if (IS_NPC(ch)) {
 	send_to_char("Not on Mobs.\n\r",ch);
  }
 
  if (get_skill(ch,skill_lookup("dreaming")) < 285) {
	send_to_char("You do not have the skill level in manipulating Tel'aran'rhiod for that.\r\n",ch);
	return;
  }
  if (IS_NULLSTR (argument)) {
	send_to_char("Syntax: setdreamapp <appearance|clear>\r\n",ch);
	return;
  }

  if (!str_cmp(argument,"clear")) {
	buf[0] = '\0';
	free_string(ch->pcdata->dreaming_appearance);
	ch->pcdata->dreaming_appearance = str_dup(buf);
	send_to_char("Dreaming Appearance cleared.\n\r",ch);
	return;
  }
  ch->pcdata->dreaming_appearance = str_dup(argument);
  send_to_char("New Dreamworld appearance set.\r\n",ch);
  return;  
}

void do_spheres( CHAR_DATA *ch, char *argument )
{
	char arg1[MIL] ;
	char arg2[MIL] ;
	char buf[MIL] ;

	int sphere_amount = 0 ;
	int high_sphere = 0 ;
	int low_sphere = 0 ;

	int i ;

	if( IS_NPC(ch))
	{
		send_to_char("Not on NPCs.\r\n", ch) ;
		return ;
	}

	if( ch->class != CLASS_CHANNELER )
	{
		send_to_char("You have no power in the True Source.\r\n", ch) ;
		return ;
	}

	argument = one_argument( argument, arg1 ) ;

	// If there's no argument, then we display syntax (IF necessary) and spheres.
	// We also display this if the character has already re-allocated their spheres.
	if( IS_NULLSTR(arg1) || ch->pcdata->spheretrain == 1 )
	{
		// Display Sphere Allocation if they haven't done it yet and their spheres are maxed.
		if( ch->pcdata->spheretrain == 0 )
		{
			int maxed_spheres = 1 ;
			for( i=0 ; i < MAX_SPHERE ; i++ ) {
				if( ch->perm_sphere[i] < ch->cre_sphere[i] )
				{
					maxed_spheres = 0 ;
					break ;
				}
			}
			if( maxed_spheres )
			{
				send_to_char("Syntax: spheres <number> <highsphere> <lowsphere>\r\n", ch) ;
				send_to_char("   This will transfer up to 10 points in the higher sphere to the lower one.\r\n", ch) ;
			}
		}

		// Display Spheres.
		sprintf(buf, "{wSpheres:   {C%d Air   {y%d Earth   {R%d Fire   {W%d Spirit   {B%d Water   {D%d Total{x\r\n",
			ch->perm_sphere[SPHERE_AIR], ch->perm_sphere[SPHERE_EARTH], ch->perm_sphere[SPHERE_FIRE], 
			ch->perm_sphere[SPHERE_SPIRIT], ch->perm_sphere[SPHERE_WATER], SPHERE_TOTAL(ch)) ;
		send_to_char( buf, ch ) ;
		return ;
	}

	if( !(sphere_amount = atoi(arg1)) )
	{
		send_to_char("Syntax: spheres <number> <highsphere> <lowsphere>\r\n", ch) ;
		send_to_char("   <number> should be, you know, a number. Between 1 and 10\r\n", ch) ;
		return ;
	}

	if( sphere_amount < 1 || sphere_amount > 10 )
	{
		send_to_char("Syntax: spheres <number> <highsphere> <lowsphere>\r\n", ch) ;
		send_to_char("   <number> should be between 1 and 10.\r\n", ch) ;
		return ;
	}

	argument = one_argument( argument, arg2 ) ;
	if( IS_NULLSTR(arg2) || IS_NULLSTR(argument))
	{
		send_to_char("Syntax: spheres <number> <highsphere> <lowsphere>\r\n", ch) ;
		send_to_char("   You must specify a highsphere and lowsphere.\r\n", ch) ;
		return ;
	}

	if( !str_cmp(arg2, "air"))
	{	high_sphere = SPHERE_AIR ;	}
	else if( !str_cmp(arg2, "earth"))
	{	high_sphere = SPHERE_EARTH ;	}
	else if( !str_cmp(arg2, "fire"))
	{	high_sphere = SPHERE_FIRE ;	}
	else if( !str_cmp(arg2, "spirit"))
	{	high_sphere = SPHERE_SPIRIT ;	}
	else if( !str_cmp(arg2, "water"))
	{	high_sphere = SPHERE_WATER ;	}
	else
	{
		send_to_char("Syntax: spheres <number> <highsphere> <lowsphere>\r\n", ch) ;
		send_to_char("   <sphere> should be air, earth, fire, spirit, or water.\r\n", ch) ;
		return ;
	}

	if( !str_cmp(argument, "air"))
	{	low_sphere = SPHERE_AIR ;	}
	else if( !str_cmp(argument, "earth"))
	{	low_sphere = SPHERE_EARTH ;	}
	else if( !str_cmp(argument, "fire"))
	{	low_sphere = SPHERE_FIRE ;	}
	else if( !str_cmp(argument, "spirit"))
	{	low_sphere = SPHERE_SPIRIT ;	}
	else if( !str_cmp(argument, "water"))
	{	low_sphere = SPHERE_WATER ;	}
	else
	{
		send_to_char("Syntax: spheres <number> <highsphere> <lowsphere>\r\n", ch) ;
		send_to_char("   <sphere> should be air, earth, fire, spirit, or water.\r\n", ch) ;
		return ;
	}

	// Can only move points from a higher sphere to a lower sphere.
	if( ch->perm_sphere[high_sphere] <= ch->perm_sphere[low_sphere])
	{
		send_to_char("Syntax: spheres <number> <highsphere> <lowsphere>\r\n", ch) ;
		send_to_char("   The first sphere must be higher than the second sphere.\r\n", ch) ;
		return ;
	}

	// We're good, let's get to allocating!!
	ch->perm_sphere[high_sphere] -= sphere_amount ;
	ch->cre_sphere[high_sphere] -= sphere_amount ;
	ch->perm_sphere[low_sphere] += sphere_amount ;
	ch->cre_sphere[low_sphere] += sphere_amount ;
	sprintf( buf, "{wYou have transfered {R%d {wpoints in {W%s {wto {W%s{w.{x\r\n", sphere_amount, arg2, argument) ;
	send_to_char( buf, ch ) ;
	ch->pcdata->spheretrain = 1 ;
	return ;
}
