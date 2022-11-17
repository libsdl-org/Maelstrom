
// Functions from controls.cc
#ifdef USE_JOYSTICK
extern void	CalibrateJoystick(char *joystick);
#endif
extern void	LoadControls(void);
extern void	SaveControls(void);
extern void	ConfigureControls(void);
extern int	PollEvent(SDL_Event *event, int timeout);
extern void	HandleEvents(int timeout);
extern int	DropEvents(void);
extern void	ShowDawn(void);

/* Generic key control definitions */
#define THRUST_KEY	0x01
#define RIGHT_KEY	0x02
#define LEFT_KEY	0x03
#define SHIELD_KEY	0x04
#define FIRE_KEY	0x05
#define PAUSE_KEY	0x06
#define ABORT_KEY	0x07

/* The controls structure */
typedef struct {
	SDL_Keycode gPauseControl;
	SDL_Keycode gShieldControl;
	SDL_Keycode gThrustControl;
	SDL_Keycode gTurnRControl;
	SDL_Keycode gTurnLControl;
	SDL_Keycode gFireControl;
	SDL_Keycode gQuitControl;
} Controls;

extern SDL_Keymod gToggleFullscreenMod;
