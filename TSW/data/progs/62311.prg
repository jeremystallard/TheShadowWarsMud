#PROG
vnum -3225
code if room $i == 62705
  if WEARS $n 62584
    bow $n
say Welcome $n!
  else
    say You shouldn't be here $n.
    mob force $n down
  endif
else
  say Blood and ashes! I should not be here...
  mob echo $I wanders off...
  mob goto 62705
  mob echo $I enters the room.
endif
~
#END

