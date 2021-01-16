/*------------------------------ INCLUDES --------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "merc.h"

/*------------------ CONSTANTS AND GLOBALS DEFINITIONS -------------------*/

BUFFER *output;

typedef struct k {
  char helpMsg[256];
}k;

typedef struct help_entry{
  unsigned int entry;
  int times;
  unsigned int vnum;
  char pname[32];
}help_entry;

typedef struct k Key;
typedef struct help_entry Data;

/* Ordering algorithm in an unsafe macro */
#define KEYCMP( l, r )	(strcasecmp(l.helpMsg, r.helpMsg))

/* typedef enum { false, true } bool; */
#define false  0
#define true   1


/* Users do not know about trees, type Index is a handle */
typedef void * Index;

bool TLookup( Index *, Key, Data * );
bool TInsert( Index *, Key, Data );
bool TDelete( Index *, Key );
//bool TDump(Index * ip);

typedef struct tNode
{
  struct tNode *left, *right;
  Key key;
  Data data;
}*TreePtr;

/* Data structures and types for splaying tree */
typedef enum { ROOT, LEFT, RIGHT } WhichLeg;
typedef struct ptrList
{
  struct ptrList *parent;
  TreePtr *tpp;
  WhichLeg leg;
}Route;

/* Basic freelist and new node management */
static TreePtr freeNodes = NULL;
static TreePtr newNode( void )
{
  TreePtr tp = freeNodes;
  
  if( tp == NULL )
    return malloc( sizeof (struct tNode) );
  freeNodes = freeNodes->left;
  return tp;
}
static void freeNode( TreePtr node )
{
  node->left = freeNodes;
  freeNodes = node;
}

static TreePtr newLeaf( Key key, Data data )
{
  TreePtr tp = newNode();
  
  tp->key = key;
  tp->data = data;
  tp->left = tp->right = NULL;
  return tp;
}

/*   Search for node in tree - used in insertion and deletion   */
static TreePtr *findNode( TreePtr *pp, Key key )
{
  if( *pp != NULL ) {
    int order = KEYCMP( key, (*pp)->key );
    
    if( order < 0 )
	 return findNode( &(*pp)->left, key );
    if( order > 0 )
	 return findNode( &(*pp)->right, key );
  }
  return pp;
}

/* Swap position of two nodes in tree */
static void exchange( TreePtr *pp1, TreePtr *pp2, TreePtr *pp3 )
{
  TreePtr tmp = *pp1;
  
  *pp1 = *pp2;
  *pp2 = *pp3;
  *pp3 = tmp;
}

static void rotate( Route *r )
{
  TreePtr *nodeBelow;
  
  if( r->leg == LEFT )
    nodeBelow = &(*r->tpp)->right;
  else
    nodeBelow = &(*r->tpp)->left;
  exchange( r->parent->tpp, r->tpp, nodeBelow );
  *r = *r->parent;
}

static void splay( Route *r )
{
  while( r->leg != ROOT ) {		  /* Until root is reached */
    if( r->parent->leg != ROOT ) {	  /* Unless child of root */
	 if( r->leg == r->parent->leg )	/* If same-handed */
	   rotate( r->parent );
	 else
	   rotate( r );
    }
    rotate( r );
  }
}

/* Splay a node to root if found in tree */
static bool doSplay( TreePtr *pp, Key key, Route *rp, WhichLeg leg )
{
  /*	Route thisLev = { rp, pp, leg };	*/
  Route thisLev;
  
  thisLev.parent = rp;
  thisLev.tpp = pp;
  thisLev.leg = leg;
  
  if( *pp != NULL ) {
    int order = KEYCMP( key, (*pp)->key );
    
    if( order < 0 )
	 return doSplay( &(*pp)->left, key, &thisLev, LEFT );
    if( order > 0 )
	 return doSplay( &(*pp)->right, key, &thisLev, RIGHT );
    splay( &thisLev );
    return true;
  }
  return false;
}

/*   Users' lookup function for simple binary tree   */
bool TLookup( Index *ip, Key key, Data *dstPtr )
{
  TreePtr *pp = (TreePtr *) ip;
  
  if( doSplay( pp, key, NULL, ROOT ) == true ) {
    *dstPtr = (*pp)->data;
    return true;
  }
  return false;
}

/*   Users' node insertion function for simple binary tree   */
bool TInsert( Index *ip, Key key, Data data )
{
  TreePtr *pp = (TreePtr *)ip;
  
  if( *(pp = findNode( pp, key )) == NULL ) {
    *pp = newLeaf( key, data );
    return true;
  }
  (*pp)->data.times++;
  return false;
}

