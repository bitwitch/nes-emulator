This directory contains test programs that can be loaded into memory using the
repl program. 

1. Write a 6502 assembly program in a file like test.a  
2. Assemble the program; I am using the open source crossassembler [acme](https://web.archive.org/web/20150520143433/https://www.esw-heim.tu-clausthal.de/~marco/smorbrod/acme/)  
`$ acme --setpc 0xC000 --cpu 6502 -o test.o test.a`  
3. Load the program using the repl program   
`> load 0xC000 test.o`  
4. Execute the program on the emulated cpu  
`> execute`  


