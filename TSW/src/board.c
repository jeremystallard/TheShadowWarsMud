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

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "colordef.h" 

DECLARE_DO_FUN( do_help		);
int colorstrlen(char *argument);
bool is_leader(CHAR_DATA * ch);         /* guild.c   */
bool is_sguild_leader(CHAR_DATA * ch);  /* sguild.c  */
bool is_ssguild_leader(CHAR_DATA * ch); /* ssguild.c */

void do_guildtalk(CHAR_DATA *ch, char *argument);

/*
 
 Note Board system, (c) 1995-96 Erwin S. Andreasen, erwin@pip.dknet.dk
 =====================================================================
 
 Basically, the notes are split up into several boards. The boards do not
 exist physically, they can be read anywhere and in any position.
 
 Each of the note boards has its own file. Each of the boards can have its own
 "rights": who can read/write.
 
 Each character has an extra field added, namele the timestamp of the last note
 read by him/her on a certain board.
 
 The note entering system is changed too, making it more interactive. When
 entering a note, a character is put AFK and into a special CON_ state.
 Everything typed goes into the note.
 
 For the immortals it is possible to purge notes based on age. An Archive
 options is available which moves the notes older than X days into a special
 board. The file of this board should then be moved into some other directory
 during e.g. the startup script and perhaps renamed depending on date.
 
 Note that write_level MUST be >= read_level or else there will be strange
 output in certain functions.
 
 Board DEFAULT_BOARD must be at least readable by *everyone*.
 
*/ 

#define L_SUP (MAX_LEVEL - 1) /* if not already defined */

void do_board (CHAR_DATA *ch, char *argument);
void do_topboard(CHAR_DATA *ch, char *argument);
void    do_poll args( ( CHAR_DATA *ch, char *argument ) );


BOARD_DATA boards[MAX_BOARD] =
{
{ "Announce",	"Announcements from TSW",		0,		LI,	"all",			DEF_INCLUDE,	MAX_LEVEL,	NULL, FALSE },
{ "General",	"General discussion",			0,		1,	"all",			DEF_NORMAL,		31,			NULL, FALSE },
{ "Ideas",		"Suggestion for improvement",	0,		1,	"all",			DEF_NORMAL,		MAX_LEVEL,	NULL, FALSE }, 
{ "Personal",	"Personal messages",			0,		1,	"all",			DEF_EXCLUDE,	31,			NULL, FALSE },
{ "Guild",		"Guild messages",				0,		1,	"all",			DEF_NORMAL,		31,			NULL, FALSE },
{ "Stories",	"Stories",						0,		1,	"all",			DEF_NORMAL,		MAX_LEVEL,	NULL, FALSE },
{ "Bugs",		"Bugs",                         0,		1,	"all",			DEF_NORMAL,		31,			NULL, FALSE },
{ "Permanent",	"Permanent messages",			0,		1,	"all",			DEF_NORMAL,		MAX_LEVEL,	NULL, FALSE },
{ "Admin",		"Admin messages",				LI,		LI, "Admin",		DEF_INCLUDE,	MAX_LEVEL,	NULL, FALSE },
{ "PK",			"PK messages",					LI,		LI, "Admin",		DEF_INCLUDE,	MAX_LEVEL,	NULL, FALSE },
{ "Immortal",	"Notes to TSW Immortals",		0,		1,	"Admin",		DEF_INCLUDE,	MAX_LEVEL,	NULL, FALSE },
{ "Newplayers",	"Notification on new players",	LEVEL_ADMIN, LEVEL_ADMIN, "Admin", DEF_INCLUDE, MAX_LEVEL, NULL, FALSE }
};

/* The prompt that the character is given after finishing a note with ~ or END */
const char * szFinishPrompt = "(" BOLD "C" NO_COLOR ")ontinue, (" BOLD "V" NO_COLOR ")iew, (" BOLD "P" NO_COLOR ")ost or (" BOLD "F" NO_COLOR ")orget it?";

long last_note_stamp = 0; /* To generate unique timestamps on notes */

#define BOARD_NOACCESS -1
#define BOARD_NOTFOUND -1

/* static bool next_board (CHAR_DATA *ch); */
static bool next_unread_board (CHAR_DATA *ch);

bool use_christmas_layout()
{
  struct tm mytime;
  
  mytime = *localtime((time_t *)&current_time);
  
  if (mytime.tm_mon+1 == 12)
    return TRUE;
  
  return FALSE;
}

/* convert a time_t to string */
/* Swordfish                  */
char * sec2str(time_t unixsec)
{
  struct tm mytime;
  static char buf[200];

  buf[0] = '\0';
  
  mytime = *localtime((time_t*)&unixsec);
  sprintf(buf, "%4d.%02d.%02d-%02d:%02d:%02d",
		mytime.tm_year+1900,
		mytime.tm_mon+1,
		mytime.tm_mday,
		mytime.tm_hour,
		mytime.tm_min,
		mytime.tm_sec);
  return (buf);
}

char *sec2strtime(time_t unixsec)
{
  struct tm mytime;
  static char buf[200];

  buf[0] = '\0';
  
  mytime = *localtime((time_t*)&unixsec);
  sprintf(buf, "%02d:%02d:%02d",
		mytime.tm_hour,
		mytime.tm_min,
		mytime.tm_sec);
  return (buf);
}

/* recycle a note */
void free_note (NOTE_DATA *note)
{
  if (note->sender)
    free_string (note->sender);
  if (note->show_sender)
    free_string(note->show_sender);
  if (note->to_list)
    free_string (note->to_list);
  if (note->show_to_list)
    free_string(note->show_to_list);
  if (note->subject)
    free_string (note->subject);
  if (note->date) /* was note->datestamp for some reason */
    free_string (note->date);
  if (note->text)
    free_string (note->text);
  
  note->next = note_free;
  note_free = note;	
}

/* allocate memory for a new note or recycle */
NOTE_DATA *new_note ()
{
  NOTE_DATA *note;
  
  if (note_free) {
    note = note_free;
    note_free = note_free->next;
  }
  else
    note = alloc_mem (sizeof(NOTE_DATA));
  
  /* Zero all the field - Envy does not gurantee zeroed memory */	
  note->next = NULL;
  note->sender = NULL;		
  note->show_sender = NULL;
  note->expire = 0;
  note->to_list = NULL;
  note->show_to_list = NULL;
  note->subject = NULL;
  note->date = NULL;
  note->date_stamp = 0;
  note->text = NULL;
  
  return note;
}

/* append this note to the given file */
static void append_note (FILE *fp, NOTE_DATA *note)
{ 
  fprintf (fp, "Sender  %s~\n", note->sender);
  if (!IS_NULLSTR(note->show_sender))
    fprintf (fp, "sSender %s~\n", note->show_sender);  
  fprintf (fp, "Date    %s~\n", note->date);
  fprintf (fp, "Stamp   %ld\n", note->date_stamp);
  fprintf (fp, "Expire  %ld\n", note->expire);
  fprintf (fp, "To      %s~\n", note->to_list);  
  if (!IS_NULLSTR(note->show_to_list))
    fprintf (fp, "sTo     %s~\n", note->show_to_list);  
  fprintf (fp, "Subject %s~\n", !IS_NULLSTR(note->subject) ? note->subject : "");	
  fprintf (fp, "Text\n%s~\n\n", !IS_NULLSTR(note->text) ? note->text : "");
}

/* Save a note in a given board */
void finish_note (BOARD_DATA *board, NOTE_DATA *note)
{
  FILE *fp;
  NOTE_DATA *p;
  char filename[200];
  
  /* The following is done in order to generate unique date_stamps */
  
  if (last_note_stamp >= current_time)
    note->date_stamp = ++last_note_stamp;
  else {
    note->date_stamp = current_time;
    last_note_stamp = current_time;
  }
  
  if (board->note_first) { /* are there any notes in there now? */
    for (p = board->note_first; p->next; p = p->next )
	 ; /* empty */
    p->next = note;
  }
  else /* nope. empty list. */
    board->note_first = note;
  
  /* append note to note file */		
  
  sprintf (filename, "%s%s", NOTE_DIR, board->short_name);
  
  fp = fopen (filename, "a");
  if (!fp) {
    bug ("Could not open one of the note files in append mode",0);
    board->changed = TRUE; /* set it to TRUE hope it will be OK later? */
    return;
  }
  
  append_note (fp, note);
  fclose (fp);
}

/* Find the number of a board */
int board_number (const BOARD_DATA *board)
{
  int i;
  
  for (i = 0; i < MAX_BOARD; i++)
    if (board == &boards[i])
	 return i;
  
  return -1;
}

/* Find a board number based on  a string */
int board_lookup (const char *name)
{
  int i;
  
  for (i = 0; i < MAX_BOARD; i++)
    if (!str_cmp (boards[i].short_name, name))
	 return i;
  
  return -1;
}

/* Remove list from the list. Do not free note */
static void unlink_note (BOARD_DATA *board, NOTE_DATA *note)
{
  NOTE_DATA *p;
  
  if (board->note_first == note)
    board->note_first = note->next;
  else {
    for (p = board->note_first; p && p->next != note; p = p->next);
    if (!p)
	 bug ("unlink_note: could not find note.",0);
    else
	 p->next = note->next;
  }
}

/* Find the nth note on a board. Return NULL if ch has no access to that note */
static NOTE_DATA* find_note (CHAR_DATA *ch, BOARD_DATA *board, int num)
{
  int count = 0;
  NOTE_DATA *p;
  
  for (p = board->note_first; p ; p = p->next)
    if (++count == num)
	 break;
  
  if ( (count == num) && is_note_to (ch, p))
    return p;
  else
    return NULL;
  
}

/* save a single board */
static void save_board (BOARD_DATA *board)
{
  FILE *fp;
  char filename[200];
  char buf[200];
  NOTE_DATA *note;
  
  sprintf (filename, "%s%s", NOTE_DIR, board->short_name);
  
  fp = fopen (filename, "w");
  if (!fp) {
    sprintf (buf, "Error writing to: %s", filename);
    bug (buf, 0);
  }
  else {
    for (note = board->note_first; note ; note = note->next)
	 append_note (fp, note);
    
    fclose (fp);
  }
}

/* Show one note to a character */
static void show_note_to_char (CHAR_DATA *ch, NOTE_DATA *note, int num, bool details)
{
  BUFFER *output;
  int fillers=0;
  char buf[4*MAX_STRING_LENGTH];
  char frombuf[256];

  ch->pcdata->last_read_note = note;

  output = new_buf();

  if (IS_IMMORTAL(ch))
  {
	if (note->show_sender != NULL)
	{
		sprintf(frombuf,"%s {x(%s)",note->show_sender, note->sender);
        }
	else
		strcpy(frombuf,note->sender);

  }
  else
  {
	if (note->show_sender != NULL)
	{
		strcpy(frombuf,note->show_sender);
	}
	else
	{
		strcpy(frombuf,note->sender);
	}
  }
  

  fillers = (20 - colorstrlen(frombuf) - 1);
  if (fillers < 0)
	fillers = 0;

  if (use_christmas_layout()) {
    sprintf (buf,
		   "{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r"
		   "{rNote{W:{x %-4d       {rFrom{W:{x %s%*s {rDate{W:{x %s {x\n\r"
		   "{rArea{W:{x %-12s {rTo{W:{x %s {x\n\r"
		   "{rSubj{W:{x %s {x\n\r"
		   "{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r"
		   "%s"
		   "{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r",
		   num, 
		   frombuf, 
		   fillers, "",
		   note->date,
		   boards[board_number (ch->pcdata->board)].short_name, 
		   note->show_to_list ? note->show_to_list : note->to_list, 
		   note->subject, note->text);
  }
  else {
    if (details)
    sprintf (buf,
		   "{W---------------------------------------------------------------------------{x\n\r"
		   "{CNote{W:{x %-4d       {CFrom{W:{x %s%*s {CDate{W:{x %s {x\n\r"
		   "{CArea{W:{x %-12s {CTo{W:{x %s {x\n\r"
		   "{CSubj{W:{x %s {x\n\r"
		   "{W---------------------------------------------------------------------------{x\n\r"
		   "{CSender {W:{x %s\n\r"
		   "{CTo     {W:{x %s\n\r"
		   "{CExpires{W:{x %s\n\r"
		   "{W---------------------------------------------------------------------------{x\n\r"
		   "%s"
		   "{W---------------------------------------------------------------------------{x\n\r",
		   num, 
		   frombuf, 
		   fillers, "",
		   note->date,
		   boards[board_number (ch->pcdata->board)].short_name, 
		   note->show_to_list ? note->show_to_list : note->to_list,
		   note->subject, 
		   note->sender, note->to_list, sec2str(note->expire),
		   note->text);
    else
    sprintf (buf,
		   "{W---------------------------------------------------------------------------{x\n\r"
		   "{CNote{W:{x %-4d       {CFrom{W:{x %s%*s {CDate{W:{x %s {x\n\r"
		   "{CArea{W:{x %-12s {CTo{W:{x %s {x\n\r"
		   "{CSubj{W:{x %s {x\n\r"
		   "{W---------------------------------------------------------------------------{x\n\r"
		   "%s"
		   "{W---------------------------------------------------------------------------{x\n\r",
		   num, 
		   frombuf, 
		   fillers, "",
		   note->date,
		   boards[board_number (ch->pcdata->board)].short_name, 
		   note->show_to_list ? note->show_to_list : note->to_list,
		   note->subject, note->text);
  }
  add_buf(output,buf);
  page_to_char( buf_string(output), ch );
  free_buf(output);
  /* send_to_char (buf,ch);	         */
}

