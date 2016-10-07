#include "stdafx.h"
#include "texture.h"

void CSTexture::BuildPOT(BMPImage *img)
{
	int w,h;
	float sw,sh;
	w = 1 << (int)ceil((log((double)img->width)/log(2.0f)));
	h = 1 << (int)ceil((log((double)img->height)/log(2.0f)));

	sw=(float)img->width/w;
	sh=(float)img->height/h;

	unsigned char *nbuf = new unsigned char[w*h*4];

	for(int y=0; y<h;++y) for(int x=0;x<w;++x) {
		nbuf[y*w*4+x*4]=img->data[(int)(sh*y)*img->width*4+(int)(sw*x)*4];
		nbuf[y*w*4+x*4+1]=img->data[(int)(sh*y)*img->width*4+(int)(sw*x)*4+1];
		nbuf[y*w*4+x*4+2]=img->data[(int)(sh*y)*img->width*4+(int)(sw*x)*4+2];
		nbuf[y*w*4+x*4+3]=img->data[(int)(sh*y)*img->width*4+(int)(sw*x)*4+3];
    }
	//gluBuild2DMipmaps ( GL_TEXTURE_2D, GL_RGBA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nbuf);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nbuf);

	delete[] nbuf;
}

void CSTexture::LoadBitmap(char *filename)
{
    BMPImage *pImage=new BMPImage;
	FILE *pFile = NULL;
    unsigned short nNumPlanes;
    unsigned short nNumBPP;
	int i;

    if( (pFile = fopen(filename, "rb") ) == NULL )
		printf("ERROR: getBitmapImageData - %s not found\n",filename);

    // Seek forward to width and height info
    fseek( pFile, 18, SEEK_CUR );

    if( (i = fread(&pImage->width, 4, 1, pFile) ) != 1 )
		printf("ERROR: getBitmapImageData - Couldn't read width from %s.\n", filename);

    if( (i = fread(&pImage->height, 4, 1, pFile) ) != 1 )
		printf("ERROR: getBitmapImageData - Couldn't read height from %s.\n", filename);

    if( (fread(&nNumPlanes, 2, 1, pFile) ) != 1 )
		printf("ERROR: getBitmapImageData - Couldn't read plane count from %s.\n", filename);

    if( nNumPlanes != 1 )
		printf( "ERROR: getBitmapImageData - Plane count from %s is not 1: %u\n", filename, nNumPlanes );

    if( (i = fread(&nNumBPP, 2, 1, pFile)) != 1 )
		printf( "ERROR: getBitmapImageData - Couldn't read BPP from %s.\n", filename );

    // Seek forward to image data
    fseek( pFile, 24, SEEK_CUR );

    if( nNumBPP != 24 )  {
        //BGRA
        int nTotalImagesize = (pImage->width * pImage->height) * 4;
        xs=pImage->width;
        ys=pImage->height;

        pImage->data = (char*) malloc( nTotalImagesize );
        data=(unsigned char*)pImage->data;

        if( (i = fread(pImage->data, nTotalImagesize, 1, pFile) ) != 1 )
            printf("ERROR: getBitmapImageData - Couldn't read image data from %s.\n", filename);


        bool hasalpha=false;
		alpha=false;
        for(int i = 0; i < xs*ys*4;i+=4)
        {
            unsigned char temp = pImage->data[i];
            pImage->data[i  ] = pImage->data[i+2];
            pImage->data[i+2] = temp;
			if(pImage->data[i+3]!=0) {
				hasalpha=true;
				if(pImage->data[i+3]!=0xFF) alpha=true;
			}
            //data[i+3] = 0xFF;
        }
        if(!hasalpha) for(int i=0;i<xs*ys*4;i+=4) pImage->data[i+3]=0xFF;

        glGenTextures(1, &tid);						// Create The Texture

        // Typical Texture Generation Using Data From The Bitmap
        glBindTexture(GL_TEXTURE_2D, tid);					// Bind To The Texture ID
        glTexParameteri(GL_TEXTURE_2D, 0x8191, GL_FALSE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);		// (Modify This For The Type Of Filtering You Want)
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);		// (Modify This For The Type Of Filtering You Want)

        // (Modify This If You Want Mipmaps)
		BuildPOT(pImage);
		//gluBuild2DMipmaps ( GL_TEXTURE_2D, GL_RGBA, pImage->width, pImage->height, GL_RGBA, GL_UNSIGNED_BYTE, pImage->data);
		//glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, pImage->width, pImage->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pImage->data);
		/*int i,j;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&i);
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&j);
		char buf[32];
		sprintf(buf,"%d,%d",i,j);
		MessageBox(NULL,buf,"",0);*/
    } else {
        //BGR
        // Calculate the image's total size in bytes. Note how we multiply the
        // result of (width * height) by 3. This is becuase a 24 bit color BMP
        // file will give you 3 bytes per pixel.
        int nTotalImagesize = (pImage->width * pImage->height) * 3;
        xs=pImage->width;
        ys=pImage->height;

        pImage->data = (char*) malloc( xs*ys*4 );
        data=(unsigned char*)pImage->data;

        if( (i = fread(pImage->data, nTotalImagesize, 1, pFile) ) != 1 )
            printf("ERROR: getBitmapImageData - Couldn't read image data from %s.\n", filename);

        //
        // Finally, rearrange BGR to RGBA
        //

        char charTemp;
        for( i = nTotalImagesize-3; i >= 0; i -= 3 )
        {
            charTemp = pImage->data[i];
            pImage->data[(i*4)/3] = pImage->data[i+2];
            pImage->data[(i*4)/3+1] = pImage->data[i+1];
            pImage->data[(i*4)/3+2] = charTemp;
            pImage->data[(i*4)/3+3] = 0xFF;
        }

        glGenTextures(1, &tid);						// Create The Texture

        // Typical Texture Generation Using Data From The Bitmap
        glBindTexture(GL_TEXTURE_2D, tid);					// Bind To The Texture ID
        glTexParameteri(GL_TEXTURE_2D, 0x8191, GL_FALSE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);		// (Modify This For The Type Of Filtering You Want)
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);		// (Modify This For The Type Of Filtering You Want)

        // (Modify This If You Want Mipmaps)
        BuildPOT(pImage);
        //gluBuild2DMipmaps ( GL_TEXTURE_2D, GL_RGBA, pImage->width, pImage->height, GL_RGBA, GL_UNSIGNED_BYTE, pImage->data);
    }
    delete pImage;

	return;
}

