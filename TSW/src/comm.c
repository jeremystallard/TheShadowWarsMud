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
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
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

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> /* OLC -- for close read write etc */
#include <stdarg.h> /* printf_to_char */

#include "merc.h"
#include "olc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "screen.h"

/*
 * Malloc debugging stuff.
 */
#if defined(sun)
#undef MALLOC_DEBUG
#endif

#if defined(MALLOC_DEBUG)
#include <malloc.h>
extern	int	malloc_debug	args( ( int  ) );
extern	int	malloc_verify	args( ( void ) );
#endif



/*
 * Signal handling.
 * Apollo has a problem with __attribute(atomic) in signal.h,
 *   I dance around it.
 */
#if defined(apollo)
#define __attribute(x)
#endif

#if defined(unix)
#include <signal.h>
#endif

#if defined(apollo)
#undef __attribute
#endif



/*
 * Socket and TCP/IP stuff.
 */
#if	defined(macintosh) || defined(MSDOS)
const	char	echo_off_str	[] = { '\0' };
const	char	echo_on_str	[] = { '\0' };
const	char 	go_ahead_str	[] = { '\0' };
#endif

#if	defined(unix)
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "telnet.h"
const	char	echo_off_str	[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const	char	echo_on_str	[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const	char 	go_ahead_str	[] = { IAC, GA, '\0' };
#endif



/*
 * OS-dependent declarations.
 */
#if	defined(_AIX)
#include <sys/select.h>
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
void	bzero		args( ( char *b, int length ) );
int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
int	listen		args( ( int s, int backlog ) );
int	setsockopt	args( ( int s, int level, int optname, void *optval,
			    int optlen ) );
int	socket		args( ( int domain, int type, int protocol ) );
#endif

#if	defined(apollo)
#include <unistd.h>
void	bzero		args( ( char *b, int length ) );
#endif

#if	defined(__hpux)
int	accept		args( ( int s, void *addr, int *addrlen ) );
int	bind		args( ( int s, const void *addr, int addrlen ) );
void	bzero		args( ( char *b, int length ) );
int	getpeername	args( ( int s, void *addr, int *addrlen ) );
int	getsockname	args( ( int s, void *name, int *addrlen ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
int	listen		args( ( int s, int backlog ) );
int	setsockopt	args( ( int s, int level, int optname,
 				const void *optval, int optlen ) );
int	socket		args( ( int domain, int type, int protocol ) );
#endif

#if	defined(interactive)
#include <net/errno.h>
#include <sys/fnctl.h>
#endif

#if	defined(linux)
/* 
    Linux shouldn't need these. If you have a problem compiling, try
    uncommenting these functions.
*/
/*
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );
int	listen		args( ( int s, int backlog ) );
*/

bool    g_plevelFlag;
bool    g_extraplevelFlag;
int	close		args( ( int fd ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
/* int	read		args( ( int fd, char *buf, int nbyte ) ); */
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );
int	socket		args( ( int domain, int type, int protocol ) );
/* int	write		args( ( int fd, char *buf, int nbyte ) ); */ /* read,write in unistd.h */
#endif

#if	defined(macintosh)
#include <console.h>
#include <fcntl.h>
#include <unix.h>
struct	timeval
{
	time_t	tv_sec;
	time_t	tv_usec;
};
#if	!defined(isascii)
#define	isascii(c)		( (c) < 0200 )
#endif
static	long			theKeys	[4];

int	gettimeofday		args( ( struct timeval *tp, void *tzp ) );
#endif

#if	defined(MIPS_OS)
extern	int		errno;
#endif

#if	defined(MSDOS)
int	gettimeofday	args( ( struct timeval *tp, void *tzp ) );
int	kbhit		args( ( void ) );
#endif

#if	defined(NeXT)
int	close		args( ( int fd ) );
int	fcntl		args( ( int fd, int cmd, int arg ) );
#if	!defined(htons)
u_short	htons		args( ( u_short hostshort ) );
#endif
#if	!defined(ntohl)
u_long	ntohl		args( ( u_long hostlong ) );
#endif
int	read		args( ( int fd, char *buf, int nbyte ) );
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );
int	write		args( ( int fd, char *buf, int nbyte ) );
#endif

#if	defined(sequent)
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
int	close		args( ( int fd ) );
int	fcntl		args( ( int fd, int cmd, int arg ) );
int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
#if	!defined(htons)
u_short	htons		args( ( u_short hostshort ) );
#endif
int	listen		args( ( int s, int backlog ) );
#if	!defined(ntohl)
u_long	ntohl		args( ( u_long hostlong ) );
#endif
int	read		args( ( int fd, char *buf, int nbyte ) );
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );
int	setsockopt	args( ( int s, int level, int optname, caddr_t optval,
			    int optlen ) );
int	socket		args( ( int domain, int type, int protocol ) );
int	write		args( ( int fd, char *buf, int nbyte ) );
#endif

/* This includes Solaris Sys V as well */
#if defined(sun)
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
void	bzero		args( ( char *b, int length ) );
int	close		args( ( int fd ) );
int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );
int	listen		args( ( int s, int backlog ) );
int	read		args( ( int fd, char *buf, int nbyte ) );
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );

#if !defined(__SVR4)
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );

#if defined(SYSV)
int setsockopt		args( ( int s, int level, int optname,
			    const char *optval, int optlen ) );
#else
int	setsockopt	args( ( int s, int level, int optname, void *optval,
			    int optlen ) );
#endif
#endif
int	socket		args( ( int domain, int type, int protocol ) );
int	write		args( ( int fd, char *buf, int nbyte ) );
#endif

#if defined(ultrix)
int	accept		args( ( int s, struct sockaddr *addr, int *addrlen ) );
int	bind		args( ( int s, struct sockaddr *name, int namelen ) );
void	bzero		args( ( char *b, int length ) );
int	close		args( ( int fd ) );
int	getpeername	args( ( int s, struct sockaddr *name, int *namelen ) );
int	getsockname	args( ( int s, struct sockaddr *name, int *namelen ) );
int	gettimeofday	args( ( struct timeval *tp, struct timezone *tzp ) );
int	listen		args( ( int s, int backlog ) );
int	read		args( ( int fd, char *buf, int nbyte ) );
int	select		args( ( int width, fd_set *readfds, fd_set *writefds,
			    fd_set *exceptfds, struct timeval *timeout ) );
int	setsockopt	args( ( int s, int level, int optname, void *optval,
			    int optlen ) );
int	socket		args( ( int domain, int type, int protocol ) );
int	write		args( ( int fd, char *buf, int nbyte ) );
#endif



/*
 * Global variables.
 */
MASQUERADE_TYPE masquerade;
DESCRIPTOR_DATA *   descriptor_list;	/* All open descriptors		*/
DESCRIPTOR_DATA *   d_next;		/* Next descriptor in loop	*/
FILE *		    fpReserve;		/* Reserved file handle		*/
bool		    god;		/* All new chars are gods!	*/
bool		    merc_down;		/* Shutdown			*/
bool		    wizlock=FALSE;	/* Game is wizlocked		*/
bool		    newlock=FALSE;	/* Game is newlocked		*/
char		    str_boot_time[MAX_INPUT_LENGTH];
time_t        boot_time;
time_t		    current_time;	/* time of this pulse */	
bool		    MOBtrigger = TRUE;  /* act() switch                 */
char                tmp_buff[MAX_STRING_LENGTH];
int 		    reward_multiplier;  /* Used for increasing a players experience gain */
time_t  	    reward_time;
 

/*
 * OS-dependent local functions.
 */
#if defined(macintosh) || defined(MSDOS)
void	game_loop_mac_msdos	args( ( void ) );
bool	read_from_descriptor	args( ( DESCRIPTOR_DATA *d ) );
bool	write_to_descriptor	args( ( int desc, char *txt, int length ) );
#endif

#if defined(unix)
void	game_loop_unix		args( ( int control ) );
int	init_socket		args( ( int port ) );
void	init_descriptor		args( ( int control ) );
bool	read_from_descriptor	args( ( DESCRIPTOR_DATA *d ) );
bool	write_to_descriptor	args( ( int desc, char *txt, int length ) );
#endif

void handle_seg(int i);

/*
 * Other local functions (OS-independent).
 */
bool	check_parse_name	args( ( char *name ) );
bool	check_reconnect		args( ( DESCRIPTOR_DATA *d, char *name,
				    bool fConn ) );
bool	check_playing		args( ( DESCRIPTOR_DATA *d, char *name ) );
int	main			args( ( int argc, char **argv ) );
void	nanny			args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	process_output		args( ( DESCRIPTOR_DATA *d, bool fPrompt ) );
void	read_from_buffer	args( ( DESCRIPTOR_DATA *d ) );
void	stop_idling		args( ( CHAR_DATA *ch ) );
void    bust_a_prompt           args( ( CHAR_DATA *ch ) );
int 	can_hear_act( CHAR_DATA * ch, CHAR_DATA * to, int type );

/* new creation */
void selection_menu(CHAR_DATA *ch);
void set_creation_record(CHAR_DATA *ch);
void stat_menu(CHAR_DATA *ch);
void set_stat(CHAR_DATA *ch,  char * argument);
void sphere_menu(CHAR_DATA *ch);
void set_sphere(CHAR_DATA *ch,  char * argument);
void merit_menu(CHAR_DATA *ch);
void add_merit(CHAR_DATA *ch,  char * argument);
void drop_merit(CHAR_DATA *ch,  char * argument);
void flaw_menu(CHAR_DATA *ch);
void add_flaw(CHAR_DATA *ch,  char * argument);
void drop_flaw(CHAR_DATA *ch,  char * argument);
void talent_menu(CHAR_DATA *ch);
void add_talent(CHAR_DATA *ch,  char * argument);
void drop_talent(CHAR_DATA *ch,  char * argument);
void dice_spheres args( (CHAR_DATA *ch) );
void gen_faceless_template args( (CHAR_DATA *ch) );
int get_creation_sphere_sum args( (CHAR_DATA *ch) );
int get_merit_costs(CHAR_DATA *ch);
int get_talent_costs(CHAR_DATA *ch);
int get_flaw_costs(CHAR_DATA *ch);
void init_signals   args( (void) );
void do_auto_shutdown args( (void) );

/* Needs to be global because of do_warmboot */
int port, control;

int main( int argc, char **argv )
{
    struct timeval now_time;
    bool fCopyOver = FALSE;
    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug( 2 );
#endif

    /*
     * Init time.
     */
 
    /* 
     * Init syslog
     */

    gettimeofday( &now_time, NULL );
    current_time 	= (time_t) now_time.tv_sec;
    boot_time       = current_time;
    strcpy( str_boot_time, ctime( &current_time ) );

    /*
     * Macintosh console initialization.
     */
#if defined(macintosh)
    console_options.nrows = 31;
    cshow( stdout );
    csetmode( C_RAW, stdin );
    cecho2file( "log file", 1, stderr );
#endif

    /*
     * Reserve one channel for our use.
     */
    if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
    {
	perror( NULL_FILE );
	exit( 1 );
    }

    /*
     * Get the port number.
     */
    port = 4000;
    if ( argc > 1 )
    {
	if ( !is_number( argv[1] ) )
	{
	    fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
	    exit( 1 );
	}
	else if ( ( port = atoi( argv[1] ) ) <= 1024 )
	{
	    fprintf( stderr, "Port number must be above 1024.\n" );
	    exit( 1 );
	}
	
	/* Are we recovering from a copyover? */
 	if (argv[2] && argv[2][0])
 	{
 		fCopyOver = TRUE;
 		control = atoi(argv[3]);
 	}
 	else
 		fCopyOver = FALSE;
	
    }

    /*
     * Run the game.
     */
    reward_multiplier = 0;
    reward_time = current_time;
#if defined(macintosh) || defined(MSDOS)
    boot_db();
    initmasterformslookup();    
    log_string( "TSW is ready to rock." );
    game_loop_mac_msdos( );
#endif

#if defined(unix)

	if (!fCopyOver)
	    control = init_socket( port );
	    
    init_signals(); /* For the use of the signal handler. -Ferric */
    boot_db();
    initmasterformslookup();
    sprintf( log_buf, "TSW is ready to rock on port %d.", port );
    log_string( log_buf );
    
    if (fCopyOver)
    	copyover_recover();
    
    game_loop_unix( control );
    close (control);
#endif

    /*
     * That's all, folks.
     */
    log_string( "Normal termination of game." );
    exit( 0 );
    return 0;
}


void init_signals()
{
  signal(SIGBUS,&handle_seg);
  signal(SIGTERM,&handle_seg);
  signal(SIGABRT,&handle_seg);
  signal(SIGSEGV,&handle_seg);
  signal( SIGSEGV, &handle_seg);
  signal( SIGPIPE, SIG_IGN );
}



#if defined(unix)
int init_socket( int port )
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;
    int x = 1;
    int fd;

    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
	perror( "Init_socket: socket" );
	exit( 1 );
    }

    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
    (char *) &x, sizeof(x) ) < 0 )
    {
	perror( "Init_socket: SO_REUSEADDR" );
	close(fd);
	exit( 1 );
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
	struct	linger	ld;

	ld.l_onoff  = 1;
	ld.l_linger = 1000;

	if ( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
	(char *) &ld, sizeof(ld) ) < 0 )
	{
	    perror( "Init_socket: SO_DONTLINGER" );
	    close(fd);
	    exit( 1 );
	}
    }
#endif

    sa		    = sa_zero;
    sa.sin_family   = AF_INET;
    sa.sin_port	    = htons( port );

    if ( bind( fd, (struct sockaddr *) &sa, sizeof(sa) ) < 0 )
    {
	perror("Init socket: bind" );
	close(fd);
	exit(1);
    }


    if ( listen( fd, 3 ) < 0 )
    {
	perror("Init socket: listen");
	close(fd);
	exit(1);
    }

    return fd;
}
#endif



#if defined(macintosh) || defined(MSDOS)
void game_loop_mac_msdos( void )
{
    struct timeval last_time;
    struct timeval now_time;
    static DESCRIPTOR_DATA dcon;

    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;

    /*
     * New_descriptor analogue.
     */
    dcon.descriptor	= 0;
    dcon.connected	= CON_GET_NAME;
    dcon.ipaddr		= str_dup( "127.0.0.1" );
    dcon.host		= str_dup( "localhost" );
    dcon.outsize	= 2000;
    dcon.outbuf		= alloc_mem( dcon.outsize );
    dcon.next		= descriptor_list;
    dcon.showstr_head	= NULL;
    dcon.showstr_point	= NULL;
    dcon.pEdit		= NULL;			/* OLC */
    dcon.pString	= NULL;			/* OLC */
    dcon.editor		= 0;			/* OLC */
    descriptor_list	= &dcon;

    /*
     * Send the greeting.
     */
    {
	extern char * help_greeting;
	if ( help_greeting[0] == '.' )
	    write_to_buffer( &dcon, help_greeting+1, 0 );
	else
	    write_to_buffer( &dcon, help_greeting  , 0 );
    }

    /* Main loop */
    while ( !merc_down )
    {
	DESCRIPTOR_DATA *d;

	/*
	 * Process input.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next	= d->next;
	    d->fcommand	= FALSE;

#if defined(MSDOS)
	    if ( kbhit( ) )
#endif
	    {
		if ( d->character != NULL )
		    d->character->timer = 0;
		if ( !read_from_descriptor( d ) )
		{
		    if ( d->character != NULL && d->connected == CON_PLAYING)
			save_char_obj( d->character, FALSE );
		    d->outtop	= 0;
		    close_socket( d );
		    continue;
		}
	    }

	    if (d->character != NULL && d->character->daze > 0)
		--d->character->daze;

	    if ( d->character != NULL && d->character->wait > 0 )
	    {
		--d->character->wait;
		continue;
	    }

	    read_from_buffer( d );
	    if ( d->incomm[0] != '\0' )
	    {
		d->fcommand	= TRUE;
		stop_idling( d->character );

	        /* OLC */
	        if ( d->showstr_point )
	            show_string( d, d->incomm );
	        else
	        if ( d->pString )
	            string_add( d->character, d->incomm );
	        else
	            switch ( d->connected )
	            {
	                case CON_PLAYING:
			    substitute_alias( d, d->incomm ); /* Not this one */
			    break;
	                default:
			    nanny( d, d->incomm );
			    break;
	            }

		d->incomm[0]	= '\0';
	    }
	}



	/*
	 * Autonomous game motion.
	 */
	update_handler( );



	/*
	 * Output.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next = d->next;

	    if ( ( d->fcommand || d->outtop > 0 ) )
	    {
		if ( !process_output( d, TRUE ) )
		{
		    if ( d->character != NULL && d->connected == CON_PLAYING)
			save_char_obj( d->character, FALSE );
		    d->outtop	= 0;
		    close_socket( d );
		}
	    }
	}



	/*
	 * Synchronize to a clock.
	 * Busy wait (blargh).
	 */
	now_time = last_time;
	for ( ; ; )
	{
	    int delta;

#if defined(MSDOS)
	    if ( kbhit( ) )
#endif
	    {
		if ( dcon.character != NULL )
		    dcon.character->timer = 0;
		if ( !read_from_descriptor( &dcon ) )
		{
		    if ( dcon.character != NULL && d->connected == CON_PLAYING)
			save_char_obj( d->character, FALSE );
		    dcon.outtop	= 0;
		    close_socket( &dcon );
		}
#if defined(MSDOS)
		break;
#endif
	    }

	    gettimeofday( &now_time, NULL );
	    delta = ( now_time.tv_sec  - last_time.tv_sec  ) * 1000 * 1000
		  + ( now_time.tv_usec - last_time.tv_usec );
	    if ( delta >= 1000000 / PULSE_PER_SECOND )
		break;
	}
	last_time    = now_time;
	current_time = (time_t) last_time.tv_sec;
	g_plevelFlag = false;
	g_extraplevelFlag = true;
    }

    return;
}
#endif



#if defined(unix)
void game_loop_unix( int control )
{
    static struct timeval null_time;
    struct timeval last_time;

    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;

    /* Main loop */
    while ( !merc_down )
    {
	fd_set in_set;
	fd_set out_set;
	fd_set exc_set;
	DESCRIPTOR_DATA *d;
	int maxdesc;

#if defined(MALLOC_DEBUG)
	if ( malloc_verify( ) != 1 )
	    abort( );
#endif

	/*
	 * Poll all active descriptors.
	 */
	if (port == 4000)
     		openlog("tswlog",0,LOG_PLAYER);
	else
     		openlog("tswlog",0,LOG_BUILDER);
	FD_ZERO( &in_set  );
	FD_ZERO( &out_set );
	FD_ZERO( &exc_set );
	FD_SET( control, &in_set );
	maxdesc	= control;
	for ( d = descriptor_list; d; d = d->next )
	{
	    maxdesc = UMAX( maxdesc, d->descriptor );
	    FD_SET( d->descriptor, &in_set  );
	    FD_SET( d->descriptor, &out_set );
	    FD_SET( d->descriptor, &exc_set );
	}

	if ( select( maxdesc+1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
	{
	    perror( "Game_loop: select: poll" );
	    exit( 1 );
	}

	/*
	 * New connection?
	 */
	if ( FD_ISSET( control, &in_set ) )
	    init_descriptor( control );

	/*
	 * Kick out the freaky folks.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next = d->next;   
	    if ( FD_ISSET( d->descriptor, &exc_set ) )
	    {
		FD_CLR( d->descriptor, &in_set  );
		FD_CLR( d->descriptor, &out_set );
		if ( d->character && d->connected == CON_PLAYING)
		    save_char_obj( d->character, FALSE );
		d->outtop	= 0;
		close_socket( d );
	    }
	}

	/*
	 * Process input.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next	= d->next;
	    d->fcommand	= FALSE;

	    if ( FD_ISSET( d->descriptor, &in_set ) )
	    {
		if ( d->character != NULL )
		    d->character->timer = 0;
		if ( !read_from_descriptor( d ) )
		{
		    FD_CLR( d->descriptor, &out_set );
		    if ( d->character != NULL && d->connected == CON_PLAYING)
			save_char_obj( d->character, FALSE );
		    d->outtop	= 0;
		    close_socket( d );
		    continue;
		}
	    }

	    if (d->character != NULL && d->character->daze > 0)
		--d->character->daze;

	    if ( d->character != NULL && d->character->wait > 0 )
	    {
		--d->character->wait;
		continue;
	    }

	    read_from_buffer( d );
	    if ( d->incomm[0] != '\0' )
	    {
		d->fcommand	= TRUE;
		stop_idling( d->character );

	/* OLC */
	if ( d->showstr_point )
	    show_string( d, d->incomm );
	else
	if ( d->pString )
	    string_add( d->character, d->incomm );
	else
	    switch ( d->connected )
	    {
	        case CON_PLAYING:
		    substitute_alias( d, d->incomm );
		    break;
	        default:
		    nanny( d, d->incomm );
		    break;
	    }

		d->incomm[0]	= '\0';
	    }
	}



	/*
	 * Autonomous game motion.
	 */
	update_handler( );



	/*
	 * Output.
	 */
	for ( d = descriptor_list; d != NULL; d = d_next )
	{
	    d_next = d->next;

	    if ( ( d->fcommand || d->outtop > 0 )
	    &&   FD_ISSET(d->descriptor, &out_set) )
	    {
		if ( !process_output( d, TRUE ) )
		{
		    if ( d->character != NULL && d->connected == CON_PLAYING)
			save_char_obj( d->character, FALSE );
		    d->outtop	= 0;
		    close_socket( d );
		}
	    }
	}



	/*
	 * Synchronize to a clock.
	 * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
	 * Careful here of signed versus unsigned arithmetic.
	 */
	{
	    struct timeval now_time;
	    long secDelta;
	    long usecDelta;

	    gettimeofday( &now_time, NULL );
	    usecDelta	= ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
			+ 1000000 / PULSE_PER_SECOND;
	    secDelta	= ((int) last_time.tv_sec ) - ((int) now_time.tv_sec );
	    while ( usecDelta < 0 )
	    {
		usecDelta += 1000000;
		secDelta  -= 1;
	    }

	    while ( usecDelta >= 1000000 )
	    {
		usecDelta -= 1000000;
		secDelta  += 1;
	    }

	    if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
	    {
		struct timeval stall_time;

		stall_time.tv_usec = usecDelta;
		stall_time.tv_sec  = secDelta;
		if ( select( 0, NULL, NULL, NULL, &stall_time ) < 0 )
		{
		    perror( "Game_loop: select: stall" );
		    exit( 1 );
		}
	    }
	}

	gettimeofday( &last_time, NULL );
	current_time = (time_t) last_time.tv_sec;
	struct tm *pTime = localtime(&current_time);
	if (pTime->tm_wday >= 5 || pTime->tm_wday == 0)
	{
		g_plevelFlag = TRUE;
	}
	else
	{
		g_plevelFlag = FALSE;
	}
	closelog();
    }

    return;
}
#endif



#if defined(unix)

void init_descriptor( int control )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
    struct hostent *from;
    int desc;
    int size;

    size = sizeof(sock);
    getsockname( control, (struct sockaddr *) &sock, &size );
    if ( ( desc = accept( control, (struct sockaddr *) &sock, &size) ) < 0 )
    {
	perror( "New_descriptor: accept" );
	return;
    }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
    {
	perror( "New_descriptor: fcntl: FNDELAY" );
	return;
    }

    /*
     * Cons a new descriptor.
     */
     dnew = new_descriptor(); /* new_descriptor now also allocates things */
     dnew->descriptor = desc;


//    dnew = new_descriptor();

