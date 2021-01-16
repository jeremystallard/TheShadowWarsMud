#PROG
vnum 1407
code if ispc $n
    mob junk $o
    mob oload 1401
    mob echoat $n $I smiles at you, "{GThank you, $n. Return this slip of paper back to the Postmaster in the Rahad for your reward.{x"
    mob echoaround $n $I smiles at $n, "{GThank you, $n. Return this slip of paper back to the Postmaster in the Rahad for your reward.{x"
    give slip $n
endif
~
#END

