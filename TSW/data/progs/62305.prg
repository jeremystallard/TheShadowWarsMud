#PROG
vnum -3231
code if ISPC $n
    if SEX $n == 1
        if RAND 50
        mob echoat $n $I curtsies before you and says, "{GGreetings, sir. How may I help you?{x"
        mob echoaround $n $I curtsies before $n and says, "{GGreetings, sir. How may I help you?{x"
        else
            if RAND 50
            mob echoat $n $I smiles pleasantly at you, "{GHello, sir. How may I be of assistance?{x"
            mob echoaround $n $I smiles pleasantly at $n, "{GHello, sir. How may I be of assistance?{x"        
            else
                if RAND 50
                mob echoat $n $I says to you, "{GGood day, sir. How may I help you?{x"
                mob echoaround $n $I says to $n, "{GGood day, sir. How may I help you?{x"
                else
                    if RAND 50
                    mob echoat $n $I curtsies before you and says, "{GGood day, sir. How may I help you?{x"
                    mob echoaround $n $I curtsies before $n and says, "{GGood day, sir. How may I help you?{x"
                    else
                    mob echoat $n $I smiles at you, "{GLight bless you, sir. How may I be of assistance?{x"
                    mob echoaround $n $I smiles at $n, "{GLight bless you, sir. How may I be of assistance?{x"
                    endif
                endif
            endif
        endif
    else
        if RAND 50
        mob echoat $n $I curtsies before you and says, "{GGreetings, madam. How may I help you?{x"
        mob echoaround $n $I curtsies before $n and says, "{GGreetings, madam. How may I help you?{x"
        else
            if RAND 50
            mob echoat $n $I smiles pleasantly at you, "{GHello, madam. How may I be of assistance?{x"
            mob echoaround $n $I smiles pleasantly at $n, "{GHello, madam. How may I be of assistance?{x"        
            else
                if RAND 50
                mob echoat $n $I says to you, "{GGood day, madam. How may I help you?{x"
                mob echoaround $n $I says to $n, "{GGood day, madam. How may I help you?{x"
                else
                    if RAND 50
                    mob echoat $n $I curtsies before you and says, "{GGood day, madam. How may I help you?{x"
                    mob echoaround $n $I curtsies before $n and says, "{GGood day, madam. How may I help you?{x"
                    else
                    mob echoat $n $I smiles at you, "{GLight bless you, madam. How may I be of assistance?{x"
                    mob echoaround $n $I smiles at $n, "{GLight bless you, madam. How may I be of assistance?{x"
                    endif
                endif
            endif
        endif
    endif
endif
~
#END