/* Save changed boards */
void save_notes ()
{
	int i;
	 
	for (i = 0; i < MAX_BOARD; i++)
		if (boards[i].changed) /* only save changed boards */
			save_board (&boards[i]);
}

/* Load a single board */
static void load_board (BOARD_DATA *board)
{
  FILE *fp, *fp_archive;
  NOTE_DATA *last_note;
  char filename[200];
  char buf[256];
  char *word=NULL;
  bool oformat=TRUE;
  
  sprintf (filename, "%s%s", NOTE_DIR, board->short_name);
  
  sprintf(buf, "Loading board %s", filename);
  log_string(buf);
  
  fp = fopen (filename, "r");
  
  /* Silently return */
  if (!fp)
    return;		
  
  /* Start note fetching. copy of db.c:load_notes() */
  
  last_note = NULL;
  
  for ( ; ; ) {
    NOTE_DATA *pnote;
    char letter;

    do {
	 letter = getc( fp );
	 if ( feof(fp) ) {
	   fclose( fp );
	   return;
	 }
    }
    while ( isspace(letter) );
    ungetc( letter, fp );
    
    pnote             = alloc_perm( sizeof(*pnote) );
    
    if ( str_cmp( word=fread_word( fp ), "sender" ) )
	 break;
    pnote->sender     = fread_string( fp );

    if (!str_cmp( word=fread_word( fp ), "ssender" ) ) {
	 pnote->show_sender       = fread_string( fp );
	 oformat = FALSE;
    }
    else {
	 if ( str_cmp( word, "date" ) )
	   break;
	 pnote->date       = fread_string( fp );
    }
    
    if (!oformat) {
	 if ( str_cmp( word=fread_word( fp ), "date" ) )
	   break;
	 pnote->date       = fread_string( fp );
	 oformat = TRUE;
    }

    if ( str_cmp( word=fread_word( fp ), "stamp" ) )
	 break;
    pnote->date_stamp = fread_number( fp );
    
    if ( str_cmp( word=fread_word( fp ), "expire" ) )
	 break;
    pnote->expire = fread_number( fp );
    
    if ( str_cmp( word=fread_word( fp ), "to" ) )
	 break;
    pnote->to_list    = fread_string( fp );

    if (!str_cmp( word=fread_word( fp ), "sto" ) ) {
	 pnote->show_to_list       = fread_string( fp );
	 oformat = FALSE;
    }
    else {
	 if ( str_cmp( word, "subject" ) )
	   break;
	 pnote->subject       = fread_string( fp );
    }

    if (!oformat) {
	 if ( str_cmp( word=fread_word( fp ), "subject" ) )
	   break;
	 pnote->subject    = fread_string( fp );
	 oformat = TRUE;	 
    }
    
    if ( str_cmp( word=fread_word( fp ), "text" ) )
	 break;
    pnote->text       = fread_string( fp );
    
    pnote->next = NULL; /* jic */
    
    /* Should this note be archived right now ? */
    /* We don't want Permanent notes to be expired, so additional check */
    if ((pnote->expire < current_time) && (str_cmp(board->short_name, "Permanent")) && (str_cmp(board->short_name,"Announce"))) {
	 char archive_name[200];
	 
	 sprintf (archive_name, "%s%s.old", NOTE_DIR, board->short_name);
	 fp_archive = fopen (archive_name, "a");
	 if (!fp_archive)
	   bug ("Could not open archive boards for writing",0);
	 else {
	   append_note (fp_archive, pnote);
	   fclose (fp_archive); /* it might be more efficient to close this later */
	 }
	 
	 free_note (pnote);
	 board->changed = TRUE;
	 continue;	 
    }
    
    
    if ( board->note_first == NULL )
	 board->note_first = pnote;
    else
	 last_note->next     = pnote;
    
    last_note         = pnote;
    
  }
  
  sprintf(buf, "Load_notes: bad key word in area <%s>", filename);
  bug(buf, 0);
  
  //bug( "Load_notes: bad key word.", 0 );
  return; /* just return */
}

/* Initialize structures. Load all boards. */
void load_boards ()
{
  int i;
  char buf[256];
  
  for (i = 0; i < MAX_BOARD; i++)
    load_board (&boards[i]);
  
  sprintf(buf, "Loaded %d boards sucessfully.", i);
  log_string(buf);
  
  return;
}

/* find out if a note is a Immortal note for THIS char */
bool is_imm_note_to (CHAR_DATA *ch, NOTE_DATA *note)
{
  // Is Imm?
  if (!IS_IMMORTAL(ch))
    return FALSE;

  if (IS_IMMORTAL(ch) &&
	 ( is_full_name ("admin", note->to_list) ||
	   is_full_name ("imm", note->to_list) ||
	   is_full_name ("imms", note->to_list) ||
	   is_full_name ("immortal", note->to_list) ||
	   is_full_name ("builder", note->to_list) ||
	   is_full_name ("builders", note->to_list) ||
	   is_full_name ("god", note->to_list) ||
	   is_full_name ("gods", note->to_list) ||
	   is_full_name ("immortals", note->to_list) ||
	   is_full_name ("guildimm", note->to_list) ||
	   is_full_name ("guildadmin", note->to_list)))
    return TRUE;
  
  if ((get_trust(ch) == MAX_LEVEL) && 
	 ( is_full_name ("imp", note->to_list) ||
	   is_full_name ("imps", note->to_list) ||
	   is_full_name ("implementor", note->to_list) ||
	   is_full_name ("implementors", note->to_list)))
    return TRUE;  

  return FALSE;
}

/* find out if a note is a guild note for THIS char */
bool is_guild_note_to (CHAR_DATA *ch, NOTE_DATA *note)
{
  char guild_leader[MAX_STRING_LENGTH];

  memset(guild_leader, 0x00, sizeof(guild_leader));

  /* In a guild ? */
  if (!is_clan(ch))
    return FALSE;

  /* My guild ? */
  if (is_full_name (player_clan(ch), note->to_list))
    return TRUE;

  /* Oguild? */
  if (is_oguild(ch) && is_full_name (player_oguild(ch), note->to_list))
    return TRUE;

  /* Guild leader? */
  if (can_guild(ch)) 
     sprintf(guild_leader, "%sLeader", player_clan(ch));
     
    if (can_guild(ch) && (is_full_name("guildleader", note->to_list) ||
                          is_full_name("guildleaders", note->to_list)||
                          is_full_name(guild_leader, note->sender)))
      return TRUE;
  
  return FALSE;
}

/* find out if a note is a SGuild note for THIS char */
bool is_sguild_note_to (CHAR_DATA *ch, NOTE_DATA *note)
{
  char sguild_leader[MAX_STRING_LENGTH];

  memset(sguild_leader, 0x00, sizeof(sguild_leader));

  /* In a guild ? */
  if (!is_sguild(ch))
    return FALSE;

  /* My guild ? */
  if (is_full_name (player_sguild(ch), note->to_list))
    return TRUE;

  /* Guild leader? */
  if (can_sguild(ch)) 
     sprintf(sguild_leader, "%sLeader", player_sguild(ch));
     
    if (can_sguild(ch) && (is_full_name("sguildleader", note->to_list) ||
                          is_full_name("sguildleaders", note->to_list)||
                          is_full_name(sguild_leader, note->sender)))
      return TRUE;
    
    return FALSE;
}

/* find out if a note is a SSGuild note for THIS char */
bool is_ssguild_note_to (CHAR_DATA *ch, NOTE_DATA *note)
{
  char ssguild_leader[MAX_STRING_LENGTH];
  
  memset(ssguild_leader, 0x00, sizeof(ssguild_leader));
  
  /* In a guild ? */
  if (!is_ssguild(ch))
    return FALSE;
  
  /* My guild ? */
  if (is_full_name (player_ssguild(ch), note->to_list))
    return TRUE;
  
  /* Guild leader? */
  if (can_ssguild(ch)) 
    sprintf(ssguild_leader, "%sLeader", player_ssguild(ch));
  
  if (can_ssguild(ch) && (is_full_name("ssguildleader", note->to_list) ||
                          is_full_name("ssguildleaders", note->to_list)||
                          is_full_name(ssguild_leader, note->sender)))
    return TRUE;
  
  return FALSE;
}

bool is_minion_note_to(CHAR_DATA *ch, NOTE_DATA *note)
{
  char minion_name[MSL];
  char buf[MSL];
  char *ptr=NULL;

  buf[0] = '\0';
  minion_name[0] = '\0';
  
  if (ch->minion == 0 || IS_NULLSTR(ch->mname))
    return FALSE;
  
  // My minion?
  strcpy(buf, ch->mname);    
  ptr = strtok(buf, "'");
  
  if (ptr == NULL)
    return FALSE;
  
  sprintf(minion_name, "@%s",  ptr);
  
  if (is_full_name(minion_name, note->to_list))
    return TRUE;    

  return FALSE;
}

/* find out if a note is a persoanal note to THIS char */
bool is_personal_note_to (CHAR_DATA *ch, NOTE_DATA *note)
{
  if (!str_cmp (ch->name, note->sender))
    return TRUE;

  if (is_full_name (ch->name, note->to_list))
    return TRUE;

  return FALSE;
}

/* Find out if a note is posted by THIS char */
bool is_note_from(CHAR_DATA *ch, NOTE_DATA *note)
{
  if (!str_cmp (ch->name, note->sender))
    return TRUE;
  
  return FALSE;
}

