#PROG
vnum -3230
code if ISPC $n
    if SEX $n == 1
        if RAND 50
        mob echoat $n $I bows before you and says, "{GGreetings, sir. How may I be of assistance?{x"
        mob echoaround $n $I bows before $n and says, "{GGreetings, sir. How may I be of assistance?{x"
        else
            if RAND 50
            mob echoat $n $I smiles at you, "{GHello, sir. How may I help you?{x"
            mob echoaround $n $I smiles at $n, "{GHello, sir. How may I help you?{x"        
            else
                if RAND 50
                mob echoat $n $I says to you, "{GGood day to you, sir. How may I help you?{x"
                mob echoaround $n $I says to $n, "{GGood day to you, sir. How may I help you?{x"
                else
                    if RAND 50
                    mob echoat $n $I bows before you and says, "{GGood day, sir. How may I help you?{x"
                    mob echoaround $n $I bows before $n and says, "{GGood day, sir. How may I help you?{x"
                    else
                    mob echoat $n $I smiles at you, "{GLight bless you, sir. How may I be of assistance?{x"
                    mob echoaround $n $I smiles at $n, "{GLight bless you, sir. How may I be of assistance?{x"
                    endif
                endif
            endif
        endif
    else
        if RAND 50
        mob echoat $n $I bows before you and says, "{GGreetings, madam. How may I help you?{x"
        mob echoaround $n $I bows before $n and says, "{GGreetings, madam. How may I help you?{x"
        else
            if RAND 50
            mob echoat $n $I grins at you, "{GHello, madam. How may I be of assistance?{x"
            mob echoaround $n $I grins at $n, "{GHello, madam. How may I be of assistance?{x"        
            else
                if RAND 50
                mob echoat $n $I says to you, "{GGood day to you, madam. How may I be of assistance?{x"
                mob echoaround $n $I says to $n, "{GGood day to you, madam. How may I be of assistance?{x"
                else
                    if RAND 50
                    mob echoat $n $I bows before you and says, "{GGood day, madam. How may I help you?{x"
                    mob echoaround $n $I bows before $n and says, "{GGood day, madam. How may I help you?{x"
                    else
                    mob echoat $n $I smiles at you, "{GLight bless you, madam. How may I help you?{x"
                    mob echoaround $n $I smiles at $n, "{GLight bless you, madam. How may I help you?{x"
                    endif
                endif
            endif
        endif
    endif
endif
~
#END

