#include "include.h"
#include "tables.h"UC
#include "lookup.h"
#include "olc.h"
#include "recycle.h"
#include "interp.h"

#define SOCIAL_FILE	DATA_DIR "socials"
#define SKILL_FILE DATA_DIR "skills"

struct savetable_type
{
	char *	field;
	sh_int	type_field;
	void *	leader_field;
	const void *	argument;
	const void *	argument2;
};

struct cmd_type		cmd;
struct race_type	race;
int MAX_SKILL;
#if defined(FIRST_BOOT)
struct pc_race_type	pcrace;
#endif
struct social_type	soc;
struct skill_type	sk;
MPROG_CODE		pcode;

char * cmd_func_name( DO_FUN * );
DO_FUN * cmd_func_lookup( char * );

char * gsn_name( sh_int * );
sh_int * gsn_lookup( char * );

char * spell_name( SPELL_FUN * );
SPELL_FUN * spell_function( char * );

typedef char * STR_FUNC ( void * );
typedef bool   STR_READ_FUNC ( void *, char * );

char * do_fun_str( void * temp )
{
	DO_FUN ** fun = (DO_FUN **) temp;

	return cmd_func_name( *fun );
}

char * position_str( void * temp )
{
	sh_int *flags = (sh_int *) temp;

	return position_table[*flags].name;
}

char * size_str( void * temp )
{
	sh_int *size = (sh_int *) temp;

	return size_table[UMAX(0, *size)].name;
}

char * race_str( void * temp )
{
	sh_int *race = (sh_int *) temp;

	return race_table[*race].name;
}

char * clan_str( void * temp )
{
	sh_int *klan = (sh_int *) temp;

	return clan_table[*klan].name;
}

char * class_str( void * temp )
{
	sh_int *class = (sh_int *) temp;

	return class_table[*class].name;
}

char * pgsn_str( void * temp )
{
	sh_int **pgsn = (sh_int **) temp;

	return gsn_name(*pgsn);
}

char * spell_fun_str( void * temp )
{
	SPELL_FUN ** spfun = (SPELL_FUN **) temp;

	return spell_name(*spfun);
}

bool race_read( void * temp, char * arg )
{
	sh_int * race = (sh_int *) temp;

	*race = race_lookup(arg);

	return (*race == 0) ? FALSE : TRUE;
}

bool clan_read( void * temp, char * arg )
{
	sh_int * klan = (sh_int *) temp;

	*klan = clan_lookup(arg);

	return TRUE;
}

bool class_read( void * temp, char * arg )
{
	sh_int * class = (sh_int *) temp;

	*class = class_lookup(arg);

	if ( *class == -1 )
	{
		*class = 0;
		return FALSE;
	}

	return TRUE;
}

bool do_fun_read( void * temp, char * arg )
{
	DO_FUN ** fun = (DO_FUN **) temp;

	*fun = cmd_func_lookup(arg);

	return TRUE;
}

bool position_read( void * temp, char * arg )
{
	sh_int * posic = (sh_int *) temp;
	sh_int ffg = position_lookup(arg);

	*posic = UMAX(0, ffg);

	if ( ffg == -1 )
		return FALSE;
	else
		return TRUE;
}

bool size_read( void * temp, char * arg )
{
	sh_int * size = (sh_int *) temp;
	int ffg = size_lookup(arg);

	*size = UMAX(0, ffg);

	if ( ffg == -1 )
		return FALSE;
	else
		return TRUE;
}

bool pgsn_read( void * temp, char * arg )
{
	sh_int ** pgsn = (sh_int **) temp;
	sh_int * blah = gsn_lookup(arg);

	*pgsn = blah;

	return !str_cmp(arg, "") || blah != NULL;
}

bool spell_fun_read( void * temp, char * arg )
{
	SPELL_FUN ** spfun = (SPELL_FUN **) temp;
	SPELL_FUN * blah = spell_function(arg);

	*spfun = blah;

	return !str_cmp(arg, "") || blah != NULL;
}

