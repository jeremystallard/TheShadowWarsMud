#PROG
vnum 1411
code if ispc $n
    mob junk $o
    mob oload 1401
    mob echoat $n $I grins at you, "{GTa very much, $n. Give this slip of paper to the Postmaster in the Rahad for a reward.{x"
    mob echoaround $n $I grins at $n, "{GTa very much, $n. Give this slip of paper to the Postmaster in the Rahad for a reward.{x"
    give slip $n
endif
~
#END

