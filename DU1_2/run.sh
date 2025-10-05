#./gennum 3000030000 
#./gennum 5000050000 100
#./gennum 7000070000 10000 > numbers.txt

#./verbank < numbers.txt
./gennum 2000020000 500 | ./verbank -v