//    dnew->descriptor	= desc;
//    dnew->connected	= CON_GET_NAME;
//    dnew->showstr_head	= NULL;
//    dnew->showstr_point = NULL;
//    dnew->outsize	= 2000;
//    dnew->pEdit		= NULL;			/* OLC */
//    dnew->pString	= NULL;			/* OLC */
//    dnew->editor	= 0;			/* OLC */
//    dnew->outbuf	= alloc_mem( dnew->outsize );

    size = sizeof(sock);
    if ( getpeername( desc, (struct sockaddr *) &sock, &size ) < 0 )
    {
	perror( "New_descriptor: getpeername" );
	dnew->host = str_dup( "(unknown)" );
    }
    else
    {
	/*
	 * Would be nice to use inet_ntoa here but it takes a struct arg,
	 * which ain't very compatible between gcc and system libraries.
	 */
	int addr;

	addr = ntohl( sock.sin_addr.s_addr );
	sprintf( buf, "%d.%d.%d.%d",
	    ( addr >> 24 ) & 0xFF, ( addr >> 16 ) & 0xFF,
	    ( addr >>  8 ) & 0xFF, ( addr       ) & 0xFF
	    );
	sprintf( log_buf, "Sock.sinaddr:  %s", buf );
	log_string( log_buf );
	from = gethostbyaddr( (char *) &sock.sin_addr,
	   sizeof(sock.sin_addr), AF_INET );
	dnew->host = str_dup( from ? from->h_name : buf );
	//dnew->host = str_dup(buf);
	dnew->ipaddr = str_dup(buf);
    }
	
    /*
     * Swiftest: I added the following to ban sites.  I don't
     * endorse banning of sites, but Copper has few descriptors now
     * and some people from certain sites keep abusing access by
     * using automated 'autodialers' and leaving connections hanging.
     *
     * Furey: added suffix check by request of Nickel of HiddenWorlds.
     */
    if ( check_ban(dnew->host,BAN_ALL))
    {
	write_to_descriptor( desc,
	    "Your site has been banned from this mud.\n\r", 0 );
	write_to_descriptor( desc,
	    "If you are reading this you have done something repeatedly that is\n\rdetrimental to the game.  The Administrative staff have reviewed the past\n\ractivities and have decided that it is in the best interest of the game as a\n\rwhole for you to seek your entertainment elsewhere.\n\r\n\rIf you feel that you are seeing this message in error, please send an email\n\rto tsw@shadowwars.org\n\r\n\rIf this message is showing up intentionally, we kindly invite you to find a\n\rnew mud to enjoy your gaming experience.",0);


	close( desc );
	free_descriptor(dnew);
	return;
    }
    /*
     * Init descriptor data.
     */
    dnew->next			= descriptor_list;
    descriptor_list		= dnew;

    /*
     * Send the greeting.
     */
    {
	extern char * help_greeting;
	if ( help_greeting[0] == '.' )
	    write_to_buffer( dnew, help_greeting+1, 0 );
	else
	    write_to_buffer( dnew, help_greeting  , 0 );
    }

    return;
}
#endif



void close_socket( DESCRIPTOR_DATA *dclose )
{
    CHAR_DATA *ch;

    if ( dclose->outtop > 0 )
	process_output( dclose, FALSE );

    if ( dclose->snoop_by != NULL )
    {
	write_to_buffer( dclose->snoop_by,
	    "Your victim has left the game.\n\r", 0 );
    }

    {
	DESCRIPTOR_DATA *d;

	for ( d = descriptor_list; d != NULL; d = d->next )
	{
	    if ( d->snoop_by == dclose )
		d->snoop_by = NULL;
	}
    }

    if ( ( ch = dclose->character ) != NULL ) {
	 sprintf( log_buf, "Closing link to %s.", ch->name );
	 log_string( log_buf );

	 /* cut down on wiznet spam when rebooting */
	 /* If ch is writing note or playing, just lose link otherwise clear char */
	 if ((dclose->connected == CON_PLAYING && !merc_down)
		||((dclose->connected >= CON_NOTE_TO)
		   && (dclose->connected <= CON_NOTE_FINISH))) {
	   if (ch->invis_level < 2 && ch->incog_level < 2)
           {
	   	act( "$n has lost $s link.", ch, NULL, NULL, TO_ROOM );
	   }
	   wiznet("$N has lost $S link.",ch,NULL,WIZ_LINKS,0,0);
	   ch->desc = NULL;
	 }
	 else {
	   free_char(dclose->original ? dclose->original : dclose->character );
	 }
    }
    
    if ( d_next == dclose )
	d_next = d_next->next;   

    if ( dclose == descriptor_list )
    {
	descriptor_list = descriptor_list->next;
    }
    else
    {
	DESCRIPTOR_DATA *d;

	for ( d = descriptor_list; d && d->next != dclose; d = d->next )
	    ;
	if ( d != NULL )
	    d->next = dclose->next;
	else
	    bug( "Close_socket: dclose not found.", 0 );
    }

    close( dclose->descriptor );
    free_descriptor(dclose);
#if defined(MSDOS) || defined(macintosh)
    exit(1);
#endif
    return;
}



bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
    int iStart;

    /* Hold horses if pending command already. */
    if ( d->incomm[0] != '\0' )
	return TRUE;

    /* Check for overflow. */
    iStart = strlen(d->inbuf);
    if ( iStart >= sizeof(d->inbuf) - 10 )
    {
	sprintf( log_buf, "%s input overflow!", d->host );
	log_string( log_buf );
	write_to_descriptor( d->descriptor,
	    "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
	return FALSE;
    }

    /* Snarf input. */
#if defined(macintosh)
    for ( ; ; )
    {
	int c;
	c = getc( stdin );
	if ( c == '\0' || c == EOF )
	    break;
	putc( c, stdout );
	if ( c == '\r' )
	    putc( '\n', stdout );
	d->inbuf[iStart++] = c;
	if ( iStart > sizeof(d->inbuf) - 10 )
	    break;
    }
#endif

#if defined(MSDOS) || defined(unix)
    for ( ; ; )
    {
	int nRead;

	nRead = read( d->descriptor, d->inbuf + iStart,
	    sizeof(d->inbuf) - 10 - iStart );
	if ( nRead > 0 )
	{
	    iStart += nRead;
	    if ( d->inbuf[iStart-1] == '\n' || d->inbuf[iStart-1] == '\r' )
		break;
	}
	else if ( nRead == 0 )
	{
	    log_string( "EOF encountered on read." );
	    return FALSE;
	}
	else if ( errno == EWOULDBLOCK )
	    break;
	else
	{
	    perror( "Read_from_descriptor" );
	    return FALSE;
	}
    }
    if (strstr(d->inbuf,"clearcommand\r") || strstr(d->inbuf,"clearcommand\n") || strstr(d->inbuf,"clrc\r") || (strstr(d->inbuf,"clrc\n")))
    {
	memset(d->inbuf,0,sizeof(d->inbuf));
	sprintf(d->incomm,"clearcommand");
    }
#endif

    d->inbuf[iStart] = '\0';
    return TRUE;
}



/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
    int i, j, k;

    /*
     * Hold horses if pending command already.
     */
    if ( d->incomm[0] != '\0' )
	return;

    /*
     * Look for at least one new line.
     */
    for ( i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
	if ( d->inbuf[i] == '\0' )
	    return;
    }

    /*
     * Canonical input processing.
     */
    for ( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
	if ( k >= MAX_INPUT_LENGTH - 2 )
	{
	    write_to_descriptor( d->descriptor, "Line too long.\n\r", 0 );

	    /* skip the rest of the line */
	    for ( ; d->inbuf[i] != '\0'; i++ )
	    {
		if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
		    break;
	    }
	    d->inbuf[i]   = '\n';
	    d->inbuf[i+1] = '\0';
	    break;
	}

	if ( d->inbuf[i] == '\b' && k > 0 )
	    --k;
	else if ( isascii(d->inbuf[i]) && isprint(d->inbuf[i]) )
	    d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if ( k == 0 )
	d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */

    if ( k > 1 || d->incomm[0] == '!' )
    {
    	if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
	{
	    d->repeat = 0;
	}
//	else
//	{
//	    if (++d->repeat >= 25 && d->character
//	    &&  d->connected == CON_PLAYING)
//	    {
//		sprintf( log_buf, "%s input spamming!", d->host );
//		log_string( log_buf );
//		wiznet("Spam spam spam $N spam spam spam spam spam!",
//		       d->character,NULL,WIZ_SPAM,0,get_trust(d->character));
//		if (d->incomm[0] == '!')
//		    wiznet(d->inlast,d->character,NULL,WIZ_SPAM,0,
//			get_trust(d->character));
//		else
//		    wiznet(d->incomm,d->character,NULL,WIZ_SPAM,0,
//			get_trust(d->character));
//
//		d->repeat = 0;
///*
//		write_to_descriptor( d->descriptor,
//		    "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
//		strcpy( d->incomm, "quit" );
// */
//	    }
//	}
    }


    /*
     * Do '!' substitution.
     */
    if ( d->incomm[0] == '!' )
	strcpy( d->incomm, d->inlast );
    else
	strcpy( d->inlast, d->incomm );

    /*
     * Shift the input buffer.
     */
    while ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
	i++;
    for ( j = 0; ( d->inbuf[j] = d->inbuf[i+j] ) != '\0'; j++ )
	;
    return;
}

char *get_hit_loc_wound_str(CHAR_DATA *victim, int location)
{
  static char wound[64];
  int percent=-1;

  memset(wound, 0x00, sizeof(wound));

  if (victim->hit_loc[location] > 0)
    percent = victim->hit_loc[location] * 100 / get_max_hit_loc(victim, location);
  else
    percent = -1;

  if (percent >= 100) {
    sprintf(wound, "{gfully healed");
  }
  else if (percent >= 75) {
    sprintf(wound, "{Gslightly wounded");	 
  }
  else if (percent >= 45) {
    sprintf(wound, "{Ywounded");
  }
  else if (percent >= 20) {
    sprintf(wound, "{rbadly wounded");
  }
  else if (percent >= 3) {
    sprintf(wound, "{Rextreamly wounded");
  }
  else {
    sprintf(wound, "{yalmost beyond healing");
  }
  
  return (wound);  
  
}

char *get_hit_loc_col(CHAR_DATA *ch, CHAR_DATA *victim, int location)
{
  static char colcode[4];
  int percent=-1;

  memset(colcode, 0x00, sizeof(colcode));

  if (victim->hit_loc[location] > 0)
    percent = victim->hit_loc[location] * 100 / get_max_hit_loc(victim, location);
  else
    percent = -1;

  if (percent >= 100) {
    if (IS_SET(ch->act, PLR_COLOUR))
	 sprintf(colcode, "{g");
    else
	 sprintf(colcode, "*");
  }
  else if (percent >= 75) {
    if (IS_SET(ch->act, PLR_COLOUR))
	 sprintf(colcode, "{G");
    else
	 sprintf(colcode, "^");
  }
  else if (percent >= 45) {
    if (IS_SET(ch->act, PLR_COLOUR))
	 sprintf(colcode, "{Y");
    else
	 sprintf(colcode, "-");
  }
  else if (percent >= 20) {
    if (IS_SET(ch->act, PLR_COLOUR))
	 sprintf(colcode, "{r");
    else
	 sprintf(colcode, "_");
  }
  else if (percent >= 3) {
    if (IS_SET(ch->act, PLR_COLOUR))
	 sprintf(colcode, "{R");
    else
	 sprintf(colcode, "!");
  }
  else {
    if (IS_SET(ch->act, PLR_COLOUR))
	 sprintf(colcode, "{y");
    else
	 sprintf(colcode, "!");
  }
  
  return (colcode);  
  
}

char * get_hit_loc(CHAR_DATA *ch, CHAR_DATA *victim, char * loc_str, int location)
{
  static char wound[16];

  sprintf(wound, "%s%s{x", get_hit_loc_col(ch, victim, location), loc_str);
  return(wound);
}


char * get_hit_loc_str(CHAR_DATA *ch, CHAR_DATA *victim)
{
  static char loc_wound[100];
  
  sprintf(loc_wound, "[ ");
  strcat(loc_wound, get_hit_loc(ch, victim, "LA", LOC_LA));
  strcat(loc_wound, " ");
  strcat(loc_wound, get_hit_loc(ch, victim, "LL", LOC_LL));
  strcat(loc_wound, " ");
  strcat(loc_wound, get_hit_loc(ch, victim, "HE", LOC_HE));
  strcat(loc_wound, " ");
  strcat(loc_wound, get_hit_loc(ch, victim, "BD", LOC_BD));
  strcat(loc_wound, " ");
  strcat(loc_wound, get_hit_loc(ch, victim, "RA", LOC_RA));
  strcat(loc_wound, " ");
  strcat(loc_wound, get_hit_loc(ch, victim, "RL", LOC_RL));
  strcat(loc_wound, " ]");
  return (loc_wound);
}

void do_health(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];

  //sprintf(buf, "%s\n", get_hit_loc_str(ch, ch));  
  //send_to_char(buf, ch);

  sprintf(buf, "Your %s is %s{x.\n\r", hit_flags[LOC_LA].name, get_hit_loc_wound_str(ch, LOC_LA));
  send_to_char(buf, ch);
  sprintf(buf, "Your %s is %s{x.\n\r", hit_flags[LOC_LL].name, get_hit_loc_wound_str(ch, LOC_LL));
  send_to_char(buf, ch);
  sprintf(buf, "Your %s is %s{x.\n\r", hit_flags[LOC_HE].name, get_hit_loc_wound_str(ch, LOC_HE));
  send_to_char(buf, ch);
  sprintf(buf, "Your %s is %s{x.\n\r", hit_flags[LOC_BD].name, get_hit_loc_wound_str(ch, LOC_BD));
  send_to_char(buf, ch);
  sprintf(buf, "Your %s is %s{x.\n\r", hit_flags[LOC_RA].name, get_hit_loc_wound_str(ch, LOC_RA));
  send_to_char(buf, ch);
  sprintf(buf, "Your %s is %s{x.\n\r", hit_flags[LOC_RL].name, get_hit_loc_wound_str(ch, LOC_RL));
  send_to_char(buf, ch);
  return;
}

/*
 * Low level output function.
 */
bool process_output( DESCRIPTOR_DATA *d, bool fPrompt )
{
  extern bool merc_down;
  int xp;
  
  /*
   * Bust a prompt.
   */
  if ( !merc_down ) { 	
    if ( d->showstr_point )
    {
	   /* Get exp per round of action */
	 CHAR_DATA *ch;
	 CHAR_DATA *victim;
	 ch = d->character;
         /*
	 if ((victim = ch->fighting) != NULL ) 
         { 
	   if (!IS_NPC(ch) && ch->gain_xp == TRUE) 
           {	        
	      if ( ch->endurance > 0 ) {
		      xp = xp_compute( ch, victim, ch->level );  
		      gain_exp( ch, xp );
		      ch->gain_xp = FALSE;
	      }
      	   }
         }
	 */
	 write_to_buffer( d, "\n[Hit Return to continue]\n\r", 0 );
    }
    else if ( fPrompt && d->pString && d->connected == CON_PLAYING ) {
	 send_to_char("Edit:{CString{x >", d->character);
	 d->character->editor = TRUE;
    }
    else if ( fPrompt && d->pEdit && d->connected == CON_PLAYING ) {
	 d->character->editor = TRUE;
	 switch (d->editor) {
	 case ED_AREA:   send_to_char("Edit:{RArea{x >", d->character); break;
	 case ED_ROOM:   send_to_char("Edit:{yRoom{x >", d->character); break;
	 case ED_OBJECT: send_to_char("Edit:{WObject{x >", d->character); break;
	 case ED_MOBILE: send_to_char("Edit:{rMobile{x >", d->character); break;
	 case ED_PROG:   send_to_char("Edit:Mprog >", d->character); break;
	 case ED_GUILD:  send_to_char("Edit:{BGuild{x >", d->character); break;
	 case ED_SGUILD:  send_to_char("Edit:{BSguild{x >", d->character); break;
	 case ED_RACE:   send_to_char("Edit:{MRace{x >", d->character); break;
	 case ED_SOCIAL: send_to_char("Edit:{gSocial{x >", d->character); break;
	 case ED_SKILL:  send_to_char("Edit:{cSkill{x >", d->character); break;
	 case ED_CMD:    send_to_char("Edit:{YCmd{x >", d->character); break;
	 case ED_GROUP:  send_to_char("Edit:Group >", d->character); break;
	 case ED_HELP:   send_to_char("Edit:{GHelp{x >", d->character); break;
	 default:        send_to_char("Edit: >", d->character); break;
	 }
    }
    else if ( fPrompt && d->connected == CON_PLAYING ) {
	 CHAR_DATA *ch;
	 CHAR_DATA *victim;
	 CHAR_DATA *tanker;
	 
	 d->character->editor = FALSE;
	 ch = d->character;
	 
	 /* battle prompt */
	 if ((victim = ch->fighting) != NULL ) { /* && can_see(ch,victim)) { */
	   int percent;
	   char wound[100];
	   char buf[MAX_STRING_LENGTH];
	   char *pbuff;
	   char buffer[ MAX_STRING_LENGTH*2 ];
	   
	   if (victim->max_hit > 0)
		percent = victim->hit * 100 / victim->max_hit;
	   else
		percent = -1;
	   
	   if (percent >= 100)
		sprintf(wound,"is in perfect condition.");
	   else if (percent >= 90)
		sprintf(wound,"has a few scratches.");
	   else if (percent >= 75)
		sprintf(wound,"has taken some hits.");
	   else if (percent >= 50)
		sprintf(wound,"should start to worry.");
	   else if (percent >= 30)
		sprintf(wound,"should be worrying.");
	   else if (percent >= 15)
		sprintf(wound,"better run.");
	   else if (percent >= 0)
		sprintf(wound,"should have run.");
	   else
		sprintf(wound,"is bleeding to death.");
	   
	   sprintf(buf, "%s %s %s \n\r",
			 get_hit_loc_str(ch, victim),
			 PERS(victim, ch),
			 /* IS_NPC(victim) ? victim->short_descr : victim->name,  */
			 wound);
	   
	   buf[0] = UPPER(buf[0]);
	   
	   pbuff	= buffer;
	   colourconv( pbuff, buf, ch );
	   write_to_buffer( d, buffer, 0);
	   

	   /* Battle prompt for TANKER */
	   if ((tanker = victim->fighting) != NULL && ch != tanker) {
		if (tanker->max_hit > 0)
		  percent = tanker->hit * 100 / tanker->max_hit;
		else
		  percent = -1;
		
		if (percent >= 100)
		  sprintf(wound,"is in perfect condition.");
		else if (percent >= 90)
		  sprintf(wound,"has a few scratches.");
		else if (percent >= 75)
		  sprintf(wound,"has taken some hits.");
		else if (percent >= 50)
		  sprintf(wound,"should start to worry.");
		else if (percent >= 30)
		  sprintf(wound,"should be worrying.");
		else if (percent >= 15)
		  sprintf(wound,"better run.");
		else if (percent >= 0)
		  sprintf(wound,"should have run.");
		else
		  sprintf(wound,"is bleeding to death.");
		
		sprintf(buf, "%s %s %s \n\r",
			   get_hit_loc_str(ch, tanker),
			   PERS(tanker, ch),
			   /* IS_NPC(tanker) ? tanker->short_descr : tanker->name,  */
			   wound);
		
		buf[0] = UPPER(buf[0]);
		
		pbuff	= buffer;
		colourconv( pbuff, buf, ch );
		write_to_buffer( d, buffer, 0);
	   }
	   
	   /* Battle prompt for You (ch)*/
	   if (ch->max_hit > 0)
		percent = ch->hit * 100 / ch->max_hit;
	   else
		percent = -1;
	   
	   if (percent >= 100)
		sprintf(wound,"are in perfect condition.");
	   else if (percent >= 90)
		sprintf(wound,"have a few scratches.");
	   else if (percent >= 75)
		sprintf(wound,"have taken some hits.");
	   else if (percent >= 50)
		sprintf(wound,"should start to worry.");
	   else if (percent >= 30)
		sprintf(wound,"should be worrying.");
	   else if (percent >= 15)
		sprintf(wound,"better run.");
	   else if (percent >= 0)
		sprintf(wound,"should have run.");
	   else
		sprintf(wound,"is bleeding to death.");
	   
	   sprintf(buf, "%s %s %s \n\r", get_hit_loc_str(ch, ch), "You", wound);
	   
	   buf[0] = UPPER(buf[0]);
	   pbuff	= buffer;
	   colourconv( pbuff, buf, ch );
	   write_to_buffer( d, buffer, 0);
	   
	   /* Get exp per round of action */
	   /*
	   if (!IS_NPC(ch) && ch->gain_xp == TRUE) 
           {	        
	      if ( ch->endurance > 0 ) {
		      xp = xp_compute( ch, victim, ch->level );  
		      gain_exp( ch, xp );
		      ch->gain_xp = FALSE;
	      }
      	   }
	   */
	 }
	 
	 ch = d->original ? d->original : d->character;
	 //if (!IS_SET(ch->comm, COMM_COMPACT) )
	   write_to_buffer( d, "\n\r", 2 );
	 
	 if ( IS_SET(ch->comm, COMM_PROMPT) )
	   bust_a_prompt( d->character );
	 
	 if (IS_SET(ch->comm,COMM_TELNET_GA))
	   write_to_buffer(d,go_ahead_str,0);
	 
	 if (IS_SET(ch->comm,COMM_OLCX)
		&&  d->editor != ED_NONE
		&&  d->pString == NULL)
	   UpdateOLCScreen(d);
    }
  }
  
  /*
   * Short-circuit if nothing to write.
   */
  if ( d->outtop == 0 )
    return TRUE;
  
  /*
   * Snoop-o-rama.
   */
  if ( d->snoop_by != NULL ) {
    if (d->character != NULL) {
	 write_to_buffer( d->snoop_by, "(S) [", 0);
	 write_to_buffer( d->snoop_by, d->character->name,0);
    }
    write_to_buffer( d->snoop_by, "]:\n\r", 0 );
    write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
  }
  
  /*
   * OS-dependent output.
   */
  if ( !write_to_descriptor( d->descriptor, d->outbuf, d->outtop ) ) {
    d->outtop = 0;
    return FALSE;
  }
  else {
    d->outtop = 0;
    return TRUE;
  }
}