/* Returns TRUE if the specified note is address to ch */
bool is_note_to (CHAR_DATA *ch, NOTE_DATA *note)
{
  char guild_leader[MAX_STRING_LENGTH];
  char sguild_leader[MAX_STRING_LENGTH];
  char ssguild_leader[MAX_STRING_LENGTH];
  char buf[MSL];
  char minion_name[MSL];

  guild_leader[0] = '\0';
  sguild_leader[0] = '\0';
  ssguild_leader[0] = '\0';
  buf[0] = '\0';
  minion_name[0] = '\0';

    //memset(guild_leader, 0x00, sizeof(guild_leader));
    //memset(sguild_leader, 0x00, sizeof(sguild_leader));
    //memset(ssguild_leader, 0x00, sizeof(ssguild_leader));
  
  /* Self */
  if (!str_cmp (ch->name, note->sender))
    return TRUE;
  
  /* All */
  if (is_full_name ("all", note->to_list))
    return TRUE;
  
  if (IS_IMMORTAL(ch) && 
	 ( is_full_name ("admin", note->to_list) ||
	   is_full_name ("imm", note->to_list) ||
	   is_full_name ("imms", note->to_list) ||
	   is_full_name ("immortal", note->to_list) ||
	   is_full_name ("builder", note->to_list) ||
	   is_full_name ("builders", note->to_list) ||
	   is_full_name ("god", note->to_list) ||
	   is_full_name ("gods", note->to_list) ||
	   is_full_name ("immortals", note->to_list) ||
	   is_full_name ("guildimm", note->to_list) ||
	   is_full_name ("guildadmin", note->to_list)))
    return TRUE;
  
  if ((get_trust(ch) == MAX_LEVEL) && 
	 ( is_full_name ("imp", note->to_list) ||
	   is_full_name ("imps", note->to_list) ||
	   is_full_name ("implementor", note->to_list) ||
	   is_full_name ("implementors", note->to_list)))
    return TRUE;
  
  if (IS_CODER(ch) && is_full_name("coder", note->to_list))
    return TRUE;
  
  /* Personal notes */
  if (is_full_name (ch->name, note->to_list))
    return TRUE;
  
  /* Guild notes */
  if ((is_clan(ch) && is_full_name (player_clan(ch), note->to_list)))
    return TRUE;

  /* Oguild guild notes*/
  if ((is_oguild(ch) && is_full_name (player_oguild(ch), note->to_list)))
    return TRUE;
  
  /* Guild leaders */
  if (is_clan(ch) && can_guild(ch)) {
    sprintf(guild_leader, "%sLeader", player_clan(ch));
    if (is_full_name("guildleader", note->to_list)  || 
	   is_full_name("guildleaders", note->to_list) ||
	   is_full_name(guild_leader, note->sender)) {
	 return TRUE;    	
    }
  }
  
  /* SGuild notes */
  if ((is_sguild(ch) && is_full_name (player_sguild(ch), note->to_list)))
    return TRUE;
  
  /* SGuild leaders */
  if (is_sguild(ch) && can_sguild(ch)) {
    sprintf(sguild_leader, "%sLeader", player_sguild(ch));
    if (is_full_name("sguildleader", note->to_list)  || 
	   is_full_name("sguildleaders", note->to_list) ||
	   is_full_name(sguild_leader, note->sender)) {
	 return TRUE;
    }
  }

  /* SSGuild notes */
  if ((is_ssguild(ch) && is_full_name (player_ssguild(ch), note->to_list)))
    return TRUE;
  
  /* SSGuild leaders */
  if (is_ssguild(ch) && can_ssguild(ch)) {
    sprintf(ssguild_leader, "%sLeader", player_ssguild(ch));
    if (is_full_name("ssguildleader", note->to_list)  || 
	   is_full_name("ssguildleaders", note->to_list) ||
	   is_full_name(ssguild_leader, note->sender)) {
	 return TRUE;
    }
  }

  /* Minion */
  if (ch->minion != 0 && !IS_NULLSTR(ch->mname)) {
    char *ptr=NULL;
    
    strcpy(buf, ch->mname);    
    ptr = strtok(buf, "'");
    
    if (ptr == NULL)
	 return FALSE;
    
    sprintf(minion_name, "@%s",  ptr);
    
    if (is_full_name(minion_name, note->to_list))
	 return TRUE;
  }
  
  /* Wolfkin */
  if (IS_WOLFKIN(ch) && is_full_name ("wolfkin", note->to_list))
    return TRUE;

  // Newbie Helpers
  if ((IS_IMMORTAL(ch) || IS_NEWBIEHELPER(ch)) &&
	 (is_full_name ("newbiehelper", note->to_list) ||
	  is_full_name ("newbiehelpers", note->to_list))) {
    return TRUE;
  }

  // Warders
  if (IS_WARDER(ch) && (is_full_name("warder", note->to_list) ||
				    is_full_name("warders", note->to_list))) {
    return TRUE;
  }
  
  /* Allow a note to e.g. 40 to send to characters level 40 and above */		
  if (is_number(note->to_list) && get_trust(ch) >= atoi(note->to_list))
    return TRUE;

  /* to darkfriends */
  if ((ch->pcdata->df_level >= 0) && (is_full_name("darkfriend",note->to_list) || is_full_name("darkfriends",note->to_list)))
  {
    return TRUE;
  }
  
  return FALSE;
}

int total_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
  NOTE_DATA *note;
  time_t last_read;
  int count = 0;
  
  if (board->read_level > get_trust(ch))
    return BOARD_NOACCESS;
  
  last_read = ch->pcdata->last_note[board_number(board)];
  
  for (note = board->note_first; note; note = note->next) {
     if (is_note_to(ch, note)) {
    	   count++;
     }
  }

  return count;
}

int total_sent_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
  
  NOTE_DATA *note;
  int count=0;

  if (board->read_level > get_trust(ch))
    return 0;

  for (note = board->note_first; note; note = note->next)
    if (is_note_from(ch, note))
	 count++;

  return count;
}

int unread_personal_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
  NOTE_DATA *note;
  time_t last_read;
  int count = 0;
  
  if (board->read_level > get_trust(ch))
    return BOARD_NOACCESS;
  
  last_read = ch->pcdata->last_note[board_number(board)];
  
  for (note = board->note_first; note; note = note->next)
    if (is_personal_note_to(ch, note) && ((long)last_read < (long)note->date_stamp))
      count++;
  
  return count;
}

int unread_imm_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
  NOTE_DATA *note;
  time_t last_read;
  int count = 0;
  
  if (board->read_level > get_trust(ch))
    return BOARD_NOACCESS;
  
  last_read = ch->pcdata->last_note[board_number(board)];
  
  for (note = board->note_first; note; note = note->next)
    if (is_imm_note_to(ch, note) && ((long)last_read < (long)note->date_stamp))
      count++;
  
  return count;
}

int unread_guild_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
  NOTE_DATA *note;
  time_t last_read;
  int count = 0;
  
  if (board->read_level > get_trust(ch))
    return BOARD_NOACCESS;
  
  last_read = ch->pcdata->last_note[board_number(board)];
  
  for (note = board->note_first; note; note = note->next)
    if (is_guild_note_to(ch, note) && ((long)last_read < (long)note->date_stamp))
      count++;

  return count;
}

int unread_sguild_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
  NOTE_DATA *note;
  time_t last_read;
  int count = 0;
  
  if (board->read_level > get_trust(ch))
    return BOARD_NOACCESS;
  
  last_read = ch->pcdata->last_note[board_number(board)];
  
  for (note = board->note_first; note; note = note->next)
    if (is_sguild_note_to(ch, note) && ((long)last_read < (long)note->date_stamp))
      count++;

  return count;
}

int unread_ssguild_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
  NOTE_DATA *note;
  time_t last_read;
  int count = 0;
  
  if (board->read_level > get_trust(ch))
    return BOARD_NOACCESS;
  
  last_read = ch->pcdata->last_note[board_number(board)];
  
  for (note = board->note_first; note; note = note->next)
    if (is_ssguild_note_to(ch, note) && ((long)last_read < (long)note->date_stamp))
      count++;
  
  return count;
}

/* Return the number of unread notes 'ch' has in 'board' */
/* Returns BOARD_NOACCESS if ch has no access to board */
int unread_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
	NOTE_DATA *note;
	time_t last_read;
	int count = 0;
	
	if (board->read_level > get_trust(ch))
		return BOARD_NOACCESS;
		
	last_read = ch->pcdata->last_note[board_number(board)];
	
	for (note = board->note_first; note; note = note->next)
		if (is_note_to(ch, note) && ((long)last_read < (long)note->date_stamp))
			count++;
			
	return count;
}

/*
 * COMMANDS
 */
/* Start writing a note */
static void do_nto (CHAR_DATA *ch, char *argument)
{
  char *strtime;
  
  if (IS_NPC(ch)) /* NPC cannot post notes */
    return;
  
  if (get_trust(ch) < ch->pcdata->board->write_level) {
    send_to_char ("You cannot post notes on this board.\n\r",ch);
    return;
  }
  
  if (!ch->pcdata->in_progress) {
    ch->pcdata->in_progress = new_note();
    ch->pcdata->in_progress->sender = str_dup (ch->name);
    
    /* convert to ascii. ctime returns a string which last character is \n, so remove that */	
    strtime = ctime (&current_time);
    strtime[strlen(strtime)-1] = '\0';
    
    ch->pcdata->in_progress->expire = current_time + 30 * 60 * 60 * 24;
    ch->pcdata->in_progress->date = str_dup (strtime);
    ch->pcdata->in_progress->subject = str_dup ("");
    ch->pcdata->in_progress->text = str_dup ("");
  }
  
  if (ch->pcdata->in_progress->to_list)
    free_string (ch->pcdata->in_progress->to_list);
  ch->pcdata->in_progress->to_list = str_dup(argument);
  
  if (ch->pcdata->in_progress->show_to_list) {
     free_string(ch->pcdata->in_progress->show_to_list);
     ch->pcdata->in_progress->show_to_list = NULL;
  }
  
  send_to_char( "Ok.\n\r", ch );
}

/* Set from field: IMMORTAL only command */
static void do_nfrom (CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  
  if (IS_NPC(ch)) /* NPC cannot post notes */
    return;
  
  if (!ch->pcdata->in_progress) {
    send_to_char("You have no note in progress.\n\r", ch);
    return;
  }
  
  if (argument[0] == '\0') {
    send_to_char("You can not set from field blank.\n\r", ch);
    return;
  }
  
  if (IS_IMMORTAL(ch)) {
    sprintf(buf, "From field set to: %s\n\r", argument);
    send_to_char(buf, ch);
  }

  free_string (ch->pcdata->in_progress->show_sender);
  ch->pcdata->in_progress->show_sender = str_dup (argument);
  
  //free_string (ch->pcdata->in_progress->sender);
  //ch->pcdata->in_progress->sender = str_dup (argument);
}

/* Start writing a note */
static void do_nsubject (CHAR_DATA *ch, char *argument)
{
  
  if (!ch->pcdata->in_progress) {
    send_to_char("You must first provide a recipient!\n\r",ch);
    return;
  }
  
  if (ch->pcdata->in_progress->subject)
    free_string (ch->pcdata->in_progress->subject);
  ch->pcdata->in_progress->subject = str_dup(argument);
  send_to_char("Ok.\n\r",ch);
}

/* Used in warmboot/crashes to autosave a note in progress */
/* will switch to personal area and post to self           */
/* : Swordfish                                             */
void do_nautopost(CHAR_DATA *ch, char *argument)
{

  /* If no note was in progress return */
  if (!ch->pcdata->in_progress) 
    return;
  
  /* Set board to Personal */
  ch->pcdata->board = &boards[3];
  
  /* Selt to list to argument which should be "self" */
  if (ch->pcdata->in_progress->to_list)
    free_string (ch->pcdata->in_progress->to_list);

  /* Makes it easy if we want more then just self */
  ch->pcdata->in_progress->to_list = str_dup(argument);
  
  /* Fix those silly strings */
  smash_tilde(ch->pcdata->in_progress->subject);  
  smash_tilde(ch->pcdata->in_progress->text);
  
  /* Post it stamp it, get rid of it */
  finish_note (ch->pcdata->board, ch->pcdata->in_progress);  
  ch->pcdata->in_progress = NULL;
}

char *get_one_note_line(char *text)
{
  static char buf[200];
  char note_buf[MSL];
  int i=0;
   	
   if (IS_NULLSTR(text)) {
      sprintf(buf, "n/a");
      return buf;
   }
   
   if (strlen(text) >= 70) {
	strncpy(&note_buf[0], text, 70);
	sprintf(buf, "%s", colorstrem(note_buf));
   }
   else {
	sprintf(buf, "%s", colorstrem(text));
   }
   
   // If newline or the like, just cut it down
   for (i=0; i < strlen(buf); i++) {
	if (buf[i] == '\r') {
	  buf[i] = 0x00;
	  break;
	}
	if (buf[i] == '\n') {
	  buf[i] = 0x00;
	  break;
	}
   }   
   
   return buf;      
}

