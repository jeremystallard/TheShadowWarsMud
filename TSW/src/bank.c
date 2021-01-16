/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  TSW Mud improvments copyright (C) 2000-2003 by Swordfish and Zandor.   *
 *                                                                         *
 *  This file includes a bank system that is used for The Shadow Wars      *
 *                                                                         *
 **************************************************************************/
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"

void do_account (CHAR_DATA *ch, char *argument)
{
  char buf[MSL];
  CHAR_DATA *mob=NULL;
  long gold   = ch->pcdata->gold_bank;
  long silver = ch->pcdata->silver_bank;
  
  if (IS_NPC(ch) || IS_IMMORTAL(ch)) {
    send_to_char("Only players need to have a bank account.\n\r",ch);
    return;
  }
  
  // Check if a banker in room
  for ( mob = ch->in_room->people; mob; mob = mob->next_in_room ) {
    if (IS_NPC(mob) && IS_SET(mob->act, ACT_BANKER))
	 break;
  }
  
  // No?
  if (mob == NULL) {
    send_to_char( "You must be at the bank!.\n\r", ch );
    return;
  }
  
  if (gold == 0 && silver == 0) {
    act("$N tells you, '{7You don't have an account in our bank. Would you like to deposit some coins?{x'", ch, NULL, mob, TO_CHAR);
    return;
  }
  
  
  // Ok, at a banker
  sprintf(buf, "$N look in a big book and tells you, '{7You have %ld {Ygo{yl{Yd{7 and %ld {Wsi{xl{Wver{7 in your account.{x'", gold, silver);
  act(buf, ch, NULL, mob, TO_CHAR);
  act("$N close the big book and look at you.", ch, NULL, mob, TO_CHAR);
  
  return;
}

void do_deposit (CHAR_DATA *ch, char *argument)
{
  long amount = 0;
  char arg1[MSL];
  char arg2[MSL];
  char buf[MSL];
  CHAR_DATA *mob=NULL;
  
  if (IS_NPC(ch) || IS_IMMORTAL(ch)) {
    send_to_char("Only players need to have a bank account.\n\r",ch);
    return;
  }
  
  // Check if a banker in room
  for ( mob = ch->in_room->people; mob; mob = mob->next_in_room ) {
    if (IS_NPC(mob) && IS_SET(mob->act, ACT_BANKER))
	 break;
  }
  
  if (mob == NULL) {
    send_to_char( "You must be at the bank!.\n\r", ch );
    return;
  }
  else { // If they are in the bank
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if (arg1[0] =='\0' || arg2[0] =='\0') {
	 act("$N tells you, '{7How much do you want to deposit?{x'", ch, NULL, mob, TO_CHAR);
	 send_to_char("Syntax: deposite <value> gold\n\r", ch);
	 send_to_char("Syntax: deposite <value> silver\n\r", ch);
	 return;
    }
    
    if (is_number( arg1 )) {
	 amount = atoi(arg1);
	 
	 if ( amount <= 0) {
	   act("$N eye you for a moment before $E says, '{7To deposit money, you must give me some bloody money!{x'", ch, NULL, mob, TO_CHAR);
	   return;
	 }
	 
	 // If they are depositing gold
	 if(!str_cmp( arg2, "gold")) {
	   if (ch->gold < amount) {
		act("$N shake $E's head and tells you, '{7You don't have that much {Ygo{yl{Yd{7.{x'", ch, NULL, mob, TO_CHAR);	  
		return;
	   }
	   else {
		ch->pcdata->gold_bank += amount;
		ch->gold -= amount;
		sprintf( buf, "$N grin at you and take your %ld {Ygo{yl{Yd{x as $E says, '{7Your gold is safe in our bank.{x'", amount);
		act(buf, ch, NULL, mob, TO_CHAR);
		
		sprintf(buf, "$N continues, '{7You now have %ld {Ygo{yl{Yd{7 and %ld {Wsi{xl{Wver{7 in your account.{x'", ch->pcdata->gold_bank, ch->pcdata->silver_bank);
		act(buf, ch, NULL, mob, TO_CHAR);
		return;
	   }
	 }
	 
	 // If they are depositing silver
	 if(!str_cmp( arg2, "silver")) {
	   if (ch->silver < amount) {
		act("$N shake $E's head and tells you, '{7You don't have that much {Wsi{xl{Wver{7.{x'", ch, NULL, mob, TO_CHAR);	  
		return;
	   }
	   else {
		ch->pcdata->silver_bank += amount;
		ch->silver -= amount;

	sprintf( buf, "$N grin at you and take your %ld {Wsi{xl{Wver{x as $E says, '{7Your silver is safe in our bank.{x'", amount);
		act(buf, ch, NULL, mob, TO_CHAR);
		
		sprintf(buf, "$N continues, '{7You now have %ld {Ygo{yl{Yd{7 and %ld {Wsi{xl{Wver{7 in your account.{x'", ch->pcdata->gold_bank, ch->pcdata->silver_bank);
		act(buf, ch, NULL, mob, TO_CHAR);
		return;
	   }
	 }
    }
  }
  return;
}

