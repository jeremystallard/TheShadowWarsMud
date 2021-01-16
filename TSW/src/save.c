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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
 
#if !defined(macintosh)
extern  int     _filbuf         args( (FILE *) );
#endif


/* int rename(const char *oldfname, const char *newfname); viene en stdio.h */

char *print_flags(int flag)
{
    int count, pos = 0;
    static char buf[52];


    for (count = 0; count < 32;  count++)
    {
        if (IS_SET(flag,1<<count))
        {
            if (count < 26)
                buf[pos] = 'A' + count;
            else
                buf[pos] = 'a' + (count - 26);
            pos++;
        }
    }

    if (pos == 0)
    {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';

    return buf;
}


/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST	100
static	OBJ_DATA *	rgObjNest	[MAX_NEST];



/*
 * Local functions.
 */
void	fwrite_char	args( ( CHAR_DATA *ch,  FILE *fp ) );
void	fwrite_obj	args( ( CHAR_DATA *ch,  OBJ_DATA  *obj,
			    FILE *fp, int iNest ) );
void	fwrite_keeper_obj	args( ( CHAR_DATA *ch,  OBJ_DATA  *obj,
			    FILE *fp, int iNest ) );
void	fwrite_pet	args( ( CHAR_DATA *pet, FILE *fp) );
void	fread_char	args( ( CHAR_DATA *ch,  FILE *fp ) );
void    fread_pet	args( ( CHAR_DATA *ch,  FILE *fp ) );
void	fread_obj	args( ( CHAR_DATA *ch,  FILE *fp ) );



/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA *ch, bool backup)
{
  char strsave[MAX_INPUT_LENGTH];
  FILE *fp;
  bool disguise=FALSE;
  
  if ( IS_NPC(ch) )
    return;
  
  //Don't save faceless characters
  if (IS_FACELESS(ch))
	return;

  // If under disguise, set flag
  if (IS_DISGUISED(ch))
    disguise = TRUE;
    
  if ( ch->desc != NULL && ch->desc->original != NULL )
    ch = ch->desc->original;
  
#if defined(unix)

  /* create god log */
  if (!backup && !disguise) {
    if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL) {
	 fclose(fpReserve);
	 sprintf(strsave, "%s%s",GOD_DIR, capitalize(ch->name));
	 if ((fp = fopen(strsave,"w")) == NULL) {
	   bug("Save_char_obj: fopen",0);
	   perror(strsave);
	 }
	 
	 fprintf(fp,"Lev %2d Trust %2d %s%s%s %s \n",
		    ch->level, get_trust(ch), ch->name, ch->pcdata->appearance,
		    ch->pcdata->ictitle, ch->pcdata->imm_info);
	 fclose( fp );
	 fpReserve = fopen( NULL_FILE, "r" );
    }
  }
#endif
  
  fclose( fpReserve );
  if (backup)
    sprintf( strsave, "%s%s", PLAYER_BACKUP_DIR, capitalize( ch->name ) );
  else if (disguise)
    sprintf( strsave, "%s%s", PLAYER_DISGUISE_DIR, capitalize( ch->name ) );
  else
    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( ch->name ) );

  //sprintf( strsave, "%s%s", backup == FALSE ? PLAYER_DIR : PLAYER_BACKUP_DIR, capitalize( ch->name ) );
  if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL ) {
    bug( "Save_char_obj: fopen", 0 );
    perror( strsave );
  }
  else {
    fwrite_char( ch, fp );
    if ( ch->carrying != NULL )
	 fwrite_obj( ch, ch->carrying, fp, 0 );
    /* save the pets */
    if (ch->pet != NULL)
	 fwrite_pet(ch->pet,fp);
    else if (ch->mount != NULL)
	 fwrite_pet(ch->mount,fp);
    fprintf( fp, "#END\n" );
  }
  fclose( fp );
  rename(TEMP_FILE,strsave);
  fpReserve = fopen( NULL_FILE, "r" );
  return;
}



/*
 * Write the char.
 */
