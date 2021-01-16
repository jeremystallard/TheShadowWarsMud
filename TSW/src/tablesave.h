void save_skills( void );
void save_races( void );
void save_progs( int minvnum, int maxvnum );
void save_table_commands ( void );
void save_socials( void );
MPROG_CODE * pedir_prog( int vnum );
#if !defined(FIRST_BOOT)
   void load_commands(void);
   void load_races(void);
   void load_socials(void);
   void load_skills(void);
#endif
