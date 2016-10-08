//g++ -o example $x86_64_pc_linux_gnu_CXXFLAGS bg_color.cpp `pkg-config.pkg-config --libs imlib2 x11`  -lgif
/* include X11 stuff */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
/* include Imlib2 stuff */
#include <Imlib2.h>
#include <gif_lib.h>
/* sprintf include */
#include <unistd.h>

#include <signal.h>

#include <iostream>
#include "./gif_err.c"

using namespace std;

//gif_is_animated();
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
	
	GifImageDesc frame_desc=gif_get_frame_desc( gif_file , frame_number );
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
		else
		{
			d32_pixel|=0xffffffff;
		}

		d32_pixel=( ( d32_pixel << 8 ) | colors[ *rasters ].Red		);
		d32_pixel=( ( d32_pixel << 8 ) | colors[ *rasters ].Green	);
		d32_pixel=( ( d32_pixel << 8 ) | colors[ *rasters ].Blue	);

		*(imlib_data+j)=d32_pixel;

		++rasters;
	}

	return imlib_data;
};
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
void XSetBackgroundImage( Display *display , Window window , Pixmap pixmap , unsigned int width , unsigned int height , int left , int top )
{
	XSetCloseDownMode ( display , RetainTemporary );
	//XClearWindow (disp, win);
	//	XSync( disp , false );  

	//cout << "Left:" << left << " Top:" << top << endl;
	imlib_render_image_on_drawable( left , top );
	//imlib_render_image_on_drawable_at_size( left, top , w , h );
	//imlib_free_image( );
	
	setRootAtoms( pixmap , display , &window );

	XSetWindowBackgroundPixmap ( display , window , pixmap );

	//XFreePixmap( disp , pixmap );
	XClearWindow ( display , window );
	XFlush( display );
}

bool interrupt=false;

void graceful_exit( int signum )
{
	//cout << "Recieved signal interrupt number " << signum << ". Exiting." << endl;
	interrupt=true;
}

struct gif_file_tmp
{
	int *top;
	int *left;
	int *width;
	int *height;
	int frames_len;
	int SWidth;
	int SHeight;
};

