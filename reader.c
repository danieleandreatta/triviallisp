#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <ctype.h>

#include "lisp.h"
#include "logging.h"

extern _ptr nil;
extern jmp_buf j_b;

static char *txt, *p;

static char token[20];

char *skip_white(char *x)
{
  for (; *x && isspace((int)*x); ++x);
  return x;
}

char *next_white(char *x)
{
  for (; *x && !isspace((int)*x); ++x);
  return x;
}

int is_lisp_special(int x)
{
  return x == '\0' || strchr(" ()\'\n", x) != NULL;
}

int is_number(char *tok) {
  int i, len;
  int ans = 1;

  len = strlen(tok);
  for (i=0; i<len; ++i) {
    ans = ans && isdigit((int)tok[i]);
  }
  return ans;
}

void next_token(void)
{
  p = skip_white(p);
  switch(*p) {
  case '\0':
    *token = '\0';
    break;
  case '(' :
    strcpy(token, "(");
    ++p;
    break;
  case ')':
    strcpy(token, ")");
    ++p;
    break;
  case '\'':
    strcpy(token, "'");
    ++p;
    break;
  default:
    {
      int i;
      for(i=0; !is_lisp_special(p[i]); ++i) { 
        token[i] = p[i];
      }
      token[i]='\0';
      p = p+i;
    }
  }
  debug("Token -> %s\n", token);
}

_ptr s_read1(void);

_ptr s_read_list(void)
{
  if (!strcmp(token, ")")) {
    next_token();
    return nil;
  }
  else {
    push( s_read1() );
    return cons(pop(), s_read_list());
  }
}

_ptr s_read1(void)
{
  if (!strcmp(token, "")) {
    printf("Error. Unexpected end of input line.\n");
    longjmp(j_b, 1);
    return nil;
  }
  else if (!strcmp(token, ")")) {
    next_token();
    return nil;
  }
  else if (!strcmp(token, "(")) {
    next_token();
    return s_read_list(); 
  }
  else if (!strcmp(token, "'")) {
    next_token();
    return cons(sym("quote"), cons(s_read1(), nil));
  }
  else if (is_number(token)) {
    _ptr a;
    a = num(atoi(token));
    next_token();
    return a;
  }
  else {
    _ptr a;
    a = sym(token);
    next_token();
    return a;
  }
}

_ptr s_read(char *x)
{
  _ptr r = nil;

  if (strlen(x) > 0) {
    txt = strdup(x);
    p = txt;

    next_token();
    r = s_read1();

    free(txt);
  }
  return r;
}
