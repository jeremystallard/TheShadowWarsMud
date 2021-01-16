/***************************************************************************
 * This file include code special dedicated to minion handeling            *
 * TSW is copyright -2003 Swordfish and Zandor                             *
 **************************************************************************/
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"

extern bool check_parse_name (char* name);  /* comm.c */
char *colorcontinue(char code, char *argument); /* act_comm.c */

int compare_dfranks(const void *v1, const void *v2)
{
  return (*(CHAR_DATA**)v1)->pcdata->df_level - (*(CHAR_DATA**)v2)->pcdata->df_level;
}

void do_dfjoin( CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  
  argument = one_argument( argument, arg1 );

  if ( arg1[0] == '\0') {
    send_to_char("Syntax:\n\r  dfjoin <dfname>\n\r\n\r",ch);
    return;
  }

  if (ch->pcdata->df_level >= 0 ) {
    send_to_char("You are already a darkfriend!!!\r\n",ch);
    return;

  }
  
  if (IS_SHADOWSPAWN(ch) && !IS_FADE(ch)) {
     send_to_char("Shadowspawn is already a close friend of the dark.\r\n",ch);
     return;  	
  }
  
  if (ch->race == race_lookup("ogier")) {
     send_to_char("Ogiers are not allowed to become a darkfriend for the time beeing.\r\n",ch);
     return;  	  	
  }

  if (!check_parse_name(colorstrem(arg1))) {
     send_to_char("Illegal Darkfriend name, try another name.\n\r", ch);
     return; 
  }

  ch->pcdata->df_level = 9;
  free_string(ch->pcdata->df_name);
  ch->pcdata->df_name = strdup(arg1);
  return;
}

void set_dftitle( CHAR_DATA *ch, char *dfname )
{
  char buf[MAX_STRING_LENGTH];
  
  strcpy( buf, dfname );
  free_string( ch->pcdata->df_name );
  ch->pcdata->df_name = str_dup(buf);
  return;
}

bool can_dfpromote(CHAR_DATA * ch)
{
  /* ok if ch is a IMMORTAL */
  if (IS_IMMORTAL(ch))
    return TRUE;

  if (ch->pcdata->df_level <= 3)
    return TRUE;
  
  return FALSE;
}

void reset_dfstatus(CHAR_DATA *ch)
{
  if (IS_NPC(ch))
     return;
	
  ch->pcdata->df_level = -1;
  free_string(ch->pcdata->df_name);  
  ch->pcdata->df_name = str_dup("");   
}

void do_dfbanish( CHAR_DATA *ch, char *argument )
{
  
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];    
  CHAR_DATA *vch;

  
  argument = one_argument( argument, arg1 );
  
  if ((ch->pcdata->df_level >=1) && (!IS_IMMORTAL(ch)))
  {
	send_to_char("The dark does not let its own go so willingly.\r\n",ch);
	return;
  }
  if ( arg1[0] == '\0') {
    send_to_char("Syntax: dfbanish <char>\n\r", ch);
    return;
  }

  if ( ( vch = get_char_world( ch, arg1)) == NULL ) {
     send_to_char( "They aren't here.\n\r", ch );
     return;  	
  }

  if IS_NPC(vch) {
    send_to_char("Mobiles can't be dfjoined or banished!\n\r",ch);
    return;
  }    
  
  /* Get rid of people that are trying to banish themselves */
  if (!IS_IMMORTAL(ch) && ch->minion != vch->minion) {
    send_to_char("That person isn't in your minion!\n\r",ch);
    return;
  }

  sprintf(buf, "%s banished %s from the dark.\n\r", ch->name, vch->name);
  log_string(buf);
  
  sprintf(buf, "You are no longer a member of the dark!!\n\r");
  send_to_char(buf, vch);


  sprintf(buf,"%s has banished you from the dark\n\r",PERS(ch, vch));
  send_to_char(buf,vch);

  sprintf(buf, "You have banished %s from the dark.\n\r", PERS(vch, ch));
  send_to_char(buf, ch);
  
  reset_dfstatus(vch);

  return;
}

/*
 * Show current playing and wizible members of a Minion
 */
