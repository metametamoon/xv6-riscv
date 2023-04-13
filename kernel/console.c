//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#include "pr_msg.h"

#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x

//
// send one character to the uart.
// called by printf(), and to echo input characters,
// but not from write().
//
void
consputc(int c)
{
  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    uartputc_sync('\b'); uartputc_sync(' '); uartputc_sync('\b');
  } else {
    uartputc_sync(c);
  }
}

struct {
  struct spinlock lock;
  
  // input
#define INPUT_BUF_SIZE 128
  char buf[INPUT_BUF_SIZE];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} cons;

//
// user write()s to the console go here.
//
int
consolewrite(int user_src, uint64 src, int n)
{
  int i;

  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    uartputc(c);
  }

  return i;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
int
consoleread(int user_dst, uint64 dst, int n)
{
  uint target;
  int c;
  char cbuf;

  target = n;
  acquire(&cons.lock);
  while(n > 0){
    // wait until interrupt handler has put some
    // input into cons.buffer.
    while(cons.r == cons.w){
      if(killed(myproc())){
        release(&cons.lock);
        return -1;
      }
      sleep(&cons.r, &cons.lock);
    }

    c = cons.buf[cons.r++ % INPUT_BUF_SIZE];

    if(c == C('D')){  // end-of-file
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons.r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&cons.lock);

  return target - n;
}

//
// the console input interrupt handler.
// uartintr() calls this for input character.
// do erase/kill processing, append to cons.buf,
// wake up consoleread() if a whole line has arrived.
//
void
consoleintr(int c)
{
  acquire(&cons.lock);

  switch(c){
  case C('P'):  // Print process list.
    procdump();
    break;
  case C('U'):  // Kill line.
    while(cons.e != cons.w &&
          cons.buf[(cons.e-1) % INPUT_BUF_SIZE] != '\n'){
      cons.e--;
      consputc(BACKSPACE);
    }
    break;
  case C('H'): // Backspace
  case '\x7f': // Delete key
    if(cons.e != cons.w){
      cons.e--;
      consputc(BACKSPACE);
    }
    break;
  default:
    if(c != 0 && cons.e-cons.r < INPUT_BUF_SIZE){
      c = (c == '\r') ? '\n' : c;

      // echo back to the user.
      consputc(c);

      // store for consumption by consoleread().
      cons.buf[cons.e++ % INPUT_BUF_SIZE] = c;

      if(c == '\n' || c == C('D') || cons.e-cons.r == INPUT_BUF_SIZE){
        // wake up consoleread() if a whole line (or end-of-file)
        // has arrived.
        cons.w = cons.e;
        wakeup(&cons.r);
      }
    }
    break;
  }
  
  release(&cons.lock);
}

void
consoleinit(void)
{
  initlock(&cons.lock, "cons");

  uartinit();

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}


void*
memmove(void *dst, const void *src, uint n);


struct {
    char buffer[MSG_BUFF_SIZE];
    struct spinlock lock;
    // int last_pos
    int head;
    int tail;
} msg_buffer;

void append_char(char ch) {
  msg_buffer.buffer[msg_buffer.tail] = ch;
  msg_buffer.tail = (msg_buffer.tail + 1) % MSG_BUFF_SIZE;
  if (msg_buffer.tail == msg_buffer.head) {
    msg_buffer.head = (msg_buffer.head + 1) % MSG_BUFF_SIZE;
    int itercount = 0;
    while (msg_buffer.buffer[msg_buffer.head] != '\n' && itercount <= MSG_BUFF_SIZE) {
      msg_buffer.head = (msg_buffer.head + 1) % MSG_BUFF_SIZE;
      itercount += 1;
    }
    if (itercount > MSG_BUFF_SIZE) {
      // no newlines... push head one position forward
      msg_buffer.head = (msg_buffer.head + 1) % MSG_BUFF_SIZE;
    } else  { // msg_buffer.head == '\n'.
      int new_pos = (msg_buffer.head + 1) % MSG_BUFF_SIZE;
      if (new_pos != msg_buffer.tail) {
        msg_buffer.head = new_pos;
      }
      // only one \n, right before the  tail; nothing needs to be done
    }
  }
}

void append_ticks() {
    append_char('[');
    uint xticks;
    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);
    uint digits_nums = 0;
    uint pow_of_10 = 1;
    while (pow_of_10 < xticks) {
        digits_nums += 1;
        pow_of_10 *= 10;
    }
    digits_nums -= 1;
    pow_of_10 /= 10;
    while (pow_of_10 > 0) {
        char next = '0' + xticks / pow_of_10;
        append_char(next);
        xticks = xticks % pow_of_10;
        pow_of_10 /= 10;
    }
    append_char(' ');
    append_char('t');
    append_char('k');
    append_char('s');
    append_char(']');
    append_char(' ');
}



static char digits[] = "0123456789abcdef";

static void
printintbuff(int xx, int base, int sign)
{
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    append_char(buf[i]);
}

static void
printptrbuff(uint64 x)
{
  int i;
  append_char('0');
  append_char('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    append_char(digits[x >> (sizeof(uint64) * 8 - 4)]);
}



// The message
void pr_msg(const char *fmt, ...) {
  va_list ap;
  int i, c;
  char *s;

  acquire(&msg_buffer.lock);
  append_ticks();

  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      append_char(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printintbuff(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printintbuff(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptrbuff(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        append_char(*s);
      break;
    case '%':
      append_char('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      append_char('%');
      append_char(c);
      break;
    }
  }
  va_end(ap);
  append_char('\n');

  release(&msg_buffer.lock);
}

void dmesg() {
  for (int i = msg_buffer.head; i != msg_buffer.tail; i = (i + 1) % MSG_BUFF_SIZE) {
      consputc(msg_buffer.buffer[i]);
  }
}