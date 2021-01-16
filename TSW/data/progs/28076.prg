#PROG
vnum 28076
code , begins to shuffle the cards, skillfully allowing them to mix together as they shuffle through his fingers. One by one, he deals the cards to each of the players, finishing with himself.
 
$highscore = 0
$winner = none
$player1 = Edorion
$player2 = Estean
$player3 = $n
$player4 = Carlomin
 
$player1 rand 21
$player2 rand 21
$player3 rand 21
$player4 rand 21

if $player1 > $highscore then $player1 = $winner
if $player2 > $highscore then $player2 = $winner
if $player3 > $highscore then $player3 = $winner
if $player4 > $highscore then $player4 = $winner
 
mob oload 28238
 
give $winner silver
~
#END

