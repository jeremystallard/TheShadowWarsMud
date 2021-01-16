#PROG
vnum -2535
code if level $n < 40
    mob echoat $n $I exclaims at you, "{GGet away from me, you ruffian!{x"
    mob echoaround $n $I exclaims at $n, "{GGet away from me, you ruffian!{x"
else
    mob echoat $n $I raises $l eyebrow in suspicion at you.
    mob echoaround $n $I raises $l eyebrow in suspicion at $n.
endif
~
#END

