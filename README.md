# trivial lisp
## trivial LISP interpreter

I was reading [Roots of Lisp](http://www.paulgraham.com/rootsoflisp.html) and the EVAL code looked simple enough to write in C.
So I did. I rewrote the EVAL loop in lisp-y C and then I filled the gaps to get a simple but (mostly) functional LISP interpreter.

It has a simple mark-and-sweep GC, and the code is woefully inefficient. But it works, most of the time. 
I think there may be a memory corruption issue, but I haven't tracked it down yet.
Also, the eval function has grown to be a huge `if/else if/else` mess, so it should be refactored using a dispatch table. Maybe. Someday.

I have added a few extenstions to the original code, namely 
- 32 bit integers numbers (and NUMBERP)
- debug functions (DEBUG, DUMP-SYM and DUMP-MEM)
- DEFUN (instead of LABEL)
- LET (not working...)

The built-in LISP commands are:
- CAR
- CDR
- CONS
- COND
- ATOMP
- EQ
- DEFUN
- LAMBDA
- NUMBERP (extension)
- arith operators + - * /
- LET (extension, not working yet)
- LABEL (not sure what to do with this one)
- BYE

There is a init.lisp file with additional functions like APPEND, REVERSE, FIRST, SECOND. Use `-i` to initialise the interpreter

`./lisp -i init.lisp`

Use `-D` to set the log level, from 0, for errors only, to 4, for debug logging. Default is 1.

A sample session:

```
$ ./lisp -D1 -i init.lisp
> (reverse '(a b c))
(C B A)
> (append '(a b c) '(d))
(A B C D)
> (listp (cons 'a 'b))
NIL
> (listp '(a b c))
T
> (pair '(a b) '(x y))
((A X) (B Y))
> (list 'a 'b)
(A B)
> (append '(a b) '(c d))
(A B C D)
> (assoc 'a '((a x) (b y)))
X
> (map 'null '(a b nil))
(NIL NIL T)
> (bye)
Exiting LISP. Bye.
```
