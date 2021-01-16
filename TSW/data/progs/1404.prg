#PROG
vnum 1404
code if ispc $n
    if carries $n 1400
        mob echoat $n $I nods to you, "{GYou need to take the large parcel to Captain Sabin at the docks in the Rahad.{x"
        mob echoaround $n $I nods to $n, "{GYou need to take the large parcel to Captain Sabin at the docks in the Rahad.{x"
    else
        if carries $n 1402
            mob echoat $n $I nods to you, "{GYou need to take the small parcel to Zendrei at Zendrei's Curiosity Shop in the Rahad.{x"
            mob echoaround $n $I nods to $n, "{GYou need to take the small parcel to Zendrei at Zendrei's Curiosity Shop in the Rahad.{x"
        else
            if carries $n 1404
                mob echoat $n $I nods to you, "{GYou need to take the letter that is sealed with blue wax to Jerein at Jerein's Cooperage in the Rahad.{x"
                mob echoaround $n $I nods to $n, "{GYou need to take the letter that is sealed with blue wax to Jerein at Jerein's Cooperage in the Rahad.{x"
            else
                if carries $n 1405
                    mob echoat $n $I nods to you, "{GYou need to take the letter that is sealed with green wax to Blarid at Blarid's Workshop in the Rahad.{x"
                    mob echoaround $n $I nods to $n, "{GYou need to take the letter that is sealed with green wax to Blarid at Blarid's Workshop in the Rahad.{x"
                else
                    if carries $n 1406
                        mob echoat $n $I nods to you, "{GYou need to take the letter that is sealed with red wax to Hadyn at the docks in the Rahad.{x"
                        mob echoaround $n $I nods to $n, "{GYou need to take the letter that is sealed with red wax to Hadyn at the docks in the Rahad.{x"
                    else
                        if carries $n 1407
                            mob echoat $n $I nods to you, "{GYou need to take the letter that is sealed with yellow wax to Riki at his fishing boat in the Rahad.{x"
                            mob echoaround $n $I nods to $n, "{GYou need to take the letter that is sealed with yellow wax to Riki at his fishing boat in the Rahad.{x"
                        else
                            if carries $n 1408
                                mob echoat $n $I nods to you, "{GYou need to take the letter that is sealed with purple wax to Leon at his warehouse in the Rahad.{x"
                                mob echoaround $n $I nods to $n, "{GYou need to take the letter that is sealed with purple wax to Leon at his warehouse in the Rahad.{x"
                            else
                                if carries $n 1409
                                    mob echoat $n $I nods to you, "{GYou need to take the letter that is sealed with cyan wax to Lise Sedai at the Roaming Gleeman Inn in the {x{GRahad.{x"
                                    mob echoaround $n $I nods to $n, "{GYou need to take the letter that is sealed with cyan wax to Lise Sedai at the Roaming Gleeman Inn in {x{Gthe Rahad.{x"
                                endif
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

