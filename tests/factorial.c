/*
* Calculo recursivo del factorial
*/
int factorial( int n )
{
 if ( n <= 1 )
 return 1;
 else
 return n * factorial( n - 1 );
}

int main( void )
{
 int n, fact;
 char c;
 c = 'A';
 n = 0xFB;
 printf( "Ingrese un entero: " );
 n = scanf( "%d" ); /* lea i */
 fact = factorial( n ); /* llame a factorial */
 printf( "Factorial de %d ", n );
 printf( "es %d\n", fact );
}
