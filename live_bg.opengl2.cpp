//g++ -o live_bg.opengl2 live_bg.opengl2.cpp `pkg-config.pkg-config --libs gl glew imlib2 x11 IL ILU` -lgif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>
#include <gif_lib.h>

void log_pixel( int pixel )
{
	std::cout <<(int) ((0xff000000 & pixel) >> 24)<< " - ";
	std::cout <<(int) ((0x00ff0000 & pixel) >> 16)<< " - ";
	std::cout <<(int) ((0x0000ff00 & pixel) >> 8)<<  " - ";
	std::cout <<(int) (0x000000ff & pixel);
	std::cout << std::endl;
}
GifFileType *gif_open_file( const char *name , int &error )
{
	GifFileType *gif_file;

	gif_file=DGifOpenFileName( name , &error );

	DGifSlurp( gif_file );

	return gif_file;
};
SavedImage *gif_get_frame( GifFileType *gif_file , unsigned int frame_number )
{
	return ( gif_file->SavedImages + frame_number );
}
GifImageDesc gif_get_frame_desc( GifFileType *gif_file , unsigned int frame_number )
{
	return gif_get_frame( gif_file , frame_number )->ImageDesc;
}
int gif_get_frame_top( GifFileType *gif_file , unsigned int frame_number )
{
	return gif_get_frame_desc( gif_file , frame_number ).Top;
}
int gif_get_frame_left( GifFileType *gif_file , unsigned int frame_number )
{
	return gif_get_frame_desc( gif_file , frame_number ).Left;
}
int *gif_get_frame_data( GifFileType *gif_file , unsigned  int frame_number )
{
	SavedImage	*frame=gif_get_frame( gif_file , frame_number );
	
	GifImageDesc frame_desc=frame->ImageDesc;
	GifColorType *colors;

	if( frame_desc.ColorMap == NULL )
	{
		colors=gif_file->SColorMap->Colors;
	}
	else
	{
		colors=frame_desc.ColorMap->Colors;
	}

	GifByteType	*rasters=frame->RasterBits;

	GraphicsControlBlock gcb;

	DGifSavedExtensionToGCB( gif_file , frame_number , &gcb );

	int TransparentColor=gcb.TransparentColor;

	unsigned int width=frame_desc.Width;
	unsigned int height=frame_desc.Height;
	unsigned int len=width*height;

	int *imlib_data=new int[width*height];
	unsigned int row=0;
	unsigned int col=0;
	for ( unsigned int j=0 ; j < len ; j++ )
	{
		if( j%(width) == 0 )
		{
			col=0;
			++row;
		}

		unsigned int index=len-(row*width)+col;

		imlib_data[index]=
		( colors[ *rasters ].Red << 24 )	|
		( colors[ *rasters ].Green << 16 )	|
		( colors[ *rasters ].Blue << 8 );

		if( *rasters == TransparentColor )
		{
			
			imlib_data[index]&=0x00;
			//log_pixel( (int) imlib_data[index] );
		}
		else
		{
			imlib_data[index]|=0xff;
			//imlib_data[index+3]=0xff;
		}

		++col;
		++rasters;
	}

	return imlib_data;
};