void fwrite_char( CHAR_DATA *ch, FILE *fp )
{
    AFFECT_DATA *paf;
    /* int sn, gn, pos, i; */ /* Used with Group */
    int sn, gn, pos, i;
    struct idName *names;

    fprintf( fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER"	);
    
    fprintf( fp, "Name %s~\n",	ch->name		);
    if (IS_WOLFKIN(ch) && ch->wkname[0] != '\0')
	 fprintf(fp, "Wkname %s~\n", ch->wkname );
    if (!IS_NULLSTR(ch->real_name))
	 fprintf(fp, "RName %s~\n", ch->real_name );
    fprintf( fp, "Email %s~\n",        ch->pcdata->email);
    fprintf( fp, "Id   %ld\n", ch->id			);
    fprintf( fp, "LogO %ld\n",	current_time		);
    fprintf( fp, "Vers %d\n",   5			);
    if (ch->short_descr[0] != '\0')
    {
	if (IS_HERO(ch) || IS_FORSAKEN(ch)) {
      	   fprintf( fp, "ShD  %s~\n",	ch->short_descr	);
        }
    }
    if( ch->long_descr[0] != '\0')
	fprintf( fp, "LnD  %s~\n",	ch->long_descr	);
    if (ch->description[0] != '\0')
    	fprintf( fp, "Desc %s~\n",	ch->description	);
    if (!IS_NULLSTR(ch->hood_description))
    	fprintf( fp, "HDesc %s~\n",	ch->hood_description);
    if ((IS_AIEL(ch) || IS_TARABONER(ch)) && !IS_NULLSTR(ch->veil_description))
    	fprintf( fp, "VDesc %s~\n",	ch->veil_description);    	
    if (!IS_NULLSTR(ch->wolf_description))
    	fprintf( fp, "WDesc %s~\n",	ch->wolf_description);
    if (!IS_NULLSTR(ch->wound_description))
	 fprintf( fp, "WoundDesc %s~\n",	ch->wound_description);
    if (!IS_NULLSTR(ch->aura_description))
	 fprintf( fp, "AuraDesc %s~\n",	ch->aura_description);
    if( ch->prompt != NULL
     || !str_cmp( ch->prompt,"[%hhp %mm %vmv] " )
     || !str_cmp( ch->prompt,"{x[%hhp %mm %vmv]{x " ) )
        fprintf( fp, "Prom %s~\n",      ch->prompt  	);
#if defined(FIRST_BOOT)
    fprintf( fp, "Race %s~\n", pc_race_table[ch->race].name );
#else
    fprintf( fp, "Race %s~\n", race_table[ch->race].name );
#endif

    /* Guild */
    if (ch->clan)
	 fprintf( fp, "Clan %s~\n",clan_table[ch->clan].name);
    if (ch->rank)
	 fprintf( fp,   "Rank %d\n", ch->rank                );
    if (ch->gtitle)
	 fprintf( fp,    "Gtitle %s~\n", ch->gtitle           );
    if (ch->ginvis)
	 fprintf( fp,    "Ginvis %d\n", ch->ginvis           );
    if (ch->gmute)
	 fprintf( fp,    "Gmute %d\n", ch->gmute           );

    /* oGuild */
    if (ch->oguild)
	 fprintf( fp, "OGuild %s~\n",  clan_table[ch->oguild].name );
    if (ch->oguild_rank)
	 fprintf( fp, "OGrank %d\n",   ch->oguild_rank             );
    if (ch->oguild_title)
	 fprintf( fp, "OGtitle %s~\n", ch->oguild_title            );
    if (ch->oguild_invis)
	 fprintf( fp, "OGinvis %d\n",  ch->oguild_invis            );
    if (ch->oguild_mute)
	 fprintf( fp, "OGmute %d\n",   ch->oguild_mute             );

    /* Subguild */
    if (ch->sguild)
	 fprintf( fp, "Sguild %s~\n",sguild_table[ch->sguild].name);
    if (ch->sguild_rank)
	 fprintf( fp,  "Srank %d\n", ch->sguild_rank               );
    if (ch->sguild_title)
	 fprintf( fp,  "Stitle %s~\n", ch->sguild_title            );
    if (ch->sguild_invis)
	 fprintf( fp,  "Sinvis %d\n", ch->sguild_invis             );
    /* SSubguild */
    if (ch->ssguild)
	 fprintf( fp, "SSguild %s~\n",ssguild_table[ch->ssguild].name);
    if (ch->ssguild_rank)
	 fprintf( fp,  "SSrank %d\n", ch->ssguild_rank               );
    if (ch->ssguild_title)
	 fprintf( fp,  "SStitle %s~\n", ch->ssguild_title            );
    if (ch->ssguild_invis)
	 fprintf( fp,  "SSinvis %d\n", ch->ssguild_invis             );
    /* Minion */
    if (ch->minion != 0) {
	 fprintf( fp, "Minion %ld\n", ch->minion);
	 
	 if (ch->mname)
	   fprintf(fp, "Mname %s~\n", ch->mname);
	 
	 fprintf(fp, "Mrank %d\n", ch->mrank);
	 
	 if (ch->mtitle)
	   fprintf(fp, "Mtitle %s~\n", ch->mtitle);
    }
    //Darkfriend
    if (!IS_NULLSTR(ch->pcdata->df_name)) {
	 fprintf( fp, "DFName %s~\n", ch->pcdata->df_name);
	 fprintf( fp, "DFLevel %d\n", ch->pcdata->df_level);
    }
    
    //Quest
    fprintf( fp, "Quest_Curr    %ld\n",   ch->pcdata->quest_curr  );
    fprintf( fp, "Quest_Accum  %ld\n",   ch->pcdata->quest_accum );
    if (ch->pcdata->nextquest != 0)
	 fprintf( fp, "QuestNext   %d\n",  ch->pcdata->nextquest   );
    else if (ch->pcdata->countdown != 0)
	 fprintf( fp, "QuestNext   %d\n",  10              );
    if (ch->pcdata->questmob != 0)
    {
	fprintf(fp, "QuestMob	%d\n", ch->pcdata->questmob);
	fprintf(fp, "QuestGiverVnum	%d\n", ch->pcdata->questgiver->pIndexData->vnum);
    }
    if (ch->pcdata->questobj != 0)
    {
	fprintf(fp, "QuestObj	%d\n", ch->pcdata->questobj);
	fprintf(fp, "QuestGiverVnum	%d\n", ch->pcdata->questgiver->pIndexData->vnum);
    }


    //Profile
    fprintf( fp, "Lastlog %s~\n",  ch->pcdata->lastlog);
    fprintf( fp, "Lastsite %s~\n",  ch->pcdata->lastsite);
    fprintf( fp, "Sex  %d\n",	ch->sex			);
    fprintf( fp, "Cla  %d\n",	ch->class		);

//    if (ch->forsaken)
//	 fprintf(fp, "Forsaken %d\n", ch->forsaken);
    fprintf( fp, "Levl %d\n",	ch->level		);
    if (ch->pcdata->extended_level)
	 fprintf( fp, "ELevl %d\n",	ch->pcdata->extended_level);


    if (!IS_NULLSTR(ch->pcdata->bondedby))
	 fprintf( fp, "Bonded %s~\n",	ch->pcdata->bondedby  );
    if ((ch->pcdata->bondedbysex))
	 fprintf( fp, "BondedSex %d~\n",	ch->pcdata->bondedbysex  );
    if ((ch->pcdata->bondcount))
	 fprintf( fp, "BondCount %d~\n",	ch->pcdata->bondcount  );
    fprintf( fp, "Bounty %d\n",	ch->pcdata->bounty  );
    if (ch->trust != 0)
	 fprintf( fp, "Tru  %d\n",	ch->trust	);
    fprintf( fp, "Sec  %d\n",    ch->pcdata->security	);	/* OLC */
    fprintf( fp, "Plyd %d\n",
		   ch->played + (int) (current_time - ch->logon)	);
    fprintf( fp, "RolePlayed %ld\n", ch->roleplayed);
		  
    fprintf( fp, "Scro %d\n", 	ch->lines		);
    fprintf( fp, "PKCount %d\n", 	ch->pk_count		);
    fprintf( fp, "PKDiedCount %d\n", 	ch->pk_died_count	);
    fprintf( fp, "Room %d\n",
		   (  ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
			 && ch->was_in_room != NULL )
		   ? ch->was_in_room->vnum
		   : ch->in_room == NULL ? 3700 : ch->in_room->vnum );
    
    fprintf( fp, "HMV  %d %d %d %d\n",
		   ch->hit, ch->max_hit, ch->endurance, ch->max_endurance);
    fprintf( fp, "HLoc %d %d %d %d %d %d\n",
		   ch->hit_loc[LOC_LA],
		   ch->hit_loc[LOC_LL],
		   ch->hit_loc[LOC_HE],
		   ch->hit_loc[LOC_BD],
		   ch->hit_loc[LOC_RL],
		   ch->hit_loc[LOC_RA]);

    if(ch->target_loc != LOC_NA)
	 fprintf(fp, "Target %d\n", ch->target_loc);
    
    if (ch->defend_loc != LOC_NA)
	 fprintf(fp, "Defend %d\n", ch->defend_loc);

    if (!IS_NULLSTR(ch->pcdata->restoremessage))
	 fprintf( fp, "RestMsg %s~\n",	ch->pcdata->restoremessage  );

    if (ch->gold > 0)
      fprintf( fp, "Gold %ld\n",	ch->gold		);
    else
      fprintf( fp, "Gold %d\n", 0			); 
    
    if (ch->silver > 0)
	 fprintf( fp, "Silv %ld\n",ch->silver		);
    else
	 fprintf( fp, "Silv %d\n",0			);
    
    if (ch->pcdata->gold_bank > 0)
      fprintf( fp, "Gold_bank %ld\n",ch->pcdata->gold_bank);
    else
      fprintf( fp, "Gold_bank %d\n", 0);

    if (ch->pcdata->silver_bank > 0)
	 fprintf( fp, "Silv_bank %ld\n",ch->pcdata->silver_bank);
    else
	 fprintf( fp, "Silv_bank %d\n",0);
    
    fprintf( fp, "Exp  %d\n",	ch->exp			);    
    if (ch->act != 0)
	fprintf( fp, "Act  %s\n",   print_flags(ch->act));
    if (ch->act2 != 0)
	 fprintf( fp, "Act2  %s\n",   print_flags(ch->act2));
    if (ch->auto_act != 0)
	 fprintf( fp, "AutoAct  %s\n",   print_flags(ch->auto_act));
    if (ch->ic_flags != 0)
	 fprintf( fp, "ICflags  %s\n",   print_flags(ch->ic_flags));
    if (ch->app != 0) 
	 fprintf( fp, "App  %s\n",   print_flags(ch->app));
//    if (ch->chan_flags != 0)
	 fprintf( fp, "ChanFlag  %s\n",   print_flags(ch->chan_flags));	 
    if (ch->affected_by != 0)
	fprintf( fp, "AfBy %s\n",   print_flags(ch->affected_by));
    fprintf( fp, "Comm %s\n",       print_flags(ch->comm));
    fprintf( fp, "Comm2 %s\n",      print_flags(ch->comm2));
    if (ch->wiznet)
    	fprintf( fp, "Wizn %s\n",   print_flags(ch->wiznet));
    if (ch->off_flags)
    	fprintf( fp, "Offe %s\n",   print_flags(ch->off_flags));
    if (ch->invis_level)
	fprintf( fp, "Invi %d\n", 	ch->invis_level	);
    if (ch->incog_level)
	fprintf(fp,"Inco %d\n",ch->incog_level);
    if (ch->insanity_points)
        fprintf(fp,"Insanity %d\n",ch->insanity_points);
    fprintf( fp, "Pos  %d\n",	
	ch->position == POS_FIGHTING ? POS_STANDING : ch->position );
/* 
    if (ch->practice != 0)
    	fprintf( fp, "Prac %d\n",	ch->practice	);
*/
    if (ch->train != 0)
	fprintf( fp, "Trai %d\n",	ch->train	);
    if (ch->saving_throw != 0)
	fprintf( fp, "Save  %d\n",	ch->saving_throw);
    fprintf( fp, "Alig  %d\n",	ch->alignment		);
    fprintf( fp, "ArrCnt  %d\n",	ch->arrow_count	);
    if (ch->hitroll != 0)
	fprintf( fp, "Hit   %d\n",	ch->hitroll	);
    if (ch->damroll != 0)
	fprintf( fp, "Dam   %d\n",	ch->damroll	);
    fprintf( fp, "ACs %d %d %d %d\n",	
	ch->armor[0],ch->armor[1],ch->armor[2],ch->armor[3]);
    if (ch->wimpy !=0 )
	fprintf( fp, "Wimp  %d\n",	ch->wimpy	);
    fprintf( fp, "Attr %d %d %d %d %d\n",
	ch->perm_stat[STAT_STR],
	ch->perm_stat[STAT_INT],
	ch->perm_stat[STAT_WIS],
	ch->perm_stat[STAT_DEX],
	ch->perm_stat[STAT_CON] );

    fprintf (fp, "AMod %d %d %d %d %d\n",
	ch->mod_stat[STAT_STR],
	ch->mod_stat[STAT_INT],
	ch->mod_stat[STAT_WIS],
	ch->mod_stat[STAT_DEX],
	ch->mod_stat[STAT_CON] );

/* Channel section START */

    fprintf(fp, "CSphere  %d %d %d %d %d\n",
		  ch->cre_sphere[SPHERE_AIR],
		  ch->cre_sphere[SPHERE_EARTH],
		  ch->cre_sphere[SPHERE_FIRE],
		  ch->cre_sphere[SPHERE_SPIRIT],
		  ch->cre_sphere[SPHERE_WATER]);
    
    fprintf(fp, "Sphere %d %d %d %d %d\n",
		  ch->perm_sphere[SPHERE_AIR],
		  ch->perm_sphere[SPHERE_EARTH],
		  ch->perm_sphere[SPHERE_FIRE],
		  ch->perm_sphere[SPHERE_SPIRIT],
		  ch->perm_sphere[SPHERE_WATER]);
    
    fprintf(fp, "Burnout %d\n",ch->burnout);
    fprintf(fp, "MaxBurnout %d\n",ch->max_burnout);
    fprintf(fp, "MSphere %d\n",ch->main_sphere);
    fprintf(fp, "Holding %ld\n", ch->holding);
    fprintf(fp, "Autoholding %d\n", ch->autoholding);
	if( !IS_NPC(ch) && ch->pcdata->spheretrain)
	{	fprintf(fp, "Spheretrain %d\n", ch->pcdata->spheretrain) ;	}

/* Channel section END */


/* Merits/Flaws/Talents */
    if (ch->merits != 0)
	 fprintf(fp, "Merits %s\n", print_flags(ch->merits));
    if (ch->flaws != 0)
	 fprintf(fp, "Flaws %s\n", print_flags(ch->flaws));
    if (ch->talents != 0)
	 fprintf(fp, "Talents %s\n", print_flags(ch->talents));
    if (ch->pcdata->forceinsanity != 0)
	 fprintf(fp, "ForceInsanity %d\n", ch->pcdata->forceinsanity);
    if (ch->pcdata->forcespark != 0)
	 fprintf(fp, "ForceSpark %d\n", ch->pcdata->forcespark);
    
    if (ch->pcdata->timeoutstamp != 0)
	fprintf(fp, "Timeout %ld\n",(long)ch->pcdata->timeoutstamp);

    if ( IS_NPC(ch) ) {
	 fprintf( fp, "Vnum %d\n",	ch->pIndexData->vnum	);
    }
    else {
	 fprintf( fp, "Pass %s~\n",	ch->pcdata->pwd		);
	 if (!IS_NULLSTR(ch->pcdata->bamfin))
	   fprintf( fp, "Bin  %s~\n",	ch->pcdata->bamfin);
	 if (!IS_NULLSTR(ch->pcdata->bamfout))
	   fprintf( fp, "Bout %s~\n",	ch->pcdata->bamfout);
	 fprintf( fp, "Appearance %s~\n",	ch->pcdata->appearance	);
	 if (!IS_NULLSTR(ch->pcdata->hood_appearance))
	   fprintf( fp, "HAppearance %s~\n",	ch->pcdata->hood_appearance	);
	 if ( (IS_AIEL(ch) || IS_TARABONER(ch)) && !IS_NULLSTR(ch->pcdata->veil_appearance))
	   fprintf( fp, "VAppearance %s~\n",	ch->pcdata->veil_appearance	);	   
	 if (!IS_NULLSTR(ch->pcdata->wolf_appearance))
	   fprintf( fp, "WAppearance %s~\n",	ch->pcdata->wolf_appearance	);
	 if (!IS_NULLSTR(ch->pcdata->dreaming_appearance))
	   fprintf( fp, "DAppearance %s~\n",	ch->pcdata->dreaming_appearance	);
	 if (!IS_NULLSTR(ch->pcdata->masquerade_appearance))
	   fprintf( fp, "MAppearance %s~\n",    ch->pcdata->masquerade_appearance	);

	 if (!IS_NULLSTR(ch->pcdata->ictitle))
	   fprintf( fp, "ICtitl %s~\n",	ch->pcdata->ictitle	);
	 
	 fprintf( fp, "Titl %s~\n",	ch->pcdata->title	);

	 if (IS_HERO(ch) && !IS_NULLSTR(ch->pcdata->imm_info))
	   fprintf( fp, "ImmInfo %s~\n",   ch->pcdata->imm_info    );

	 // Voices
	 if (!IS_NULLSTR(ch->pcdata->say_voice))
	   fprintf( fp, "VoiceSay %s~\n",   ch->pcdata->say_voice     ) ;
	 if (!IS_NULLSTR(ch->pcdata->ask_voice))
	   fprintf( fp, "VoiceAsk %s~\n",   ch->pcdata->ask_voice     );
	 if (!IS_NULLSTR(ch->pcdata->exclaim_voice))
	   fprintf( fp, "VoiceExc %s~\n",   ch->pcdata->exclaim_voice );
	 if (!IS_NULLSTR(ch->pcdata->battlecry_voice))
	   fprintf( fp, "VoiceBattle %s~\n",   ch->pcdata->battlecry_voice );
	 if (!IS_NULLSTR(ch->pcdata->polls))
	 {
	    fprintf(fp,"Polls %s~\n",ch->pcdata->polls);
	 }
	 fprintf( fp, "Pnts %d\n",   	ch->pcdata->points      );
	 fprintf( fp, "TSex %d\n",	ch->pcdata->true_sex	);
	 fprintf( fp, "LLev %d\n",	ch->pcdata->last_level	);
	 fprintf( fp, "Home %d\n", ch->pcdata->home);
	 fprintf( fp, "ICLocRecall %d\n", ch->pcdata->iclocrecall);
	 fprintf( fp, "HMVP %d %d\n", ch->pcdata->perm_hit, 
			ch->pcdata->perm_endurance);
	 
	 fprintf( fp, "Cnd  %d %d %d %d\n",
			ch->pcdata->condition[0],
			ch->pcdata->condition[1],
			ch->pcdata->condition[2],
			ch->pcdata->condition[3] );
	 
	 /*
	 * Write Colour Config Information.
	 */
	fprintf( fp, "Coloura     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",   
		ch->pcdata->auction[2],
		ch->pcdata->auction[0],
		ch->pcdata->auction[1],
		ch->pcdata->gossip[2],
		ch->pcdata->gossip[0],
		ch->pcdata->gossip[1],
		ch->pcdata->music[2],
		ch->pcdata->music[0],
		ch->pcdata->music[1],
		ch->pcdata->chat[2],
		ch->pcdata->chat[0],
		ch->pcdata->chat[1],
		ch->pcdata->minion[2],
		ch->pcdata->minion[0],
		ch->pcdata->minion[1] );
	fprintf( fp, "Colourb     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",   
		ch->pcdata->immtalk[2],
		ch->pcdata->immtalk[0],
		ch->pcdata->immtalk[1],
		ch->pcdata->game[2],
		ch->pcdata->game[0],
		ch->pcdata->game[1],
		ch->pcdata->tell[2],
		ch->pcdata->tell[0],
		ch->pcdata->tell[1],
		ch->pcdata->reply[2],
		ch->pcdata->reply[0],
		ch->pcdata->reply[1],
		ch->pcdata->gtell[2],
		ch->pcdata->gtell[0],
		ch->pcdata->gtell[1] );
	fprintf( fp, "Colourc     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",   
		ch->pcdata->room_exits[2],
		ch->pcdata->room_exits[0],
		ch->pcdata->room_exits[1],
		ch->pcdata->room_things[2],
		ch->pcdata->room_things[0],
		ch->pcdata->room_things[1],
		ch->pcdata->prompt[2],
		ch->pcdata->prompt[0],
		ch->pcdata->prompt[1],
		ch->pcdata->room_people[2],
		ch->pcdata->room_people[0],
		ch->pcdata->room_people[1],
		ch->pcdata->room[2],
		ch->pcdata->room[0],
		ch->pcdata->room[1] );
	fprintf( fp, "Colourd     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",   
		ch->pcdata->bondtalk[2],
		ch->pcdata->bondtalk[0],
		ch->pcdata->bondtalk[1],
		ch->pcdata->channel_name[2],
		ch->pcdata->channel_name[0],
		ch->pcdata->channel_name[1],
		ch->pcdata->wiznet[2],
		ch->pcdata->wiznet[0],
		ch->pcdata->wiznet[1],
		ch->pcdata->fight_death[2],
		ch->pcdata->fight_death[0],
		ch->pcdata->fight_death[1],
		ch->pcdata->fight_yhit[2],
		ch->pcdata->fight_yhit[0],
		ch->pcdata->fight_yhit[1] );
	fprintf( fp, "Coloure     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",   
		ch->pcdata->fight_ohit[2],
		ch->pcdata->fight_ohit[0],
		ch->pcdata->fight_ohit[1],
		ch->pcdata->fight_thit[2],
		ch->pcdata->fight_thit[0],
		ch->pcdata->fight_thit[1],
		ch->pcdata->fight_skill[2],
		ch->pcdata->fight_skill[0],
		ch->pcdata->fight_skill[1],
		ch->pcdata->sayt[2],
		ch->pcdata->sayt[0],
		ch->pcdata->sayt[1],
		ch->pcdata->osay[2],
		ch->pcdata->osay[0],
		ch->pcdata->osay[1]);
	fprintf( fp, "Colourf     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",   
		ch->pcdata->wolfkin_talk[2],
		ch->pcdata->wolfkin_talk[0],
		ch->pcdata->wolfkin_talk[1],
		ch->pcdata->guild_talk[2],
		ch->pcdata->guild_talk[0],
		ch->pcdata->guild_talk[1],
		ch->pcdata->race_talk[2],
		ch->pcdata->race_talk[0],
		ch->pcdata->race_talk[1],
		ch->pcdata->df_talk[2],
		ch->pcdata->df_talk[0],
		ch->pcdata->df_talk[1],
		ch->pcdata->newbie[2],
		ch->pcdata->newbie[0],
		ch->pcdata->newbie[1]);

	fprintf( fp, "Colourg     %d%d%d %d%d%d %d%d%d\n", 
		    ch->pcdata->sguild_talk[2],
		    ch->pcdata->sguild_talk[0],
		    ch->pcdata->sguild_talk[1],
		    ch->pcdata->ssguild_talk[2],
		    ch->pcdata->ssguild_talk[0],
		    ch->pcdata->ssguild_talk[1],
		    ch->pcdata->leader_talk[2],
		    ch->pcdata->leader_talk[0],
		    ch->pcdata->leader_talk[1]);

	fprintf( fp, "Colourh     %d%d%d %d%d%d %d%d%d %d%d%d %d%d%d\n",
		    ch->pcdata->oguild_talk[2],
		    ch->pcdata->oguild_talk[0],
		    ch->pcdata->oguild_talk[1],
		    0,0,0,   // dummy for later use
		    0,0,0,   // dummy for later use
		    0,0,0,   // dummy for later use		    
		    0,0,0); // dummy for later use
	 
	/* write alias */
	for (pos = 0; pos < MAX_ALIAS; pos++) {
	  if (ch->pcdata->alias[pos] == NULL
		 ||  ch->pcdata->alias_sub[pos] == NULL)
	    break;
	  
	  fprintf(fp,"Alias %s %s~\n",ch->pcdata->alias[pos],
			ch->pcdata->alias_sub[pos]);
	}
	
	/* write ignore */
	for (pos = 0; pos < MAX_IGNORE; pos++) {
	  if (IS_NULLSTR(ch->pcdata->ignore[pos]))
	    break;
	  
	  fprintf(fp, "Ignore %s~\n", ch->pcdata->ignore[pos]);
	}

	for (pos = 0; pos < MAX_SHEAT_LOC; pos++) {
	  if (ch->sheat_where_name[pos] == NULL)
	    break;
	  
	  fprintf(fp, "SheatWName %s~\n", ch->sheat_where_name[pos]);
	}

	/* Disguise */
	for (pos = 0; pos < MAX_DISGUISE; pos++) {
	  if (ch->pcdata->disguise[pos] != NULL)
	    fprintf(fp, "Disguise %s~\n", ch->pcdata->disguise[pos]);
	}

	GATEKEY_DATA * keys = ch->pcdata->keys;
        while (keys != NULL) 
        {
		fprintf(fp, "GateKey %s %s~\n", keys->key_alias, keys->key_value);
		keys = keys->next;
	}

	/* Save note board status */
	/* Save number of boards in case that number changes */
	fprintf (fp, "Boards       %d ", MAX_BOARD);
	for (i = 0; i < MAX_BOARD; i++)
	  fprintf (fp, "%s %ld ", boards[i].short_name, ch->pcdata->last_note[i]);
	fprintf (fp, "\n");
	
	for ( sn = 0; sn < MAX_SKILL; sn++ ) {
	  if ( skill_table[sn].name != NULL && ch->pcdata->learned[sn] != 0 ) {
	    fprintf( fp, "Sk %d '%s'\n",
			   ch->pcdata->learned[sn], skill_table[sn].name );
	  }
	}

	   for ( gn = 0; gn < MAX_GROUP; gn++ ) {
		if ( group_table[gn].name != NULL && ch->pcdata->group_known[gn]) {
		  fprintf( fp, "Gr '%s'\n",group_table[gn].name);
		}
	   }
    }
    
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
	if (paf->type < 0 || paf->type>= MAX_SKILL)
	    continue;
	
	fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d %ld\n",
	    skill_table[paf->type].name,
	    paf->where,
	    paf->level,
	    paf->duration,
	    paf->modifier,
	    paf->location,
	    paf->bitvector,
	    paf->casterId
	    );
    }

    for( names = ch->pcdata->names; names; names = names->next ) {
	 /* One entry for each name in the players know list */
	 fprintf( fp, "Know %ld '%s'\n", names->id, names->name );
    }

    /* Next BM training time */
    if (ch->pcdata->next_bmtrain)
	 fprintf( fp, "BMtime %ld\n",	ch->pcdata->next_bmtrain 	);
	 
    /* Next Speardancer training time */
    if (ch->pcdata->next_sdtrain)
	 fprintf( fp, "SDtime %ld\n",	ch->pcdata->next_sdtrain 	);
	 
    /* Next DU training time */
    if (ch->pcdata->next_dutrain)
	 fprintf( fp, "DUtime %ld\n",	ch->pcdata->next_dutrain 	);
	 
    /* Backup time */
    if (ch->pcdata->last_backup)
	 fprintf( fp, "Backup %ld\n",	ch->pcdata->last_backup 	);

    /* Next CA time */
    if (ch->pcdata->next_createangreal)
	 fprintf( fp, "CAtime %ld\n",	ch->pcdata->next_createangreal 	);
    if (ch->pcdata->next_createangreal)
	 fprintf( fp, "CA24time %ld\n",	ch->pcdata->next_24hourangreal 	);
    if (ch->pcdata->next_createangreal)
	 fprintf( fp, "CA24amount %d\n",	ch->pcdata->createangrealcount 	);
    /* World info */
    fprintf(fp, "World %s\n", print_flags(ch->world));
    if (ch->world_vnum)
	 fprintf(fp, "Wvnum %d\n", ch->world_vnum);

    /* Web vote time */
    if (ch->pcdata->last_web_vote)
	 fprintf( fp, "Webvote %ld\n", ch->pcdata->last_web_vote );

    if (ch->pcdata->reward_multiplier)
    {
	 fprintf( fp, "RewMult %d\n", ch->pcdata->reward_multiplier );
	 fprintf( fp, "RewTime %ld\n", ch->pcdata->reward_time );
    }
    
 
    /* RP Timer and bonus saving */
    if (ch->pcdata->rprewardtimer)
    {
	fprintf( fp, "RPTimer %ld\n", ch->pcdata->rprewardtimer );
    }
    else
	fprintf(fp, "RPTimer %d\n", 0 );

    if (ch->pcdata->rpbonus)
    {
	fprintf( fp, "RPBonus %d\n", ch->pcdata->rpbonus );
    }
    else
	fprintf( fp, "RPBonus %d\n", 0 );

 
    // Trolloc kill counters
    if (ch->race == race_lookup("trolloc")) {
       fprintf( fp, "TR_ClanKill %d\n", ch->pcdata->clan_kill_cnt);
       fprintf( fp, "TR_PCKill %d\n", ch->pcdata->pc_kill_cnt);
    }	 

    if (ch->skim_to)
	 fprintf( fp, "Skim_to %d\n", ch->skim_to);
    
    fprintf( fp, "End\n\n" );
    return;
}

