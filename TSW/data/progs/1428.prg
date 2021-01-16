#PROG
vnum 1428
code if ispc $n
    mob echo $I mutters to $kself as he puts something on the display in the wooden rack before moving back to the shop's counter.
    if rand 50
        mob oload 1461 0 none
        else
        mob oload 1450 0 none
    endif
endif
~
#END

