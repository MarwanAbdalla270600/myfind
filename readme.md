# Myfind


## Programm Description
**myfind** is a minimal version of the linux **find** command. Supported parameters are 

 - print
 - ls
 - name
 - type 
 - user
For further information check the man page of the **find** command. The options of **myfind** are supposed to act exactly like the original **find** command.

## Execution 

The programm is compiled by following command:
gcc -std=c99 -Wall -Wextra -pedantic -Wno-unused-parameter main.c -o ./myfind
The programm is executed by following command:
./myfind

## Restriction

You can add any combination of parameters, however they are limited to 100 parameters. If there is need for you can change the restriction of 100 in the source code. 