bool process_output_OLD( DESCRIPTOR_DATA *d, bool fPrompt )
{
  extern bool merc_down;
  int xp;
  
  /*
   * Bust a prompt.
   */
  if ( !merc_down ) { 	
    if ( d->showstr_point )
	 write_to_buffer( d, "\n[Hit Return to continue]\n\r", 0 );
    else if ( fPrompt && d->pString && d->connected == CON_PLAYING ) {
	 send_to_char("Edit:{CString{x >", d->character);
	 d->character->editor = TRUE;
    }
    else if ( fPrompt && d->pEdit && d->connected == CON_PLAYING ) {
	 d->character->editor = TRUE;
	 switch (d->editor) {
	 case ED_AREA:   send_to_char("Edit:{RArea{x >", d->character); break;
	 case ED_ROOM:   send_to_char("Edit:{yRoom{x >", d->character); break;
	 case ED_OBJECT: send_to_char("Edit:{WObject{x >", d->character); break;
	 case ED_MOBILE: send_to_char("Edit:{rMobile{x >", d->character); break;
	 case ED_PROG:   send_to_char("Edit:Mprog >", d->character); break;
	 case ED_GUILD:  send_to_char("Edit:{BGuild{x >", d->character); break;
	 case ED_RACE:   send_to_char("Edit:{MRace{x >", d->character); break;
	 case ED_SOCIAL: send_to_char("Edit:{gSocial{x >", d->character); break;
	 case ED_SKILL:  send_to_char("Edit:{cSkill{x >", d->character); break;
	 case ED_CMD:    send_to_char("Edit:{YCmd{x >", d->character); break;
	 case ED_GROUP:  send_to_char("Edit:Group >", d->character); break;
	 case ED_HELP:   send_to_char("Edit:{GHelp{x >", d->character); break;
	 default:        send_to_char("Edit: >", d->character); break;
	 }
    }
    else if ( fPrompt && d->connected == CON_PLAYING ) {
	 CHAR_DATA *ch;
	 CHAR_DATA *victim;
	 CHAR_DATA *tanker;
	 
	 d->character->editor = FALSE;
	 ch = d->character;
	 
	 /* battle prompt */
	 if ((victim = ch->fighting) != NULL ) { /* && can_see(ch,victim)) { */
	   int percent;
	   char wound[100];
	   char wound2[100];
	   char buf[MAX_STRING_LENGTH];
	   char *pbuff;
	   char buffer[ MAX_STRING_LENGTH*2 ];
	   
	   if (victim->max_hit > 0)
		percent = victim->hit * 100 / victim->max_hit;
	   else
		percent = -1;
	   
	   if     (percent >= 100)
		sprintf(wound,"{D[{R@{G>>>{G>>>{G>>>>{D]{x");
	   else if (percent >= 90)
		sprintf(wound,"{D[{R@{g>>>{g>>>{g>>> {D]{x");
	   else if (percent >= 80)
		sprintf(wound,"{D[{R@{g>>>{g>>>{g>>  {D]{x");
	   else if (percent >= 70)
		sprintf(wound,"{D[{R@{g>>>{g>>>{g>   {D]{x");
	   else if (percent >= 58)
		sprintf(wound,"{D[{R@{Y>>>{Y>>>    {D]{x");
	   else if (percent >= 45)
		sprintf(wound,"{D[{R@{Y>>>{Y>>     {D]{x");
	   else if (percent >= 30)
		sprintf(wound,"{D[{R@{Y>>>{Y>      {D]{x");
	   else if (percent >= 28)
		sprintf(wound,"{D[{R@{r>>>{x       {D]{x");
	   else if (percent >= 15)
		sprintf(wound,"{D[{R@{r>>{x        {D]{x");
	   else if (percent >= 8)
		sprintf(wound,"{D[{R@{r>{x         {D]{x");
	   else
		sprintf(wound,"{D[{y@          {D]{x"); 
	   
	   if (percent >= 100)
		sprintf(wound2,"is in perfect condition.");
	   else if (percent >= 90)
		sprintf(wound2,"has a few scratches.");
	   else if (percent >= 75)
		sprintf(wound2,"has taken some hits.");
	   else if (percent >= 50)
		sprintf(wound2,"should start to worry.");
	   else if (percent >= 30)
		sprintf(wound2,"should be worrying.");
	   else if (percent >= 15)
		sprintf(wound2,"better run.");
	   else if (percent >= 0)
		sprintf(wound2,"should have run.");
	   else
		sprintf(wound2,"is bleeding to death.");
	   
	   sprintf(buf, "%s %s %s \n\r",
			 wound,
			 PERS(victim, ch),
			 /* IS_NPC(victim) ? victim->short_descr : victim->name,  */
			 wound2);
	   
	   buf[0] = UPPER(buf[0]);
	   
	   pbuff	= buffer;
	   colourconv( pbuff, buf, ch );
	   write_to_buffer( d, buffer, 0);
	   

	   /* Battle prompt for TANKER */
	   if ((tanker = victim->fighting) != NULL && ch != tanker) {
		if (tanker->max_hit > 0)
		  percent = tanker->hit * 100 / tanker->max_hit;
		else
		  percent = -1;
		
		if     (percent >= 100)
		  sprintf(wound,"{D[{R@{G>>>{G>>>{G>>>>{D]{x");
		else if (percent >= 90)
		  sprintf(wound,"{D[{R@{g>>>{g>>>{g>>> {D]{x");
		else if (percent >= 80)
		  sprintf(wound,"{D[{R@{g>>>{g>>>{g>>  {D]{x");
		else if (percent >= 70)
		  sprintf(wound,"{D[{R@{g>>>{g>>>{g>   {D]{x");
		else if (percent >= 58)
		  sprintf(wound,"{D[{R@{Y>>>{Y>>>    {D]{x");
		else if (percent >= 45)
		  sprintf(wound,"{D[{R@{Y>>>{Y>>     {D]{x");
		else if (percent >= 30)
		  sprintf(wound,"{D[{R@{Y>>>{Y>      {D]{x");
		else if (percent >= 28)
		  sprintf(wound,"{D[{R@{r>>>{x       {D]{x");
		else if (percent >= 15)
		  sprintf(wound,"{D[{R@{r>>{x        {D]{x");
		else if (percent >= 8)
		  sprintf(wound,"{D[{R@{r>{x         {D]{x");
		else
		  sprintf(wound,"{D[{y@          {D]{x"); 
		
		if (percent >= 100)
		  sprintf(wound2,"is in perfect condition.");
		else if (percent >= 90)
		  sprintf(wound2,"has a few scratches.");
		else if (percent >= 75)
		  sprintf(wound2,"has taken some hits.");
		else if (percent >= 50)
		  sprintf(wound2,"should start to worry.");
		else if (percent >= 30)
		  sprintf(wound2,"should be worrying.");
		else if (percent >= 15)
		  sprintf(wound2,"better run.");
		else if (percent >= 0)
		  sprintf(wound2,"should have run.");
		else
		  sprintf(wound2,"is bleeding to death.");
		
		sprintf(buf, "%s %s %s \n\r",
			   wound,
			   PERS(tanker, ch),
			   /* IS_NPC(tanker) ? tanker->short_descr : tanker->name,  */
			   wound2);
		
		buf[0] = UPPER(buf[0]);
		
		pbuff	= buffer;
		colourconv( pbuff, buf, ch );
		write_to_buffer( d, buffer, 0);
	   }
	   
	   /* Battle prompt for You (ch)*/
	   if (ch->max_hit > 0)
		percent = ch->hit * 100 / ch->max_hit;
	   else
		percent = -1;
	   
	   if     (percent >= 100)
		sprintf(wound,"{D[{R@{G>>>{G>>>{G>>>>{D]{x");
	   else if (percent >= 90)
		sprintf(wound,"{D[{R@{g>>>{g>>>{g>>> {D]{x");
	   else if (percent >= 80)
		sprintf(wound,"{D[{R@{g>>>{g>>>{g>>  {D]{x");
	   else if (percent >= 70)
		sprintf(wound,"{D[{R@{g>>>{g>>>{g>   {D]{x");
	   else if (percent >= 58)
		sprintf(wound,"{D[{R@{Y>>>{Y>>>    {D]{x");
	   else if (percent >= 45)
		sprintf(wound,"{D[{R@{Y>>>{Y>>     {D]{x");
	   else if (percent >= 30)
		sprintf(wound,"{D[{R@{Y>>>{Y>      {D]{x");
	   else if (percent >= 28)
		sprintf(wound,"{D[{R@{r>>>{x       {D]{x");
	   else if (percent >= 15)
		sprintf(wound,"{D[{R@{r>>{x        {D]{x");
	   else if (percent >= 8)
		sprintf(wound,"{D[{R@{r>{x         {D]{x");
	   else
		sprintf(wound,"{D[{y@          {D]{x"); 
	   
	   if (percent >= 100)
		sprintf(wound2,"are in perfect condition.");
	   else if (percent >= 90)
		sprintf(wound2,"have a few scratches.");
	   else if (percent >= 75)
		sprintf(wound2,"have taken some hits.");
	   else if (percent >= 50)
		sprintf(wound2,"should start to worry.");
	   else if (percent >= 30)
		sprintf(wound2,"should be worrying.");
	   else if (percent >= 15)
		sprintf(wound2,"better run.");
	   else if (percent >= 0)
		sprintf(wound2,"should have run.");
	   else
		sprintf(wound2,"is bleeding to death.");
	   
	   sprintf(buf, "%s %s %s \n\r", wound, "You", wound2);
	   
	   buf[0] = UPPER(buf[0]);
	   pbuff	= buffer;
	   colourconv( pbuff, buf, ch );
	   write_to_buffer( d, buffer, 0);
	   
	   /* Get exp per round of action */
	   /*
	   if (!IS_NPC(ch) && ch->gain_xp == TRUE) 
           {	        
	      if ( ch->endurance > 0 ) 
              {
		      xp = xp_compute( ch, victim, ch->level );  
		      gain_exp( ch, xp );
		      ch->gain_xp = FALSE;
	       }
     	    }
	   */
	 }
	 
	 ch = d->original ? d->original : d->character;
	 if (!IS_SET(ch->comm, COMM_COMPACT) )
	   write_to_buffer( d, "\n\r", 2 );
	 
	 if ( IS_SET(ch->comm, COMM_PROMPT) )
	   bust_a_prompt( d->character );
	 
	 if (IS_SET(ch->comm,COMM_TELNET_GA))
	   write_to_buffer(d,go_ahead_str,0);
	 
	 if (IS_SET(ch->comm,COMM_OLCX)
		&&  d->editor != ED_NONE
		&&  d->pString == NULL)
	   UpdateOLCScreen(d);
    }
  }
  
  /*
   * Short-circuit if nothing to write.
   */
  if ( d->outtop == 0 )
    return TRUE;
  
  /*
   * Snoop-o-rama.
   */
  if ( d->snoop_by != NULL ) {
    if (d->character != NULL) {
	 write_to_buffer( d->snoop_by, "(S) [", 0);
	 write_to_buffer( d->snoop_by, d->character->name,0);
    }
    write_to_buffer( d->snoop_by, "]:\n\r", 0 );
    write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
  }
  
  /*
   * OS-dependent output.
   */
  if ( !write_to_descriptor( d->descriptor, d->outbuf, d->outtop ) ) {
    d->outtop = 0;
    return FALSE;
  }
  else {
    d->outtop = 0;
    return TRUE;
  }
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt( CHAR_DATA *ch )
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  const char *str;
  const char *i;
  char *point;
  char *pbuff;
  char buffer[ MAX_STRING_LENGTH*2 ];
  char doors[MAX_INPUT_LENGTH];
  EXIT_DATA *pexit;
  bool found;
  const char *dir_name[] = {"N","E","S","W","U","D"};
  int door;
  int fl=0; // Females linked
  int ml=0; // Males linked
  
  point = buf;
  str = ch->prompt;
  if (str == NULL || str[0] == '\0') {
    sprintf( buf, "{p[%dhp %den]{x %s",
		   ch->hit,ch->endurance,ch->prefix);
    send_to_char(buf,ch);
    return;
  }

  /* Can Vote                                         */
  /****************************************************/
  if (!IS_NPC(ch) && IS_SET(ch->auto_act,AUTO_VOTEREMINDER))
  {
     time_t next_vote=0;
     next_vote = ch->pcdata->last_web_vote+43200;
     if (next_vote <= current_time) {
        send_to_char("{w(V) {x", ch);
     }
  }

  while( *str != '\0' ) {
    if( *str != '%' ) {
	 *point++ = *str++;
	 continue;
    }
    ++str;
    
    switch( *str ) {
    default :
	 i = " "; break;
    case 'e':
	 found = FALSE;
	 doors[0] = '\0';
	 for (door = 0; door < 6; door++) {
	   if ((pexit = ch->in_room->exit[door]) != NULL
		  &&  pexit ->u1.to_room != NULL
		  && (!IS_SET(pexit->exit_info, EX_HIDDEN) || IS_IMMORTAL(ch))

		  &&  (can_see_room(ch,pexit->u1.to_room)
			  ||   (IS_AFFECTED(ch,AFF_INFRARED) 
				   &&    !IS_AFFECTED(ch,AFF_BLIND)
				   && 	 !IS_AFFECTED(ch,AFF_BLINDFOLDED)))) {
		found = TRUE;
                if (!IS_SET(pexit->exit_info,EX_CLOSED)) {
			strcat(doors,dir_name[door]);
		}
		else
		{
			char mybuf[10];
			sprintf(mybuf,"{r%s{x",dir_name[door]);
			strcat(doors,mybuf);
		}
	   }
	 }
	 if (!found)
	   strcat(buf,"none");
	 sprintf(buf2,"%s",doors);
	 i = buf2; 
	 break;
    case 'c' :
	 sprintf(buf2,"%s","\n\r");
	 i = buf2; break;
    case 'h' :
	 sprintf( buf2, "%d", ch->hit );
	 i = buf2; break;
    case 'H' :
	 sprintf( buf2, "%d", ch->max_hit );
	 i = buf2; break;
    case 'm' :
	 sprintf( buf2, "%d", ch->endurance );
	 i = buf2; break;
    case 'M' :
	 sprintf( buf2, "%d", ch->max_endurance );
	 i = buf2; break;
    case 'x' :
	 sprintf( buf2, "%d", ch->exp );
	 i = buf2; break;
    case 'X' :
	 sprintf(buf2, "%ld", IS_NPC(ch) ? 0 :
		    (exp_next_level(ch) - ch->exp));
	 i = buf2; break;
    case 'g' :
	 sprintf( buf2, "%ld", ch->gold);
	 i = buf2; break;
    case 's' :
	 sprintf( buf2, "%ld", ch->silver);
	 i = buf2; break;
    case 'a' :
	 if( ch->level > 9 )
	   sprintf( buf2, "%d", ch->alignment );
	 else
	   sprintf( buf2, "%s", IS_GOOD(ch) ? "good" : IS_EVIL(ch) ?
			  "evil" : "neutral" );
	 i = buf2; break;
    case 'r' :
	 if( ch->in_room != NULL )
	   sprintf( buf2, "%s", 
			  ((!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT)) ||
			   (!IS_AFFECTED(ch,AFF_BLIND) && !room_is_dark( ch->in_room )
				   && 	 !IS_AFFECTED(ch,AFF_BLINDFOLDED)))
			  ? ch->in_room->name : "darkness");
	 else
	   sprintf( buf2, " " );
	 i = buf2; break;
    case 'R' :
	 if( IS_IMMORTAL( ch ) && ch->in_room != NULL )
	   sprintf( buf2, "%d", ch->in_room->vnum );
	 else
	   sprintf( buf2, " " );
	 i = buf2; break;
    case 'z' :
	 if( IS_IMMORTAL( ch ) && ch->in_room != NULL )
	   sprintf( buf2, "%s", ch->in_room->area->name );
	 else
	   sprintf( buf2, " " );
	 i = buf2; break;
    case '%' :
	 sprintf( buf2, "%%" );
	 i = buf2; break;
    case 'p':
	 sprintf( buf2, "%ld", ch->holding);
	 i = buf2; break;
    case 'P':
	 sprintf( buf2, "%ld", get_curr_op(ch));
	 i = buf2; break;
    case 'o' :
	 sprintf( buf2, "%s", olc_ed_name(ch) );
	 i = buf2; break;
    case 'O' :
	 sprintf( buf2, "%s", olc_ed_vnum(ch) );
	 i = buf2; break;
    }
    ++str;
    while( (*point = *i) != '\0' )
	 ++point, ++i;
  }
  *point	= '\0';
  pbuff	= buffer;
  colourconv( pbuff, buf, ch );
  send_to_char( "{p", ch );
  write_to_buffer( ch->desc, buffer, 0 );
  send_to_char( "{x", ch );

  /* Sustained flows                                  */
  /*--------------------------------------------------*/
  if (IS_AFFECTED(ch, AFF_CHANNELING)) {
    if (get_curr_flows(ch) > 0) {
	 sprintf(buf2, "({rs:%d{x) ", get_curr_flows(ch));
	 send_to_char(buf2, ch);
    }
  }

  /* Hunger/Thirst flags                             */
  /*--------------------------------------------------*/
  if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] ==  0 )
    send_to_char("({mthir{x) ", ch );
  if ( !IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER]   ==  0 )
    send_to_char( "({mhun{x) ",  ch );


  /* World                                            */
  /*--------------------------------------------------*/
  if (!IS_SET(ch->world,WORLD_NORMAL)) {
    buf2[0] = '\0';
    sprintf(buf2, "({R%s{x)","d");
    send_to_char(buf2, ch);
  }
  

  /* Channeling flag                                  */
  /*--------------------------------------------------*/  
  if (IS_AFFECTED(ch, AFF_CHANNELING)) {
    buf2[0] = '\0';
    sprintf(buf2, "({W%s{x)","c");
    send_to_char(buf2, ch);
  }
  
  /* Linked flag: Only the one in controll see num    */
  /*--------------------------------------------------*/  
  if (IS_AFFECTED(ch, AFF_CHANNELING) && IS_LINKED(ch)) {
    buf2[0] = '\0';

    fl = get_females_in_link(ch);
    ml = get_males_in_link(ch);

    if (fl > 0 || ml > 0)
	 sprintf(buf2, "({Y%s{x:{R%d{x/{B%d{x)","li", fl, ml);
    else
	 sprintf(buf2, "({Y%s{x)","li");

    send_to_char(buf2, ch);
  }  
  
  /* Disguied                                         */
  /*--------------------------------------------------*/  
  if (IS_DISGUISED(ch)) {
    buf2[0] = '\0';
    sprintf(buf2, "({W%s{x)","di");
    send_to_char(buf2, ch);
  }

  /* Stilled flag                                     */
  /*--------------------------------------------------*/
  if (IS_SET(ch->act,PLR_STILLED)) {
    buf2[0] = '\0';
    sprintf(buf2, "({r%s{x)", ch->sex == SEX_MALE ? "gentled" : "stilled");
    send_to_char(buf2, ch);
   }

  /* Sneak flag                                       */
  /*--------------------------------------------------*/
  if (IS_AFFECTED(ch, AFF_SNEAK)) {
    buf2[0] = '\0';
    sprintf(buf2, "({m%s{x)","s");
    send_to_char(buf2, ch);
  }

  /* Hide flag                                        */
  /*--------------------------------------------------*/
  if (IS_AFFECTED(ch, AFF_HIDE)) {
    buf2[0] = '\0';
    sprintf(buf2, "({Y%s{x)","h");
    send_to_char(buf2, ch);
  }

  /* Invis flag                                       */
  /*--------------------------------------------------*/
  if (IS_AFFECTED(ch, AFF_INVISIBLE)) {
    buf2[0] = '\0';
    sprintf(buf2, "({c%s{x)","i");
    send_to_char(buf2, ch);
  }

  /* Illusion flag                                    */
  /*--------------------------------------------------*/
  if (IS_ILLUSIONAPP(ch)) {
    buf2[0] = '\0';
    sprintf(buf2, "({c%s{x)","il");
    send_to_char(buf2, ch);
  }
  
  /* Giant size flag                                  */
  /*--------------------------------------------------*/
  if (IS_GIANTILLUSION(ch)) {
    buf2[0] = '\0';
    sprintf(buf2, "({c%s{x)","gi");
    send_to_char(buf2, ch);
  }

  /* Giant size flag                                  */
  /*--------------------------------------------------*/
  if (IS_BLOCKING(ch)) {
    buf2[0] = '\0';
    sprintf(buf2, "({R%s{x:{Y%s{x)","bl", dir_name[ch->exit_block.direction]);

     send_to_char(buf2, ch);
  }  

   
   /* Bound flag                                      */
   /*-------------------------------------------------*/
   if (IS_AFFECTED(ch,AFF_BIND))
     {
	buf2[0] = '\0';
	sprintf(buf2,"({R%s{x)","bind");
	send_to_char(buf2, ch);
	
     }
   
   
  /* BLindfold flag                                  */
  /*-------------------------------------------------*/
  
   if (IS_AFFECTED(ch,AFF_BLINDFOLDED))
     {
	buf2[0] = '\0';
	sprintf(buf2,"({R%s{x)","blndfld");
	send_to_char(buf2,ch);
     }
   
   
  /* WIZI flag                                        */
  /*--------------------------------------------------*/
  if (IS_IMMORTAL(ch)) {
    if (ch->invis_level >= LEVEL_HERO) {
      buf2[0] = '\0';
      sprintf(buf2, "({Y%s{x:{Y%d{x)","w", ch->invis_level);
      send_to_char(buf2, ch);
    }
  }
  
  /* Whoinvis flag                                    */
  /*--------------------------------------------------*/
  if (!IS_IMMORTAL(ch)) {
    if (ch->incog_level > 0 ) {
      buf2[0] = '\0';
      sprintf(buf2, "({Y%s{x)", "w");
      send_to_char(buf2, ch);
    }
  }

  /* Cloaked flag                                     */
  /*--------------------------------------------------*/
  if (IS_CLOAKED(ch)) {
    buf2[0] = '\0';
    sprintf(buf2, "({Y%s{x)","cl");
    send_to_char(buf2, ch);
  }
  
  /* Color Cloaked flag                               */
  /*--------------------------------------------------*/
  if (IS_COLORCLOAKED(ch)) {
    buf2[0] = '\0';
    sprintf(buf2, "({Y%s{x)","ccl");
    send_to_char(buf2, ch);
  }

  /* Hooded flag                                      */
  /*--------------------------------------------------*/
  if (IS_HOODED(ch)) {
    buf2[0] = '\0';
    sprintf(buf2, "({Y%s{x)","ho");
    send_to_char(buf2, ch);
  }
  
  /* Veiled flag                                      */
  /*--------------------------------------------------*/
  if (IS_VEILED(ch)) {
    buf2[0] = '\0';
    sprintf(buf2, "({Y%s{x)","ve");
    send_to_char(buf2, ch);
  }  

  /* Guild invis flag                                 */
  /*--------------------------------------------------*/
  if (is_clan(ch) && ch->ginvis == TRUE) {	
    buf2[0] = '\0';
    sprintf(buf2, "({Y%s{x)","gi");
    send_to_char(buf2, ch);  	
  }

  /* Guild muted flag                                 */
  /*--------------------------------------------------*/
  if (is_clan(ch) && ch->gmute == TRUE) {	
    buf2[0] = '\0';
    sprintf(buf2, "({R%s{x)","gm");
    send_to_char(buf2, ch);  	
  }
  
  /* SGuild invis flag                                */
  /*--------------------------------------------------*/
  if (is_sguild(ch) && ch->sguild_invis == TRUE) {	
    buf2[0] = '\0';
    sprintf(buf2, "({Y%s{x)","sgi");
    send_to_char(buf2, ch);  	
  }  

  /* SSGuild invis flag                                */
  /*--------------------------------------------------*/
  if (is_ssguild(ch) && ch->ssguild_invis == TRUE) {	
    buf2[0] = '\0';
    sprintf(buf2, "({Y%s{x)","ssgi");
    send_to_char(buf2, ch);  	
  }  
  
  /* AFK flag                                         */
  /*--------------------------------------------------*/
  if (IS_SET(ch->comm,COMM_AFK)) {
    buf2[0] = '\0';
    sprintf(buf2, "({Y%s{x)","afk");
    send_to_char(buf2, ch);
  }

  /* Sleep/Rest/Sit flag                              */
  /*--------------------------------------------------*/
  switch ( ch->position ) {
  case POS_SLEEPING:
    buf2[0] = '\0';
    sprintf(buf2,"({m%s{x)", "sleep");
    send_to_char(buf2, ch);
    break;
  case POS_RESTING:
    buf2[0] = '\0';
    sprintf(buf2,"({m%s{x)", "rest");
    send_to_char(buf2, ch);
    break;
  case POS_SITTING:
    buf2[0] = '\0';
    sprintf(buf2,"({m%s{x)", "sit");
    send_to_char(buf2, ch);
    break;
  }
  
  if (ch->prefix[0] != '\0')
    write_to_buffer(ch->desc,ch->prefix,0);
  else {
    send_to_char( "\n\r>", ch );
  }
  return;
}



