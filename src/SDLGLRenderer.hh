// $Id$

#ifndef __SDLGLRENDERER_HH__
#define __SDLGLRENDERER_HH__

// Only compile on systems that have OpenGL headers.
#include "config.h"
#if (defined(HAVE_GL_GL_H) || defined(HAVE_GL_H))
#define __SDLGLRENDERER_AVAILABLE__

#include <SDL/SDL.h>
#include "openmsx.hh"
#include "Renderer.hh"
#include "CharacterConverter.hh"
#include "BitmapConverter.hh"
#include "VDP.hh"

#ifdef HAVE_GL_GL_H
#include <GL/gl.h>
#else // HAVE_GL_H
#include <gl.h>
#endif

class SpriteChecker;
class VDPVRAM;

/** Factory method to create SDLGLRenderer objects.
  * TODO: Add NTSC/PAL selection
  *   (immutable because it is colour encoding, not refresh frequency).
  */
Renderer *createSDLGLRenderer(VDP *vdp, bool fullScreen, const EmuTime &time);

/** Hi-res (640x480) renderer on SDL.
  */
class SDLGLRenderer : public Renderer
{
public:
	// TODO: Make private, if it remains at all.
	typedef GLuint Pixel;

	/** Constructor.
	  * It is suggested to use the createSDLGLRenderer factory
	  * function instead, which automatically selects a colour depth.
	  */
	SDLGLRenderer(VDP *vdp, SDL_Surface *screen, bool fullScreen, const EmuTime &time);

	/** Destructor.
	  */
	virtual ~SDLGLRenderer();

	// Renderer interface:

	void frameStart(const EmuTime &time);
	void putImage(const EmuTime &time);
	void setFullScreen(bool);
	void updateTransparency(bool enabled, const EmuTime &time);
	void updateForegroundColour(int colour, const EmuTime &time);
	void updateBackgroundColour(int colour, const EmuTime &time);
	void updateBlinkForegroundColour(int colour, const EmuTime &time);
	void updateBlinkBackgroundColour(int colour, const EmuTime &time);
	void updateBlinkState(bool enabled, const EmuTime &time);
	void updatePalette(int index, int grb, const EmuTime &time);
	void updateVerticalScroll(int scroll, const EmuTime &time);
	void updateHorizontalAdjust(int adjust, const EmuTime &time);
	void updateDisplayEnabled(bool enabled, const EmuTime &time);
	void updateDisplayMode(int mode, const EmuTime &time);
	void updateNameBase(int addr, const EmuTime &time);
	void updatePatternBase(int addr, const EmuTime &time);
	void updateColourBase(int addr, const EmuTime &time);
	void updateVRAM(int addr, byte data, const EmuTime &time);

private:
	typedef void (SDLGLRenderer::*RenderMethod)
		(Pixel *pixelPtr, int line, int x, int y);
	typedef void (SDLGLRenderer::*PhaseHandler)(int limit);
	typedef void (SDLGLRenderer::*DirtyChecker)
		(int addr, byte data, const EmuTime &time);

	inline void sync(const EmuTime &time);
	inline void renderUntil(int limit);

	inline void renderBitmapLines(byte line, const EmuTime &until);
	inline void renderPlanarBitmapLines(byte line, const EmuTime &until);
	inline void renderCharacterLines(byte line, const EmuTime &until);

	/** Get width of the left border in pixels.
	  * This is equal to the X coordinate of the display area.
	  */
	inline int getLeftBorder();

	/** Get width of the display area in pixels.
	  */
	inline int getDisplayWidth();

	/** Get a pointer to the start of a VRAM line in the cache.
	  * @param displayCache The display cache to use.
	  * @param line The VRAM line, range depends on display cache.
	  */
	inline Pixel *getLinePtr(Pixel *displayCache, int line);

	/** Get the pixel colour of a graphics 7 colour index.
	  */
	inline Pixel graphic7Colour(byte index);

	/** Get the pixel colour of the border.
	  * SCREEN6 has separate even/odd pixels in the border.
	  * TODO: Implement the case that even_colour != odd_colour.
	  */
	inline Pixel getBorderColour();

	void renderText1(Pixel *pixelPtr, int line, int x, int y);
	void renderText1Q(Pixel *pixelPtr, int line, int x, int y);
	void renderText2(Pixel *pixelPtr, int line, int x, int y);
	void renderGraphic1(Pixel *pixelPtr, int line, int x, int y);
	void renderGraphic2(Pixel *pixelPtr, int line, int x, int y);
	void renderGraphic4(Pixel *pixelPtr, int line, int x, int y);
	void renderGraphic5(Pixel *pixelPtr, int line, int x, int y);
	void renderGraphic6(Pixel *pixelPtr, int line, int x, int y);
	void renderGraphic7(Pixel *pixelPtr, int line, int x, int y);
	void renderMulti(Pixel *pixelPtr, int line, int x, int y);
	void renderMultiQ(Pixel *pixelPtr, int line, int x, int y);
	void renderBogus(Pixel *pixelPtr, int line, int x, int y);