/* Start writing a note */
static void do_npost (CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  char from_name[MAX_STRING_LENGTH];
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim;
  int quiet=FALSE;

  int notenr=1;
  NOTE_DATA *p;

  if (!ch->pcdata->in_progress) {
    send_to_char("You have no note in progress.\n\r", ch);
    return;
  }

  if (get_trust(ch) < ch->pcdata->board->write_level) {
    send_to_char ("You cannot post notes on this board.\n\r",ch);
    return;
  }
  
  if (!str_cmp(argument, "quiet")) {
    quiet = TRUE;
  }

  // If IMM post wizi, make it quiet
  if (IS_IMMORTAL(ch) && ch->invis_level >= LEVEL_HERO) {
    quiet = TRUE;
  }

  /* Find what note number this is */
  if (ch->pcdata->board->note_first) {
    for (p = ch->pcdata->board->note_first; p->next; p = p->next )
	 notenr++;
  }

  /* Different Post options */
  if ((!str_cmp(argument, "dfpost")) && (ch->pcdata->df_level >=0)) {
    quiet = TRUE;
    do_nfrom(ch,ch->pcdata->df_name);
  }
  
  if ((!str_cmp(argument, "wkpost")) && IS_WOLFKIN(ch) && !IS_NULLSTR(ch->wkname)) {
    quiet = TRUE;	
    do_nfrom(ch, ch->wkname);
  }
  
  if ((!str_cmp(argument, "leaderpost")) && can_guild(ch)) {
    quiet = FALSE;
    sprintf(from_name, "%sLeader", clan_table[ch->clan].name);
    do_nfrom(ch, from_name);
  }

  if  (!str_cmp(argument, "facelesspost")) {
    quiet = FALSE;
    sprintf(from_name, "Faceless %s", capitalize(race_table[ch->race].name));
    do_nfrom(ch, from_name);

    /* Log it, since this can be abused and we want to know who did it if*/
    if (!IS_IMMORTAL(ch)) {
	 sprintf(buf, "$N posted a faceless note <%d> to <%s> in area <%s> with subject <%s>",
		    notenr+1, 
		    ch->pcdata->in_progress->to_list,
		    boards[board_number (ch->pcdata->board)].short_name,
		    ch->pcdata->in_progress->subject);
	 wiznet(buf, ch, NULL, WIZ_SECURE, 0,  get_trust(ch));
	 sprintf(buf, "%s posted a faceless note <%d> to <%s> in area <%s> with subject <%s>",
		    ch->name,
		    notenr+1, 
		    ch->pcdata->in_progress->to_list,
		    boards[board_number (ch->pcdata->board)].short_name,
		    ch->pcdata->in_progress->subject);
	 log_string(buf);

    }

  }

  sprintf(buf, "{W[{CNote{W]:{x New Note: %d  Posted to Area: %s.\n\r", 
		notenr+1,
		boards[board_number (ch->pcdata->board)].short_name);
  send_to_char(buf,ch);
  
  for ( d = descriptor_list; d; d = d->next ) {
    if (d->character != NULL && d->connected == CON_PLAYING) {
      victim = d->character;
      if (victim == ch) {
	   continue;
      }
      else {
	   if (!quiet) {
		buf[0] = '\0';
		if (is_note_to(victim, ch->pcdata->in_progress)) {

		  /* Personal */
		  if (is_personal_note_to(victim, ch->pcdata->in_progress)) {
		    sprintf(buf, "{W[{CNote{W]:{x New Personal Note: %d  In Area: %s  From: %s.\n\r"
				  "        Subj: %s\n\r"
				  "        Text: {B%s...{x\n\r",
				  notenr+1,
				  boards[board_number (ch->pcdata->board)].short_name,
				  ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender : ch->pcdata->in_progress->sender,
				  ch->pcdata->in_progress->subject,
				  get_one_note_line(ch->pcdata->in_progress->text));
		    if (can_see(ch, victim) && !IS_SET(victim->comm,COMM_QUIET) &&!ch->pcdata->in_progress->show_to_list) {
			 sprintf(buf2, "\n\r   >    %s is {gonline{x and has been notified with the new note.\n\r",
				    COLORNAME(victim));
			 send_to_char(buf2, ch);
		    }
			 }

		  /* Guild */
		  else if (is_guild_note_to(victim, ch->pcdata->in_progress)) {
		    sprintf(buf, "{W[{CNote{W]:{x New {8Guild Note{x: %d  In Area: %s  From: %s.\n\r"
				  "        Subj: %s\n\r"
				  "        Text: {B%s...{x\n\r",
				  notenr+1,
				  boards[board_number (ch->pcdata->board)].short_name,
				  ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender : ch->pcdata->in_progress->sender,
				  ch->pcdata->in_progress->subject,
				  get_one_note_line(ch->pcdata->in_progress->text));				  
		  }

		  /* SGuild */
		  else if (is_sguild_note_to(victim, ch->pcdata->in_progress)) {
		    sprintf(buf, "{W[{CNote{W]:{x New {SSGuild Note{x: %d  In Area: %s  From: %s.\n\r"
				  "        Subj: %s\n\r"
				  "        Text: {B%s...{x\n\r",
				  notenr+1,
				  boards[board_number (ch->pcdata->board)].short_name,
				  ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender : ch->pcdata->in_progress->sender,
				  ch->pcdata->in_progress->subject,
				  get_one_note_line(ch->pcdata->in_progress->text));
		  }

		  /* SSGuild */
		  else if (is_ssguild_note_to(victim, ch->pcdata->in_progress)) {
		    sprintf(buf, "{W[{CNote{W]:{x New {USSGuild Note{x: %d  In Area: %s  From: %s.\n\r"
				  "        Subj: %s\n\r"
				  "        Text: {B%s...{x\n\r",
				  notenr+1,
				  boards[board_number (ch->pcdata->board)].short_name,
				  ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender : ch->pcdata->in_progress->sender,
				  ch->pcdata->in_progress->subject,
				  get_one_note_line(ch->pcdata->in_progress->text));
		  }
		  
		  /* Immortal note */
		  else if (is_imm_note_to(victim, ch->pcdata->in_progress)) {
		    sprintf(buf, "{W[{CNote{W]:{x New Immortal Note: %d  In Area: %s  From: %s.\n\r"
				  "        Subj: %s\n\r"
				  "        Text: {B%s...{x\n\r",
				  notenr+1,
				  boards[board_number (ch->pcdata->board)].short_name,
				  ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender : ch->pcdata->in_progress->sender,
				  ch->pcdata->in_progress->subject,
				  get_one_note_line(ch->pcdata->in_progress->text));
		  }

		  /* Minion note */
		  else if (is_minion_note_to(victim, ch->pcdata->in_progress))  {
		    sprintf(buf, "{W[{CNote{W]:{x New {hMinion Note{x: %d  In Area: %s  From: %s.\n\r"
				  "        Subj: %s\n\r"
				  "        Text: {B%s...{x\n\r",
				  notenr+1,
				  boards[board_number (ch->pcdata->board)].short_name,
				  ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender : ch->pcdata->in_progress->sender,
				  ch->pcdata->in_progress->subject,
				  get_one_note_line(ch->pcdata->in_progress->text));
		  }
		  
		  /* All */
		  else {
		    sprintf(buf, "{W[{CNote{W]:{x New Public Note: %d  In Area: %s  From: %s.\n\r"
				  "        Subj: %s\n\r"
				  "        Text: {B%s...{x\n\r",
				  notenr+1,
				  boards[board_number (ch->pcdata->board)].short_name,
				  ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender : ch->pcdata->in_progress->sender,
				  ch->pcdata->in_progress->subject,
				  get_one_note_line(ch->pcdata->in_progress->text));
		  }
		  if (!IS_SET(victim->comm,COMM_QUIET))
		    send_to_char(buf, victim);
		}
	   }
      }
    }
  }
  
  smash_tilde(ch->pcdata->in_progress->subject);  
  smash_tilde(ch->pcdata->in_progress->text);
  
  finish_note (ch->pcdata->board, ch->pcdata->in_progress);  
  ch->pcdata->in_progress = NULL;

/* We don't want to show this to other in room - Atwain */
/*  act (BOLD C_B_GREEN "$n finishes $s note." NO_COLOR , ch, NULL, NULL, TO_ROOM);*/
}

/* Remove the bottom Line*/
static void do_nminus (CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  int len;
  bool found = FALSE;
  
  if (!ch->pcdata->in_progress) {
    send_to_char("You have no note in progress.\n\r", ch);
    return;
  }
  
  if (ch->pcdata->in_progress->text == NULL || ch->pcdata->in_progress->text[0] == '\0') {
    send_to_char("No lines left to remove.\n\r",ch);
    return;
  }
  
  strcpy(buf,ch->pcdata->in_progress->text);
  
  for (len = strlen(buf); len > 0; len--) {
    if (buf[len] == '\r') {
	 if (!found) {  /* back it up */
	   if (len > 0)
		len--;
	   found = TRUE;
	 }
	 else { /* found the second one */
	   buf[len + 1] = '\0';
	   free_string(ch->pcdata->in_progress->text);
	   ch->pcdata->in_progress->text = str_dup(buf);
	   return;
	 }
    }
  }
  buf[0] = '\0';
  free_string(ch->pcdata->in_progress->text);
  ch->pcdata->in_progress->text = str_dup(buf);
  return;
  
}
/* Start writing a note */
static void do_nplus (CHAR_DATA *ch, char *argument)
{
  BUFFER *buffer;
  
  if (!ch->pcdata->in_progress) {
    send_to_char("Specify recipient.\n\r",ch);
    return;
  }
  if (!ch->pcdata->in_progress->text) {
    if (strlen(ch->pcdata->in_progress->text)+strlen(argument) >= 4096) {
	 send_to_char( "Note too long.\n\r", ch );
	 return;
    }
  }
  buffer = new_buf();
  add_buf(buffer,ch->pcdata->in_progress->text);
  add_buf(buffer,argument);
  add_buf(buffer,"\n\r");
  
  if (ch->pcdata->in_progress->text)
    free_string( ch->pcdata->in_progress->text );
  ch->pcdata->in_progress->text = str_dup( buf_string(buffer) );
  free_buf(buffer);
  send_to_char( "Ok.\n\r", ch );
  return;
}


/* Show the note*/
static void do_nshow (CHAR_DATA *ch, char *argument)
{
  BUFFER *output;
  char buf[4*MAX_STRING_LENGTH];
  int num=0;
  int fillers=0;
  
  if (!ch->pcdata->in_progress) {
    send_to_char("No note in progress{y!!{x\n\r",ch);
    return;
  }
  
  output = new_buf();
  
  fillers = (20 - colorstrlen(ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender : ch->pcdata->in_progress->sender) - 1);
  
  if (use_christmas_layout()) {
    sprintf (buf,
		   "{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r"
		   "{rNote{W:{x %-4d       {rFrom{W:{x %s%*s {rDate{W:{x %s {x\n\r"
		   "{rArea{W:{x %-12s {rTo{W:{x %s {x\n\r"
		   "{rSubj{W:{x %s {x\n\r"
		   "{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r"
		   "%s"
		   "{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r",
		   num, 
		   ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender :ch->pcdata->in_progress->sender, 
		   fillers, "",
		   ch->pcdata->in_progress->date,
		   boards[board_number (ch->pcdata->board)].short_name, 
		   ch->pcdata->in_progress->show_to_list ? ch->pcdata->in_progress->show_to_list : ch->pcdata->in_progress->to_list, 
		   ch->pcdata->in_progress->subject, ch->pcdata->in_progress->text);
  }
  else {  
     sprintf (buf,
		    "{W---------------------------------------------------------------------------{x\n\r"
		    "{CNote{W:{x %-4d       {CFrom{W:{x %s%*s {CDate{W:{x %s {x\n\r"
		    "{CArea{W:{x %-12s {CTo{W:{x %s {x\n\r"
		    "{CSubj{W:{x %s {x\n\r"
		    "{W---------------------------------------------------------------------------{x\n\r"
		    "%s"
		    "{W---------------------------------------------------------------------------{x\n\r",
		    num, 
		    ch->pcdata->in_progress->show_sender ? ch->pcdata->in_progress->show_sender :ch->pcdata->in_progress->sender, 
		    fillers, "",
		    ch->pcdata->in_progress->date,
		    boards[board_number (ch->pcdata->board)].short_name, 
		    ch->pcdata->in_progress->show_to_list ? ch->pcdata->in_progress->show_to_list : ch->pcdata->in_progress->to_list,
		    ch->pcdata->in_progress->subject, ch->pcdata->in_progress->text);
  }
  add_buf(output,buf);
  page_to_char( buf_string(output), ch );
  free_buf(output);
  //send_to_char (buf,ch);
  return;
}