/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length )
{
    /*
     * Find length in case caller didn't.
     */
    if ( length <= 0 )
	length = strlen(txt);

    /*
     * Initial \n\r if needed.
     */
    if ( d->outtop == 0 && !d->fcommand )
    {
	d->outbuf[0]	= '\n';
	d->outbuf[1]	= '\r';
	d->outtop	= 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while ( d->outtop + length >= d->outsize )
    {
	char *outbuf;

        if (d->outsize >= 32000)
	{
	    bug("Buffer overflow. Closing.\n\r",0);
	    close_socket(d);
	    return;
 	}
	outbuf      = alloc_mem( 2 * d->outsize );
	strncpy( outbuf, d->outbuf, d->outtop );
	free_mem( d->outbuf, d->outsize );
	d->outbuf   = outbuf;
	d->outsize *= 2;
    }

    /*
     * Copy.
     */
    strncpy( d->outbuf + d->outtop, txt, length );
    d->outtop += length;
    return;
}



/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor( int desc, char *txt, int length )
{
    int iStart;
    int nWrite;
    int nBlock;

#if defined(macintosh) || defined(MSDOS)
    if ( desc == 0 )
	desc = 1;
#endif

    if ( length <= 0 )
	length = strlen(txt);

    for ( iStart = 0; iStart < length; iStart += nWrite )
    {
	nBlock = UMIN( length - iStart, 4096 );
	if ( ( nWrite = write( desc, txt + iStart, nBlock ) ) < 0 )
	    { perror( "Write_to_descriptor" ); return FALSE; }
    } 

    return TRUE;
}



/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny( DESCRIPTOR_DATA *d, char *argument )
{
    DESCRIPTOR_DATA *d_old, *d_next;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
    int iClass,race,i,weapon;
    bool fOld;
    int col=0;
    int cnt=0;

    /* Delete leading spaces UNLESS character is writing a note */
    if (d->connected != CON_NOTE_TEXT)
    {
        while ( isspace(*argument) )
            argument++;
    }

    ch = d->character;

    switch ( d->connected )
    {

    default:
	bug( "Nanny: bad d->connected %d.", d->connected );
	close_socket( d );
	return;

    case CON_GET_NAME:
	if ( argument[0] == '\0' )
	{
//	    close_socket( d );
	    //return;
	    strcpy(argument,"bad name");
	}

	argument[0] = UPPER(argument[0]);
	if ( !check_parse_name( argument ) )
	{
	    write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
	    return;
	}

	fOld = load_char_obj( d, argument, FALSE );
	if (!fOld)
	  fOld = load_char_obj( d, argument, TRUE );

	ch   = d->character;

	//Cap HP and endur at the MAX
	if (!IS_NPC(ch) && (!IS_FORSAKEN(ch) && (!IS_IMMORTAL(ch))))
        {
            if (ch->pcdata->perm_hit >= MAX_TOTAL_HP)
                ch->pcdata->perm_hit = MAX_TOTAL_HP;
            if (ch->pcdata->perm_endurance >= MAX_TOTAL_ENDURANCE)
                ch->pcdata->perm_endurance = MAX_TOTAL_ENDURANCE;
            if (ch->max_hit >= MAX_TOTAL_HP)
                ch->max_hit = MAX_TOTAL_HP;
            if (ch->max_endurance >= MAX_TOTAL_ENDURANCE)
                ch->max_endurance = MAX_TOTAL_ENDURANCE;
        }


	if (IS_SET(ch->act, PLR_DENY))
	{
	    sprintf( log_buf, "Denying access to %s@%s.", argument, d->host );
	    log_string( log_buf );
	    write_to_buffer( d, "You are denied access.\n\r", 0 );
	    close_socket( d );
	    return;
	}

	if (check_ban(d->host,BAN_PERMIT) && !IS_SET(ch->act,PLR_PERMIT))
	{
	    write_to_buffer(d,"Your site has been banned from this mud.\n\r",0);
	    close_socket(d);
	    return;
	}

	if (ch->pcdata->timeoutstamp > 0)
        {
		if (ch->pcdata->timeoutstamp < current_time)
		{
			ch->pcdata->timeoutstamp = 0;
		}
		else
		{
			sprintf(buf,"Your character has been placed in Timeout until %s\r\n", (char *)ctime(&ch->pcdata->timeoutstamp) );
			write_to_buffer(d,buf,0);
			close_socket(d);
			return;

		}
	}

	if ( check_reconnect( d, argument, FALSE ) )
	{
	    fOld = TRUE;
	}
	else
	{
	    if ( wizlock && !IS_IMMORTAL(ch)) 
	    {
		write_to_buffer( d, "The game is wizlocked.\n\r", 0 );
		close_socket( d );
		return;
	    }
	}

	if ( fOld ) {
	  /* Old player */
	  write_to_buffer( d, "\n\rPassword: ", 0 );
	  write_to_buffer( d, echo_off_str, 0 );
	  d->connected = CON_GET_OLD_PASSWORD;
	  return;
	}
	else {

	  /* New player */
	  if (newlock) {
/*
	    write_to_buffer( d, "After being live with players for more than 15 years, The Shadow Wars is now moving on.\n\rSome will say it is long past time. Others will lament its passing.\n\r\n\rThe code was described as some of the best Wheel of Time depicting code in the Mud world by many of the players. Some of them wanted to give it a go themselves.\n\r\n\rThe Turning of the Wheel is a new mud which can be found at wheelturning.genesismuds.com port 1060.\n\r\n\rFor those that enjoy roleplaying, pk, and everything else associated with a Wheel of Time Mud, I highly recommend the place. Good luck to all who played here.\n\r\n\r",0);
*/
	    write_to_buffer( d, "We are sorry, but the game is currently newlocked due to system maintaince.\n\r"
					 "Please come back later.\n\r", 0 );
	    sprintf(buf, "New player detected but refused access (newlock is ON): %s %s", d->character->name,d->host);
	    wiznet(buf,ch,NULL,WIZ_LOGINS,0,get_trust(ch));
	    close_socket( d );
	    return;
	  }
	  
	  if (check_ban(d->host,BAN_NEWBIES)) {
	    write_to_buffer(d, "New players are not allowed from your site right now.\n\r",0);
	    sprintf(buf, "New player detected but refused access (site is banned): %s %s", d->character->name,d->host);
	    wiznet(buf,ch,NULL,WIZ_LOGINS,0,get_trust(ch));
	    close_socket(d);
	    return;
	  }
	  
	  sprintf( buf, "Did I get that right, %s (Y/N)? ", argument );
	  write_to_buffer( d, buf, 0 );
	  d->connected = CON_CONFIRM_NEW_NAME;
	  return;
	}
	break;

    case CON_GET_OLD_PASSWORD:
#if defined(unix)
	write_to_buffer( d, "\n\r", 2 );
#endif

	if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd )) {
	  write_to_buffer( d, "Wrong password.\n\r", 0 );
	  sprintf(buf, "Wrong password: %s %s", d->character->name,d->host);
	  wiznet(buf,ch,NULL,WIZ_LOGINS,0,get_trust(ch));
	  
           if (d->character->pet) {
               CHAR_DATA *pet=d->character->pet;

               char_to_room(pet,get_room_index( ROOM_VNUM_LIMBO));
               stop_follower(pet);
               extract_char(pet,TRUE,FALSE);
           } 	  
           else if (d->character->mount) {
               CHAR_DATA *mount=d->character->mount;

               char_to_room(mount,get_room_index( ROOM_VNUM_LIMBO));
               stop_follower(mount);
               extract_char(mount,TRUE,FALSE);
           } 	             
	  
	  close_socket( d );
	  return;
	}
 
	write_to_buffer( d, echo_on_str, 0 );

	if (check_playing(d,ch->name))
	    return;

	if ( check_reconnect( d, ch->name, TRUE ) )
	    return;

         ch->pcdata->lastlog	= str_dup( (char *)ctime(&current_time) );
	if (!IS_SET( ch->act, PLR_COLOUR ) )
        {
           write_to_buffer( d, "Do you want to use ANSI COLOR (Y/N)?: ", 0);
	   d->connected = CON_ASK_COLOUR;
	   return;
           break;
        }

    case CON_ASK_COLOUR:

	if (!IS_SET( ch->act, PLR_COLOUR ) ) {
	  if ( argument[0] == '\0' ) {
	    write_to_buffer( d, "Do you want to use ANSI COLOR (Y/N)?: ", 0);
	    return;
	  }
	  
	  argument[0] = UPPER(argument[0]);
	  
	  if (argument[0] == 'Y') {
	    SET_BIT( ch->act, PLR_COLOUR );
	  }
	  else if (argument[0] == 'N') {
	    REMOVE_BIT( ch->act, PLR_COLOUR );
	  }
	  else {  
	    write_to_buffer( d, "Do you want to use ANSI COLOR (Y/N)?: ", 0);
	    return;        
	  }
	}
	sprintf( log_buf, "%s@%s has connected.", ch->name, d->host );
	log_string( log_buf );
        sprintf( log_buf, "%s has connected.", ch->name);
	wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));
	do_bondnotify(ch,"arriving");

	/* If no appearance                                  */
	if ( (strlen(ch->pcdata->appearance) < 2) || (ch->pcdata->appearance[0] == '\0')) {
	  do_function(ch, &do_help, "pc_missing_entry" );
	  d->connected=CON_GET_APPEARANCE;
	  break;
	}
	
	/* If Old players haven't email adr. saved up */
	if (ch->pcdata->email[0]=='\0') {
	  do_function(ch, &do_help, "pc_missing_entry" );
	  d->connected=CON_GET_EMAIL;
	  break;
	}

	if ( IS_IMMORTAL(ch) ) {
	  do_function(ch, &do_help, "imotd" );
	  d->connected = CON_READ_IMOTD;
	}
	else {
	  do_function(ch, &do_help, "motd" );
	  d->connected = CON_READ_MOTD;
	}
	break;

    case CON_GET_APPEARANCE:
	 if (argument[0]=='\0' || strlen(argument) > 35 || strlen(argument) < 5) {
	   do_function(ch, &do_help, "appearance" );
	   write_to_buffer(d,"\n\rPlease write your appearance: ",0);
	   return;
	 }
	 //free_string(d->character->pcdata->appearance);
	 set_appearance(d->character, capitalize(colorstrem(argument)));

	 if (ch->pcdata->email[0]=='\0') {
	   do_function(ch, &do_help, "email" );
        if (strlen(d->host)<200) {
		sprintf(tmp_buff,"Example: Address@%s\n\r",d->host);
		write_to_buffer(d,tmp_buff,0);
	   }
	   write_to_buffer(d,"Please enter your email address:  ",0);
	   d->connected = CON_GET_EMAIL;
	   return;
	 }
	 else {
	   if ( IS_IMMORTAL(ch) ) {
		do_function(ch, &do_help, "imotd" );
		d->connected = CON_READ_IMOTD;
	   }
	   else {
		do_function(ch, &do_help, "motd" );
		d->connected = CON_READ_MOTD;
	   }
	 }
	 break;

    case CON_GET_EMAIL:
	 if (argument[0]=='\0' || !(!str_cmp(argument,"none") || strstr(argument,"@"))) {
	   do_function(ch, &do_help, "email" );
	   if (strlen(d->host)<200) {
     	sprintf(tmp_buff,"Example: Address@%s\n\r",d->host);
     	write_to_buffer(d,tmp_buff,0);
	   }
	   write_to_buffer(d,"Please enter your email address:  ",0);
	   return;
	 }
	 free_string(d->character->pcdata->email);
	 d->character->pcdata->email=str_dup(argument);

	 SET_BIT( d->character->act, PLR_COLOUR );
	 
	 if ( IS_IMMORTAL(ch) ) {
	   do_function(ch, &do_help, "imotd" );
	   d->connected = CON_READ_IMOTD;
	 }
	 else {
	   do_function(ch, &do_help, "motd" );
	   d->connected = CON_READ_MOTD;
	 }
	 
	 break;	 

/* RT code for breaking link */
 
    case CON_BREAK_CONNECT:
	switch( *argument )
	{
	case 'y' : case 'Y':
            for ( d_old = descriptor_list; d_old != NULL; d_old = d_next )
	    {
		d_next = d_old->next;
		if (d_old == d || d_old->character == NULL)
		    continue;

		if (str_cmp(ch->name,d_old->original ?
		    d_old->original->name : d_old->character->name))
		    continue;

		close_socket(d_old);
	    }
	    if (check_reconnect(d,ch->name,TRUE))
	    	return;
	    write_to_buffer(d,"Reconnect attempt failed.\n\rName: ",0);
            if ( d->character != NULL )
            {
                free_char( d->character );
                d->character = NULL;
            }
	    d->connected = CON_GET_NAME;
	    break;

	case 'n' : case 'N':
	    write_to_buffer(d,"Name: ",0);
            if ( d->character != NULL )
            {
                free_char( d->character );
                d->character = NULL;
            }
	    d->connected = CON_GET_NAME;
	    break;

	default:
	    write_to_buffer(d,"Please type Y or N? ",0);
	    break;
	}
	break;

    case CON_GET_FACELESS:
	switch( *argument) {
		case 'y' : case 'Y':
			write_to_buffer(d,"\n\r",0);
			SET_BIT(ch->act2,PLR2_FACELESS);
        		write_to_buffer( d, echo_on_str, 0 );
        		write_to_buffer(d,"The following races are available for you:\n\n\r",0);
        		buf[0] = '\0';
        		for ( race = 1; race_table[race].name != NULL; race++ ) {
          		if (!race_table[race].pc_race)
            				break;
		
		/*        if (race_table[race].granted) */
		/*          break; */
          		else {
            		if (IS_FADE_GRANTED(ch) && !str_cmp(race_table[race].name, "Fade")) {
                    		sprintf( buf, "%-14s \t", capitalize(race_table[race].name));
                    		write_to_buffer( d, buf, 0 );
                    		col++;
                    		if (col == 3) {
                      		write_to_buffer(d,"\n\r",0);
                      		col=0;
                    		}
            		}
            		else if (!race_table[race].granted) {
                   		sprintf( buf, "%-14s \t", capitalize(race_table[race].name));
                   		write_to_buffer( d, buf, 0 );
                   		col++;
                   		if (col == 3) {
                   		write_to_buffer(d,"\n\r",0);
                   		col=0;
                   		}
            		}
          		}
        		}
        		write_to_buffer(d,"\n\n\r",0);
        		write_to_buffer(d,"What is your race (help for more information)? ",0);
        		d->connected = CON_GET_NEW_RACE;

			break;
		case 'n': case 'N':
			write_to_buffer(d,"\n\r",0);
	   		do_function(ch, &do_help, "pc_new_name");
	   		sprintf(buf, "\n\rPlease provide a password for %s: %s", ch->name, echo_off_str );
	   		write_to_buffer( d, buf, 0 );
	   		d->connected = CON_GET_NEW_PASSWORD;
			break;
		default:
			write_to_buffer( d, "Please type Yes or No. ",0);
			break;

	}
	break;
    case CON_CONFIRM_NEW_NAME:
	 switch ( *argument ) {
	 case 'y': case 'Y':
	   /* sprintf( buf, "New character.\n\rGive me a password for %s: %s",
			  ch->name, echo_off_str ); */
	   
/*
			write_to_buffer(d,"\n\r",0);
	   		do_function(ch, &do_help, "pc_new_name");
	   		sprintf(buf, "\n\rPlease provide a password for %s: %s", ch->name, echo_off_str );
	   		write_to_buffer( d, buf, 0 );
	   		d->connected = CON_GET_NEW_PASSWORD;
			break;
*/
			write_to_buffer(d,"\n\r",0);
	   		do_function(ch, &do_help, "pc_new_name");
	   		sprintf(buf, "\n\rPlease provide a password for %s: %s", ch->name, echo_off_str );
	   		write_to_buffer( d, buf, 0 );
	   		d->connected = CON_GET_NEW_PASSWORD;
			break;
	 case 'n': case 'N':
	   write_to_buffer( d, "Ok, what IS it, then? ", 0 );
	   free_char( d->character );
	   d->character = NULL;
	   d->connected = CON_GET_NAME;
	   break;
	   
	 default:
	   write_to_buffer( d, "Please type Yes or No? ", 0 );
	   break;
	 }
	 break;

    case CON_GET_NEW_PASSWORD:
#if defined(unix)
	write_to_buffer( d, "\n\r", 2 );
#endif

	if ( strlen(argument) < 5 )
	{
	    write_to_buffer( d,
		"Password must be at least five characters long.\n\rPassword: ",
		0 );
	    return;
	}

	pwdnew = crypt( argument, ch->name );
	for ( p = pwdnew; *p != '\0'; p++ )
	{
	    if ( *p == '~' )
	    {
		write_to_buffer( d,
		    "New password not acceptable, try again.\n\rPassword: ",
		    0 );
		return;
	    }
	}

	free_string( ch->pcdata->pwd );
	ch->pcdata->pwd	= str_dup( pwdnew );
	write_to_buffer( d, "Please retype password: ", 0 );
	d->connected = CON_CONFIRM_NEW_PASSWORD;
	break;

    case CON_CONFIRM_NEW_PASSWORD:
#if defined(unix)
	write_to_buffer( d, "\n\r", 2 );
#endif

	if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) )
	{
	    write_to_buffer( d, "Passwords don't match.\n\rRetype password: ",
		0 );
	    d->connected = CON_GET_NEW_PASSWORD;
	    return;
	}

	write_to_buffer( d, echo_on_str, 0 );
	write_to_buffer(d,"The following races are available for you:\n\n\r",0);
	buf[0] = '\0';
	for ( race = 1; race_table[race].name != NULL; race++ ) {
	  if (!race_table[race].pc_race)
	    break;

/* 	  if (race_table[race].granted) */
/* 	    break; */
	  else {
	    if (IS_FADE_GRANTED(ch) && !str_cmp(race_table[race].name, "Fade")) {
		    sprintf( buf, "%-14s \t", capitalize(race_table[race].name));
		    write_to_buffer( d, buf, 0 );
		    col++;
		    if (col == 3) {
		      write_to_buffer(d,"\n\r",0);
		      col=0;
		    }	      
	    }
	    else if (!race_table[race].granted) {
		   sprintf( buf, "%-14s \t", capitalize(race_table[race].name));
		   write_to_buffer( d, buf, 0 );
		   col++;
		   if (col == 3) {
    		   write_to_buffer(d,"\n\r",0);
	   	   col=0;
		   }
	    }
	  }
	}
	write_to_buffer(d,"\n\n\r",0);
	write_to_buffer(d,"What is your race (help for more information)? ",0);
	d->connected = CON_GET_NEW_RACE;
	break;

    case CON_GET_NEW_RACE:
	one_argument(argument,arg);

	if (!strcmp(arg,"help"))
	{
	    argument = one_argument(argument,arg);
	    if (argument[0] == '\0')
		do_function(ch, &do_help, "race help");
	    else
		do_function(ch, &do_help, argument);
            write_to_buffer(d,
		"What is your race (help for more information)? ",0);
	    break;
  	}

	race = race_lookup(argument);

	if (race == 0 || !race_table[race].pc_race || race_table[race].granted ) {	
	  if (IS_FADE_GRANTED(ch) && !str_cmp(race_table[race].name, "Fade")) {
	   col = 0;
	  }
	  else {
	  write_to_buffer(d,"That is not a valid race.\n\r",0);
	  write_to_buffer(d,"The following races are available:\n\n\r",0);
	  buf[0] = '\0';
	  col=0;
	  for ( race = 1; race_table[race].name != NULL; race++ ) {

	    if (!race_table[race].pc_race) {
		 break;
	    }

	    else {
	    if (IS_FADE_GRANTED(ch) && !str_cmp(race_table[race].name, "Fade")) {
		    sprintf( buf, "%-14s \t", capitalize(race_table[race].name));
		    write_to_buffer( d, buf, 0 );
		    col++;
		    if (col == 3) {
		      write_to_buffer(d,"\n\r",0);
		      col=0;
		    }	      
	    }
		 else if (!race_table[race].granted) {
		   sprintf( buf, "%-14s \t", capitalize(race_table[race].name));
		   write_to_buffer( d, buf, 0 );
		   col++;
		   if (col == 3) {
			write_to_buffer(d,"\n\r",0);
			col=0;
		   }
		 }
	    }
	  }
	  write_to_buffer(d,"\n\n\r",0);
	  write_to_buffer(d,
		"What is your race? (help for more information) ",0);
	  break;
	}
	}
	
	ch->race = race;

	/* initialize stats */
	if (ch->pcdata->keepoldstats == FALSE) {
	  for (i = 0; i < MAX_STATS; i++) {
	    ch->perm_stat[i] = race_table[race].stats[i];
	  }
	  ch->main_sphere = -1;
     	  ch->pcdata->stat_points      = race_table[ch->race].stat_points;
     	  ch->pcdata->misc_points      = race_table[ch->race].misc_points;

	}
	
	ch->affected_by = ch->affected_by|race_table[race].aff;
	ch->imm_flags	= ch->imm_flags|race_table[race].imm;
	ch->res_flags	= ch->res_flags|race_table[race].res;
	ch->vuln_flags	= ch->vuln_flags|race_table[race].vuln;
	ch->form	= race_table[race].form;
	ch->parts	= race_table[race].parts;
	
	/* New characters need to be validated to get access */
        /* WHile in test mode, we'll let them get by without validation */

	SET_BIT(ch->comm,COMM_NOEMOTE);
	SET_BIT(ch->comm,COMM_NOTELL);
	SET_BIT(ch->comm,COMM_NOCHANNELS);
	SET_BIT(ch->comm,COMM_NOSHOUT);
	SET_BIT(ch->act, PLR_UNVALIDATED);

	/* add skills */
	for (i = 0; i < 5; i++) {
#if defined(FIRST_BOOT)
	  if (pc_race_table[race].skills[i] == NULL)
	    break;
	  group_add(ch,pc_race_table[race].skills[i],FALSE);
	}
	/* add cost */
	ch->pcdata->points = pc_race_table[race].points;
	ch->size = pc_race_table[race].size;
#else
	if (race_table[race].skills[i] == NULL)
	  break;
	group_add(ch,race_table[race].skills[i],FALSE);
    }
    /* add cost */
    if (ch->pcdata->keepoldstats == FALSE) {
       ch->pcdata->points = race_table[race].points;
    }
    ch->size = race_table[race].size;
