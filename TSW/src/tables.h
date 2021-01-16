/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
 ***************************************************************************/
 
/***************************************************************************
*	ROM 2.4 is copyright 1993-1998 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Russ Taylor (rtaylor@hypercube.org)				   *
*	    Gabrielle Taylor (gtaylor@hypercube.org)			   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#if !defined(_TABLES_H)
#define _TABLES_H


struct flag_type
{
    char *name;
    int bit;
    bool settable;
};

struct background_type
{
  char   *name;
  int    bit;
  int    cost;
  int    class;
  char   *race;
  char   *nrace;
  bool   settable;
};

struct position_type
{
    char *name;
    char *short_name;
};

struct sex_type
{
    char *name;
};

struct size_type
{
    char *name;
};

struct	bit_type
{
	const	struct	flag_type *	table;
	char *				help;
};

struct recval_type
{
	int numhit;
	int typhit;
	int bonhit;
	int ac;
	int numdam;
	int typdam;
	int bondam;
};
/* game tables */
/* extern	const	struct	clan_type	clan_table[MAX_CLAN]; */
extern	const	struct	position_type	position_table[];
extern	const	struct	sex_type	sex_table[];
extern	const	struct	size_type	size_table[];

/* flag tables */
extern	const	struct	flag_type	act_flags[];
extern	const	struct	flag_type	guild_guard_flags[];
extern	const	struct	flag_type	gain_flags[];
extern	const	struct	flag_type	train_flags[];
extern	const	struct	flag_type	plr_flags[];
extern	const	struct	flag_type	affect_flags[];
extern	const	struct	flag_type	off_flags[];
extern	const	struct	flag_type	imm_flags[];
extern	const	struct	flag_type	form_flags[];
extern	const	struct	flag_type	part_flags[];
extern	const	struct	flag_type	comm_flags[];
extern	const	struct	flag_type	extra_flags[];
extern	const	struct	flag_type	wear_flags[];
extern	const	struct	flag_type	weapon_flags[];
extern	const	struct	flag_type	container_flags[];
extern	const	struct	flag_type	portal_flags[];
extern	const	struct	flag_type	room_flags[];
extern	const	struct	flag_type	exit_flags[];
extern 	const	struct  flag_type	mprog_flags[];
extern	const	struct	flag_type	area_flags[];
extern	const	struct	flag_type	sector_flags[];
extern	const	struct	flag_type	door_resets[];
extern	const	struct	flag_type	wear_loc_strings[];
extern	const	struct	flag_type	wear_loc_flags[];
extern	const	struct	flag_type	res_flags[];
extern	const	struct	flag_type	imm_flags[];
extern	const	struct	flag_type	vuln_flags[];
extern	const	struct	flag_type	type_flags[];
extern	const	struct	flag_type	apply_flags[];
extern	const	struct	flag_type	sex_flags[];
extern	const	struct	flag_type	furniture_flags[];
extern	const	struct	flag_type	weapon_class[];
extern	const	struct	flag_type	apply_types[];
extern	const	struct	flag_type	weapon_type2[];
extern	const	struct	flag_type	apply_types[];
extern	const	struct	flag_type	size_flags[];
extern	const	struct	flag_type	position_flags[];
extern	const	struct	flag_type	ac_type[];
extern	const	struct	bit_type	bitvector_type[];
extern    const     struct    flag_type	guild_flags[];
extern    const     struct    flag_type	sguild_flags[];
extern    const     struct    flag_type	ssguild_flags[];
extern	const	struct	recval_type	recval_table[];
extern	const	struct	flag_type	target_table[];
extern	const	struct	flag_type	restriction_table[];
extern	const	struct	flag_type	dam_classes[];
extern	const	struct	flag_type	log_flags[];
extern	const	struct	flag_type	show_flags[];
extern	const	struct	flag_type	stat_table[];
extern    const     struct    flag_type world_table[];
extern	const	struct	flag_type	ore_types[];
extern	const	struct	flag_type	gem_types[];
extern	const	struct	flag_type	mining_quality[];


/* background char info */
extern    const     struct    background_type merit_table[];
extern    const     struct    background_type flaw_table[];
extern    const     struct    background_type talent_table[];

/* hit locations */
extern    const     struct    flag_type hit_flags[];
extern    const     struct    flag_type short_hit_flags[];

/* IC */
extern    const     struct    flag_type ic_flags[];
#endif // _TABLES_H
