#PROG
vnum -3224
code if room $i == 62705
else
  say Blood and ashes! I should not be here...
  mob echo $I wanders off...
  mob goto 62705
  mob echo $I enters the room.
endif
~
#END

