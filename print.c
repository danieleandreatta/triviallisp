#include <stdio.h>

#include "lisp.h"
#include "symbols.h"

#if 0
void print_symbol_(_ptr x)
{
  if (null(x)) return;
  print_symbol(A(x));
  print_symbol_(B(x));
}
#endif

void print_atom(_ptr x)
{
  switch(SUBT(x)){
  case T:
    printf("T");
    break;
  case LETTER:
    //printf("%c",GET_SYM(x));
    break;
  case NUMBER:
    printf("%d", A(x));
    break;
  default:
    printf("%s", GET_SYM(x));
    break;
  }
}

void print_list(_ptr x)
{
  print(A(x));
  if (TYPE(B(x))!= NIL) {
    printf(" ");
    print_list(B(x));
  }
}

void print(_ptr x)
{
  switch (TYPE(x)) {
  case CONS:
    printf("(");
    if (listp(x)) {
      print_list(x);
    }
    else {
      print(car(x));
      printf(" . ");
      print(cdr(x));
    }
    printf(")");
    break;
  case ATOM:
    print_atom(x);
    break;
  case NIL:
    printf("NIL");
    break;
  default:
    break;
  }
}

