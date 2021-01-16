#PROG
vnum 1426
code if ispc $n
    if carries $n 1410
        mob echoat $n $I beams broadly at you, "{GWell... what are you waiting for? Hand those rats' tails {x{Gover to me, $n!{x"
        mob echoaround $n $I beams broadly at $n, "{GWell... what are you waiting for? Hand those rats' {x{Gtails over to me, $n!{x"
        else
        if level $n > 19
            mob echoat $n $I beams broadly at you, "{GThat's what I like to hear... enthusiasm!{x"
            mob echoaround $n $I beams broadly at $n, "{GThat's what I like to hear... enthusiasm!{x"
            mob echoat $n $I's head tilts slightly to one side as $j considers you, "{GI need some help to {x{Gkill off the rats in this area.{x"
            mob echoaround $n $I's head tilts slightly to one side as $j considers $n, "{GI need some help {x{Gto kill off the rats in this area.{x"
            mob echo $I shakes a coin purse and it jingles loudly, "{GBring me back their tails and I will {x{Gpay you one gold coin for each of them.{x"
        endif
    endif
endif
~
#END

