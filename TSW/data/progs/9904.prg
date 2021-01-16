#PROG
vnum 9904
code if isimmort $n
  if name $n Eldoran
    say Welcome home Eldoran!
  else
    say Welcome to Eldoran's Room, Immortal $n.
  endif
else
  say Welcome to Eldoran's room, $n.  Please make yourself comfortable.
endif
~
#END

