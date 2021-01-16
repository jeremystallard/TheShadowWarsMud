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
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "music.h"
#include "recycle.h"
#include "magic.h"

/*
 * Local functions.
 */
int	hit_gain	     args( ( CHAR_DATA *ch, bool is_locgain, int location) );
int	endurance_gain args( ( CHAR_DATA *ch ) );
int	move_gain      args( ( CHAR_DATA *ch ) );
void	mobile_update 	args( ( void ) );
void	weather_update	args( ( void ) );
void  weather_effect args( ( void ) );
void	char_update	args( ( void ) );
void	obj_update	args( ( void ) );
void	aggr_update	args( ( void ) );
void hint_update    args( ( void ) );
void one_hit ( CHAR_DATA *ch, CHAR_DATA *victim, int dt, int dual, long target );

void    dtrap_update    args( ( void ) );
void    unstoppableweave args((CHAR_DATA *ch,CHAR_DATA* victim,char * weave));

/* used for saving */

int	save_number = 0;

void gain_burnout(CHAR_DATA *ch, int bos)
{
  char buf[MAX_STRING_LENGTH];
  
  /* NPCs don't BO */
  if (IS_NPC(ch))
    return;
  
  if ((ch->burnout + bos) >= ch->max_burnout) {
    ch->burnout = ch->max_burnout;
    
    send_to_char("{rYou feel a intence glow and the world seems to b{yu{Yr{rn away from you.{x\n\r", ch);
    
    if (!IS_IMMORTAL(ch)) {
	 SET_BIT(ch->act,PLR_STILLED);

	 /* Tell wiznet */
	 sprintf(buf, "{r%s burns out and lose the ability to channel.{x", ch->name);
	 wiznet(buf,NULL,NULL,WIZ_CHANNELING,0,0);
    }
    else {
	 send_to_char("{yYour immortality prevents you from losing the ability to channel.{x\n\r", ch);
    }
    return;
  }

  /* Tell wiznet */
  sprintf(buf, "%s has gained <%d> burnout points.", ch->name, bos);
  log_string(buf);
  wiznet(buf,NULL,NULL,WIZ_CHANNELING,0,0);
  
  ch->burnout += bos;
  return;
}

void update_holding(CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH];

  if (ch->channeling_pulse < 0)
    return;
  
  if (ch->channeling_pulse == 0) {
    
    /* 5% chance to take a BO if you hold longer then you are capable of */
    if (!IS_FORSAKEN(ch) && !IS_DR(ch) && !IS_IMMORTAL(ch)) {
	 if (number_chance(5)) {
	   sprintf(buf, "{rYou feel the strain as you hold %s longer than you can control safely.{x\n\r",
			 ch->sex == SEX_MALE ? "Saidin" : "Saidar");
	   send_to_char(buf, ch);
	   do_function(ch, &do_unchannel, "" );
	   gain_burnout(ch, 1);
	 }
	 else {
	   sprintf(buf, "You lose control of %s.\n\r", ch->sex == SEX_MALE ? "Saidin" : "Saidar");
	   send_to_char(buf, ch);
	   
	   do_function(ch, &do_unchannel, "" );
	 }
    }
  }
  
  sprintf(buf, "Updating channeling pulse for (%s) - next pulse (%d)",
		ch->name, ch->channeling_pulse-1);
  wiznet(buf,NULL,NULL,WIZ_TICKS,0,0);
  
  ch->channeling_pulse--;
  return;
}

void update_rpcounter(CHAR_DATA *ch)
{
	int modtime = 0;
        int chance = 50;
	long beforecheck, aftercheck;
	char buf[256];
   
        if (IS_NPC(ch))
	   return;

        if ((IS_WOLFKIN(ch) && !IS_IMMORTAL(ch)) || (ch->sex == SEX_MALE && ch->class == CLASS_CHANNELER && !IS_IMMORTAL(ch) && (!IS_SET(ch->act,PLR_STILLED))))
        {
	   if (!ch->pcdata->bondcount)  //If not bonded, 5 hours, otherwise 3
           {
		modtime = 360000;
           }
	   else
           {
                modtime = 360000 / ch->pcdata->bondcount;
		//chance = 100;
	   }

           beforecheck = ch->roleplayed % modtime;
           aftercheck = (ch->roleplayed + (current_time - ch->lastrpupdate)) % modtime;
	
           if ( beforecheck > aftercheck )
           {
               /*
               if (is_clan(ch))
               {
                  if (!strcasecmp(player_clan(ch),"Children") || !strcasecmp(player_clan(ch),"WhiteTower"))
		       if (ch->rank < 9) //Not prisoners
	                  chance = 100;
               }
	       */
               if (IS_FORSAKEN(ch) || (IS_TAVEREN(ch) && ch->insanity_points > 48 && !ch->pcdata->forceinsanity) || (IS_DR(ch) && ch->insanity_points > 48 && !ch->pcdata->forceinsanity)  || (!IS_NPC(ch) && ch->pcdata->df_level != -1  && ch->pcdata->df_level <= 6))
	       {
		    sprintf(buf,"%s: Checking Insanity: Is Forsaken, Taveren, DR, or High DF\r\n",ch->name);
		    log_string(buf);
           	    wiznet(buf,ch,NULL,WIZ_LEVELS,0,get_trust(ch));	
		    chance = 0;
	       }
               if (IS_SPECIAL(ch))
                    chance = chance / 4;
               if (IS_SET(ch->in_room->room_flags,ROOM_STEDDING))
	       {
		    sprintf(buf,"%s: Checking Insanity: Is inside a stedding\r\n",ch->name);
		    log_string(buf);
           	    wiznet(buf,ch,NULL,WIZ_LEVELS,0,get_trust(ch));	
		    chance = 0;
	       }

               if (ch->pcdata->learned[skill_lookup("seize")] <= 50)
	       {
		    sprintf(buf,"%s: Checking Insanity: Doesn't know how to channel yet - Skipping\r\n",ch->name);
		    log_string(buf);
           	    wiznet(buf,ch,NULL,WIZ_LEVELS,0,get_trust(ch));	
		    chance = 0;
	       }

	       if ((IS_DR(ch) || IS_TAVEREN(ch)) && chance > 0)
		    chance = chance / 2;

               /*
	       if (ch->pcdata->bondcount)  //If not bonded, 5 hours, otherwise 3
	       {
		    chance += (10*ch->pcdata->bondcount);
	       }
		*/
               int perc_chance = number_percent();
	
	       sprintf(buf,"%s: Insanity Roll: Spark? (%s)",ch->name,chance > perc_chance ? "TRUE" : "FALSE");
	       log_string(buf);
               wiznet(buf,ch,NULL,WIZ_LEVELS,0,get_trust(ch));	
		
	       if (chance > perc_chance)
               {
		  if (ch->pcdata->forceinsanity)
			ch->pcdata->forceinsanity = 0;
                  ch->insanity_points++;
		// Log it
	        char buf[256];
		sprintf(buf, "$N gained an insanity point (total=%d) at %d roleplay hours", ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s gained an insanity point (total=%d) at %d roleplay hours", ch->name, ch->insanity_points, (int)(ch->roleplayed + (current_time - ch->lastrpupdate)) / 3600);
		log_string(buf);
		if (IS_WOLFKIN(ch))
                {
			handle_wolfkin_insanity(ch);	
		}
		else
		{
			handle_mc_insanity(ch);	
		}
               }
           }
        }
	ch->roleplayed += (current_time - ch->lastrpupdate);
	ch->lastrpupdate = current_time;

}

void unstoppableweave(CHAR_DATA *ch,CHAR_DATA* victim,char * weave)
{

   int air = ch->perm_sphere[SPHERE_AIR];
   int earth = ch->perm_sphere[SPHERE_EARTH];
   int fire = ch->perm_sphere[SPHERE_FIRE];
   int water = ch->perm_sphere[SPHERE_WATER];
   int spirit = ch->perm_sphere[SPHERE_SPIRIT];
   int preweave = ch->pcdata->learned[skill_lookup(weave)];

   ch->pcdata->learned[skill_lookup(weave)] = 400;
   ch->perm_sphere[SPHERE_AIR] = 3000;
   ch->perm_sphere[SPHERE_EARTH] = 3000;
   ch->perm_sphere[SPHERE_FIRE] = 3000;
   ch->perm_sphere[SPHERE_WATER] = 3000;
   ch->perm_sphere[SPHERE_SPIRIT] = 3000;

   if (victim == NULL)
   {
     
        char buf[256];
        sprintf(buf,"'%s'",weave);
	do_function(ch,&do_cast,buf);
   }
   else
   {
       char buf[256];
       sprintf(buf,"'%s' %s",weave,victim->name);
       do_function(ch,&do_cast,buf);
   }

   ch->pcdata->learned[skill_lookup(weave)] = preweave;
   ch->perm_sphere[SPHERE_AIR] = air;
   ch->perm_sphere[SPHERE_EARTH] = earth;
   ch->perm_sphere[SPHERE_FIRE] = fire;
   ch->perm_sphere[SPHERE_WATER] = water;
   ch->perm_sphere[SPHERE_SPIRIT] = spirit;

}

void handle_mc_insanity(CHAR_DATA *ch)
{
 	int sn; //sn to use for giving an MC skills needed to complete the insainty

	switch (ch->insanity_points)
	{
	   case 5:
         	  act("You grab your head as you feel a HUGE headache coming on, like it is going to make your head split open.", ch, NULL, NULL, TO_CHAR);
         	  act("$n grabs $s head looking to be in pain.", ch, NULL, NULL, TO_ROOM);
		  do_function(ch, &do_seize, "" );
		  break;
	   case 10:
         	  act("Your eyes roll up as the headache returns, more intense this time, the after affects lasting a bit longer.", ch, NULL, NULL, TO_CHAR);
         	  act("$n's eyes roll back in the head slightly, evidently in pain.", ch, NULL, NULL, TO_ROOM);
		  do_function(ch, &do_seize, "" );
		  break;
	   case 15:
         	  act("You jerk suddenly, grab your head and fall to your knees in pain.", ch, NULL, NULL, TO_CHAR);
         	  act("$n looses $s balance as $m grabs $s head and almost hits the floor.", ch, NULL, NULL, TO_ROOM);
               
                  sn = skill_lookup( "create flame" );
                  if (ch->pcdata->learned[sn] < 100) 
			ch->pcdata->learned[sn] = 100; 
		  do_function(ch, &do_cast, "'create flame'" );
		  break;
	   case 20:
         	  act("You jerk suddenly, grab your head and fall to your knees in pain.", ch, NULL, NULL, TO_CHAR);
         	  act("$n looses $s balance as $m grabs $s head and almost hits the floor.", ch, NULL, NULL, TO_ROOM);
		  do_function(ch,&do_nofollow,"");
		 int preweave = ch->pcdata->learned[skill_lookup("earthquake")];
                 if (preweave < 300) 
                    ch->pcdata->learned[skill_lookup("earthquake")]  = 300; 
		 do_function(ch,&do_cast,"'earthquake'");
                 ch->pcdata->learned[skill_lookup("earthquake")]  = preweave; 
		  break;
  	   case 25:
         	  act("The pain is so intense you collapse to the ground, a scream of pain escaping from your mouth.", ch, NULL, NULL, TO_CHAR);
         	  act("$n slumps to the ground unconsious after letting out a scream of pain.", ch, NULL, NULL, TO_ROOM);
	          do_function(ch,&do_seize,"");
		  raw_kill(ch);
		  break;
	   case 30:
         	  act("You grab your chest, in pain, unable to catch your breath.", ch, NULL, NULL, TO_CHAR);
         	  act("$n grabs his chest, noticably in pain, and unable to catch his breath.", ch, NULL, NULL, TO_ROOM);
		  do_function(ch,&do_nofollow,"");
		  do_function(ch,&do_nofollow,"");
		  unstoppableweave(ch,NULL,"blizzard");
		  unstoppableweave(ch,NULL,"harm");
		  break;
	   case 35:
         	  act("You fall to your knees pain filling your body.", ch, NULL, NULL, TO_CHAR);
         	  act("$n falls to his knees pain evident on his face.", ch, NULL, NULL, TO_ROOM);
		  do_function(ch,&do_nofollow,"");
		  do_function(ch,&do_nofollow,"");
		  unstoppableweave(ch,NULL,"earthquake");
		  unstoppableweave(ch,NULL,"call");
		  unstoppableweave(ch,NULL,"inferno");
		  unstoppableweave(ch,NULL,"harm");
		  break;
	   case 40:
         	  act("You loose momentary control as your body is filled with pain.", ch, NULL, NULL, TO_CHAR);
         	  act("$n body twists in pain.", ch, NULL, NULL, TO_ROOM);
		  do_function(ch,&do_nofollow,"");
		  do_function(ch,&do_nofollow,"");
		  unstoppableweave(ch,NULL,"blizzard");
		  unstoppableweave(ch,NULL,"inferno");
                  break;
	   case 45:
         	  act("You loose control, flames leaping up around you, pain filling your entire body.", ch, NULL, NULL, TO_CHAR);
         	  act("$n lets out a pained scream as flames begin to leap across the ground.", ch, NULL, NULL, TO_ROOM);
		  do_function(ch,&do_nofollow,"");
		  do_function(ch,&do_nofollow,"");
		  unstoppableweave(ch,NULL,"earthquake");
		  unstoppableweave(ch,NULL,"inferno");
		  raw_kill(ch);
		  break;
           case 48:
         	  act("You loose control, the ground begins to shake and flames begin to leap up everywhere.", ch, NULL, NULL, TO_CHAR);
         	  act("The ground begins to shake and flames begin to leap through the air toward you.", ch, NULL, NULL, TO_ROOM);
		  do_function(ch,&do_nofollow,"");
		  do_function(ch,&do_nofollow,"");
		  unstoppableweave(ch,NULL,"earthquake");
		  unstoppableweave(ch,NULL,"inferno");
		  do_slayroom(ch,"");
		  raw_kill(ch);
                  break;
	   case 50:
         	  act("You are no longer in control. Flame erupts everywhere, burning evertyhing to death, including yourself.", ch, NULL, NULL, TO_CHAR);
         	  act("Flames begin to appear everwhere, engulfing everything that remains in the room.", ch, NULL, NULL, TO_ROOM);
		  do_function(ch,&do_nofollow,"");
		  do_function(ch,&do_nofollow,"");
		  unstoppableweave(ch,NULL,"earthquake");
		  unstoppableweave(ch,NULL,"inferno");
		  do_slayroom(ch,"");
		  raw_kill(ch);
    		  make_corpse(ch);
    		  stop_follower(ch);
    		  char_from_room( ch );
    		  char_to_room( ch, get_room_index( ROOM_VNUM_DEATH));
    		  do_restore(ch,ch->name);
    		  save_char_obj( ch, FALSE );
		  break;

		default: break;
	}

}

void handle_wolfkin_insanity(CHAR_DATA *ch)
{
 	int sn; //sn to use for giving an MC skills needed to complete the insainty

	switch (ch->insanity_points)
	{
	   case 5:
         	  act("You notice that there is an odd odor in the air", ch, NULL, NULL, TO_CHAR);
         	  act("$n tilts $s head and sniffs around.", ch, NULL, NULL, TO_ROOM);
		  break;
	   case 10:
         	  act("You feel a presense for a moment, off in the distance, calling you.", ch, NULL, NULL, TO_CHAR);
         	  act("$n gazes off into the distance.", ch, NULL, NULL, TO_ROOM);
		  break;
	   case 15:
         	  act("You start to feel the call of the wolf pack, summoning you to join them.", ch, NULL, NULL, TO_CHAR);
         	  act("$n gazes off into the distance with a blank look on $s face.", ch, NULL, NULL, TO_ROOM);
		  break;
	   case 20:
         	  act("You feel an itch starting to form behind your ear that you just can't scratch enough.", ch, NULL, NULL, TO_CHAR);
         	  act("$n starts to scratch behind $s ear.", ch, NULL, NULL, TO_ROOM);
		  break;
  	   case 25:
         	  act("The feel an overwhelming urge to howl at the moon.", ch, NULL, NULL, TO_CHAR);
         	  act("$n glances skyward for a moment.", ch, NULL, NULL, TO_ROOM);
		  break;
	   case 30:
         	  act("You sense the overwhelming body odor of your companion(s).", ch, NULL, NULL, TO_CHAR);
         	  act("$n wrinkles $s nose as though detecting something foul.", ch, NULL, NULL, TO_ROOM);
		  break;
	   case 35:
         	  act("You notice that the wolves calling in the distance are harder to ignore.", ch, NULL, NULL, TO_CHAR);
         	  act("$n gazes off into the distance for a long moment.", ch, NULL, NULL, TO_ROOM);
		  break;
	   case 40:
         	  act("You start to itch all over.", ch, NULL, NULL, TO_CHAR);
         	  act("$n body twists in discomfort.", ch, NULL, NULL, TO_ROOM);
                  break;
	   case 45:
         	  act("You feel like your companions are trying to cage you.", ch, NULL, NULL, TO_CHAR);
         	  act("$n curls $s lips back in a snarl.", ch, NULL, NULL, TO_ROOM);
		  break;
           case 48:
         	  act("You have given yourself over to the wolves a little too much, incorporate it into your RP.", ch, NULL, NULL, TO_CHAR);
                  break;
	   case 50:
         	  act("You are no longer in control. Your mind is gone, off to run with the wolves.", ch, NULL, NULL, TO_CHAR);
         	  act("$n gives $sself over to the call of the wolves, letting their mind be consumed.", ch, NULL, NULL, TO_ROOM);
		  raw_kill(ch);
    		  make_corpse(ch);
    		  stop_follower(ch);
    		  char_from_room( ch );
    		  char_to_room( ch, get_room_index( ROOM_VNUM_DEATH));
    		  do_restore(ch,ch->name);
    		  save_char_obj( ch, FALSE );
		  break;

		default: break;
	}

}

void update_skimming(CHAR_DATA *ch)
{
  CHAR_DATA *vch=NULL;
  CHAR_DATA *vch_next=NULL;
  
  if (ch->skim_pulse < 0)
    return;
  
  if (ch->skim_pulse == 0) {
    ch->skim_pulse--;
    
    send_to_char("You feel the motion slowing down as you arrive to your destination.\n\r", ch);
    ch->skim_motion = FALSE;
    
    // Followers?
    for ( vch = char_list; vch != NULL; vch = vch_next ) {
	 vch_next = vch->next;
	 if (ch == vch)
	   continue;
	 if (!IS_SAME_WORLD(vch, ch))
	   continue;
	 if (!is_same_group(ch, vch))
	   continue;
	 send_to_char("You feel the motion slowing down as you arrive to your destination.\n\r", vch);
	 vch->skim_motion = FALSE;
    }
    
    return;
  }
  
  send_to_char("You feel a motion moving you through the {Ddark void{x.\n\r", ch);
  ch->skim_pulse--;
  
  // Followers?
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next = vch->next;
    if (ch == vch)
	 continue;
    if (!IS_SAME_WORLD(vch, ch))
	 continue;
    if (!is_same_group(ch, vch))
	 continue;    
    send_to_char("You feel a motion moving you through the {Ddark void{x.\n\r", vch);
  }

  return;
}

