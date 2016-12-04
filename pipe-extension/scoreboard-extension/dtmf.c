#include <stdio.h>


int main(char* args[]){
    float kx = 1.65;
    float x2 = -0.57;
    float x1 = 0.0;
    float ky = 0.80;
    float y2 = -0.92;
    float y1 = 0.0;

    float x0;
    float y0;


    int top = 100;
    int count = 0;
    while(top != count){
        x0 = (kx * x1) - x2;
        y0 = (ky * y1) - y2;
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = y0;

        printf("Float: X0:%f\n", x0);
        printf("Float: Y0:%f\n\n\n", y0);
        count += 1;
    }


}