void do_dflist( CHAR_DATA *ch, char *argument )
{
  BUFFER *output;
  char buf[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;  
  CHAR_DATA *wch;    
  CHAR_DATA *pcs[MAX_PC_ONLINE];
  int cnt = 0;
  int i   = 0;
  int fillers=0;
  int fillers2=0;

  if ((ch->pcdata->df_level == -1) && (!IS_IMMORTAL(ch)) ) {
    send_to_char("You aren't a darkfriend.\n\r", ch);
    return;
  }

  if ((ch->pcdata->df_level > 2) && (!IS_IMMORTAL(ch))) {
    send_to_char("You aren't high enough in the Great Lords favor.\n\r", ch);
    return;
  }

  output = new_buf();

  for (d = descriptor_list; d != NULL; d = d->next) {    
    if (d->connected != CON_PLAYING || !can_see(ch,d->character))
	 continue;
    
    wch = ( d->original != NULL ) ? d->original : d->character;

    if (!can_see(ch,wch))
	 continue;

    if (wch->pcdata->df_level == -1)
       continue;

    pcs[cnt++] = wch;
  }

  /* Sort PC array based on name first */
  qsort (pcs, cnt, sizeof(wch), compare_char_names);

  /* Then we sort based on rank */
  qsort (pcs, cnt, sizeof(wch), compare_dfranks);

  for (i=0; i < cnt; i++) {
    fillers = (16 - colorstrlen(pcs[i]->pcdata->df_name)-1);
    fillers2 = (16 - colorstrlen(PERS_NAME(pcs[i], ch))-1);

    sprintf(buf, "[Darkfriend (%d)]	%s	{x(%s{x)\n\r", 
		  pcs[i]->pcdata->df_level, 
		  PERS_NAME(pcs[i], ch),
		  pcs[i]->pcdata->df_name);
    add_buf(output,buf);
  }
  
  sprintf(buf, "\nDarkfriends found: {y%d{x.\n\r", cnt);
  add_buf(output,buf);

  page_to_char(buf_string(output),ch);
  free_buf(output);
}

void do_dfpromote(CHAR_DATA * ch, char *argument)
{
  char arg1[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  int cnt;

  argument = one_argument(argument, arg1);

  if (ch->pcdata->df_level == -1) {
    send_to_char("You aren't a friend of the dark.\n\r", ch);
    return;
  }

  if ((ch->pcdata->df_level > 3) && (!IS_IMMORTAL(ch)))
  {
    send_to_char("You are not high enough in the Great Lords favor.\n\r", ch);
    return;
  }

  if (arg1[0] == '\0' || argument[0] == '\0') {
    send_to_char("Syntax: dfpromote <who> <rank #>\n\r", ch);
    return;
  }

   if ( ( victim = get_realname_char_world( ch, arg1)) == NULL ) {
      send_to_char( "They aren't playing.\n\r", ch );
      return;
   }

  if (!IS_IMMORTAL(ch) && IS_IMMORTAL(victim)) {
    send_to_char("You can't promote Immortals.\n\r", ch);
    return;
  }

  if (!IS_IMMORTAL(ch) && victim == ch) {
    send_to_char("You can't promote your self.\n\r", ch);
    return;
  }

  cnt = atoi(argument);
  if ((cnt <= ch->pcdata->df_level) && (!IS_IMMORTAL(ch)))
  {
    send_to_char("You can't promote someone that high.\n\r", ch);
    return;
  }

  if (cnt < 0 || cnt > MAX_RANK -1 ) {
    sprintf(buf, "Rank needs to be between 1 and %d.\n\r", MAX_RANK);
    send_to_char(buf, ch);
    return;
  }
  
  if (cnt < victim->pcdata->df_level) {
    sprintf(buf, "You have been promoted to rank %d in the dark.\n\r", cnt);
    send_to_char(buf, victim);

    sprintf(buf, "%s has been promoted to rank %d in the darkfriends guild!\n\r", PERS_NAME(victim, ch), cnt);
    send_to_char(buf, ch);
  }
  else {
    sprintf(buf, "You have been demoted to rank %d in the darkfriends guilds.\n\r", cnt);
    send_to_char(buf, victim);
    
    sprintf(buf, "%s has been demoted to rank %d in the darkfriends guilds!\n\r", PERS_NAME(victim, ch), cnt);
    send_to_char(buf, ch);
  }
  
  victim->pcdata->df_level = cnt;
  return;
}

void do_dfmembers( CHAR_DATA *ch, char *argument )
{
  struct stat sb;
  time_t last_on_time;
  CHAR_DATA *victim;  
  BUFFER *pbuf;
  struct dirent **Dir;  
  int n;
  int i;
  char fname[80];
  char buf[MSL];
  char buffer[MSL];
  int nMatch=0;
  char read[MAX_INPUT_LENGTH];
  char *ptr, *text;
  bool find_more = TRUE;
  bool isOnline = FALSE;
  FILE *fp;
  int fillers=0;

  char name[MSL];
  int level = 0;
  char dfName[MSL];
  char last_on_time_str[MSL];
  int dfRank  = 0;  
  int mCnt = 0;

  if (!(ch->pcdata->df_level > 1) && !IS_IMMORTAL(ch)) {
    send_to_char("You can't show the member list with your rank.\n\r", ch);
    return;
  }
    
  n = scandir(PLAYER_DIR, &Dir, 0, alphasort);

  if (n < 0) {
     send_to_char("No players found!\n\r", ch);
     return;	
  }

  pbuf = new_buf();
  
  send_to_char("{C                                         DARKFRIENDS                                         {x\n\r",ch);
  send_to_char("{RName           Rank  DF_Name         Lvl  Last on                  {x\n\r",ch);
  send_to_char("{R==================================================================={x\n\r",ch);
  
  for (i=0; i<n; i++) {
	sprintf(fname, PLAYER_DIR "%s", Dir[i]->d_name); {

	  if (Dir[i]->d_name[0] >= 'A' && Dir[i]->d_name[0] <= 'Z' && Dir[i]->d_name[0] != '.') {

	    if ( ( fp = fopen( fname, "r" ) ) == NULL )
		 continue;

	    nMatch++;

	    /* Name not so hard to find.. it's the FNAME!! */
	    sprintf(name, "%s", Dir[i]->d_name);
	    
	    while( find_more && (fgets( read, 80, fp  ) != NULL) )  {
		 int len;
		 len = strlen( read );
		 if( read[len-2] == '~' )
		   read[len-2] = '\0';
		 
		 memset(buf, 0x00, sizeof(buf));
		 ptr = read; 
		 text = buf;
		 
		 while( *ptr != ' ' )   /* Grab the keyword only */
		   *text++ = *ptr++ ;		 

		 switch( buf[0] ) {
		 default:  
		   continue;
		   break;

		 case 'A':
		   if( !strcmp( buf, "Alias" ) )  {
			find_more = FALSE;
			break;
		   }
		   break;

		 case 'C':
		   if( !strcmp( buf, "Colors" ) ) /* Done looking */ {
			find_more = FALSE;
			break; 
		   }
		   break;
		 case 'D':
		   if ( !strcmp( buf, "DFName" ) ) 
		   {
			while ( *ptr == ' ' ) ptr++; 
			sprintf(dfName, "%s", ptr);
			break;
		   }
		   if( !strcmp( buf, "DFLevel" ) ) {
			while ( *ptr == ' ' ) ptr++;
			dfRank = atoi(ptr);
			break;
		   }


		 case 'E':
		   if( !strcmp( buf, "End" ) ) /* Done looking */ {
			find_more = FALSE;
			break; 
		   }
		   break;
		 case 'L':
		   if( !strcmp( buf, "Levl" ) ) {
			while ( *ptr == ' ' ) ptr++; 
			level = atoi(ptr);
			break;
		   }
		   break;
		 }

		 memset(read, 0x00, sizeof(read));		 
	    }

	    fclose(fp);	 
	    free(Dir[i]);
	    find_more = TRUE;

	    if (ch->pcdata->df_level != -1) {
		 mCnt++;
		 
		 /* Is Victim online ? */
		 if ((victim = get_char_world(ch, name)) != NULL) {
		   isOnline = TRUE;
		 }
		 else {
		   isOnline = FALSE;
		   stat(fname,&sb);
		   last_on_time = (sb.st_mtime);
		   sprintf(last_on_time_str, "%s", (char *)ctime(&last_on_time));
		   last_on_time_str[strlen(last_on_time_str)-1] = '\0';
		 }


		 /* Adjust to real rank */

		 if (IS_IMMORTAL(ch)) {
		   sprintf(buffer, "{B[{c%-12s{B][{c%3d{B][{c%s%*s{B][{c%3d{B][{c%24s{B]{x\n\r", 
				 name, 
				 dfRank, 
				 dfName, 
				 fillers, "",
				 level, 
				 isOnline ? "Online" : last_on_time_str);
		 }
		 else {
		   sprintf(buffer, "{B[{c%-12s{B][{c%3d{B][{c%s%*s{B][{c%3s{B][{c%24s{B]{x\n\r", 
				 name, 
				 dfRank, 
				 dfName, 
				 fillers, "",
				 "xxx", 
				 isOnline ? "Online" : last_on_time_str);
		 }

		 add_buf(pbuf, buffer);
		 
		 isOnline  = FALSE;
		 
		 memset(name, 0x00, sizeof(name));
		 memset(dfName, 0x00, sizeof(dfName));
		 dfRank = -1;
		 level = 0;
		 fillers = 0;
	    }	    
	  }
	}
   }
   
   free(Dir);
   sprintf( buffer, "\n\r{cDarkfriends found{C: {Y%d{x\n\r", mCnt );
   add_buf(pbuf, buffer);
   page_to_char(buf_string(pbuf), ch);
   free_buf(pbuf);
   return;
}

void do_dftell( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *victim=NULL;

  // Only for PCs
  if (IS_NPC(ch))
    return;
  
  //Is DF?
  if (ch->pcdata->df_level == -1) {
    send_to_char("You aren't a friend of the dark.\n\r", ch);
    return;
  }
  
  // Make sure df_name is set
  if (IS_NULLSTR(ch->pcdata->df_name)) {
    send_to_char("You need to have a valid DFname set before using this.\n\r", ch);
    return;
  }

  if ( IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm,COMM_DEAF)) {
    send_to_char( "Your message didn't get through.\n\r", ch );
    return;
  }
  
  if ( IS_SET(ch->comm, COMM_QUIET) ) {
    send_to_char( "You must turn off quiet mode first.\n\r", ch);
    return;
  }
  
  argument = one_argument( argument, arg );

  if ( IS_NULLSTR(arg) || IS_NULLSTR(argument)) {
    send_to_char( "Send a dftell to whom?\n\r", ch );
    return;
  }

  if (( victim = get_realname_char_world( ch, arg )) == NULL || IS_NPC(victim)) {
    sprintf(buf, "%s isn't here.\n\r", capitalize(arg));
    send_to_char(buf, ch);
    return;
  }

  if (ch == victim) {
    send_to_char("You don't need to send dftells to your self.\n\r", ch);
    return;
  }
  
  // Link dead?
  if ( victim->desc == NULL && !IS_NPC(victim)) {
    act_old("$N seems to have misplaced $S link...try again later.", ch,NULL,victim,TO_CHAR, POS_DEAD);
    return;
  }

  if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF)) && !IS_IMMORTAL(ch)) {
    act_old("$E is not receiving tells.", ch, 0, victim, TO_CHAR, POS_DEAD);
    return;
  }

  sprintf(buf,"{x%s dftells you '{X%s{x'\n\r",ch->pcdata->df_name,argument);	 
  buf[0] = UPPER(buf[0]);
  send_to_char(buf, victim);

  add_buf(victim->pcdata->buffer,buf);

  act_old("{xYou dftell $N '{X$t{x'", ch, argument, victim, TO_CHAR, POS_DEAD);
  
  victim->dfreply = ch;
  
  return;
}

