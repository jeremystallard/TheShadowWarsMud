#PROG
vnum 1403
code if ispc $n
    mob junk $o
    mob oload 1401
    mob echoat $n $I says to you, "{GThank you very much, $n. Take this slip of paper back to the Postmaster in the Rahad for your reward.{x"
    mob echoaround $n $I says to $n, "{GThank you very much, $n. Take this slip of paper back to the Postmaster in the Rahad for your reward.{x"
    give slip $n
endif
~
#END

