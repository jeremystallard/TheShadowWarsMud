#PROG
vnum 1409
code if ispc $n
    if rand 50
        mob echo $I shakes $k head slowly, "{G$n... $O{G does not belong to me.{x"
        give $o $n
    else
        if rand 50
            mob echo $I looks at $O and frowns, "{G$n... $O{G does not belong to me.{x"
            give $o $n
        else
            if rand 50
                mob echoat $n $I shrugs $k shoulders at you, "{G$n... $O{G does not belong to me.{x"
                mob echoaround $n $I shrugs $k shoulders at $n, "{G$n... $O{G does not belong to me.{x"
                give $o $n
            else
                if rand 50
                    mob echo $I sighs sadly, "{G$n... $O{G does not belong to me.{x"
                    give $o $n
                else
                    if rand 50
                        mob echo $I shakes $k head slowly, "{GThis item does not belong to me, $n.{x"
                        give $o $n
                    else
                        if rand 50
                            mob echo $I looks at $O and frowns, "{GThis item does not belong to me, $n.{x"
                            give $o $n
                        else
                            if rand 50
                                mob echoat $n $I shrugs $k shoulders at you, "{GThis item does not belong to me, $n.{x"
                                mob echoaround $n $I shrugs $k shoulders at $n, "{GThis item does not belong to me, $n.{x"
                                give $o $n
                            else
                                mob echo $I sighs sadly, "{GThis item does not belong to me, $n.{x"
                                give $o $n
                            endif
                        endif
                    endif
                endif
            endif
        endif
    endif
endif
~
#END

