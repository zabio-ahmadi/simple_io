#include <stdio.h>

int inc(int value, int notUsed) {
    return ++value;
}

int inc4(int value, int notUsed) {
    return value + 4;
}

int init(int notUsed, int index) {
    return index;
}

int print(int value, int index) {
    if(index == 0)
        printf("[");
    printf("%d ", value);
    return value;
}

void map(int (*func) (int, int), int tab[], int length) {
    for(int i=0;i<length;i++)
        tab[i] = func(tab[i], i);
}

#define SIZE 10
int main() {
    int nums[SIZE];
    map(init, nums, SIZE);
    map(print, nums, SIZE);
    map(inc, nums, SIZE);
    map(print, nums, SIZE);
    map(inc4, nums, SIZE);
    map(print, nums, SIZE);
}
