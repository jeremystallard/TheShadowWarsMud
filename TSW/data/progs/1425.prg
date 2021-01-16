#PROG
vnum 1425
code if ispc $n
and level $n > 19
    if carries $n 1410
    else
        mob echoat $n $I hisses angrily at you, "{GI'll bloody well find someone else to help me then!{x"
        mob echoaround $n $I hisses angrily at $n.
    endif
endif
~
#END

