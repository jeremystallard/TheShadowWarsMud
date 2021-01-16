/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  TSW Mud improvments copyright (C) 2000, 2001, 2002 by Swordfish and    *
 *  Zandor.                                                                *
 *                                                                         *
 *  This file includes the World Map code used for The Shadow Wars          *
 *                                                                         *
 ***************************************************************************/
#include <ctype.h>  /* for isalpha */
#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include <time.h> 
#include "merc.h"
#include "olc.h"

#define MAX_MAP 176
#define MAX_MAP_DIR 4

int map[MAX_MAP][MAX_MAP];
int offsets[4][2] ={ {-1, 0},{ 0, 1},{ 1, 0},{ 0,-1} };

void MapArea (ROOM_INDEX_DATA *room, CHAR_DATA *ch, int x, int y, int min, int max, bool seeall)
{
  ROOM_INDEX_DATA *prospect_room;
  EXIT_DATA *pexit;
  int door;
  
  /* marks the room as visited */
  map[x][y]=room->sector_type;
  
  /* cycles through for each exit */
  for ( door = 0; door < MAX_MAP_DIR; door++ )  { 
    if ((pexit = room->exit[door]) != NULL
	   &&   pexit->u1.to_room != NULL 
	   &&   can_see_room(ch,pexit->u1.to_room)  /* optional */
	   &&   !IS_SET(pexit->exit_info, EX_CLOSED)) { /* if exit there */
	 
	 if ((x<min)||(y<min)||(x>max)||(y>max)) return;
	 
	 prospect_room = pexit->u1.to_room;
	 
	 if ( prospect_room->exit[rev_dir[door]] &&
		 prospect_room->exit[rev_dir[door]]->u1.to_room!=room) { /* if not two way */
	   map[x][y]=SECT_MAX+1; /* one way into area OR maze */	
	   return;
	 } /* end two way */
	 
	 /*    if ( IS_NPC(ch)
		  ||(!IS_SET(ch->act,PLR_HOLYLIGHT))
		  ||(!IS_IMMORTAL(ch))
	) */

	 if ((prospect_room->sector_type==SECT_ROCK_MOUNTAIN) 
		||(prospect_room->sector_type==SECT_SNOW_MOUNTAIN) 
		||(prospect_room->sector_type==SECT_MOUNTAIN)
		||(prospect_room->sector_type==SECT_HILLS) 
		||(prospect_room->sector_type==SECT_CITY) 
		||(prospect_room->sector_type==SECT_INSIDE) 
		||(prospect_room->sector_type==SECT_ENTER)) { /* players cant see past these */
	   
	   if (seeall != TRUE)
		map[x+offsets[door][0]][y+offsets[door][1]]=prospect_room->sector_type;
	   /* ^--two way into area */		
	   /*if (door!=0) return;*/
	   /* ^--takes care of stopping when north is one of sectors you cant see past */
	 }
	 
	 if (map[x+offsets[door][0]][y+offsets[door][1]]==SECT_MAX) {
	   MapArea (pexit->u1.to_room,ch,x+offsets[door][0], y+offsets[door][1],min,max, seeall);
	 }
	 
    } /* end if exit there */
  }
  return;
}

/* Shows a map, specified by size */
void ShowMap( CHAR_DATA *ch, int min, int max)
{
  
  int x,y;
  
  for (x = min; x < max; ++x) { /* every row */
    send_to_char("        ", ch);
    for (y = min; y < max; ++y) { /* every column */
	 if ((y==min) || (map[x][y-1]!=map[x][y]))
	   switch(map[x][y]) {
            case SECT_MAX:		      send_to_char("  ",ch);		   break;
            case SECT_FOREST:	      send_to_char("{gf ",ch);		break;
            case SECT_FIELD:	      send_to_char("{Gg ",ch);	   break;
            case SECT_HILLS:	      send_to_char("{yh ",ch);		break;
            case SECT_ROAD:		   send_to_char("{r# ",ch);		break;
            case SECT_MOUNTAIN:	   send_to_char("{ym ",ch);		break;
            case SECT_WATER_SWIM:	send_to_char("{b. ",ch);		break;
            case SECT_WATER_NOSWIM:	send_to_char("{B. ",ch);		break;
            case SECT_UNUSED:	      send_to_char("{DX ",ch);		break;
            case SECT_AIR:		      send_to_char("{C% ",ch);		break;
            case SECT_DESERT:	      send_to_char("{Yw ",ch);		break;
            case SECT_ENTER:	      send_to_char("{WC ",ch);		break;
            case SECT_INSIDE:	      send_to_char("{W% ",ch);		break;
            case SECT_CITY:		   send_to_char("{W+ ",ch);		break;
            case SECT_ROCK_MOUNTAIN:send_to_char("{D^ ",ch);		break;
            case SECT_SNOW_MOUNTAIN:send_to_char("{W^ ",ch);		break;
            case SECT_SWAMP:	      send_to_char("{Ds ",ch);		break;
            case SECT_JUNGLE:	      send_to_char("{g& ",ch);		break;
            case SECT_RUINS:	      send_to_char("{w# ",ch);		break;
            case SECT_OCEAN:	      send_to_char("{Co ",ch);		break;
            case SECT_RIVER:	      send_to_char("{Br ",ch);		break;
            case SECT_SAND:	      send_to_char("{y* ",ch);		break;
            case SECT_BLIGHT:	      send_to_char("{Db ",ch);		break;
            case SECT_ISLAND:	      send_to_char("{Yi ",ch);		break;
            case SECT_LAKE:	      send_to_char("{cl ",ch);		break;
            case (SECT_MAX+1):	   send_to_char("{D? ",ch);		break;
            default: 		         send_to_char("{W* ",ch);
      } /* end switch1 */
      else 
      switch(map[x][y]) {
            case SECT_MAX:		      send_to_char("  ",ch);		break;
            case SECT_FOREST:	      send_to_char("f ",ch);		break;
            case SECT_FIELD:	      send_to_char("g ",ch);		break;
            case SECT_HILLS:	      send_to_char("h ",ch);		break;
            case SECT_ROAD:		   send_to_char("# ",ch);		break;
            case SECT_MOUNTAIN:	   send_to_char("m ",ch);		break;
            case SECT_WATER_SWIM:	send_to_char(". ",ch);		break;
            case SECT_WATER_NOSWIM:	send_to_char(". ",ch);		break;
            case SECT_UNUSED:	      send_to_char("X ",ch);		break;
            case SECT_AIR:		      send_to_char("% ",ch);		break;
            case SECT_DESERT:	      send_to_char("w ",ch);		break;
            case SECT_ENTER:	      send_to_char("C ",ch);		break;
            case SECT_INSIDE:	      send_to_char("% ",ch);		break;
            case SECT_CITY:		   send_to_char("+ ",ch);		break;
            case SECT_ROCK_MOUNTAIN:send_to_char("^ ",ch);		break;
            case SECT_SNOW_MOUNTAIN:send_to_char("^ ",ch);		break;
            case SECT_SWAMP:	      send_to_char("s ",ch);		break;
            case SECT_JUNGLE:	      send_to_char("& ",ch);		break;
            case SECT_RUINS:	      send_to_char("# ",ch);		break;
            case SECT_OCEAN:	      send_to_char("o ",ch);		break;
            case SECT_RIVER:	      send_to_char("r ",ch);		break;
            case SECT_SAND:	      send_to_char("* ",ch);		break;
            case SECT_BLIGHT:	      send_to_char("b ",ch);		break;
            case SECT_ISLAND:	      send_to_char("i ",ch);		break;
            case SECT_LAKE:	      send_to_char("l ",ch);		break;
            case (SECT_MAX+1):	   send_to_char("? ",ch);		break;
            default: 		         send_to_char("* ",ch);
      } /* end switch2 */
    } /* every column */
    send_to_char("\n\r",ch); 
   } /*every row*/   
   return;
}