/*   Users' node deletion function for simple binary tree   */
bool TDelete( Index *ip, Key key )
{
  TreePtr *pp = (TreePtr *)ip;
  
  if( *(pp = findNode( pp, key )) != NULL ) {
    TreePtr subTree = (*pp)->left;
    
    freeNode( *pp );
    *pp = (*pp)->right;
    while( *pp != NULL )
	 pp = &(*pp)->left;
    *pp = subTree;
    return true;
  }
  return false;
}

/*   Tree printing routines, not mentioned in viewgraphs   */
/*   User must provide functions PrtKey( Key) and PrtData( Data )  */
/*   which print the key and data value, respectively  */
static void OutSubTree(CHAR_DATA *ch, TreePtr tp, int spaces )
{ 
  char buf[MAX_STRING_LENGTH];
   
  if( tp != NULL )
    {
	 OutSubTree(ch, tp->left, spaces);
    if (tp->data.times >= 5) {
	   sprintf(buf,"<{R%3d{x> : %s\n", tp->data.times, tp->key.helpMsg);
    }
    else {
      sprintf(buf,"<%3d> : %s\n", tp->data.times, tp->key.helpMsg);
    }
    add_buf(output,buf);
	 OutSubTree(ch, tp->right, spaces);
    }
}

/*   Users' entry point for tree dumping   */
void TreeDump(CHAR_DATA *ch, Index *ip )
{
  char buf[MAX_STRING_LENGTH];
  TreePtr tp = (TreePtr) *ip;

  sprintf(buf, "------:----------------------------------------------------\n\r");  
  strcat(buf , "{WTimes : Help entry not found                              :{x\n\r");
  strcat(buf , "------:----------------------------------------------------\n\r");
  add_buf(output,buf);
  OutSubTree(ch, tp, 0 );
  page_to_char( buf_string(output), ch );
}

void do_helplog(CHAR_DATA *ch, char *argument )
{
   FILE *rfp;
   char readbuffer[256];
   char buf[MAX_STRING_LENGTH];
   unsigned int cnt=0;
   char *ptr;
   int column=0;
   char vnum[6];

   /* Tree */
   Key k;
   Data d;
   Index ix = 0;

   if ( argument[0] == '\0' ) {
      send_to_char("Syntax:\n\r", ch );
      send_to_char("helplog <view/print/reset>\n\r", ch);
      return;
   }
   
   /* reset buffer each time called */
   memset(readbuffer, 0x00, sizeof(readbuffer));
   
   output = new_buf();
   
   /* test if we can open the file */
   if ((rfp = fopen(HELP_FILE, "r")) == NULL) {
	sprintf(buf,"No entries in the Helplog. The file <%s> don't exist.\n\r", HELP_FILE);
	send_to_char(buf, ch);
	return;
   }
   
   if (!str_prefix (argument, "view")) {
	while (fgets(readbuffer, sizeof(readbuffer), rfp)) {
	  cnt++;
	  for (column = 1;column < 4;column++) {
	    switch (column) {
	    case 1:
		 ptr = strtok(readbuffer, "]");
		 memset(vnum, 0x00, sizeof(vnum));
		 memcpy(vnum, &ptr[1], 5);
		 d.vnum = atoi(vnum);
		 break;
	    case 2:
		 ptr = strtok(NULL, ":");
		 *ptr++;
		 strcpy(d.pname, ptr);
		 break;
	    case 3:
		 ptr = strtok(NULL, "\n");
		 *ptr++;
		 strcpy(k.helpMsg, ptr);
		 break;   
	    }
	  }
	  d.entry = cnt;
	  d.times = 1;
	  TInsert(&ix, k, d);
	}
	TreeDump(ch, &ix);
	free_buf(output);
	fclose(rfp);
	return;
   }
   
   if (!str_prefix (argument, "print")) {
	while (fgets(readbuffer, sizeof(readbuffer), rfp)) {
	  cnt++;
	  sprintf(buf, "[%3d]: %s",cnt,readbuffer);
	  add_buf(output,buf);
	}
	page_to_char( buf_string(output), ch );
	free_buf(output);
	fclose(rfp);
	return;
   }
   
   if (!str_prefix (argument, "reset")) {
	sprintf(buf, "Helplog reset to initial state. File <%s> removed.\n",HELP_FILE); 
	send_to_char(buf, ch);
	unlink(HELP_FILE);
	return;
   }
   return;
}