int main(int argc, char **argv)
{
	signal( SIGINT , graceful_exit );

	unsigned int			depth;
	unsigned int			border;
	unsigned int			screen;
	unsigned int			width	;
	unsigned int			height;
	int			w;
	int			h;
	int			x;
	int			y;


	Display *disp;
	Window	win;
	Visual  *vis;
	Colormap cm;
	Pixmap pixmap;

	/* connect to X */
	disp  = XOpenDisplay(NULL);
	/* get default visual , colormap etc. you could ask imlib2 for what it */
	/* thinks is the best, but this example is intended to be simple */
	screen 	= DefaultScreen			( disp					);
	win			= DefaultRootWindow ( disp 					);
	depth		= DefaultDepth			( disp , screen );
	vis			= DefaultVisual			( disp , screen );
	cm			= DefaultColormap		( disp , screen );
	width		= DisplayWidth			( disp , screen );
	height	= DisplayHeight			( disp , screen );
	
	x=0 ; y=0 ;  border=0 ;

	/* create a window 640x480 */
	//win = XCreateSimpleWindow(disp, DefaultRootWindow( disp ), 0, 0, width, height, 0, 0, 0);
	//XLowerWindow( disp , win );
	

	//XSelectInput(disp, win, PointerMotionMask | ExposureMask);
	pixmap=XCreatePixmap( disp , win , width , height , depth );
	//XGetGeometry(  disp , win , &win , &x , &y , &width , &height , &border , &depth  );
	//XMapWindow( disp, win );
/*
	std::cout << "Screen: " << screen <<endl ;
	std::cout << "Depth: "  << depth  <<endl ;
	std::cout << "Width: "  << width  <<endl ;
	std::cout << "Height: "  << height  <<endl ;
	std::cout << "Border: "  << border  <<endl ;
	std::cout << "X: "  << x  <<endl ;
	std::cout << "Y: "  << y  <<endl ;
*/
	imlib_set_cache_size( 1 );
	imlib_set_color_usage( 128 );
	imlib_context_set_display		( disp		);
	imlib_context_set_visual		( vis		);
	imlib_context_set_colormap		( cm		);
	imlib_context_set_drawable		( pixmap	);

	int error;

	GifFileType *gif_file=gif_open_file( "./animated3.gif" , error );
	
	gif_file_tmp gif_loop_data;
	gif_loop_data.SWidth=gif_file->SWidth;
	gif_loop_data.SHeight=gif_file->SHeight;
	gif_loop_data.frames_len=gif_file->ImageCount;

	if( gif_loop_data.frames_len )
	{
		Imlib_Image *gif_images=new Imlib_Image[gif_loop_data.frames_len];
		
		gif_loop_data.width=new int[gif_loop_data.frames_len];
		gif_loop_data.height=new int[gif_loop_data.frames_len];
		gif_loop_data.left=new int[gif_loop_data.frames_len];
		gif_loop_data.top=new int[gif_loop_data.frames_len];

		unsigned int i=0;

		imlib_context_set_dither		( 1 );
		imlib_context_set_blend			( 1 );

		for( i=0 ; i < gif_loop_data.frames_len ; i++ )
		{
			gif_loop_data.width[i]=(gif_file->SavedImages+i)->ImageDesc.Width;
			gif_loop_data.height[i]=(gif_file->SavedImages+i)->ImageDesc.Height;
			gif_loop_data.left[i]=gif_get_frame_left( gif_file , i );
			gif_loop_data.top[i]=gif_get_frame_top( gif_file , i );
/*
			cout << "Geom:" << gif_loop_data.width[i] << " X ";
			cout << gif_loop_data.height[i] << endl;
			cout << "Geom:" << gif_loop_data.SWidth << " X ";
			cout << gif_loop_data.SHeight << endl;
			cout << "Marg:" << gif_loop_data.left[i] << " X ";
			cout << gif_loop_data.top[i] << endl;
			cout << "Interlace:" << (gif_file->SavedImages+i)->ImageDesc.Interlace << endl;
*/
			imlib_context_set_image
			(
				*(gif_images+i)=imlib_create_image_using_data(
					gif_loop_data.width[i],
					gif_loop_data.height[i],
					gif_get_frame_data( gif_file , i )
				)
			);

			imlib_image_set_has_alpha		( 1 );
			imlib_image_set_irrelevant_alpha( 0	);

			imlib_render_image_on_drawable(
				gif_loop_data.left[i] ,
				gif_loop_data.top[i]
			);

			imlib_free_image_and_decache();
			imlib_context_set_image
			(
				imlib_create_image_from_drawable(
					pixmap,
					0,
					0,
					gif_loop_data.SWidth,
					gif_loop_data.SHeight,
					0
				)
			);

			imlib_context_set_image(
				*(gif_images+i)=imlib_create_cropped_scaled_image(
					0,
					0,
					gif_loop_data.SWidth,
					gif_loop_data.SHeight,
					width,
					height
				)
			);

			cout << "Image "<< i << " " << sizeof(*(gif_images+i)) <<endl;
		}

		DGifCloseFile( gif_file , &error );



		imlib_context_set_dither		( 0 );
		imlib_context_set_blend			( 0 );
		imlib_set_cache_size( 10 * 1024 * 1024 );

		i=0;
		while( 1 && ! interrupt )
		{
/*
			if(top>1)
			{
				top=-h+top-1;
				left=21;
			}

			//cout << "Frame " << i ;
			//cout << " Top: " << gif_get_frame_top( gif_file , i );
			//cout << " Left: " << gif_get_frame_left( gif_file , i ) << endl;
*/
			imlib_context_set_image		( *(gif_images+i) );

			imlib_image_set_has_alpha		( 0 );
			imlib_image_set_irrelevant_alpha( 1	);

			XSetBackgroundImage
			(
				disp ,
				win ,
				pixmap ,
				width ,
				height ,
				0,
				0
			);

			usleep( 50000 );

			++i;

			if(i==gif_loop_data.frames_len)
			{
				i=0;
			}
		}

		XClearWindow (disp, win);
		XFreePixmap( disp , pixmap );
		XFlush( disp );
		XSync( disp , false );  
		XKillClient (disp, AllTemporary);
	}

	return 0;
}