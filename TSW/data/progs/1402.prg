#PROG
vnum 1402
code if ispc $n
    if carries $n 1400
    or carries $n 1402
        mob echoat $n $I frowns at you, "{GYou have not delivered the parcel that I gave you.{x"
        mob echoaround $n $I frowns at $n, "{GYou have not delivered the parcel that I gave you.{x"
        else
        if carries $n 1404
        or carries $n 1405
            mob echoat $n $I frowns at you, "{GYou have not delivered the letter that I gave you.{x"
            mob echoaround $n $I frowns at $n, "{GYou have not delivered the letter that I gave you.{x"
            else
            mob call 1405 $n
        endif
    endif
endif
~
#END