void do_dfreply( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Dfreply what?\n\r", ch);
    return;
  }

  if (IS_SET(ch->comm, COMM_NOTELL) ) {
    send_to_char( "Your message didn't get through.\n\r", ch );
    return;
  }

  if ( IS_SET(ch->comm, COMM_QUIET) ) {
    send_to_char( "You must turn off quiet mode first.\n\r", ch);
    return;
  }
  
  if (( victim = ch->dfreply) == NULL ) {
    send_to_char("They aren't here.\n\r", ch );
    return;
  }
  
  if (victim->desc == NULL) {
    send_to_char("They aren't here.\n\r", ch );
    return;
  }
  
  sprintf(buf,"{x%s dfreplies, '{X%s{x'\n\r", PERS_NAME(ch,victim),argument);
  buf[0] = UPPER(buf[0]);
  add_buf(victim->pcdata->buffer,buf);
  send_to_char(buf, victim);
  
  sprintf(buf,"{xYou dfreply to %s '{X%s{x'\n\r", victim->pcdata->df_name, argument);
  send_to_char(buf, ch);
  
  
  return;
}

void do_dfrename( CHAR_DATA *ch, char *argument)
{
  char old_name[MAX_INPUT_LENGTH];
  char new_name[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];

  CHAR_DATA* victim;

  argument = one_argument(argument, old_name); /* find new/old name */
  one_argument (argument, new_name);

  /* Trivial checks */
  if (!old_name[0]) {
    send_to_char ("Rename who?\r\n",ch);
    return;
  }

  victim = get_char_world (ch, old_name);

  if (!victim) {
    send_to_char ("There is no such a person online.\r\n",ch);
    return;
  }

  if (IS_NPC(victim)) {
    send_to_char ("You cannot use Rename on NPCs.\r\n",ch);
    return;
  }

  /* allow rename self new_name,but otherwise only lower level */
  if ( (victim != ch) && (get_trust (victim) >= get_trust (ch)) ) {
    send_to_char ("You failed.\r\n",ch);
    return;
  }

  if (!victim->desc || (victim->desc->connected != CON_PLAYING) ) {
    send_to_char ("This player has lost his link or is inside a pager or the like.\r\n",ch);
    return;
  }

  if (!new_name[0]) {
    send_to_char ("Rename to what new name?\r\n",ch);
    return;
  }

  free_string( victim->pcdata->df_name);
  victim->pcdata->df_name = str_dup( new_name );

  sprintf(buf, "You will be known as '%s' amoung your fellow darkfriends.\n\r", victim->pcdata->df_name);
  send_to_char(buf, victim);
  sprintf(buf, "They will be known as '%s' amoung their fellow darkfriends.\n\r", victim->pcdata->df_name);
  send_to_char(buf, ch);

  return;
}