	/** Render in background colour.
	  * Used for borders and during blanking.
	  * @param limit Render lines [currentLine..limit).
	  */
	void blankPhase(int limit);

	/** Render pixels according to VRAM.
	  * Used for the display part of scanning.
	  * @param limit Render lines [currentLine..limit).
	  */
	void displayPhase(int limit);

	/** Dirty checking that does nothing (but is a valid method).
	  */
	void checkDirtyNull(int addr, byte data, const EmuTime &time);

	/** Dirty checking for MSX1 display modes.
	  */
	void checkDirtyMSX1(int addr, byte data, const EmuTime &time);

	/** Dirty checking for Text2 display mode.
	  */
	void checkDirtyText2(int addr, byte data, const EmuTime &time);

	/** Dirty checking for bitmap modes.
	  */
	void checkDirtyBitmap(int addr, byte data, const EmuTime &time);

	/** Draw sprites on this line over the background.
	  */
	void drawSprites(int absLine);

	/** Set all dirty / clean.
	  */
	void setDirty(bool);

	/** RenderMethods for each screen mode.
	  */
	static RenderMethod modeToRenderMethod[];

	/** DirtyCheckers for each screen mode.
	  */
	static DirtyChecker modeToDirtyChecker[];

	/** The VDP of which the video output is being rendered.
	  */
	VDP *vdp;

	/** The VRAM whose contents are used for rendering.
	  */
	VDPVRAM *vram;

	/** The sprite checker whose sprites are rendered.
	  */
	SpriteChecker *spriteChecker;

	/** Current time: the moment up until when the rendering is emulated.
	  */
	EmuTimeFreq<VDP::TICKS_PER_SECOND> currentTime;

	/** RGB colours corresponding to each VDP palette entry.
	  * palFg has entry 0 set to the current background colour,
	  * palBg has entry 0 set to black.
	  */
	Pixel palFg[16], palBg[16];

	/** RGB colours corresponding to each Graphic 7 sprite colour.
	  */
	Pixel palGraphic7Sprites[16];

	/** RGB colours of current sprite palette.
	  * Points to either palBg or palGraphic7Sprites.
	  */
	Pixel *palSprites;

	/** RGB colours corresponding to each possible V9938 colour.
	  * Used by updatePalette to adjust palFg and palBg.
	  */
	Pixel V9938_COLOURS[8][8][8];

	/** RGB colours corresponding to the 256 colour palette of Graphic7.
	  * Used by BitmapConverter.
	  */
	Pixel PALETTE256[256];

	/** Rendering method for the current display mode.
	  */
	RenderMethod renderMethod;

	/** Phase handler: current drawing mode (off, blank, display).
	  */
	PhaseHandler phaseHandler;

	/** Dirty checker: update dirty tables on VRAM write.
	  */
	DirtyChecker dirtyChecker;

	/** Number of the next line to render.
	  * Absolute line number: [0..262) for NTSC, [0..313) for PAL.
	  * Any number larger than the number of lines means
	  * "no more lines to render for this frame".
	  */
	int nextLine;

	/** The surface which is visible to the user.
	  */
	SDL_Surface *screen;

	/** Cache for rendered VRAM in character modes.
	  * Cache line (N + scroll) corresponds to display line N.
	  * It holds a single page of 256 lines.
	  */
	Pixel *charDisplayCache;

	/** Cache for rendered VRAM in bitmap modes.
	  * Cache line N corresponds to VRAM at N * 128.
	  * It holds up to 4 pages of 256 lines each.
	  * In Graphics6/7 the lower two pages are used.
	  */
	Pixel *bitmapDisplayCache;

	/** Display mode the line is valid in.
	  * 0xFF means invalid in every mode.
	  */
	byte lineValidInMode[256 * 4];

	/** Absolute line number of first bottom erase line.
	  */
	int lineBottomErase;

	/** Line to render at top of display.
	  * After all, our screen is 240 lines while display is 262 or 313.
	  */
	int lineRenderTop;

	/** Dirty tables indicate which character blocks must be repainted.
	  * The anyDirty variables are true when there is at least one
	  * element in the dirty table that is true.
	  */
	bool anyDirtyColour, dirtyColour[1 << 10];
	bool anyDirtyPattern, dirtyPattern[1 << 10];
	bool anyDirtyName, dirtyName[1 << 12];
	// TODO: Introduce "allDirty" to speed up screen splits.

	/** Did foreground colour change since last screen update?
	  */
	bool dirtyForeground;

	/** Did background colour change since last screen update?
	  */
	bool dirtyBackground;

	/** VRAM to pixels converter for character display modes.
	  */
	CharacterConverter<Pixel> characterConverter;

	/** VRAM to pixels converter for bitmap display modes.
	  */
	BitmapConverter<Pixel> bitmapConverter;

};

#endif // OpenGL header check.
#endif // __SDLGLRENDERER_HH__

