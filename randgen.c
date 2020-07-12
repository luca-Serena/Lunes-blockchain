#include <stdio.h>
void printRandoms(int lower, int upper,  
                             int count, FILE *fp) 
{ 
    int i; 
    for (i = 0; i < count; i++) { 
        int num = (rand() % 
           (upper - lower + 1)) + lower; 
        int num2 = (rand() % 
           (upper - lower + 1)) + lower; 
        fprintf(fp, "%d %d\n", num, num2); 
    } 
} 
  
// Driver code 
int main() 
{ 
    FILE *fp;
    fp = fopen("Rand2.seed", "w+");
    int lower = 0, upper = 2147483647
, count = 33063; 
  
    // Use current time as  
    // seed for random generator 
    srand(time(0)); 
  
    printRandoms(lower, upper, count, fp); 
    fclose (fp);
  
    return 0; 
} 
