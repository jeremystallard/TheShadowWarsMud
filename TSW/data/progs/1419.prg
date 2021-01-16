#PROG
vnum 1419
code if ispc $n
    mob junk $o
    mob oload 1401
    mob echoat $n $I grins broadly at you, "{GThanks! Don't forget to return this slip of paper back to the Postmaster in the Rahad for a reward.{x"
    mob echoaround $n $I grins broadly at $n, "{GThanks! Don't forget to return this slip of paper back to the Postmaster in the Rahad for a reward.{x"
    give slip $n
endif
~
#END

