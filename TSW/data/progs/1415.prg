#PROG
vnum 1415
code if ispc $n
    mob junk $o
    mob oload 1401
    mob echoat $n $I nods to you, "{GThank you, $n. Now take this slip of paper and return it to the Postmaster in the Rahad for a reward.{x"
    mob echoaround $n $I nods to $n, "{GThank you, $n. Now take this slip of paper and return it to the Postmaster in the Rahad for a reward.{x"
    give slip $n
endif
~
#END