void do_withdraw (CHAR_DATA *ch, char *argument)
{
  long amount = 0;
  char arg1[MSL];
  char arg2[MSL];
  char buf[MSL];
  CHAR_DATA *mob=NULL;
  
  if (IS_NPC(ch) || IS_IMMORTAL(ch)) {
    send_to_char("Only players need to have a bank account.\n\r", ch);
    return;
  }

  // Check if a banker in room
  for ( mob = ch->in_room->people; mob; mob = mob->next_in_room ) {
    if (IS_NPC(mob) && IS_SET(mob->act, ACT_BANKER))
	 break;
  }

  // If not in bank let them know
  if (mob == NULL) {
    send_to_char( "You must be at the bank!.\n\r", ch );
    return;
  }
  else { // If they are in the bank
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if (arg1[0] == '\0' || arg2[0] == '\0') {
	 act("$N tells you, '{7How much do you want to withdraw?{x'", ch, NULL, mob, TO_CHAR);
	 send_to_char("Syntax: withdraw <value> gold\n\r", ch);
	 send_to_char("Syntax: withdraw <value> silver\n\r", ch);
	 return;
    }
    
    if( is_number( arg1 ) ) {
	 amount = atoi(arg1);
	 
	 if ( amount <= 0 ) {
	   act("$N eye you for a moment before $E says, '{7To withdraw that amount, you have to have it in the bloody bank first!{x'", ch, NULL, mob, TO_CHAR);
	   return;
	 }
	 
	 // If they are withdrawing gold
	 if(!str_cmp( arg2, "gold")) {
	   if (ch->pcdata->gold_bank < amount) {
		act("$N shake $E's head and tells you, '{7You don't have that much {Ygo{yl{Yd{7 in your account.{x'", ch, NULL, mob, TO_CHAR);
		return;
	   }
	   else {
		ch->pcdata->gold_bank -= amount;
		ch->gold += amount;
		sprintf( buf, "$N look at you before $E hands you %ld {Ygo{yl{Yd{x coins as $E says, '{7Your gold is safer in our bank.{x' and nods to $Mself.", amount);
		act(buf, ch, NULL, mob, TO_CHAR);
		
		sprintf(buf, "$N continues, '{7You now have %ld {Ygo{yl{Yd{7 and %ld {Wsi{xl{Wver{7 in your account.{x'", ch->pcdata->gold_bank, ch->pcdata->silver_bank);
		act(buf, ch, NULL, mob, TO_CHAR);
		return;
	   }
	 }
	 
	 // If they are withdrawing silver
	 if(!str_cmp( arg2, "silver")) {
	   if (ch->pcdata->silver_bank < amount) {
		act("$N shake $E's head and tells you, '{7You don't have that much {Wsi{xl{Wver{7 in your account.{x'", ch, NULL, mob, TO_CHAR);
		return;
	   }
	   else {
		ch->pcdata->silver_bank -= amount;
		ch->silver += amount;
		sprintf( buf, "$N look at you before $E hands you %ld {Wsi{xl{Wver{x coins as $E says, '{7Your silver is safer in our bank.{x' and nods to $Mself.", amount);
		act(buf, ch, NULL, mob, TO_CHAR);
		
		sprintf(buf, "$N continues, '{7You now have %ld {Ygo{yl{Yd{7 and %ld {Wsi{xl{Wver{7 in your account.{x'", ch->pcdata->gold_bank, ch->pcdata->silver_bank);
		act(buf, ch, NULL, mob, TO_CHAR);
		
		return;
	   }
	 }
    }
  }
  return;
}
