#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>
#include <math.h>
#include <omp.h>

#ifndef NUM_THREADS
#define NUM_THREADS 64
#endif

int read_BMP(char* filename, unsigned char *info, unsigned char **dataR, unsigned char **dataG, unsigned char **dataB, int *size, int *width, int *height, int *offset, int *row_padded)
{
    int i = 0, j, k, read_bytes, h, w, o, p;
    unsigned char *data;

    FILE* f = fopen(filename, "rb");

    if(f == NULL)
    {
        printf ("Invalid filename: %s\n", filename);
        return -1;
    }


    read_bytes = fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header
    if (read_bytes != 54)
    {
        printf ("Error at read: %d instead of 54 bytes", read_bytes);
        return -1;
    }


    // extract image data from header
    *width = *(int*)&info[18];
    *height = *(int*)&info[22];
    *size = *(int*)&info[2];
    *offset = *(int*)&info[10];
    *row_padded = (*width*3 + 3) & (~3);


    //printf ("Filename: %s, Width: %d, Row_padded: %d, Height: %d, Size:  %d, Offset: %d\n", filename, *width, *row_padded, *height, *size, *offset);
    w = *width;
    p = *row_padded;
    h = *height;
    o = *offset;

    data = (unsigned char*) malloc (p * h);
    *dataR = (unsigned char*) malloc (w * h);
    *dataG = (unsigned char*) malloc (w * h);
    *dataB = (unsigned char*) malloc (w * h);

    fseek(f, sizeof(unsigned char) * o, SEEK_SET);
    read_bytes = fread(data, sizeof(unsigned char), p * h, f);
    if (read_bytes != p * h)
    {
        printf ("Error at read: %d\n", read_bytes);
        free (data);
        return -1;
    }
    for (k = 0; k < h; k++)
    {
        i = k * p;
        for (j = 0; j < w; j++)
        {
            (*dataB)[k*w + j] = data[i];
            (*dataG)[k*w + j] = data[i + 1];
            (*dataR)[k*w + j] = data[i + 2];

            //printf ("BGR %d %d i= %d: %d %d %d\n", k, j, i, data[i], data[i+1], data[i+2]);
            i+= 3;
        }
    }

    free (data);
    fclose(f);
    return 0;
}

int write_BMP(char* filename, float *dataB, float *dataG, float *dataR, unsigned char *header, int offset, int width,  int row_padded, int height)
{
    int write_bytes = 0, i, pad_size;
    FILE* f = fopen(filename, "wb");
    unsigned char null_byte = 0, valR, valB, valG;

    write_bytes = fwrite (header, sizeof(unsigned char), offset, f);
    if (write_bytes < offset)
    {
        printf( "Error at writing the header\n");
        return -1;
    }


    for (i = 0; i< width*height; i++)
    {
        if ( dataB[i] > 256.0f || dataR[i] > 256.0f || dataG[i] > 256.0f ){
            printf( "Error: invalid value %f %f %f", dataB[i], dataG[i], dataR[i]);
            return -1;
        }

        valB = dataB[i];
        valG = dataG[i];
        valR = dataR[i];
        write_bytes = fwrite(&valB, sizeof( unsigned char ), 1, f );
	if (write_bytes != 1)
	{
	    printf ("Error at write: i = %d %d\n", i, valB);
	    return -1;
	}
        write_bytes = fwrite(&valG, sizeof( unsigned char ), 1, f );
	if (write_bytes != 1)
	{
	    printf ("Error at write: i = %d %d\n", i, valG);
	    return -1;
	}
        write_bytes = fwrite(&valR, sizeof( unsigned char ), 1, f );
	if (write_bytes != 1)
	{
	    printf ("Error at write: i = %d %d\n", i, valR);
	    return -1;
	}

        if ((i + 1) % width == 0 ) {
            pad_size = row_padded - width *3 ;
            while( pad_size-- > 0 ) {
                fwrite(&null_byte, sizeof( unsigned char), 1, f );
            }
        }
    }

    fclose (f);
    return 0;
}

/**
 * Convolves a kernel and buffer together.
 * Kernel and buffer must have a size that is a multiple of 8
 * @param  kernel [description]
 * @param  buffer [description]
 * @param  ksize  [description]
 * @return        [description]
 */
