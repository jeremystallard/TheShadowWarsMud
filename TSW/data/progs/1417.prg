#PROG
vnum 1417
code if ispc $n
    mob junk $o
    mob oload 1401
    mob echoat $n $I says to you, "{GYou'd better take this slip of paper back to the Postmaster in the Rahad for a reward.{x"
    mob echoaround $n $I says to $n, "{GYou'd better take this slip of paper back to the Postmaster in the Rahad for a reward.{x"
    give slip $n
endif
~
#END