// Function load a image, turn it into a texture, and return the texture ID as a GLuint for use
GLuint loadImage(const char* theFileName)
{
	ILuint imageID;				// Create an image ID as a ULuint

	GLuint textureID;			// Create a texture ID as a GLuint

	ILboolean success;			// Create a flag to keep track of success/failure

	ILenum error;				// Create a flag to keep track of the IL error state

	ilGenImages(1, &imageID); 		// Generate the image ID

	ilBindImage(imageID); 			// Bind the image

	success = ilLoadImage(theFileName); 	// Load the image file

	// If we managed to load the image, then we can start to do things with it...
	if (success)
	{
		// If the image is flipped (i.e. upside-down and mirrored, flip it the right way up!)
		ILinfo ImageInfo;
		iluGetImageInfo(&ImageInfo);
		if (ImageInfo.Origin == IL_ORIGIN_UPPER_LEFT)
		{
			iluFlipImage();
		}

		// Convert the image into a suitable format to work with
		// NOTE: If your image contains alpha channel you can replace IL_RGB with IL_RGBA
		success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);

		// Quit out if we failed the conversion
		if (!success)
		{
			error = ilGetError();
			std::cout << "Image conversion failed - IL reports error: " << error << " - " << iluErrorString(error) << std::endl;
			exit(-1);
		}

 	}
  	else // If we failed to open the image file in the first place...
  	{
		error = ilGetError();
		std::cout << "Image load failed - IL reports error: " << error << " - " << iluErrorString(error) << std::endl;
		exit(-1);
  	}

	//glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	// Generate a new texture
	glGenTextures(1, &imageID);

	// Bind the texture to a name
	glBindTexture(GL_TEXTURE_2D, imageID);

	// Set texture clamping method
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Set texture interpolation method to use linear interpolation (no MIPMAPS)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Specify the texture specification
	//glCompressedTexImage2D
	glTexImage2D(GL_TEXTURE_2D, 				// Type of texture
				 0,				// Pyramid level (for mip-mapping) - 0 is the top level
				 ilGetInteger(IL_IMAGE_FORMAT),	// Internal pixel format to use. Can be a generic type like GL_RGB or GL_RGBA, or a sized type
				 ilGetInteger(IL_IMAGE_WIDTH),	// Image width
				 ilGetInteger(IL_IMAGE_HEIGHT),	// Image height
				 0,				// Border width in pixels (can either be 1 or 0)
				 ilGetInteger(IL_IMAGE_FORMAT),	// Format of image pixel data
				 GL_UNSIGNED_BYTE,		// Image data type
				 ilGetData()
	);			// The actual image data itself

	std::cout << "Texture creation successful." << std::endl;

	return textureID; // Return the GLuint to the texture so you can use it!
}
void refresh_last_image( int width , int height , Display *disp , Pixmap pixmap )
{
	
}
int setRootAtoms ( Pixmap pixmap , Display *display , Window *window )
{
  Atom atom_root, atom_eroot, type;
  unsigned char *data_root, *data_eroot;
  int format;
  unsigned long length, after;

  atom_root = XInternAtom (display, "_XROOTMAP_ID", True);
  atom_eroot = XInternAtom (display, "ESETROOT_PMAP_ID", True);

  //XA_PIXMAP=20;
  // doing this to clean up after old background
  if (atom_root != None && atom_eroot != None)
    {
      XGetWindowProperty ( display, *window,
			  atom_root, 0L, 1L, False, AnyPropertyType,
			  &type, &format, &length, &after, &data_root);
      if (type == XA_PIXMAP)
	{
	  XGetWindowProperty ( display, *window,
			      atom_eroot, 0L, 1L, False, AnyPropertyType,
			      &type, &format, &length, &after, &data_eroot);
	  if (data_root && data_eroot && type == XA_PIXMAP &&
	      *((Pixmap *) data_root) == *((Pixmap *) data_eroot))
	    {
	      XKillClient (display, *((Pixmap *) data_root));
	    }
	}
    }

  atom_root = XInternAtom (display, "_XROOTPMAP_ID", False);
  atom_eroot = XInternAtom (display, "ESETROOT_PMAP_ID", False);

  if (atom_root == None || atom_eroot == None)
    return 0;

  // setting new background atoms
  XChangeProperty (display, *window,
		   atom_root, XA_PIXMAP, 32, PropModeReplace,
		   (unsigned char *) &pixmap, 1);
  XChangeProperty (display, *window, atom_eroot,
		   XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap,
		   1);

  return 1;
}
int main(int argc, char* argv[])
{

 	unsigned int			depth;
	unsigned int			screen;
	unsigned int			width	;
	unsigned int			height;

	Display *disp;
	Window	win;
	Visual  *vis;
	Colormap cm;

	/* connect to X */
	disp  = XOpenDisplay(NULL);
	/* get default visual , colormap etc. you could ask imlib2 for what it */
	/* thinks is the best, but this example is intended to be simple */
	screen 	= DefaultScreen			( disp					);
	win		= DefaultRootWindow ( disp 					);
	depth	= DefaultDepth			( disp , screen );
	vis		= DefaultVisual			( disp , screen );
	cm		= DefaultColormap		( disp , screen );
	width	= DisplayWidth			( disp , screen );
	height	= DisplayHeight			( disp , screen );

	XSetWindowAttributes attributes;
	attributes.colormap = cm;
	attributes.event_mask =  ExposureMask | KeyPressMask;

	XChangeWindowAttributes (
		disp,
		win,
		CWColormap | CWEventMask ,
		&attributes
	);

	GLint  att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24};

	GLXContext glc = glXCreateContext (
		disp ,
		glXChooseVisual(disp, 0, att) ,
		NULL ,
		GL_TRUE
	);

	Pixmap pixmap=XCreatePixmap( disp , win , width , height , depth );
	setRootAtoms( pixmap , disp , &win );

 	XLowerWindow( disp , win );