void update_study(CHAR_DATA *ch)
{
  char key[17];
  char buf[MAX_STRING_LENGTH];

  if (ch->study_pulse < 0)
    return;
  
  if (ch->study_pulse == 0) {
    memset(key, 0x00, sizeof(key));
    send_to_char("You memorize the surrounding area.\n\r", ch);
    strcpy(key, vnum2key(ch, ch->in_room->vnum));
    sprintf(buf, "Key is: {r%s{x\n\r", key);
    send_to_char(buf, ch);
    ch->study = FALSE;
    ch->study_pulse = -1;
    if (ch->study_name != NULL) {
	sprintf(buf,"%s %s",ch->study_name, key);
	do_addkey(ch,buf);
    }
    return;
  }
  else {
    send_to_char("You continue to study your surroundings.\n\r", ch);
    ch->study_pulse--;
    return;
  }
  
  return;
}

void check_warmboot(void)
{
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim;
  int mins=0;
  int secs=0;

  
  buf[0] = '\0';
  
  if (pulse_warmboot < 0)
    return;
  
  if (pulse_warmboot == 0) {
    if (iscopyin)
       do_copyinboot(warmboot_pc, NULL);
    else
       do_warmboot(warmboot_pc, NULL);
  }
  
  if ((pulse_warmboot % 30) == 0) {
    if (pulse_warmboot > 60) {
	 mins = pulse_warmboot/60;
	 secs = pulse_warmboot-60*mins;
	 sprintf(buf,"%s by %s{x in %d %s%s.\n\r%s",
		    iswarmboot ? "Warmboot" : "Crash", 
		    (warmboot_pc->invis_level  >= LEVEL_HERO) ? "someone" : COLORNAME(warmboot_pc), 
		    mins,
		    (mins >  1) ? "minutes" : "minute",
		    (secs == 0) ? "" : " and 30 seconds",
		    warmbootMsg);
    }
    else {
	 sprintf(buf,"%s by %s{x in %d seconds.\n\r%s",
		    iswarmboot ? "Warmboot" : "Shutdown", 
                    (warmboot_pc->invis_level  >= LEVEL_HERO) ? "someone" : COLORNAME(warmboot_pc),
		    pulse_warmboot,
		    warmbootMsg);
    }
    
    for ( d = descriptor_list; d; d = d->next ) {
	 if (d->character != NULL && d->connected == CON_PLAYING) {
	   victim = d->character;
	   send_to_char(buf, victim);
	 }
    }
  }
  
  --pulse_warmboot;
  return;
}

void check_shutdown(void)
{
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim;
  int mins=0;
  int secs=0;
    
  buf[0] = '\0';
  
  if (pulse_shutdown < 0) {
    return;
  }
  
  if (pulse_shutdown == 0)
    do_shutdown(warmboot_pc, NULL);
  
  if ((pulse_shutdown % 30) == 0) {
    if (pulse_shutdown > 60) {
	 mins = pulse_shutdown/60;
	 secs = pulse_shutdown-60*mins;
	 sprintf(buf,"%s by %s{x in %d %s%s.\n\r%s",
		    isshutdown ? "Shutdown" : "Crash", 
		    (warmboot_pc->invis_level  >= LEVEL_HERO) ? "someone" : COLORNAME(warmboot_pc), 
		    mins,
		    (mins >  1) ? "minutes" : "minute",
		    (secs == 0) ? "" : " and 30 seconds",
		    warmbootMsg);
    }
    else {
	 sprintf(buf,"%s by %s{x in %d seconds.\n\r%s",
		    isshutdown ? "Shutdown" : "Shutdown", 
                    (warmboot_pc->invis_level  >= LEVEL_HERO) ? "someone" : COLORNAME(warmboot_pc),
		    pulse_shutdown,
		    warmbootMsg);
    }
    
    for ( d = descriptor_list; d; d = d->next ) {
	 if (d->character != NULL && d->connected == CON_PLAYING) {
	   victim = d->character;
	   send_to_char(buf, victim);
	   save_polls(victim,"");
	 }
    }
  }
  
  --pulse_shutdown;
  return;
}

// check if a character is strong enough for a weave.
bool can_handle_weave(CHAR_DATA *ch, int sn)
{
  int i=0;
  
  for (i = 0; i < MAX_SPHERE; i++)
    if (skill_table[sn].spheres[i] > ch->perm_sphere[i])
	 return FALSE;
  
  return TRUE;
}

