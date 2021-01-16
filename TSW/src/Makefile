CC      = cc
#PROF    = -O3 -ggdb
PROF    = -O3
NOCRYPT =
RM_CMD  = rm -f *.o *.a *~
#C_FLAGS =  -Wall $(PROF) $(NOCRYPT) -DFIRST_BOOT
#C_FLAGS =  -Wall $(PROF) $(NOCRYPT) -pg
C_FLAGS =  -Wall $(NOCRYPT) -pg -ggdb3 -ggdb
#L_FLAGS =  $(PROF) -lcrypt -lefence
L_FLAGS =  -lcrypt 

O_FILES = act_comm.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
	alias.o ban.o comm.o const.o db.o db2.o effects.o fight.o flags.o \
	handler.o healer.o interp.o board.o lookup.o magic.o magic2.o \
	music.o recycle.o save.o scan.o skills.o special.o tables.o \
	update.o mob_cmds.o mob_prog.o olc.o olc_act.o olc_save.o bit.o \
	mem.o string.o pedit.o hedit.o cmdedit.o raedit.o skedit.o \
	sedit.o gedit.o tablesave.o screen.o track.o dice_games.o \
	vmap.o helplog.o weaves.o guild.o blowfish.o wolfkin.o \
	shadowspawn.o minion.o darkfriend.o quest.o sguild.o ssguild.o oguild.o \
	ogier.o letters.o bank.o vote.o vehicle.o objdump.o legions.o masterforms.o

tsw: $(O_FILES)
	rm -f tsw
	$(CC) $(L_FLAGS) -o tsw $(O_FILES)

purify: $(O_FILES)
	rm -f tsw
	purify $(CC) $(L_FLAGS) -o tsw $(O_FILES)

.c.o: merc.h
	$(CC) -c $(C_FLAGS) $<

clean::
	$(RM_CMD)