/*
  	ilutRenderer(ILUT_OPENGL);
	ilInit();
	iluInit();
	ilutInit();
	ilutRenderer(ILUT_OPENGL);
*/
	
	glXMakeCurrent ( disp , pixmap , glc );

	//GLuint textureHandle = loadImage("test.png");
	//ilDeleteImages(1, &textureHandle); // Because we have already copied image data into texture data we can release memory used by image.
	int error;
	GifFileType *file=gif_open_file( "test.gif" , error );
	int frames_len=file->ImageCount;

	GLuint textures[ frames_len ];

	int i;

	glEnable(GL_TEXTURE_2D);
//	glEnable(GL_DITHER);
	glEnable(GL_BLEND);
//	glBlendFunc( GL_SRC_COLOR , GL_DST_COLOR );
	glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for( i = (frames_len-1) ; i >= 0 ; i-- )
	{
		std::cout << "Texture: " << i << std::endl;
		// Generate a new texture
		glGenTextures( 1 , &textures[ i ] );

		// Bind the texture to a name
		glBindTexture( GL_TEXTURE_2D , textures[ i ] );

		// Set texture clamping method
		glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_WRAP_S , GL_CLAMP );
		glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_WRAP_T , GL_CLAMP );

		// Set texture interpolation method to use linear interpolation (no MIPMAPS)
		glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_MAG_FILTER , GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_MIN_FILTER , GL_LINEAR );

		GifImageDesc desc=gif_get_frame_desc( file , i );

		// Specify the texture specification
		//glCompressedTexImage2D
		glTexImage2D (
			GL_TEXTURE_2D, 				// Type of texture
			0,				// Pyramid level (for mip-mapping) - 0 is the top level
			GL_RGBA,	// Internal pixel format to use. Can be a generic type like GL_RGB or GL_RGBA, or a sized type
			desc.Width,	// Image width
			desc.Height,	// Image height
			0,				// Border width in pixels (can either be 1 or 0)
			GL_RGBA,	// Format of image pixel data
			GL_UNSIGNED_INT_8_8_8_8,		// Image data type
			gif_get_frame_data( file , i ) // The actual image data itself
		);
	}

	i=0;

	glDrawBuffer( (GLenum ) GL_BACK );
	//glReadBuffer( (GLenum ) GL_BACK );

	while( 1 )
	{
		//XSetCloseDownMode ( disp , RetainTemporary );

		std::cout << " Showing image: " << textures[i] << std::endl;

		//glCopyPixels( 0 , 0 , width , height , GL_COLOR );
		//glClear( GL_COLOR_BUFFER_BIT );

		glBindTexture( GL_TEXTURE_2D , textures[ i ] );

		glMatrixMode(GL_PROJECTION);

//		glPushMatrix();
		glLoadIdentity();

		glOrtho(0, width, 0, height, -1, 1);

//		glMatrixMode(GL_MODELVIEW);

//		glPushMatrix();
//		glLoadIdentity();
		glDisable(GL_LIGHTING);

		glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex2i(0, 0);
			glTexCoord2i(0, 1); glVertex2i(0, height);
			glTexCoord2i(1, 1); glVertex2i(width, height);
			glTexCoord2i(1, 0); glVertex2i(width, 0);
		glEnd();

//		glPopMatrix();

//		glMatrixMode(GL_PROJECTION);
//		glPopMatrix();

//		glMatrixMode(GL_MODELVIEW);

		glXSwapBuffers ( disp, pixmap );
		glFlush();
		glFinish();

		setRootAtoms( pixmap , disp , &win );
		XSetWindowBackgroundPixmap ( disp , win , pixmap );
		XClearWindow ( disp , win );

		usleep( 50000 );
		//sleep(1);
		

		++i;

		if( i==frames_len )
		{
			i=0;
		}
	}

	glDisable(GL_TEXTURE_2D);

//	glXSwapBuffers ( disp, pixmap );
/*
	XSelectInput(disp, win, ExposureMask);

	while(1)
	{
		XWindowAttributes       gwa;
		XEvent xev;

		XNextEvent( disp, &xev );

		std::cout << "New Event Type "<< xev.type << std::endl;

		if( xev.type == Expose )
		{
			XGetWindowAttributes(disp, win, &gwa);
			glViewport(0, 0, gwa.width, gwa.height);
			refresh_last_image( width , height , disp , win );
		}
		else
		{
			if (xev.type == KeyPress)
			{
				glXMakeCurrent( disp , None , NULL );
				glXDestroyContext( disp , glc );
				XDestroyWindow(disp , win);
				XKillClient (disp, AllTemporary);
				XCloseDisplay( disp );
				exit( 0 );
			}

		}
	}
*/
 	return 0;
}