/* write a pet */
void fwrite_pet( CHAR_DATA *pet, FILE *fp)
{
    AFFECT_DATA *paf;
    
    fprintf(fp,"#PET\n");
    
    fprintf(fp,"Vnum %d\n",pet->pIndexData->vnum);
    
    fprintf(fp,"Name %s~\n", pet->name);
    fprintf(fp,"LogO %ld\n", current_time);
    if (pet->short_descr != pet->pIndexData->short_descr)
    	fprintf(fp,"ShD  %s~\n", pet->short_descr);
    if (pet->long_descr != pet->pIndexData->long_descr)
    	fprintf(fp,"LnD  %s~\n", pet->long_descr);
    if (pet->description != pet->pIndexData->description)
    	fprintf(fp,"Desc %s~\n", pet->description);
    fprintf( fp, "Room %d\n",
        (  pet->in_room == get_room_index( ROOM_VNUM_LIMBO )
        && pet->was_in_room != NULL )
            ? pet->was_in_room->vnum
            : pet->in_room == NULL ? 3700 : pet->in_room->vnum );
    if (pet->race != pet->pIndexData->race)
    	fprintf(fp,"Race %s~\n", race_table[pet->race].name);
    if (pet->clan)
        fprintf( fp, "Clan %s~\n",clan_table[pet->clan].name);
    fprintf(fp,"Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
    	fprintf(fp,"Levl %d\n", pet->level);
    fprintf(fp, "HMV  %d %d %d %d\n",
    	pet->hit, pet->max_hit, pet->endurance, pet->max_endurance);
    if (pet->gold > 0)
    	fprintf(fp,"Gold %ld\n",pet->gold);
    if (pet->silver > 0)
	fprintf(fp,"Silv %ld\n",pet->silver);
    if (pet->exp > 0)
    	fprintf(fp, "Exp  %d\n", pet->exp);
    if (pet->act != pet->pIndexData->act)
    	fprintf(fp, "Act  %s\n", print_flags(pet->act));
    if (pet->affected_by != pet->pIndexData->affected_by)
    	fprintf(fp, "AfBy %s\n", print_flags(pet->affected_by));
    if (pet->comm != 0)
    	fprintf(fp, "Comm %s\n", print_flags(pet->comm));
    fprintf(fp,"Pos  %d\n", pet->position = POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)
    	fprintf(fp, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->pIndexData->alignment)
    	fprintf(fp, "Alig %d\n", pet->alignment);
    if (pet->arrow_count != 0)
    	fprintf(fp, "ArrCnt %d\n", pet->arrow_count);
    if (pet->hitroll != pet->pIndexData->hitroll)
    	fprintf(fp, "Hit  %d\n", pet->hitroll);
    if (pet->damroll != pet->pIndexData->damage[DICE_BONUS])
    	fprintf(fp, "Dam  %d\n", pet->damroll);
    fprintf(fp, "ACs  %d %d %d %d\n",
    	pet->armor[0],pet->armor[1],pet->armor[2],pet->armor[3]);
    fprintf(fp, "Attr %d %d %d %d %d\n",
    	pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
    	pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
    	pet->perm_stat[STAT_CON]);
    fprintf(fp, "AMod %d %d %d %d %d\n",
    	pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
    	pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX],
    	pet->mod_stat[STAT_CON]);
    
    for ( paf = pet->affected; paf != NULL; paf = paf->next )
    {
    	if (paf->type < 0 || paf->type >= MAX_SKILL)
    	    continue;
    	    
    	fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
    	    skill_table[paf->type].name,
    	    paf->where, paf->level, paf->duration, paf->modifier,paf->location,
    	    paf->bitvector);
    }
    
    fprintf(fp,"End\n");
    return;
}

    
/*
 * Write an object and its contents.
 */
void fwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if ( obj->next_content != NULL )
		fwrite_obj( ch, obj->next_content, fp, iNest );


    fprintf( fp, "#O\n" );
    fprintf( fp, "Vnum %d\n",   obj->pIndexData->vnum        );
    if (!obj->pIndexData->new_format)
	fprintf( fp, "Oldstyle\n");
    if (obj->enchanted)
	fprintf( fp,"Enchanted\n");
    fprintf( fp, "Nest %d\n",	iNest	  	     );

    /* these data are only used if they do not match the defaults */

    if ( obj->name != obj->pIndexData->name)
    	fprintf( fp, "Name %s~\n",	obj->name		     );
    if ( obj->short_descr != obj->pIndexData->short_descr)
        fprintf( fp, "ShD  %s~\n",	obj->short_descr	     );
    if ( obj->description != obj->pIndexData->description)
        fprintf( fp, "Desc %s~\n",	obj->description	     );
    if ( obj->extra_flags != obj->pIndexData->extra_flags)
        fprintf( fp, "ExtF %d\n",	obj->extra_flags	     );
    if ( obj->wear_flags != obj->pIndexData->wear_flags)
        fprintf( fp, "WeaF %d\n",	obj->wear_flags		     );
    if ( obj->item_type != obj->pIndexData->item_type)
        fprintf( fp, "Ityp %d\n",	obj->item_type		     );
    if ( obj->weight != obj->pIndexData->weight)
        fprintf( fp, "Wt   %d\n",	obj->weight		     );
    if ( obj->condition != obj->pIndexData->condition)
	fprintf( fp, "Cond %d\n",	obj->condition		     );

    if (obj->owner != NULL && obj->owner[0] != '\0')
       fprintf( fp, "Owner %s~\n", 	obj->owner		     );

    /* variable data */

    fprintf( fp, "Wear %d\n",   obj->wear_loc                );
    if (obj->level != obj->pIndexData->level)
        fprintf( fp, "Lev  %d\n",	obj->level		     );
    if (obj->timer != 0)
        fprintf( fp, "Time %d\n",	obj->timer	     );
    fprintf( fp, "Cost %d\n",	obj->cost		     );
    if (obj->value[0] != obj->pIndexData->value[0]
    ||  obj->value[1] != obj->pIndexData->value[1]
    ||  obj->value[2] != obj->pIndexData->value[2]
    ||  obj->value[3] != obj->pIndexData->value[3]
    ||  obj->value[4] != obj->pIndexData->value[4]) 
    	fprintf( fp, "Val  %d %d %d %d %d\n",
	    obj->value[0], obj->value[1], obj->value[2], obj->value[3],
	    obj->value[4]	     );

    switch ( obj->item_type )
    {
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_PILL:
	if ( obj->value[1] > 0 )
	{
	    fprintf( fp, "Spell 1 '%s'\n", 
		skill_table[obj->value[1]].name );
	}

	if ( obj->value[2] > 0 )
	{
	    fprintf( fp, "Spell 2 '%s'\n", 
		skill_table[obj->value[2]].name );
	}

	if ( obj->value[3] > 0 )
	{
	    fprintf( fp, "Spell 3 '%s'\n", 
		skill_table[obj->value[3]].name );
	}

	break;

    case ITEM_STAFF:
    case ITEM_WAND:
	if ( obj->value[3] > 0 )
	{
	    fprintf( fp, "Spell 3 '%s'\n", 
		skill_table[obj->value[3]].name );
	}

	break;
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
	if (paf->type < 0 || paf->type >= MAX_SKILL)
	    continue;
        fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
            skill_table[paf->type].name,
            paf->where,
            paf->level,
            paf->duration,
            paf->modifier,
            paf->location,
            paf->bitvector
            );
    }

    for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
    {
	fprintf( fp, "ExDe %s~ %s~\n",
	    ed->keyword, ed->description );
    }

    fprintf( fp, "End\n\n" );

    if ( obj->contains != NULL )
	fwrite_obj( ch, obj->contains, fp, iNest + 1 );

    return;
}