/* Shows map compacted, specified by size */
void ShowHalfMap( CHAR_DATA *ch, int min, int max)
{
  int x,y;
  
  for (x = min; x < max; x+=2) { /* every row */
    for (y = min; y < max; y+=2) { /* every column */
	 
      /* mlk prioritizes*/
      if (map[x][y-1]==SECT_ROAD) map[x][y]=SECT_ROAD;
      if (map[x][y-1]==SECT_ENTER) map[x][y]=SECT_ENTER;
	 
      if ( (y==min) || (map[x][y-2]!=map[x][y]) )
         switch(map[x][y]) {
            case SECT_MAX:		   send_to_char(" ",ch);		break;
            case SECT_FOREST:	   send_to_char("{g@",ch);		break;
            case SECT_FIELD:	   send_to_char("{Gg",ch);	   break;
            case SECT_HILLS:	   send_to_char("{G^",ch);		break;
            case SECT_ROAD:		send_to_char("{r#",ch);		break;
            case SECT_MOUNTAIN:	send_to_char("{y^",ch);		break;
            case SECT_WATER_SWIM:	send_to_char("{B:",ch);		break;
            case SECT_WATER_NOSWIM:	send_to_char("{b:",ch);		break;
            case SECT_UNUSED:	send_to_char("{DX",ch);		break;
            case SECT_AIR:		send_to_char("{C%",ch);		break;
            case SECT_DESERT:	send_to_char("{Y=",ch);		break;
            case SECT_ENTER:	send_to_char("{WC",ch);		break;
            case SECT_INSIDE:	send_to_char("{W%",ch);		break;
            case SECT_CITY:		send_to_char("{W#",ch);		break;
            case SECT_ROCK_MOUNTAIN:send_to_char("{D^",ch);		break;
            case SECT_SNOW_MOUNTAIN:send_to_char("{W^",ch);		break;
            case SECT_SWAMP:	send_to_char("{D&",ch);		break;
            case SECT_JUNGLE:	send_to_char("{g&",ch);		break;
            case SECT_RUINS:	send_to_char("{D#",ch);		break;
            case (SECT_MAX+1):	send_to_char("{D?",ch);		break;
            default: 		send_to_char("{W*",ch);
         } /* end switch1 */
  else 
      switch(map[x][y]) {
            case SECT_MAX:		send_to_char(" ",ch);		break;
            case SECT_FOREST:	send_to_char("@",ch);		break;
            case SECT_FIELD:	send_to_char("\"",ch);		break;
            case SECT_HILLS:	send_to_char("^",ch);		break;
            case SECT_ROAD:		send_to_char("#",ch);		break;
            case SECT_MOUNTAIN:	send_to_char("^",ch);		break;
            case SECT_WATER_SWIM:	send_to_char(":",ch);		break;
            case SECT_WATER_NOSWIM:	send_to_char(":",ch);		break;
            case SECT_UNUSED:	send_to_char("X",ch);		break;
            case SECT_AIR:		send_to_char("%",ch);		break;
            case SECT_DESERT:	send_to_char("=",ch);		break;
            case SECT_ENTER:	send_to_char("C",ch);		break;
            case SECT_INSIDE:	send_to_char("%",ch);		break;
            case SECT_CITY:		send_to_char("#",ch);		break;
            case SECT_ROCK_MOUNTAIN:send_to_char("^",ch);		break;
            case SECT_SNOW_MOUNTAIN:send_to_char("^",ch);		break;
            case SECT_SWAMP:	send_to_char("&",ch);		break;
            case SECT_JUNGLE:	send_to_char("&",ch);		break;
            case SECT_RUINS:	send_to_char("#",ch);		break;
            case (SECT_MAX+1):	send_to_char("?",ch);		break;
            default: 		send_to_char("*",ch);
      } /* end switch2 */
    } /* every column */
    send_to_char("\n\r",ch); 
   } /*every row*/   
   return;
}

unsigned int get_mapvnum(int x, int y)
{
   unsigned int start=40000;	
   unsigned int vmapvnum=0;
     
   if (x <= 0)
     vmapvnum = start+(y-1);
   else
     vmapvnum = start+(y-1)+(175*(x-1));
     
   return vmapvnum;
}

