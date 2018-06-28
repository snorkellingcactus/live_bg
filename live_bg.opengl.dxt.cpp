//g++ -o live_bg.opengl.dxt live_bg.opengl.dxt.cpp `pkg-config.pkg-config --libs gl glew imlib2 x11` -lgif -ltxc_dxtn

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
extern "C" {
	#include <txc_dxtn.h>
}
#include <Imlib2.h>
#include <gif_lib.h>
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
DATA32 *gif_get_frame_data( GifFileType *gif_file , unsigned  int frame_number )
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

	unsigned int len=frame_desc.Width*frame_desc.Height;

	DATA32 *imlib_data=new DATA32[len];
	for
	(
		unsigned int j=0 ;
		j < len ;
		++j
	)
	{
		DATA32 d32_pixel=0xffffffff;

		if( *rasters == TransparentColor )
		{
			d32_pixel&= 0xffffff00;
		}

		d32_pixel=( ( d32_pixel << 8 ) | colors[ *rasters ].Red		);
		d32_pixel=( ( d32_pixel << 8 ) | colors[ *rasters ].Green	);
		d32_pixel=( ( d32_pixel << 8 ) | colors[ *rasters ].Blue	);

		imlib_data[j]=d32_pixel;

		++rasters;
	}

	return imlib_data;
};
GLubyte * D32_to_GL_UBYTE (
	DATA32 *data ,
	unsigned int width ,
	unsigned int height ,
	unsigned short int comps=4
)
{
	unsigned int len=width*height;

	GLubyte *gl_rgba=new GLubyte[len*comps];

	for( unsigned int j=0 ; j<len ; j++ )
	{
		unsigned int k=comps*j;
		DATA32 pixel=data[j];

		gl_rgba[k]	= ((0x00ff0000 & pixel) >> 16	) ;
		gl_rgba[k+1] = ((0x0000ff00 & pixel) >> 8	) ;
		gl_rgba[k+2] = ( 0x000000ff & pixel			) ;
		gl_rgba[k+3] = ((0xff000000 & pixel) >> 24	) ;
	}

	return gl_rgba;
}
/*
unsigned int * D32_to_GL_RGBA( DATA32 *data , int width , int height )
{
	unsigned int len=width*height;

	unsigned int *gl_rgba=new unsigned int[len];

	for( unsigned int j=0 ; j<len ; j++ )
	{
		DATA32 pixel=data[j];

		gl_rgba[j]=pixel<<8;
	}

	return gl_rgba;
}
*/
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

	GLint  att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24};

	GLXContext glc = glXCreateContext (
		disp ,
		glXChooseVisual(disp, 0, att) ,
		NULL ,
		GL_TRUE
	);

	Pixmap pixmap=XCreatePixmap( disp , win , width , height , depth );
	setRootAtoms( pixmap , disp , &win );

// 	XLowerWindow( disp , win );