#define FIELD_STRING                0
#define FIELD_FUNCION_INT_TO_STR    1
#define FIELD_SHINT                 2
#define FIELD_FLAGSTRING            3
#define FIELD_INT                   4
#define FIELD_FLAGVECTOR            5
#define FIELD_BOOL                  6
#define FIELD_SHINT_ARRAY           7
#define FIELD_STRING_ARRAY          8
#define FIELD_FUNCION_SHINT_TO_STR	9
#define FIELD_SHINT_FLAGSTRING     10
#define FIELD_BOOL_ARRAY           11
#define FIELD_INUTIL               12
#define FIELD_INT_BACKGROUNDSTRING 13

const struct savetable_type progcodesavetable [] =
{
	{	"vnum",		FIELD_SHINT,				(void *) &pcode.vnum,	NULL,	NULL	},
	{	"code",		FIELD_STRING,			(void *) &pcode.code,	NULL,	NULL	},
	{	NULL,		0,				NULL,			NULL,	NULL	}
};

const struct savetable_type skillsavetable [] =
{
	{	"name",		FIELD_STRING,			(void *) &sk.name,		NULL,			NULL	},
	{  "restriction", FIELD_SHINT_FLAGSTRING,  (void *) &sk.restriction,  restriction_table, NULL },
	{  "talent", FIELD_INT_BACKGROUNDSTRING,  (void *) &sk.talent_req,  talent_table, NULL },
	{	"skill_level",	FIELD_SHINT_ARRAY,		(void *) &sk.skill_level,	(void *) MAX_CLASS,	NULL	},
	{	"rating",	FIELD_SHINT_ARRAY,		(void *) &sk.rating,		(void *) MAX_CLASS,	NULL	},
	{	"spell_fun",	FIELD_FUNCION_INT_TO_STR,	(void *) &sk.spell_fun,		spell_fun_str,		spell_fun_read	},
	{	"target",	FIELD_SHINT_FLAGSTRING,		(void *) &sk.target,		target_table,		NULL	},
	{	"minimum_position", FIELD_FUNCION_SHINT_TO_STR,	(void *) &sk.minimum_position,	position_str,		position_read	},
	{	"pgsn",		FIELD_FUNCION_INT_TO_STR,	(void *) &sk.pgsn,		pgsn_str,		pgsn_read	},
	{	"slot",		FIELD_SHINT,			(void *) &sk.slot,		NULL,			NULL	},
	{    "min_spheres",      FIELD_SHINT_ARRAY,   (void *) &sk.spheres, (void *) MAX_SPHERE, NULL },
	{	"min_endurance",	FIELD_SHINT,			(void *) &sk.min_endurance,		NULL,			NULL	},
	{	"beats",	FIELD_SHINT,			(void *) &sk.beats,		NULL,			NULL	},
	{	"noun_damage",	FIELD_STRING,			(void *) &sk.noun_damage,	NULL,			NULL	},
	{	"msg_off",	FIELD_STRING,			(void *) &sk.msg_off,		NULL,			NULL	},
	{	"msg_obj",	FIELD_STRING,			(void *) &sk.msg_obj,		NULL,			NULL	},
	{	NULL,		0,				NULL,				NULL,			NULL	}
};

const struct savetable_type socialsavetable [] =
{
	{	"name",		FIELD_STRING,	(void *) &soc.name,		NULL,	NULL	},
	{	"char_no_arg",	FIELD_STRING,	(void *) &soc.char_no_arg, 	NULL,	NULL	},
	{	"others_no_arg",FIELD_STRING,	(void *) &soc.others_no_arg,	NULL,	NULL	},
	{	"char_found",	FIELD_STRING,	(void *) &soc.char_found,	NULL,	NULL	},
	{	"others_found",	FIELD_STRING,	(void *) &soc.others_found,	NULL,	NULL	},
	{	"vict_found",	FIELD_STRING,	(void *) &soc.vict_found,	NULL,	NULL	},
	{	"char_auto",	FIELD_STRING,	(void *) &soc.char_auto,	NULL,	NULL	},
	{	"others_auto",	FIELD_STRING,	(void *) &soc.others_auto,	NULL,	NULL	},
	{	NULL,		0,		NULL,				NULL,	NULL	}
};

