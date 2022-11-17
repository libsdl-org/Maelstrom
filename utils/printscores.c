
#include <stdio.h>

typedef	struct {
	char	name[20];
	int	wave;
	long	score;	
	} Scores;

main(int argc, char *argv[])
{
	Scores scores[10];
	FILE *scores_fp;
	int i;

	if ( argc != 2 ) {
		fprintf(stderr, "Usage: %s scores\n", argv[0]);
		exit(1);
	}

	/* Read first scores file */
	if ( (scores_fp=fopen(argv[1], "r")) == NULL ) {
		perror(argv[1]);
		exit(3);
	}
	(void) fread(&scores[0], sizeof(Scores), 10, scores_fp);
	fclose(scores_fp);

	printf("Name			Score	Wave\n");
	for ( i=0; i<10; ++i ) {
		printf("%-20s	%-3.1ld	%d\n", scores[i].name, 
					scores[i].score, scores[i].wave);
	}
}

