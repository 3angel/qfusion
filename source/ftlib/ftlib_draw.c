/*
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2015 Chasseur de bots

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "ftlib_local.h"

//===============================================================================
//STRINGS HELPERS
//===============================================================================

/*
* FTLIB_fontHeight
*/
size_t FTLIB_fontHeight( qfontface_t *font )
{
	if( !font ) {
		return 0;
	}
	return font->height;
}

/*
* FTLIB_strWidth
* doesn't count invisible characters. Counts up to given length, if any.
*/
size_t FTLIB_strWidth( const char *str, qfontface_t *font, size_t maxlen )
{
	const char *s = str, *olds;
	size_t width = 0;
	wchar_t num, prev_num = 0;
	qglyph_t *glyph;

	if( !str || !font )
		return 0;

	while( *s && *s != '\n' )
	{
		if( maxlen && (size_t)( s - str ) >= maxlen )  // stop counting at desired len
			return width;

		olds = s;

		switch( Q_GrabWCharFromColorString( &s, &num, NULL ) )
		{
		case GRABCHAR_CHAR:
			if( num < ' ' )
				break;

			glyph = FTLIB_GetGlyph( font, num );
			if( !glyph )
			{
				num = FTLIB_REPLACEMENT_GLYPH;
				glyph = FTLIB_GetGlyph( font, num );
			}

			if( !glyph->shader )
				font->f->renderString( font, olds );

			if( prev_num && font->hasKerning )
				width += font->f->getKerning( font, prev_num, num );

			width += glyph->x_advance;

			prev_num = num;
			break;

		case GRABCHAR_COLOR:
			break;

		case GRABCHAR_END:
			return width;

		default:
			assert( 0 );
		}
	}

	return width;
}

/*
* FTLIB_StrlenForWidth
* returns the len allowed for the string to fit inside a given width when using a given font.
*/
size_t FTLIB_StrlenForWidth( const char *str, qfontface_t *font, size_t maxwidth )
{
	const char *s, *olds;
	size_t width = 0;
	int gc;
	int advance = 0;
	wchar_t num, prev_num = 0;
	qglyph_t *glyph;

	if( !str || !font )
		return 0;

	s = str;

	while( s )
	{
		olds = s;
		gc = Q_GrabWCharFromColorString( &s, &num, NULL );
		if( gc == GRABCHAR_CHAR )
		{
			if( num == '\n' )
				break;

			if( num < ' ' )
				continue;

			glyph = FTLIB_GetGlyph( font, num );
			if( !glyph )
			{
				num = FTLIB_REPLACEMENT_GLYPH;
				glyph = FTLIB_GetGlyph( font, num );
			}

			if( !glyph->shader )
				font->f->renderString( font, olds );

			advance = glyph->x_advance;
			if( prev_num && font->hasKerning ) {
				advance += font->f->getKerning( font, prev_num, num );
			}

			if( maxwidth && ( ( width + advance ) > maxwidth ) )
			{
				s = olds;
				break;
			}

			width += advance;

			prev_num = num;
		}
		else if( gc == GRABCHAR_COLOR )
			continue;
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}

	return (unsigned int)( s - str );
}

//===============================================================================
//STRINGS DRAWING
//===============================================================================

/*
* FTLIB_DrawRawChar
* 
* Draws one graphics character with 0 being transparent.
* It can be clipped to the top of the screen to allow the console to be
* smoothly scrolled off.
*/
void FTLIB_DrawRawChar( int x, int y, wchar_t num, qfontface_t *font, vec4_t color )
{
	qglyph_t *glyph;

	if( ( num <= ' ' ) || !font )
		return;

	glyph = FTLIB_GetGlyph( font, num );
	if( !glyph )
	{
		num = FTLIB_REPLACEMENT_GLYPH;
		glyph = FTLIB_GetGlyph( font, num );
	}

	if( y <= -font->height )
		return; // totally off screen

	if( !glyph->shader )
		font->f->renderString( font, Q_WCharToUtf8Char( num ) );

	trap_R_DrawStretchPic( x + glyph->x_offset, y + font->glyphYOffset + glyph->y_offset, 
		glyph->width, glyph->height,
		glyph->s1, glyph->t1, glyph->s2, glyph->t2,
		color, glyph->shader );
}

