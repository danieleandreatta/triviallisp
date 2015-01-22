#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <setjmp.h>
#include <unistd.h>

#include "lisp.h"
#include "reader.h"
#include "symbols.h"
#include "logging.h"

_ptr nil;     // Symbol NIL
_ptr t;       // Symbol T
_ptr global;  // Symbol global definitions
_ptr curr;    // Current Expression
_ptr stack;   // Local definitions

jmp_buf j_b;

#define N 1000
cell *memory;
_ptr mem;
_ptr free_mem = 0;

#define Ntbl 500
#define SYM_LEN 20

int _trace=0;

char **tbl;
int tbl_pos=0;

void def_add(_ptr x, _ptr y)
{
  global = cons(cons(x, cons(y, nil)), global );
}

_ptr sym_(char *x)
{
  int i, k=-1;
  for (i=0; i<tbl_pos; ++i){
    if (!strcasecmp(x, tbl[i])) {
      k = i;
      break;
    }
  }
  if (k<0) {
    if (tbl_pos<Ntbl) {
      k = tbl_pos;
      for (i=0; i<=strlen(x); ++i) {
        tbl[tbl_pos][i] = toupper((int)x[i]);
      }
      tbl_pos++;
    }
    else {
      error("Out of symbol space. Aborting...");
      exit(EXIT_FAILURE);
    }
  }
  return (_ptr)k;
}

__inline__ _ptr sym(char *s)
{
  return atom(sym_(s), nil, SYMBOL);
}

void dump_sym()
{
  int i;
  printf("--- Symbol Table ---\n");
  for (i=0; i<tbl_pos; ++i) {
    printf("%d -- %s\n", i, tbl[i]);
  }
}

#define MARK(i)   memory[(i)].subt |= 0x8000
#define UMARK(i)  memory[(i)].subt &= 0x7FFF
#define MARKED(i) memory[(i)].subt &  0x8000

void gc_mark(_ptr a)
{
  MARK(a);
  if (not(atomp(a))) {
    gc_mark(A(a));
    gc_mark(B(a));
  }
}
  
void gc(_type type, _ptr a, _ptr b)
{
  int i;

  info("** Performig GC...");

  //  dump_mem();
  // Mark phase
  if (type == CONS) {
    // These are the current poiters to the result being calculated
    gc_mark(a);
    gc_mark(b);
  }
  gc_mark(global); // Global definitions
  gc_mark(stack);  // Local definitions
  gc_mark(curr);   // Current program being executed

  //dump_mem();

  debug("Done mark phase");

  // Sweep phase
  mem = nil;
  free_mem = 0;
  for (i=N-1; i>1; --i) {
    if (MARKED(i)) {
      UMARK(i);
    }
    else {
      B(i) = mem;
      ++free_mem;
      mem  = i;
    }
  }
  info("** Free memory %d", free_mem);
}

__inline__ void push(_ptr x)
{
  stack = cons(x,stack);
}

__inline__ _ptr pop()   
{
  _ptr _tos_;
  _tos_ = A(stack);
  stack = B(stack);
  return _tos_;
}

void mem_init(int mem_size, int sym_size, int sym_len)
{
  int i;

  memory = (cell*) malloc(mem_size * sizeof(cell));

  // allocate space for tbl matrix.

  tbl = (char **) malloc(sizeof(*tbl) * sym_size);

  char *tmp = (char *) malloc(sizeof(**tbl) * sym_size * (sym_len+1));

  for (i=0; i<sym_size; ++i) {
    tbl[i] = tmp + i * (sym_len+1);
  }

  // We handle nil and t in an ad-hoc way. 
  // nil is always in position 0.
  // t is always in position 1.

  // NIL
  TYPE(0) = NIL;
  SUBT(0) = NIL;
  A(0)    = sym_("NIL");
  B(0)    = 0;
  nil = 0;

  // T
  TYPE(1) = ATOM;
  SUBT(1) = T;
  A(1)    = sym_("T");
  B(1)    = 0;
  t = 1;

  for (i=2; i<N-1; ++i) {
    B(i) = i+1;
  }
  B(N-1) = nil;
  mem = 2;
  free_mem = N-2;
}

_ptr alloc_cell_(_type type, _ptr a, _ptr b, int try_gc) {
  // type, a, b are the current intermediate results
  // for the expression being evaluated.
  // we need to know them here so we do not GC them

  _ptr x;
  //  printf("Free mem: %d\n", free_mem);
  if (mem != nil) {
    x = mem;
    --free_mem;
    mem = B(mem);
  }
  else {
    if (try_gc) {
      gc(type, a, b);
      x = alloc_cell_(type, a, b, 0);
    } else {
      error("Out of memory. Aborting!");
      exit(EXIT_FAILURE);
    }
  }
  return x;
}

