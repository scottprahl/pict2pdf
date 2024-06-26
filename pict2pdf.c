/* 
Copyright (c) 2002, Scott Prahl

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ApplicationServices/ApplicationServices.h>
#include <string.h>
#include <unistd.h>

char version[]="1.1";

static double min4(double a, double b, double c, double d)
{
	double x=a;
	if (b<x) x=b;
	if (c<x) x=c;
	if (d<x) x=d;
	return x;
}

static double max4(double a, double b, double c, double d)
{
	double x=a;
	if (b>x) x=b;
	if (c>x) x=c;
	if (d>x) x=d;
	return x;
}

/* find the bounding box for a rectangle rotated about its center */

static CGRect
BoundingBox(CGRect r, double angle)
{
	CGRect bb;
	double tr,br,tl,bl,w,h;
	double sintheta = sin(angle);
	double costheta = cos(angle);
	
	w = r.size.width/2.0;
	h = r.size.height/2.0;
	
	tr =  w*costheta - h*sintheta;
	br =  w*costheta + h*sintheta;
	tl = -w*costheta - h*sintheta;
	bl = -w*costheta + h*sintheta;
	bb.origin.x = min4(bl,tr,br,tl);
	bb.size.width = max4(bl,tr,br,tl)-bb.origin.x;
	bb.origin.x += r.origin.x + w;
	
	tr =  w*sintheta + h*costheta;
	br =  w*sintheta - h*costheta;
	tl = -w*sintheta + h*costheta;
	bl = -w*sintheta - h*costheta;
	
	bb.origin.y = min4(bl,tr,br,tl);
	bb.size.height = max4(bl,tr,br,tl)-bb.origin.y;
	bb.origin.y += r.origin.y + h;
	return bb;
}

/* draw the give picture reference into the specified context */
/* some code adapted from Apple's example CGDrawPicture.c     */
static OSErr Pict2Context(QDPictRef pict_ref, CGContextRef ctx, float angle, 
float scalex, float scaley, float lnweight)
{
    CGRect			pict_rect, bb_rect;
    OSStatus		err;
    
    angle *= 3.14159265358979423/180.0;  /* convert to radians */

    pict_rect = QDPictGetBounds(pict_ref);
    	
	pict_rect.origin.x *= scalex;
	pict_rect.origin.y *= scaley;
	pict_rect.size.width *= scalex;
	pict_rect.size.height *= scaley;

/* determine bounding box for rotated PICT */	
	bb_rect = BoundingBox(pict_rect,angle);
    CGContextBeginPage(ctx, &bb_rect);

	if (lnweight > 0)
		CGContextStrokeRectWithWidth(ctx, bb_rect, lnweight);

/* rotate context */
	if (angle != 0.0){
		CGPoint p;
		p.x = pict_rect.origin.x + pict_rect.size.width/2;
		p.y = pict_rect.origin.y + pict_rect.size.height/2;
		CGContextTranslateCTM(ctx, p.x, p.y);
		CGContextRotateCTM(ctx, angle);
		CGContextTranslateCTM(ctx, -p.x, -p.y);
	}

/* Avoid bug in 10.0 and 10.1 with patterns */
	CGContextSaveGState(ctx);
	CGContextScaleCTM(ctx, scalex, scaley);

	pict_rect.origin.x /= scalex;
	pict_rect.origin.y /= scaley;
	pict_rect.size.width /= scalex;
	pict_rect.size.height /= scaley;

    err = QDPictDrawToCGContext(ctx, pict_rect, pict_ref);
    CGContextRestoreGState(ctx);
    
    CGContextEndPage(ctx);
    CGContextFlush(ctx);

    return err;
}

char * CreatePDFname(char *pict)
{
    char *p, *pdf;
    pdf = malloc(strlen(pict) + 4);  /* may need to append .pdf */
    if (pdf == NULL) exit(1);
    
    strcpy(pdf,pict);
    p = strstr(pdf, ".pict");
    if (p == NULL) {
    	p = strstr(pdf, ".PICT");
    	if (p == NULL)
    		p = pdf + strlen(pict);
    }
    strcpy(p,".pdf");
    return pdf;
}