void check_talent(CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH];
  int sn;
  int chance            =  0;

  // The different chances:
  int wk_chance           = ch->pcdata->forcespark ? 0 : 95; /* This variables hold percent chance. */ 
  int sniffing_chance     = ch->pcdata->forcespark ? 0 : 80;  /* 98 -> 2% chance to spark            */
  int traveling_chance    = ch->pcdata->forcespark ? 0 : 80;
  int residue_chance      = ch->pcdata->forcespark ? 0 : 80;
  int illusion_chance     = ch->pcdata->forcespark ? 0 : 80;
  int compulsion_chance   = ch->pcdata->forcespark ? 0 : 89;
  int cloud_chance        = ch->pcdata->forcespark ? 0 : 80;
  int tree_singing_chance = ch->pcdata->forcespark ? 0 : 80;
  int lost_talent_chance  = ch->pcdata->forcespark ? 0 : 80;
  int keeping_chance      = ch->pcdata->forcespark ? 0 : 80;
  
  if (IS_IMMORTAL(ch))
    return;
  
  if (IS_NPC(ch))
    return;

  // ###### WOLFKIN TALENT
  if (IS_SET(ch->talents, TALENT_WOLFKIN)) {
    sn = skill_lookup("entice");
    if (sn > 0) {
	 if ((ch->level >= skill_table[sn].skill_level[ch->class]) && (ch->pcdata->learned[sn] <= 0)) {
	   chance = number_percent();
	   if (chance > wk_chance) {
		send_to_char("[ {BSpark {x]{W:{x You suddenly get exceptionally sharp hearing, smelling, and vision skills.\n\r", ch);
		
		if (!IS_SET(ch->merits, MERIT_ACUTESENSES))
	       SET_BIT(ch->merits, MERIT_ACUTESENSES);    
		
		if (!IS_SET(ch->affected_by, AFF_INFRARED))
		  SET_BIT(ch->affected_by, AFF_INFRARED);
		
	     ch->pcdata->learned[sn] = 25;
		send_to_char("[ {BSpark {x]{W:{x You now know how to 'entice' your wolf brothers to assist you.\n\r", ch);
		
		send_to_char("[ {BSpark {x]{W:{x Your eyes have turned {YYellow{x!!\n\r", ch);
		
		// Log it
		sprintf(buf, "$N sparked the wolfkin talent (chance=%d, limit=%d) and gained '%s'", chance, wk_chance, skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s sparked the wolfkin talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, wk_chance, skill_table[sn].name);
		log_string(buf);
	   }
	 }
    }
  }
    
  // ###### SNIFFING TALENT
  if (IS_SET(ch->talents, TALENT_SNIFFING)) {
    sn = skill_lookup("hunt");
    if (sn > 0) {
	 if ((ch->level >= skill_table[sn].skill_level[ch->class]) && (ch->pcdata->learned[sn] <= 0)) {
	   chance = number_percent();
	   if (chance > sniffing_chance) {
		send_to_char("[ {BSpark {x]{W: You suddenly get aware of different smells in the air!\n\r", ch);
		ch->pcdata->learned[sn] = 1;
		send_to_char("[ {BSpark {x]{W:{x You start to know how to 'hunt' by using your senses.\n\r", ch);
		
		// Log it
		sprintf(buf, "$N sparked the sniffing talent (chance=%d, limit=%d) and gained '%s'", chance, sniffing_chance, skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s sparked the sniffing talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, sniffing_chance, skill_table[sn].name);
		log_string(buf);
	   }
	 }
    }
  }

  // ###### TRAVELING TALENT
  if (IS_SET(ch->talents, TALENT_TRAVELING)) {
    sn = skill_lookup("gate");
    if (sn > 0 && can_handle_weave(ch, sn)) {
	 if ((ch->level >= skill_table[sn].skill_level[ch->class]) && (ch->pcdata->learned[sn] <= 0)) {
	   chance = number_percent();
	   if (chance > traveling_chance) {
		sprintf(buf, "[ {BSpark {x]{W:{x By combining flows of %s you see how you can travel!\n\r", (char *)flow_text(sn, ch));
		send_to_char(buf, ch);
		ch->pcdata->learned[sn] = 1;
		
		// Log it
		sprintf(buf, "$N sparked the traveling talent (chance=%d, limit=%d) and gained '%s'", chance, traveling_chance, skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s sparked the traveling talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, traveling_chance, skill_table[sn].name);
		log_string(buf);

		// Dreamgate
		sn = skill_lookup("dreamgate");
		ch->pcdata->learned[sn] = 1;
		
		// Skimming
		sn = skill_lookup("skimming");
		ch->pcdata->learned[sn] = 1;
		// Log it

		sprintf(buf, "$N sparked the traveling talent (chance=%d, limit=%d) and gained '%s'", chance, traveling_chance, skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s sparked the traveling talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, traveling_chance, skill_table[sn].name);
		log_string(buf);

	   }
	 }
    }
  }
  
  // ###### RESIDUE TALENT
  if (IS_SET(ch->talents, TALENT_RESIDUES)) {
    sn = skill_lookup("study residue");
    
    if (sn > 0 && can_handle_weave(ch, sn)) {
	 if ((ch->level >= skill_table[sn].skill_level[ch->class]) && (ch->pcdata->learned[sn] <= 0)) {
	   chance = number_percent();
	   if (chance > residue_chance) {
		sprintf(buf, "[ {BSpark {x]{W:{x You suddenly get aware of residues and is able to '%s' now!\n\r", skill_table[sn].name);
		send_to_char(buf, ch);
		ch->pcdata->learned[sn] = 1;
    		sn = skill_lookup("invert weaves");
		
		// Log it
		sprintf(buf, "$N sparked the residues talent (chance=%d, limit=%d) and gained '%s'", chance, residue_chance, skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s sparked the residues talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, residue_chance, skill_table[sn].name);
		log_string(buf);
	   }
	 }     	
    }  	
  }
  
  // ###### ILLUSION TALENT
  if (IS_SET(ch->talents, TALENT_ILLUSION)) {    
    if (ch->level >= skill_table[gsn_invis].skill_level[ch->class] && (ch->pcdata->learned[gsn_invis] <= 0) && can_handle_weave(ch, gsn_invis)) {
	 chance = number_percent();
	 if (chance > illusion_chance) {
	   sprintf(buf, "[ {BSpark {x]{W:{x By combining flows of %s you see how you can create illusions!\n\r", (char *)flow_text(gsn_invis, ch));
	   send_to_char(buf, ch);
	   ch->pcdata->learned[gsn_invis] = 1;
	   sprintf(buf, "[ {BSpark {x]{W:{x By combining flows of %s you see how you can create mirror of mists!\n\r", (char *)flow_text(gsn_invis, ch));
	   send_to_char(buf, ch);
	   ch->pcdata->learned[gsn_mirror_of_mists] = 1;
	   
	   // Log it
	   sprintf(buf, "$N sparked the illusion talent (chance=%d, limit=%d) and gained '%s'", chance, illusion_chance, skill_table[gsn_invis].name);
	   wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
	   sprintf(buf, "$N sparked the illusion talent (chance=%d, limit=%d) and gained '%s'", chance, illusion_chance, skill_table[gsn_mirror_of_mists].name);
	   wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
	   sprintf(buf, "%s sparked the illusion talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, illusion_chance, skill_table[gsn_invis].name);
	   log_string(buf);
	   sprintf(buf, "%s sparked the illusion talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, illusion_chance, skill_table[gsn_mirror_of_mists].name);
	   log_string(buf);
	 }
    }
  }

  // ###### COMPULSION TALENT
  if (IS_SET(ch->talents, TALENT_COMPULSION)) {
    if (ch->level >= skill_table[gsn_charm_person].skill_level[ch->class] && (ch->pcdata->learned[gsn_charm_person] <= 0) && can_handle_weave(ch, gsn_charm_person)) {
	 chance = number_percent();
	 if (chance > compulsion_chance) {
	   sprintf(buf, "[ {BSpark {x]{W:{x By combining flows of %s you see how you can compulse someone!\n\r", (char *)flow_text(gsn_charm_person, ch));
	   send_to_char(buf, ch);
	   ch->pcdata->learned[gsn_charm_person] = 1;
	   
	   // Log it
	   sprintf(buf, "$N sparked the compulsion talent (chance=%d, limit=%d) and gained '%s'", chance, compulsion_chance, skill_table[gsn_charm_person].name);
	   wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
	   sprintf(buf, "%s sparked the compulsion talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, compulsion_chance, skill_table[gsn_charm_person].name);
	   log_string(buf);
	 }
    }
  }

  // ###### CLOUD DANCING TALENT
  if (IS_SET(ch->talents, TALENT_CLOUDDANCING)) {
    sn = skill_lookup("control weather");
    if (sn > 0 && can_handle_weave(ch, sn)) {
	 if ((ch->level >= skill_table[sn].skill_level[ch->class]) && (ch->pcdata->learned[sn] <= 0)) {
	   chance = number_percent();
	   if (chance > cloud_chance) {
		sprintf(buf, "[ {BSpark {x]{W:{x By combining flows of %s you see how you can control the weather!\n\r", (char *)flow_text(sn, ch));
		send_to_char(buf, ch);
		ch->pcdata->learned[sn] = 1;
		
		// Log it
		sprintf(buf, "$N sparked the cloud dancing talent (chance=%d, limit=%d) and gained '%s'", chance, cloud_chance, skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s sparked the cloud dancing talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, cloud_chance, skill_table[sn].name);
		log_string(buf);
	   }
	 }
    }
  }

  // ###### TREE SINGING TALENT
  if (IS_SET(ch->talents, TALENT_TREE_SINGING)) {
    sn = gsn_singwood;
    if (sn > 0) {
	 if ((ch->level >= skill_table[sn].skill_level[ch->class]) && (ch->pcdata->learned[sn] <= 0)) {
	   chance = number_percent();
	   if (chance > tree_singing_chance) {
		send_to_char("[ {BSpark {x]{W:{x You suddenly are able to sense the trees and their feelings.\n\r", ch);
		send_to_char("[ {BSpark {x]{W:{x You now know how to 'singwood' from trees.\n\r", ch);
		ch->pcdata->learned[sn] = 1;
		
		// Log it
		sprintf(buf, "$N sparked the tree singing talent (chance=%d, limit=%d) and gained '%s'", chance, tree_singing_chance, skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s sparked the tree singing talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, tree_singing_chance,  skill_table[sn].name);
		log_string(buf);
	   }
	 }
    }
  }

  // ###### LOST TALENT
  // Atm only create angreal is in this talent
  if ((IS_SET(ch->talents, TALENT_LOST)) || IS_SET(ch->talents, TALENT_CREATE_ANGREAL)) {
    sn = gsn_create_angreal;
    if (sn > 0 && can_handle_weave(ch, sn)) {
	 if ((ch->level >= skill_table[sn].skill_level[ch->class]) && (ch->pcdata->learned[sn] <= 0)) {
	   chance = number_percent();
	   if (chance > lost_talent_chance) {
		sprintf(buf, "[ {BSpark {x]{W:{x By combining flows of %s you see how you can create angreal!\n\r", (char *)flow_text(sn, ch));
		send_to_char(buf, ch);
		
		ch->pcdata->learned[sn] = 1;
		
		// Log it
		sprintf(buf, "$N sparked the lost talent (chance=%d, limit=%d) and gained '%s'", chance, lost_talent_chance, skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s sparked the lost talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, lost_talent_chance,  skill_table[sn].name);
		log_string(buf);

		REMOVE_BIT(ch->talents, TALENT_LOST);
		SET_BIT(ch->talents, TALENT_CREATE_ANGREAL);
	   }
	 }
    }
  }

  // ###### KEEPING TALENT
  if (IS_SET(ch->talents, TALENT_KEEPING)) {
    sn = gsn_keep;
    if (sn > 0 && can_handle_weave(ch, sn)) {
	 if ((ch->level >= skill_table[sn].skill_level[ch->class]) && (ch->pcdata->learned[sn] <= 0)) {
	   chance = number_percent();
	   if (chance > keeping_chance) {
		sprintf(buf, "[ {BSpark {x]{W:{x By combining flows of %s you see how to protect perishable objects!\n\r", (char *)flow_text(sn, ch));
		send_to_char(buf, ch);
		
		ch->pcdata->learned[sn] = 1;
		
		// Log it
		sprintf(buf, "$N sparked the keeping talent (chance=%d, limit=%d) and gained '%s'", chance, keeping_chance, skill_table[sn].name);
		wiznet(buf, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
		sprintf(buf, "%s sparked the keeping talent (chance=%d, limit=%d) and gained '%s'", ch->name, chance, keeping_chance,  skill_table[sn].name);
		log_string(buf);
	   }
	 }
    }
  }
  
  return;
}

/*
 * Advancement stuff.
 */
void advance_level( CHAR_DATA *ch, bool hide, bool extended)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  char tmp[MAX_STRING_LENGTH];
  int add_hp;
  int add_endurance;
  int add_train;
  int add_sphere;
  int add_skill;
  bool Sflag=FALSE;
  bool Eflag=FALSE; 
  int i;
  int sn=0;
  int skill_chance = 4;
  int weave_chance = 9;
  int bonus_pc_train = 0;
  
  ch->pcdata->last_level = (ch->played + (int)(current_time - ch->logon))/3600;
  
  add_hp        = number_range( 10, (get_curr_stat(ch,STAT_CON)
							 + get_curr_stat(ch,STAT_STR)));
  
  add_endurance = number_range( 10, (get_curr_stat(ch,STAT_CON)
							 + get_curr_stat(ch,STAT_DEX)));
  
  if (ch->class == CLASS_CHANNELER) {
    add_hp        = add_hp * 2;
    add_endurance = add_endurance * 2;
  }
  
  if (ch->class == CLASS_WARRIOR) {
    add_hp        = add_hp * 2;
    add_endurance = add_endurance * 2;
  }

  if (ch->class == CLASS_THIEF) {
    add_hp        = add_hp        * 2;
    add_endurance = add_endurance * 2;
  }
  
  /* WIS + INT / 2 */
  add_train = ((get_curr_stat(ch,STAT_WIS) + get_curr_stat(ch,STAT_INT))/2);
  
  add_hp	      = UMAX(  10, add_hp   );
  add_endurance = UMAX(  10, add_endurance );
  
  /* Adjust hit and endurance gain according to merit/flaws */
  //#define TARGET_WEAK_HP	      4000
  //#define TARGET_NORMAL_HP      5000
  //#define TARGET_TOUGH_HP       6000
  if (IS_SET(ch->merits, MERIT_TOUGH))
  {
	if (ch->max_hit >= TARGET_TOUGH_HP)
	{
		add_hp = number_range(6,9);
	}
	else
    {
    		add_hp = add_hp * 2;
	}
  }
  else
  if (IS_SET(ch->flaws, FLAW_WEAK))
  {
    if (ch->max_hit >= TARGET_WEAK_HP)
    {
	add_hp = number_range(3,6);
    }
    else
    	add_hp = add_hp/2;
  }
  else
  {
	if (ch->max_hit >= TARGET_NORMAL_HP)
        {
		add_hp = number_range(6,9);
        }
  }
  if (IS_SET(ch->merits, MERIT_LONGWINDED))
    add_endurance = add_endurance * 2;

  if (ch->max_hit >= MAX_PC_HP) {
    if (extended)
	 add_hp = add_hp / 3;
    else
	 add_hp = add_hp / 2;
  }
  
  if (ch->max_endurance >= MAX_PC_END) {
    if (extended)
	 add_endurance = add_endurance / 3;
    else
	 add_endurance = add_endurance / 2;
  }
  
  /* Extended level */
  if(extended) {
    add_train = add_train / 4;

    add_skill= number_range(2,4);
    if (IS_SET(ch->merits, MERIT_FASTLEARNER)) {
	add_skill += 1;
    }

    bonus_pc_train = get_level(ch) - LEVEL_HERO - 1;
    
    /* Increase a random skill above 85 every mod level */
    if ((get_level(ch) % 2) == 0) {

      if (IS_SET(ch->merits, MERIT_FASTLEARNER)) {
         weave_chance += 2;
         skill_chance += 2;
      }
  
      if (IS_SET(ch->flaws, FLAW_SLOWLEARNER)) {
         weave_chance -= 2;
         skill_chance -= 1;         
      }

	 // For channelers, first check increase on weaves.. then skills.
	 if (ch->class == CLASS_CHANNELER) {
	   for (sn = 0; sn < MAX_SKILL; sn++) {
		if (skill_table[sn].spell_fun == spell_null)
		  continue;
		if ((ch->pcdata->learned[sn] >= (MAX_PC_TRAIN-15)) &&
		    ch->pcdata->learned[sn] < (MAX_PC_TRAIN + bonus_pc_train) && number_percent() <= weave_chance ) {
		  Eflag = TRUE;
		  break;
		}
	   }
	   if (!Eflag) {
		for (sn = 0; sn < MAX_SKILL; sn++) {
		  if ((ch->pcdata->learned[sn] >= (MAX_PC_TRAIN-15)) &&
			 ch->pcdata->learned[sn] < MAX_PC_TRAIN && number_percent() <= skill_chance ) {
		    Eflag = TRUE;
		    break;		
		  }
		}
	   }
	 }
	 // Other than Channelers...
	 else {
	   for (sn = 0; sn < MAX_SKILL; sn++) {
		if ((ch->pcdata->learned[sn] >= (MAX_PC_TRAIN-15)) &&
		    ch->pcdata->learned[sn] < (MAX_PC_TRAIN + bonus_pc_train) && number_percent() <= skill_chance ) {
		  Eflag = TRUE;
		  break;		
		}
	   }
	 }
	 
	 if (Eflag) {	   
	   sprintf(buf3, "You gain {W%d{x point in '%s'.\n\r", add_skill, skill_table[sn].name);
	   ch->pcdata->learned[sn] += add_skill;
	   
	   // Log it
	   sprintf(tmp, "$N, level %d, gained %d point in '%s' from extended leveling (learned=%d).", get_level(ch), add_skill, skill_table[sn].name, ch->pcdata->learned[sn]);
	   wiznet(tmp, ch, NULL, WIZ_LEVELS, 0, get_trust(ch));
	   sprintf(tmp, "%s, level %d, gained %d point in '%s' from extended leveling (learned=%d).", ch->name, get_level(ch), add_skill, skill_table[sn].name, ch->pcdata->learned[sn]);
	   log_string(tmp);
	}
    }
  }
  
  if (ch->max_hit <= MAX_TOTAL_HP)
  {
  	ch->max_hit 	               += add_hp;
  	ch->pcdata->perm_hit	     += add_hp;
  	// Fix hit locations:
  	ch->hit_loc[LOC_LA] = UMIN(ch->hit_loc[LOC_LA] + (add_hp/LOC_MOD_LA), get_max_hit_loc(ch, LOC_LA)); 
  	ch->hit_loc[LOC_LL] = UMIN(ch->hit_loc[LOC_LL] + (add_hp/LOC_MOD_LL), get_max_hit_loc(ch, LOC_LL));
  	ch->hit_loc[LOC_HE] = UMIN(ch->hit_loc[LOC_HE] + (add_hp/LOC_MOD_HE), get_max_hit_loc(ch, LOC_HE)); 
  	ch->hit_loc[LOC_BD] = UMIN(ch->hit_loc[LOC_BD] + (add_hp/LOC_MOD_BD), get_max_hit_loc(ch, LOC_BD));
  	ch->hit_loc[LOC_RA] = UMIN(ch->hit_loc[LOC_RA] + (add_hp/LOC_MOD_RA), get_max_hit_loc(ch, LOC_RA));
  	ch->hit_loc[LOC_RL] = UMIN(ch->hit_loc[LOC_RL] + (add_hp/LOC_MOD_RL), get_max_hit_loc(ch, LOC_RL));    
  }
  else
 	add_hp = 0;
  if (ch->max_endurance <= MAX_TOTAL_ENDURANCE)
  {
  	ch->max_endurance	          += add_endurance;
  	ch->pcdata->perm_endurance	+= add_endurance;
  }
  else
	add_endurance = 0;

  ch->train                   += add_train;

    
  /* Increase spheres if below creation sphere */
  if (ch->class == CLASS_CHANNELER 
	 && ch->pcdata->learned[skill_lookup(ch->sex == SEX_MALE ? "seize" : "embrace")] > 0 
	 && !IS_SET(ch->act,PLR_STILLED)
	 && !IS_LINKED(ch)) {
    sprintf(buf2, "Your spheres increase ");
    for (i=0; i < MAX_SPHERE; i++) {
	 if (ch->cre_sphere[i] != 0 && ch->perm_sphere[i] < ch->cre_sphere[i]) {
	   Sflag = TRUE;
	   add_sphere = dice(1,5);
	   
	   if (ch->perm_sphere[i] + add_sphere >= ch->cre_sphere[i]) {
		add_sphere = ch->cre_sphere[i] - ch->perm_sphere[i];
		ch->perm_sphere[i] = ch->cre_sphere[i];
	   }
	   else {
		ch->perm_sphere[i] += add_sphere;
	   }
	   
	   sprintf(tmp, "{c%d{x %s ", add_sphere, sphere_table[i].name);
	   strcat(buf2, tmp);
	 }
    }
    buf2[strlen(buf2)-1] = '\0';
    strcat(buf2, ".\n\r");
  }
  
  if (!hide) {
    send_to_char("{WYou advance a l{re{Wv{re{Wl{r!{x\n\r", ch);
    sprintf(buf,
		  "You gain {R%d{x hit point%s, {c%d{x endurance, and {W%d{x train%s.\n\r",
		  add_hp, add_hp == 1 ? "" : "s", add_endurance,
		  add_train, add_train == 1 ? "" : "s");
    send_to_char( buf, ch );
    if (Sflag)
	 send_to_char( buf2, ch);
    if (Eflag)
	 send_to_char( buf3, ch);
  }
  
  check_talent(ch);

  if (!hide)
     send_to_char("Now is a good time to '{Wbackup{x' your character.\n\r", ch);

  return;
}   

/* Calculate max legal stockup for a character */
long stock_exp( CHAR_DATA *ch)
{
  long stockup_limit=0;
  int lvl_limit=3;        /* Levels to stock up exp */
  //int i=0;
  
  stockup_limit = lvl_limit * 2000;
/*
  for(i=0;i<lvl_limit;i++) {
    stockup_limit += (((get_level(ch)+i)*(get_level(ch)+i))*3)*100;
  }
*/
  
  return (stockup_limit);
}

void gain_exp( CHAR_DATA *ch, int gain )
{
  char buf[MAX_STRING_LENGTH];
  
  if ( IS_NPC(ch) || ch->level > LEVEL_HERO )
    return;
  
  if (IS_SET(ch->comm,COMM_AFK)) {
     send_to_char("{YYou are AFK, please act like it!{x\n\r", ch);
     return;
  }
  
  if (gain < 0) {
    ch->exp = ch->exp + gain;
    sprintf(buf, "You have lost {r%d{x experience points.\n\r", -gain);
    send_to_char( buf, ch );
    
    if (IS_SET(ch->act, PLR_CANLEVEL) && ch->exp < exp_next_level(ch)) {
	 REMOVE_BIT(ch->act, PLR_CANLEVEL);
    }
    return;
  }
  
  /* Don't allow stock up more than x lvls */
  if (ch->exp > stock_exp(ch) && !IS_RP(ch) && get_level(ch) < LEVEL_HERO - 1) {
    sprintf(buf, "{rYou need to spend some of your experience point before gaining any new.{x\n\r");  
    send_to_char(buf, ch);
    return;
  }
  
  ch->exp = ch->exp + gain;
  sprintf( buf, "You receive {r%d{x experience points.\n\r", gain );
  send_to_char( buf, ch );
  
  if (ch->level < LEVEL_HERO && ch->exp >= exp_next_level(ch) && !IS_SET(ch->act, PLR_CANLEVEL)) {
    send_to_char("{WYou gain a l{re{Wv{re{Wl{r!!{x\n\r", ch );
    send_to_char("Use the 'level' command to advance your level.\n\r", ch);
    SET_BIT(ch->act, PLR_CANLEVEL);
  }
  else if (IS_SET(ch->act, PLR_CANLEVEL) && ch->exp < exp_next_level(ch)) {
    REMOVE_BIT(ch->act, PLR_CANLEVEL);
  }
  return;
}

void do_level(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  
  if (IS_NPC(ch))
    return;

  if (IS_FACELESS(ch))
  {
	send_to_char("You are a temporary character and not allowed to level.\r\n",ch);
	return;
  }
  if (IS_IMMORTAL(ch)) {
    send_to_char("Don't think so!\n\r", ch);
    return;
  }

/*
  if (ch->level >= LEVEL_HERO-1) {
    send_to_char("You have reached the level limit!\n\rUse your levels wise and go seek fame and it's RP.\n\r", ch);
    return;
  }
*/

  if (!IS_SET(ch->act, PLR_CANLEVEL)) {
    send_to_char("You don't have enough experience points to gain a level!\n\r", ch);
    return;
  }

  if (get_level(ch) >= MAX_NORMAL_LEVEL)
  {
     if (!str_cmp(argument,""))
     {
	sprintf(buf,"The syntax for levelling after level %d is: level <skill to train>\n\r",MAX_NORMAL_LEVEL);
	send_to_char(buf,ch);
	return;
     }
     if (ch->pcdata->learned[skill_lookup(argument)] <= 0)
     {
  	send_to_char("You haven't learned that skill yet.\r\n",ch); 
	return;
     }

     int skillsn = skill_lookup(argument);
     if (is_masterform(skillsn))
     {
	send_to_char("You have to train up master forms other ways.\r\n", ch);
	return;
     }

     if (ch->pcdata->learned[skillsn] >= MAX_TRAINABLE_SKILL_LEVEL)
     {
	sprintf(buf,"You are a master at %s and can learn no more.\r\n",skill_table[skillsn].name);
	send_to_char(buf,ch);
	return;
     }


     //we've made it through all of the exceptions, now give them a point in their chosen skill
     if (get_level(ch) % 5) //every normal level 1 through 4 give 1 point
     {
     	ch->pcdata->learned[skill_lookup(argument)]++;
     	sprintf(buf,"You advance a level and gain one point in %s\n\r",skill_table[skillsn].name);
     	send_to_char(buf,ch);
     }
     else
     {
	if (IS_SET(ch->flaws, FLAW_SLOWLEARNER))
        {
     		ch->pcdata->learned[skill_lookup(argument)]++;
     		sprintf(buf,"You advance a level and gain one point in %s\n\r",skill_table[skillsn].name);
     		send_to_char(buf,ch);
        }
	else
	if (IS_SET(ch->merits, MERIT_FASTLEARNER))
	{
     		ch->pcdata->learned[skill_lookup(argument)] += 3;
     		sprintf(buf,"You advance a level and gain three points in %s\n\r",skill_table[skillsn].name);
     		send_to_char(buf,ch);
	}
        else
        {
     		ch->pcdata->learned[skill_lookup(argument)] += 2;
     		sprintf(buf,"You advance a level and gain two points in %s\n\r",skill_table[skillsn].name);
     		send_to_char(buf,ch);

        }

     }
     if ((get_level(ch) % 25) == 0)
     {
	if ((ch->pcdata->learned[gsn_speardancer]> 50 && ch->pcdata->learned[gsn_speardancer] < 94) || ch->pcdata->learned[gsn_speardancer] >= 95)
        {
		ch->pcdata->learned[gsn_speardancer]++;
		send_to_char("You also gain a point in speardancer.\r\n",ch);
        }
	if ((ch->pcdata->learned[gsn_blademaster] > 50 && ch->pcdata->learned[gsn_blademaster] < 94) || ch->pcdata->learned[gsn_blademaster] >= 95)
        {
		ch->pcdata->learned[gsn_blademaster]++;
		send_to_char("You also gain a point in blademaster.\r\n",ch);
        }
	if ((ch->pcdata->learned[gsn_duelling] > 50 && ch->pcdata->learned[gsn_duelling] < 94) || ch->pcdata->learned[gsn_duelling] >= 95)
        {
		ch->pcdata->learned[gsn_duelling]++;
		send_to_char("You also gain a point in duelling.\r\n",ch);
        }
	if ((ch->pcdata->learned[gsn_axemaster] > 50 && ch->pcdata->learned[gsn_axemaster] < 94) || ch->pcdata->learned[gsn_axemaster] >= 95)
        {
		ch->pcdata->learned[gsn_axemaster]++;
		send_to_char("You also gain a point in axemaster.\r\n",ch);
        }
	if ((ch->pcdata->learned[gsn_flailmaster] > 50 && ch->pcdata->learned[gsn_flailmaster] < 94) || ch->pcdata->learned[gsn_flailmaster] >= 95)
        {
		ch->pcdata->learned[gsn_flailmaster]++;
		send_to_char("You also gain a point in flailmaster.\r\n",ch);
        }
	if ((ch->pcdata->learned[gsn_whipmaster] > 50 && ch->pcdata->learned[gsn_whipmaster] < 94) || ch->pcdata->learned[gsn_whipmaster] >= 95)
        {
		ch->pcdata->learned[gsn_whipmaster]++;
		send_to_char("You also gain a point in whipmaster.\r\n",ch);
        }
	if ((ch->pcdata->learned[gsn_staffmaster] > 50 && ch->pcdata->learned[gsn_staffmaster] < 94) || ch->pcdata->learned[gsn_staffmaster] >= 95)
        {
		ch->pcdata->learned[gsn_staffmaster]++;
		send_to_char("You also gain a point in staffmaster.\r\n",ch);
        }
	if ((ch->pcdata->learned[gsn_martialarts] > 50 && ch->pcdata->learned[gsn_martialarts] < 94) || ch->pcdata->learned[gsn_martialarts] >= 95)
        {
		ch->pcdata->learned[gsn_martialarts]++;
		send_to_char("You also gain a point in martialarts.\r\n",ch);
        }
     }


  }

  OBJ_DATA *obj;
  OBJ_DATA *obj_next;

  for ( obj = ch->carrying; obj != NULL; obj = obj_next ) {
       obj_next = obj->next_content;
       if ( obj->wear_loc != WEAR_NONE &&
               obj->wear_loc != WEAR_SCABBARD_1 &&
               obj->wear_loc != WEAR_SCABBARD_2 &&
               obj->pIndexData->vnum != OBJ_VNUM_STONE && 
               can_see_obj( ch, obj ) ) 
       {
	       if ((ch->level > obj->level + 10) && obj->wear_loc != WEAR_LIGHT && (ch->level > LEVEL_HERO -1))
               {
		  obj->condition -= 10;
		  if (obj->condition <= 0)
 		  {
			obj->condition = 0; 
	   		SET_BIT(obj->extra_flags, ITEM_BROKEN);
        		act ( "Your $p is old and worn and can no longer be used!", ch, obj, NULL, TO_CHAR);
	   		unequip_char(ch, obj);
		  }
	       }
       }
  }

  ch->exp = ch->exp - exp_next_level(ch);

  if (ch->level >= LEVEL_HERO-1)
    ch->pcdata->extended_level += 1;
  else
    ch->level += 1;

  sprintf(buf,"%s gained level %d",ch->name,get_level(ch));
  log_string(buf);
  sprintf(buf,"$N has attained level %d!",get_level(ch));
  wiznet(buf,ch,NULL,WIZ_LEVELS,0,0);

  if (get_level(ch) <= MAX_NORMAL_LEVEL)
  {
     if (ch->level >= LEVEL_HERO-1)
       advance_level(ch, FALSE, TRUE);
     else
       advance_level(ch, FALSE, FALSE);
  }

  save_char_obj(ch, FALSE);
  
  if (ch->exp < exp_next_level(ch)) {
    REMOVE_BIT(ch->act, PLR_CANLEVEL);
  }
  
  return;
}


void gain_exp_OLD( CHAR_DATA *ch, int gain )
{
  char buf[MAX_STRING_LENGTH];
  unsigned long levelExp=0;
  
  if ( IS_NPC(ch) || ch->level >= LEVEL_HERO )
    return;

  /* EXP needed for level : Make function later*/
  levelExp = ((ch->level*ch->level)*3)*100;
  
  ch->exp = ch->exp + gain;
  
  if (ch->level < LEVEL_HERO &&
	 ch->exp >= levelExp) {
    send_to_char( "{WYou gain a l{re{Wv{re{Wl{r!!{x\n\r", ch );
    ch->level += 1;
    sprintf(buf,"%s gained level %d",ch->name,ch->level);
    log_string(buf);
    sprintf(buf,"$N has attained level %d!",ch->level);
    wiznet(buf,ch,NULL,WIZ_LEVELS,0,0);
    advance_level(ch,FALSE, FALSE);
    ch->exp = 0;
    save_char_obj(ch, FALSE);
  }
  return;
}

/*
 * Regeneration stuff.
 */
int hit_gain( CHAR_DATA *ch, bool is_locgain, int location)
{
  int gain;
  int number;
  
  if (ch->in_room == NULL)
    return 0;
  
  if (ch->fighting != NULL || ch->position == POS_FIGHTING)
    return 0;
  
  /**********
   * For NPC's
   *   
   *  sleeping = 10% 
   *  resting  = 7%
   *  standing = 4%
   *  fighting = 1%
   * 
   *    fast healing bonus of skill % / 10 
   ******/
  if ( IS_NPC(ch) ) {
    gain =  5 + ch->level;
    if (IS_AFFECTED(ch,AFF_REGENERATION))
	 gain *= 2;
    
    switch(ch->position) {
    default : 		gain /= 2;			break;
    case POS_SLEEPING: 	gain = 3 * gain/2;		break;
    case POS_RESTING:  					break;
    case POS_FIGHTING:	gain = 0;		 	break;
    }
  }
  else {
    /**********
	* For PC's
	*   
	*  sleeping = 10% 
	*  resting  = 7%
	*  standing = 4%
	*  fighting = 1%
	* 
	*    fast healing bonus of skill % / 10 
	******/
    switch(ch->position) {
    default : 		gain = ch->max_hit * .04;	break;
    case POS_SLEEPING: 	gain = ch->max_hit * .10;	break;
    case POS_RESTING:   gain = ch->max_hit * .07;	break;
    case POS_FIGHTING:	gain = ch->max_hit * .01; 	break;
    }
    
    number = number_percent();
    if (number < get_skill(ch,gsn_fast_healing)) {
	 gain += (number * gain) / 100;
	 if (ch->hit < ch->max_hit)
	   check_improve(ch,gsn_fast_healing,TRUE,8);
    }
    
    if ( ch->pcdata->condition[COND_HUNGER]   == 0 )
	 gain /= 2;
    
    if ( ch->pcdata->condition[COND_THIRST] == 0 )
	 gain /= 2;
  }
  
  gain = gain * (ch->in_room->heal_rate / 100);
  
  if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
    gain = gain * ch->on->value[3] / 100;
  
  if ( IS_AFFECTED(ch, AFF_SUFFOCATING) )
    gain = 0;

  if ( IS_AFFECTED(ch, AFF_POISON) )
    gain /= 4;
  
  if (IS_AFFECTED(ch, AFF_PLAGUE))
    gain /= 8;
  
  if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED(ch,AFF_SLOW))
    gain /=2 ;
  
  if (!IS_NPC(ch))
    gain += 50;
  
  // 15% faster heal as warder
  if (IS_WARDER(ch)) {
    gain = gain * 1.15;
  }
  
  if (is_locgain)
    return UMIN(gain, get_max_hit_loc(ch, location) - ch->hit_loc[location] );
  else
    return UMIN(gain, ch->max_hit - ch->hit);
}



int endurance_gain( CHAR_DATA *ch )
{
  int gain;
  int number;
  
  if (ch->in_room == NULL)
    return 0;

  if (ch->fighting != NULL || ch->position == POS_FIGHTING)
    return 0;
  
  if ( IS_NPC(ch) ) {
    gain = 5 + ch->level;
    switch (ch->position) {
    default:		gain /= 2;		break;
    case POS_SLEEPING:	gain = 3 * gain/2;	break;
    case POS_RESTING:				break;
    case POS_FIGHTING:	gain /= 3;		break;
    }
  }
  else {
    /**********
	* For PC's
	*   
	*  sleeping = 10% 
	*  resting  = 7%
	*  standing = 4%
	*  fighting = 1%
	* 
	*    fast healing bonus of skill % / 10 
	******/
    switch(ch->position) {
    case POS_SLEEPING: 	gain = ch->max_endurance * .10;	break;
    case POS_RESTING:   gain = ch->max_endurance * .07;	break;
    case POS_FIGHTING:	gain = ch->max_endurance * .01; 	break;
    default : 		gain = ch->max_endurance * .04;	break;
    }
    
    number = number_percent();
    if (number < get_skill(ch,gsn_meditation)) {
	 gain += number * gain / 100;
	 if (ch->endurance < ch->max_endurance)
	   check_improve(ch,gsn_meditation,TRUE,8);
    }
    
    if ( ch->pcdata->condition[COND_HUNGER]   == 0 )
	 gain /= 2;
    
    if ( ch->pcdata->condition[COND_THIRST] == 0 )
	 gain /= 2;
    
  }
  
  gain = gain * ch->in_room->endurance_rate / 100;
  
  if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
    gain = gain * ch->on->value[4] / 100;
  
  if ( IS_AFFECTED( ch, AFF_POISON ) )
    gain /= 4;
  
  if (IS_AFFECTED(ch, AFF_PLAGUE))
    gain /= 8;
  
  if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED(ch,AFF_SLOW))
    gain /= 2;
  
  if (!IS_NPC(ch)) 
    gain += 50;

  // 15% faster heal as warder
  if (IS_WARDER(ch)) {
    gain = gain * 1.15;
  }
  
  return UMIN(gain, ch->max_endurance - ch->endurance);
}