const struct savetable_type racesavetable [] =
{
	{	"name",		FIELD_STRING,			(void *) &race.name,	NULL,		NULL		},
	{	"pc",		FIELD_BOOL,			(void *) &race.pc_race,	NULL,		NULL		},
	{	"granted",	FIELD_BOOL,			(void *) &race.granted,	NULL,		NULL		},
	{	"act",		FIELD_FLAGVECTOR,		(void *) &race.act,	NULL,		NULL		},
	{	"aff",		FIELD_FLAGVECTOR,		(void *) &race.aff,	NULL,		NULL		},
	{	"off",		FIELD_FLAGVECTOR,		(void *) &race.off,	NULL,		NULL		},
	{	"imm",		FIELD_FLAGVECTOR,		(void *) &race.imm,	NULL,		NULL		},
	{	"res",		FIELD_FLAGVECTOR,		(void *) &race.res,	NULL,		NULL		},
	{	"vuln",		FIELD_FLAGVECTOR,		(void *) &race.vuln,	NULL,		NULL		},
	{	"form",		FIELD_FLAGVECTOR,		(void *) &race.form,	NULL,		NULL		},
	{	"parts",	FIELD_FLAGVECTOR,		(void *) &race.parts,	NULL,		NULL		},
#if !defined(FIRST_BOOT)
	{	"points",	FIELD_SHINT,		(void *) &race.points,		NULL,			NULL	},

	{	"sphere_points",	FIELD_SHINT,		(void *) &race.sphere_points,		NULL,			NULL	},
	{	"stat_points",	FIELD_SHINT,		(void *) &race.stat_points,		NULL,			NULL	},
	{	"misc_points",	FIELD_SHINT,		(void *) &race.misc_points,		NULL,			NULL	},
	{	"sphere",	FIELD_SHINT_ARRAY,	(void *) &race.sphere,		(void *) MAX_SPHERE,	NULL	},

	{	"class_mult",	FIELD_SHINT_ARRAY,	(void *) &race.class_mult,	(void *) MAX_CLASS,	NULL	},
	{	"who_name",	FIELD_STRING,		(void *) &race.who_name,	NULL,			NULL	},
	{	"skills",	FIELD_STRING_ARRAY,	(void *) &race.skills,		(void *) 5,		NULL	},
	{	"stats",	FIELD_SHINT_ARRAY,	(void *) &race.stats,		(void *) MAX_STATS,	NULL	},
	{	"max_stats",	FIELD_SHINT_ARRAY,	(void *) &race.max_stats,	(void *) MAX_STATS,	NULL	},
	{	"size",		FIELD_FUNCION_SHINT_TO_STR,(void *) &race.size,		size_str,		size_read},
#endif
	{	NULL,		0,				NULL,			NULL,		NULL		}
};

#if defined(FIRST_BOOT)
const struct savetable_type pcracesavetable [] =
{
	{	"points",	FIELD_SHINT,		(void *) &pcrace.points,	NULL,			NULL	},
	{	"class_mult",	FIELD_SHINT_ARRAY,	(void *) &pcrace.class_mult,	(void *) MAX_CLASS,	NULL	},
	{	"who_name",	FIELD_STRING,		(void *) &pcrace.who_name,	NULL,			NULL	},
	{	"skills",	FIELD_STRING_ARRAY,	(void *) &pcrace.skills,	(void *) 5,		NULL	},
	{	"stats",	FIELD_SHINT_ARRAY,	(void *) &pcrace.stats,		(void *) MAX_STATS,	NULL	},
	{	"max_stats",	FIELD_SHINT_ARRAY,	(void *) &pcrace.max_stats,	(void *) MAX_STATS,	NULL	},
	{	"size",		FIELD_FUNCION_SHINT_TO_STR,(void *) &pcrace.size,	size_str,		size_read},
	{	NULL,		0,			NULL,				NULL,			NULL	}
};
#endif

