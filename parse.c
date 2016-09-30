#include <stdio.h>
#include <stdlib.h>
#include <string.h>





int main(){

    char test[] = "LB $30, ($23)";
    char reg[3];
    int reg_index;
    char *token = NULL;
    char *token1 = NULL;
    char *token2 = NULL;
    char *token3 = NULL;
    int reg1 = 0;
    token = strtok(test, " \t");
    printf("TOKENS: |%s|\n", token);
    token1 = strtok(NULL, ", ");
    reg1 = (int) strtol((token1 + 1), (char **)NULL, 10);
    printf("TOKENS: |%d|\n", reg1);
    token2 = strtok(NULL, ", ");
    reg_index = strcspn(token2, "$");
    printf("INDEX: %d", strlen(token2));
    token2[strlen(token2) -1] = '\0';
    printf("INDEX: %s", token2);
    memcpy(reg, token2, reg_index-1);
   printf("TOKENS: |%s|\n", reg);
//    token3 = strtok(NULL, ", ");
 //   printf("TOKENS: |%s|\n", token3);

}
