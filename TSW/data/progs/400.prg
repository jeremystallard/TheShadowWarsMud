#PROG
vnum 400
code if carries $n 405 
and mobexists monstrous
mob echo A deafening screech comes from just above as something descends into the tree.
mob remember $n
mob delay 1
break
else
mob gtransfer $n 409
endif
~
#END