const struct savetable_type cmdsavetable [] =
{
	{	"name",		FIELD_STRING,			(void *) &cmd.name,	NULL,		NULL		},
	{	"do_fun",	FIELD_FUNCION_INT_TO_STR,	(void *) &cmd.do_fun,	do_fun_str,	do_fun_read	},
	{	"position",	FIELD_FUNCION_SHINT_TO_STR,	(void *) &cmd.position,	position_str,	position_read	},
	{	"level",	FIELD_SHINT,			(void *) &cmd.level,	NULL,		NULL		},
	{	"log",		FIELD_SHINT_FLAGSTRING,		(void *) &cmd.log,	log_flags,	NULL		},
	{	"show",		FIELD_SHINT_FLAGSTRING,		(void *) &cmd.show,	show_flags,	NULL		},
	{	NULL,		0,				NULL,			NULL,		NULL		}
};

void load_struct( FILE *fp, void * typebase, const struct savetable_type * table, void * leader )
{
	char * word;
	const struct savetable_type * temp;
	sh_int * pshint;
	int * pentero;
	char ** pcadena;
	char * cadena;
	int * pint;
	STR_READ_FUNC * function;
	struct flag_type * flagtable;
	struct background_type * backgroundtable;
	bool found = FALSE;
	bool * pbool;
	int cnt = 0, i;

	while ( str_cmp((word = fread_word(fp)), "#END") )
	{
		for ( temp = table; !IS_NULLSTR(temp->field); temp++ )
		{
			if ( !str_cmp( word, temp->field ) )
			{
				// lo encontramos!
				switch(temp->type_field)
				{
					case FIELD_STRING:
					pcadena = (char **) ((int) temp->leader_field - (int) typebase + (int) leader);
					*pcadena = fread_string(fp);
					found = TRUE, cnt++;
					break;

					case FIELD_SHINT:
					pshint = (sh_int *) ((int) temp->leader_field - (int) typebase + (int) leader);
					*pshint = (sh_int) fread_number(fp);
					found = TRUE, cnt++;
					break;

					case FIELD_INT:
					pint = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
					*pint = fread_number(fp);
					found = TRUE, cnt++;
					break;

					case FIELD_FUNCION_INT_TO_STR:
					function = temp->argument2;
					cadena = fread_string(fp);
					pint = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
					if ((*function) (pint, cadena) == FALSE)
						bugf( "load_struct : field %s invalido, cadena %s",
							temp->field, cadena );
					free_string(cadena);
					found = TRUE, cnt++;
					break;

					case FIELD_FUNCION_SHINT_TO_STR:
					function = temp->argument2;
					cadena = fread_string(fp);
					pshint = (sh_int *) ((int) temp->leader_field - (int) typebase + (int) leader);
					if ((*function) (pshint, cadena) == FALSE)
						bugf( "load_struct : field %s invalid, cadena %s",
							temp->field, cadena );
					free_string(cadena);
					found = TRUE, cnt++;
					break;

					case FIELD_FLAGSTRING:
					flagtable = (struct flag_type *) temp->argument;
					pentero = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
					cadena = fread_string(fp);
					if ( (*pentero = flag_value(flagtable, cadena)) == NO_FLAG )
						*pentero = 0;
					free_string(cadena);
					found = TRUE, cnt++;
					break;

					case FIELD_SHINT_FLAGSTRING:
					flagtable = (struct flag_type *) temp->argument;
					pshint = (sh_int *) ((int) temp->leader_field - (int) typebase + (int) leader);
					cadena = fread_string(fp);
					if ( (*pshint = flag_value(flagtable, cadena)) == NO_FLAG )
						*pshint = 0;
					free_string(cadena);
					found = TRUE, cnt++;
					break;
					
					case FIELD_INT_BACKGROUNDSTRING:
					backgroundtable = (struct background_type *) temp->argument;
					pint = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
					cadena = fread_string(fp);
					if ( (*pint = background_flag_value(backgroundtable, cadena)) == NO_FLAG )
						*pint = 0;
					free_string(cadena);
					found = TRUE, cnt++;
					break;					

					case FIELD_FLAGVECTOR:
					pentero = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
					*pentero = fread_flag(fp);
					found = TRUE, cnt++;
					break;

					case FIELD_BOOL:
					pbool = (bool *) ((int) temp->leader_field - (int) typebase + (int) leader);
					cadena = fread_word(fp);
					*pbool = str_cmp( cadena, "false" ) ? TRUE : FALSE;
					found = TRUE, cnt++;
					break;

					case FIELD_SHINT_ARRAY:
					pshint = (sh_int *) ((int) temp->leader_field - (int) typebase + (int) leader);
					i = 0;
					while( str_cmp((cadena = fread_word(fp)), "@") )
					{
						if ( i == (int) temp->argument )
							bugf( "load_struct : field_shint_array %s con exceso of elementos",
								temp->field );
						else
							pshint[i++] = (sh_int) atoi(cadena);
					}
					found = TRUE, cnt++;
					break;

					case FIELD_STRING_ARRAY:
					pcadena = (char **) ((int) temp->leader_field - (int) typebase + (int) leader);
					i = 0;
					while( str_cmp((cadena = fread_string(fp)), "@") )
					{
						if ( i == (int) temp->argument )
							bugf( "load_struct : field_string_array %s con exceso of elementos",
								temp->field);
						else
							pcadena[i++] = cadena;
					}
					found = TRUE, cnt++;
					break;

					case FIELD_INUTIL:
					fread_to_eol(fp);
					found = TRUE, cnt++;
					break;

					case FIELD_BOOL_ARRAY:
					pbool = (bool *) ((int) temp->leader_field - (int) typebase + (int) leader);
					i = 0;
					while( str_cmp((cadena = fread_word(fp)), "@") )
					{
						if ( (temp->argument != NULL
						  && i == (int) temp->argument)
						||   (temp->argument == NULL
						  && temp->argument2 != NULL
						  && i == *( (int *) temp->argument2)) )
							bugf( "load_struct : field_bool_array %s con exceso of elementos",
								temp->field );
						else
							pbool[i++] = (bool) atoi(cadena);
					}
					found = TRUE, cnt++;
					break;
				} // switch
				if (found == TRUE)
					break;
			} // if
		} // for

		if (found == FALSE)
		{
			bugf( "load_struct : key %s no encontrada", word );
			fread_to_eol(fp);
		}
		else
			found = FALSE;
	} // while
}

