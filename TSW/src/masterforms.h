#ifndef MASTERFORMS_H_
#define MASTERFORMS_H_
#include "merc.h"

#define WEAPON_NONE -1
#define MASTERFORMS 8
#define MOVES_PER_FORM 12

struct masterform_type
{
	char * attackstring;
	char * extended_attackString;
	char * defensestring;
};

struct formlookup
{
	int weapontype;
	int formindex;
	sh_int gsn;	
	unsigned int weapon_vnum;
};

extern struct formlookup masterformslookup_table[];
extern const struct masterform_type masterforms_table[];

unsigned int get_mf_weapontype(sh_int gsn_mf);


#endif /*MASTERFORMS_H_*/
