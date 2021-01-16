#PROG
vnum -2436
code if HOUR < 6
    mob echoat $n $I looks at you in a suspicious manner.
    mob echoaround $n $I looks at $n in a suspicious manner.
else
    mob echoat $n $I smartly salutes you.
    mob echoaround $n $I smartly salute $n.
    if HOUR < 12
        if sex $n == 1
        say Good morning, sir.
        else
        say Good morning, madam.
        endif
    else
        if HOUR < 18
            if sex $n == 1
            say Good day, sir.
            else
            say Good day, madam.
            endif
        else
            if sex $n == 1
            say Good evening, sir.
            else
            say Good evening, madam.
            endif         
        endif
    endif
endif
~
#END

