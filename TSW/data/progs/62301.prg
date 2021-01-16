#PROG
vnum -3235
code if ispc $n
    if SEX $n == 1
        mob echoat $n $I curtsies before you and says, "{GHello, sir. How may I help you?{x"
        mob echoaround $n $I curtsies before $n and says, "{GHello, sir. How may I help you?{x"
    else
        if SEX $n == 2
            mob echoat $n $I curtsies before you and says, "{GHello, madam. How may I help you?{x"
            mob echoaround $n $I curtsies before $n and says, "{GHello, madam. How may I help you?{x"
        else
            mob echoat $n $I shrinks back in horror away from you!
            mob echoaround $n $I shrinks back in horror away from $n!
        endif
    endif
endif
~
#END