int move_gain( CHAR_DATA *ch )
{
  int gain;

    if (ch->in_room == NULL)
	return 0;

    if ( IS_NPC(ch) )
    {
	gain = ch->level;
    }
    else
    {
	gain = UMAX( 15, ch->level );

	switch ( ch->position )
	{
	case POS_SLEEPING: gain += get_curr_stat(ch,STAT_DEX);		break;
	case POS_RESTING:  gain += get_curr_stat(ch,STAT_DEX) / 2;	break;
	}

	if ( ch->pcdata->condition[COND_HUNGER]   == 0 )
	    gain /= 2;

	if ( ch->pcdata->condition[COND_THIRST] == 0 )
	    gain /= 2;
    }

    gain = gain * ch->in_room->heal_rate/100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
	gain = gain * ch->on->value[3] / 100;

    if ( IS_AFFECTED(ch, AFF_POISON) )
	gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED(ch,AFF_SLOW))
        gain /=2 ;

    return UMIN(gain, ch->max_endurance - ch->endurance);
}



void gain_condition( CHAR_DATA *ch, int iCond, int value )
{
  int condition;
  
  if ( value == 0 || IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL)
    return;
  
  condition = ch->pcdata->condition[iCond];

  if (condition == -1)
    return;
  
  ch->pcdata->condition[iCond] = URANGE( 0, condition + value, 48 );
  
  if ( ch->pcdata->condition[iCond] == 0 ) {
    switch ( iCond ) {
//    case COND_HUNGER:
//	 send_to_char( "You are hungry.\n\r",  ch );
//	 break;
	 
 //   case COND_THIRST:
//	 send_to_char( "You are thirsty.\n\r", ch );
//	 break;
	 
    case COND_DRUNK:
	 if ( condition != 0 )
	   send_to_char( "You are sober.\n\r", ch );
	 break;
    }
  }
  return;
}