//	glewExperimental=TRUE;
	

	glXMakeCurrent ( disp , pixmap , glc );

	GLenum err=glewInit();
	if(err!=GLEW_OK) {
		// Problem: glewInit failed, something is seriously wrong.
		std::cout << "glewInit failed: " << glewGetErrorString(err) << std::endl;
		exit(1);
	}

	std::cout<<"::::::::::::::::::::::::::::::::."<<std::endl
	<< glGetString(GL_EXTENSIONS) <<std::endl;
	
	int error;

	std::cout << glXQueryExtensionsString( disp , error ) << std::endl;
	GifFileType *gif_file=gif_open_file( "test.gif" , error );
	
	int sWidth=gif_file->SWidth;
	int sHeight=gif_file->SHeight;
	int frames_len=gif_file->ImageCount;
	GLuint textures[ frames_len ];
	GifImageDesc desc;

	GLint gl_tex_format;

	if( glewGetExtension( "GL_EXT_texture_compression_s3tc" ) )
	{
		std::cout << "Using S3TC compression." << std::endl;
		gl_tex_format=GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}
	else
	{
		gl_tex_format=GL_RGB8;
	}

	imlib_set_cache_size( 0 );
	imlib_set_color_usage( 128 );
	imlib_context_set_display		( disp		);
	imlib_context_set_visual		( vis		);
	imlib_context_set_colormap		( cm		);
	imlib_context_set_drawable		( pixmap	);
	imlib_context_set_dither		( 1 );
	imlib_context_set_blend			( 1 );

	unsigned int i;

	for( i=0 ; i < frames_len ; i++ )
	{
		desc=gif_get_frame_desc( gif_file , i );
		
		imlib_context_set_image(
			imlib_create_image_using_data(
				desc.Width ,
				desc.Height,
				gif_get_frame_data( gif_file , i )
			)
		);

		imlib_image_set_has_alpha		( 1 );
		imlib_image_set_irrelevant_alpha( 0	);

		imlib_render_image_on_drawable( desc.Left , desc.Top );
		std::cout << "Texture: " << i << std::endl;

		imlib_free_image_and_decache();

		imlib_context_set_image (
			imlib_create_image_from_drawable(
				pixmap,
				0,
				0,
				sWidth,
				sHeight,
				0
			)
		);

		imlib_context_set_image(
			imlib_create_cropped_scaled_image(
				0,
				0,
				sWidth,
				sHeight,
				width,
				height
			)
		);

		imlib_image_flip_vertical();

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

		GLubyte *data;

		if( gl_tex_format==GL_COMPRESSED_RGBA_S3TC_DXT5_EXT )
		{
			data=new GLubyte[width*height*4];
			tx_compress_dxtn (
				( GLint ) 4 ,
				( GLint ) width ,
				( GLint ) height ,
				D32_to_GL_UBYTE (
					imlib_image_get_data() ,
					width ,
					height
				) ,
				( GLenum ) gl_tex_format ,
				data,
				( GLint ) 0
			);
		}
		else
		{
			data=D32_to_GL_UBYTE (
				imlib_image_get_data() ,
				width ,
				height
			);
		}

		imlib_free_image_and_decache();

		// Specify the texture specification
		//glCompressedTexImage2D
		//dxt5 size=((width + 3) / 4) * ((height + 3) / 4) * 16
		glCompressedTexImage2D (
			GL_TEXTURE_2D , 				// Type of texture
			0 ,				// Pyramid level (for mip-mapping) - 0 is the top level
			gl_tex_format ,	// Internal pixel format to use. Can be a generic type like GL_RGB or GL_RGBA, or a sized type
			width ,	// Image width
			height ,	// Image height
			0 ,				// Border width in pixels (can either be 1 or 0)
			((width + 3) / 4) * ((height + 3) / 4) * 16,
			data // The actual image data itself
		);

		delete data;
	}

	DGifCloseFile( gif_file , &error );

	glEnable(GL_TEXTURE_2D);
//	glEnable(GL_DITHER);
	glDisable(GL_BLEND);
//	glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glDrawBuffer( (GLenum ) GL_BACK );
	//glReadBuffer( (GLenum ) GL_BACK );

	i=0;

	while( 1 )
	{
		//XSetCloseDownMode ( disp , RetainTemporary );

		//glCopyPixels( 0 , 0 , width , height , GL_COLOR );
		//glClear( GL_COLOR_BUFFER_BIT );

		glBindTexture( GL_TEXTURE_2D , textures[ i ] );

		glMatrixMode(GL_PROJECTION);

//		glPushMatrix();
		glLoadIdentity();

		glViewport( 0 , 0 , width , height );

		glOrtho (
			0 ,
			width ,
			0 ,
			height ,
			-1 ,
			1
		);

//		glMatrixMode(GL_MODELVIEW);

//		glPushMatrix();
//		glLoadIdentity();
//		glDisable(GL_LIGHTING);

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

 	return 0;
}