void save_struct( FILE *fp, void * typebase, const struct savetable_type * table, void * leader )
{
	const struct savetable_type * temp;
	char ** pcadena;
	sh_int * pshint;
	int * pint;
	STR_FUNC * function;
	char * cadena;
	int * pentero;
	bool * pbool;
	const struct flag_type * flagtable;
	const struct background_type * backgroundtable;
	int cnt = 0, i;

	for ( temp = table; !IS_NULLSTR(temp->field); temp++ )
	{
		switch(temp->type_field)
		{
			default:
			bugf( "save_struct : type_field %d invalido, field %s",
				temp->type_field, temp->field );
			break;

			case FIELD_STRING:
			pcadena = (char **) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s %s~\n", temp->field, !IS_NULLSTR(*pcadena) ? *pcadena : "" );
			break;

			case FIELD_SHINT:
			pshint = (sh_int *) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s %d\n", temp->field, *pshint );
			break;

			case FIELD_INT:
			pentero = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s %d\n", temp->field, *pentero );
			break;

			case FIELD_FUNCION_INT_TO_STR:
			function = temp->argument;
			pentero = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
			cadena = (*function) ((void *) pentero);
			fprintf( fp, "%s %s~\n", temp->field, cadena );
			break;

			case FIELD_FUNCION_SHINT_TO_STR:
			function = temp->argument;
			pshint = (sh_int *) ((int) temp->leader_field - (int) typebase + (int) leader);
			cadena = (*function) ((void *) pshint);
			fprintf( fp, "%s %s~\n", temp->field, cadena );
			break;

			case FIELD_FLAGSTRING:
			flagtable = (struct flag_type *) temp->argument;
			pentero = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s %s~\n", temp->field, flag_string(flagtable, *pentero) );
			break;

			case FIELD_SHINT_FLAGSTRING:
			flagtable = (struct flag_type *) temp->argument;
			pshint = (sh_int *) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s %s~\n", temp->field, flag_string(flagtable, *pshint) );
			break;

			case FIELD_INT_BACKGROUNDSTRING:
			backgroundtable = (struct background_type *) temp->argument;
			pint = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s %s~\n", temp->field, background_flag_string(backgroundtable, *pint) );
			break;
			
			case FIELD_FLAGVECTOR:
			pentero = (int *) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s %s\n", temp->field, print_flags(*pentero) );
			break;

			case FIELD_BOOL:
			pbool = (bool *) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s %s\n", temp->field,
						(*pbool == TRUE) ? "true" : "false" );
			break;

			case FIELD_SHINT_ARRAY:
			pshint = (sh_int *) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s ", temp->field );
			for ( i = 0; i < (int) temp->argument; i++ )
				fprintf( fp, "%d ", pshint[i] );
			fprintf( fp, "@\n" );
			break;

			case FIELD_STRING_ARRAY:
			pcadena = (char **) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s ", temp->field );
			for ( i = 0; i < (int) temp->argument; i++ )
				fprintf( fp, "%s~ ", !IS_NULLSTR(pcadena[i]) ? pcadena[i] : "" );
			fprintf( fp, "@~\n" );
			break;

			case FIELD_BOOL_ARRAY:
			pbool = (bool *) ((int) temp->leader_field - (int) typebase + (int) leader);
			fprintf( fp, "%s ", temp->field );
			for ( i = 0; i < (temp->argument ? (int) temp->argument : *(int *) temp->argument2); i++ )
				fprintf( fp, "%d ", pbool[i] == TRUE ? 1 : 0 );
			fprintf( fp, "@\n" );
			break;

			case FIELD_INUTIL:
			break;
		}

		cnt++;
	}
};