/* Start writing a note */
static void do_nwrite (CHAR_DATA *ch, char *argument)
{
  char *strtime;
  char buf[200];
  
  if (IS_NPC(ch)) /* NPC cannot post notes */
    return;
  
  if (get_trust(ch) < ch->pcdata->board->write_level) {
    send_to_char ("You cannot post notes on this board.\n\r",ch);
    return;
  }
  
  /* continue previous note, if any text was written*/ 
  if (!ch->pcdata->in_progress) {
    ch->pcdata->in_progress = new_note();
    ch->pcdata->in_progress->sender = str_dup (ch->name);
    
    /* convert to ascii. ctime returns a string which last character is \n, so remove that */	
    strtime = ctime (&current_time);
    strtime[strlen(strtime)-1] = '\0';
    
    ch->pcdata->in_progress->date = str_dup (strtime);
  }
  
  /* Don't want other to know do we ? */
  /* act (BOLD C_B_GREEN "$n starts writing a note." NO_COLOR , ch, NULL, NULL, TO_ROOM); */
  
  /* Begin writing the note ! */
  if (!ch->pcdata->in_progress->to_list) { /* Are we continuing an old note or not? */
    switch (ch->pcdata->board->force_type) {
    case DEF_NORMAL:
	 sprintf (buf, "If you press Return, default recipient \"" BOLD "%s" NO_COLOR "\" will be chosen.\n\r",
			ch->pcdata->board->names);
	 break;
    case DEF_INCLUDE:
	 sprintf (buf, "The recipient list MUST include \"" BOLD "%s" NO_COLOR "\". If not, it will be added automatically.\n\r",
			ch->pcdata->board->names);
	 break;
	 
    case DEF_EXCLUDE:
	 sprintf (buf, "The recipient of this note must NOT include: \"" BOLD "%s" NO_COLOR "\".",
			ch->pcdata->board->names);
	 
	 break;
    }			
    
    send_to_char (buf,ch);
    send_to_char ("\n\r" BOLD C_B_YELLOW "To" NO_COLOR ":      ",ch);
    
    ch->desc->connected = CON_NOTE_TO;
    /* nanny takes over from here */
    
  }
  else { /* we are continuing, print out all the fields and the note so far*/
    ch->desc->connected = CON_PLAYING;
    string_append(ch,&ch->pcdata->in_progress->text); 
  }  
}

static void do_nread (CHAR_DATA *ch, char *argument)
{
  char arg1[MSL];
  NOTE_DATA *p;
  int count = 0, number;
  time_t *last_note = &ch->pcdata->last_note[board_number(ch->pcdata->board)];
  int last = 0;
  bool fCheck=FALSE;
  
  argument = one_argument(argument, arg1);

  if (IS_IMMORTAL(ch) && !str_cmp(argument, "check"))
    fCheck = TRUE;
  
  if (!str_cmp(arg1, "again")) { /* read last note again */
    p = ch->pcdata->last_read_note;
    if (!p) {
	 send_to_char ("You need to read a note before you can read it again.\n\r",ch);
	 return;	 
    }
    show_note_to_char (ch,p,count,fCheck);
  }
  else if (is_number (arg1)) {
    number = atoi(arg1);
    
    for (p = ch->pcdata->board->note_first; p; p = p->next)
	 if (++count == number)
	   break;
    
    if (!p || !is_note_to(ch, p))
	 send_to_char ("No such note.\n\r",ch);
    else {
	 show_note_to_char (ch,p,count,fCheck);
	 *last_note =  UMAX (*last_note, p->date_stamp);
    }
  }
  else { /* Find next area with notes, or return to first area */
    char buf[200];
    
    count = 1;
    for (p = ch->pcdata->board->note_first; p ; p = p->next, count++)
	 if ((p->date_stamp > *last_note) && is_note_to(ch,p)) {
	   show_note_to_char (ch,p,count,fCheck);
	   /* Advance if new note is newer than the currently newest for that char */
	   *last_note =  UMAX (*last_note, p->date_stamp);
	   return;
	 }
    
    /* What area was we standing in ? */
    last = board_number(ch->pcdata->board);
    
    /* If unread notes, change to area and tell Player */
    if (next_unread_board(ch)) {
	 sprintf(buf, "You have no new notes in area %s.\n\r", boards[last].short_name);
	 send_to_char (buf,ch);
	 sprintf(buf, "Changed to area %s.\n\r", ch->pcdata->board->short_name);
	 send_to_char (buf,ch);
    }
    else {
	 /* Return back to first Note area when no more unread notes - Atwain */
	 ch->pcdata->board = &boards[0];
	 sprintf(buf, "You have no unread notes. Returning to area %s.\n\r", ch->pcdata->board->short_name);
	 send_to_char (buf,ch);
	 return;
    }
  }
}

/* Remove a note */
static void do_nremove (CHAR_DATA *ch, char *argument)
{
  NOTE_DATA *p;
  
  if (!is_number(argument)) {
    send_to_char ("Remove which note?\n\r",ch);
    return;
  }
  
  p = find_note (ch, ch->pcdata->board, atoi(argument));
  if (!p) {
    send_to_char ("No such note.\n\r",ch);
    return;
  }
  
  if (str_cmp(ch->name,p->sender) && (get_trust(ch) < MAX_LEVEL)) {
    send_to_char ("You are not authorized to remove this note.\n\r",ch);
    return;
  }
  
  unlink_note (ch->pcdata->board,p);
  free_note (p);
  send_to_char ("Note removed!\n\r",ch);
  
  save_board(ch->pcdata->board); /* save the board */
}

static void do_nclear (CHAR_DATA *ch, char *argument)
{
  if (ch->pcdata->in_progress) {
    free_note (ch->pcdata->in_progress);
    ch->pcdata->in_progress = NULL;
    send_to_char ("Working note cleared from buffer.\n\r", ch);
  }
  else
    send_to_char ("No note to clear from buffer.\n\r",ch);
}

static void do_nadjust (CHAR_DATA *ch, char *argument)
{
  NOTE_DATA *p = NULL;
  int number=0;
  char *strtime=NULL;
  char leader[MAX_STRING_LENGTH];

  if (is_number (argument)) {
    number = atoi(argument);
  }
  else {
    send_to_char ("Adjust which note?\n\r",ch);
    return;
  }
  
  p = find_note (ch, ch->pcdata->board, atoi(argument));
  
  if (!p) {
    send_to_char ("No such note.\n\r",ch);
    return;
  }
  
  memset(leader, 0x00, sizeof(leader));

  // Is ch sender, else check if ch is leader
  if (str_cmp(ch->name,p->sender) && (get_trust(ch) < MAX_LEVEL)) {
    
    // Guild Leader?
    if (is_leader(ch)) {
	 sprintf(leader, "%sLeader", player_clan(ch));
	 if (str_cmp(leader,p->sender) && (get_trust(ch) < MAX_LEVEL)) {
	   send_to_char ("You are not authorized to adjust this note.\n\r",ch);
	   return;
	 }
    }
    else if (is_sguild_leader(ch)) {
	 sprintf(leader, "%sLeader", player_sguild(ch));
	 if (str_cmp(leader,p->sender) && (get_trust(ch) < MAX_LEVEL)) {
	   send_to_char ("You are not authorized to adjust this note.\n\r",ch);
	   return;
	 }
    }
    else {
	 send_to_char ("You are not authorized to adjust this note.\n\r",ch);
	 return;
    }
  }
  
  /* Create new note and copy elements from old note */
  ch->pcdata->in_progress = new_note();
  ch->pcdata->in_progress->sender = str_dup(p->sender);
  if (p->show_sender)
    ch->pcdata->in_progress->show_sender = str_dup(p->show_sender);
  /* convert to ascii. ctime returns a string which last character is \n, so remove that */	
  strtime = ctime (&current_time);
  strtime[strlen(strtime)-1] = '\0';
  ch->pcdata->in_progress->expire = current_time + 30 * 60 * 60 * 24;
  ch->pcdata->in_progress->date = str_dup (strtime);
  ch->pcdata->in_progress->subject = str_dup(p->subject);
  ch->pcdata->in_progress->text = str_dup(p->text);
  ch->pcdata->in_progress->to_list = str_dup(p->to_list);
  
  if (p->show_to_list)
    ch->pcdata->in_progress->show_to_list = str_dup(p->show_to_list);
  
  /* Remove note from board */
  unlink_note (ch->pcdata->board,p);
  free_note (p);
  save_board(ch->pcdata->board);
  
  /* Done */
  send_to_char ("Note ready to be adjusted.\n\r",ch);
}

static void do_nforward (CHAR_DATA *ch, char *argument)
{
  NOTE_DATA *p = NULL;
  int number=0;
  char *strtime=NULL;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  BUFFER *buffer;
  
  argument = one_argument (argument, arg);
  
  if (ch->pcdata->in_progress) {
    send_to_char("You have a note in progress. Please finish this before forwarding a note.\n\r", ch);
    return;
  }
  
  if (is_number (arg)) {
    number = atoi(arg);
  }
  else {
    send_to_char ("Forward which note?\n\r",ch);
    return;
  }

  if (!argument[0]) {
    send_to_char("Foward note to whom?\n\r", ch);
    return;
  }

  p = find_note (ch, ch->pcdata->board, number);

  if (!p) {
    send_to_char ("No such note.\n\r",ch);
    return;
  }

  /* Init */
  memset(buf, 0x00, sizeof(buf));
  buffer = new_buf();
  
  /* Forward 'new' note */
  ch->pcdata->in_progress = new_note();
  ch->pcdata->in_progress->sender = str_dup(ch->name);
  /* convert to ascii. ctime returns a string which last character is \n, so remove that */	
  strtime = ctime (&current_time);
  strtime[strlen(strtime)-1] = '\0';
  ch->pcdata->in_progress->expire = current_time + 30 * 60 * 60 * 24;
  ch->pcdata->in_progress->date = str_dup (strtime);

  sprintf(buf, "%s%s", "Fwd: ", p->subject);
  ch->pcdata->in_progress->subject = str_dup(buf);

  /* Build forward text */
  sprintf(buf, "From: %s\n", p->show_sender ? p->show_sender : p->sender);
  add_buf(buffer, buf);
  sprintf(buf, "Date: %s\n\n", p->date);
  add_buf(buffer, buf);
  add_buf(buffer, p->text);
  ch->pcdata->in_progress->text = str_dup(buffer->string);
  
  ch->pcdata->in_progress->to_list = str_dup(argument);
  
  free_buf(buffer);

  do_npost(ch, "");
}

static void do_nreply (CHAR_DATA *ch, char *argument)
{
  NOTE_DATA *p = NULL;
  char *strtime;
  char buf[MAX_STRING_LENGTH];

  p = ch->pcdata->last_read_note;

  // If no note to reply on
  if (!p) {
    send_to_char ("You need to read a note before you can reply.\n\r",ch);
    return;
  }

  // One thing at a time
  if (ch->pcdata->in_progress) {
    send_to_char ("You already have a note in progress!\n\r",ch);
    return;
  }
  
  // Ok, make a new note - and fill in
  ch->pcdata->in_progress = new_note();
  ch->pcdata->in_progress->sender = str_dup(ch->name);
  
  /* convert to ascii. ctime returns a string which last character is \n, so remove that */	
  strtime = ctime (&current_time);
  strtime[strlen(strtime)-1] = '\0';
  
  ch->pcdata->in_progress->expire = current_time + 30 * 60 * 60 * 24;
  ch->pcdata->in_progress->date = str_dup (strtime);
  sprintf(buf, "Re: %s", p->subject);
  ch->pcdata->in_progress->subject = str_dup (buf);
  ch->pcdata->in_progress->text = str_dup ("");
  
  if (ch->pcdata->in_progress->to_list)
    free_string (ch->pcdata->in_progress->to_list);
  
  sprintf(buf, "%s %s", p->sender, p->to_list);
  ch->pcdata->in_progress->to_list = str_dup(buf);
  
  if (p->show_sender || p->show_to_list) {
    sprintf(buf, "%s %s",
		  p->show_sender ? p->show_sender : p->sender, 
		  p->show_to_list ? p->show_to_list : p->to_list);
    ch->pcdata->in_progress->show_to_list = str_dup(buf);
  }

  // Reset
  ch->pcdata->last_read_note = NULL;
  
  send_to_char( "Note ready to be replyed on.\n\r", ch );
}

