#PROG
vnum 1400
code if ispc $n
    if carries $n 1400
    or carries $n 1402
    or carries $n 1404
    or carries $n 1405
    or carries $n 1406
    or carries $n 1407
    or carries $n 1408
    or carries $n 1409
        mob echoat $n $I says to you, "{GGood day, $n. You still need to deliver that item which I gave you earlier.{x"
        mob echoaround $n $I says to $n, "{GGood day, $n. You still need to deliver that item which I gave you earlier.{x"
        mob echo $I says, "{GIf you need a reminder of where to deliver it then just say 'help'.{x"
        else
        if carries $n 1401
            mob echoat $n $I grins at you, "{GGood day, $n. I see that you have a delivery slip for me...{x"
            mob echoaround $n $I grins at $n, "{GGood day, $n. I see that you have a delivery slip for me...{x"
            else
            mob echo $I sighs as $j sorts through a pile of letters and parcels that lies upon the counter, "{GI have so much work to do and so little time to do it in...{x"
            mob echoat $n $I says to you, "{GWould you be kind enough to help me deliver some of these items?{x"
            mob echoaround $n $I says to $n, "{GWould you be kind enough to help me deliver some of these items?{x"
        endif
    endif
endif
~
#END

