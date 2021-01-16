#PROG
vnum -2534
code if level $n < 40
    mob echoat $n $I points at you and exclaims, "{GSomebody remove this beggar from here at once!{x"
    mob echoaround $n $I points at $n and exclaims, "{GSomebody remove this beggar from here at once!{x"
else
    mob echoat $n $I sniffs haughtily at you.
    mob echoaround $n $I sniffs haughtily at $n.
endif
~
#END

