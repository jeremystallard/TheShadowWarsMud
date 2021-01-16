#PROG
vnum -1523
code if ispc $n
    if carries $n 64303
        if carries $n 64304
            if carries $n 64305
                if carries $n 64306
                    if carries $n 64307
                        if carries $n 64308
                            if carries $n 64309
                                mob echoat $n $I gives you a toothy grin, "{cThat is the correct password.{x"
                                mob echoaround $n $I gives $n a toothy grin, "{cThat is the correct password.{x"
                                mob echoat $n As quick as a flash $I takes the {ws{Wlip{ws{x of {wp{Wape{wr{x from you.
                                mob echoaround $n As quick as a flash $I takes some {ws{Wlip{ws{x of {wp{Wape{wr{x from $n.
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
                                mob echoat $n $I snarls angrily at you, "{cYou're a cheater!{x"
                                mob echoaround $n $I snarls angrily at $n, "{cYou're a cheater!{x"
                            endif
                        else
                            mob echoat $n $I snarls angrily at you, "{cYou're a cheater!{x"
                            mob echoaround $n $I snarls angrily at $n, "{cYou're a cheater!{x"
                        endif
                    else
                        mob echoat $n $I snarls angrily at you, "{cYou're a cheater!{x"
                        mob echoaround $n $I snarls angrily at $n, "{cYou're a cheater!{x"
                    endif
                else
                    mob echoat $n $I snarls angrily at you, "{cYou're a cheater!{x"
                    mob echoaround $n $I snarls angrily at $n, "{cYou're a cheater!{x"
                endif
            else
                mob echoat $n $I snarls angrily at you, "{cYou're a cheater!{x"
                mob echoaround $n $I snarls angrily at $n, "{cYou're a cheater!{x"
            endif
        else
            mob echoat $n $I snarls angrily at you, "{cYou're a cheater!{x"
            mob echoaround $n $I snarls angrily at $n, "{cYou're a cheater!{x"
        endif
    else
        mob echoat $n $I snarls angrily at you, "{cYou're a cheater!{x"
        mob echoaround $n $I snarls angrily at $n, "{cYou're a cheater!{x"
   endif
endif
~
#END

