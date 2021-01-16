#PROG
vnum 1421
code if ispc $n
    mob junk $o
    mob oload 1401
    mob echoat $n $I flashes a wide grin at you, "{GThank you very much, $n. Now take this slip of paper {x{Gback to the Postmaster in the Rahad for your reward.{x"
    mob echoaround $n $I flashes a wide grin at $n, "{GThank you very much, $n. Now take this slip of paper {x{Gback to the Postmaster in the Rahad for your reward.{x"
    give slip $n
endif
~
#END

