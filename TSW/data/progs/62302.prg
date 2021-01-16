#PROG
vnum -3234
code if ispc $n
    if SEX $n == 1
        mob echoat $n $I grins at you, "{GLight bless you, sir. How may I be of assistance?{x"
        mob echoaround $n $I grins at $n, "{GLight bless you, sir. How may I be of assistance?{x"
    else
        if SEX $n == 2
            mob echoat $n $I grins at you, "{GLight bless you, madam. How may I be of assistance?{x"
            mob echoaround $n $I grins at $n, "{GLight bless you, madam. How may I be of assistance?{x"
        else
            mob echoat $n $I recoils in horror away from you!
            mob echoaround $n $I recoils in horror away from $n!
        endif
    endif
endif
~
#END