void CSTexture::CKCopy(CSTexture *src,int r,int g,int b)
{
	xs=src->xs;
	ys=src->ys;
	alpha=src->alpha;
	data=(unsigned char*)malloc(xs*ys*4);

	for(int i=0;i<xs*ys*4;i+=4) {
		if(src->data[i]==r && src->data[i+1]==g && src->data[i+2]==b) {
			memset(data+i,0,4);
		} else
			memcpy(data+i,src->data+i,4);
	}

	glGenTextures(1, &tid);						// Create The Texture

	// Typical Texture Generation Using Data From The Bitmap
	glBindTexture(GL_TEXTURE_2D, tid);					// Bind To The Texture ID
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);		// (Modify This For The Type Of Filtering You Want)
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);		// (Modify This For The Type Of Filtering You Want)

	// (Modify This If You Want Mipmaps)
	gluBuild2DMipmaps ( GL_TEXTURE_2D, GL_RGBA, xs, ys, GL_RGBA, GL_UNSIGNED_BYTE, data);

	return;
}

void CSTexture::SubareaCopy(CSTexture *src,int x1,int y1,int x2,int y2)
{
	xs=x2-x1;
	ys=y2-y1;
	alpha=src->alpha;
	data=(unsigned char*)malloc(xs*ys*4);

	for(int y=y1;y<y2;++y) {
		memcpy(data+(y-y1)*xs*4,src->data+(y*src->xs*4+x1*4),(x2-x1)*4);
	}

	glGenTextures(1, &tid);						// Create The Texture

	// Typical Texture Generation Using Data From The Bitmap
	glBindTexture(GL_TEXTURE_2D, tid);					// Bind To The Texture ID
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);		// (Modify This For The Type Of Filtering You Want)
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);		// (Modify This For The Type Of Filtering You Want)

    BMPImage img;
    img.height=ys;
    img.width=xs;
    img.data=(char*)data;

    BuildPOT(&img);

	// (Modify This If You Want Mipmaps)
	//gluBuild2DMipmaps ( GL_TEXTURE_2D, GL_RGBA, xs, ys, GL_RGBA, GL_UNSIGNED_BYTE, data);

	return;
}

void CSTexture::Bind()
{
	/*if(alpha) {
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
	} else {
		glDisable(GL_BLEND);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER,0.5);
	}*/
	glBindTexture(GL_TEXTURE_2D,tid);
}