_ptr alloc_cell(_type type, _ptr a, _ptr b) {
  return alloc_cell_(type, a, b, 1);
}

_ptr str(char *name)
{
  _ptr c, a;

  if (*name) {
    a = atom((_ptr)name[0], nil, LETTER);
    c = cons(a, str(name+1));
  }
  else {
    c = nil;
  }
  c = atom(c, nil, TEXT);
  return c;
}

_ptr num(int x)
{
  return atom((_ptr)x, nil, NUMBER);
}

_ptr cons(_ptr car, _ptr cdr)
{
  return new_cell(CONS, 0, car, cdr);
}

_ptr atom(_ptr sym, _ptr data, _type subt)
{
  return new_cell(ATOM, subt, sym, data);
}

_ptr new_cell(_type type, _type subt, _ptr a, _ptr b)
{
  _ptr x;

  x = alloc_cell(type, a, b);
  TYPE(x) = type;
  SUBT(x) = subt;
  A(x)    = a;
  B(x)    = b;

  return x;
}

void dump_mem()
{
  int i, k;
  _ptr x;

  printf("--- Memory Dump ---\n");
  for (i=0; i<N/2; ++i) {
    for (k=0; k<2; ++k) {
      x = i*2+k;
      printf("%4.4X%4.4X %8.8X %8.8X ", TYPE(x), SUBT(x), A(x), B(x));
    }
    printf("\n");
  }
}
 
void init(int mem_size, int sym_size, int sym_len)
{
  _ptr y;

  mem_init(mem_size, sym_size, sym_len); // Defines nil and t also.

  stack = nil;
  global = nil;
  curr = nil;

  y   = sym("car");
  def_add(y, y);

  y   = sym("cdr");
  def_add(y, y);

  y   = sym("cond");
  def_add(y, y);

  y   = sym("cons");
  def_add(y, y);

  y   = sym("atomp");
  def_add(y, y);

  y   = sym("lambda");
  def_add(y,y);
  
  y   = sym("defun");
  def_add(y,y);
}

_ptr null(_ptr x)
{
  if (x==nil) {
    return t;
  }
  else {
    return nil;
  }
}

__inline__ _ptr car(_ptr x)
{
  return A(x);
}

__inline__ _ptr cdr(_ptr x)
{
  return B(x);
}

_ptr numberp(_ptr x)
{
  return (TYPE(x)==ATOM && SUBT(x) == NUMBER) ? t : nil;
}

_ptr atomp(_ptr x)
{
  return (TYPE(x)==ATOM || TYPE(x)==NIL) ? t : nil;
}

__inline__ _ptr caar(_ptr x)
{
  //  (car (car x));
  return A(A(x));
}

__inline__ _ptr cadr(_ptr x)
{
  //  (car (cdr x));
  return A(B(x));
}

__inline__ _ptr caddr(_ptr x)
{
  //  (car (cdr (cdr x)));
  return A(B(B(x)));
}

__inline__ _ptr caddar(_ptr x)
{
  //  (car (cdr (cdr (car x))));
  return A(B(B(A(x))));
}

__inline__ _ptr cadar(_ptr x)
{
  //  (car (cdr (car x)));
  return A(B(A(x)));
}

__inline__ _ptr caadr(_ptr x)
{
  //  (car (car (cdr x)));
  return A(A(B(x)));
}

_ptr assoc(_ptr x, _ptr y)
{
  if (null(y)) {
    return nil;
  }
  else if (eq(caar(y), x)) {
    return cadar(y);
  }
  else {
    return assoc(x, cdr(y));
  }
}

_ptr append(_ptr x, _ptr y)
{
  if (null(x)) {
    return y;
  }
  else {
    return cons(car(x), append(cdr(x), y));
  }
}

_ptr pair(_ptr x, _ptr y)
{
  if (and(null(x), null(y))) {
    return nil;
  }
  else if (and(not(atomp(x)), not(atomp(y)))) {
    return cons(cons(car(x), cons(car(y),nil)), pair(cdr(x), cdr(y)));
  }
  else {
    error("Error in PAIR\n");
    return nil;
  }
}

_ptr list_eval(_ptr l, _ptr env)
{
  if (null(l)) {
    return nil;
  }
  else {
    push(eval(car(l), env));
    push(list_eval(cdr(l), env));
    return cons(pop(), pop());
  }
}

