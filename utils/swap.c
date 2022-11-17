
/* Just an example of swapping two numbers without a temp variable

    It's really just for swapping two registers without accessing
main memory for a third memory storage location.  It's not meant to
be more than a speed trick. :)
*/

main()
{
	register int A = 1, B = 2;

	printf("A = %d, B = %d\n", A, B);
	A = (A^B);
	B = (B^A);
	A = (A^B);
	printf("*swap*\n");
	printf("A = %d, B = %d\n", A, B);
}
