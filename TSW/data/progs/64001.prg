#PROG
vnum -1535
code mob echo $I studies the coin very carefully... 
mob echoat $n $I smiles at you, "{cThis old coin is from Manetheren. I shall give you ten {x{cgold coins for it.{x"
mob echoaround $n $I smiles at $n, "{cThis old coin is from Manetheren. I shall give you ten {x{cgold coins for it.{x"
mob junk all
mob oload 64288
give bag $n
~
#END

