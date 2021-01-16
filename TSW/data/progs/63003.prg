#PROG
vnum -2533
code if level $n < 40
    mob echoat $n $I sneers at you, "{GBe a good little citizen and scoot back off to Low Caemlyn.{x"
    mob echoaround $n $I sneers at $n, "{GBe a good little citizen and scoot back off to Low Caemlyn.{x"
else
    mob echoat $n $I sniffs haughtily at you.
    mob echoaround $n $I sniffs haughtily at $n.
endif
~
#END

