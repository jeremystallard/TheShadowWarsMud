#PROG
vnum -1526
code if ispc $n
    if isfollow $n
    else
        mob echoat $n $I warily eyes you and says, "{cDo you know the password?{x"
        mob echoaround $n $I warily eyes $n and says, "{cDo you know the password?{x"
    endif
endif
~
#END

