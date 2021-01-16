#PROG
vnum -1534
code mob echo $I studies the coin very carefully... 
mob echoat $n $I frowns at you, "{cThis old coin is from Essenia. I shall give you five gold {x{ccoins for it.{x"
mob echoaround $n $I frowns at $n, "{cThis old coin is from Essenia. I shall give you five {x{cgold coins for it.{x"
mob junk all
mob oload 64294
give bag $n
~
#END