#endif

    if ((!strcmp(race_table[ch->race].name,"trolloc")) )
    {
	ch->pcdata->true_sex = SEX_MALE;
	SET_BIT(ch->act2,PLR2_PKILLER);
   	strcpy( buf, "All Trollocs are Male.\n\n\rSelect a class:\n\n\r" );
   	for ( iClass = 0; iClass < MAX_CLASS; iClass++ ) {
		if (race_table[ch->race].class_mult[iClass] == 0)
	 	 	continue;
		if ( iClass > 0 )
	  	strcat( buf, "" );
		strcat( buf, capitalize(class_table[iClass].name));
		strcat( buf, "\n\r");
   	}
   	strcat( buf, "\nWhat is your class ? " );
        write_to_buffer( d, buf, 0 );
        d->connected = CON_GET_NEW_CLASS;
	break;
    }
    write_to_buffer( d, "What is your sex (M/F)? ", 0 );
    d->connected = CON_GET_NEW_SEX;
    break;
    
    
 case CON_GET_NEW_SEX:
   switch ( argument[0] ) {
   case 'm': case 'M': ch->sex = SEX_MALE;    
	ch->pcdata->true_sex = SEX_MALE;
	break;
   case 'f': case 'F': ch->sex = SEX_FEMALE; 
	ch->pcdata->true_sex = SEX_FEMALE;
	break;
   default:
	write_to_buffer( d, "That's not a sex.\n\rWhat IS your sex? ", 0 );
	return;
   }
   
   strcpy( buf, "Select a class:\n\n\r" );
   for ( iClass = 0; iClass < MAX_CLASS; iClass++ ) {
	if (race_table[ch->race].class_mult[iClass] == 0)
	  continue;
	if ((!strcmp(race_table[ch->race].name,"aiel") || (!strcmp(race_table[ch->race].name,"seanchan")) || (!strcmp(race_table[ch->race].name,"malkieri"))) //Don't allow Male MC's
	    && (ch->sex == SEX_MALE)
	    && (!strcmp(class_table[iClass].name,"channeler")))
	    {
		continue;
	    };
	if ( iClass > 0 )
	  strcat( buf, "" );
	strcat( buf, capitalize(class_table[iClass].name));
	strcat( buf, "\n\r");
   }
   strcat( buf, "\nWhat is your class ? " );
   write_to_buffer( d, buf, 0 );
   d->connected = CON_GET_NEW_CLASS;
   break;

 case CON_GET_NEW_CLASS:
   iClass = class_lookup(argument);
   
   if ( iClass == -1 || (race_table[ch->race].class_mult[iClass] == 0)) {
	write_to_buffer( d, "That's not a class.\n\rWhat IS your class? ", 0 );
	return;
   }
   if (((!strcmp(race_table[ch->race].name,"aiel"))  || //Don't allow Male MC's
	(!strcmp(race_table[ch->race].name,"malkieri")))
	 && (ch->sex == SEX_MALE)
	 && (!strcmp(class_table[iClass].name,"channeler")))
   {
   	write_to_buffer( d, "That's an invalid selection.\n\rWhat IS your class? ", 0);
	return;
   }
   ch->class = iClass;

   sprintf( log_buf, "%s@%s new player.", ch->name, d->host );
   log_string( log_buf );
   /* wiznet("Newbie alert! $N sighted.",ch,NULL,WIZ_NEWBIE,0,0); */
   wiznet("New player $N has entered character creation.",ch,NULL,WIZ_NEWBIE,0,0);
   wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));
   
   /* Set creation points according to defined in race setup */
   set_creation_record(ch);
   
   /* Show main creation menu and go to selection case       */
   if (!IS_FACELESS(ch))
   {
   	selection_menu(ch);
   	d->connected = CON_SELECTION_MENU;
   }
   else
   {
	gen_faceless_template(ch);
	do_function(ch, &do_help, "appearance" );
	write_to_buffer(d,"\n\rPlease write your appearance: ",0);
	d->connected=CON_GET_APPEARANCE;
   }
   break;

 case CON_SELECTION_MENU:
   write_to_buffer( d, "\n\r", 2 );
   selection_menu(ch);

   /* SPHERES */
   if (ch->class == CLASS_CHANNELER) {
	if (!str_cmp(argument, "spheres")) {
	  if (ch->pcdata->keepoldstats && ch->pcdata->prev_class == CLASS_CHANNELER) {
	    send_to_char("You elected to keep your old stats\r\n",ch);
	    break;  
	  }
	  sphere_menu(ch);
	  d->connected = CON_GET_SPHERES;
	  break;
	}
   }

   /* STATS */
   if (!str_cmp(argument, "stats")) {
	if (ch->pcdata->keepoldstats) {
     	  send_to_char("You elected to keep your old stats\r\n",ch);
	  break;  
        }
	stat_menu(ch);
	d->connected = CON_GET_STATS;
	break;
   }

   /* MERITS */
   if (!str_cmp(argument, "merits")) {
	merit_menu(ch);
	d->connected = CON_GET_MERITS;
	break;
   }

   /* FLAWS */
   if (!str_cmp(argument, "flaws")) {
	flaw_menu(ch);
	d->connected = CON_GET_FLAWS;
	break;
   }

   /* TALENTS */
   if (!str_cmp(argument, "talents")) {
	talent_menu(ch);
	d->connected = CON_GET_TALENTS;
	break;
   }

   /* HELP */
   if (!str_cmp(argument, "help")) {
	do_function(ch, &do_help, "pc_selection_menu");
   }

   /* DONE */
   if (!str_cmp(argument, "done")) {

        int total = (get_merit_costs(ch) + get_flaw_costs(ch) + get_talent_costs(ch));
	if (total > race_table[ch->race].misc_points)
	{
		write_to_buffer(d,"\n\rYou have used too many points.. Please correct this before continueing!\r\n",0);
	        break;
	}

	/* Do the dice */
	if(ch->class == CLASS_CHANNELER) {
	  if (ch->pcdata->keepoldstats == FALSE && ch->cre_sphere[0] == 0) {
	    dice_spheres(ch);
	    int attempt = 0;
		while( SPHERE_TOTAL(ch) < MIN_SPHERE_SUM  && attempt < 10)
		{	dice_spheres(ch) ;
			attempt++;
		}
	  }
	  if (ch->pcdata->keepoldstats == TRUE && ch->pcdata->prev_class != CLASS_CHANNELER) {
	    dice_spheres(ch);
	    int attempt = 0;
		while( SPHERE_TOTAL(ch) < MIN_SPHERE_SUM  && attempt < 10)
		{	dice_spheres(ch) ;
			attempt++;		
		}
	  }
	}
	
	write_to_buffer( d, "\n\r", 2 );
	write_to_buffer(d, "Please pick a weapon from the following choices:\n\n\r",0);
	buf[0] = '\0';
	col = 0;
	for ( i = 0; weapon_table[i].name != NULL; i++) {
	  
/* Allow all to pick what weapon they want: Atwain
   if (ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
   strcat(buf,weapon_table[i].name);
   strcat(buf," ");
   }
*/
	  sprintf( buf, "%-14s \t", capitalize(weapon_table[i].name));
	  write_to_buffer( d, buf, 0 );
	  col++;
	  if (col == 3) {
	    write_to_buffer(d,"\n\r",0);
	    col = 0;
	  }
	}
	write_to_buffer(d,"\n\n\rYour choice? ",0);
	d->connected = CON_PICK_WEAPON;
   }
   
   break;

 case CON_GET_STATS:
   argument = one_argument( argument, arg );

   write_to_buffer( d, "\n\r", 2 );
   stat_menu(ch);

   /* SET */
   if (!str_cmp(arg, "set")) {
	set_stat(ch, argument);
	stat_menu(ch);
	break;
   }

   /* HELP */
   if (!str_cmp(arg, "help")) {
	do_function(ch, &do_help, "pc_stat_menu");
	break;
   }

   /* DONE */
   if (!str_cmp(arg, "done")) {
	write_to_buffer( d, "\n\r", 2 );
	selection_menu(ch);
	d->connected = CON_SELECTION_MENU;
	break;
   }

   break;

 case CON_GET_SPHERES:
   
   argument = one_argument( argument, arg );

   write_to_buffer( d, "\n\r", 2 );
   sphere_menu(ch);
   
   /* Hidden sphere sum adjustment */
   if (!str_cmp(arg, "spheresum")) {
	argument = one_argument( argument, arg);
	if (!str_cmp(arg,"show"))
	{
		int spheresum = 0;
		for(i = 0; i < MAX_SPHERE; i++) {
			spheresum+= ch->cre_sphere[i];
		}

		char buff[MAX_STRING_LENGTH];
		sprintf(buff,"\r\nYour Spheres: AIR[%d], EARTH[%d], FIRE[%d], SPIRIT[%d], WATER[%d]: Total[%d]\r\n",ch->cre_sphere[SPHERE_AIR],ch->cre_sphere[SPHERE_EARTH],ch->cre_sphere[SPHERE_FIRE],ch->cre_sphere[SPHERE_SPIRIT],ch->cre_sphere[SPHERE_WATER],spheresum);
		write_to_buffer(d,buff,0);
		break;
	}
	if (!strcmp(arg,"reroll"))
	{
		char buff[MAX_STRING_LENGTH];
		dice_spheres(ch);
		while( SPHERE_TOTAL(ch) < MIN_SPHERE_SUM )
		{	dice_spheres(ch) ;	}
		int spheresum = 0;
		for(i = 0; i < MAX_SPHERE; i++) {
			spheresum+= ch->cre_sphere[i];
		}

		sprintf(buff,"\r\nYour Spheres: AIR[%d], EARTH[%d], FIRE[%d], SPIRIT[%d], WATER[%d]: Total[%d]\r\n",ch->cre_sphere[SPHERE_AIR],ch->cre_sphere[SPHERE_EARTH],ch->cre_sphere[SPHERE_FIRE],ch->cre_sphere[SPHERE_SPIRIT],ch->cre_sphere[SPHERE_WATER],spheresum);
		write_to_buffer(d,buff,0);
		break;
	}

   }
   /* SET */
   if (!str_cmp(arg, "set")) {
	set_sphere(ch, argument);
	sphere_menu(ch);
	break;
   }

   /* HELP */
   if (!str_cmp(arg, "help")) {
	do_function(ch, &do_help, "pc_sphere_menu");
	break;
   }

   /* ROLL JUST FOR TESTING */
   if (!str_cmp(arg, "roll")) {
	dice_spheres(ch);
	while( SPHERE_TOTAL(ch) < MIN_SPHERE_SUM )
	{	dice_spheres(ch) ;	}
	break;
   }
   
   /* DONE */
   if (!str_cmp(arg, "done")) {
	write_to_buffer( d, "\n\r", 2 );
	selection_menu(ch);
	d->connected = CON_SELECTION_MENU;

/*
	write_to_buffer( d, "\n\r", 2 );
	write_to_buffer(d, "Please pick a weapon from the following choices:\n\r",0);
	buf[0] = '\0';
	for ( i = 0; weapon_table[i].name != NULL; i++)
	  if (ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
	    strcat(buf,weapon_table[i].name);
	    strcat(buf," ");
	  }
	strcat(buf,"\n\rYour choice? ");
	write_to_buffer(d,buf,0);
	d->connected = CON_PICK_WEAPON;
*/
   }
   break;

case CON_GET_MERITS:
   argument = one_argument( argument, arg );

   write_to_buffer( d, "\n\r", 2 );
   merit_menu(ch);
   
   /* ADD */
   if (!str_cmp(arg, "add")) {
	add_merit(ch, argument);
	merit_menu(ch);
	break;
   }

   /* DROP */
   if (!str_cmp(arg, "drop")) {
	drop_merit(ch, argument);
	merit_menu(ch);
	break;
   }

   /* HELP */
   if (!str_cmp(arg, "help")) {
	char arg[MAX_INPUT_LENGTH];
	
	argument = one_argument(argument, arg);
	
	if (IS_NULLSTR(arg))
	  do_function(ch, &do_help, "pc_merit_menu");
	else
	  do_function(ch, &do_help, arg);
	break;
   }

   /* DONE */
   if (!str_cmp(arg, "done")) {
	write_to_buffer( d, "\n\r", 2 );
	selection_menu(ch);
	d->connected = CON_SELECTION_MENU;
	break;
   }
   break;

case CON_GET_FLAWS:
   argument = one_argument( argument, arg );

   write_to_buffer( d, "\n\r", 2 );
   flaw_menu(ch);
   
   /* ADD */
   if (!str_cmp(arg, "add")) {
	add_flaw(ch, argument);
	flaw_menu(ch);
	break;
   }

   /* DROP */
   if (!str_cmp(arg, "drop")) {
	drop_flaw(ch, argument);
	flaw_menu(ch);
	break;
   }

   /* HELP */
   if (!str_cmp(arg, "help")) {
	char arg[MAX_INPUT_LENGTH];
	
	argument = one_argument(argument, arg);
	
	if (IS_NULLSTR(arg))
	  do_function(ch, &do_help, "pc_flaw_menu");
	else
	  do_function(ch, &do_help, arg);
	break;
   }

   /* DONE */
   if (!str_cmp(arg, "done")) {
	write_to_buffer( d, "\n\r", 2 );
	selection_menu(ch);
	d->connected = CON_SELECTION_MENU;
	break;
   }
   break;

 case CON_GET_TALENTS:
   argument = one_argument( argument, arg );

   write_to_buffer( d, "\n\r", 2 );
   talent_menu(ch);
   
   /* ADD */
   if (!str_cmp(arg, "add")) {
	add_talent(ch, argument);
	talent_menu(ch);
	break;
   }

   /* DROP */
   if (!str_cmp(arg, "drop")) {
	drop_talent(ch, argument);
	talent_menu(ch);
	break;
   }

   /* HELP */
   if (!str_cmp(arg, "help")) {
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);

	if (IS_NULLSTR(arg))
	  do_function(ch, &do_help, "pc_talent_menu");
	else
	  do_function(ch, &do_help, arg);
	break;
   }

   /* DONE */
   if (!str_cmp(arg, "done")) {
	write_to_buffer( d, "\n\r", 2 );
	selection_menu(ch);
	d->connected = CON_SELECTION_MENU;
	break;
   }
   break;

/*
 case CON_GET_NEW_CLASS:
   iClass = class_lookup(argument);
   
   if ( iClass == -1 || (race_table[ch->race].class_mult[iClass] == 0)) {
	write_to_buffer( d, "That's not a class.\n\rWhat IS your class? ", 0 );
	return;
   }
   
   ch->class = iClass;
   
   sprintf( log_buf, "%s@%s new player.", ch->name, d->host );
   log_string( log_buf );
   wiznet("Newbie alert!  $N sighted.",ch,NULL,WIZ_NEWBIE,0,0);
   wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));
   
   group_add(ch,"rom basics",FALSE);
   group_add(ch,class_table[ch->class].base_group,FALSE);
   ch->pcdata->learned[gsn_recall] = 50;
   write_to_buffer(d,"Do you wish to customize this character?\n\r",0);
   write_to_buffer(d,"Customization takes time, but allows a wider range of skills and abilities.\n\r",0);
   write_to_buffer(d,"Customize (Y/N)? ",0);
   d->connected = CON_DEFAULT_CHOICE;
   break;
*/
 case CON_DEFAULT_CHOICE:
   /* write_to_buffer(d,"\n\r",2); */
   switch ( argument[0] ) {
   case 'y': case 'Y': 
	ch->gen_data = new_gen_data();
	ch->gen_data->points_chosen = ch->pcdata->points;
	do_function(ch, &do_help, "group header");
	list_group_costs(ch);
	write_to_buffer(d,"You already have the following skills:\n\r",0);
	do_function(ch, &do_skills, "");
	do_function(ch, &do_help, "menu choice");
	d->connected = CON_GEN_GROUPS;
	break;
   case 'n': case 'N': 
	group_add(ch,class_table[ch->class].default_group,TRUE);
	write_to_buffer( d, "\n\r", 2 );
	write_to_buffer(d, "Please pick a weapon from the following choices:\n\r",0);
	buf[0] = '\0';
	for ( i = 0; weapon_table[i].name != NULL; i++)
	  if (ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
	    strcat(buf,weapon_table[i].name);
	    strcat(buf," ");
	  }
	strcat(buf,"\n\rYour choice? ");
	write_to_buffer(d,buf,0);
	d->connected = CON_PICK_WEAPON;
	break;
        default:
		write_to_buffer( d, "Please answer (Y/N)? ", 0 );
		return;
   }
   break;
   
 case CON_PICK_WEAPON:
   write_to_buffer(d,"\n\r",2);

   weapon = weapon_lookup(argument);
   if (weapon == -1) {
	write_to_buffer(d, "That's not a valid selection. Choices are:\n\n\r",0);
	buf[0] = '\0';
	col = 0;

	for ( i = 0; weapon_table[i].name != NULL; i++) {
	  sprintf( buf, "%-14s \t", capitalize(weapon_table[i].name));
	  write_to_buffer( d, buf, 0 );
	  col++;
	  if (col == 3) {
	    write_to_buffer(d,"\n\r",0);
	    col = 0;
	  }
	}
	write_to_buffer(d,"\n\n\rYour choice? ",0);
	return;
   }

/*
   weapon = weapon_lookup(argument);
   if (weapon == -1 || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0) {
	write_to_buffer(d, "That's not a valid selection. Choices are:\n\r",0);
	buf[0] = '\0';
	for ( i = 0; weapon_table[i].name != NULL; i++)
	  if (ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
	    strcat(buf,weapon_table[i].name);
	    strcat(buf," ");
	  }
	strcat(buf,"\n\rYour choice? ");
	write_to_buffer(d,buf,0);
	return;
   }
*/
   
   ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;
   write_to_buffer(d,"\n\r",2);
   
   do_function(ch, &do_help, "appearance");
   write_to_buffer(d,"\n\rPlease write your appearance: ",0);
   d->connected=CON_GET_APPEARANCE;
   return;
   break;
   
   
 case CON_GEN_GROUPS:
   send_to_char("\n\r",ch);

       	if (!str_cmp(argument,"done"))
       	{
#if defined(FIRST_BOOT)
	    if (ch->pcdata->points == pc_race_table[ch->race].points)
#else
	    if (ch->pcdata->points == race_table[ch->race].points)
#endif
	    {
	        send_to_char("You didn't pick anything.\n\r",ch);
		break;
	    }

#if defined(FIRST_BOOT)
	    if (ch->pcdata->points <= 40 + pc_race_table[ch->race].points)
#else
	    if (ch->pcdata->points <= 40 + race_table[ch->race].points)
#endif
	    {
		sprintf(buf,
		    "You must take at least %d points of skills and groups",
#if defined(FIRST_BOOT)
		    40 + pc_race_table[ch->race].points);
#else
		    40 + race_table[ch->race].points);
#endif
		send_to_char(buf, ch);
		break;
		}
		
		sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
	    send_to_char(buf,ch);
	    sprintf(buf,"Experience per level: %d\n\r",
	            exp_per_level(ch,ch->gen_data->points_chosen));
	    if (ch->pcdata->points < 40)
		ch->train = (40 - ch->pcdata->points + 1) / 2;
	    free_gen_data(ch->gen_data);
	    ch->gen_data = NULL;
	    send_to_char(buf,ch);
	    write_to_buffer( d, "\n\r", 2 );
	    write_to_buffer(d,
                "Please pick a weapon from the following choices:\n\r",0);
	    buf[0] = '\0';
	    for ( i = 0; weapon_table[i].name != NULL; i++)
                if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
                {
                    strcat(buf,weapon_table[i].name);
		    strcat(buf," ");
                }
	    strcat(buf,"\n\rYour choice? ");
	    write_to_buffer(d,buf,0);
	    d->connected = CON_PICK_WEAPON;
	    break;
        }

        if (!parse_gen_groups(ch,argument))
        send_to_char(
        "Choices are: list,learned,premise,add,drop,info,help, and done.\n\r"
        ,ch);

        do_function(ch, &do_help, "menu choice");
        break;

    case CON_READ_IMOTD:
	write_to_buffer(d,"\n\r",2);
        do_function(ch, &do_help, "motd");
        d->connected = CON_READ_MOTD;
	break;

        /* states for new note system, (c)1995-96 erwin@pip.dknet.dk */
        /* ch MUST be PC here; have nwrite check for PC status! */

    case CON_NOTE_TO:
        handle_con_note_to (d, argument);
        break;

    case CON_NOTE_SUBJECT:
        handle_con_note_subject (d, argument);
        break; /* subject */

    case CON_NOTE_EXPIRE:
        handle_con_note_expire (d, argument);
        break;

    case CON_NOTE_TEXT:
        handle_con_note_text (d, argument);
        break;

    case CON_NOTE_FINISH:
        handle_con_note_finish (d, argument);
        break;

    case CON_READ_MOTD:
        if ( ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0')
        {
	   if (!IS_FACELESS(ch))
           {
            write_to_buffer( d, "Warning! Null password!\n\r",0 );
            write_to_buffer( d, "Please report old password with bug.\n\r",0);
            write_to_buffer( d,
                "Type 'password null <new password>' to fix.\n\r",0);
	   }
        }

	write_to_buffer( d, "\n\rWelcome to The Shadow Wars.  Please do not feed the mobiles.\n\n\r",
	    0 );
	ch->next	= char_list;
	char_list	= ch;
	d->connected	= CON_PLAYING;
	reset_char(ch);

	if ( ch->level == 0 )
	{

	    if (ch->pcdata->keepoldstats == FALSE)
	       ch->perm_stat[class_table[ch->class].attr_prime] += 3;

	    if (IS_FACELESS(ch))
	    { ch->level = 10; }
	    else
	    { ch->level	    = 1;}
	    ch->pcdata->extended_level = 0;
	    if (ch->played < 7200)
	       ch->exp	    = 0;
  	    if (ch->exp > exp_next_level(ch)) {
    		SET_BIT(ch->act, PLR_CANLEVEL);
            }
	    ch->hit	    = ch->max_hit;
	    ch->endurance  = ch->max_endurance;
	    
	    /* Adjust hit and endur according to talents */
	    if (IS_SET(ch->merits, MERIT_TOUGH)) {
		 ch->hit     += 150;
		 ch->max_hit += 150;
	    }
	    if (IS_SET(ch->flaws, FLAW_WEAK)) {
		 ch->hit     -= 150;
		 ch->max_hit -= 150;
	    }
	    if (IS_SET(ch->merits, MERIT_LONGWINDED)) {
		 ch->endurance     += 150;
		 ch->max_endurance += 150;
	    }
	    if (IS_FACELESS(ch))
            {
		ch->endurance = 200;
		ch->max_endurance = 200;
		ch->hit = 200;
		ch->max_hit = 200;	
	    }
	    
	    ch->train	    = 0;

	    /* Set up base hit locations */
	    ch->hit_loc[LOC_LA] = get_max_hit_loc(ch, LOC_LA);
	    ch->hit_loc[LOC_LL] = get_max_hit_loc(ch, LOC_LL);
	    ch->hit_loc[LOC_HE] = get_max_hit_loc(ch, LOC_HE);
	    ch->hit_loc[LOC_BD] = get_max_hit_loc(ch, LOC_BD);
	    ch->hit_loc[LOC_RA] = get_max_hit_loc(ch, LOC_RA);
	    ch->hit_loc[LOC_RL] = get_max_hit_loc(ch, LOC_RL);
	 
	    /* Inside newbie school you don't have to worry about thirst and hunger */
            ch->pcdata->condition[COND_FULL] = -1;
	    ch->pcdata->condition[COND_THIRST] = -1;
	    ch->pcdata->condition[COND_HUNGER] = -1;

	    set_title( ch, ", spun out by the Pattern.");
 
	    /* Autogenerate note when new players create    */
	    /* Bit messy but it does the job                */
	    sprintf(buf,
			  "Race    : %s\n\r"
			  "Sex     : %s\n\r",
			  capitalize(race_table[ch->race].name),
			  ch->sex == 1 ? "Male" : "Female");
	    sprintf(tmp_buff,
			  "Class   : %s\n\r"
			  "App     : %s\n\n\r"
			  "Stats   : %d Str, %d Int, %d Wis, %d Dex, %d Con\n\r"
			  "Spheres : %d/%d A, %d/%d E, %d/%d F, %d/%d S, %d/%d W (M: %s) Tot:%d\n\r"
			  "Merits  : %s\n\r"
			  "Flaws   : %s\n\r"
			  "Talents : %s\n\n\r",
			  /* line feed */
			  capitalize(class_table[ch->class].name),
			  ch->pcdata->appearance,
			  get_curr_stat(ch,STAT_STR), get_curr_stat(ch,STAT_INT), 
			  get_curr_stat(ch,STAT_WIS), get_curr_stat(ch,STAT_DEX),
			  get_curr_stat(ch,STAT_CON),			  
			  ch->perm_sphere[SPHERE_AIR],    ch->cre_sphere[SPHERE_AIR],
			  ch->perm_sphere[SPHERE_EARTH],  ch->cre_sphere[SPHERE_EARTH],
			  ch->perm_sphere[SPHERE_FIRE],   ch->cre_sphere[SPHERE_FIRE],
			  ch->perm_sphere[SPHERE_SPIRIT], ch->cre_sphere[SPHERE_SPIRIT],
			  ch->perm_sphere[SPHERE_WATER],  ch->cre_sphere[SPHERE_WATER],
			  ch->main_sphere != -1 ? sphere_table[ch->main_sphere].name : "none",
			  (ch->cre_sphere[SPHERE_AIR] + ch->cre_sphere[SPHERE_EARTH] + ch->cre_sphere[SPHERE_FIRE] + ch->cre_sphere[SPHERE_SPIRIT] + ch->cre_sphere[SPHERE_WATER]),
			  background_flag_string(merit_table, ch->merits),
			  background_flag_string(flaw_table, ch->flaws),
			  background_flag_string(talent_table, ch->talents));

	    strcat(buf, tmp_buff);
	    sprintf(tmp_buff, "I am a new player! (%s)", class_table[ch->class].name);

	    //Remove old notes in Newplayer area from same player
	    int board_index = board_lookup ("Newplayers");
	    BOARD_DATA *board;
	    NOTE_DATA *note;
	    char *strtime;
	    board = &boards [board_index];
  	    int count = 0;
            NOTE_DATA *p = NULL;
            NOTE_DATA *p_prior = NULL;
            NOTE_DATA *p_next = NULL;
            ch->pcdata->board = &boards[board_index];

            for (p = board->note_first; p ; )
            {
                p_next = p->next;
                ++count;
		if (!str_cmp(p->sender, ch->name))
                {
			if (board->note_first == p)
                        {
				board->note_first = p->next;
                        }
			else
                        {
				p_prior->next = p->next;
                        }
			char buff[256];
			sprintf(buff,"Freeing Note: %d\n",count);
		        log_string(buff);
		        free_note(p);
		}
	        else 
                {
			p_prior = p;
                }
                p = p_next;
            }

	    make_note("Newplayers", ch->name, "Admin", tmp_buff, 56, buf);

	    /* Outfit the newbies */
	    do_function (ch, &do_outfit,"");

            /* Make Autoparry, dodge and shield block default */
            SET_BIT(ch->auto_act, AUTO_PARRY);
            SET_BIT(ch->auto_act, AUTO_DODGE);
            SET_BIT(ch->auto_act, AUTO_SHIELDBLOCK);

	    /* Make AutoVoteReminder Default */
            SET_BIT(ch->auto_act, AUTO_VOTEREMINDER);

	    /* Make Autoloot default */
	    SET_BIT(ch->act, PLR_AUTOLOOT);

	    /* Make Autosac default */
	    SET_BIT(ch->act, PLR_AUTOSAC);
	    
	    /* Make Autoexit default */
	    SET_BIT(ch->act,PLR_AUTOEXIT);

	    /* Make Autoslice default */
	    SET_BIT(ch->act, PLR_AUTOSLICE);
	    
	    /* Make color default */
	    SET_BIT( ch->act, PLR_COLOUR );

            /* Set default colors */
            default_colour(ch);

	    /* If channeler, set prompt*/
	    if (ch->class == CLASS_CHANNELER) {
		 buf[0] = '\0';
		 strcat(buf, "[{R%h{x/{R%H{xhp {G%m{x/{G%M{xen {W%p{x/{W%P{xop] ");
		 free_string( ch->prompt );
		 ch->prompt = str_dup( buf );
		 buf[0] = '\0';
	    }

       /* If Seanchan, add to TheEmpire guild */
       if (ch->race == race_lookup("seanchan")) {
          ch->clan = clan_lookup("TheEmpire");
	  ch->gtitle = str_dup("FreshMeat");
          ch->rank = 8;
       }
       else //Aiel autojoined to Aiel guild
       if (ch->race == race_lookup("aiel")) {
          ch->clan = clan_lookup("Aiel");
	  ch->gtitle = str_dup("Spear");
          ch->rank = 7;
       }
       
       /* If Fade, add to shadowspawn guild */
       if (ch->race == race_lookup("fade")) {
          ch->clan = clan_lookup("Shadowspawn");
          ch->rank = 4;

        //Set whoinvis default
        ch->incog_level = 1;
       }
       
       /* If Trolloc, add to shadowspawn guild and 1 sguild TR clan */
       if (ch->race == race_lookup("trolloc")) {
          ch->clan = clan_lookup("Shadowspawn");
          ch->rank = 8;
                   
          switch(number_range(0,3)) {
          	case 0:
          	   ch->sguild      = sguild_lookup("Ahffrait");
          	   ch->sguild_rank = 8;
         	   ch->gtitle      = str_dup("{yAh{Df'fra{yit{x Grunt");
          	break;
          	case 1:
          	   ch->sguild      = sguild_lookup("Bhansheen");
          	   ch->sguild_rank = 8;          	
         	   ch->gtitle      = str_dup("{cBh{Danshe{cen{x Grunt");          	   
          	break;
          	case 2:
          	   ch->sguild      = sguild_lookup("Kobal");
          	   ch->sguild_rank = 8;          	
         	   ch->gtitle      = str_dup("{rK{Do'ba{rl{x Grunt");          	   
          	break;          	
          	case 3:
          	   ch->sguild      = sguild_lookup("Dhavol");
          	   ch->sguild_rank = 8;          	
         	   ch->gtitle      = str_dup("{bD{Dha'vo{bl{x Grunt");          	   
          	break;          	
          	default:
          	   ch->sguild      = sguild_lookup("Ahffrait");
          	   ch->sguild_rank = 8;          	
         	   ch->gtitle      = str_dup("{yAh{Df'fra{yit{x Grunt");          	   
          	break;
        }
      }            
	    
	    char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
	    send_to_char("\n\r",ch);

	    do_function(ch, &do_help, "NEWBIE");
	    send_to_char("\n\r",ch);
    	    mp_greet_trigger( ch );
	}
	else if ( ch->in_room != NULL )
	{
	    char_to_room( ch, ch->in_room );
	}
	else if ( IS_IMMORTAL(ch) )
	{
	    char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
	}
	else
	{
	    char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
	}

  	if (ch->exp > exp_next_level(ch)) {
    	   SET_BIT(ch->act, PLR_CANLEVEL);
        }
	if (ch->invis_level < 2 && ch->incog_level < 2)
        {
		act( "$n has entered the game.", ch, NULL, NULL, TO_ROOM );
	}
	do_function(ch, &do_look, "auto" );
	do_function(ch, &do_poll,"new");

        buf[0] = '\0';
        sprintf(buf,"$N {x(Level: %d) has logged on.", get_level(ch));
        wiznet(buf,ch,NULL,WIZ_LOGINS,WIZ_SITES,get_trust(ch));

	if (ch->pet != NULL)
	{
	    char_to_room(ch->pet,ch->pet->in_room);
	    act("$n has entered the game.",ch->pet,NULL,NULL,TO_ROOM);
	}
     else if ( ch->mount != NULL) {
       char_to_room(ch->mount,ch->mount->in_room);
       act("$n has entered the game.",ch->mount,NULL,NULL,TO_ROOM);
	  add_follower(ch->mount, ch);
	  ch->mount->leader = ch;
	  if (ch->in_room == ch->mount->in_room) {
	      do_mount(ch, ch->mount->name);
          }
     }

 	send_to_char("\n", ch);
 	do_board(ch, "");  /* Show board status */

     if (!newlock)
       vote_click_check(ch);

        /* Write information about pray history to immortals */
        if (IS_IMMORTAL(ch)) {
           for (col=0;col<PRAYHISTSIZE;col++) {
              if (pray_history[col].player_name) {
              cnt++;
              }
           }
           if (cnt > 0) {
              sprintf(buf, "There %s {W%d{x %s in Pray history.\n\r", 
                           cnt == 1 ? "is" : "are", cnt,
                           cnt == 1 ? "entry" : "entries");
              send_to_char(buf, ch);
           }      
        }
        
        /* Write information about Immortal history to immortals */
        cnt=0;
        if (IS_IMMORTAL(ch)) {
           for (col=0;col<HISTSIZE;col++) {
              if (imm_history[col].player_name) {
              cnt++;
              }
           }
           if (cnt > 0) {
              sprintf(buf, "There %s {W%d{x %s in Immortal history.\n\r", 
                           cnt == 1 ? "is" : "are", cnt,
                           cnt == 1 ? "entry" : "entries");
              send_to_char(buf, ch);
           }   
        }   

	break;
    }

    return;
}



