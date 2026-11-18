/* Prototipos con puntero y argumentos variables */
int printf(char *fmt, ...);
int scanf(char *fmt, ...);
void saludar(void);
int g;

int main(void)
{
 int a, b;
 char c;
 a = -32768;
 b = 0xFFFF;
 c = 'x';
 while (a < 10) {
   if (a == 5)
     continue;
   if (a != 7)
     if (b > 0) a = a + 1; else a = a + 2;
   else
     break;
   a = -(a + 1) * (3 - +2) / 4;
 }
 return a >= b;
}
