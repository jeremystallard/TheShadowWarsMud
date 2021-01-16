#PROG
vnum -1532
code if ispc $n
    if isfollow $n
    else
        if sex $n == 1
            mob echoat $n $I bows before you and says, "{cGood day to you sir and welcome to {x{cIllian.{x"
            mob echoaround $n $I bows before $n and says, "{cGood day to you sir and welcome {x{cto Illian.{x"
        else
            mob echoat $n $I bows before you and says, "{cGood day to you madam and welcome {x{cto Illian.{x"
            mob echoaround $n $I bows before $n and says, "{cGood day to you madam and {x{cwelcome to Illian.{x"
        endif
        mob echo $I grins impishly, "{cI know of lots of places and things to do in Illian. {x{cIf you want to learn more about the city, say {g'{Gyes{g'{c.{x"
    endif
endif
~
#END

