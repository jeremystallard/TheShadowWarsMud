#PROG
vnum -2532
code if level $n < 40
    mob echoat $n $I shakes $l head at you, "{GThis place is not for your sort!{x"
    mob echoaround $n $I shakes $l head at $n, "{GThis place is not for your sort!{x"
else
    mob echo $I shakes $l head sadly, "{GI would rather be in good ol' Lugard than here.{x"
endif
~
#END