void save_table_commands( void )
{
	FILE * fp;
	struct cmd_type * temp;
#if !defined(FIRST_BOOT)
extern	struct cmd_type * cmd_table;
#endif
	int cnt = 0;

	fp = fopen( DATA_DIR "commands", "w" );

	if ( !fp )
	{
		perror( "save_table_commands" );
		return;
	}

	for ( temp = cmd_table; !IS_NULLSTR(temp->name); temp++ )
		cnt++;

	fprintf( fp, "%d\n\n", cnt );

	for ( temp = cmd_table; !IS_NULLSTR(temp->name); temp++ )
	{
		fprintf( fp, "#COMMAND\n" );
		save_struct( fp, &cmd, cmdsavetable, temp );
		fprintf( fp, "#END\n\n" );
	}

	fclose(fp);
}

#if !defined(FIRST_BOOT)
void load_commands( void )
{
	FILE * fp;
extern	struct cmd_type *	cmd_table;
extern	int			MAX_CMD;
static	struct cmd_type		emptycmd;
	int i = 0, length;
	char * word;

	fp = fopen( DATA_DIR "commands", "r" );

	if ( fp == NULL )
	{
		perror( "load_table_commands ");
		return;
	}

	length = fread_number(fp);

	MAX_CMD = length;

	flog( "Creating cmd_table of length %d, tama~no %d", length + 1,
		sizeof(struct cmd_type) * (length + 1) );
	cmd_table = calloc( sizeof(struct cmd_type), length + 1);

	for ( i = 0; i <= length; i++ )
		cmd_table[i] = emptycmd; // limpiar

	i = 0;

	while(TRUE)
	{
		word = fread_word(fp);

		if ( str_cmp(word, "#COMMAND") )
		{
			bugf( "load_table_commands : word %s", word );
			fclose(fp);
			return;
		}

		load_struct( fp, &cmd, cmdsavetable, &cmd_table[i++] );

		if ( i == length )
		{
			flog( "Table of commands loaded." );
			fclose(fp);
			cmd_table[i].name = str_dup( "" );
			return;
		}
	}
}

void load_races( void )
{
	FILE * fp;
	extern int maxrace;
static	struct race_type cRace;
	char * word;
	int i;

	fp = fopen( DATA_DIR "races", "r" );

	if ( !fp )
	{
		perror( "load_table_races" );
		return;
	}

	maxrace = fread_number(fp);

	flog( "Creating race_table of length %d, tama~no %d", maxrace + 1,
		sizeof(struct race_type) * (maxrace + 1) );

	race_table = calloc( sizeof(struct race_type), maxrace + 1 );

	// limpiar races
	for ( i = 0; i <= maxrace; i++ )
	{
		race_table[i] = cRace;
		race_table[i].race_id = i;
	}

	i = 0;

	while(TRUE)
	{
		word = fread_word(fp);

		if ( str_cmp(word, "#RACE") )
		{
			bugf( "load_table_races : word %s", word );
			fclose(fp);
			return;
		}

		load_struct( fp, &race, racesavetable, &race_table[i++] );

		if ( i == maxrace )
		{
			flog( "Table of races loaded." );
			fclose(fp);
			race_table[i].name = NULL;
			return;
		}
	}
}

