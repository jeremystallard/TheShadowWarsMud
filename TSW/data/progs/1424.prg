#PROG
vnum 1424
code if ispc $n
and level $n > 19
    if carries $n 1410
        mob echoat $n $I casts a shifty glance at you, "{GHave you brought me any rats' tails?{x"
        mob echoaround $n $I casts a shifty glance at $n, "{GHave you brought me any rats' tails?{x"
    else
        mob echoat $n $I whispers to you, "{GPst! Would you like to earn some easy cash?{x"
        mob echoaround $n $I whispers to $n, "{GPst! Would you like to earn some easy cash?{x"
    endif
endif
~
#END