static void do_nsearch(CHAR_DATA *ch, char *argument)
{
  int num=0;
  int found = 0;
  int fillers = 0;
  time_t last_note;
  NOTE_DATA *p;
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  BUFFER *output;
  int i=0;


  argument = one_argument (argument, arg);

  if (IS_NULLSTR(arg) || IS_NULLSTR(argument)) {
    send_to_char("Syntax: note search <from/to> <name>\n\r", ch);
    return;
  }

  if (str_cmp(arg, "from") && str_cmp(arg, "to")) {
    send_to_char("Syntax: note search <from/to> <name>\n\r", ch);
    return;
  }
  
  output   = new_buf();
  
  // search all boards
  for (i = 0; i < MAX_BOARD; i++) {
    num=0;
    found=0;
    
    if (unread_notes(ch,&boards[i]) == BOARD_NOACCESS)
	 continue;

    ch->pcdata->board = &boards[i];
    
    buf [0] = '\0';
    sprintf (buf, "{xListing notes found in area {D[{W %s {D]{W:{x\n\r",
		   boards[board_number (ch->pcdata->board)].short_name);

    add_buf(output,buf);

    last_note = ch->pcdata->last_note[board_number (ch->pcdata->board)];

    /* Note List From                                                  */
    /*-----------------------------------------------------------------*/
    if (!str_cmp (arg, "from")) {
	 for (p = ch->pcdata->board->note_first; p; p = p->next) {
	   num++;
	   if (is_note_to(ch,p)) {
		if (!str_cmp (argument, p->show_sender ? p->show_sender : p->sender)) {
		  found++;
		  buf [0] = '\0';
		  fillers = (16 - colorstrlen(p->show_sender ? p->show_sender : p->sender) - 1);
		  sprintf (buf, "[%4d%s] [%s] {CFrom{W:{x %s%*s {CSubj{W:{x %s\n\r",
				 num,
				 last_note < p->date_stamp ? "N" : " ",
				 sec2str(p->date_stamp),
				 p->show_sender ? p->show_sender : p->sender, 
				 fillers, "",
				 p->subject);
		  add_buf(output,buf);
		}
	   }
	 }
    }

    /* Note List To                                                    */
    /*-----------------------------------------------------------------*/
    if (!str_cmp (arg, "to")) {
	 for (p = ch->pcdata->board->note_first; p; p = p->next) {
	   num++;
	   if (is_note_to(ch,p)) {
		if (is_name(argument, p->to_list)) {
		  found++;
		  buf [0] = '\0';
		  fillers = (16 - colorstrlen(p->show_sender ? p->show_sender : p->sender) - 1);
		  sprintf (buf, "[%4d%s] [%s] {CFrom{W:{x %s%*s {CSubj{W:{x %s\n\r",
				 num,
				 last_note < p->date_stamp ? "N" : " ",
				 sec2str(p->date_stamp),
				 p->show_sender ? p->show_sender : p->sender, 
				 fillers, "",
				 p->subject);
		  add_buf(output,buf);
		}
	   }
	 }
    }    	 
  }
  
  page_to_char( buf_string(output), ch );
  free_buf(output);  
  
  return;
}

/* List all notes or if argument given, list N of the last notes */
/* Shows REAL note numbers! */
static void do_nlist (CHAR_DATA *ch, char *argument)
{
  int count= 0, show = 0, num = 0, has_shown = 0;
  int found = 0;
  int fillers = 0;
  time_t last_note;
  NOTE_DATA *p;
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  BUFFER *output;
  
  output   = new_buf();
  argument = one_argument (argument, arg);
  
  if (is_number(arg)) {	 /* first, count the number of notes */
    show = atoi(arg);
    
    for (p = ch->pcdata->board->note_first; p; p = p->next)
	 if (is_note_to(ch,p))
	   count++;
  }
  
  buf [0] = '\0';
  sprintf (buf, "{xListing notes in area {D[{W %s {D]{W:{x\n\r",
		 boards[board_number (ch->pcdata->board)].short_name);
  
  /* send_to_char(buf,ch); */
  add_buf(output,buf);
  
  last_note = ch->pcdata->last_note[board_number (ch->pcdata->board)];
  
  /* Note List From                                                  */
  /*-----------------------------------------------------------------*/
  if (!str_cmp (arg, "from")) {
    for (p = ch->pcdata->board->note_first; p; p = p->next) {
	 num++;
	 if (is_note_to(ch,p)) {
	   has_shown++;
	   if (!show || ((count-show) < has_shown)) {
		if (!str_cmp (argument, p->show_sender ? p->show_sender : p->sender)) {
		  found++;
		  buf [0] = '\0';
		  fillers = (20 - colorstrlen(p->show_sender ? p->show_sender : p->sender) - 1);
		  sprintf (buf, "[%4d%s] [%s] {CFrom{W:{x %s%*s {CSubj{W:{x %s\n\r",
				 num,
				 last_note < p->date_stamp ? "N" : " ",
				 sec2str(p->date_stamp),
				 p->show_sender ? p->show_sender : p->sender, 
				 fillers, "",
				 p->subject);
		  /* send_to_char (buf,ch); */
		  add_buf(output,buf);
		}
	   }
	 }
    }
    if (found == 0) {
	 buf [0] = '\0';
	 sprintf(buf, "No notes found from %s.\n\r", argument);
	 /*send_to_char (buf,ch);*/
	 add_buf(output,buf);
    }
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
  }

  /* Note List To                                                    */
  /*-----------------------------------------------------------------*/
  if (!str_cmp (arg, "to")) {
    for (p = ch->pcdata->board->note_first; p; p = p->next) {
	 num++;
	 if (is_note_to(ch,p)) {
	   has_shown++;
	   if (!show || ((count-show) < has_shown)) {
		//if (!str_cmp (argument, p->to_list)) {
		if (is_name(argument, p->to_list)) {
		  found++;
		  buf [0] = '\0';
		  fillers = (20 - colorstrlen(p->show_sender ? p->show_sender : p->sender) - 1);
		  sprintf (buf, "[%4d%s] [%s] {CFrom{W:{x %s%*s {CSubj{W:{x %s\n\r",
				 num,
				 last_note < p->date_stamp ? "N" : " ",
				 sec2str(p->date_stamp),
				 p->show_sender ? p->show_sender : p->sender, 
				 fillers, "",
				 p->subject);
		  /* send_to_char (buf,ch); */
		  add_buf(output,buf);
		}
	   }
	 }
    }
    if (found == 0) {
	 buf [0] = '\0';
	 sprintf(buf, "No notes found to %s.\n\r", argument);
	 /* send_to_char (buf,ch); */
	 add_buf(output,buf);
    }
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
  }

  /* Note List New                                                   */
  /*-----------------------------------------------------------------*/
  if (!str_cmp (arg, "new")) {
    for (p = ch->pcdata->board->note_first; p; p = p->next) {
	 num++;
	 if (is_note_to(ch,p)) {
	   has_shown++;
	   if (!show || ((count-show) < has_shown)) {
		if (last_note < p->date_stamp) {
		  found++;
		  buf [0] = '\0';
		  fillers = (20 - colorstrlen(p->show_sender ? p->show_sender : p->sender) - 1);
		  sprintf (buf, "[%4d%s] [%s] {CFrom{W:{x %s%*s {CSubj{W:{x %s\n\r",
				 num,
				 last_note < p->date_stamp ? "N" : " ",
				 sec2str(p->date_stamp),
				 p->show_sender ? p->show_sender : p->sender,
				 fillers, "",
				 p->subject);
		  /* send_to_char (buf,ch); */
		  add_buf(output,buf);
		}
	   }
	 }
    }
    if (found == 0) {
	 buf [0] = '\0';
	 sprintf(buf, "No new notes found.\n\r");
	 /* send_to_char (buf,ch); */
	 add_buf(output,buf);
    }
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
  }
  
  /* Normal note list                                                */
  /*-----------------------------------------------------------------*/
  for (p = ch->pcdata->board->note_first; p; p = p->next) {
    num++;
    if (is_note_to(ch,p)) {
	 has_shown++; /* note that we want to see X VISIBLE note, not just last X */
	 if (!show || ((count-show) < has_shown)) {
	   buf [0] = '\0';
	   fillers = (20 - colorstrlen(p->show_sender ? p->show_sender : p->sender) - 1);
	   sprintf (buf, "[%4d%s] [%s] {CFrom{W:{x %s%*s {CSubj{W:{x %s\n\r",
			  num,
			  last_note < p->date_stamp ? "N" : " ",
			  sec2str(p->date_stamp),
			  p->show_sender ? p->show_sender : p->sender,
			  fillers, "",
			  p->subject);
	   /* send_to_char (buf,ch); */
	   add_buf(output,buf);
	 }
    }
  }
  page_to_char( buf_string(output), ch );
  free_buf(output);
}

/* catch up with some notes */
static void do_ncatchup (CHAR_DATA *ch, char *argument)
{
  NOTE_DATA *p;
  char buf[MAX_INPUT_LENGTH];
  int i=0;
  
  if (!IS_NULLSTR(argument) && !str_cmp(argument, "all")) {
    for (i = 0; i < MAX_BOARD; i++) {
	 ch->pcdata->board = &boards[i];
	 for (p = ch->pcdata->board->note_first; p && p->next; p = p->next);
	 if (p)
	   ch->pcdata->last_note[i] = p->date_stamp;	 
    }
    send_to_char("All notes skipped in all areas.\n\r", ch);
    ch->pcdata->board = &boards[0];
  }
  else {  
    /* Find last note */	
    for (p = ch->pcdata->board->note_first; p && p->next; p = p->next);
    
    if (!p) {
	 sprintf(buf, "You have no unread notes in area %s.\n\r", boards[board_number (ch->pcdata->board)].short_name);
	 send_to_char(buf, ch);
    }
    else {
	 ch->pcdata->last_note[board_number(ch->pcdata->board)] = p->date_stamp;
	 sprintf(buf, "All notes skipped in area %s.\n\r", boards[board_number (ch->pcdata->board)].short_name);
	 send_to_char(buf, ch);
    }
  }
}

static void do_nformat (CHAR_DATA *ch, char *argument) {
  
  if (!ch->pcdata->in_progress) {
    send_to_char("You have no note in progress.\n\r", ch);
    return;
  }
  
  ch->pcdata->in_progress->text = format_string( ch->pcdata->in_progress->text );
  send_to_char("Note formatted.\n\r", ch);
  return;
}

/* Dispatch function for backwards compatibility */
void do_note (CHAR_DATA *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  
  if (IS_NPC(ch))
    return;
  
  argument = one_argument (argument, arg);
  
  if ((!arg[0]) || (!str_cmp(arg, "read"))) /* 'note' or 'note read X' */
    do_nread (ch, argument);
  
  else if (!str_cmp (arg, "list"))
    do_nlist (ch, argument);
  
  else if (!str_cmp (arg, "to"))
    do_nto (ch, argument);
  
  else if (!str_cmp (arg, "from") && IS_IMMORTAL(ch))
    do_nfrom(ch, argument);
  
  else if (!str_prefix (arg, "subject"))
    do_nsubject (ch, argument);
  
  else if (!str_cmp (arg, "+"))
    do_nplus (ch, argument);
  
  else if (!str_cmp (arg, "-"))
    do_nminus (ch, argument);
  
  else if (!str_cmp (arg, "show"))
    do_nshow (ch, argument);
  
  else if (!str_cmp (arg, "dfpost"))
    do_npost (ch, arg);

  else if (!str_cmp (arg, "post"))
    do_npost (ch, argument);
  
  else if (!str_cmp (arg, "send"))
    do_npost (ch, argument);
  
  else if (!str_cmp (arg, "write"))
    do_nwrite (ch, argument);
  
  else if (!str_cmp (arg, "remove"))
    do_nremove (ch, argument);
  
  else if (!str_cmp (arg, "purge"))
    send_to_char ("Obsolete.\n\r",ch);
  
  else if (!str_cmp (arg, "archive"))
    send_to_char ("Obsolete.\n\r",ch);
  
  else if (!str_prefix (arg, "area"))
    do_board(ch,argument);
  
  else if (!str_prefix (arg, "catchup"))
    do_ncatchup (ch, argument);
  
  else if (!str_prefix (arg, "scan"))
    do_board (ch, "");

  else if (!str_prefix (arg, "top"))
    do_topboard(ch, "");
  
  else if (!str_cmp (arg, "clear"))
    do_nclear(ch, "");
  
  else if(!str_cmp (arg, "adjust"))
    do_nadjust(ch, argument);
  
  else if(!str_cmp (arg, "forward"))
    do_nforward(ch, argument);
  
  else if(!str_cmp (arg, "format"))
    do_nformat(ch, argument);

  else if(!str_cmp (arg, "reply"))
    do_nreply(ch, argument);

  else if(!str_cmp (arg, "search"))
    do_nsearch(ch, argument);
  
  else 
    /* do_help (ch, "note"); */
    send_to_char("\nType '{Whelp note{x' for usage.\n\r", ch);
}

// Count number of notes in -this- board, from a given sender
int num_notes_from(BOARD_DATA *board, char *from)
{
  int count=0;
  NOTE_DATA *note;
  
  for (note = board->note_first; note; note = note->next) {
    if (!str_cmp(note->sender, from))
	 count++;
  }
  
  return count;
}

// Simple data struct
typedef struct show_data {
  char name[128];
  int num;
}show_data;