/*
 * Loop through all areas and check if there is a
 * weave in the room and update it
 */
void room_weave_update( void )
{
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  AFFECT_DATA *pWeave;
  AFFECT_DATA *pWeave_next;
  unsigned int vnum;
  char buf [MAX_STRING_LENGTH];
  time_t st_time;
  time_t en_time;
  unsigned int cnt=0;
  unsigned int rcnt=0;

  st_time = time(NULL);
  
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->weave_cnt <= 0) {       
	 continue;
    }
    else {
	 sprintf(buf, "Area <%s> has <%d> weaves set in it", pArea->name, pArea->weave_cnt);
	 wiznet(buf,NULL,NULL,WIZ_TICKS,0,0);
    }
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {
	   
	   cnt++;
	   
	   /* Update weaves */
	   for (pWeave = pRoom->weaves; pWeave != NULL; pWeave = pWeave_next) {
		pWeave_next = pWeave->next;
		rcnt++;
		
		if (pWeave->duration != SUSTAIN_WEAVE) {
		  if (pWeave->duration > 0)
		    pWeave->duration--;
		  else
		    room_weave_remove(pRoom, pWeave);	   
		}
	   }
	 }
    }
  }
  en_time = time(NULL);
  
  sprintf(buf, "Weave update checked <%d> rooms found <%d> weaves - used time=%ldsec",cnt, rcnt, en_time - st_time);
  wiznet(buf,NULL,NULL,WIZ_TICKS,0,0);
}

/*
 * Loop through all areas and check if there is a
 * ward in the room and update it
 */
void ward_update( void )
{
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  WARD_DATA *pWard;
  WARD_DATA *pWard_next;
  unsigned int vnum;
  char buf [MAX_STRING_LENGTH];
  time_t st_time;
  time_t en_time;
  unsigned int cnt=0;
  unsigned int rcnt=0;

  st_time = time(NULL);

  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    if (pArea->wards_cnt <= 0) {       
       continue;
    }
    else {
    	sprintf(buf, "Area <%s> has <%d> wards set in it", pArea->name, pArea->wards_cnt);
    	wiznet(buf,NULL,NULL,WIZ_TICKS,0,0);
    }
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {

	   cnt++;
	   
	   /* Update wards */
	   for (pWard = pRoom->wards; pWard != NULL; pWard = pWard_next) {
		pWard_next = pWard->next;
		rcnt++;

		if (pWard->duration != SUSTAIN_WEAVE) {
		  if (pWard->duration > 0)
		    pWard->duration--;
		  else
		    ward_remove(pRoom, pWard);	   
		}
	   }
	 }
    }
  }
  en_time = time(NULL);
  
  sprintf(buf, "Ward update checked <%d> rooms found <%d> wards - used time=%ldsec",cnt, rcnt, en_time - st_time);
  wiznet(buf,NULL,NULL,WIZ_TICKS,0,0);
}

/*
 * Loop through all areas and check if there is a
 * residue in the room and update it
 */
void residue_update( void )
{
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  RESIDUE_DATA *pResidue;
  RESIDUE_DATA *pResidue_next;
  unsigned int vnum;
  char buf [MAX_STRING_LENGTH];
  time_t st_time;
  time_t en_time;
  unsigned int cnt=0;
  unsigned int rcnt=0;

  st_time = time(NULL);

  for ( pArea = area_first; pArea != NULL; pArea = pArea->next ) {
    for ( vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++ ) {
	 if ( ( pRoom = get_room_index(vnum) ) ) {

	   cnt++;
	   
	   /* Update residues */
	   for (pResidue = pRoom->residues; pResidue != NULL; pResidue = pResidue_next) {
		pResidue_next = pResidue->next;
		rcnt++;
		
		if (pResidue->duration > 0)
		  pResidue->duration--;
		else
		  residue_remove(pRoom, pResidue);	   
	   }
	 }
    }
  }
  en_time = time(NULL);
  
  sprintf(buf, "Residue update checked <%d> rooms found <%d> residues - used time=%ldsec",cnt, rcnt, en_time - st_time);
  wiznet(buf,NULL,NULL,WIZ_TICKS,0,0);
}

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update( void )
{
  CHAR_DATA *ch;
  CHAR_DATA *ch_next;
  EXIT_DATA *pexit;
  int door;
  
  /* Examine all mobs. */
  for ( ch = char_list; ch != NULL; ch = ch_next ) {
    ch_next = ch->next;
    
    if ( !IS_NPC(ch) || ch->in_room == NULL || IS_AFFECTED(ch,AFF_CHARM))
	 continue;

    if (ch->in_room->area->empty && !IS_SET(ch->act,ACT_UPDATE_ALWAYS))
	 continue;

    /* Examine call for special procedure */
    if ( ch->spec_fun != 0 ) {
	 if ( (*ch->spec_fun) ( ch ) )
	   continue;
    }

    if (ch->pIndexData->pShop != NULL) /* give him some gold */
	 if ((ch->gold * 100 + ch->silver) < ch->pIndexData->wealth) {
	   ch->gold += ch->pIndexData->wealth * number_range(1,20)/5000000;
	   ch->silver += ch->pIndexData->wealth * number_range(1,20)/50000;
	 }
	 
    /*
	* Check triggers only if mobile still in default position
	*/
    if ( ch->position == ch->pIndexData->default_pos ) {
	 /* Delay */
	 if ( HAS_TRIGGER( ch, TRIG_DELAY) 
		 &&   ch->mprog_delay > 0 ) {
	   if ( --ch->mprog_delay <= 0 ) {
		mp_percent_trigger( ch, NULL, NULL, NULL, TRIG_DELAY );
		continue;
	   }
	 } 
	 if ( HAS_TRIGGER( ch, TRIG_RANDOM) ) {
	   if( mp_percent_trigger( ch, NULL, NULL, NULL, TRIG_RANDOM ) )
		continue;
	 }
    }

    /* That's all for sleeping / busy monster, and empty zones */
    if ( ch->position != POS_STANDING )
	 continue;

    /* Scavenge */
    if ( IS_SET(ch->act, ACT_SCAVENGER)
	    &&   ch->in_room->contents != NULL
	    &&   number_bits( 6 ) == 0 ) {
	 OBJ_DATA *obj;
	 OBJ_DATA *obj_best;
	 int max;
	 
	 max         = 1;
	 obj_best    = 0;
	 for ( obj = ch->in_room->contents; obj; obj = obj->next_content ) {
	   if ( CAN_WEAR(obj, ITEM_TAKE) && can_loot(ch, obj)
		   && obj->cost > max  && obj->cost > 0
		   && !(!str_cmp("quest object", obj->name))) {
		obj_best    = obj;
		max         = obj->cost;
	   }
	 }
	 
	 if ( obj_best ) {
	   obj_from_room( obj_best );
	   obj_to_char( obj_best, ch );
	   act( "$n gets $p.", ch, obj_best, NULL, TO_ROOM );
	 }
    }

    if (IS_SET(ch->act, ACT_TRAIN))
    {
	if (IS_SET(ch->train_flags, TRAIN_BLADEMASTER) ||
	    IS_SET(ch->train_flags,TRAIN_DUELLING) ||
	    IS_SET(ch->train_flags,TRAIN_MASTERFORMS) ||
	    IS_SET(ch->train_flags,TRAIN_SPEARDANCER))
        {
    	 ROOM_INDEX_DATA *from_location=NULL;
	 ROOM_INDEX_DATA * to_location = NULL;
 	 from_location = ch->in_room;
 	 CHAR_DATA *bch; 
	 int dir = 0;
  	for( dir = 0; dir <= 5; dir++ ) {
    		if ((pexit = from_location->exit[dir]) != NULL
	   	&& ( to_location = pexit->u1.to_room ) != NULL
	   	&& can_see_room(ch, pexit->u1.to_room)) {
	 
	 		if (IS_SET(pexit->exit_info, EX_BLOCKED)) {
	   		bch = get_blocker(ch->in_room, dir);
  			/* Supplize!!! */
    			act("$N says '{7It's time to practice the forms..{x'",bch,NULL,ch,TO_CHAR);
			char_from_room(ch);
			char_to_room(ch,to_location);
    			one_hit( ch, bch, find_relevant_masterform(ch), FALSE, 0);

	 		}
    		}
	  }
  	}
    }
    /* Wander */
    if ( !IS_SET(ch->act, ACT_SENTINEL) 
	    && number_bits(3) == 0
	    && ( door = number_bits( 5 ) ) <= 5
	    && ( pexit = ch->in_room->exit[door] ) != NULL
	    &&   pexit->u1.to_room != NULL
	    &&   !IS_SET(pexit->exit_info, EX_CLOSED)
	    &&   !IS_SET(pexit->exit_info, EX_HIDDEN)
	    &&   !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
	    &&   pexit->u1.to_room->sector_type != SECT_WATER_SWIM
            &&   pexit->u1.to_room->sector_type != SECT_WATER_NOSWIM
            &&   pexit->u1.to_room->sector_type != SECT_AIR
            &&   pexit->u1.to_room->sector_type != SECT_OCEAN
            &&   pexit->u1.to_room->sector_type != SECT_RIVER
            &&   pexit->u1.to_room->sector_type != SECT_LAKE
	    && ( !IS_SET(ch->act, ACT_STAY_AREA)
	    ||   pexit->u1.to_room->area == ch->in_room->area ) 
	    && ( !IS_SET(ch->act, ACT_OUTDOORS)
	    ||   !IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)) 
	    && ( !IS_SET(ch->act, ACT_INDOORS)
	    ||   IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS))) {
	 move_char( ch, door, FALSE );
    }
  }
  return;
}



/*
 * Update the weather.
 */
void weather_update( void )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int diff;

    buf[0] = '\0';

    switch ( ++time_info.hour )
    {
    case  5:
	weather_info.sunlight = SUN_LIGHT;
	strcat( buf, "The day has begun.\n\r" );
	break;

    case  6:
	weather_info.sunlight = SUN_RISE;
	strcat( buf, "The sun rises in the east.\n\r" );
	break;

    case 19:
	weather_info.sunlight = SUN_SET;
	strcat( buf, "The sun slowly disappears in the west.\n\r" );
	break;

    case 20:
	weather_info.sunlight = SUN_DARK;
	strcat( buf, "The night has begun.\n\r" );
	break;

    case 24:
	time_info.hour = 0;
	time_info.day++;
	break;
    }

    if ( time_info.day   >= 35 )
    {
	time_info.day = 0;
	time_info.month++;
    }

    if ( time_info.month >= 12 )
    {
	time_info.month = 0;
	time_info.year++;
    }

    /*
     * Weather change.
     */
    if ( time_info.month >= 7 && time_info.month <= 11 )
	diff = weather_info.mmhg >  985 ? -2 : 2;
    else
	diff = weather_info.mmhg > 1015 ? -2 : 2;

    if (g_FreezeWeather)
    {
 	weather_info.change = 0;
    }
    else
    {
    	weather_info.change   += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
    	weather_info.change    = UMAX(weather_info.change, -12);
    	weather_info.change    = UMIN(weather_info.change,  12);

        weather_info.mmhg += weather_info.change;
        weather_info.mmhg  = UMAX(weather_info.mmhg,  960);
        weather_info.mmhg  = UMIN(weather_info.mmhg, 1040);
    }

    switch ( weather_info.sky )
    {
    default: 
	bug( "Weather_update: bad sky %d.", weather_info.sky );
	weather_info.sky = SKY_CLOUDLESS;
	break;

    case SKY_CLOUDLESS:
	if ( weather_info.mmhg <  990
	|| ( weather_info.mmhg < 1010 && number_bits( 2 ) == 0 ) )
	{
	    strcat( buf, "The sky is getting cloudy.\n\r" );
	    weather_info.sky = SKY_CLOUDY;
	}
	break;

    case SKY_CLOUDY:
	if ( weather_info.mmhg <  970
	|| ( weather_info.mmhg <  990 && number_bits( 2 ) == 0 ) )
	{
	    strcat( buf, "It starts to rain.\n\r" );
	    weather_info.sky = SKY_RAINING;
	}

	if ( weather_info.mmhg > 1030 && number_bits( 2 ) == 0 )
	{
	    strcat( buf, "The clouds disappear.\n\r" );
	    weather_info.sky = SKY_CLOUDLESS;
	}
	break;

    case SKY_RAINING:
	if ( weather_info.mmhg <  970 && number_bits( 2 ) == 0 )
	{
	    strcat( buf, "Lightning flashes in the sky.\n\r" );
	    weather_info.sky = SKY_LIGHTNING;
	}

	if ( weather_info.mmhg > 1030
	|| ( weather_info.mmhg > 1010 && number_bits( 2 ) == 0 ) )
	{
	    strcat( buf, "The rain stopped.\n\r" );
	    weather_info.sky = SKY_CLOUDY;
	}
	break;

    case SKY_LIGHTNING:
	if ( weather_info.mmhg > 1010
	|| ( weather_info.mmhg >  990 && number_bits( 2 ) == 0 ) )
	{
	    strcat( buf, "The lightning has stopped.\n\r" );
	    weather_info.sky = SKY_RAINING;
	    break;
	}
	break;
    }

    if ( buf[0] != '\0' )
    {
	for ( d = descriptor_list; d != NULL; d = d->next )
	{
	    if ( d->connected == CON_PLAYING
	    /* &&   IS_OUTSIDE(d->character) */
            && !IS_SET(d->character->in_room->room_flags, ROOM_INDOORS)
            && d->character->in_room->sector_type != SECT_INSIDE
	    &&   IS_AWAKE(d->character) )
		send_to_char( buf, d->character );
	}
    }

    return;
}

