
#include <stdio.h>

typedef	struct {
	char	name[20];
	int	wave;
	long	score;	
	} Scores;

int CompareScores(Scores *A, Scores *B)
{
	if ( A->score < B->score )
		return(1);
	else if ( A->score > B->score )
		return(-1);
	else
		return(0);
}
int EqualScores(Scores *A, Scores *B)
{
	if ( (strcmp(A->name, B->name) == 0) &&
			(A->score == B->score) && (A->wave == B->wave) ) {
		return(0);
	}
	return(1);
}

main(int argc, char *argv[])
{
	int i, written;
	Scores scores[20];
	FILE *scores_fp;

	if ( argc != 4 ) {
		fprintf(stderr, "Usage: %s scores1 scores2 outfile\n", argv[0]);
		exit(1);
	}

	/* Read first scores file */
	if ( (scores_fp=fopen(argv[1], "r")) == NULL ) {
		perror(argv[1]);
		exit(3);
	}
	(void) fread(&scores[0], sizeof(Scores), 10, scores_fp);
	fclose(scores_fp);

	/* Read second scores file */
	if ( (scores_fp=fopen(argv[2], "r")) == NULL ) {
		perror(argv[2]);
		exit(3);
	}
	(void) fread(&scores[10], sizeof(Scores), 10, scores_fp);
	fclose(scores_fp);

	/* Sort and merge them */
	qsort(scores, 20, sizeof(Scores), CompareScores);

	/* Write out the top ten scores */
	if ( (scores_fp=fopen(argv[3], "w")) == NULL ) {
		perror(argv[3]);
		exit(3);
	}
	for ( i=0, written=0; (i<20)&&(written<10); ++i ) {
		/* Skip duplicate scores */
		if ( i && (EqualScores(&scores[i], &scores[i-1]) == 0) )
			continue;
		(void) fwrite(&scores[i], sizeof(Scores), 1, scores_fp);
		++written;
	}
	while ( written < 10 ) {
		(void) fwrite(&scores[19], sizeof(Scores), 1, scores_fp);
		++written;
	}
	fclose(scores_fp);

	printf("Scores merged.\n");
}

