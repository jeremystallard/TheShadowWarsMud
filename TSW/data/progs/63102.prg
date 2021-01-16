#PROG
vnum -2434
code if ispc $n
    if rand 50
        mob echo $I grumbles irritably to $kself as $j sweeps the floor.
    else
        if rand 50
            mob echo $I polishes one of the lamps.
        else
            if rand 50
                mob echo $I runs $l finger along a nearby table, inspecting it for signs of dust.
            else
                if sex $i == 1
                    mob echoat $n $I bows politely to you before hurrying off to do $l chores.
                    mob echoaround $n $I bows politely to $n before hurrying off to do $l chores.
                else
                    mob echoat $n $I curtsies politely to you before hurrying off to do $l chores.
                    mob echoaround $n $I curtsies politely to $n before hurrying off to do $l chores.
                endif
            endif
        endif
    endif
endif  
~
#END

