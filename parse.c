#include <stdio.h>
#include <stdlib.h>
#include <string.h>





int main(){

    char test[] = "ADDI $30, Rsrc1, 11";

    char *token = NULL;
    char *token1 = NULL;
    char *token2 = NULL;
    char *token3 = NULL;
    int reg = 0;
    token = strtok(test, " \t");
    printf("TOKENS: |%s|\n", token);
    token1 = strtok(NULL, ", ");
    reg = (int) strtol((token1 + 1), (char **)NULL, 10);
    printf("TOKENS: |%d|\n", reg);
    token2 = strtok(NULL, ", ");
    printf("TOKENS: |%s|\n", token2);
    token3 = strtok(NULL, ", ");
    printf("TOKENS: |%s|\n", token3);


}