/*
 * Weather effects.
 */
void weather_effect(void)
{
   DESCRIPTOR_DATA *d;
   AFFECT_DATA     af;

   for ( d = descriptor_list; d != NULL; d = d->next ) {

      if ( d->connected == CON_PLAYING
          && IS_SET(d->character->in_room->room_flags, ROOM_VMAP)
          && IS_AWAKE  ( d->character )
          && !IS_IMMORTAL(d->character)
          && number_chance(1) /* 1% chance */
          && weather_info.sky == SKY_LIGHTNING ) {

/* For now only on VMAP any level */
/*          && IS_OUTSIDE( d->character ) */
/*          && d->character->level > 17 */


         send_to_char("{WA bright flash of light and a crack of thunder are all the "
                      "warning you get as lightning strikes next to you.{x\n\r",d->character);
         act( "Lighting flashes down and strikes at $n.", d->character, NULL, NULL,TO_ROOM );

         if(check_immune(d->character,DAM_LIGHTNING) != IS_IMMUNE) {
            if(d->character->fighting) {
               stop_fighting(d->character,TRUE); 
            }
            if(check_immune(d->character,DAM_LIGHTNING) != IS_RESISTANT) {
               if(d->character->hit > d->character->hit /2) {
                  d->character->hit -= d->character->hit/10; 
               }
               WAIT_STATE(d->character,25); 
               if (!IS_AFFECTED(d->character,AFF_BLIND)) {
                   af.where       = TO_AFFECTS;
			    af.casterId    = d->character->id;
                   af.type        = gsn_blindness;
                   af.level       = d->character->level;
                   af.duration    = 3;
                   af.location    = APPLY_HITROLL;
                   af.modifier    = -4;
                   af.bitvector   = AFF_BLIND;
                   affect_to_char(d->character,&af);
               }
            } 
            else {
               if(d->character->hit > d->character->hit /2) {
                  d->character->hit -= d->character->hit/50;
               }
               WAIT_STATE(d->character,10); 
               if (!IS_AFFECTED(d->character,AFF_BLIND)) {
                   af.where       = TO_AFFECTS;
			    af.casterId    = d->character->id;
                   af.type        = gsn_blindness;
                   af.level       = d->character->level;
                   af.duration    = 2;
                   af.location    = APPLY_HITROLL;
                   af.modifier    = -4;
                   af.bitvector   = AFF_BLIND;
                   affect_to_char(d->character,&af);
               }
            }
            if(check_immune(d->character,DAM_LIGHTNING) == IS_VULNERABLE) {
               d->character->hit -= d->character->hit/2;
               WAIT_STATE(d->character,40);
               if (!IS_AFFECTED(d->character,AFF_BLIND)) {
                   af.where       = TO_AFFECTS;
			    af.casterId     = d->character->id;
                   af.type        = gsn_blindness;
                   af.level       = d->character->level;
                   af.duration    = 4;
                   af.location    = APPLY_HITROLL;
                   af.modifier    = -4;
                   af.bitvector   = AFF_BLIND;
                   affect_to_char(d->character,&af);
               }
            }
         }
      }
   }
}


void sense_human_msg_to_trolloc(CHAR_DATA *ch, CHAR_DATA *victim)
{

  return;

  int direction=-1;
  int distance=0;
  bool found;
  ROOM_INDEX_DATA *was_in_room;
  char buf[MAX_STRING_LENGTH];
  bool fArea=FALSE;

  // same room, stop here
  if (ch->in_room == victim->in_room) 
    return;  
  if ( IS_SET(ch->in_room->room_flags,ROOM_VMAP))
    return;
  
  // Find the general direction to the SS
  direction = find_path( ch->in_room->vnum, victim->in_room->vnum, ch, -40000, fArea );
  
  if (direction != -1 && direction >= 0 && direction <= 9) {
    was_in_room = ch->in_room;
    found = FALSE;
    for( distance = 1; distance <= 4; distance++ ) {
	 EXIT_DATA *pexit;
	 CHAR_DATA *list;
	 CHAR_DATA *rch;
	 
	 if( ( pexit = ch->in_room->exit[direction] ) != NULL
		&& pexit->u1.to_room != NULL
		&& pexit->u1.to_room != was_in_room ) {
	   
	   ch->in_room = pexit->u1.to_room;
	   
	   list = ch->in_room->people;

	   for( rch = list; rch != NULL; rch = rch->next_in_room ) {
		if (rch == victim) {
		  found = TRUE;
		}
	   }
	 }
    }

    ch->in_room = was_in_room;

    if (found) {
	 act("$n sniff the air.", ch, NULL, NULL, TO_ROOM);
	 act("You sniff the air.", ch, NULL, NULL, TO_CHAR);
	 sprintf(buf, "{DYour nose pick up the sent of flesh somewhere %sward!!!{x.\n\r", dir_name[direction] );
	 send_to_char(buf, ch);
	 return;
    }
  }    
  
  return;
}

void trolloc_update( CHAR_DATA *ch )
{
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;

  if (IS_NPC(ch))
    return;
  
  if (ch->race != race_lookup("trolloc"))
    return;

  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if ( vch->in_room == NULL )
	 continue;
    if (ch == vch)
	 continue;
    if (IS_NPC(vch))
	 continue;
    if (!IS_SAME_WORLD(vch, ch))
	 continue;
    if (IS_SHADOWSPAWN(vch))
	 continue;  
    if ( vch->in_room->area == ch->in_room->area ) {

	 sense_human_msg_to_trolloc(ch, vch);
	 
	 ch->pcdata->next_trupdate = current_time+120;

	 return;
    }
  }
}

void ssmsg_to_warder(CHAR_DATA *ch, CHAR_DATA *victim)
{
  int direction=-1;
  int distance=0;
  bool found;
  ROOM_INDEX_DATA *was_in_room;
  char buf[MAX_STRING_LENGTH];
  bool fArea=FALSE;

  // IF same room, stop here
  if (ch->in_room == victim->in_room) {
    send_to_char("{YYou feel the presence of {Dshadowspawn {Yis overwhelming close{x.\n\r", ch);
    return;
  }

  // Find the general direction to the SS
  direction = find_path( ch->in_room->vnum, victim->in_room->vnum, ch, -40000, fArea );
  
  if (direction != -1 && direction >= 0 && direction <= 9) {

    // If distance is close (1 room away - straigth line) same msg as in room, 
    // just to make it less obvious.
    was_in_room = ch->in_room;
    found = FALSE;
    for( distance = 1; distance <= 2; distance++ ) {
	 EXIT_DATA *pexit;
	 CHAR_DATA *list;
	 CHAR_DATA *rch;
	 
	 if( ( pexit = ch->in_room->exit[direction] ) != NULL
		&& pexit->u1.to_room != NULL
		&& pexit->u1.to_room != was_in_room ) {

	   ch->in_room = pexit->u1.to_room;
	   
	   list = ch->in_room->people;

	   for( rch = list; rch != NULL; rch = rch->next_in_room ) {
		if (rch == victim) {
		  found = TRUE;
		}
	   }
	 }
    }

    ch->in_room = was_in_room;

    // Found = 1-2 room away, straight line
    if (found) {
	 send_to_char("{YYou feel the presence of {Dshadowspawn {Yis overwhelming close{x.\n\r", ch);
	 return;
    }
    // More than 2 rooms away, give direction
    else {
	 sprintf(buf, "{YYou feel the presence of {Dshadowspawn {Ysomewhere to the %s{x.\n\r", dir_name[direction] );
	 send_to_char(buf, ch);
	 return;
    
    }
  }    

  // Default if find path return funky value etc.
  else {    
    send_to_char("{YYou feel the presence of {Dshadowspawn {Ynearby{x.\n\r", ch);
    return;
  }
}

/*
 * Update all PCs with Warder merit for shadowspawn sensing
 */
void warder_update( CHAR_DATA *ch )
{

  return;
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  //time_t st_time;
  //time_t en_time;

  //st_time = time(NULL);

  if (IS_NPC(ch))
    return;
  
  if (!IS_WARDER(ch))
    return;

  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next	= vch->next;
    if ( vch->in_room == NULL )
	 continue;
    if (ch == vch)
	 continue;
    if (IS_NPC(vch))
	 continue;
    if (!IS_SAME_WORLD(vch, ch))
	 continue;
    if (!IS_SHADOWSPAWN(vch))
	 continue;
    // Ok, tell about shadowspawn
    if ( vch->in_room->area == ch->in_room->area ) {
	 
	 //printf("Shadowspawn found: %s in room: %d\n", vch->name, vch->in_room->vnum);
	 
          if ( IS_SET(ch->in_room->room_flags,ROOM_VMAP))
             return;
	 ssmsg_to_warder(ch, vch);

	 //send_to_char("{YYou feel the presence of {Dshadowspawn {Ynearby{x.\n\r", ch);
	 ch->pcdata->next_warderupdate = current_time+180; // Every 3 minute
	 
	 //en_time = time(NULL);
	 //printf("warder update - time=%ldsec\n", en_time - st_time);
	 return;
    }
  }

  //en_time = time(NULL);
  //printf("warder update - !time=%ldsec\n", en_time - st_time);
  return;
}

/*
 * Update all chars, including mobs.
 */
