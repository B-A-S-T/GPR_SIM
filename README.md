# GPR_SIM

  Created by Ian Thomas with help of Lanxin Ma, September 2016.
  Group 1 Computer Architecture
  
  GPR simulator that allows for checking for palindromes.
  32 Registers
  ----------------------------------------------------------------------------------------------------
  COMPILE & RUN:
  gcc gpr_sim.c -w -o GPR_SIM
  ./GPR_SIM palindrome.s
  ----------------------------------------------------------------------------------------------------
  USAGE:
  .data section -> .text section
  Delcare data as so:
  .data
  .asciiz "String 0"
  .asciiz "String 1"
  .label label_name instruction_offset_in_hex
  .space string_name num_bytes
  
  .text
  LI $0, 15
  SYSCALL 9
  END

  SYSCALLS:
  Move address into $0
  Add number next to SYSCALL
  Functions:
  0 - Read string
  1 - Print String
  9 - Exit
  ----------------------------------------------------------------------------------------------------
  
  FINAL NOTES: 
  If file cannot be found when passing in source, try using the 
  path instead. Haven't had this problem but it is a possibility.
  
  Instructions are case sensitive. All instructions must be made with CAPS.
  Look at palindrome.s for complete details.
  