void load_socials (void)
{
	FILE *fp;
	int i;
	extern int maxSocial;
	char * key;
	
	fp = fopen (SOCIAL_FILE, "r");

	if (!fp)
	{
		perror(SOCIAL_FILE);
		exit(1);
	}

	fscanf (fp, "%d\n", &maxSocial);

	flog( "creating social_table of length %d, tama~no %d", maxSocial + 1,
		sizeof(struct social_type) * (maxSocial + 1) );
	/* IMPORTANT to use malloc so we can realloc later on */
	social_table = malloc (sizeof(struct social_type) * (maxSocial+1));
	
	for (i = 0; i < maxSocial; i++)
	{
		if ( str_cmp((key = fread_word(fp)), "#SOCIAL") )
		{
			bugf( "load_socials : key %s non-existant",
				key );
			exit(1);
		}
		load_struct (fp, &soc, socialsavetable, &social_table[i]);
	}

	/* For backwards compatibility */
	social_table[maxSocial].name = str_dup(""); /* empty! */		

	fclose (fp);

	flog( "Table of socials loaded." );
}

void	load_skills( void )
{
	FILE *fp;
static	struct skill_type skzero;
	int i = 0;
	char *word;

	fp = fopen( SKILL_FILE, "r" );

	if ( !fp )
	{
		bug( "Can not find " SKILL_FILE " for loading skills.", 0 );
		exit(1);
	}

	fscanf( fp, "%d\n", &MAX_SKILL );

	flog( "Creating table of skills of length %d, tama~no %d",
		MAX_SKILL + 1, sizeof(struct skill_type) * (MAX_SKILL + 1) );
	skill_table = malloc( sizeof(struct skill_type) * (MAX_SKILL + 1) );

	if ( !skill_table )
	{
		bug( "Error! Skill_table == NULL, MAX_SKILL : %d", MAX_SKILL );
		exit(1);
	}

	for ( ; ; )
	{
		word = fread_word( fp );

		if ( !str_cmp( word, "#!" ) )
			break;

		if ( str_cmp( word, "#SKILL" ) )
		{
			bugf( "Load_skills : key non-existant (%s)", word );
			exit(1);
		}

		if ( i >= MAX_SKILL )
		{
			bug( "Load_skills : Number of skills greater thanMAX_SKILL", 0 );
			exit(1);
		}

		skill_table[i] = skzero;
		load_struct( fp, &sk, skillsavetable, &skill_table[i++] );
	}

	skill_table[MAX_SKILL].name = NULL;

	fclose(fp);
}
#endif

void save_races( void )
{
	FILE * fp;
	struct race_type * temp;
#if !defined(FIRST_BOOT)
extern	struct race_type * race_table;
#endif
	int cnt = 0;

	fp = fopen( DATA_DIR "tempraces", "w" );

	if ( !fp )
	{
		perror( "save_races : fopen" );
		return;
	}

	for ( temp = race_table; !IS_NULLSTR(temp->name); temp = temp++ )
		cnt++;

	fprintf( fp, "%d\n\n", cnt );

	for ( temp = race_table, cnt = 0; !IS_NULLSTR(temp->name); temp = temp++ )
	{
		fprintf( fp, "#RACE\n" );
		save_struct( fp, &race, racesavetable, temp );
#if defined(FIRST_BOOT)
		if (cnt < MAX_PC_RACE)
			save_struct( fp, &pcrace, pcracesavetable, &pc_race_table[cnt]);
		cnt++;
#endif
		fprintf( fp, "#END\n\n" );
	}

	fclose(fp);

	if (rename(DATA_DIR "tempraces", DATA_DIR "races") == -1)
		perror("save_races : rename");
}