/*
 * Write an object and its contents.
 */
void fwrite_keeper_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    //if ( obj->next_content != NULL )
	//	fwrite_obj( ch, obj->next_content, fp, iNest );


    fprintf( fp, "#O\n" );
    fprintf( fp, "Vnum %d\n",   obj->pIndexData->vnum        );
    if (!obj->pIndexData->new_format)
	fprintf( fp, "Oldstyle\n");
    if (obj->enchanted)
	fprintf( fp,"Enchanted\n");
    fprintf( fp, "Nest %d\n",	iNest	  	     );

    /* these data are only used if they do not match the defaults */

    if ( obj->name != obj->pIndexData->name)
    	fprintf( fp, "Name %s~\n",	obj->name		     );
    if ( obj->short_descr != obj->pIndexData->short_descr)
        fprintf( fp, "ShD  %s~\n",	obj->short_descr	     );
    if ( obj->description != obj->pIndexData->description)
        fprintf( fp, "Desc %s~\n",	obj->description	     );
    if ( obj->extra_flags != obj->pIndexData->extra_flags)
        fprintf( fp, "ExtF %d\n",	obj->extra_flags	     );
    if ( obj->wear_flags != obj->pIndexData->wear_flags)
        fprintf( fp, "WeaF %d\n",	obj->wear_flags		     );
    if ( obj->item_type != obj->pIndexData->item_type)
        fprintf( fp, "Ityp %d\n",	obj->item_type		     );
    if ( obj->weight != obj->pIndexData->weight)
        fprintf( fp, "Wt   %d\n",	obj->weight		     );
    if ( obj->condition != obj->pIndexData->condition)
	fprintf( fp, "Cond %d\n",	obj->condition		     );

    if (obj->owner != NULL && obj->owner[0] != '\0')
       fprintf( fp, "Owner %s~\n", 	obj->owner		     );

    /* variable data */

    fprintf( fp, "Wear %d\n",   obj->wear_loc                );
    if (obj->level != obj->pIndexData->level)
        fprintf( fp, "Lev  %d\n",	obj->level		     );
    if (obj->timer != 0)
        fprintf( fp, "Time %d\n",	obj->timer	     );
    fprintf( fp, "Cost %d\n",	obj->cost		     );
    if (obj->value[0] != obj->pIndexData->value[0]
    ||  obj->value[1] != obj->pIndexData->value[1]
    ||  obj->value[2] != obj->pIndexData->value[2]
    ||  obj->value[3] != obj->pIndexData->value[3]
    ||  obj->value[4] != obj->pIndexData->value[4]) 
    	fprintf( fp, "Val  %d %d %d %d %d\n",
	    obj->value[0], obj->value[1], obj->value[2], obj->value[3],
	    obj->value[4]	     );

    switch ( obj->item_type )
    {
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_PILL:
	if ( obj->value[1] > 0 )
	{
	    fprintf( fp, "Spell 1 '%s'\n", 
		skill_table[obj->value[1]].name );
	}

	if ( obj->value[2] > 0 )
	{
	    fprintf( fp, "Spell 2 '%s'\n", 
		skill_table[obj->value[2]].name );
	}

	if ( obj->value[3] > 0 )
	{
	    fprintf( fp, "Spell 3 '%s'\n", 
		skill_table[obj->value[3]].name );
	}

	break;

    case ITEM_STAFF:
    case ITEM_WAND:
	if ( obj->value[3] > 0 )
	{
	    fprintf( fp, "Spell 3 '%s'\n", 
		skill_table[obj->value[3]].name );
	}

	break;
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
	if (paf->type < 0 || paf->type >= MAX_SKILL)
	    continue;
        fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
            skill_table[paf->type].name,
            paf->where,
            paf->level,
            paf->duration,
            paf->modifier,
            paf->location,
            paf->bitvector
            );
    }

    for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
    {
	fprintf( fp, "ExDe %s~ %s~\n",
	    ed->keyword, ed->description );
    }

    fprintf( fp, "End\n\n" );

    if ( obj->contains != NULL )
	fwrite_obj( ch, obj->contains, fp, iNest + 1 );

    return;
}

/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( DESCRIPTOR_DATA *d, char *name, bool disguise )
{
    char strsave[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;
    int stat;
    int i;

    ch = new_char();
    ch->pcdata = new_pcdata();

    d->character			= ch;
    ch->desc				= d;
    ch->name				= str_dup( name );
    ch->id				= get_pc_id();
    ch->race				= race_lookup("human");
    ch->max_burnout			= MAX_BURNOUT;

    /* default world */
    SET_BIT(ch->world, WORLD_NORMAL);
    
    // default channeling flags
    SET_BIT(ch->chan_flags, CHAN_SEEAREAWEAVES);
    SET_BIT(ch->chan_flags, CHAN_SEECHANNELING);

	ch->pcdata->spheretrain				= 0 ;
    
    ch->pcdata->guarding                = NULL;
    ch->pcdata->guarded_by              = NULL;
    
    // Timers
    ch->pcdata->next_assassinate        = current_time + 45;
    ch->pcdata->next_recall             = current_time + 60;
    ch->pcdata->next_wrap               = current_time + 45;

    ch->link_info                       = NULL;
    ch->act				= PLR_NOSUMMON;
    ch->act2				= 0;
    ch->comm				= COMM_COMBINE 
					| COMM_PROMPT;
    ch->prompt 				= str_dup("[{R%h{x/{R%H{xhp {G%m{x/{G%M{xen] ");
    ch->pcdata->confirm_delete		= FALSE;
    ch->pcdata->board                   = &boards[DEFAULT_BOARD];
    ch->pcdata->pwd			= str_dup( "" );
    ch->pcdata->bamfin			= str_dup( "" );
    ch->pcdata->bamfout			= str_dup( "" );
    ch->pcdata->email                   = str_dup( "" );
    ch->pcdata->appearance     		= str_dup( "" );
    ch->pcdata->ictitle			= str_dup( "" );
    ch->pcdata->title			= str_dup( "" );
    ch->pcdata->imm_info                = str_dup( "" );
    ch->pcdata->bondedby 		= str_dup( "" );
    ch->pcdata->restoremessage 		= str_dup( "" );
    ch->pcdata->df_level 		= -1;
    ch->pcdata->df_name  		= str_dup( "" );
    ch->pcdata->clan_kill_cnt           = 0;
    ch->pcdata->pc_kill_cnt             = 0;
    ch->pcdata->bounty			= 0;
    ch->pcdata->next_createangreal      =0;
    if(!IS_NULLSTR(d->host))
	 ch->pcdata->lastsite		= str_dup( d->host );
    ch->pcdata->lastlog			= str_dup( (char *)ctime(&current_time) );
    ch->pcdata->iclocrecall             = ROOM_VNUM_RECALL;
    ch->pcdata->quest_curr = 0;
    ch->pcdata->quest_accum = 0;
    ch->pcdata->nextquest = 0;

    ch->target_loc = LOC_NA;
    ch->defend_loc = LOC_NA;
    
    for (stat =0; stat < MAX_STATS; stat++)
	 ch->perm_stat[stat]		= 13;

    //ch->pcdata->condition[COND_THIRST]	= 48; 
    //ch->pcdata->condition[COND_FULL]	= 48;
    //ch->pcdata->condition[COND_HUNGER]	= 48;
    ch->pcdata->condition[COND_THIRST]	= -1; 
    ch->pcdata->condition[COND_FULL]	= -1;
    ch->pcdata->condition[COND_HUNGER]	= -1;
    ch->riding	= FALSE;
    ch->mount	= NULL;
    ch->pcdata->security		= 0;	/* OLC */

    for (i=0;i<HISTSIZE;i++) {

	 // Init tell_hist
	 ch->pcdata->tell_history[i].player_name = NULL;
	 ch->pcdata->tell_history[i].line_data = NULL;
	 ch->pcdata->tell_history[i].when = 0;

	 // Init bond_hist
	 ch->pcdata->bond_history[i].player_name = NULL;
	 ch->pcdata->bond_history[i].line_data = NULL;
	 ch->pcdata->bond_history[i].when = 0;
	 
	 // Init minion_hist
	 ch->pcdata->minion_history[i].player_name = NULL;
	 ch->pcdata->minion_history[i].line_data = NULL;
	 ch->pcdata->minion_history[i].when = 0;

	 // Init group_hist
	 ch->pcdata->group_history[i].player_name = NULL;
	 ch->pcdata->group_history[i].line_data = NULL;
	 ch->pcdata->group_history[i].when = 0;
    }

    ch->pcdata->auction[0]	= ( BRIGHT );
    ch->pcdata->auction[1]	= ( YELLOW );
    ch->pcdata->auction[2]	= 0;
    ch->pcdata->gossip[0]	= ( NORMAL );
    ch->pcdata->gossip[1]	= ( MAGENTA );
    ch->pcdata->gossip[2]	= 0;
    ch->pcdata->music[0]	= ( BRIGHT );
    ch->pcdata->music[1]	= ( RED );
    ch->pcdata->music[2]	= 0;
    ch->pcdata->chat[0]	= ( BRIGHT );
    ch->pcdata->chat[1]	= ( YELLOW );
    ch->pcdata->chat[2]	= 0;
    ch->pcdata->minion[0]	= ( BRIGHT );
    ch->pcdata->minion[1]	= ( WHITE );
    ch->pcdata->minion[2]	= 0;
    ch->pcdata->immtalk[0]	= ( NORMAL );
    ch->pcdata->immtalk[1]	= ( YELLOW );
    ch->pcdata->immtalk[2]	= 0;
    ch->pcdata->game[0]	= ( NORMAL );
    ch->pcdata->game[1]	= ( GREEN );
    ch->pcdata->game[2]	= 0;
    ch->pcdata->tell[0]	= ( NORMAL );
    ch->pcdata->tell[1]	= ( CYAN );
    ch->pcdata->tell[2]	= 0;
    ch->pcdata->reply[0]	= ( NORMAL );
    ch->pcdata->reply[1]	= ( YELLOW );
    ch->pcdata->reply[2]	= 0;
    ch->pcdata->gtell[0]		= ( BRIGHT );
    ch->pcdata->gtell[1]		= ( YELLOW );
    ch->pcdata->gtell[2]		= 1;
    ch->pcdata->sayt[0]		= ( NORMAL );
    ch->pcdata->sayt[1]		= ( GREEN );
    ch->pcdata->sayt[2]		= 0;
    ch->pcdata->room_exits[0]	= ( NORMAL );
    ch->pcdata->room_exits[1]	= ( GREEN );
    ch->pcdata->room_exits[2]	= 0;
    ch->pcdata->room_things[0]	= ( NORMAL );
    ch->pcdata->room_things[1]	= ( WHITE );
    ch->pcdata->room_things[2]	= 0;
    ch->pcdata->room_people[0]	= ( NORMAL );
    ch->pcdata->room_people[1]	= ( WHITE );
    ch->pcdata->room_people[2]	= 0;
    ch->pcdata->prompt[0]	= ( NORMAL );
    ch->pcdata->prompt[1]	= ( WHITE );
    ch->pcdata->prompt[2]	= 0;
    ch->pcdata->room[0]	= ( NORMAL );
    ch->pcdata->room[1]	= ( WHITE );
    ch->pcdata->room[2]	= 0;
    ch->pcdata->bondtalk[0]	= ( BRIGHT );
    ch->pcdata->bondtalk[1]	= ( WHITE );
    ch->pcdata->bondtalk[2]	= 0;
    ch->pcdata->channel_name[0]	= ( BRIGHT );
    ch->pcdata->channel_name[1]	= ( WHITE );
    ch->pcdata->channel_name[2]	= 0;
    ch->pcdata->wiznet[0]	= ( BRIGHT );
    ch->pcdata->wiznet[1]	= ( WHITE );
    ch->pcdata->wiznet[2]	= 0;
    ch->pcdata->fight_death[0]	= ( BRIGHT );
    ch->pcdata->fight_death[1]	= ( RED );
    ch->pcdata->fight_death[2]	= 0;
    ch->pcdata->fight_yhit[0]	= ( NORMAL );
    ch->pcdata->fight_yhit[1]	= ( GREEN );
    ch->pcdata->fight_yhit[2]	= 0;
    ch->pcdata->fight_ohit[0]	= ( NORMAL );
    ch->pcdata->fight_ohit[1]	= ( YELLOW );
    ch->pcdata->fight_ohit[2]	= 0;
    ch->pcdata->fight_thit[0]	= ( NORMAL );
    ch->pcdata->fight_thit[1]	= ( RED );
    ch->pcdata->fight_thit[2]	= 0;
    ch->pcdata->fight_skill[0]	= ( BRIGHT );
    ch->pcdata->fight_skill[1]	= ( WHITE );
    ch->pcdata->fight_skill[2]	= 0;

    ch->pcdata->sguild_talk[0] = ( BRIGHT );
    ch->pcdata->sguild_talk[1] = ( BLUE );
    ch->pcdata->sguild_talk[2] = 0;

    ch->pcdata->ssguild_talk[0] = ( BRIGHT );
    ch->pcdata->ssguild_talk[1] = ( CYAN );
    ch->pcdata->ssguild_talk[2] = 0;

    ch->pcdata->leader_talk[0] = ( BRIGHT );
    ch->pcdata->leader_talk[1] = ( RED );
    ch->pcdata->leader_talk[2] = 0;

    ch->pcdata->oguild_talk[0] = ( BRIGHT );
    ch->pcdata->oguild_talk[1] = ( YELLOW );
    ch->pcdata->oguild_talk[2] = 0;

    
    found = FALSE;
    fclose( fpReserve );

    
#if defined(unix)
    /* decompress if .gz file exists */
    sprintf( strsave, "%s%s%s", PLAYER_DIR, capitalize(name),".gz");

    if ( ( fp = fopen( strsave, "r" ) ) != NULL ) {
	 fclose(fp);
	 sprintf(buf,"gzip -dfq %s",strsave);
	 system(buf);
    }
#endif

    if (disguise) {
	 sprintf( strsave, "%s%s", PLAYER_DISGUISE_DIR, capitalize( name ) );
    }
    else {
	 sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( name ) );
    }

    if ( ( fp = fopen( strsave, "r" ) ) != NULL ) {
	 int iNest;
	 
	for ( iNest = 0; iNest < MAX_NEST; iNest++ )
	    rgObjNest[iNest] = NULL;

	found = TRUE;
	for ( ; ; )
	{
	    char letter;
	    char *word;

	    letter = fread_letter( fp );
	    if ( letter == '*' )
	    {
		fread_to_eol( fp );
		continue;
	    }

	    if ( letter != '#' )
	    {
		bug( "Load_char_obj: # not found.", 0 );
		break;
	    }

	    word = fread_word( fp );
	    if      ( !str_cmp( word, "PLAYER" ) ) fread_char ( ch, fp );
	    else if ( !str_cmp( word, "OBJECT" ) ) fread_obj  ( ch, fp );
	    else if ( !str_cmp( word, "O"      ) ) fread_obj  ( ch, fp );
	    else if ( !str_cmp( word, "PET"    ) ) fread_pet  ( ch, fp );
	    else if ( !str_cmp( word, "END"    ) ) break;
	    else
	    {
		bug( "Load_char_obj: bad section.", 0 );
		break;
	    }
	}
	fclose( fp );
    }

    fpReserve = fopen( NULL_FILE, "r" );

    /* initialize race */
    if (found)
    {
	int i;

	if (ch->race == 0)
	    ch->race = race_lookup("human");

	ch->size = race_table[ch->race].size;
	ch->dam_type = 17; /*punch */

	for (i = 0; i < 5; i++)
	{
	    if (race_table[ch->race].skills[i] == NULL)
	    	break;
	    group_add(ch,race_table[ch->race].skills[i],FALSE);
	}
	ch->affected_by = ch->affected_by|race_table[ch->race].aff;
	ch->imm_flags	= ch->imm_flags | race_table[ch->race].imm;
	ch->res_flags	= ch->res_flags | race_table[ch->race].res;
	ch->vuln_flags	= ch->vuln_flags | race_table[ch->race].vuln;
	ch->form	= race_table[ch->race].form;
	ch->parts	= race_table[ch->race].parts;
    }

	
    /* RT initialize skills */

    if (found && ch->version < 2)  /* need to add the new skills */
    {
	group_add(ch,"rom basics",FALSE);
	group_add(ch,class_table[ch->class].base_group,FALSE);
	group_add(ch,class_table[ch->class].default_group,TRUE);
	ch->pcdata->learned[gsn_recall] = 50;
    }
 
    /* fix levels */
    if (found && ch->version < 3 && (ch->level > 35 || ch->trust > 35))
    {
	switch (ch->level)
	{
	    case(40) : ch->level = 100;	break;  /* imp -> imp */
	    case(39) : ch->level = 98; 	break;	/* god -> supreme */
	    case(38) : ch->level = 96;  break;	/* deity -> god */
	    case(37) : ch->level = 93;  break;	/* angel -> demigod */
	}

        switch (ch->trust)
        {
            case(40) : ch->trust = 100;  break;	/* imp -> imp */
            case(39) : ch->trust = 98;  break;	/* god -> supreme */
            case(38) : ch->trust = 96;  break;	/* deity -> god */
            case(37) : ch->trust = 93;  break;	/* angel -> demigod */
            case(36) : ch->trust = 91;  break;	/* hero -> hero */
        }
    }

    /* ream gold */
    if (found && ch->version < 4)
    {
	ch->gold   /= 100;
    }

    return found;
}