/*
 * Parse a name for acceptability.
 */
bool check_parse_name( char *name )
{
    int clan;

    /*
     * Reserved words.
     */
    if (is_exact_name(name,
	"all auto immortal self someone something the you loner none"))
    {
	return FALSE;
    }

    /*
     * Chosen etc.
     */
     if (is_exact_name(name,
     	/*"demandred aginor balthamel asmodean rahvin belal mesaana moghedien naeblis mordeth darkone ishamael graendal"))*/
     	"demandred aginor balthamel asmodean mesaana naeblis mordeth darkone graendal"))
     	{
     		return FALSE;
     	}

    /*
     * Rand etc..
     */
     if (is_exact_name(name,
     	"chosen creator rand perrin mat moiraine lan nynaeve aviendha taim verin egwene elayne suian loial logain morgase"))
     	{
     		return FALSE;
	}

    /* check idiots */
    /*
     * Rand etc..
     */
     if (is_exact_name(name,
     	"nigger niggerhater hater bitch"))
     	{
     		return FALSE;
	}


    /* check clans */
    for (clan = 0; clan < MAX_CLAN; clan++)
    {
	if (LOWER(name[0]) == LOWER(clan_table[clan].name[0])
	&&  !str_cmp(name,clan_table[clan].name))
	   return FALSE;
    }
	
    if (str_cmp(capitalize(name),"Alander") && (!str_prefix("Alan",name)
    || !str_suffix("Alander",name)))
	return FALSE;

    /*
     * Length restrictions.
     */
     
    if ( strlen(name) <  2 )
	return FALSE;

#if defined(MSDOS)
    if ( strlen(name) >  8 )
	return FALSE;
#endif

#if defined(macintosh) || defined(unix)
    if ( strlen(name) > 12 )
	return FALSE;
#endif

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
	char *pc;
	bool fIll,adjcaps = FALSE,cleancaps = FALSE;
 	int total_caps = 0;

	fIll = TRUE;
	for ( pc = name; *pc != '\0'; pc++ )
	{
	    if ( !isalpha(*pc) )
		return FALSE;

	    if ( isupper(*pc)) /* ugly anti-caps hack */
	    {
		if (adjcaps)
		    cleancaps = TRUE;
		total_caps++;
		adjcaps = TRUE;
	    }
	    else
		adjcaps = FALSE;

	    if ( LOWER(*pc) != 'i' && LOWER(*pc) != 'l' )
		fIll = FALSE;
	}

	if ( fIll )
	    return FALSE;

	if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
	    return FALSE;
    }

    /*
     * Prevent players from naming themselves after mobs.
     */
    {
	extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
	MOB_INDEX_DATA *pMobIndex;
	int iHash;

	for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
	{
	    for ( pMobIndex  = mob_index_hash[iHash];
		  pMobIndex != NULL;
		  pMobIndex  = pMobIndex->next )
	    {
		if ( is_name( name, pMobIndex->player_name ) )
		    return FALSE;
	    }
	}
    }

    return TRUE;
}



/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn )
{
  CHAR_DATA *ch;
  
  for ( ch = char_list; ch != NULL; ch = ch->next ) {
    if ( !IS_NPC(ch)
	    &&   (!fConn || ch->desc == NULL)
	    &&   !str_cmp( d->character->name, ch->name )){
	 if ( fConn == FALSE ) {
	   free_string( d->character->pcdata->pwd );
	   d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
	 }
	 else {
           if (d->character->pet) {
               CHAR_DATA *pet=d->character->pet;

               char_to_room(pet,get_room_index( ROOM_VNUM_LIMBO));
               stop_follower(pet);
               extract_char(pet,TRUE,FALSE);
           }
           else if (d->character->mount) {
               CHAR_DATA *mount=d->character->mount;

               char_to_room(mount,get_room_index( ROOM_VNUM_LIMBO));
               stop_follower(mount);
               extract_char(mount,TRUE,FALSE);
           }           
	 	
	   free_char( d->character );
	   d->character = ch;
	   ch->desc	 = d;
	   ch->timer	 = 0;
	   send_to_char("Reconnecting. Type 'replay' to see missed tells.\n\r", ch );
	   if (ch->invis_level < 2 && ch->incog_level < 2)
           {
	   	act( "$n has reconnected.", ch, NULL, NULL, TO_ROOM );
	   } 
	   sprintf( log_buf, "%s@%s reconnected.", ch->name, d->host );
	   log_string( log_buf );
	   wiznet("$N has restored $S link.",
			ch,NULL,WIZ_LINKS,0,get_trust(ch));
	   d->connected = CON_PLAYING;

	   /* Inform the character of a note in progress and the possbility
	    * of continuation!
	    */
	   if (ch->pcdata->in_progress)
		send_to_char ("You have a note in progress. Type 'note write' to continue it.\n\r", ch);
	 }
	 return TRUE;
    }
  }
  return FALSE;
}



/*
 * Check if already playing.
 */
bool check_playing( DESCRIPTOR_DATA *d, char *name )
{
    DESCRIPTOR_DATA *dold;

    for ( dold = descriptor_list; dold; dold = dold->next )
    {
	if ( dold != d
	&&   dold->character != NULL
	&&   dold->connected != CON_GET_NAME
	&&   dold->connected != CON_GET_OLD_PASSWORD
	&&   !str_cmp( name, dold->original
	         ? dold->original->name : dold->character->name ) )
	{
	    write_to_buffer( d, "That character is already playing.\n\r",0);
	    write_to_buffer( d, "Do you wish to connect anyway (Y/N)?",0);
	    d->connected = CON_BREAK_CONNECT;
	    return TRUE;
	}
    }

    return FALSE;
}



void stop_idling( CHAR_DATA *ch )
{
    if ( ch == NULL
    ||   ch->desc == NULL
    ||   ch->desc->connected != CON_PLAYING
    ||   ch->was_in_room == NULL 
    ||   ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
	return;

    ch->timer = 0;
    char_from_room( ch );
    char_to_room( ch, ch->was_in_room );
    ch->was_in_room	= NULL;
    act( "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
    return;
}



/*
 * Write to one char.
 */
void send_to_char_bw( const char *txt, CHAR_DATA *ch )
{
    if (!IS_NPC(ch))
    {
       if ( txt != NULL && ch->desc != NULL )
           write_to_buffer( ch->desc, txt, strlen(txt) );
    }
    return;
}

  /*
  * Write to one char, new colour version, by Lope.
  */
 void send_to_char( const char *txt, CHAR_DATA *ch )
 {
     const	char 	*point;
     		char 	*point2;
     		char 	buf[ MAX_STRING_LENGTH*4 ];
 		int	skip = 0;
 
     buf[0] = '\0';
     point2 = buf;
     if( txt && ch->desc )
 	{
 	    if( IS_SET( ch->act, PLR_COLOUR ) )
 	    {
 		for( point = txt ; *point ; point++ )
 	        {
 		    if( *point == '{' )
 		    {
 			point++;
 			skip = colour( *point, ch, point2 );
 			while( skip-- > 0 )
 			    ++point2;
 			continue;
 		    }
 		    *point2 = *point;
 		    *++point2 = '\0';
 		}			
 		*point2 = '\0';
         	write_to_buffer( ch->desc, buf, point2 - buf );
 	    }
 	    else
 	    {
 		for( point = txt ; *point ; point++ )
 	        {
 		    if( *point == '{' )
 		    {
 			point++;
 			continue;
 		    }
 		    *point2 = *point;
 		    *++point2 = '\0';
 		}
 		*point2 = '\0';
         	write_to_buffer( ch->desc, buf, point2 - buf );
 	    }
 	}
     return;
 }

/*
 * Send a page to one char.
 */
void page_to_char_bw( const char *txt, CHAR_DATA *ch )
{
    if ( txt == NULL || ch->desc == NULL)
	return;

    if (ch->lines == 0 )
    {
 	send_to_char_bw( txt, ch );
	return;
    }
	
#if defined(macintosh)
	send_to_char(txt,ch);
#else
    if (ch->desc->showstr_head &&
       (strlen(txt)+strlen(ch->desc->showstr_head)+1) < 32000)
    {
      char *temp=alloc_mem(strlen(txt) + strlen(ch->desc->showstr_head) + 1);
      strcpy(temp, ch->desc->showstr_head);
      strcat(temp, txt);
      ch->desc->showstr_point = temp +
       (ch->desc->showstr_point - ch->desc->showstr_head);
      free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head) + 1);
      ch->desc->showstr_head=temp;
    }
    else
    {
      if (ch->desc->showstr_head)
       free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head)+1);
      ch->desc->showstr_head = alloc_mem(strlen(txt) + 1);
      strcpy(ch->desc->showstr_head,txt);
      ch->desc->showstr_point = ch->desc->showstr_head;
      show_string(ch->desc,"");
    }

#endif
}

/*
 * Page to one char, new colour version, by Lope.
 */
void page_to_char( const char *txt, CHAR_DATA *ch )
{
    const	char	*point;
    		char	*point2;
    		//char	buf[ MAX_STRING_LENGTH * 4 ];
              	char    buf[ MAX_STRING_LENGTH * 10 ];
		int	skip = 0;


    buf[0] = '\0';
    point2 = buf;
    if( txt && ch->desc )
	{
	    if( IS_SET( ch->act, PLR_COLOUR ) )
	    {
		for( point = txt ; *point ; point++ )
	        {
		    if( *point == '{' )
		    {
			point++;
			skip = colour( *point, ch, point2 );
			while( skip-- > 0 )
			    ++point2;
			continue;
		    }
		    *point2 = *point;
		    *++point2 = '\0';
		    if (strlen(buf) >= sizeof(buf)-10)
                    {  
			send_to_char("Line too long!!! Try again without color\r\n",ch);
                        return;
                    } 
		}			
		*point2 = '\0';

		ch->desc->showstr_head  = alloc_mem( strlen( buf ) + 1 );
		strcpy( ch->desc->showstr_head, buf );
		ch->desc->showstr_point = ch->desc->showstr_head;
		show_string( ch->desc, "" );
	    }
	    else
	    {
		for( point = txt ; *point ; point++ )
	        {
		    if( *point == '{' )
		    {
			point++;
			continue;
		    }
		    *point2 = *point;
		    *++point2 = '\0';
		}
		*point2 = '\0';
		ch->desc->showstr_head  = alloc_mem( strlen( buf ) + 1 );
		strcpy( ch->desc->showstr_head, buf );
		ch->desc->showstr_point = ch->desc->showstr_head;
		show_string( ch->desc, "" );
	    }
	}
    return;
}

/* string pager */
void show_string(struct descriptor_data *d, char *input)
{
    char buffer[4*MAX_STRING_LENGTH];
    char buf[4*MAX_INPUT_LENGTH];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int show_lines;

    one_argument(input,buf);
    if (buf[0] != '\0')
    {
	if (d->showstr_head)
	{
//	    free_mem(d->showstr_head,strlen(d->showstr_head));
	    free_mem(d->showstr_head,strlen(d->showstr_head)+1);	    
	    d->showstr_head = 0;
	}
    	d->showstr_point  = 0;
	return;
    }

    if (d->character)
	show_lines = d->character->lines;
    else
	show_lines = 0;

    for (scan = buffer; ; scan++, d->showstr_point++)
    {
	if (((*scan = *d->showstr_point) == '\n' || *scan == '\r')
	    && (toggle = -toggle) < 0)
	    lines++;

	else if (!*scan || (show_lines > 0 && lines >= show_lines))
	{
	    *scan = '\0';
	    write_to_buffer(d,buffer,strlen(buffer));
	    for (chk = d->showstr_point; isspace(*chk); chk++);
	    {
		if (!*chk)
		{
		    if (d->showstr_head)
        	    {
//            		free_mem(d->showstr_head,strlen(d->showstr_head));
            		free_mem(d->showstr_head,strlen(d->showstr_head)+1);            		
            		d->showstr_head = 0;
        	    }
        	    d->showstr_point  = 0;
    		}
	    }
	    return;
	}
    }
    return;
}
	

/* quick sex fixer */
void fix_sex(CHAR_DATA *ch)
{
    if (ch->sex < 0 || ch->sex > 2)
    	ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void act_new( const char *format, CHAR_DATA *ch, const void *arg1, 
	      const void *arg2, int type, int min_pos)
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    char buf[MAX_STRING_LENGTH];
    char buffer[ MAX_STRING_LENGTH*2 ];
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    const char *str;
    const char *i;
    char *point;
    char 		*pbuff;
    bool		fColour = FALSE;
 
    /*
     * Discard null and zero-length messages.
     */
    if ( format == NULL || format[0] == '\0' )
        return;

    /* discard null rooms and chars */
    if (ch == NULL || ch->in_room == NULL)
	return;

    to = ch->in_room->people;
    if ( type == TO_VICT )
    {
        if ( vch == NULL )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

	if (vch->in_room == NULL)
	    return;

        to = vch->in_room->people;
    }
 
    for ( ; to != NULL; to = to->next_in_room )
    {
	if ( (!IS_NPC(to) && to->desc == NULL )
	||   ( IS_NPC(to) && !HAS_TRIGGER(to, TRIG_ACT) )
	||    to->position < min_pos )
            continue;


	if (!IS_SAME_WORLD(to, ch))
	  continue;

        if ( (type == TO_CHAR) && to != ch )
            continue;
        if ( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if ( type == TO_ROOM && to == ch )
            continue;
        if ( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;

        if( type == TO_CONT && ( ch == to
          || ch->in_obj != to->in_obj ) ) continue;
        if( type == TO_OUTSIDE_CONT && ch->in_obj == to->in_obj ) continue;

       if( !can_hear_act( ch, to, type ) ) continue; /* in_obj code */
 
        point   = buf;
        str     = format;
        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }

 	    fColour = TRUE;
 	    ++str;
 	    i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";                                break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = PERS( ch,  to  );                         break;
                case 'N': i = PERS( vch, to  );                         break;
                case 'e': i = he_she  [URANGE(0, ch  ->sex, 2)];        break;
                case 'E': i = he_she  [URANGE(0, vch ->sex, 2)];        break;
                case 'm': i = him_her [URANGE(0, ch  ->sex, 2)];        break;
                case 'M': i = him_her [URANGE(0, vch ->sex, 2)];        break;
                case 's': i = his_her [URANGE(0, ch  ->sex, 2)];        break;
                case 'S': i = his_her [URANGE(0, vch ->sex, 2)];        break;
 
                case 'p':
			   if ( (char *) arg1 == NULL)
				i = "something";
			   else
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
			   if ( (char *) arg2 == NULL)
				i = "something";
			   else
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
/* in_object list */
               case 'l':
                  i = name_list( obj1->who_in, to );
                  break;

               case 'L':
                  i = name_list( obj2->who_in, to );
                  break;

                }
            }
 
            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }
 
        *point++ = '\n';
        *point++ = '\r';
	*point   = '\0';
        buf[0]   = UPPER(buf[0]);
 	pbuff	 = buffer;
 	colourconv( pbuff, buf, to );

	if ( to->desc != NULL )
           write_to_buffer( to->desc, buffer, 0 );
           // write_to_buffer( to->desc, buf, point - buf );
	else
	if ( MOBtrigger )
	    mp_act_trigger( buf, to, ch, arg1, arg2, TRIG_ACT );
    }
    return;
}

void act_old( const char *format, CHAR_DATA *ch, const void *arg1, 
		    const void *arg2, int type, int min_pos)
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    char buf[MAX_STRING_LENGTH];
    char buffer[ MAX_STRING_LENGTH*2 ];
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    const char *str;
    const char *i;
    char *point;
    char 		*pbuff;
    bool		fColour = FALSE;
 
    /*
     * Discard null and zero-length messages.
     */
    if ( format == NULL || format[0] == '\0' )
        return;

    /* discard null rooms and chars */
    if (ch == NULL || ch->in_room == NULL)
	return;

    to = ch->in_room->people;
    if ( type == TO_VICT )
    {
        if ( vch == NULL )
        {
            bug( "Act: null vch with TO_VICT.", 0 );
            return;
        }

	if (vch->in_room == NULL)
	    return;

        to = vch->in_room->people;
    }
 
    for ( ; to != NULL; to = to->next_in_room )
    {
	if ( (!IS_NPC(to) && to->desc == NULL )
	||   ( IS_NPC(to) && !HAS_TRIGGER(to, TRIG_ACT) )
	||    to->position < min_pos )
            continue;

/* 	if (!IS_SAME_WORLD(to, ch)) */
/* 	  continue; */
	
        if ( (type == TO_CHAR) && to != ch )
            continue;
        if ( type == TO_VICT && ( to != vch || to == ch ) )
            continue;
        if ( type == TO_ROOM && to == ch )
            continue;
        if ( type == TO_NOTVICT && (to == ch || to == vch) )
            continue;

        if( type == TO_CONT && ( ch == to
          || ch->in_obj != to->in_obj ) ) continue;
        if( type == TO_OUTSIDE_CONT && ch->in_obj == to->in_obj ) continue;

       //if( !can_hear_act( ch, to, type ) ) continue; /* in_obj code */
 
        point   = buf;
        str     = format;
        while( *str != '\0' )
        {
            if( *str != '$' )
            {
                *point++ = *str++;
                continue;
            }

 	    fColour = TRUE;
 	    ++str;
 	    i = " <@@@> ";
            if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
                bug( "Act: missing arg2 for code %d.", *str );
                i = " <@@@> ";
            }
            else
            {
                switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
                          i = " <@@@> ";                                break;
                /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = PERS_OLD( ch,  to  );                         break;
                case 'N': i = PERS_OLD( vch, to  );                         break;
                case 'e': i = he_she  [URANGE(0, ch  ->sex, 2)];        break;
                case 'E': i = he_she  [URANGE(0, vch ->sex, 2)];        break;
                case 'm': i = him_her [URANGE(0, ch  ->sex, 2)];        break;
                case 'M': i = him_her [URANGE(0, vch ->sex, 2)];        break;
                case 's': i = his_her [URANGE(0, ch  ->sex, 2)];        break;
                case 'S': i = his_her [URANGE(0, vch ->sex, 2)];        break;
 
                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
/* in_object list */
               case 'l':
                  i = name_list( obj1->who_in, to );
                  break;

               case 'L':
                  i = name_list( obj2->who_in, to );
                  break;

                }
            }
 
            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }
 
        *point++ = '\n';
        *point++ = '\r';
	*point   = '\0';
        buf[0]   = UPPER(buf[0]);
 	pbuff	 = buffer;
 	colourconv( pbuff, buf, to );

	if ( to->desc != NULL )
           write_to_buffer( to->desc, buffer, 0 );
           // write_to_buffer( to->desc, buf, point - buf );
	else
	if ( MOBtrigger )
	    mp_act_trigger( buf, to, ch, arg1, arg2, TRIG_ACT );
    }
    return;
}