void save_socials(void)
{
	FILE *fp;
	int i;
	extern int maxSocial;
	
	fp = fopen (SOCIAL_FILE, "w");
	
	if (!fp)
	{
		perror( SOCIAL_FILE );
		return;
	}

	fprintf (fp, "%d\n", maxSocial);

	for ( i = 0 ; i < maxSocial ; i++)
	{
		fprintf( fp, "#SOCIAL\n" );
		save_struct( fp, &soc, socialsavetable, &social_table[i] );
		fprintf( fp, "#END\n\n" );
	}

	fclose (fp);
}

void save_skills( void )
{
	FILE *fpn;
	int i;

	fpn = fopen( SKILL_FILE, "w" );

	if ( !fpn )
	{
		bug( "Save_skills : NULL fpn", 0 );
		fclose( fpn );
		return;
	}

	fprintf( fpn, "%d\n", MAX_SKILL );

	for ( i = 0; i < MAX_SKILL; ++i )
	{
		fprintf( fpn, "#SKILL\n" );
		save_struct( fpn, &sk, skillsavetable, &skill_table[i] );
		fprintf( fpn, "#END\n\n" );
	}

	fprintf( fpn, "#!\n" );

	fclose( fpn );
}

void save_progs( int minvnum, int maxvnum )
{
	FILE * fp;
	MPROG_CODE * pMprog;
	char buf[64];

	for ( pMprog = mprog_list; pMprog; pMprog = pMprog->next )
		if ( pMprog->changed == TRUE
		&&   ENTRE_I(minvnum, pMprog->vnum, maxvnum) )
		{
			sprintf(buf, PROG_DIR "%d.prg", pMprog->vnum );
			fp = fopen( buf, "w" );

			if ( !fp )
			{
				perror(buf);
				return;
			}

			fprintf( fp, "#PROG\n" );
			save_struct( fp, &pcode, progcodesavetable, pMprog );
			fprintf( fp, "#END\n\n" );
			fclose(fp);

			pMprog->changed = FALSE;
		}
}

void load_prog( FILE * fp, MPROG_CODE ** prog )
{
extern	MPROG_CODE * mprog_list;
static	MPROG_CODE mprog_zero;
	char * word = fread_word(fp);

	if (str_cmp(word, "#PROG"))
	{
		bugf("load_prog : key %s invalid", word);
		*prog = NULL;
		return;
	}

	*prog = alloc_perm(sizeof(MPROG_CODE));
    //*prog = malloc(sizeof(struct mprog_code));

	// blanquear
	**prog = mprog_zero;

	MPROG_CODE *newCode = malloc(sizeof(MPROG_CODE));;
	memset(newCode,0,sizeof(MPROG_CODE));
	//load_struct( fp, &pcode, progcodesavetable, *prog );
	load_struct( fp, &pcode, progcodesavetable, newCode);
	memcpy(*prog,(void *)newCode,sizeof(MPROG_CODE));
	free(newCode);

	// a la lista
	if (mprog_list == NULL)
		mprog_list = *prog;
	else
	{
		// al comienzo o al final? VNUM decide
		if ((*prog)->vnum < mprog_list->vnum)
		{
			(*prog)->next	= mprog_list;
			mprog_list	= *prog;
		}
		else
		{
			MPROG_CODE * temp, * prev = mprog_list;

			for ( temp = mprog_list->next; temp; temp = temp->next )
			{
				if ( temp->vnum > (*prog)->vnum )
					break;
				prev = temp;
			}
			prev->next = *prog;
			(*prog)->next = temp;
		}
	}
}

MPROG_CODE * pedir_prog( int vnum )
{
	FILE * fp;
	MPROG_CODE * prog;
	char buf[128];
	extern bool fBootDb;

	prog = get_mprog_index(vnum);

	if (prog != NULL)
		return prog;

	sprintf(buf, PROG_DIR "%d.prg", vnum );

	fp = fopen(buf,"r");

	if ( !fp )
	{
		if ( fBootDb == TRUE )
			perror("pedir_prog");
		return NULL;
	}

	load_prog(fp, &prog);

	fclose(fp);

	return prog;
}