// count the n, top senders in -this- board
void show_top_notes (CHAR_DATA *ch, BOARD_DATA *board)
{
  char buf [MAX_INPUT_LENGTH];
  int show=3;
  struct show_data sd[show];
  int i=0;
  int k=0;
  NOTE_DATA *note;
  int num_notes=0;
  bool inserted=FALSE;
  int fillers=0;

  // Reset array every time
  for (i=0; i < show; i++) {
    memset(sd[i].name, 0x00, sizeof(sd[i].name));
    sd[i].num = 0;
  }
   
  // Can you see me?
  if (board->read_level > get_trust(ch))
    return;
  
  // Count notes in this board
  for (note = board->note_first; note; note = note->next) {    
    num_notes = num_notes_from(board, note->sender);
 
    // Check if name already inserted
    for (i=0; i < show; i++) {
	 if (!str_cmp(sd[i].name, note->sender)) {
	   inserted = TRUE;
	   break;
	 }
	 else {
	   inserted = FALSE;	   
	 }
    }

    // Insert if higher
    if (num_notes > 0) {
	 for (i=0; i < show; i++) {
	   if (num_notes > sd[i].num && !inserted) {
		
		// Last in array - insert it
		if (i == show-1) {
		  sprintf(sd[i].name, "%s", note->sender);
		  sd[i].num = num_notes;
		  inserted = TRUE;
		}
		// Arrange array - "shift right"
		// then insert new elem
		else {
		  if (sd[i].num >= sd[i+1].num) {
		    for (k=show-1; k > 0; k--) {
			 sprintf(sd[k].name, "%s", sd[k-1].name);
			 sd[k].num = sd[k-1].num;
		    }
		  }
		  sprintf(sd[i].name, "%s", note->sender);
		  sd[i].num = num_notes;
		  inserted = TRUE;
		}
	   }
	 }
    }
  }

  for (i=0; i < show; i++) {
    if (!IS_NULLSTR(sd[i].name)) {
	 if (colorstrlen(sd[i].name) > 16)
	   sd[i].name[16] = '\0';
	 fillers = (16 - colorstrlen(sd[i].name));
    }
    else
	 fillers = (16 - colorstrlen("{Rn/a{x"));
    sprintf(buf, "[%s%3d{x] %s%*s", sd[i].num > 0 ? "{y" : "{x", sd[i].num, !IS_NULLSTR(sd[i].name) ? sd[i].name : "{Rn/a{x", fillers, "");
    send_to_char(buf, ch);
  }
  
  send_to_char("\n\r", ch);
  return;
}