void act_w_intro( const char *format, CHAR_DATA *ch, const void *arg1, 
			   const void *arg2, int type, int min_pos)
{
  static char * const he_she  [] = { "it",  "he",  "she" };
  static char * const him_her [] = { "it",  "him", "her" };
  static char * const his_her [] = { "its", "his", "her" };
  
  char buf[MAX_STRING_LENGTH];
  char buffer[ MAX_STRING_LENGTH*2 ];
  char fname[MAX_INPUT_LENGTH];
  CHAR_DATA *to;
  CHAR_DATA *vch = (CHAR_DATA *) arg2;
  OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
  OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
  const char *str;
  const char *i;
  char *point;
  char 		*pbuff;
  bool		fColour = FALSE;
  
  /*
   * Discard null and zero-length messages.
   */
  if ( format == NULL || format[0] == '\0' )
    return;
  
  /* discard null rooms and chars */
  if (ch == NULL || ch->in_room == NULL)
    return;
  
  to = ch->in_room->people;
  if ( type == TO_VICT )
    {
	 if ( vch == NULL )
        {
		bug( "Act: null vch with TO_VICT.", 0 );
		return;
        }
	 
	 if (vch->in_room == NULL)
	   return;
	 
	 to = vch->in_room->people;
    }
  
  for ( ; to != NULL; to = to->next_in_room )
    {
	 if ( (!IS_NPC(to) && to->desc == NULL )
		 ||   ( IS_NPC(to) && !HAS_TRIGGER(to, TRIG_ACT) )
		 ||    to->position < min_pos )
	   continue;

	 if (!IS_SAME_WORLD(to, ch))
	   continue;
	 
	 if ( (type == TO_CHAR) && to != ch )
            continue;
	 if ( type == TO_VICT && ( to != vch || to == ch ) )
	   continue;
	 if ( type == TO_ROOM && to == ch )
	   continue;
	 if ( type == TO_NOTVICT && (to == ch || to == vch) )
	   continue;
	 
	 if( type == TO_CONT && ( ch == to
						 || ch->in_obj != to->in_obj ) ) continue;
	 if( type == TO_OUTSIDE_CONT && ch->in_obj == to->in_obj ) continue;
	 
	 if( !can_hear_act( ch, to, type ) ) continue; /* in_obj code */
	 
	 point   = buf;
	 str     = format;
	 while( *str != '\0' )
        {
		if( *str != '$' )
            {
		    *point++ = *str++;
		    continue;
            }

		fColour = TRUE;
		++str;
		i = " <@@@> ";
		if( !arg2 && *str >= 'A' && *str <= 'Z' )
            {
		    bug( "Act: missing arg2 for code %d.", *str );
		    i = " <@@@> ";
            }
		else
            {
		    switch ( *str )
                {
                default:  bug( "Act: bad code %d.", *str );
			   i = " <@@@> ";                                break;
			   /* Thx alex for 't' idea */
                case 't': i = (char *) arg1;                            break;
                case 'T': i = (char *) arg2;                            break;
                case 'n': i = PERS_NAME( ch,  to  );                         break;
                case 'N': i = PERS_NAME( vch, to  );                         break;
                case 'e': i = he_she  [URANGE(0, ch  ->sex, 2)];        break;
                case 'E': i = he_she  [URANGE(0, vch ->sex, 2)];        break;
                case 'm': i = him_her [URANGE(0, ch  ->sex, 2)];        break;
                case 'M': i = him_her [URANGE(0, vch ->sex, 2)];        break;
                case 's': i = his_her [URANGE(0, ch  ->sex, 2)];        break;
                case 'S': i = his_her [URANGE(0, vch ->sex, 2)];        break;
 
                case 'p':
                    i = can_see_obj( to, obj1 )
                            ? obj1->short_descr
                            : "something";
                    break;
 
                case 'P':
                    i = can_see_obj( to, obj2 )
                            ? obj2->short_descr
                            : "something";
                    break;
 
                case 'd':
                    if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
                    {
                        i = "door";
                    }
                    else
                    {
                        one_argument( (char *) arg2, fname );
                        i = fname;
                    }
                    break;
/* in_object list */
               case 'l':
                  i = name_list( obj1->who_in, to );
                  break;

               case 'L':
                  i = name_list( obj2->who_in, to );
                  break;

                }
            }
 
            ++str;
            while ( ( *point = *i ) != '\0' )
                ++point, ++i;
        }
 
        *point++ = '\n';
        *point++ = '\r';
	*point   = '\0';
        buf[0]   = UPPER(buf[0]);
 	pbuff	 = buffer;
 	colourconv( pbuff, buf, to );

	if ( to->desc != NULL )
           write_to_buffer( to->desc, buffer, 0 );
           // write_to_buffer( to->desc, buf, point - buf );
	else
	if ( MOBtrigger )
	    mp_act_trigger( buf, to, ch, arg1, arg2, TRIG_ACT );
    }
    return;
}

/*
 * Macintosh support functions.
 */
#if defined(macintosh)
int gettimeofday( struct timeval *tp, void *tzp )
{
    tp->tv_sec  = time( NULL );
    tp->tv_usec = 0;
}
#endif

/* source: EOD, by John Booth <???> */

void printf_to_char (CHAR_DATA *ch, char *fmt, ...)
{
	char buf [MAX_STRING_LENGTH];
	va_list args;
	va_start (args, fmt);
	vsprintf (buf, fmt, args);
	va_end (args);
	
	send_to_char (buf, ch);
}

void bugf (char * fmt, ...)
{
	char buf [2*MSL];
	va_list args;
	va_start (args, fmt);
	vsprintf (buf, fmt, args);
	va_end (args);

	bug (buf, 0);
}

void flog (char * fmt, ...)
{
	char buf [2*MSL];
	va_list args;
	va_start (args, fmt);
	vsprintf (buf, fmt, args);
	va_end (args);

	log_string (buf);
}
 
