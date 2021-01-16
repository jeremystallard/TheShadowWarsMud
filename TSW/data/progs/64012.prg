#PROG
vnum -1524
code if ispc $n
    if sex $n == 1
        mob echoat $n $I smirks at you, "{cThere is no need to be sarcastic, mate.{x"
        mob echoaround $n $I smirks at $n, "{cThere is no need to be sarcastic, mate.{x"
    else
        mob echoat $n $I sneers at you, "{cGet back in the kitchen, wench!{x"
        mob echoaround $n $I sneers at $n, "{cGet back in the kitchen, wench!{x"
    endif
endif
~
#END

