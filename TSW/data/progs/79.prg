#PROG
vnum 79
code if room $i != 76
  if hour == 20
  or hour == 12
    if isdelay $i
      mob cast haste
    else
      emote mews hungrily.
      emote wanders off on silent paws in search of food.
      mob goto 76
      emote enters the room on silent paws.
      emote eats some cat food from his bowl.
      if hour == 20
        yawn
        sleep couch
      endif
    endif
  endif
endif
~
#END

