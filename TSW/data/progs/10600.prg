#PROG
vnum 10600
code if isfollow $n
    else
    if ispc $n
        if race $n domani
            mob echo $I says, "CHECK #2 - You are a Domani."
            mob call 10601 $n
            else
            if race $n trolloc
            or race $n fade
            or race $n gholam
                mob call 10602 $n
                else
                if class $n thief
                    mob call 10603 $n
                    else
                    mob echoat $n $I shrugs his shoulders at you.
                    mob echoaround $n $I shrugs his shoulders at $n.
                endif
            endif
        endif
    endif
endif
~
#END

