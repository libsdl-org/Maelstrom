package com.ambrosiasw.maelstrom;

import org.libsdl.app.SDLActivity; 
import android.os.*; 

/* 
 * A sample wrapper class that just calls SDLActivity 
 */ 

public class Maelstrom extends SDLActivity { 
    protected void onCreate(Bundle savedInstanceState) { 
	super.onCreate(savedInstanceState); 
    } 
    
    protected void onDestroy() { 
	super.onDestroy(); 
    } 
}
