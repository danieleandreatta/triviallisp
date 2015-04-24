#ifndef __LISP_H__
#define __LISP_H__
typedef unsigned short int _type;
typedef unsigned int _ptr;

#define PTR_MAX UINT_MAX

typedef struct cell {
  _type type;
  _type subt;
  _ptr  a;
  _ptr  b;
} cell;

_ptr null(_ptr);
void print_symbol(_ptr);
_ptr str(char *);
_ptr sym(char *);
_ptr num(int);
_ptr new_cell(_type, _type, _ptr, _ptr);
_ptr atom(_ptr, _ptr, _type);
_ptr cons(_ptr, _ptr);
_ptr car(_ptr);
_ptr cdr(_ptr);
_ptr atomp(_ptr);
_ptr cond(_ptr);
_ptr eq(_ptr, _ptr);
void print(_ptr);
void print_list(_ptr);
_ptr eval(_ptr, _ptr);
_ptr not(_ptr);
_ptr and(_ptr, _ptr);
void dump_mem(void);
void push(_ptr);
_ptr pop(void);
_ptr listp(_ptr x);

#define TYPE(x) memory[(x)].type
#define SUBT(x) memory[(x)].subt
#define A(x)    memory[(x)].a
#define B(x)    memory[(x)].b
#define GET_SYM(x)  tbl[memory[(x)].a]

extern cell *memory;
extern char **tbl;

#define println(x) print(x); puts("")
#define printevalln(x,y) \
         curr=(x); \
         print(curr); puts(""); \
         print(eval(curr, y)); puts("")

#endif
