#define FRACTION (1<<14)
#include <stdint.h>
#include <stdio.h>

int Int_SUB_Float(int i, int f);
int Int_MUL_Float(int i, int f);
int Float_ADD_Int(int f, int i);
int Float_MUL_Float(int f, int i);
int Float_DIV_Float(int f1, int f2);
int Float_SUB_Float(int f1, int f2);
int Float_DIV_Int(int f, int i);
int Float_ADD_Float(int f1, int f2);

int Int_SUB_Float(int i, int f) {
  int res=i*FRACTION - f; 
  return res;  
}
  
int Int_MUL_Float(int i, int f) {
  int res=i*f;
  return res;
}

int Float_ADD_Int(int f, int i) {
  int res= f + i*FRACTION;
  return res;
}  

int Float_MUL_Float(int f, int i) {
  int64_t temp = f;
  temp = temp*i / FRACTION;
  int res=(int)temp;
  return res;
}

int Float_DIV_Float(int f1, int f2) {
  int64_t temp = f1;
  temp = temp*FRACTION / f2;
  int res=(int)temp;
  return res;
}

int Float_ADD_Float(int f1, int f2) {
  int res=f1+f2;
  return res;
}

int Float_SUB_Float(int f1, int f2) {
  int res=f1-f2;
  return res;
}

int Float_DIV_Int(int f, int i) {
  int res = f/i;
  return res;
}