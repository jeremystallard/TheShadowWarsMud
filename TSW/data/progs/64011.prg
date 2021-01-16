#PROG
vnum -1525
code if ispc $n
    if carries $n 64303
    and carries $n 64304
    and carries $n 64305
    and carries $n 64306
    and carries $n 64307
    and carries $n 64308
    and carries $n 64309
        mob echoat $n $I grins as he takes the {ws{Wlip{ws{x of {wp{Wape{wr{x from you, "{cThe password is correct{x."
        mob echoaround $n $I grins as he takes some {ws{Wlip{ws{x of {wp{Wape{wr{x from $n, "{cThe password is correct{x."
        mob remove $n 64303
        mob remove $n 64304
        mob remove $n 64305
        mob remove $n 64306
        mob remove $n 64307
        mob remove $n 64308
        mob remove $n 64309
        mob junk all
        mob oload 64310
        give tree $n
    else
        mob echoat $n $I snarls at you, "{cStop cheating!{x"
        mob echoaround $n $I snarls at $n, "{cStop cheating!{x"
    endif
endif
~
#END

