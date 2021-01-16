#PROG
vnum -3228
code if carries $n 62572
say You have a red token!
mob echoat $n $I takes the red token from you.
mob echoaround $n $I takes the red token from $n.
mob remove $n 62572
mob junk all
mob oload 62584
say Wear this brooch and it should get you past that pesky guard in the Crown of Roses Inn.
give brooch $n
endif
if carries $n 62577 
say Ah! You have a green token!
mob echoat $n $I takes the green token from you.
mob echoaround $n $I takes the green token from $n.
mob remove $n 62577
mob junk all
mob oload 62585
say This key should be useful to you in the Crown of Roses Inn.
give key $n
endif
if carries $n 62582 
say At last! You have a blue token!
mob echoat $n $I takes the blue token from you.
mob echoaround $n $I takes the blue token from $n.
mob remove $n 62582
mob junk all
mob oload 62586
say You can use this key in the Crown of Roses Inn.
give key $n
endif
~
#END