/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    free_string(field);			\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

void fread_char( CHAR_DATA *ch, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;
    //int count = 0;
    int sheath_count = 0;
    int ignore_count = 0;
    int disguise_count = 0;
    int alias_count = 0;
    int lastlogoff = current_time;
    int percent;
    int t_int;

    sprintf(buf,"Loading %s.",ch->name);
    log_string(buf);

    for ( ; ; )
    {
	word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;

	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'A':
	    KEY( "Act",		ch->act,		fread_flag( fp ) );
	    KEY( "Act2",	ch->act2,		fread_flag( fp ) );
	    KEY( "AutoAct",      ch->auto_act,  fread_flag( fp ) );
	    KEY( "App",          ch->app,       fread_flag( fp ) );
	    KEY( "AffectedBy",	ch->affected_by,	fread_flag( fp ) );
	    KEY( "AfBy",	ch->affected_by,	fread_flag( fp ) );
	    KEY( "Alignment",	ch->alignment,		fread_number( fp ) );
	    KEY( "Alig",	ch->alignment,		fread_number( fp ) );
	    KEY( "ArrCnt",	ch->arrow_count,	fread_number( fp ) );
	    KEY( "AuraDesc", ch->aura_description, fread_string( fp ) );
	    KEY( "Autoholding", ch->autoholding,        fread_number( fp ) );

	    if (!str_cmp( word, "Alia"))
	    {
		if (alias_count >= MAX_ALIAS)
		{
		    fread_to_eol(fp);
		    fMatch = TRUE;
		    break;
		}

		ch->pcdata->alias[alias_count] 	= str_dup(fread_word(fp));
		ch->pcdata->alias_sub[alias_count]	= str_dup(fread_word(fp));
		alias_count++;
		fMatch = TRUE;
		break;
	    }

            if (!str_cmp( word, "Alias"))
            {
                if (alias_count >= MAX_ALIAS)
                {
                    fread_to_eol(fp);
                    fMatch = TRUE;
                    break;
                }
 
                ch->pcdata->alias[alias_count]        = str_dup(fread_word(fp));
                ch->pcdata->alias_sub[alias_count]    = fread_string(fp);
                alias_count++;
                fMatch = TRUE;
                break;
            }

	    if (!str_cmp( word, "AC") || !str_cmp(word,"Armor"))
	    {
		fread_to_eol(fp);
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word,"ACs"))
	    {
		int i;

		for (i = 0; i < 4; i++)
		    ch->armor[i] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word, "AffD")) {
		 AFFECT_DATA *paf;
		 int sn;

		 paf = new_affect();
		 
		 sn = skill_lookup(fread_word(fp));
		 if (sn < 0)
		   bug("Fread_char: unknown skill.",0);
		 else
		   paf->type = sn;

		 paf->level	= fread_number( fp );
		 paf->duration	= fread_number( fp );
		 paf->modifier	= fread_number( fp );
		 paf->location	= fread_number( fp );
		 paf->bitvector	= fread_number( fp );
		 paf->next	     = ch->affected;
		 ch->affected	= paf;
		 fMatch = TRUE;
		 break;
	    }

	    if (!str_cmp(word, "Affc")) {
		 AFFECT_DATA *paf;
		 int sn;
		 
		 paf = new_affect();
		 
		 sn = skill_lookup(fread_word(fp));
		 if (sn < 0)
		   bug("Fread_char: unknown skill.",0);
		 else
		   paf->type = sn;
		 
		 paf->where  = fread_number(fp);
		 paf->level      = fread_number( fp );
		 paf->duration   = fread_number( fp );
		 paf->modifier   = fread_number( fp );
		 paf->location   = fread_number( fp );
		 paf->bitvector  = fread_number( fp );
		 paf->casterId   = fread_number( fp );
		 paf->next       = ch->affected;
		 ch->affected    = paf;
		 fMatch = TRUE;
		 break;
	    }

	    if ( !str_cmp( word, "Appearance" )  || !str_cmp( word, "appearance")) {
	      ch->pcdata->appearance = fread_string( fp );
	      if (ch->pcdata->appearance[0] != '.' && ch->pcdata->appearance[0] != ',' 
		  &&  ch->pcdata->appearance[0] != '!' && ch->pcdata->appearance[0] != '?') {
		sprintf( buf, "%s", ch->pcdata->appearance );
		free_string( ch->pcdata->appearance );
		ch->pcdata->appearance = str_dup( buf );
	      }
	      fMatch = TRUE;
	      break;
	    }

	    if ( !str_cmp( word, "AttrMod"  ) || !str_cmp(word,"AMod"))
	    {
		int stat;
		for (stat = 0; stat < MAX_STATS; stat ++)
		   ch->mod_stat[stat] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "AttrPerm" ) || !str_cmp(word,"Attr"))
	    {
		int stat;

		for (stat = 0; stat < MAX_STATS; stat++)
		    ch->perm_stat[stat] = fread_number(fp);
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'B':
         KEY( "Backup", ch->pcdata->last_backup, fread_number( fp ));
	    KEY( "Bamfin",	ch->pcdata->bamfin,	fread_string( fp ) );
	    KEY( "Bamfout",	ch->pcdata->bamfout,	fread_string( fp ) );
	    KEY( "Bin",		ch->pcdata->bamfin,	fread_string( fp ) );
	    KEY( "Bonded", 	ch->pcdata->bondedby,	fread_string( fp ) );
	    KEY( "BondCount", 	ch->pcdata->bondcount,	fread_number( fp ) );
	    KEY( "BondedSex", 	ch->pcdata->bondedbysex,fread_number( fp ) );
	    KEY( "Bounty", 	ch->pcdata->bounty,	fread_number( fp ) );
	    KEY( "Bout",	ch->pcdata->bamfout,	fread_string( fp ) );
	    KEY( "Burnout",     ch->burnout,		fread_number( fp ) );
	    KEY( "BMtime", 	ch->pcdata->next_bmtrain,	fread_number( fp ) );

	    /* Read in board status */
	    if (!str_cmp(word, "Boards" ))
	    {
		int i,num = fread_number (fp); /* number of boards saved */
                char *boardname;

                for (; num ; num-- ) /* for each of the board saved */
                {
		    boardname = fread_word (fp);
		    i = board_lookup (boardname); /* find board number */

		    if (i == BOARD_NOTFOUND) /* Does board still exist ? */
                    {
			sprintf (buf, "fread_char: %s had unknown board name: %s. Skipped.",
			    ch->name, boardname);
			log_string (buf);
			fread_number (fp); /* read last_note and skip info */
		    }
		    else /* Save it */
			ch->pcdata->last_note[i] = fread_number (fp);
		} /* for */

                fMatch = TRUE;
	      } /* Boards */
	    break;

	case 'C':
	  KEY( "CAtime",		ch->pcdata->next_createangreal,		fread_number( fp ) );
	  KEY( "CA24time",		ch->pcdata->next_24hourangreal,		fread_number( fp ) );
	  KEY( "CA24amount",		ch->pcdata->createangrealcount,		fread_number( fp ) );
	  if (!str_cmp( word, "ChanFlag")) {
	     ch->chan_flags = 0;
	     KEY( "ChanFlag",      ch->chan_flags,  fread_flag( fp ) );
	  }
	  KEY( "Class",	ch->class,		fread_number( fp ) );
	  KEY( "Cla",		ch->class,		fread_number( fp ) );
	  KEY( "Clan",	ch->clan,	clan_lookup(fread_string(fp)));
	  KEY( "Comm",	ch->comm,		fread_flag( fp ) ); 
	  KEY( "Comm2", ch->comm2,              fread_flag( fp ) );
	  
	  if ( !str_cmp( word, "Condition" ) || !str_cmp(word,"Cond")) {
	    ch->pcdata->condition[0] = fread_number( fp );
	    ch->pcdata->condition[1] = fread_number( fp );
	    ch->pcdata->condition[2] = fread_number( fp );
	    ch->pcdata->condition[0] = -1;
	    ch->pcdata->condition[1] = -1;
	    ch->pcdata->condition[2] = -1;
	    fMatch = TRUE;
	    break;
	    }

	  if (!str_cmp(word,"Cnd")) {
	    ch->pcdata->condition[0] = fread_number( fp );
	    ch->pcdata->condition[1] = fread_number( fp );
	    ch->pcdata->condition[2] = fread_number( fp );
	    ch->pcdata->condition[3] = fread_number( fp );
	    ch->pcdata->condition[0] = -1;
	    ch->pcdata->condition[1] = -1;
	    ch->pcdata->condition[2] = -1;
	    ch->pcdata->condition[3] = -1;
	    fMatch = TRUE;
	    break;
	  }
	  
	  if( !str_cmp( word, "Coloura" ) ) {
	         LOAD_COLOUR( auction )
		 LOAD_COLOUR( gossip )
		 LOAD_COLOUR( music )
		 LOAD_COLOUR( chat )
		 LOAD_COLOUR( minion )
		 fMatch = TRUE;
	    break;
	  }
	  
	  if( !str_cmp( word, "Colourb" ) ) {
	    LOAD_COLOUR( immtalk )
		 LOAD_COLOUR( game )
		 LOAD_COLOUR( tell )
		 LOAD_COLOUR( reply )
		 LOAD_COLOUR( gtell )
		 fMatch = TRUE;
	    break;
	  }

	  if( !str_cmp( word, "Colourc" ) ) {
	    LOAD_COLOUR( room_exits )
		 LOAD_COLOUR( room_things )
		 LOAD_COLOUR( prompt )
		 LOAD_COLOUR( room_people )
		 LOAD_COLOUR( room )
		 fMatch = TRUE;
		break;
	  }

	  if( !str_cmp( word, "Colourd" ) ) {
	    LOAD_COLOUR( bondtalk )
		 LOAD_COLOUR( channel_name )
		 LOAD_COLOUR( wiznet )
		 LOAD_COLOUR( fight_death )
		 LOAD_COLOUR( fight_yhit )
		 fMatch = TRUE;
	    break;
	  }

	  if( !str_cmp( word, "Coloure" ) ) {
		 LOAD_COLOUR( fight_ohit )
		 LOAD_COLOUR( fight_thit )
		 LOAD_COLOUR( fight_skill )
	    	 LOAD_COLOUR( sayt )
	    	 LOAD_COLOUR( osay )
		 fMatch = TRUE;
	    break;
	  }

	  if( !str_cmp( word, "Colourf" ) ) {
		 LOAD_COLOUR( wolfkin_talk )
		 LOAD_COLOUR( guild_talk )
		 LOAD_COLOUR( race_talk )
	    	 LOAD_COLOUR( df_talk )
	    	 LOAD_COLOUR( newbie )
		 fMatch = TRUE;
	    break;
	  }
	  
	  if( !str_cmp( word, "Colourg" ) ) {
	    LOAD_COLOUR(sguild_talk);
	    LOAD_COLOUR(ssguild_talk);
	    LOAD_COLOUR(leader_talk);	    
	    fMatch = TRUE;
	    break;
	  }

          if( !str_cmp( word, "Colourh" ) ) {             
             LOAD_COLOUR(oguild_talk);
             t_int = fread_number( fp );
             t_int = fread_number( fp );
             t_int = fread_number( fp );
             t_int = fread_number( fp );
             fMatch = TRUE;
	     break;
          }

	  if (!str_cmp(word, "CSphere")) {
	    sh_int sphere=0;

	    for (sphere = 0; sphere < MAX_SPHERE; sphere++)
		 ch->cre_sphere[sphere] = fread_number(fp);
	    fMatch = TRUE;
	    break;
	  }
	  
	  break;

	case 'D':
	    KEY( "Damroll",	ch->damroll,		fread_number( fp ) );
	    KEY( "Dam",		ch->damroll,		fread_number( fp ) );
	    KEY( "Defend",  ch->defend_loc,          fread_number(fp));
	    KEY( "Description",	ch->description,	fread_string( fp ) );
	    KEY( "Desc",	ch->description,	fread_string( fp ) );
	    KEY( "DFName",	ch->pcdata->df_name,	fread_string( fp ) );
	    KEY( "DFLevel",	ch->pcdata->df_level,	fread_number( fp ) );
	    KEY( "DUtime", 	ch->pcdata->next_dutrain,	fread_number( fp ) );	    
	    KEY( "DAppearance", ch->pcdata->dreaming_appearance, fread_string(fp));
	    
	    if (!str_cmp( word, "Disguise")) {
		 if (disguise_count >= MAX_DISGUISE) {
		   fread_to_eol(fp);
		   fMatch = TRUE;
		   break;
		 }
		 
		 ch->pcdata->disguise[disguise_count] = fread_string(fp);
		 disguise_count++;
		 fMatch = TRUE;
		 break;
	    }

	    break;

	case 'E':
	    if ( !str_cmp( word, "End" ) )
	    {
    		/* adjust hp endurance move up  -- here for speed's sake */
    		percent = (current_time - lastlogoff) * 25 / ( 2 * 60 * 60);

		percent = UMIN(percent,100);
 
    		if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
    		&&  !IS_AFFECTED(ch,AFF_PLAGUE) && !IS_AFFECTED(ch,AFF_SUFFOCATING))
    		{
        	    ch->hit	+= (ch->max_hit - ch->hit) * percent / 100;
        	    ch->endurance    += (ch->max_endurance - ch->endurance) * percent / 100;
    		}
		return;
	    }
	    KEY( "Exp",		ch->exp,		fread_number( fp ) );
	    KEY( "Email",       ch->pcdata->email,      fread_string( fp ) );
	    KEY( "ELevl",       ch->pcdata->extended_level, fread_number( fp ));
	    break;

	case 'F':
	  KEY( "Flaws", ch->flaws, fread_flag( fp ) );
	  KEY( "ForceInsanity", ch->pcdata->forceinsanity, fread_number( fp ) );
	  KEY( "ForceSpark", ch->pcdata->forcespark, fread_number( fp ) );