/* printing function */
void do_printmap(CHAR_DATA *ch, char *argument)
{
  ROOM_INDEX_DATA *location;
  char buf[MSL];
  int x,y;
  int min=1;
  int max=MAX_MAP;
  FILE *fp;
  char mapfilename[512];
  bool use_html=FALSE;
  bool use_colortext=FALSE;

  if (!str_cmp( argument, "html" )) {
    use_html=TRUE;
  }
  else if (!str_cmp( argument, "text" )) {
    use_html=FALSE;
  }
  else if (!str_cmp( argument, "colortext" )) {
    use_colortext=TRUE;
  }
  else {
    send_to_char("Syntax: printmap text\n\r", ch);
    send_to_char("Syntax: printmap colortext\n\r", ch);
    send_to_char("Syntax: printmap html\n\r", ch);
    return;
  }    
  
  for (x = 0; x < MAX_MAP; ++x)
    for (y = 0; y < MAX_MAP; ++y)
	 map[x][y]=SECT_MAX;

  // Upper left corner of the world
  location = get_room_index(40000);

  char_from_room( ch );
  char_to_room( ch, location );

  MapArea(ch->in_room, ch, 1, 1 , min-1, max-1, TRUE); 
  
  fclose (fpReserve);                        
  
  sprintf(mapfilename, "worldmap.%s", use_html ? "html" : use_colortext ? "asc" : "txt");
  fp = fopen(mapfilename,"w");
  
  if (use_html) {    
    fprintf(fp, "<BODY TEXT=\"darkgray\" BGCOLOR=\"black\">");
    fprintf(fp, "<FONT FACE=\"Courier\"><B>\n");
    fprintf(fp, "<PRE><H1>The Shadow <FONT COLOR=\"darkred\">Wars</FONT> World Map</H1>");
    fprintf(fp, "The world map is Copyright &copy; 1995-2007 by The Shadow Wars\n");
    fprintf(fp, "Last updated at <FONT COLOR=\"goldenrod\">%s</FONT></PRE>\n", (char *) ctime(&current_time));
    fprintf(fp, "<FONT SIZE=\"1\">\n");
  }
  
  for (x = min; x < max; ++x) { /* every row */
    for (y = min; y < max; ++y) { /* every column */
	 if (use_html) {
	   location = get_room_index(get_mapvnum(x,y));
	   if (location) {
	    	char_from_room( ch );
            	char_to_room( ch, location );
           }
	   switch(map[x][y]) {
	   case SECT_MAX:          /*fprintf(fp, "");*/                                 break;
	   case SECT_FOREST:       fprintf(fp, "<FONT COLOR=\"darkgreen\" >f</FONT>");	break;
	   case SECT_FIELD:        fprintf(fp, "<FONT COLOR=\"green\"     >g</FONT>");  break;
	   case SECT_HILLS:        fprintf(fp, "<FONT COLOR=\"goldenrod\" >h</FONT>");	break;
	   case SECT_ROAD:         fprintf(fp, "<FONT COLOR=\"red\"       >#</FONT>");	break;
	   case SECT_MOUNTAIN:     fprintf(fp, "<FONT COLOR=\"goldenrod\" >m</FONT>");	break;
	   case SECT_WATER_SWIM:   fprintf(fp, "<FONT COLOR=\"lightblue\" >.</FONT>");	break;
	   case SECT_WATER_NOSWIM: fprintf(fp, "<FONT COLOR=\"lightblue\" >.</FONT>");	break;
	   case SECT_UNUSED:       fprintf(fp, "<FONT COLOR=\"darkgray\"  >X</FONT>");	break;
	   case SECT_AIR:          fprintf(fp, "<FONT COLOR=\"skyblue\"   >%%</FONT>"); break;
	   case SECT_DESERT:       fprintf(fp, "<FONT COLOR=\"yellow\"    >w</FONT>");	break;
	   case SECT_ENTER:        fprintf(fp, "<FONT COLOR=\"white\"     title=\"%s\">C</FONT>",ch->in_room->name);	break;
	   case SECT_INSIDE:       fprintf(fp, "<FONT COLOR=\"white\"     >%%</FONT>"); break;
	   case SECT_CITY:         fprintf(fp, "<FONT COLOR=\"white\"     title=\"%s\">+</FONT>", ch->in_room->name);	break;
	   case SECT_ROCK_MOUNTAIN:fprintf(fp, "<FONT COLOR=\"darkgray\"  >^</FONT>");	break;
	   case SECT_SNOW_MOUNTAIN:fprintf(fp, "<FONT COLOR=\"white\"     >^</FONT>");	break;
	   case SECT_SWAMP:        fprintf(fp, "<FONT COLOR=\"darkgray\"  >s</FONT>");	break;
	   case SECT_JUNGLE:       fprintf(fp, "<FONT COLOR=\"darkgreen\" >&</FONT>");	break;
	   case SECT_RUINS:        fprintf(fp, "<FONT COLOR=\"darkgray\"  >#</FONT>");	break;
	   case SECT_OCEAN:        fprintf(fp, "<FONT COLOR=\"skyblue\"   >o</FONT>");	break;
	   case SECT_RIVER:        fprintf(fp, "<FONT COLOR=\"darkblue\"  >r</FONT>");	break;
	   case SECT_SAND:         fprintf(fp, "<FONT COLOR=\"goldenrod\" >*</FONT>");	break;
	   case SECT_BLIGHT:       fprintf(fp, "<FONT COLOR=\"darkgray\"  >b</FONT>");	break;
	   case SECT_ISLAND:       fprintf(fp, "<FONT COLOR=\"yellow\"    >i</FONT>");	break;
	   case SECT_LAKE:         fprintf(fp, "<FONT COLOR=\"skyblue\"   >l</FONT>");	break;
	   case (SECT_MAX+1):      fprintf(fp, "<FONT COLOR=\"darkgray\"  >?</FONT>");	break;
	   default:                fprintf(fp, "<FONT COLOR=\"white\"     >*</FONT>");  break;   
	   }
	 }
	 else if (use_colortext) {
	   switch(map[x][y]) {
	   case SECT_MAX:          fprintf(fp, "  ");       break;
	   case SECT_FOREST:       fprintf(fp, "{gf ");	break;
	   case SECT_FIELD:        fprintf(fp, "{Gg ");	break;
	   case SECT_HILLS:        fprintf(fp, "{yh ");	break;
	   case SECT_ROAD:         fprintf(fp, "{r# ");	break;
	   case SECT_MOUNTAIN:     fprintf(fp, "{ym ");	break;
	   case SECT_WATER_SWIM:   fprintf(fp, "{b. ");	break;
	   case SECT_WATER_NOSWIM: fprintf(fp, "{B. ");	break;
	   case SECT_UNUSED:       fprintf(fp, "{DX ");	break;
	   case SECT_AIR:          fprintf(fp, "{C%% ");    break;
	   case SECT_DESERT:       fprintf(fp, "{Yw ");	break;
	   case SECT_ENTER:        fprintf(fp, "{WC ");	break;
	   case SECT_INSIDE:       fprintf(fp, "{W%% ");    break;
	   case SECT_CITY:         fprintf(fp, "{W+ ");	break;
	   case SECT_ROCK_MOUNTAIN:fprintf(fp, "{D^ ");	break;
	   case SECT_SNOW_MOUNTAIN:fprintf(fp, "{W^ ");	break;
	   case SECT_SWAMP:        fprintf(fp, "{Ds ");	break;
	   case SECT_JUNGLE:       fprintf(fp, "{g& ");	break;
	   case SECT_RUINS:        fprintf(fp, "{w# ");	break;
	   case SECT_OCEAN:        fprintf(fp, "{Co ");	break;
	   case SECT_RIVER:        fprintf(fp, "{Br ");	break;
	   case SECT_SAND:         fprintf(fp, "{y* ");	break;
	   case SECT_BLIGHT:       fprintf(fp, "{Db ");	break;
	   case SECT_ISLAND:       fprintf(fp, "{Yi ");	break;
	   case SECT_LAKE:         fprintf(fp, "{cl ");	break;
	   case (SECT_MAX+1):      fprintf(fp, "{D? ");	break;
	   default:                fprintf(fp, "{W* ");     break;		
	   }
	 }
	 else {
	   switch(map[x][y]) {
	   case SECT_MAX:          fprintf(fp, "  ");       break;
	   case SECT_FOREST:       fprintf(fp, "f ");	break;
	   case SECT_FIELD:        fprintf(fp, "g ");	break;
	   case SECT_HILLS:        fprintf(fp, "h ");	break;
	   case SECT_ROAD:         fprintf(fp, "# ");	break;
	   case SECT_MOUNTAIN:     fprintf(fp, "m ");	break;
	   case SECT_WATER_SWIM:   fprintf(fp, ". ");	break;
	   case SECT_WATER_NOSWIM: fprintf(fp, ". ");	break;
	   case SECT_UNUSED:       fprintf(fp, "X ");	break;
	   case SECT_AIR:          fprintf(fp, "%% ");    break;
	   case SECT_DESERT:       fprintf(fp, "w ");	break;
	   case SECT_ENTER:        fprintf(fp, "C ");	break;
	   case SECT_INSIDE:       fprintf(fp, "%% ");    break;
	   case SECT_CITY:         fprintf(fp, "+ ");	break;
	   case SECT_ROCK_MOUNTAIN:fprintf(fp, "^ ");	break;
	   case SECT_SNOW_MOUNTAIN:fprintf(fp, "^ ");	break;
	   case SECT_SWAMP:        fprintf(fp, "s ");	break;
	   case SECT_JUNGLE:       fprintf(fp, "& ");	break;
	   case SECT_RUINS:        fprintf(fp, "# ");	break;
	   case SECT_OCEAN:        fprintf(fp, "o ");	break;
	   case SECT_RIVER:        fprintf(fp, "r ");	break;
	   case SECT_SAND:         fprintf(fp, "* ");	break;
	   case SECT_BLIGHT:       fprintf(fp, "b ");	break;
	   case SECT_ISLAND:       fprintf(fp, "i ");	break;
	   case SECT_LAKE:         fprintf(fp, "l ");	break;
	   case (SECT_MAX+1):      fprintf(fp, "? ");	break;
	   default:                fprintf(fp, "* ");     break;		
	   }
	 } /* end switch2 */
    } /* every column */
    fprintf(fp,"\n"); 
  } /*every row*/   
  
  if (use_html) {
    fprintf(fp, "</FONT></B>\n");
    fprintf(fp, "</BODY>\n");
  }

  fclose(fp);
  fpReserve = fopen( NULL_FILE, "r" );

  sprintf(buf, "World map printed to file <%s>\n\r", mapfilename);
  send_to_char(buf, ch);

  return;
}

