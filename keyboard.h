
/* Feel free to remove any of these entries if the keysym is not
   defined in your header files..
*/
static struct keycode {
#ifdef DEBUG_KEYBOARD_H
	char  *symname;
#endif
	KeySym key;
	KeySym shift;
	char  *name;
	char  *ascii;
	char  *shiftascii;
	} keycodes[] = {
		{ 
#ifdef DEBUG_KEYBOARD_H
		 "XK_Escape",
#endif
		  XK_Escape,	XK_Escape,	"Escape",	"\033",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F1",
#endif
		  XK_F1,	XK_F11,		"F1",	"\033[11~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F2",
#endif
		  XK_F2,	XK_F12,		"F2",	"\033[12~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F3",
#endif
		  XK_F3,	XK_F13,		"F3",	"\033[13~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F4",
#endif
		  XK_F4,	XK_F14,		"F4",	"\033[14~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F5",
#endif
		  XK_F5,	XK_F15,		"F5",	"\033[15~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F6",
#endif
		  XK_F6,	XK_F16,		"F6",	"\033[17~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F7",
#endif
		  XK_F7,	XK_F17,		"F7",	"\033[18~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F8",
#endif
		  XK_F8,	XK_F18,		"F8",	"\033[19~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F9",
#endif
		  XK_F9,	XK_F19,		"F9",	"\033[20~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F10",
#endif
		  XK_F10,	XK_F20,		"F10",	"\033[21~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F11",
#endif
		  XK_F11,	XK_F11,		"F11",	"\033[23~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_F12",
#endif
		  XK_F12,	XK_F12,		"F12",	"\033[24~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_grave",
#endif
		  XK_grave,	XK_asciitilde,	"`",	"`",	"~" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_1",
#endif
		  XK_1,		XK_exclam,	"1",	"1",	"!" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_2",
#endif
		  XK_2,		XK_at,		"2",	"2",	"@" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_3",
#endif
		  XK_3,		XK_numbersign,	"3",	"3",	"#" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_4",
#endif
		  XK_4,		XK_dollar,	"4",	"4",	"$" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_5",
#endif
		  XK_5,		XK_percent,	"5",	"5",	"%" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_6",
#endif
		  XK_6,		XK_asciicircum,	"6",	"6",	"^" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_7",
#endif
		  XK_7,		XK_ampersand,	"7",	"7",	"&" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_8",
#endif
		  XK_8,		XK_asterisk,	"8",	"8",	"*" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_9",
#endif
		  XK_9,		XK_parenleft,	"9",	"9",	"(" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_0",
#endif
		  XK_0,		XK_parenright,	"0",	"0",	")" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_minus",
#endif
		  XK_minus,	XK_underscore,	"-",	"-",	"_" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_equal",
#endif
		  XK_equal,	XK_plus,	"=",	"=",	"+" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_backslash",
#endif
		  XK_backslash,	XK_bar,		"\\",	"\\",	"|" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_BackSpace",
#endif
		  XK_BackSpace,	XK_BackSpace,	"Backspace","\b",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Tab",
#endif
		  XK_Tab,	XK_Tab,		"Tab",	"\011",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_q",
#endif
		  XK_q,		XK_Q,		"Q",	"q",	"Q" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_w",
#endif
		  XK_w,		XK_W,		"W",	"w",	"W" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_e",
#endif
		  XK_e,		XK_E,		"E",	"e",	"E" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_r",
#endif
		  XK_r,		XK_R,		"R",	"r",	"R" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_t",
#endif
		  XK_t,		XK_T,		"T",	"t",	"T" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_y",
#endif
		  XK_y,		XK_Y,		"Y",	"y",	"Y" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_u",
#endif
		  XK_u,		XK_U,		"U",	"u",	"U" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_i",
#endif
		  XK_i,		XK_I,		"I",	"i",	"I" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_o",
#endif
		  XK_o,		XK_O,		"O",	"o",	"O" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_p",
#endif
		  XK_p,		XK_P,		"P",	"p",	"P" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_bracketleft",
#endif
		  XK_bracketleft, XK_braceleft,	"[",	"[",	"{" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_bracketright",
#endif
		  XK_bracketright, XK_braceright,"]",	"]",	"}" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Return",
#endif
		  XK_Return,	XK_Return,	"Return","\r",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_a",
#endif
		  XK_a,		XK_A,		"A",	"a",	"A" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_s",
#endif
		  XK_s,		XK_S,		"S",	"s",	"S" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_d",
#endif
		  XK_d,		XK_D,		"D",	"d",	"D" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_f",
#endif
		  XK_f,		XK_F,		"F",	"f",	"F" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_g",
#endif
		  XK_g,		XK_G,		"G",	"g",	"G" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_h",
#endif
		  XK_h,		XK_H,		"H",	"h",	"H" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_j",
#endif
		  XK_j,		XK_J,		"J",	"j",	"J" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_k",
#endif
		  XK_k,		XK_K,		"K",	"k",	"K" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_l",
#endif
		  XK_l,		XK_L,		"L",	"l",	"L" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_semicolon",
#endif
		  XK_semicolon, XK_colon,	";",	";",	":" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_apostrophe",
#endif
		  XK_apostrophe, XK_quotedbl,	"'",	"'",	"\"" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_z",
#endif
		  XK_z,		XK_Z,		"Z",	"z",	"Z" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_x",
#endif
		  XK_x,		XK_X,		"X",	"x",	"X" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_c",
#endif
		  XK_c,		XK_C,		"C",	"c",	"C" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_v",
#endif
		  XK_v,		XK_V,		"V",	"v",	"V" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_b",
#endif
		  XK_b,		XK_B,		"B",	"b",	"B" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_n",
#endif
		  XK_n,		XK_N,		"N",	"n",	"N" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_m",
#endif
		  XK_m,		XK_M,		"M",	"m",	"M" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_comma",
#endif
		  XK_comma,	XK_less,	",",	",",	"<" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_period",
#endif
		  XK_period,	XK_greater,	".",	".",	">" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_slash",
#endif
		  XK_slash,	XK_question,	"/",	"/",	"?" },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_space",
#endif
		  XK_space,	XK_space,	"Space"," ",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Pause",
#endif
		  XK_Pause,	XK_Pause,	"Pause","",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Insert",
#endif
		  XK_Insert,	XK_Insert,	"Insert",	"\033[2~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Home",
#endif
		  XK_Home,	XK_Home,	"Home",		"\033[H",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Prior",
#endif
		  XK_Prior,	XK_Prior,	"Page Up",	"\033[5~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Delete",
#endif
		  XK_Delete,	XK_Delete,	"Delete",	"\0177",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_End",
#endif
		  XK_End,	XK_End,		"End",		"\033Ow",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Next",
#endif
		  XK_Next,	XK_Next,	"Page Down",	"\033[6~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Up",
#endif
		  XK_Up,	XK_Up,		"Up Arrow",	"\033[A",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Down",
#endif
		  XK_Down,	XK_Down,	"Down Arrow",	"\033[B",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Left",
#endif
		  XK_Left,	XK_Left,	"Left Arrow",	"\033[D",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Right",
#endif
		  XK_Right,	XK_Right,	"Right Arrow",	"\033[C",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Num_Lock",
#endif
		  XK_Num_Lock,	XK_Num_Lock,	"Num Lock",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_Divide",
#endif
		  XK_KP_Divide,	XK_KP_Divide,	"[/]",	"/",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_Multiply",
#endif
		  XK_KP_Multiply, XK_KP_Multiply,"[*]",	"*",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_Subtract",
#endif
		  XK_KP_Subtract, XK_KP_Subtract,"[-]",	"\033Om",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_7",
#endif
		  XK_KP_7,	XK_KP_7,	"[7]",	"\033[H",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_8",
#endif
		  XK_KP_8,	XK_KP_8,	"[8]",	"\033OA",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_9",
#endif
		  XK_KP_9,	XK_KP_9,	"[9]",	"\033[5~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_Add",
#endif
		  XK_KP_Add,	XK_KP_Add,	"[+]",	"+",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_4",
#endif
		  XK_KP_4,	XK_KP_4,	"[4]",	"\033OD",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_5",
#endif
		  XK_KP_5,	XK_KP_5,	"[5]",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_6",
#endif
		  XK_KP_6,	XK_KP_6,	"[6]",	"\033OC",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_1",
#endif
		  XK_KP_1,	XK_KP_1,	"[1]",	"\033Ow",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_2",
#endif
		  XK_KP_2,	XK_KP_2,	"[2]",	"\033OB",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_3",
#endif
		  XK_KP_3,	XK_KP_3,	"[3]",	"\033[6~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_Enter",
#endif
		  XK_KP_Enter,	XK_KP_Enter,	"Enter",	"\r",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_0",
#endif
		  XK_KP_0,	XK_KP_0,	"[0]",	"\033[2~",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_KP_Decimal",
#endif
		  XK_KP_Decimal, XK_KP_Decimal,"[.]",	"\0177",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Shift_L",
#endif
		  XK_Shift_L,	XK_Shift_L,	"Left Shift",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Shift_R",
#endif
		  XK_Shift_R,	XK_Shift_R,	"Right Shift",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Control_L",
#endif
		  XK_Control_L,	XK_Control_L,	"Left Control",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Control_R",
#endif
		  XK_Control_R,	XK_Control_R,	"Right Control","",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Caps_Lock",
#endif
		  XK_Caps_Lock,	XK_Caps_Lock,	"Caps Lock",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Alt_L",
#endif
		  XK_Alt_L,	XK_Alt_L,	"Left Alt",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Alt_R",
#endif
		  XK_Alt_R,	XK_Alt_R,	"Right Alt",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Meta_L",
#endif
		  XK_Meta_L,	XK_Meta_L,	"Left Meta",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		 "XK_Meta_R",
#endif
		  XK_Meta_R,	XK_Meta_R,	"Right Meta",	"",	NULL },
		{
#ifdef DEBUG_KEYBOARD_H
		  NULL,
#endif
		  0,		0,		NULL,	"",	NULL }
	};

