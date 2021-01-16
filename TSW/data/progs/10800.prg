#PROG
vnum 10800
code if ispc $n
    if rand 66
        mob echo $I mutters to $kself as $j starts sweeping the floor.
        else
        if rand 66
            mob echo $I stops in the corridor to chat to one of $l colleagues.
            else
            mob echo $I pauses to wipe one of the lanterns with a soft cloth.
        endif
    endif
endif
~
#END