/*
* FTLIB_DrawClampChar
* 
* Draws one graphics character with 0 being transparent.
* Clipped to [xmin, ymin; xmax, ymax].
*/
void FTLIB_DrawClampChar( int x, int y, wchar_t num, int xmin, int ymin, int xmax, int ymax, qfontface_t *font, vec4_t color )
{
	qglyph_t *glyph;
	int x2, y2;
	float s1 = 0.0f, t1 = 0.0f, s2 = 1.0f, t2 = 1.0f;
	float tw, th;

	if( ( num <= ' ' ) || !font || ( xmax <= xmin ) || ( ymax <= ymin ) )
		return;

	glyph = FTLIB_GetGlyph( font, num );
	if( !glyph )
	{
		num = FTLIB_REPLACEMENT_GLYPH;
		glyph = FTLIB_GetGlyph( font, num );
	}

	if( !glyph->shader )
		font->f->renderString( font, Q_WCharToUtf8Char( num ) );

	if( !glyph->width || !glyph->height )
		return;

	x += glyph->x_offset;
	y += font->glyphYOffset + glyph->y_offset;
	x2 = x + glyph->width;
	y2 = y + glyph->height;
	if( ( x > xmax ) || ( y > ymax ) || ( x2 <= xmin ) || ( y2 <= ymin ) )
		return;

	++xmax;
	++ymax;

	if( x < xmin )
	{
		s1 = ( xmin - x ) / ( float )glyph->width;
		x = xmin;
	}
	if( y < ymin )
	{
		t1 = ( ymin - y ) / ( float )glyph->height;
		y = ymin;
	}
	if( x2 > xmax )
	{
		s2 = 1.0f - ( x2 - xmax ) / ( float )glyph->width;
		x2 = xmax;
	}
	if( y2 > ymax )
	{
		t2 = 1.0f - ( y2 - ymax ) / ( float )glyph->height;
		y2 = ymax;
	}

	tw = glyph->s2 - glyph->s1;
	th = glyph->t2 - glyph->t1;

	trap_R_DrawStretchPic( x, y, x2 - x, y2 - y,
		glyph->s1 + tw * s1, glyph->t1 + th * t1,
		glyph->s1 + tw * s2, glyph->t1 + th * t2,
		color, glyph->shader );
}

/*
* FTLIB_DrawClampString
*/
void FTLIB_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, qfontface_t *font, vec4_t color )
{
	int xoffset = 0;
	vec4_t scolor;
	int colorindex;
	wchar_t num, prev_num = 0;
	const char *s = str, *olds;
	int gc;
	qglyph_t *glyph, *prev_glyph = NULL;

	if( !str || !font || ( xmax <= xmin ) || ( ymax <= ymin ) || ( x > xmax ) || ( y > ymax ) )
		return;

	Vector4Copy( color, scolor );

	while( 1 )
	{
		olds = s;
		gc = Q_GrabWCharFromColorString( &s, &num, &colorindex );
		if( gc == GRABCHAR_CHAR )
		{
			if( num == '\n' )
				break;

			if( num < ' ' )
				continue;

			glyph = FTLIB_GetGlyph( font, num );
			if( !glyph )
			{
				num = FTLIB_REPLACEMENT_GLYPH;
				glyph = FTLIB_GetGlyph( font, num );
			}

			if( !glyph->shader )
				font->f->renderString( font, olds );

			if( prev_num )
			{
				xoffset += prev_glyph->x_advance;
				if( font->hasKerning )
					xoffset += font->f->getKerning( font, prev_num, num );
			}

			if( x + xoffset > xmax )
				break;

			FTLIB_DrawClampChar( x + xoffset, y, num, xmin, ymin, xmax, ymax, font, scolor );

			prev_num = num;
			prev_glyph = glyph;
		}
		else if( gc == GRABCHAR_COLOR )
		{
			assert( ( unsigned )colorindex < MAX_S_COLORS );
			VectorCopy( color_table[colorindex], scolor );
		}
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}
}

/*
* FTLIB_DrawRawString - Doesn't care about aligning. Returns drawn len.
* It can stop when reaching maximum width when a value has been parsed.
*/
size_t FTLIB_DrawRawString( int x, int y, const char *str, size_t maxwidth, qfontface_t *font, vec4_t color )
{
	unsigned int xoffset = 0;
	vec4_t scolor;
	const char *s, *olds;
	int gc, colorindex;
	wchar_t num, prev_num = 0;
	qglyph_t *glyph, *prev_glyph = NULL;

	if( !str || !font )
		return 0;

	Vector4Copy( color, scolor );

	s = str;

	while( s )
	{
		olds = s;
		gc = Q_GrabWCharFromColorString( &s, &num, &colorindex );
		if( gc == GRABCHAR_CHAR )
		{
			if( num == '\n' )
				break;

			if( num < ' ' )
				continue;

			glyph = FTLIB_GetGlyph( font, num );
			if( !glyph )
			{
				num = FTLIB_REPLACEMENT_GLYPH;
				glyph = FTLIB_GetGlyph( font, num );
			}

			if( !glyph->shader )
				font->f->renderString( font, olds );

			if( maxwidth && ( ( xoffset + glyph->x_advance ) > maxwidth ) )
			{
				s = olds;
				break;
			}

			if( prev_num )
			{
				xoffset += prev_glyph->x_advance;
				if( font->hasKerning )
					xoffset += font->f->getKerning( font, prev_num, num );
			}

			FTLIB_DrawRawChar( x + xoffset, y, num, font, scolor );

			prev_num = num;
			prev_glyph = glyph;
		}
		else if( gc == GRABCHAR_COLOR )
		{
			assert( ( unsigned )colorindex < MAX_S_COLORS );
			VectorCopy( color_table[colorindex], scolor );
		}
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}

	return ( s - str );
}