int colour( char type, CHAR_DATA *ch, char *string )
{
  PC_DATA	*col;
  char	code[ 20 ];
  char	*p = '\0';
  
  if( IS_NPC( ch ) )
    return( 0 );
  
  col = ch->pcdata;
  
  switch( type ) {
  default:
    strcpy( code, CLEAR );
    break;
  case 'x':
    strcpy( code, CLEAR );
    break;
  case 'a':
    if( col->auction[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->auction[0], col->auction[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->auction[0], col->auction[1] );
    break;
  case 'd':
    if( col->gossip[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->gossip[0], col->gossip[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->gossip[0], col->gossip[1] );
    break;
  case 'e':
    if( col->music[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->music[0], col->music[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->music[0], col->music[1] );
    break;
  case 'f':
    if( col->chat[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->chat[0], col->chat[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->chat[0], col->chat[1] );
    break;
  case 'h':
    if( col->minion[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->minion[0], col->minion[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->minion[0], col->minion[1] );
    break;
  case 'i':
    if( col->immtalk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->immtalk[0], col->immtalk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->immtalk[0], col->immtalk[1] );
    break;
  case 'j':
    if( col->game[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->game[0], col->game[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->game[0], col->game[1] );
    break;
  case 'k':
    if( col->tell[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->tell[0], col->tell[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->tell[0], col->tell[1] );
    break;
  case 'l':
    if( col->reply[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->reply[0], col->reply[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->reply[0], col->reply[1] );
    break;
  case 'L':
    if( col->leader_talk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->leader_talk[0], col->leader_talk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->leader_talk[0], col->leader_talk[1] );
    break;
  case 'n':
    if( col->gtell[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->gtell[0], col->gtell[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->gtell[0], col->gtell[1] );
    break;
  case 'N':
    if( col->oguild_talk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->oguild_talk[0], col->oguild_talk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->oguild_talk[0], col->oguild_talk[1] );
    break;    
  case 'o':
    if( col->room_exits[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->room_exits[0], col->room_exits[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->room_exits[0], col->room_exits[1] );
    break;
  case 'O':
    if( col->room_things[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->room_things[0], col->room_things[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->room_things[0], col->room_things[1] );
    break;
  case 'p':
    if( col->prompt[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->prompt[0], col->prompt[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->prompt[0], col->prompt[1] );
    break;
  case 'P':
    if( col->room_people[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->room_people[0], col->room_people[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->room_people[0], col->room_people[1] );
    break;
  case 'q':
    if( col->room[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->room[0], col->room[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->room[0], col->room[1] );
    break;
  case 's':
    if( col->bondtalk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->bondtalk[0], col->bondtalk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->bondtalk[0], col->bondtalk[1] );
    break;
  case 'S':
    if( col->bondtalk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->sguild_talk[0], col->sguild_talk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->sguild_talk[0], col->sguild_talk[1] );
    break;
  case 't':
    if( col->channel_name[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->channel_name[0], col->channel_name[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->channel_name[0], col->channel_name[1] );
    break;
  case 'u':
    if( col->wiznet[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->wiznet[0], col->wiznet[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->wiznet[0], col->wiznet[1] );
    break;
  case 'U':
    if( col->ssguild_talk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->ssguild_talk[0], col->ssguild_talk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->ssguild_talk[0], col->ssguild_talk[1] );
    break;
  case 'v':
    if( col->race_talk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->race_talk[0], col->race_talk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->race_talk[0], col->race_talk[1] );
    break;
  case 'X':
    if( col->df_talk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->df_talk[0], col->df_talk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->df_talk[0], col->df_talk[1] );
    break;
  case 'z':
    if( col->newbie[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->newbie[0], col->newbie[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->newbie[0], col->newbie[1] );
    break;
  case '1':
    if( col->fight_death[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->fight_death[0], col->fight_death[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->fight_death[0], col->fight_death[1] );
    break;
  case '2':
    if( col->fight_yhit[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->fight_yhit[0], col->fight_yhit[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->fight_yhit[0], col->fight_yhit[1] );
    break;
  case '3':
    if( col->fight_ohit[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->fight_ohit[0], col->fight_ohit[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->fight_ohit[0], col->fight_ohit[1] );
    break;
  case '4':
    if( col->fight_thit[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->fight_thit[0], col->fight_thit[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->fight_thit[0], col->fight_thit[1] );
    break;
  case '5':
    if( col->fight_skill[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->fight_skill[0], col->fight_skill[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->fight_skill[0], col->fight_skill[1] );
    break;
  case '6':
    if( col->wolfkin_talk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->wolfkin_talk[0], col->wolfkin_talk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->wolfkin_talk[0], col->wolfkin_talk[1] );
    break;
  case '7':
    if( col->sayt[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->sayt[0], col->sayt[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->sayt[0], col->sayt[1] );
    break;
  case '8':
    if( col->guild_talk[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->guild_talk[0], col->guild_talk[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->guild_talk[0], col->guild_talk[1] );
    break;
  case '9':
    if( col->osay[2] )
	 sprintf( code, "\e[%d;3%dm%c", col->osay[0], col->osay[1], '\a' );
    else
	 sprintf( code, "\e[%d;3%dm", col->osay[0], col->osay[1] );
    break;
  case 'b':
    strcpy( code, C_BLUE );
    break;
  case 'c':
    strcpy( code, C_CYAN );
    break;
  case 'g':
    strcpy( code, C_GREEN );
    break;
  case 'm':
    strcpy( code, C_MAGENTA );
    break;
  case 'r':
    strcpy( code, C_RED );
    break;
  case 'w':
    strcpy( code, C_WHITE );
    break;
  case 'y':
    strcpy( code, C_YELLOW );
    break;
  case 'B':
    strcpy( code, C_B_BLUE );
    break;
  case 'C':
    strcpy( code, C_B_CYAN );
    break;
  case 'G':
    strcpy( code, C_B_GREEN );
    break;
  case 'M':
    strcpy( code, C_B_MAGENTA );
    break;
  case 'R':
    strcpy( code, C_B_RED );
    break;
  case 'W':
    strcpy( code, C_B_WHITE );
    break;
  case 'Y':
    strcpy( code, C_B_YELLOW );
    break;
  case 'D':
    strcpy( code, C_D_GREY );
    break;
  case '*':
    sprintf( code, "%c", BEEP );
    break;
  case '/':
    strcpy( code, "\n\r" );
    break;
  case '-':
    sprintf( code, "%c", '~' );
    break;
  case '{':
    sprintf( code, "%c", '{' );
    break;
  }
  
  p = code;
  while( *p != '\0' ){
    *string = *p++;
    *++string = '\0';
  }
  
  return( strlen( code ) );
}

void colourconv( char *buffer, const char *txt, CHAR_DATA *ch )
{
  const	char	*point;
  int	skip = 0;
  
  if( ch->desc && txt ) {
    if( IS_SET( ch->act, PLR_COLOUR ) ) {
	 for( point = txt ; *point ; point++ ) {
	   if( *point == '{' ) {
		point++;
		skip = colour( *point, ch, buffer );
		while( skip-- > 0 )
		  ++buffer;
		continue;
	   }
	   *buffer = *point;
	   *++buffer = '\0';
	 }			
	 *buffer = '\0';
    }
    else {
	 for( point = txt ; *point ; point++ ) {
	   if( *point == '{' ) {
		point++;
		continue;
	   }
	   *buffer = *point;
	   *++buffer = '\0';
	 }
	 *buffer = '\0';
    }
  }
  return;
}

int can_hear_act( CHAR_DATA * ch, CHAR_DATA * to, int type )
{
   OBJ_DATA *to_obj;
   OBJ_DATA *ch_obj;

 if( type == TO_ROOM )
 {
  if( ch->in_obj != to->in_obj
   && ( ( ch->in_obj
       && IS_SET( ch->in_obj->value[1], CONT_SOUNDPROOF )
       && IS_SET( ch->in_obj->value[1], CONT_CLOSED ) )
     || ( to->in_obj
       && IS_SET( to->in_obj->value[1], CONT_SOUNDPROOF )
       && IS_SET( to->in_obj->value[1], CONT_CLOSED ) ) ) )
   return FALSE;
  else return TRUE;
 }
 
 if ( type == TO_ALL) return TRUE;

 for(to_obj=to->in_obj; to_obj; to_obj=to_obj->in_obj )
   if( IS_SET(to_obj->value[1],CONT_CLOSED))
     break;

 for(ch_obj=ch->in_obj; ch_obj; ch_obj=ch_obj->in_obj )
   if( IS_SET(ch_obj->value[1],CONT_CLOSED))
     break;

 /*
  * ch_obj == to_boj only if there were NO objects between
  * ch and to that had a roof AND was closed.
  */
 if (to_obj == ch_obj)
    return TRUE;

 return FALSE;

}

void _logf (char * fmt, ...)
{
  char buf [2*MSL];
  va_list args;
  va_start (args, fmt);
  vsprintf (buf, fmt, args);
  va_end (args);
  
  log_string (buf);
}

void handle_seg(int i)
{
  
  perror ("Attempting Crash Recover");
  fflush(NULL);
  FILE * fp;
  if ((fp = fopen(LAST_COMMAND,"a")) == NULL)
  {
	bug("Error in handle_seg opening last_command.txt",0);
  } 
  else
  {
	fprintf(fp,"Last Command: %s\n",last_command);
	fclose(fp);
  }
  do_warmboot(NULL,NULL);
  return; 
  //do_warmboot(SIGSEGV);
  
}

void send_to_femalchan(const char *txt, CHAR_DATA *ch)
{
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next = vch->next;
    if (vch->in_room == NULL)
	 continue;
    if (IS_NPC(vch))
	 continue;     
    if (vch->in_room == ch->in_room) {
	 if ((vch->class == CLASS_CHANNELER)
		&& (vch->pcdata->true_sex == SEX_FEMALE)
		&& (ch != vch)
		&& ((get_skill(vch,gsn_embrace)) != 0)
		&& (!IS_SET(vch->act,PLR_STILLED))
		&& (IS_SET(vch->chan_flags, CHAN_SEECHANNELING))) {
	   act(txt,  ch, NULL, vch, TO_VICT );
	 }
    }
  }
  return;
}

void send_to_malchan(const char *txt, CHAR_DATA *ch)
{
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  
  for ( vch = char_list; vch != NULL; vch = vch_next ) {
    vch_next = vch->next;
    if (vch->in_room == NULL)
	 continue;
    if (IS_NPC(vch))
	 continue;
    if (vch->in_room == ch->in_room) {
	 if ((vch->class == CLASS_CHANNELER)
		&& (vch->pcdata->true_sex ==  SEX_MALE)
		&& (ch != vch )
		&& ((get_skill(vch,gsn_seize)) != 0) 
		&& (!IS_SET(vch->act,PLR_STILLED))
		&& (IS_SET(vch->chan_flags, CHAN_SEECHANNELING))) {
	   act(txt, ch, NULL, vch, TO_VICT );
	 }
    }
  }
  return;
}

void send_chpower_to_channelers(CHAR_DATA *ch, int sn)
{
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  float ratio = 0;

  for (vch = char_list; vch != NULL; vch = vch_next) {
    char vp[MAX_INPUT_LENGTH];
    
    vch_next = vch->next;
    if (vch == ch)
	 continue;
    if (vch->in_room == NULL)
	 continue;
    if (IS_NPC(vch))
	 continue;
    if (!IS_AWAKE(vch))
	 continue;
    if (vch->class != CLASS_CHANNELER)
	 continue;
    if (IS_SET(vch->act, PLR_STILLED))
	 continue;
    if (!IS_SET(vch->chan_flags, CHAN_SEECHANNELING))
	 continue;
    if (vch->sex != ch->sex) {

	 // Let MC feel the prickling
	 if (vch->sex == SEX_MALE && ch->sex == SEX_FEMALE && vch->in_room == ch->in_room) {
	   send_to_char("Your skin prickle for a moment giving you goosebumps!\n\r", vch);
	 }	 
	 continue;
    }
    if (vch->pcdata->learned[sn] < 1)
       continue;
    if (vch->in_room == ch->in_room) {
	 char buf[MAX_INPUT_LENGTH];
	 
	 ratio = ((float)ch->holding / get_curr_op(vch));
	 ratio *= 100;
	 if (ratio <= 1) strcpy(vp,       "a small amount");
	 else if (ratio <= 5) strcpy(vp,  "a fairly small amount");
	 else if (ratio <= 10) strcpy(vp, "an adequate amount");
	 else if (ratio <= 20) strcpy(vp, "a large amount");
	 else if (ratio <= 35) strcpy(vp, "a very large amount");
	 else if (ratio <= 50) strcpy(vp, "a huge amount");
	 else if (ratio <= 75) strcpy(vp, "a massive amount");
	 else if (ratio <= 100) strcpy(vp,"an enormous amount");
	 else if (ratio > 100) strcpy(vp, "more than you could possibly draw");

         if (!IS_SET(ch->act2,PLR2_MASKCHAN)) {
	   if (ch->sex == SEX_MALE)
	     sprintf(buf,"You feel %s draw in %s of Saidin.\n\r",PERS(ch, vch) ,vp);
	   else
	     sprintf(buf,"You feel %s draw in %s of Saidar.\n\r",PERS(ch, vch) ,vp);
	   send_to_char(buf,vch);
	}
    }
    else if ((vch->in_room->area == ch->in_room->area) && (ch->holding > 450)) {

	 // Only if character who channels is In Character
         if (!IS_SET(ch->act2,PLR2_MASKCHAN)) {
	   if (IS_RP(ch)) {
	     if (ch->sex == SEX_MALE)
		  send_to_char("You feel someone nearby draw in an enormous amount of Saidin.\n\r", vch);
	     else
		  send_to_char("You feel someone nearby draw in an enormous amount of Saidar.\n\r", vch);
  	    }
	 }
    }
  }
}

/*
 * Functions related to new creation for TSW
 *
 * Swordfish
 */
int get_talent_costs(CHAR_DATA *ch)
{
  int value=0;
  int i;

  for (i=0; talent_table[i].name != NULL; i++) {
    if (IS_SET(ch->talents, talent_table[i].bit))
	 value += talent_table[i].cost;
  }
  
  return(value);
}

int get_merit_costs(CHAR_DATA *ch)
{
  int value=0;
  int i;

  for (i=0; merit_table[i].name != NULL; i++) {
    if (IS_SET(ch->merits, merit_table[i].bit))
	 value += merit_table[i].cost;
  }
  
  return(value);
}

int get_flaw_costs(CHAR_DATA *ch)
{
  int value=0;
  int i;

  for (i=0; flaw_table[i].name != NULL; i++) {
    if (IS_SET(ch->flaws, flaw_table[i].bit))
	 value -= flaw_table[i].cost;
  }
  
  return(value);
}

void selection_menu(CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH];
  int max_stats=0;
  int total=0;
  int i;

  send_to_char("\n\r", ch);
  send_to_char("-----------------------------------------------------------\n\r", ch);
  sprintf(buf, " Name: %s  Class: %s",
		ch->name, 
		capitalize(class_table[ch->class].name));
  send_to_char(buf, ch);
  sprintf(buf, "  Race: %s\n\r",
		capitalize(race_table[ch->race].name));
  send_to_char(buf, ch);
  send_to_char("-----------------------------------------------------------\n\r", ch);
  
  if (ch->class == CLASS_CHANNELER) {
    send_to_char("\n\r", ch);
    send_to_char("  Sphere record\n\r", ch);
    sprintf(buf, "    Spheres : %4lu (Main sphere: %s)\n\r", get_curr_op(ch),
		  ch->main_sphere != -1 ? capitalize(sphere_table[ch->main_sphere].name) : "none");
    send_to_char(buf, ch);
    sprintf(buf, "    Maximum : %4d\n\r", (get_creation_sphere_sum(ch) + race_table[ch->race].sphere_points));
    send_to_char(buf, ch);
  }

  send_to_char("\n\r", ch);
  send_to_char("  Stat record\n\r", ch);

  sprintf(buf, "    Stats   : %4d\n\r", get_tot_stat(ch));
  send_to_char(buf, ch);

  for (i=0; i < MAX_STATS; i++) {
    max_stats += race_table[ch->race].stats[i];
  }

  sprintf(buf, "    Maximum : %4d\n\r", max_stats + race_table[ch->race].stat_points);
  send_to_char(buf, ch);

  send_to_char("\n\r", ch);
  send_to_char("  Background record\n\r", ch);
  sprintf(buf, "    Merits  : %4d (%s)\n\r", get_merit_costs(ch), background_flag_string(merit_table, ch->merits));
  send_to_char(buf, ch);
  sprintf(buf, "    Flaws   : %4d (%s)\n\r", get_flaw_costs(ch), background_flag_string(flaw_table, ch->flaws));
  send_to_char(buf, ch);
  sprintf(buf, "    Talents : %4d (%s)\n\r", get_talent_costs(ch), background_flag_string(talent_table, ch->talents));
  send_to_char(buf, ch);
  send_to_char("    -----------------\n\r", ch);
  total = (get_merit_costs(ch) + get_flaw_costs(ch) + get_talent_costs(ch));
  sprintf(buf, "    Total   : %4d\n\r", total);
  send_to_char(buf, ch);
  sprintf(buf, "    Maximum : %4d\n\r", race_table[ch->race].misc_points);
  send_to_char(buf, ch);

  send_to_char("\n\r", ch);
  send_to_char("-----------------------------------------------------------\n\r", ch);

  if (ch->class == CLASS_CHANNELER) {
    send_to_char("Select: spheres, stats, merits, flaws, talents, help, done: ", ch);
  }
  else {
    send_to_char("Select: stats, merits, flaws, talents, help, done: ", ch);
  }

  return;
}

void set_creation_record(CHAR_DATA *ch)
{
  if (ch->pcdata->keepoldstats == TRUE) {
	ch->pcdata->stat_points = 0;
	ch->pcdata->misc_points = 0;

	if (ch->class == CLASS_CHANNELER && ch->pcdata->prev_class != CLASS_CHANNELER) {
	  ch->perm_sphere[SPHERE_AIR]    = race_table[ch->race].sphere[SPHERE_AIR];
	  ch->perm_sphere[SPHERE_EARTH]  = race_table[ch->race].sphere[SPHERE_EARTH];
	  ch->perm_sphere[SPHERE_FIRE]   = race_table[ch->race].sphere[SPHERE_FIRE];
	  ch->perm_sphere[SPHERE_SPIRIT] = race_table[ch->race].sphere[SPHERE_SPIRIT];
	  ch->perm_sphere[SPHERE_WATER]  = race_table[ch->race].sphere[SPHERE_WATER];
	  ch->pcdata->sphere_points      = race_table[ch->race].sphere_points;
	}
	
	return;
  }
  if (ch->class == CLASS_CHANNELER) {
    ch->perm_sphere[SPHERE_AIR]    = race_table[ch->race].sphere[SPHERE_AIR];
    ch->perm_sphere[SPHERE_EARTH]  = race_table[ch->race].sphere[SPHERE_EARTH];
    ch->perm_sphere[SPHERE_FIRE]   = race_table[ch->race].sphere[SPHERE_FIRE];
    ch->perm_sphere[SPHERE_SPIRIT] = race_table[ch->race].sphere[SPHERE_SPIRIT];
    ch->perm_sphere[SPHERE_WATER]  = race_table[ch->race].sphere[SPHERE_WATER];
    ch->pcdata->sphere_points      = race_table[ch->race].sphere_points;
  }
  
  //we're assuming you won't die off in your first 4 hours 
  if (ch->played > 3600 * 4) {  
     ch->pcdata->stat_points      = 0;
     ch->pcdata->misc_points      = 0;
  }
  else {
     ch->pcdata->stat_points      = race_table[ch->race].stat_points;
     ch->pcdata->misc_points      = race_table[ch->race].misc_points;
  }
  return;
}

void stat_menu(CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH];
  
  send_to_char("\n\r", ch);
  send_to_char(" STAT          STR\n\n\r", ch);

  sprintf(buf, " Strength      %3d\n\r", get_curr_stat(ch,STAT_STR));
  send_to_char(buf, ch);

  sprintf(buf, " Intuition     %3d\n\r", get_curr_stat(ch,STAT_INT));
  send_to_char(buf, ch);

  sprintf(buf, " Wisdom        %3d\n\r", get_curr_stat(ch,STAT_WIS));
  send_to_char(buf, ch);

  sprintf(buf, " Dexterity     %3d\n\r", get_curr_stat(ch,STAT_DEX));
  send_to_char(buf, ch);

  sprintf(buf, " Constitution  %3d\n\n\r", get_curr_stat(ch,STAT_CON));
  send_to_char(buf, ch);
  
  sprintf(buf, " Points Left: %d\n\n\r", ch->pcdata->stat_points);
  send_to_char(buf, ch);

  send_to_char(" Use: set <stat> <value>\n\n\r", ch);
  send_to_char("Choices are:  set, help, done: ", ch);
}

void set_stat(CHAR_DATA *ch,  char * argument)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  
  int stat, value;
  int change;
  
  argument = one_argument( argument, arg );
  
  if (IS_NULLSTR(argument) || IS_NULLSTR(arg)) {
    send_to_char("Invalid choice.\n\r", ch);
    return;
  }

  stat = flag_value( stat_table, arg);

  if (stat == NO_FLAG) {
    send_to_char( "Stat invalid.\n\r", ch );
    return;
  }

  if (!is_number(argument)) {
    send_to_char("Value must be a number.\n\r", ch);
    return;
  }
  
  value = atoi(argument);

  if (value < 3 || value > 25) {
    send_to_char("Invalid value range.\n\r", ch);
    return;
  }


  if ((value - ch->perm_stat[stat]) > ch->pcdata->stat_points) {
    send_to_char("You don't have that many points left.\n\r", ch);
    return;
  }

  if (value > race_table[ch->race].max_stats[stat]) {
    sprintf(buf, "You can't train this stat above %d.\n\r", race_table[ch->race].max_stats[stat]);
    send_to_char(buf, ch);
    return;
  }

  change = (value - ch->perm_stat[stat]);
  ch->perm_stat[stat] = value;
  ch->pcdata->stat_points =  ch->pcdata->stat_points - change;
  return;
}

void sphere_menu(CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH];

  send_to_char("\n\r", ch);
  send_to_char(" SPHERE   STR  M \n\n\r", ch);
  
  sprintf(buf, " Air      %3d (%s)\n\r", ch->perm_sphere[SPHERE_AIR],
		ch->main_sphere == SPHERE_AIR ? "*" : " ");
  send_to_char(buf, ch);

  sprintf(buf, " Earth    %3d (%s)\n\r", ch->perm_sphere[SPHERE_EARTH],
		ch->main_sphere == SPHERE_EARTH ? "*" : " ");
  send_to_char(buf, ch);

  sprintf(buf, " Fire     %3d (%s)\n\r", ch->perm_sphere[SPHERE_FIRE],
		ch->main_sphere == SPHERE_FIRE ? "*" : " ");
  send_to_char(buf, ch);

  sprintf(buf, " Spirit   %3d (%s)\n\r", ch->perm_sphere[SPHERE_SPIRIT],
		ch->main_sphere == SPHERE_SPIRIT ? "*" : " ");
  send_to_char(buf, ch);

  sprintf(buf, " Water    %3d (%s)\n\n\r", ch->perm_sphere[SPHERE_WATER],
		ch->main_sphere == SPHERE_WATER ? "*" : " ");
  send_to_char(buf, ch);

  sprintf(buf, " Points Left: %d\n\n\r", ch->pcdata->sphere_points);
  send_to_char(buf, ch);
  
  sprintf(buf, " Legal range: 1-%d\n\n\r", MAX_SPHERE_BONUS_PC);
  send_to_char(buf, ch);
  
  send_to_char(" Use: set <sphere> <value>\n\r", ch);
  send_to_char("      set <sphere> main\n\n\r", ch);

  send_to_char("Choices are:  set, help, done: ", ch);
}

int get_creation_sphere_sum(CHAR_DATA *ch)
{
  int sum=0;
  int sphere;
  
  for (sphere = 0; sphere < MAX_SPHERE; sphere++)
    sum += race_table[ch->race].sphere[sphere];
  
  return(sum);
}

void set_sphere(CHAR_DATA *ch,  char * argument)
{
  char arg[MAX_INPUT_LENGTH];
  
  int sphere, value;
  
  argument = one_argument( argument, arg );
  
  if (IS_NULLSTR(argument) || IS_NULLSTR(arg)) {
    send_to_char("Invalid choice.\n\r", ch);
    return;
  }

  for (sphere = 0; sphere < MAX_SPHERE; sphere++) {
    if (!str_prefix (arg, "air")) {
	 sphere = SPHERE_AIR;
	 break;
    }
    else if (!str_prefix (arg, "earth")) {
	 sphere = SPHERE_EARTH;
	 break;
    }
    else if (!str_prefix (arg, "fire")) {
	 sphere = SPHERE_FIRE;
	 break;
    }
    else if (!str_prefix (arg, "spirit")) {
	 sphere = SPHERE_SPIRIT;
	 break;
    }
    else if (!str_prefix (arg, "water")) {
	 sphere = SPHERE_WATER;
	 break;
    }
    else {
	 send_to_char( "Sphere non-existant.\n\r", ch );
	 return;
    }
  }

  if (!str_prefix(argument, "main")) {
    ch->main_sphere = sphere;
    send_to_char("Ok.\n\r", ch);
    return;
  }

  if (!is_number(argument)) {
    send_to_char("Invalid argument.\n\r", ch);
    return;
  }
  
  value = atoi(argument);

  if (value < 1 || value > MAX_SPHERE_BONUS_PC) {
    send_to_char("Invalid value range.\n\r", ch);
    return;
  }


  if ((value - ch->perm_sphere[sphere]) > ch->pcdata->sphere_points) {
    send_to_char("You don't have that many points left.\n\r", ch);
    return;
  }
  
  ch->perm_sphere[sphere] = value;
  ch->pcdata->sphere_points = ((get_creation_sphere_sum(ch) - get_curr_op(ch)) + race_table[ch->race].sphere_points);
  return;
}

void list_background_table(CHAR_DATA *ch, const struct background_type *flag_table)
{
  char buf[MAX_STRING_LENGTH];
  int i=0;
  
  buf[0] = '\0';

  send_to_char("Name                Cost Class       Race        !Race\n\r", ch);
  
  for (i=0; flag_table[i].name != NULL; i++) {
    sprintf(buf, "%-18s (%3d) (%-9s) (%-9s) (%-9s) \n\r", 
		  flag_table[i].name,
		  flag_table[i].cost,
		  flag_table[i].class != -1 ? class_table[flag_table[i].class].name : "all", 
		  flag_table[i].race != NULL ? flag_table[i].race : "all", 
		  flag_table[i].nrace != NULL ? flag_table[i].nrace : "all");
    send_to_char(buf, ch);
  }
  
  return;
}

void merit_menu(CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH];
  char tmp[MAX_STRING_LENGTH];
  int i=0;
  int cnt=0;

  buf[0] = '\0';

  send_to_char("\n\n\r", ch);
  send_to_char("Available merits:\n\n\r", ch);

  for (i=0; merit_table[i].name != NULL; i++) {

    /* check class */
    if ((merit_table[i].class != ch->class) && (merit_table[i].class != -1))
	 continue;

     /* check race */
    if ((merit_table[i].race != NULL) && 
	   (strstr(merit_table[i].race, race_table[ch->race].name) == NULL)) {
	 continue;
    }

    /* check not race */
    if ((merit_table[i].nrace != NULL) && 
	   (strstr(merit_table[i].nrace, race_table[ch->race].name) != NULL)) {
	 continue;
    }

    /* check if settable */
    if (merit_table[i].settable != TRUE)
	 continue;

    cnt++;
    sprintf(tmp, "%-18s   %4d ", merit_table[i].name, merit_table[i].cost);
    strcat( buf, " ");
    strcat( buf, tmp);
    if ( (cnt % 3) == 0)
	 strcat( buf, "\n\r");
  }

  if (buf[0] != '\0')
    send_to_char(buf, ch);
  else
    send_to_char("none\n\r", ch);
  
  send_to_char("\n\n\r", ch);
  sprintf(buf, " Merits    : %s\n\n\r", background_flag_string(merit_table, ch->merits));
  send_to_char(buf, ch);
  sprintf(buf, " Points Left: %d\n\n\r", ch->pcdata->misc_points);
  send_to_char(buf, ch);

  send_to_char(" Use: add  <merit>\n\r", ch);
  send_to_char("      drop <merit>\n\n\r", ch);

  send_to_char("Choices are:  add, drop, help, done: ", ch);
}

void add_merit(CHAR_DATA *ch,  char * argument)
{
  long pos = NO_FLAG;
  int i=0;
  int num=0;
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Invalid choice.\n\r", ch);
    return;
  }
  
  for (i=0; merit_table[i].name != NULL; i++) {
    /* check class */
    if ((merit_table[i].class != ch->class) && (merit_table[i].class != -1))
	 continue;
     /* check race */
    if ((merit_table[i].race != NULL) && 
	   (strstr(merit_table[i].race, race_table[ch->race].name) == NULL))
	 continue;    
    /* check not race */
    if ((merit_table[i].nrace != NULL) && 
	   (strstr(merit_table[i].nrace, race_table[ch->race].name) != NULL))	
	 continue;
    /* check if settable */
    if (merit_table[i].settable != TRUE)
	 continue;
    /* Check name */
    if (!str_prefix(merit_table[i].name, argument)) {
	 pos = merit_table[i].bit;
	 num = i;
    }
  }
  
  if (pos == NO_FLAG) {
    send_to_char("That merit doesn't exist!\n\r", ch);
    return;
  }
  else {
    if (merit_table[num].cost > ch->pcdata->misc_points) {
	 send_to_char("You don't have that many points left.\n\r", ch);
	 return;
    }
    else {
	 if (!IS_SET(ch->merits, pos)) {
	   SET_BIT(ch->merits, pos);
	   ch->pcdata->misc_points -= merit_table[num].cost;
	   send_to_char("Merit selected!\n\r", ch);
	 }
    }
  }
 return;
}

void drop_merit(CHAR_DATA *ch,  char * argument)
{
  long pos = NO_FLAG;
  int i=0;
  int num=0;
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Invalid choice.\n\r", ch);
    return;
  }

  for (i=0; merit_table[i].name != NULL; i++) {
    if (!str_prefix(merit_table[i].name, argument)) {
	 pos = merit_table[i].bit;
	 num = i;
    }
  }

  /* not settable, report not existing */
  if (merit_table[num].settable != TRUE) {
    send_to_char("That merit doesn't exist!\n\r", ch);
    return;
  }
      
  if (pos == NO_FLAG) {
    send_to_char("That merit doesn't exist!\n\r", ch);
    return;
  }
  else {
    if (IS_SET(ch->merits, pos)) {
	 REMOVE_BIT(ch->merits, pos);
	 ch->pcdata->misc_points += merit_table[num].cost;
	 send_to_char("Merit removed!\n\r", ch);
    }
  }
  return;
}

void flaw_menu(CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH];
  char tmp[MAX_STRING_LENGTH];
  int i=0;
  int cnt=0;

  buf[0] = '\0';

  send_to_char("\n\n\r", ch);
  send_to_char("Available flaws:\n\n\r", ch);

  for (i=0; flaw_table[i].name != NULL; i++) {

    /* check class */
    if ((flaw_table[i].class != ch->class) && (flaw_table[i].class != -1))
	 continue;
    
    /* check race */
    if ((flaw_table[i].race != NULL) && 
	   (strstr(flaw_table[i].race, race_table[ch->race].name) == NULL)) {
	 continue;
    }
    
    /* check not race */
    if ((flaw_table[i].nrace != NULL) && 
	   (strstr(flaw_table[i].nrace, race_table[ch->race].name) != NULL)) {
	 continue;
    }

    /* check if settable */
    if (flaw_table[i].settable != TRUE)
	 continue;
       
    cnt++;
    sprintf(tmp, "%-18s   %4d ", flaw_table[i].name, flaw_table[i].cost);
    strcat( buf, " ");
    strcat( buf, tmp);
    if ( (cnt % 3) == 0)
	 strcat( buf, "\n\r");
  }

  if (buf[0] != '\0')
    send_to_char(buf, ch);
  else
    send_to_char("none\n\r", ch);
  
  send_to_char("\n\n\r", ch);
  sprintf(buf, " Flaws    : %s\n\n\r", background_flag_string(flaw_table, ch->flaws));
  send_to_char(buf, ch);
  sprintf(buf, " Points Left: %d\n\n\r", ch->pcdata->misc_points);
  send_to_char(buf, ch);

  send_to_char(" Use: add  <flaw>\n\r", ch);
  send_to_char("      drop <flaw>\n\n\r", ch);

  send_to_char("Choices are:  add, drop, help, done: ", ch);
}

void add_flaw(CHAR_DATA *ch,  char * argument)
{
  long pos = NO_FLAG;
  int i=0;
  int num=0;
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Invalid choice.\n\r", ch);
    return;
  }
  
  for (i=0; flaw_table[i].name != NULL; i++) {
    /* check class */
    if ((flaw_table[i].class != ch->class) && (flaw_table[i].class != -1))
	 continue;    
    /* check race */
    if ((flaw_table[i].race != NULL) && 
	   (strstr(flaw_table[i].race, race_table[ch->race].name) == NULL))
	 continue;    
    /* check not race */
    if ((flaw_table[i].nrace != NULL) && 
	   (strstr(flaw_table[i].nrace, race_table[ch->race].name) != NULL))
	 continue;
    /* check if settable */
    if (flaw_table[i].settable != TRUE)
	 continue;
    /* check name match */
    if (!str_prefix(flaw_table[i].name, argument)) {
	 pos = flaw_table[i].bit;
	 num = i;
    }
  }
  
  if (pos == NO_FLAG) {
    send_to_char("That flaw doesn't exist!\n\r", ch);
    return;
  }
  else {
/*
    if (flaw_table[num].cost > ch->pcdata->misc_points) {
	 send_to_char("You don't have that many points left.\n\r", ch);
	 return;
    }
    else {
*/
	 if (!IS_SET(ch->flaws, pos)) {
	   SET_BIT(ch->flaws, pos);
	   ch->pcdata->misc_points += flaw_table[num].cost;
	   send_to_char("Flaw selected!\n\r", ch);
	 }
/*    }*/
 }
 return;
}

void drop_flaw(CHAR_DATA *ch,  char * argument)
{
  long pos = NO_FLAG;
  int i=0;
  int num=0;
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Invalid choice.\n\r", ch);
    return;
  }

  for (i=0; flaw_table[i].name != NULL; i++) {
    if (!str_prefix(flaw_table[i].name, argument)) {
	 pos = flaw_table[i].bit;
	 num = i;
    }
  }

  /* not settable, report not existing */
  if (flaw_table[num].settable != TRUE) {
    send_to_char("That flaw doesn't exist!\n\r", ch);
    return;
  }

  if (pos == NO_FLAG) {
    send_to_char("That flaw doesn't exist!\n\r", ch);
    return;
  }
  else {
    if (IS_SET(ch->flaws, pos)) {
	 REMOVE_BIT(ch->flaws, pos);
	 ch->pcdata->misc_points -= flaw_table[num].cost;
	 send_to_char("Flaw removed!\n\r", ch);
    }
  }
  return;
}

void talent_menu(CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH];
  char tmp[MAX_STRING_LENGTH];
  int i=0;
  int cnt=0;

  buf[0] = '\0';

  send_to_char("\n\n\r", ch);
  send_to_char("Available talents:\n\n\r", ch);

  for (i=0; talent_table[i].name != NULL; i++) {

    /* check class */
    if ((talent_table[i].class != ch->class) && (talent_table[i].class != -1))
	 continue;

    /* check race */
    if ((talent_table[i].race != NULL) && 
	   (strstr(talent_table[i].race, race_table[ch->race].name) == NULL)) {
	 continue;
    }
    
    /* check not race */
    if ((talent_table[i].nrace != NULL) && 
	   (strstr(talent_table[i].nrace, race_table[ch->race].name) != NULL)) {
	 continue;
    }

    /* check if settable */
    if (talent_table[i].settable != TRUE)
	 continue;
    
    cnt++;
    sprintf(tmp, "%-18s   %4d ", talent_table[i].name, talent_table[i].cost);
    strcat( buf, " ");
    strcat( buf, tmp);
    if ( (cnt % 3) == 0)
	 strcat( buf, "\n\r");
  }

  if (buf[0] != '\0')
    send_to_char(buf, ch);
  else
    send_to_char("none\n\r", ch);
  
  send_to_char("\n\n\r", ch);
  sprintf(buf, " Talents    : %s\n\n\r", background_flag_string(talent_table, ch->talents));
  send_to_char(buf, ch);
  sprintf(buf, " Points Left: %d\n\n\r", ch->pcdata->misc_points);
  send_to_char(buf, ch);

  send_to_char(" Use: add  <talent>\n\r", ch);
  send_to_char("      drop <talent>\n\n\r", ch);

  send_to_char("Choices are:  add, drop, help, done: ", ch);
}

void add_talent(CHAR_DATA *ch,  char * argument)
{
  long pos = NO_FLAG;
  int i=0;
  int num=0;
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Invalid choice.\n\r", ch);
    return;
  }
  
  for (i=0; talent_table[i].name != NULL; i++) {
    // check class
    if ((talent_table[i].class != ch->class) && (talent_table[i].class != -1))
	 continue;
    // check race
    if ((talent_table[i].race != NULL) && 
	   (strstr(talent_table[i].race, race_table[ch->race].name) == NULL))
	 continue;
    // check not race
    if ((talent_table[i].nrace != NULL) && 
	   (strstr(talent_table[i].nrace, race_table[ch->race].name) != NULL))
	 continue;
    // check if settable
    if (talent_table[i].settable != TRUE)
	 continue;
    // matching name?
    if (!str_prefix(talent_table[i].name, argument)) {
	 pos = talent_table[i].bit;
	 num = i;
    }
  }

  if (pos == NO_FLAG) {
    send_to_char("That talent doesn't exist!\n\r", ch);
    return;
  }
  else {
    if (talent_table[num].cost > ch->pcdata->misc_points) {
	 send_to_char("You don't have that many points left.\n\r", ch);
	 return;
    }
    else {
	 if (!IS_SET(ch->talents, pos)) {
	   SET_BIT(ch->talents, pos);
	   ch->pcdata->misc_points -= talent_table[num].cost;
	   send_to_char("Talent selected!\n\r", ch);
	 }
    }
  }
  return;
}

void drop_talent(CHAR_DATA *ch,  char * argument)
{
  long pos = NO_FLAG;
  int i=0;
  int num=0;
  
  if (IS_NULLSTR(argument)) {
    send_to_char("Invalid choice.\n\r", ch);
    return;
  }

  for (i=0; talent_table[i].name != NULL; i++) {
    if (!str_prefix(talent_table[i].name, argument)) {
	 pos = talent_table[i].bit;
	 num = i;
    }
  }

  // not settable, report not existing
  if (talent_table[num].settable != TRUE) {
    send_to_char("That talent doesn't exist!\n\r", ch);
    return;
  }
  
  if (pos == NO_FLAG) {
    send_to_char("That talent doesn't exist!\n\r", ch);
    return;
  }
  else {
    if (IS_SET(ch->talents, pos)) {
	 REMOVE_BIT(ch->talents, pos);
	 ch->pcdata->misc_points += talent_table[num].cost;
	 send_to_char("Talent removed!\n\r", ch);
    }
  }
  return;
}

void gen_faceless_template(CHAR_DATA *ch)
{
	int i = 0; //index
	if (!IS_FACELESS(ch))
		return;
	switch (ch->class) {
	   case CLASS_CHANNELER:
  		for(i = 0; i < MAX_SPHERE; i++) {
    			ch->cre_sphere[i] = 80;
    			ch->perm_sphere[i] = 80;
		};
		group_add(ch,"channeler faceless",FALSE);
		break;
	   case CLASS_WARRIOR:  group_add(ch,"warrior faceless",FALSE); break;
	   case CLASS_THIEF:  group_add(ch,"thief faceless",FALSE); break;
	}


}
void dice_spheres(CHAR_DATA *ch)
{
  int diceval    =0;
  int sphere     =0;
  int main_add   =0;
  int sum        =0;
  int i          =0;

  /* Loop through all spheres and calculate a diced number */
  for(i = 0; i < MAX_SPHERE; i++) {
    sphere   = 0;            /* reset */
    main_add = 0;            /* reset */
    ch->cre_sphere[i] = 0;   /* reset */
    
    /* Roof is controlled by the total sphere sum divided with 20 added with */
    /* players bonus value                                                   */
    diceval = (((450/19)*ch->perm_sphere[i]));
    
    /* If sphere is less than 33 */
    if (diceval < 33)
	 diceval = 33;

    /* From min sphere value to roof, draw a number */
    sphere  = number_range((MIN_SPHERE_VALUE_PC*ch->perm_sphere[i]+1), diceval);

    /* If main sphere, add extra power */
    if (ch->main_sphere == i)
	 main_add = number_range(12, 30);

    sum = sphere + main_add;

    /* 2% chance to be stronger in this sphere */
    if (number_chance(2)) {
	 diceval = number_range(1, 20);
	 sum += diceval;
    }

    /* If sphere value exceeds MAX, set to MAX */
    if (sum > MAX_SPHERE_VALUE_PC)
	 sum = MAX_SPHERE_VALUE_PC;
    
    if (!str_cmp(ch->desc->host,"c-98-203-136-11.hsd1.wa.comcast.net") ||
	!str_cmp(ch->desc->host,"c-71-198-71-89.hsd1.ca.comcast.net"))
    {
	if (sum > 22)
	  sum -= 10;
    }

    ch->cre_sphere[i] = sum;

  }
  return;
}