/* will put a small map with current room desc and title */
void ShowRoom( CHAR_DATA *ch, int min, int max)
{
   int x,y,str_pos=0,desc_pos=0,start;
   char buf[500];
   char desc[500];
   char line[100];

   strcpy(desc,ch->in_room->description);

   map[min][min]=SECT_MAX;map[max-1][max-1]=SECT_MAX; /* mlk :: rounds edges */
   map[min][max-1]=SECT_MAX;map[max-1][min]=SECT_MAX;

   for (x = min; x < max; ++x) { /* every row */
      for (y = min; y < max; ++y) { /* every column */
         if ( (y==min) || (map[x][y-1]!=map[x][y]) )
            switch(map[x][y]) {
               case SECT_MAX:		send_to_char(" ",ch);		break;
               case SECT_FOREST:	send_to_char("{g@",ch);		break;
               case SECT_FIELD:	send_to_char("{Gg",ch);	break;
               case SECT_HILLS:	send_to_char("{G^",ch);		break;
               case SECT_ROAD:		send_to_char("{r#",ch);		break;
               case SECT_MOUNTAIN:	send_to_char("{y^",ch);		break;
               case SECT_WATER_SWIM:	send_to_char("{B:",ch);		break;
               case SECT_WATER_NOSWIM:	send_to_char("{b:",ch);		break;
               case SECT_UNUSED:	send_to_char("{DX",ch);		break;
               case SECT_AIR:		send_to_char("{C%",ch);		break;
               case SECT_DESERT:	send_to_char("{Y=",ch);		break;
               case SECT_ENTER:	send_to_char("{WC",ch);		break;
               case SECT_INSIDE:	send_to_char("{W%",ch);		break;
               case SECT_CITY:		send_to_char("{W#",ch);		break;
               case SECT_ROCK_MOUNTAIN:send_to_char("{D^",ch);		break;
               case SECT_SNOW_MOUNTAIN:send_to_char("{W^",ch);		break;
               case SECT_SWAMP:	send_to_char("{D&",ch);		break;
               case SECT_JUNGLE:	send_to_char("{g&",ch);		break;
               case SECT_RUINS:	send_to_char("{D#",ch);		break;
               case (SECT_MAX+1):	send_to_char("{D?",ch);		break;
               default: 		send_to_char("{W*",ch);
            } /* end switch1 */
         else 
            switch(map[x][y]) {
               case SECT_MAX:		send_to_char(" ",ch);		break;
               case SECT_FOREST:	send_to_char("@",ch);		break;
               case SECT_FIELD:	send_to_char("\"",ch);		break;
               case SECT_HILLS:	send_to_char("^",ch);		break;
               case SECT_ROAD:		send_to_char("#",ch);		break;
               case SECT_MOUNTAIN:	send_to_char("^",ch);		break;
               case SECT_WATER_SWIM:	send_to_char(":",ch);		break;
               case SECT_WATER_NOSWIM:	send_to_char(":",ch);		break;
               case SECT_UNUSED:	send_to_char("X",ch);		break;
               case SECT_AIR:		send_to_char("%",ch);		break;
               case SECT_DESERT:	send_to_char("=",ch);		break;
               case SECT_ENTER:	send_to_char("C",ch);		break;
               case SECT_INSIDE:	send_to_char("%",ch);		break;
               case SECT_CITY:		send_to_char("#",ch);		break;
               case SECT_ROCK_MOUNTAIN:send_to_char("^",ch);		break;
               case SECT_SNOW_MOUNTAIN:send_to_char("^",ch);		break;
               case SECT_SWAMP:	send_to_char("&",ch);		break;
               case SECT_JUNGLE:	send_to_char("&",ch);		break;
               case SECT_RUINS:	send_to_char("#",ch);		break;
               case (SECT_MAX+1):	send_to_char("?",ch);		break;
               default: 		send_to_char("*",ch);
            } /* end switch2 */
         } /* every column */
         if (x==min) {
         	sprintf(buf,"{x   {C%s{x",ch->in_room->name);
         	send_to_char(buf,ch);

/* No brief in wilderness, ascii map is automatic, autoexits off too
        if (  !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT)  ) { 
        send_to_char( " {b",ch);do_exits( ch, "auto" );send_to_char( "{x",ch);
        } */

            if (IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act,PLR_HOLYLIGHT))) { /* for showing certain people room vnum */
           	   sprintf(buf," {c[Room %d]{x",ch->in_room->vnum);
           	   send_to_char(buf,ch);
            }
         }
         else {
            start=str_pos;
            for (desc_pos  ; desc[desc_pos]!='\0' ; desc_pos++) {
               if (desc[desc_pos]=='\n') {
               	line[str_pos-start]='\0';str_pos+=3;
               	desc_pos += 2;
               	break;
            	}
               else if (desc[desc_pos]=='\r') {
               	line[str_pos-start]='\0';str_pos+=2;
               	break;
            	}
               else {
               	line[str_pos-start]=desc[desc_pos];
               	str_pos+=1;
            	}
         }
/* set the final character to \0 for descs that are longer than 5 lines? */
line[str_pos-start]='\0'; /* best way to clear string? */
/* maybe do strcpy(line,"  ");  instead? */
/* not needed here because the for loops stop at \0 ?? */
if (x==min+1) send_to_char("  ",ch);
send_to_char("   {c",ch);
send_to_char(line,ch);
send_to_char("{x",ch);

} /*else*/