//	  KEY( "Forsaken", ch->forsaken, fread_flag( fp ) );
	  if (!str_cmp(word, "Forsaken")) {
	    SET_BIT(ch->ic_flags, IC_FORSAKEN);
	  }
	  break;
	case 'G':
	  KEY( "Gtitle", ch->gtitle,	fread_string( fp ));
	  KEY( "Ginvis", ch->ginvis,    fread_number( fp ));
	  KEY( "Gmute", ch->gmute,    fread_number( fp ));
	  KEY( "Gold",	ch->gold,		fread_number( fp ) );
	  KEY ("Gold_bank", ch->pcdata->gold_bank, fread_number(fp));
	  if (!str_cmp(word, "GateKey") )
          {
		char * tmpStr = fread_string( fp );
		sprintf(buf,"Read Gatekey: %s - calling do_addkey",tmpStr);
		log_string(buf);
		do_addkey(ch,tmpStr);
	  }

	  if ( !str_cmp( word, "Group" )  || !str_cmp(word,"Gr")) {
	    int gn;
	    char *temp;
 
	    temp = fread_word( fp ) ;
	    gn = group_lookup(temp);
	    /* gn    = group_lookup( fread_word( fp ) ); */
	    if ( gn < 0 ) {
		 fprintf(stderr,"%s",temp);
		 bug( "Fread_char: unknown group. ", 0 );
	    }
	    else
		 gn_add(ch,gn);
	    fMatch = TRUE;
	  }
	  break;

	case 'H':
	    KEY( "HAppearance", ch->pcdata->hood_appearance, fread_string(fp));
	    KEY( "HDesc",	ch->hood_description,	fread_string( fp ) );
	    KEY( "Hitroll",	ch->hitroll,		fread_number( fp ) );
	    KEY( "Hit",		ch->hitroll,		fread_number( fp ) );
	    KEY( "Home", 	ch->pcdata->home,       fread_number( fp ) );
	    KEY( "Holding", ch->holding,        fread_number( fp ) );

	    if ( !str_cmp( word, "HpEnduranceMove" ) || !str_cmp(word,"HMV"))
	    {
		ch->hit		= fread_number( fp );
		ch->max_hit	= fread_number( fp );
		ch->endurance	= fread_number( fp );
		ch->max_endurance	= fread_number( fp );
		fMatch = TRUE;
		break;
	    }

            if ( !str_cmp( word, "HpEnduranceMovePerm" ) || !str_cmp(word,"HMVP"))
            {
                ch->pcdata->perm_hit	= fread_number( fp );
                ch->pcdata->perm_endurance   = fread_number( fp );
                fMatch = TRUE;
                break;
            }
      
		  if ( !str_cmp( word, "Hloc")) {
		    int loc;
		    for (loc = 0; loc < MAX_HIT_LOC; loc++)
			 ch->hit_loc[loc] = fread_number(fp);
		    fMatch = TRUE;
		    break;
		  }

	    break;

	case 'I':
         KEY( "ICflags", ch->ic_flags,  fread_flag( fp ) );
	    KEY( "Id",		ch->id,			fread_number( fp ) );
	    KEY( "InvisLevel",	ch->invis_level,	fread_number( fp ) );
	    KEY( "Inco",	ch->incog_level,	fread_number( fp ) );
	    KEY( "Invi",	ch->invis_level,	fread_number( fp ) );
	    KEY( "Insanity",	ch->insanity_points,	fread_number( fp ) );
	    KEY( "ICLocRecall",	ch->pcdata->iclocrecall, fread_number( fp ) );

	    if (!str_cmp( word, "Ignore")) {
		 if (ignore_count >= MAX_IGNORE) {
		   fread_to_eol(fp);
		   fMatch = TRUE;
		   break;
		 }
               
		 ch->pcdata->ignore[ignore_count] = fread_string(fp);
		 ignore_count++;
		 fMatch = TRUE;
		 break;
	    }

	    if ( !str_cmp( word, "ICTitle" )  || !str_cmp( word, "ICtitl"))
	    {
		ch->pcdata->ictitle = fread_string( fp );
    		if (!IS_NULLSTR(ch->pcdata->ictitle) && ch->pcdata->ictitle[0] != '.' && ch->pcdata->ictitle[0] != ',' 
		&&  ch->pcdata->ictitle[0] != '!' && ch->pcdata->ictitle[0] != '?')
		{
		    sprintf( buf, "%s", ch->pcdata->ictitle );
		    free_string( ch->pcdata->ictitle );
		    ch->pcdata->ictitle = str_dup( buf );
		}
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "ImmInfo" )  || !str_cmp( word, "Imm_Info")) {
	      ch->pcdata->imm_info = fread_string( fp );
	      if (!IS_NULLSTR(ch->pcdata->imm_info) && ch->pcdata->imm_info[0] != '.' && ch->pcdata->imm_info[0] != ',' 
		  &&  ch->pcdata->imm_info[0] != '!' && ch->pcdata->imm_info[0] != '?') {
		sprintf( buf, "%s", ch->pcdata->imm_info );
		free_string( ch->pcdata->imm_info );
		ch->pcdata->imm_info = str_dup( buf );
                if (ch->level == LEVEL_HERO)
                {  
		    sprintf( buf, "HERO");
		    free_string( ch->pcdata->imm_info );
		    ch->pcdata->imm_info = str_dup( buf );
                } 
		/*
                if ((ch->level > LEVEL_HERO) && (ch->level < (LEVEL_ADMIN - 1)))
                {  
		    sprintf( buf, "BUILDER");
		    free_string( ch->pcdata->imm_info );
		    ch->pcdata->imm_info = str_dup( buf );
                } 
		*/
	      }
	      fMatch = TRUE;
	      break;
	    }
	    break;

	case 'K':
	  /* Check for the right keyword */
	  if ( !str_cmp( word, "Know" ) )
	    {
		 int id;
		 char *temp;
		 
		 id = fread_number( fp );
		 temp = fread_word( fp );
		 add_know( ch, id, temp );
		 fMatch = TRUE;
		 break;
	    }
	  break;
	  
	case 'L':
	    KEY( "Lastlog",	ch->pcdata->lastlog,    fread_string( fp ) );
	    //KEY( "Lastsite",	ch->pcdata->lastsite,  fread_string( fp ) );
	    KEY( "LastLevel",	ch->pcdata->last_level, fread_number( fp ) );
	    KEY( "LLev",	ch->pcdata->last_level, fread_number( fp ) );
	    KEY( "Level",	ch->level,		fread_number( fp ) );
	    KEY( "Lev",		ch->level,		fread_number( fp ) );
	    KEY( "Levl",	ch->level,		fread_number( fp ) );
	    KEY( "LogO",	lastlogoff,		fread_number( fp ) );
	    KEY( "LongDescr",	ch->long_descr,		fread_string( fp ) );
	    KEY( "LnD",		ch->long_descr,		fread_string( fp ) );
	    break;

	case 'M':
	  KEY  ( "MAppearance", ch->pcdata->masquerade_appearance, fread_string(fp));
	  KEY  ( "MaxBurnout", ch->max_burnout,   fread_number( fp ) );
	  KEY  ( "Minion",   ch->minion,          fread_number( fp ) );
	  KEY  ( "Mname",    ch->mname,           fread_string( fp ) );
	  KEY  ( "Mrank",    ch->mrank,           fread_number( fp ) );
	  KEY  ( "Mtitle",   ch->mtitle,          fread_string( fp ) );
	  
	  KEY( "MSphere",   ch->main_sphere,     fread_number( fp ) );
	  KEY( "Merits",    ch->merits,          fread_flag( fp ) );
	  break;
	case 'N':
	    KEYS( "Name",	ch->name,		fread_string( fp ) );
	    break;
	case 'O':
	  KEY( "OGuild",  ch->oguild,       clan_lookup(fread_string(fp)));
	  KEY( "OGrank",  ch->oguild_rank,  fread_number(fp));	  
	  KEY( "OGtitle", ch->oguild_title, fread_string(fp));
	  KEY( "OGinvis", ch->oguild_invis, fread_number(fp));
	  KEY( "OGmute",  ch->oguild_mute,  fread_number( fp ));
	  KEY( "Offe",	ch->off_flags,		fread_flag( fp ) );
	  break;
	case 'P':
	    KEY( "Password",	ch->pcdata->pwd,	fread_string( fp ) );
	    KEY( "Pass",	ch->pcdata->pwd,	fread_string( fp ) );
	    KEY( "Played",	ch->played,		fread_number( fp ) );
	    KEY( "Plyd",	ch->played,		fread_number( fp ) );
	    KEY( "PKCount",	ch->pk_count,		fread_number( fp ) );
	    KEY( "PKDiedCount",	ch->pk_died_count,	fread_number( fp ) );
	    KEY( "Points",	ch->pcdata->points,	fread_number( fp ) );
	    KEY( "Pnts",	ch->pcdata->points,	fread_number( fp ) );
	    KEY( "Polls",	ch->pcdata->polls,	fread_string( fp ) );
	    KEY( "Position",	ch->position,		fread_number( fp ) );
	    KEY( "Pos",		ch->position,		fread_number( fp ) );
