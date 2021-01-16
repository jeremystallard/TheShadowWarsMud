
/*          Dice Game Snippet for the EmberMUD Codebase by Rahl            */
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
 * A few really simple dice games. If you want to change the odds, simply  *
 * change the dice() numbers.                                              *
 *                                                                         *
 * To install: add dice_games.o to your makefile, add do_games to interp.c *
 * and interp.h, and add spec_gamemaster to special.c                      *
 *                                                                         *
 * If you choose to use this code, please retain my name in this file and  *
 * send me an email (dwa1844@rit.edu) saying you are using it. Suggestions *
 * for improvement are welcome.   -- Rahl (Daniel Anderson)                *
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

void win( CHAR_DATA *ch, long amnt );
void lose( CHAR_DATA *ch, long amnt );

void win( CHAR_DATA *ch, long amnt )
{
    char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    ch->silver += amnt;
    sprintf( buf, "You win %ld silver!\n\r", amnt );
    send_to_char( buf, ch );
}

void lose( CHAR_DATA *ch, long amnt )
{
    char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    ch->silver -= amnt;
    sprintf( buf, "You lost %ld silver!\n\r", amnt );
    send_to_char( buf, ch );
}

void game_even_odd( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    long amount;
    int roll;
    CHAR_DATA *gamemaster;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    buf[0] = '\0';

    if ( arg[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: game even-odd <even|odd> <amount>\n\r", ch );
        return;
    }

    for ( gamemaster = ch->in_room->people; gamemaster != NULL; gamemaster = gamemaster->next_in_room )
    {
        if ( !IS_NPC( gamemaster ) )
            continue;
        if ( gamemaster->spec_fun == spec_lookup( "spec_gamemaster" ) )
            break;
    }

    if ( gamemaster == NULL || gamemaster->spec_fun != spec_lookup( "spec_gamemaster" ) )
    {
        send_to_char("You can't do that here.\n\r",ch);
        return;
    }

    if ( gamemaster->fighting != NULL)
    {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        return;
    }

    if ( !is_number( arg2 ) )
    {
        send_to_char( "You must bet a number.\n\r", ch );
        return;
    }

    roll = dice( 2, 16 );

    amount = atol( arg2 );

    if ( amount < 1 )
    {
        send_to_char( "Bet SOMETHING, will you?\n\r", ch );
        return;
    }

    if (amount > ch->silver) {
        send_to_char( "We don't take credit here?\n\r", ch );
        return;
    }

	if (amount > 10000) {
		send_to_char( "100 gold or 10000 silver is the max bet here.\n\r",ch);
		return; 	
	}
    sprintf( buf, "%s rolls the dice.\n\rThe roll is %d.\n\r", gamemaster->short_descr, roll );
    send_to_char( buf, ch );
    
    if ( !str_cmp( arg, "odd" ) )
    {
        if ( roll %2 != 0 )     /* you win! */
            win( ch, amount );
        else
            lose( ch, amount );
        return;
    }
    else if ( !str_cmp( arg, "even" ) )
    {
        if ( roll %2 == 0 )
            win( ch, amount );
        else
            lose( ch, amount );
        return;
    }
    else
    {
        send_to_char( "Syntax: game even-odd <even|odd> <amount>\n\r", ch );
    }
    return;
}


void game_high_low( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    long amount;
    int roll;
    CHAR_DATA *gamemaster;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    buf[0] = '\0';

    if ( arg[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: game high-low <high|low> <amount>\n\r", ch );
        return;
    }

    for ( gamemaster = ch->in_room->people; gamemaster != NULL; gamemaster = gamemaster->next_in_room )
    {
        if ( !IS_NPC( gamemaster ) )
            continue;
        if ( gamemaster->spec_fun == spec_lookup( "spec_gamemaster" ) )
            break;
    }

    if ( gamemaster == NULL || gamemaster->spec_fun != spec_lookup( "spec_gamemaster" ) )
    {
        send_to_char("You can't do that here.\n\r",ch);
        return;
    }

    if ( gamemaster->fighting != NULL)
    {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        return;
    }

    if ( !is_number( arg2 ) )
    {
        send_to_char( "You must bet a number.\n\r", ch );
        return;
    }

    roll = dice( 2, 8 );

    amount = atol( arg2 );

    if ( amount < 1 )
    {
        send_to_char( "Bet SOMETHING, will you?\n\r", ch );
        return;
    }

    if (amount > ch->silver) {
        send_to_char( "We don't take credit here?\n\r", ch );
        return;
    }
   	if (amount > 10000) {
		send_to_char( "100 gold or 10000 silver is the max bet here.\n\r",ch);
		return; 	
	}
    
    
    sprintf( buf, "%s rolls the dice.\n\rThe roll is %d.\n\r", gamemaster->short_descr, roll );
    send_to_char( buf, ch );
    
    if ( !str_cmp( arg, "high" ) )
    {
        if ( roll > 9 )     /* you win! */
            win( ch, amount);// * 1.5 );
        else
            lose( ch, amount );
        return;
    }
    else if ( !str_cmp( arg, "low" ) )
    {
        if ( roll < 8 )
            win( ch, amount);// * 1.5 );
        else
            lose( ch, amount );
        return;
    }
    else
    {
        send_to_char( "Syntax: game high-low <high|low> <amount>\n\r", ch );
    }
    return;
}


void game_higher_lower( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    long amount;
    int your_roll, his_roll;
    CHAR_DATA *gamemaster;

    argument = one_argument( argument, arg );

    buf[0] = '\0';

    if ( arg[0] == '\0' )
    {
        send_to_char( "Syntax: game higher-lower <amount>\n\r", ch );
        return;
    }

    for ( gamemaster = ch->in_room->people; gamemaster != NULL; gamemaster = gamemaster->next_in_room )
    {
        if ( !IS_NPC( gamemaster ) )
            continue;
        if ( gamemaster->spec_fun == spec_lookup( "spec_gamemaster" ) )
            break;
    }

    if ( gamemaster == NULL || gamemaster->spec_fun != spec_lookup( "spec_gamemaster" ) )
    {
        send_to_char("You can't do that here.\n\r",ch);
        return;
    }

    if ( gamemaster->fighting != NULL)
    {
        send_to_char("Wait until the fighting stops.\n\r",ch);
        return;
    }

    if ( !is_number( arg ) )
    {
        send_to_char( "You must bet a number.\n\r", ch );
        return;
    }

    your_roll = dice( 2, 4 );
    his_roll = dice( 2, 6 );

    amount = atol( arg );

    if ( amount < 1 )
    {
        send_to_char( "Bet SOMETHING, will you?\n\r", ch );
        return;
    }
    if (amount > ch->silver) {
        send_to_char( "We don't take credit here?\n\r", ch );
        return;
    }

	if (amount > 10000) {
		send_to_char( "100 gold or 10000 silver is the max bet here.\n\r",ch);
		return; 	
	}


    sprintf( buf, "%s rolls the dice and gets a %d.\n\rYour roll is %d.\n\r",                                gamemaster->short_descr, his_roll, your_roll );
    send_to_char( buf, ch );
    
    if ( your_roll > his_roll )     /* you win! */
        win( ch, amount * 2 );
    else
        lose( ch, amount );
    return;
}

void do_game( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    
    argument = one_argument( argument, arg );

    if ( !str_cmp( arg, "higher-lower" ) )
        game_higher_lower( ch, argument );
    else if ( !str_cmp( arg, "even-odd" ) )
        game_even_odd( ch, argument );
    else if ( !str_cmp( arg, "high-low" ) )
        game_high_low( ch, argument );
    else
    {
        send_to_char( "Current games are: higher-lower, even-odd, and high-low.\n\r", ch );
        return;
    }
}


void do_dicethrow(CHAR_DATA *ch, char *argument )
{
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char arg3[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH];
	char buff[MAX_STRING_LENGTH];

	bool dch = FALSE ,dcl = FALSE, dce=FALSE;
	int sum=0,diceroll=0, numberofdice=0;
	int i = 0;

	if (argument[0] == '\0')
        {
	   numberofdice = 6;
	}
	else
        {
	   argument = one_argument(argument,arg1);
	   if (!is_number(arg1))
           {
		   send_to_char("Syntax: Dicethrow <number of dice>\r\n",ch);
		   return;
	   }
	   numberofdice = atoi(arg1);
	   if (numberofdice < 1 || numberofdice > 6)
           {
		send_to_char("dicethrow: You must specifiy 1 to 6 dice\r\n",ch);
		return;
	   }
	   if (argument[0] != '\0')
           {
	      argument = one_argument(argument,arg2);
	      if (!str_cmp(arg2,"ch"))
	      {
		dch = TRUE;
	      }
	      else if (!str_cmp(arg2,"cl"))
              {
		dcl = TRUE; 
	      }
	      else if (!str_cmp(arg2,"ce"))
              {
		argument=one_argument(argument,arg3);
		dce = TRUE;
              }
           }
	}
	
	sprintf(buffer,"<DICE GAME>$n rolls the dice: ");
	  
	   for (i = 0; i < numberofdice;i++)
           {
		diceroll=number_range(1,6);
		if (dch)
		{
		   if (diceroll < 5)
                   {
			diceroll += 2;
 	           }
		}	
		else if (dcl)
                {
		   if (diceroll > 2)
                   {
			diceroll -= 2;
 	           }
		}
		else if (dce)
                {
		   if (strlen(arg3) > i)
                   {
			diceroll = arg3[i] - '0';
                   }			
		}
		sum += diceroll;
		sprintf(buff,"%d ",diceroll);
		strcat(buffer,buff);		
           }
	   sprintf(buff,"\r\nFor a total of: %d\n\r",sum);
	   strcat(buffer,buff);
	   act(buffer,ch,NULL,NULL,TO_ROOM);
	   act(buffer,ch,NULL,NULL,TO_CHAR);
	   if (IS_RP(ch))
           {
	      char rp_buffer[MAX_STRING_LENGTH];
	      sprintf(rp_buffer,"%s: %s",ch->name,buffer);
	      log_rp_string(ch,rp_buffer);
	   }
}

//Multisided dice roll
void do_multidicethrow(CHAR_DATA *ch, char *argument )
{
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	char arg3[MAX_STRING_LENGTH];
	char buffer[MAX_STRING_LENGTH];
	char buff[MAX_STRING_LENGTH];

	int sum=0,diceroll=0, numberofdice=0, numberofsides=6;
	int i = 0;

	if (argument[0] == '\0')
        {
		   send_to_char("Syntax: Dicethrow <number of dice> <number of sides>\r\n",ch);
		   return;
	}
	
	argument = one_argument(argument,arg1);
	if (!is_number(arg1))
        {
		   send_to_char("Syntax: Dicethrow <number of dice> <number of sides>\r\n",ch);
		   return;
	}
        numberofdice = atoi(arg1);
        if (numberofdice < 1 || numberofdice > 10)
        {
		send_to_char("dicethrow: You must specifiy 1 to 10 dice\r\n",ch);
		return;
	}
	if (argument[0] == '\0')
        {
		   send_to_char("Syntax: Dicethrow <number of dice> <number of sides>\r\n",ch);
		   return;
	}
	else
        {
		argument = one_argument(argument,arg2);
		if (!is_number(arg2)) {
		   send_to_char("Syntax: Dicethrow <number of dice> <number of sides>\r\n",ch);
		   return;
		}
     		numberofsides = atoi(arg2); 
		if (numberofsides < 2 || numberofsides > 40) {
		   send_to_char("Please use more than 1 side and less than 40\r\n",ch);
		   return;

		}
        }
	
	sprintf(buffer,"<DICE GAME>$n rolls %d %d sided dice: ", numberofdice, numberofsides);
	  
	   for (i = 0; i < numberofdice;i++)
           {
		diceroll=number_range(1,numberofsides);
		sum += diceroll;
		sprintf(buff,"%d ",diceroll);
		strcat(buffer,buff);		
           }
	   sprintf(buff,"\r\nFor a total of: %d\n\r",sum);
	   strcat(buffer,buff);
	   act(buffer,ch,NULL,NULL,TO_ROOM);
	   act(buffer,ch,NULL,NULL,TO_CHAR);
	   if (IS_RP(ch))
           {
	      char rp_buffer[MAX_STRING_LENGTH];
	      sprintf(rp_buffer,"%s: %s",ch->name,buffer);
	      log_rp_string(ch,rp_buffer);
	   }
}

