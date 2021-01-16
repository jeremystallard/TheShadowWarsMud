#PROG
vnum -3229
code if ISPC $n
    if RAND 50
    mob echoat $n $I looks at you in a suspicious manner.
    mob echoaround $n $I looks at $n in a suspicious manner.
    else
        if RAND 50
        mob echoat $n $I eyes you warily.
        mob echoaround $n $I eyes $n warily.
        else
            if RAND 50
            mob echoat $n $I eyes you cautiously.
            mob echoaround $n $I eyes $n cautiously.
            else
                if RAND 50
                mob echoat $n $I watches you warily.
                mob echoaround $n $I watches $n warily.
                else
                    if RAND 50
                    mob echoat $n $I watches you cautiously.
                    mob echoaround $n $I watches $n cautiously.
                    else
                    mob echoat $n $I raises $l eyebrow in suspicion at you.
                    mob echoaround $n $I raises $l eyebrow in suspicion at $n.
                    endif
                endif
            endif
        endif
    endif
endif
~
#END