void do_topboard(CHAR_DATA *ch, char *argument)
{
  int i=0;
  char buf [MAX_INPUT_LENGTH];
  
  // No NPC
  if (IS_NPC(ch))
    return;

  // Only Imms
  if (!IS_IMMORTAL(ch)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  
  send_to_char ("{W-----------------------------------------------------------------------------{x\n\r"
			 "{CName           #1                    #2                    #3 {x \n\r"
			 "{W-----------------------------------------------------------------------------{x\n\r",ch);
  
  for (i = 0; i < MAX_BOARD; i++) {   
    sprintf (buf, "%-12s: ", boards[i].short_name);
    send_to_char (buf,ch);
    show_top_notes(ch, &boards[i]);
  }

  send_to_char ("{W-----------------------------------------------------------------------------{x\n\r",ch);
  return;
}

void boards_stat(CHAR_DATA *ch) 
{
  char buf [MAX_INPUT_LENGTH];
  int i;
  int total=0;
  int total_sent=0;
  int percent=0;

  for (i = 0; i < MAX_BOARD; i++) {

    //Don't count Newplayers notes, but all others
    if (str_cmp(boards[i].short_name, "Newplayers")) {
	 if (total_notes(ch,&boards[i]) != BOARD_NOACCESS)
	   total    += total_notes (ch,&boards[i]);
	 total_sent += total_sent_notes(ch, &boards[i]);
    }
  }

  if (total > 0)
    percent=(int)((total_sent/(double)total)*100);
  
  if (use_christmas_layout())
     sprintf(buf, "    Posted Notes: {G%d{x  Total Notes: {G%d{x  Percent of total: {G%d{x%%\n\r", total_sent, total, percent);
  else
     sprintf(buf, "    Posted Notes: {y%d{x  Total Notes: {y%d{x  Percent of total: {y%d{x%%\n\r", total_sent, total, percent);
  send_to_char(buf, ch);
  return;
}

/* Show all accessible boards with their numbers of unread messages OR
   change board. New board name can be given as a number or as a name (e.g.
    board personal or board 4 */
void do_board (CHAR_DATA *ch, char *argument)
{
  int i, count, number;
  char buf[200];
  
  if (IS_NPC(ch))
    return;
  
  if (!argument[0]) { /* show boards */
    int unread=0;
    int total=0;
    int personal=0;
    int guild=0;
    int sguild=0;
    int ssguild=0;
    int imm=0;
    
    count = 1;
    if (use_christmas_layout()) {
	 send_to_char("{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r"
			    "{x    {WName        Unread   Guild  SGuild SSGuild   Pers.    Imm.   Total{x\n\r"
			    "{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r", ch);
    }
    else {
	 send_to_char ("{W----------------------------------------------------------------------{x\n\r"
				"{x    {CName        Unread   Guild  SGuild SSGuild   Pers.    Imm.   Total{x\n\r"
				"{W----------------------------------------------------------------------{x\n\r",ch);
    }
    for (i = 0; i < MAX_BOARD; i++) {
	 
	 // Find how many of the different sections are unread.
	 unread   = unread_notes (ch,&boards[i]);           /* how many unread notes?    */
	 total    = total_notes (ch,&boards[i]);            /* total notes in this board */
	 personal = unread_personal_notes (ch,&boards[i]);  /* how many personal notes?  */
	 guild    = unread_guild_notes(ch,&boards[i]);      /* how many guild notes?     */
	 sguild   = unread_sguild_notes(ch,&boards[i]);     /* how many sguild notes?    */
	 ssguild  = unread_ssguild_notes(ch,&boards[i]);    /* how many ssguild notes?    */
	 imm      = unread_imm_notes(ch, &boards[i]);       /* how many imm notes?       */
	 
	 /* If user has access, show board. Else keep it hidden */
	 if (unread != BOARD_NOACCESS) { 
	   sprintf (buf, "%s %s%-12s{x  %s%4d{x    %s%4d{x    %s%4d{x    %s%4d{x    %s%4d{x    %s%4d{x    %4d\n\r", 
			  board_number (ch->pcdata->board) == i ? use_christmas_layout() ? "{D[{G@{D]{x" : "{D[{W*{D]{x" : "{D[{x {D]{x",
			  unread   ? "{W" : "{x",
			  boards[i].short_name, 
			  unread   ? "{W" : "{x", 
			  unread,
			  guild    ? "{8" : "{x",
			  guild,
			  sguild   ? "{S" : "{x",
			  sguild,
			  ssguild  ? "{U" : "{x",
			  ssguild,
			  personal ? "{k" : "{x",
			  personal,
			  imm      ? "{i" : "{x",
			  imm,
			  total);
	   send_to_char (buf,ch);	                    
	   count++;
	 } /* if has access */	 
    } /* for each board */

    if (use_christmas_layout()) {
	 send_to_char("{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r", ch);
	 boards_stat(ch);
	 send_to_char("{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{g-{R-{x\n\r", ch);
    }
    else {
	 send_to_char ("{W----------------------------------------------------------------------{x\n\r",ch);
	 boards_stat(ch);
	 send_to_char ("{W----------------------------------------------------------------------{x\n\r",ch);
    }      
    do_poll(ch,"new");
    return;			
  } /* if empty argument */
  
  /* Change board based on its number */
  if (is_number(argument)) {
    count = 0;
    number = atoi(argument);
    for (i = 0; i < MAX_BOARD; i++)
	 if (unread_notes(ch,&boards[i]) != BOARD_NOACCESS)		
	   if (++count == number)
		break;
    
    if (count == number) /* found the board.. change to it */ {
	 ch->pcdata->board = &boards[i];
	 sprintf (buf, "Note area changed to %s. %s.\n\r",boards[i].short_name,
		     (get_trust(ch) < boards[i].write_level) 
		     ? "You can only read here" 
		     : "You can both read and post here");
	 send_to_char (buf,ch);
    }			
    else /* no such board */
	 send_to_char ("That is not a valid area name or number.\n\r",ch);
    
    return;
  }
  
  /* Non-number given, find board with that name */  
  for (i = 0; i < MAX_BOARD; i++)
    if (!str_prefix(argument, boards[i].short_name))
	 break;
  
  if (i == MAX_BOARD) {
    send_to_char ("That is not a valid area name or number.\n\r",ch);
    return;
  }
  
  /* Does ch have access to this board? */	
  if (unread_notes(ch,&boards[i]) == BOARD_NOACCESS) {
    send_to_char ("That is not a valid area name or number.\n\r",ch);
    return;
  }
  
  ch->pcdata->board = &boards[i];
  sprintf (buf, "Note area changed to %s. %s.\n\r",boards[i].short_name,
		 (get_trust(ch) < boards[i].write_level) 
		 ? "You can only read here" 
		 : "You can both read and post here");
  send_to_char (buf,ch);
}

/* Send a note to someone on the personal board */
void personal_message (const char *sender, const char *to, const char *subject, const int expire_days, const char *text)
{
	make_note ("Personal", sender, to, subject, expire_days, text);
}

void make_note (const char* board_name, const char *sender, const char *to, const char *subject, const int expire_days, const char *text)
{
	int board_index = board_lookup (board_name);
	BOARD_DATA *board;
	NOTE_DATA *note;
	char *strtime;
	
	if (board_index == BOARD_NOTFOUND)
	{
		bug ("make_note: board not found",0);
		return;
	}
	
	if (strlen(text) > MAX_NOTE_TEXT)
	{
		bug ("make_note: text too long (%d bytes)", strlen(text));
		return;
	}
	
	
	board = &boards [board_index];
	
	note = new_note(); /* allocate new note */
	
	note->sender = str_dup (sender);
	note->to_list = str_dup(to);
	note->subject = str_dup (subject);
	note->expire = current_time + expire_days * 60 * 60 * 24;
	note->text = str_dup (text);

	/* convert to ascii. ctime returns a string which last character is \n, so remove that */	
	strtime = ctime (&current_time);
	strtime[strlen(strtime)-1] = '\0';
	
	note->date = str_dup (strtime);
	
	finish_note (board, note);
	
}

/* Tries to change to next board with unread notes */
static bool next_unread_board (CHAR_DATA *ch)
{
  int i = 0;
  
  for (i = 0; i < MAX_BOARD; i++) {
    if (unread_notes(ch,&boards[i]) == BOARD_NOACCESS)
	 continue;
    
    if (i == MAX_BOARD)
	 return FALSE;
    
    if (unread_notes(ch,&boards[i]) > 0) {
	 ch->pcdata->board = &boards[i];
	 return TRUE;
    }
  }
  return FALSE;
}

/* tries to change to the next accessible board */
/* static bool next_board (CHAR_DATA *ch) */
/* { */
/* 	int i = board_number (ch->pcdata->board) + 1; */
	
/* 	while ((i < MAX_BOARD) && (unread_notes(ch,&boards[i]) == BOARD_NOACCESS)) */
/* 		i++; */
		
/* 	if (i == MAX_BOARD) */
/* 		return FALSE; */
/* 	else */
/* 	{ */
/* 		ch->pcdata->board = &boards[i]; */
/* 		return TRUE; */
/* 	} */
/* } */

void handle_con_note_to (DESCRIPTOR_DATA *d, char * argument)
{
	char buf [MAX_INPUT_LENGTH];
	CHAR_DATA *ch = d->character;

	if (!ch->pcdata->in_progress)
	{
		d->connected = CON_PLAYING;
		bug ("nanny: In CON_NOTE_TO, but no note in progress",0);
		return;
	}

	strcpy (buf, argument);
	smash_tilde (buf); /* change ~ to - as we save this field as a string later */

	switch (ch->pcdata->board->force_type)
	{
		case DEF_NORMAL: /* default field */
			if (!buf[0]) /* empty string? */
			{
				ch->pcdata->in_progress->to_list = str_dup (ch->pcdata->board->names);
				sprintf (buf, "Assumed default recipient: " BOLD "%s" NO_COLOR "\n\r", ch->pcdata->board->names);
				write_to_buffer (d, buf, 0);
			}
			else
				ch->pcdata->in_progress->to_list = str_dup (buf);
				
			break;
		
		case DEF_INCLUDE: /* forced default */
			if (!is_full_name (ch->pcdata->board->names, buf))
			{
				strcat (buf, " ");
				strcat (buf, ch->pcdata->board->names);
				ch->pcdata->in_progress->to_list = str_dup(buf);

				sprintf (buf, "\n\rYou did not specify %s as recipient, so it was automatically added.\n\r"
				         BOLD C_B_YELLOW "New To" NO_COLOR " :  %s\n\r",
						 ch->pcdata->board->names, ch->pcdata->in_progress->to_list);
				write_to_buffer (d, buf, 0);
			}
			else
				ch->pcdata->in_progress->to_list = str_dup (buf);
			break;
		
		case DEF_EXCLUDE: /* forced exclude */
			if (!buf[0])
			{
				write_to_buffer (d, "You must specify a recipient.\n\r"
									BOLD C_B_YELLOW "To" NO_COLOR ":      ", 0);
				return;
			}
			
			if (is_full_name (ch->pcdata->board->names, buf))
			{
				sprintf (buf, "You are not allowed to send notes to %s on this board. Try again.\n\r"
				         BOLD C_B_YELLOW "To" NO_COLOR ":      ", ch->pcdata->board->names);
				write_to_buffer (d, buf, 0);
				return; /* return from nanny, not changing to the next state! */
			}
			else
				ch->pcdata->in_progress->to_list = str_dup (buf);
			break;
		
	}		

	write_to_buffer (d, BOLD C_B_YELLOW "\n\rSubject" NO_COLOR ": ", 0);
	d->connected = CON_NOTE_SUBJECT;
}

void handle_con_note_subject (DESCRIPTOR_DATA *d, char * argument)
{
	char buf [MAX_INPUT_LENGTH];
	CHAR_DATA *ch = d->character;

	if (!ch->pcdata->in_progress)
	{
		d->connected = CON_PLAYING;
		bug ("nanny: In CON_NOTE_SUBJECT, but no note in progress",0);
		return;
	}

	strcpy (buf, argument);
	smash_tilde (buf); /* change ~ to - as we save this field as a string later */
	
	/* Do not allow empty subjects */
	
	if (!buf[0])		
	{
		write_to_buffer (d, "Please find a meaningful subject!\n\r",0);
		write_to_buffer (d, BOLD C_B_YELLOW "Subject" NO_COLOR ": ", 0);
	}
	else  if (strlen(buf)>60)
	{
		write_to_buffer (d, "No, no. This is just the Subject. You're note writing the note yet. Twit.\n\r",0);
	}
	else
	/* advance to next stage */
	{
		ch->pcdata->in_progress->subject = str_dup(buf);
		if (IS_IMMORTAL(ch)) /* immortals get to choose number of expire days */
		{
			sprintf (buf,"\n\rHow many days do you want this note to expire in?\n\r"
			             "Press Enter for default value for this board, " BOLD "%d" NO_COLOR " days.\n\r"
           				 BOLD C_B_YELLOW "Expire" NO_COLOR ":  ",
		                 ch->pcdata->board->purge_days);
			write_to_buffer (d, buf, 0);				               
			d->connected = CON_NOTE_EXPIRE;
		}
		else
		{
			ch->pcdata->in_progress->expire = 
				current_time + ch->pcdata->board->purge_days * 24L * 3600L;				
			sprintf (buf, "This note will expire %s\r",ctime(&ch->pcdata->in_progress->expire));
			write_to_buffer (d,buf,0);
		ch->desc->connected = CON_PLAYING;		            
                string_append(ch,&ch->pcdata->in_progress->text); 
		}
	}
}

void handle_con_note_expire(DESCRIPTOR_DATA *d, char * argument)
{
	CHAR_DATA *ch = d->character;
	char buf[MAX_STRING_LENGTH];
	time_t expire;
	int days;

	if (!ch->pcdata->in_progress)
	{
		d->connected = CON_PLAYING;
		bug ("nanny: In CON_NOTE_EXPIRE, but no note in progress",0);
		return;
	}
	
	/* Numeric argument. no tilde smashing */
	strcpy (buf, argument);
	if (!buf[0]) /* assume default expire */
		days = 	ch->pcdata->board->purge_days;
	else /* use this expire */
		if (!is_number(buf))
		{
			write_to_buffer (d,"Write the number of days!\n\r",0);
			write_to_buffer (d,BOLD C_B_YELLOW "Expire" NO_COLOR ":  ",0);
			return;
		}
		else
		{
			days = atoi (buf);
			if (days <= 0)
			{
				write_to_buffer (d, "This is a positive MUD. Use positive numbers only! :)\n\r",0);
				write_to_buffer (d, BOLD C_B_YELLOW "Expire" NO_COLOR ":  ",0);
				return;
			}
		}
			
	expire = current_time + (days*24L*3600L); /* 24 hours, 3600 seconds */

	ch->pcdata->in_progress->expire = expire;
	
	/* note that ctime returns XXX\n so we only need to add an \r */

		ch->desc->connected = CON_PLAYING;		            
                string_append(ch,&ch->pcdata->in_progress->text); 
}



void handle_con_note_text (DESCRIPTOR_DATA *d, char * argument)
{
	CHAR_DATA *ch = d->character;
	char buf[MAX_STRING_LENGTH];
	char letter[4*MAX_STRING_LENGTH];
	
	if (!ch->pcdata->in_progress)
	{
		d->connected = CON_PLAYING;
		bug ("nanny: In CON_NOTE_TEXT, but no note in progress",0);
		return;
	}

	/* First, check for EndOfNote marker */

	strcpy (buf, argument);
	if ((!str_cmp(buf, "~")) || (!str_cmp(buf, "END")))
	{
		write_to_buffer (d, "\n\r\n\r",0);
		write_to_buffer (d, szFinishPrompt, 0);
		write_to_buffer (d, "\n\r", 0);
		d->connected = CON_NOTE_FINISH;
		return;
	}
	
	smash_tilde (buf); /* smash it now */

	/* Check for too long lines. Do not allow lines longer than 80 chars */
	
	if (strlen (buf) > MAX_LINE_LENGTH)
	{
		write_to_buffer (d, "Too long line rejected. Do NOT go over 80 characters!\n\r",0);
		return;
	}
	
	/* Not end of note. Copy current text into temp buffer, add new line, and copy back */

	/* How would the system react to strcpy( , NULL) ? */		
	if (ch->pcdata->in_progress->text)
	{
		strcpy (letter, ch->pcdata->in_progress->text);
		free_string (ch->pcdata->in_progress->text);
		ch->pcdata->in_progress->text = NULL; /* be sure we don't free it twice */
	}
	else
		strcpy (letter, "");
		
	/* Check for overflow */
	
	if ((strlen(letter) + strlen (buf)) > MAX_NOTE_TEXT)
	{ /* Note too long, take appropriate steps */
		write_to_buffer (d, "Note too long!\n\r", 0);
		free_note (ch->pcdata->in_progress);
		ch->pcdata->in_progress = NULL;			/* important */
		d->connected = CON_PLAYING;
		return;			
	}
	
	/* Add new line to the buffer */
	
	strcat (letter, buf);
	strcat (letter, "\r\n"); /* new line. \r first to make note files better readable */

	/* allocate dynamically */		
	ch->pcdata->in_progress->text = str_dup (letter);
}

void handle_con_note_finish (DESCRIPTOR_DATA *d, char * argument)
{

  CHAR_DATA *ch = d->character;
  
  if (!ch->pcdata->in_progress) {
    d->connected = CON_PLAYING;
    bug ("nanny: In CON_NOTE_FINISH, but no note in progress",0);
    return;
  }
		
  switch (tolower(argument[0])) {
  case 'c': /* keep writing */
    write_to_buffer (d,"Continuing note...\n\r",0);
    d->connected = CON_NOTE_TEXT;
    break;
    
  case 'v': /* view note so far */
    if (ch->pcdata->in_progress->text)
	 {
	   write_to_buffer (d,C_B_GREEN "Text of your note so far:" NO_COLOR "\n\r",0);
	   write_to_buffer (d, ch->pcdata->in_progress->text, 0);
	 }
    else
	 write_to_buffer (d,"You haven't written a thing!\n\r\n\r",0);
    write_to_buffer (d, szFinishPrompt, 0);
    write_to_buffer (d, "\n\r",0);
    break;
    
  case 'p': /* post note */
    finish_note (ch->pcdata->board, ch->pcdata->in_progress);
    write_to_buffer (d, "Note posted.\n\r",0);
    d->connected = CON_PLAYING;
    /* remove AFK status */
    ch->pcdata->in_progress = NULL;
    /* Don't want other to know we are doing note business eh ? */
    /*				act (BOLD C_B_GREEN "$n finishes $s note." NO_COLOR , ch, NULL, NULL, TO_ROOM);*/
    break;
    
  case 'f':
    write_to_buffer (d, "Note cancelled!\n\r",0);
    free_note (ch->pcdata->in_progress);
    ch->pcdata->in_progress = NULL;
    d->connected = CON_PLAYING;
    /* remove afk status */
    break;
    
  default: /* invalid response */
    write_to_buffer (d, "Huh? Valid answers are:\n\r\n\r",0);
    write_to_buffer (d, szFinishPrompt, 0);
    write_to_buffer (d, "\n\r",0);
    
  }
}

/* Send a request to a Guild with a note */
void do_request (CHAR_DATA *ch, char *argument)
{
  char note_buf1[MAX_STRING_LENGTH];
  char note_buf2[MAX_STRING_LENGTH];
  char buf[MSL];
  int guild=0;
  CHAR_DATA *guard=NULL;
  
  // onlu PCs
  if (IS_NPC(ch))
    return;
  
  if (IS_NULLSTR(argument)) {
    for (guard = ch->in_room->people; guard; guard = guard->next_in_room ) {
	 if (IS_NPC(guard) && guard->guild_guard &&
	 	(ch->clan == guard->guild_guard || !is_clan(ch))) {
	   REMOVE_BIT(guard->comm,COMM_NOCLAN);
	   REMOVE_BIT(guard->comm,COMM_NOCHANNELS);
	   guard->clan = guard->guild_guard;
	   free_string( guard->gtitle );
	   guard->gtitle = str_dup("Guild Guard");
	   if (ch->clan == guard->guild_guard)
	      sprintf(buf, "Guild member %s request assistance at %s.", ch->name, guard->in_room->name);
	   else
	   	  sprintf(buf, "%s request assistance at %s.", ch->pcdata->appearance, guard->in_room->name);
	   do_guildtalk(guard, buf);
	   
	   if (ch->clan == guard->guild_guard)
	      act("You request $N to ask your guild for assistance.", ch, NULL, guard, TO_CHAR);
	   else {	 		
	   	  act("You request assistance from $N's guild.", ch, NULL, guard, TO_CHAR);
	   	  sprintf(buf, "$N look at you for a moment then nod as $E say, '{7Please wait here and I will request some from the %s {7to come assist you.{x'", clan_table[guard->guild_guard].who_name);
	   	  act(buf, ch, NULL, guard, TO_CHAR);
	   }
	   
	   guard->clan = 0;
	   return;
	 }
    }

/*
    if (is_clan(ch)) {
	 send_to_char("You are already in a guild.\n\r", ch);
	 return;
    }    
*/
    
    send_to_char("Syntax: request          (in a room with a guild guard)\n\r", ch);
    send_to_char("Syntax: request <guild>  (leave a note to said guild)\n\r", ch);
    return;
  }

  if ((guild = clan_lookup(argument)) == 0) {
    send_to_char("No such guild exists.\n\r", ch);
    return;
  }

/*
  if (is_clan(ch)) {
    send_to_char("You are already in a guild.\n\r", ch);
    return;
  }
*/
  
  sprintf(note_buf1, 
		"Dear %s{x,\n\n\r"
		"I request to join the %s{x.\n\n\r"
		"Signed by the hand of %s{x,\n\r"
		"%s.\n\r",
		clan_table[guild].who_name,
		clan_table[guild].who_name,
		ch->name,
		ch->in_room->area->name);
  
  sprintf(note_buf2, "Request to join the %s.", clan_table[guild].name);
  
  make_note("Guild", ch->name, clan_table[guild].name, note_buf2, 56, note_buf1);
  
  sprintf(note_buf1, "You have left a note to %s about your request to join.\n\r", clan_table[guild].name);
  send_to_char(note_buf1, ch);

  return;
}
