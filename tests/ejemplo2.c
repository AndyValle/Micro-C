/*
* Programa de prueba #2 (sin el & de scanf)
*/
int count( int n );
int
main( void )
{
 int i, sum;
 i = scanf( "%d" ); /* lea i */
 sum = count(i); /* llame a count */
 printf( "%d\n", sum ); /* imprima resultados */
}

int
count( int n )
{
 int i, sum;
 i = 1;
 sum = 0;
 while ( i <= n ) {
 sum = sum + i;
 i = i + 1;
 }
 return sum;
}
