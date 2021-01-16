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
#include "recycle.h"

#define STANDARD_ALLOCMEM(type, name)			\
type * name ## _free;					\
int name ## _created;					\
int name ## _allocated;					\
							\
type * new_ ## name( void )				\
{							\
	type * temp;					\
static	type   tZero;					\
							\
	if ( name ## _free )				\
	{						\
		temp = name ## _free;			\
		name ## _free = name ## _free->next;	\
	}						\
	else						\
	{						\
		temp = alloc_mem( sizeof(*temp) );	\
		name ## _allocated++;			\
	}						\
							\
	*temp = tZero;					\
							\
	name ## _created++;				\
							\
	return temp;					\
}

#define STANDARD_FREEMEM(type, name)			\
int name ## _freed;					\
							\
void free_ ## name( type * temp )			\
{							\
	temp->next = name ## _free;			\
	name ## _free = temp;				\
	name ## _freed++;				\
}

#define VAL_ALLOCMEM(type, name)			\
type * name ## _free;					\
int name ## _created;					\
int name ## _allocated;					\
							\
type * new_ ## name( void )				\
{							\
	type * temp;					\
static	type   tZero;					\
							\
	if ( name ## _free )				\
	{						\
		temp = name ## _free;			\
		name ## _free = name ## _free->next;	\
	}						\
	else						\
	{						\
		temp = alloc_mem( sizeof(*temp) );	\
		name ## _allocated++;			\
	}						\
							\
	*temp = tZero;					\
							\
	VALIDATE(temp);					\
							\
	name ## _created++;				\
							\
	return temp;					\
}

#define VAL_FREEMEM(type, name)				\
int name ## _freed;					\
							\
void free_ ## name( type * temp )			\
{							\
	if (!IS_VALID(temp))				\
		return;					\
							\
	INVALIDATE(temp);				\
	temp->next = name ## _free;			\
	name ## _free = temp;				\
	name ## _freed++;				\
}

#define allocfunc(a,b)		\
STANDARD_ALLOCMEM(a,b)		\
STANDARD_FREEMEM(a,b)

#define allocfunc_val(a,b)	\
VAL_ALLOCMEM(a,b)		\
VAL_FREEMEM(a,b)

#include "allocfunc.h"

#undef allocfunc
#undef allocfunc_val


/* Stuff for the PK Ranking system */
PKINFO_TYPE	  *     pkranks = NULL;

void load_pkfile()
{
        FILE * fp = fopen(PKRANK_FILE,"r");
	if (fp)
	{
		PKINFO_TYPE * pkholder;
		char * tmp = fread_string(fp);
		while (strcmp(tmp,"END"))
		{
			PKINFO_TYPE * pk = alloc_perm(sizeof(*pk));
			pk->character = str_dup(tmp);
			bug("Read: %s",pk->character);
			pk->pk_count = fread_number(fp);
			bug("Read: %d",pk->pk_count);
			pk->pk_death_count = fread_number(fp);
			bug("Read: %d",pk->pk_death_count);
			if (pkranks == NULL)
			{
				pkranks = pk;
				pkholder = pk;
				pk->next = NULL;
			}
			else;
			{
				pkholder->next = pk;
				pkholder = pk;
				pk->next = NULL;

			}
			tmp = fread_string(fp);
		}		
	}
	else
	{
		pkranks = NULL;
		fp = fopen(PKRANK_FILE,"w");
		fprintf(fp,"END~\n");
		fclose(fp);
	}
}
PKINFO_TYPE * pkupdate(CHAR_DATA * ch)
{
	static PKINFO_TYPE pk_zero;
	PKINFO_TYPE * pk;

	if (pkranks == NULL)
	{
		pk = alloc_perm(sizeof(*pk));
		pkranks = pk;	
		pk->character = str_dup(ch->name);
		pk->pk_count = ch->pk_count;
		pk->pk_death_count = ch->pk_died_count;
		pk->next = NULL;
		pk_zero = *pk;
	}
	else
	{
		//pk = &pk_zero;
		pk = pkranks;
		PKINFO_TYPE * prior;
		prior = pk;
		while (pk && strcmp(pk->character,ch->name))
		{
			prior = pk;
			pk = pk->next;
		}
		if (!pk)
		{
			pk = alloc_perm(sizeof(*pk));
			prior->next = pk;
			pk->next = NULL;
			pk->character = str_dup(ch->name);
			pk->pk_count = ch->pk_count;
			pk->pk_death_count = ch->pk_died_count;
		}
		else if (pk && !strcmp(pk->character, ch->name))
		{
			pk->pk_count = ch->pk_count;
			pk->pk_death_count = ch->pk_died_count;

		}
	}

        FILE * fp = fopen(PKRANK_FILE,"w");
        if (fp)
        {
	   PKINFO_TYPE * tmp = &pk_zero;
	   while(tmp)
	   {
		fprintf(fp,"%s~\n",tmp->character);
	   	fprintf(fp,"%d %d\n",tmp->pk_count, tmp->pk_death_count);
		tmp = tmp->next;
	   }
	   fprintf(fp,"END~\n");
           fclose(fp);
        }
	return pk;
}


/* stuff for recycling ban structures */
BAN_DATA *ban_free;

BAN_DATA *new_ban(void)
{
    static BAN_DATA ban_zero;
    BAN_DATA *ban;

    if (ban_free == NULL)
	ban = alloc_perm(sizeof(*ban));
    else
    {
	ban = ban_free;
	ban_free = ban_free->next;
    }

    *ban = ban_zero;
    VALIDATE(ban);
    ban->name = &str_empty[0];
    return ban;
}

void free_ban(BAN_DATA *ban)
{
    if (!IS_VALID(ban))
	return;

    free_string(ban->name);
    INVALIDATE(ban);

    ban->next = ban_free;
    ban_free = ban;
}

/* stuff for recycling descriptors */
DESCRIPTOR_DATA *descriptor_free;

DESCRIPTOR_DATA *new_descriptor(void)
{
    static DESCRIPTOR_DATA d_zero;
    DESCRIPTOR_DATA *d;

    if (descriptor_free == NULL)
	d = alloc_perm(sizeof(*d));
    else
    {
	d = descriptor_free;
	descriptor_free = descriptor_free->next;
    }
	
    *d = d_zero;
    VALIDATE(d);
    
    d->connected	= CON_GET_NAME;
    d->showstr_head	= NULL;
    d->showstr_point = NULL;
    d->outsize	= 2000;
    d->pEdit		= NULL;			/* OLC */
    d->pString	= NULL;			/* OLC */
    d->editor	= 0;			/* OLC */
    d->outbuf	= alloc_mem( d->outsize );
    
    return d;
}

void free_descriptor(DESCRIPTOR_DATA *d)
{
    if (!IS_VALID(d))
	return;

    free_string( d->ipaddr );
    free_string( d->host );
    free_mem( d->outbuf, d->outsize );
    INVALIDATE(d);
    d->next = descriptor_free;
    descriptor_free = d;
}

/* stuff for recycling gen_data */
GEN_DATA *gen_data_free;

GEN_DATA *new_gen_data(void)
{
    static GEN_DATA gen_zero;
    bool * tempgendata;
    GEN_DATA *gen;

    if (gen_data_free == NULL)
	gen = alloc_perm(sizeof(*gen));
    else
    {
	gen = gen_data_free;
	gen_data_free = gen_data_free->next;
    }
    *gen = gen_zero;

    tempgendata = realloc( gen->skill_chosen, sizeof( bool ) * MAX_SKILL );
    gen->skill_chosen			= tempgendata;
    gen->skill_chosen[MAX_SKILL-1]	= 0;

    tempgendata = realloc( gen->group_chosen, sizeof( bool ) * MAX_GROUP );
    gen->group_chosen			= tempgendata;
    gen->group_chosen[MAX_GROUP-1]	= 0;




    VALIDATE(gen);
    return gen;
}

void free_gen_data(GEN_DATA *gen)
{
    if (!IS_VALID(gen))
	return;

    INVALIDATE(gen);

    gen->next = gen_data_free;
    gen_data_free = gen;
} 

/* stuff for recycling extended descs */
EXTRA_DESCR_DATA *extra_descr_free;

EXTRA_DESCR_DATA *new_extra_descr(void)
{
    EXTRA_DESCR_DATA *ed;

    if (extra_descr_free == NULL)
	ed = alloc_perm(sizeof(*ed));
    else
    {
	ed = extra_descr_free;
	extra_descr_free = extra_descr_free->next;
    }

    ed->keyword = &str_empty[0];
    ed->description = &str_empty[0];
    VALIDATE(ed);
    return ed;
}

void free_extra_descr(EXTRA_DESCR_DATA *ed)
{
    if (!IS_VALID(ed))
	return;

    free_string(ed->keyword);
    free_string(ed->description);
    INVALIDATE(ed);
    
    ed->next = extra_descr_free;
    extra_descr_free = ed;
}

/* stuff for recycling room weaves */
AFFECT_DATA *room_weave_free;

AFFECT_DATA *new_room_weave(void)
{
  static AFFECT_DATA wd_zero;
  AFFECT_DATA *wd;

  if (room_weave_free == NULL)
    wd = alloc_perm(sizeof(*wd));
  else {
    wd = room_weave_free;
    room_weave_free = room_weave_free->next;
  }
  
  *wd = wd_zero;
  VALIDATE(wd);
  return wd;
}

void free_room_weave(AFFECT_DATA *wd)
{
  if (!IS_VALID(wd))
    return;
  
  INVALIDATE(wd);
  wd->next = room_weave_free;
  room_weave_free = wd;
}

/* stuff for recycling wards */
WARD_DATA *ward_free;

WARD_DATA *new_ward(void)
{
  static WARD_DATA wd_zero;
  WARD_DATA *wd;

  if (ward_free == NULL)
    wd = alloc_perm(sizeof(*wd));
  else {
    wd = ward_free;
    ward_free = ward_free->next;
  }

  *wd = wd_zero;
  VALIDATE(wd);
  return wd;
}

void free_ward(WARD_DATA *wd)
{
  if (!IS_VALID(wd))
    return;

  INVALIDATE(wd);
  wd->next = ward_free;
  ward_free = wd;
}

/* stuff fof recycling residues */
RESIDUE_DATA *residue_free;

RESIDUE_DATA *new_residue(void)
{
  static RESIDUE_DATA rd_zero;
  RESIDUE_DATA *rd;

  if (residue_free == NULL)
    rd = alloc_perm(sizeof(*rd));
  else {
    rd = residue_free;
    residue_free = residue_free->next;
  }

  *rd = rd_zero;
  VALIDATE(rd);
  return rd;
}

void free_residue(RESIDUE_DATA *rd)
{
  if (!IS_VALID(rd))
    return;

  INVALIDATE(rd);
  rd->next = residue_free;
  residue_free = rd;
}


/* stuff for recycling affects */
AFFECT_DATA *affect_free;

AFFECT_DATA *new_affect(void)
{
    static AFFECT_DATA af_zero;
    AFFECT_DATA *af;

    if (affect_free == NULL)
	af = alloc_perm(sizeof(*af));
    else
    {
	af = affect_free;
	affect_free = affect_free->next;
    }

    *af = af_zero;


    VALIDATE(af);
    return af;
}

void free_affect(AFFECT_DATA *af)
{
    if (!IS_VALID(af))
	return;

    INVALIDATE(af);
    af->next = affect_free;
    affect_free = af;
}

/* stuff for recycling objects */
OBJ_DATA *obj_free;

OBJ_DATA *new_obj(void)
{
    static OBJ_DATA obj_zero;
    OBJ_DATA *obj;

    if (obj_free == NULL)
	obj = alloc_perm(sizeof(*obj));
    else
    {
	obj = obj_free;
	obj_free = obj_free->next;
    }
    //*obj = obj_zero;
    memset(obj,0,sizeof(OBJ_DATA));
    VALIDATE(obj);

    return obj;
}

void free_obj(OBJ_DATA *obj)
{
    AFFECT_DATA *paf, *paf_next;
    EXTRA_DESCR_DATA *ed, *ed_next;

    if (!IS_VALID(obj))
	return;

    for (paf = obj->affected; paf != NULL; paf = paf_next)
    {
	paf_next = paf->next;
	free_affect(paf);
    }
    obj->affected = NULL;

    for (ed = obj->extra_descr; ed != NULL; ed = ed_next )
    {
	ed_next = ed->next;
	free_extra_descr(ed);
     }
     obj->extra_descr = NULL;
   
    free_string( obj->name        );
    free_string( obj->description );
    free_string( obj->short_descr );
    free_string( obj->owner     );
    INVALIDATE(obj);

    obj->next   = obj_free;
    obj_free    = obj; 
}

/* stuff for recyling intros */
ID_NAME *idname_free;

void free_idname( struct idName *idname )
{
  /* Set next pointer to current head of free list */
  idname->next = idname_free;
  /* Point head of free list to newly freed struct */
  idname_free = idname;
}

ID_NAME *new_idname(void)
{
   static struct idName idname_Zero;
   struct idName *idname;
  
   /* Check for previously freed structures */
   if ( idname_free == NULL ) {
	idname = alloc_perm( sizeof( *idname ) );
   }
   else {
	/* Got one so point to it */
	idname = idname_free;
	/* Move the free list on one */
	idname_free = idname_free->next;
   }
  
   /* Empty recycled structures */
   *idname = idname_Zero;
   idname->name              = &str_empty[0];
   idname->id                = 0;
   
   return idname;
}

/* stuff for recyling characters */
CHAR_DATA *char_free;

CHAR_DATA *new_char (void)
{
  static CHAR_DATA ch_zero;
  CHAR_DATA *ch;
  int i;
  
  if (char_free == NULL)
    ch = alloc_perm(sizeof(*ch));
  else {
    ch = char_free;
    char_free = char_free->next;
  }
  
  //*ch				= ch_zero;
  memset(ch,0,sizeof(CHAR_DATA));
  VALIDATE(ch);
  ch->name                    = &str_empty[0];
  ch->wkname                  = &str_empty[0];
  ch->real_name               = &str_empty[0];
  ch->short_descr             = &str_empty[0];
  ch->long_descr              = &str_empty[0];
  ch->description             = &str_empty[0];
  ch->hood_description        = &str_empty[0];
  ch->veil_description        = &str_empty[0];
  ch->wolf_description        = &str_empty[0];
  ch->wound_description       = &str_empty[0];
  ch->aura_description        = &str_empty[0];
  ch->prompt                  = &str_empty[0];
  ch->prefix                  = &str_empty[0];
  for (i=0; i<MAX_SHEAT_LOC; i++)
    ch->sheat_where_name[i] = NULL;
  
  ch->logon                   = current_time;
  ch->surrender_timeout       = current_time - 1;
  ch->lastrpupdate            = current_time;
  ch->roleplayed 	      = 0;
  ch->lines                   = PAGELEN;
  ch->in_obj                  = NULL;
  for (i = 0; i < 4; i++)
    ch->armor[i]            = 100;
  ch->bIsLinked               = FALSE;
  ch->bIsReadyForLink         = FALSE;
  ch->link_info               = NULL;
  ch->pIsLinkedBy	      = NULL;
  ch->position                = POS_STANDING;
  ch->hit                     = MIN_PC_HP;
  ch->max_hit                 = MIN_PC_HP;
  ch->endurance               = MIN_PC_END;
  ch->max_endurance           = MIN_PC_END;
  ch->gain_xp = FALSE;
  ch->pk_count 		      = 0;
  ch->pk_died_count 		      = 0;
  ch->study = FALSE;
  ch->study_pulse = -1;
  ch->arrow_count = 0;
  ch->exit_block.vnum      = -1;
  ch->exit_block.direction = -1;
  ch->exit_block.blocking  = FALSE;   
  ch->arena = 0;
  ch->next_trainmove = 0;
  ch->act = 0;
  ch->act2 = 0;
  ch->to_hunt = NULL;
  ch->carrying = NULL;

  for (i = 0; i < MAX_STATS; i ++)
    {
	 ch->perm_stat[i] = 13;
	 ch->mod_stat[i] = 0;
    }
  
  return ch;
}


void free_char (CHAR_DATA *ch)
{
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    if (!IS_VALID(ch))
	return;

    if (IS_NPC(ch))
	mobile_count--;

    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    {
	obj_next = obj->next_content;
	extract_obj(obj);
    }

    for (paf = ch->affected; paf != NULL; paf = paf_next)
    {
	paf_next = paf->next;
	affect_remove(ch,paf);
    }


    free_string(ch->name);
    free_string(ch->wkname);
    free_string(ch->real_name);
    free_string(ch->short_descr);
    free_string(ch->long_descr);
    free_string(ch->description);
    free_string(ch->hood_description);
    free_string(ch->veil_description);
    free_string(ch->wolf_description);
    free_string(ch->wound_description);
    free_string(ch->aura_description);
    free_string(ch->prompt);
    free_string(ch->prefix);
    
    free_string(ch->gtitle);
    free_string(ch->sguild_title);
    free_string(ch->ssguild_title);
    free_string(ch->mtitle);
    free_string(ch->mname);
    
    
  /*  free_note  (ch->pnote); */
    free_pcdata(ch->pcdata);

    ch->next = char_free;
    char_free  = ch;

    INVALIDATE(ch);
    return;
}

PC_DATA *pcdata_free;

PC_DATA *new_pcdata(void)
{
    int cnt;

    static PC_DATA pcdata_zero;
    PC_DATA *pcdata;

    if (pcdata_free == NULL)
	pcdata = alloc_perm(sizeof(*pcdata));
    else
    {
	pcdata = pcdata_free;
	pcdata_free = pcdata_free->next;
    }

    *pcdata = pcdata_zero;

    for (cnt = 0; cnt < MAX_ALIAS; cnt++)
    {
	pcdata->alias[cnt] = NULL;
	pcdata->alias_sub[cnt] = NULL;
    }

    for (cnt = 0; cnt < MAX_IGNORE; cnt++) {
        pcdata->ignore[cnt] = NULL;
    }
    
    for (cnt = 0; cnt < MAX_DISGUISE; cnt++) {
       pcdata->disguise[cnt] = NULL;
    }

    pcdata->buffer = new_buf();
    pcdata->polls = str_dup("");

#if !defined(FIRST_BOOT)
    pcdata->learned = new_learned();
    pcdata->group_known = new_gknown();
#endif

    pcdata->reward_multiplier = 0;
    pcdata->reward_time = current_time; 
    
    pcdata->keepoldstats = FALSE;
    pcdata->referrer = str_dup("");
    pcdata->forcespark = 0;
    pcdata->forceinsanity = 0;
    pcdata->timeoutstamp = 0;
    pcdata->rprewardtimer = 0;
    pcdata->rpbonus = 0;
    pcdata->bondcount = 0;
    pcdata->bondedbysex = 0;
    pcdata->createangrealcount = 0;
    pcdata->next_24hourangreal = 0;
    pcdata->next_createangreal = 0;
    pcdata->keys = NULL;
    
    VALIDATE(pcdata);
    return pcdata;
}
	

void free_pcdata(PC_DATA *pcdata)
{
    int cnt;
    GATEKEY_DATA * keys;
    GATEKEY_DATA * keys_next;

    if (!IS_VALID(pcdata))
	return;

#if !defined(FIRST_BOOT)
    free_learned(pcdata->learned);
    free_gknown(pcdata->group_known);
#endif

    free_string(pcdata->email);
    free_string(pcdata->pwd);
    free_string(pcdata->bamfin);
    free_string(pcdata->bamfout);
    free_string(pcdata->title);    
    free_string(pcdata->appearance);
    free_string(pcdata->hood_appearance);
    free_string(pcdata->veil_appearance);
    free_string(pcdata->wolf_appearance);
    free_string(pcdata->dreaming_appearance);
    free_string(pcdata->illusion_appearance);
    free_string(pcdata->ictitle);
    free_string(pcdata->imm_info);
    free_string(pcdata->lastsite);
    free_string(pcdata->lastlog);
    free_string(pcdata->afkmsg);
    free_string(pcdata->semote);
    
    free_string(pcdata->say_voice);
    free_string(pcdata->ask_voice);
    free_string(pcdata->exclaim_voice);
    free_string(pcdata->battlecry_voice);
    
    free_string(pcdata->bondedby);
    free_string(pcdata->restoremessage);
    
    free_string(pcdata->df_name);
    free_string(pcdata->polls);

    free_buf(pcdata->buffer);
    
    for (cnt = 0; cnt < MAX_ALIAS; cnt++)
    {
	free_string(pcdata->alias[cnt]);
	free_string(pcdata->alias_sub[cnt]);
    }
    
    for (cnt = 0; cnt < MAX_IGNORE; cnt++) {
       free_string(pcdata->ignore[cnt]);
    }
    
    for (cnt = 0; cnt < MAX_DISGUISE; cnt++) {
       free_string(pcdata->disguise[cnt]);
    }

    for (keys = pcdata->keys; keys != NULL; keys = keys_next) 
    {
	keys_next = keys->next;
	free_string(keys->key_alias);
	free_string(keys->key_value);
	free(keys);
    } 
    
    INVALIDATE(pcdata);
    pcdata->next = pcdata_free;
    pcdata_free = pcdata;

    return;
}

	


/* stuff for setting ids */
long	last_pc_id;
long	last_mob_id;

long get_pc_id(void)
{
    int val;

    val = (current_time <= last_pc_id) ? last_pc_id + 1 : current_time;
    last_pc_id = val;
    return val;
}

long get_mob_id(void)
{
    last_mob_id++;
    return last_mob_id;
}

MEM_DATA *mem_data_free;

/* procedures and constants needed for buffering */

BUFFER *buf_free;

MEM_DATA *new_mem_data(void)
{
    MEM_DATA *memory;
  
    if (mem_data_free == NULL)
	memory = alloc_mem(sizeof(*memory));
    else
    {
	memory = mem_data_free;
	mem_data_free = mem_data_free->next;
    }

    memory->next = NULL;
    memory->id = 0;
    memory->reaction = 0;
    memory->when = 0;
    VALIDATE(memory);

    return memory;
}

void free_mem_data(MEM_DATA *memory)
{
    if (!IS_VALID(memory))
	return;

    memory->next = mem_data_free;
    mem_data_free = memory;
    INVALIDATE(memory);
}



/* buffer sizes */
const int buf_size[MAX_BUF_LIST] =
{
    16,32,64,128,256,1024,2048,4096,8192,16384
};

/* local procedure for finding the next acceptable size */
/* -1 indicates out-of-boundary error */
int get_size (int val)
{
    int i;

    for (i = 0; i < MAX_BUF_LIST; i++)
	if (buf_size[i] >= val)
	{
	    return buf_size[i];
	}
    
    return -1;
}

BUFFER *new_buf()
{
    BUFFER *buffer;

    if (buf_free == NULL) 
	buffer = alloc_perm(sizeof(*buffer));
    else
    {
	buffer = buf_free;
	buf_free = buf_free->next;
    }

    buffer->next	= NULL;
    buffer->state	= BUFFER_SAFE;
    buffer->size	= get_size(BASE_BUF);

    buffer->string	= alloc_mem(buffer->size);
    buffer->string[0]	= '\0';
    VALIDATE(buffer);

    return buffer;
}

BUFFER *new_buf_size(int size)
{
    BUFFER *buffer;
 
    if (buf_free == NULL)
        buffer = alloc_perm(sizeof(*buffer));
    else
    {
        buffer = buf_free;
        buf_free = buf_free->next;
    }
 
    buffer->next        = NULL;
    buffer->state       = BUFFER_SAFE;
    buffer->size        = get_size(size);
    if (buffer->size == -1)
    {
        bug("new_buf: buffer size %d too large.",size);
        exit(1);
    }
    buffer->string      = alloc_mem(buffer->size);
    buffer->string[0]   = '\0';
    VALIDATE(buffer);
 
    return buffer;
}


void free_buf(BUFFER *buffer)
{
    if (!IS_VALID(buffer))
	return;

    free_mem(buffer->string,buffer->size);
    buffer->string = NULL;
    buffer->size   = 0;
    buffer->state  = BUFFER_FREED;
    INVALIDATE(buffer);

    buffer->next  = buf_free;
    buf_free      = buffer;
}


bool add_buf(BUFFER *buffer, char *string)
{
    int len;
    char *oldstr;
    int oldsize;

    oldstr = buffer->string;
    oldsize = buffer->size;

    if (buffer->state == BUFFER_OVERFLOW) /* don't waste time on bad strings! */
	return FALSE;

    len = strlen(buffer->string) + strlen(string) + 1;

    while (len >= buffer->size) /* increase the buffer size */
    {
	buffer->size 	= get_size(buffer->size + 1);
	{
	    if (buffer->size == -1) /* overflow */
	    {
		buffer->size = oldsize;
		buffer->state = BUFFER_OVERFLOW;
		bug("buffer overflow past size %d",buffer->size);
		return FALSE;
	    }
  	}
    }

    if (buffer->size != oldsize)
    {
	buffer->string	= alloc_mem(buffer->size);

	strcpy(buffer->string,oldstr);
	free_mem(oldstr,oldsize);
    }

    strcat(buffer->string,string);
    return TRUE;
}


void clear_buf(BUFFER *buffer)
{
    buffer->string[0] = '\0';
    buffer->state     = BUFFER_SAFE;
}


char *buf_string(BUFFER *buffer)
{
    return buffer->string;
}

/* stuff for recycling mobprograms */
MPROG_LIST *mprog_free;

MPROG_LIST *new_mprog(void)
{
   static MPROG_LIST mp_zero;
   MPROG_LIST *mp;

   if (mprog_free == NULL)
       mp = alloc_perm(sizeof(*mp));
   else
   {
       mp = mprog_free;
       mprog_free=mprog_free->next;
   }

   *mp = mp_zero;
   mp->vnum             = 0;
   mp->trig_type        = 0;
   mp->code             = str_dup("");
   VALIDATE(mp);
   return mp;
}

void free_mprog(MPROG_LIST *mp)
{
   if (!IS_VALID(mp))
      return;

   INVALIDATE(mp);
   mp->next = mprog_free;
   mprog_free = mp;
}

HELP_AREA * had_free;

HELP_AREA * new_had ( void )
{
	HELP_AREA * had;
static	HELP_AREA   zHad;

	if ( had_free )
	{
		had		= had_free;
		had_free	= had_free->next;
	}
	else
		had		= alloc_perm( sizeof( *had ) );

	*had = zHad;

	return had;
}

HELP_DATA * help_free;

HELP_DATA * new_help ( void )
{
	HELP_DATA * help;

	if ( help_free )
	{
		help		= help_free;
		help_free	= help_free->next;
	}
	else
		help		= alloc_perm( sizeof( *help ) );

	return help;
}

void free_help(HELP_DATA *help)
{
	free_string(help->keyword);
	free_string(help->text);
	help->next = help_free;
	help_free = help;
}

sh_int *new_learned( void )
{
	sh_int *temp;
	int i;

	temp = malloc( sizeof( sh_int ) * MAX_SKILL );

	for ( i = 0; i < MAX_SKILL; ++i )
		temp[i] = 0;

	return temp;
}

void	free_learned( sh_int *temp )
{
	free(temp);
	temp = NULL;
	return;
}

bool	*new_gknown( void )
{
	bool *temp;
	int i;
	
	temp = malloc( sizeof( bool ) * MAX_GROUP );
	for ( i = 0; i < MAX_GROUP; ++i )
		temp[i] = FALSE;
	
	return temp;
}

void	free_gknown( bool *temp )
{
	free(temp);
	temp = NULL;
	return;
}

SLEEP_DATA *sd_free;

LINK_DATA * new_link_info()
{
  LINK_DATA *ptrLinkData = NULL;

  ptrLinkData = malloc(sizeof(*ptrLinkData));

  ptrLinkData->linked = NULL;
  ptrLinkData->next   = NULL;
  
  return ptrLinkData;
}

void free_link_info(LINK_DATA * ptrLinkData)
{
	ptrLinkData->linked = NULL;
	ptrLinkData->next   = NULL;
	free(ptrLinkData);
}

SLEEP_DATA *new_sleep_data(void)
{
  SLEEP_DATA *sd;
  if (sd_free == NULL)
    sd = alloc_perm(sizeof(*sd));
  else {
    sd = sd_free;
    sd_free=sd_free->next;
  }
  
  sd->vnum             = 0;
  sd->timer            = 0;
  sd->line             = 0;
  sd->prog             = NULL;
  sd->mob              = NULL;
  sd->ch               = NULL;
  sd->next             = NULL;
  sd->prev             = NULL;
  VALIDATE(sd);
  return sd;
}


void free_sleep_data(SLEEP_DATA *sd)
{
  if (!IS_VALID(sd))
    return;
  
  INVALIDATE(sd);
  sd->next = sd_free;
  sd_free = sd;
}
