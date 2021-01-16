#PROG
vnum 7705
code if carries $i 7956
    mob echoat $n There is a soft click as you place $O into a circular recess in $I's chest...
    mob echoaround $n There is a soft click as $N places $O into a circular recess in $I's chest...
    mob echo $I's eyes briefly flicker {bb{Blu{be{x!
else
    mob echoat $n There is a soft click as you place $O into a circular recess in $I's chest...
    mob echoaround $n There is a soft click as $N places $O into a circular recess in $I's chest...
    mob junk all.orb
    mob echo $I's eyes glow {rr{Re{rd{x as $j springs forward and attacks!
    mob kill $n
endif
~
#END

