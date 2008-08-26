
// Functions from scores.cc
extern void	LoadScores(void);
extern void	SaveScores(void);
extern int	ZapHighScores(void);
extern int	GetStartLevel(void);
extern void	PrintHighScores(void);

/* The high scores structure */
typedef	struct {
	char name[20];
	Uint32 wave;
	Uint32 score;	
} Scores;

