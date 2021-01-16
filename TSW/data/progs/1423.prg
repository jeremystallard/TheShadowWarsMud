#PROG
vnum 1423
code if ispc $n
    mob junk $o
    mob oload 1401
    mob echoat $n $I gives you a gracious nod, "{GThank you, $n. Now take this slip of paper back to the {x{GPostmaster in the Rahad for your reward.{x"
    mob echoaround $n $I gives $n a gracious nod, "{GThank you, $n. Now take this slip of paper back to the {x{GPostmaster in the Rahad for your reward.{x"
    give slip $n
endif
~
#END