_ptr cond_eval(_ptr c, _ptr env)
{
  if (eval(caar(c), env)) {
    return eval(cadar(c), env);
  }
  else {
    return cond_eval(cdr(c), env);
  }
}

_ptr eval(_ptr e, _ptr env)
{
  _ptr r;

  if (_trace) {
    printf("- ");
    println(e);
  }

  // Save the local parameters
  push(env);

  if (atomp(e)) {
    // Simple atom, find/print its definition
    r = nil;

    if (numberp(e)) {
      r = e;
    }
    else if (null(e)) {
      r = nil;
    }
    else if (eq(e, t)) {
      r = t;
    }
    else {
      r = assoc(e, env);
    }
  }
  else if (atomp(car(e))) {
    // Function. see if it is an internal one first
    if (eq(car(e), sym("quote"))) {
      r = cadr(e);
    }
    else if (eq(car(e), sym("numberp"))) {
      r = numberp(eval(cadr(e), env));
    }
    else if (eq(car(e), sym("atomp"))) {
      r = atomp(eval(cadr(e), env));
    }
    else if (eq(car(e), sym("eq"))) {
      push(eval(cadr(e), env));
      push(eval(caddr(e), env));
      r = eq(pop(), pop());
    }
    else if (eq(car(e), sym("car"))) {
      r = car(eval(cadr(e), env));
    }
    else if (eq(car(e), sym("cdr"))) {
      r = cdr(eval(cadr(e), env));
    }
    else if (eq(car(e), sym("cons"))) {
      push(eval(cadr(e), env));
      push(eval(caddr(e), env));
      r = cons(pop(), pop());
    }
    else if (eq(car(e), sym("cond"))) {
      r = cond_eval(cdr(e), env);
    }
    else if (eq(car(e), sym("defun"))) {
      push(cons(sym("lambda"), cdr(cdr(e))));
      def_add(cadr(e), pop()); 
      r = nil;
    }
    else if (eq(car(e), sym("lambda"))) {
      r = e;
    }
    else if (eq(car(e), sym("let"))) {
      push(eval(cadr(cadr(e)), env));
      push(append(cons(cons(caadr(e),cons(pop(), nil)), nil), env));
      r = eval(caddr(e), pop());
    }
    else if (eq(car(e), sym("debug"))) {
      if (null(cadr(e)) || eq(cadr(e), sym("debug"))) {
        set_level(DEBUG);
      } 
      else if (eq(cadr(e), sym("trace"))) {
        set_level(TRACE);
      }
      else if (eq(cadr(e), sym("info"))) {
        set_level(INFO);
      }
      else if (eq(cadr(e), sym("warn"))) {
        set_level(WARN);
      }
      else if (eq(cadr(e), sym("error"))) {
        set_level(ERROR);
      }
      else {
        error("Unknown debug level"); 
      }
      _trace = !_trace; // Toggle trace state
      if (_trace) puts("Start debugging.");
      else puts("Done debugging");
    }
    else if (eq(car(e), sym("dump-mem"))) {
      dump_mem();
    }
    else if (eq(car(e), sym("dump-sym"))) {
      dump_sym();
    }
    else if (eq(car(e), sym("bye"))) {
      r = car(e);
    } 
    else if (eq(car(e), sym("+"))) {
      _ptr a, b;
      a = eval(cadr(e), env);
      b = eval(caddr(e), env);
      if (numberp(a) && numberp(b)) {
        r = num(A(a) + A(b));
      }
      else {
        error("'+ is not defined for non-numbers");
      }
    }
    else if (eq(car(e), sym("-"))) {
      _ptr a, b;
      a = eval(cadr(e), env);
      b = eval(caddr(e), env);
      if (numberp(a) && numberp(b)) {
        r = num(A(a) - A(b));
      }
      else {
        error("'- is not defined for non-numbers");
      }
    }
    else if (eq(car(e), sym("*"))) {
      _ptr a, b;
      a = eval(cadr(e), env);
      b = eval(caddr(e), env);
      if (numberp(a) && numberp(b)) {
        r = num(A(a) * A(b));
      }
      else {
        error("'* is not defined for non-numbers");
      }
    }
    else if (eq(car(e), sym("/"))) {
      _ptr a, b;
      a = eval(cadr(e), env);
      b = eval(caddr(e), env);
      if (numberp(a) && numberp(b)) {
        r = num(A(a) / A(b));
      }
      else {
        error("'/ is not defined for non-numbers");
      }
    }
    else {
      // User defined function, with DEFUN
      r = assoc(car(e), env);
      if (null(r)) {
        // Try to find the function
        r = assoc(car(e), global);
      }
      if (null(r)) {
        error("**** Undefined function %s ****\n", GET_SYM(car(e)));
      }
      else {
        push(r);
        r = eval(cons(pop(), cdr(e)), env);
      }
    }
  }
  else if (eq(caar(e), sym("label"))) {
    // Label.
    r = eval(
          cons(caddar(e),
               cdr(e)), 
          cons(cons(cadar(e), 
               cons(car(e), nil)), env));
  }
  else if (eq(caar(e), sym("lambda"))) {
    // Evaluating a lambda function
    // e.g. ((lambda (x) (x)) 'a)
    push(list_eval(cdr(e), env));
    r = eval(caddar(e), 
	     append(pair(cadar(e), pop()), env));
  }
  else {
    printf("Unknown Symbol.\n");
    r = nil;
  }

  // Discard local params
  pop();

  if (_trace) {
    printf("---> ");
    println(r);
  }

  return r;
}