float convolve(float** dsts, const float *kernel, const float *buffer, const int ksize, const int pos, const int pos2) {
    float sum = 0.0f;
    __m256 eight_sum = _mm256_set_ps(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    int i;
    for(i=0; i<ksize; i++)
    {
        __m256 k256 = _mm256_broadcast_ss((float *) kernel + i);
        __m256 m_res = _mm256_mul_ps(k256, _mm256_load_ps(buffer + (i * 8)));
        eight_sum = _mm256_add_ps(eight_sum, m_res);
    }

    float* f = (float*)&eight_sum;
    for (i = 0; i < 3; i++) {
        dsts[i][pos] = f[i];
    }

    if (pos2 != -1)
    {
        for (i = 0; i < 3; i++) {
            dsts[i][pos2] = f[i + 4];
        }
    }

    return sum;
}

/**
 * Performs a Gaussian blur on the supplied image
 * @param srcs   [description]
 * @param dsts   [description]
 * @param width  [description]
 * @param height [description]
 * @param sigma  [description]
 * @param ksize  [description]
 */
void gaussian_blur(unsigned char **srcs, float **dsts, int width, int height, float sigma, int ksize)
{
    int x, y, i, i2, x1, x2, y1, y2;

    int halfksize = ksize / 2;
    float sum = 0.f, t;
    float *kernel;

    int kpad = 8 - (ksize % 8);

    int kmallocres = posix_memalign((void**)&kernel, 64, (ksize + kpad) * sizeof(float));
    if (kmallocres)
    {
        printf ("Error in memory allocation!\n");
        return;
    }

    // if sigma too small, just copy src to dst
    if (ksize <= 1)
    {
        int color;
        for (color = 0; color < 3; color++)
        {
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                    dsts[color][y*width + x] = srcs[color][y*width + x];
            return;
        }
    }


    //compute the Gaussian kernel values
    for (i = 0; i < ksize; i++)
    {
        x = i - halfksize;
        t = expf(- x * x/ (2.0f * sigma * sigma)) / (sqrt(2.0f * M_PI) * sigma);
        kernel[i] = t;
        sum += t;
    }
    for (i = 0; i < ksize; i++)
    {
        kernel[i] /= sum;
        //printf ("Kernel [%d] = %f\n", i, kernel[i]);
    }

    #pragma omp parallel num_threads(NUM_THREADS) private(x, y, x1, x2, y1, y2, i, i2)
    {
        float *buffer;
        int blocres = posix_memalign((void**)&buffer, 64, (8 * ksize) * sizeof(float));

        // blur each row
        #pragma omp for
        for (y = 0; y < height; y++)
        {
            int px = 0;
            for (x1 = 0; x1 < halfksize; x1++)
            {
                px = x1 * 8;
                buffer[px] = (float) srcs[0][y*width];
                buffer[px + 1] = (float) srcs[1][y*width];
                buffer[px + 2] = (float) srcs[2][y*width];
                buffer[px + 3] = 0.0;
                buffer[px + 4] = buffer[px];
                buffer[px + 5] = buffer[px + 1];
                buffer[px + 6] = buffer[px + 2];
                buffer[px + 7] = 0.0;
            }

            for (x1 = halfksize; x1 < ksize-1; x1++)
            {
                px = x1 * 8;
                buffer[px] = (float) srcs[0][y*width + x1-halfksize];
                buffer[px + 1] = (float) srcs[1][y*width + x1-halfksize];
                buffer[px + 2] = (float) srcs[2][y*width + x1-halfksize];
                buffer[px + 3] = 0.0;
                buffer[px + 4] = buffer[px];
                buffer[px + 5] = buffer[px + 1];
                buffer[px + 6] = buffer[px + 2];
                buffer[px + 7] = 0.0;
            }
            x1 = 0;
            for (x1 = 0; x1 < width; x1+=2)
            {
                i = ((x1+ksize-1)%ksize) * 8;
                i2 = (((x1+1+ksize-1)%ksize) * 8) + 4;
                x2 = x1 + 1;

                if(x1 < width - halfksize)
                {
                    buffer[i] = (float) srcs[0][ y * width + x1 + halfksize ];
                    buffer[i + 1] = (float) srcs[1][ y * width + x1 + halfksize ];
                    buffer[i + 2] = (float) srcs[2][ y * width + x1 + halfksize ];

                }
                else
                {
                    buffer[i] = (float) srcs[0][ y * width + width - 1 ];
                    buffer[i + 1] = (float) srcs[1][ y * width + width - 1 ];
                    buffer[i + 2] = (float) srcs[2][ y * width + width - 1 ];
                }

                // copy over to other pixel
                buffer[i + 4] = buffer[i];
                buffer[i + 5] = buffer[i + 1];
                buffer[i + 6] = buffer[i + 2];

                if (x2 < width)
                {
                    if(x2 < width - halfksize)
                    {
                        buffer[i2] = (float) srcs[0][ y * width + x2 + halfksize ];
                        buffer[i2 + 1] = (float) srcs[1][ y * width + x2 + halfksize ];
                        buffer[i2 + 2] = (float) srcs[2][ y * width + x2 + halfksize ];
                    }
                    else
                    {
                        buffer[i2] = (float) srcs[0][ y * width + width - 1 ];
                        buffer[i2 + 1] = (float) srcs[1][ y * width + width - 1 ];
                        buffer[i2 + 2] = (float) srcs[2][ y * width + width - 1 ];
                    }
                    convolve(dsts, kernel, buffer, ksize, y*width + x1, y*width + x2);
                }
                else
                {
                    convolve(dsts, kernel, buffer, ksize, y*width + x1, -1);
                }

                buffer[i2 - 4] = buffer[i2];
                buffer[i2 - 3] = buffer[i2 + 1];
                buffer[i2 - 2] = buffer[i2 + 2];
            }
        }

        // blur each column
        #pragma omp for
        for (x = 0; x < width; x++)
        {
            int yp;
            for (y1 = 0; y1 < halfksize  ; y1++)
            {
                yp = y1 * 8;
                buffer[yp] = dsts[0][0*width + x];
                buffer[yp + 1] = dsts[1][0*width + x];
                buffer[yp + 2] = dsts[2][0*width + x];
                buffer[yp + 3] = 0.0;
                buffer[yp + 4] = buffer[yp];
                buffer[yp + 5] = buffer[yp + 1];
                buffer[yp + 6] = buffer[yp + 2];
                buffer[yp + 7] = 0.0;
            }
            for (y1=halfksize; y1 < ksize-1; y1++)
            {
                yp = y1 * 8;
                buffer[yp] = dsts[0][(y1-halfksize)*width + x];
                buffer[yp + 1] = dsts[1][(y1-halfksize)*width + x];
                buffer[yp + 2] = dsts[2][(y1-halfksize)*width + x];
                buffer[yp + 3] = 0.0;
                buffer[yp + 4] = buffer[yp];
                buffer[yp + 5] = buffer[yp + 1];
                buffer[yp + 6] = buffer[yp + 2];
                buffer[yp + 7] = 0.0;
            }

            for (y1 = 0; y1 < height; y1++)
            {
                y2 = y1 + 1;
                i = ((y1+ksize-1)%ksize) * 8;
                i2 = (((y2+ksize-1)%ksize) * 8) + 4;

                if(y1 < height - halfksize)
                {
                    buffer[i] = dsts[0][(y1+halfksize)*width + x];
                    buffer[i + 1] = dsts[1][(y1+halfksize)*width + x];
                    buffer[i + 2] = dsts[2][(y1+halfksize)*width + x];
                }
                else
                {
                    buffer[i] = dsts[0][(height-1)*width + x];
                    buffer[i + 1] = dsts[1][(height-1)*width + x];
                    buffer[i + 2] = dsts[2][(height-1)*width + x];
                }

                // copy
                buffer[i + 4] = buffer[i];
                buffer[i + 5] = buffer[i + 1];
                buffer[i + 6] = buffer[i + 2];

                if (y2 < height) {
                    if(y2 < height - halfksize)
                    {
                        buffer[i2] = dsts[0][(y2+halfksize)*width + x];
                        buffer[i2 + 1] = dsts[1][(y2+halfksize)*width + x];
                        buffer[i2 + 2] = dsts[2][(y2+halfksize)*width + x];
                    }
                    else
                    {
                        buffer[i2] = dsts[0][(height-1)*width + x];
                        buffer[i2 + 1] = dsts[1][(height-1)*width + x];
                        buffer[i2 + 2] = dsts[2][(height-1)*width + x];
                    }
                    convolve(dsts, kernel, buffer, ksize, y1*width + x, y2*width + x);
                } else {
                    convolve(dsts, kernel, buffer, ksize, y1*width + x, -1);
                }

                buffer[i2 - 4] = buffer[i2];
                buffer[i2 - 3] = buffer[i2 + 1];
                buffer[i2 - 2] = buffer[i2 + 2];
            }
        }

        free(buffer);
    }

    // clean up
    free(kernel);
}

int main(int argc, char ** argv)
{
    unsigned char info[54], *dataR = NULL, *dataG = NULL, *dataB = NULL;
    int blur_size, ret_code = 0, size, width, height, offset, row_padded;
    char *in_filename, *out_filename;
    float* dstB, *dstR, *dstG, sigma;


    if (argc != 5)
    {
        printf ("Usage: %s <filename.bmp> <sigma> <blur_size> <output_filename.bmp>", argv[0]);
        return -1;
    }
    in_filename = argv[1];
    out_filename = argv[4];
    blur_size = atoi (argv[3]);
    sigma = atof (argv[2]);
    ret_code = read_BMP(in_filename, info, &dataR, &dataG, &dataB, &size, &width, &height, &offset, &row_padded);
    if (ret_code < 0)
    {
        free (dataB);
        free (dataR);
        free (dataG);
        return -1;
    }

    unsigned char* srcs[] = {dataB, dataG, dataR};

    dstB = (float*)malloc (width*height* sizeof(float));
    dstR = (float*)malloc (width*height* sizeof(float));
    dstG = (float*)malloc (width*height* sizeof(float));

    float* dsts[] = {dstB, dstG, dstR};

    gaussian_blur(srcs, dsts, width, height, sigma, blur_size);

    ret_code = write_BMP (out_filename, dstB, dstG, dstR, info, offset, width, row_padded, height);


    free (dstB);
    free (dstR);
    free (dstG);
    free (dataB);
    free (dataR);
    free (dataG);

    return ret_code;
}