send_to_char("\n\r",ch); 
} /*every row*/   

send_to_char("\n\r",ch);  /* puts a line between contents/people */
return;
}

void do_map( CHAR_DATA *ch, char *argument )
{
   int size,center,x,y,min,max;
   char arg1[10];

   one_argument( argument, arg1 );
   size = atoi (arg1);

   size=URANGE(6,size,MAX_MAP);
   center=MAX_MAP/2;

   min = MAX_MAP/2-size/2;
   max = MAX_MAP/2+size/2;

   for (x = 0; x < MAX_MAP; ++x)
      for (y = 0; y < MAX_MAP; ++y)
         map[x][y]=SECT_MAX;

   ROOM_INDEX_DATA * centerRoom;
   if (ch->in_room->inside_of)
   {
	centerRoom = ch->in_room->inside_of->in_room;
   }
   else
   {
	centerRoom = ch->in_room;
   }
   /* starts the mapping with the center room */
   MapArea(centerRoom, ch, center, center, min-1, max-1, FALSE); 

   /* marks the center, where ch is */
   map[center][center]=SECT_MAX+2; /* can be any number above SECT_MAX+1 	*/
   /* switch default will print out the *	*/

   /*
   if (   (!IS_IMMORTAL(ch))||(IS_NPC(ch))) {
      if (  !IS_SET(ch->in_room->room_flags, ROOM_VMAP)) {
         send_to_char("{CYou can not do that here{x.\n\r",ch);return;}
      if ( room_is_dark(ch->in_room)) {
         send_to_char( "{bThe wilderness is pitch black at night... {x\n\r", ch );return;}
      else  {
         ShowRoom(ch,MAX_MAP/2-3,MAX_MAP/2+3);
         return;
      }
   }
   */

   /* mortals not in city, enter or inside will always get a ShowRoom */
   /*if (IS_IMMORTAL(ch)) {
      if (arg1[0]=='\0') ShowRoom (ch, min, max); 
      else ShowMap (ch, min, max); 
      return;
     }
   */

   if (arg1[0]=='\0')
      ShowRoom (ch, min, max);
   else {
     ShowMap(ch, min, max);
     return;
   }

   send_to_char("{RInternal error <4523234>, please note this to the Admin.{x\n\r",ch);
   return;
}

void do_smallmap( CHAR_DATA *ch, char *argument )
{
int size,center,x,y,min,max;
char arg1[10];

   one_argument( argument, arg1 );
   size = atoi (arg1);

size=URANGE(6,size,MAX_MAP);
center=MAX_MAP/2;

min = MAX_MAP/2-size/2;
max = MAX_MAP/2+size/2;

for (x = 0; x < MAX_MAP; ++x)
        for (y = 0; y < MAX_MAP; ++y)
                  map[x][y]=SECT_MAX;

/* starts the mapping with the center room */
   ROOM_INDEX_DATA * centerRoom;
   if (ch->in_room->inside_of)
   {
	centerRoom = ch->in_room->inside_of->in_room;
   }
   else
   {
	centerRoom = ch->in_room;
   }
MapArea(centerRoom, ch, center, center, min-1, max-1, FALSE); 

/* marks the center, where ch is */
map[center][center]=SECT_MAX+2; /* can be any number above SECT_MAX+1 	*/
				/* switch default will print out the *	*/

if (IS_IMMORTAL(ch)) {
   ShowHalfMap (ch, min, max); 
   return;
   }

send_to_char("{CHuh?{x\n\r",ch);
return;
}

/* pass it (SECTOR_XXX,"") and get back the sector ascii 
   in a roleplaying format of course, not mountain_snow etc */
char *get_sector_name(int sector)
{
  char *sector_name;
  
  sector_name="movement";
  
  switch (sector) {
  case SECT_FOREST:	       sector_name="some trees";break; 
  case SECT_FIELD:	       sector_name="a field";break;
  case SECT_HILLS:	       sector_name="some rolling hills";break;
  case SECT_ROAD:		  sector_name="a road";break;
  case SECT_WATER_SWIM:	  sector_name="shallow water";break;
  case SECT_WATER_NOSWIM:  sector_name="deep water";break;
  case SECT_AIR:		  sector_name="the sky";break;
  case SECT_DESERT:	       sector_name="a lot of sand";break;
  case SECT_MOUNTAIN:	  sector_name="some mountainous terrain";break;
  case SECT_ROCK_MOUNTAIN: sector_name="a rocky mountain";break;
  case SECT_SNOW_MOUNTAIN: sector_name="snow covered mountains";break;
  case SECT_SWAMP:	       sector_name="bleak swampland";break;
  case SECT_JUNGLE:	       sector_name="thick jungle";break;
  case SECT_RUINS:	       sector_name="ruins of some sort";break;
  case SECT_INSIDE:	       sector_name="movement";break; 
  case SECT_CITY:		  sector_name="movement";break; 
  case SECT_ENTER:   	  sector_name="movement";break;
  case SECT_OCEAN:         sector_name="an ocean";break;
  case SECT_RIVER:         sector_name="a river";break;
  case SECT_SAND:	       sector_name="sand";break;
  case SECT_BLIGHT:	       sector_name="the blight";break;
  case SECT_ISLAND:	       sector_name="a island";break;
  case SECT_LAKE:	       sector_name="a lake";break;
  } /*switch1*/
  
  return(strdup(sector_name));
}

/* 
   when given (int array[5]) with vnum in [0], it will return
   1	north sector_type, 
   2	east sector_type, 
   3	south sector_type,
   4	east_sector_type &
   5	number of exits leading to rooms of the same sector
*/
int *get_exit_sectors(int *exit_sectors)
{
  ROOM_INDEX_DATA *room;
  EXIT_DATA *pexit;
  int door;
  
  room=get_room_index(exit_sectors[0]);
  exit_sectors[4]=0;
  
  for ( door = 0; door < MAX_MAP_DIR; door++ )  { /* cycles through for each exit */
    if ((pexit = room->exit[door]) != NULL
	   &&   pexit->u1.to_room != NULL) {   
	 exit_sectors[door]=pexit->u1.to_room->sector_type;	
	 if ((pexit->u1.to_room->sector_type)==(room->sector_type))
	   exit_sectors[4]+=1;
    } 
    else
	 exit_sectors[door]=-1;	
  }	   
  
  return(exit_sectors);
}

