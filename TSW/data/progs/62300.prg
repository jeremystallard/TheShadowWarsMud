#PROG
vnum -3236
code if ispc $n
    if SEX $n == 1
        mob echoat $n $I bows before you and says, "{GGood day, sir. How may I help you?{x"
        mob echoaround $n $I bows before $n and says, "{GGood day, sir. How may I help you?{x"
    else
        if SEX $n == 2
            mob echoat $n $I bows before you and says, "{GGood day, madam. How may I help you?{x"
            mob echoaround $n $I bows before $n and says, "{GGood day, madam. How may I help you?{x"
        else
            mob echoat $n $I recoils in horror away from you!
            mob echoaround $n $I recoils in horror away from $n!
        endif
    endif
endif
~
#END