_ptr eq(_ptr a, _ptr b)
{
  return (atomp(a) && atomp(b) && A(a)==A(b)) ? t : nil;
}

__inline__ _ptr not(_ptr a)
{
  return null(a);
}

_ptr and(_ptr a, _ptr b)
{
  return (!null(a) && !null(b)) ? t : nil;
}

_ptr listp(_ptr x)
{
  switch(TYPE(x)){
  case NIL:
    return t;
  case CONS:
    return listp(cdr(x));
  default:
    return nil;
  }
}

int eval_from_file(FILE *fin) {
  _ptr x;
  char str[256];
  if (fgets(str, 255, fin)) {
    curr = s_read(str);
    // println(curr);
    x = eval(curr, nil);
    if (eq(x, sym("bye"))) return 0;
    println(x);
    return 1;
  }
  else {
    // EOF. Quit
    puts("");
    return 0;
  }
}

int main(int argc, char * argv[]) {
  _ptr x;
  char *init_file = NULL;
  int opt;

  int mem_size = N;
  int sym_size = Ntbl;
  int sym_len = SYM_LEN;
  int debug_lvl = INFO;

  while ((opt = getopt(argc, argv, "i:D:")) != -1) {
    switch (opt) {
    case 'i':
      // Initialization file
      init_file = optarg;
      break;
    case 'D':
      // Debug level 0 - error -> 5 - debug
      debug_lvl = atoi(optarg);
      break;
    default: /* '?' */
      fprintf(stderr, "Usage: %s [-i file]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  set_level(debug_lvl);

  debug("Parsed args");

  init(mem_size, sym_size, sym_len);

  debug("initialized memory");

  debug("Init file: %s", init_file);

  if (init_file != NULL) {
    FILE *fin = fopen(init_file, "rb");
    // eval on line at a time
    while (eval_from_file(fin));
    fclose(fin);
    debug("read init file");
  }

  setjmp(j_b);

  debug("done setjmp");

  do {
    printf("> ");
  } while (eval_from_file(stdin));

  puts("Exiting LISP. Bye.");
  exit(0);

  printevalln( s_read("'test"), nil );
  printevalln( s_read("'(a b ())"), nil );
  printevalln( s_read("(lambda f (x) (eq x 'test))"), nil );

  //a = list(2, sym("quote"), sym("a"));
  //push(a);
  //println(a);

  //b = list(2, sym("quote"), sym("b"));
  //push(b);
  //printevalln(b, nil);

  //  x = list(3 , sym("cons"), a, sym("nil"));
  //  x = list(3, sym("let"), list(2, sym("x"), a), sym("x"));
  //  x = list(3, sym("let"), list(2, sym("x"), a),
  //	   list(3, sym("cons"), sym("x"), sym("nil")));
  //  x = list(2, sym("quote"),
  //	   list(2, sym("a"), sym("b")));
  //  printevalln(x,nil);

  x = s_read("(let (f (lambda (x) (eq x 'a))) (f 'a))");

  printevalln(x, nil);

  x = s_read("(cons 'a 'b)");
  printevalln(x, nil);

  x = s_read("(defun f (x) (eq x 'a))");
  printevalln(x, nil);

  x = s_read("(f 'a)");
  printevalln(x, nil);

  /*
  printevalln(sym("a"),
	  cons(cons(sym("a"),
		    cons(sym("x"), nil)),
	       nil));

  x = cons(sym("eq"),
	   cons(cons(sym("quote"),
		     cons(sym("a"),nil)),
		cons(cons(sym("quote"),
			  cons(sym("a"), nil)),nil)));
  println(x);
  println(eval(x, nil));

  dump_sym();
  println(global);
  */

  return EXIT_SUCCESS;
}