/*
	    KEY( "Practice",	ch->practice,		fread_number( fp ) );
	    KEY( "Prac",	ch->practice,		fread_number( fp ) );
*/

	    KEYS( "Prompt",  ch->prompt,         fread_string( fp ) );
 	    KEYS( "Prom",	   ch->prompt,		     fread_string( fp ) );
	    
	    break;

	case 'Q':
	    KEY( "Quest_Curr",   ch->pcdata->quest_curr, fread_number( fp ) );
   	    KEY( "Quest_Accum",  ch->pcdata->quest_accum, fread_number( fp ) );
	    KEY( "QuestNext",   ch->pcdata->nextquest,    fread_number( fp ));
	    KEY( "QuestMob",   ch->pcdata->questmob,    fread_number( fp ));
	    KEY( "QuestObj",   ch->pcdata->questobj,    fread_number( fp ));
            if (!str_cmp(word, "QuestGiverVnum"))
            {
	    	ch->pcdata->questgivervnum = fread_number( fp );
		CHAR_DATA * giver = NULL;
  		for ( giver = char_list; giver != NULL; giver = giver->next) {
			if (!IS_NPC(giver))
				continue;
			if (giver->pIndexData->vnum == ch->pcdata->questgivervnum)
			   break;
		}
		if (giver != NULL)
		{
			ch->pcdata->questgiver = giver;	
			ch->pcdata->countdown = ch->pcdata->nextquest;
			ch->pcdata->nextquest = 0;
		}
		else
		{
			ch->pcdata->questmob = 0;
			ch->pcdata->questobj = 0;
			ch->pcdata->questgivervnum = 0;
			ch->pcdata->questgiver = NULL;	
		}
            }
	    break;

	case 'R':
	  KEY( "RName",  ch->real_name,         fread_string( fp ) );
	    KEY( "Race",        ch->race,	
				race_lookup(fread_string( fp )) );

	    KEY( "Rank",        ch->rank, fread_number( fp ) );
	    KEY( "RewMult",     ch->pcdata->reward_multiplier, fread_number( fp ) );
	    KEY( "RewTime",     ch->pcdata->reward_time, fread_number( fp ) );
	    KEY( "RestMsg",  	ch->real_name,         fread_string( fp ) );

	    if ( !str_cmp( word, "Room" ) ) {
		 ch->in_room = get_room_index( fread_number( fp ) );
		 if ( ch->in_room == NULL )
		   ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
		 fMatch = TRUE;
		 break;
	    }
	    KEY( "RPTimer", ch->pcdata->rprewardtimer, fread_number( fp ) );
	    KEY( "RPBonus", ch->pcdata->rpbonus, fread_number( fp ) );
	    KEY( "RolePlayed",	ch->roleplayed,	 fread_number( fp ) );

	    break;

	case 'S':
	  KEY( "Skim_to", ch->skim_to, fread_number(fp));
	  KEY( "Sguild", ch->sguild, sguild_lookup(fread_string(fp)));
	  KEY( "Srank", ch->sguild_rank, fread_number(fp));	  
	  KEY( "Stitle", ch->sguild_title, fread_string(fp));
	  KEY( "Sinvis", ch->sguild_invis, fread_number(fp));

	  KEY( "SSguild", ch->ssguild, ssguild_lookup(fread_string(fp)));
	  KEY( "SSrank", ch->ssguild_rank, fread_number(fp));	  
	  KEY( "SStitle", ch->ssguild_title, fread_string(fp));
	  KEY( "SSinvis", ch->ssguild_invis, fread_number(fp));

	  KEY( "SavingThrow", ch->saving_throw,	fread_number( fp ) );
	  KEY( "Save",	    ch->saving_throw,	fread_number( fp ) );
	  KEY( "Scro",	    ch->lines,		     fread_number( fp ) );
	  KEY( "SDtime", 	ch->pcdata->next_sdtrain,	fread_number( fp ) );
	  KEY( "Sex",	    ch->sex,		     fread_number( fp ) );
	  KEY( "ShortDescr",  ch->short_descr,	fread_string( fp ) );
	  KEY( "ShD",	    ch->short_descr,	fread_string( fp ) );
	  KEY( "Sec",         ch->pcdata->security,fread_number( fp ) );	/* OLC */
	  KEY( "Silv",        ch->silver,          fread_number( fp ) );
	  KEY( "Silv_bank", ch->pcdata->silver_bank,  fread_number(fp));
	  KEY( "Spheretrain", ch->pcdata->spheretrain,	fread_number(fp));

	    if (!str_cmp( word, "SheatWName")) {
		 if (sheath_count >= MAX_SHEAT_LOC) {
		   fread_to_eol(fp);
		   fMatch = TRUE;
		   break;
		 }
		 
		 ch->sheat_where_name[sheath_count] = fread_string(fp);
		 sheath_count++;
		 fMatch = TRUE;
		 break;
	    }

	    if ( !str_cmp( word, "Skill" ) || !str_cmp(word,"Sk"))
	    {
		int sn;
		int value;
		char *temp;

		value = fread_number( fp );
		temp = fread_word( fp ) ;
		sn = skill_lookup(temp);
		/* sn    = skill_lookup( fread_word( fp ) ); */
		if ( sn < 0 )
		{
		    fprintf(stderr,"%s",temp);
		    bug( "Fread_char: unknown skill. ", 0 );
		}
		else
		    ch->pcdata->learned[sn] = value;
		fMatch = TRUE;
		break;
	    }

	    if (!str_cmp(word, "Sphere")) {
		 sh_int sphere=0;

		 for (sphere = 0; sphere < MAX_SPHERE; sphere++)
		   ch->perm_sphere[sphere] = fread_number(fp);
		 fMatch = TRUE;
		 break;
	    }
	    break;

	case 'T':
	  KEY( "Target", ch->target_loc, fread_number(fp));
	  KEY( "TrueSex",     ch->pcdata->true_sex,  	fread_number( fp ) );
	  KEY( "TSex",	ch->pcdata->true_sex,   fread_number( fp ) );
	  KEY( "Trai",	ch->train,		fread_number( fp ) );
	  KEY( "Trust",	ch->trust,		fread_number( fp ) );
	  KEY( "Tru",		ch->trust,		fread_number( fp ) );
	  KEY( "Talents",   ch->talents,        fread_flag( fp ) );
	  KEY( "Timeout",   ch->pcdata->timeoutstamp,        fread_number( fp ) );
	  
	  // Trolloc kill counters
	  KEY( "TR_ClanKill", ch->pcdata->clan_kill_cnt, fread_number( fp ) );
	  KEY( "TR_PCKill", ch->pcdata->pc_kill_cnt, fread_number( fp ) );

	  if ( !str_cmp( word, "Title" )  || !str_cmp( word, "Titl")) {
	    ch->pcdata->title = fread_string( fp );
	    if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ',' 
		   &&  ch->pcdata->title[0] != '!' && ch->pcdata->title[0] != '?') {
		 sprintf( buf, " %s", ch->pcdata->title );
		 free_string( ch->pcdata->title );
		 ch->pcdata->title = str_dup( buf );
		}
	    fMatch = TRUE;
	    break;
	  }
	  break;

	case 'V':
	  KEY( "VAppearance", ch->pcdata->veil_appearance, fread_string(fp));
	  KEY( "VDesc",	ch->veil_description,	fread_string( fp ) );		
	  KEY( "VoiceSay", ch->pcdata->say_voice, fread_string(fp));
	  KEY( "VoiceAsk", ch->pcdata->ask_voice, fread_string(fp));
	  KEY( "VoiceExc", ch->pcdata->exclaim_voice, fread_string(fp));
	  KEY( "VoiceBattle", ch->pcdata->battlecry_voice, fread_string(fp));

	    KEY( "Version",     ch->version,		fread_number ( fp ) );
	    KEY( "Vers",	ch->version,		fread_number ( fp ) );
	    if ( !str_cmp( word, "Vnum" ) )
	    {
		ch->pIndexData = get_mob_index( fread_number( fp ) );
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'W':
	    KEY( "WAppearance", ch->pcdata->wolf_appearance, fread_string(fp));
	    KEY( "WDesc",	ch->wolf_description,	fread_string( fp ) );
	    KEY( "WoundDesc", ch->wound_description, fread_string( fp ) );
	    KEY( "Wimpy",	ch->wimpy,		fread_number( fp ) );
	    KEY( "Wimp",	ch->wimpy,		fread_number( fp ) );
	    KEY( "Wizn",	ch->wiznet,		fread_flag( fp ) );
	    KEY( "Wkname",  ch->wkname,         fread_string( fp ) );
	    KEY( "World",   ch->world,          fread_flag( fp ) );
	    KEY( "Wvnum",   ch->world_vnum,     fread_number( fp ) );
	    KEY( "Webvote", ch->pcdata->last_web_vote, fread_number( fp ));
	    break;
	}

	if ( !fMatch )
	{
	    bug( "Fread_char: no match.", 0 );
	    bug( word, 0 );
	    fread_to_eol( fp );
	}
    }
}

