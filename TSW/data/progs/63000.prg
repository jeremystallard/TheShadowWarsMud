#PROG
vnum -2536
code if HOUR < 6
    mob echoat $n $I looks at you in a suspicious manner.
    mob echoaround $n $I looks at $n in a suspicious manner.
    say Cor blimey! You're up early!
else
    if HOUR < 12
        mob echoat $n $I smartly salutes you.
        mob echoaround $n $I smartly salute $n.
        say Good morning, citizen.
    else
        if HOUR < 18
            mob echoat $n $I smartly salutes you.
            mob echoaround $n $I smartly salute $n.
            say Good day, citizen.
        else
            mob echoat $n $I smartly salutes you.
            mob echoaround $n $I smartly salute $n.
            say Good evening, citizen.
        endif
    endif
endif
~
#END