/* create a PDF file context and draw the picture in that Graphics Context */
static OSErr WritePictAsPDF(char *pict, char *pdf, float angle, 
                            float scale, float width, float height, float lnweight)
{
    CFStringRef 	pict_name, pdf_name;
    CFURLRef		pict_url, pdf_url;
    QDPictRef 		pict_ref;
    CGRect			pict_rect;
    CGContextRef	pdf_ctx;
    OSStatus		err;
    float			scalex, scaley;
    
    pict_name = CFStringCreateWithCString(NULL, pict, kCFStringEncodingMacRoman);
    pict_url  = CFURLCreateWithFileSystemPath(NULL, pict_name, kCFURLPOSIXPathStyle, FALSE);
    pict_ref  = QDPictCreateWithURL(pict_url);

    CFRelease(pict_name);
    CFRelease(pict_url);
    if (pict_ref == NULL) return 1;

    pict_rect = QDPictGetBounds(pict_ref);
    
    if (pict_rect.size.width == 0 || pict_rect.size.height == 0) {
    	scalex = scale;
    	scaley = scale;
    } else if (width > 0 && height == -1.0) {
    	scalex = width/pict_rect.size.width;
    	scaley = scalex;
    } else if (height > 0 && width == -1.0) {
    	scalex = height/pict_rect.size.height;
    	scaley = scalex;
    } else if (height > 0 && width > 0) {
        scalex = width/pict_rect.size.width;
    	scaley = height/pict_rect.size.height;
    } else {
    	scalex = scale;
    	scaley = scale;
    }
    
    pdf_name  = CFStringCreateWithCString(NULL, pdf, kCFStringEncodingMacRoman);
    pdf_url   = CFURLCreateWithFileSystemPath(NULL, pdf_name,  kCFURLPOSIXPathStyle, FALSE);
	pdf_ctx   = CGPDFContextCreateWithURL(pdf_url, &pict_rect, NULL);

	err = Pict2Context(pict_ref, pdf_ctx, angle, scalex, scaley, lnweight);
	
    CFRelease(pdf_ctx);
    CFRelease(pdf_name);
    CFRelease(pdf_url); 
    return err;
}

void usage(void)
{
	fprintf(stderr, "usage: pict2pdf [-fv] [-r angle] [-s scale] [-w width] [-h height] file1 [file2]\n");
    fprintf(stderr, "                 -b lnwidth add border lnwidth pixels wide\n");
    fprintf(stderr, "                 -f         overwrite existing PDF files\n");
    fprintf(stderr, "                 -h height  height in pixels (1/72 inch)\n");
    fprintf(stderr, "                 -r angle   rotate CCW by angle degrees\n");
    fprintf(stderr, "                 -s scale   scale image (0.5 => 50%%)\n");
    fprintf(stderr, "                 -v         print version info\n");
    fprintf(stderr, "                 -w width   width in pixels (1/72 inch)\n");
	exit(2);
}

int
main (int argc, char *argv[ ])
{
    extern char *optarg;
    extern int optind, optopt;
    int c,n,err;
    int force_new_pdf = 0;
    float angle = 0.0;
    float scale = 1.0;
    float width = -1.0;
    float height = -1.0;
	float lnweight = 0.0;
	
    while ((c = getopt(argc, argv, "fvb:h:r:s:w:")) != -1) {
        switch (c) {
        case 'f':
            force_new_pdf = 1;
            break;
        
        case 'h':
            n=sscanf(optarg, "%g", &height);
            if (n!=1) {
                printf("unrecognized height: -w %s\n",optarg);
                usage();
            }
            break;

        case 'b':
            n=sscanf(optarg, "%g", &lnweight);
            if (n!=1) {
                printf("unrecognized line weight: -b %s\n",optarg);
                usage();
            }
            break;

        case 'w':
            n=sscanf(optarg, "%g", &width);
            if (n!=1) {
                printf("unrecognized width: -w %s\n",optarg);
                usage();
            }
            break;

        case 'r':
            n=sscanf(optarg, "%g", &angle);
            if (n!=1) {
                printf("unrecognized rotation angle: -r %s\n",optarg);
                usage();
            }
            break;
        
        case 's':
            n=sscanf(optarg, "%g", &scale);
            if (n!=1) {
                printf("unrecognized scaling factor: -s %s\n",optarg);
                usage();
            }
            break;
        
        case 'v':
            fprintf(stderr,"pict2pdf version %s\n",version);
            exit(1);
            break;
        
        case '?':
            printf("unrecognized option: %c\n",optopt);
            usage();
            break;
        }
    }
    
    for (; optind < argc; optind++) {
        FILE *fp;
        char *pdf;
        int pdf_file_exists = 0;
        
        pdf = CreatePDFname(argv[optind]);
        
        if (!force_new_pdf) {  /* no need to check, since we will overwrite */
            fp = fopen(pdf, "r");
            if (fp != NULL) {
                pdf_file_exists = 1;
                fclose(fp);
            }
        }
            
        if (force_new_pdf || !pdf_file_exists) {
        
            err = WritePictAsPDF(argv[optind], pdf, angle, scale, width, height, lnweight);
            if (err) exit(err);
        }
        
        free(pdf);
    }
    
    return 0;
}

