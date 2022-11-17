
#include <stdio.h>
#include "hash.tmpl"

int findfirst(int *isit)
{
	return(1);
}

main(int argc, char *argv[])
{
	/*
	 *  Test the linked list class
	 */

	List<int> *ints = new List<int>;
	int *saved;

	ints->Add(3);
	*ints += 4;
	if ( ints->Search(findfirst) )
		printf("The first item added was %d\n", *ints->LastMatch());
	else
		fprintf(stderr, "I couldn't find the first item I added!\n");
	printf("4 %s in the list\n", *ints == 4 ? "is" : "should be (?!)");
	printf("There are %d items in the list (should be 2)\n", ints->Size());
  	saved = *ints += 5;
	printf("Added and removing %d from list\n", *saved);
	if ( *ints -= saved ) {
		printf(" -- now %d numbers big\n", ints->Size());
	} else
		fprintf(stderr, "I couldn't remove something I just added!\n");
	delete ints;

	/*
	 *  Test the hash table class
	 */
	int i;
	Hash<int> *hash = new Hash<int>(32);

	for ( i=0; i<10; ++i )
		hash->Add(i, i+20);
	for ( i=0; i<10; ++i ) {
		if ( *hash == i )
			printf("The item for key %d is -= %d =-\n", i,
							*hash->Search(i));
		else
			fprintf(stderr, "Couldn't find item for key %d!\n", i);
	}
	for ( i=0; i<10; ++i )
		*hash -= i;
	for ( i=0; i<10; ++i ) {
		if ( *hash == i )
			fprintf(stderr, "Found deleted item! (key = %d)\n", i);
	}
	/* Test hash table iteration */
	hash->InitIterator();
	if ( hash->Iterate() )
		fprintf(stderr, "Iterate succeeded on empty hash table!\n");
	for ( i=0; i<100; ++i )
		hash->Add(i, i*i);
	hash->InitIterator();
	for ( i=0; i<100; ++i ) {
		int key;
		saved = hash->Iterate(&key);
		if ( saved ) {
			printf("Hash saved entry %d with value %d\n",
								key, *saved);
		} else
			fprintf(stderr, "Missing a hash table entry!\n");
	}
	if ( hash->Iterate() )
		fprintf(stderr, "Iterate succeeded on empty hash table!\n");
	delete hash;

	printf("Tests complete\n");
}
