#PROG
vnum 13371
code if hpcnt $i < 50
say You have not seen the last of me! I'll haunt your dreams for eternity.
mob echo A shimmering gateway appears and Lord Gaebril steps through it, disappearing.
mob oload 13371
drop spear
mob transfer rahvin
mob goto 13300
endif
~
#END

