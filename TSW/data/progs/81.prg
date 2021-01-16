#PROG
vnum 81
code mob remember Paradigm
if pos $q sleeping
  if hour == 6
    emote wakes Paradigm.
    mob vforce 76 wake
    mob vforce 76 stretch
    pat paradigm
  endif
endif
~
#END

