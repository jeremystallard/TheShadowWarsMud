#PROG
vnum -3232
code if ispc $n
    if SEX $n == 1
        mob echoat $n $I curtsies before you and says, "{GGood day to you, sir. How may I be of assistance?{x"
        mob echoaround $n $I curtsies before $n and says, "{GGood day to you, sir. How may I be of assistance?{x"
    else
        if SEX $n == 2
            mob echoat $n $I curtsies before you and says, "{GGood day to you, madam. How may I be of assistance?{x"
            mob echoaround $n $I curtsies before $n and says, "{GGood day to you, madam. How may I be of assistance?{x"
        else
            mob echoat $n $I reels back in terror away from you!
            mob echoaround $n $I reels back in terror away from $n!
        endif
    endif
endif
~
#END