/* load a pet from the forgotten reaches */
void fread_pet( CHAR_DATA *ch, FILE *fp )
{
    char *word;
    CHAR_DATA *pet;
    bool fMatch;
    int lastlogoff = current_time;
    int percent;

    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? "END" : fread_word(fp);
    if (!str_cmp(word,"Vnum"))
    {
    	int vnum;
    	
    	vnum = fread_number(fp);
    	if (get_mob_index(vnum) == NULL)
	{
    	    bug("Fread_pet: bad vnum %d.",vnum);
	    pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
	}
    	else
    	    pet = create_mobile(get_mob_index(vnum));
    }
    else
    {
        bug("Fread_pet: no vnum in file.",0);
        pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }
    
    for ( ; ; )
    {
    	word 	= feof(fp) ? "END" : fread_word(fp);
    	fMatch = FALSE;
    	
    	switch (UPPER(word[0]))
    	{
    	case '*':
    	    fMatch = TRUE;
    	    fread_to_eol(fp);
    	    break;
    		
    	case 'A':
    	    KEY( "Act",		pet->act,		fread_flag(fp));
    	    KEY( "Act2",	pet->act2,		fread_flag(fp));
    	    KEY( "AfBy",	pet->affected_by,	fread_flag(fp));
    	    KEY( "Alig",	pet->alignment,		fread_number(fp));
	    KEY( "ArrCnt",	pet->arrow_count,	fread_number( fp ) );
    	    
    	    if (!str_cmp(word,"ACs"))
    	    {
    	    	int i;
    	    	
    	    	for (i = 0; i < 4; i++)
    	    	    pet->armor[i] = fread_number(fp);
    	    	fMatch = TRUE;
    	    	break;
    	    }
    	    
    	    if (!str_cmp(word,"AffD"))
    	    {
    	    	AFFECT_DATA *paf;
    	    	int sn;
    	    	
    	    	paf = new_affect();
    	    	
    	    	sn = skill_lookup(fread_word(fp));
    	     	if (sn < 0)
    	     	    bug("Fread_char: unknown skill.",0);
    	     	else
    	     	   paf->type = sn;
    	     	   
    	     	paf->level	= fread_number(fp);
    	     	paf->duration	= fread_number(fp);
    	     	paf->modifier	= fread_number(fp);
    	     	paf->location	= fread_number(fp);
    	     	paf->bitvector	= fread_number(fp);
    	     	paf->next	= pet->affected;
    	     	pet->affected	= paf;
    	     	fMatch		= TRUE;
    	     	break;
    	    }

            if (!str_cmp(word,"Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
 
                paf = new_affect();
 
                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.",0);
                else
                   paf->type = sn;
 
		paf->where	= fread_number(fp);
                paf->level      = fread_number(fp);
                paf->duration   = fread_number(fp);
                paf->modifier   = fread_number(fp);
                paf->location   = fread_number(fp);
                paf->bitvector  = fread_number(fp);
                paf->next       = pet->affected;
                pet->affected   = paf;
                fMatch          = TRUE;
                break;
            }
    	     
    	    if (!str_cmp(word,"AMod"))
    	    {
    	     	int stat;
    	     	
    	     	for (stat = 0; stat < MAX_STATS; stat++)
    	     	    pet->mod_stat[stat] = fread_number(fp);
    	     	fMatch = TRUE;
    	     	break;
    	    }
    	     
    	    if (!str_cmp(word,"Attr"))
    	    {
    	         int stat;
    	         
    	         for (stat = 0; stat < MAX_STATS; stat++)
    	             pet->perm_stat[stat] = fread_number(fp);
    	         fMatch = TRUE;
    	         break;
    	    }
    	    break;
    	     
    	 case 'C':
             KEY( "Clan",       pet->clan,       clan_lookup(fread_string(fp)));
    	     KEY( "Comm",	pet->comm,		fread_flag(fp));
    	     break;
    	     
    	 case 'D':
    	     KEY( "Dam",	pet->damroll,		fread_number(fp));
    	     KEY( "Desc",	pet->description,	fread_string(fp));
    	     break;
    	     
    	 case 'E':
    	     if (!str_cmp(word,"End"))
		  {
		    pet->leader = ch;
		    //pet->master = ch;
		    //ch->pet = pet;
		    if (IS_SET(pet->act,ACT_RIDEABLE))
			 {	
			   ch->mount = pet;
			   pet->mount = ch;
			 }
		    else {
			 pet->leader = ch;
			 pet->master = ch;
			 ch->pet = pet;
		    }
		/* adjust hp endurance move up  -- here for speed's sake */
    		percent = (current_time - lastlogoff) * 25 / ( 2 * 60 * 60);
 
    		if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
    		&&  !IS_AFFECTED(ch,AFF_PLAGUE) && !IS_AFFECTED(ch,AFF_SUFFOCATING))
    		{
		    percent = UMIN(percent,100);
    		    pet->hit	+= (pet->max_hit - pet->hit) * percent / 100;
        	    pet->endurance   += (pet->max_endurance - pet->endurance) * percent / 100;
    		}
    	     	return;
	     }
    	     KEY( "Exp",	pet->exp,		fread_number(fp));
    	     break;
    	     
    	 case 'G':
    	     KEY( "Gold",	pet->gold,		fread_number(fp));
    	     break;
    	     
    	 case 'H':
    	     KEY( "Hit",	pet->hitroll,		fread_number(fp));
    	     
    	     if (!str_cmp(word,"HMV"))
    	     {
    	     	pet->hit	= fread_number(fp);
    	     	pet->max_hit	= fread_number(fp);
    	     	pet->endurance	= fread_number(fp);
    	     	pet->max_endurance	= fread_number(fp);
    	     	fMatch = TRUE;
    	     	break;
    	     }
    	     break;
    	     
     	case 'L':
    	     KEY( "Levl",	pet->level,		fread_number(fp));
    	     KEY( "LnD",	pet->long_descr,	fread_string(fp));
	     KEY( "LogO",	lastlogoff,		fread_number(fp));
    	     break;
    	     
    	case 'N':
    	     KEY( "Name",	pet->name,		fread_string(fp));
    	     break;
    	     
    	case 'P':
    	     KEY( "Pos",	pet->position,		fread_number(fp));
    	     break;
    	     
	case 'R':
    	    KEY( "Race",	pet->race, race_lookup(fread_string(fp)));
	    if ( !str_cmp( word, "Room" ) ) {
		 pet->in_room = get_room_index( fread_number( fp ) );
		 if ( pet->in_room == NULL )
		   pet->in_room = get_room_index( ROOM_VNUM_LIMBO );
		 fMatch = TRUE;
		 break;
	    }
    	    break;
 	    
    	case 'S' :
    	    KEY( "Save",	pet->saving_throw,	fread_number(fp));
    	    KEY( "Sex",		pet->sex,		fread_number(fp));
    	    KEY( "ShD",		pet->short_descr,	fread_string(fp));
            KEY( "Silv",        pet->silver,            fread_number( fp ) );
    	    break;
    	    
    	if ( !fMatch )
    	{
    	    bug("Fread_pet: no match.",0);
    	    fread_to_eol(fp);
    	}
    	
    	}
    }
}

extern	OBJ_DATA	*obj_free;

void fread_obj( CHAR_DATA *ch, FILE *fp )
{
    OBJ_DATA *obj;
    char *word;
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;
    bool new_format;  /* to prevent errors */
    bool make_new;    /* update object */
    
    fVnum = FALSE;
    obj = NULL;
    first = TRUE;  /* used to counter fp offset */
    new_format = FALSE;
    make_new = FALSE;

    word   = feof( fp ) ? "End" : fread_word( fp );
    if (!str_cmp(word,"Vnum" ))
    {
        int vnum;
	first = FALSE;  /* fp will be in right place */
 
        vnum = fread_number( fp );
        if (  get_obj_index( vnum )  == NULL )
	{
            bug( "Fread_obj: bad vnum %d.", vnum );
	}
        else
	{
	    obj = create_object(get_obj_index(vnum),-1);
	    new_format = TRUE;
	}
	    
    }

    if (obj == NULL)  /* either not found or old style */
    {
    	obj = new_obj();
    	obj->name		= str_dup( "" );
    	obj->short_descr	= str_dup( "" );
    	obj->description	= str_dup( "" );
    	obj->owner		= str_dup( "" );
    }

    fNest		= FALSE;
    fVnum		= TRUE;
    iNest		= 0;

    for ( ; ; )
    {
	if (first)
	    first = FALSE;
	else
	    word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;

	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'A':
	    if (!str_cmp(word,"AffD"))
	    {
		AFFECT_DATA *paf;
		int sn;

		paf = new_affect();

		sn = skill_lookup(fread_word(fp));
		if (sn < 0)
		    bug("Fread_obj: unknown skill.",0);
		else
		    paf->type = sn;

		paf->level	= fread_number( fp );
		paf->duration	= fread_number( fp );
		paf->modifier	= fread_number( fp );
		paf->location	= fread_number( fp );
		paf->bitvector	= fread_number( fp );
		paf->next	= obj->affected;
		obj->affected	= paf;
		fMatch		= TRUE;
		break;
	    }
            if (!str_cmp(word,"Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
 
                paf = new_affect();
 
                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_obj: unknown skill.",0);
                else
                    paf->type = sn;
 
		paf->where	= fread_number( fp );
                paf->level      = fread_number( fp );
                paf->duration   = fread_number( fp );
                paf->modifier   = fread_number( fp );
                paf->location   = fread_number( fp );
                paf->bitvector  = fread_number( fp );
                paf->next       = obj->affected;
                obj->affected   = paf;
                fMatch          = TRUE;
                break;
            }
	    break;

	case 'C':
	    KEY( "Cond",	obj->condition,		fread_number( fp ) );
	    KEY( "Cost",	obj->cost,		fread_number( fp ) );
	    break;

	case 'D':
	    KEY( "Description",	obj->description,	fread_string( fp ) );
	    KEY( "Desc",	obj->description,	fread_string( fp ) );
	    break;

	case 'E':

	    if ( !str_cmp( word, "Enchanted"))
	    {
		obj->enchanted = TRUE;
	 	fMatch 	= TRUE;
		break;
	    }

	    KEY( "ExtraFlags",	obj->extra_flags,	fread_number( fp ) );
	    KEY( "ExtF",	obj->extra_flags,	fread_number( fp ) );

	    if ( !str_cmp( word, "ExtraDescr" ) || !str_cmp(word,"ExDe"))
	    {
		EXTRA_DESCR_DATA *ed;

		ed = new_extra_descr();

		ed->keyword		= fread_string( fp );
		ed->description		= fread_string( fp );
		ed->next		= obj->extra_descr;
		obj->extra_descr	= ed;
		fMatch = TRUE;
	    }

	    if ( !str_cmp( word, "End" ) )
	    {
		if ( !fNest || ( fVnum && obj->pIndexData == NULL ) )
		{
		    bug( "Fread_obj: incomplete object.", 0 );
		    free_obj(obj);
		    return;
		}
		else
	        {
		    if ( !fVnum )
		    {
			free_obj( obj );
			obj = create_object( get_obj_index( OBJ_VNUM_DUMMY ), 0 );
		    }

		    if (!new_format)
		    {
		    	obj->next	= object_list;
		    	object_list	= obj;
		    	obj->pIndexData->count++;
		    }

		    if (!obj->pIndexData->new_format 
		    && obj->item_type == ITEM_ARMOR
		    &&  obj->value[1] == 0)
		    {
			obj->value[1] = obj->value[0];
			obj->value[2] = obj->value[0];
		    }
		    if (make_new)
		    {
			int wear;
			
			wear = obj->wear_loc;
			extract_obj(obj);

			obj = create_object(obj->pIndexData,0);
			obj->wear_loc = wear;
		    }
		    if ( iNest == 0 || rgObjNest[iNest] == NULL )
  		    {
			if (IS_OBJ_STAT(obj,ITEM_KEEPER) || ch == NULL)
			{
		  	   obj_to_room(obj,get_room_index( g_CurrentKeeper ));
			}
			else
			{
			   obj_to_char( obj, ch );
			}
		    }
		    else
			obj_to_obj( obj, rgObjNest[iNest-1] );
		    return;
		}
	    }
	    break;

	case 'I':
	    KEY( "ItemType",	obj->item_type,		fread_number( fp ) );
	    KEY( "Ityp",	obj->item_type,		fread_number( fp ) );
	    break;

	case 'L':
	    KEY( "Level",	obj->level,		fread_number( fp ) );
	    KEY( "Lev",		obj->level,		fread_number( fp ) );
	    break;

	case 'N':
	    KEY( "Name",	obj->name,		fread_string( fp ) );

	    if ( !str_cmp( word, "Nest" ) )
	    {
		iNest = fread_number( fp );
		if ( iNest < 0 || iNest >= MAX_NEST )
		{
		    bug( "Fread_obj: bad nest %d.", iNest );
		}
		else
		{
		    rgObjNest[iNest] = obj;
		    fNest = TRUE;
		}
		fMatch = TRUE;
	    }
	    break;

   	case 'O':
	  KEY( "Owner",		obj->owner,	fread_string( fp ) );
	  if ( !str_cmp( word,"Oldstyle" ) ) {
	    if (obj->pIndexData != NULL && obj->pIndexData->new_format)
		 make_new = TRUE;
	    fMatch = TRUE;
	  }
	  break;
		    

	case 'S':
	    KEY( "ShortDescr",	obj->short_descr,	fread_string( fp ) );
	    KEY( "ShD",		obj->short_descr,	fread_string( fp ) );

	    if ( !str_cmp( word, "Spell" ) )
	    {
		int iValue;
		int sn;
		char *temp;

		iValue = fread_number( fp );
		temp = fread_word( fp ) ;
		sn = skill_lookup(temp);
		if ( iValue < 0 || iValue > 3 )
		{
		    bug( "Fread_obj: bad iValue %d.", iValue );
		}
		else if ( sn < 0 )
		{
		    bug( "Fread_obj: unknown skill.", 0 );
		}
		else
		{
		    obj->value[iValue] = sn;
		}
		fMatch = TRUE;
		break;
	    }

	    break;

	case 'T':
	    KEY( "Timer",	obj->timer,		fread_number( fp ) );
	    KEY( "Time",	obj->timer,		fread_number( fp ) );
	    break;

	case 'V':
	    if ( !str_cmp( word, "Values" ) || !str_cmp(word,"Vals"))
	    {
		obj->value[0]	= fread_number( fp );
		obj->value[1]	= fread_number( fp );
		obj->value[2]	= fread_number( fp );
		obj->value[3]	= fread_number( fp );
		if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
		   obj->value[0] = obj->pIndexData->value[0];
		fMatch		= TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Val" ) )
	    {
		obj->value[0] 	= fread_number( fp );
	 	obj->value[1]	= fread_number( fp );
	 	obj->value[2] 	= fread_number( fp );
		obj->value[3]	= fread_number( fp );
		obj->value[4]	= fread_number( fp );
		fMatch = TRUE;
		break;
	    }

	    if ( !str_cmp( word, "Vnum" ) )
	    {
		int vnum;

		vnum = fread_number( fp );
		if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL )
		    bug( "Fread_obj: bad vnum %d.", vnum );
		else
		    fVnum = TRUE;
		fMatch = TRUE;
		break;
	    }
	    break;

	case 'W':
	    KEY( "WearFlags",	obj->wear_flags,	fread_number( fp ) );
	    KEY( "WeaF",	obj->wear_flags,	fread_number( fp ) );
	    KEY( "WearLoc",	obj->wear_loc,		fread_number( fp ) );
	    KEY( "Wear",	obj->wear_loc,		fread_number( fp ) );
	    KEY( "Weight",	obj->weight,		fread_number( fp ) );
	    KEY( "Wt",		obj->weight,		fread_number( fp ) );
	    break;

	}

	if ( !fMatch )
	{
	    bug( word,0);
	    bug( "Fread_obj: no match.", 0 );
	    fread_to_eol( fp );
	}
    }
}


