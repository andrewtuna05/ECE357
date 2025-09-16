#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
    int i,r,fn();
    for(i=0;i<10;i++)
        r=fn();
    printf("%d\n",r);
}
int fn(void){
    static int s=5;
    return s++;
}