void char_update( void )
{   
  CHAR_DATA *ch;
  CHAR_DATA *ch_next;
  CHAR_DATA *ch_quit;
  int hit_add=0;
  
  ch_quit	= NULL;
  
  /* update save counter */
  save_number++;
  
  if (save_number > 29)
    save_number = 0;
  
  for ( ch = char_list; ch != NULL; ch = ch_next ) {
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;
    
    ch_next = ch->next;
    
    // if ( ch->timer > 30 )
    if ( (ch->timer > 30) && (!IS_IMMORTAL(ch)))
      ch_quit = ch;
    
    /* Immortals goes idle, but never log or void */
    if (IS_IMMORTAL(ch))
      ch->timer++;
    
    if ( ch->position >= POS_DEAD ) {
	 /* check to see if we need to go home */
	 if (IS_NPC(ch) && ch->zone != NULL && ch->zone != ch->in_room->area
		&& ch->desc == NULL &&  ch->fighting == NULL 
		&& !IS_AFFECTED(ch,AFF_CHARM) && number_percent() < 5) {
	   act("$n wanders on home.",ch,NULL,NULL,TO_ROOM);
	   extract_char(ch,TRUE,FALSE);
	   continue;
	 }
	 
	 if ( ch->hit  < ch->max_hit ) {
	   hit_add = hit_gain(ch, FALSE, -1);
	   ch->hit += hit_add;
	}
	 else
	   ch->hit = ch->max_hit;
	 
	 /* Hit location updates */ 
	if (!IS_NPC(ch) && ch->fighting == NULL) {
	 if (ch->hit_loc[LOC_LA] < get_max_hit_loc(ch, LOC_LA))
	   ch->hit_loc[LOC_LA] += UMAX(1, hit_gain(ch, TRUE, LOC_LA)/number_range(2, LOC_MOD_LA));
	 else
	   ch->hit_loc[LOC_LA] = get_max_hit_loc(ch, LOC_LA);
	 
	 if (ch->hit_loc[LOC_LL] < get_max_hit_loc(ch, LOC_LL))
	   ch->hit_loc[LOC_LL] += UMAX(1, hit_gain(ch, TRUE, LOC_LL)/number_range(2, LOC_MOD_LL));
	 else
	   ch->hit_loc[LOC_LL] = get_max_hit_loc(ch, LOC_LL);
	 
	 if (ch->hit_loc[LOC_HE] < get_max_hit_loc(ch, LOC_HE))
	   ch->hit_loc[LOC_HE] += UMAX(1, hit_gain(ch, TRUE, LOC_HE)/number_range(2, LOC_MOD_HE));
	 else
	   ch->hit_loc[LOC_HE] = get_max_hit_loc(ch, LOC_HE);
	 
	 if (ch->hit_loc[LOC_BD] < get_max_hit_loc(ch, LOC_BD))
	   ch->hit_loc[LOC_BD] += UMAX(1, hit_gain(ch, TRUE, LOC_BD)/number_range(2, LOC_MOD_BD));
	 else
	   ch->hit_loc[LOC_BD] = get_max_hit_loc(ch, LOC_BD);
	 
	 if (ch->hit_loc[LOC_RL] < get_max_hit_loc(ch, LOC_RL))
	   ch->hit_loc[LOC_RL] += UMAX(1, hit_gain(ch, TRUE, LOC_RL)/number_range(2, LOC_MOD_RL));
	 else
	   ch->hit_loc[LOC_RL] = get_max_hit_loc(ch, LOC_RL);
	 
	 if (ch->hit_loc[LOC_RA] < get_max_hit_loc(ch, LOC_RA))
	   ch->hit_loc[LOC_RA] += UMAX(1, hit_gain(ch, TRUE, LOC_RA)/number_range(2, LOC_MOD_RA));
	 else
	   ch->hit_loc[LOC_RA] = get_max_hit_loc(ch, LOC_RA);
	}        
	 
	 if ( ch->endurance < ch->max_endurance )
	   ch->endurance += endurance_gain(ch);
	 else
	   ch->endurance = ch->max_endurance;
	 
/*
	    if ( ch->move < ch->max_move )
		ch->move += move_gain(ch);
	    else
		ch->move = ch->max_move;
*/
    }
    
    if (MOUNTED(ch)) 
	 REMOVE_BIT(ch->affected_by,(C|D|P|Q));
    
    if ( ch->position == POS_STUNNED )
	 update_pos( ch );

    if (IS_NPC(ch) && IS_SET(ch->act,ACT_ROAMERTRAINER) 
	&& ch->next_trainmove > 0 && ch->next_trainmove < current_time)
    {
  	 ROOM_INDEX_DATA *room;
	 room = get_random_room(ch, FALSE);
	 if (room != NULL) {
	   act ("$n says, \"I have stayed here as long as I can.  It is time for me to move on.\"",ch,NULL,NULL,TO_ROOM); 
	   char_from_room(ch);
	   char_to_room(ch, room);
	   ch->next_trainmove = 0;
	 }
    }
    
    if ( !IS_NPC(ch) && ch->level < LEVEL_IMMORTAL ) {
	 OBJ_DATA *obj;
	 
	 if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
		 &&   obj->item_type == ITEM_LIGHT
		 &&   obj->value[2] > 0 ) {
	   if ( --obj->value[2] == 0 && ch->in_room != NULL ) {
		--ch->in_room->light;
		act( "$p goes out.", ch, obj, NULL, TO_ROOM );
		act( "$p flickers and goes out.", ch, obj, NULL, TO_CHAR );
		extract_obj( obj );
	   }
	   else if ( obj->value[2] <= 5 && ch->in_room != NULL)
		act("$p flickers.",ch,obj,NULL,TO_CHAR);
	 }
	 
/*	    if (IS_IMMORTAL(ch)) */
/*		ch->timer = 0;        */

	 if ( ++ch->timer >= 12 ) {
	   if ( ch->was_in_room == NULL && ch->in_room != NULL ) {
		ch->was_in_room = ch->in_room;
		if ( ch->fighting != NULL )
		  stop_fighting( ch, TRUE );
    		if (!IS_AFFECTED(ch, AFF_INVISIBLE))
                {
		   act( "$n disappears into the void.",
			ch, NULL, NULL, TO_ROOM );
		}
		
  		if (!IS_SET(ch->comm,COMM_AFK)) {
		   send_to_char( "You disappear into the void.\n\r", ch );
		}
      
		if (ch->level > 1)
		  save_char_obj( ch, FALSE );
		char_from_room( ch );
		char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
	   }
	 }
	 
	 // Warders can go a little longer
	 if (IS_WARDER(ch)) {
	   if (number_chance(50)) {
		gain_condition( ch, COND_DRUNK,  -1 );
		gain_condition( ch, COND_FULL, ch->size > SIZE_MEDIUM ? -4 : -2 );
		gain_condition( ch, COND_THIRST, -1 );
		gain_condition( ch, COND_HUNGER, ch->size > SIZE_MEDIUM ? -2 : -1);
	   }
	 }
	 else {
	   gain_condition( ch, COND_DRUNK,  -1 );
	   gain_condition( ch, COND_FULL, ch->size > SIZE_MEDIUM ? -4 : -2 );
	   gain_condition( ch, COND_THIRST, -1 );
	   gain_condition( ch, COND_HUNGER, ch->size > SIZE_MEDIUM ? -2 : -1);
	 }
    }
    
    for ( paf = ch->affected; paf != NULL; paf = paf_next ) {
	 paf_next	= paf->next;
	 if ( paf->duration > 0 ) {
	   paf->duration--;
	   if (number_range(0,4) == 0 && paf->level > 0)
		paf->level--;  /* spell strength fades with time */
	 }
	 else if ( paf->duration < 0 )
	   ;
	 else {
	   if ( paf_next == NULL
		   ||   paf_next->type != paf->type
		   ||   paf_next->duration > 0 ) {
		if ( paf->type > 0 && skill_table[paf->type].msg_off ) {
		  send_to_char( skill_table[paf->type].msg_off, ch );
		  send_to_char( "\n\r", ch );
		}
	   }
	   if (paf->type == gsn_bof) {
		OBJ_DATA *bof;
		bof = get_eq_char(ch, WEAR_WIELD);
		extract_obj( bof );
	   }
	   if (paf->type == gsn_wof) {
		extract_wof(ch);
	   }
	   if (paf->type == gsn_woa) {
		extract_woa(ch);
	   }

	   affect_remove( ch, paf );
	 }
    }
    
    /* Update holding timers */
    if (!IS_NPC(ch) && IS_CHANNELING(ch)) 
	 update_holding(ch); 
    
    /* Update endur cost if sustaining weaves */
    if (IS_CHANNELING(ch))
	 update_sustained_weaves(ch);
    
    if (!IS_NPC(ch) && ch->study)
	 update_study(ch);

    if (ch->skim)
	 update_skimming(ch);


    /* Increment the RP Counter for those in RP */
    if (IS_RP(ch))
    {
	update_rpcounter(ch);
    }

    /* Update warder's: Shadow spawn sensing */
    if (!IS_NPC(ch) && IS_WARDER(ch)) {
	 if(ch->pcdata->next_warderupdate <= current_time) {
	   warder_update(ch);
	 }
    }

    // Trolloc update
    if (!IS_NPC(ch) && ch->race == race_lookup("trolloc")) {
	 if (ch->pcdata->next_trupdate <= current_time)
	   trolloc_update(ch);
    }

    // Masquerade check update
    if (IS_MASQUERADED(ch) && !can_use_masquerade(ch)) {
	 send_to_char("{mYou have left the masquerade area and remove the mask from your face{x.\n\r", ch);
	 REMOVE_BIT(ch->app,APP_MASQUERADE);
    }
	 

    
    /*
	* Careful with the damages here,
	*   MUST NOT refer to ch after damage taken,
	*   as it may be lethal damage (on NPC).
	*/
    
    if (is_affected(ch, gsn_plague) && ch != NULL) {
	 AFFECT_DATA *af, plague;
	 CHAR_DATA *vch;
	 int dam;
	 
	 if (ch->in_room == NULL)
	   continue;
	 
	 act("$n writhes in agony as plague sores erupt from $s skin.",
		ch,NULL,NULL,TO_ROOM);
	 send_to_char("You writhe in agony from the plague.\n\r",ch);
	 for ( af = ch->affected; af != NULL; af = af->next )
	   {
		if (af->type == gsn_plague)
		  break;
	   }
	 
	 if (af == NULL)
	   {
		REMOVE_BIT(ch->affected_by,AFF_PLAGUE);
		continue;
	   }
	 
	 if (af->level == 1)
	   continue;
	 
	 plague.where		= TO_AFFECTS;
	 plague.type 		= gsn_plague;
	 plague.level 		= af->level - 1; 
	 plague.duration 	= number_range(1,2 * plague.level);
	 plague.location		= APPLY_STR;
	 plague.modifier 	= -5;
	 plague.bitvector 	= AFF_PLAGUE;
	 
	 for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	   {
		if (!saves_spell(plague.level - 2,vch,DAM_DISEASE) 
		    &&  !IS_IMMORTAL(vch)
		    &&  !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(4) == 0)
		  {
		    send_to_char("You feel hot and feverish.\n\r",vch);
		    act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM);
		    affect_join(vch,&plague);
		  }
	   }
	 
	 dam = UMIN(ch->level,af->level/5+1);
	 ch->endurance -= dam;
	 damage( ch, ch, dam, gsn_plague,DAM_DISEASE,FALSE);
    }
    else if ( IS_AFFECTED(ch, AFF_POISON) && ch != NULL
		    &&   !IS_AFFECTED(ch,AFF_SLOW))
	 
	 {
	   AFFECT_DATA *poison;
	   
	   poison = affect_find(ch->affected,gsn_poison);
	   
	   if (poison != NULL)
		{
		  act( "$n shivers and suffers.", ch, NULL, NULL, TO_ROOM );
		  send_to_char( "{GYou shiver and suffer.{x\n\r", ch );
		  damage(ch,ch,poison->level/10 + 1,gsn_poison,
			    DAM_POISON,FALSE);
		}
	 }
    
    else if ( ch->position == POS_INCAP && number_range(0,1) == 0)
	 {
	   damage( ch, ch, 1, TYPE_UNDEFINED, DAM_NONE,FALSE);
	 }
    else if ( ch->position == POS_MORTAL )
	 {
	   damage( ch, ch, 1, TYPE_UNDEFINED, DAM_NONE,FALSE);
	 }
  }
  
  /*
   * Autosave and autoquit.
   * Check that these chars still exist.
   */
  for ( ch = char_list; ch != NULL; ch = ch_next )
    {
	 ch_next = ch->next;
	 
	 if (IS_NPC(ch))
	    continue;

	 if (!IS_VALID(ch)) {
	   bug("char_update: Trying to work with an invalidated character.", 0);
	   break;
	 }
	 
	 if (ch->desc != NULL && ch->desc->descriptor % 30 == save_number)
	   {
		save_char_obj(ch, FALSE);
	   }
	 
	 //if (ch == ch_quit)
	 if ((ch->desc == NULL && !IS_IMMORTAL(ch)) || (ch == ch_quit))
	   {
	   	if (ch->position == POS_FIGHTING)
	   	   stop_fighting(ch, TRUE);
		do_function(ch, &do_quit, "" );
	   }
    }
  
  return;
}




/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update( void )
{   
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf, *paf_next;
	for ( obj = object_list; obj != NULL; obj = obj_next )
    {
	CHAR_DATA *rch;
	char *message;

	obj_next = obj->next;

	/* go through affects and decrement */
        for ( paf = obj->affected; paf != NULL; paf = paf_next )
        {
            paf_next    = paf->next;
            if ( paf->duration > 0 )
            {
                paf->duration--;
                if (number_range(0,4) == 0 && paf->level > 0)
                  paf->level--;  /* spell strength fades with time */
            }
            else if ( paf->duration < 0 )
                ;
            else
            {
                if ( paf_next == NULL
                ||   paf_next->type != paf->type
                ||   paf_next->duration > 0 )
                {

                    if ( paf->type > 0 && skill_table[paf->type].msg_obj )
                    {
			if (obj->carried_by != NULL)
			{
			    rch = obj->carried_by;
				act(skill_table[paf->type].msg_obj,
				rch,obj,NULL,TO_CHAR);
			
			}
			if (obj->in_room != NULL 
			&& obj->in_room->people != NULL)
			{
			    rch = obj->in_room->people;
			    act(skill_table[paf->type].msg_obj,
				rch,obj,NULL,TO_ALL);
			}
                    }
                }

                affect_remove_obj( obj, paf );
            }
        }


   if (obj->item_type == ITEM_ANGREAL && obj->wear_loc >= 0)
   {
   	    if (obj->value[2] > 0 && obj->value[2] < 990)
   		   obj->value[2]--;
   		if (obj->value[2] == 0)
   			obj->timer = 1;
   }

	if ( obj->timer <= 0 || --obj->timer > 0 )
	    continue;

	switch ( obj->item_type )
	{
	default:              message = "$p crumbles into dust.";  break;
	case ITEM_FOUNTAIN:   message = "$p dries up.";         break;
	case ITEM_CORPSE_NPC: message = "$p decays into dust."; break;
	case ITEM_CORPSE_PC:  message = "$p decays into dust."; break;
	case ITEM_FOOD:       message = "$p decomposes.";	break;
	case ITEM_POTION:     message = "$p has evaporated from disuse.";	
								break;
	case ITEM_PORTAL:     message = "$p winks out of existence."; break;
	case ITEM_CONTAINER: 
	    if (CAN_WEAR(obj,ITEM_WEAR_FLOAT))
		if (obj->contains)
		    message = 
		"$p flickers and vanishes, spilling its contents on the floor.";
		else
		    message = "$p flickers and vanishes.";
	    else
		message = "$p crumbles into dust.";
	    break;
	}

	if ( obj->carried_by != NULL )
	{
	    if (IS_NPC(obj->carried_by) 
	    &&  obj->carried_by->pIndexData->pShop != NULL)
		obj->carried_by->silver += obj->cost/5;
	    else
	    {
	    	act( message, obj->carried_by, obj, NULL, TO_CHAR );
		if ( obj->wear_loc == WEAR_FLOAT)
		    act(message,obj->carried_by,obj,NULL,TO_ROOM);
	    }
	}
	else if ( obj->in_room != NULL
	&&      ( rch = obj->in_room->people ) != NULL )
	{
	    if (! (obj->in_obj && obj->in_obj->pIndexData->vnum == OBJ_VNUM_PIT
	           && !CAN_WEAR(obj->in_obj,ITEM_TAKE)))
	    {
	    	act( message, rch, obj, NULL, TO_ROOM );
	    	act( message, rch, obj, NULL, TO_CHAR );
	    }
	}

        if ((obj->item_type == ITEM_CORPSE_PC || obj->wear_loc == WEAR_FLOAT)
	&&  obj->contains)
	{   /* save the contents */
     	    OBJ_DATA *t_obj, *next_obj;

	    for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj)
	    {
		next_obj = t_obj->next_content;
		obj_from_obj(t_obj);

		if (obj->in_obj) /* in another object */
		    obj_to_obj(t_obj,obj->in_obj);

		else if (obj->carried_by)  /* carried */
		    if (obj->wear_loc == WEAR_FLOAT)
			if (obj->carried_by->in_room == NULL)
			    extract_obj(t_obj);
			else
			    obj_to_room(t_obj,obj->carried_by->in_room);
		    else
		    	obj_to_char(t_obj,obj->carried_by);

		else if (obj->in_room == NULL)  /* destroy it */
		    extract_obj(t_obj);

		else /* to a room */
		    obj_to_room(t_obj,obj->in_room);
	    }
	}

	extract_obj( obj );
    }

    return;
}