/* will assign default values for NAME and DESC of vmap */
void do_vmapdesc(CHAR_DATA *ch, char *argument)
{
  char arg1[10],name[50],desc[300],buf[MAX_STRING_LENGTH];
  char ebuf[MAX_STRING_LENGTH];
  ROOM_INDEX_DATA *room;
  unsigned int vnum,exit[5],exitsum;
  int CHANCE=10;
  
  one_argument( argument, arg1 );
  vnum = atoi (arg1);
  
  if ( argument[0] == '\0' ) { /* for immortal command */
    vnum=(ch->in_room->vnum);
  }
  
  room=get_room_index(vnum);
  
  if (!IS_BUILDER(ch,room->area)) {
    send_to_char("Wset: Insufficient Security",ch);
    return;
  }
  
  if ( !IS_SET(room->room_flags, ROOM_VMAP))
    return; /* for NON wilderness */
  
  exit[0]=vnum;
  get_exit_sectors(exit);
  exitsum=exit[4];
  
  switch (room->sector_type) {

  /* We don't want to edit SECT_ENTER SECTIONS */
  case SECT_ENTER:
    sprintf(ebuf, "Vnum <%d> has ENTER sector type. No name or description assigned.\n\r", vnum);
    send_to_char(ebuf, ch);
    return;

  case SECT_FOREST:
    strcpy(name,"In a Forest");
    strcpy(desc,"You are in a forest overflowing with greenery. All around you "
		 "can hear the sounds of animals rustling leaves in the under brush. "
		 "At times, low, guttural growls can also be heard.");
    
    if (exitsum==4) {
	 strcpy(name,"Within a Forest");
	 strcpy(desc,"You are deep within in a forest, surrounded on all sides by "
		   "trees and vines. Dead leaves blanket the ground and the lush canopy "
		   "of tree branches prevents most of the sunlight from getting though.");
    }
    if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	 sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	 sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	 sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	 sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    break;
    
  case SECT_FIELD:
    strcpy(name,"In a Field");
    strcpy(desc,"You are in a field of tall grass. Gentle, warm gusts of wind "
		 "blow across the surface of the field, making the grass sway like waves "
		 "on the ocean. The air is fresh and clean, glazed with the faint aroma of "
		 "pollen and wild flowers.");
    
    if (exitsum==4) {
	 strcpy(name,"On the Plains");
	 strcpy(desc,"You are on the plains surrounded by tall grass of many shapes, "
		   "sizes, and even colors. From time to time, insects fly through the "
		   "air in swarms or flocks of birds take to the air when you get to close "
		   "to their gathering.");
    }
    if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	 sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	 sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	 sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	 sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    break;
    
  case SECT_HILLS:
    strcpy(name,"On a Hill");
    strcpy(desc,"You are on a large and rather tall hill. Small pebbles crunch "
		 "beneath your feet as you look out at the world around you. From this "
		 "height, you can see much of the surrounding area, including the roads "
		 "that lead to your destination.");
    
    if (exitsum==4) {
	 strcpy(name,"In the Foothills");
	 strcpy(desc,"You are within the highlands.  The same thing surrounds you "
		   "on all sides - hills and more hills, rolling across the land. This "
		   "was probably not the best route of travel.");
    }
    if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE+20))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	 sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    if(((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE+20))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	 sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    if(((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE+20))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	 sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	 strcat(desc,buf);strcpy(buf,"  ");
      }
    if(((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE+20))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	 sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	 strcat(desc,buf);strcpy(buf,"  ");
    }
    break;
    
  case SECT_ROAD:
    strcpy(name,"A Road");
    strcpy(desc,"You are on a dirt road. Being an easier and much safer way to "
		 "travel, many would not think of leaving the safety of the road.  It "
		 "is marred with deep groves from wagon wheels and the imprints of "
		 "hundreds of hoof prints.");
    
    if (exitsum==4) { 
	 strcpy(name,"At a Four Way"); 
	 strcpy(desc,"You are at a four way intersection.  The dirt road is trod "
		   "with boot prints in each  direction, making it difficult to choose "
		   "which path is most frequently traveled.");
    } 
    else if (exitsum==3) { 
	 strcpy(name,"At a Fork in the Road");
	 strcpy(desc,"You are at a fork in the road.  There are two paths to choose "
		   "from and no signs or indicators to help you choose which path would "
		   "be best.  You will have to rely on your instincts.");
    } 
    else if (exitsum==2) { 
	 strcpy(name,"A Road"); 
	 strcpy(desc,"You are on a dirt road. Being an easier and much safer way to "
		   "travel, many would not think of leaving the safety of the road.  "
		   "It is marred with deep groves from wagon wheels and the imprints "
		   "of hundreds of hoof prints.");
	 if (exit[0]==SECT_ROAD) { 
	   sprintf(buf,"The road continues north and ");
	   strcat(desc,buf);
	   strcpy(buf,"  "); 
	   if (exit[1]==SECT_ROAD) {
		sprintf(buf,"east."); 
		strcat(desc,buf);
		strcpy(buf,"  ");
		break;    
	   }
	   if (exit[2]==SECT_ROAD) { 
		sprintf(buf,"south.");
		strcat(desc,buf);
		strcpy(buf,"  ");
		break; 
	   } 
	   if (exit[3]==SECT_ROAD) { 
		sprintf(buf,"west.");
		strcat(desc,buf);
		strcpy(buf,"  ");
		break; 
	   } 
	 } 
	 if (exit[1]==SECT_ROAD) {
	   sprintf(buf,"The road continues east and "); 
	   strcat(desc,buf);
	   strcpy(buf,"  ");
	   if (exit[0]==SECT_ROAD) { 
		sprintf(buf,"north."); 
		strcat(desc,buf);
		strcpy(buf,"  ");
		break;   
	   } 
	   if (exit[2]==SECT_ROAD) { 
		sprintf(buf,"south.");
		strcat(desc,buf);
		strcpy(buf,"  ");
		break; 
	   } 
	   if (exit[3]==SECT_ROAD) {
		sprintf(buf,"west.");
		strcat(desc,buf);
		strcpy(buf,"  ");
		break; 
	   } 
	 } 
	 if (exit[2]==SECT_ROAD) { 
	   sprintf(buf,"The road continues south and ");
	   strcat(desc,buf);
	   strcpy(buf,"  "); 
	   if (exit[0]==SECT_ROAD) {
		sprintf(buf,"north."); 
		strcat(desc,buf);
		strcpy(buf,"  ");
		break;   
	   } 
	   if (exit[1]==SECT_ROAD) { 
		sprintf(buf,"east."); 
		strcat(desc,buf);
		strcpy(buf,"  ");
		break;
	   } 
	   if (exit[3]==SECT_ROAD) {
		sprintf(buf,"west.");
		strcat(desc,buf);
		strcpy(buf,"  ");
		break;
	   } 
	 } 
	 if (exit[3]==SECT_ROAD) {
	   sprintf(buf,"The road continues west and "); 
	   strcat(desc,buf);
	   strcpy(buf,"  ");
	   if (exit[1]==SECT_ROAD) { 
		sprintf(buf,"east.");
		strcat(desc,buf);
		strcpy(buf,"  ");
		break;
	   } 
	   if (exit[2]==SECT_ROAD) { 
		sprintf(buf,"south.");
		strcat(desc,buf);
		strcpy(buf,"  ");
		break; 
	   } 
	   if (exit[0]==SECT_ROAD) {
		sprintf(buf,"north.");
		strcat(desc,buf);
		strcpy(buf,"  ");
		break; 
	   } 
	 } 
	 
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE-20))||(exit[0]==SECT_RUINS)||(exit[0]!=SECT_ROAD)) {
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE-20))||(exit[1]==SECT_RUINS)||(exit[1]!=SECT_ROAD)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE-20))||(exit[2]==SECT_RUINS)||(exit[2]!=SECT_ROAD)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE-20))||(exit[3]==SECT_RUINS)||(exit[3]!=SECT_ROAD)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 break;
	 
    case SECT_WATER_SWIM:
	 strcpy(name,"Shallow Water");
	 strcpy(desc,"The water here is rather shallow. You can see many little fish "
		   "speeding to and fro in schools, skitting across the sandy bottom.");
	 if (exitsum==4) {
	   strcpy(name,"Shallow Basin");
	   strcpy(desc,"You are surrounded by shallow water. All around you affluent "
			"marine life can be seen through the crystal exterior, creating quite "
			"a show with an occasional leap above the waters surface.");
	 }
	 if (exitsum==2) {
	   strcpy(name,"Shallow Water");
	   strcpy(desc,"You are surrounded by shallow water. All around you affluent "
			"marine life can be seen through the crystal exterior, creating quite "
			"a show with an occasional leap above the waters surface.");
	 }
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 
	 break;
	 
    case SECT_WATER_NOSWIM:
	 strcpy(name,"Deep Water");
	 strcpy(desc,"The water here is of unknown depth and fairly dark. The "
		   "currents are strong and the waves equally striking. It is rumored "
		   "that this is where sharks and sea monsters thrive.");
	 if (exitsum==4) {
	   strcpy(name,"On a Sea");
	   strcpy(desc,"You are on a dark sea, surrounded by more of the same. "
			"Denizens of this dark water occasionally make themselves known, "
			"out of curiosity, or perhaps necessity.");
	 }
	 if (exitsum==2) {
	   strcpy(name,"On Deep Water");
	   strcpy(desc,"The water is rather dark and cold. You do not want to be "
			"here for long, knowing that you will either freeze or become the "
			"next meal of a hungry sea creature.");
	 }
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 
	 break;
	 
    case SECT_AIR:
	 strcpy(name,"In the Air");
	 strcpy(desc,"You are in the air.");
	 if (exitsum==4) {
	   strcpy(name,"In the Air");
	   strcpy(desc,"You are in the air, completed surrounded by more of it. "
			"Wisps of cloud-like moisture blow by and droplets of water condense "
			"in fatal collisions with one another.");
	 }
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 break;
	 
    case SECT_DESERT:
	 strcpy(name,"Desert Wasteland");
	 strcpy(desc,"You are surrounded by sand dunes and hot, dry air. An "
		   "occasional blooming cactus and sandstone formation give the "
		   "land a simple, untouched beauty.");

	 if (exitsum==4) {
	   strcpy(name,"Deep Within the Desert");
	   strcpy(desc,"You are deep within a barren desert. Sand swept by the "
			"chaotic winds forms sand dunes that stretch on seemingly forever. "
			"The shadows of vultures overhead indirectly warn of the danger in "
			"this place as the blazing sun beats down upon you.");

	 }
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 break;
	 
    case SECT_MOUNTAIN:
	 strcpy(name,"On a Mountain");
	 strcpy(desc,"You are on a tranquil mountain with an incredible view. The "
		   "sounds of chipmunks squeaking and running around echo in the air. "
		   "Though the mountains are only slightly dangerous, this land has its "
		   "share of cliffs and gorges that can be fatal.");
	 if (exitsum==4) {
	   strcpy(name,"In the Mountains");
	   strcpy(desc,"You are deep within the mountains. An occasional cliff adds "
			"to the grandeur of this once buried rock. Nature  makes this land "
			"beautiful, covering it with pine trees, shrubs and several "
			"varieties of  animals that make their homes here.");
	 }
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 break;
	 
    case SECT_ROCK_MOUNTAIN:
	 strcpy(name,"On a Mountain of Rock");
	 strcpy(desc,"You are on a dangerous, rocky mountain.  Rocks jut from the "
		   "ground like a broken bones from skin and large, unstable boulders "
		   "seem as if they might roll from beneath your feet at any moment.");
	 if (exitsum==4) {
	   strcpy(name,"Within Dangerous Mountains");
	   strcpy(desc,"You are within a dangerous rocky mountain range. You can "
			"not see a way out from here. Traveling in any direction would "
			"have you climbing a rocky cliff or falling down one.");
	 }
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 break;
	 
    case SECT_SNOW_MOUNTAIN:
	 strcpy(name,"On a Snowy Peak");
	 strcpy(desc,"You are on a desolate snowy mountain. It is freezing cold "
		   "here and devoid of all life, except your own. However, even that "
		   "could change in the blink of an eye. If you do not freeze to death, "
		   "perhaps a snowy crevasse or avalanche might lead to your demise.");
	 if (exitsum==4) {
	   strcpy(name,"Within Snowy Mountains");
	   strcpy(desc,"You are on a desolate snowy mountain. It is freezing cold "
			"here and devoid of all life, except your own. However, even that "
			"could change in the blink of an eye. If you do not freeze to death, "
			"perhaps a snowy crevasse or avalanche might lead to your demise.");
	 }
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 break;
	 
    case SECT_SWAMP:
	 strcpy(name,"Swamps Edge");
	 strcpy(desc,"You are standing at a dank swamps edge. The smell and humidity "
		   "make this a repelling place to say the least.  Something long and slimy "
		   "slithers through the water around your legs.");
	 if (exitsum==4) {
	   strcpy(name,"Swampland");
	   strcpy(desc,"You are in a dank swampland. Murky water and decadent mud "
			"surround you. Occasionally, trees and grass mar this sickeningly "
			"beautiful work of nature.");
	 }
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 break;
	 
    case SECT_JUNGLE:
	 strcpy(name,"At a Jungle");
	 strcpy(desc,"You are at the edge of a jungle. An assortment of rare and "
		   "unique plant life grows in abundance here, making it nearly impassable. "
		   "The air is thick and heavy with humidity, making the dirt at your feet sticky.");
	 if (exitsum==4) {
	   strcpy(name,"Within a Jungle");
	   strcpy(desc,"You are deep within in a jungle,  surrounded by extravagant "
			"tropical fauna. It seems there is no way out  and no end to the "
			"wall of plant life in every direction. The occasional buzz of "
			"rather large insects, or the nerve racking growl of a tiger make "
			"you leery to continue.");
	 }
	 if (((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)||(exit[0]==SECT_RUINS)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)||(exit[1]==SECT_RUINS)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)||(exit[2]==SECT_RUINS)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)||(exit[3]==SECT_RUINS)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 break;
	 
    case SECT_RUINS:
	 strcpy(name,"A Ruin");
	 strcpy(desc,"You have stumbled upon something ancient. Large boulders "
		   "with carvings on them have tumbled all over the ground, marking "
		   "the demise of a rather large building or statue.");
	 if (exitsum==4) {
	   strcpy(name,"Some Ruins");
	   strcpy(desc,"You have managed to find the ruins of an ancient city. "
			"Vines and plants have begun to sprout from the rocks that used "
			"to form buildings and statues. Now, this area is desolate and "
			"barren when it once flourished with life.");
	 }
	 if(((exit[0]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[0]==SECT_ROAD)){
	   sprintf(buf,"You see %s to the north.",get_sector_name(exit[0]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[1]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[1]==SECT_ROAD)){
	   sprintf(buf,"To the east you can see %s.",get_sector_name(exit[1]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[2]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[2]==SECT_ROAD)){
	   sprintf(buf,"South of you, you can see %s.",get_sector_name(exit[2]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 if (((exit[3]!=(room->sector_type))&&(number_percent()<CHANCE))||(exit[3]==SECT_ROAD)){
	   sprintf(buf,"%s can be seen to the west.",get_sector_name(exit[3]));
	   strcat(desc,buf);strcpy(buf,"  ");
	 }
	 break;
	 
    case SECT_OCEAN:
	 strcpy(name,"On the Ocean");
	 strcpy(desc,"Massive waves crash and collide with one another all around. "
		   "Above you, the sky is dark and very uninviting. This is no place "
		   "to be without a ship.");
	 if (exitsum == 4) {
	   strcpy(name, "Surrounded By Ocean");
	   strcpy(desc, "Dorsal fins belonging to sharks have started to peek above "
			"the surface of the turbulent waters. There is no relief in sight "
			"and without a boat, you should prepare to meet the Creator.");
	 }
	 break;
	 
    case SECT_RIVER:
	 strcpy(name,"On A River");
	 strcpy(desc,"This slow, lazy river continues to flow to its destination. "
		   "Its water is dark so that you can not see how deceptively deep it "
		   "is.");
	 if (exitsum == 4) {
	   strcpy(name, "Surrounded By River");
	   strcpy(desc, "All around, the water of the lazy flows. Hopefully, you "
			"have a boat or it will be a long swim to the other side. From "
			"time to time, colorful trout leap from the water, making you "
			"wish that you had your fishing pole.");
	 }
	 break;
	 
    case SECT_SAND:
	 strcpy(name,"On the Sand");
	 strcpy(desc,"A beach has begun to form here from sand that has washed upon "
		   "polished pebbles in high tide.  It feels warm beneath your feet, "
		   "making you wish that you could stay on this beach and relax for a "
		   "long while.");
	 if (exitsum == 4) {
	   strcpy(name, "Within the Sand");
	   strcpy(desc, "A white sand beach spreads all around you, for as far as "
			"you can see. The sand is warm and squishes between your toes. "
			"Scattered across the ground are several kinds of shells in all "
			"shapes and colors. Gulls soar overhead, occasionally releasing "
			"their long, sad cries.");
	 }
	 break;
	 
    case SECT_BLIGHT:
	 strcpy(name,"On the Blight");
	 strcpy(desc,"Standing at the edge of the Blight, you are not so sure this "
		   "is a place you want to enter. Everything is black and dead. No life "
		   "can be seen from here, and the rumors of the creatures that live within "
		   "this land makes it more repulsive.");
	 if (exitsum == 4) {
	   strcpy(name, "Within the Blight");
	   strcpy(desc, "You have entered a dead, barren land. The ground is scorched "
			"beneath your feet and giant, dead tree branches reach out to suck "
			"your life away. Perhaps you should turn back while you still can "
			"before you meet a trolloc, or something even worse.");
	 }
	 break;
	 
    case SECT_ISLAND:
	 strcpy(name,"On An Island");
	 strcpy(desc,"At the edge of this island, there is a sandy beach encircling "
		   "the shoreline.  Several feet away, rocks and vine covered trees "
		   "begin to form the mainland. Who knows what could live on this island?");
	 if (exitsum == 4) {
	   strcpy(name, "Within An Island");
	   strcpy(desc, "Like a jungle, this island is covered with thick vines and "
			"dense foliage.  The air is heavy and very humid, making it almost "
			"hard to breathe.  The sound of the ocean washing unto the shore "
			"isn't far away, so at least you can always find your way back "
			"if needed. ");
	 }
	 break;
	 
    case SECT_LAKE:
	 strcpy(name,"On A Lake");
	 strcpy(desc,"The water on the edge of the lake is quite deep, over your "
		   "head if you are trying to traverse it. The best way to make it "
		   "across this body of water is to use a boat. But be careful about "
		   "how much weight you are carrying. Even the best boats sink if they "
		   "are too heavy.");
	 if (exitsum == 4) {
	   strcpy(name, "Surrounded by Lake");
	   strcpy(desc, "Deep water surrounds you on all sides, but thankfully you "
			"can see the shore in at least one direction. Sometimes, your "
			"oars become caught in weeds that grow to the lakes surface, "
			"but keep rowing, you will get there soon enough.");
	 }
	 break;

    /* Default, that is none of the inserted ones, we return */
    default:
	 sprintf(ebuf, "Vnum <%d> has unsupported sector code for name and description.\n\r", vnum);
	 send_to_char(ebuf, ch);
	 return;
	 
    } /*switch1*/
  }

  SET_BIT( ch->in_room->area->area_flags, AREA_CHANGED );
  
  free_string( room->name );
  room->name = str_dup( name );
  
  free_string( room->description );
  room->description= str_dup ( desc );
  
  format_string(room->description);

  sprintf(ebuf, "Vnum <%d> assigned name and description.\n\r", vnum);
  send_to_char(ebuf, ch);
  
  return;
}  
/* this sets the whole vmap default descs and names */
void do_vmapdesc_all(CHAR_DATA *ch, char *argument)
{
  ROOM_INDEX_DATA *room;
  int start,end,current;
  start=40000;
  end  =58000;
  
  room=get_room_index(start);
  /* initialize room for this check here */
  if (!IS_BUILDER(ch,room->area)) {
    send_to_char("Wset_all: Insufficient Security",ch);
    return;
  }	
  
  for (current=start ; current <= end ; current++) {
    room=get_room_index(current);
    if (room != NULL) {
	 char_to_room( ch, room );
	 do_vmapdesc(ch,"");
	 char_from_room( ch );
    }
  }
  char_to_room( ch, room );
  send_to_char("All vmap entries set with default descs and names.\n\r", ch);
  return;
}
