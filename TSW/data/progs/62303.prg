#PROG
vnum -3233
code if ispc $n
    if SEX $n == 1
        mob echoat $n $I curtsies before you and says, "{GLight bless you, sir. How may I be of assistance?{x"
        mob echoaround $n $I curtsies before $n and says, "{GLight bless you, sir. How may I be of assistance?{x"
    else
        if SEX $n == 2
            mob echoat $n $I curtsies before you and says, "{GLight bless you, madam. How may I be of assistance?{x"
            mob echoaround $n $I curtsies before $n and says, "{GLight bless you, madam. How may I be of assistance?{x"
        else
            mob echoat $n $I shrinks back in horror away from you!
            mob echoaround $n $I shrinks back in horror away from $n!
        endif
    endif
endif
~
#END

