for pattern in $*; do
   grep "$pattern" *.c || echo "Pattern $pattern not found"
done

