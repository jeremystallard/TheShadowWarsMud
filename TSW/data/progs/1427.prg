#PROG
vnum 1427
code if ispc $n
    mob junk tail
    mob oload 1411 0 room
    mob echoat $n $I grins slyly at you as $j hands you some money, "{GGood job my friend, good job! Here is {x{Gyour reward...{x"
    mob echoaround $n $I grins slyly at $n as $j hands $n some money, "{GGood job my friend, good job! Here {x{Gis your reward...{x"
    mob force $n get coin
endif
~
#END

