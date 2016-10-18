#include <stdio.h>

struct to_copy {
    int bleh;
    int boop;
};

struct container{
    struct to_copy copied;
};
int main(){
    struct to_copy b;
    struct container a;
    b.bleh = 5;
    a.copied = b;
    printf("Struct value copied: %d\n", a.copied.bleh);
}