/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update( void )
{
  CHAR_DATA *wch;
  CHAR_DATA *wch_next;
  CHAR_DATA *ch;
  CHAR_DATA *ch_next;
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  CHAR_DATA *victim;
  int percent_chance; 
  //int chance; 

  for ( wch = char_list; wch != NULL; wch = wch_next ) {
    wch_next = wch->next;
    if ( IS_NPC(wch)
	    ||   wch->level >= LEVEL_IMMORTAL
	    ||   wch->in_room == NULL 
	    ||   wch->in_room->area->empty)
	 continue;
    
    for ( ch = wch->in_room->people; ch != NULL; ch = ch_next ) {
	 int count;
	 
	 ch_next	= ch->next_in_room;

	 if ( !IS_NPC(ch)
		 ||   (!IS_SET(ch->act, ACT_AGGRESSIVE) && !IS_SET(ch->guild_guard_flags, GGUARD_AGGRESSIVE))
		 ||   IS_SET(ch->in_room->room_flags,ROOM_SAFE)
		 ||   IS_AFFECTED(ch,AFF_CALM)
		 ||   ch->fighting != NULL
		 ||   ch->race == wch->race   //make mobs allow people of same race through
		 ||   IS_AFFECTED(ch, AFF_CHARM)
		 ||   !IS_AWAKE(ch)
		 ||   ( IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch) )
		 ||   !can_see( ch, wch ) 
		 ||   number_bits(1) == 0)
	   continue;
	 
	     
												  
	 /*
	  * Ok we have a 'wch' player character and a 'ch' npc aggressor.
	  * Now make the aggressor fight a RANDOM pc victim in the room,
	  *   giving each 'vch' an equal chance of selection.
	  */
	 count	= 0;
	 victim	= NULL;
	 percent_chance = number_percent();
	 for ( vch = wch->in_room->people; vch != NULL; vch = vch_next ) {
	   vch_next = vch->next_in_room;

	   // Guild guard?
	   if (!IS_NPC(vch) && IS_RP(vch) &&
		  vch->level < LEVEL_IMMORTAL &&
		  (ch->guild_guard != vch->clan && IS_SET(ch->guild_guard_flags, GGUARD_AGGRESSIVE))) {
		victim = vch;
		count++;

		if ( victim == NULL )
		  continue;

		if (IS_SET(ch->guild_guard_flags, GGUARD_SHADOWSPAWN) && !IS_SHADOWSPAWN(victim))
		  continue;
		
		if (IS_SET(ch->guild_guard_flags, GGUARD_RACE) && ch->race == victim->race)
		  continue;				
		
		if (((victim->pcdata->last_subdue + 480) < current_time) || victim->fighting != NULL) {
		  act("$n scream to $N, '{7You don't belong here! Alarm! ALARM!{x'", ch, NULL, vch, TO_NOTVICT);
		  act("$n scream to you, '{7You don't belong here! Alarm! ALARM!{x'", ch, NULL, vch, TO_VICT);

		  // If set up to report to guild
		  if (IS_SET(ch->guild_guard_flags, GGUARD_REPORT_GUILD)) {
		    char buf[MSL];
		    REMOVE_BIT(ch->comm,COMM_NOCLAN);
		    REMOVE_BIT(ch->comm,COMM_NOCHANNELS);
		    ch->clan = ch->guild_guard;
		    free_string( ch->gtitle );
		    ch->gtitle = str_dup("Guild Guard");
		    if (IS_SET(ch->guild_guard_flags, GGUARD_SHADOWSPAWN) && IS_SHADOWSPAWN(victim))
			 sprintf(buf, "Shadowspawn Alarm! Shadowspawn Alarm! Alarm at %s!", ch->in_room->area->name);
		    else if (IS_SET(ch->guild_guard_flags, GGUARD_RACE) && ch->race != victim->race)
			 sprintf(buf, "%s Alarm! Alarm at %s!", capitalize(race_table[victim->race].name), ch->in_room->area->name);
		    else		    
			 sprintf(buf, "Alarm! Alarm! Alarm! Alarm at %s!", ch->in_room->area->name);
		    do_guildtalk(ch, buf);
		    ch->clan = 0;
		  }

		  multi_hit( ch, victim, TYPE_UNDEFINED );		
		}
		
		continue;
	   }
	   else {
		if ( !IS_NPC(vch)
			&&   vch->level < LEVEL_IMMORTAL
			&&   ch->level >= vch->level - 5 
			&&   ( !IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch) )
			&&   can_see( ch, vch )) {
		  
		  //			chance = get_skill(ch,gsn_alertness) + get_skill(ch,gsn_observation);
		  //chance = chance / 2;
		  //			if ((chance - (number_percent()/5))
		  //			     > (get_skill(vch,gsn_sneak) + (number_percent()/5)))
		  //			{
		  //		           if ( number_range( 0, count ) == 0 )
		  victim = vch;
		  count++;
		  //			}
		}
	   }
	 }
	 
	 if ( victim == NULL )
	   continue;

	 //printf("### Agro on <%s> sub_time <%ld> current <%ld>\n", victim->name, victim->pcdata->last_subdue+60, current_time);
	 if (((victim->pcdata->last_subdue + 480) < current_time) || victim->fighting != NULL) {
	   if (IS_SET(ch->act, ACT_AGGRESSIVE)) {
		do_function(ch, &do_emote, "screams and attacks!");
	   multi_hit( ch, victim, TYPE_UNDEFINED );
	   }
	 }
    }
  }
  
  return;
}

void sleep_update(void)
{
  SLEEP_DATA *temp = first_sleep, *temp_next;
  
  for( ; temp != NULL; temp = temp_next) {
    bool delete = FALSE;
    
    temp_next = temp->next;
    
    /* checks to make sure the mob still exists*/
    if(!temp->mob)
	 delete = TRUE;
    /*checks to make sure the character is still in the same room as the mob*/
    else if(temp->mob && temp->ch && temp->mob->in_room != temp->ch->in_room)
	 delete = TRUE;
    if(delete) {
	 /* some slick linked list manipulation */
	 if(temp->prev)
	   temp->prev->next = temp->next;
	 if(temp->next)
	   temp->next->prev = temp->prev;
	 if( temp == first_sleep && (temp->next == NULL || temp->prev == NULL) )
	   first_sleep = temp->next;
	 free_sleep_data(temp);
	 continue;
    }
    
    if(--temp->timer <= 0) {
	 program_flow(temp->vnum, temp->prog->code, temp->mob, temp->ch, NULL, NULL, temp->line);
	 
	 /* more slick linked list manipulation */
	 if(temp->prev)
	   temp->prev->next = temp->next;
	 if(temp->next)
	   temp->next->prev = temp->prev;
	 if( temp == first_sleep && (temp->next == NULL || temp->prev == NULL) )
	   first_sleep = temp->next;
	 free_sleep_data(temp);
    }
  }
}

void hint_update( void )
{
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  CHAR_DATA *bm;
  bool bm_found=FALSE;
  char buf[MAX_STRING_LENGTH];

  for(vch = char_list; vch != NULL; vch = vch_next) {
    vch_next = vch->next;
    
    if(!IS_NPC(vch) && !IS_SET(vch->comm, COMM_NOHINT)) {
      send_to_char("[{MHint{x]{W:{x ",vch);
	 
	 //switch(0) {
	 switch(number_range(0,28)) {
	 default: send_to_char("Use the '{Wpray{x' command to contact the immortals.", vch); break;
	 case 0: 
	 case 15:
	 case 18:
  	   if (vch->class == CLASS_CHANNELER) 
           {
	      for (bm = char_list; bm!= NULL;bm = bm->next)
	      {
		       if (!IS_NPC(bm))
		         continue;
		       if (bm->sex != vch->sex)
			 continue;
                       if (!IS_SET(bm->act,ACT_ROAMERTRAINER))
                         continue;
                       if (!IS_SET(bm->act,ACT_TRAIN))
                         continue;

			if (number_percent() > 50)
			{
			   bm_found = TRUE;
			   sprintf(buf, "Channelling assistance can be found in %s today", !IS_NULLSTR(bm->in_room->area->name) ? bm->in_room->area->name : "Randland");
			   send_to_char(buf,vch);
		           wiznet("$N got a hint about where the Channeller Trainer is located.", vch, NULL, WIZ_LEVELS, 0, get_trust(vch));
		           sprintf(buf, "%s got a hint about where the Channeller Trainer is located.", vch->name);
		           log_string(buf);
                           break;
               		}
			else
			{
			  continue;
			}  
	       }
	      if (!bm_found) {
  		   send_to_char("Use the '{Wnewbie{x' command to ask questions to the newbie helpers, immortals and other newbies.",vch); 
	      }

	   }
	   if (char_knows_masterform(vch) && (vch->pcdata->next_bmtrain <= current_time)) {
		    for (bm = char_list; bm != NULL; bm = bm->next) {
		       if (!IS_NPC(bm))
		         continue;
		       if (IS_SET(bm->train_flags, TRAIN_MASTERFORMS) && number_percent() <= 33) {
		          bm_found = TRUE;
		          sprintf(buf, "Assistance from a weapons expert can be found in %s today.", !IS_NULLSTR(bm->in_room->area->name) ? bm->in_room->area->name : "Randland");
		          send_to_char(buf, vch);
		          
		          // Log it
		          wiznet("$N got a hint about where the Masterforms trainer is located.", vch, NULL, WIZ_LEVELS, 0, get_trust(vch));
		          sprintf(buf, "%s got a hint about where the Masterforms trainer is located.", vch->name);
		          log_string(buf);
              
		          break;
		       }
		   }
		   if (!bm_found) {
  		   send_to_char("Use the '{Wnewbie{x' command to ask questions to the newbie helpers, immortals and other newbies.",vch); 
		   }
	   }
/*
	   if ((vch->pcdata->learned[gsn_duelling] > 0) && (vch->pcdata->next_dutrain <= current_time)) {
		    for (bm = char_list; bm != NULL; bm = bm->next) {
		       if (!IS_NPC(bm))
		         continue;
		       if (IS_SET(bm->train_flags, TRAIN_DUELLING) && number_percent() <= 6) {
		          bm_found = TRUE;
		          sprintf(buf, "Assistance from a master thief can be found in %s today.", !IS_NULLSTR(bm->in_room->area->name) ? bm->in_room->area->name : "Randland");
		          send_to_char(buf, vch);
		          
		          // Log it
		          wiznet("$N got a hint about where the Thiefmaster is located.", vch, NULL, WIZ_LEVELS, 0, get_trust(vch));
		          sprintf(buf, "%s got a hint about where the Thiefmaster is located.", vch->name);
		          log_string(buf);
              
		          break;
		       }
		   }
		   if (!bm_found) {
  		   send_to_char("Use the '{Wnewbie{x' command to ask questions to the newbie helpers, immortals and other newbies.",vch); 
		   }
     }
	*/
	   else   
		   send_to_char("Use the '{Wnewbie{x' command to ask questions to the newbie helpers, immortals and other newbies.",vch); 
	   break;
	 case  1: send_to_char("See '{Whelp who{x' for description of the characters in front of the player names.",vch); break;
	 case  2: send_to_char("Use the '{Wgraduate{x' command to leave newbie school. Make sure you don't leave before you are ready!",vch); break;
	 case  3: send_to_char("See '{Whelp gain{x' or '{Whelp train{x' for learning and training skills or weaves.",vch); break;
	 case  4: send_to_char("Use the '{Wlevel{x' command to spend experience and to advance a level.",vch); break;
	 case  5: send_to_char("See '{Whelp combat_prompt{x' for description of the messages you see each round of a fight.",vch); break;
	 case  6: send_to_char("The {WOP{x in channelers prompt and score is short for the One Power. It is your sphere sum.",vch); break;
	 case  7: send_to_char("It is not possible to train STATS. It is possible to increase STATS with quest tokens.",vch); break;
	 case  8: send_to_char("It is easier to level when you have friends with you.",vch); break;
	 case  9: send_to_char("Use the '{Wskill{x' and '{Wweave{x' command to display your skills and weaves.",vch); break;
	 case 10: send_to_char("See '{Whelp skills{x' or '{Whelp weaves{x' for a listing and description of flags listed with each skill and weave.",vch); break;
	 case 11: send_to_char("Use the '{Wpray{x' command to contact the immortals.", vch); break;
	 case 12: send_to_char("Keep extra weapons on you, mobs like to break or disarm people that attack them.", vch); break;
	 case 13: send_to_char("Explore, explore explore. The only way you'll learn is to do it on your own.", vch); break;
	 case 14: send_to_char("Mobs 10 levels higher will give you the best experience.", vch); break;
	 case 16: send_to_char("Don't drop your bedroll or sleeping item to sleep on it, instead just type sleep bed. This will prevent you from losing it.", vch); break;
	 case 17: send_to_char("Make sure to think out the background of your character before you roleplay, have a sense of who you want to portray your character as.", vch); break;
	 case 19: send_to_char("Use the '{Wareas{x' command to find out what areas are available to level in and their level range.", vch); break;         
	 case 20: send_to_char("See '{Whelp map{x' to find out the general location of all areas.", vch); break;
	 case 21: send_to_char("Use the '{Wglance{x' command instead of '{Wlook{x' to see another person's description.", vch); break;         
	 case 22: send_to_char("Keep up on notes, they'll let you in on what's going on. Use the '{Wnote{x' command. See '{Whelp note{x' for help.", vch); break;
	 case 23: send_to_char("See '{Whelp vmap{x' to find out how to enter and leave a city to the World Map.", vch); break;
	 case 24: send_to_char("Use the '{Wgame{x' channel to ask game related questions.", vch); break;
	 case 25: send_to_char("See the help for your chosen race to help with your character description.", vch); break;
	 case 26: send_to_char("See '{Whelp Guildleader{x' for a list of the current guildleaders.", vch); break;
	 case 27: send_to_char("Don't RP what you can't back up with code strength.", vch); break;
	 case 28: send_to_char("Consider the IC distance and RP your journeys between areas to take realistic time.", vch); break;
	   
	   //	   send_to_char("", vch); break;
	 }
      send_to_char("\n\r",vch);
    }
  }
  return;
}

void web_update( void )
{
//  log_string("web_update start...");

  do_webwho();

//  log_string("web_update stop....");
  
  return;
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void update_handler( void )
{
  extern time_t boot_time;
  extern bool newlock;
  static  int     pulse_area;
  static  int     pulse_mobile;
  static  int     pulse_violence;
  static  int     pulse_point;
  static  int     pulse_music;
  static  int     pulse_warmboot;
  static  int     pulse_shutdown;
  static  int     pulse_quest;
  static  int     pulse_hint;
  static  int     pulse_web;
  static  int     pulse_dtrap;

  if ( --pulse_dtrap    <= 0)
    {
        pulse_dtrap     = PULSE_DEATHTRAP;
        dtrap_update     ( );
    }



    if ((current_time > boot_time+15) && --pulse_hint <= 0) {
      pulse_hint = PULSE_HINT;
      hint_update ( );
    }

    //Only do web updates if game port. E.g not newbie locked.
    if (port = 4000) {
       if (--pulse_web <= 0) {
	    pulse_web = PULSE_WEB;
	    web_update ( );
       }
    }

    if ( --pulse_quest     <= 0 )
    {
	pulse_quest	= PULSE_TICK;
	quest_update	( );
    }

    if ( --pulse_area     <= 0 )
    {
	pulse_area	= PULSE_AREA;
	/* number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 ); */
	area_update	( );
    }

    if ( --pulse_music	  <= 0 )
    {
	pulse_music	= PULSE_MUSIC;
	song_update();
    }

    if ( --pulse_mobile   <= 0 )
    {
	pulse_mobile	= PULSE_MOBILE;
	mobile_update	( );
    }

    if ( --pulse_violence <= 0 )
    {
	pulse_violence	= PULSE_VIOLENCE;
	violence_update	( );
    }

    if ( --pulse_point    <= 0 )
    {
	wiznet("TICK!",NULL,NULL,WIZ_TICKS,0,0);
	pulse_point     = PULSE_TICK;

	/* number_range( PULSE_TICK / 2, 3 * PULSE_TICK / 2 ); */

	weather_update	   ( );
	weather_effect    ( );
	char_update	   ( );
	obj_update	   ( );
	residue_update    ( );
	ward_update       ( );
	room_weave_update ( );
    }

    if (iswarmboot) {
	 if (--pulse_warmboot <= 0 ) {
	   pulse_warmboot =  PULSE_PER_SECOND;
	   //printf("### check_warmboot\n");
	   check_warmboot ( );
	 }
    }
    
    if (isshutdown) {
	 if (--pulse_shutdown <= 0) {
	   pulse_shutdown = PULSE_PER_SECOND;
	   //printf("### check_shutdown\n");
	   check_shutdown ( ) ;
	 }
    }

    aggr_update( );
    sleep_update( );
    tail_chain( );
    return;
}

void dtrap_update( void )
{
   CHAR_DATA *ch;
   CHAR_DATA *ch_next;


   for ( ch = char_list; ch != NULL; ch = ch_next)

   {
       ch_next = ch->next;

       if (!IS_NPC(ch)
       && !IS_IMMORTAL(ch)
       && IS_SET(ch->in_room->room_flags, ROOM_DEATHTRAP) )
       {

          do_look( ch, "dt" );

          if (!IS_AFFECTED(ch,AFF_SAP))
          {
             if (ch->position == POS_STANDING && ch->hit > 20)
             {
                ch->position = POS_RESTING;
                 ch->hit /= 2;
                send_to_char("You better get out of here fast!!!!\n\r", ch);
             }
             else
             {
                ch->hit = 1;
                raw_kill(ch);
                send_to_char("You are dead!!!!", ch);
             }
	}
      }
    }


}

